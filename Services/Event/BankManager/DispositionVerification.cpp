
/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Event/BankManager/DispositionVerification.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/Logging/Logger.hpp>
#include <El/Service/CompoundService.hpp>
#include <El/Service/Service.hpp>

#include <Services/Commons/Event/BankSessionImpl.hpp>
#include <Services/Commons/Event/BankClientSessionImpl.hpp>

#include "BankManagerMain.hpp"
#include "SessionSupport.hpp"
#include "DispositionVerification.hpp"

namespace NewsGate
{
  namespace Event
  {
    //
    // DispositionVerification class
    //
    DispositionVerification::DispositionVerification(
      const BankDisposition& disposition,
      const char* session_guid,
      const ACE_Time_Value& presence_poll_timeout,
      const ACE_Time_Value& presence_check_period,
      BankManagerStateCallback* callback)
      throw(Exception, El::Exception)
        : BankManagerState(callback, "DispositionVerification"),
          disposition_(disposition),
          session_guid_(session_guid),
          presence_poll_timeout_(presence_poll_timeout),
          presence_check_period_(presence_check_period)
    {
      El::Service::CompoundServiceMessage_var msg =
        new CheckBanksPresence(this);
      
      deliver_at_time(msg.in(),
                      ACE_OS::gettimeofday() + presence_check_period_);
    }
    
    DispositionVerification::~DispositionVerification() throw()
    {
    }

    ::NewsGate::Event::BankSession*
    DispositionVerification::bank_login(const char* bank_ior)
      throw(NewsGate::Event::NotReady,
            NewsGate::Event::ImplementationException,
            ::CORBA::SystemException)
    {
      if(started())
      {
        callback_->disposition_breakdown();
      }

      NewsGate::Event::NotReady e;

      e.reason = CORBA::string_dup(
        "NewsGate::Event::DispositionVerification::bank_login: "
        "quite unexpected");

      if(Application::will_trace(El::Logging::MIDDLE))
      {
        std::ostringstream ostr;
        ostr << e.reason.in() << "; bank " << bank_ior;
        
        Application::logger()->trace(ostr.str(),
                                     Aspect::STATE,
                                     El::Logging::MIDDLE);
      }
              
      throw e;
    }

    void
    DispositionVerification::ping(
      const char* bank_ior,
      ::NewsGate::Event::BankSessionId* current_session_id)
      throw(NewsGate::Event::Logout,
            NewsGate::Event::ImplementationException,
            ::CORBA::SystemException)
    {
      if(!started())
      {
        return;
      }

      BankSessionIdImpl* session_id =
        dynamic_cast<BankSessionIdImpl*>(current_session_id);

      if(session_id == 0)
      {
        NewsGate::Event::ImplementationException e;
        
        e.description = CORBA::string_dup(
          "NewsGate::Event::DispositionVerification::ping: "
          "dynamic_cast failed");

        throw e;
      }
      
      {
        WriteGuard guard(srv_lock_);

        if(session_id->guid != session_guid_ ||
           session_id->index >= disposition_.size() ||
           session_id->banks_count != disposition_.size() ||
           disposition_[session_id->index].bank.reference() != bank_ior)
        {
          guard.release();
          
          NewsGate::Event::Logout e;
      
          e.reason = CORBA::string_dup("NewsGate::Event::"
                                       "DispositionVerification::ping: "
                                       "wrong session id");
              
          if(Application::will_trace(El::Logging::MIDDLE))
          {
            std::ostringstream ostr;
            ostr << e.reason.in() << "; bank " << bank_ior;
        
            Application::logger()->trace(ostr.str(),
                                         Aspect::STATE,
                                         El::Logging::MIDDLE);
          }
          
          callback_->disposition_breakdown();

          throw e;
        }

        disposition_[session_id->index].timestamp = ACE_OS::gettimeofday();

        if(Application::will_trace(El::Logging::HIGH))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::DispositionVerification::ping: bank "
               << bank_ior << " - timestamp updated";
          
          Application::logger()->trace(ostr.str(),
                                       Aspect::STATE,
                                       El::Logging::HIGH);
        }
      }
    }

    ::NewsGate::Event::BankClientSession*
    DispositionVerification::bank_client_session()
      throw(NewsGate::Event::ImplementationException,
            ::CORBA::SystemException)
    {
      try
      {
        BankManagerApp* app = Application::instance();
      
        const Server::Config::EventBankManagerType::
          bank_client_management_type&
          conf = app->config().bank_client_management();

        BankClientSessionImpl_var session = BankClientSessionImpl_init::create(
          app->orb(),
          app->bank_manager_corba_ref(),
          ACE_Time_Value(conf.session_refresh_period()),
          ACE_Time_Value(conf.bank_invalidate_timeout()),
          conf.request_threads());

        BankClientSessionImpl::BankRecordArray& banks = session->banks();
      
        banks.resize(disposition_.size());

        size_t i = 0;
        
        for(BankDisposition::const_iterator it = disposition_.begin();
            it != disposition_.end(); it++, i++)
        {
          banks[i].bank = it->bank;}
      
        return session._retn();
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Event::DispositionVerification::"
          "bank_client_session: El::Exception caught. Description:\n"
             << e.what();

        NewsGate::Event::ImplementationException ex;
        ex.description = CORBA::string_dup(ostr.str().c_str());
        throw ex;
      }      
    }

    bool
    DispositionVerification::notify(El::Service::Event* event)
      throw(El::Exception)
    {
      if(BankManagerState::notify(event))
      {
        return true;
      }

      if(dynamic_cast<CheckBanksPresence*>(event) != 0)
      {
        check_banks_presence();
        return true;
      }

      std::ostringstream ostr;
      ostr << "NewsGate::RSS::DispositionVerification::notify: unknown "
           << *event;
      
      El::Service::Error error(ostr.str(), this);
      callback_->notify(&error);
      
      return true;
    }
    
    void
    DispositionVerification::check_banks_presence() throw()
    {
      if(!started())
      {
        return;
      }

      try
      {
        if(Application::will_trace(El::Logging::MIDDLE))
        {
          Application::logger()->trace(
            "NewsGate::Event::DispositionVerification::"
            "check_banks_presence: checking presence",
            Aspect::STATE,
            El::Logging::MIDDLE);
        }

        bool some_bank_dissapeared = false;
        ACE_Time_Value tm = ACE_OS::gettimeofday();
        
        {
          WriteGuard guard(srv_lock_);

          for(BankDisposition::const_iterator it = disposition_.begin();
              it != disposition_.end(); it++)
          {
            if(it->timestamp + presence_poll_timeout_ < tm)
            {
              if(Application::will_trace(El::Logging::MIDDLE))
              {
                std::ostringstream ostr;
                ostr << "NewsGate::Event::DispositionVerification::"
                  "check_banks_presence: bank " << it->bank.reference()
                     << " seems to dissapear";
                
                Application::logger()->trace(ostr.str(),
                                             Aspect::STATE,
                                             El::Logging::MIDDLE);
              }

              some_bank_dissapeared = true;
            }
          }
        }
        
        if(some_bank_dissapeared)
        {
          callback_->disposition_breakdown();
        }
        else
        {
          El::Service::CompoundServiceMessage_var msg =
            new CheckBanksPresence(this);
          
          deliver_at_time(msg.in(), tm + presence_check_period_);
        }
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "DispositionVerification::check_banks_presence: "
            "El::Exception caught. Description:" << std::endl << e;

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(...)
        {
          El::Service::Error error(
            "DispositionVerification::check_banks_presence: unknown caught.",
            this);
          
          callback_->notify(&error);
        }
      } 
    }
    
  }
}
