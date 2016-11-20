/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Services/Feeds/RSS/Puller/PullerImpl.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <Python.h>

#include <El/CORBA/Corba.hpp>

#include <string>
#include <sstream>

#include <ace/OS.h>
#include <ace/High_Res_Timer.h>

#include "PullerImpl.hpp"
#include "PullerMain.hpp"
#include "SessionSupport.hpp"
#include "PullingFeeds.hpp"

namespace NewsGate
{
  namespace RSS
  {
    PullerImpl::PullerImpl(El::Service::Callback* callback)
      throw(Exception, El::Exception)
        : El::Service::CompoundService<PullerState>(callback, "PullerImpl"),
          session_(0)
    {
      PullerState_var st = new LogingIn(
        ACE_Time_Value(Application::instance()->
                       config().puller_management().login_retry_period()),
        this);

      state(st.in());
    }

    PullerImpl::~PullerImpl() throw()
    {
    }

    void
    PullerImpl::logout(::NewsGate::RSS::SessionId session)
      throw(NewsGate::RSS::Puller::ImplementationException,
            CORBA::SystemException)
    {
      PullerState_var st = state();
      return st->logout(session);
    }

    void
    PullerImpl::update_feeds(::NewsGate::RSS::SessionId session,
                             ::NewsGate::RSS::FeedPack* feeds)
      throw(NewsGate::RSS::Puller::NotReady,
            NewsGate::RSS::Puller::ImplementationException,
            CORBA::SystemException)
    {
      PullerState_var st = state();
      st->update_feeds(session, feeds);
    }

    void
    PullerImpl::login_completed() throw()
    {
      try
      {
        if(Application::will_trace(El::Logging::LOW))
        {
          std::ostringstream ostr;
          ostr << "PullerImpl::login_completed(), session " << session();
        
          Application::logger()->trace(ostr.str(),
                                       Aspect::STATE,
                                       El::Logging::LOW);
        }

        PullerState_var st =
          new PullingFeeds(
            this,
            Application::instance()->config().puller_management(),
            Application::instance()->config().feed_request(),
            Application::instance()->config().saving_feed_state(),
            Application::instance()->config().saving_feed_stat(),
            Application::instance()->config().saving_traffic());
        
        state(st.in());
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "PullerImpl::login_completed: "
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
    PullerImpl::initiate_logout() throw()
    {
      try
      {
        Application::logger()->trace("PullerImpl::initiate_logout()",
                                     Aspect::STATE,
                                     El::Logging::LOW);

        PullerState_var st = new Logout(this);
        state(st.in());
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "PullerImpl::initiate_logout: "
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
    PullerImpl::logout_completed() throw()
    {
      try
      {  
        Application::logger()->trace("PullerImpl::logout_completed()",
                                     Aspect::STATE,
                                     El::Logging::LOW);

        RSS::PullerManager_var puller_manager =
          Application::instance()->puller_manager();
          
        // TODO: grab and push all feed responses

        puller_manager->puller_logout(session());

        PullerState_var st = new LogingIn(
          ACE_Time_Value(Application::instance()->
                         config().puller_management().login_retry_period()),
          this);
        
        state(st.in());
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "PullerImpl::logout_completed: "
             << "got El::Exception.\n" << e;
      
        El::Service::Error error(ostr.str(), this, El::Service::Error::ALERT);
        callback_->notify(&error);
      }
      catch (const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "PullerImpl::logout_completed: "
             << "got CORBA::Exception.\n" << e;
      
        El::Service::Error error(ostr.str(), this, El::Service::Error::ALERT);
        callback_->notify(&error);
      }
    }

  }  
}
