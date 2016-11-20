/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/tests/RSSFeed/Service/RSSFeedImpl.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_TESTS_RSSFEED_SERVICE_RSSFEEDIMPL_HPP_
#define _NEWSGATE_SERVER_TESTS_RSSFEED_SERVICE_RSSFEEDIMPL_HPP_

#include <string>
#include <iostream>
#include <vector>

#include <El/Exception.hpp>
#include <El/RefCount/All.hpp>
#include <El/SyncPolicy.hpp>
#include <El/Moment.hpp>

#include <El/Service/CompoundService.hpp>

#include <xsd/TestRSSFeed/Config.hpp>

#include <tests/RSSFeed/Service/RSSFeed.hpp>
#include <tests/Commons/SourceText.hpp>

namespace NewsGate
{
  namespace RSS
  {
    class FeedImpl
      : public virtual El::RefCount::DefaultImpl<El::Sync::ThreadRWPolicy>
    {
    public:
      
      typedef ::Server::TestRSSFeed::Config::ConfigType::rss_feeds_type::
      feed_type Config;
      
    public:  
      EL_EXCEPTION(Exception, El::ExceptionBase);

      FeedImpl(El::Service::GenericService* container, const Config& config)
        throw(Exception, El::Exception);
      
      virtual ~FeedImpl() throw();

      void get(const ::NewsGate::RSS::FeedService::Request& req,
               ::NewsGate::RSS::FeedService::Response* res)
        throw(Exception, El::Exception);

      void id(const char* val) throw(El::Exception);

      FeedImpl* clone() const throw(El::Exception);

    public:

      struct Message
      {
        std::string pub_date;
        std::string title;
        std::string description;
        std::string category;
        std::string link;
        std::string guid;
        El::Moment pub_moment;
      };

      typedef std::vector<Message> MessageArray;

      void add_message() throw();
 
    private:

      FeedImpl(const FeedImpl& feed) throw(El::Exception);
      
      void generate_channel_info() throw(Exception, El::Exception);

      void set_last_build_date(const ACE_Time_Value& cur_time)
        throw(Exception, El::Exception);
      
      void fill_message(Message& msg, const ACE_Time_Value& time)
        throw(Exception, El::Exception);

      void fill_message_pub_date(Message& msg, const ACE_Time_Value& time)
        throw(Exception, El::Exception);
      
      class AddMessage : public El::Service::CompoundServiceMessage
      {
      public:
        AddMessage(FeedImpl* feed_impl,
                   El::Service::GenericService* service)
          throw(El::Exception);

        virtual ~AddMessage() throw();
        
      private:
        virtual void execute() throw(El::Exception);
        
      private:
        FeedImpl* feed_impl_;
      };

    private:
      El::Service::GenericService* container_;
      const Config& config_;
      Test::SourceText_var source_text_;
      std::string title_;
      std::string description_;
      std::string id_;
      MessageArray messages_;
      std::string last_build_date_;
      unsigned long msg_number_;
      unsigned long update_number_;
      ACE_Time_Value creation_time_;
    };

    typedef El::RefCount::SmartPtr<FeedImpl> FeedImpl_var;

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
    // FeedImpl class
    //
    inline
    FeedImpl::~FeedImpl() throw()
    {
    }

    inline
    void
    FeedImpl::id(const char* val) throw(El::Exception)
    {
      WriteGuard_ guard(lock_i());
      id_ = val;
    }
    
    //
    // FeedImpl::AddMessage class
    //
    inline
    FeedImpl::AddMessage::AddMessage(
      FeedImpl* feed_impl,
      El::Service::GenericService* service) throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(service, 0, false),
          El::Service::CompoundServiceMessage(service, 0),
          feed_impl_(feed_impl)
    {
    }

    inline
    FeedImpl::AddMessage::~AddMessage() throw()
    {
    }
    
    inline
    void
    FeedImpl::AddMessage::execute() throw(El::Exception)
    {
      feed_impl_->add_message();
    }
  }
}

#endif // _NEWSGATE_SERVER_TESTS_RSSFEED_SERVICE_RSSFEEDIMPL_HPP_
