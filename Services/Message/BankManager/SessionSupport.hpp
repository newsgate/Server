/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Message/BankManager/SessionSupport.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MESSAGE_BANKMANAGER_SESSIONSUPPORT_HPP_
#define _NEWSGATE_SERVER_SERVICES_MESSAGE_BANKMANAGER_SESSIONSUPPORT_HPP_

#include <string>
#include <list>
#include <ext/hash_map>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/Hash/Hash.hpp>

#include <Services/Commons/Message/BankSessionImpl.hpp>
#include <Services/Commons/Message/MessageServices.hpp>

#include "SubService.hpp"

namespace NewsGate
{
  namespace Message
  {
    //
    // RegisteringBanks class
    //
    class RegisteringBanks : public BankManagerState
    {
    public:
        
      EL_EXCEPTION(Exception, BankManagerState::Exception);

      RegisteringBanks(const ACE_Time_Value& presence_poll_timeout,
                       const ACE_Time_Value& registration_timeout,
                       BankManagerStateCallback* callback)
        throw(Exception, El::Exception);

      virtual ~RegisteringBanks() throw();

      virtual ::NewsGate::Message::BankSession* bank_login(
        const char* bank_ior,
        ::NewsGate::Message::BankSessionId* current_session_id)
        throw(NewsGate::Message::NotReady,
              NewsGate::Message::ImplementationException,
              CORBA::SystemException);

      virtual void ping(const char* bank_ior,
                        ::NewsGate::Message::BankSessionId* current_session_id)
        throw(NewsGate::Message::Logout,
              NewsGate::Message::ImplementationException,
              CORBA::SystemException);      
      
      virtual void terminate(
        const char* bank_ior,
        ::NewsGate::Message::BankSessionId* current_session_id)
        throw(NewsGate::Message::ImplementationException,
              CORBA::SystemException);      
      
      virtual ::NewsGate::Message::BankClientSession* bank_client_session()
        throw(NewsGate::Message::ImplementationException,
              CORBA::SystemException);
      
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

      virtual void message_sharing_unregister(
        const char* manager_persistent_id)
        throw(NewsGate::Message::ImplementationException,
              CORBA::SystemException);
      
      virtual void set_message_fetch_filter(
        ::NewsGate::Message::Transport::SetMessageFetchFilterRequest* filter,
        ::CORBA::Boolean shared,
        const char* validation_id)
        throw(NewsGate::Message::ImplementationException,
              ::CORBA::SystemException);
      
      virtual void set_message_categorizer(
        ::NewsGate::Message::Transport::SetMessageCategorizerRequest*
        categorizer,
        const char* validation_id)
        throw(NewsGate::Message::ImplementationException,
              ::CORBA::SystemException);

      virtual bool notify(El::Service::Event* event) throw(El::Exception);

    private:

      void save_bank_info(const char* bank_ior,
                          ::NewsGate::Message::BankSessionIdImpl* session)
        throw(NewsGate::Message::NotReady,
              NewsGate::Message::ImplementationException,
              CORBA::SystemException,
              El::Exception);

      ::NewsGate::Message::BankSession* login(const char* bank_ior)
        throw(NewsGate::Message::NotReady,
              NewsGate::Message::ImplementationException,
              CORBA::SystemException,
              El::Exception);
      
      void create_disposition() throw(Exception, El::Exception);
      void start_gathering_info() throw();
      void finalize_registration() throw();

    private:

      struct FinalizeRegistration :
        public El::Service::CompoundServiceMessage
      {
        FinalizeRegistration(RegisteringBanks* state) throw(El::Exception);
      };

      class BankMap :
        public __gnu_cxx::hash_map<std::string,
                                   BankSessionIdImpl_var,
                                   El::Hash::String>
      {
      public:
        BankMap() throw(El::Exception);        
      };

      struct BankRecord
      {
        ACE_Time_Value timestamp;
        size_t banks_count;
        std::string bank_ior;
      };

      class BankRecordMap :
        public __gnu_cxx::hash_map<unsigned long,
                                   BankRecord,
                                   El::Hash::Numeric<unsigned long> >
      {
      public:
        BankRecordMap() throw(El::Exception);
      };

    private:
      ACE_Time_Value presence_poll_timeout_;
      ACE_Time_Value registration_timeout_;
      ACE_Time_Value end_registration_time_;
      unsigned long banks_count_;
      bool continue_session_;
      bool gathering_info_;
      BankMap banks_;
      BankDisposition disposition_;
    };

    //
    // DispositionDisbandment class
    //
    class DispositionDisbandment : public BankManagerState
    {
    public:
        
      EL_EXCEPTION(Exception, BankManagerState::Exception);

      DispositionDisbandment(const ACE_Time_Value& reset_timeout,
                             BankManagerStateCallback* callback)
        throw(Exception, El::Exception);

      virtual ~DispositionDisbandment() throw();

      virtual ::NewsGate::Message::BankSession* bank_login(
        const char* bank_ior,
        ::NewsGate::Message::BankSessionId* current_session_id)
        throw(NewsGate::Message::NotReady,
              NewsGate::Message::ImplementationException,
              CORBA::SystemException);

      virtual void ping(const char* bank_ior,
                        ::NewsGate::Message::BankSessionId* current_session_id)
        throw(NewsGate::Message::Logout,
              NewsGate::Message::ImplementationException,
              CORBA::SystemException);

      virtual void terminate(
        const char* bank_ior,
        ::NewsGate::Message::BankSessionId* current_session_id)
        throw(NewsGate::Message::ImplementationException,
              CORBA::SystemException);

      virtual ::NewsGate::Message::BankClientSession* bank_client_session()
        throw(NewsGate::Message::ImplementationException,
              CORBA::SystemException);

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

      virtual void message_sharing_unregister(
        const char* manager_persistent_id)
        throw(NewsGate::Message::ImplementationException,
              CORBA::SystemException);
      
      virtual void set_message_fetch_filter(
        ::NewsGate::Message::Transport::SetMessageFetchFilterRequest* filter,
        ::CORBA::Boolean shared,
        const char* validation_id)
        throw(NewsGate::Message::ImplementationException,
              ::CORBA::SystemException);

      virtual void set_message_categorizer(
        ::NewsGate::Message::Transport::SetMessageCategorizerRequest*
        categorizer,
        const char* validation_id)
        throw(NewsGate::Message::ImplementationException,
              ::CORBA::SystemException);

      virtual bool notify(El::Service::Event* event) throw(El::Exception);

    private:

      void finalize_disbandment() throw();

      struct FinalizeDisbandment : public El::Service::CompoundServiceMessage
      {
        FinalizeDisbandment(DispositionDisbandment* state)
          throw(El::Exception);
      };
      
    private:
      ACE_Time_Value reset_timeout_;
      ACE_Time_Value end_disbandment_time_;
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
    // RegisteringBanks class
    //
    
    inline
    RegisteringBanks::~RegisteringBanks() throw()
    {
    }

    //
    // RegisteringBanks::BankMap class
    //
    
    inline
    RegisteringBanks::BankMap::BankMap() throw(El::Exception)
    {
    }

    //
    // RegisteringBanks::BankRecordMap class
    //
    
    inline
    RegisteringBanks::BankRecordMap::BankRecordMap() throw(El::Exception)
    {
    }
    
    //
    // RegisteringBanks::FinalizePullersRegistration class
    //
    
    inline
    RegisteringBanks::FinalizeRegistration::
    FinalizeRegistration(RegisteringBanks* state) throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }
    
    //
    // DispositionDisbandment::FinalizeDisbandment class
    //

    inline
    DispositionDisbandment::FinalizeDisbandment::FinalizeDisbandment(
      DispositionDisbandment* state)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }    
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_MESSAGE_BANKMANAGER_SESSIONSUPPORT_HPP_
