/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Message/Bank/BankImpl.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_BANKIMPL_HPP_
#define _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_BANKIMPL_HPP_

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/Service/CompoundService.hpp>

#include <Services/Commons/Message/MessageServices_s.hpp>

#include "SubService.hpp"
#include "ManagingMessages.hpp"

namespace NewsGate
{
  namespace Message
  {
    class BankImpl :
      public virtual POA_NewsGate::Message::Bank,
      public virtual El::Service::CompoundService<BankState>, 
      protected virtual BankStateCallback
    {
    public:
      EL_EXCEPTION(Exception, El::ExceptionBase);
      EL_EXCEPTION(InvalidArgument, Exception);

    public:

      BankImpl(El::Service::Callback* callback)
        throw(InvalidArgument, Exception, El::Exception);
      
      virtual ~BankImpl() throw();
      
      //
      // IDL:NewsGate/Message/Bank/post_messages:1.0
      //
      virtual void post_messages(
        ::NewsGate::Message::Transport::MessagePack* messages,
        ::NewsGate::Message::PostMessageReason reason,
        const char* validation_id)
        throw(NewsGate::Message::NotReady,
              NewsGate::Message::ImplementationException,
              CORBA::SystemException);
      
      //
      // IDL:NewsGate/Message/Bank/message_sharing_register:1.0
      //
      virtual void message_sharing_register(
        const char* manager_persistent_id,
        ::NewsGate::Message::BankClientSession* message_sink,
        CORBA::ULong flags,
        CORBA::ULong expiration_time)
        throw(NewsGate::Message::NotReady,
              NewsGate::Message::ImplementationException,
              CORBA::SystemException);

      //
      // IDL:NewsGate/Message/Bank/message_sharing_unregister:1.0
      //
      virtual void message_sharing_unregister(
        const char* manager_persistent_id)
        throw(NewsGate::Message::ImplementationException,
              CORBA::SystemException);
      
      //
      // IDL:NewsGate/Message/Bank/message_sharing_offer:1.0
      //
      virtual void message_sharing_offer(
        ::NewsGate::Message::Transport::MessageSharingInfoPack*
          offered_messages,
        const char* validation_id,
        ::NewsGate::Message::Transport::IdPack_out requested_messages)
        throw(NewsGate::Message::ImplementationException,
              CORBA::SystemException);
      
      //
      // IDL:NewsGate/Message/Bank/search:1.0
      //
      virtual void search(const ::NewsGate::Message::SearchRequest& request,
                          ::NewsGate::Message::MatchedMessages_out result)
        throw(NewsGate::Message::NotReady,
              NewsGate::Message::ImplementationException,
              ::CORBA::SystemException);
      
      //
      // IDL:NewsGate/Message/Bank/get_messages:1.0
      //
      virtual ::NewsGate::Message::Transport::StoredMessagePack* get_messages(
        ::NewsGate::Message::Transport::IdPack* message_ids,
        ::CORBA::ULongLong gm_flags,
        ::CORBA::Long img_index,
        ::CORBA::Long thumb_index,
        ::NewsGate::Message::Transport::IdPack_out notfound_message_ids)
        throw(NewsGate::Message::NotReady,
              NewsGate::Message::ImplementationException,
              ::CORBA::SystemException);
      
      //
      // IDL:NewsGate/Message/Bank/send_request:1.0
      //
      virtual void send_request(
        ::NewsGate::Message::Transport::Request* req,
        ::NewsGate::Message::Transport::Response_out resp)
        throw(NewsGate::Message::NotReady,
              NewsGate::Message::ImplementationException,
              CORBA::SystemException);

//      virtual void wait() throw(Exception, El::Exception);
      
      void flush_messages() throw(Exception, El::Exception);
      
    private:
      //
      // BankStateCallback interface methods
      //

      virtual void login_completed(Message::BankSession* session) throw();
      virtual void logout(Message::BankSessionId* session_id) throw();

    private:

      ManagingMessages_var managing_messages_state_;
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

#endif // _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_BANKIMPL_HPP_
