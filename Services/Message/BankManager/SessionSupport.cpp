/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Message/BankManager/SessionSupport.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <ace/OS.h>

#include <Services/Commons/Message/MessageBankRecord.hpp>
#include <Services/Commons/Message/BankSessionImpl.hpp>
#include <Services/Commons/Message/BankClientSessionImpl.hpp>

#include "BankManagerMain.hpp"
#include "SessionSupport.hpp"

namespace NewsGate
{
  namespace Message
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
          end_registration_time_(ACE_OS::gettimeofday() +
                                 registration_timeout),
          banks_count_(0),
          continue_session_(true),
          gathering_info_(true)
    {
      El::Service::CompoundServiceMessage_var msg =
        new FinalizeRegistration(this);
      
      deliver_at_time(msg.in(), end_registration_time_);        
    }

    void
    RegisteringBanks::start_gathering_info() throw()
    {
      banks_count_ = 0;
      continue_session_ = true;
      gathering_info_ = true;

      banks_.clear();
      disposition_.clear();

      end_registration_time_ = ACE_OS::gettimeofday() + registration_timeout_;
    }
    
    ::NewsGate::Message::BankSession*
    RegisteringBanks::bank_login(
      const char* bank_ior,
      ::NewsGate::Message::BankSessionId* current_session_id)
      throw(NewsGate::Message::NotReady,
            NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      if(!started())
      {
        NewsGate::Message::NotReady e;
          
        e.reason = CORBA::string_dup(
          "NewsGate::Message::RegisteringBanks::bank_login: "
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
        NewsGate::Message::BankSessionIdImpl* session_id =
          dynamic_cast<NewsGate::Message::BankSessionIdImpl*>(
            current_session_id);
      
        if(session_id == 0)
        {
          NewsGate::Message::ImplementationException e;
        
          e.description = CORBA::string_dup(
            "NewsGate::Message::RegisteringBanks::bank_login: "
            "dynamic cast failed");
        
          throw e;
        }

        WriteGuard guard(srv_lock_);

        if(gathering_info_)
        {
          save_bank_info(bank_ior, session_id);
        }
        else
        {
          return login(bank_ior);
        }

        NewsGate::Message::ImplementationException e;
        
        e.description = CORBA::string_dup(
          "NewsGate::Message::RegisteringBanks::bank_login: "
          "unexpected execution path");
        
        throw e;
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::RegisteringBanks::"
            "bank_login: El::Exception caught. "
            "Description:" << std::endl << e;

          NewsGate::Message::ImplementationException e;
          e.description = CORBA::string_dup(ostr.str().c_str());
          throw e;
        }
        catch(const CORBA::Exception& )
        {
          throw;
        }
        catch(...)
        {
          throw NewsGate::Message::NotReady();
        }
      } 
      
    }

    void
    RegisteringBanks::ping(
      const char* bank_ior,
      ::NewsGate::Message::BankSessionId* current_session_id)
      throw(NewsGate::Message::Logout,
            NewsGate::Message::ImplementationException,
            CORBA::SystemException)
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
      
          NewsGate::Message::Logout e;
          
          e.reason =
            CORBA::string_dup("NewsGate::Message::RegisteringBanks::ping: "
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
          NewsGate::Message::ImplementationException e;
        
          e.description = CORBA::string_dup(
            "NewsGate::Message::RegisteringBanks::ping: "
            "dynamic_cast failed");

          throw e;
        }
        
        if(session_id->index >= disposition_.size() ||
           session_id->banks_count != disposition_.size() ||
           disposition_[session_id->index].bank.reference() != bank_ior)
        {
          start_gathering_info();

          NewsGate::Message::Logout e;
      
          e.reason = CORBA::string_dup("NewsGate::Message::"
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
          ostr << "NewsGate::Message::RegisteringBanks::ping: bank "
               << bank_ior << " - no action";
          
          Application::logger()->trace(ostr.str(),
                                       Aspect::STATE,
                                       El::Logging::HIGH);
        }
      }
      
    }

    void
    RegisteringBanks::terminate(
      const char* bank_ior,
      ::NewsGate::Message::BankSessionId* current_session_id)
      throw(NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
    }
      
    ::NewsGate::Message::BankClientSession*
    RegisteringBanks::bank_client_session()
      throw(NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      BankManagerApp* app = Application::instance();

      const Server::Config::MessageBankManagerType::
        bank_client_management_type&
        conf = app->config().bank_client_management();
        
      return BankClientSessionImpl_init::create(
        app->orb(),
        app->bank_manager_corba_ref(),
        callback_->get_process_id(),
        ACE_Time_Value(conf.session_refresh_period()),
        ACE_Time_Value(conf.bank_invalidate_timeout()),
        conf.message_post_retries(),
        conf.request_threads());
    }
    
    void
    RegisteringBanks::save_bank_info(
      const char* bank_ior,
      ::NewsGate::Message::BankSessionIdImpl* session_id)
      throw(NewsGate::Message::NotReady,
            NewsGate::Message::ImplementationException,
            CORBA::SystemException,
            El::Exception)
    {
      if(!banks_count_)
      {
        banks_count_ = session_id->banks_count;
      }

      if(banks_count_ && session_id->banks_count &&
         banks_count_ != session_id->banks_count)
      {
        continue_session_ = false;
      }
      
      BankMap::const_iterator it = banks_.find(bank_ior);

      if(it == banks_.end())
      {
        session_id->_add_ref();
        banks_[bank_ior] = session_id;

        end_registration_time_ =
          ACE_OS::gettimeofday() + registration_timeout_;
      }
      else if(!it->second->is_equal(session_id))
      {
        continue_session_ = false;        
      }

      NewsGate::Message::NotReady e;
          
      e.reason = CORBA::string_dup(
        "NewsGate::Message::RegisteringBanks::save_bank_info: "
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
    
    ::NewsGate::Message::BankSession*
    RegisteringBanks::login(
      const char* bank_ior)
      throw(NewsGate::Message::NotReady,
            NewsGate::Message::ImplementationException,
            CORBA::SystemException,
            El::Exception)
    {
      size_t i = 0;
      
      for(BankDisposition::const_iterator it = disposition_.begin();
          it != disposition_.end(); it++, i++)
      {
        if(it->bank.reference() == bank_ior)
        {
          if(Application::will_trace(El::Logging::MIDDLE))
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Message::RegisteringBanks::login: bank "
                 << bank_ior << " is logged in";

            Application::logger()->trace(ostr.str(),
                                         Aspect::STATE,
                                         El::Logging::MIDDLE);
          }
          
          end_registration_time_ =
            ACE_OS::gettimeofday() + registration_timeout_;

          std::string mirror =
            Application::instance()->config().message_sharing().mirror();
            
          NewsGate::Message::BankSessionImpl_var session =
            new NewsGate::Message::BankSessionImpl(
              new NewsGate::Message::BankSessionIdImpl(i, banks_count_),
              callback_->get_process_id(),
              mirror == "absolute");

          return session._retn();
        }
      }

      start_gathering_info();

      NewsGate::Message::NotReady e;
          
      e.reason = CORBA::string_dup(
        "NewsGate::Message::RegisteringBanks::login: "
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
    RegisteringBanks::finalize_registration() throw()
    {
      if(!started())
      {
        return;
      }

      try
      {
        ACE_Time_Value tm = ACE_OS::gettimeofday();
        ACE_Time_Value end_registration_time;

        bool finalize = false;
        BankDisposition disposition;
        
        {
          WriteGuard guard(srv_lock_);

          if(banks_.begin() == banks_.end())
          {
            end_registration_time_ = tm + registration_timeout_;
          }
            
          end_registration_time = end_registration_time_;
          finalize = tm >= end_registration_time_;

          if(finalize)
          {
            if(gathering_info_)
            {
              create_disposition();
              
              finalize = false;
              gathering_info_ = false;
              end_registration_time_ = tm + registration_timeout_;
            }
            else
            {
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
          callback_->finalize_bank_registration(disposition);
          return;
        }
      
        El::Service::CompoundServiceMessage_var msg =
          new FinalizeRegistration(this);
        
        deliver_at_time(msg.in(), end_registration_time);        
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::RegisteringBanks::"
            "finalize_registration: El::Exception caught. "
            "Description:" << std::endl << e;

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(...)
        {
        }
      } 
    }

    void
    RegisteringBanks::create_disposition() throw(Exception, El::Exception)
    {
      El::MySQL::Connection_var connection =
        Application::instance()->dbase()->connect();

      if(!banks_count_)
      {
        continue_session_ = false;
      }
      
      if(continue_session_)
      {
        //
        // Trying to continue existing session
        //
        disposition_.resize(banks_count_);

        //
        // Assigning existing banks to their existing places
        //
        for(BankMap::const_iterator it = banks_.begin();
            it != banks_.end() && continue_session_; ++it)
        {
          if(it->second->banks_count)
          {
            size_t index = it->second->index;
                  
            if(!disposition_[index].bank.empty())
            {
              continue_session_ = false;
            }
            else
            {
              disposition_[index].bank =
                BankClientSessionImpl::BankRef(it->first.c_str(),
                                               Application::instance()->orb());
            }
          }          
        }

        //
        // Assigning new banks to free places
        // (can happen when replace dead old host with new one)
        //
        for(BankMap::const_iterator it = banks_.begin();
            it != banks_.end() && continue_session_; ++it)
        {
          if(!it->second->banks_count)
          {
            BankDisposition::iterator dit = disposition_.begin();
            
            for(; dit != disposition_.end() && !dit->bank.empty(); dit++);

            if(dit == disposition_.end())
            {
              continue_session_ = false;
            }
            else
            {
              dit->bank = BankClientSessionImpl::BankRef(
                it->first.c_str(),
                Application::instance()->orb());
            }
          }          
        }
        
        if(continue_session_)
        {
          std::ostringstream ostr;
          ostr << "select last_check_banks_presence_time from "
            "MessageBankManagerState where bank_manager_id='"
               << connection->escape(
                 Application::instance()->bank_manager_id()) << "'";

          El::MySQL::Result_var result = connection->query(ostr.str().c_str());

          ACE_Time_Value last_check_banks_presence_time;
          MessageBankManagerStateInfo bank_manager_state_info(result.in());
            
          if(bank_manager_state_info.fetch_row() &&
             !bank_manager_state_info.last_check_banks_presence_time().
             is_null())
          {
            last_check_banks_presence_time =
              El::Moment(bank_manager_state_info.
                         last_check_banks_presence_time());
          }

          ACE_Time_Value current_time = ACE_OS::gettimeofday();
            
          BankRecordMap bank_record_map;
          ACE_Time_Value time_shift;

          if(last_check_banks_presence_time != ACE_Time_Value::zero)
          {
            time_shift = current_time - last_check_banks_presence_time;

            result =
              connection->query("select bank_ior, session_id, "
                                "last_ping_time from MessageBankSession");

            BankSessionInfo bank_session(result.in());

            BankSessionIdImpl_var session_id = new BankSessionIdImpl();
            
            while(bank_session.fetch_row())
            {
              if(!bank_session.session_id().is_null() &&
                 !bank_session.last_ping_time().is_null())
              {
                session_id->from_string(bank_session.session_id().c_str());    

                BankRecord br;

                br.timestamp = El::Moment(bank_session.last_ping_time());

                br.bank_ior = bank_session.bank_ior();
                br.banks_count = session_id->banks_count;

                BankRecordMap::iterator it =
                  bank_record_map.find(session_id->index);

                if(it == bank_record_map.end() ||
                   it->second.timestamp < br.timestamp)
                {
                  bank_record_map[session_id->index] = br;
                }
              }
            }
          }

          size_t i = 0;
          
          for(BankDisposition::iterator it = disposition_.begin();
              it != disposition_.end() && continue_session_; it++, i++)
          {
            if(it->bank.empty())
            {
              //
              // look into DB if it where some bank taking this
              // place in disposition. If it where and last ping time still
              // fresh, then assign this place for this bank, set
              // last ping time from DB and continue the session.
              //

              BankRecordMap::iterator br_it = bank_record_map.find(i);

              if(br_it != bank_record_map.end() &&
                 br_it->second.banks_count == banks_count_)
              {
                ACE_Time_Value tm = br_it->second.timestamp + time_shift;
                  
                if(tm + presence_poll_timeout_ < current_time)
                {
                  continue_session_ = false;
                }
                else
                {
                  it->bank = BankClientSessionImpl::BankRef(
                    br_it->second.bank_ior.c_str(),
                    Application::instance()->orb());
                    
                  it->timestamp = tm;
                }
              }
              else
              {
                continue_session_ = false;
              }
                
            }
          }
        }
        else
        {
          continue_session_ = false;        
        }
      }

      if(Application::will_trace(El::Logging::MIDDLE))
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::RegisteringBanks::create_disposition: "
          "creating " << (continue_session_ ? "same" : "new")
             << " disposition";
        
        Application::logger()->trace(ostr.str(),
                                     Aspect::STATE,
                                     El::Logging::MIDDLE);
      }
              
      if(continue_session_)
      {
        return;
      }

      El::MySQL::Result_var result =
        connection->query("delete from MessageBankSession");
        
      disposition_.resize(0);
      banks_count_ = 0;
      
      for(BankMap::const_iterator it = banks_.begin();
          it != banks_.end(); ++it, banks_count_++)
      {
        disposition_.push_back(
          BankInfo(BankClientSessionImpl::BankRef(
                     it->first.c_str(), Application::instance()->orb())));
      }
      
    }

    char*
    RegisteringBanks::message_sharing_register(
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
      NewsGate::Message::NotReady e;
      
      e.reason = CORBA::string_dup(
        "NewsGate::Message::RegisteringBanks::message_sharing_register: "
        "still registering banks ...");

      throw e;
    }

    void
    RegisteringBanks::message_sharing_unregister(
      const char* manager_persistent_id)
      throw(NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
    }
      
    void
    RegisteringBanks::set_message_fetch_filter(
      ::NewsGate::Message::Transport::SetMessageFetchFilterRequest* filter,
      ::CORBA::Boolean shared,
        const char* validation_id)
      throw(NewsGate::Message::ImplementationException,
            ::CORBA::SystemException)
    {
    }
    
    void
    RegisteringBanks::set_message_categorizer(
      ::NewsGate::Message::Transport::SetMessageCategorizerRequest*
      categorizer,
      const char* validation_id)
      throw(NewsGate::Message::ImplementationException,
            ::CORBA::SystemException)
    {
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
      ostr << "NewsGate::Message::RegisteringBanks::notify: unknown "
           << *event;
      
      El::Service::Error error(ostr.str(), this);
      callback_->notify(&error);
      
      return true;
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

    ::NewsGate::Message::BankSession*
    DispositionDisbandment::bank_login(
      const char* bank_ior,
      ::NewsGate::Message::BankSessionId* current_session_id)
      throw(NewsGate::Message::NotReady,
            NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      NewsGate::Message::NotReady e;
      
      e.reason = CORBA::string_dup(
        "NewsGate::Message::DispositionDisbandment::bank_login: "
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
      ::NewsGate::Message::BankSessionId* current_session_id)
      throw(NewsGate::Message::Logout,
            NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      if(!started())
      {
        return;
      }
      
      {
        WriteGuard guard(srv_lock_);
        end_disbandment_time_ = ACE_OS::gettimeofday() + reset_timeout_;
      }
      
      NewsGate::Message::Logout e;
      
      e.reason =
        CORBA::string_dup("NewsGate::Message::DispositionDisbandment::ping: "
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
      
    void
    DispositionDisbandment::terminate(
      const char* bank_ior,
      ::NewsGate::Message::BankSessionId* current_session_id)
      throw(NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
    }
      
    ::NewsGate::Message::BankClientSession*
    DispositionDisbandment::bank_client_session()
      throw(NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      BankManagerApp* app = Application::instance();
      
      const Server::Config::MessageBankManagerType::
        bank_client_management_type&
        conf = app->config().bank_client_management();

      return BankClientSessionImpl_init::create(
        app->orb(),
        app->bank_manager_corba_ref(),
        callback_->get_process_id(),
        ACE_Time_Value(conf.session_refresh_period()),
        ACE_Time_Value(conf.bank_invalidate_timeout()),
        conf.message_post_retries(),
        conf.request_threads());
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
          ostr << "NewsGate::Message::DispositionDisbandment::"
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
    
    char*
    DispositionDisbandment::message_sharing_register(
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
      NewsGate::Message::NotReady e;

      e.reason = CORBA::string_dup(
        "NewsGate::Message::DispositionDisbandment::message_sharing_register: "
        "unregistering banks ...");

      throw e;
    }

    void
    DispositionDisbandment::message_sharing_unregister(
      const char* manager_persistent_id)
      throw(NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
    }
      
    void
    DispositionDisbandment::set_message_fetch_filter(
      ::NewsGate::Message::Transport::SetMessageFetchFilterRequest* filter,
      ::CORBA::Boolean shared,
      const char* validation_id)
      throw(NewsGate::Message::ImplementationException,
            ::CORBA::SystemException)
    {
    }
    
    void
    DispositionDisbandment::set_message_categorizer(
      ::NewsGate::Message::Transport::SetMessageCategorizerRequest*
      categorizer,
      const char* validation_id)
      throw(NewsGate::Message::ImplementationException,
            ::CORBA::SystemException)
    {
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
      ostr << "NewsGate::Message::DispositionDisbandment::notify: unknown "
           << *event;
      
      El::Service::Error error(ostr.str(), this);
      callback_->notify(&error);
      
      return true;
    }

  }
}
