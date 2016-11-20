/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Message/BankManagerManager/SubService.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MESSAGE_BANKMANAGER_SUBSERVICE_HPP_
#define _NEWSGATE_SERVER_SERVICES_MESSAGE_BANKMANAGER_SUBSERVICE_HPP_

#include <vector>
#include <string>

#include <El/Exception.hpp>
#include <El/RefCount/All.hpp>
#include <El/Service/CompoundService.hpp>

#include <Services/Commons/Message/MessageServices.hpp>
#include <Services/Commons/Message/BankClientSessionImpl.hpp>

namespace NewsGate
{
  namespace Message
  {
    struct BankInfo
    {
      BankClientSessionImpl::BankRef bank;
      bool active;
      ACE_Time_Value timestamp;

      BankInfo(const BankClientSessionImpl::BankRef& bank_ref,
               bool is_active = true,
               const ACE_Time_Value& ts = ACE_Time_Value::zero)
        throw(El::Exception);

      BankInfo() throw() : active(true) {}
    };
    
    typedef std::vector<BankInfo> BankDisposition;
    
    class BankManagerStateCallback : public virtual El::Service::Callback
    {
    public:
      virtual void finalize_bank_registration(
        const BankDisposition& disposition) throw() = 0;

      virtual void disposition_breakdown() throw() = 0;
      virtual void finalize_disposition_disbandment() throw() = 0;
      virtual const char* get_process_id() throw() = 0;
    };

    //
    // State basic class
    //
    class BankManagerState :
      public El::Service::CompoundService<El::Service::Service,
                                          BankManagerStateCallback>
    {
    public:
        
      EL_EXCEPTION(Exception, El::ExceptionBase);

      BankManagerState(BankManagerStateCallback* callback, const char* name)
        throw(Exception, El::Exception);

      ~BankManagerState() throw();
        
      virtual ::NewsGate::Message::BankSession* bank_login(
        const char* bank_ior,
        ::NewsGate::Message::BankSessionId* current_session_id)
        throw(NewsGate::Message::NotReady,
              NewsGate::Message::ImplementationException,
              CORBA::SystemException) = 0;

      virtual void ping(const char* bank_ior,
                        ::NewsGate::Message::BankSessionId* current_session_id)
        throw(NewsGate::Message::Logout,
              NewsGate::Message::ImplementationException,
              CORBA::SystemException) = 0;

      virtual void terminate(
        const char* bank_ior,
        ::NewsGate::Message::BankSessionId* current_session_id)
        throw(NewsGate::Message::ImplementationException,
              CORBA::SystemException) = 0;

      virtual ::NewsGate::Message::BankClientSession* bank_client_session()
        throw(NewsGate::Message::ImplementationException,
              CORBA::SystemException) = 0;

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
              CORBA::SystemException) = 0;

      virtual void message_sharing_unregister(
        const char* manager_persistent_id)
        throw(NewsGate::Message::ImplementationException,
              CORBA::SystemException) = 0;

      virtual void set_message_fetch_filter(
        ::NewsGate::Message::Transport::SetMessageFetchFilterRequest* filter,
        ::CORBA::Boolean shared,
        const char* validation_id)
        throw(NewsGate::Message::ImplementationException,
              ::CORBA::SystemException) = 0;

      virtual void set_message_categorizer(
        ::NewsGate::Message::Transport::SetMessageCategorizerRequest*
        categorizer,
        const char* validation_id)
        throw(NewsGate::Message::ImplementationException,
              ::CORBA::SystemException) = 0;      
    };

    typedef El::RefCount::SmartPtr<BankManagerState> BankManagerState_var;

    namespace Aspect
    {
      extern const char STATE[];
      extern const char MSG_SHARING[];
      extern const char MSG_FILTER_DIST[];
      extern const char MSG_CATEGORIZER_DIST[];
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
    // BankManagerState class
    //
    inline
    BankManagerState::BankManagerState(BankManagerStateCallback* callback,
                                       const char* name)
      throw(Exception, El::Exception)
        : El::Service::CompoundService<
             El::Service::Service,
             BankManagerStateCallback>(callback, name)
    {
    }
    
    inline
    BankManagerState::~BankManagerState() throw()
    {
    }

    //
    // BankInfo struct
    //
    inline
    BankInfo::BankInfo(const BankClientSessionImpl::BankRef& bank_ref,
                       bool is_active,
                       const ACE_Time_Value& ts)
      throw(El::Exception)
        : bank(bank_ref),
          active(is_active),
          timestamp(ts)
    {
    }
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_MESSAGE_BANKMANAGER_SUBSERVICE_HPP_
