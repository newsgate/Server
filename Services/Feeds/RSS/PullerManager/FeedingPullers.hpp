/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Feeds/RSS/PullerManager/FeedingPullers.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_PULLERMANAGER_FEEDINGPULLERS_HPP_
#define _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_PULLERMANAGER_FEEDINGPULLERS_HPP_

#include <stdint.h>

#include <string>
#include <list>
#include <iostream>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/MySQL/DB.hpp>

#include <xsd/Config.hpp>

#include <Services/Feeds/RSS/Commons/RSSFeedServices.hpp>
#include <Services/Feeds/RSS/Commons/TransportImpl.hpp>

#include "SubService.hpp"

namespace NewsGate
{
  namespace RSS
  {
    class FeedingPullers : public PullerManagerState      
    {
    public:
        
      EL_EXCEPTION(Exception, PullerManagerState::Exception);

      typedef ::Server::Config::RSSFeedPullerManagerType::fetching_feeds_type
      FetchingFeedsConfig;

      typedef ::Server::Config::RSSFeedPullerManagerType::saving_feed_state_type
      SavingFeedStateConfig;

      typedef ::Server::Config::RSSFeedPullerManagerType::saving_feed_stat_type
      SavingFeedStatConfig;
      
      FeedingPullers(const ACE_Time_Value& puller_presence_poll_timeout,
                     const FetchingFeedsConfig& fetching_feeds_config,
                     const SavingFeedStateConfig& saving_feed_state_config,
                     const SavingFeedStatConfig& saving_feed_stat_config,
                     PullerManagerStateCallback* callback)
        throw(Exception, El::Exception);

      virtual ~FeedingPullers() throw();

      virtual ::NewsGate::RSS::SessionId puller_login(
        ::NewsGate::RSS::Puller_ptr puller_object)
        throw(NewsGate::RSS::PullerManager::ImplementationException,
              CORBA::SystemException);

      virtual void puller_logout(::NewsGate::RSS::SessionId session)
        throw(NewsGate::RSS::PullerManager::ImplementationException,
              CORBA::SystemException);

      virtual CORBA::Boolean feed_state(
        ::NewsGate::RSS::SessionId session,
        ::NewsGate::RSS::FeedStateUpdatePack* state_update)
        throw(NewsGate::RSS::PullerManager::Logout,
              NewsGate::RSS::PullerManager::ImplementationException,
              CORBA::SystemException);

      virtual void ping(::NewsGate::RSS::SessionId session)
        throw(NewsGate::RSS::PullerManager::Logout,
              NewsGate::RSS::PullerManager::ImplementationException,
              CORBA::SystemException);
      
      virtual bool notify(El::Service::Event* event) throw(El::Exception);

      void check_pullers_presence() throw();
      void check_feeds() throw();
      
      void write_feed_state() throw();
      void write_feed_stat() throw();

    private:

      void fetch_updated_feeds() throw(Exception, El::Exception);

      struct FeedRedirect
      {
        Feed::Id feed;
        std::string new_location;
      };

      typedef std::list<FeedRedirect> FeedRedirectList;

      void write_state_update(El::MySQL::Connection* connection,
                              const Transport::FeedStateUpdate& state_update,
                              FeedRedirectList& failed_redirects)
        throw(Exception, El::Exception);
      
      void disable_fail_redirected_feeds(
        const FeedRedirectList& failed_redirects)
        throw(Exception, El::Exception);
      
      void write_stat(El::MySQL::Connection* connection,
                      const char* file_name)
        throw(Exception, El::Exception);
      
      bool feed_state_to_stream(unsigned long updated_fields,
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
                                const char* entropy_updated,
                                uint32_t size,
                                int32_t heuristics_counter,
                                const std::string& cache,
                                std::ostream& ostr)
        throw(Exception, El::Exception);
      
      void delete_expired_messages(
        const Transport::FeedStateUpdateArray& state_updates)
        throw(Exception, El::Exception);
      
      void write_updated_messages(
        const Transport::FeedStateUpdateArray& state_updates)
        throw(Exception, El::Exception);
      
    private:

      struct CheckPullersPresence : public El::Service::CompoundServiceMessage
      {
        CheckPullersPresence(FeedingPullers* state) throw(El::Exception);
      };

      struct CheckFeeds : public El::Service::CompoundServiceMessage
      {
        CheckFeeds(FeedingPullers* state) throw(El::Exception);
      };

      struct WriteFeedState :  public El::Service::CompoundServiceMessage
      {
        WriteFeedState(FeedingPullers* state) throw(El::Exception);
      };

      struct WriteFeedStat : public El::Service::CompoundServiceMessage
      {
        WriteFeedStat(FeedingPullers* state) throw(El::Exception);
      };
      
    private:

      typedef std::list<Transport::FeedStateUpdatePackImpl::Var>
      FeedStateUpdatePackImplList;
      
      ACE_Time_Value puller_presence_poll_timeout_;
      const FetchingFeedsConfig& fetching_feeds_config_;
      
      const SavingFeedStateConfig& saving_feed_state_config_;
      const SavingFeedStatConfig& saving_feed_stat_config_;

      uint64_t read_feeds_stamp_;
      uint64_t current_feeds_stamp_;
      uint64_t current_max_id_;
      uint64_t current_read_id_;

      FeedStateUpdatePackImplList feed_state_updates_;
      std::string feed_stat_cache_dir_;

      ACE_Time_Value next_stat_cleanup_;
    };

  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace RSS
  {
    //
    // FeedingPullers::CheckPullersPresence class
    //
    inline
    FeedingPullers::~FeedingPullers() throw()
    {
    }
    
    //
    // FeedingPullers::CheckPullersPresence class
    //
    inline
    FeedingPullers::CheckPullersPresence::CheckPullersPresence(
      FeedingPullers* state) throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }
    
    //
    // FeedingPullers::CheckFeeds class
    //
    inline
    FeedingPullers::CheckFeeds::CheckFeeds(FeedingPullers* state)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }
    
    //
    // FeedingPullers::WriteFeedState class
    //
    inline
    FeedingPullers::WriteFeedState::WriteFeedState(FeedingPullers* state)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }
    
    //
    // FeedingPullers::WriteFeedStat class
    //
    
    inline
    FeedingPullers::WriteFeedStat::WriteFeedStat(FeedingPullers* state)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }
    
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_PULLERMANAGER_FEEDINGPULLERS_HPP_
