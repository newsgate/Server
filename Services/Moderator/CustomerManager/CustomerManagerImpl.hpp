/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/CustomerManager/CustomerManagerImpl.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MODERATOR_CUSTOMERMANAGER_CUSTOMERMANAGERIMPL_HPP_
#define _NEWSGATE_SERVER_SERVICES_MODERATOR_CUSTOMERMANAGER_CUSTOMERMANAGERIMPL_HPP_

#include <string>

#include <ext/hash_map>
#include <google/sparse_hash_map>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/Service/CompoundService.hpp>
#include <El/Hash/Hash.hpp>
#include <El/String/StringPtr.hpp>
#include <El/Guid.hpp>

#include <Services/Moderator/Commons/CustomerManager_s.hpp>

namespace NewsGate
{
  namespace Moderation
  {
    class CustomerManagerImpl :
      public virtual POA_NewsGate::Moderation::CustomerManager,
      public virtual El::Service::CompoundService<>
    {
    public:
      EL_EXCEPTION(Exception, El::ExceptionBase);
      EL_EXCEPTION(InvalidArgument, Exception);

    public:

      CustomerManagerImpl(El::Service::Callback* callback)
        throw(InvalidArgument, Exception, El::Exception);

      virtual ~CustomerManagerImpl() throw();

    protected:      

      //
      // IDL:NewsGate/Moderation/CustomerManager/get_customer:1.0
      //
      virtual ::NewsGate::Moderation::CustomerInfo* get_customer(
        ::NewsGate::Moderation::CustomerId id)
        throw(NewsGate::Moderation::PermissionDenied,
              NewsGate::Moderation::AccountNotExist,
              NewsGate::Moderation::NotReady,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);      

      //
      // IDL:NewsGate/Moderation/CustomerManager/update_customer:1.0
      //
      virtual ::NewsGate::Moderation::CustomerInfo* update_customer(
        const ::NewsGate::Moderation::CustomerUpdateInfo& customer_info)
        throw(NewsGate::Moderation::PermissionDenied,
              NewsGate::Moderation::AccountNotExist,
              NewsGate::Moderation::NotReady,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);      
      
      virtual bool notify(El::Service::Event* event) throw(El::Exception);
    };

    typedef El::RefCount::SmartPtr<CustomerManagerImpl>
    CustomerManagerImpl_var;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Moderation
  {
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_MODERATOR_CUSTOMERMANAGER_CUSTOMERMANAGERIMPL_HPP_
