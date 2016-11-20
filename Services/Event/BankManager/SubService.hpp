/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Event/BankManagerManager/SubService.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_SERVICES_EVENT_BANKMANAGER_SUBSERVICE_HPP_
#define _NEWSGATE_SERVER_SERVICES_EVENT_BANKMANAGER_SUBSERVICE_HPP_

#include <vector>
#include <string>

#include <El/Exception.hpp>
#include <El/RefCount/All.hpp>
#include <El/Service/CompoundService.hpp>

#include <Services/Commons/Event/EventServices.hpp>
#include <Services/Commons/Event/BankClientSessionImpl.hpp>

namespace NewsGate
{
  namespace Event
  {
    struct BankInfo
    {
      BankClientSessionImpl::BankRef bank;
      ACE_Time_Value timestamp;

      BankInfo(const BankClientSessionImpl::BankRef& bank_ref,
               const ACE_Time_Value& ts = ACE_Time_Value::zero)
        throw(El::Exception);

      BankInfo() throw() {}
    };
    
    typedef std::vector<BankInfo> BankDisposition;
    
    class BankManagerStateCallback : public virtual El::Service::Callback
    {
    public:
      virtual void finalize_bank_registration(
        const BankDisposition& disposition, const char* session_guid)
        throw() = 0;

      virtual void disposition_breakdown() throw() = 0;
      virtual void finalize_disposition_disbandment() throw() = 0;
    };

    //
    // State basic class
    //
    class BankManagerState :
      public El::Service::CompoundService<El::Service::Service,
                                          BankManagerStateCallback>
    {
    public:
        
      EL_EXCEPTION(Exception, El::ExceptionBase);

      BankManagerState(BankManagerStateCallback* callback, const char* name)
        throw(Exception, El::Exception);

      ~BankManagerState() throw();
        
      virtual ::NewsGate::Event::BankSession* bank_login(const char* bank_ior)
        throw(NewsGate::Event::NotReady,
              NewsGate::Event::ImplementationException,
              ::CORBA::SystemException) = 0;

      virtual void ping(const char* bank_ior,
                        ::NewsGate::Event::BankSessionId* current_session_id)
        throw(NewsGate::Event::Logout,
              NewsGate::Event::ImplementationException,
              ::CORBA::SystemException) = 0;

      virtual ::NewsGate::Event::BankClientSession* bank_client_session()
        throw(NewsGate::Event::ImplementationException,
              ::CORBA::SystemException) = 0;
    };

    typedef El::RefCount::SmartPtr<BankManagerState> BankManagerState_var;

    namespace Aspect
    {
      extern const char STATE[];
    }
    
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Event
  {
    //
    // BankManagerState class
    //
    inline
    BankManagerState::BankManagerState(BankManagerStateCallback* callback,
                                       const char* name)
      throw(Exception, El::Exception)
        : El::Service::CompoundService<
             El::Service::Service,
             BankManagerStateCallback>(callback, name)
    {
    }
    
    inline
    BankManagerState::~BankManagerState() throw()
    {
    }

    //
    // BankInfo struct
    //
    inline
    BankInfo::BankInfo(const BankClientSessionImpl::BankRef& bank_ref,
                       const ACE_Time_Value& ts)
      throw(El::Exception)
        : bank(bank_ref),
          timestamp(ts)
    {
    }    
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_EVENT_BANKMANAGER_SUBSERVICE_HPP_
