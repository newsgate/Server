/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Message/Bank/SessionSupport.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_SESSIONSUPPORT_HPP_
#define _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_SESSIONSUPPORT_HPP_

#include <string>

#include <ace/OS.h>

#include <El/Exception.hpp>

#include "SubService.hpp"

namespace NewsGate
{
  namespace Message
  {
    //
    // RegisteringBanks class
    //
    class LogingIn : public BankState
    {
    public:
        
      EL_EXCEPTION(Exception, BankState::Exception);

      LogingIn(Message::BankSessionId* prev_session_id,
               const ACE_Time_Value& login_retry_period,
               BankStateCallback* callback)
        throw(Exception, El::Exception);

      virtual void post_messages(
        ::NewsGate::Message::Transport::MessagePack* messages,
        ::NewsGate::Message::PostMessageReason reason,
        const char* validation_id)
        throw(NewsGate::Message::NotReady,
              NewsGate::Message::ImplementationException,
              CORBA::SystemException);

      virtual void message_sharing_register(
        const char* manager_persistent_id,
        ::NewsGate::Message::BankClientSession* message_sink,
        CORBA::ULong flags,
        CORBA::ULong expiration_time)
        throw(NewsGate::Message::NotReady,
              NewsGate::Message::ImplementationException,
              CORBA::SystemException);

      virtual void message_sharing_unregister(
        const char* manager_persistent_id)
        throw(NewsGate::Message::ImplementationException,
              CORBA::SystemException);

      virtual void message_sharing_offer(
        ::NewsGate::Message::Transport::MessageSharingInfoPack*
          offered_messages,
        const char* validation_id,
        ::NewsGate::Message::Transport::IdPack_out requested_messages)
        throw(NewsGate::Message::ImplementationException,
              CORBA::SystemException);
      
      virtual void search(const ::NewsGate::Message::SearchRequest& request,
                          ::NewsGate::Message::MatchedMessages_out result)
        throw(NewsGate::Message::NotReady,
              NewsGate::Message::ImplementationException,
              ::CORBA::SystemException);
      
      virtual ::NewsGate::Message::Transport::StoredMessagePack* get_messages(
        ::NewsGate::Message::Transport::IdPack* message_ids,
        ::CORBA::ULongLong gm_flags,
        ::CORBA::Long img_index,
        ::CORBA::Long thumb_index,
        ::NewsGate::Message::Transport::IdPack_out notfound_message_ids)
        throw(NewsGate::Message::NotReady,
              NewsGate::Message::ImplementationException,
              ::CORBA::SystemException);
      
      virtual void send_request(
        ::NewsGate::Message::Transport::Request* req,
        ::NewsGate::Message::Transport::Response_out resp)
        throw(NewsGate::Message::NotReady,
              NewsGate::Message::InvalidData,
              NewsGate::Message::ImplementationException,
              CORBA::SystemException);      

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
      Message::BankSessionId_var prev_session_id_;
    };

  }
}


///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Message
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

#endif // _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_SESSIONSUPPORT_HPP_
