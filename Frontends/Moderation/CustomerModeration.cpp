/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Frontends/Moderation/CustomerModeration.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <Python.h>
#include <El/CORBA/Corba.hpp>

#include <string>
#include <sstream>

#include <El/Exception.hpp>

#include <El/Python/Object.hpp>
#include <El/Python/Module.hpp>
#include <El/Python/Utility.hpp>
#include <El/Python/RefCount.hpp>

#include <Services/Moderator/Commons/CustomerManager.hpp>

#include "CustomerModeration.hpp"
#include "Moderation.hpp"

namespace NewsGate
{
  namespace CustomerModeration
  {
    Manager::Type Manager::Type::instance;
    CustomerInfo::Type CustomerInfo::Type::instance;
    CustomerUpdateInfo::Type CustomerUpdateInfo::Type::instance;
    
    //
    // NewsGate::CustomerModeration::CustomerModerationPyModule class
    //
    class CustomerModerationPyModule :
      public El::Python::ModuleImpl<CustomerModerationPyModule>
    {
    public:
      static CustomerModerationPyModule instance;

      CustomerModerationPyModule() throw(El::Exception);
      
      virtual void initialized() throw(El::Exception);

      El::Python::Object_var account_not_exist_ex;
      El::Python::Object_var permission_denied_ex;
   };
  
    CustomerModerationPyModule CustomerModerationPyModule::instance;
  
    CustomerModerationPyModule::CustomerModerationPyModule()
      throw(El::Exception)
        : El::Python::ModuleImpl<CustomerModerationPyModule>(
        "newsgate.moderation.customer",
        "Module containing Customer Moderation types.",
        true)
    {
    }

    void
    CustomerModerationPyModule::initialized() throw(El::Exception)
    {
      account_not_exist_ex = create_exception("AccountNotExist");
      permission_denied_ex = create_exception("PermissionDenied");

      add_member(PyLong_FromLong(Moderation::CS_ENABLED), "CS_ENABLED");
      add_member(PyLong_FromLong(Moderation::CS_DISABLED), "CS_DISABLED");    
    }
    
    //
    // NewsGate::CustomerModeration::Manager class
    //
    
    Manager::Manager(PyTypeObject *type, PyObject *args, PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type)
    {
      throw Exception(
        "NewsGate::CustomerModeration::Manager::Manager: unforseen way "
        "of object creation");
    }
    
    PyObject*
    Manager::py_get_customer(PyObject* args)
      throw(El::Exception)
    {
      unsigned long id = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "k:newsgate.moderation.customer.Manager.get_customer",
           &id))
      {
        El::Python::handle_error(
          "NewsGate::CustomerModeration::Manager::py_get_customer");
      }

      try
      {
        Moderation::CustomerInfo_var result_info;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::CustomerManager_var customer_manager =
            manager_.object();
          
          result_info = customer_manager->get_customer(id);
        }
      
        return new CustomerInfo(*result_info);
      }
      catch(const Moderation::AccountNotExist& )
      {
        El::Python::report_error(
          CustomerModerationPyModule::instance.account_not_exist_ex.in(),
          "Account not exist");
      }
      catch(const Moderation::PermissionDenied& )
      {
        El::Python::report_error(
          CustomerModerationPyModule::instance.permission_denied_ex.in(),
          "Permission denied");
      }
      catch(const Moderation::NotReady& e)
      {
        std::ostringstream ostr;
        ostr << "System not ready. Reason:\n" << e.reason.in();
        
        El::Python::report_error(
          ModerationPyModule::instance.not_ready_ex.in(),
          ostr.str().c_str());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::CustomerModeration::Manager::py_get_customer: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::CustomerModeration::Manager::py_update_customer: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::CustomerModeration::Manager::py_update_customer: "
        "unexpected execution path");
    }        

    PyObject*
    Manager::py_update_customer(PyObject* args)
      throw(El::Exception)
    {
      PyObject* ui = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "O:newsgate.moderation.customer.Manager.update_customer",
           &ui))
      {
        El::Python::handle_error(
          "NewsGate::ModeratorModeration::Manager::py_update_customer");
      }

      CustomerUpdateInfo* update = CustomerUpdateInfo::Type::down_cast(ui);

      ::NewsGate::Moderation::CustomerUpdateInfo_var cui =
          new ::NewsGate::Moderation::CustomerUpdateInfo();

      cui->id = update->id;
      cui->status = update->status;
      cui->balance_change = update->balance_change;

      try
      {
        Moderation::CustomerInfo_var result_info;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::CustomerManager_var customer_manager =
            manager_.object();
          
          result_info = customer_manager->update_customer(cui.in());
        }
      
        return new CustomerInfo(*result_info);
      }
      catch(const Moderation::AccountNotExist& )
      {
        El::Python::report_error(
          CustomerModerationPyModule::instance.account_not_exist_ex.in(),
          "Account not exist");
      }
      catch(const Moderation::PermissionDenied& )
      {
        El::Python::report_error(
          CustomerModerationPyModule::instance.permission_denied_ex.in(),
          "Permission denied");
      }
      catch(const Moderation::NotReady& e)
      {
        std::ostringstream ostr;
        ostr << "System not ready. Reason:\n" << e.reason.in();
        
        El::Python::report_error(
          ModerationPyModule::instance.not_ready_ex.in(),
          ostr.str().c_str());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::CustomerModeration::Manager::py_update_customer: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::CustomerModeration::Manager::py_update_customer: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::CustomerModeration::Manager::py_update_customer: "
        "unexpected execution path");
    }        
  }
}
