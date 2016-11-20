/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Feeds/RSS/PullerManager/SessionSupport.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <ace/OS.h>

#include "PullerManagerMain.hpp"
#include "SessionSupport.hpp"

namespace NewsGate
{
  namespace RSS
  {
    //
    // RegisteringPullers class
    //
    
    RegisteringPullers::RegisteringPullers(
      const ACE_Time_Value& puller_registration_timeout,
      PullerManagerStateCallback* callback)
      throw(Exception, El::Exception)
        : PullerManagerState(callback, "RegisteringPullers"),
          sequence_num_(0),
          puller_registration_timeout_(puller_registration_timeout),
          end_registration_time_(ACE_OS::gettimeofday() +
                                 puller_registration_timeout)
    {
      callback->session(Application::instance()->pick_session());

      std::ostringstream ostr;
      ostr << "RegisteringPullers::RegisteringPullers: "
           << callback->session() << " session";
        
      Application::logger()->trace(ostr.str(),
                                   Aspect::STATE,
                                   El::Logging::LOW);

      El::Service::CompoundServiceMessage_var msg =
        new FinalizePullersRegistration(this);
      
      deliver_at_time(msg.in(), end_registration_time_);        
    }
    
    ::NewsGate::RSS::SessionId
    RegisteringPullers::puller_login(
      ::NewsGate::RSS::Puller_ptr puller_object)
      throw(NewsGate::RSS::PullerManager::ImplementationException,
            CORBA::SystemException)
    {
      if(!started())
      {
        return 0;
      }

      unsigned long long session = callback_->session();
      ::NewsGate::RSS::SessionId session_id = 0;
      
      {
        WriteGuard guard(srv_lock_);      

        session_id = (session << 32) | ++sequence_num_;
      }
      
      callback_->register_puller(session_id, puller_object);

      {
        WriteGuard guard(srv_lock_);      

        end_registration_time_ = ACE_OS::gettimeofday() +
          puller_registration_timeout_;
      }
      
      return session_id;
    }

    void
    RegisteringPullers::puller_logout(
      ::NewsGate::RSS::SessionId session)
      throw(NewsGate::RSS::PullerManager::ImplementationException,
            CORBA::SystemException)
    {
      if(!started())
      {
        return;
      }

      callback_->unregister_puller(session);

      {
        WriteGuard guard(srv_lock_);

        end_registration_time_ = ACE_OS::gettimeofday() +
          puller_registration_timeout_;
      }      
    }
    
    CORBA::Boolean
    RegisteringPullers::feed_state(
      ::NewsGate::RSS::SessionId session,
      ::NewsGate::RSS::FeedStateUpdatePack* state_update)
      throw(NewsGate::RSS::PullerManager::Logout,
            NewsGate::RSS::PullerManager::ImplementationException,
            CORBA::SystemException)
    {
      if(!started())
      {
        return 0;
      }
        
      bool wrong_session =
        ((session >> 32) & 0xFFFFFFFF) != callback_->session();
      
      if(wrong_session)
      {
        std::ostringstream ostr;
        ostr << "RegisteringPullers::feed_state: "
          "quite unexpected; relogin requested.";

        El::Service::Error error(ostr.str(),
                                 this,
                                 El::Service::Error::WARNING);

        callback_->notify(&error);

        NewsGate::RSS::PullerManager::Logout ex;
        ex.reason = CORBA::string_dup("Invalid session id");
        throw ex;
      }

      // Suggesting to resend later
      return 0;
    }
    
    void
    RegisteringPullers::ping(
      ::NewsGate::RSS::SessionId session)
      throw(NewsGate::RSS::PullerManager::Logout,
            NewsGate::RSS::PullerManager::ImplementationException,
            CORBA::SystemException)
    {
      if(!started())
      {
        return;
      }
      
      bool wrong_session =
        ((session >> 32) & 0xFFFFFFFF) != callback_->session();
      
      if(wrong_session)
      {
        std::ostringstream ostr;
        ostr << "RegisteringPullers::ping: "
          "quite unexpected; relogin requested.";

        El::Service::Error error(ostr.str(),
                                 this,
                                 El::Service::Error::WARNING);
        
        callback_->notify(&error);

        NewsGate::RSS::PullerManager::Logout ex;
        ex.reason = CORBA::string_dup("Invalid session id");
        throw ex;
      }
    }
    
    void
    RegisteringPullers::finalize_registration() throw()
    {
      if(!started())
      {
        return;
      }

      try
      {
        ACE_Time_Value tm = ACE_OS::gettimeofday();
        ACE_Time_Value end_registration_time;

        PullerSessionVector pullers = callback_->pullers();
        
        bool finalize = false;
        
        {
          WriteGuard guard(srv_lock_);

          if(pullers.begin() == pullers.end())
          {
            end_registration_time_ = tm + puller_registration_timeout_;
          }
            
          end_registration_time = end_registration_time_;
          finalize = tm >= end_registration_time_;
        }
      
        if(finalize) 
        {
          callback_->finalize_puller_registration();
          return;
        }
      
        El::Service::CompoundServiceMessage_var msg =
          new FinalizePullersRegistration(this);
        
        deliver_at_time(msg.in(), end_registration_time);        
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "RegisteringPullers::"
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

    bool
    RegisteringPullers::notify(El::Service::Event* event)
      throw(El::Exception)
    {
      if(PullerManagerState::notify(event))
      {
        return true;
      }
        
      if(dynamic_cast<FinalizePullersRegistration*>(event) != 0)
      {
        finalize_registration();
        return true;
      }

      std::ostringstream ostr;
      ostr << "NewsGate::RSS::RegisteringPullers::notify: unknown "
           << *event;
      
      El::Service::Error error(ostr.str(), this);
      callback_->notify(&error);
      
      return true;
    }

    //
    // EndingFeedingSession class
    //
    
    EndingFeedingSession::EndingFeedingSession(
      const ACE_Time_Value& puller_reset_timeout,
      PullerManagerStateCallback* callback)
      throw(Exception, El::Exception)
        : PullerManagerState(callback, "EndingFeedingSession"),
          puller_reset_timeout_(puller_reset_timeout),
          finalize_session_time_(ACE_OS::gettimeofday() +
                                 puller_reset_timeout)          
    {
      std::ostringstream ostr;
      ostr << "EndingFeedingSession::EndingFeedingSession: "
           << callback->session() << " session";
        
      Application::logger()->trace(ostr.str(),
                                   Aspect::STATE,
                                   El::Logging::LOW);

      El::Service::CompoundServiceMessage_var msg = new FinalizeSession(this);
      deliver_at_time(msg.in(), finalize_session_time_);        
    }
    
    ::NewsGate::RSS::SessionId
    EndingFeedingSession::puller_login(
      ::NewsGate::RSS::Puller_ptr puller_object)
      throw(NewsGate::RSS::PullerManager::ImplementationException,
            CORBA::SystemException)
    {
      return 0;
    }

    void
    EndingFeedingSession::puller_logout(
      ::NewsGate::RSS::SessionId session)
      throw(NewsGate::RSS::PullerManager::ImplementationException,
            CORBA::SystemException)
    {
      WriteGuard guard(srv_lock_);
      finalize_session_time_ = ACE_OS::gettimeofday() + puller_reset_timeout_;
    }
    
    CORBA::Boolean
    EndingFeedingSession::feed_state(
      ::NewsGate::RSS::SessionId session,
      ::NewsGate::RSS::FeedStateUpdatePack* state_update)
      throw(NewsGate::RSS::PullerManager::Logout,
            NewsGate::RSS::PullerManager::ImplementationException,
            CORBA::SystemException)
    {
      if(!started())
      {
        return 0;
      }
        
      {
        WriteGuard guard(srv_lock_);

        finalize_session_time_ =
          ACE_OS::gettimeofday() + puller_reset_timeout_;
      }

      bool wrong_session =
        ((session >> 32) & 0xFFFFFFFF) != callback_->session();
      
      if(wrong_session)
      {
        NewsGate::RSS::PullerManager::Logout ex;
        ex.reason = CORBA::string_dup("Invalid session id");
        throw ex;
      }

      NewsGate::RSS::PullerManager::Logout ex;
      ex.reason = CORBA::string_dup("Session is closing");
      throw ex;
    }

    void
    EndingFeedingSession::ping(
      ::NewsGate::RSS::SessionId session)
      throw(NewsGate::RSS::PullerManager::Logout,
            NewsGate::RSS::PullerManager::ImplementationException,
            CORBA::SystemException)
    {
      if(!started())
      {
        return;
      }
        
      {
        WriteGuard guard(srv_lock_);

        finalize_session_time_ =
          ACE_OS::gettimeofday() + puller_reset_timeout_;
      }

      bool wrong_session =
        ((session >> 32) & 0xFFFFFFFF) != callback_->session();
      
      if(wrong_session)
      {
        NewsGate::RSS::PullerManager::Logout ex;
        ex.reason = CORBA::string_dup("Invalid session id");
        throw ex;
      }

      NewsGate::RSS::PullerManager::Logout ex;
      ex.reason = CORBA::string_dup("Session is closing");
      throw ex;
    }
    
    void
    EndingFeedingSession::finalize_session() throw()
    {
      if(!started())
      {
        return;
      }
      
      try
      {
        ACE_Time_Value tm = ACE_OS::gettimeofday();
        ACE_Time_Value finalize_session_time;
        
        bool finalize = false;
        
        {
          WriteGuard guard(srv_lock_);
            
          finalize_session_time = finalize_session_time_;
          finalize = tm >= finalize_session_time_;
        }
      
        if(finalize) 
        {
          callback_->finalize_pullers_session();
          return;
        }
      
        El::Service::CompoundServiceMessage_var msg =
          new FinalizeSession(this);
        
        deliver_at_time(msg.in(), finalize_session_time);        
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "EndingFeedingSession::"
            "finalize_session: El::Exception caught. "
            "Description:" << std::endl << e;

          El::Service::Error error(ostr.str(), this);          
          callback_->notify(&error);
        }
        catch(...)
        {
        }
      }
    }
      
    bool
    EndingFeedingSession::notify(El::Service::Event* event)
      throw(El::Exception)
    {
      if(PullerManagerState::notify(event))
      {
        return true;
      }
        
      if(dynamic_cast<FinalizeSession*>(event) != 0)
      {
        finalize_session();
        return true;
      }

      std::ostringstream ostr;
      ostr << "NewsGate::RSS::EndingFeedingSession::notify: unknown"
           << *event;
      
      El::Service::Error error(ostr.str(), this);
      callback_->notify(&error);
      
      return true;
    }

  }
}
