/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Feeds/RSS/Puller/SubService.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_PULLER_SUBSERVICE_HPP_
#define _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_PULLER_SUBSERVICE_HPP_

#include <stdint.h>

#include <El/Exception.hpp>
#include <El/Service/CompoundService.hpp>

#include <Services/Feeds/RSS/Commons/RSSFeedServices.hpp>

namespace NewsGate
{
  namespace RSS
  {
    class PullerStateCallback : public virtual El::Service::Callback
    {
    public:
      virtual void session(uint64_t val) throw() = 0;
      virtual uint64_t session() throw() = 0;

      virtual void login_completed() throw() = 0;
      virtual void initiate_logout() throw() = 0;
      virtual void logout_completed() throw() = 0;
    };
    
    //
    // State basic class
    //
    class PullerState :
      public El::Service::CompoundService<El::Service::Service,
                                          PullerStateCallback>

    {
    public:
        
      EL_EXCEPTION(Exception, El::ExceptionBase);

      PullerState(PullerStateCallback* callback, const char* name)
        throw(Exception, El::Exception);

      ~PullerState() throw();
        
      virtual void logout(::NewsGate::RSS::SessionId session)
        throw(NewsGate::RSS::Puller::ImplementationException,
              CORBA::SystemException) = 0;

      virtual void update_feeds(::NewsGate::RSS::SessionId session,
                                ::NewsGate::RSS::FeedPack* feeds)
        throw(NewsGate::RSS::Puller::NotReady,
              NewsGate::RSS::Puller::ImplementationException,
              CORBA::SystemException) = 0;
    };

    typedef El::RefCount::SmartPtr<PullerState> PullerState_var;

    namespace Aspect
    {
      extern const char STATE[];
      extern const char PULLING_FEEDS[];
      extern const char MSG_MANAGEMENT[];
    }
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
    // State class
    //
    
    inline
    PullerState::PullerState(PullerStateCallback* callback, const char* name)
      throw(Exception, El::Exception)
        : El::Service::CompoundService<
             El::Service::Service,
             PullerStateCallback>(callback, name)
    {
    }
    
    inline
    PullerState::~PullerState() throw()
    {
    }
    
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_PULLER_SUBSERVICE_HPP_
