/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/tests/RSSFeed/Service/RSSFeedServiceImpl.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <El/CORBA/Corba.hpp>

#include <string>
#include <sstream>

#include <ace/OS.h>

#include "RSSFeedServiceImpl.hpp"
#include "RSSFeedMain.hpp"

namespace NewsGate
{
  namespace RSS
  {
    FeedServiceImpl::FeedServiceImpl(El::Service::Callback* callback,
                                     const Config& config)
      throw(Exception, El::Exception)
        : El::Service::GenericService(callback, "FeedServiceImpl")
    {
      for(Config::feed_sequence::const_iterator it = config.feed().begin();
          it != config.feed().end(); it++)
      {
        FeedImpl_var feed = new FeedImpl(this, *it);

        std::string id;
        
        if(it->count() > 1)
        {
          id = it->id() + "0";
          feed->id(id.c_str());

          for(unsigned long i = 1; i < it->count(); i++)
          {
            FeedImpl_var new_feed = feed->clone();

            std::ostringstream ostr;
            ostr << it->id().c_str() << i;

            std::string id = ostr.str();
            
            new_feed->id(id.c_str());
            feeds_[id] = new_feed;
          }
        }
        else
        {
          id = it->id().c_str();
        }

        feeds_[id] = feed;
      }

//      feeds_.optimize();
    }

    FeedServiceImpl::~FeedServiceImpl() throw()
    {
    }

    void
    FeedServiceImpl::get(
      const ::NewsGate::RSS::FeedService::Request& req,
      ::NewsGate::RSS::FeedService::Response_out res)
      throw(NewsGate::RSS::FeedService::UnknownFeed,
            NewsGate::RSS::FeedService::ImplementationException,
            CORBA::SystemException)
    {
      FeedHashTable::iterator it = feeds_.find(req.feed_name.in());

      if(it == feeds_.end())
      {
        NewsGate::RSS::FeedService::UnknownFeed ex;
        throw ex;
      }

      ::NewsGate::RSS::FeedService::Response_var resp =
          new NewsGate::RSS::FeedService::Response();

      try
      {
        it->second->get(req, resp);
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "FeedServiceImpl::get: El::Exception caught. Description:\n"
             << e;
        
        NewsGate::RSS::FeedService::ImplementationException ex;
        ex.description = ostr.str().c_str();
        throw ex;
      }

      res = resp._retn();
    }
  }  
}
