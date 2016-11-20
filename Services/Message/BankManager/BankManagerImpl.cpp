/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Message/BankManager/BankManagerImpl.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <string>
#include <sstream>

#include <El/Exception.hpp>
#include <El/Guid.hpp>

#include <ace/OS.h>

#include "BankManagerImpl.hpp"
#include "BankManagerMain.hpp"
#include "SessionSupport.hpp"
#include "DispositionVerification.hpp"

namespace NewsGate
{
  namespace Message
  {
    //
    // BankManagerImpl class
    //
    BankManagerImpl::BankManagerImpl(El::Service::Callback* callback)
      throw(InvalidArgument, Exception, El::Exception)
        : El::Service::CompoundService<BankManagerState>(callback,
                                                         "BankManagerImpl")
    {
      El::Guid guid;
      guid.generate();
      
      process_id_ = guid.string(El::Guid::GF_DENSE);
      
      if(Application::will_trace(El::Logging::LOW))
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::BankManagerImpl::BankManagerImpl: "
          "starting with process_id=" << process_id_;

        Application::logger()->info(ostr.str(), Aspect::STATE);
      }

      const Server::Config::MessageBankManagerType::bank_management_type&
        config =  Application::instance()->config().bank_management();
      
      BankManagerState_var st = new RegisteringBanks(
        ACE_Time_Value(config.presence_poll_timeout()),
        ACE_Time_Value(config.registration_timeout()),
        this);

      state(st.in());
    }

    BankManagerImpl::~BankManagerImpl() throw()
    {
      // Check if state is active, then deactivate and log error
    }

    char*
    BankManagerImpl::persistent_id() throw(CORBA::SystemException)
    {
      return CORBA::string_dup(Application::instance()->bank_manager_id());
    }

    ::NewsGate::Message::BankSession*
    BankManagerImpl::bank_login(
      const char* bank_ior,
      ::NewsGate::Message::BankSessionId* current_session_id)
      throw(NewsGate::Message::NotReady,
            NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      BankManagerState_var st = state();
      return st->bank_login(bank_ior, current_session_id);
    }
    
    void
    BankManagerImpl::ping(
      const char* bank_ior,
      ::NewsGate::Message::BankSessionId* current_session_id)
      throw(NewsGate::Message::Logout,
            NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      BankManagerState_var st = state();
      st->ping(bank_ior, current_session_id);
    }
    
    void
    BankManagerImpl::terminate(
      const char* bank_ior,
      ::NewsGate::Message::BankSessionId* current_session_id)
      throw(NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      BankManagerState_var st = state();
      st->terminate(bank_ior, current_session_id);
    }
    
    ::NewsGate::Message::BankClientSession*
    BankManagerImpl::bank_client_session()
      throw(NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      BankManagerState_var st = state();
      return st->bank_client_session();
    }

    void
    BankManagerImpl::set_message_fetch_filter(
      ::NewsGate::Message::Transport::SetMessageFetchFilterRequest* filter,
      ::CORBA::Boolean shared,
        const char* validation_id)
      throw(NewsGate::Message::ImplementationException,
            ::CORBA::SystemException)
    {
      BankManagerState_var st = state();
      st->set_message_fetch_filter(filter, shared, validation_id);
    }

    void
    BankManagerImpl::set_message_categorizer(
      ::NewsGate::Message::Transport::SetMessageCategorizerRequest*
      categorizer,
      const char* validation_id)
      throw(NewsGate::Message::ImplementationException,
            ::CORBA::SystemException)
    {
      BankManagerState_var st = state();
      st->set_message_categorizer(categorizer, validation_id);
    }
    
    char*
    BankManagerImpl::message_sharing_register(
      CORBA::ULong interface_version,
      const char* manager_persistent_id,
      ::NewsGate::Message::BankClientSession* message_sink,
      CORBA::ULong flags,
      CORBA::ULong expiration_time,
      ::NewsGate::Message::Transport::ColoFrontendPack*& colo_frontends)
      throw(NewsGate::Message::NotReady,
            NewsGate::Message::IncompartibleInterface,
            NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      BankManagerState_var st = state();
      return st->message_sharing_register(interface_version,
                                          manager_persistent_id,
                                          message_sink,
                                          flags,
                                          expiration_time,
                                          colo_frontends);
    }
    
    void
    BankManagerImpl::message_sharing_unregister(
      const char* manager_persistent_id)
      throw(NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      BankManagerState_var st = state();
      return st->message_sharing_unregister(manager_persistent_id);
    }
    
    void
    BankManagerImpl::finalize_bank_registration(
      const BankDisposition& disposition) throw()
    {
      try
      {
        if(Application::will_trace(El::Logging::LOW))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::BankManagerImpl::"
            "finalize_bank_registration: disposition\n";

          unsigned long i = 0;
          for(BankDisposition::const_iterator it = disposition.begin();
              it != disposition.end(); it++, i++)
          {
            ostr << "  " << std::dec << i << " " << it->bank.reference()
                 << std::endl;
          }

          Application::logger()->trace(ostr.str(),
                                       Aspect::STATE,
                                       El::Logging::LOW);
        }

        const Server::Config::MessageBankManagerType::bank_management_type&
          config = Application::instance()->config().bank_management();
          
        BankManagerState_var st = new DispositionVerification(
          disposition,
          ACE_Time_Value(config.presence_poll_timeout()),
          ACE_Time_Value(config.presence_check_period()),
          Application::instance()->config().message_sharing(),
          Application::instance()->config().message_management(),
          this);
        
        state(st.in());  
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::BankManagerImpl::"
            "finalize_bank_registration: "
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
    BankManagerImpl::disposition_breakdown() throw()
    {
      try
      {
        if(Application::will_trace(El::Logging::LOW))
        {
          Application::logger()->trace(
            "NewsGate::Message::BankManagerImpl::disposition_breakdown()",
            Aspect::STATE,
            El::Logging::LOW);
        }
        
        BankManagerState_var st = new DispositionDisbandment(
          ACE_Time_Value(Application::instance()->
                         config().bank_management().reset_timeout()),
          this);
        
        state(st.in());
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::BankManagerImpl::"
            "disposition_breakdown: El::Exception caught. Description:"
               << std::endl << e;

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(...)
        {
        }
      }      
    }
    
    void
    BankManagerImpl::finalize_disposition_disbandment() throw()
    {
      try
      {
        if(Application::will_trace(El::Logging::LOW))
        {
          Application::logger()->trace(
            "NewsGate::Message::BankManagerImpl::"
            "finalize_disposition_disbandment()",
            Aspect::STATE,
            El::Logging::LOW);
        }
        
        const Server::Config::MessageBankManagerType::bank_management_type&
          config = Application::instance()->config().bank_management();
      
        BankManagerState_var st = new RegisteringBanks(
          ACE_Time_Value(config.presence_poll_timeout()),
          ACE_Time_Value(config.registration_timeout()),
          this);

        state(st.in());
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::BankManagerImpl::"
            "finalize_disposition_disbandment: "
            "El::Exception caught. Description:"
               << std::endl << e;

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(...)
        {
        }
      }      
    }
    
  }  
}
