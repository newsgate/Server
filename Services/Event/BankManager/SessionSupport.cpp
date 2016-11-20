/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Event/BankManager/SessionSupport.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/Logging/Logger.hpp>
#include <El/Service/CompoundService.hpp>
#include <El/Guid.hpp>

#include <Services/Commons/Event/BankSessionImpl.hpp>
#include <Services/Commons/Event/BankClientSessionImpl.hpp>

#include "BankManagerMain.hpp"
#include "SessionSupport.hpp"

namespace NewsGate
{
  namespace Event
  {
    //
    // RegisteringBanks class
    //
    RegisteringBanks::RegisteringBanks(
      const ACE_Time_Value& presence_poll_timeout,
      const ACE_Time_Value& registration_timeout,
      BankManagerStateCallback* callback)
      throw(Exception, El::Exception)
        : BankManagerState(callback, "RegisteringBanks"),
          presence_poll_timeout_(presence_poll_timeout),
          registration_timeout_(registration_timeout),
          gathering_info_(true)
    {
      start_gathering_info();
      
      El::Service::CompoundServiceMessage_var msg =
        new FinalizeRegistration(this);
      
      deliver_at_time(msg.in(), end_registration_time_);        
    }

    void
    RegisteringBanks::start_gathering_info() throw()
    {
      El::Guid guid;
      guid.generate();
      
      session_guid_ = guid.string(El::Guid::GF_DENSE);
      gathering_info_ = true;

      banks_.clear();
      disposition_.clear();

      end_registration_time_ = ACE_OS::gettimeofday() + registration_timeout_;
    }

    ::NewsGate::Event::BankSession*
    RegisteringBanks::bank_login(const char* bank_ior)
      throw(NewsGate::Event::NotReady,
            NewsGate::Event::ImplementationException,
            ::CORBA::SystemException)
    {
      if(!started())
      {
        NewsGate::Event::NotReady e;
          
        e.reason = CORBA::string_dup(
          "NewsGate::Event::RegisteringBanks::bank_login: "
          "registering is just completed ...");

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

      try
      {
        WriteGuard guard(srv_lock_);

        if(gathering_info_)
        {
          save_bank_info(bank_ior);
        }
        else
        {
          return login(bank_ior);
        }

        NewsGate::Event::ImplementationException e;
        
        e.description = CORBA::string_dup(
          "NewsGate::Event::RegisteringBanks::bank_login: "
          "unexpected execution path");
        
        throw e;
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::RegisteringBanks::"
            "bank_login: El::Exception caught. "
            "Description:" << std::endl << e;

          NewsGate::Event::ImplementationException e;
          e.description = CORBA::string_dup(ostr.str().c_str());
          throw e;
        }
        catch(const CORBA::Exception& )
        {
          throw;
        }
        catch(...)
        {
          throw NewsGate::Event::NotReady();
        }
      }

    }

    void
    RegisteringBanks::save_bank_info(const char* bank_ior)
      throw(NewsGate::Event::NotReady,
            NewsGate::Event::ImplementationException,
            CORBA::SystemException,
            El::Exception)
    {
      BankSet::const_iterator it = banks_.find(bank_ior);

      if(it == banks_.end())
      {
        banks_.insert(bank_ior);

        end_registration_time_ =
          ACE_OS::gettimeofday() + registration_timeout_;
      }

      NewsGate::Event::NotReady e;
          
      e.reason = CORBA::string_dup(
        "NewsGate::Event::RegisteringBanks::save_bank_info: "
        "still waiting for others ...");
          
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
    
    ::NewsGate::Event::BankSession*
    RegisteringBanks::login(const char* bank_ior)
      throw(NewsGate::Event::NotReady,
            NewsGate::Event::ImplementationException,
            CORBA::SystemException,
            El::Exception)
    {
      size_t i = 0;
      BankDisposition::const_iterator prev = disposition_.end();
        
      for(BankDisposition::const_iterator it = disposition_.begin();
          it != disposition_.end(); it++, i++)
      {
        if(it->bank.reference() != bank_ior)
        {
          prev = it;
          continue;
        }
        
        if(Application::will_trace(El::Logging::MIDDLE))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::RegisteringBanks::login: bank "
               << bank_ior << " is logged in";

          Application::logger()->trace(ostr.str(),
                                       Aspect::STATE,
                                       El::Logging::MIDDLE);
        }
          
        end_registration_time_ =
          ACE_OS::gettimeofday() + registration_timeout_;

        NewsGate::Event::Bank_var left_neighbour;
        NewsGate::Event::Bank_var right_neighbour;

        if(disposition_.size() > 1)
        {
          BankDisposition::const_iterator next = it + 1;

          if(next == disposition_.end())
          {
            next = disposition_.begin();
          }

          right_neighbour = next->bank.object();

          left_neighbour = prev == disposition_.end() ?
            disposition_.rbegin()->bank.object() : prev->bank.object();
        }

        NewsGate::Event::BankSessionImpl_var session =
          new NewsGate::Event::BankSessionImpl(
            new NewsGate::Event::BankSessionIdImpl(session_guid_.c_str(),
                                                   i,
                                                   disposition_.size()),
            left_neighbour.in(),
            right_neighbour.in(),
            disposition_.size());

        return session._retn();
      }

      start_gathering_info();

      NewsGate::Event::NotReady e;
          
      e.reason = CORBA::string_dup(
        "NewsGate::Event::RegisteringBanks::login: "
        "some new bank appeared; reiterating ...");

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
    RegisteringBanks::ping(
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

      {
        WriteGuard guard(srv_lock_);

        if(gathering_info_)
        {
          end_registration_time_ =
            ACE_OS::gettimeofday() + registration_timeout_;
      
          NewsGate::Event::Logout e;
          
          e.reason =
            CORBA::string_dup("NewsGate::Event::RegisteringBanks::ping: "
                              "creating new disposition");

          if(Application::will_trace(El::Logging::MIDDLE))
          {
            std::ostringstream ostr;
            ostr << e.reason.in() << "; bank " << bank_ior;
            
            Application::logger()->trace(ostr.str().c_str(),
                                         Aspect::STATE,
                                         El::Logging::MIDDLE);
          }
          
          throw e;
        }
        
        BankSessionIdImpl* session_id =
          dynamic_cast<BankSessionIdImpl*>(current_session_id);

        if(session_id == 0)
        {
          NewsGate::Event::ImplementationException e;
        
          e.description = CORBA::string_dup(
            "NewsGate::Event::RegisteringBanks::ping: "
            "dynamic_cast failed");

          throw e;
        }
        
        if(session_id->guid != session_guid_ ||
           session_id->index >= disposition_.size() ||
           session_id->banks_count != disposition_.size() ||
           disposition_[session_id->index].bank.reference() != bank_ior)
        {
          start_gathering_info();

          NewsGate::Event::Logout e;
      
          e.reason = CORBA::string_dup("NewsGate::Event::"
                                       "RegisteringBanks::ping: "
                                       "wrong session id");
              
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

        if(Application::will_trace(El::Logging::HIGH))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::RegisteringBanks::ping: bank "
               << bank_ior << " - no action";
          
          Application::logger()->trace(ostr.str(),
                                       Aspect::STATE,
                                       El::Logging::HIGH);
        }
      }
      
    }

    ::NewsGate::Event::BankClientSession*
    RegisteringBanks::bank_client_session()
      throw(NewsGate::Event::ImplementationException,
            ::CORBA::SystemException)
    {
      BankManagerApp* app = Application::instance();
      
      const Server::Config::EventBankManagerType::
        bank_client_management_type&
        conf = app->config().bank_client_management();

      return BankClientSessionImpl_init::create(
        app->orb(),
        app->bank_manager_corba_ref(),
        ACE_Time_Value(conf.session_refresh_period()),
        ACE_Time_Value(conf.bank_invalidate_timeout()),
        conf.request_threads());
    }

    bool
    RegisteringBanks::notify(El::Service::Event* event) throw(El::Exception)
    {
      if(BankManagerState::notify(event))
      {
        return true;
      }

      if(dynamic_cast<FinalizeRegistration*>(event) != 0)
      {
        finalize_registration();
        return true;
      }
      
      std::ostringstream ostr;
      ostr << "NewsGate::Event::RegisteringBanks::notify: unknown " << *event;
      
      El::Service::Error error(ostr.str(), this);
      callback_->notify(&error);
      
      return true;
    }

    void
    RegisteringBanks::finalize_registration() throw()
    {
      if(!started())
      {
        return;
      }

      ACE_Time_Value tm = ACE_OS::gettimeofday();
      
      try
      {
        bool finalize = false;
        BankDisposition disposition;
        
        {
          WriteGuard guard(srv_lock_);

          if(banks_.begin() == banks_.end())
          {
            end_registration_time_ = tm + registration_timeout_;
          }
            
          if(tm >= end_registration_time_)
          {
            if(gathering_info_)
            {
              create_disposition();
              
              gathering_info_ = false;
              end_registration_time_ = tm + registration_timeout_;
            }
            else
            {
              finalize = true;
              disposition = disposition_;
              
              for(BankDisposition::iterator it = disposition.begin();
                  it != disposition.end(); it++)
              {
                if(it->timestamp == ACE_Time_Value::zero)
                {
                  it->timestamp = tm;
                }
              }
              
            }
          } 
        }
      
        if(finalize) 
        {
          callback_->finalize_bank_registration(disposition,
                                                session_guid_.c_str());
          return;
        }      
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::RegisteringBanks::"
            "finalize_registration: El::Exception caught. "
            "Description:" << std::endl << e;

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);

          WriteGuard guard(srv_lock_);
          start_gathering_info();
        }
        catch(...)
        {
        }
      }

      El::Service::CompoundServiceMessage_var msg =
        new FinalizeRegistration(this);
      
      ReadGuard guard(srv_lock_);
      deliver_at_time(msg.in(), end_registration_time_);
    }

    void
    RegisteringBanks::create_disposition() throw(Exception, El::Exception)
    {
      disposition_.clear();
      disposition_.reserve(banks_.size());

      for(BankSet::const_iterator it = banks_.begin();
          it != banks_.end(); ++it)
      {
        try
        {
          disposition_.push_back(
            BankInfo(BankClientSessionImpl::BankRef(
                       it->c_str(),
                       Application::instance()->orb())));
        }
        catch (const CORBA::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "RegisteringBanks::create_disposition: "
            "CORBA::Exception caught while creating reference to "
               << *it << "Description: \n" << e;

          throw Exception(ostr.str().c_str());
        }
      }
    }

    //
    // DispositionDisbandment class
    //
    DispositionDisbandment::DispositionDisbandment(
      const ACE_Time_Value& reset_timeout,
      BankManagerStateCallback* callback)
      throw(Exception, El::Exception)
        : BankManagerState(callback, "DispositionDisbandment"),
          reset_timeout_(reset_timeout),
          end_disbandment_time_(ACE_OS::gettimeofday() +
                                reset_timeout)
    {
      El::Service::CompoundServiceMessage_var msg =
        new FinalizeDisbandment(this);
      
      deliver_at_time(msg.in(), end_disbandment_time_);        
    }
    
    DispositionDisbandment::~DispositionDisbandment() throw()
    {
    }

    ::NewsGate::Event::BankSession*
    DispositionDisbandment::bank_login(const char* bank_ior)
      throw(NewsGate::Event::NotReady,
            NewsGate::Event::ImplementationException,
            ::CORBA::SystemException)
    {
      NewsGate::Event::NotReady e;
      
      e.reason = CORBA::string_dup(
        "NewsGate::Event::DispositionDisbandment::bank_login: "
        "disbanding current disposition");

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
    DispositionDisbandment::ping(
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
      
      {
        WriteGuard guard(srv_lock_);
        end_disbandment_time_ = ACE_OS::gettimeofday() + reset_timeout_;
      }
      
      NewsGate::Event::Logout e;
      
      e.reason =
        CORBA::string_dup("NewsGate::Event::DispositionDisbandment::ping: "
                          "creating new disposition");

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

    ::NewsGate::Event::BankClientSession*
    DispositionDisbandment::bank_client_session()
      throw(NewsGate::Event::ImplementationException,
            ::CORBA::SystemException)
    {
      BankManagerApp* app = Application::instance();
      
      const Server::Config::EventBankManagerType::
        bank_client_management_type&
        conf = app->config().bank_client_management();

      return BankClientSessionImpl_init::create(
        app->orb(),
        app->bank_manager_corba_ref(),
        ACE_Time_Value(conf.session_refresh_period()),
        ACE_Time_Value(conf.bank_invalidate_timeout()),
        conf.request_threads());
    }

    bool
    DispositionDisbandment::notify(El::Service::Event* event)
      throw(El::Exception)
    {
      if(BankManagerState::notify(event))
      {
        return true;
      }

      if(dynamic_cast<FinalizeDisbandment*>(event) != 0)
      {
        finalize_disbandment();
        return true;
      }
      
      std::ostringstream ostr;
      ostr << "NewsGate::Event::DispositionDisbandment::notify: unknown "
           << *event;
      
      El::Service::Error error(ostr.str(), this);
      callback_->notify(&error);
      
      return true;
    }

    void
    DispositionDisbandment::finalize_disbandment() throw()
    {
      if(!started())
      {
        return;
      }

      try
      {
        ACE_Time_Value tm = ACE_OS::gettimeofday();
        ACE_Time_Value end_disbandment_time;

        bool finalize = false;
        
        {
          WriteGuard guard(srv_lock_);

          end_disbandment_time = end_disbandment_time_;
          finalize = tm >= end_disbandment_time_;
        }
      
        if(finalize) 
        {
          callback_->finalize_disposition_disbandment();
          return;
        }
      
        El::Service::CompoundServiceMessage_var msg = new
          FinalizeDisbandment(this);
        
        deliver_at_time(msg.in(), end_disbandment_time);        
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::DispositionDisbandment::"
            "finalize_disbandment: El::Exception caught. "
            "Description:" << std::endl << e;

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
