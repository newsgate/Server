/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Feeds/RSS/Puller/SessionSupport.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_PULLER_SESSIONSUPPORT_HPP_
#define _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_PULLER_SESSIONSUPPORT_HPP_

#include <ace/OS.h>

#include <El/Exception.hpp>

#include "SubService.hpp"

#include <Services/Feeds/RSS/Commons/RSSFeedServices.hpp>

namespace NewsGate
{
  namespace RSS
  {
//
// LogingIn class
//    
    class LogingIn : public PullerState
    {
    public:
        
      EL_EXCEPTION(Exception, PullerState::Exception);

      LogingIn(const ACE_Time_Value& login_retry_period,
               PullerStateCallback* callback)
        throw(Exception, El::Exception);

      virtual ~LogingIn() throw();

      virtual void logout(::NewsGate::RSS::SessionId session)
        throw(NewsGate::RSS::Puller::ImplementationException,
              CORBA::SystemException);

      virtual void update_feeds(::NewsGate::RSS::SessionId session,
                                ::NewsGate::RSS::FeedPack* feeds)
        throw(NewsGate::RSS::Puller::NotReady,
              NewsGate::RSS::Puller::ImplementationException,
              CORBA::SystemException);

      virtual bool notify(El::Service::Event* event) throw(El::Exception);

    private:

      void finalize_login() throw();

      struct FinalizeLogin : public El::Service::CompoundServiceMessage
      {
        FinalizeLogin(LogingIn* state) throw(El::Exception);
      };

    private:
      ACE_Time_Value login_retry_period_;
    };
    

//
// Logout class
//    
    class Logout : public PullerState    
    {
    public:
        
      EL_EXCEPTION(Exception, PullerState::Exception);

      Logout(PullerStateCallback* callback)
        throw(Exception, El::Exception);

      virtual void logout(::NewsGate::RSS::SessionId session)
        throw(NewsGate::RSS::Puller::ImplementationException,
              CORBA::SystemException);

      virtual void update_feeds(::NewsGate::RSS::SessionId session,
                                ::NewsGate::RSS::FeedPack* feeds)
        throw(NewsGate::RSS::Puller::NotReady,
              NewsGate::RSS::Puller::ImplementationException,
              CORBA::SystemException);
       
      virtual bool notify(El::Service::Event* event) throw(El::Exception);

      void finalize_logout() throw();
      
    private:

      struct FinalizeLogout : public El::Service::CompoundServiceMessage
      {
        FinalizeLogout(Logout* state) throw(El::Exception);
      };
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
    // LogingIn class
    //

    inline
    LogingIn::~LogingIn() throw()
    {
    }

    //
    // LogingIn::FinalizeLogin class
    //
    
    inline
    LogingIn::FinalizeLogin::FinalizeLogin(LogingIn* state)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }
    
    //
    // Logout::FinalizeLogout class
    //
    
    inline
    Logout::FinalizeLogout::FinalizeLogout(Logout* state) throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }

  }
}

#endif // _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_PULLER_SESSIONSUPPORT_HPP_
