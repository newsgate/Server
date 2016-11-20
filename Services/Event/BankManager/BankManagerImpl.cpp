/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Event/BankManager/BankManagerImpl.cpp
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
  namespace Event
  {
    //
    // BankManagerImpl class
    //
    BankManagerImpl::BankManagerImpl(El::Service::Callback* callback)
      throw(InvalidArgument, Exception, El::Exception)
        : El::Service::CompoundService<BankManagerState>(callback,
                                                         "BankManagerImpl")
    {
      if(Application::will_trace(El::Logging::LOW))
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::BankManagerImpl::BankManagerImpl: "
          "starting";

        Application::logger()->info(ostr.str(), Aspect::STATE);
      }

      const Server::Config::EventBankManagerType::bank_management_type&
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

    ::NewsGate::Event::BankSession*
    BankManagerImpl::bank_login(const char* bank_ior)
      throw(NewsGate::Event::NotReady,
            NewsGate::Event::ImplementationException,
            ::CORBA::SystemException)
    {
      BankManagerState_var st = state();
      return st->bank_login(bank_ior);
    }

    void
    BankManagerImpl::ping(const char* bank_ior,
                          ::NewsGate::Event::BankSessionId* current_session_id)
      throw(NewsGate::Event::Logout,
            NewsGate::Event::ImplementationException,
            ::CORBA::SystemException)
    {
      BankManagerState_var st = state();
      st->ping(bank_ior, current_session_id);
    }

    ::NewsGate::Event::BankClientSession*
    BankManagerImpl::bank_client_session()
      throw(NewsGate::Event::ImplementationException,
            ::CORBA::SystemException)
    {
      BankManagerState_var st = state();
      return st->bank_client_session();
    }

    void
    BankManagerImpl::finalize_bank_registration(
      const BankDisposition& disposition, const char* session_guid) throw()
    {
      try
      {
        if(Application::will_trace(El::Logging::LOW))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::BankManagerImpl::"
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

        const Server::Config::EventBankManagerType::bank_management_type&
          config = Application::instance()->config().bank_management();
          
        BankManagerState_var st = new DispositionVerification(
          disposition,
          session_guid,
          ACE_Time_Value(config.presence_poll_timeout()),
          ACE_Time_Value(config.presence_check_period()),
          this);
        
        state(st.in());  
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::BankManagerImpl::"
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
            "NewsGate::Event::BankManagerImpl::disposition_breakdown()",
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
          ostr << "NewsGate::Event::BankManagerImpl::"
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
            "NewsGate::Event::BankManagerImpl::"
            "finalize_disposition_disbandment()",
            Aspect::STATE,
            El::Logging::LOW);
        }
        
        const Server::Config::EventBankManagerType::bank_management_type&
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
          ostr << "NewsGate::Event::BankManagerImpl::"
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
