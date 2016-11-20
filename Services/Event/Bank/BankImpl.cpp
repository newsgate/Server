/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Event/Bank/BankImpl.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <string>
#include <sstream>

#include <ace/OS.h>

#include <Commons/Event/TransportImpl.hpp>

#include "SubService.hpp"
#include "BankImpl.hpp"
#include "BankMain.hpp"
#include "ManagingEvents.hpp"
#include "SessionSupport.hpp"

namespace NewsGate
{
  namespace Event
  {
    //
    // BankImpl class
    //
    BankImpl::BankImpl(El::Service::Callback* callback)
      throw(InvalidArgument, Exception, El::Exception)
        : El::Service::CompoundService<BankState>(callback, "BankImpl")
    {
      const Server::Config::EventBankType::bank_management_type&
          config = Application::instance()->config().bank_management();
          
      BankState_var st = new LogingIn(
        ACE_Time_Value(config.login_retry_period()),
        this);
        
      state(st.in());
    }

    BankImpl::~BankImpl() throw()
    {
      // Check if state is active, then deactivate and log error
    }

    ::CORBA::ULong
    BankImpl::get_message_events(
      ::NewsGate::Message::Transport::IdPack* messages,
      ::NewsGate::Message::Transport::MessageEventPack_out events)
      throw(NewsGate::Event::NotReady,
            NewsGate::Event::ImplementationException,
            ::CORBA::SystemException)
    {
      BankState_var st = state();
      return st->get_message_events(messages, events);
    }
    
    void
    BankImpl::post_message_digest(
      ::NewsGate::Event::Transport::MessageDigestPack* digests,
      ::NewsGate::Message::Transport::MessageEventPack_out events)
      throw(NewsGate::Event::NotReady,
            NewsGate::Event::ImplementationException,
            ::CORBA::SystemException)
    {
      BankState_var st = state();
      st->post_message_digest(digests, events);
    }

    void
    BankImpl::delete_messages(::NewsGate::Message::Transport::IdPack* ids)
      throw(NewsGate::Event::NotReady,
            NewsGate::Event::ImplementationException,
            ::CORBA::SystemException)
    {
      BankState_var st = state();
      st->delete_messages(ids);
    }

    void
    BankImpl::get_events(
      ::NewsGate::Event::Transport::EventIdRelPack* ids,
      ::NewsGate::Event::Transport::EventObjectRelPack_out events)
      throw(NewsGate::Event::NotReady,
            NewsGate::Event::ImplementationException,
            ::CORBA::SystemException)
    {
      BankState_var st = state();
      st->get_events(ids, events);
    }

    void
    BankImpl::push_events(
      ::NewsGate::Event::Transport::MessageDigestPack* digests,
      ::NewsGate::Event::Transport::EventPushInfoPack* events)
      throw(NewsGate::Event::NotReady,
            NewsGate::Event::ImplementationException,
            ::CORBA::SystemException)
    {
      BankState_var st = state();
      st->push_events(digests, events);
    }
    
    void
    BankImpl::login_completed(Event::BankSession* session) throw()
    {
      try
      {
        if(Application::will_trace(El::Logging::LOW))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::BankImpl::login_completed()";

          Application::logger()->trace(ostr.str(),
                                       Aspect::STATE,
                                       El::Logging::LOW);
        }

        const Server::Config::EventBankType::bank_management_type&
          config =  Application::instance()->config().bank_management();
          
        BankState_var st = new ManagingEvents(
          session,
          ACE_Time_Value(config.report_presence_period()),
          this);
        
        state(st.in());  
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::BankImpl::login_completed: "
            "El::Exception caught. Description:" << std::endl << e;

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(...)
        {
        }
      }
    }
    
    void
    BankImpl::logout(Event::BankSessionId* session_id) throw()
    {
      try
      {
        if(Application::will_trace(El::Logging::LOW))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::BankImpl::logout()";

          Application::logger()->trace(ostr.str(),
                                       Aspect::STATE,
                                       El::Logging::LOW);
        }

        const Server::Config::EventBankType::bank_management_type&
          config =  Application::instance()->config().bank_management();
          
        BankState_var st = new LogingIn(
          ACE_Time_Value(config.login_retry_period()),
          this);
        
        state(st.in());  
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::BankImpl::logout: "
            "El::Exception caught. Description:" << std::endl << e;

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(...)
        {
        }
      }      
    }
    
  }
}
