/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Message/Bank/SessionSupport.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <ace/OS.h>

#include <Commons/Message/TransportImpl.hpp>
#include <Services/Commons/Message/MessageBankRecord.hpp>

#include "BankMain.hpp"
#include "SessionSupport.hpp"

namespace NewsGate
{
  namespace Message
  {
    //
    // LogingIn class
    //
    LogingIn::LogingIn(Message::BankSessionId* prev_session_id,
                       const ACE_Time_Value& login_retry_period,
                       BankStateCallback* callback)
      throw(Exception, El::Exception)
        : BankState(callback, "LogingIn"),
          login_retry_period_(login_retry_period)
    {
      if(prev_session_id != 0)
      {
        prev_session_id->_add_ref();
        prev_session_id_ = prev_session_id;
      }

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
          if(prev_session_id_.in() == 0)
          {
            prev_session_id_ = Application::instance()->create_session_id();

            El::MySQL::Connection_var connection =
              Application::instance()->dbase()->connect();

            std::ostringstream ostr;
            ostr << "select bank_ior, session_id, last_ping_time from "
              "MessageBankSession where bank_ior='"
                 << connection->escape(Application::instance()->bank_ior())
                 << "'";

            El::MySQL::Result_var result =
              connection->query(ostr.str().c_str());

            BankSessionInfo bank_session(result.in());
            
            if(bank_session.fetch_row() &&
               !bank_session.session_id().is_null())
            {
              prev_session_id_->from_string(bank_session.session_id().c_str());
            }
          }

          Message::BankManager_var bank_manager =
            Application::instance()->bank_manager();
            
          Message::BankSession_var session = bank_manager->bank_login(
            Application::instance()->bank_ior(),
            prev_session_id_.in());

          CORBA::String_var session_id = session->id()->to_string();

          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();
          
          std::string bank_ior =
            connection->escape(Application::instance()->bank_ior());

          std::string sess_id = connection->escape(session_id.in());
          
          std::ostringstream ostr;
          ostr << "insert into MessageBankSession set bank_ior='" << bank_ior
               << "', session_id='" << sess_id
               << "' on duplicate key update session_id='" << sess_id << "'";

          El::MySQL::Result_var result =
            connection->query(ostr.str().c_str());

          callback_->login_completed(session.in());
          
          return;          
        }        
        catch(const NotReady& e)
        {
          if(Application::will_trace(El::Logging::MIDDLE))
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Message::LogingIn::finalize_login: "
              "bank manager not ready. Reason:\n" << e.reason.in();
            
            Application::logger()->trace(ostr.str(),
                                         Aspect::STATE,
                                         El::Logging::MIDDLE);
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::LogingIn::finalize_login: "
            "got El::Exception.\n" << e.what();
        
          El::Service::Error error(ostr.str(),
                                   this,
                                   El::Service::Error::ALERT);
          callback_->notify(&error);
       }
        catch (const CORBA::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::LogingIn::finalize_login: "
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
          ostr << "LogingIn::finalize_login: "
            "El::Exception caught while rescheduling the task. Description:"
               << std::endl << e.what();

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        
      }
      catch(...)
      {
        El::Service::Error error(
          "LogingIn::finalize_login: "
          "unknown exception thrown from other exception handler",
          this);

        callback_->notify(&error);
        
        return;
      }      
    }
    
    void
    LogingIn::post_messages(
      ::NewsGate::Message::Transport::MessagePack* messages,
      ::NewsGate::Message::PostMessageReason reason,
      const char* validation_id)
      throw(NewsGate::Message::NotReady,
            NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      NewsGate::Message::NotReady e;
      e.reason = CORBA::string_dup("LogingIn::post_messages: "
                                   "still logging in ...");

      throw e;
    }

    void
    LogingIn::search(const ::NewsGate::Message::SearchRequest& request,
                     ::NewsGate::Message::MatchedMessages_out result)
      throw(NewsGate::Message::NotReady,
            NewsGate::Message::ImplementationException,
            ::CORBA::SystemException)
    {
      NewsGate::Message::NotReady e;
      e.reason = CORBA::string_dup("LogingIn::search: "
                                   "still logging in ...");

      throw e;
    }

    ::NewsGate::Message::Transport::StoredMessagePack*
    LogingIn::get_messages(
      ::NewsGate::Message::Transport::IdPack* message_ids,
      ::CORBA::ULongLong gm_flags,
      ::CORBA::Long img_index,
      ::CORBA::Long thumb_index,
      ::NewsGate::Message::Transport::IdPack_out notfound_message_ids)
      throw(NewsGate::Message::NotReady,
            NewsGate::Message::ImplementationException,
            ::CORBA::SystemException)
    {
      NewsGate::Message::NotReady e;
      e.reason = CORBA::string_dup("LogingIn::get_messages: "
                                   "still logging in ...");

      throw e;
    }
      
    void
    LogingIn::send_request(
      ::NewsGate::Message::Transport::Request* req,
      ::NewsGate::Message::Transport::Response_out resp)
      throw(NewsGate::Message::NotReady,
            NewsGate::Message::InvalidData,
            NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      NewsGate::Message::NotReady e;
      e.reason = CORBA::string_dup("LogingIn::send_request: "
                                   "still logging in ...");

      throw e;
    }
    
    void
    LogingIn::message_sharing_register(
      const char* manager_persistent_id,
      ::NewsGate::Message::BankClientSession* message_sink,
      CORBA::ULong flags,
      CORBA::ULong expiration_time)
      throw(NewsGate::Message::NotReady,
            NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      NewsGate::Message::NotReady e;
      e.reason = CORBA::string_dup("LogingIn::message_sharing_register: "
                                   "still logging in ...");

      throw e;
    }

    void
    LogingIn::message_sharing_unregister(
      const char* manager_persistent_id)
      throw(NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
    }

    void
    LogingIn::message_sharing_offer(
      ::NewsGate::Message::Transport::MessageSharingInfoPack* offered_messages,
      const char* validation_id,
      ::NewsGate::Message::Transport::IdPack_out requested_messages)
      throw(NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      try
      {
        requested_messages = Transport::IdPackImpl::Init::create(
          new IdArray());
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::LogingIn::message_sharing_offer: "
          "El::Exception caught. Description:\n" << e.what();

        ImplementationException e;
        e.description = CORBA::string_dup(ostr.str().c_str());

        throw e;
      }
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
      ostr << "NewsGate::Message::LogingIn::notify: unknown " << *event;
      
      El::Service::Error error(ostr.str(), this);
      callback_->notify(&error);
      
      return true;
    }

    
  }
}
