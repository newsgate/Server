/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Feeds/RSS/PullerManager/FeedingPullers.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <unistd.h>
#include <limits.h>

#include <string>
#include <sstream>
#include <fstream>

#include <El/Moment.hpp>
#include <El/Lang.hpp>
#include <El/Country.hpp>
#include <El/MySQL/DB.hpp>

#include <ace/OS.h>
#include <ace/High_Res_Timer.h>

#include <Commons/Feed/Types.hpp>
#include <Commons/Message/TransportImpl.hpp>

#include <Services/Feeds/RSS/Commons/RSSFeedServices.hpp>

#include "FeedingPullers.hpp"
#include "PullerManagerMain.hpp"
#include "FeedRecord.hpp"

namespace
{
  const ACE_Time_Value WRITE_STATE_RETRY_PERIOD(1);
  const ACE_Time_Value WRITE_STAT_RETRY_PERIOD(1);
}

namespace NewsGate
{
  namespace RSS
  {
    //
    // FeedingPullers class
    //
    FeedingPullers::FeedingPullers(
      const ACE_Time_Value& puller_presence_poll_timeout,
      const FetchingFeedsConfig& fetching_feeds_config,
      const SavingFeedStateConfig& saving_feed_state_config,
      const SavingFeedStatConfig& saving_feed_stat_config,
      PullerManagerStateCallback* callback)
      throw(Exception, El::Exception)
        : PullerManagerState(callback, "FeedingPullers"),
          puller_presence_poll_timeout_(puller_presence_poll_timeout),
          fetching_feeds_config_(fetching_feeds_config),
          saving_feed_state_config_(saving_feed_state_config),
          saving_feed_stat_config_(saving_feed_stat_config),
          read_feeds_stamp_(0),
          current_feeds_stamp_(0),
          current_max_id_(0),
          current_read_id_(0)
    {
      std::ostringstream ostr;
      ostr << "FeedingPullers::FeedingPullers: "
           << callback->session() << " session";
        
      Application::logger()->trace(ostr.str(),
                                   Aspect::STATE,
                                   El::Logging::LOW);

      std::string cache_file = saving_feed_stat_config.cache_file();
      std::string::size_type pos = cache_file.rfind('/');

      if(pos == std::string::npos)
      {
        std::ostringstream ostr;
        ostr << "FeedingPullers::FeedingPullers: cache file name '"
             << cache_file << "' should be an absolute path name";

        throw Exception(ostr.str());
      }
      
      feed_stat_cache_dir_ = cache_file.substr(0, pos);

      El::Service::CompoundServiceMessage_var msg = new CheckFeeds(this);
      deliver_now(msg.in());

      ACE_Time_Value cur_time = ACE_OS::gettimeofday();
      
      msg = new CheckPullersPresence(this);
      deliver_at_time(msg.in(), cur_time + puller_presence_poll_timeout_);
      
      msg = new WriteFeedState(this);
      
      deliver_at_time(
        msg.in(),
        cur_time + ACE_Time_Value(saving_feed_state_config_.max_delay()));

      msg = new WriteFeedStat(this);
      
      deliver_at_time(
        msg.in(),
        cur_time + ACE_Time_Value(saving_feed_stat_config_.period()));
    }
    
    bool
    FeedingPullers::notify(El::Service::Event* event) throw(El::Exception)
    {
      if(PullerManagerState::notify(event))
      {
        return true;
      }
        
      if(dynamic_cast<CheckPullersPresence*>(event) != 0)
      {
        check_pullers_presence();
        return true;
      }

      if(dynamic_cast<CheckFeeds*>(event) != 0)
      {
        check_feeds();
        return true;
      }

      if(dynamic_cast<WriteFeedState*>(event) != 0)
      {
        write_feed_state();
        return true;
      }

      if(dynamic_cast<WriteFeedStat*>(event) != 0)
      {
        write_feed_stat();
        return true;
      }

      std::ostringstream ostr;
      ostr << "NewsGate::RSS::FeedingPullers::notify: unknown "
           << *event;
      
      El::Service::Error error(ostr.str(), this);
      callback_->notify(&error);
      
      return true;
    }

    ::NewsGate::RSS::SessionId
    FeedingPullers::puller_login(
      ::NewsGate::RSS::Puller_ptr puller_object)
      throw(NewsGate::RSS::PullerManager::ImplementationException,
            CORBA::SystemException)
    {
      if(started())
      {
        callback_->pullers_mistiming();      
      }
      
      return 0;
    }

    void
    FeedingPullers::puller_logout(
      ::NewsGate::RSS::SessionId session)
      throw(NewsGate::RSS::PullerManager::ImplementationException,
            CORBA::SystemException)
    {
      if(!started())
      {
        return;
      }
      
      callback_->pullers_mistiming();      
    }
    
    CORBA::Boolean
    FeedingPullers::feed_state(
      ::NewsGate::RSS::SessionId session,
      ::NewsGate::RSS::FeedStateUpdatePack* state_update)
      throw(NewsGate::RSS::PullerManager::Logout,
            NewsGate::RSS::PullerManager::ImplementationException,
            CORBA::SystemException)
    {
      if(!started())
      {
        return 0;
      }

      bool wrong_session =
        ((session >> 32) & UINT32_MAX) != callback_->session();

      if(wrong_session)
      {
        callback_->pullers_mistiming();

        NewsGate::RSS::PullerManager::Logout ex;
        ex.reason = CORBA::string_dup("Invalid session id");
        throw ex;
      }

      bool result = false;
      
      try
      {
        Transport::FeedStateUpdatePackImpl::Type* state_update_pack =
          dynamic_cast<Transport::FeedStateUpdatePackImpl::Type*>(
            state_update);

        if(state_update_pack == 0)
        {
          throw Exception(
            "FeedingPullers::feed_state: dynamic cast of state_update failed");
        }
          
        result =
          callback_->process_feed_state_update(session, state_update_pack);

        if(result)
        {
          state_update_pack->_add_ref();

          WriteGuard guard(srv_lock_);
          feed_state_updates_.push_back(state_update_pack);
        }
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "FeedingPullers::feed_state: "
            "El::Exception caught. Description:" << std::endl << e;

          NewsGate::RSS::PullerManager::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
        catch(const NewsGate::RSS::PullerManager::Logout&)
        {
          throw;
        }
        catch(...)
        {
        }
      }

      return result ? 1 : 0;
    }
    
    void
    FeedingPullers::ping(
      ::NewsGate::RSS::SessionId session)
      throw(NewsGate::RSS::PullerManager::Logout,
            NewsGate::RSS::PullerManager::ImplementationException,
            CORBA::SystemException)
    {
      if(!started())
      {
        return;
      }
      
      bool wrong_session =
        ((session >> 32) & UINT32_MAX) != callback_->session();
      
      if(wrong_session)
      {
        callback_->pullers_mistiming();

        NewsGate::RSS::PullerManager::Logout ex;
        ex.reason = CORBA::string_dup("Invalid session id");
        throw ex;
      }

      try
      {
        callback_->process_ping(session);
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "FeedingPullers::ping: "
            "El::Exception caught. Description:" << std::endl << e;

          NewsGate::RSS::PullerManager::Logout ex;
          ex.reason = ostr.str().c_str();
          throw ex;
        }
        catch(const NewsGate::RSS::PullerManager::Logout&)
        {
          throw;
        }
        catch(...)
        {
        }
      }
    }
    
    void
    FeedingPullers::write_feed_state() throw()
    {
      if(!started())
      {
        return;
      }

      try
      {
        bool has_state = false;
        
        {
          ReadGuard guard(srv_lock_);
          has_state = feed_state_updates_.begin() != feed_state_updates_.end();
        }

        if(has_state)
        {
          for(unsigned long written = 0;
              written < saving_feed_state_config_.packet_size(); )
          {
            Transport::FeedStateUpdatePackImpl::Var state_update_pack;
        
            {
              WriteGuard guard(srv_lock_);
              
              if(feed_state_updates_.begin() != feed_state_updates_.end())
              {
                state_update_pack = *feed_state_updates_.begin();
                feed_state_updates_.pop_front();
              }
              else
              {
                break;
              }
            }

            try
            {
              const Transport::FeedStateUpdateArray& state_updates =
                state_update_pack->entities();

              FeedRedirectList failed_redirects;
              
              {
                El::MySQL::Connection_var connection =
                  Application::instance()->dbase()->connect();
              
                for(Transport::FeedStateUpdateArray::const_iterator it =
                      state_updates.begin(); it != state_updates.end(); it++)
                {
                  write_state_update(connection.in(), *it, failed_redirects);
                }
              }

              disable_fail_redirected_feeds(failed_redirects);
              delete_expired_messages(state_updates);
              write_updated_messages(state_updates);
                    
              written += state_updates.size();
            }
            catch(...)
            {
              WriteGuard guard(srv_lock_);
              feed_state_updates_.push_front(state_update_pack);
              
              throw;
            }
          }
        }
        
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "FeedingPullers::write_feed_state: El::Exception caught. "
            "Description:" << std::endl << e;

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(...)
        {
        }
      } 

      try
      {
        bool has_state = false;
        
        {
          ReadGuard guard(srv_lock_);
          has_state = feed_state_updates_.begin() != feed_state_updates_.end();
        }

        El::Service::CompoundServiceMessage_var msg = new WriteFeedState(this);

        ACE_Time_Value tm = ACE_OS::gettimeofday() +
          (has_state ? WRITE_STATE_RETRY_PERIOD :
           ACE_Time_Value(saving_feed_state_config_.max_delay()));

        deliver_at_time(msg.in(), tm);
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "FeedingPullers::write_feed_state: El::Exception "
            "caught while scheduling the task. "
            "Description:" << std::endl << e;
          
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(...)
        {
        }
      } 
      
    }
    
    void
    FeedingPullers::disable_fail_redirected_feeds(
      const FeedRedirectList& failed_redirects)
      throw(Exception, El::Exception)
    {
      if(failed_redirects.empty())
      {
        return;
      }

      El::MySQL::Connection_var connection =
        Application::instance()->dbase()->connect();

      El::MySQL::Result_var qresult = connection->query("begin");
      

//      El::MySQL::Result_var qresult =
//        connection->query("lock tables FeedUpdateNum write, Feed write");

      try
      {
        qresult =
          connection->query("select update_num from FeedUpdateNum for update");
          
        FeedUpdateNum feeds_stamp(qresult.in());
            
        if(!feeds_stamp.fetch_row())
        {
          throw Exception(
            "NewsGate::Moderation::FeedManagerImpl::add_feeds: "
            "failed to get latest feed update number");
        }

        uint64_t update_num = (uint64_t)feeds_stamp.update_num() + 1;
        
        {
          std::ostringstream ostr;
          ostr << "update FeedUpdateNum set update_num=" << update_num;
              
          qresult = connection->query(ostr.str().c_str());
        }

        for(FeedRedirectList::const_iterator it = failed_redirects.begin();
            it != failed_redirects.end(); it++)
        {
          std::ostringstream ostr;
          ostr << "update Feed set update_num=" << update_num
               << ", comment='Redirected to existing feed "
               << it->new_location << "', status='P' where id=" << it->feed;
              
          std::string query = ostr.str();          
          qresult = connection->query(query.c_str());

          if(Application::will_trace(El::Logging::HIGH))
          {
            std::ostringstream ostr;
            ostr << "FeedingPullers::disable_fail_redirected_feeds: "
              "executing query:\n" << query;
          
            Application::logger()->trace(ostr.str(),
                                         Aspect::PULLING_FEEDS,
                                         El::Logging::HIGH);
          }
          
        }

        qresult = connection->query("commit");    
//        qresult = connection->query("unlock tables");
      }
      catch(...)
      {
        qresult = connection->query("rollback");
//        qresult = connection->query("unlock tables");
        throw;
      }

      qresult = connection->query("begin");
      
      try
      {
        qresult = connection->query(
          "select update_num from MessageFilterUpdateNum for update");

        MessageFilterUpdateNum num(qresult.in());
            
        if(!num.fetch_row())
        {
          throw Exception(
            "FeedingPullers::disable_fail_redirected_feeds: "
            "failed to get latest message filter update number");
        }

        uint64_t update_num = (uint64_t)num.update_num() + 1;

        {
          std::ostringstream ostr;
          ostr << "update MessageFilterUpdateNum set update_num="
               << update_num;
              
          qresult = connection->query(ostr.str().c_str());
        }
          
        qresult = connection->query("commit");
      }
      catch(...)
      {
        qresult = connection->query("rollback");
        throw;
      }
    }
    
    void
    FeedingPullers::write_state_update(
      El::MySQL::Connection* connection,
      const Transport::FeedStateUpdate& state_update,
      FeedRedirectList& failed_redirects)
      throw(Exception, El::Exception)
    {
      std::string channel_title;

      if(state_update.updated_fields &
         Transport::FeedStateUpdate::UF_CHANNEL_TITLE)
      {
        channel_title = connection->escape(state_update.channel.title.c_str());
      }
        
      std::string channel_description;

      if(state_update.updated_fields &
         Transport::FeedStateUpdate::UF_CHANNEL_DESCRIPTION)
      {
        channel_description =
          connection->escape(state_update.channel.description.c_str());
      }

      std::string channel_html_link;

      if(state_update.updated_fields &
         Transport::FeedStateUpdate::UF_CHANNEL_HTML_LINK)
      {
        channel_html_link =
          connection->escape(state_update.channel.html_link.c_str());
      }

      std::string channel_last_build_date;

      if(state_update.updated_fields &
         Transport::FeedStateUpdate::UF_CHANNEL_LAST_BUILD_DATE)
      {
        channel_last_build_date =
          El::Moment(ACE_Time_Value(state_update.channel.last_build_date)).
          iso8601(false);
      }
      
      std::string last_request_date;

      if(state_update.updated_fields &
         Transport::FeedStateUpdate::UF_LAST_REQUEST_DATE)
      {
        last_request_date =
          El::Moment(ACE_Time_Value(state_update.last_request_date)).
          iso8601(false);
      }
      
      std::string entropy_updated_date;

      if(state_update.updated_fields &
         Transport::FeedStateUpdate::UF_ENTROPY_UPDATED_DATE)
      {
        entropy_updated_date =
          El::Moment(ACE_Time_Value(state_update.entropy_updated_date)).
          iso8601(false);
      }
      
      std::string last_modified_hdr;

      if(state_update.updated_fields &
         Transport::FeedStateUpdate::UF_HTTP_LAST_MODIFIED_HDR)
      {
        last_modified_hdr =
          connection->escape(state_update.http.last_modified_hdr.c_str());
      }
      
      std::string etag_hdr;

      if(state_update.updated_fields &
         Transport::FeedStateUpdate::UF_HTTP_ETAG_HDR)
      {
        etag_hdr = connection->escape(state_update.http.etag_hdr.c_str());
      }

      std::string cache;

      if(state_update.updated_fields & Transport::FeedStateUpdate::UF_CACHE)
      {
        cache = connection->escape(state_update.cache.c_str(),
                                   state_update.cache.size());
      }
      
      bool update = channel_title.empty() && channel_description.empty() &&
        channel_html_link.empty();
      
      bool written = false;

      std::string query;
      
      if(update)
      {
        std::ostringstream ostr;
        ostr << "update RSSFeedState set ";

        if(feed_state_to_stream(state_update.updated_fields,
                                channel_title.c_str(),
                                channel_description.c_str(),
                                channel_html_link.c_str(),
                                state_update.channel.lang,
                                state_update.channel.country,
                                state_update.channel.ttl,
                                channel_last_build_date.c_str(),
                                last_request_date.c_str(),
                                last_modified_hdr.c_str(),
                                etag_hdr.c_str(),
                                state_update.http.content_length_hdr,
                                state_update.http.single_chunked,
                                state_update.http.first_chunk_size,
                                state_update.entropy,
                                entropy_updated_date.c_str(),
                                state_update.size,
                                state_update.heuristics_counter,
                                cache,
                                ostr))
        {
          ostr << " where feed_id=" << state_update.id;

          query = ostr.str();
          
          El::MySQL::Result_var result = connection->query(query.c_str());
          written = connection->affected_rows() == 1;
        }
        else
        {
          written = true;
        }
      }
      
      if(!written)
      {
        std::ostringstream ostr;
        ostr << "insert into RSSFeedState set feed_id=" << state_update.id
             << ", ";

        if(feed_state_to_stream(state_update.updated_fields,
                                channel_title.c_str(),
                                channel_description.c_str(),
                                channel_html_link.c_str(),
                                state_update.channel.lang,
                                state_update.channel.country,
                                state_update.channel.ttl,
                                channel_last_build_date.c_str(),
                                last_request_date.c_str(),
                                last_modified_hdr.c_str(),
                                etag_hdr.c_str(),
                                state_update.http.content_length_hdr,
                                state_update.http.single_chunked,
                                state_update.http.first_chunk_size,
                                state_update.entropy,
                                entropy_updated_date.c_str(),
                                state_update.size,
                                state_update.heuristics_counter,
                                cache,
                                ostr))
        {
          ostr << " on duplicate key update ";
          
          feed_state_to_stream(state_update.updated_fields,
                               channel_title.c_str(),
                               channel_description.c_str(),
                               channel_html_link.c_str(),
                               state_update.channel.lang,
                               state_update.channel.country,
                               state_update.channel.ttl,
                               channel_last_build_date.c_str(),
                               last_request_date.c_str(),
                               last_modified_hdr.c_str(),
                               etag_hdr.c_str(),
                               state_update.http.content_length_hdr,
                               state_update.http.single_chunked,
                               state_update.http.first_chunk_size,
                               state_update.entropy,
                               entropy_updated_date.c_str(),
                               state_update.size,
                               state_update.heuristics_counter,
                               cache,
                               ostr);
          
          query = ostr.str();

          El::MySQL::Result_var result = connection->query(query.c_str());

          written = connection->affected_rows() == 1 ||
            connection->affected_rows() == 2;
        }
        else
        {
          written = true;
        }
      }

      if(!written)
      {
        std::ostringstream ostr;
        ostr << "FeedingPullers::write_state_update: problem writing state: "
          "affected rows " << connection->affected_rows() << " for query\n"
             << query;

        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);

        return;
      }

      if(Application::will_trace(El::Logging::HIGH))
      {
        std::ostringstream ostr;
        ostr << "FeedingPullers::write_state_update: executing query:\n"
             << query;
          
        Application::logger()->trace(ostr.str(),
                                     Aspect::PULLING_FEEDS,
                                     El::Logging::HIGH);
      }

      if(state_update.updated_fields &
         Transport::FeedStateUpdate::UF_HTTP_NEW_LOCATION)
      {
        std::string query;
        
        {
          std::ostringstream ostr;
          ostr << "select id, url, status from Feed where id=" <<
            state_update.id;
          
          query = ostr.str();        
        }

        El::MySQL::Result_var result = connection->query(query.c_str());
        
        FeedStatusRecord record(result.in());
        std::string current_location;

        if(record.fetch_row())
        {
          current_location = record.url();
          
          std::ostringstream ostr;
          
          ostr << "insert into FeedFormerUrl set feed_id=" << state_update.id
               << ", url='" << connection->escape(current_location.c_str())
               << "' on duplicate key update replaced=DEFAULT";

          query = ostr.str();
          result = connection->query(query.c_str());
        }

//        std::cerr << "Move from '" << current_location << "' to '"
//                  << state_update.http.new_location << "'\n";

        std::string new_location =
          connection->escape(state_update.http.new_location.c_str());
        
        {
          std::ostringstream ostr;
          ostr << "update ignore Feed set url='"
               << new_location << "' where id=" << state_update.id;

          query = ostr.str();
        }
        
        result = connection->query(query.c_str());

        if(connection->affected_rows() == 0)
        {
          FeedRedirect rd;
          rd.feed = state_update.id;
          rd.new_location = new_location;
          failed_redirects.push_back(rd);
        }

        if(Application::will_trace(El::Logging::HIGH))
        {
          std::ostringstream ostr;
          ostr << "FeedingPullers::write_state_update: "
               << connection->affected_rows() << " rows affected by query:\n"
               << query;
          
          Application::logger()->trace(ostr.str(),
                                       Aspect::PULLING_FEEDS,
                                       El::Logging::HIGH);
        }
      }

      if(state_update.updated_fields &
         Transport::FeedStateUpdate::UF_CHANNEL_TYPE)
      {
        std::ostringstream ostr;
        ostr << "update ignore Feed set type=" << state_update.channel.type
             << " where id=" << state_update.id;

        std::string query = ostr.str();
        El::MySQL::Result_var result = connection->query(query.c_str());

        if(Application::will_trace(El::Logging::HIGH))
        {
          std::ostringstream ostr;
          ostr << "FeedingPullers::write_state_update: "
               << connection->affected_rows() << " rows affected by query:\n"
               << query;
          
          Application::logger()->trace(ostr.str(),
                                       Aspect::PULLING_FEEDS,
                                       El::Logging::HIGH);
        }        
      }
    }

    void
    FeedingPullers::delete_expired_messages(
      const Transport::FeedStateUpdateArray& state_updates)
      throw(Exception, El::Exception)
    {      
      std::ostringstream ostr;
      ostr << "delete from RSSFeedMessageCodes where ";

      bool written = false;
      
      for(Transport::FeedStateUpdateArray::const_iterator it =
            state_updates.begin(); it != state_updates.end(); it++)
      {
        const Transport::FeedStateUpdate& state_update = *it;

        if(state_update.updated_fields &
           Transport::FeedStateUpdate::UF_EXPIRED_MESSAGES)
        {
          if(written)
          {
            ostr << " or ";
          }
          else
          {
            written = true;
          }
          
          ostr << "feed_id=" << state_update.id << " and ( ";

          const NewsGate::Message::LocalIdArray& expired_messages =
            state_update.expired_messages;
            
          for(NewsGate::Message::LocalIdArray::const_iterator eit =
                expired_messages.begin(); eit != expired_messages.end(); eit++)
          {
            if(eit != expired_messages.begin())
            {
              ostr << " or ";
            }
            
            ostr << "msg_id=" << *eit;
          }

          ostr << " )";
        }
          
      }

      if(written)
      {
        std::string query = ostr.str();
          
        if(Application::will_trace(El::Logging::HIGH))
        {
          std::ostringstream ostr;
          ostr << "FeedingPullers::delete_expired_messages: executing query:\n"
               << query;
          
          Application::logger()->trace(ostr.str(),
                                       Aspect::PULLING_FEEDS,
                                       El::Logging::HIGH);
        }
        
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();
        
        El::MySQL::Result_var result = connection->query(query.c_str());
      }
    }

    void
    FeedingPullers::write_updated_messages(
      const Transport::FeedStateUpdateArray& state_updates)
      throw(Exception, El::Exception)
    {
      std::string filename = saving_feed_state_config_.cache_file();
      std::fstream file(filename.c_str(), ios::out);

      if(!file.is_open())
      {
        std::ostringstream ostr;
        ostr << "FeedingPullers::write_updated_messages: failed to open file '"
             << filename << "' for write access";
        
        throw Exception(ostr.str());
      }

//      bool written = false;
      bool first_line = true;
      
      for(Transport::FeedStateUpdateArray::const_iterator it =
            state_updates.begin(); it != state_updates.end(); it++)
      {
        const Transport::FeedStateUpdate& state_update = *it;
          
        if(state_update.updated_fields &
           Transport::FeedStateUpdate::UF_UPDATED_MESSAGES)
        {
          const NewsGate::Message::LocalCodeArray& updated_messages =
            state_update.updated_messages;
            
          for(NewsGate::Message::LocalCodeArray::const_iterator uit =
                updated_messages.begin(); uit != updated_messages.end(); uit++)
          {
//            written = true;

            if(first_line)
            {
              first_line = false;
            }
            else
            {
              file << std::endl;
            }
            
            file << state_update.id << "\t" << uit->id << "\t"
                 << uit->published;
          }
        }

        if(state_update.updated_fields &
           Transport::FeedStateUpdate::UF_NEW_MESSAGES)
        {
          const NewsGate::Message::LocalCodeArray& new_messages =
            state_update.new_messages;
            
          for(NewsGate::Message::LocalCodeArray::const_iterator nit =
                new_messages.begin(); nit != new_messages.end(); nit++)
          {
//            written = true;

            if(first_line)
            {
              first_line = false;
            }
            else
            {
              file << std::endl;
            }
            
            file << state_update.id << "\t" << nit->id << "\t"
                 << nit->published;
          }
        }
      }

      if(file.bad() || file.fail())
      {
        file.close();
        unlink(filename.c_str());
        
        std::ostringstream ostr;
        ostr << "FeedingPullers::write_updated_messages: failed to write "
          "to file '" << filename << "'";
        
        throw Exception(ostr.str());
      }
    
      file.close();

      El::MySQL::Connection_var connection =
        Application::instance()->dbase()->connect();
      
      El::MySQL::Result_var result =
        connection->query("delete from RSSFeedMessageCodesBuff");

      std::string query =
        std::string("LOAD DATA INFILE '") +
        connection->escape(filename.c_str()) +
        "' REPLACE INTO TABLE RSSFeedMessageCodesBuff character set binary";
      
      result = connection->query(query.c_str());

      result = connection->query(
        "INSERT INTO RSSFeedMessageCodes "
        "SELECT * "
        "FROM RSSFeedMessageCodesBuff "
        "ON DUPLICATE KEY UPDATE "
        "published = published_");

      unlink(filename.c_str());
    }
    
    void
    FeedingPullers::write_stat(El::MySQL::Connection* connection,
                               const char* file_name)
      throw(Exception, El::Exception)
    {
      El::MySQL::Result_var result =
        connection->query("delete from RSSFeedStatBuff");

      std::string query =
        std::string("LOAD DATA INFILE '") + connection->escape(file_name) +
        "' INTO TABLE RSSFeedStatBuff character set binary";
      
      result = connection->query(query.c_str());

      result = connection->query(
        "INSERT INTO RSSFeedStat "
        "SELECT * "
        "FROM RSSFeedStatBuff "
        "ON DUPLICATE KEY UPDATE "
        "requests = requests + requests_, "
        "failed = failed + failed_, "
        "unchanged = unchanged + unchanged_, "
        "not_modified = not_modified + not_modified_, "
        "presumably_unchanged = presumably_unchanged + presumably_unchanged_, "
        "has_changes = has_changes + has_changes_, "
        "wasted = wasted + wasted_, "
        "outbound = outbound + outbound_, "
        "inbound = inbound + inbound_, "
        "requests_duration = requests_duration + requests_duration_, "
        "messages = messages + messages_, "
        "messages_size = messages_size + messages_size_, "
        "messages_delay = messages_delay + messages_delay_, "
        "max_message_delay = if(max_message_delay_ > max_message_delay, "
            "max_message_delay_, max_message_delay), "
        "mistiming = if(abs(mistiming) > abs(mistiming_), "
            "mistiming, mistiming_)");
      
      unlink(file_name);
    }
    
    bool
    FeedingPullers::feed_state_to_stream(unsigned long updated_fields,
                                         const char* channel_title,
                                         const char* channel_description,
                                         const char* channel_html_link,
                                         uint16_t channel_lang,
                                         uint16_t channel_country,
                                         uint16_t channel_ttl,
                                         const char* channel_last_build_date,
                                         const char* last_request_date,
                                         const char* last_modified_hdr,
                                         const char* etag_hdr,
                                         int64_t content_length_hdr,
                                         int8_t single_chunked,
                                         int64_t first_chunk_size,
                                         uint32_t entropy,
                                         const char* entropy_updated_date,
                                         uint32_t size,
                                         int32_t heuristics_counter,
                                         const std::string& cache,
                                         std::ostream& ostr)
      throw(Exception, El::Exception)
    {
      bool written = false;
      
      if(updated_fields & Transport::FeedStateUpdate::UF_CHANNEL_TITLE)
      {
        ostr << "channel_title='" << channel_title << "'";
        written = true;
      }

      if(updated_fields & Transport::FeedStateUpdate::UF_CHANNEL_DESCRIPTION)
      {
        if(written)
        {
          ostr << ", ";
        }
        else
        {
          written = true;
        }
        
        ostr << "channel_description='" << channel_description << "'";
      }
      
      if(updated_fields & Transport::FeedStateUpdate::UF_CHANNEL_HTML_LINK)
      {
        if(written)
        {
          ostr << ", ";
        }
        else
        {
          written = true;
        }
        
        ostr << "channel_html_link='" << channel_html_link << "'";
      }
      
      if(updated_fields & Transport::FeedStateUpdate::UF_CHANNEL_LANG)
      {
        if(written)
        {
          ostr << ", ";
        }
        else
        {
          written = true;
        }
        
        ostr << "channel_lang=" << channel_lang;
      }

      if(updated_fields & Transport::FeedStateUpdate::UF_CHANNEL_COUNTRY)
      {
        if(written)
        {
          ostr << ", ";
        }
        else
        {
          written = true;
        }
        
        ostr << "channel_country=" << channel_country;
      }

      if(updated_fields & Transport::FeedStateUpdate::UF_CHANNEL_TTL)
      {
        if(written)
        {
          ostr << ", ";
        }
        else
        {
          written = true;
        }
        
        ostr << "channel_ttl=" << channel_ttl;
      }

      if(updated_fields &
         Transport::FeedStateUpdate::UF_CHANNEL_LAST_BUILD_DATE)
      {
        if(written)
        {
          ostr << ", ";
        }
        else
        {
          written = true;
        }
        
        ostr << "channel_last_build_date='" << channel_last_build_date
             << "'";
      }
        
      if(updated_fields & Transport::FeedStateUpdate::UF_LAST_REQUEST_DATE)
      {
        if(written)
        {
          ostr << ", ";
        }
        else
        {
          written = true;
        }
        
        ostr << "last_request_date='" << last_request_date << "'";
      }

      if(updated_fields &
         Transport::FeedStateUpdate::UF_HTTP_LAST_MODIFIED_HDR)
      {
        if(written)
        {
          ostr << ", ";
        }
        else
        {
          written = true;
        }
        
        ostr << "last_modified_hdr='" << last_modified_hdr << "'";
      }

      if(updated_fields & Transport::FeedStateUpdate::UF_HTTP_ETAG_HDR)
      {
        if(written)
        {
          ostr << ", ";
        }
        else
        {
          written = true;
        }
        
        ostr << "etag_hdr='" << etag_hdr << "'";
      }

      if(updated_fields &
         Transport::FeedStateUpdate::UF_HTTP_CONTENT_LENGTH_HDR)
      {
        if(written)
        {
          ostr << ", ";
        }
        else
        {
          written = true;
        }
        
        ostr << "content_length_hdr=" << content_length_hdr;
      }

      if(updated_fields & Transport::FeedStateUpdate::UF_ENTROPY)
      {
        if(written)
        {
          ostr << ", ";
        }
        else
        {
          written = true;
        }
        
        ostr << "entropy=" << entropy;
      }

      if(updated_fields & Transport::FeedStateUpdate::UF_ENTROPY_UPDATED_DATE)
      {
        if(written)
        {
          ostr << ", ";
        }
        else
        {
          written = true;
        }
        
        ostr << "entropy_updated_date='" << entropy_updated_date << "'";
      }

      if(updated_fields & Transport::FeedStateUpdate::UF_SIZE)
      {
        if(written)
        {
          ostr << ", ";
        }
        else
        {
          written = true;
        }
        
        ostr << "size=" << size;
      }
      
      if(updated_fields & Transport::FeedStateUpdate::UF_HTTP_SINGLE_CHUNKED)
      {
        if(written)
        {
          ostr << ", ";
        }
        else
        {
          written = true;
        }
        
        ostr << "single_chunked=" << (long)single_chunked;
      }
      
      if(updated_fields & Transport::FeedStateUpdate::UF_HTTP_FIRST_CHUNK_SIZE)
      {
        if(written)
        {
          ostr << ", ";
        }
        else
        {
          written = true;
        }
        
        ostr << "first_chunk_size=" << first_chunk_size;
      }

      if(updated_fields & Transport::FeedStateUpdate::UF_HEURISTIC_COUNTER)
      {
        if(written)
        {
          ostr << ", ";
        }
        else
        {
          written = true;
        }
        
        ostr << "heuristics_counter=" << heuristics_counter;
      }

      if(updated_fields & Transport::FeedStateUpdate::UF_CACHE)
      {
        if(written)
        {
          ostr << ", ";
        }
        else
        {
          written = true;
        }
        
        ostr << "cache='" << cache << "'";
      }

      return written;
    }

    void
    FeedingPullers::write_feed_stat() throw()
    {
      if(!started())
      {
        return;
      }

      ACE_DIR* dir = 0;
      bool has_more_stat = false;
      
      try
      {
        dir = ACE_OS_Dirent::opendir(feed_stat_cache_dir_.c_str());

        if(dir == 0)
        {
          int error = ACE_OS::last_error();

          std::ostringstream ostr;
          ostr << "FeedingPullers::write_feed_stat: ACE_OS_Dirent::opendir("
               << feed_stat_cache_dir_
               << ") failed. Errno " << error << ". Description:\n"
               << ACE_OS::strerror(error);

          throw Exception(ostr.str());
        }

        typedef std::list<std::string> FileList;
        FileList stat_files;

        dirent* dir_entry = 0;
        unsigned long files = 0;
        size_t len = saving_feed_stat_config_.cache_file().length();
        
        while(files < saving_feed_stat_config_.files_at_once() &&
              (dir_entry = ACE_OS_Dirent::readdir(dir)) != 0)
        {
          std::string fname = feed_stat_cache_dir_ + "/" + dir_entry->d_name;
          
          if(!strncmp(fname.c_str(),
                      saving_feed_stat_config_.cache_file().c_str(),
                      len))
          {
            stat_files.push_back(fname);
            files++;
          }
          
        }

        has_more_stat = dir_entry != 0;

        ACE_OS_Dirent::closedir(dir);
        dir = 0;

        ACE_Time_Value cur_time = ACE_OS::gettimeofday();
        bool do_cleanup = false;
        
        {
          WriteGuard lock(srv_lock_);
          
          do_cleanup = next_stat_cleanup_ <= cur_time;

          if(do_cleanup)
          {
            next_stat_cleanup_ =
              (next_stat_cleanup_ == ACE_Time_Value::zero ? cur_time :
               next_stat_cleanup_) + ACE_Time_Value(86400);
          }
        }
          
        if(files || do_cleanup)
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          if(do_cleanup)
          {
            cur_time.sec((cur_time.sec() / 86400 -
                          saving_feed_stat_config_.keep_days()) * 86400);
            
            cur_time.usec(0);

            std::ostringstream ostr;
            ostr << "delete from RSSFeedStat where date < '"
                 << El::Moment(cur_time).iso8601(false) << "'";
            
            El::MySQL::Result_var result =
              connection->query(ostr.str().c_str());
          }

          if(files)
          {
            for(FileList::const_iterator it = stat_files.begin();
                it != stat_files.end(); it++)
            {
              write_stat(connection.in(), it->c_str());
            }
          }
        }
      }
      catch(const El::Exception& e)
      {
        if(dir)
        {
          ACE_OS_Dirent::closedir(dir);
        }
        
        try
        {
          std::ostringstream ostr;
          ostr << "FeedingPullers::write_feed_stat: El::Exception caught. "
            "Description:" << std::endl << e;

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(...)
        {
          El::Service::Error error(
            "FeedingPullers::write_feed_stat: unknown error", this);
          
          callback_->notify(&error);
        }
      } 

      try
      {
        El::Service::CompoundServiceMessage_var msg = new WriteFeedStat(this);

        ACE_Time_Value tm = ACE_OS::gettimeofday() +
          (has_more_stat ? WRITE_STAT_RETRY_PERIOD :
           ACE_Time_Value(saving_feed_stat_config_.period()));

        deliver_at_time(msg.in(), tm);
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "FeedingPullers::write_feed_stat: El::Exception "
            "caught while scheduling the task. "
            "Description:" << std::endl << e;
          
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(...)
        {
        }
      } 
      
    }
    
    void
    FeedingPullers::check_pullers_presence() throw()
    {
      if(!started())
      {
        return;
      }
      
      try
      {
        if(!callback_->check_pullers_presence(puller_presence_poll_timeout_))
        {          
          callback_->pullers_mistiming();
          return;
        }
      
        El::Service::CompoundServiceMessage_var msg =
          new CheckPullersPresence(this);
        
        deliver_at_time(
          msg.in(),
          ACE_OS::gettimeofday() + puller_presence_poll_timeout_);        
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "FeedingPullers::"
            "check_pullers_presence: El::Exception caught. "
            "Description:" << std::endl << e;

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(...)
        {
        }
      } 
    }

    void
    FeedingPullers::check_feeds() throw()
    {
      if(!started())
      {
        return;
      }
      
      try
      {
        bool need_fetch_feeds = false;
          
        if(current_feeds_stamp_ != 0)
        {
          // Need to continue reading updated feeds

          need_fetch_feeds = true;
        }
        else
        {
          // Check if there is an update of Feed

          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();
          
//          El::MySQL::Result_var result = connection->query(
//            "lock table FeedUpdateNum read");

          El::MySQL::Result_var result;

          try
          {
            result = connection->query(
              "select update_num from FeedUpdateNum");
          
            FeedUpdateNum feeds_stamp(result.in());
            
            if(!feeds_stamp.fetch_row())
            {
              throw Exception("FeedingPullers::check_feeds: "
                              "failed to get latest feed update number");
            }

            current_feeds_stamp_ = feeds_stamp.update_num();
            need_fetch_feeds = current_feeds_stamp_ > read_feeds_stamp_;
          }
          catch(...)
          {
//            result = connection->query("unlock tables");
            throw;
          }
          
//          result = connection->query("unlock tables");
        }
        
        if(Application::will_trace(El::Logging::MIDDLE))
        {
          std::ostringstream ostr;
          ostr << "FeedingPullers::check_feeds: ";

          if(need_fetch_feeds)
          {
            ostr << "need fetch feeds (current feeds stamp "
                 << read_feeds_stamp_ << ", new one "
                 << current_feeds_stamp_ << ", read up to id "
                 << current_read_id_ << ", max id " << current_max_id_ << ")";
          }
          else
          {
            ostr << "no changes (current feeds stamp "
                 << read_feeds_stamp_ << ")";
          }
          
          Application::logger()->trace(ostr.str(),
                                       Aspect::FEED_MANAGEMENT,
                                       El::Logging::MIDDLE);
        }
         
        if(need_fetch_feeds)
        {
          fetch_updated_feeds();
        }
        else
        {
          current_feeds_stamp_ = 0;
        }
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "FeedingPullers::"
            "check_feeds: El::Exception caught. "
            "Description:" << std::endl << e;

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(...)
        {
        }
      } 
      
      try
      {
        El::Service::CompoundServiceMessage_var msg = new CheckFeeds(this);
        
        deliver_at_time(
          msg.in(),
          ACE_OS::gettimeofday() +
            ACE_Time_Value(fetching_feeds_config_.check_feeds_period()));
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "FeedingPullers::"
            "check_feeds: El::Exception caught while scheduling the task. "
            "Description:" << std::endl << e;
          
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(...)
        {
        }
      } 
    }

    void
    FeedingPullers::fetch_updated_feeds() throw(Exception, El::Exception)
    {
      PullerSessionVector pullers = callback_->pullers();
      size_t puller_count = pullers.size();

      if(!puller_count)
      {
        callback_->pullers_mistiming();
        
        throw Exception(
          "FeedingPullers::fetch_updated_feeds: "
          "pullers number unexpectedly is 0. Mistiming reported, "
          "pullers relogin to follow");
      }

      El::MySQL::Connection_var connection =
        Application::instance()->dbase()->connect();

      El::MySQL::Result_var result;
      
//      El::MySQL::Result_var result = connection->query(
//      "lock tables Feed read, RSSFeedState read, RSSFeedMessageCodes read");

      if(current_max_id_ == 0)
      {
        result = connection->query("select max(id) as value from Feed");

        MaxId max_id(result.in());
        if(!max_id.fetch_row())
        {
          throw Exception(
            "FeedingPullers::fetch_updated_feeds: "
            "failed to get feed max id");
        }
        
        current_max_id_ = max_id.value();
      }

      uint64_t max_id = current_read_id_ +
        fetching_feeds_config_.feeds_per_query();
      
      bool last_query = false;
      
      if(max_id >= current_max_id_)
      {
        max_id = current_max_id_;
        last_query = true;
      }

      std::string query;
      {
        std::ostringstream ostr;

        ostr << "select Feed.id as id, Feed.type as type, Feed.url as url, "
          "Feed.encoding as encoding, Feed.space as space, Feed.lang as lang, "
          "Feed.country as country, Feed.status as status, "
          "Feed.keywords as keywords, "
          "Feed.adjustment_script as adjustment_script, "
          "RSSFeedState.channel_title as channel_title, "
          "RSSFeedState.channel_description as channel_description, "
          "RSSFeedState.channel_html_link as channel_html_link, "
          "RSSFeedState.channel_lang as channel_lang, "
          "RSSFeedState.channel_country as channel_country, "
          "RSSFeedState.channel_ttl as channel_ttl, "
          "RSSFeedState.channel_last_build_date as channel_last_build_date, "
          "RSSFeedState.last_request_date as last_request_date, "
          "RSSFeedState.last_modified_hdr as last_modified_hdr, "
          "RSSFeedState.etag_hdr as etag_hdr, "
          "RSSFeedState.content_length_hdr as content_length_hdr, "
          "RSSFeedState.entropy as entropy, "
          "RSSFeedState.entropy_updated_date as entropy_updated_date, "
          "RSSFeedState.size as size, "
          "RSSFeedState.single_chunked as single_chunked, "
          "RSSFeedState.first_chunk_size as first_chunk_size, "
          "RSSFeedState.heuristics_counter as heuristics_counter, "
          "RSSFeedState.cache as cache "
          "from Feed left join "
          "RSSFeedState on Feed.id=RSSFeedState.feed_id "
          "where update_num>" << read_feeds_stamp_
             << " and update_num<=" << current_feeds_stamp_ << " and id>"
             << current_read_id_ << " and id<=" << max_id << " and type>="
             << NewsGate::Feed::TP_RSS << " and type<="
             << NewsGate::Feed::TP_HTML;

        query = ostr.str();
      }
        
      if(Application::will_trace(El::Logging::MIDDLE))
      {
        std::ostringstream ostr;
        ostr << "FeedingPullers::fetch_updated_feeds: "
            "making query " << query;
            
          Application::logger()->trace(ostr.str(),
                                       Aspect::FEED_MANAGEMENT,
                                       El::Logging::MIDDLE);
      }
    
      result = connection->query(query.c_str());
      FeedRecord record(result.in());
      
      std::ostringstream ostr;        
      if(Application::will_trace(El::Logging::HIGH))
      {
        ostr << "FeedingPullers::fetch_updated_feeds: "
          "pulling result:\n";
      }

      typedef std::vector<Transport::FeedPackImpl::Var> FeedPacks;
      
      FeedPacks feed_packs(puller_count);
      feed_packs.resize(puller_count);
      
      while(record.fetch_row())
      {
        if(Application::will_trace(El::Logging::HIGH))
        {
          ostr << "    id=" << record.id()
               << ", status=" << record.status().value()
               << ", type=" << record.type().value()
               << ", url=" << record.url().value()
               << ", encoding=" << record.encoding().value()
               << ", space=" << record.space().value()
               << ", lang=" << record.lang().value()
               << ", country=" << record.country().value()
               << ", keywords="
               << (record.keywords().is_null() ?
                   "" : record.keywords().value())
               << ", adjustment_script="
               << (record.adjustment_script().is_null() ?
                   "" : record.adjustment_script().value())
               << ", channel_title=" << record.channel_title().value()
               << ", channel_description="
               << record.channel_description().value()
               << ", channel_html_link="
               << record.channel_html_link().value()
               << ", channel_lang=" << El::Lang(
                 record.channel_lang().is_null() ? El::Lang::EC_NUL :
                 (El::Lang::ElCode)record.channel_lang().value())
               << ", channel_country=" << El::Country(
                 record.channel_country().is_null() ? El::Country::EC_NUL :
                 (El::Country::ElCode)record.channel_country().value())
               << ", channel_ttl=" << record.channel_ttl()
               << ", channel_last_build_date="
               << record.channel_last_build_date().value()
               << ", last_request_date=" << record.last_request_date().value()
               << ", last_modified_hdr=" << record.last_modified_hdr().value()
               << ", etag_hdr=" << record.etag_hdr().value()
               << ", content_length_hdr=" << record.content_length_hdr()
               << ", entropy=" << record.entropy()
               << ", entropy_updated_date="
               << record.entropy_updated_date().value()
               << ", size=" << record.size()
               << ", single_chunked=";

          if(record.single_chunked().is_null())
          {
            ostr << "null";
          }
          else
          {
            ostr << (long)record.single_chunked();
          }
          
          ostr << ", first_chunk_size=" << record.first_chunk_size()
               << ", heuristics_counter=" << record.heuristics_counter()
               << ", cache=" << (record.cache().is_null() ? 0 :
                                 record.cache().length())
               << std::endl;
        }

        if(record.type() == 0 ||
           record.type() >= NewsGate::Feed::TP_TYPES_COUNT)
        {
          std::ostringstream ostr;
          ostr << "FeedingPullers::fetch_updated_feeds: uexpected type "
               << record.type() << " for feed " << record.id();

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
          
          continue;
        }

        if(record.space() >= NewsGate::Feed::SP_SPACES_COUNT)
        {
          std::ostringstream ostr;
          ostr << "FeedingPullers::fetch_updated_feeds: uexpected space "
               << record.space() << " for feed " << record.id();

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
          
          continue;
        }

        char status = record.status()[0];

        if(status != 'E' && !read_feeds_stamp_)
        {
          // Do not need to pass disabled feeds when filling puller initially
          continue;
        }
          
        size_t index = record.id() % puller_count;

        Transport::FeedPackImpl::Var feed_pack = feed_packs[index];

        if(feed_pack.in() == 0)
        {
          feed_pack = new Transport::FeedPackImpl::Type(
            new Transport::FeedArray());
          
          feed_packs[index] = feed_pack;
        }
        
        Transport::FeedArray& feed_seq = feed_pack->entities();

        size_t len = feed_seq.size();
        feed_seq.resize(len + 1);

        Transport::Feed& feed = feed_seq[len];
        Transport::FeedState& feed_state = feed.state;
        
        feed_state.id = record.id();
        feed.status = status;
        
        feed_state.last_request_date = 0;
        feed_state.channel.lang = 0;
        feed_state.channel.country = 0;
        feed_state.channel.ttl = 0;
        feed_state.channel.last_build_date = 0;
        feed_state.http.content_length_hdr = -2; // -2 means undefined,
                                                 // -1 header is absent,
                                                 // other - content length

        feed_state.http.single_chunked = -1;
        feed_state.http.first_chunk_size = -2;
        feed_state.entropy = 0;
        feed_state.entropy_updated_date = 0;
        feed_state.size = 0;
        feed_state.heuristics_counter = 0;
        feed_state.last_messages.clear();
        feed.url = record.url().c_str();
        
        if(status == 'E')
        {
          feed.encoding = record.encoding();
          feed.state.channel.type = record.type();
          feed.space = record.space();
          feed.lang = record.lang();
          feed.country = record.country();

          if(!record.keywords().is_null())
          {
            feed.keywords = record.keywords();
          }

          if(!record.adjustment_script().is_null())
          {
            feed.adjustment_script = record.adjustment_script();
          }

          if(!record.channel_title().is_null())
          {
            feed_state.channel.title = record.channel_title().c_str();
          }
          
          if(!record.channel_description().is_null())
          {
            feed_state.channel.description =
              record.channel_description().c_str();
          }
          
          if(!record.channel_html_link().is_null())
          {
            feed_state.channel.html_link =
              record.channel_html_link().c_str();
          }
          
          if(!record.channel_lang().is_null())
          {
            feed_state.channel.lang = record.channel_lang().value();
          }
          
          if(!record.channel_country().is_null())
          {
            feed_state.channel.country = record.channel_country().value();
          }
          
          if(!record.channel_ttl().is_null())
          {
            feed_state.channel.ttl = record.channel_ttl().value();
          }
          
          if(!record.channel_last_build_date().is_null())
          {
            feed_state.channel.last_build_date =
              ACE_Time_Value(record.channel_last_build_date().moment()).sec();
          }
          
          if(!record.last_request_date().is_null())
          {
            feed_state.last_request_date =
              ACE_Time_Value(record.last_request_date().moment()).sec();
          }
          
          if(!record.last_modified_hdr().is_null())
          {
            feed_state.http.last_modified_hdr =
              record.last_modified_hdr().c_str();
          }

          if(!record.etag_hdr().is_null())
          {
            feed_state.http.etag_hdr = record.etag_hdr().c_str();
          }

          if(!record.content_length_hdr().is_null())
          {
            feed_state.http.content_length_hdr =
              record.content_length_hdr().value();
          }

          if(!record.single_chunked().is_null())
          {
            feed_state.http.single_chunked = record.single_chunked().value();
          }

          if(!record.first_chunk_size().is_null())
          {
            feed_state.http.first_chunk_size =
              record.first_chunk_size().value();
          }

          if(!record.entropy().is_null())
          {
            feed_state.entropy = record.entropy().value();
          }

          if(!record.entropy_updated_date().is_null())
          {
            feed_state.entropy_updated_date =
              ACE_Time_Value(record.entropy_updated_date().moment()).sec();
          }
          
          if(!record.size().is_null())
          {
            feed_state.size = record.size().value();
          }

          if(!record.heuristics_counter().is_null())
          {
            feed_state.heuristics_counter =
              record.heuristics_counter().value();
          }

          if(!record.cache().is_null())
          {
            feed_state.cache = record.cache().value();
          }          
        }
      }

      for(FeedPacks::iterator it = feed_packs.begin(); it != feed_packs.end();
          it++)
      {
        Transport::FeedPackImpl::Var feed_pack = *it;

        if(feed_pack.in() != 0)
        {
          Transport::FeedArray& feed_seq = feed_pack->entities();

          for(Transport::FeedArray::iterator it = feed_seq.begin();
              it != feed_seq.end(); it++)
          {
            Transport::Feed& feed = *it;

            if(feed.status == 'E')
            {
              NewsGate::Message::LocalCodeArray& codes =
                feed.state.last_messages;
              
              std::ostringstream ostr;
              ostr <<
                "select msg_id, published from RSSFeedMessageCodes "
                "where feed_id=" << feed.state.id;
                
              result = connection->query(ostr.str().c_str());
              
              FeedMessageCode record(result.in());
      
              while(record.fetch_row())
              {
                NewsGate::Message::LocalCode code;
                
                code.id = record.msg_id();
                code.published = record.published();

                NewsGate::Message::LocalCodeArray::iterator cit =
                  codes.begin();
                
                for(;cit != codes.end() && code.published < cit->published;
                    ++cit);

                codes.insert(cit, code);
              }

              if(Application::will_trace(El::Logging::HIGH))
              {
                ostr << ", last_messages=" << codes.string();
              }
            }
          }          
        }
      }

//      result = connection->query("unlock tables");

      if(Application::will_trace(El::Logging::HIGH))
      {
        Application::logger()->trace(ostr.str(),
                                     Aspect::FEED_MANAGEMENT,
                                     El::Logging::MIDDLE);
      }

      for(size_t i = 0; i < puller_count; i++)
      {
        Transport::FeedPackImpl::Var feed_pack = feed_packs[i];

        if(feed_pack.in() != 0)
        {
          try
          {
            pullers[i].puller->
              update_feeds(pullers[i].session, feed_pack.in());
          }
          catch(const Puller::ImplementationException& e)
          {
            std::ostringstream ostr;
            
            ostr << "FeedingPullers::"
              "fetch_updated_feeds: update_feeds(" << pullers[i].session
                 << ", ...) have thrown Puller::ImplementationException. "
              "Description:\n" << e.description.in();

            El::Service::Error error(ostr.str(), this);
            callback_->notify(&error);
          }
          catch(const RSS::Puller::NotReady& e)
          {
            std::ostringstream ostr;
            
            ostr << "FeedingPullers::"
              "fetch_updated_feeds: update_feeds(" << pullers[i].session
                 << ", ...) have thrown Puller::NotReady. "
              "Description:\n" << e.reason.in();

            Application::logger()->trace(ostr.str(),
                                         Aspect::STATE,
                                         El::Logging::LOW);
          }
          catch(const CORBA::Exception& e)
          {
            std::ostringstream ostr;
            
            ostr << "FeedingPullers::"
              "fetch_updated_feeds: update_feeds(" << pullers[i].session
                 << ", ...) have thrown CORBA::Exception. "
              "Unexpected situation - performing pullers relogin !"
              " Exception Description:\n" << e;

            El::Service::Error error(ostr.str(), this);
            callback_->notify(&error);

            if(started())
            {
              callback_->pullers_mistiming();      
            }

            return;
          }
        }
      }

      if(last_query)
      {
        read_feeds_stamp_ = current_feeds_stamp_;

        current_feeds_stamp_ = 0;
        current_max_id_ = 0;
        current_read_id_ = 0;
      }
      else
      {
        current_read_id_ = max_id;
      }
    }
    
  }
}
