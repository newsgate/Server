/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Feeds/RSS/PullerManager/PullerManagerImpl.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_PULLERMANAGER_PULLERMANAGERIMPL_HPP_
#define _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_PULLERMANAGER_PULLERMANAGERIMPL_HPP_

#include <ext/hash_map>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/Hash/Hash.hpp>

#include <El/RefCount/All.hpp>

#include <El/Service/Service.hpp>
#include <El/Service/CompoundService.hpp>

#include <Services/Feeds/RSS/Commons/RSSFeedServices_s.hpp>

#include "SubService.hpp"

namespace NewsGate
{
  namespace RSS
  {
    class PullerManagerImpl :
      public virtual POA_NewsGate::RSS::PullerManager,
      public virtual El::Service::CompoundService<PullerManagerState>, 
      protected virtual PullerManagerStateCallback
    {
    public:
      EL_EXCEPTION(Exception, El::ExceptionBase);
      EL_EXCEPTION(InvalidArgument, Exception);

    public:

      PullerManagerImpl(El::Service::Callback* callback)
        throw(InvalidArgument, Exception, El::Exception);

      virtual ~PullerManagerImpl() throw();

    protected:

      //
      // IDL:NewsGate/RSS/PullerManager/puller_login:1.0
      //
      virtual ::NewsGate::RSS::SessionId puller_login(
        ::NewsGate::RSS::Puller_ptr puller_object)
        throw(NewsGate::RSS::PullerManager::ImplementationException,
              CORBA::SystemException);

      //
      // IDL:NewsGate/RSS/PullerManager/puller_logout:1.0
      //
      virtual void puller_logout(::NewsGate::RSS::SessionId session)
        throw(NewsGate::RSS::PullerManager::ImplementationException,
              CORBA::SystemException);

      //
      // IDL:NewsGate/RSS/PullerManager/feed_state:1.0
      //
      virtual CORBA::Boolean feed_state(
        ::NewsGate::RSS::SessionId session,
        ::NewsGate::RSS::FeedStateUpdatePack* state_update)
        throw(NewsGate::RSS::PullerManager::Logout,
              NewsGate::RSS::PullerManager::ImplementationException,
              CORBA::SystemException);

      //
      // IDL:NewsGate/RSS/PullerManager/ping:1.0
      //
      virtual void ping(::NewsGate::RSS::SessionId session)
        throw(NewsGate::RSS::PullerManager::Logout,
              NewsGate::RSS::PullerManager::ImplementationException,
              CORBA::SystemException);
      
      //
      // IDL:NewsGate/RSS/PullerManager/feed_stat:1.0
      //
      virtual void feed_stat(
        ::NewsGate::RSS::SessionId session,
        ::NewsGate::RSS::FeedsStatistics* stat)
        throw(NewsGate::RSS::PullerManager::Logout,
              NewsGate::RSS::PullerManager::ImplementationException,
              CORBA::SystemException);

    private:
      //
      // PullerManagerStateCallback interface methods
      //
      
      virtual void session(uint32_t val) throw();
      virtual uint32_t session() throw();

      virtual void register_puller(::NewsGate::RSS::SessionId session_id,
                                   ::NewsGate::RSS::Puller_ptr puller_object)
        throw();
      
      virtual void unregister_puller(::NewsGate::RSS::SessionId session_id)
        throw();
      
      virtual void finalize_puller_registration() throw();
      
      virtual bool check_pullers_presence(const ACE_Time_Value& timeout)
        throw();

      virtual bool process_feed_state_update(
        ::NewsGate::RSS::SessionId session,
        ::NewsGate::RSS::Transport::FeedStateUpdatePackImpl::Type*
          state_update)
        throw(El::Exception);
      
      virtual bool process_feed_stat(::NewsGate::RSS::SessionId session,
                                     ::NewsGate::RSS::FeedsStatistics* stat)
        throw(El::Exception);
      
      virtual void process_ping(::NewsGate::RSS::SessionId session)
        throw(El::Exception);

      virtual void pullers_mistiming() throw();

      virtual void finalize_pullers_session() throw();
      
      virtual PullerSessionVector pullers() throw();
      
    private:

      struct PullerRecord
      {
        ::NewsGate::RSS::Puller_var reference;
        ACE_Time_Value timestamp;
      };      

      class PullerHashTable :
        public __gnu_cxx::hash_map<
            ::NewsGate::RSS::SessionId,
            PullerRecord,
            El::Hash::Numeric< ::NewsGate::RSS::SessionId> >
      {
      public:
        PullerHashTable() throw(El::Exception);
      };
      
      uint32_t session_;
      PullerHashTable pullers_;
      uint32_t feed_stat_num_;
    };

    typedef El::RefCount::SmartPtr<PullerManagerImpl>
    PullerManagerImpl_var;

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
    // PullerManagerImpl::PullerHashTable class
    //
    
    inline
    PullerManagerImpl::PullerHashTable::PullerHashTable() throw(El::Exception)
    {
    }

    //
    // PullerManagerImpl class
    //
    
    inline
    void
    PullerManagerImpl::session(uint32_t val) throw()
    {
      WriteGuard guard(srv_lock_);      
      session_ = val;
    }    
      
    inline
    uint32_t
    PullerManagerImpl::session() throw()
    {
      ReadGuard guard(srv_lock_);
      return session_;
    }
    
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_PULLERMANAGER_PULLERMANAGERIMPL_HPP_
