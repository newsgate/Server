/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Message/Bank/SubService.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_SUBSERVICE_HPP_
#define _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_SUBSERVICE_HPP_

#include <El/Exception.hpp>
#include <El/RefCount/All.hpp>
#include <El/Service/CompoundService.hpp>

#include <Services/Commons/Message/MessageServices_s.hpp>

namespace NewsGate
{
  namespace Message
  {
    class BankStateCallback : public virtual El::Service::Callback
    {
    public:
      virtual void login_completed(Message::BankSession* session) throw() = 0;
      virtual void logout(Message::BankSessionId* session_id) throw() = 0;
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
        
      virtual void post_messages(
        ::NewsGate::Message::Transport::MessagePack* messages,
        ::NewsGate::Message::PostMessageReason reason,
        const char* validation_id)
        throw(NewsGate::Message::NotReady,
              NewsGate::Message::ImplementationException,
              CORBA::SystemException) = 0;
      
      virtual void message_sharing_register(
        const char* manager_persistent_id,
        ::NewsGate::Message::BankClientSession* message_sink,
        CORBA::ULong flags,
        CORBA::ULong expiration_time)
        throw(NewsGate::Message::NotReady,
              NewsGate::Message::ImplementationException,
              CORBA::SystemException) = 0;

      virtual void message_sharing_unregister(
        const char* manager_persistent_id)
        throw(NewsGate::Message::ImplementationException,
              CORBA::SystemException) = 0;

      virtual void message_sharing_offer(
        ::NewsGate::Message::Transport::MessageSharingInfoPack*
          offered_messages,
        const char* validation_id,
        ::NewsGate::Message::Transport::IdPack_out requested_messages)
        throw(NewsGate::Message::ImplementationException,
              CORBA::SystemException) = 0;
      
      virtual void search(const ::NewsGate::Message::SearchRequest& request,
                          ::NewsGate::Message::MatchedMessages_out result)
        throw(NewsGate::Message::NotReady,
              NewsGate::Message::ImplementationException,
              ::CORBA::SystemException) = 0;

      virtual ::NewsGate::Message::Transport::StoredMessagePack* get_messages(
        ::NewsGate::Message::Transport::IdPack* message_ids,
        ::CORBA::ULongLong gm_flags,
        ::CORBA::Long img_index,
        ::CORBA::Long thumb_index,
        ::NewsGate::Message::Transport::IdPack_out notfound_message_ids)
        throw(NewsGate::Message::NotReady,
              NewsGate::Message::ImplementationException,
              ::CORBA::SystemException) = 0;
      
      virtual void send_request(
        ::NewsGate::Message::Transport::Request* req,
        ::NewsGate::Message::Transport::Response_out resp)
        throw(NewsGate::Message::NotReady,
              NewsGate::Message::InvalidData,
              NewsGate::Message::ImplementationException,
              CORBA::SystemException) = 0;      
    };

    typedef El::RefCount::SmartPtr<BankState> BankState_var;

    namespace Aspect
    {
      extern const char STATE[];
      extern const char MSG_SHARING[];
      extern const char MSG_MANAGEMENT[];
      extern const char DB_PERFORMANCE[];
      extern const char PERFORMANCE[];
      extern const char MSG_CATEGORIZATION[];
    }
    
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

#endif // _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_SUBSERVICE_HPP_
