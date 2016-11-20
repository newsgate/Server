/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Feeds/RSS/Puller/PullersImpl.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_PULLER_PULLERIMPL_HPP_
#define _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_PULLER_PULLERIMPL_HPP_

#include <string>

#include <El/Exception.hpp>

#include <El/Service/Service.hpp>
#include <El/Service/CompoundService.hpp>

#include <Services/Feeds/RSS/Commons/RSSFeedServices_s.hpp>

#include "SubService.hpp"

namespace NewsGate
{
  namespace RSS
  {
    class PullerImpl :
      public virtual POA_NewsGate::RSS::Puller,
      public virtual El::Service::CompoundService<PullerState>, 
      protected virtual PullerStateCallback
    {
    public:
      
      EL_EXCEPTION(Exception, El::ExceptionBase);

      PullerImpl(El::Service::Callback* callback)
        throw(Exception, El::Exception);

      virtual ~PullerImpl() throw();

    protected:

      //
      // IDL:NewsGate/RSS/Puller/logout:1.0
      //
      virtual void logout(::NewsGate::RSS::SessionId session)
        throw(NewsGate::RSS::Puller::ImplementationException,
              CORBA::SystemException);

      //
      // IDL:NewsGate/RSS/Puller/update_feeds2:1.0
      //
      virtual void update_feeds(::NewsGate::RSS::SessionId session,
                                ::NewsGate::RSS::FeedPack* feeds)
        throw(NewsGate::RSS::Puller::NotReady,
              NewsGate::RSS::Puller::ImplementationException,
              CORBA::SystemException);

    private:

      //
      // PullerStateCallback interface methods
      //
      virtual void session(uint64_t val) throw();
      virtual uint64_t session() throw();

      virtual void login_completed() throw();
      virtual void initiate_logout() throw();
      virtual void logout_completed() throw();
      
    private:
      uint64_t session_;
    };

    typedef El::RefCount::SmartPtr<PullerImpl> PullerImpl_var;

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
    // PullerImpl class
    //
    
    inline
    void
    PullerImpl::session(uint64_t val) throw()
    {
      WriteGuard guard(srv_lock_);      
      session_ = val;
    }    
      
    inline
    uint64_t
    PullerImpl::session() throw()
    {
      ReadGuard guard(srv_lock_);
      return session_;
    }

  }
}

#endif // _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_PULLER_PULLERIMPL_HPP_
