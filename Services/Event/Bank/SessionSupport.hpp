/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Event/Bank/SessionSupport.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_EVENT_BANK_SESSIONSUPPORT_HPP_
#define _NEWSGATE_SERVER_SERVICES_EVENT_BANK_SESSIONSUPPORT_HPP_

#include <string>

#include <ace/OS.h>

#include <El/Exception.hpp>

#include "SubService.hpp"

namespace NewsGate
{
  namespace Event
  {
    //
    // RegisteringBanks class
    //
    class LogingIn : public BankState
    {
    public:
        
      EL_EXCEPTION(Exception, BankState::Exception);

      LogingIn(const ACE_Time_Value& login_retry_period,
               BankStateCallback* callback)
        throw(Exception, El::Exception);

      virtual ::CORBA::ULong get_message_events(
        ::NewsGate::Message::Transport::IdPack* messages,
        ::NewsGate::Message::Transport::MessageEventPack_out events)
        throw(NewsGate::Event::NotReady,
              NewsGate::Event::ImplementationException,
              ::CORBA::SystemException);
      
      virtual void post_message_digest(
        ::NewsGate::Event::Transport::MessageDigestPack* digests,
        ::NewsGate::Message::Transport::MessageEventPack_out events)
        throw(NewsGate::Event::NotReady,
              NewsGate::Event::ImplementationException,
              ::CORBA::SystemException);      

      virtual void get_events(
        ::NewsGate::Event::Transport::EventIdRelPack* ids,
        ::NewsGate::Event::Transport::EventObjectRelPack_out events)
        throw(NewsGate::Event::NotReady,
              NewsGate::Event::ImplementationException,
              ::CORBA::SystemException);    

      virtual void push_events(
        ::NewsGate::Event::Transport::MessageDigestPack* digests,
        ::NewsGate::Event::Transport::EventPushInfoPack* events)
        throw(NewsGate::Event::NotReady,
              NewsGate::Event::ImplementationException,
              ::CORBA::SystemException);
      
      virtual void delete_messages(::NewsGate::Message::Transport::IdPack* ids)
        throw(NewsGate::Event::NotReady,
              NewsGate::Event::ImplementationException,
              ::CORBA::SystemException);

      virtual ~LogingIn() throw();

      bool notify(El::Service::Event* event) throw(El::Exception);
      
    private:

      void finalize_login() throw();

      struct FinalizeLogin : public El::Service::CompoundServiceMessage
      {
        FinalizeLogin(LogingIn* state) throw(El::Exception);
      };
      
    private:
      ACE_Time_Value login_retry_period_;
    };

  }
}


///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Event
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
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_EVENT_BANK_SESSIONSUPPORT_HPP_
