/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

#include <Python.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <limits.h>

#include <libxml/xpath.h>


#include <El/CORBA/Corba.hpp>

#include <string>
#include <list>
#include <sstream>
#include <fstream>
#include <vector>
#include <memory>

#include <ace/OS.h>
#include <ace/High_Res_Timer.h>

//#include <Magick++.h>

#include <xercesc/sax2/SAX2XMLReader.hpp>

#include <El/Exception.hpp>
#include <El/String/ListParser.hpp>
#include <El/String/Manip.hpp>
#include <El/Image/ImageInfo.hpp>
#include <El/Guid.hpp>
#include <El/CRC.hpp>

#include <El/Net/Socket/Stream.hpp>
#include <El/Net/HTTP/Session.hpp>
#include <El/Net/HTTP/StatusCodes.hpp>
#include <El/Net/HTTP/Headers.hpp>
#include <El/HTML/LightParser.hpp>
#include <El/Python/Sandbox.hpp>
#include <El/LibXML/Traverser.hpp>

#include <Commons/Message/Message.hpp>
#include <Commons/Message/TransportImpl.hpp>
#include <Commons/Feed/Types.hpp>

#include <xsd/DataFeed/RSS/Data.hpp>
#include <xsd/DataFeed/RSS/ParserFactory.hpp>
#include <xsd/DataFeed/RSS/HTMLFeedParser.hpp>

#include <xsd/ConfigParser.hpp>

#include <Services/Commons/Message/BankClientSessionImpl.hpp>
#include <Services/Feeds/RSS/Commons/RSSFeedServices.hpp>
#include <Services/Feeds/RSS/Puller/ThumbGen.hpp>

#include "PullerMain.hpp"
#include "PullingFeeds.hpp"

namespace
{
  const size_t MAX_MSG_URL_LEN = 2048;
};

namespace NewsGate
{
  namespace RSS
  {
    //
    // PullingFeeds class
    //

    PullingFeeds::PullingFeeds(
      PullerStateCallback* callback,
      const PullerManagementConfig& puller_management_config,
      const FeedRequestConfig& feed_request_config,
      const SavingFeedStateConfig& saving_feed_state_config,
      const SavingFeedStatConfig& saving_feed_stat_config,
      const SavingTrafficConfig& saving_traffic_config)
      throw(Exception, El::Exception)
        : PullerState(callback, "PullingFeeds"),
          address_info_(/*GEOIP_COUNTRY_EDITION, GEOIP_INDEX_CACHE*/),
          message_bank_client_sessions_index_(0),
          message_entities_reserve_(100),
          accept_feed_counter_(0),
          request_feeds_(true),
          puller_management_config_(puller_management_config),
          feed_request_config_(feed_request_config),
          saving_feed_state_config_(saving_feed_state_config),
          saving_feed_stat_config_(saving_feed_stat_config),
          saving_traffic_config_(saving_traffic_config)
    {
      Application::logger()->trace(
        "PullingFeeds::PullingFeeds: pulling init",
        Aspect::STATE,
        El::Logging::LOW);

//      ACE_OS::sleep(20);
      
      {
        std::string extensions =
          feed_request_config_.image().extension_whitelist();

        El::String::ListParser parser(extensions.c_str());
        const char* ext;
        while((ext = parser.next_item()) != 0)
        {
          std::string file_ext;
          El::String::Manip::to_lower(ext, file_ext);

          if(!file_ext.empty())
          {
            image_extension_whitelist_.insert(file_ext);
          }
        }        
      }
      
      {
        std::string prefixes = feed_request_config_.image().prefix_blacklist();

        El::String::ListParser parser(prefixes.c_str());
        
        const char* prefix;
        while((prefix = parser.next_item()) != 0)
        {
          image_prefix_blacklist_.push_back(prefix);
        }
      }

      process_pool_ =
        new El::Service::ProcessPool(
          this,
          "libThumbGen.so",
          "create_task_factory",
          0,
          0,
          "PullingFeedsProcessPool",
          feed_request_config_.thumb_gen().processes(),
          feed_request_config_.thumb_gen().timeout() * 1000);

      size_t timeout = feed_request_config_.python().sandbox().timeout();
      
      sandbox_service_ =
        new El::Python::SandboxService(
          this,
          "RSSPullerSandboxService",
          feed_request_config_.python().sandbox().processes(),
//          timeout ? (timeout + 1) * 1000 : 0,
          // Process-level timeout doubled to ensure python-level timeout
          // works before
          timeout ? timeout * 2000 : 0,
          "libFeedParsing.so");      

      create_message_bank_manager_refs();

      load_stat_cache();

      El::Service::Service_var req_feeds =
        new RequestingFeeds(this, feed_request_config_.threads());

      state(req_feeds.in());

      El::Service::CompoundServiceMessage_var msg = new PushFeedState(this);

      ACE_Time_Value now = ACE_OS::gettimeofday();
      
      deliver_at_time(
        msg.in(),
        now + ACE_Time_Value(saving_feed_state_config_.max_delay()));

      msg = new PushFeedStat(this);

      deliver_at_time(
        msg.in(),
        feed_stat_last_save_ +
        ACE_Time_Value(saving_feed_stat_config_.period()));
      
      msg = new Poll(this);

      deliver_at_time(
        msg.in(),
        now + ACE_Time_Value(puller_management_config_.poll_period()));

      Application::logger()->trace(
        "PullingFeeds::PullingFeeds: start pulling",
        Aspect::STATE,
        El::Logging::HIGH);
    }

    void
    PullingFeeds::create_message_bank_manager_refs()
      throw(Exception, El::Exception)
    {
      typedef Server::Config::RSSFeedPullerType::message_post_type::
        bank_manager_sequence MessagePostManagerConf;

      PullerApp* app = Application::instance();

      const MessagePostManagerConf& config =
        app->config().message_post().bank_manager();

      for(MessagePostManagerConf::const_iterator it = config.begin();
          it != config.end(); it++)
      {
        std::string ref = it->ref();

        try
        {
          message_bank_managers_.push_back(
            BankManagerRef(ref.c_str(), app->orb()));
        }
        catch(const CORBA::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::RSS::PullingFeeds::"
            "create_message_bank_manager_refs: CORBA::Exception caught. "
            "Description:\n" << e;
        
          throw Exception(ostr.str());
        }        
      }
    }
    
    void
    PullingFeeds::bank_client_sessions(MessageBankClientSessionArray& sessions)
      throw(Exception, El::Exception)
    {
      WriteGuard guard(srv_lock_);

      for(MessageBankManagerList::iterator it = message_bank_managers_.begin();
          it != message_bank_managers_.end(); )
      { 
        try
        {
          NewsGate::Message::BankManager_var bank_manager = it->object();
          
          NewsGate::Message::BankClientSession_var bank_client_session =
            bank_manager->bank_client_session();

          Message::BankClientSessionImpl* bank_client_session_impl =
            dynamic_cast<Message::BankClientSessionImpl*>(
              bank_client_session.in());

          if(bank_client_session_impl == 0)
          {
            throw Exception(
              "NewsGate::RSS::PullingFeeds::bank_client_sessions: "
              "dynamic_cast<Message::BankClientSessionImpl*> failed");
          }
          
          bank_client_session_impl->init_threads(this);

          message_bank_client_sessions_.push_back(bank_client_session);

          MessageBankManagerList::iterator cur = it++;
          message_bank_managers_.erase(cur);
          continue;
        }
        catch(const ::NewsGate::Message::ImplementationException& e)
        {
          if(Application::will_trace(El::Logging::MIDDLE))
          {
            std::ostringstream ostr;
            ostr << "NewsGate::RSS::PullingFeeds::"
              "bank_client_sessions: ImplementationException caught. "
              "Description:\n" << e.description.in();
          
            Application::logger()->trace(ostr.str(),
                                         Aspect::MSG_MANAGEMENT,
                                         El::Logging::MIDDLE);
          }
        }
        catch(const CORBA::Exception& e)
        {
          if(Application::will_trace(El::Logging::MIDDLE))
          {
            std::ostringstream ostr;
            ostr << "NewsGate::RSS::PullingFeeds::"
              "bank_client_sessions: CORBA::Exception caught. "
              "Description:\n" << e;
          
            Application::logger()->trace(ostr.str(),
                                         Aspect::MSG_MANAGEMENT,
                                         El::Logging::MIDDLE);
          }
        }

        it++;
      }

      if(message_bank_client_sessions_.size() == 0)
      {
        throw Exception("NewsGate::RSS::PullingFeeds::bank_client_sessions: "
                        "no message bank client sessions available");        
      }
      
      sessions.clear();
      sessions.reserve(message_bank_client_sessions_.size());

      if(message_bank_client_sessions_index_ >=
         message_bank_client_sessions_.size())
      {
        message_bank_client_sessions_index_ = 0;
      }

      for(unsigned long i = message_bank_client_sessions_index_;
          i < message_bank_client_sessions_.size(); i++)
      {
        sessions.push_back(message_bank_client_sessions_[i]);
      }
      
      for(unsigned long i = 0; i < message_bank_client_sessions_index_; i++)
      {
        sessions.push_back(message_bank_client_sessions_[i]);
      }

      message_bank_client_sessions_index_++;
    }

    void
    PullingFeeds::logout(::NewsGate::RSS::SessionId session)
      throw(NewsGate::RSS::Puller::ImplementationException,
            CORBA::SystemException)
    {
      if(started())
      {
        callback_->initiate_logout();
      }
    }
    
    void
    PullingFeeds::update_feeds(::NewsGate::RSS::SessionId session,
                               ::NewsGate::RSS::FeedPack* feeds)
      throw(NewsGate::RSS::Puller::NotReady,
            NewsGate::RSS::Puller::ImplementationException,
            CORBA::SystemException)
    {
      try
      {
        if(!started())
        {
          NewsGate::RSS::Puller::NotReady ex;
          ex.reason = CORBA::string_dup("Logout in progress");
          throw ex;
        }
        
        Application::logger()->trace("PullingFeeds::update_feeds()",
                                     Aspect::STATE,
                                     El::Logging::LOW);
        
        if(session == callback_->session())
        {
          El::Service::CompoundServiceMessage_var msg =
            new AcceptFeeds(this, feeds);
          
          deliver_now(msg.in());
          
          return;
        }

        if(Application::will_trace(El::Logging::LOW))
        {
          std::ostringstream ostr;
          ostr << "PullingFeeds::update_feeds: "
            "unexpected session " << session << " instead of "
               << callback_->session() << "; logging out ...";
          
          Application::logger()->trace(ostr.str(),
                                       Aspect::STATE,
                                       El::Logging::LOW);
        }
      }
      catch(const El::Exception& e)
      {
        NewsGate::RSS::Puller::ImplementationException ex;

        try
        {
          std::ostringstream ostr;
          ostr << "PullingFeeds::update_feeds: "
            "got El::Exception. Description:\n" << e;

          ex.description = CORBA::string_dup(ostr.str().c_str());
        }
        catch(...)
        {
        }

        throw ex;
      }

      callback_->initiate_logout();
    }

    void
    PullingFeeds::accept_feeds(::NewsGate::RSS::FeedPack* feeds)
      throw()
    {
      try
      {
        std::ostringstream ostr;
        ostr << "PullingFeeds::accept_feeds: reading feed infos, thread 0x"
             << std::hex << pthread_self();
          
        Application::logger()->trace(
          ostr.str(),
          Aspect::STATE,
          El::Logging::HIGH);


        Transport::FeedPackImpl::Type* feed_pack =
            dynamic_cast<Transport::FeedPackImpl::Type*>(feeds);

        if(feed_pack == 0)
        {
          throw Exception("PullingFeeds::accept_feeds: dynamic_cast<"
            "::NewsGate::RSS::Transport::FeedPackImpl::Type*>(feeds) failed");
        }
    
        Transport::FeedArray& seq = feed_pack->entities();

        WriteGuard guard(srv_lock_);

        accept_feed_counter_++;
        
        for(Transport::FeedArray::iterator it = seq.begin(); it != seq.end();
            it++)
        {
          Transport::Feed& feed = *it;
          Feed::Id feed_id = feed.state.id;
          
          if(feed.status == 'E')
          {
            FeedInfoTable::iterator it = feeds_.find(feed_id);

            bool is_new_feed = false;
          
            if(it == feeds_.end())
            {
              is_new_feed = true;

              it = feeds_.insert(
                FeedInfoTable::value_type(feed_id, FeedInfo())).first;
            } 
          
            FeedInfo& feed_info = it->second;

            if(feed_info.url != feed.url)
            {
              //
              // If feed url have changed, then need to reset the state
              // as probably feed characteristics will also change
              //
              feed_info.url = feed.url;
              feed_info.type = (::NewsGate::Feed::Type)feed.state.channel.type;
//              feed_info.state.channel.type = (::NewsGate::Feed::Type)feed.type;
              feed_info.state = feed.state;
              feed_info.last_messages.swap(feed.state.last_messages);
              // TODO: update html_feed_cache here
            }

            feed_info.encoding = feed.encoding;
            feed_info.space = (::NewsGate::Feed::Space)feed.space;
            feed_info.lang = El::Lang((El::Lang::ElCode)feed.lang);
            
            feed_info.country =
              El::Country((El::Country::ElCode)feed.country);

            feed_info.keywords.clear();
            
            El::String::ListParser parser(feed.keywords.c_str(), "\n\r");
            const char* kw;
            while((kw = parser.next_item()) != 0)
            {
              std::string k;
              El::String::Manip::trim(kw, k);
              feed_info.keywords.push_back(k);
            }

            std::string trimmed;
            El::String::Manip::trim(feed.adjustment_script.c_str(), trimmed);
              
            feed_info.adjustment_script = trimmed.empty() ? "" :
              feed.adjustment_script.c_str();
            
            if(feed_info.state.last_request_date == 0)
            {
              feed_info.state.heuristics_counter = -1 *
                (saving_traffic_config_.heuristics().apply_after() +
                 saving_traffic_config_.heuristics().calc_entropy_after());
            }

            if(is_new_feed)
            {
              feed_info.update_number = accept_feed_counter_;
              schedule_request(feed_info, ST_FIRST_REQUEST);
            }
            else
            {
              // TODO: if required, then cancel request task for
              // feed.id and reschedule it
            }          
          }
          else
          {
            feeds_.erase(feed_id);
            
            // TODO: cancel request task for feed.id
          } 
        }

        Application::logger()->trace(
          "PullingFeeds::accept_feeds: requests scheduled",
          Aspect::STATE,
          El::Logging::HIGH);
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "PullingFeeds::accept_feeds: "
          "El::Exception caught. Description:" << std::endl << e;

        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
      }
      
    }
    
    void
    PullingFeeds::schedule_request(FeedInfo& feed_info,
                                   ScheduleRequestType type)
      throw(Exception, El::Exception)
    {
      RequestingFeeds* requesting_feeds =
        dynamic_cast<RequestingFeeds*>(state_.in());

      if(requesting_feeds == 0)
      {
        El::Service::Error error(
          "PullingFeeds::schedule_request: "
          "dynamic_cast<RequestingFeeds*> failed",
          this);
        
        callback_->notify(&error);
            
        return;
      }
        
      El::Service::CompoundServiceMessage_var msg =
        new RequestFeed(this, feed_info.state.id, feed_info.update_number);

      ACE_Time_Value cur_time = ACE_OS::gettimeofday();

      if(type == ST_SKIPPED_REQUEST)
      {
        ACE_Time_Value request_time = cur_time +
          ACE_Time_Value(saving_feed_state_config_.
                         delay_feed_requests_on_failure());

        if(Application::will_trace(El::Logging::MIDDLE))
        {
          std::ostringstream ostr;
          ostr << "PullingFeeds::schedule_request: skipped request: id="
               << feed_info.state.id << ", url=" << feed_info.url
               << ", next request time=" << El::Moment(request_time).rfc0822()
               << " (shift from now="
               << ACE_Time_Value(request_time - cur_time).sec() << " sec)";

          Application::logger()->trace(ostr.str(),
                                     Aspect::PULLING_FEEDS,
                                     El::Logging::MIDDLE);
        }

        requesting_feeds->deliver_at_time(msg.in(), request_time);
        return;
      }

      if(feed_info.state.last_request_date == 0)
      {
        if(type == ST_FIRST_REQUEST)
        {
          requesting_feeds->deliver_now(msg.in());
        }
        else
        {
          ACE_Time_Value request_time = cur_time +
            ACE_Time_Value(saving_feed_state_config_.
                           delay_feed_requests_on_failure());
          
          if(Application::will_trace(El::Logging::MIDDLE))
          {
            std::ostringstream ostr;
            ostr << "PullingFeeds::schedule_request: rescheduling request: id="
                 << feed_info.state.id << ", url=" << feed_info.url
                 << ", next request time="
                 << El::Moment(request_time).rfc0822()
                 << " (shift from now="
                 << ACE_Time_Value(request_time - cur_time).sec() << " sec)";

            Application::logger()->trace(ostr.str(),
                                         Aspect::PULLING_FEEDS,
                                         El::Logging::MIDDLE);
          }

          requesting_feeds->deliver_at_time(msg.in(), request_time);
        }
        
        return;
      }

      const Message::LocalCodeArray& msg_codes = feed_info.last_messages;
      size_t intervals = 0;

      typedef std::vector<uint64_t> DateArray;
      DateArray msg_dates;
      ACE_Time_Value last_message_time;
      
      if(msg_codes.size() > 0)
      {
        last_message_time = ACE_Time_Value(msg_codes.begin()->published);
          
        intervals = msg_codes.size() - 1;

        if(intervals > feed_request_config_.approximate_msg_intervals())
        {
          intervals = feed_request_config_.approximate_msg_intervals();
        }

        if(intervals)
        {
          uint64_t cur_time_sec = cur_time.sec();          
        
          for(Message::LocalCodeArray::const_iterator i = msg_codes.begin();
              i != msg_codes.end(); i++)
          {
            uint64_t published = std::min(i->published, cur_time_sec);

            if(!published)
            {
              continue;
            }

            if((uint64_t)last_message_time.sec() < published)
            {
              last_message_time = ACE_Time_Value(published);
            }
            
            DateArray::iterator it = msg_dates.begin();
            for(; it != msg_dates.end() && *it > published; ++it);
            
            if(it == msg_dates.end() || *it != published)
            {
              msg_dates.insert(it, published);
            }
          }

          intervals = msg_dates.empty() ? 0 :
            std::min(intervals, msg_dates.size() - 1);
        }
      }      

      time_t avg_time = intervals ?
        (msg_dates[0] - msg_dates[intervals]) / intervals : 0;

      time_t ttl = feed_info.state.channel.ttl * 60;
      
      time_t message_period = ttl ?
        (avg_time ? std::min(ttl, avg_time) : ttl) : avg_time;
      
      if(message_period)
      {
        message_period =
          (time_t)(feed_request_config_.message_period_factor() *
                   message_period);
      }
      else
      {
        message_period = feed_request_config_.default_period();
      }

      time_t last_request_date = feed_info.state.last_request_date;

      if(last_message_time.sec())
      {
        feed_info.no_message_request_time = 0;
      }
      else
      {
        if(!feed_info.no_message_request_time)
        {
          feed_info.no_message_request_time = cur_time.sec();
        }
        
        time_t last_build_date = feed_info.state.channel.last_build_date;
          
        last_message_time = last_build_date ?
          std::min(last_build_date, feed_info.no_message_request_time) :
          feed_info.no_message_request_time;
      }
      
      if(last_message_time > cur_time)
      {
        last_message_time = cur_time;
      }

      ACE_Time_Value request_time =
        (last_message_time == ACE_Time_Value::zero ?
         cur_time : last_message_time) + ACE_Time_Value(message_period);      

      if(type == ST_FIRST_REQUEST && request_time < cur_time &&
         request_time.sec() < last_request_date)
      {
        // Step from last_request_date
        
        time_t delay = last_request_date - request_time.sec();

        if(ttl)
        {
//          delay = (ttl + delay) / 2;
          delay = std::min(ttl, delay);
        }
          
        request_time = ACE_Time_Value(last_request_date + delay);
      }

      bool forecast_failed = false;

      if(request_time < cur_time)
      {
        if(type == ST_FIRST_REQUEST)
        {
          request_time = cur_time;
        }
        else
        {
          forecast_failed = true;

          unsigned long delay = cur_time.sec() - request_time.sec();

          if(ttl && (unsigned long)ttl < delay)
          {
            delay = (ttl + delay) / 2;
          }
          
          request_time = cur_time + ACE_Time_Value(delay);
        }
      }
      
      bool adjust_to_max = last_request_date && request_time >
        ACE_Time_Value(last_request_date + feed_request_config_.max_period());
      
      if(adjust_to_max)
      {
        request_time = last_request_date + feed_request_config_.max_period();
      }

      bool adjust_to_min = last_request_date && request_time <
        ACE_Time_Value(last_request_date + feed_request_config_.min_period());
      
      if(adjust_to_min)
      {
        request_time =
          last_request_date + feed_request_config_.min_period();
      }

      if(Application::will_trace(El::Logging::MIDDLE))
      {
        std::ostringstream ostr;
        ostr << "PullingFeeds::schedule_request: id=" << feed_info.state.id
             << ", url=" << feed_info.url
             << ", message_period=" << message_period << " (avg_time="
             << avg_time << ", ttl=" << ttl << "), last_message_time="
             << El::Moment(last_message_time).rfc0822()
             << ", request_time=" << El::Moment(request_time).rfc0822()
             << " (shift from last message="
             << ACE_Time_Value(request_time - last_message_time).sec()
             << " sec, from now="
             << ACE_Time_Value(request_time - cur_time).sec() << " sec), "
          "forecast=" << (forecast_failed ? "failed" : "ok")
             << ", adjustment="
             << (adjust_to_max ? "max" : (adjust_to_min ? "min" : "none"));

        Application::logger()->trace(ostr.str(),
                                     Aspect::PULLING_FEEDS,
                                     El::Logging::MIDDLE);
      }
      
      requesting_feeds->deliver_at_time(msg.in(), request_time);      
    }
    
    void
    PullingFeeds::poll() throw()
    {
      try
      {
        std::ostringstream ostr;
        ostr << "PullingFeeds::poll: polling, thread 0x" << std::hex
         << pthread_self();
              
        Application::logger()->trace(
          ostr.str(),
          Aspect::STATE,
          El::Logging::HIGH);
        
        try
        {
          RSS::PullerManager_var puller_manager =
            Application::instance()->puller_manager();

          puller_manager->ping(callback_->session());
        }
        catch(const NewsGate::Message::NotReady& e)
        {
          std::ostringstream ostr;
          ostr << "PullingFeeds::poll: bank not ready. Reason:\n"
               << e.reason.in();
            
          El::Service::Error error(ostr.str(),
                                   this,
                                   El::Service::Error::WARNING);
          
          callback_->notify(&error);
        }
        catch(const NewsGate::RSS::PullerManager::Logout& e)
        {  
          if(started())
          {
            callback_->initiate_logout();
          }
          
          return;
        }
        catch(const CORBA::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "PullingFeeds::poll: "
            "CORBA::Exception caught. Description:" << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "PullingFeeds::poll: "
            "El::Exception caught. Description:" << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }

        try
        {
          El::Service::CompoundServiceMessage_var msg = new Poll(this);
          
          deliver_at_time(
            msg.in(),
            ACE_OS::gettimeofday() +
              ACE_Time_Value(puller_management_config_.poll_period()));
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "PullingFeeds::poll: El::Exception "
            "caught while scheduling task. Description:"
               << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        
      }
      catch(...)
      {
        El::Service::Error error(
          "PullingFeeds::poll: unexpected exception caught.",
          this);
          
        callback_->notify(&error);
      }    
    }
    
    void
    PullingFeeds::push_feed_state() throw()
    {
      bool success = false;
      
      try
      {
        Application::logger()->trace(
          "PullingFeeds::push_feed_state: pushing state",
          Aspect::STATE,
          El::Logging::HIGH);
        
        try
        {
          FeedRequestResultPackList feed_request_results;
  
          {
            WriteGuard guard(srv_lock_);
            
            if(!feed_request_results_.empty())
            {
              feed_request_results.splice(feed_request_results.begin(),
                                          feed_request_results_,
                                          feed_request_results_.begin(),
                                          feed_request_results_.end());
            }
            
          }

          RSS::PullerManager_var puller_manager =
            Application::instance()->puller_manager();

          RSS::SessionId session = callback_->session();

          if(!feed_request_results.empty())
          {
            try
            {                
              push_request_results(feed_request_results,
                                   session,
                                   puller_manager.in());
            }
            catch(...)
            {
              WriteGuard guard(srv_lock_);
            
              feed_request_results_.splice(feed_request_results_.begin(),
                                           feed_request_results,
                                           feed_request_results.begin(),
                                           feed_request_results.end());
            
              throw;
            }
          }
          else
          {
            puller_manager->ping(session);
          }

          success = true;
        }
        catch(const NewsGate::Message::NotReady& e)
        {
          std::ostringstream ostr;
          ostr << "PullingFeeds::push_feed_state: bank not ready. "
            "Reason:\n" << e.reason.in();
            
          El::Service::Error error(ostr.str(),
                                   this,
                                   El::Service::Error::WARNING);
          
          callback_->notify(&error);
        }
        catch(const NewsGate::RSS::PullerManager::Logout& e)
        {  
          if(started())
          {
            callback_->initiate_logout();
          }
          
          return;
        }
        catch(const CORBA::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "PullingFeeds::push_feed_state: "
            "CORBA::Exception caught. Description:" << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "PullingFeeds::push_feed_state: "
            "El::Exception caught. Description:" << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }

        try
        {
          El::Service::CompoundServiceMessage_var msg =
            new PushFeedState(this);
          
          deliver_at_time(
            msg.in(),
            ACE_OS::gettimeofday() +
              ACE_Time_Value(saving_feed_state_config_.max_delay()));
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "PullingFeeds::push_feed_state: "
            "El::Exception caught while scheduling task. Description:"
               << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        
        Application::logger()->trace(
          "PullingFeeds::push_feed_state: state pushed",
          Aspect::STATE,
          El::Logging::HIGH);
      }
      catch(...)
      {
        El::Service::Error error(
          "PullingFeeds::push_feed_state: unexpected exception caught.",
          this);
          
        callback_->notify(&error);
      }    
      
      {
        WriteGuard guard(srv_lock_);
        request_feeds_ = success;
      }
    }

    void
    PullingFeeds::push_request_results(
      FeedRequestResultPackList& feed_request_results,
      const RSS::SessionId& session,
      RSS::PullerManager_ptr puller_manager)
      throw(Exception, El::Exception, CORBA::Exception)
    {
      MessageBankClientSessionArray sessions;
      bank_client_sessions(sessions);

      FeedRequestResultPackList::iterator it;
      while((it = feed_request_results.begin()) != feed_request_results.end())
      {
        Message::Transport::RawMessagePackImpl::Var& message_pack =
          it->message_pack;
        
        if(message_pack.in() != 0 &&
           (message_pack->serialized() || message_pack->entities().size() > 0))
        {
          for(MessageBankClientSessionArray::iterator it = sessions.begin();
              it != sessions.end(); it++)
          {
            try
            {
              it->in()->post_messages(
                message_pack.in(),
                NewsGate::Message::PMR_NEW_MESSAGES,
                NewsGate::Message::BankClientSession::
                PS_DISTRIBUTE_BY_MSG_ID,
                "");

              message_pack = 0;
              
              break;
            }
            catch(const NewsGate::Message::BankClientSession::
                  FailedToPostMessages& e)
            {
              if(Application::will_trace(El::Logging::MIDDLE))
              {
                std::ostringstream ostr;
                ostr << "NewsGate::RSS::PullingFeeds::push_request_results: "
                  "FailedToPostMessages caught. Description:\n"
                     << e.description.in();
            
                Application::logger()->trace(ostr.str(),
                                             Aspect::PULLING_FEEDS,
                                             El::Logging::MIDDLE);
              }
              
              message_pack =
                dynamic_cast<Message::Transport::RawMessagePackImpl::Type*>(
                  e.messages.in());

              if(message_pack.in() == 0)
              {
                throw Exception(
                  "NewsGate::RSS::PullingFeeds::push_request_results: "
                  "dynamic_cast<Message::Transport::RawMessagePackImpl::Type*>"
                  " failed");
              }

              message_pack->_add_ref();
            }
          }
/*
          Do not push messages randomly anymore as now core work detection
          heavy relies on fact that specific feed messages concentrated in
          the same message bank. Putting a message to improper bank reduces
          quality of that message core words detection, which will affect
          quality of events at the end of the day.
          
          if(message_pack.in() != 0)
          {
            //
            // Couldn't push all messages distributing by message id hash;
            // all clusters were tried. Now will try to push to any message
            // bank.
            //

            for(MessageBankClientSessionArray::iterator it = sessions.begin();
                it != sessions.end(); it++)
            {
              try
              {
                it->in()->post_messages(
                  message_pack.in(),
                  NewsGate::Message::PMR_NEW_MESSAGES,
                  NewsGate::Message::BankClientSession::
                  PS_TO_RANDOM_BANK);

                message_pack = 0;
              
                break;
              }
              catch(const NewsGate::Message::BankClientSession::
                    FailedToPostMessages& e)
              {
                if(Application::will_trace(El::Logging::MIDDLE))
                {
                  std::ostringstream ostr;
                  ostr << "NewsGate::RSS::PullingFeeds::push_request_results: "
                    "FailedToPostMessages caught (2). Description:\n"
                       << e.description.in();
            
                  Application::logger()->trace(ostr.str(),
                                               Aspect::PULLING_FEEDS,
                                               El::Logging::LOW);
                }
              
                message_pack =
                  dynamic_cast<Message::Transport::RawMessagePackImpl::Type*>(
                    e.messages.in());

                if(message_pack.in() == 0)
                {
                  throw Exception(
                    "NewsGate::RSS::PullingFeeds::push_request_results: "
                    "dynamic_cast<Message::Transport::RawMessagePackImpl::"
                    "Impl*> failed (2)");
                }

                message_pack->_add_ref();
              }
            
            }
          }
*/
        }

        if(message_pack.in() != 0)
        {
          NewsGate::Message::NotReady e;

          std::ostringstream ostr;
          ostr << "NewsGate::RSS::PullingFeeds::push_request_results: "
            "no bank could accept messages; giving up";

          e.reason = CORBA::string_dup(ostr.str().c_str());
          
          throw e;
        }
        
        if(puller_manager->feed_state(session, it->state_update_pack.in()))
        {
          accumulate_feed_stat(it->requests_info);                  
          feed_request_results.erase(it);
        }
      }
    }
    
    void
    PullingFeeds::accumulate_feed_stat(
      const FeedRequestInfoList& requests_info)
      throw(Exception, El::Exception)
    { 
      WriteGuard guard(srv_lock_);

      for(FeedRequestInfoList::const_iterator ri_it = requests_info.begin();
          ri_it != requests_info.end(); ri_it++)
      {
        El::Moment date = El::Moment(ri_it->time).date();
                      
        FeedDailyStatList::iterator ds_it =
          feed_daily_stat_.begin();
                    
        for(; ds_it != feed_daily_stat_.end() &&
              date > ds_it->date; ds_it++);

        if(ds_it == feed_daily_stat_.end() || date < ds_it->date)
        {
          FeedDailyStat ds;
                        
          ds.date = date;                      
          ds_it = feed_daily_stat_.insert(ds_it, ds);
        }

        FeedStatTable& feed_stat_tbl = ds_it->feed_stat;
        FeedStatTable::iterator fs_it = feed_stat_tbl.find(ri_it->feed_id);

        bool inserted = false;
                    
        if(fs_it == feed_stat_tbl.end())
        {
          fs_it = feed_stat_tbl.insert(
            FeedStatTable::value_type(
              ri_it->feed_id,
              ::NewsGate::RSS::Transport::FeedStat())).first;
                      
          inserted = true;
        }
                      
        ::NewsGate::RSS::Transport::FeedStat& feed_stat = fs_it->second;

        if(inserted)
        {
          feed_stat.id = fs_it->first;
          feed_stat.requests = 0;
          feed_stat.failed = 0;
          feed_stat.unchanged = 0;
          feed_stat.not_modified = 0;
          feed_stat.presumably_unchanged = 0;
          feed_stat.has_changes = 0;
          feed_stat.wasted = 0;
          feed_stat.outbound = 0;
          feed_stat.inbound = 0;
          feed_stat.requests_duration = 0;
          feed_stat.messages = 0;
          feed_stat.messages_size = 0;
          feed_stat.messages_delay = 0;
          feed_stat.max_message_delay = 0;
          feed_stat.mistiming = 0;
        }
        
        feed_stat.requests++;

        switch(ri_it->result)
        {
        case FeedRequestInfo::RT_FAILED:
          {
            feed_stat.failed++;
            break;
          }
        case FeedRequestInfo::RT_NOT_MODIFIED:
          {
            feed_stat.not_modified++;
            break;
          }
          
        case FeedRequestInfo::RT_PRESUMABLY_UNCHANGED:
          {
            feed_stat.presumably_unchanged++;
            break;
          }
        case FeedRequestInfo::RT_HAS_CHANGES:
          {
            feed_stat.has_changes++;
            break;
          }
        case FeedRequestInfo::RT_UNCHANGED:
          {
            feed_stat.unchanged++;
            break;
          }
        case FeedRequestInfo::RT_WASTED:
          {
            feed_stat.wasted += ri_it->wasted;
            break;
          }          
        default:;
        }

        feed_stat.outbound += ri_it->outbound_bytes;
        feed_stat.inbound += ri_it->inbound_bytes;

        feed_stat.requests_duration += ri_it->duration.sec() * 1000 +
          ri_it->duration.usec() / 1000;
                    
        feed_stat.messages += ri_it->messages;
        feed_stat.messages_size += ri_it->messages_size;
        feed_stat.messages_delay += ri_it->messages_delay;

        if(ri_it->max_message_delay > feed_stat.max_message_delay)
        {
          feed_stat.max_message_delay = ri_it->max_message_delay;
        }
                    
        if(abs(feed_stat.mistiming) < abs(ri_it->mistiming))
        {
          feed_stat.mistiming = ri_it->mistiming;
        }
      }
    }
                  
    void
    PullingFeeds::push_feed_stat() throw()
    {
      ACE_Time_Value now = ACE_OS::gettimeofday();
      ACE_Time_Value next_save_time_shift;
        
      try
      {
        try
        {
          RSS::PullerManager_var puller_manager =
            Application::instance()->puller_manager();

          RSS::SessionId session = callback_->session();

          FeedDailyStatList feed_daily_stat;
          
          {
            WriteGuard guard(srv_lock_);
            
            if(!feed_daily_stat_.empty())
            {
              feed_daily_stat.splice(feed_daily_stat.begin(),
                                     feed_daily_stat_,
                                     feed_daily_stat_.begin(),
                                     feed_daily_stat_.end());
            }
          }

          if(!feed_daily_stat.empty())
          {
              
            try
            {
              FeedDailyStatList::iterator it;
              
              while((it = feed_daily_stat.begin()) != feed_daily_stat.end())
              {
                ::NewsGate::RSS::Transport::FeedsStatisticsImpl::Var st =
                  new ::NewsGate::RSS::Transport::FeedsStatisticsImpl::Type(
                    new ::NewsGate::RSS::Transport::FeedsStatistics());

                ::NewsGate::RSS::Transport::FeedsStatistics& state =
                    st->entity();

                state.date = ACE_Time_Value(it->date).sec();

                FeedStatTable& feed_stat = it->feed_stat;

                size_t len = feed_stat.size() <
                  saving_feed_stat_config_.packet_size() ?
                  feed_stat.size() : saving_feed_stat_config_.packet_size();

                ::NewsGate::RSS::Transport::FeedStatArray& feed_stat_seq =
                    state.feeds_stat;
                
                feed_stat_seq.resize(len);

                typedef std::vector< ::NewsGate::Feed::Id > FeedIdArray;
                FeedIdArray feed_stat_ids;

                bool whole_package_sent = feed_stat.size() == len;

                if(!whole_package_sent)
                {
                  feed_stat_ids.reserve(len);
                }
                
                size_t i = 0;
                
                for(FeedStatTable::iterator fs_it = feed_stat.begin();
                    i < len; fs_it++, i++)
                {
                  feed_stat_seq[i] = fs_it->second;
                  
                  if(!whole_package_sent)
                  {
                    feed_stat_ids.push_back(feed_stat_seq[i].id);
                  }
                }
                
                puller_manager->feed_stat(session, st.in());

                if(whole_package_sent)
                {
                  feed_daily_stat.erase(it);
                }
                else
                {
                  for(FeedIdArray::const_iterator it = feed_stat_ids.begin();
                      it != feed_stat_ids.end(); it++)
                  {
                    feed_stat.erase(*it);
                  }
/*                  
                  //
                  // Deserializing required, this why feed_stat_seq reaquired 
                  //
                  ::NewsGate::RSS::Transport::FeedStatArray& feed_stat_seq =
                      st->entity().feeds_stat;
                  
                  for(size_t i = 0; i < len; i++)
                  {
                    feed_stat.erase(feed_stat_seq[i].id);                 
                  }
*/
                }

                //
                // Unusable at this point
                //
                unlink(saving_feed_stat_config_.cache_file().c_str());      
              }
            }
            catch(...)
            {
              WriteGuard guard(srv_lock_);
            
              feed_daily_stat_.splice(feed_daily_stat_.begin(),
                                      feed_daily_stat,
                                      feed_daily_stat.begin(),
                                      feed_daily_stat.end());

              next_save_time_shift = saving_feed_stat_config_.retry_timeout();
              
              throw;
            }
          }
          
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "PullingFeeds::push_feed_stat: "
            "El::Exception caught. Description:" << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(const NewsGate::RSS::PullerManager::Logout& e)
        {  
          if(started())
          {
            callback_->initiate_logout();
          }
          
          return;
        }
        catch(const CORBA::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "PullingFeeds::push_feed_stat: "
            "CORBA::Exception caught. Description:" << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }

        try
        {
          El::Service::CompoundServiceMessage_var msg = new PushFeedStat(this);

          ACE_Time_Value next_save_time;

          if(next_save_time_shift != ACE_Time_Value::zero)
          {
            //
            // Couldn't send to puller manager; need to resend later
            //
            next_save_time = now + next_save_time_shift;
          }
          else
          {
            feed_stat_last_save_ = now;

            next_save_time =
              now + ACE_Time_Value(saving_feed_stat_config_.period());
          }

          if(El::Moment(next_save_time).date() > El::Moment(now).date())
          {
            El::Moment tomorrow = El::Moment(El::Moment(now).date() +
                                             ACE_Time_Value(86400)).date();

            tomorrow.set_time_iso8601(
              saving_feed_stat_config_.flash_prev_day().c_str());

            next_save_time = tomorrow;
          }

          if(Application::will_trace(El::Logging::LOW))
          {
            std::ostringstream ostr;
            ostr << "PullingFeeds::push_feed_stat: scheduling next stat save "
              "at " << El::Moment(next_save_time).iso8601();
            
            Application::logger()->trace(ostr.str(),
                                         Aspect::PULLING_FEEDS,
                                         El::Logging::LOW);
          }
          
          deliver_at_time(msg.in(), next_save_time);
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "PullingFeeds::push_feed_state: "
            "El::Exception caught while scheduling task. Description:"
               << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        
      }
      catch(...)
      {
        El::Service::Error error(
          "PullingFeeds::push_feed_state: unexpected exception caught.",
          this);
        
        callback_->notify(&error);
      }    
      
    }

    void
    PullingFeeds::request_feed(Feed::Id id,
                               uint64_t feed_info_update_number) throw()
    {
      try
      {
        Feed::Id feed_id = 0;
        FeedInfo feed_info;
        bool request_feeds = false;
        
        try
        {      
          {
            ReadGuard guard(srv_lock_);
      
            FeedInfoTable::iterator it = feeds_.find(id);

            if(it == feeds_.end() ||
               it->second.update_number != feed_info_update_number)
            {
              return;
            }

            request_feeds = request_feeds_;
            feed_info = it->second;
            feed_id = it->first;
          }

          if(request_feeds)
          {
            HTTPInfo http_info;
            FeedRequestInfo request_info(feed_id);

            if(feed_info.type == ::NewsGate::Feed::TP_HTML)
            {
              process_html_feed(feed_id, feed_info);
            }
            else
            {
              RSS::ParserPtr parser(
                pull_feed(feed_info, request_info, http_info));
              
              process_channel(feed_id,
                              feed_info,
                              http_info,
                              parser->rss_channel(),
                              request_info);
            }
          }
          
        }
        catch(const ServiceStopped& e)
        {
          std::ostringstream ostr;
          ostr << "PullingFeeds::request_feed: ServiceStopped caught. "
            "Description:\n" << e;
          
          Application::logger()->trace(ostr.str(),
                                       Aspect::PULLING_FEEDS,
                                       El::Logging::LOW);
          
          return;
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "PullingFeeds::request_feed: "
            "El::Exception caught while requesting feed '"
               << feed_info.url << "'. Description:" << std::endl << e;
          
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }

        if(feed_id)
        {
          try
          {
            WriteGuard guard(srv_lock_);
          
            FeedInfoTable::iterator it = feeds_.find(feed_id);

            if(it != feeds_.end())
            {
              schedule_request(
                it->second,
                request_feeds ? ST_REGULAR : ST_SKIPPED_REQUEST);
            }
          }
          catch(const El::Exception& e)
          {
            std::ostringstream ostr;
            ostr << "PullingFeeds::request_feed: "
              "El::Exception caught while rescheduling the task. Description:"
                 << std::endl << e;
      
            El::Service::Error error(ostr.str(), this);
            callback_->notify(&error);
          }
        }
        
      }
      catch(...)
      {
        El::Service::Error error(
          "PullingFeeds::request_feed: unexpected exception caught.",
          this);

        callback_->notify(&error);
      }    
    }  

    El::Net::HTTP::Session*
    PullingFeeds::start_session(const char* s_url,
                                const FeedInfo& feed_info,
                                FeedRequestInfo& request_info,
                                HTTPInfo& http_info,
                                FeedRequestInterceptor& interceptor,
                                OStringSteamPtr& request_trace_stream,
                                OStringSteamPtr& response_trace_stream)
      throw(Exception, El::Exception)
    {
      El::Net::HTTP::URL_var url = new El::Net::HTTP::URL(s_url);
      
      HTTPSessionPtr session(
        new El::Net::HTTP::Session(url,
                                   El::Net::HTTP::HTTP_1_1,
                                   &interceptor));
      
      El::Net::HTTP::HeaderList headers;
              
      const char* last_modified_hdr =
        feed_info.state.http.last_modified_hdr.c_str();
        
      if(*last_modified_hdr != '\0')
      {
        headers.add(El::Net::HTTP::HD_IF_MODIFIED_SINCE,
                    last_modified_hdr);
      }
          
      const char* etag_hdr = feed_info.state.http.etag_hdr.c_str();
                
      if(*etag_hdr != '\0')
      {
        headers.add(El::Net::HTTP::HD_IF_NONE_MATCH, etag_hdr);
      }

      // text/html;q=0.1, added to fit some sick wer servers like http://www.intermoda.ru/rss/news
      // read NG-141 for more details
      headers.add(
        El::Net::HTTP::HD_ACCEPT,
        "text/html;q=0.1,text/xml,application/xml,application/atom+xml,"
        "application/rss+xml,application/rdf+xml,*/*;q=0.1");

      headers.add(El::Net::HTTP::HD_ACCEPT_ENCODING, "gzip,deflate");
      headers.add(El::Net::HTTP::HD_ACCEPT_LANGUAGE, "en-us,*");
      
      headers.add(El::Net::HTTP::HD_USER_AGENT,
                  feed_request_config_.user_agent().c_str());

      ACE_Time_Value timeout(feed_request_config_.xml_feed_timeout());

      {
        ACE_High_Res_Timer timer;
        timer.start();
            
        session->open(&timeout,
                      &timeout,
                      &timeout,
                      1024,
                      feed_request_config_.stream_recv_buf_size());

        if(Application::will_trace(El::Logging::LOW))
        {
          request_trace_stream.reset(new std::ostringstream);          
          session->debug_ostream(request_trace_stream.get());

          response_trace_stream.reset(new std::ostringstream);
          session->debug_istream(response_trace_stream.get());
        }
        
        std::string new_permanent_url =
          session->send_request(El::Net::HTTP::GET,
                                El::Net::HTTP::ParamList(),
                                headers,
                                0,
                                0,
                                feed_request_config_.redirects_to_follow());
              
        timer.stop();

        ACE_Time_Value tm;
        timer.elapsed_time(tm);

        if(!new_permanent_url.empty())
        {
          http_info.new_location = new_permanent_url;
          http_info.new_location_permanent = true;
        }
        else if(strcmp(session->url()->string(), url->string()))
        {
          http_info.new_location = session->url()->string();
          http_info.new_location_permanent = false;
        }
              
        request_info.duration += tm;
        request_info.outbound_bytes += session->sent_bytes(true);
      }
            
      request_info.time = ACE_OS::gettimeofday();

      {
        ACE_High_Res_Timer timer;
        timer.start();
            
        session->recv_response_status();

        timer.stop();

        ACE_Time_Value tm;
        timer.elapsed_time(tm);
        request_info.duration += tm;
      }
      
      request_info.inbound_bytes += session->received_bytes(true);
      http_info.status_code = session->status_code();

      if(session->status_code() == El::Net::HTTP::SC_NOT_MODIFIED)
      {
        request_info.result = FeedRequestInfo::RT_NOT_MODIFIED;
        return 0;
      }

      if(session->status_code() != El::Net::HTTP::SC_OK)
      {
        std::ostringstream ostr;
        ostr << "PullingFeeds::start_session: bad status "
             << session->status_code() << ", " << session->status_text();

        throw Exception(ostr.str());
      }
              
      ACE_High_Res_Timer timer;
      timer.start();

      El::Net::HTTP::Header header;              
      while(session->recv_response_header(header))
      {
        const char* name = header.name.c_str();
        
        request_info.inbound_bytes += session->received_bytes(true);
        
        if(!strcasecmp(name, El::Net::HTTP::HD_LAST_MODIFIED))
        {
          if(!header.value.empty())
          {
            http_info.last_modified_hdr = header.value;
          }
        }
        else if(!strcasecmp(name, El::Net::HTTP::HD_ETAG))
        {
          if(!header.value.empty())
          {
            http_info.etag_hdr = header.value;
          }
        }
        else if(strcasecmp(name, El::Net::HTTP::HD_CONTENT_TYPE) == 0)
        {
          El::Net::HTTP::content_type(header.value.c_str(), http_info.charset);
        }      
      }

      return session.release();
    }

    void
    PullingFeeds::process_html_feed(Feed::Id feed_id,
                                    const FeedInfo& feed_info)
      throw(ServiceStopped, Exception, El::Exception)
    {
      if(!started())
      {
        throw
          ServiceStopped("PullingFeeds::process_html_feed: shutting down");
      }
      
      if(Application::will_trace(El::Logging::HIGH))
      {
        std::ostringstream ostr;
        ostr << "PullingFeeds::process_html_feed: id=" << feed_id << ", url="
             << feed_info.url << ": processing ...";
        
        Application::logger()->trace(ostr.str(),
                                     Aspect::PULLING_FEEDS,
                                     El::Logging::HIGH);
      }

      FeedInfo new_feed_info = feed_info;

      FeedRequestInfo request_info(feed_id);
      
      request_info.result = FeedRequestInfo::RT_FAILED;
      request_info.time = ACE_OS::gettimeofday();

      Transport::FeedStateUpdate state_update;
      state_update.id = feed_id;

      size_t prev_cache_size = 0;
      bool not_modified = false;
      size_t old_msg_count = 0;
      bool indexing_forbidden = false;

      Message::Transport::RawMessagePackImpl::Var message_pack;
      
      RSS::HTMLFeed::Context ctx;
      
      try
      {
        HTTPInfo http_info;        
        
        FeedRequestInterceptor interceptor;
        OStringSteamPtr request_trace_stream;
        OStringSteamPtr response_trace_stream;

        HTTPSessionPtr session(
          start_session(feed_info.url.c_str(),
                        feed_info,
                        request_info,
                        http_info,
                        interceptor,
                        request_trace_stream,
                        response_trace_stream));

        if(session.get() == 0)
        {
          not_modified = true;
          request_info.result = FeedRequestInfo::RT_NOT_MODIFIED;

          uint64_t last_request_date = request_info.time.sec();
          
          new_feed_info.state.last_request_date = last_request_date;
          state_update.set_last_request_date(last_request_date);
        }
        else
        {
          if(feed_request_config_.full_article().obey_robots_txt() &&
             !Application::instance()->robots_checker().allowed(
               session->url(),
               feed_request_config_.user_agent().c_str()))
          {
            session.reset(0);
            indexing_forbidden = true;

            uint64_t last_request_date = request_info.time.sec();
          
            new_feed_info.state.last_request_date = last_request_date;
            state_update.set_last_request_date(last_request_date);
          }
          else
          {
            ctx.fill(feed_info.url.c_str(),
                     feed_info.space,
                     feed_info.country,
                     feed_info.lang,
                     feed_info.keywords,
                     &address_info_,
                     ACE_Time_Value(feed_request_config_.html_feed_timeout()),
                     feed_request_config_.user_agent().c_str(),
                     feed_request_config_.redirects_to_follow(),
                     feed_request_config_.image().referer().c_str(),
                     feed_request_config_.socket_recv_buf_size(),
                     feed_request_config_.image().min_width(),
                     feed_request_config_.image().min_height(),
                     image_prefix_blacklist_,
                     image_extension_whitelist_,
                     feed_request_config_.limits().message_title(),
                     feed_request_config_.limits().message_description(),
                     feed_request_config_.limits().message_description_chars(),
                     feed_request_config_.limits().max_image_count(),
                     feed_request_config_.full_article().max_size(),
                     feed_request_config_.cache_dir().c_str(),
                     feed_info.state.cache.c_str(),
                     feed_request_config_.html_cache().max_size(),
                     feed_request_config_.html_cache().timeout(),
                     feed_info.encoding.c_str());

            prev_cache_size = ctx.cache.size();
        
            RSS::HTMLFeed::Code code;

            const std::string& script = feed_info.adjustment_script;

            std::string trimmed;
            El::String::Manip::trim(script.c_str(), trimmed);
        
            if(trimmed.empty())
            {
              throw Exception("No script provided");
            }
        
            code.compile(script.c_str(), feed_id, feed_info.url.c_str());
        
            std::auto_ptr<El::Python::Sandbox> sandbox(
              Config::create_python_sandbox(
                feed_request_config_.python().sandbox()));

            ACE_High_Res_Timer timer;
            timer.start();
          
            try
            {
              code.run(ctx,
                       session.release(),
                       sandbox_service_.in(),
                       sandbox.get(),
                       Application::logger());

              new_feed_info.state.http.last_modified_hdr =
                http_info.last_modified_hdr;
          
              new_feed_info.state.http.etag_hdr = http_info.etag_hdr;

              if(new_feed_info.state.http.last_modified_hdr !=
                 feed_info.state.http.last_modified_hdr)
              {
                state_update.http_last_modified_hdr(
                  new_feed_info.state.http.last_modified_hdr.c_str());
              }
              
              if(new_feed_info.state.http.etag_hdr !=
                 feed_info.state.http.etag_hdr)
              {
                state_update.http_etag_hdr(
                  new_feed_info.state.http.etag_hdr.c_str());
              }            
            }
            catch(const El::Exception& e)
            {
              if(ctx.interrupted)
              {
                std::ostringstream ostr;
                ostr << "PullingFeeds::process_html_feed: processing "
                  "interrupted (feed " << feed_id << " " << feed_info.url
                     << "). Description:\n" << e;
              
                Application::logger()->trace(ostr.str(),
                                             Aspect::PULLING_FEEDS,
                                             El::Logging::LOW);
              }
              else
              {
                throw;
              }
            }

            timer.stop();

            ACE_Time_Value tm;
            timer.elapsed_time(tm);

            request_info.inbound_bytes += ctx.inbound_bytes;
            request_info.outbound_bytes += ctx.outbound_bytes;            
            request_info.duration += tm;
            
            new_feed_info.state.cache = ctx.cache.stringify();
      
            if(new_feed_info.state.cache != feed_info.state.cache)
            {
              state_update.set_cache(new_feed_info.state.cache);
            }

            uint64_t last_request_date = request_info.time.sec();
      
            uint64_t published_threshold =
              last_request_date - saving_feed_state_config_.message_timeout();

            Message::LocalCodeArray& new_messages =
              new_feed_info.last_messages;

            new_feed_info.state.last_request_date = last_request_date;
            state_update.set_last_request_date(last_request_date);      

            for(Message::Automation::MessageArray::iterator
                  i(ctx.messages.begin()), e(ctx.messages.end()); i != e; ++i)
            {
              Message::Automation::Message& msg = *i;

              Message::LocalCode msg_code;

              El::CRC(msg_code.id,
                      (const unsigned char*)msg.url.c_str(),
                      msg.url.size());
        
              msg_code.published = last_request_date;

              Message::LocalCodeArray::iterator nm(new_messages.begin());
              Message::LocalCodeArray::iterator nme(new_messages.end());

              for(; nm != nme && nm->id != msg_code.id; ++nm);

              if(nm != nme)
              {
                ++old_msg_count;

                new_messages.erase(nm);
                new_messages.insert(new_messages.begin(), msg_code);
          
                if(Application::will_trace(El::Logging::HIGH))
                {
                  std::ostringstream ostr;
                  ostr << "PullingFeeds::process_html_feed: old "
                    "message: msg_code=" << msg_code.string() << ", title='"
                       << msg.title << "', url=" << msg.url << ", feed "
                       << feed_id << " " << feed_info.url;
            
                  Application::logger()->trace(ostr.str(),
                                               Aspect::PULLING_FEEDS,
                                               El::Logging::HIGH);
                }
          
                continue;
              }
        
              if(Application::will_trace(El::Logging::HIGH))
              {
                std::ostringstream ostr;
                ostr << "PullingFeeds::process_html_feed: new "
                  "message: msg_code=" << msg_code.string() << ", title='"
                     << msg.title << "', url=" << msg.url << ", feed "
                     << feed_id << " " << feed_info.url;
          
                if(!msg.valid)
                {
                  ostr << ";\nmessage is not valid, reason: " << msg.log;
                }
          
                Application::logger()->trace(ostr.str(),
                                             Aspect::PULLING_FEEDS,
                                             El::Logging::HIGH);
              }

              if(msg.valid)
              {                
                post_message(msg, msg_code, feed_id, feed_info, message_pack);

                request_info.messages++;

                std::string msg_pub_date = msg_code.pub_date().rfc0822();
                
                request_info.messages_size += msg.title.length() +
                  msg.description.length() + msg.url.length() +
                  msg_pub_date.length();
                
                new_messages.insert(new_messages.begin(), msg_code);
                state_update.new_messages.push_back(msg_code);
          
                state_update.updated_fields |=
                  Transport::FeedStateUpdate::UF_NEW_MESSAGES;

                Transport::ChannelState& cs = new_feed_info.state.channel;
                const Message::Automation::Source& src = msg.source;
                
                if(cs.title != src.title && !src.title.empty())
                {
                  cs.title = src.title;
                  state_update.channel_title(cs.title.c_str());
                }

                if(cs.html_link != src.html_link && !src.html_link.empty())
                {
                  cs.html_link = src.html_link;
                  state_update.channel_html_link(cs.html_link.c_str());
                }
              }
            }

            if(request_info.messages)
            {
              request_info.result = FeedRequestInfo::RT_PARSED_TO_THE_END;
              http_info.feed_size = request_info.messages;
            }
            else
            {
              request_info.result = FeedRequestInfo::RT_WASTED;
              request_info.wasted = 1;
            }
            
            for(Message::LocalCodeArray::iterator i(new_messages.begin());
                i != new_messages.end(); )
            {
              if(i->published < published_threshold)
              {
                state_update.expired_messages.push_back(i->id);
          
                state_update.updated_fields |=
                  Transport::FeedStateUpdate::UF_EXPIRED_MESSAGES;

                i = new_messages.erase(i);
              }
              else
              {
                ++i;
              }
            }
          }

          if(http_info.new_location_permanent)
          {
            new_feed_info.url = http_info.new_location;
            state_update.http_new_location(http_info.new_location.c_str());
          }

          size_t new_size =
            std::max(http_info.feed_size, new_feed_info.state.size);
          
          if(new_feed_info.state.size != new_size)
          {
            new_feed_info.state.size = new_size;
            state_update.set_size(new_size);
          }
        }
      }
      catch(const El::Exception& e)
      {
        uint64_t last_request_date = request_info.time.sec();
        
        new_feed_info.state.last_request_date = last_request_date;
        state_update.set_last_request_date(last_request_date);
        
        
        std::ostringstream ostr;
        ostr << "PullingFeeds::process_html_feed: processing failed (feed "
             << feed_id << " " << feed_info.url << "). Error description:\n"
             << e;
        
        Application::logger()->trace(ostr.str(),
                                     Aspect::PULLING_FEEDS,
                                     El::Logging::LOW);
      }
      
      if(Application::will_trace(El::Logging::LOW))
      {
        std::ostringstream ostr;
        ostr << "PullingFeeds::process_html_feed: id=" << feed_id << ", url="
             << feed_info.url << ": ";

        if(not_modified)
        {
          ostr << "page not modified";
        }
        else if(indexing_forbidden)
        {
          ostr << "indexing forbidden";
        }
        else
        {        
          ostr << request_info.messages << " valid, " << old_msg_count
               << " old messages from " << ctx.messages.size()
               << "; cache size " << ctx.cache.size() << " ("
               << new_feed_info.state.cache.size() << "), prev "
               << prev_cache_size << " (" << feed_info.state.cache.size()
               << ")";
        }
        
        Application::logger()->trace(ostr.str(),
                                     Aspect::PULLING_FEEDS,
                                     El::Logging::LOW);
      }
      
      set_feed_request_result(feed_id,
                              feed_info.url.c_str(),
                              new_feed_info,
                              message_pack,
                              state_update,
                              request_info);
    }
    
    RSS::Parser*
    PullingFeeds::pull_feed(const FeedInfo& feed_info,
                            FeedRequestInfo& request_info,
                            HTTPInfo& http_info)
      throw(ServiceStopped, Exception, El::Exception)
    {
      if(feed_info.encoding.empty())
      {
        try
        {
          return pull_feed(feed_info, true, request_info, http_info, 0);
        }
        catch(const RSS::Parser::EncodingError& e)
        {
          // Will try without HTTP-level encoding specification
        }

        return pull_feed(feed_info, false, request_info, http_info, 0);
      }
      else
      {
        return pull_feed(feed_info,
                         false,
                         request_info,
                         http_info,
                         feed_info.encoding.c_str());
      }
    }
    
    RSS::Parser*
    PullingFeeds::pull_feed(const FeedInfo& feed_info,
                            bool use_http_charset,
                            FeedRequestInfo& request_info,
                            HTTPInfo& http_info,
                            const char* encoding)
      throw(ServiceStopped,
            RSS::Parser::EncodingError,
            Exception,
            El::Exception)
    {
      FeedRequestInterceptor interceptor;
            
      RSS::ParserPtr parser(
        RSS::ParserFactory::create(Feed::TP_UNDEFINED, &interceptor));

      const std::string& url = feed_info.url;

      http_info = HTTPInfo();
      http_info.url = url;

      bool check_last_build_date_as_heuristics =
        saving_traffic_config_.last_build_based() > 1;
      
      OStringSteamPtr request_trace_stream;
      OStringSteamPtr response_trace_stream;
          
      El::Moment channel_last_build_date;

      if(feed_info.state.channel.last_build_date)
      {
        channel_last_build_date =
          ACE_Time_Value(feed_info.state.channel.last_build_date);

        interceptor.recv_buffer_size =
          feed_request_config_.socket_recv_buf_size();
      }
          
      try
      {
        HTTPSessionPtr session(
          start_session(url.c_str(),
                        feed_info,
                        request_info,
                        http_info,
                        interceptor,
                        request_trace_stream,
                        response_trace_stream));

        if(session.get() != 0)
        {
          http_info.content_length_hdr = session->content_length();

          if(saving_traffic_config_.last_build_based() == 1)
          {
            interceptor.last_build_date = channel_last_build_date;
          }
              
          if(feed_info.state.heuristics_counter >= 0 &&
             (unsigned long)feed_info.state.heuristics_counter <
             saving_traffic_config_.heuristics().sequence_len())
          {
            //
            // Apply heuristics if required
            //
            
            if(check_last_build_date_as_heuristics)
            {
              interceptor.last_build_date = channel_last_build_date;
            }
                
            if(saving_traffic_config_.heuristics().content_length_based() &&
               http_info.content_length_hdr > 0 &&
               feed_info.state.http.content_length_hdr > 0)
            {
              interceptor.content_length = http_info.content_length_hdr;
                  
              interceptor.prev_content_length =
                feed_info.state.http.content_length_hdr;
            }

            if(saving_traffic_config_.heuristics().single_chunk_based() &&
               feed_info.state.http.single_chunked > 0 &&
               feed_info.state.http.first_chunk_size > 0)
            {
              interceptor.prev_first_chunk_size =
                feed_info.state.http.first_chunk_size;
            }

            if(saving_traffic_config_.heuristics().ordering_based())
            {
              interceptor.old_messages_limit = feed_info.state.entropy;
              interceptor.last_messages = &feed_info.last_messages;
            }
          }
              
          ACE_High_Res_Timer timer;
          timer.start();
          
          try
          {
            std::auto_ptr<El::XML::EntityResolver> entity_resolver;

            if(feed_request_config_.entity_resolver().present())
            {
              entity_resolver.reset(
                NewsGate::Config::create_entity_resolver(
                  *feed_request_config_.entity_resolver()));
            }
            
            parser->parse(session->response_body(),
                          session->url()->string(),
                          encoding && *encoding != '\0' ? encoding :
                          (use_http_charset ? http_info.charset.c_str() : 0),
                          feed_request_config_.stream_recv_buf_size(),
                          false,
                          entity_resolver.get());
                
            request_info.result = FeedRequestInfo::RT_PARSED_TO_THE_END;
            http_info.feed_size = parser->rss_channel().items.size();
          }
          catch(const ServiceStopped&)
          {
            throw;
          }
          catch(const FeedRequestInterceptor::ParseInterrupted& e)
          {
            if(dynamic_cast<
               const FeedRequestInterceptor::SameLastBuildDate*>(&e) != 0)
            {
              if(check_last_build_date_as_heuristics)
              {
                http_info.heuristics_applied = true;
              }
              
              request_info.result = FeedRequestInfo::RT_UNCHANGED;
            }
            else
            {
              http_info.heuristics_applied = true;

              if(dynamic_cast<const
                 FeedRequestInterceptor::OldMessagesLimitReached*>(&e) != 0)
              {
                if(interceptor.new_messages > 0 &&
                   interceptor.new_messages + interceptor.old_messages <
                   feed_info.state.size)
                {
                  request_info.result = FeedRequestInfo::RT_HAS_CHANGES;
                }
                else
                {
                  //
                  // Request wasted as a fraction of 1
                  //
                  request_info.result = FeedRequestInfo::RT_WASTED;
                }
              }
              else // SameContentLength, SameFirstChunkSize
              { 
                request_info.result = FeedRequestInfo::RT_PRESUMABLY_UNCHANGED;
              }
            }
                
            if(Application::will_trace(El::Logging::HIGH))
            {
              std::ostringstream ostr;
              ostr << "PullingFeeds::pull_feed: id=" << feed_info.state.id
                   << ", parse interrupted: " << e;

              Application::logger()->trace(ostr.str(),
                                           Aspect::PULLING_FEEDS,
                                           El::Logging::HIGH);
            }
          }

          timer.stop();

          ACE_Time_Value tm;
          timer.elapsed_time(tm);
              
          request_info.duration += tm;
          request_info.inbound_bytes += session->received_bytes(true);

          El::Geography::AddressInfo* address_info = 0;
          
          switch(request_info.result)
          {
          case FeedRequestInfo::RT_PARSED_TO_THE_END:
          case FeedRequestInfo::RT_HAS_CHANGES:
          case FeedRequestInfo::RT_WASTED:
            {
              address_info = &address_info_;
              break;
            }
          default: break;
          }
          
          parser->rss_channel().adjust(
            http_info.new_location_permanent ?
            http_info.new_location.c_str() : http_info.url.c_str(),
            session->url()->string(),
            feed_info.country,
            feed_info.lang,
            address_info);

          if(!http_info.heuristics_applied)
          {
            if(interceptor.chunks > 0)
            {
              http_info.single_chunked = interceptor.chunks == 1;
                  
              if(http_info.single_chunked)
              {
                http_info.first_chunk_size = interceptor.first_chunk_size;
              }
            }
          }              
        }
      }
      catch(const El::Exception& e)
      {
        if(use_http_charset &&
           dynamic_cast<const RSS::Parser::EncodingError*>(&e) != 0 &&
           !http_info.charset.empty())
        {
          throw;
        }
          
        if(dynamic_cast<const El::Net::HTTP::Timeout*>(&e) != 0 ||
           dynamic_cast<const El::Net::Socket::Stream::Timeout*>(&e) != 0)
        {
          request_info.duration +=
            ACE_Time_Value(feed_request_config_.xml_feed_timeout());
        }
            
        if(request_info.time == ACE_Time_Value::zero)
        {
          request_info.time = ACE_OS::gettimeofday();
        }

        request_info.result = FeedRequestInfo::RT_FAILED;

        if(Application::will_trace(El::Logging::LOW))
        {
          http_info.fail_reason = e.what();
        }
      }

      unsigned long log_level =
        request_info.result == FeedRequestInfo::RT_FAILED ?
        El::Logging::LOW : El::Logging::HIGH;

      if(Application::will_trace(log_level))
      {
        std::ostringstream ostr;
        ostr << "PullingFeeds::pull_feed: id=" << feed_info.state.id
             << " (" << request_info.result_str()
             << "), url=" << url << ", thread 0x" << std::hex
             << pthread_self() << std::dec << std::endl;

        if(!http_info.fail_reason.empty())
        {
          ostr << "Fail Reason:\n" << http_info.fail_reason
               << std::endl;
        }
            
        if(request_trace_stream.get())
        {
          ostr << "Request:\n" << request_trace_stream->str() << std::endl;
        }

        if(response_trace_stream.get())
        {
          ostr << "Response:\n" << response_trace_stream->str() << std::endl;
        }

        if(request_info.parsed())
        {
          ostr << "Channel:\n";
          parser->rss_channel().dump(ostr);
        }
              
        Application::logger()->trace(ostr.str(),
                                     Aspect::PULLING_FEEDS,
                                     log_level);
      }

      return parser.release();
    }
  
    struct MessagePresence
    {
      Message::LocalId id;
      bool present;
      
      MessagePresence(const Message::LocalId& val) throw()
          : id(val), present(false) {}
    };
        
    void
    PullingFeeds::process_channel(Feed::Id feed_id,
                                  const FeedInfo& current_feed_info,
                                  HTTPInfo& http_info,
                                  Channel& channel,
                                  FeedRequestInfo& request_info)
      throw(ServiceStopped, Exception, El::Exception)
    {      
      
      FeedInfo feed_info;
      
      {
        WriteGuard guard(srv_lock_);

        FeedInfoTable::iterator fit = feeds_.find(feed_id);

        if(fit == feeds_.end() || fit->second.url != current_feed_info.url)
        {
          //
          // If feed do not exist anymore (disabled) or url have changed
          // then no need to process request result
          //
          return;
        }

        feed_info = fit->second;
      }

      Transport::FeedStateUpdate state_update;
      Message::Transport::RawMessagePackImpl::Var message_pack;

      state_update.id = feed_id;

      Transport::FeedState& current_state = feed_info.state;
      Transport::ChannelState& current_channel_state = current_state.channel;
      Transport::HTTPState& current_http_state = current_state.http;

      if(channel.type != Feed::TP_UNDEFINED &&
         channel.type != current_channel_state.type)
      {
        state_update.channel_type(channel.type);
        current_channel_state.type = channel.type;
      }

      try
      {
        std::wstring title;
        El::String::Manip::utf8_to_wchar(channel.title.c_str(), title);
            
        El::HTML::LightParser parser;
        
        parser.parse(title.c_str(),
                     http_info.url.c_str(),
                     El::HTML::LightParser::PF_LAX,
                     feed_request_config_.limits().channel_title());

        El::String::Manip::compact(parser.text.c_str(),
                                   channel.title);
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
          
        ostr << "PullingFeeds::process_channel: "
          "title parsing error:\n" << e
             << "'\nTitle: '" << channel.title
             << "'\nDescription: '" << channel.description << "'";
          
        Application::logger()->trace(ostr.str(),
                                     Aspect::PULLING_FEEDS,
                                     El::Logging::HIGH);

        channel.title.clear();
      }

      if(request_info.result == FeedRequestInfo::RT_PARSED_TO_THE_END &&
         channel.title != current_channel_state.title)
      {
        state_update.channel_title(channel.title.c_str());
        current_channel_state.title = channel.title;
      }
        
      try
      {
        std::wstring desc;
        El::String::Manip::utf8_to_wchar(channel.description.c_str(), desc);
            
        El::HTML::LightParser parser;
        
        parser.parse(
          desc.c_str(),
          http_info.url.c_str(),
          El::HTML::LightParser::PF_LAX,
          feed_request_config_.limits().channel_description());
        
        El::String::Manip::compact(parser.text.c_str(),
                                   channel.description);
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
          
        ostr << "PullingFeeds::process_channel: "
          "description parsing error:\n" << e
             << "'\nTitle: '" << channel.title
             << "'\nDescription: '" << channel.description << "'";
          
        Application::logger()->trace(ostr.str(),
                                     Aspect::PULLING_FEEDS,
                                     El::Logging::HIGH);

        channel.description.clear();
      }

      unsigned long channel_last_build_sec =
        ACE_Time_Value(channel.last_build_date).sec();
              
      if(request_info.result == FeedRequestInfo::RT_PARSED_TO_THE_END)
      {
        if(channel.description != current_channel_state.description)
        {
          state_update.channel_description(channel.description.c_str());
          current_channel_state.description = channel.description;
        }

        if(channel.html_link != current_channel_state.html_link)
        {
          state_update.channel_html_link(channel.html_link.c_str());
          current_channel_state.html_link = channel.html_link;
        }

        if(channel.lang.el_code() != current_channel_state.lang)
        {
          state_update.channel_lang(channel.lang.el_code());
          current_channel_state.lang = channel.lang.el_code();
        }

        if(channel.country.el_code() != current_channel_state.country)
        {
          state_update.channel_country(channel.country.el_code());
          current_channel_state.country = channel.country.el_code();
        }

        if(channel.ttl != current_channel_state.ttl)
        {
          state_update.channel_ttl(channel.ttl);
          current_channel_state.ttl = channel.ttl;
        }

        if(channel_last_build_sec != current_channel_state.last_build_date)
        {
          state_update.channel_last_build_date(channel_last_build_sec);
          current_channel_state.last_build_date = channel_last_build_sec;
        }   
      }

      if(request_info.parsed() && http_info.last_modified_hdr !=
         current_http_state.last_modified_hdr)
      {
        state_update.http_last_modified_hdr(
          http_info.last_modified_hdr.c_str());

        current_http_state.last_modified_hdr =
          http_info.last_modified_hdr.c_str();
      }

      if(request_info.parsed() && http_info.etag_hdr !=
         current_http_state.etag_hdr)
      {
        state_update.http_etag_hdr(http_info.etag_hdr.c_str());
        current_http_state.etag_hdr = http_info.etag_hdr.c_str();
      }

      if(request_info.parsed() &&
         http_info.content_length_hdr != current_http_state.content_length_hdr)
      {
        state_update.http_content_length_hdr(http_info.content_length_hdr);
        current_http_state.content_length_hdr = http_info.content_length_hdr;
      }

      if(http_info.new_location_permanent &&
         request_info.result != FeedRequestInfo::RT_FAILED)
      {
        feed_info.url = http_info.new_location;
        state_update.http_new_location(http_info.new_location.c_str());
      }

      if(request_info.result == FeedRequestInfo::RT_PARSED_TO_THE_END)
      {
        if(http_info.single_chunked >= 0 &&
           http_info.single_chunked != current_http_state.single_chunked)
        {
          char single_chunked =
            http_info.single_chunked && !current_http_state.single_chunked ?
            0 : http_info.single_chunked;
          
          state_update.http_single_chunked(single_chunked);
          current_http_state.single_chunked = single_chunked;
        }

        if(http_info.first_chunk_size != current_http_state.first_chunk_size)
        {
          state_update.http_first_chunk_size(http_info.first_chunk_size);
          current_http_state.first_chunk_size = http_info.first_chunk_size;
        }

        if(http_info.feed_size != current_state.size)
        {
          state_update.set_size(http_info.feed_size);
          current_state.size = http_info.feed_size;
        }
      }
      
      if(request_info.result == FeedRequestInfo::RT_PARSED_TO_THE_END ||
         request_info.result == FeedRequestInfo::RT_HAS_CHANGES ||
         request_info.result == FeedRequestInfo::RT_WASTED)
      {
        process_messages(channel,
                         state_update,
                         feed_info,
                         request_info,
                         http_info,
                         message_pack);
      }

      long current_heuristics_counter = current_state.heuristics_counter;

      if(current_state.heuristics_counter < 0)
      {
        if(request_info.result == FeedRequestInfo::RT_PARSED_TO_THE_END &&
           request_info.messages > 0)
        {
          current_state.heuristics_counter++;
        }
      }
      else if(http_info.heuristics_applied)
      {
        current_state.heuristics_counter++;
      }
      else if(request_info.parsed())
      {
        current_state.heuristics_counter = 0;
      }

      if(current_heuristics_counter != current_state.heuristics_counter ||
         current_state.last_request_date == 0)
      {
        state_update.set_heuristics_counter(current_state.heuristics_counter);
      }      

      if(http_info.entropy && http_info.entropy >= current_state.entropy)
      {
        state_update.set_entropy_updated_date(request_info.time.sec());        
        current_state.entropy_updated_date = request_info.time.sec();

        if(Application::will_trace(El::Logging::HIGH))
        {
          std::ostringstream ostr;
          ostr << "PullingFeeds::process_channel: id=" << feed_info.state.id
               << ", entropy max reached " << http_info.entropy;

          Application::logger()->trace(ostr.str(),
                                       Aspect::PULLING_FEEDS,
                                       El::Logging::HIGH);
        }
      }

      if(http_info.entropy > current_state.entropy)
      {
        state_update.set_entropy(http_info.entropy);
        current_state.entropy = http_info.entropy;        
      }

      if(saving_traffic_config_.heuristics().force_down_entropy_after() > 0 &&
         request_info.result == FeedRequestInfo::RT_PARSED_TO_THE_END &&
         current_state.entropy > 0 &&
         current_state.heuristics_counter == 0 &&
         request_info.time.sec() -
         (time_t)current_state.entropy_updated_date >=
         (time_t)saving_traffic_config_.heuristics().
         force_down_entropy_after())
      {
        //
        // If much enough time passed from a moment entropy reached it maximum
        // or was forced down last time, then can force down it a bit
        //
        
        current_state.entropy--;        
        state_update.set_entropy(current_state.entropy);

        current_state.entropy_updated_date = request_info.time.sec();
        
        state_update.set_entropy_updated_date(
          current_state.entropy_updated_date);

        if(Application::will_trace(El::Logging::HIGH))
        {
          std::ostringstream ostr;
          ostr << "PullingFeeds::process_channel: id=" << feed_info.state.id
               << ", entropy forced down to " << current_state.entropy;

          Application::logger()->trace(ostr.str(),
                                       Aspect::PULLING_FEEDS,
                                       El::Logging::HIGH);
        }
      }

      state_update.set_last_request_date(request_info.time.sec());
      current_state.last_request_date = request_info.time.sec();

      set_feed_request_result(feed_id,
                              current_feed_info.url.c_str(),
                              feed_info,
                              message_pack,
                              state_update,
                              request_info);
    }

    void
    PullingFeeds::set_feed_request_result(
      Feed::Id feed_id,
      const char* feed_url,
      const FeedInfo& feed_info,
      Message::Transport::RawMessagePackImpl::Var& message_pack,
      const Transport::FeedStateUpdate& state_update,
      const FeedRequestInfo& request_info)
      throw(El::Exception)
    {
      WriteGuard guard(srv_lock_);

      FeedInfoTable::iterator fit = feeds_.find(feed_id);

      if(fit == feeds_.end() || fit->second.url != feed_url)
      {
        //
        // If feed do not exist anymore (disabled) or url have changed
        // then no need to process request result
        //
        return;
      }

      fit->second = feed_info;

      FeedRequestResultPackList::reverse_iterator it;
      
      if(feed_request_results_.empty() ||
         ((it = feed_request_results_.rbegin()))->
         state_update_pack->entities().size() >=
         saving_feed_state_config_.packet_size() ||
         (it->message_pack.in() != 0 && it->message_pack->serialized()))
      {
        feed_request_results_.push_back(FeedRequestResultPack());
        it = feed_request_results_.rbegin();
        
        it->state_update_pack =
          Transport::FeedStateUpdatePackImpl::Init::create(
            new Transport::FeedStateUpdateArray());
      }
            
      it->state_update_pack->entities().push_back(state_update);      
      it->requests_info.push_back(request_info);

      Message::Transport::RawMessagePackImpl::Var& result_message_pack =
        it->message_pack;
      
      if(result_message_pack.in() == 0)
      {
        result_message_pack = message_pack._retn();
      }
      else if(message_pack.in() != 0)
      {
        result_message_pack->entities().insert(
          result_message_pack->entities().end(),
          message_pack->entities().begin(),
          message_pack->entities().end());
      }
    }

    void
    PullingFeeds::process_messages(
      Channel& channel,
      Transport::FeedStateUpdate& state_update,
      FeedInfo& current_feed_info,
      FeedRequestInfo& request_info,
      HTTPInfo& http_info,
      Message::Transport::RawMessagePackImpl::Var& message_pack)
      throw(ServiceStopped, Exception, El::Exception)
    {
      PullerApp* app = Application::instance();
      
      unsigned long oldest_message_time = ACE_OS::gettimeofday().sec() -
        app->config().message_post().message_max_age();
          
      Transport::FeedState& current_state = current_feed_info.state;

      Message::LocalCodeArray& current_state_messages =
        current_feed_info.last_messages;

      //
      // Putting into new list of current messages only those which are
      // not too old
      //
      std::vector<MessagePresence> current_messages;        
      current_messages.reserve(current_state_messages.size());

      for(Message::LocalCodeArray::const_iterator
            it = current_state_messages.begin();
          it != current_state_messages.end(); it++)
      {
        current_messages.push_back(MessagePresence(it->id));
      }
        
      Message::LocalCodeArray new_state_messages;
      new_state_messages.reserve(current_state_messages.size());

      std::vector<MessagePresence>::iterator cit = current_messages.begin();
      
      for(Message::LocalCodeArray::const_iterator
            mit = current_state_messages.begin();
          mit != current_state_messages.end(); mit++, cit++)
      {
        const Message::LocalCode& msg_code = *mit;
        const Message::LocalId& msg_id = msg_code.id;

        ItemList::reverse_iterator it = channel.items.rbegin();
          
        for(; it != channel.items.rend() && it->code.id != msg_id; it++);
          
        if(it != channel.items.rend())
        {
          new_state_messages.push_back(msg_code);
          cit->present = true;
        }
      }

      uint32_t messages_count = channel.items.size();
      uint32_t old_messages_can_preserve = 0;

      uint32_t max_state_messages =
        std::max(std::max(messages_count, current_state.size) *
                 saving_feed_state_config_.state_messages_factor(),
                 saving_feed_state_config_.min_state_messages());

      if(Application::will_trace(El::Logging::LOW))
      {
        std::ostringstream ostr;
        
        ostr << "PullingFeeds::process_messages: for feed "
             << current_state.id << " (" << current_feed_info.url
             << ") max_state_messages=" << max_state_messages
             << ", messages_count=" << messages_count
             << ", current_state.size=" << current_state.size;

        Application::logger()->trace(ostr.str(),
                                     Aspect::PULLING_FEEDS,
                                     El::Logging::LOW);
      }

      if(max_state_messages > messages_count)
      {
        old_messages_can_preserve = max_state_messages - messages_count;
      }
        
      ACE_Time_Value tm = request_info.time -
        ACE_Time_Value(saving_feed_state_config_.message_timeout());

      uint32_t preserved_messages_count = 0;
      
      cit = current_messages.begin();
      
      for(Message::LocalCodeArray::const_iterator
            mit = current_state_messages.begin();
          mit != current_state_messages.end() &&
            preserved_messages_count < old_messages_can_preserve; mit++, cit++)
      {
        if(cit->present)
        { // Still present in the feed and have already been put
          // into new_state_messages
          continue;
        }
          
        const Message::LocalCode& msg_code = *mit;
        
        if(!http_info.heuristics_applied &&
           ACE_Time_Value(msg_code.published) < tm)
        { // Message not in the feed and quite old, so most likelly will not
          // appear in the feed anymore
          break;
        }

        new_state_messages.push_back(msg_code);
        preserved_messages_count++;
        cit->present = true;
      }

      for(cit = current_messages.begin(); cit != current_messages.end(); cit++)
      {
        if(!cit->present)
        {
          state_update.expired_messages.push_back(cit->id);
          
          state_update.updated_fields |=
            Transport::FeedStateUpdate::UF_EXPIRED_MESSAGES;
        }
      }
        
      //
      // Completing building new list of current messages by
      // adding new messages
      //

      time_t date_for_no_pubdate = 0;
      time_t request_time = request_info.time.sec();

      bool message_set_changed = false;
      uint32_t old_messages = 0;
      
      //
      // First several (defined by calc_entropy_after parameter) requests to
      // a feed do not calc enthropy as there could be messages appearing,
      // disapearing and appearing again in the feed
      //
      bool calc_entropy = current_state.heuristics_counter >=
        -1 * (long)saving_traffic_config_.heuristics().apply_after();

      MsgAdjustmentCodePtr msg_adjustment_code;

      for(ItemList::iterator it = channel.items.begin();
          it != channel.items.end(); it++)
      {
        Item& message = *it;
        Message::LocalCode& msg_code = message.code;

        Message::LocalCodeArray::iterator nsm = new_state_messages.begin();
      
        for(; nsm != new_state_messages.end() &&
              msg_code.id != nsm->id; nsm++);
        
        if(nsm == new_state_messages.end())
        {
          //
          // New message detected
          //

          if(!started())
          {
            throw
              ServiceStopped("PullingFeeds::process_messages: shutting down");
          }
        
          request_info.messages++;
          message_set_changed = true;

          long mistiming = 0;

          if(date_for_no_pubdate == 0)
          {
            // Not set yet
            
            if(channel.last_build_date != El::Moment::null)
            {
              date_for_no_pubdate =
                ACE_Time_Value(channel.last_build_date).sec();

              if((uint64_t)date_for_no_pubdate <
                 current_state.last_request_date)
              {
                // If in presense of a new message last_build_date is older
                // that previous request time, then
                // something is wrong here, so setting to request_time as if
                // there no last_build_date provided.
                
                date_for_no_pubdate = request_time;
              }
            }

            if(date_for_no_pubdate <= 0 || date_for_no_pubdate > request_time)
            {
              date_for_no_pubdate = request_time;
            }
          }

          if(msg_code.published <= 0 ||
             (time_t)msg_code.published > date_for_no_pubdate)
          {
            msg_code.published = date_for_no_pubdate;
          }

          if((time_t)msg_code.published > request_time)
          {
            mistiming = msg_code.published - request_time;
            msg_code.published = request_time;
          }

/*          
          //
          // This code made very old message to reappear in case they were
          // not present in feed at previous request. NOT ROBUST, VERY BAD.
          // Initially the code were invented to struggle feeds with system
          // time set improperly but drawback is much worse.
          // This why commented out.
          //
          if(msg_code.published < current_state.last_request_date)
          {
            msg_code.published = current_state.last_request_date;
          }
*/
/*
          if(msg_code.published <= 0 || msg_code.published > msg_code.published)
          {
            msg_code.published = msg_code.published;
          }
*/
          
          state_update.new_messages.push_back(msg_code);
          
          state_update.updated_fields |=
            Transport::FeedStateUpdate::UF_NEW_MESSAGES;

          //
          // Save new message entity if possible
          //          

          ::NewsGate::Message::Automation::Message adjusted_msg;
          
          if(message.code.id == 0)
          {
            adjusted_msg.log = "invalid message url";
          }
          else if(message.code.published <= oldest_message_time)
          {
            std::ostringstream ostr;
            ostr << "message too old (" <<
              El::Moment(ACE_Time_Value(message.code.published)).rfc0822()
                 << ")";
              
            adjusted_msg.log = ostr.str();
          }
          else
          {
            adjust_message(msg_adjustment_code,
                           message,
                           channel,
                           current_feed_info,
                           state_update.id,
                           adjusted_msg);
          
            if(adjusted_msg.valid)
            {
              post_message(adjusted_msg,
                           message.code,
                           state_update.id,
                           current_feed_info,
                           message_pack);
            }
          }
          
          //
          // Set number of old messages which goes before the new ones.
          // Will call it enthropy as it is also a measure of mess and
          // never decrease
          //
          
          if(calc_entropy)
          {
            http_info.entropy = old_messages;
          }
          
          new_state_messages.push_back(msg_code);
          
          std::string msg_up_date = msg_code.pub_date().rfc0822();
          
          request_info.messages_size += message.title.length() +
            message.description.length() + message.url.length() +
            msg_up_date.length();
          
          ACE_Time_Value msg_time(msg_code.published);
          ACE_Time_Value message_delay;

          //
          // Check for mistiming and delay
          //
                      
          if(mistiming == 0)
          {
            if(current_state_messages.size() > 0 &&
               current_state.last_request_date && msg_time >=
               ACE_Time_Value(current_state.last_request_date))
            {
              message_delay = request_info.time - msg_time;
              request_info.messages_delay += message_delay.sec();

              if(request_info.max_message_delay <
                 (unsigned long)message_delay.sec())
              {
                request_info.max_message_delay = message_delay.sec();
              }
            }
                        
          }
          else if(abs(request_info.mistiming) < abs(mistiming))
          {
            request_info.mistiming = mistiming;
          }

          if(Application::will_trace(El::Logging::HIGH))
          {
            std::ostringstream ostr;
            ostr << "PullingFeeds::process_messages: new "
              "message: msg_code=" << message.code.string() << ", title='"
                 << message.title << "', up_date=" << msg_up_date
                 << ", delay=" << El::Moment::time(message_delay) << ", url="
                 << message.url << ", feed " << state_update.id << " "
                 << current_feed_info.url;

            if(!adjusted_msg.valid)
            {
              ostr << ";\nmessage is not valid, reason: " << adjusted_msg.log;
            }
          
            Application::logger()->trace(ostr.str(),
                                         Aspect::PULLING_FEEDS,
                                         El::Logging::HIGH);
          }
        }
        else
        {
          old_messages++;
          
          if(msg_code.published > nsm->published &&
             (time_t)msg_code.published < request_time)
          {
            //
            // Old message with newer data detected (have seen such feeds).
            // Will consider feed changed to ensure message new date reflected.
            //

            nsm->published = msg_code.published;
            message_set_changed = true;

            state_update.updated_messages.push_back(*nsm);
            
            state_update.updated_fields |=
              Transport::FeedStateUpdate::UF_UPDATED_MESSAGES;
          }
        }
        
      }

      if(message_set_changed)
      {
        //
        // Create ordered message code sequence
        //

        current_state_messages.resize(0);
        current_state_messages.reserve(new_state_messages.size());

        for(Message::LocalCodeArray::iterator it = new_state_messages.begin();
            it != new_state_messages.end(); it++)
        {
          const Message::LocalCode& msg_code = *it;
            
          Message::LocalCodeArray::iterator
            cit = current_state_messages.begin();
          
          for(; cit != current_state_messages.end() &&
                msg_code.published < cit->published; cit++);

          current_state_messages.insert(cit, msg_code);  
        }

        if(current_state_messages.size() > max_state_messages)
        {
          std::ostringstream ostr;
          ostr << "PullingFeeds::process_messages: number of channel items "
            "in feed " << current_state.id << " ("
               << current_state_messages.size()
               << ") have exceeded maximum allowed "
               << max_state_messages << ". Truncating ...";

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);

          current_state_messages.resize(max_state_messages);
        }
      }

      if(request_info.messages == 0)
      {
        request_info.result = FeedRequestInfo::RT_WASTED;
        request_info.wasted = old_messages && old_messages <
          current_state.size ? float(old_messages) / current_state.size : 1.0;
              
        if(Application::will_trace(El::Logging::HIGH))
        {
          std::ostringstream ostr;
          ostr << "PullingFeeds::process_messages: id=" << state_update.id
               << " - request wasted " << request_info.wasted;
                
          Application::logger()->trace(ostr.str(),
                                       Aspect::PULLING_FEEDS,
                                       El::Logging::HIGH);
        }
      }  
    }

    void
    PullingFeeds::adjust_message(
      MsgAdjustmentCodePtr& code,
      const Item& message,
      const Channel& channel,
      FeedInfo& feed_info,
      ::NewsGate::Feed::Id feed_id,
      ::NewsGate::Message::Automation::Message& adjusted_msg)
      throw(ServiceStopped, Exception, El::Exception)
    {
      adjusted_msg.keywords = feed_info.keywords;
      
      ::NewsGate::Feed::Automation::Item item;
      
      MsgAdjustment::Context context(
        adjusted_msg,
        item,
        feed_info.adjustment_script_requested_item_article,
        feed_info.encoding.c_str());

      RequestInterceptor interceptor;
      
      if(context.fill(
           message,
           channel,
           feed_info.url.c_str(),
           feed_info.encoding.c_str(),
           feed_info.space,
           ACE_Time_Value(feed_request_config_.xml_feed_timeout()),
           feed_request_config_.user_agent().c_str(),
           feed_request_config_.redirects_to_follow(),
           &interceptor,
           feed_request_config_.image().referer().c_str(),
           feed_request_config_.socket_recv_buf_size(),
           feed_request_config_.image().min_width(),
           feed_request_config_.image().min_height(),
           image_prefix_blacklist_,
           image_extension_whitelist_,
           feed_request_config_.limits().message_title(),
           feed_request_config_.limits().message_description(),
           feed_request_config_.limits().message_description_chars(),
           feed_request_config_.limits().max_image_count()) &&
         !feed_info.adjustment_script.empty())
      {
        if(feed_request_config_.full_article().obey_robots_txt() &&
           context.item.article_session.get() &&
           !Application::instance()->robots_checker().allowed(
             context.item.article_session->url(),
             feed_request_config_.user_agent().c_str()))
        {
          context.item.article_session.reset(0);
        }

        bool running = false;

        try
        {
          item.article_max_size =
            feed_request_config_.full_article().max_size();
          
          item.cache_dir = feed_request_config_.cache_dir();
          item.feed_id = feed_id;

          if(code.get() == 0)
          {
            code.reset(new MsgAdjustment::Code());

            code->compile(feed_info.adjustment_script.c_str(),
                          context.item.feed_id,
                          context.message.source.url.c_str());
          }
          
          std::auto_ptr<El::Python::Sandbox> sandbox(
            Config::create_python_sandbox(
              feed_request_config_.python().sandbox(),
              this));          

          running = true;

          code->run(context,
                    sandbox_service_.in(),
                    sandbox.get(),
                    Application::logger());
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "PullingFeeds::adjust_messages: message adjustment script "
            "failed (feed " << feed_id << " " << feed_info.url
               << "). Error description:\n" << e;

          if(running)
          {
            Application::logger()->trace(ostr.str(),
                                         Aspect::PULLING_FEEDS,
                                         El::Logging::LOW);
          }
          else
          {
            Application::logger()->alert(ostr.str(), Aspect::PULLING_FEEDS);
          }
        }

        feed_info.adjustment_script_requested_item_article =
          context.item_article_requested;
      }
      
      if(!started())
      {
        throw ServiceStopped("PullingFeeds::adjust_messages: shutting down");
      }
    }

    bool
    PullingFeeds::interrupt_execution(std::string& reason) throw(El::Exception)
    {
      if(started())
      {
        return false;
      }

      reason = "application shutting down";
      return true;
    }

    bool
    PullingFeeds::exclude_string(std::string& text, const char* entry)
      throw(El::Exception)
    {
      if(entry && *entry)
      {
        std::size_t pos = text.find(entry);

        if(pos != std::string::npos)
        {
          text = std::string(text.c_str(), pos) +
            (text.c_str() + pos + strlen(entry));

          return true;
        }
      }
      
      return false;
    }
    
    void
    PullingFeeds::post_message(
      ::NewsGate::Message::Automation::Message& message,
      const Message::LocalCode& message_code,
      ::NewsGate::Feed::Id feed_id,
      const FeedInfo& current_feed_info,
      Message::Transport::RawMessagePackImpl::Var& message_pack)
      throw(ServiceStopped, Exception, El::Exception)
    {
      if(message_pack.in() == 0)
      {
        Message::RawMessageArrayPtr entities(
          new Message::RawMessageArray());
            
        entities->reserve(message_entities_reserve_);
            
        message_pack =
          Message::Transport::RawMessagePackImpl::Init::create(
            entities.release());
      }

      Message::RawMessageArray& entities = message_pack->entities();
      entities.push_back(Message::RawMessage());

      if(entities.size() > message_entities_reserve_)
      {
        message_entities_reserve_ = entities.size() * 3 / 2;
      }
          
      Message::RawMessage& entity = *entities.rbegin();

      entity.content->source.title = message.source.title;
      entity.content->source.html_link = message.source.html_link;

      entity.id_from_url(message.url.c_str(), current_feed_info.url.c_str());
      entity.published = message_code.published;

      entity.source_id = feed_id;
      entity.space = message.space;
      entity.source_url = message.source.url;
      entity.lang = message.lang.el_code();
      entity.country = message.country.el_code();

      Message::RawContent* content = entity.content.in();

      content->url = message.url;
      content->title = message.title;
      content->description = message.description;
      
      {
        std::ostringstream ostr;
        bool first = true;
        
        for(::NewsGate::Message::Automation::StringArray::const_iterator
              i(message.keywords.begin()), e(message.keywords.end());
            i != e; ++i)
        {
          std::string kw;
          El::String::Manip::trim(i->c_str(), kw);

          if(!kw.empty())
          {
            if(first)
            {
              first = false;
            }
            else
            {
              ostr << ", ";
            }
        
            ostr << kw;
          }
        }

        content->keywords = ostr.str();
      }
      
      if(!message.images.empty())
      {
        std::ostringstream ostr;
        ostr << "Images for " << message_code.string() << ": ";
        
        Message::RawImageArrayPtr msg_images(new Message::RawImageArray());

        const ThumSizeConf& thumb_sizes =
          feed_request_config_.image().thumbnail().size();

        typedef __gnu_cxx::hash_set<uint64_t, El::Hash::Numeric<uint64_t> >
          ImageHashSet;
        
        ImageHashSet image_hashes;

        size_t len = 0;

        for(::NewsGate::Message::Automation::ImageArray::iterator
              i(message.images.begin()), e(message.images.end()); i != e; ++i)
        {
          if(!started())
          {
            throw ServiceStopped("PullingFeeds::post_message: shutting down");
          }

          ::NewsGate::Message::Automation::Image& img = *i;
          
          ostr << std::endl << img.src << " (status " << img.status << ") : ";
/*
          if(img.src == "http://mc.yandex.ru/watch/2133667")
          {
            std::cerr << "found\n";
          }
*/

          if(msg_images->size() ==
             feed_request_config_.limits().max_image_count())
          {
            ostr << "max image count reached; skipped";
            continue;
          }

          if(img.status != ::NewsGate::Message::Automation::Image::IS_VALID)
          {
            ostr << "bad status; skipped";
            continue;
          }
/*
          if(img.src == "http://mc.yandex.ru/watch/2133667")
          {
            std::cerr << "found2\n";
          }          
*/        
          NewsGate::Message::ImageThumbArray thumbnails;
          uint64_t image_hash = 0;
          
          if(!thumb_sizes.empty() &&
             create_thumbnails(img,
                               thumb_sizes,
                               thumbnails,
                               image_hash,
                               &ostr))
          {
            if(image_hash &&
               image_hashes.find(image_hash) != image_hashes.end())
            {
              ostr << "hash dupl; skipped";
              continue;
            }

            Message::RawImageArray::const_iterator it = msg_images->begin();
            for(; it != msg_images->end() && it->src != img.src; ++it);

            if(it != msg_images->end())
            {
              ostr << "source dupl; skipped";
              continue;
            }

            std::string alternate = img.alt;

            if(!exclude_string(alternate, img.src.c_str()))
            {
              El::Net::HTTP::URL_var url =
                new El::Net::HTTP::URL(img.src.c_str());
            
              const char* fname = strrchr(url->path(), '/');
              
              if(fname && !exclude_string(alternate, ++fname))
              {
                const char* ext = strrchr(fname, '.');
                
                if(ext)
                {
                  exclude_string(alternate,
                                 std::string(fname, ext - fname).c_str());
                }
              }
            }
            
            exclude_string(alternate, content->title.c_str());
            El::String::Manip::trim(alternate.c_str(), alternate);

            if(alternate.empty())
            {
              img.alt.clear();
            }
            
            size_t size = img.src.length() + img.alt.length() +
                sizeof(El::HTML::LightParser::Image);
            
            if(len + size > feed_request_config_.limits().image_alt_text())
            {
              ostr << "exeeding len; skipped";
              continue;
            }
            
            len += size;
            
            msg_images->push_back(Message::RawImage());
            
            Message::RawImage& mim = *msg_images->rbegin();
            
            mim.src = img.src;
            mim.alt = img.alt;
            mim.width = img.width;
            mim.height = img.height;
            
            thumbnails.release(mim.thumbs);
            image_hashes.insert(image_hash);

            ostr << "added";
          }
          else
          {
            ostr << "; skipped";
          }
        }
        
        if(!msg_images->empty())
        {
          content->images.reset(msg_images.release());
        }

        Application::logger()->trace(ostr.str(),
                                     Aspect::PULLING_FEEDS,
                                     El::Logging::HIGH);
      }
    }

    bool
    PullingFeeds::create_thumbnails(
      ::NewsGate::Message::Automation::Image& img,
      const ThumSizeConf& thumb_sizes,
      NewsGate::Message::ImageThumbArray& thumbnails,
      uint64_t& image_hash,
      std::ostream* log_ostr)
      throw(ServiceStopped, Exception, El::Exception)
    {
      if(img.src.empty())
      {
        if(log_ostr)
        {
          *log_ostr << "src empty (2)";
        }
        
        return false;
      }
      
      thumbnails.clear();
      image_hash = 0;

      RequestInterceptor interceptor;
      
      try
      {
        El::Net::HTTP::URL_var url = new El::Net::HTTP::URL(img.src.c_str());
        
        El::Net::HTTP::HeaderList headers;

        headers.add(El::Net::HTTP::HD_ACCEPT, "*/*");
        headers.add(El::Net::HTTP::HD_ACCEPT_ENCODING, "gzip,deflate");
        headers.add(El::Net::HTTP::HD_ACCEPT_LANGUAGE, "en-us,*");
      
        headers.add(El::Net::HTTP::HD_USER_AGENT,
                    feed_request_config_.user_agent().c_str());

        HTTPSessionPtr session(
          new El::Net::HTTP::Session(url,
                                     El::Net::HTTP::HTTP_1_1,
                                     &interceptor));
        
        ACE_Time_Value timeout(feed_request_config_.xml_feed_timeout());
        session->open(&timeout, &timeout, &timeout);

        session->send_request(El::Net::HTTP::GET,
                              El::Net::HTTP::ParamList(),
                              headers,
                              0,
                              0,
                              feed_request_config_.redirects_to_follow());
      
        session->recv_response_status();

        if(session->status_code() != El::Net::HTTP::SC_OK)
        {
          if(log_ostr)
          {
            *log_ostr << "bad HTTP response (2) " << session->status_code();
          }

          return false;
        }

        El::Net::HTTP::Header header;
        while(session->recv_response_header(header));

        std::string img_path;
        
        {
          El::Guid guid;
          guid.generate();
          
          std::ostringstream ostr;
          
          ostr << feed_request_config_.image().thumbnail().cache_dir().c_str()
               << "/" << guid.string(El::Guid::GF_DENSE);

          img_path = ostr.str();
        }

        try
        {
          if(!session->save_body(
               img_path.c_str(),
               feed_request_config_.image().thumbnail().max_image_size(),
               &image_hash))
          {
            unlink(img_path.c_str());            
            return false;
          }          

          ThumbGenTask_var task =
            new ThumbGenTask(img_path.c_str(),
                             feed_request_config_.image().min_width(),
                             feed_request_config_.image().min_height(),
                             &Application::instance()->mime_types());

          for(ThumSizeConf::const_iterator i(thumb_sizes.begin()),
                e(thumb_sizes.end()); i != e; ++i)
          {
            task->thumbs.push_back(
              ThumbGenTask::Thumb(i->width(), i->height(), i->crop()));
          }

          try
          {
            if(process_pool_->execute(task.in()) && process_pool_->started())
            {
              task->wait();

              std::string error = task->error();
              
              if(!error.empty())
              {
                throw ImageNotLoadable(error.c_str());
              }

              if(task->skip)
              {
                throw ImageNotLoadable(task->issue.c_str());
              }
            }
            else
            {
              throw ImageNotLoadable(
                "PullingFeeds::create_thumbnails: "
                "can't enqueue task; probably terminating ...");
            }
          }
          catch(const ImageNotLoadable& ex)
          {
            unlink(img_path.c_str());
            
            size_t thumb_num = 0;
            
            for(ThumSizeConf::const_iterator i(thumb_sizes.begin()),
                  e(thumb_sizes.end()); i != e; ++i)
            {
              std::ostringstream ostr;
              ostr << img_path << "." << thumb_num++;
              unlink(ostr.str().c_str());
            }
            
            if(log_ostr)
            {
              *log_ostr << "ImageNotLoadable caught while "
                "reading image " << img.src << ". Description: " << ex;
            }
            
            return false;
          }

          if(log_ostr && !task->issue.empty())
          {
            *log_ostr << task->issue;
          }

          img.width = task->image_width;
          img.height = task->image_height;
          
          thumbnails.resize(thumb_sizes.size());

          size_t j = 0;

          for(ThumbGenTask::ThumbArray::const_iterator i(task->thumbs.begin()),
                e(task->thumbs.end()); i != e; ++i)
          {
            const ThumbGenTask::Thumb& thumb = *i;

            if(!thumb.path.empty())
            {
              Message::ImageThumb& t = thumbnails[j++];

              t.type = thumb.type;
              t.height(thumb.height);
              t.width(thumb.width);
              t.read_image(thumb.path.c_str(), true);
              t.cropped(thumb.crop);
              
              unlink(thumb.path.c_str());
            }
          }

          thumbnails.resize(j, true);          
          unlink(img_path.c_str());
        }
        catch(const El::Exception&)
        {
          unlink(img_path.c_str());
          throw;
        }
      }
      catch(const ::El::Net::Exception& e)
      {
        if(log_ostr)
        {
          *log_ostr << "HTTP error (2): " << e;
        }
        
        return false;
      }

      return thumbnails.size() > 0;
    }    
/*
    bool
    PullingFeeds::create_thumbnails(
      ::NewsGate::Message::Automation::Image& img,
      const ThumSizeConf& thumb_sizes,
      NewsGate::Message::ImageThumbArray& thumbnails,
      uint64_t& image_hash,
      std::ostream* log_ostr)
      throw(ServiceStopped, Exception, El::Exception)
    {
      if(img.src.empty())
      {
        if(log_ostr)
        {
          *log_ostr << "src empty (2)";
        }
        
        return false;
      }
      
      thumbnails.clear();
      image_hash = 0;

      RequestInterceptor interceptor;
      
      try
      {
        El::Net::HTTP::URL_var url = new El::Net::HTTP::URL(img.src.c_str());
        
        El::Net::HTTP::HeaderList headers;

        headers.add(El::Net::HTTP::HD_ACCEPT, "*""/""*");
        headers.add(El::Net::HTTP::HD_ACCEPT_ENCODING, "gzip,deflate");
        headers.add(El::Net::HTTP::HD_ACCEPT_LANGUAGE, "en-us,*");
      
        headers.add(El::Net::HTTP::HD_USER_AGENT,
                    feed_request_config_.user_agent().c_str());

        HTTPSessionPtr session(
          new El::Net::HTTP::Session(url,
                                     El::Net::HTTP::HTTP_1_1,
                                     &interceptor));
        
        ACE_Time_Value timeout(feed_request_config_.xml_feed_timeout());
        session->open(&timeout, &timeout, &timeout);

        session->send_request(El::Net::HTTP::GET,
                              El::Net::HTTP::ParamList(),
                              headers,
                              0,
                              0,
                              feed_request_config_.redirects_to_follow());
      
        session->recv_response_status();

        if(session->status_code() != El::Net::HTTP::SC_OK)
        {
          if(log_ostr)
          {
            *log_ostr << "bad HTTP response (2) " << session->status_code();
          }

          return false;
        }

        El::Net::HTTP::Header header;
        while(session->recv_response_header(header));

        std::string img_path;
        
        {
          El::Guid guid;
          guid.generate();
          
          std::ostringstream ostr;
          
          ostr << feed_request_config_.image().thumbnail().cache_dir().c_str()
               << "/" << guid.string(El::Guid::GF_DENSE);

          img_path = ostr.str();
        }

        try
        {
          if(!session->save_body(
               img_path.c_str(),
               feed_request_config_.image().thumbnail().max_image_size(),
               &image_hash))
          {
            unlink(img_path.c_str());            
            return false;
          }
          
          try
          {
            check_image_loadable(img_path.c_str());
          }
          catch(const ImageNotLoadable& e)
          {
            unlink(img_path.c_str());

            if(log_ostr)
            {
              *log_ostr << "ImageNotLoadable caught while "
                "reading image " << img.src << ". Description: " << e;
            }
            
            return false;
          }
          
          Magick::Image image;
          std::string thumb_type_lower;
          
          try
          {
            image.read(img_path.c_str());
            unlink(img_path.c_str());
            
            img.width = image.baseColumns();
            img.height = image.baseRows();

            if(img.height < 1 || img.width < 1)
            {
              if(log_ostr)
              {
                *log_ostr << "zero size image (" << img.width << "x"
                          << img.height << ")";
              }
              
              return false;
            }

            if(img.height < feed_request_config_.image().min_height() ||
               img.width < feed_request_config_.image().min_width())
            {
              if(log_ostr)
              {
                *log_ostr << "small image (" << img.width << "x" << img.height
                          << "<" << feed_request_config_.image().min_width()
                          << "x" << feed_request_config_.image().min_height()
                          << ")";
              }
              
              return false;
            }

            std::string thumb_type = image.magick();

            if(thumb_type.empty())
            {
              if(log_ostr)
              {
                *log_ostr << "unknown image type at " << img.src;
              }
              
              return false;
            }
            
            El::String::Manip::to_lower(thumb_type.c_str(), thumb_type_lower);

            const El::Net::HTTP::MimeTypeMap& mime_types =
              Application::instance()->mime_types();

            if(mime_types.find(thumb_type_lower) == mime_types.end())
            {
              image.magick("JPEG");
              thumb_type = image.magick();

              if(thumb_type.empty())
              {
                if(log_ostr)
                {
                  *log_ostr << "can't convert to JPEG " << img.src;
                }
                
                return false;
              }

              El::String::Manip::to_lower(thumb_type.c_str(),
                                          thumb_type_lower);
            }
          }
          catch(const Magick::Exception& e)
          {
            unlink(img_path.c_str());

            if(log_ostr)
            {
              *log_ostr << "ImageMagick error while reading " << img.src
                        << ": " << e;
            }
            
            return false;
          }

          thumbnails.resize(thumb_sizes.size());
          
          size_t i = 0;
          bool image_as_thumbnail = true;
          bool crop_max = true;
          
          for(ThumSizeConf::const_iterator it = thumb_sizes.begin();
              it != thumb_sizes.end(); it++)
          {
            uint16_t max_width = it->width();
            uint16_t max_height = it->height();

            if(!max_width || !max_height)
            {
              continue;
            }
            
            bool crop = it->crop();
            
            Message::ImageThumb& thumb = thumbnails[i];
            thumb.type = thumb_type_lower;

            try
            {
              if(img.height <= max_height && img.width <= max_width)
              {
                if(!crop || (img.height == max_height && img.width == max_width))
                {    
                  if(image_as_thumbnail)
                  {
                    image.write(img_path.c_str());
                  
                    thumb.height(img.height);
                    thumb.width(img.width);
                    thumb.read_image(img_path.c_str(), true);
                    unlink(img_path.c_str());
                  
                    ++i;
                    image_as_thumbnail = false;
                  }
              
                  continue;
                }
                else                  
                {
                  if(crop_max)
                  {
                    if(max_width > max_height)
                    {
                      max_height =
                        std::min((uint16_t)(((double)img.width /  max_width) *
                                            max_height + 0.5),
                                 (uint16_t)img.height);
                      
                      max_width = img.width;
                    }
                    else
                    {
                      max_width =
                        std::min((uint16_t)(((double)img.height / max_height) *
                                            max_width + 0.5),
                                 (uint16_t)img.width);
                      
                      max_height = img.height;
                    }

                    if(!max_width || !max_height)
                    {
                      continue;
                    }
                    
                    crop_max = false;
                  }
                  else
                  {
                    continue;
                  }
                }
              }
              
              Magick::Image copy(image);

              double aspect = (double)max_width / max_height;
              double img_aspect = (double)img.width / img.height;

              if(crop)
              {
                if(img.height < max_height)
                {
                  copy.crop(Magick::Geometry(max_width,
                                             img.height,
                                             (img.width - max_width) / 2,
                                             0));
                }
                else if(img.width < max_width)
                {
                  copy.crop(Magick::Geometry(img.width,
                                             max_height,
                                             0,
                                             (img.height - max_height) / 2));
                }
                else
                {
                  if(img_aspect <= aspect)
                  {
                    unsigned short height =
                      (unsigned short)((double)img.width / aspect);

                    copy.crop(Magick::Geometry(img.width,
                                               height,
                                               0,
                                               (img.height - height) / 2));
                  }
                  else
                  {
                    unsigned short width =
                      (unsigned short)((double)img.height * aspect);
                
                    copy.crop(Magick::Geometry(width,
                                               img.height,
                                               (img.width - width) / 2,
                                               0));
                  }

                  copy.scale(Magick::Geometry(max_width, max_height));
                }
              }
              else
              {
                unsigned long width = 0;
                unsigned long height = 0;

                if(aspect <= img_aspect)
                {
                  // Shrink image width to max_width, height calc accordingly
                  width = max_width;
                  height = (unsigned short)((double)img.width / img_aspect);
                }
                else
                {
                  // Shrink image height to max_height, width calc accordingly
              
                  height = max_height;
                  width = (unsigned short)(img_aspect * img.height);
                }

                copy.scale(Magick::Geometry(width, height));
              }

              copy.write(img_path.c_str());

              //
              // Get exact dimensions of resulted thumbnail
              //
              copy.read(img_path.c_str());
              
              thumb.width(copy.baseColumns());
              thumb.height(copy.baseRows());
              thumb.cropped(crop);

              copy = Magick::Image();

              thumb.read_image(img_path.c_str(), true);
              unlink(img_path.c_str());

              ++i;
            }
            catch(const Magick::Exception& e)
            {
              unlink(img_path.c_str());

              if(log_ostr)
              {
                *log_ostr << "ImageMagick operation failed for " << img.src
                          << ": " << e << "; ";
              }

              continue;
            }
          }
          
          thumbnails.resize(i, true);
        }
        catch(const El::Exception&)
        {
          unlink(img_path.c_str());
          throw;
        }
      }
      catch(const ::El::Net::Exception& e)
      {
        if(log_ostr)
        {
          *log_ostr << "HTTP error (2): " << e;
        }
        
        return false;
      }

      return thumbnails.size() > 0;
    }

    void
    PullingFeeds::check_image_loadable(const char* img_path) const
      throw(ImageNotLoadable, El::Exception)
    {      
      CheckImageLoadableTask_var task = new CheckImageLoadableTask(img_path);

      if(process_pool_->execute(task.in()) && process_pool_->started())
      {
        task->wait();

        if(!task->is_loadable)
        {
          throw ImageNotLoadable(task->error().c_str());
        }
      }
      else
      {
        throw ImageNotLoadable("PullingFeeds::check_image_loadable: "
                               "can't enqueue task; probably terminating ...");
      }
    }
*/
/*    
    bool
    PullingFeeds::read_image_size(::NewsGate::Message::Automation::Image& img,
                                  std::ostream* log_ostr) const
      throw(ServiceStopped, Exception, El::Exception)
    {
      if(img.src.empty())
      {
        if(log_ostr)
        {
          *log_ostr << "src empty (1)";
        }

        return false;
      }

      ::NewsGate::Message::Automation::Image img_r = img;

      if(!read_image_size(img, 0, log_ostr) ||
         !read_image_size(img_r,
                          feed_request_config_.image().referer().c_str(),
                          log_ostr))
      {
        return false;
      }

      if(img.height != img_r.height || img.width != img_r.width)
      {
        if(log_ostr)
        {
          *log_ostr << "ref/no-ref size differs (" << img_r.width << "x"
                    << img_r.height << "!=" << img.width << "x" << img.height
                    << ")";
        }

        return false;
      }

      if(img.height < feed_request_config_.image().min_height() ||
         img.width < feed_request_config_.image().min_width())
      {
        if(log_ostr)
        {
          *log_ostr << "small image (" << img.width << "x" << img.height
                    << "<" << feed_request_config_.image().min_width()
                    << "x" << feed_request_config_.image().min_height() << ")";
        }

        return false;
      }

      return true;
    }
    
    bool
    PullingFeeds::read_image_size(::NewsGate::Message::Automation::Image& img,
                                  const char* referer,
                                  std::ostream* log_ostr) const
      throw(ServiceStopped, Exception, El::Exception)
    {
      RequestInterceptor interceptor;
      
      interceptor.recv_buffer_size =
        feed_request_config_.socket_recv_buf_size();

      try
      {
        El::Net::HTTP::URL_var url = new El::Net::HTTP::URL(img.src.c_str());
        
        El::Net::HTTP::HeaderList headers;

        headers.add(El::Net::HTTP::HD_ACCEPT, "*""/""*");
        headers.add(El::Net::HTTP::HD_ACCEPT_ENCODING, "gzip,deflate");
        headers.add(El::Net::HTTP::HD_ACCEPT_LANGUAGE, "en-us,*");
      
        headers.add(El::Net::HTTP::HD_USER_AGENT,
                    feed_request_config_.user_agent().c_str());

        if(referer && *referer != '\0')
        {
          headers.add(El::Net::HTTP::HD_REFERER, referer);
        }
        
        HTTPSessionPtr session(
          new El::Net::HTTP::Session(url,
                                     El::Net::HTTP::HTTP_1_1,
                                     &interceptor));

        ACE_Time_Value timeout(feed_request_config_.xml_feed_timeout());

        session->open(&timeout,
                      &timeout,
                      &timeout,
                      1024,
                      feed_request_config_.stream_recv_buf_size());

        session->send_request(El::Net::HTTP::GET,
                              El::Net::HTTP::ParamList(),
                              headers,
                              0,
                              0,
                              feed_request_config_.redirects_to_follow());
      
        session->recv_response_status();

        if(session->status_code() != El::Net::HTTP::SC_OK)
        {
          if(log_ostr)
          {
            *log_ostr << "bad HTTP response (1) " << session->status_code();
          }
          
          return false;
        }

        El::Net::HTTP::Header header;
        while(session->recv_response_header(header));
        
        El::Image::ImageInfo info;
        
        if(info.read(session->response_body()))
        {
          img.width = info.width;
          img.height = info.height;          
        }
        else
        {
          if(log_ostr)
          {
            *log_ostr << "can't recognize image at " << img.src;
          }

          return false;
        }
        
      }
      catch(const ::El::Net::Exception& e)
      {
        if(log_ostr)
        {
          *log_ostr << "HTTP error (1): " << e;
        }
        
        return false;
      }

      return true;
    }
    */
    bool
    PullingFeeds::start() throw(Exception, El::Exception)
    {
      if(PullerState::start())
      {
        return process_pool_->start() && sandbox_service_->start();
      }

      return false;
    }
    
    bool
    PullingFeeds::stop() throw(Exception, El::Exception)
    {
      return PullerState::stop();
/*      
      if(PullerState::stop())
      {
        return process_pool_->stop();
      }
      
      return false;
*/
    }
  
    void
    PullingFeeds::wait() throw(Exception, El::Exception)
    {
      PullerState::wait();

      // Need to stop process pool after PullingFeeds
      // object stopped completelly
      
      sandbox_service_->stop();
      process_pool_->stop();
      
      sandbox_service_->wait();
      process_pool_->wait();

      WriteGuard guard(srv_lock_);

      if(feed_daily_stat_.empty())
      {
        return;
      }

      std::fstream file(saving_feed_stat_config_.cache_file().c_str(),
                        ios::out);

      if(!file.is_open())
      {
        std::ostringstream ostr;
        ostr << "PullingFeeds::wait_object: failed to open file '"
             << saving_feed_stat_config_.cache_file().c_str()
             << "' for write access";

        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
        return;
      }

      file << El::Moment(feed_stat_last_save_).iso8601() << std::endl;
        
      file << feed_daily_stat_.size() << std::endl;
      
      for(FeedDailyStatList::iterator it = feed_daily_stat_.begin();
          it != feed_daily_stat_.end(); it++)
      {
        file << it->date.iso8601() << std::endl;
        
        const FeedStatTable& feed_stat = it->feed_stat;

        file << feed_stat.size() << std::endl;

        for(FeedStatTable::const_iterator fs_it = feed_stat.begin();
            fs_it != feed_stat.end(); ++fs_it)
        {
          const ::NewsGate::RSS::Transport::FeedStat& stat = fs_it->second;

          file << stat.id << "\t" << stat.requests << "\t" << stat.failed
               << "\t" << stat.unchanged << "\t" << stat.not_modified
               << "\t" << stat.presumably_unchanged << "\t"
               << stat.has_changes << "\t" << stat.wasted << "\t"
               << stat.outbound << "\t" << stat.inbound << "\t"
               << stat.requests_duration << "\t"
               << stat.messages << "\t" << stat.messages_size << "\t"
               << stat.messages_delay << "\t" << stat.max_message_delay
               << "\t" << stat.mistiming << std::endl;
        }
      }

      bool failed = file.bad() || file.fail();
      
      file.close();
      feed_daily_stat_.clear();

      if(failed)
      {
        std::ostringstream ostr;
        ostr << "PullingFeeds::wait_object: failed to write to file '"
             << saving_feed_stat_config_.cache_file().c_str()
             << "'";

        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
      }
      
    }

    void
    PullingFeeds::load_stat_cache() throw(Exception, El::Exception)
    {
      std::fstream file(saving_feed_stat_config_.cache_file().c_str(),
                        ios::in);

      if(!file.is_open())
      {
        std::ostringstream ostr;
        ostr << "PullingFeeds::load_stat_cache: failed to open file '"
             << saving_feed_stat_config_.cache_file().c_str()
             << "' for read access";

        El::Service::Error error(ostr.str(),
                                 this,
                                 El::Service::Error::WARNING);
        
        callback_->notify(&error);
        return;
      }

      std::string line;
      std::getline(file, line);

      FeedDailyStatList daily_stat;
      ACE_Time_Value feed_stat_last_save;
      
      bool success = true;
      
      try
      {
        feed_stat_last_save =
          El::Moment(line.c_str(), El::Moment::TF_ISO_8601);
      }
      catch(const El::Moment::InvalidArg&)
      {
        success = false;
      }

      if(success)
      {
        unsigned long stat_days = 0;
        file >> stat_days;
        
        for(unsigned long i = 0; (success = !file.bad() && !file.fail()) &&
              i < stat_days; i++)
        {
          daily_stat.push_back(FeedDailyStat());
          
          FeedDailyStatList::reverse_iterator it = daily_stat.rbegin();
          FeedDailyStat& daily_stat = *it;
          
          // Skip carriage return
          std::getline(file, line);
          
          // Read date
          std::getline(file, line);
          
          if(file.bad() || file.fail())
          {
            std::ostringstream ostr;
            ostr << "PullingFeeds::load_stat_cache: failed to read "
              "stat_date from file '"
                 << saving_feed_stat_config_.cache_file().c_str()
                 << "'";
            
            throw Exception(ostr.str());
          }
          
          daily_stat.date.set_iso8601(line.c_str());
        
          unsigned long stat_feeds = 0;
          file >> stat_feeds;
      
          FeedStatTable& feed_stat = it->feed_stat;
          
          for(size_t i = 0; (success = !file.bad() && !file.fail()) &&
                i < stat_feeds; i++)
          {
            Feed::Id id = 0;
            file >> id;

            FeedStatTable::iterator fs_it = feed_stat.insert(
              FeedStatTable::value_type(
                id,
                ::NewsGate::RSS::Transport::FeedStat())).first;
          
            ::NewsGate::RSS::Transport::FeedStat& stat = fs_it->second;
            stat.id = id;

            file >> stat.requests >> stat.failed >> stat.unchanged
                 >> stat.not_modified  >> stat.presumably_unchanged
                 >> stat.has_changes >> stat.wasted >> stat.outbound
                 >> stat.inbound >> stat.requests_duration >> stat.messages
                 >> stat.messages_size >> stat.messages_delay
                 >> stat.max_message_delay >> stat.mistiming;
          }
        }
      }
      
      file.close();

      if(success)
      {
        feed_stat_last_save_ = feed_stat_last_save;
        feed_daily_stat_.swap(daily_stat);
      }
      else
      {
        std::ostringstream ostr;
        ostr << "PullingFeeds::load_stat_cache: failed to read "
          "stat record from file '"
             << saving_feed_stat_config_.cache_file().c_str()
             << "'";
        
        El::Service::Error error(ostr.str(),
                                 this,
                                 El::Service::Error::ALERT);
        
        callback_->notify(&error);
      }
    }

    bool
    PullingFeeds::notify(El::Service::Event* event) throw(El::Exception)
    {
      RequestFeed* rf = dynamic_cast<RequestFeed*>(event);
      
      if(rf != 0)
      {
        if(rf->time != ACE_Time_Value::zero)
        {
          RequestingFeeds* requesting_feeds =
            dynamic_cast<RequestingFeeds*>(state_.in());

          if(requesting_feeds == 0)
          {
            El::Service::Error error("PullingFeeds::notify: "
                                     "dynamic_cast<RequestingFeeds*> failed",
                                     this);
        
            callback_->notify(&error);   
            return false;
          }

          requesting_feeds->deliver_now(rf);
          return true;
        }
        
        request_feed(rf->feed_id, rf->feed_info_update_number);
        return true;
      }

      if(PullerState::notify(event))
      {
        return true;
      }

      AcceptFeeds* af = dynamic_cast<AcceptFeeds*>(event);
      
      if(af != 0)
      {
        accept_feeds(af->feeds.in());
        return true;
      }

      if(dynamic_cast<Poll*>(event) != 0)
      {
        poll();
        return true;
      }

      if(dynamic_cast<PushFeedState*>(event) != 0)
      {
        push_feed_state();
        return true;
      }

      if(dynamic_cast<PushFeedStat*>(event) != 0)
      {
        push_feed_stat();
        return true;
      }

      std::ostringstream ostr;
      ostr << "NewsGate::RSS::PullingFeeds::notify: unknown "
           << *event;
      
      El::Service::Error error(ostr.str(), this);
      callback_->notify(&error);
      
      return true;
    }

    //
    // PullingFeeds::RequestInterceptor struct
    //
    void
    PullingFeeds::RequestInterceptor::socket_stream_created(
      El::Net::Socket::Stream& stream)
      throw(El::Exception)
    {
      if(!recv_buffer_size)
      {
        return;
      }
      
      ACE_SOCK_Stream* socket = &stream.socket();
        
      unsigned int bufsize = 0;
      int buflen = sizeof(bufsize);
        
      if(socket->get_option(SOL_SOCKET, SO_RCVBUF, &bufsize, &buflen) == -1)
      {
        int error = ACE_OS::last_error();
        
        std::ostringstream ostr;
        ostr << "PullingFeeds::RequestInterceptor::"
          "socket_stream_created: unable to get socket option SO_RCVBUF. "
          "Error " << error << ", reason " << ACE_OS::strerror(error);
        
        throw Exception(ostr.str());
      }

      if(bufsize > recv_buffer_size)
      {
        bufsize = recv_buffer_size;
          
        if(socket->set_option(SOL_SOCKET,
                              SO_RCVBUF,
                              &bufsize,
                              sizeof(bufsize)) == -1)
        {
          int error = ACE_OS::last_error();
        
          std::ostringstream ostr;
            
          ostr << "PullingFeeds::RequestInterceptor::"
            "socket_stream_created: unable to set socket option SO_RCVBUF "
            "with size " << bufsize << ". Error " << error << ", reason "
               << ACE_OS::strerror(error);
        
          throw Exception(ostr.str());
        }
      }
    }

    void
    PullingFeeds::RequestInterceptor::socket_stream_connected(
      El::Net::Socket::Stream& stream)
      throw(ServiceStopped, El::Exception)
    {
      if(Application::instance()->shutting_down())
      {
        throw
          ServiceStopped("PullingFeeds::RequestInterceptor::"
                         "socket_stream_connected: shutting down");

      }      
    }    

    void
    PullingFeeds::RequestInterceptor::socket_stream_read(
      const unsigned char* buff, size_t size)
      throw(ServiceStopped, El::Exception)
    {
      if(Application::instance()->shutting_down())
      {
        throw
          ServiceStopped("PullingFeeds::RequestInterceptor::"
                         "socket_stream_read: shutting down");
      }
    }

    void
    PullingFeeds::RequestInterceptor::socket_stream_write(
      const unsigned char* buff,
      size_t size)
      throw(ServiceStopped, El::Exception)
    {
      if(Application::instance()->shutting_down())
      {
        throw
          ServiceStopped("PullingFeeds::RequestInterceptor::"
                         "socket_stream_connected: shutting down");

      }      
    }
    
    //
    // PullingFeeds::FeedRequestInterceptor struct
    //
    void
    PullingFeeds::FeedRequestInterceptor::chunk_begins(unsigned long long size)
      throw(El::Exception)
    {
      if(size && chunks++ == 0)
      {
        first_chunk_size = size;
      }
    }

    void
    PullingFeeds::FeedRequestInterceptor::socket_stream_created(
      El::Net::Socket::Stream& stream)
      throw(El::Exception)
    {
      if(recv_buffer_size)
      {
        socket = &stream.socket();
        
        unsigned int bufsize = 0;
        int buflen = sizeof(bufsize);
        
        if(socket->get_option(SOL_SOCKET, SO_RCVBUF, &bufsize, &buflen) == -1)
        {
          int error = ACE_OS::last_error();
        
          std::ostringstream ostr;
          ostr << "PullingFeeds::FeedRequestInterceptor::"
            "socket_stream_created: unable to get socket option SO_RCVBUF. "
            "Error " << error << ", reason " << ACE_OS::strerror(error);
        
          throw Exception(ostr.str());
        }

        if(bufsize > recv_buffer_size)
        {
          prev_recv_buffer_size = bufsize;        
          bufsize = recv_buffer_size;
          
          if(socket->set_option(SOL_SOCKET,
                                SO_RCVBUF,
                                &bufsize,
                                sizeof(bufsize)) == -1)
          {
            int error = ACE_OS::last_error();
        
            std::ostringstream ostr;
            ostr << "PullingFeeds::FeedRequestInterceptor::"
              "socket_stream_created: unable to set socket option SO_RCVBUF "
              "with size " << bufsize << ". Error " << error << ", reason "
                 << ACE_OS::strerror(error);
        
            throw Exception(ostr.str());
          }
        }
      }
    }
    
    void
    PullingFeeds::FeedRequestInterceptor::socket_stream_connected(
      El::Net::Socket::Stream& stream)
      throw(ServiceStopped, El::Exception)
    {
      if(Application::instance()->shutting_down())
      {
        throw
          ServiceStopped("PullingFeeds::FeedRequestInterceptor::"
                         "socket_stream_connected: shutting down");

      }      
    }    

    void
    PullingFeeds::FeedRequestInterceptor::socket_stream_read(
      const unsigned char* buff, size_t size)
      throw(ServiceStopped, El::Exception)
    {
      if(Application::instance()->shutting_down())
      {
        throw ServiceStopped("PullingFeeds::FeedRequestInterceptor::"
                             "socket_stream_read: shutting down");
      }      
    }

    void
    PullingFeeds::FeedRequestInterceptor::socket_stream_write(
      const unsigned char* buff, size_t size)
      throw(ServiceStopped, El::Exception)
    {
      if(Application::instance()->shutting_down())
      {
        throw ServiceStopped("PullingFeeds::FeedRequestInterceptor::"
                             "socket_stream_write: shutting down");
      }      
    }

    void
    PullingFeeds::FeedRequestInterceptor::post_last_build_date(
      RSS::Channel& channel) throw(SameLastBuildDate,
                                   SameContentLength,
                                   SameFirstChunkSize,
                                   El::Exception)
    {
//      if(channel != 0)
      {
        if(last_build_date != El::Moment::null &&
           channel.last_build_date == last_build_date)
        {
          std::ostringstream ostr;
          ostr << "PullingFeeds::FeedRequestInterceptor::"
            "post_last_build_date: same lastBuildDate="
               << last_build_date.rfc0822();

          throw SameLastBuildDate(ostr.str());
        }

        last_build_date = channel.last_build_date;
      }
      
      check_content_length();
    }

    void
    PullingFeeds::FeedRequestInterceptor::post_item(RSS::Channel& channel)
      throw(ServiceStopped,
            SameContentLength,
            SameFirstChunkSize,
            El::Exception)
    {
      if(Application::instance()->shutting_down())
      {
        throw
          ServiceStopped("PullingFeeds::FeedRequestInterceptor::post_item: "
                         "shutting down");
      }
      
      if(!check_item(channel))
      {
        check_content_length();
      }

      if(prev_recv_buffer_size)
      {
        //
        // restoring original buffer size to read the rest of content
        // in a high rate
        //
        
        unsigned int bufsize = prev_recv_buffer_size;
        prev_recv_buffer_size = 0;

        if(socket->set_option(SOL_SOCKET,
                              SO_RCVBUF,
                              &bufsize,
                              sizeof(bufsize)) == -1)
        {
          int error = ACE_OS::last_error();
          
          std::ostringstream ostr;
          ostr << "PullingFeeds::FeedRequestInterceptor::post_item: "
            "unable to set socket option SO_RCVBUF with size " << bufsize
               << ". Error " << error << ", reason "
               << ACE_OS::strerror(error);
          
          throw Exception(ostr.str());
        }
      }
      
    }
    
    void
    PullingFeeds::FeedRequestInterceptor::check_content_length()
      throw(SameContentLength, SameFirstChunkSize, El::Exception)
    {
      if(content_length > 0 && content_length == prev_content_length)
      {
        std::ostringstream ostr;
        ostr << "PullingFeeds::FeedRequestInterceptor::check_content_length: "
          "same content_length=" << content_length;
        
        throw SameContentLength(ostr.str());
      }

      if(first_chunk_size > 0 && chunks == 1 &&
         first_chunk_size == prev_first_chunk_size)
      {
        std::ostringstream ostr;
        ostr << "PullingFeeds::FeedRequestInterceptor::check_content_length: "
          "same first_chunk_size=" << first_chunk_size;
        
        throw SameFirstChunkSize(ostr.str());
      }      
    }

    bool
    PullingFeeds::FeedRequestInterceptor::check_item(
      const RSS::Channel& channel)
      throw(OldMessagesLimitReached, El::Exception)
    {
      if(last_messages != 0/* && channel != 0*/)
      {
        ItemList::const_reverse_iterator it = channel.items.rbegin();
        const Message::LocalId msg_code_id = it->code.id;

        Message::LocalCodeArray::const_iterator lit = last_messages->begin();
        for(; lit != last_messages->end() && msg_code_id != lit->id; lit++);

        if(lit == last_messages->end())
        {
          new_messages++;          
        }
        else
        {
          if(++old_messages > old_messages_limit)
          {
            std::ostringstream ostr;
            ostr << "PullingFeeds::FeedRequestInterceptor::check_item: "
              "reached old_messages_limit=" << old_messages_limit;
        
            throw OldMessagesLimitReached(ostr.str());
          }
        }
      }
      
      return new_messages > 0;
    }
    
  }
}
