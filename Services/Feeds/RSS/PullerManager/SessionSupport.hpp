/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Feeds/RSS/PullerManager/SessionSupport.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_PULLERMANAGER_SESSIONSUPPORT_HPP_
#define _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_PULLERMANAGER_SESSIONSUPPORT_HPP_

#include <stdint.h>

#include <ace/OS.h>

#include <El/Exception.hpp>

#include <Services/Feeds/RSS/Commons/RSSFeedServices.hpp>

#include "SubService.hpp"

namespace NewsGate
{
  namespace RSS
  {
    class RegisteringPullers : public PullerManagerState      
    {
    public:
        
      EL_EXCEPTION(Exception, PullerManagerState::Exception);

      RegisteringPullers(const ACE_Time_Value& puller_registration_timeout,
                         PullerManagerStateCallback* callback)
        throw(Exception, El::Exception);

      virtual ~RegisteringPullers() throw() {}

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

      void finalize_registration() throw();

    private:

      class FinalizePullersRegistration :
        public El::Service::CompoundServiceMessage
      {
      public:
          FinalizePullersRegistration(RegisteringPullers* state)
            throw(El::Exception);
      };

    private:
      uint32_t sequence_num_;
      ACE_Time_Value puller_registration_timeout_;
      ACE_Time_Value end_registration_time_;
    };

    class EndingFeedingSession : public PullerManagerState
    {
    public:
        
      EL_EXCEPTION(Exception, PullerManagerState::Exception);

      EndingFeedingSession(const ACE_Time_Value& puller_reset_timeout,
                           PullerManagerStateCallback* callback)
        throw(Exception, El::Exception);

      virtual ~EndingFeedingSession() throw() {}      

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

      void finalize_session() throw();

    private:

      class FinalizeSession :
        public El::Service::CompoundServiceMessage
      {
      public:
        FinalizeSession(EndingFeedingSession* state) throw(El::Exception);
      };

    private:
      ACE_Time_Value puller_reset_timeout_;
      ACE_Time_Value finalize_session_time_;
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
    // RegisteringPullers::FinalizePullersRegistration class
    //
    
    inline
    RegisteringPullers::FinalizePullersRegistration::
    FinalizePullersRegistration(RegisteringPullers* state) throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }
    
    //
    // EndingFeedingSession::FinalizeSession class
    //
    
    inline
    EndingFeedingSession::FinalizeSession::
    FinalizeSession(EndingFeedingSession* state) throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }
    
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_PULLERMANAGER_SESSIONSUPPORT_HPP_
