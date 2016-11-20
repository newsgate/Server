/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Event/Bank/SubService.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_SERVICES_EVENT_BANK_SUBSERVICE_HPP_
#define _NEWSGATE_SERVER_SERVICES_EVENT_BANK_SUBSERVICE_HPP_

#include <El/Exception.hpp>
#include <El/RefCount/All.hpp>
#include <El/Service/CompoundService.hpp>

#include <Services/Commons/Event/EventServices_s.hpp>

namespace NewsGate
{
  namespace Event
  {
    class BankStateCallback : public virtual El::Service::Callback
    {
    public:
      virtual void login_completed(Event::BankSession* session) throw() = 0;
      virtual void logout(Event::BankSessionId* session_id) throw() = 0;
    };

    //
    // State basic class
    //
    class BankState :
      public El::Service::CompoundService<El::Service::Service,
                                          BankStateCallback>
    {
    public:
        
      EL_EXCEPTION(Exception, El::ExceptionBase);

      BankState(BankStateCallback* callback, const char* name)
        throw(Exception, El::Exception);

      virtual ~BankState() throw() {}
        
      virtual ::CORBA::ULong get_message_events(
        ::NewsGate::Message::Transport::IdPack* messages,
        ::NewsGate::Message::Transport::MessageEventPack_out events)
        throw(NewsGate::Event::NotReady,
              NewsGate::Event::ImplementationException,
              ::CORBA::SystemException) = 0;
      
      virtual void post_message_digest(
        ::NewsGate::Event::Transport::MessageDigestPack* digests,
        ::NewsGate::Message::Transport::MessageEventPack_out events)
        throw(NewsGate::Event::NotReady,
              NewsGate::Event::ImplementationException,
              ::CORBA::SystemException) = 0;      

      virtual void get_events(
        ::NewsGate::Event::Transport::EventIdRelPack* ids,
        ::NewsGate::Event::Transport::EventObjectRelPack_out events)
        throw(NewsGate::Event::NotReady,
              NewsGate::Event::ImplementationException,
              ::CORBA::SystemException) = 0;
      
      virtual void push_events(
        ::NewsGate::Event::Transport::MessageDigestPack* digests,
        ::NewsGate::Event::Transport::EventPushInfoPack* events)
        throw(NewsGate::Event::NotReady,
              NewsGate::Event::ImplementationException,
              ::CORBA::SystemException) = 0;

      virtual void delete_messages(::NewsGate::Message::Transport::IdPack* ids)
        throw(NewsGate::Event::NotReady,
              NewsGate::Event::ImplementationException,
              ::CORBA::SystemException) = 0;      
    };

    typedef El::RefCount::SmartPtr<BankState> BankState_var;

    namespace Aspect
    {
      extern const char STATE[];
      extern const char EVENT_MANAGEMENT[];
    } 
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
    // BankImpl::State class
    //
    inline
    BankState::BankState(BankStateCallback* callback, const char* name)
      throw(Exception, El::Exception)
        : El::Service::CompoundService<El::Service::Service,
                                       BankStateCallback>(callback, name)
    {
    }
    
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_EVENT_BANK_SUBSERVICE_HPP_
