/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Ad/AdServer/AdServerImpl.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <map>

#include <El/Exception.hpp>
#include <El/MySQL/DB.hpp>

#include <ace/OS.h>
#include <ace/High_Res_Timer.h>

#include "AdServerImpl.hpp"
#include "AdServerMain.hpp"
#include "DB_Record.hpp"

namespace NewsGate
{
  namespace Ad
  {
    namespace Aspect
    {
      const char STATE[] = "State";
    }

    const char CONDITION_SELECT[] =
      "select AdCondition.id as id, rnd_mod, rnd_mod_from, "
      "rnd_mod_to, group_freq_cap, group_count_cap, "
      "query_types, query_type_exclusions, "
      "page_sources, page_source_exclusions, "
      "message_sources, message_source_exclusions, "
      "page_categories, page_category_exclusions, "
      "message_categories, message_category_exclusions, "
      "search_engines, search_engine_exclusions, crawlers, "
      "crawler_exclusions, languages, language_exclusions, "
      "countries, country_exclusions, ip_masks, ip_mask_exclusions, "
      "tags, tag_exclusions, referers, referer_exclusions, "
      "content_languages, content_language_exclusions";
    
    //
    // AdServerImpl class
    //
    AdServerImpl::AdServerImpl(El::Service::Callback* callback)
      throw(Exception, El::Exception)
        : El::Service::CompoundService<>(
            callback,
            "AdServerImpl",
            1),
          selector_cache_(Application::instance()->config().cache_file() +
                          ".sel")
    {
//      ACE_OS::sleep(20);
      
      if(Application::will_trace(El::Logging::LOW))
      {
        Application::logger()->info(
          "NewsGate::Ad::AdServerImpl::AdServerImpl: "
          "starting",
          Aspect::STATE);
      }

      meters_[PI_DESK_PAPER].dump_header("DESK_PAPER");
      meters_[PI_DESK_NLINE].dump_header("DESK_NLINE");
      meters_[PI_DESK_COLUMN].dump_header("DESK_COLUMN");
      meters_[PI_TAB_PAPER].dump_header("TAB_PAPER");
      meters_[PI_TAB_NLINE].dump_header("TAB_NLINE");
      meters_[PI_TAB_COLUMN].dump_header("TAB_COLUMN");
      meters_[PI_MOB_NLINE].dump_header("MOB_NLINE");
      meters_[PI_MOB_COLUMN].dump_header("MOB_COLUMN");
      meters_[PI_DESK_MESSAGE].dump_header("PI_DESK_MESSAGE");
      meters_[PI_TAB_MESSAGE].dump_header("PI_TAB_MESSAGE");
      meters_[PI_MOB_MESSAGE].dump_header("PI_MOB_MESSAGE");

      std::fstream file;
      file.open(selector_cache_.c_str(), ios::in);

      if(file.is_open())
      {
        El::BinaryInStream bstr(file);
        std::auto_ptr<Selector> selector(new Selector());
        
        try
        {
          bstr >> *selector;
          selector_.reset(selector.release());
          creatives_.reset(selector_creatives(*selector_.get()));
        }
        catch(const El::Exception&)
        {
        }

        file.close();
        unlink(selector_cache_.c_str());  
      }

      El::Service::CompoundServiceMessage_var msg = new CheckForUpdate(this);
      deliver_now(msg.in());      
    }

    AdServerImpl::~AdServerImpl() throw()
    {
      // Check if state is active, then deactivate and log error
    }

    ::CORBA::Boolean
    AdServerImpl::selector_changed(
      ::CORBA::ULong interface_version,
      ::CORBA::ULongLong update_number)
      throw(::NewsGate::Ad::IncompatibleVersion,
            NewsGate::Ad::ImplementationException,
            CORBA::SystemException)
    {
      if(interface_version != ::NewsGate::Ad::AdServer::INTERFACE_VERSION)
      {
        IncompatibleVersion ex;
        ex.current_version = ::NewsGate::Ad::AdServer::INTERFACE_VERSION;
        throw ex;
      }

      ReadGuard guard(srv_lock_);
      return update_number != (selector_.get() ? selector_->update_number : 0);
    }
      
    ::NewsGate::Ad::Transport::Selector*
    AdServerImpl::get_selector(
      ::CORBA::ULong interface_version)
      throw(::NewsGate::Ad::IncompatibleVersion,
            NewsGate::Ad::ImplementationException,
            CORBA::SystemException)
    {
      if(interface_version != ::NewsGate::Ad::AdServer::INTERFACE_VERSION)
      {
        IncompatibleVersion ex;
        ex.current_version = ::NewsGate::Ad::AdServer::INTERFACE_VERSION;
        throw ex;
      }

      std::ostringstream ostr;
      SelectorPtr selector(new Selector());
      bool written = false;
        
      {
        El::BinaryOutStream bstr(ostr);
        
        ReadGuard guard(srv_lock_);

        if(selector_.get())
        {
          bstr << *selector_;
          written = true;
        }
      }

      if(written)
      {
        std::istringstream istr(ostr.str());
        El::BinaryInStream bstr(istr);
        bstr >> *selector;
      }

      return Transport::SelectorImpl::Init::create(selector.release());
    }
    
    ::NewsGate::Ad::Transport::SelectionResult*
    AdServerImpl::select(::CORBA::ULong interface_version,
                          ::NewsGate::Ad::Transport::SelectionContext* ctx)
      throw(::NewsGate::Ad::IncompatibleVersion,
            NewsGate::Ad::ImplementationException,
            CORBA::SystemException)
    {
      if(interface_version != ::NewsGate::Ad::AdServer::INTERFACE_VERSION)
      {
        IncompatibleVersion ex;
        ex.current_version = ::NewsGate::Ad::AdServer::INTERFACE_VERSION;
        throw ex;
      }

      Transport::SelectionContextImpl::Type* context =
          dynamic_cast<Transport::SelectionContextImpl::Type*>(ctx);
        
      if(context == 0)
      {
        ImplementationException ex;
        
        ex.description = "NewsGate::Ad::AdServerImpl::select:"
          " dynamic_cast<Transport::SelectionContextImpl::"
          "Type*> failed";

        throw ex;
      }

      SelectionContext& ct = context->entity();
      El::Stat::TimeMeter& meter = meters_[ct.page];
      
      El::Stat::TimeMeasurement measurement(meter);
      std::auto_ptr<SelectionResult> result(new SelectionResult());

      try
      {
        ReadGuard guard(srv_lock_);
        
        if(selector_.get())
        {
          selector_->select(ct, *result);
        }
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Ad::AdServerImpl::select: El::Exception caught. "
          "Description:\n" << ostr.str();

        ImplementationException ex;
        ex.description = ostr.str().c_str();
        throw ex;
      }

      if(Application::will_trace(El::Logging::HIGH))
      {
        measurement.stop();

        if(meter.meterings() >= 10000)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Ad::AdServerImpl::select: ";
            
          meter.dump(ostr);
          meter.reset();
            
          Application::logger()->trace(ostr.str(),
                                       Aspect::STATE,
                                       El::Logging::HIGH);
        }
      }      
      
      return Transport::SelectionResultImpl::Init::create(result.release());
    }
    
    ::NewsGate::Ad::Transport::Selection*
    AdServerImpl::selection(::CORBA::ULong interface_version,
                            ::CORBA::ULongLong id)
      throw(::NewsGate::Ad::NotFound,
            ::NewsGate::Ad::IncompatibleVersion,
            ::NewsGate::Ad::ImplementationException,
            CORBA::SystemException)
    {
      if(interface_version != ::NewsGate::Ad::AdServer::INTERFACE_VERSION)
      {
        IncompatibleVersion ex;
        ex.current_version = ::NewsGate::Ad::AdServer::INTERFACE_VERSION;
        throw ex;
      }

      std::auto_ptr<Selection> result(new Selection());

      {
        ReadGuard guard(srv_lock_);

        if(creatives_.get())
        {
          CreativePtrMap::const_iterator i = creatives_->find(id);

          if(i == creatives_->end())
          {
            throw NotFound();
          }

          const Creative& cr = *(i->second);
          
          result->id = cr.id;
          result->width = cr.width;
          result->height = cr.height;
          result->text = cr.text;
          result->inject = cr.inject;
        }
      }
      
      return Transport::SelectionImpl::Init::create(result.release());
    }

    AdServerImpl::CreativePtrMap*
    AdServerImpl::selector_creatives(const Selector& selector)
      throw(El::Exception)
    {
      CreativePtrMapPtr creatives(new CreativePtrMap());
      
      for(PageMap::const_iterator i(selector.pages.begin()),
            e(selector.pages.end()); i != e; ++i)
      {
        for(SlotMap::const_iterator j(i->second.slots.begin()),
              je(i->second.slots.end()); j != je; ++j)
        {
          for(CreativeArray::const_iterator i(j->second.creatives.begin()),
                e(j->second.creatives.end()); i != e; ++i)
          {
            const Creative& cr = *i;
            creatives->insert(std::make_pair(cr.id, &cr));
          }
        }
      }

      return creatives.release();
    }
    
    bool
    AdServerImpl::notify(El::Service::Event* event) throw(El::Exception)
    {
      if(El::Service::CompoundService<>::notify(event))
      {
        return true;
      }
      
      CheckForUpdate* cfu = dynamic_cast<CheckForUpdate*>(event);
      
      if(cfu != 0)
      {
        check_for_update(cfu);
        return true;
      }

      return false;
    }
    
    void
    AdServerImpl::wait() throw(Exception, El::Exception)
    {
      El::Service::CompoundService<>::wait();

      WriteGuard guard(srv_lock_);
      
      if(selector_cache_.empty())
      {
        return;
      }

      if(selector_.get())
      {
        std::fstream file;
        El::BinaryOutStream bstr(file);

        file.open(selector_cache_.c_str(), ios::out);

        if(file.is_open())
        {
          bstr << *selector_;
        }
        else
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Ad::AdServerImpl::wait: failed to open '"
               << selector_cache_ << "' for write access";
        
          Application::logger()->alert(ostr.str().c_str(), Aspect::STATE);
        }
      }
      else
      {
        unlink(selector_cache_.c_str());
      }
      
      selector_cache_.clear();
    }

    void
    AdServerImpl::check_for_update(CheckForUpdate* cfu) throw(El::Exception)
    {
      size_t delay_factor = 1;
      
      try
      {
        if(Application::instance()->external_ad_server().empty())
        {
          check_db_update();
        }
        else
        {
          if(!check_external_update())
          {
            delay_factor = 10;
          }
        }
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Ad::AdServerImpl::check_for_update: "
          "El::Exception caught. Description:\n" << e.what();

        Application::logger()->alert(ostr.str().c_str(), Aspect::STATE);

        delay_factor = 10;
      }
      
      deliver_at_time(
        cfu,
        ACE_Time_Value(ACE_OS::gettimeofday().sec() +
                       Application::instance()->config().check_period() *
                       delay_factor));
    }

    bool
    AdServerImpl::check_external_update() throw(Exception, El::Exception)
    {
      try
      {      
        Ad::AdServer_var ad_server =
          Application::instance()->external_ad_server().object();

        Ad::SelectorUpdateNumber update_number = 0;
        
        {
          ReadGuard guard(srv_lock_);
        
          if(selector_.get())
          {
            update_number = selector_->update_number;
          }
        }

        if(!ad_server->selector_changed(Ad::AdServer::INTERFACE_VERSION,
                                         update_number))
        {
          return true;
        }

        Transport::Selector_var sel =
          ad_server->get_selector(Ad::AdServer::INTERFACE_VERSION);

        Ad::Transport::SelectorImpl::Type* sel_transport =
          dynamic_cast<Ad::Transport::SelectorImpl::Type*>(sel.in());

        if(sel_transport == 0)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Ad::AdServerImpl::check_external_update: "
            "dynamic_cast<Ad::Transport::SelectorImpl::Type*>(sel.in()) "
            "failed";
            
          throw Exception(ostr.str());
        }

        SelectorPtr selector(sel_transport->release());
        CreativePtrMapPtr creatives(selector_creatives(*selector.get()));

        if(Application::will_trace(El::Logging::HIGH))
        { 
          std::ostringstream ostr;
          ostr << "::NewsGate::Ad::AdServerImpl::check_external_update: "
            "update " << selector->update_number
               << " applied:\n";

          selector->dump(ostr);
            
          Application::logger()->trace(ostr.str(),
                                       Aspect::STATE,
                                       El::Logging::HIGH);
        }

        {
          WriteGuard guard(srv_lock_);
          selector_.reset(selector.release());
          creatives_.reset(creatives.release());
        }
      }
      catch(const Ad::IncompatibleVersion& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Ad::AdServerImpl::check_external_update: "
          "Ad::IncompatibleVersion caught.\nExternal ad server varsion is "
             << e.current_version << ", current one is "
             << Ad::AdServer::INTERFACE_VERSION;

        Application::logger()->trace(ostr.str(),
                                     Aspect::STATE,
                                     El::Logging::HIGH);
        
        return false;
      }
      catch(const Ad::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Ad::AdServerImpl::check_external_update: "
          "Ad::ImplementationException caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Ad::AdServerImpl::check_external_update: "
          "CORBA::Exception caught. Description:\n"
             << e;
        
        throw Exception(ostr.str());
      }

      return true;
    }
    
    void
    AdServerImpl::check_db_update() throw(El::Exception)
    {
      Ad::SelectorUpdateNumber update_number = 0;
        
      {
        ReadGuard guard(srv_lock_);
        
        if(selector_.get())
        {
          update_number = selector_->update_number;
        }
      }
      
      El::MySQL::Connection_var connection =
        Application::instance()->dbase()->connect();

      El::MySQL::Result_var result = connection->query("begin");

      try
      {
        //
        // "for update" clause is used as a lock to ensure data consistency
        //
        result = connection->query(
          "select status, update_num, pcws_weight_zones, "
          "pcws_reduction_rate from AdSelector for update");

        DB::AdSelector sel(result.in());
            
        if(!sel.fetch_row())
        {
          throw Exception(
            "::NewsGate::Ad::AdServerImpl::check_db_update: "
            "failed to get ads update AdSelector");
        }

        unsigned long long update_num = sel.update_num();

        if(update_num != update_number)
        {
          SelectorPtr selector(
            new Selector(
              Application::instance()->config().group_cap_timeout(),
              Application::instance()->config().group_cap_max_count(),
              Application::instance()->config().counter_cap_timeout(),
              Application::instance()->config().counter_cap_max_count(),
              update_num));          

          if(std::string(sel.status()) == "E")
          {
            if(sel.pcws_weight_zones() > 0)
            {
              selector->creative_weight_strategy.reset(
                new Ad::Selector::ProbabilisticCreativeWeightStrategy(
                  sel.pcws_reduction_rate(), sel.pcws_weight_zones()));
            }
            
            result = connection->query(
              "select id, max_ad_num from AdPage where status='E'");

            DB::Page page(result.in());
            
            while(page.fetch_row())
            {
              PageMap::iterator i = selector->pages.find(page.id());

              if(i == selector->pages.end())
              {
                std::ostringstream ostr;
                ostr << "::NewsGate::Ad::AdServerImpl::check_db_update: "
                     << "uexpected page id " << page.id();

                throw Exception(ostr.str());
              }

              i->second.max_ad_num = page.max_ad_num();            
            }

            for(PageMap::iterator i(selector->pages.begin()),
                  e(selector->pages.end()); i != e; ++i)
            {
              load_page(connection.in(), i->first, i->second, *selector);
            }
          }
          
          if(Application::will_trace(El::Logging::HIGH))
          {
            std::ostringstream ostr;
            ostr << "::NewsGate::Ad::AdServerImpl::check_db_update: update "
                 << update_num << " applied\n*** Basic ***\n";

            selector->dump(ostr);
            
            selector->finalize();

            ostr << "\n*** Finalized ***\n";
            selector->dump(ostr);
            
            Application::logger()->trace(ostr.str(),
                                         Aspect::STATE,
                                         El::Logging::HIGH);
          }
          else
          {
            selector->finalize();
          }

          CreativePtrMapPtr creatives(selector_creatives(*selector.get()));
          
          {
            WriteGuard guard(srv_lock_);
            selector_.reset(selector.release());
            creatives_.reset(creatives.release());
          }
        }
        
        result = connection->query("commit");
      }
      catch(...)
      {
        result = connection->query("rollback");
        throw;
      }
    }

    void
    AdServerImpl::load_page(El::MySQL::Connection* connection,
                             PageId id,
                             Page& page,
                             Selector& selector)
      throw(Exception, El::Exception)
    {
      El::MySQL::Result_var result;
      
      {
        std::ostringstream ostr;
        ostr << "select AdPageAdvRestriction.advertiser as advertiser, "
          "least(AdPageAdvRestriction.max_ad_num,Advertiser.max_ads_per_page) "
          "as max_ad_num from AdPageAdvRestriction "
          "join Advertiser on Advertiser.id=AdPageAdvRestriction.advertiser "
          "where Advertiser.status='E' and page=" << id;

        result = connection->query(ostr.str().c_str());

        DB::PageAdvRestriction par(result.in());
            
        while(par.fetch_row())
        {
          page.adv_restrictions[par.advertiser()] =
            AdvRestriction(par.max_ad_num());
        }
      }
      
      {
        std::ostringstream ostr;
        ostr << "select advertiser, advertiser2, max_ad_num from "
          "AdPageAdvAdvRestriction join Advertiser on "
          "Advertiser.id=AdPageAdvAdvRestriction.advertiser "
          "where Advertiser.status='E' and page=" << id;

        result = connection->query(ostr.str().c_str());

        DB::AdPageAdvAdvRestriction par(result.in());
            
        while(par.fetch_row())
        {
          page.add_max_ad_num(par.advertiser(),
                              par.advertiser2(),
                              par.max_ad_num());
        }
      }
      
      {
        std::ostringstream ostr;
        ostr << "select AdPlacement.id as id, AdGroup.id as group_id, "
          "AdGroup.cap_min_time as group_cap_min_time, "
          "AdPlacement.slot as slot, "
          "AdSize.width as width, AdSize.height as height, "
          "AdPlacement.cpm * AdGroup.auction_factor as weight, inject, "
          "Ad.advertiser as advertiser, Ad.text as text from AdPlacement "
          "join AdSlot on AdPlacement.slot=AdSlot.id "
          "join Ad on AdPlacement.ad=Ad.id "
          "join AdSize on Ad.size=AdSize.id "
          "join AdGroup on AdPlacement.group_id=AdGroup.id "
          "join AdCampaign on AdGroup.campaign=AdCampaign.id "
          "join Advertiser on Ad.advertiser=Advertiser.id "
          "where AdPlacement.status='E' and AdSlot.status='E' and "
          "Ad.status='E' and AdSize.status='E' and AdGroup.status='E' and "
          "AdCampaign.status='E' and Advertiser.status='E' and "
          "AdSize.width >= AdSlot.min_width and "
          "AdSize.width <= AdSlot.max_width and "
          "AdSize.height >= AdSlot.min_height and "
          "AdSize.height <= AdSlot.max_height and "
          "AdSlot.page=" << id;
        
        result = connection->query(ostr.str().c_str());

//        std::cerr << ostr.str() << std::endl;

        DB::Placement placement(result.in());
            
        while(placement.fetch_row())
        {
          SlotId slot_id = placement.slot();

          SlotMap::iterator i = page.slots.find(slot_id);
          
          if(i == page.slots.end())
          {
            std::ostringstream ostr;
            ostr << "::NewsGate::Ad::AdServerImpl::load_page: "
                 << "uexpected slot id " << slot_id << "; page " << id << ":";
            
            page.dump(ostr);
            
            throw Exception(ostr.str());
          }

          std::string inject = placement.inject();
          
          i->second.creatives.push_back(
            Creative(placement.id(),
                     placement.group_id(),
                     placement.group_cap_min_time(),
                     placement.width(),
                     placement.height(),
                     placement.weight(),
                     placement.advertiser(),
                     placement.text().c_str(),
                     inject == "D" ? CI_DIRECT : CI_FRAME));
        }
      }

      for(SlotMap::iterator i(page.slots.begin()), e(page.slots.end());
          i != e; ++i)
      {
        CreativeArray& creatives = i->second.creatives;
        
        for(CreativeArray::iterator i(creatives.begin()), e(creatives.end());
            i != e; ++i)
        {
          {
            result =
              query_placement_group_conditions(i->id,
                                               connection,
                                               "AdPlacement");
            
            DB::Condition row(result.in());
            
            while(row.fetch_row())
            {
              add_condition(row, *i, selector);
            }
          }
          
          {
            result =
              query_placement_campaign_conditions(i->id,
                                                  connection,
                                                  "AdPlacement");
            
            DB::Condition row(result.in());
            
            while(row.fetch_row())
            {
              add_condition(row, *i, selector);
            }
          }
        }
      }

      {
        std::ostringstream ostr;
        ostr << "select AdCounterPlacement.id as id, AdGroup.id as group_id, "
          "AdGroup.cap_min_time as group_cap_min_time, "
          "AdCounterPlacement.advertiser as advertiser, "
          "AdCounter.text as text from AdCounterPlacement "
          "left join AdPage on AdCounterPlacement.page=AdPage.id "
          "left join AdCounter on AdCounterPlacement.counter=AdCounter.id "
          "left join AdGroup on AdCounterPlacement.group_id=AdGroup.id "
          "left join AdCampaign on AdGroup.campaign=AdCampaign.id "
          "left join Advertiser on AdCounter.advertiser=Advertiser.id "
          "where AdCounterPlacement.status='E' and AdPage.status='E' and "
          "AdCounter.status='E' and AdGroup.status='E' and "
          "AdCampaign.status='E' and Advertiser.status='E' and "
          "AdCounterPlacement.page=" << id;
        
        result = connection->query(ostr.str().c_str());

//        std::cerr << ostr.str() << std::endl;

        DB::CounterPlacement placement(result.in());
            
        while(placement.fetch_row())
        {
          page.counters.push_back(
            Counter(placement.id(),
                    placement.group_id(),
                    placement.group_cap_min_time(),
                    placement.advertiser(),
                    placement.text().c_str())); 
        }
      }

      for(Ad::CounterArray::iterator i(page.counters.begin()),
            e(page.counters.end()); i != e; ++i)
      {
        {
          result =
            query_placement_group_conditions(i->id,
                                             connection,
                                             "AdCounterPlacement");
            
          DB::Condition row(result.in());
            
          while(row.fetch_row())
          {
            add_condition(row, *i, selector);
          }
        }
          
        {
          result =
            query_placement_campaign_conditions(i->id,
                                                connection,
                                                "AdCounterPlacement");
            
          DB::Condition row(result.in());
            
          while(row.fetch_row())
          {
            add_condition(row, *i, selector);
          }
        }
      }
    }

    El::MySQL::Result*
    AdServerImpl::query_placement_group_conditions(
      uint64_t id,
      El::MySQL::Connection* connection,
      const char* table_name)
      throw(El::Exception)
    {
      std::ostringstream ostr;
      ostr << CONDITION_SELECT
           << " from " << table_name << " join AdGroupCondition on "
           << table_name << ".group_id=AdGroupCondition.group_id "
        "join AdCondition on "
        "AdGroupCondition.condition_id=AdCondition.id "
        "where AdCondition.status='E' and " << table_name << ".id=" << id;
      
      return connection->query(ostr.str().c_str());
    }
    
    El::MySQL::Result*
    AdServerImpl::query_placement_campaign_conditions(
      uint64_t id,
      El::MySQL::Connection* connection,
      const char* table_name)
      throw(El::Exception)
    {
      std::ostringstream ostr;
      ostr << CONDITION_SELECT
           << " from " << table_name << " join AdGroup on "
           << table_name << ".group_id=AdGroup.id "
        "join AdCampaign on AdCampaign.id=AdGroup.campaign "
        "join AdCampaignCondition on "
        "AdCampaignCondition.campaign_id=AdCampaign.id "
        "join AdCondition on "
        "AdCampaignCondition.condition_id=AdCondition.id "
        "where AdCondition.status='E' and " << table_name << ".id=" << id;
      
      return connection->query(ostr.str().c_str());
    }
    
    void
    AdServerImpl::row_to_condition(DB::Condition& row, Condition& condition)
      throw(El::Exception)
    {
      condition.id = row.id();
      condition.rnd_mod = row.rnd_mod();
      condition.rnd_mod_from = row.rnd_mod_from();
      condition.rnd_mod_to = row.rnd_mod_to();
      condition.group_freq_cap = row.group_freq_cap();
      condition.group_count_cap = row.group_count_cap();
      condition.query_types = row.query_types();
      condition.query_type_exclusions = row.query_type_exclusions();
        
      text_to_url_array(row.page_sources().c_str(), condition.page_sources);

      text_to_url_array(row.page_source_exclusions().c_str(),
                        condition.page_source_exclusions);

      text_to_url_array(row.message_sources().c_str(),
                        condition.message_sources);

      text_to_url_array(row.message_source_exclusions().c_str(),
                        condition.message_source_exclusions);

      text_to_string_array(row.page_categories().c_str(),
                           condition.page_categories);

      text_to_string_array(row.page_category_exclusions().c_str(),
                           condition.page_category_exclusions);

      text_to_string_array(row.message_categories().c_str(),
                           condition.message_categories);

      text_to_string_array(row.message_category_exclusions().c_str(),
                           condition.message_category_exclusions);

      text_to_string_array(row.tags().c_str(), condition.tags);
      
      text_to_string_array(row.tag_exclusions().c_str(),
                           condition.tag_exclusions);

      text_to_url_array(row.referers().c_str(), condition.referers);
      
      text_to_url_array(row.referer_exclusions().c_str(),
                        condition.referer_exclusions);

      text_to_set(row.search_engines().c_str(),
                  condition.search_engines);

      text_to_set(row.search_engine_exclusions().c_str(),
                  condition.search_engine_exclusions);
            
      text_to_set(row.crawlers().c_str(), condition.crawlers);

      text_to_set(row.crawler_exclusions().c_str(),
                  condition.crawler_exclusions);

      text_to_lang_set(row.languages().c_str(), condition.languages);

      text_to_lang_set(row.language_exclusions().c_str(),
                       condition.language_exclusions);

      text_to_lang_array(row.content_languages().c_str(),
                         condition.content_languages);

      text_to_lang_array(row.content_language_exclusions().c_str(),
                         condition.content_language_exclusions);

      text_to_country_set(row.countries().c_str(), condition.countries);

      text_to_country_set(row.country_exclusions().c_str(),
                          condition.country_exclusions);

      text_to_ipmask_array(row.ip_masks().c_str(), condition.ip_masks);

      text_to_ipmask_array(row.ip_mask_exclusions().c_str(),
                           condition.ip_mask_exclusions);
    }
            
    void
    AdServerImpl::add_condition(DB::Condition& row,
                                Creative& creative,
                                Selector& selector)
      throw(El::Exception)
    {
      Condition condition;
      row_to_condition(row, condition);
      
      creative.add_condition(
        &selector.conditions.insert(
          std::make_pair(condition.id, condition)).first->second);      
    }

    void
    AdServerImpl::add_condition(DB::Condition& row,
                                Counter& counter,
                                Selector& selector)
      throw(El::Exception)
    {
      Condition condition;
      row_to_condition(row, condition);
      
      counter.add_condition(
        &selector.conditions.insert(
          std::make_pair(condition.id, condition)).first->second);      
    }

    void
    AdServerImpl::text_to_string_array(const char* text,
                                       Ad::StringArray& array)
      throw(El::Exception)
    {
      std::istringstream istr(text);
      std::string line;
      
      while(std::getline(istr, line))
      {
        std::string trimed;
        El::String::Manip::trim(line.c_str(), trimed);

        if(!trimed.empty())
        {
          Condition::add_string(trimed.c_str(), array);
        }
      }
    }

    void
    AdServerImpl::text_to_url_array(const char* text, Ad::StringArray& array)
      throw(El::Exception)
    {
      std::istringstream istr(text);
      std::string line;
      
      while(std::getline(istr, line))
      {
        std::string trimed;
        El::String::Manip::trim(line.c_str(), trimed);

        if(!trimed.empty())
        {
          Condition::add_url(trimed.c_str(), array);
        }
      }
    }

    void
    AdServerImpl::text_to_ipmask_array(const char* text,
                                       std::vector<El::Net::IpMask>& array)
      throw(El::Exception)
    { 
      std::istringstream istr(text);
      std::string line;
      
      while(std::getline(istr, line))
      {
        std::string trimed;
        El::String::Manip::trim(line.c_str(), trimed);

        if(!trimed.empty())
        {
          try
          {
            array.push_back(El::Net::IpMask(trimed.c_str()));
          }
          catch(...)
          {
          }
        }
      }
    }

    void
    AdServerImpl::text_to_set(const char* text, StringSet& set)
      throw(El::Exception)
    {
      std::istringstream istr(text);
      std::string line;
      
      while(std::getline(istr, line))
      {
        std::string trimed;
        El::String::Manip::trim(line.c_str(), trimed);

        if(!trimed.empty())
        {
          set.insert(trimed);
        }
      }
    }

    void
    AdServerImpl::text_to_lang_set(const char* text, Ad::LangSet& set)
      throw(El::Exception)
    {
      std::istringstream istr(text);
      std::string line;
      
      while(std::getline(istr, line))
      {
        std::string trimed;
        El::String::Manip::trim(line.c_str(), trimed);

        if(!trimed.empty())
        {
          try
          {
            set.insert(El::Lang(trimed.c_str()));
          }
          catch(...)
          {
          }
        }
      }      
    }

    void
    AdServerImpl::text_to_lang_array(const char* text, Ad::LangArray& array)
      throw(El::Exception)
    {
      std::istringstream istr(text);
      std::string line;
      
      while(std::getline(istr, line))
      {
        std::string trimed;
        El::String::Manip::trim(line.c_str(), trimed);

        if(!trimed.empty())
        {
          if(trimed == Condition::ANY)
          {
            array.push_back(Condition::ANY_LANG);
          }
          else if(trimed == Condition::NONE)
          {
            array.push_back(Condition::NONE_LANG);
          }
          else
          { 
            try
            {
              array.push_back(El::Lang(trimed.c_str()));
            }
            catch(...)
            {
            }
          }
        }
      }      
    }

    void
    AdServerImpl::text_to_country_set(const char* text, Ad::CountrySet& set)
      throw(El::Exception)
    {
      std::istringstream istr(text);
      std::string line;
      
      while(std::getline(istr, line))
      {
        std::string trimed;
        El::String::Manip::trim(line.c_str(), trimed);

        if(!trimed.empty())
        {
          try
          {
            set.insert(El::Country(trimed.c_str()));
          }
          catch(...)
          {
          }
        }
      }      
    }
  }
}
