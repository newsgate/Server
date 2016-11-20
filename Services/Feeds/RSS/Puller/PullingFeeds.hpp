/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Feeds/RSS/Puller/PullingFeeds.hpp
 * @author Karen Arutyunov
 * $Id:$optimize
 */

#ifndef _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_PULLER_PULLINGFEEDS_HPP_
#define _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_PULLER_PULLINGFEEDS_HPP_

#include <stdint.h>

#include <string>
#include <list>
#include <vector>

#include <ext/hash_map>

#include <ace/OS.h>
#include <ace/Synch.h>
#include <ace/Guard_T.h>

#include <El/Exception.hpp>
#include <El/Lang.hpp>
#include <El/Country.hpp>
#include <El/Moment.hpp>
#include <El/ArrayPtr.hpp>
#include <El/Hash/Hash.hpp>
#include <El/Geography/AddressInfo.hpp>
#include <El/Service/ProcessPool.hpp>
#include <El/Net/HTTP/Session.hpp>
#include <El/Python/Sandbox.hpp>
#include <El/LibXML/HTMLParser.hpp>

#include <ace/OS.h>

#include <Commons/Feed/Types.hpp>
#include <Commons/Message/Message.hpp>
#include <Commons/Message/TransportImpl.hpp>

#include <xsd/DataFeed/RSS/Data.hpp>
#include <xsd/DataFeed/RSS/MsgAdjustment.hpp>
#include <xsd/DataFeed/RSS/Parser.hpp>

#include <Services/Commons/Message/MessageServices.hpp>
#include <Services/Feeds/RSS/Commons/TransportImpl.hpp>
#include <Services/Feeds/RSS/Commons/RSSFeedServices.hpp>

#include "SubService.hpp"

namespace NewsGate
{
  namespace RSS
  {
//
// PullingFeeds class
//    
    class PullingFeeds :
      public PullerState,
      public El::Python::SandboxService::Callback
    {
    public:
        
      EL_EXCEPTION(Exception, PullerState::Exception);
      EL_EXCEPTION(ImageNotLoadable, Exception);
      EL_EXCEPTION(ServiceStopped, Exception);

      typedef ::Server::Config::RSSFeedPullerType::puller_management_type
      PullerManagementConfig;

      typedef ::Server::Config::RSSFeedPullerType::feed_request_type
      FeedRequestConfig;

      typedef ::Server::Config::RSSFeedPullerType::saving_feed_state_type
      SavingFeedStateConfig;

      typedef ::Server::Config::RSSFeedPullerType::saving_feed_stat_type
      SavingFeedStatConfig;

      typedef ::Server::Config::RSSFeedPullerType::saving_traffic_type
      SavingTrafficConfig;

      PullingFeeds(PullerStateCallback* callback,
                   const PullerManagementConfig& puller_management_config,
                   const FeedRequestConfig& feed_request_config,
                   const SavingFeedStateConfig& saving_feed_state_config,
                   const SavingFeedStatConfig& saving_feed_stat_config,
                   const SavingTrafficConfig& saving_traffic_config)
        throw(Exception, El::Exception);

      virtual ~PullingFeeds() throw();

      virtual void logout(::NewsGate::RSS::SessionId session)
        throw(NewsGate::RSS::Puller::ImplementationException,
              CORBA::SystemException);

      virtual void update_feeds(::NewsGate::RSS::SessionId session,
                                ::NewsGate::RSS::FeedPack* feeds)
        throw(NewsGate::RSS::Puller::NotReady,
              NewsGate::RSS::Puller::ImplementationException,
              CORBA::SystemException);

      virtual bool notify(El::Service::Event* event) throw(El::Exception);

      virtual bool interrupt_execution(std::string& reason)
        throw(El::Exception);

    private:

      struct AcceptFeeds : public El::Service::CompoundServiceMessage
      {
        ::NewsGate::RSS::FeedPack_var feeds;
        
        AcceptFeeds(PullingFeeds* state, ::NewsGate::RSS::FeedPack* feeds_pack)
          throw(El::Exception);

        virtual ~AcceptFeeds() throw();
      };
 
      struct RequestFeed : public El::Service::CompoundServiceMessage
      {
        Feed::Id feed_id;
        uint64_t feed_info_update_number;

        RequestFeed(PullingFeeds* service,
                    const Feed::Id& id,
                    uint64_t feed_info_update_number_val)
          throw(El::Exception);
          
      };
      
      struct PushFeedState : public El::Service::CompoundServiceMessage
      {
        PushFeedState(PullingFeeds* service) throw(El::Exception);
      };
      
      struct PushFeedStat : public El::Service::CompoundServiceMessage
      {
        PushFeedStat(PullingFeeds* service) throw(El::Exception);
      };
      
      struct Poll : public El::Service::CompoundServiceMessage
      {
        Poll(PullingFeeds* state) throw(El::Exception);
      };

      class RequestingFeeds;

    public:

      struct FeedInfo
      {
        std::string url;
        std::string encoding;
        NewsGate::Feed::Type type;
        NewsGate::Feed::Space space;
        El::Lang lang;
        El::Country country;
        Transport::FeedState state;
        NewsGate::Message::LocalCodeArray last_messages;
        uint64_t update_number;
        Message::Automation::StringArray keywords;
        std::string adjustment_script;
        bool adjustment_script_requested_item_article;
        time_t no_message_request_time;

        FeedInfo() throw(El::Exception);
      };

      class FeedInfoTable :
        public __gnu_cxx::hash_map<Feed::Id,
                                   FeedInfo,
                                   El::Hash::Numeric<Feed::Id> >
      {
      public:
        FeedInfoTable() throw(El::Exception);
      };

      struct FeedRequestInfo
      {
        enum Result
        {
          RT_NONE,
          RT_PARSED_TO_THE_END,
          RT_NOT_MODIFIED,
          RT_PRESUMABLY_UNCHANGED,
          RT_HAS_CHANGES,
          RT_UNCHANGED,
          RT_FAILED,
          RT_WASTED
        };

        Feed::Id feed_id;
        ACE_Time_Value time;
        Result result;
        uint32_t outbound_bytes;
        uint32_t inbound_bytes;
        ACE_Time_Value duration;
        uint32_t messages;
        uint32_t messages_size;
        uint64_t messages_delay;
        uint64_t max_message_delay;
        int64_t mistiming;
        float wasted;

        FeedRequestInfo(Feed::Id feed_id_val) throw();

        const char* result_str() const throw(El::Exception);
        bool parsed() throw();
      };
      
      typedef std::list<FeedRequestInfo> FeedRequestInfoList;
      
      struct FeedRequestResultPack
      {
        Transport::FeedStateUpdatePackImpl::Var state_update_pack;
        FeedRequestInfoList requests_info;
        NewsGate::Message::Transport::RawMessagePackImpl::Var message_pack;
      };

      typedef std::list<FeedRequestResultPack> FeedRequestResultPackList;

      class FeedStatTable :
        public __gnu_cxx::hash_map<Feed::Id,
                                   ::NewsGate::RSS::Transport::FeedStat,
                                   El::Hash::Numeric<Feed::Id> >
      {
      public:
        FeedStatTable() throw(El::Exception);
      };

      struct FeedDailyStat
      {
        El::Moment date;
        FeedStatTable feed_stat;
      };

      typedef std::list<FeedDailyStat> FeedDailyStatList;

      void accept_feeds(::NewsGate::RSS::FeedPack* feeds) throw();
      void request_feed(Feed::Id id, uint64_t feed_info_update_number) throw();
      void poll() throw();
      void push_feed_state() throw();
      void push_feed_stat() throw();

      void push_request_results(
        FeedRequestResultPackList& feed_request_results,
        const RSS::SessionId& session,
        RSS::PullerManager_ptr puller_manager)
        throw(Exception, El::Exception, CORBA::Exception);
      
      void accumulate_feed_stat(const FeedRequestInfoList& requests_info)
        throw(Exception, El::Exception);
      
      enum ScheduleRequestType
      {
        ST_REGULAR,
        ST_FIRST_REQUEST,
        ST_SKIPPED_REQUEST
      };
    
      void schedule_request(FeedInfo& info, ScheduleRequestType type)
        throw(Exception, El::Exception);
      
      void load_stat_cache() throw(Exception, El::Exception);
      
      virtual void wait() throw(Exception, El::Exception);
      virtual bool start() throw(Exception, El::Exception);
      virtual bool stop() throw(Exception, El::Exception);
      
      struct FeedRequestInterceptor
        : public virtual El::Net::HTTP::Session::Interceptor,
          public virtual NewsGate::RSS::Parser::Interceptor
      {
        //
        // Should be preset
        //
        uint32_t recv_buffer_size;
        uint64_t content_length;
        uint64_t prev_content_length;
        uint64_t prev_first_chunk_size;
        uint32_t old_messages_limit;
        const NewsGate::Message::LocalCodeArray* last_messages;
//        const Channel* channel;
        El::Moment last_build_date;

        //
        // Calculated
        //
        uint32_t chunks;
        uint64_t first_chunk_size;
        uint32_t old_messages;
        uint32_t new_messages;
        uint32_t prev_recv_buffer_size;
        ACE_SOCK_Stream* socket;

      public:
        EL_EXCEPTION(Exception, El::ExceptionBase);

        EL_EXCEPTION(ParseInterrupted, Exception);
        EL_EXCEPTION(SameLastBuildDate, ParseInterrupted);
        EL_EXCEPTION(SameContentLength, ParseInterrupted);
        EL_EXCEPTION(SameFirstChunkSize, ParseInterrupted);
        EL_EXCEPTION(OldMessagesLimitReached, ParseInterrupted);
        
        FeedRequestInterceptor() throw();
        
        virtual ~FeedRequestInterceptor() throw();
        
      private:
        virtual void chunk_begins(unsigned long long size)
          throw(El::Exception);

        virtual void socket_stream_created(El::Net::Socket::Stream& stream)
          throw(El::Exception);
        
        virtual void post_last_build_date(RSS::Channel& channel)
          throw(SameLastBuildDate,
                SameContentLength,
                SameFirstChunkSize,
                El::Exception);

        virtual void post_item(RSS::Channel& channel)
          throw(ServiceStopped,
                SameContentLength,
                SameFirstChunkSize,
                El::Exception);        
        
        virtual void socket_stream_connected(El::Net::Socket::Stream& stream)
          throw(ServiceStopped, El::Exception);
        
        virtual void socket_stream_read(const unsigned char* buff, size_t size)
          throw(ServiceStopped, El::Exception);
        
        virtual void socket_stream_write(const unsigned char* buff,
                                         size_t size)
          throw(ServiceStopped, El::Exception);

        void check_content_length()
          throw(SameContentLength, SameFirstChunkSize, El::Exception);

        bool check_item(const RSS::Channel& channel)
          throw(OldMessagesLimitReached, El::Exception);
      };

      struct RequestInterceptor
        : public virtual El::Net::HTTP::Session::Interceptor
      {
        //
        // Should be preset
        //
        uint32_t recv_buffer_size;
        
      public:
        EL_EXCEPTION(Exception, El::ExceptionBase);
        
        RequestInterceptor() throw() : recv_buffer_size(0) {}
        virtual ~RequestInterceptor() throw() {}
        
      private:
        virtual void chunk_begins(unsigned long long size)
          throw(El::Exception) {}

        virtual void socket_stream_created(El::Net::Socket::Stream& stream)
          throw(El::Exception);

        virtual void socket_stream_connected(El::Net::Socket::Stream& stream)
          throw(ServiceStopped, El::Exception);
        
        virtual void socket_stream_read(const unsigned char* buff, size_t size)
          throw(ServiceStopped, El::Exception);

        virtual void socket_stream_write(const unsigned char* buff,
                                         size_t size)
          throw(ServiceStopped, El::Exception);
      };

      struct HTTPInfo
      {
        std::string url;
        uint32_t status_code;
        std::string last_modified_hdr;
        std::string etag_hdr;
        std::string new_location;
        bool new_location_permanent;
        int64_t content_length_hdr;
        char single_chunked;
        int64_t first_chunk_size;
        std::string fail_reason;
        bool heuristics_applied;
        uint32_t entropy;
        uint32_t feed_size;
        std::string charset;

        HTTPInfo() throw(El::Exception);
      };

      typedef std::auto_ptr<std::ostringstream> OStringSteamPtr;
      
      El::Net::HTTP::Session* start_session(
        const char* url,
        const FeedInfo& feed_info,
        FeedRequestInfo& request_info,
        HTTPInfo& http_info,
        FeedRequestInterceptor& interceptor,
        OStringSteamPtr& request_trace_stream,
        OStringSteamPtr& response_trace_stream)
        throw(Exception, El::Exception);
      
      void process_html_feed(Feed::Id feed_id,
                             const FeedInfo& feed_info)
        throw(ServiceStopped, Exception, El::Exception);

      RSS::Parser* pull_feed(const FeedInfo& feed_info,
                             FeedRequestInfo& request_info,
                             HTTPInfo& http_info)
        throw(ServiceStopped, Exception, El::Exception);
      
      RSS::Parser* pull_feed(const FeedInfo& feed_info,
                             bool use_http_charset,
                             FeedRequestInfo& request_info,
                             HTTPInfo& http_info,
                             const char* encoding)
        throw(ServiceStopped,
              RSS::Parser::EncodingError,
              Exception,
              El::Exception);
      
      void process_channel(Feed::Id feed_id,
                           const FeedInfo& current_feed_info,
                           HTTPInfo& http_info,
                           Channel& channel,
                           FeedRequestInfo& request_info)
        throw(ServiceStopped, Exception, El::Exception);

      void process_messages(
        Channel& channel,
        Transport::FeedStateUpdate& state_update,
        FeedInfo& current_feed_info,
        FeedRequestInfo& request_info,
        HTTPInfo& http_info,
        NewsGate::Message::Transport::RawMessagePackImpl::Var& message_pack)
        throw(ServiceStopped, Exception, El::Exception);
      
      void set_feed_request_result(
        Feed::Id feed_id,
        const char* feed_url,
        const FeedInfo& feed_info,
        Message::Transport::RawMessagePackImpl::Var& message_pack,
        const Transport::FeedStateUpdate& state_update,
        const FeedRequestInfo& request_info)
        throw(El::Exception);

      typedef std::auto_ptr<MsgAdjustment::Code> MsgAdjustmentCodePtr;

      void adjust_message(
        MsgAdjustmentCodePtr& code,
        const Item& message,
        const Channel& channel,
        FeedInfo& feed_info,
        ::NewsGate::Feed::Id feed_id,
        ::NewsGate::Message::Automation::Message& adjusted_msg)
        throw(ServiceStopped, Exception, El::Exception);
      
      void post_message(
        ::NewsGate::Message::Automation::Message& message,
        const Message::LocalCode& message_code,
        ::NewsGate::Feed::Id feed_id,
        const FeedInfo& current_feed_info,
        Message::Transport::RawMessagePackImpl::Var& message_pack)
        throw(ServiceStopped, Exception, El::Exception);
      
      void create_message_bank_manager_refs()
        throw(Exception, El::Exception);
      
      typedef std::vector<NewsGate::Message::BankClientSession_var>
      MessageBankClientSessionArray;

      void bank_client_sessions(MessageBankClientSessionArray& sessions)
        throw(Exception, El::Exception);

      typedef Server::Config::RSSFeedPullerType::feed_request_type::
              image_type::thumbnail_type::size_sequence ThumSizeConf;
      
      bool create_thumbnails(::NewsGate::Message::Automation::Image& img,
                             const ThumSizeConf& thumb_sizes,
                             NewsGate::Message::ImageThumbArray& thumbnails,
                             uint64_t& image_hash,
                             std::ostream* log_ostr)
        throw(ServiceStopped, Exception, El::Exception);
/*      
      void check_image_loadable(const char* img_path) const
        throw(ImageNotLoadable, El::Exception);
      
*/
/*      
      bool read_image_size(::NewsGate::Message::Automation::Image& img,
                           std::ostream* log_ostr) const
        throw(ServiceStopped, Exception, El::Exception);
      
      bool read_image_size(::NewsGate::Message::Automation::Image& img,
                           const char* referer,
                           std::ostream* log_ostr) const
        throw(ServiceStopped, Exception, El::Exception);
*/    
      static bool exclude_string(std::string& text, const char* entry)
        throw(El::Exception);
      
    private:
      typedef std::auto_ptr<El::Net::HTTP::Session> HTTPSessionPtr;
      typedef __gnu_cxx::hash_set<std::string, El::Hash::String> StringSet;
      
      typedef ACE_Thread_Mutex ImageMutex;
      typedef ACE_Guard<ImageMutex> ImageGuard;

      mutable ImageMutex image_lock_;

      El::Geography::AddressInfo address_info_;
      
      typedef El::Corba::SmartRef<NewsGate::Message::BankManager>
      BankManagerRef;

      typedef std::list<BankManagerRef> MessageBankManagerList;
      MessageBankManagerList message_bank_managers_;
      
      MessageBankClientSessionArray message_bank_client_sessions_;
      uint32_t message_bank_client_sessions_index_;
      uint32_t message_entities_reserve_;
      uint64_t accept_feed_counter_;
      
      FeedInfoTable feeds_;
      FeedRequestResultPackList feed_request_results_;
      FeedDailyStatList feed_daily_stat_;
      ACE_Time_Value feed_stat_last_save_;
      bool request_feeds_;
      El::Service::ProcessPool_var process_pool_;
      El::Python::SandboxService_var sandbox_service_;
      ::NewsGate::Message::Automation::StringSet image_extension_whitelist_;
      ::NewsGate::Message::Automation::StringArray image_prefix_blacklist_;

      const PullerManagementConfig& puller_management_config_;
      const FeedRequestConfig& feed_request_config_;
      const SavingFeedStateConfig& saving_feed_state_config_;
      const SavingFeedStatConfig& saving_feed_stat_config_;
      const SavingTrafficConfig& saving_traffic_config_;
    };

//
// PullingFeeds::RequestingFeeds class
//    
    class PullingFeeds::RequestingFeeds :
      public El::Service::CompoundService<El::Service::Service, 
                                          PullingFeeds>
    {
    public:
      
      EL_EXCEPTION(Exception, PullerState::Exception);
      
      RequestingFeeds(PullingFeeds* callback, unsigned long threads)
        throw(Exception, El::Exception);
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
    // PullingFeeds::FeedInfo struct
    //
    
    inline
    PullingFeeds::FeedInfo::FeedInfo() throw(El::Exception)
        : type(::NewsGate::Feed::TP_UNDEFINED),
          space(::NewsGate::Feed::SP_UNDEFINED),
          lang(El::Lang::nonexistent),
          country(El::Country::nonexistent),
          update_number(0),
          adjustment_script_requested_item_article(false),
          no_message_request_time(0)
    {
    }

    //
    // PullingFeeds::FeedInfoTable class
    //
    
    inline
    PullingFeeds::FeedInfoTable::FeedInfoTable() throw(El::Exception)
    {
    }
    
    //
    // PullingFeeds::FeedStatTable class
    //
    
    inline
    PullingFeeds::FeedStatTable::FeedStatTable() throw(El::Exception)
    {
    }

    //
    // PullingFeeds class
    //
    
    inline
    PullingFeeds::~PullingFeeds() throw()
    {
    }

    //
    // PullingFeeds::FeedRequestInfo struct
    //
    inline
    PullingFeeds::FeedRequestInfo::FeedRequestInfo(Feed::Id feed_id_val)
      throw()
        : feed_id(feed_id_val),
          result(RT_NONE),
          outbound_bytes(0),
          inbound_bytes(0),
          messages(0),
          messages_size(0),
          messages_delay(0),
          max_message_delay(0),
          mistiming(0),
          wasted(0)
    {
    }
    
    inline
    bool
    PullingFeeds::FeedRequestInfo::parsed() throw()
    {
      return result == RT_PARSED_TO_THE_END || result == RT_UNCHANGED ||
        result == RT_WASTED;
    }
    
    inline
    const char*
    PullingFeeds::FeedRequestInfo::result_str() const throw(El::Exception)
    {
      switch(result)
      {
      case RT_NONE: return "none";
      case RT_PARSED_TO_THE_END: return "parsed to the end";
      case RT_NOT_MODIFIED: return "not modified";
      case RT_PRESUMABLY_UNCHANGED: return "presumably unchanged";
      case RT_HAS_CHANGES: return "has changes";
      case RT_UNCHANGED: return "unchanged";
      case RT_FAILED: return "failed";
      case RT_WASTED: return "wasted";
      }

      return "unexpected";
    }
    
    //
    // PullingFeeds::FeedRequestInterceptor struct
    //
    inline
    PullingFeeds::FeedRequestInterceptor::FeedRequestInterceptor() throw()
        : recv_buffer_size(0),
          content_length(0),
          prev_content_length(0),
          prev_first_chunk_size(0),
          old_messages_limit(UINT32_MAX),
          last_messages(0),
//          channel(0),
          chunks(0),
          first_chunk_size(0),
          old_messages(0),
          new_messages(0),
          prev_recv_buffer_size(0),
          socket(0)
    {
    }

    inline
    PullingFeeds::FeedRequestInterceptor::~FeedRequestInterceptor() throw()
    {
    }

    //
    // PullingFeeds::HTTPInfo struct
    //
    inline
    PullingFeeds::HTTPInfo::HTTPInfo() throw(El::Exception)
        : status_code(0),
          new_location_permanent(false),
          content_length_hdr(-1),
          single_chunked(-1),
          first_chunk_size(-1),
          heuristics_applied(false),
          entropy(0),
          feed_size(0)
    {
    }

    //
    // PullingFeeds::AcceptFeeds class
    //
    
    inline
    PullingFeeds::AcceptFeeds::
    AcceptFeeds(PullingFeeds* state,
                ::NewsGate::RSS::FeedPack* feeds_pack) throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
      feeds_pack->_add_ref();
      feeds = feeds_pack;
    }

    inline
    PullingFeeds::AcceptFeeds::~AcceptFeeds() throw()
    {
    }
    
    //
    // PullingFeeds::RequestingFeeds class
    //
    
    inline
    PullingFeeds::RequestingFeeds::RequestingFeeds(
      PullingFeeds* callback,
      unsigned long threads)
      throw(Exception, El::Exception)
        : El::Service::CompoundService<
            El::Service::Service, 
            PullingFeeds>(
              callback,
              "PullingFeeds::RequestingFeeds",
              threads,
              1024 * 1000)
    {
    }

    //
    // PullingFeeds::PushFeedState class
    //

    inline
    PullingFeeds::PushFeedState::PushFeedState(PullingFeeds* state)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }
    
    //
    // PullingFeeds::PushFeedStat class
    //

    inline
    PullingFeeds::PushFeedStat::PushFeedStat(PullingFeeds* state)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }
    
    //
    // FeedingPullers::Poll class
    //
    
    inline
    PullingFeeds::Poll::Poll(PullingFeeds* state)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }

    //
    // PullingFeeds::RequestFeed class
    //
    inline
    PullingFeeds::RequestFeed::RequestFeed(
      PullingFeeds* state,
      const Feed::Id& id,
      uint64_t feed_info_update_number_val)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state),
          feed_id(id),
          feed_info_update_number(feed_info_update_number_val)
    {
    }

  }
}

#endif // _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_PULLER_PULLINGFEEDS_HPP_
