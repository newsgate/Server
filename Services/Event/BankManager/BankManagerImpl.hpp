/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Event/BankManager/BankManagerImpl.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_EVENT_BANKMANAGER_BANKMANAGERIMPL_HPP_
#define _NEWSGATE_SERVER_SERVICES_EVENT_BANKMANAGER_BANKMANAGERIMPL_HPP_

#include <string>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/Hash/Hash.hpp>
#include <El/Service/CompoundService.hpp>

#include <Services/Commons/Event/EventServices_s.hpp>

#include "SubService.hpp"

namespace NewsGate
{
  namespace Event
  {
    class BankManagerImpl :
      public virtual POA_NewsGate::Event::BankManager,
      public virtual El::Service::CompoundService<BankManagerState>, 
      protected virtual BankManagerStateCallback
    {
    public:
      EL_EXCEPTION(Exception, El::ExceptionBase);
      EL_EXCEPTION(InvalidArgument, Exception);

    public:

      BankManagerImpl(El::Service::Callback* callback)
        throw(InvalidArgument, Exception, El::Exception);

      virtual ~BankManagerImpl() throw();

    protected:

      //
      // IDL:NewsGate/Event/BankManager/bank_login:1.0
      //
      virtual ::NewsGate::Event::BankSession* bank_login(const char* bank_ior)
        throw(NewsGate::Event::NotReady,
              NewsGate::Event::ImplementationException,
              ::CORBA::SystemException);

      //
      // IDL:NewsGate/Event/BankManager/ping:1.0
      //
      virtual void ping(const char* bank_ior,
                        ::NewsGate::Event::BankSessionId* current_session_id)
        throw(NewsGate::Event::Logout,
              NewsGate::Event::ImplementationException,
              ::CORBA::SystemException);

      //
      // IDL:NewsGate/Event/BankManager/bank_client_session:1.0
      //
      virtual ::NewsGate::Event::BankClientSession* bank_client_session()
        throw(NewsGate::Event::ImplementationException,
              ::CORBA::SystemException);

    private:
      
      //
      // BankManagerStateCallback interface methods
      //
      
      virtual void finalize_bank_registration(
        const BankDisposition& disposition, const char* session_guid) throw();
      
      virtual void disposition_breakdown() throw();

      virtual void finalize_disposition_disbandment() throw();
    };

    typedef El::RefCount::SmartPtr<BankManagerImpl> BankManagerImpl_var;

  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Event
  {
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_EVENT_BANKMANAGER_BANKMANAGERIMPL_HPP_
