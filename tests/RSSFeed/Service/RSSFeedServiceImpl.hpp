/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/tests/RSSFeed/Service/RSSFeedServiceImpl.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_TESTS_RSSFEED_SERVICE_RSSFEEDSERVICEIMPL_HPP_
#define _NEWSGATE_SERVER_TESTS_RSSFEED_SERVICE_RSSFEEDSERVICEIMPL_HPP_

#include <string>
#include <ext/hash_map>

#include <El/Exception.hpp>
#include <El/Hash/Hash.hpp>

#include <El/Service/CompoundService.hpp>

#include <tests/RSSFeed/Service/RSSFeed_s.hpp>

#include "RSSFeedImpl.hpp"

namespace NewsGate
{
  namespace RSS
  {
    class FeedServiceImpl :
      public virtual POA_NewsGate::RSS::FeedService,
      public virtual El::Service::GenericService    
    {
    public:
      
      EL_EXCEPTION(Exception, El::ExceptionBase);

      typedef ::Server::TestRSSFeed::Config::ConfigType::rss_feeds_type
      Config;

    public:

      FeedServiceImpl(El::Service::Callback* callback, const Config& config)
        throw(Exception, El::Exception);

      virtual ~FeedServiceImpl() throw();

    private:

      //
      // IDL:NewsGate/RSS/FeedService/get:1.0
      //
      virtual void get(const ::NewsGate::RSS::FeedService::Request& req,
                       ::NewsGate::RSS::FeedService::Response_out res)
        throw(NewsGate::RSS::FeedService::UnknownFeed,
              NewsGate::RSS::FeedService::ImplementationException,
              CORBA::SystemException);

    private:

      class FeedHashTable :
        public __gnu_cxx::hash_map<std::string, FeedImpl_var, El::Hash::String>
      {
      public:
        FeedHashTable() throw(El::Exception);
      };
      
      FeedHashTable feeds_;
    };

    typedef El::RefCount::SmartPtr<FeedServiceImpl> FeedServiceImpl_var;

  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace RSS
  {
    inline
    FeedServiceImpl::FeedHashTable::FeedHashTable() throw(El::Exception)
    {
    }
    
  }
}

#endif // _NEWSGATE_SERVER_TESTS_RSSFEED_SERVICE_RSSFEEDSERVICEIMPL_HPP_
