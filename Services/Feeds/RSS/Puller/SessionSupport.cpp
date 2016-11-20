/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Services/Feeds/RSS/Puller/SessionSupport.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <El/CORBA/Corba.hpp>

#include "PullerMain.hpp"
#include "SessionSupport.hpp"

namespace NewsGate
{
  namespace RSS
  {
    //
    // LogingIn class
    //

    LogingIn::LogingIn(const ACE_Time_Value& login_retry_period,
                       PullerStateCallback* callback)
      throw(Exception, El::Exception)
        : PullerState(callback, "LogingIn"),
          login_retry_period_(login_retry_period)
    {
      Application::logger()->trace(
        "LogingIn::LogingIn: starting login",
        Aspect::STATE,
        El::Logging::LOW);

      El::Service::CompoundServiceMessage_var msg = new FinalizeLogin(this);
      deliver_now(msg.in());
    }

    void
    LogingIn::logout(::NewsGate::RSS::SessionId session)
      throw(NewsGate::RSS::Puller::ImplementationException,
            CORBA::SystemException)
    {
    }
    
    void
    LogingIn::update_feeds(::NewsGate::RSS::SessionId session,
                           ::NewsGate::RSS::FeedPack* feeds)
      throw(NewsGate::RSS::Puller::NotReady,
            NewsGate::RSS::Puller::ImplementationException,
            CORBA::SystemException)
    {
      NewsGate::RSS::Puller::NotReady ex;
      ex.reason = CORBA::string_dup("Login in progress");
      throw ex;
    }

    void
    LogingIn::finalize_login() throw()
    {
      if(!started())
      {
        return;
      }

      try
      {
        try
        {
          RSS::PullerManager_var puller_manager =
            Application::instance()->puller_manager();

          RSS::Puller_ptr puller_corba_ref =
            Application::instance()->puller_corba_ref();

          if(CORBA::is_nil (puller_corba_ref))
          {
            //
            // Means quite unexpected unrecoverable situation,
            // which requires bugfix.
            // This why assuming CRITICAL
            //
            std::ostringstream ostr;
            ostr << "LogingIn::finalize_login: "
                 << "Application::instance()->puller_corba_ref gives "
              "nil reference";
            
            El::Service::Error error(ostr.str(), this);
            callback_->notify(&error);
          }
            
          RSS::SessionId session = puller_manager->puller_login(
            puller_corba_ref);

          if(session)
          {
            if(Application::will_trace(El::Logging::LOW))
            {
              std::ostringstream ostr;
              ostr << "LogingIn::finalize_login: "
                "completing login; session " << session;
            
              Application::logger()->trace(ostr.str(),
                                           Aspect::STATE,
                                           El::Logging::LOW);
            }
            
            callback_->session(session);
            callback_->login_completed();
            return;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "LogingIn::finalize_login: "
               << "got El::Exception.\n" << e.what();
        
          El::Service::Error error(ostr.str(),
                                   this,
                                   El::Service::Error::ALERT);
          
          callback_->notify(&error);
        }
        catch (const CORBA::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "LogingIn::finalize_login: "
               << "got CORBA::Exception.\n" << e;
        
          El::Service::Error error(ostr.str(),
                                   this,
                                   El::Service::Error::ALERT);
          
          callback_->notify(&error);
        }

        try
        {
          El::Service::CompoundServiceMessage_var msg =
            new FinalizeLogin(this);
          
          deliver_at_time(msg.in(),
                          ACE_OS::gettimeofday() + login_retry_period_);
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "LogingIn::finalize_login: "
            "El::Exception caught while rescheduling the task. Description:"
               << std::endl << e.what();

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        
      }
      catch(...)
      {
        El::Service::Error error(
          "LogingIn::finalize_login: "
          "unknown exception thrown from other exception handler",
          this);
          
        callback_->notify(&error);
        
        return;
      }      
    }

    bool
    LogingIn::notify(El::Service::Event* event) throw(El::Exception)
    {
      if(PullerState::notify(event))
      {
        return true;
      }

      if(dynamic_cast<FinalizeLogin*>(event) != 0)
      {
        finalize_login();
        return true;
      }
      
      std::ostringstream ostr;
      ostr << "NewsGate::RSS::LogingIn::notify: unknown " << *event;
      
      El::Service::Error error(ostr.str(), this);
      callback_->notify(&error);
      
      return true;
    }

    //
    // Logout class
    //

    Logout::Logout(
      PullerStateCallback* callback)
      throw(Exception, El::Exception)
        : PullerState(callback, "Logout")
    {
      Application::logger()->trace(
        "Logout::Logout: loging out",
        Aspect::STATE,
        El::Logging::LOW);

      El::Service::CompoundServiceMessage_var msg = new FinalizeLogout(this);
      deliver_now(msg.in());
    }

    void
    Logout::logout(::NewsGate::RSS::SessionId session)
      throw(NewsGate::RSS::Puller::ImplementationException,
            CORBA::SystemException)
    {
    }
    
    void
    Logout::update_feeds(::NewsGate::RSS::SessionId session,
                         ::NewsGate::RSS::FeedPack* feeds)
      throw(NewsGate::RSS::Puller::NotReady,
            NewsGate::RSS::Puller::ImplementationException,
            CORBA::SystemException)
    {
      NewsGate::RSS::Puller::NotReady ex;
      ex.reason = CORBA::string_dup("Logout in progress");
      throw ex;
    }

    void
    Logout::finalize_logout() throw()
    {
      callback_->logout_completed();
    }

    bool
    Logout::notify(El::Service::Event* event) throw(El::Exception)
    {
      if(PullerState::notify(event))
      {
        return true;
      }

      if(dynamic_cast<FinalizeLogout*>(event) != 0)
      {
        finalize_logout();
        return true;
      }
      
      std::ostringstream ostr;
      ostr << "NewsGate::RSS::LogingIn::notify: unknown " << *event;
      
      El::Service::Error error(ostr.str(), this);
      callback_->notify(&error);
      
      return true;
    }    
  }
}
