/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Frontends/Moderation/CustomerModeration.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_FRONTENDS_MODERATION_CUSTOMERMODERATION_HPP_
#define _NEWSGATE_SERVER_FRONTENDS_MODERATION_CUSTOMERMODERATION_HPP_

#include <string>

#include <ace/Synch.h>
#include <ace/Guard_T.h>

#include <El/Exception.hpp>

#include <El/Python/Exception.hpp>
#include <El/Python/Object.hpp>
#include <El/Python/RefCount.hpp>
#include <El/Python/Logger.hpp>

#include <Services/Moderator/Commons/CustomerManager.hpp>

namespace NewsGate
{  
  namespace CustomerModeration
  {
    EL_EXCEPTION(Exception, El::ExceptionBase);

    typedef El::Corba::SmartRef<NewsGate::Moderation::CustomerManager>
    ManagerRef;    
    
    class Manager : public El::Python::ObjectImpl
    {
    public:
      Manager(PyTypeObject *type, PyObject *args, PyObject *kwds)
        throw(Exception, El::Exception);      
    
      Manager(const ManagerRef& manager_ref) throw(Exception, El::Exception);

      virtual ~Manager() throw() {}

      PyObject* py_get_customer(PyObject* args) throw(El::Exception);
      PyObject* py_update_customer(PyObject* args) throw(El::Exception);

      class Type : public El::Python::ObjectTypeImpl<Manager, Manager::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_METHOD_VARARGS(py_get_customer,
                               "get_customer",
                               "Gets customer information");
        
        PY_TYPE_METHOD_VARARGS(py_update_customer,
                               "update_customer",
                               "Updates customer information");
      };

    private:
      ManagerRef manager_;
    };

    typedef El::Python::SmartPtr<Manager> Manager_var;

    class CustomerInfo : public El::Python::ObjectImpl
    {
    public:
      Moderation::CustomerId id;
      Moderation::CustomerStatus status;
      double balance;
    
    public:
      CustomerInfo(PyTypeObject *type = 0,
                   PyObject *args = 0,
                   PyObject *kwds = 0)
        throw(Exception, El::Exception);

      CustomerInfo(const ::NewsGate::Moderation::CustomerInfo& info)
        throw(El::Exception);
    
      virtual ~CustomerInfo() throw() {}

      class Type : public El::Python::ObjectTypeImpl<CustomerInfo,
                                                     CustomerInfo::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Customer identifier");

        PY_TYPE_MEMBER_ENUM(status,
                            Moderation::CustomerStatus,
                            Moderation::CS_COUNT - 1,
                            "status",
                            "Customer status");
        
        PY_TYPE_MEMBER_FLOAT(balance,
                             "balance",
                             "Customer balance");
      };
    };

    typedef El::Python::SmartPtr<CustomerInfo> CustomerInfo_var;

    class CustomerUpdateInfo : public El::Python::ObjectImpl
    {
    public:
      Moderation::CustomerId id;
      Moderation::CustomerStatus status;
      double balance_change;
    
    public:
      CustomerUpdateInfo(PyTypeObject *type = 0,
                         PyObject *args = 0,
                         PyObject *kwds = 0)
        throw(Exception, El::Exception);
    
      virtual ~CustomerUpdateInfo() throw() {}

      class Type : public El::Python::ObjectTypeImpl<CustomerUpdateInfo,
                                                     CustomerUpdateInfo::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Customer identifier");

        PY_TYPE_MEMBER_ENUM(status,
                            Moderation::CustomerStatus,
                            Moderation::CS_COUNT - 1,
                            "status",
                            "Customer new status");
        
        PY_TYPE_MEMBER_FLOAT(balance_change,
                             "balance_change",
                             "Balance change");
      };
    };

    typedef El::Python::SmartPtr<CustomerUpdateInfo> CustomerUpdateInfo_var;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{  
  namespace CustomerModeration
  {
    //
    // NewsGate::CustomerModeration::Manager class
    //
    inline
    Manager::Manager(const ManagerRef& manager)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          manager_(manager)
    {
    }    

    //
    // NewsGate::CustomerModeration::Manager::Type class
    //
    inline
    Manager::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<Manager, Manager::Type>(
          "newsgate.moderation.customer.Manager",
          "Object representing customer management functionality")
    {
      tp_new = 0;
    }

    //
    // NewsGate::CustomerModeration::CustomerInfo::Type class
    //
    inline
    CustomerInfo::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<CustomerInfo,
                                     CustomerInfo::Type>(
        "newsgate.moderation.customer.CustomerInfo",
        "Object encapsulating customer options")
    {
    }
    
    //
    // NewsGate::CustomerModeration::CustomerInfo class
    //
    inline
    CustomerInfo::CustomerInfo(PyTypeObject *type,
                               PyObject *args,
                               PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          status(Moderation::CS_DISABLED),
          balance(0)
    {
    }

    inline
    CustomerInfo::CustomerInfo(
      const ::NewsGate::Moderation::CustomerInfo& info)
      throw(El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          id(info.id),
          status(info.status),
          balance(info.balance)
    {
    }    
    
    //
    // NewsGate::CustomerModeration::CustomerUpdateInfo::Type class
    //
    inline
    CustomerUpdateInfo::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<
      CustomerUpdateInfo,
      CustomerUpdateInfo::Type>(
        "newsgate.moderation.customer.CustomerUpdateInfo",
        "Object encapsulating customer update options")
    {
    }
    
    //
    // NewsGate::CustomerModeration::CustomerUpdateInfo class
    //
    inline
    CustomerUpdateInfo::CustomerUpdateInfo(
      PyTypeObject *type,
      PyObject *args,
      PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          status(Moderation::CS_DISABLED),
          balance_change(0)
    {
    }
  }
}

#endif // _NEWSGATE_SERVER_FRONTENDS_MODERATION_CUSTOMERMODERATION_HPP_
