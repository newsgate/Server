/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Feeds/RSS/PullerManager/SubService.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_PULLERMANAGER_SUBSERVICE_HPP_
#define _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_PULLERMANAGER_SUBSERVICE_HPP_

#include <vector>

#include <El/Exception.hpp>

#include <El/Service/Service.hpp>
#include <El/Service/CompoundService.hpp>

#include <Services/Feeds/RSS/Commons/RSSFeedServices.hpp>
#include <Services/Feeds/RSS/Commons/TransportImpl.hpp>

namespace NewsGate
{
  namespace RSS
  {
    struct PullerSession
    {
      ::NewsGate::RSS::Puller_var puller;
      ::NewsGate::RSS::SessionId session;
    };
    
    typedef std::vector<PullerSession> PullerSessionVector;
    
    class PullerManagerStateCallback : public virtual El::Service::Callback
    {
    public:
      virtual void session(uint32_t val) throw() = 0;
      virtual uint32_t session() throw() = 0;
      
      virtual void register_puller(::NewsGate::RSS::SessionId session_id,
                                   ::NewsGate::RSS::Puller_ptr puller_object)
        throw() = 0;

      virtual void unregister_puller(::NewsGate::RSS::SessionId session_id)
        throw() = 0;

      virtual void finalize_puller_registration() throw() = 0;

      virtual bool check_pullers_presence(const ACE_Time_Value& timeout)
        throw() = 0;

      virtual bool process_feed_state_update(
        ::NewsGate::RSS::SessionId session,
        ::NewsGate::RSS::Transport::FeedStateUpdatePackImpl::Type*
          state_update)
        throw(El::Exception) = 0;

      virtual void process_ping(::NewsGate::RSS::SessionId session)
        throw(El::Exception) = 0;

      virtual bool process_feed_stat(::NewsGate::RSS::SessionId session,
                                     ::NewsGate::RSS::FeedsStatistics* stat)
        throw(El::Exception) = 0;

      virtual void pullers_mistiming() throw() = 0;

      virtual void finalize_pullers_session() throw() = 0;

      virtual PullerSessionVector pullers() throw() = 0;
    };

    //
    // State basic class
    //
    class PullerManagerState :
      public El::Service::CompoundService<El::Service::Service,
                                          PullerManagerStateCallback>
    {
    public:
        
      EL_EXCEPTION(Exception, El::ExceptionBase);

      PullerManagerState(PullerManagerStateCallback* callback,
                         const char* name)
        throw(Exception, El::Exception);

      ~PullerManagerState() throw();
        
      virtual ::NewsGate::RSS::SessionId puller_login(
        ::NewsGate::RSS::Puller_ptr puller_object)
        throw(NewsGate::RSS::PullerManager::ImplementationException,
              CORBA::SystemException) = 0;

      virtual void puller_logout(::NewsGate::RSS::SessionId session)
        throw(NewsGate::RSS::PullerManager::ImplementationException,
              CORBA::SystemException) = 0;

      virtual CORBA::Boolean feed_state(
        ::NewsGate::RSS::SessionId session,
        ::NewsGate::RSS::FeedStateUpdatePack* state_update)
        throw(NewsGate::RSS::PullerManager::Logout,
              NewsGate::RSS::PullerManager::ImplementationException,
              CORBA::SystemException) = 0;

      virtual void ping(::NewsGate::RSS::SessionId session)
        throw(NewsGate::RSS::PullerManager::Logout,
              NewsGate::RSS::PullerManager::ImplementationException,
              CORBA::SystemException) = 0;
    };

    typedef El::RefCount::SmartPtr<PullerManagerState>
    PullerManagerState_var;

    namespace Aspect
    {
      extern const char STATE[];
      extern const char PULLING_FEEDS[];
      extern const char FEED_MANAGEMENT[];
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace RSS
  {
    //
    // PullerManagerImpl::State class
    //
    inline
    PullerManagerState::PullerManagerState(
      PullerManagerStateCallback* callback,
      const char* name)
      throw(Exception, El::Exception)
        : El::Service::CompoundService<
            El::Service::Service,
            PullerManagerStateCallback>(callback, name)
    {
    }
    
    inline
    PullerManagerState::~PullerManagerState() throw()
    {
    }
    
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_PULLERMANAGER_SUBSERVICE_HPP_
