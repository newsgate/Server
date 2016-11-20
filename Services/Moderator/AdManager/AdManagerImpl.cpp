/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/AdManager/AdManagerImpl.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <string>
#include <sstream>

#include <md5.h>
#include <ace/OS.h>
#include <mysql/mysqld_error.h>

#include <El/Exception.hpp>
#include <El/Guid.hpp>
#include <El/String/Manip.hpp>

#include "AdManagerImpl.hpp"
#include "AdManagerMain.hpp"
#include "DB_Record.hpp"

namespace Aspect
{
  const char STATE[] = "State";
}

namespace NewsGate
{
  namespace Moderation
  {
    namespace Ad
    {
      //
      // AdManagerImpl class
      //
      AdManagerImpl::AdManagerImpl(El::Service::Callback* callback)
        throw(InvalidArgument, Exception, El::Exception)
          : El::Service::CompoundService<>(callback, "AdManagerImpl")
      {
      }

      AdManagerImpl::~AdManagerImpl() throw()
      {
        // Check if state is active, then deactivate and log error
      }

      bool
      AdManagerImpl::notify(El::Service::Event* event)
        throw(El::Exception)
      {
        if(El::Service::CompoundService<>::notify(event))
        {
          return true;
        }

        return true;
      }

      ::NewsGate::Moderation::Ad::PageSeq*
      AdManagerImpl::get_pages(
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id)
        throw(NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        PageSeq_var pages = new PageSeq();
        
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
        
          try
          {
            std::ostringstream ostr;
            ostr << "select id, name, status, "
              "AdPage.max_ad_num as max_ad_num, "
              "AdPageAdvRestriction.max_ad_num as advertiser_max_ad_num "
              "from AdPage left join AdPageAdvRestriction on "
              "AdPage.id=AdPageAdvRestriction.page and "
              "AdPageAdvRestriction.advertiser=" << advertiser_id
                 << " order by name";
            
            result = connection->query(ostr.str().c_str());
            
            {
              DB::Page row(result.in());
            
              while(row.fetch_row())
              {
                size_t len = pages->length();
                pages->length(len + 1);

                Page& page = (*pages)[len];
                page.id = row.id();
                page.name = row.name().c_str();
                page.max_ad_num = row.max_ad_num();

                page.advertiser_info.id = advertiser_id;
                  
                page.advertiser_info.max_ad_num =
                  row.advertiser_max_ad_num().is_null() ? MAX_AD_NUM :
                  row.advertiser_max_ad_num();
            
                page.status = std::string(row.status()) == "E" ?
                  PS_ENABLED : PS_DISABLED;
              }
            }
          
            for(size_t i = 0; i < pages->length(); ++i)
            {
              Page& page = (*pages)[i];
              load_slots(page.id, connection.in(), page.slots);
              
              load_adv_restrictions(page.id,
                                    advertiser_id,
                                    connection.in(),
                                    page.advertiser_info.adv_max_ad_nums);
            }
        
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::get_pages: "
            "El::Exception caught. Description:\n" << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return pages._retn();
      }
    
      ::NewsGate::Moderation::Ad::Page*
      AdManagerImpl::get_page(
        ::NewsGate::Moderation::Ad::PageId id,
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id)
        throw(NewsGate::Moderation::ObjectNotFound,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        Page_var page;
          
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            page = load_page(id, advertiser_id, connection.in());
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::get_page: "
            "El::Exception caught. Description:\n" << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return page._retn();
      }

      ::NewsGate::Moderation::Ad::Page*
      AdManagerImpl::update_page(
        const ::NewsGate::Moderation::Ad::PageUpdate& update)
        throw(NewsGate::Moderation::ObjectNotFound,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        Page_var page;
          
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            result = connection->query(
              "select update_num from AdSelector for update");

            if(update.flags & PU_AD_MANAGEMENT_INFO)
            {
              std::ostringstream ostr;
              ostr << "update AdPage set status='"
                   << (update.status == PS_ENABLED ? 'E' : 'D')
                   << "', max_ad_num=" << update.max_ad_num << " where id="
                   << update.id;

              result = connection->query(ostr.str().c_str());

              for(size_t i = 0; i < update.slot_updates.length(); ++i)
              {
                const SlotUpdate& su = update.slot_updates[i];
              
                std::ostringstream ostr;
                ostr << "update AdSlot set status='"
                     << (su.status == SS_ENABLED ? 'E' : 'D')
                     << "' where id=" << su.id << " and page=" << update.id;

                result = connection->query(ostr.str().c_str());
              }
            }

            const PageAdvertiserInfoUpdate& u = update.advertiser_info_update;
                
            if(update.flags & PU_ADVERTISER_INFO)
            {
              std::ostringstream ostr;

              if(u.max_ad_num < MAX_AD_NUM)
              { 
                ostr << "insert into AdPageAdvRestriction set advertiser="
                     << u.id << ", page=" << update.id
                     << ", max_ad_num=" << u.max_ad_num
                     << " on duplicate key update max_ad_num=" << u.max_ad_num;
              }
              else
              {
                ostr << "delete from AdPageAdvRestriction where advertiser="
                     << u.id << " and page=" << update.id;
              }

              result = connection->query(ostr.str().c_str());

              {
                std::ostringstream ostr;
                ostr << "delete from AdPageAdvAdvRestriction where advertiser="
                     << u.id << " and page=" << update.id;
              
                result = connection->query(ostr.str().c_str());
              }

              if(u.adv_max_ad_nums.length())
              {
                std::ostringstream ostr;
                ostr << "insert into AdPageAdvAdvRestriction "
                  "(advertiser, page, advertiser2, max_ad_num) values";

                for(size_t i = 0; i < u.adv_max_ad_nums.length(); ++i)
                {
                  const AdvertiserMaxAdNum& val = u.adv_max_ad_nums[i];
                  
                  ostr << (i ? ", " : " ") << "(" <<  u.id << "," << update.id
                       << "," << val.id << "," << val.max_ad_num << ")";
                }

                result = connection->query(ostr.str().c_str());
              }
              
            }
            
            result = connection->query(
              "update AdSelector set update_num=update_num+1");
            
            page = load_page(update.id, u.id, connection.in());
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::update_page: "
            "El::Exception caught. Description:\n" << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return page._retn();
      }

      void
      AdManagerImpl::load_adv_restrictions(
        PageId page_id,
        AdvertiserId advertiser_id,
        El::MySQL::Connection* connection,
        AdvertiserMaxAdNumSeq& adv_max_ad_nums)
        throw(El::Exception)
      {
        std::ostringstream ostr;
        ostr << "select advertiser2, Advertiser.name as advertiser2_name, "
          "max_ad_num from AdPageAdvAdvRestriction join Advertiser on "
          "AdPageAdvAdvRestriction.advertiser2=Advertiser.id "
          "where page=" << page_id << " and advertiser=" << advertiser_id;
            
        El::MySQL::Result_var result = connection->query(ostr.str().c_str());
        DB::PageAdvAdvRestriction row(result.in());
        
        while(row.fetch_row())
        {
          size_t len = adv_max_ad_nums.length();
          adv_max_ad_nums.length(len + 1);

          AdvertiserMaxAdNum& adv_max_ad_num = adv_max_ad_nums[len];
          adv_max_ad_num.id = row.advertiser2();
          adv_max_ad_num.name = row.advertiser2_name().c_str();
          adv_max_ad_num.max_ad_num = row.max_ad_num();
        }
      }
      
      void
      AdManagerImpl::load_slots(PageId page_id,
                                El::MySQL::Connection* connection,
                                SlotSeq& slots)
        throw(El::Exception)
      {
        std::ostringstream ostr;
        ostr << "select id, name, status, min_width, max_width, "
          "min_height, max_height from AdSlot where page=" << page_id
             << " order by name";
            
        El::MySQL::Result_var result = connection->query(ostr.str().c_str());
        DB::Slot row(result.in());
            
        while(row.fetch_row())
        {
          size_t len = slots.length();
          slots.length(len + 1);

          Slot& slot = slots[len];
          slot.id = row.id();
          slot.name = row.name().c_str();
              
          slot.status = std::string(row.status()) == "E" ?
            SS_ENABLED : SS_DISABLED;

          slot.min_width = row.min_width();
          slot.max_width = row.max_width();
          slot.min_height = row.min_height();
          slot.max_height = row.max_height();
        }
      }

      Page*
      AdManagerImpl::load_page(PageId id,
                               AdvertiserId advertiser_id,
                               El::MySQL::Connection* connection)
        throw(NewsGate::Moderation::ObjectNotFound, El::Exception)
      {
        Page_var page = new Page();
        
        std::ostringstream ostr;
        ostr << "select id, name, status, AdPage.max_ad_num as max_ad_num, "
          "AdPageAdvRestriction.max_ad_num as advertiser_max_ad_num "
          "from AdPage left join AdPageAdvRestriction on "
          "AdPage.id=AdPageAdvRestriction.page and "
          "AdPageAdvRestriction.advertiser=" << advertiser_id
             << " where AdPage.id=" << id;

        El::MySQL::Result_var result = connection->query(ostr.str().c_str());
        DB::Page row(result.in());
            
        if(!row.fetch_row())
        {
          std::ostringstream ostr;
          ostr << "Can't find page with id " << id;

          NewsGate::Moderation::ObjectNotFound ex;
          ex.reason = ostr.str().c_str();
          throw ex;
        }
              
        page->id = row.id();
        page->name = row.name().c_str();
        page->max_ad_num = row.max_ad_num();

        page->advertiser_info.id = advertiser_id;
          
        page->advertiser_info.max_ad_num =
          row.advertiser_max_ad_num().is_null() ? MAX_AD_NUM :
          row.advertiser_max_ad_num();
              
        page->status = std::string(row.status()) == "E" ?
          PS_ENABLED : PS_DISABLED;

        load_slots(id, connection, page->slots);

        load_adv_restrictions(id,
                              advertiser_id,
                              connection,
                              page->advertiser_info.adv_max_ad_nums);
        
        return page._retn();
      }

      ::NewsGate::Moderation::Ad::Global*
      AdManagerImpl::get_global(
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id)
        throw(NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        Global_var global;
          
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            global = load_global(advertiser_id, connection.in());
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::get_global: "
            "El::Exception caught. Description:\n" << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return global._retn();
      }

      ::NewsGate::Moderation::Ad::Global*
      AdManagerImpl::update_global(
        const ::NewsGate::Moderation::Ad::GlobalUpdate& update)
        throw(NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        Global_var global;
          
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            result = connection->query(
              "select update_num from AdSelector for update");

            if(update.flags & PU_AD_MANAGEMENT_INFO)
            {
              std::ostringstream ostr;
              ostr << "update AdSelector set status='"
                   << (update.selector_status == RS_ENABLED ? 'E' : 'D')
                   << "', pcws_weight_zones=" << update.pcws_weight_zones
                   << ", pcws_reduction_rate=" << update.pcws_reduction_rate;

              result = connection->query(ostr.str().c_str());
            }            

            if(update.flags & PU_ADVERTISER_INFO)
            {
              std::ostringstream ostr;
              ostr << "update Advertiser set max_ads_per_page="
                   << update.adv_max_ads_per_page << " where id="
                   << update.advertiser;

              result = connection->query(ostr.str().c_str());
            }
            
            result = connection->query(
              "update AdSelector set update_num=update_num+1");
            
            global = load_global(update.advertiser, connection.in());
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::update_global:"
            " El::Exception caught. Description:\n" << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return global._retn();
      }

      Global*
      AdManagerImpl::load_global(
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
        El::MySQL::Connection* connection)
        throw(NewsGate::Moderation::ImplementationException,
              El::Exception)
      {
        Global_var global = new Global();

        std::ostringstream ostr;
        ostr << "select AdSelector.status as selector_status, "
          "Advertiser.max_ads_per_page as adv_max_ads_per_page, "
          "update_num, pcws_weight_zones, pcws_reduction_rate from "
          "AdSelector join Advertiser where Advertiser.id="
             << advertiser_id;
        
        El::MySQL::Result_var result = connection->query(ostr.str().c_str());
        DB::Global row(result.in());
            
        if(!row.fetch_row())
        {
          NewsGate::Moderation::ImplementationException ex;
          
          ex.description =
            "::NewsGate::Moderation::Ad::AdManagerImpl::load_global:"
            "can't fetch data";
          
          throw ex;
        }
              
        global->selector_status = std::string(row.selector_status()) == "E" ?
          RS_ENABLED : RS_DISABLED;

        global->adv_max_ads_per_page = row.adv_max_ads_per_page();
        global->update_number = row.update_num();
        global->pcws_weight_zones = row.pcws_weight_zones();
        global->pcws_reduction_rate = row.pcws_reduction_rate();

        return global._retn();
      }

      ::NewsGate::Moderation::Ad::SizeSeq*
      AdManagerImpl::get_sizes()
        throw(NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        SizeSeq_var sizes;
          
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            sizes = load_sizes(connection.in());
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::get_sizes: "
            "El::Exception caught. Description:\n" << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return sizes._retn();
      }
        
      ::NewsGate::Moderation::Ad::SizeSeq*
      AdManagerImpl::update_sizes(
        const ::NewsGate::Moderation::Ad::SizeUpdateSeq& updates)
        throw(NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        SizeSeq_var sizes;
          
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            result = connection->query(
              "select update_num from AdSelector for update");            
            
            for(size_t i = 0; i < updates.length(); ++i)
            {
              std::ostringstream ostr;
              ostr << "update AdSize set status='"
                 << (updates[i].status == ZS_ENABLED ? 'E' : 'D')
                 << "' where id=" << updates[i].id;
              
              result = connection->query(ostr.str().c_str());
            }

            result = connection->query(
              "update AdSelector set update_num=update_num+1");
            
            sizes = load_sizes(connection.in());
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::update_sizes: "
            "El::Exception caught. Description:\n" << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return sizes._retn();
      }

      SizeSeq*
      AdManagerImpl::load_sizes(El::MySQL::Connection* connection)
        throw(El::Exception)
      {
        SizeSeq_var sizes = new SizeSeq();
        
        std::ostringstream ostr;
        ostr << "select id, name, status, width, height from AdSize order "
          "by width desc";
            
        El::MySQL::Result_var result = connection->query(ostr.str().c_str());
        DB::Size row(result.in());
            
        while(row.fetch_row())
        {
          size_t len = sizes->length();
          sizes->length(len + 1);

          Size& size = (*sizes)[len];
          size.id = row.id();
          size.name = row.name().c_str();
              
          size.status = std::string(row.status()) == "E" ?
            ZS_ENABLED : ZS_DISABLED;

          size.width = row.width();
          size.height = row.height();
        }

        return sizes._retn();
      }

      ::NewsGate::Moderation::Ad::AdvertSeq*
      AdManagerImpl::get_ads(
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id)
        throw(NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        AdvertSeq_var ads;
          
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            ads = load_ads(advertiser_id, connection.in());
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::get_ads: "
            "El::Exception caught. Description:\n" << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return ads._retn();
      }
      
      AdvertSeq*
      AdManagerImpl::load_ads(
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
        El::MySQL::Connection* connection)
        throw(El::Exception)
      {
        AdvertSeq_var ads = new AdvertSeq();
        
        std::ostringstream ostr;
        ostr << "select Ad.id as id, Ad.name as name, Ad.status as status, "
          "Ad.size as size, AdSize.name as size_name, width, height, "
          "text from Ad join AdSize on Ad.size=AdSize.id where advertiser="
             << advertiser_id << " order by name";
            
        El::MySQL::Result_var result = connection->query(ostr.str().c_str());
        DB::Ad row(result.in());
            
        while(row.fetch_row())
        {
          size_t len = ads->length();
          ads->length(len + 1);

          Advert& ad = (*ads)[len];
          ad.id = row.id();
          ad.advertiser = advertiser_id;
          ad.name = row.name().c_str();
          ad.size_name = row.size_name().c_str();

          std::string status(row.status());
          
          ad.status = status == "E" ?
            AS_ENABLED : (status == "D" ? AS_DISABLED : AS_DELETED);
          
          ad.size = row.size();
          ad.width = row.width();
          ad.height = row.height();
          ad.text = row.text().c_str();
        }

        for(size_t i = 0; i < ads->length(); ++i)
        {
          load_ad_placements((*ads)[i], advertiser_id, connection);
        }

        return ads._retn();
      }

      Advert*
      AdManagerImpl::load_ad(
        ::NewsGate::Moderation::Ad::AdvertId id,
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
        El::MySQL::Connection* connection)
        throw(NewsGate::Moderation::ObjectNotFound, El::Exception)
      {
        Advert_var ad = new Advert();
        
        std::ostringstream ostr;
        ostr << "select Ad.id as id, Ad.name as name, Ad.status as status, "
          "Ad.size as size, AdSize.name as size_name, width, height, "
          "text from Ad join AdSize on Ad.size=AdSize.id where advertiser="
             << advertiser_id << " and Ad.id=" << id;
            
        El::MySQL::Result_var result = connection->query(ostr.str().c_str());
        DB::Ad row(result.in());
            
        if(!row.fetch_row())
        {
          std::ostringstream ostr;
          ostr << "Can't find ad with id " << id << " for advertiser "
               << advertiser_id;

          NewsGate::Moderation::ObjectNotFound ex;
          ex.reason = ostr.str().c_str();
          throw ex;
        }
        
        ad->id = row.id();
        ad->advertiser = advertiser_id;
        ad->name = row.name().c_str();
        ad->size_name = row.size_name().c_str();
              
        std::string status(row.status());
          
        ad->status = status == "E" ?
          AS_ENABLED : (status == "D" ? AS_DISABLED : AS_DELETED);
        
        ad->size = row.size();
        ad->width = row.width();
        ad->height = row.height();
        ad->text = row.text().c_str();

        load_ad_placements(*ad, advertiser_id, connection);

        return ad._retn();
      }

      void
      AdManagerImpl::load_ad_placements(
        ::NewsGate::Moderation::Ad::Advert& ad,
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
        El::MySQL::Connection* connection)
        throw(El::Exception)
      {
        El::MySQL::Result_var result;
        
        {
          std::ostringstream ostr;
          ostr << "select id from AdPlacement where advertiser="
               << advertiser_id << " and ad=" << ad.id;
          
          result = connection->query(ostr.str().c_str());

          DB::Placement row(result.in(), 1);

          while(row.fetch_row())
          {
            size_t len = ad.placements.length();
            ad.placements.length(len + 1);

            ad.placements[len].id = row.id();
          }
        }

        for(size_t i = 0; i < ad.placements.length(); ++i)
        {
          Placement& plc = ad.placements[i];
          load_placement(plc, plc.id, advertiser_id, connection);
        }
      }
                         
      ::NewsGate::Moderation::Ad::Advert*
      AdManagerImpl::get_ad(
        const ::NewsGate::Moderation::Ad::AdvertId id,
        const ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id)
        throw(NewsGate::Moderation::ObjectNotFound,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        Advert_var ad;
          
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            ad = load_ad(id, advertiser_id, connection.in());
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::get_ad: "
            "El::Exception caught. Description:\n" << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return ad._retn();
      }
        
      ::NewsGate::Moderation::Ad::Advert*
      AdManagerImpl::update_ad(
        const ::NewsGate::Moderation::Ad::AdvertUpdate& update)
        throw(NewsGate::Moderation::ObjectNotFound,
              NewsGate::Moderation::ObjectAlreadyExist,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        Advert_var ad;
          
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            result = connection->query(
              "select update_num from AdSelector for update");

            char status;
            
            switch(update.status)
            {
            case AS_ENABLED: status = 'E'; break;
            case AS_DISABLED: status = 'D'; break;
            default: status = 'L'; break;
            }            
            
            std::ostringstream ostr;
            ostr << "update Ad set status='" << status
                 << "', name='" << connection->escape(update.name.in())
                 << "', size=" << update.size << ", text='"
                 << connection->escape(update.text.in()) << "' where id="
                 << update.id << " and advertiser=" << update.advertiser;

            try
            {
              result = connection->query(ostr.str().c_str());
            }
            catch(const El::Exception& e)
            {
              int code = mysql_errno(connection->mysql());

              if(code == ER_DUP_ENTRY)
              {
                NewsGate::Moderation::ObjectAlreadyExist ex;
                ex.reason = e.what();
                throw ex;
              }
              
              throw;
            }
            
            result = connection->query(
              "update AdSelector set update_num=update_num+1");
            
            ad = load_ad(update.id, update.advertiser, connection.in());
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::update_ad: "
            "El::Exception caught. Description:\n" << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return ad._retn();
      }

      ::NewsGate::Moderation::Ad::Advert*
      AdManagerImpl::create_ad(const ::NewsGate::Moderation::Ad::Advert& ad)
        throw(NewsGate::Moderation::ObjectAlreadyExist,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        Advert_var new_ad;
          
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            result = connection->query(
              "select update_num from AdSelector for update");

            char status;
            
            switch(ad.status)
            {
            case AS_ENABLED: status = 'E'; break;
            case AS_DISABLED: status = 'D'; break;
            default: status = 'L'; break;
            }
            
            std::ostringstream ostr;
            ostr << "insert into Ad set advertiser=" << ad.advertiser
                 << ", status='" << status
                 << "', name='" << connection->escape(ad.name.in())
                 << "', size=" << ad.size << ", text='"
                 << connection->escape(ad.text.in()) << "'";

            try
            {
              result = connection->query(ostr.str().c_str());
            }
            catch(const El::Exception& e)
            {
              int code = mysql_errno(connection->mysql());

              if(code == ER_DUP_ENTRY)
              {
                NewsGate::Moderation::ObjectAlreadyExist ex;
                ex.reason = e.what();
                throw ex;
              }
              
              throw;
            }

            AdvertId id = connection->insert_id();
            
            result = connection->query(
              "update AdSelector set update_num=update_num+1");

            try
            {
              new_ad = load_ad(id, ad.advertiser, connection.in());
            }
            catch(const NewsGate::Moderation::ObjectNotFound& e)
            {
              std::ostringstream ostr;
              ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::create_ad: "
                "ObjectNotFound caught. Description:\n" << e.reason.in();

              NewsGate::Moderation::ImplementationException ex;
              ex.description = ostr.str().c_str();
              throw ex;
            }
            
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::create_ad: "
            "El::Exception caught. Description:\n" << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return new_ad._retn();        
      }

      ::NewsGate::Moderation::Ad::CounterSeq*
      AdManagerImpl::get_counters(
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id)
        throw(NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        CounterSeq_var counters;
          
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            counters = load_counters(advertiser_id, connection.in());
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::get_counters: "
            "El::Exception caught. Description:\n" << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return counters._retn();
      }
      
      CounterSeq*
      AdManagerImpl::load_counters(
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
        El::MySQL::Connection* connection)
        throw(El::Exception)
      {
        CounterSeq_var counters = new CounterSeq();
        
        std::ostringstream ostr;
        ostr << "select id, name, status, text from AdCounter "
          "where advertiser=" << advertiser_id << " order by name";
            
        El::MySQL::Result_var result = connection->query(ostr.str().c_str());
        DB::Counter row(result.in());
            
        while(row.fetch_row())
        {
          size_t len = counters->length();
          counters->length(len + 1);

          Counter& cntr = (*counters)[len];
          cntr.id = row.id();
          cntr.advertiser = advertiser_id;
          cntr.name = row.name().c_str();

          std::string status(row.status());
          
          cntr.status = status == "E" ?
            US_ENABLED : (status == "D" ? US_DISABLED : US_DELETED);
          
          cntr.text = row.text().c_str();
        }

        for(size_t i = 0; i < counters->length(); ++i)
        {
          load_counter_placements((*counters)[i], advertiser_id, connection);
        }

        return counters._retn();
      }

      Counter*
      AdManagerImpl::load_counter(
        ::NewsGate::Moderation::Ad::CounterId id,
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
        El::MySQL::Connection* connection)
        throw(NewsGate::Moderation::ObjectNotFound, El::Exception)
      {
        Counter_var counter = new Counter();
        
        std::ostringstream ostr;
        ostr << "select id, name, status, text from AdCounter where "
          "advertiser=" << advertiser_id << " and id=" << id;
            
        El::MySQL::Result_var result = connection->query(ostr.str().c_str());
        DB::Counter row(result.in());
            
        if(!row.fetch_row())
        {
          std::ostringstream ostr;
          ostr << "Can't find counter with id " << id << " for advertiser "
               << advertiser_id;

          NewsGate::Moderation::ObjectNotFound ex;
          ex.reason = ostr.str().c_str();
          throw ex;
        }
        
        counter->id = row.id();
        counter->advertiser = advertiser_id;
        counter->name = row.name().c_str();
              
        std::string status(row.status());
          
        counter->status = status == "E" ?
          US_ENABLED : (status == "D" ? US_DISABLED : US_DELETED);
        
        counter->text = row.text().c_str();

        load_counter_placements(*counter, advertiser_id, connection);

        return counter._retn();
      }

      void
      AdManagerImpl::load_counter_placements(
        ::NewsGate::Moderation::Ad::Counter& counter,
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
        El::MySQL::Connection* connection)
        throw(El::Exception)
      {
        El::MySQL::Result_var result;
        
        {
          std::ostringstream ostr;
          ostr << "select id from AdCounterPlacement where advertiser="
               << advertiser_id << " and counter=" << counter.id;
          
          result = connection->query(ostr.str().c_str());

          DB::Placement row(result.in(), 1);

          while(row.fetch_row())
          {
            size_t len = counter.placements.length();
            counter.placements.length(len + 1);

            counter.placements[len].id = row.id();
          }
        }

        for(size_t i = 0; i < counter.placements.length(); ++i)
        {
          CounterPlacement& plc = counter.placements[i];
          load_counter_placement(plc, plc.id, advertiser_id, connection);
        }
      }
                         
      ::NewsGate::Moderation::Ad::Counter*
      AdManagerImpl::get_counter(
        const ::NewsGate::Moderation::Ad::CounterId id,
        const ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id)
        throw(NewsGate::Moderation::ObjectNotFound,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        Counter_var counter;
          
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            counter = load_counter(id, advertiser_id, connection.in());
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::get_counter: "
            "El::Exception caught. Description:\n" << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return counter._retn();
      }
        
      ::NewsGate::Moderation::Ad::Counter*
      AdManagerImpl::update_counter(
        const ::NewsGate::Moderation::Ad::CounterUpdate& update)
        throw(NewsGate::Moderation::ObjectNotFound,
              NewsGate::Moderation::ObjectAlreadyExist,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        Counter_var counter;
          
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            result = connection->query(
              "select update_num from AdSelector for update");

            char status;
            
            switch(update.status)
            {
            case US_ENABLED: status = 'E'; break;
            case US_DISABLED: status = 'D'; break;
            default: status = 'L'; break;
            }
            
            std::ostringstream ostr;
            ostr << "update AdCounter set status='" << status
                 << "', name='" << connection->escape(update.name.in())
                 << "', text='" << connection->escape(update.text.in())
                 << "' where id=" << update.id << " and advertiser="
                 << update.advertiser;

            try
            {
              result = connection->query(ostr.str().c_str());
            }
            catch(const El::Exception& e)
            {
              int code = mysql_errno(connection->mysql());

              if(code == ER_DUP_ENTRY)
              {
                NewsGate::Moderation::ObjectAlreadyExist ex;
                ex.reason = e.what();
                throw ex;
              }
              
              throw;
            }
            
            result = connection->query(
              "update AdSelector set update_num=update_num+1");
            
            counter =
              load_counter(update.id, update.advertiser, connection.in());
            
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::update_counter: "
            "El::Exception caught. Description:\n" << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return counter._retn();
      }

      ::NewsGate::Moderation::Ad::Counter*
      AdManagerImpl::create_counter(
        const ::NewsGate::Moderation::Ad::Counter& counter)
        throw(NewsGate::Moderation::ObjectAlreadyExist,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        Counter_var new_counter;
          
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            result = connection->query(
              "select update_num from AdSelector for update");

            char status;
            
            switch(counter.status)
            {
            case US_ENABLED: status = 'E'; break;
            case US_DISABLED: status = 'D'; break;
            default: status = 'L'; break;
            }
            
            std::ostringstream ostr;
            ostr << "insert into AdCounter set advertiser="
                 << counter.advertiser << ", status='" << status
                 << "', name='" << connection->escape(counter.name.in())
                 << "', text='" << connection->escape(counter.text.in())
                 << "'";

            try
            {
              result = connection->query(ostr.str().c_str());
            }
            catch(const El::Exception& e)
            {
              int code = mysql_errno(connection->mysql());

              if(code == ER_DUP_ENTRY)
              {
                NewsGate::Moderation::ObjectAlreadyExist ex;
                ex.reason = e.what();
                throw ex;
              }
              
              throw;
            }

            CounterId id = connection->insert_id();
            
            result = connection->query(
              "update AdSelector set update_num=update_num+1");

            try
            {
              new_counter =
                load_counter(id, counter.advertiser, connection.in());
            }
            catch(const NewsGate::Moderation::ObjectNotFound& e)
            {
              std::ostringstream ostr;
              ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::"
                "create_counter: ObjectNotFound caught. Description:\n"
                   << e.reason.in();

              NewsGate::Moderation::ImplementationException ex;
              ex.description = ostr.str().c_str();
              throw ex;
            }
            
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::create_counter: "
            "El::Exception caught. Description:\n" << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return new_counter._retn();
      }

      ::NewsGate::Moderation::Ad::ConditionSeq*
      AdManagerImpl::get_conditions(
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id)
        throw(NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        ConditionSeq_var conditions;
          
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            conditions = load_conditions(advertiser_id, connection.in());
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::get_conditions: "
            "El::Exception caught. Description:\n" << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return conditions._retn();
      }

      ConditionSeq*
      AdManagerImpl::load_conditions(
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
        El::MySQL::Connection* connection)
        throw(El::Exception)
      {
        ConditionSeq_var conditions = new ConditionSeq();
        
        std::ostringstream ostr;
        ostr << "select id, name, status, "
          "rnd_mod, rnd_mod_from, rnd_mod_to, group_freq_cap, "
          "group_count_cap, query_types, query_type_exclusions, "
          "page_sources, page_source_exclusions, "
          "message_sources, message_source_exclusions, "
          "page_categories, page_category_exclusions, message_categories, "
          "message_category_exclusions, search_engines, "
          "search_engine_exclusions, crawlers, crawler_exclusions, "
          "languages, language_exclusions, countries, country_exclusions, "
          "ip_masks, ip_mask_exclusions, "
          "tags, tag_exclusions, referers, referer_exclusions, "
          "content_languages, content_language_exclusions "
          "from AdCondition where advertiser="
             << advertiser_id << " order by name";
            
        El::MySQL::Result_var result = connection->query(ostr.str().c_str());
        DB::Condition row(result.in());
            
        while(row.fetch_row())
        {
          size_t len = conditions->length();
          conditions->length(len + 1);

          Condition& cond = (*conditions)[len];
          cond.id = row.id();
          cond.advertiser = advertiser_id;
          cond.name = row.name().c_str();

          std::string status(row.status());
          
          cond.status = status == "E" ?
            CS_ENABLED : (status == "D" ? CS_DISABLED : CS_DELETED);
          
          cond.rnd_mod = row.rnd_mod();
          cond.rnd_mod_from = row.rnd_mod_from();
          cond.rnd_mod_to = row.rnd_mod_to();
          cond.group_freq_cap = row.group_freq_cap();
          cond.group_count_cap = row.group_count_cap();
          cond.query_types = row.query_types();
          cond.query_type_exclusions = row.query_type_exclusions();

          cond.page_sources = row.page_sources().c_str();
          
          cond.page_source_exclusions =
            row.page_source_exclusions().c_str();
          
          cond.message_sources = row.message_sources().c_str();
          
          cond.message_source_exclusions =
            row.message_source_exclusions().c_str();
          
          cond.page_categories = row.page_categories().c_str();
          
          cond.page_category_exclusions =
            row.page_category_exclusions().c_str();
          
          cond.message_categories = row.message_categories().c_str();
          
          cond.message_category_exclusions =
            row.message_category_exclusions().c_str();
          
          cond.search_engines = row.search_engines().c_str();
          
          cond.search_engine_exclusions =
            row.search_engine_exclusions().c_str();
          
          cond.crawlers = row.crawlers().c_str();
          cond.crawler_exclusions = row.crawler_exclusions().c_str();
          
          cond.languages = row.languages().c_str();
          cond.language_exclusions = row.language_exclusions().c_str();
          
          cond.countries = row.countries().c_str();
          cond.country_exclusions = row.country_exclusions().c_str();
          
          cond.ip_masks = row.ip_masks().c_str();
          cond.ip_mask_exclusions = row.ip_mask_exclusions().c_str();

          cond.tags = row.tags().c_str();
          cond.tag_exclusions = row.tag_exclusions().c_str();

          cond.referers = row.referers().c_str();
          cond.referer_exclusions = row.referer_exclusions().c_str();

          cond.content_languages = row.content_languages().c_str();
          
          cond.content_language_exclusions =
            row.content_language_exclusions().c_str();
        }

        return conditions._retn();
      }
      
      Condition*
      AdManagerImpl::load_condition(
        ::NewsGate::Moderation::Ad::ConditionId id,
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
        El::MySQL::Connection* connection)
        throw(NewsGate::Moderation::ObjectNotFound, El::Exception)
      {
        Condition_var cond = new Condition();
        
        std::ostringstream ostr;
        ostr << "select id, name, status, "
          "rnd_mod, rnd_mod_from, rnd_mod_to, group_freq_cap, "
          "group_count_cap, query_types, query_type_exclusions, "
          "page_sources, page_source_exclusions, "
          "message_sources, message_source_exclusions, "
          "page_categories, page_category_exclusions, message_categories, "
          "message_category_exclusions, search_engines, "
          "search_engine_exclusions, crawlers, crawler_exclusions, "
          "languages, language_exclusions, countries, country_exclusions, "
          "ip_masks, ip_mask_exclusions, "
          "tags, tag_exclusions, referers, referer_exclusions, "
          "content_languages, content_language_exclusions "
          "from AdCondition where advertiser="
             << advertiser_id << " and id=" << id;
            
        El::MySQL::Result_var result = connection->query(ostr.str().c_str());
        DB::Condition row(result.in());
            
        if(!row.fetch_row())
        {
          std::ostringstream ostr;
          ostr << "Can't find condition with id " << id << " for advertiser "
               << advertiser_id;

          NewsGate::Moderation::ObjectNotFound ex;
          ex.reason = ostr.str().c_str();
          throw ex;
        }
        
        cond->id = row.id();
        cond->advertiser = advertiser_id;
        cond->name = row.name().c_str();

        std::string status(row.status());
          
        cond->status = status == "E" ?
          CS_ENABLED : (status == "D" ? CS_DISABLED : CS_DELETED);
          
        cond->rnd_mod = row.rnd_mod();
        cond->rnd_mod_from = row.rnd_mod_from();
        cond->rnd_mod_to = row.rnd_mod_to();
        cond->group_freq_cap = row.group_freq_cap();
        cond->group_count_cap = row.group_count_cap();
        cond->query_types = row.query_types();
        cond->query_type_exclusions = row.query_type_exclusions();

        cond->page_sources = row.page_sources().c_str();
          
        cond->page_source_exclusions =
          row.page_source_exclusions().c_str();
          
        cond->message_sources = row.message_sources().c_str();
          
        cond->message_source_exclusions =
          row.message_source_exclusions().c_str();
          
        cond->page_categories = row.page_categories().c_str();
          
        cond->page_category_exclusions =
          row.page_category_exclusions().c_str();
          
        cond->message_categories = row.message_categories().c_str();
          
        cond->message_category_exclusions =
          row.message_category_exclusions().c_str();
          
        cond->search_engines = row.search_engines().c_str();
          
        cond->search_engine_exclusions =
          row.search_engine_exclusions().c_str();
          
        cond->crawlers = row.crawlers().c_str();
        cond->crawler_exclusions = row.crawler_exclusions().c_str();

        cond->languages = row.languages().c_str();
        cond->language_exclusions = row.language_exclusions().c_str();

        cond->countries = row.countries().c_str();
        cond->country_exclusions = row.country_exclusions().c_str();

        cond->ip_masks = row.ip_masks().c_str();
        cond->ip_mask_exclusions = row.ip_mask_exclusions().c_str();

        cond->tags = row.tags().c_str();
        cond->tag_exclusions = row.tag_exclusions().c_str();
        
        cond->referers = row.referers().c_str();
        cond->referer_exclusions = row.referer_exclusions().c_str();

        cond->content_languages = row.content_languages().c_str();
          
        cond->content_language_exclusions =
          row.content_language_exclusions().c_str();
        
        return cond._retn();
      }

      ::NewsGate::Moderation::Ad::Condition*
      AdManagerImpl::get_condition(
        ::NewsGate::Moderation::Ad::ConditionId id,
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id)
        throw(NewsGate::Moderation::ObjectNotFound,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        Condition_var condition;
          
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            condition = load_condition(id, advertiser_id, connection.in());
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::get_condition: "
            "El::Exception caught. Description:\n" << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return condition._retn();
      }
      
      ::NewsGate::Moderation::Ad::Condition*
      AdManagerImpl::update_condition(
        const ::NewsGate::Moderation::Ad::ConditionUpdate& update)
        throw(NewsGate::Moderation::ObjectNotFound,
              NewsGate::Moderation::ObjectAlreadyExist,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        Condition_var cond;
          
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            result = connection->query(
              "select update_num from AdSelector for update");

            char status;
            
            switch(update.status)
            {
            case CS_ENABLED: status = 'E'; break;
            case CS_DISABLED: status = 'D'; break;
            default: status = 'L'; break;
            }            
            
            std::ostringstream ostr;
            ostr << "update AdCondition set status='" << status
                 << "', name='" << connection->escape(update.name.in())
                 << "', rnd_mod=" << update.rnd_mod
                 << ", rnd_mod_from=" << update.rnd_mod_from
                 << ", rnd_mod_to=" << update.rnd_mod_to
                 << ", group_freq_cap=" << update.group_freq_cap
                 << ", group_count_cap=" << update.group_count_cap
                 << ", query_types=" << update.query_types
                 << ", query_type_exclusions=" << update.query_type_exclusions
                 << ", page_sources='"
                 << connection->escape(update.page_sources.in())
                 << "', page_source_exclusions='"
                 << connection->escape(update.page_source_exclusions.in())
                 << "', message_sources='"
                 << connection->escape(update.message_sources.in())
                 << "', message_source_exclusions='"
                 << connection->escape(update.message_source_exclusions.in())
                 << "', page_categories='"
                 << connection->escape(update.page_categories.in())
                 << "', page_category_exclusions='"
                 << connection->escape(update.page_category_exclusions.in())
                 << "', message_categories='"
                 << connection->escape(update.message_categories.in())
                 << "', message_category_exclusions='"
                 << connection->escape(update.message_category_exclusions.in())
                 << "', search_engines='"
                 << connection->escape(update.search_engines.in())
                 << "', search_engine_exclusions='"
                 << connection->escape(update.search_engine_exclusions.in())
                 << "', crawlers='"
                 << connection->escape(update.crawlers.in())
                 << "', crawler_exclusions='"
                 << connection->escape(update.crawler_exclusions.in())    
                 << "', languages='"
                 << connection->escape(update.languages.in())
                 << "', language_exclusions='"
                 << connection->escape(update.language_exclusions.in())
                 << "', countries='"
                 << connection->escape(update.countries.in())
                 << "', country_exclusions='"
                 << connection->escape(update.country_exclusions.in())
                 << "', ip_masks='"
                 << connection->escape(update.ip_masks.in())
                 << "', ip_mask_exclusions='"
                 << connection->escape(update.ip_mask_exclusions.in())
                 << "', tags='"
                 << connection->escape(update.tags.in())
                 << "', tag_exclusions='"
                 << connection->escape(update.tag_exclusions.in())
                 << "', referers='"
                 << connection->escape(update.referers.in())
                 << "', referer_exclusions='"
                 << connection->escape(update.referer_exclusions.in())
                 << "', content_languages='"
                 << connection->escape(update.content_languages.in())
                 << "', content_language_exclusions='"
                 << connection->escape(update.content_language_exclusions.in())
                 << "' where id=" << update.id << " and advertiser="
                 << update.advertiser;

            try
            {
              result = connection->query(ostr.str().c_str());
            }
            catch(const El::Exception& e)
            {
              int code = mysql_errno(connection->mysql());

              if(code == ER_DUP_ENTRY)
              {
                NewsGate::Moderation::ObjectAlreadyExist ex;
                ex.reason = e.what();
                throw ex;
              }
              
              throw;
            }
            
            result = connection->query(
              "update AdSelector set update_num=update_num+1");
            
            cond =
              load_condition(update.id, update.advertiser, connection.in());
            
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::"
            "update_condition: El::Exception caught. Description:\n"
               << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return cond._retn();
      }
        
      ::NewsGate::Moderation::Ad::Condition*
      AdManagerImpl::create_condition(
        const ::NewsGate::Moderation::Ad::Condition& condition)
        throw(NewsGate::Moderation::ObjectAlreadyExist,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        Condition_var new_cond;
          
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            result = connection->query(
              "select update_num from AdSelector for update");

            char status;
            
            switch(condition.status)
            {
            case CS_ENABLED: status = 'E'; break;
            case CS_DISABLED: status = 'D'; break;
            default: status = 'L'; break;
            }
            
            std::ostringstream ostr;
            ostr << "insert into AdCondition set advertiser="
                 << condition.advertiser
                 << ", status='" << status
                 << "', name='" << connection->escape(condition.name.in())
                 << "', rnd_mod=" << condition.rnd_mod
                 << ", rnd_mod_from=" << condition.rnd_mod_from
                 << ", rnd_mod_to=" << condition.rnd_mod_to
                 << ", group_freq_cap=" << condition.group_freq_cap
                 << ", group_count_cap=" << condition.group_count_cap
                 << ", query_types=" << condition.query_types
                 << ", query_type_exclusions="
                 << condition.query_type_exclusions
                 << ", page_sources='"
                 << connection->escape(condition.page_sources.in())
                 << "', page_source_exclusions='"
                 << connection->escape(
                   condition.page_source_exclusions.in())
                 << "', message_sources='"
                 << connection->escape(condition.message_sources.in())
                 << "', message_source_exclusions='"
                 << connection->escape(
                   condition.message_source_exclusions.in())
                 << "', page_categories='"
                 << connection->escape(condition.page_categories.in())
                 << "', page_category_exclusions='"
                 << connection->escape(
                   condition.page_category_exclusions.in())
                 << "', message_categories='"
                 << connection->escape(condition.message_categories.in())
                 << "', message_category_exclusions='"
                 << connection->escape(
                   condition.message_category_exclusions.in())
                 << "', search_engines='"
                 << connection->escape(condition.search_engines.in())
                 << "', search_engine_exclusions='"
                 << connection->escape(condition.search_engine_exclusions.in())
                 << "', crawlers='"
                 << connection->escape(condition.crawlers.in())
                 << "', crawler_exclusions='"
                 << connection->escape(condition.crawler_exclusions.in())    
                 << "', languages='"
                 << connection->escape(condition.languages.in())
                 << "', language_exclusions='"
                 << connection->escape(condition.language_exclusions.in())    
                 << "', countries='"
                 << connection->escape(condition.countries.in())
                 << "', country_exclusions='"
                 << connection->escape(condition.country_exclusions.in())    
                 << "', ip_masks='"
                 << connection->escape(condition.ip_masks.in())
                 << "', ip_mask_exclusions='"
                 << connection->escape(condition.ip_mask_exclusions.in())    
                 << "', tags='"
                 << connection->escape(condition.tags.in())
                 << "', tag_exclusions='"
                 << connection->escape(condition.tag_exclusions.in())
                 << "', referers='"
                 << connection->escape(condition.referers.in())
                 << "', referer_exclusions='"
                 << connection->escape(condition.referer_exclusions.in())
                 << "', content_languages='"
                 << connection->escape(condition.content_languages.in())
                 << "', content_language_exclusions='"
                 << connection->escape(
                   condition.content_language_exclusions.in())
                 << "'";

            try
            {
              result = connection->query(ostr.str().c_str());
            }
            catch(const El::Exception& e)
            {
              int code = mysql_errno(connection->mysql());

              if(code == ER_DUP_ENTRY)
              {
                NewsGate::Moderation::ObjectAlreadyExist ex;
                ex.reason = e.what();
                throw ex;
              }
              
              throw;
            }

            ConditionId id = connection->insert_id();
            
            result = connection->query(
              "update AdSelector set update_num=update_num+1");

            try
            {
              new_cond = load_condition(id,
                                        condition.advertiser,
                                        connection.in());
            }
            catch(const NewsGate::Moderation::ObjectNotFound& e)
            {
              std::ostringstream ostr;
              ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::"
                "create_condition: ObjectNotFound caught. Description:\n"
                   << e.reason.in();

              NewsGate::Moderation::ImplementationException ex;
              ex.description = ostr.str().c_str();
              throw ex;
            }
            
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::"
            "create_condition: El::Exception caught. Description:\n"
               << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return new_cond._retn();        
      }

      ::NewsGate::Moderation::Ad::CampaignSeq*
      AdManagerImpl::get_campaigns(
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id)
        throw(NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        CampaignSeq_var campaigns;
          
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            campaigns = load_campaigns(advertiser_id, connection.in());
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::get_campaigns: "
            "El::Exception caught. Description:\n" << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return campaigns._retn();
      }

      CampaignSeq*
      AdManagerImpl::load_campaigns(
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
        El::MySQL::Connection* connection)
        throw(El::Exception)
      {
        CampaignSeq_var campaigns = new CampaignSeq();
        
        std::ostringstream ostr;
        ostr << "select id, name, status from AdCampaign where advertiser="
             << advertiser_id << " order by name";
            
        El::MySQL::Result_var result = connection->query(ostr.str().c_str());
        DB::Campaign row(result.in());
            
        while(row.fetch_row())
        {
          size_t len = campaigns->length();
          campaigns->length(len + 1);

          Campaign& cmp = (*campaigns)[len];
          cmp.id = row.id();
          cmp.advertiser = advertiser_id;
          cmp.name = row.name().c_str();

          std::string status(row.status());
          
          cmp.status = status == "E" ?
            MS_ENABLED : (status == "D" ? MS_DISABLED : MS_DELETED);          
        }

        for(size_t i = 0; i < campaigns->length(); ++i)
        {
          Campaign& cmp = (*campaigns)[i];
          
          load_campaign_conditions(cmp.conditions,
                                   cmp.id,
                                   advertiser_id,
                                   connection);

          load_groups(cmp.groups, cmp.id, advertiser_id, connection);
        }

        return campaigns._retn();
      }

      void
      AdManagerImpl::load_groups(
        GroupSeq& groups,
        ::NewsGate::Moderation::Ad::CampaignId id,
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
        El::MySQL::Connection* connection)
        throw(El::Exception)
      {        
        std::ostringstream ostr;
        ostr << "select AdGroup.id as id, campaign, "
          "AdCampaign.name as campaign_name, AdGroup.name as name, "
          "AdGroup.status as status, auction_factor from "
          "AdGroup join AdCampaign on AdGroup.campaign=AdCampaign.id "
          "where AdGroup.campaign=" << id << " and AdGroup.advertiser="
             << advertiser_id << " order by name";
            
        El::MySQL::Result_var result = connection->query(ostr.str().c_str());
        DB::Group row(result.in());
            
        while(row.fetch_row())
        {
          size_t len = groups.length();
          groups.length(len + 1);

          Group& grp = groups[len];
          grp.id = row.id();
          grp.campaign = id;
          grp.advertiser = advertiser_id;
          grp.name = row.name().c_str();
          grp.campaign_name = row.campaign_name().c_str();

          std::string status(row.status());
          
          grp.status = status == "E" ?
            GS_ENABLED : (status == "D" ? GS_DISABLED : GS_DELETED);

          grp.auction_factor = row.auction_factor();
        }

        for(size_t i = 0; i < groups.length(); ++i)
        {
          Group& grp = groups[i];
          
          load_group_conditions(grp.conditions,
                                grp.id,
                                advertiser_id,
                                connection);

          load_group_placements(grp.ad_placements,
                                grp.id,
                                advertiser_id,
                                connection);          

          load_group_counter_placements(grp.counter_placements,
                                        grp.id,
                                        advertiser_id,
                                        connection);      
        }
      }
      
      ::NewsGate::Moderation::Ad::Campaign*
      AdManagerImpl::get_campaign(
        ::NewsGate::Moderation::Ad::CampaignId id,
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id)
        throw(NewsGate::Moderation::ObjectNotFound,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        Campaign_var campaign;
        
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            campaign = load_campaign(id, advertiser_id, connection.in());
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::get_campaign: "
            "El::Exception caught. Description:\n" << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return campaign._retn();
      }
        
      Campaign*
      AdManagerImpl::load_campaign(
        ::NewsGate::Moderation::Ad::CampaignId id,
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
        El::MySQL::Connection* connection)
        throw(NewsGate::Moderation::ObjectNotFound, El::Exception)
      {
        Campaign_var cmp = new Campaign();
        
        std::ostringstream ostr;
        ostr << "select id, name, status from AdCampaign where advertiser="
             << advertiser_id << " and id=" << id;
            
        El::MySQL::Result_var result = connection->query(ostr.str().c_str());
        DB::Campaign row(result.in());
            
        if(!row.fetch_row())
        {
          std::ostringstream ostr;
          ostr << "Can't find campaign with id " << id << " for advertiser "
               << advertiser_id;

          NewsGate::Moderation::ObjectNotFound ex;
          ex.reason = ostr.str().c_str();
          throw ex;
        }
        
        cmp->id = row.id();
        cmp->advertiser = advertiser_id;
        cmp->name = row.name().c_str();

        std::string status(row.status());
          
        cmp->status = status == "E" ?
          MS_ENABLED : (status == "D" ? MS_DISABLED : MS_DELETED);

        load_campaign_conditions(cmp->conditions,
                                 id,
                                 advertiser_id,
                                 connection);
        
        load_groups(cmp->groups, id, advertiser_id, connection);
        
        return cmp._retn();
      }

      ::NewsGate::Moderation::Ad::Campaign*
      AdManagerImpl::update_campaign(
        const ::NewsGate::Moderation::Ad::CampaignUpdate& update)
        throw(NewsGate::Moderation::ObjectNotFound,
              NewsGate::Moderation::ObjectAlreadyExist,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        Campaign_var cmp;
          
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            result = connection->query(
              "select update_num from AdSelector for update");

            char status;
            
            switch(update.status)
            {
            case MS_ENABLED: status = 'E'; break;
            case MS_DISABLED: status = 'D'; break;
            default: status = 'L'; break;
            }            
            
            std::ostringstream ostr;
            ostr << "update AdCampaign set status='" << status
                 << "', name='" << connection->escape(update.name.in())
                 << "' where id=" << update.id << " and advertiser="
                 << update.advertiser;

            try
            {
              result = connection->query(ostr.str().c_str());
            }
            catch(const El::Exception& e)
            {
              int code = mysql_errno(connection->mysql());

              if(code == ER_DUP_ENTRY)
              {
                NewsGate::Moderation::ObjectAlreadyExist ex;
                ex.reason = e.what();
                throw ex;
              }
              
              throw;
            }
            
            {
              std::ostringstream ostr;
              ostr << "delete from AdCampaignCondition where campaign_id="
                   << update.id << " and advertiser="
                   << update.advertiser;
              
              result = connection->query(ostr.str().c_str());
            }

            if(update.conditions.length())
            {
              std::ostringstream ostr;
              ostr << "insert into AdCampaignCondition "
                "(campaign_id, condition_id, advertiser) values";

              for(size_t i = 0; i < update.conditions.length(); ++i)
              {
                ostr << (i ? ", " : " ") << "(" <<  update.id << ","
                     << update.conditions[i] << "," << update.advertiser
                     << ")";
              }

              result = connection->query(ostr.str().c_str());
            }
            
            result = connection->query(
              "update AdSelector set update_num=update_num+1");
            
            cmp = load_campaign(update.id, update.advertiser, connection.in());
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::"
            "update_campaign: El::Exception caught. Description:\n"
               << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return cmp._retn();
      }
        
      ::NewsGate::Moderation::Ad::Campaign*
      AdManagerImpl::create_campaign(
        const ::NewsGate::Moderation::Ad::Campaign& campaign)
        throw(NewsGate::Moderation::ObjectAlreadyExist,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        Campaign_var new_cmp;
          
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            result = connection->query(
              "select update_num from AdSelector for update");

            char status;
            
            switch(campaign.status)
            {
            case MS_ENABLED: status = 'E'; break;
            case MS_DISABLED: status = 'D'; break;
            default: status = 'L'; break;
            }
            
            std::ostringstream ostr;
            ostr << "insert into AdCampaign set advertiser="
                 << campaign.advertiser << ", status='" << status
                 << "', name='" << connection->escape(campaign.name.in())
                 << "'";

            try
            {
              result = connection->query(ostr.str().c_str());
            }
            catch(const El::Exception& e)
            {
              int code = mysql_errno(connection->mysql());

              if(code == ER_DUP_ENTRY)
              {
                NewsGate::Moderation::ObjectAlreadyExist ex;
                ex.reason = e.what();
                throw ex;
              }
              
              throw;
            }

            CampaignId id = connection->insert_id();
            
            if(campaign.conditions.length())
            {
              std::ostringstream ostr;
              ostr << "insert into AdCampaignCondition "
                "(campaign_id, condition_id, advertiser) values";

              for(size_t i = 0; i < campaign.conditions.length(); ++i)
              {
                ostr << (i ? ", " : " ") << "(" <<  id << ","
                     << campaign.conditions[i].id << ","
                     << campaign.advertiser << ")";
              }

              result = connection->query(ostr.str().c_str());
            }            

            result = connection->query(
              "update AdSelector set update_num=update_num+1");

            try
            {
              new_cmp = load_campaign(id,
                                      campaign.advertiser,
                                      connection.in());
            }
            catch(const NewsGate::Moderation::ObjectNotFound& e)
            {
              std::ostringstream ostr;
              ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::"
                "create_campaign: ObjectNotFound caught. Description:\n"
                   << e.reason.in();

              NewsGate::Moderation::ImplementationException ex;
              ex.description = ostr.str().c_str();
              throw ex;
            }
            
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::"
            "create_campaign: El::Exception caught. Description:\n"
               << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return new_cmp._retn();        
      }
        
      Group*
      AdManagerImpl::load_group(
        ::NewsGate::Moderation::Ad::GroupId id,
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
        El::MySQL::Connection* connection)
        throw(NewsGate::Moderation::ObjectNotFound, El::Exception)
      {
        Group_var grp = new Group();
        
        std::ostringstream ostr;

        ostr << "select AdGroup.id as id, campaign, "
          "AdCampaign.name as campaign_name, AdGroup.name as name, "
          "AdGroup.status as status, auction_factor from "
          "AdGroup join AdCampaign on AdGroup.campaign=AdCampaign.id "
          "where AdGroup.advertiser=" << advertiser_id
             << " and AdGroup.id=" << id;
            
        El::MySQL::Result_var result = connection->query(ostr.str().c_str());
        DB::Group row(result.in());
            
        if(!row.fetch_row())
        {
          std::ostringstream ostr;
          ostr << "Can't find group with id " << id << " for advertiser "
               << advertiser_id;

          NewsGate::Moderation::ObjectNotFound ex;
          ex.reason = ostr.str().c_str();
          throw ex;
        }
        
        grp->id = id;
        grp->campaign = row.campaign();
        grp->advertiser = advertiser_id;
        grp->name = row.name().c_str();
        grp->campaign_name = row.campaign_name().c_str();

        std::string status(row.status());
          
        grp->status = status == "E" ?
          GS_ENABLED : (status == "D" ? GS_DISABLED : GS_DELETED);
        
        grp->auction_factor = row.auction_factor();

        load_group_conditions(grp->conditions, id, advertiser_id, connection);
        
        load_group_placements(grp->ad_placements,
                              id,
                              advertiser_id,
                              connection);
        
        load_group_counter_placements(grp->counter_placements,
                                      id,
                                      advertiser_id,
                                      connection);
        
        return grp._retn();
      }

      void
      AdManagerImpl::load_group_conditions(
        ConditionSeq& conditions,
        ::NewsGate::Moderation::Ad::GroupId id,
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
        El::MySQL::Connection* connection)
        throw(El::Exception)
      {
        std::ostringstream ostr;
        ostr << "select AdGroupCondition.condition_id as id, "
          "AdCondition.name as name, AdCondition.status as status "
          "from AdGroupCondition join AdCondition "
          "on AdGroupCondition.condition_id=AdCondition.id where "
          "AdGroupCondition.group_id=" << id << " and "
          "AdGroupCondition.advertiser=" << advertiser_id << " order by name";
            
        El::MySQL::Result_var result = connection->query(ostr.str().c_str());
        DB::GroupCondition row(result.in());
            
        while(row.fetch_row())
        {
          size_t len = conditions.length();
          conditions.length(len + 1);

          Condition& cond = conditions[len];
          cond.id = row.id();
          cond.name = row.name().c_str();

          std::string status(row.status());
          
          cond.status = status == "E" ? CS_ENABLED :
            (status == "D" ? CS_DISABLED : CS_DELETED);
        }
      }

      void
      AdManagerImpl::load_campaign_conditions(
        ConditionSeq& conditions,
        ::NewsGate::Moderation::Ad::CampaignId id,
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
        El::MySQL::Connection* connection)
        throw(El::Exception)
      {
        std::ostringstream ostr;
        ostr << "select AdCampaignCondition.condition_id as id, "
          "AdCondition.name as name, AdCondition.status as status "
          "from AdCampaignCondition join AdCondition "
          "on AdCampaignCondition.condition_id=AdCondition.id where "
          "AdCampaignCondition.campaign_id=" << id << " and "
          "AdCampaignCondition.advertiser=" << advertiser_id
             << " order by name";
            
        El::MySQL::Result_var result = connection->query(ostr.str().c_str());
        DB::GroupCondition row(result.in());
            
        while(row.fetch_row())
        {
          size_t len = conditions.length();
          conditions.length(len + 1);

          Condition& cond = conditions[len];
          cond.id = row.id();
          cond.name = row.name().c_str();

          std::string status(row.status());
          
          cond.status = status == "E" ? CS_ENABLED :
            (status == "D" ? CS_DISABLED : CS_DELETED);
        }
      }

      ::NewsGate::Moderation::Ad::Group*
      AdManagerImpl::get_group(
        ::NewsGate::Moderation::Ad::GroupId id,
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id)
        throw(NewsGate::Moderation::ObjectNotFound,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        Group_var group;
        
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            group = load_group(id, advertiser_id, connection.in());
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::get_group: "
            "El::Exception caught. Description:\n" << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return group._retn();
      }
      
      ::NewsGate::Moderation::Ad::Group*
      AdManagerImpl::update_group(
        const ::NewsGate::Moderation::Ad::GroupUpdate& update)
        throw(NewsGate::Moderation::ObjectNotFound,
              NewsGate::Moderation::ObjectAlreadyExist,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        Group_var grp;
          
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            result = connection->query(
              "select update_num from AdSelector for update");

            char status;
            
            switch(update.status)
            {
            case GS_ENABLED: status = 'E'; break;
            case GS_DISABLED: status = 'D'; break;
            default: status = 'L'; break;
            }            
            
            {
              std::ostringstream ostr;
              ostr << "update AdGroup set status='" << status
                   << "', name='" << connection->escape(update.name.in())
                   << "', auction_factor=" << update.auction_factor;

              if(update.reset_cap_min_time)
              {
                ostr << ", cap_min_time=" << ACE_OS::gettimeofday().sec();
              }
              
              ostr << " where id=" << update.id << " and advertiser="
                   << update.advertiser;

              try
              {
                result = connection->query(ostr.str().c_str());
              }
              catch(const El::Exception& e)
              {
                int code = mysql_errno(connection->mysql());

                if(code == ER_DUP_ENTRY)
                {
                  NewsGate::Moderation::ObjectAlreadyExist ex;
                  ex.reason = e.what();
                  throw ex;
                }
              
                throw;
              }
            }
            
            {
              std::ostringstream ostr;
              ostr << "delete from AdGroupCondition where group_id="
                   << update.id << " and advertiser="
                   << update.advertiser;
              
              result = connection->query(ostr.str().c_str());
            }

            if(update.conditions.length())
            {
              std::ostringstream ostr;
              ostr << "insert into AdGroupCondition "
                "(group_id, condition_id, advertiser) values";

              for(size_t i = 0; i < update.conditions.length(); ++i)
              {
                ostr << (i ? ", " : " ") << "(" <<  update.id << ","
                     << update.conditions[i] << "," << update.advertiser
                     << ")";
              }

              result = connection->query(ostr.str().c_str());
            }
            
            result = connection->query(
              "update AdSelector set update_num=update_num+1");
            
            grp = load_group(update.id, update.advertiser, connection.in());
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::"
            "update_group: El::Exception caught. Description:\n"
               << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return grp._retn();
      }
        
      ::NewsGate::Moderation::Ad::Group*
      AdManagerImpl::create_group(
        const ::NewsGate::Moderation::Ad::Group& grp)
        throw(NewsGate::Moderation::ObjectAlreadyExist,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        Group_var new_grp;
          
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            result = connection->query(
              "select update_num from AdSelector for update");

            char status;
            
            switch(grp.status)
            {
            case GS_ENABLED: status = 'E'; break;
            case GS_DISABLED: status = 'D'; break;
            default: status = 'L'; break;
            }
            
            std::ostringstream ostr;
            ostr << "insert into AdGroup set advertiser="
                 << grp.advertiser << ", campaign=" << grp.campaign
                 << ", status='" << status
                 << "', name='" << connection->escape(grp.name.in())
                 << "', auction_factor=" << grp.auction_factor;

            try
            {
              result = connection->query(ostr.str().c_str());
            }
            catch(const El::Exception& e)
            {
              int code = mysql_errno(connection->mysql());

              if(code == ER_DUP_ENTRY)
              {
                NewsGate::Moderation::ObjectAlreadyExist ex;
                ex.reason = e.what();
                throw ex;
              }
              
              throw;
            }

            GroupId id = connection->insert_id();

            if(grp.conditions.length())
            {
              std::ostringstream ostr;
              ostr << "insert into AdGroupCondition "
                "(group_id, condition_id, advertiser) values";

              for(size_t i = 0; i < grp.conditions.length(); ++i)
              {
                ostr << (i ? ", " : " ") << "(" <<  id << ","
                     << grp.conditions[i].id << "," << grp.advertiser << ")";
              }

              result = connection->query(ostr.str().c_str());
            }            
            
            result = connection->query(
              "update AdSelector set update_num=update_num+1");

            try
            {
              new_grp = load_group(id, grp.advertiser, connection.in());
            }
            catch(const NewsGate::Moderation::ObjectNotFound& e)
            {
              std::ostringstream ostr;
              ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::"
                "create_group: ObjectNotFound caught. Description:\n"
                   << e.reason.in();

              NewsGate::Moderation::ImplementationException ex;
              ex.description = ostr.str().c_str();
              throw ex;
            }
            
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::"
            "create_group: El::Exception caught. Description:\n"
               << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return new_grp._retn();        
      }

      void
      AdManagerImpl::load_counter_placement(
        CounterPlacement& plc,
        ::NewsGate::Moderation::Ad::CounterPlacementId id,
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
        El::MySQL::Connection* connection)
        throw(NewsGate::Moderation::ObjectNotFound,
              El::Exception)
      {        
        std::ostringstream ostr;

        ostr << "select AdCounterPlacement.id as id, group_id, "
          "AdGroup.name as group_name, AdGroup.campaign as campaign, "
          "AdCampaign.name as campaign_name, AdCounterPlacement.name as name, "
          "AdCounterPlacement.status as status, page, "
          "AdPage.name as page_name, AdPage.status as page_status, "
          "counter, AdCounter.name as counter_name, "
          "AdCounter.status as counter_status, "
          "Advertiser.status as adv_status, Advertiser.name as adv_name, "
          "AdSelector.status as selector_status, "
          "AdGroup.status as group_status, "
          "AdCampaign.status as campaign_status "
          "from AdCounterPlacement "
          "join AdGroup on AdCounterPlacement.group_id=AdGroup.id "
          "join AdCampaign on AdGroup.campaign=AdCampaign.id "
          "join AdPage on page=AdPage.id "
          "join AdCounter on AdCounterPlacement.counter=AdCounter.id "
          "join Advertiser on Advertiser.id=AdCounterPlacement.advertiser "
          "join AdSelector "
          "where AdCounterPlacement.advertiser=" << advertiser_id
             << " and AdCounterPlacement.id=" << id;

//        std::cerr << ostr.str() << std::endl;
            
        El::MySQL::Result_var result = connection->query(ostr.str().c_str());
        DB::CounterPlacement row(result.in());
            
        if(!row.fetch_row())
        {
          std::ostringstream ostr;
          ostr << "Can't find counter placement with id " << id
               << " for advertiser " << advertiser_id;

          NewsGate::Moderation::ObjectNotFound ex;
          ex.reason = ostr.str().c_str();
          throw ex;
        }
        
        plc.id = id;
        plc.group = row.group_id();
        plc.group_name = row.group_name().c_str();
        plc.campaign = row.campaign();
        plc.campaign_name = row.campaign_name().c_str();
        plc.advertiser = advertiser_id;
        plc.name = row.name().c_str();
        
        plc.placement_page.id = row.page();
        plc.placement_page.name = row.page_name().c_str();

        std::string page_status(row.page_status());
          
        plc.placement_page.status = page_status == "E" ?
          PS_ENABLED : PS_DISABLED;
        
        plc.placement_counter.id = row.counter();
        plc.placement_counter.name = row.counter_name().c_str();

        std::string counter_status(row.counter_status());
          
        plc.placement_counter.status = counter_status == "E" ?
          US_ENABLED : (counter_status == "D" ? US_DISABLED : US_DELETED);

        std::string status(row.status());
          
        plc.status = status == "E" ?
          OS_ENABLED : (status == "D" ? OS_DISABLED : OS_DELETED);        

        plc.display_status = counter_placement_display_status(row).c_str();
      }

      std::string
      AdManagerImpl::counter_placement_display_status(
        DB::CounterPlacement& row) throw(El::Exception)
      {
        std::ostringstream ostr;
        std::string status(row.status());
          
        if(status != "E")
        {
          ostr << "<li>counter placement "
               << (status == "D" ? "disabled" : "deleted")
               << "</li>";
        }

        status = row.page_status();
        
        if(status != "E")
        {
          ostr << "<li>page <a href=\"/psp/ad/page?id=" << row.page()
               << "\" target=\"_blank\">";

          El::String::Manip::xml_encode(row.page_name().c_str(), ostr);
          ostr << "</a> disabled</li>";
        }

        status = row.counter_status();
        
        if(status != "E")
        {
          ostr << "<li>counter <a href=\"/psp/ad/counter?id=" << row.counter()
               << "\" target=\"_blank\">";

          El::String::Manip::xml_encode(row.counter_name().c_str(), ostr);
          
          ostr << "</a> " << (status == "D" ? "disabled" : "deleted")
               << "</li>";
        }
        
        status = row.group_status();
        
        if(status != "E")
        {
          ostr << "<li>group <a href=\"/psp/ad/group?id=" << row.group_id()
               << "\" target=\"_blank\">";

          El::String::Manip::xml_encode(row.group_name().c_str(), ostr);
          
          ostr << "</a> " << (status == "D" ? "disabled" : "deleted")
               << "</li>";
        }

        status = row.campaign_status();
        
        if(status != "E")
        {
          ostr << "<li>campaign <a href=\"/psp/ad/campaign?id="
               << row.campaign()
               << "\" target=\"_blank\">";

          El::String::Manip::xml_encode(row.campaign_name().c_str(), ostr);
          
          ostr << "</a> " << (status == "D" ? "disabled" : "deleted")
               << "</li>";
        }

        status = row.adv_status();
        
        if(status != "E")
        {
          ostr << "<li>advertiser '";
          El::String::Manip::xml_encode(row.adv_name().c_str(), ostr);
          ostr << "' disabled</li>";
        }

        status = row.selector_status();
        
        if(status != "E")
        {
          ostr << "<li>selector disabled</li>";
        }

        return ostr.str();
      }

      void
      AdManagerImpl::load_placement(
        Placement& plc,
        ::NewsGate::Moderation::Ad::PlacementId id,
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
        El::MySQL::Connection* connection)
        throw(NewsGate::Moderation::ObjectNotFound,
              El::Exception)
      {        
        std::ostringstream ostr;

        ostr << "select AdPlacement.id as id, group_id, "
          "AdGroup.name as group_name, AdGroup.campaign as campaign, "
          "AdCampaign.name as campaign_name, AdPlacement.name as name, "
          "AdPlacement.status as status, slot, AdSlot.name as slot_name, "
          "AdSlot.min_width as slot_min_width, "
          "AdSlot.max_width as slot_max_width, "
          "AdSlot.min_height as slot_min_height, "
          "AdSlot.max_height as slot_max_height, "
          "AdSlot.status as slot_status, ad, Ad.name as ad_name, "
          "Ad.status as ad_status, Ad.size as ad_size, "
          "AdSize.name as ad_size_name, AdSize.width as ad_width, "
          "AdSize.height as ad_height, cpm, inject, auction_factor, "
          "AdPage.max_ad_num as page_max_ad_num, AdPage.id as page_id, "
          "AdPage.name as page_name, "
          "AdPageAdvRestriction.max_ad_num as adv_rst_max_ad_num, "
          "Advertiser.max_ads_per_page as adv_max_ads_per_page, "
          "Advertiser.status as adv_status, Advertiser.name as adv_name, "
          "AdSelector.status as selector_status, "
          "AdGroup.status as group_status, "
          "AdSize.status as size_status, AdPage.status as page_status, "
          "AdCampaign.status as campaign_status "
          "from AdPlacement join AdGroup on AdPlacement.group_id=AdGroup.id "
          "join AdCampaign on AdGroup.campaign=AdCampaign.id "
          "join AdSlot on slot=AdSlot.id join Ad on AdPlacement.ad=Ad.id "
          "join AdSize on Ad.size=AdSize.id "
          "join AdPage on AdPage.id=AdSlot.page "
          "left join AdPageAdvRestriction on "
          "AdPage.id=AdPageAdvRestriction.page "
          "and AdPlacement.advertiser=AdPageAdvRestriction.advertiser "
          "join Advertiser on Advertiser.id=AdPlacement.advertiser "
          "join AdSelector "
          "where AdPlacement.advertiser=" << advertiser_id
             << " and AdPlacement.id=" << id;

//        std::cerr << ostr.str() << std::endl;
            
        El::MySQL::Result_var result = connection->query(ostr.str().c_str());
        DB::Placement row(result.in());
            
        if(!row.fetch_row())
        {
          std::ostringstream ostr;
          ostr << "Can't find placement with id " << id << " for advertiser "
               << advertiser_id;

          NewsGate::Moderation::ObjectNotFound ex;
          ex.reason = ostr.str().c_str();
          throw ex;
        }
        
        plc.id = id;
        plc.group = row.group_id();
        plc.group_name = row.group_name().c_str();
        plc.campaign = row.campaign();
        plc.campaign_name = row.campaign_name().c_str();
        plc.advertiser = advertiser_id;
        plc.name = row.name().c_str();
        
        plc.placement_slot.id = row.slot();
        plc.placement_slot.name = row.slot_name().c_str();
        plc.placement_slot.min_width = row.slot_min_width();
        plc.placement_slot.max_width = row.slot_max_width();
        plc.placement_slot.min_height = row.slot_min_height();
        plc.placement_slot.max_height = row.slot_max_height();

        std::string slot_status(row.slot_status());
          
        plc.placement_slot.status = slot_status == "E" ?
          SS_ENABLED : SS_DISABLED;
        
        plc.ad.id = row.ad();
        plc.ad.name = row.ad_name().c_str();

        std::string ad_status(row.ad_status());
          
        plc.ad.status = ad_status == "E" ?
          AS_ENABLED : (ad_status == "D" ? AS_DISABLED : AS_DELETED);

        plc.ad.size = row.ad_size();
        plc.ad.size_name = row.ad_size_name().c_str();
        plc.ad.width = row.ad_width();
        plc.ad.height = row.ad_height();

        std::string status(row.status());
          
        plc.status = status == "E" ?
          TS_ENABLED : (status == "D" ? TS_DISABLED : TS_DELETED);
        
        plc.cpm = row.cpm();

        std::string inject(row.inject());
        plc.inject = inject == "D" ? PI_DIRECT : PI_FRAME;

        plc.display_status = placement_display_status(row).c_str();
      }

      std::string
      AdManagerImpl::placement_display_status(DB::Placement& row)
        throw(El::Exception)
      {
        std::ostringstream ostr;
        std::string status(row.status());
          
        if(status != "E")
        {
          ostr << "<li>placement " << (status == "D" ? "disabled" : "deleted")
               << "</li>";
        }

        if(row.cpm() == 0)
        {
          ostr << "<li>placement CPM is 0</li>";
        }
        
        status = row.slot_status();
        
        if(status != "E")
        {
          ostr << "<li>slot <a href=\"/psp/ad/page?id=" << row.page_id()
               << "\" target=\"_blank\">";

          El::String::Manip::xml_encode(row.slot_name().c_str(), ostr);
          ostr << "</a> disabled</li>";
        }

        status = row.ad_status();
        
        if(status != "E")
        {
          ostr << "<li>ad <a href=\"/psp/ad/ad?id=" << row.ad()
               << "\" target=\"_blank\">";

          El::String::Manip::xml_encode(row.ad_name().c_str(), ostr);
          
          ostr << "</a> " << (status == "D" ? "disabled" : "deleted")
               << "</li>";
        }
        
        status = row.size_status();
        
        if(status != "E")
        {
          ostr << "<li>size <a href=\"/psp/ad/sizes\" target=\"_blank\">";

          El::String::Manip::xml_encode(row.ad_size_name().c_str(), ostr);
          
          ostr << "</a> disabled</li>";
        }

        if(row.ad_width().value() < row.slot_min_width().value() ||
           row.ad_width().value() > row.slot_max_width().value() ||
           row.ad_height().value() < row.slot_min_height().value() ||
           row.ad_height().value() > row.slot_max_height().value())
        {
          ostr << "<li>ad <a href=\"/psp/ad/ad?id=" << row.ad()
               << "\" target=\"_blank\">";

          El::String::Manip::xml_encode(row.ad_name().c_str(), ostr);
          
          ostr << "</a> size " << row.ad_width() << "x" << row.ad_height()
               << " do not match <a href=\"/psp/ad/page?id=" << row.page_id()
               << "\" target=\"_blank\">";

          El::String::Manip::xml_encode(row.slot_name().c_str(), ostr);
          
          ostr << "</a> slot size ["
               << row.slot_min_width() << "-" << row.slot_max_width() << "x"
               << row.slot_min_height() << "-" << row.slot_max_height()
               << "]</li>";
        }

        status = row.group_status();
        
        if(status != "E")
        {
          ostr << "<li>group <a href=\"/psp/ad/group?id=" << row.group_id()
               << "\" target=\"_blank\">";

          El::String::Manip::xml_encode(row.group_name().c_str(), ostr);
          
          ostr << "</a> " << (status == "D" ? "disabled" : "deleted")
               << "</li>";
        }

        if(row.auction_factor() == 0)
        {
          ostr << "<li>group <a href=\"/psp/ad/group?id=" << row.group_id()
               << "\" target=\"_blank\">";
          
          El::String::Manip::xml_encode(row.group_name().c_str(), ostr);
          
          ostr << "</a> Auction Factor is 0</li>";
        }

        status = row.campaign_status();
        
        if(status != "E")
        {
          ostr << "<li>campaign <a href=\"/psp/ad/campaign?id="
               << row.campaign()
               << "\" target=\"_blank\">";

          El::String::Manip::xml_encode(row.campaign_name().c_str(), ostr);
          
          ostr << "</a> " << (status == "D" ? "disabled" : "deleted")
               << "</li>";
        }

        status = row.page_status();
        
        if(status != "E")
        {
          ostr << "<li>page <a href=\"/psp/ad/page?id=" << row.page_id()
               << "\" target=\"_blank\">";

          El::String::Manip::xml_encode(row.page_name().c_str(), ostr);
          
          ostr << "</a> disabled</li>";
        }

        if(row.page_max_ad_num() == 0)
        {
          ostr << "<li>page <a href=\"/psp/ad/page?id=" << row.page_id()
               << "\" target=\"_blank\">";
          
          El::String::Manip::xml_encode(row.page_name().c_str(), ostr);
          
          ostr << "</a> Max Ad Count is 0</li>";
        }

        if(!row.adv_rst_max_ad_num().is_null() &&
           row.adv_rst_max_ad_num() == 0)
        {
          ostr << "<li>page <a href=\"/psp/ad/page?id=" << row.page_id()
               << "\" target=\"_blank\">";
          
          El::String::Manip::xml_encode(row.page_name().c_str(), ostr);
          
          ostr << "</a> Advertiser '";
          El::String::Manip::xml_encode(row.adv_name().c_str(), ostr);
          ostr << "' Max Ad Count is 0</li>";
        }
        
        status = row.adv_status();
        
        if(status != "E")
        {
          ostr << "<li>advertiser '";
          El::String::Manip::xml_encode(row.adv_name().c_str(), ostr);
          ostr << "' disabled</li>";
        }

        if(row.adv_max_ads_per_page() == 0)
        {
          ostr << "<li>advertiser '";
          El::String::Manip::xml_encode(row.adv_name().c_str(), ostr);
          ostr << "' Max Page Ad Count is 0</li>";
        }
        
        status = row.selector_status();
        
        if(status != "E")
        {
          ostr << "<li>selector disabled</li>";
        }

        return ostr.str();
      }

      void
      AdManagerImpl::load_group_placements(
        PlacementSeq& placements,
        ::NewsGate::Moderation::Ad::GroupId id,
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
        El::MySQL::Connection* connection)
        throw(El::Exception)
      {
        std::ostringstream ostr;

        ostr << "select AdPlacement.id as id, group_id, "
          "AdGroup.name as group_name, AdGroup.campaign as campaign, "
          "AdCampaign.name as campaign_name, AdPlacement.name as name, "
          "AdPlacement.status as status, slot, AdSlot.name as slot_name, "
          "AdSlot.min_width as slot_min_width, "
          "AdSlot.max_width as slot_max_width, "
          "AdSlot.min_height as slot_min_height, "
          "AdSlot.max_height as slot_max_height, "
          "AdSlot.status as slot_status, ad, Ad.name as ad_name, "
          "Ad.status as ad_status, Ad.size as ad_size, "
          "AdSize.name as ad_size_name, AdSize.width as ad_width, "
          "AdSize.height as ad_height, cpm, inject, auction_factor, "
          "AdPage.max_ad_num as page_max_ad_num, AdPage.id as page_id, "
          "AdPage.name as page_name, "
          "AdPageAdvRestriction.max_ad_num as adv_rst_max_ad_num, "
          "Advertiser.max_ads_per_page as adv_max_ads_per_page, "
          "Advertiser.status as adv_status, Advertiser.name as adv_name, "
          "AdSelector.status as selector_status, "
          "AdGroup.status as group_status, "
          "AdSize.status as size_status, AdPage.status as page_status, "
          "AdCampaign.status as campaign_status "
          "from AdPlacement join AdGroup on AdPlacement.group_id=AdGroup.id "
          "join AdCampaign on AdGroup.campaign=AdCampaign.id "
          "join AdSlot on slot=AdSlot.id join Ad on AdPlacement.ad=Ad.id "
          "join AdSize on Ad.size=AdSize.id "
          "join AdPage on AdPage.id=AdSlot.page "
          "left join AdPageAdvRestriction on "
          "AdPage.id=AdPageAdvRestriction.page "
          "and AdPlacement.advertiser=AdPageAdvRestriction.advertiser "
          "join Advertiser on Advertiser.id=AdPlacement.advertiser "
          "join AdSelector "
          "where AdPlacement.advertiser=" << advertiser_id
             << " and AdPlacement.group_id=" << id << " order by name";
        
        El::MySQL::Result_var result = connection->query(ostr.str().c_str());
        DB::Placement row(result.in());
            
        while(row.fetch_row())
        {
          size_t len = placements.length();
          placements.length(len + 1);

          Placement& plc = placements[len];

          plc.id = row.id();
          plc.group = row.group_id();
          plc.group_name = row.group_name().c_str();
          plc.campaign = row.campaign();
          plc.campaign_name = row.campaign_name().c_str();
          plc.advertiser = advertiser_id;
          plc.name = row.name().c_str();

          plc.placement_slot.id = row.slot();
          plc.placement_slot.name = row.slot_name().c_str();
          plc.placement_slot.min_width = row.slot_min_width();
          plc.placement_slot.max_width = row.slot_max_width();
          plc.placement_slot.min_height = row.slot_min_height();
          plc.placement_slot.max_height = row.slot_max_height();

          std::string slot_status(row.slot_status());
          
          plc.placement_slot.status = slot_status == "E" ?
            SS_ENABLED : SS_DISABLED;
          
          plc.ad.id = row.ad();
          plc.ad.name = row.ad_name().c_str();

          std::string ad_status(row.ad_status());
          
          plc.ad.status = ad_status == "E" ?
            AS_ENABLED : (ad_status == "D" ? AS_DISABLED : AS_DELETED);

          plc.ad.size = row.ad_size();
          plc.ad.size_name = row.ad_size_name().c_str();
          plc.ad.width = row.ad_width();
          plc.ad.height = row.ad_height();          
        
          std::string status(row.status());
          
          plc.status = status == "E" ?
            TS_ENABLED : (status == "D" ? TS_DISABLED : TS_DELETED);
        
          plc.cpm = row.cpm();

          std::string inject(row.inject());
          plc.inject = inject == "D" ? PI_DIRECT : PI_FRAME;

          plc.display_status = placement_display_status(row).c_str();
        }
      }      

      void
      AdManagerImpl::load_group_counter_placements(
        CounterPlacementSeq& placements,
        ::NewsGate::Moderation::Ad::GroupId id,
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
        El::MySQL::Connection* connection)
        throw(El::Exception)
      {
        std::ostringstream ostr;

        ostr << "select AdCounterPlacement.id as id, group_id, "
          "AdGroup.name as group_name, AdGroup.campaign as campaign, "
          "AdCampaign.name as campaign_name, AdCounterPlacement.name as name, "
          "AdCounterPlacement.status as status, page, "
          "AdPage.name as page_name, AdPage.status as page_status, "
          "counter, AdCounter.name as counter_name, "
          "AdCounter.status as counter_status, "
          "Advertiser.status as adv_status, Advertiser.name as adv_name, "
          "AdSelector.status as selector_status, "
          "AdGroup.status as group_status, "
          "AdCampaign.status as campaign_status "
          "from AdCounterPlacement "
          "join AdGroup on AdCounterPlacement.group_id=AdGroup.id "
          "join AdCampaign on AdGroup.campaign=AdCampaign.id "
          "join AdPage on page=AdPage.id "
          "join AdCounter on AdCounterPlacement.counter=AdCounter.id "
          "join Advertiser on Advertiser.id=AdCounterPlacement.advertiser "
          "join AdSelector "
          "where AdCounterPlacement.advertiser=" << advertiser_id
             << " and AdCounterPlacement.group_id=" << id << " order by name";
        
        El::MySQL::Result_var result = connection->query(ostr.str().c_str());
        DB::CounterPlacement row(result.in());
            
        while(row.fetch_row())
        {
          size_t len = placements.length();
          placements.length(len + 1);

          CounterPlacement& plc = placements[len];

          plc.id = row.id();
          plc.group = row.group_id();
          plc.group_name = row.group_name().c_str();
          plc.campaign = row.campaign();
          plc.campaign_name = row.campaign_name().c_str();
          plc.advertiser = advertiser_id;
          plc.name = row.name().c_str();

          plc.placement_page.id = row.page();
          plc.placement_page.name = row.page_name().c_str();

          std::string page_status(row.page_status());
          
          plc.placement_page.status = page_status == "E" ?
            PS_ENABLED : PS_DISABLED;
        
          plc.placement_counter.id = row.counter();
          plc.placement_counter.name = row.counter_name().c_str();

          std::string counter_status(row.counter_status());
          
          plc.placement_counter.status = counter_status == "E" ?
            US_ENABLED : (counter_status == "D" ? US_DISABLED : US_DELETED);

          std::string status(row.status());
          
          plc.status = status == "E" ?
            OS_ENABLED : (status == "D" ? OS_DISABLED : OS_DELETED);        

          plc.display_status = counter_placement_display_status(row).c_str();
        }
      }
      
      ::NewsGate::Moderation::Ad::Placement*
      AdManagerImpl::get_placement(
        ::NewsGate::Moderation::Ad::PlacementId id,
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id)
        throw(NewsGate::Moderation::ObjectNotFound,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        Placement_var placement = new Placement();
        
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            load_placement(*placement, id, advertiser_id, connection.in());
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::get_placement: "
            "El::Exception caught. Description:\n" << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return placement._retn();
      }
      
      ::NewsGate::Moderation::Ad::Placement*
      AdManagerImpl::update_placement(
        const ::NewsGate::Moderation::Ad::PlacementUpdate& update)
        throw(NewsGate::Moderation::ObjectNotFound,
              NewsGate::Moderation::ObjectAlreadyExist,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        Placement_var plc = new Placement();
          
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            result = connection->query(
              "select update_num from AdSelector for update");

            char status;
            
            switch(update.status)
            {
            case TS_ENABLED: status = 'E'; break;
            case TS_DISABLED: status = 'D'; break;
            default: status = 'L'; break;
            }

            char inject;
            
            switch(update.inject)
            {
            case PI_DIRECT: inject = 'D'; break;
            default: inject = 'F'; break;
            }
            
            {
              std::ostringstream ostr;
              ostr << "update AdPlacement set status='" << status
                   << "', name='" << connection->escape(update.name.in())
                   << "', slot=" << update.slot << ", ad=" << update.ad
                   << ", cpm=" << update.cpm << ", inject='" << inject
                   << "' where id=" << update.id
                   << " and advertiser=" << update.advertiser;

              try
              {
                result = connection->query(ostr.str().c_str());
              }
              catch(const El::Exception& e)
              {
                int code = mysql_errno(connection->mysql());

                if(code == ER_DUP_ENTRY)
                {
                  NewsGate::Moderation::ObjectAlreadyExist ex;
                  ex.reason = e.what();
                  throw ex;
                }
              
                throw;
              }
            }            
            
            result = connection->query(
              "update AdSelector set update_num=update_num+1");
            
            load_placement(*plc,
                           update.id,
                           update.advertiser,
                           connection.in());
            
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::"
            "update_placement: El::Exception caught. Description:\n"
               << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return plc._retn();
      }
        
      ::NewsGate::Moderation::Ad::Placement*
      AdManagerImpl::create_placement(
        const ::NewsGate::Moderation::Ad::Placement& plc)
        throw(NewsGate::Moderation::ObjectAlreadyExist,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        Placement_var new_plc = new Placement();
          
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            result = connection->query(
              "select update_num from AdSelector for update");

            char status;
            
            switch(plc.status)
            {
            case TS_ENABLED: status = 'E'; break;
            case TS_DISABLED: status = 'D'; break;
            default: status = 'L'; break;
            }

            char inject;
            
            switch(plc.inject)
            {
            case PI_DIRECT: inject = 'D'; break;
            default: inject = 'F'; break;
            }
            
            std::ostringstream ostr;
            ostr << "insert into AdPlacement set advertiser="
                 << plc.advertiser << ", group_id=" << plc.group
                 << ", slot=" << plc.placement_slot.id << ", ad=" << plc.ad.id
                 << ", status='" << status
                 << "', name='" << connection->escape(plc.name.in())
                 << "', cpm=" << plc.cpm << ", inject='" << inject << "'";

//            std::cerr << ostr.str() << std::endl;
            
            try
            {
              result = connection->query(ostr.str().c_str());
            }
            catch(const El::Exception& e)
            {
              int code = mysql_errno(connection->mysql());

              if(code == ER_DUP_ENTRY)
              {
                NewsGate::Moderation::ObjectAlreadyExist ex;
                ex.reason = e.what();
                throw ex;
              }
              
              throw;
            }

            PlacementId id = connection->insert_id();

            result = connection->query(
              "update AdSelector set update_num=update_num+1");

            try
            {
              load_placement(*new_plc, id, plc.advertiser, connection.in());
            }
            catch(const NewsGate::Moderation::ObjectNotFound& e)
            {
              std::ostringstream ostr;
              ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::"
                "create_placement: ObjectNotFound caught. Description:\n"
                   << e.reason.in();

              NewsGate::Moderation::ImplementationException ex;
              ex.description = ostr.str().c_str();
              throw ex;
            }
            
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::"
            "create_placement: El::Exception caught. Description:\n"
               << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return new_plc._retn();
      }
      
      ::NewsGate::Moderation::Ad::CounterPlacement*
      AdManagerImpl::get_counter_placement(
        ::NewsGate::Moderation::Ad::CounterPlacementId id,
        ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id)
        throw(NewsGate::Moderation::ObjectNotFound,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        CounterPlacement_var placement = new CounterPlacement();
        
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            load_counter_placement(*placement,
                                   id,
                                   advertiser_id,
                                   connection.in());
            
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::"
            "get_counter_placement: "
            "El::Exception caught. Description:\n" << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return placement._retn();
      }
      
      ::NewsGate::Moderation::Ad::CounterPlacement*
      AdManagerImpl::update_counter_placement(
        const ::NewsGate::Moderation::Ad::CounterPlacementUpdate& update)
        throw(NewsGate::Moderation::ObjectNotFound,
              NewsGate::Moderation::ObjectAlreadyExist,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        CounterPlacement_var plc = new CounterPlacement();
          
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            result = connection->query(
              "select update_num from AdSelector for update");

            char status;
            
            switch(update.status)
            {
            case OS_ENABLED: status = 'E'; break;
            case OS_DISABLED: status = 'D'; break;
            default: status = 'L'; break;
            }

            {
              std::ostringstream ostr;
              ostr << "update AdCounterPlacement set status='" << status
                   << "', name='" << connection->escape(update.name.in())
                   << "', page=" << update.page << ", counter="
                   << update.counter << " where id=" << update.id
                   << " and advertiser=" << update.advertiser;

              try
              {
                result = connection->query(ostr.str().c_str());
              }
              catch(const El::Exception& e)
              {
                int code = mysql_errno(connection->mysql());

                if(code == ER_DUP_ENTRY)
                {
                  NewsGate::Moderation::ObjectAlreadyExist ex;
                  ex.reason = e.what();
                  throw ex;
                }
              
                throw;
              }
            }            
            
            result = connection->query(
              "update AdSelector set update_num=update_num+1");
            
            load_counter_placement(*plc,
                                   update.id,
                                   update.advertiser,
                                   connection.in());
            
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::"
            "update_counter_placement: El::Exception caught. Description:\n"
               << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return plc._retn();
      }
        
      ::NewsGate::Moderation::Ad::CounterPlacement*
      AdManagerImpl::create_counter_placement(
        const ::NewsGate::Moderation::Ad::CounterPlacement& plc)
        throw(NewsGate::Moderation::ObjectAlreadyExist,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        CounterPlacement_var new_plc = new CounterPlacement();
          
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Result_var result = connection->query("begin");
          
          try
          {
            result = connection->query(
              "select update_num from AdSelector for update");

            char status;
            
            switch(plc.status)
            {
            case OS_ENABLED: status = 'E'; break;
            case OS_DISABLED: status = 'D'; break;
            default: status = 'L'; break;
            }

            std::ostringstream ostr;
            ostr << "insert into AdCounterPlacement set advertiser="
                 << plc.advertiser << ", group_id=" << plc.group
                 << ", page=" << plc.placement_page.id
                 << ", counter=" << plc.placement_counter.id
                 << ", status='" << status
                 << "', name='" << connection->escape(plc.name.in())
                 << "'";

//            std::cerr << ostr.str() << std::endl;
            
            try
            {
              result = connection->query(ostr.str().c_str());
            }
            catch(const El::Exception& e)
            {
              int code = mysql_errno(connection->mysql());

              if(code == ER_DUP_ENTRY)
              {
                NewsGate::Moderation::ObjectAlreadyExist ex;
                ex.reason = e.what();
                throw ex;
              }
              
              throw;
            }

            PlacementId id = connection->insert_id();

            result = connection->query(
              "update AdSelector set update_num=update_num+1");

            try
            {
              load_counter_placement(*new_plc,
                                     id,
                                     plc.advertiser,
                                     connection.in());
            }
            catch(const NewsGate::Moderation::ObjectNotFound& e)
            {
              std::ostringstream ostr;
              ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::"
                "create_counter_placement: ObjectNotFound caught. "
                "Description:\n" << e.reason.in();

              NewsGate::Moderation::ImplementationException ex;
              ex.description = ostr.str().c_str();
              throw ex;
            }
            
            result = connection->query("commit");
          }
          catch(...)
          {
            result = connection->query("rollback");
            throw;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Ad::AdManagerImpl::"
            "create_counter_placement: El::Exception caught. Description:\n"
               << e.what();

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        
        return new_plc._retn();
      }
    }
  }
}
