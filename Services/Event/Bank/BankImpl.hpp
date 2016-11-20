/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Event/Bank/BankImpl.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_EVENT_BANK_BANKIMPL_HPP_
#define _NEWSGATE_SERVER_SERVICES_EVENT_BANK_BANKIMPL_HPP_

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/Service/CompoundService.hpp>

#include <Services/Commons/Event/EventServices_s.hpp>

#include "SubService.hpp"

namespace NewsGate
{
  namespace Event
  {
    class BankImpl : public virtual POA_NewsGate::Event::Bank,
                     public virtual El::Service::CompoundService<BankState>,
                     public virtual BankStateCallback
    {
    public:
      EL_EXCEPTION(Exception, El::ExceptionBase);
      EL_EXCEPTION(InvalidArgument, Exception);

    public:

      BankImpl(El::Service::Callback* callback)
        throw(InvalidArgument, Exception, El::Exception);
      
      virtual ~BankImpl() throw();
      
    //
    // IDL:NewsGate/Event/Bank/get_message_events:1.0
    //
    virtual ::CORBA::ULong get_message_events(
      ::NewsGate::Message::Transport::IdPack* messages,
      ::NewsGate::Message::Transport::MessageEventPack_out events)
        throw(NewsGate::Event::NotReady,
              NewsGate::Event::ImplementationException,
              ::CORBA::SystemException);
      //
      // IDL:NewsGate/Event/EventBank/post_message_digest:1.0
      //
      virtual void post_message_digest(
        ::NewsGate::Event::Transport::MessageDigestPack* digests,
        ::NewsGate::Message::Transport::MessageEventPack_out events)
        throw(NewsGate::Event::NotReady,
              NewsGate::Event::ImplementationException,
              ::CORBA::SystemException);      

      //
      // IDL:NewsGate/Event/Bank/get_events:1.0
      //
      virtual void get_events(
        ::NewsGate::Event::Transport::EventIdRelPack* ids,
        ::NewsGate::Event::Transport::EventObjectRelPack_out events)
        throw(NewsGate::Event::NotReady,
              NewsGate::Event::ImplementationException,
              ::CORBA::SystemException);

      //
      // IDL:NewsGate/Event/Bank/push_events:1.0
      //
      virtual void push_events(
        ::NewsGate::Event::Transport::MessageDigestPack* digests,
        ::NewsGate::Event::Transport::EventPushInfoPack* events)
        throw(NewsGate::Event::NotReady,
              NewsGate::Event::ImplementationException,
              ::CORBA::SystemException);
      
      //
      // IDL:NewsGate/Event/BankClientSession/delete_messages:1.0
      //
      virtual void delete_messages(::NewsGate::Message::Transport::IdPack* ids)
        throw(NewsGate::Event::NotReady,
              NewsGate::Event::ImplementationException,
              ::CORBA::SystemException);
      
    private:
      
      //
      // BankStateCallback interface methods
      //

      virtual void login_completed(Event::BankSession* session) throw();
      virtual void logout(Event::BankSessionId* session_id) throw();
    };

    typedef El::RefCount::SmartPtr<BankImpl> BankImpl_var;

  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Message
  {
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_EVENT_BANK_BANKIMPL_HPP_
