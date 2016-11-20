/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Message/Bank/BankImpl.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <string>
#include <sstream>

#include <ace/OS.h>

#include "BankImpl.hpp"
#include "BankMain.hpp"
#include "SessionSupport.hpp"
#include "ManagingMessages.hpp"

namespace NewsGate
{
  namespace Message
  {
    //
    // BankImpl class
    //
    BankImpl::BankImpl(El::Service::Callback* callback)
      throw(InvalidArgument, Exception, El::Exception)
        : El::Service::CompoundService<BankState>(callback, "BankImpl")
    {
      BankState_var st = new LogingIn(
        0,
        ACE_Time_Value(Application::instance()->
                       config().bank_management().login_retry_period()),
        this);

      state(st.in());
    }

    BankImpl::~BankImpl() throw()
    {
      // Check if state is active, then deactivate and log error
    }
/*
    void
    BankImpl::wait() throw(Exception, El::Exception)
    {
      El::Service::CompoundService<BankState>::wait();
      state(0);
    }
*/  
    void
    BankImpl::login_completed(Message::BankSession* session) throw()
    {
      try
      {
        if(Application::will_trace(El::Logging::LOW))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::BankImpl::login_completed()";

          Application::logger()->trace(ostr.str(),
                                       Aspect::STATE,
                                       El::Logging::LOW);
        }

        if(managing_messages_state_.in() == 0)
        {
          const Server::Config::MessageBankType::bank_management_type&
            config =  Application::instance()->config().bank_management();
          
          managing_messages_state_ = new ManagingMessages(
            session,
            ACE_Time_Value(config.report_presence_period()),
            this);
        }
        else
        {
          managing_messages_state_->session(session);
        }
        
        state(managing_messages_state_.in());  
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::BankImpl::login_completed: "
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
    BankImpl::flush_messages() throw(Exception, El::Exception)
    {
      if(managing_messages_state_.in())
      {
        managing_messages_state_->flush_messages();
      }
    }
    
    void
    BankImpl::logout(Message::BankSessionId* session_id) throw()
    {
      try
      {
        if(Application::will_trace(El::Logging::LOW))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::BankImpl::logout()";

          Application::logger()->trace(ostr.str(),
                                       Aspect::STATE,
                                       El::Logging::LOW);
        }

        const Server::Config::MessageBankType::bank_management_type&
          config =  Application::instance()->config().bank_management();
          
        BankState_var st = new LogingIn(
          session_id,
          ACE_Time_Value(config.login_retry_period()),
          this);
        
        state(st.in());  
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::BankImpl::logout: "
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
    BankImpl::post_messages(
      ::NewsGate::Message::Transport::MessagePack* messages,
      ::NewsGate::Message::PostMessageReason reason,
      const char* validation_id)
      throw(NewsGate::Message::NotReady,
            NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      BankState_var st = state();
      st->post_messages(messages, reason, validation_id);
    }

    void
    BankImpl::search(const ::NewsGate::Message::SearchRequest& request,
                     ::NewsGate::Message::MatchedMessages_out result)
      throw(NewsGate::Message::NotReady,
            NewsGate::Message::ImplementationException,
            ::CORBA::SystemException)
    {
      BankState_var st = state();
      st->search(request, result);
    }

    ::NewsGate::Message::Transport::StoredMessagePack*
    BankImpl::get_messages(
      ::NewsGate::Message::Transport::IdPack* message_ids,
      ::CORBA::ULongLong gm_flags,
      ::CORBA::Long img_index,
      ::CORBA::Long thumb_index,
      ::NewsGate::Message::Transport::IdPack_out notfound_message_ids)
      throw(NewsGate::Message::NotReady,
            NewsGate::Message::ImplementationException,
            ::CORBA::SystemException)
    {
      BankState_var st = state();
      return st->get_messages(message_ids,
                              gm_flags,
                              img_index,
                              thumb_index,
                              notfound_message_ids);
    }

    void
    BankImpl::send_request(
      ::NewsGate::Message::Transport::Request* req,
      ::NewsGate::Message::Transport::Response_out resp)
      throw(NewsGate::Message::NotReady,
            NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      BankState_var st = state();
      st->send_request(req, resp);
    }
    
    void
    BankImpl::message_sharing_register(
      const char* manager_persistent_id,
      ::NewsGate::Message::BankClientSession* message_sink,
      CORBA::ULong flags,
      CORBA::ULong expiration_time)
      throw(NewsGate::Message::NotReady,
            NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      BankState_var st = state();
      st->message_sharing_register(manager_persistent_id,
                                   message_sink,
                                   flags,
                                   expiration_time);
    }

    void
    BankImpl::message_sharing_unregister(
      const char* manager_persistent_id)
      throw(NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      BankState_var st = state();
      st->message_sharing_unregister(manager_persistent_id);
    }
    
    void
    BankImpl::message_sharing_offer(
      ::NewsGate::Message::Transport::MessageSharingInfoPack* offered_messages,
      const char* validation_id,
      ::NewsGate::Message::Transport::IdPack_out requested_messages)
      throw(NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      BankState_var st = state();
      
      st->message_sharing_offer(offered_messages,
                                validation_id,
                                requested_messages);
    }
  }  
}
