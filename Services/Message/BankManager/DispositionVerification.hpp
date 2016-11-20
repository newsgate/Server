/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Message/BankManager/DispositionVerification.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MESSAGE_BANKMANAGER_DISPOSITIONVERIFICATION_HPP_
#define _NEWSGATE_SERVER_SERVICES_MESSAGE_BANKMANAGER_DISPOSITIONVERIFICATION_HPP_

#include <string>
#include <list>
#include <set>

#include <ext/hash_map>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/MySQL/DB.hpp>
#include <El/Service/Service.hpp>
#include <El/Hash/Hash.hpp>

#include <Commons/Message/TransportImpl.hpp>
#include <Commons/Message/Categorizer.hpp>

#include <Services/Commons/Message/BankSessionImpl.hpp>
#include <Services/Commons/Message/MessageServices.hpp>

#include "SubService.hpp"

namespace NewsGate
{
  namespace Message
  {
    //
    // DispositionVerification class
    //
    class DispositionVerification : public BankManagerState
    {
    public:
        
      EL_EXCEPTION(Exception, BankManagerState::Exception);

      DispositionVerification(
        const BankDisposition& disposition,
        const ACE_Time_Value& presence_poll_timeout,
        const ACE_Time_Value& presence_check_period,
        const Server::Config::MessageBankManagerType::message_sharing_type&
          message_sharing_conf,
        const Server::Config::MessageManagementType&
          message_management_conf,
        BankManagerStateCallback* callback)
        throw(Exception, El::Exception);

      virtual ~DispositionVerification() throw();

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

      ::NewsGate::Message::BankClientSession* create_client_session(
        bool invalidate_inactive)
        throw(CORBA::SystemException, El::Exception);
      
      virtual bool notify(El::Service::Event* event) throw(El::Exception);

    private:
      
      bool discover_message_sharing_managers() throw(El::Exception);
      
      bool check_validation_id(const char* validation_id) const
        throw();
      
      void check_banks_presence() throw();
      void check_message_fetch_filter() throw();
      void check_message_categorizer() throw();

      typedef std::set<Categorizer::Category::Id> IdSet;
      
      void load_category(Categorizer::Category::Id id,
                         Categorizer::CategoryMap& categories,
                         El::MySQL::Connection* connection,
                         IdSet& context)
        throw(Exception, El::Exception);
      
      void accept_message_fetch_filter(
        ::NewsGate::Message::Transport::SetMessageFetchFilterRequest* filter,
        bool shared) throw();

      void accept_message_categorizer(
        ::NewsGate::Message::Transport::SetMessageCategorizerRequest*
        categorier) throw();

      void propagate_message_fetch_filter(
        ::NewsGate::Message::Transport::SetMessageFetchFilterRequestImpl::Type*
          request,
        bool shared)
        throw(Exception, El::Exception);

      void propagate_message_categorizer(
        ::NewsGate::Message::Transport::SetMessageCategorizerRequestImpl::Type*
        request,
        bool shared)
        throw(Exception, El::Exception);
      
      void message_sharing_register_foreign_client(
        CORBA::ULong interface_version,
        const char* manager_persistent_id,
        ::NewsGate::Message::BankClientSession* message_sink,
        CORBA::ULong flags,
        CORBA::ULong expiration_time,
        Transport::ColoFrontendArray* colo_frontends)
        throw();

      void disposition_breakdown() throw();
      void message_sharing_register_self() throw();

      void clean_sinks() throw(Exception, El::Exception);

      struct CheckBanksPresence : public El::Service::CompoundServiceMessage
      {
        CheckBanksPresence(DispositionVerification* state)
          throw(El::Exception);
      };
      
      struct MessageSharingRegisterForeignClient :
        public El::Service::CompoundServiceMessage
      {
        MessageSharingRegisterForeignClient(
          CORBA::ULong interface_version_val,
          const char* manager_persistent_id_val,
          ::NewsGate::Message::BankClientSession* message_sink_val,
          CORBA::ULong flags_val,
          CORBA::ULong expiration_time_val,
          ::NewsGate::Message::Transport::ColoFrontendPack* colo_frontends_val,
          DispositionVerification* state_val)
          throw(El::Exception);

        virtual ~MessageSharingRegisterForeignClient() throw();

        CORBA::ULong interface_version;
        std::string manager_persistent_id;
        ::NewsGate::Message::BankClientSession_var message_sink;
        CORBA::ULong flags;
        CORBA::ULong expiration_time;
        Transport::ColoFrontendArrayPtr colo_frontends;

      private:
        MessageSharingRegisterForeignClient(
          MessageSharingRegisterForeignClient&);
        
        void operator=(MessageSharingRegisterForeignClient&);
      };

      typedef El::RefCount::SmartPtr<MessageSharingRegisterForeignClient>
      MessageSharingRegisterForeignClient_var;
      
      struct MessageSharingRegisterSelf :
        public El::Service::CompoundServiceMessage
      {
        MessageSharingRegisterSelf(DispositionVerification* state)
          throw(El::Exception);
      };
      
      struct CheckMessageFetchFilter :
        public El::Service::CompoundServiceMessage
      {
        CheckMessageFetchFilter(DispositionVerification* state)
          throw(El::Exception);
      };

      struct CheckMessageCategorizer :
        public El::Service::CompoundServiceMessage
      {
        CheckMessageCategorizer(DispositionVerification* state)
          throw(El::Exception);
      };

      struct AcceptMessageFetchFilter :
        public El::Service::CompoundServiceMessage
      {
        Transport::SetMessageFetchFilterRequest_var filter;
        bool shared;
          
        AcceptMessageFetchFilter(
          DispositionVerification* state,
          Transport::SetMessageFetchFilterRequest* filter_val,
          bool shared_val)
          throw(El::Exception);

        ~AcceptMessageFetchFilter() throw() {}
      };

      struct AcceptMessageCategorizer :
        public El::Service::CompoundServiceMessage
      {
        Transport::SetMessageCategorizerRequest_var categorizer;
          
        AcceptMessageCategorizer(
          DispositionVerification* state,
          Transport::SetMessageCategorizerRequest* categorizer_val)
          throw(El::Exception);

        ~AcceptMessageCategorizer() throw() {}
      };

    private:

      struct MessageSharingManager
      {
        BankManager_var reference;
        unsigned long flags;
        std::string ior;

        MessageSharingManager() throw();
      };

      typedef std::list<MessageSharingManager> MessageSharingManagerList;

      struct SharingSink
      {
        ::NewsGate::Message::BankManager_var manager;
        unsigned long flags;
        ACE_Time_Value expiration_time;
        Transport::ColoFrontendArray colo_frontends;

        SharingSink() throw() : flags(0) {}
      };
      
      class SharingSinkMap : public __gnu_cxx::hash_map<std::string,
                                                        SharingSink,
                                                        El::Hash::String>
      {
      public:
        SharingSinkMap() throw(El::Exception) {}
      };
      
      BankDisposition disposition_;
      SharingSinkMap sharing_sink_map_;

      typedef std::auto_ptr<Message::Transport::SharingIdSet> SharingIdSetPtr;
      SharingIdSetPtr sharing_ids_;
      
      ACE_Time_Value presence_poll_timeout_;
      ACE_Time_Value presence_check_period_;

      const Server::Config::MessageBankManagerType::message_sharing_type&
      message_sharing_conf_;

      const Server::Config::MessageManagementType& message_management_conf_;
      
      MessageSharingManagerList message_sharing_managers_;
      bool message_sharing_managers_discovered_;

      ACE_Time_Value message_fetch_filter_resend_time_;
      uint64_t message_fetch_filter_stamp_;      

      ACE_Time_Value message_categorizer_resend_time_;
      uint64_t message_categorizer_stamp_;

      Transport::ColoFrontend own_colo_frontend_;
      Transport::ColoFrontend master_colo_frontend_;
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
    // DispositionVerification::MessageSharingManager class
    //
    inline
    DispositionVerification::MessageSharingManager::MessageSharingManager()
      throw() : flags(0)
    {
    }

    //
    // DispositionVerification::AcceptMessageFetchFilter class
    //
    inline
    DispositionVerification::AcceptMessageFetchFilter::
    AcceptMessageFetchFilter(
      DispositionVerification* state,
      Transport::SetMessageFetchFilterRequest* filter_val,
      bool shared_val)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state),
          filter(filter_val),
          shared(shared_val)
    {
      filter->_add_ref();
    }
    
    //
    // DispositionVerification::AcceptMessageCategorizer class
    //
    inline
    DispositionVerification::AcceptMessageCategorizer::
    AcceptMessageCategorizer(
      DispositionVerification* state,
      Transport::SetMessageCategorizerRequest* categorizer_val)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state),
          categorizer(categorizer_val)
    {
      categorizer->_add_ref();
    }

    //
    // DispositionVerification::CheckBanksPresence class
    //
    inline
    DispositionVerification::CheckBanksPresence::CheckBanksPresence(
      DispositionVerification* state) throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }
    
    //
    // DispositionVerification::MessageSharingRegisterSelf class
    //
    inline
    DispositionVerification::MessageSharingRegisterSelf::
    MessageSharingRegisterSelf(DispositionVerification* state)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }
    
    //
    // DispositionVerification::MessageSharingRegister class
    //
    inline
    DispositionVerification::MessageSharingRegisterForeignClient::
    MessageSharingRegisterForeignClient(
      CORBA::ULong interface_version_val,
      const char* manager_persistent_id_val,
      ::NewsGate::Message::BankClientSession* message_sink_val,
      CORBA::ULong flags_val,
      CORBA::ULong expiration_time_val,
      ::NewsGate::Message::Transport::ColoFrontendPack* colo_frontends_val,
      DispositionVerification* state)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state),
          interface_version(interface_version_val),
          manager_persistent_id(manager_persistent_id_val),
          message_sink(message_sink_val),
          flags(flags_val),
          expiration_time(expiration_time_val)
    {
      if(message_sink_val != 0)
      {
        message_sink->_add_ref();
      }

      if(colo_frontends_val)
      {
        Transport::ColoFrontendPackImpl::Type* cf =
          dynamic_cast<Transport::ColoFrontendPackImpl::Type*>(
            colo_frontends_val);
        
        if(cf == 0)
        {
          throw Exception(
            "dynamic_cast<Transport::ColoFrontendPackImpl::Type*> failed");
        }
        
        colo_frontends.reset(cf->release());
        assert(colo_frontends.get() != 0);
      }
    }

    inline
    DispositionVerification::MessageSharingRegisterForeignClient::
    ~MessageSharingRegisterForeignClient()
      throw()
    {
    }

    //
    // DispositionVerification::CheckMessageFetchFilter class
    //
    inline
    DispositionVerification::CheckMessageFetchFilter::CheckMessageFetchFilter(
      DispositionVerification* state)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }
    
    //
    // DispositionVerification::CheckMessageCategorizer class
    //
    inline
    DispositionVerification::CheckMessageCategorizer::CheckMessageCategorizer(
      DispositionVerification* state)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_MESSAGE_BANKMANAGER_DISPOSITIONVERIFICATION_HPP_
