/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Event/Bank/SessionSupport.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <ace/OS.h>

//#include <Commons/Message/TransportImpl.hpp>
//#include <Services/Commons/Message/MessageBankRecord.hpp>

#include "BankMain.hpp"
#include "SessionSupport.hpp"

namespace NewsGate
{
  namespace Event
  {
    //
    // LogingIn class
    //
    LogingIn::LogingIn(const ACE_Time_Value& login_retry_period,
                       BankStateCallback* callback)
      throw(Exception, El::Exception)
        : BankState(callback, "LogingIn"),
          login_retry_period_(login_retry_period)
    {
      El::Service::CompoundServiceMessage_var msg = new FinalizeLogin(this);
      deliver_now(msg.in());
    }

    void
    LogingIn::finalize_login() throw()
    {
      if(!started())
      {
        return;
      }
      
      try
      {
        try
        {
          Event::BankManager_var bank_manager =
            Application::instance()->bank_manager();
            
          Event::BankSession_var session = bank_manager->bank_login(
            Application::instance()->bank_ior());
          
          callback_->login_completed(session.in());
          return;
        }        
        catch(const NotReady& e)
        {
          if(Application::will_trace(El::Logging::MIDDLE))
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Event::LogingIn::finalize_login: "
              "bank manager not ready. Reason:\n" << e.reason.in();
            
            Application::logger()->trace(ostr.str(),
                                         Aspect::STATE,
                                         El::Logging::MIDDLE);
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::LogingIn::finalize_login: "
            "got El::Exception.\n" << e.what();
        
          El::Service::Error error(ostr.str(),
                                   this,
                                   El::Service::Error::ALERT);
          callback_->notify(&error);
       }
        catch (const CORBA::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::LogingIn::finalize_login: "
            "got CORBA::Exception.\n" << e;
        
          El::Service::Error error(ostr.str(),
                                   this,
                                   El::Service::Error::ALERT);
          callback_->notify(&error);
        }

        try
        {
          El::Service::CompoundServiceMessage_var msg =
            new FinalizeLogin(this);
          
          deliver_at_time(msg.in(),
                          ACE_OS::gettimeofday() + login_retry_period_);
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::LogingIn::finalize_login: "
            "El::Exception caught while rescheduling the task. Description:"
               << std::endl << e.what();

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        
      }
      catch(...)
      {
        El::Service::Error error(
          "NewsGate::Event::LogingIn::finalize_login: "
          "unknown exception thrown from other exception handler",
          this);

        callback_->notify(&error);
        
        return;
      }      
    }

    ::CORBA::ULong
    LogingIn::get_message_events(
      ::NewsGate::Message::Transport::IdPack* messages,
      ::NewsGate::Message::Transport::MessageEventPack_out events)
      throw(NewsGate::Event::NotReady,
            NewsGate::Event::ImplementationException,
            ::CORBA::SystemException)
    {
      NewsGate::Event::NotReady e;
      
      e.reason = CORBA::string_dup("LogingIn::get_message_events: "
                                   "still logging in ...");

      throw e;
    }
      
    void
    LogingIn::post_message_digest(
      ::NewsGate::Event::Transport::MessageDigestPack* digests,
      ::NewsGate::Message::Transport::MessageEventPack_out events)
      throw(NewsGate::Event::NotReady,
            NewsGate::Event::ImplementationException,
            ::CORBA::SystemException)
    {
      NewsGate::Event::NotReady e;
      
      e.reason = CORBA::string_dup("LogingIn::post_messages: "
                                   "still logging in ...");

      throw e;
    }
    
    void
    LogingIn::get_events(
      ::NewsGate::Event::Transport::EventIdRelPack* ids,
      ::NewsGate::Event::Transport::EventObjectRelPack_out events)
      throw(NewsGate::Event::NotReady,
            NewsGate::Event::ImplementationException,
            ::CORBA::SystemException)
    {
      NewsGate::Event::NotReady e;
      
      e.reason = CORBA::string_dup("LogingIn::post_messages: "
                                   "still logging in ...");

      throw e;
    }
    
    void
    LogingIn::push_events(
      ::NewsGate::Event::Transport::MessageDigestPack* digests,
      ::NewsGate::Event::Transport::EventPushInfoPack* events)
      throw(NewsGate::Event::NotReady,
            NewsGate::Event::ImplementationException,
            ::CORBA::SystemException)
    {
      NewsGate::Event::NotReady e;
      
      e.reason = CORBA::string_dup("LogingIn::push_events: "
                                   "still logging in ...");

      throw e;
    }
    
    void
    LogingIn::delete_messages(::NewsGate::Message::Transport::IdPack* ids)
      throw(NewsGate::Event::NotReady,
            NewsGate::Event::ImplementationException,
            ::CORBA::SystemException)
    {
      NewsGate::Event::NotReady e;
      
      e.reason = CORBA::string_dup("LogingIn::delete_messages: "
                                   "still logging in ...");

      throw e;
    }

    bool
    LogingIn::notify(El::Service::Event* event) throw(El::Exception)
    {
      if(BankState::notify(event))
      {
        return true;
      }

      if(dynamic_cast<FinalizeLogin*>(event) != 0)
      {
        finalize_login();
        return true;
      }
      
      std::ostringstream ostr;
      ostr << "NewsGate::Event::LogingIn::notify: unknown " << *event;
      
      El::Service::Error error(ostr.str(), this);
      callback_->notify(&error);
      
      return true;
    }

    
  }
}
