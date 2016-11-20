/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Message/BankManager/BankManagerImpl.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MESSAGE_BANKMANAGER_BANKMANAGERIMPL_HPP_
#define _NEWSGATE_SERVER_SERVICES_MESSAGE_BANKMANAGER_BANKMANAGERIMPL_HPP_

#include <string>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/Hash/Hash.hpp>
#include <El/Service/CompoundService.hpp>

#include <Services/Commons/Message/MessageServices_s.hpp>

#include "SubService.hpp"

namespace NewsGate
{
  namespace Message
  {
    class BankManagerImpl :
      public virtual POA_NewsGate::Message::BankManager,
      public virtual El::Service::CompoundService<BankManagerState>, 
      protected virtual BankManagerStateCallback
    {
    public:
      EL_EXCEPTION(Exception, El::ExceptionBase);
      EL_EXCEPTION(InvalidArgument, Exception);

    public:

      BankManagerImpl(El::Service::Callback* callback)
        throw(InvalidArgument, Exception, El::Exception);

      virtual ~BankManagerImpl() throw();

    protected:

      //
      // IDL:NewsGate/Message/BankManager/bank_login:1.0
      //
      virtual ::NewsGate::Message::BankSession* bank_login(
        const char* bank_ior,
        ::NewsGate::Message::BankSessionId* current_session_id)
        throw(NewsGate::Message::NotReady,
              NewsGate::Message::ImplementationException,
              CORBA::SystemException);

      //
      // IDL:NewsGate/Message/BankManager/ping:1.0
      //
      virtual void ping(const char* bank_ior,
                        ::NewsGate::Message::BankSessionId* current_session_id)
        throw(NewsGate::Message::Logout,
              NewsGate::Message::ImplementationException,
              CORBA::SystemException);
      
      //
      // IDL:NewsGate/Message/BankManager/terminate:1.0
      //
      virtual void terminate(
        const char* bank_ior,
        ::NewsGate::Message::BankSessionId* current_session_id)
        throw(NewsGate::Message::ImplementationException,
              CORBA::SystemException);
      
      //
      // IDL:NewsGate/Message/BankManager/bank_client_session:1.0
      //
      virtual ::NewsGate::Message::BankClientSession* bank_client_session()
        throw(NewsGate::Message::ImplementationException,
              CORBA::SystemException);
      
      //
      // IDL:NewsGate/Message/BankManager/message_sharing_register:1.0
      //
      virtual char* message_sharing_register(
        CORBA::ULong interface_version,
        const char* manager_persistent_id,
        ::NewsGate::Message::BankClientSession* message_sink,
        CORBA::ULong flags,
        CORBA::ULong expiration_time,
        ::NewsGate::Message::Transport::ColoFrontendPack*& colo_frontends)
        throw(NewsGate::Message::NotReady,
              NewsGate::Message::IncompartibleInterface,
              NewsGate::Message::ImplementationException,
              CORBA::SystemException);

      //
      // IDL:NewsGate/Message/BankManager/message_sharing_unregister:1.0
      //
      virtual void message_sharing_unregister(
        const char* manager_persistent_id)
        throw(NewsGate::Message::ImplementationException,
              CORBA::SystemException);

      //
      // IDL:NewsGate/Message/BankManager/set_message_fetch_filter:1.0
      //
      virtual void set_message_fetch_filter(
        ::NewsGate::Message::Transport::SetMessageFetchFilterRequest* filter,
        ::CORBA::Boolean shared,
        const char* validation_id)
        throw(NewsGate::Message::ImplementationException,
              ::CORBA::SystemException);

      //
      // IDL:NewsGate/Message/BankManager/set_message_categorizer:1.0
      //
      virtual void set_message_categorizer(
        ::NewsGate::Message::Transport::SetMessageCategorizerRequest*
        categorizer,
        const char* validation_id)
        throw(NewsGate::Message::ImplementationException,
              ::CORBA::SystemException);
      
      //
      // IDL:NewsGate/Message/BankManager/process_id:1.0
      //
      virtual char* process_id() throw(CORBA::SystemException);
      
      //
      // IDL:NewsGate/Message/BankManager/persistent_id:1.0
      //
      virtual char* persistent_id() throw(CORBA::SystemException);

    private:
      //
      // BankManagerStateCallback interface methods
      //
      
      virtual void finalize_bank_registration(
        const BankDisposition& disposition) throw();
      
      virtual void disposition_breakdown() throw();

      virtual void finalize_disposition_disbandment() throw();

      virtual const char* get_process_id() throw();
      
    private:
      std::string process_id_;
    };

    typedef El::RefCount::SmartPtr<BankManagerImpl> BankManagerImpl_var;

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
    // BankManagerImpl class
    //
   
    inline
    char*
    BankManagerImpl::process_id() throw(CORBA::SystemException)
    {
      return CORBA::string_dup(process_id_.c_str());
    }

    inline
    const char*
    BankManagerImpl::get_process_id() throw()
    {
      return process_id_.c_str();
    }
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_MESSAGE_BANKMANAGER_BANKMANAGERIMPL_HPP_
