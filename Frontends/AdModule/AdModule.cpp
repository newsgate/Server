/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Frontends/AdModule/AdModule.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <Python.h>
#include <El/CORBA/Corba.hpp>

#include <iostream>
#include <string>
#include <sstream>
#include <utility>
#include <vector>

#include <El/Exception.hpp>

#include <El/Python/Object.hpp>
#include <El/Python/Module.hpp>
#include <El/Python/Utility.hpp>
#include <El/Python/RefCount.hpp>

#include <El/PSP/Config.hpp>

#include <Commons/Ad/Ad.hpp>
#include <Commons/Ad/Python/Ad.hpp>

#include <Services/Commons/Ad/AdServices.hpp>
#include <Services/Commons/Ad/TransportImpl.hpp>

#include "AdModule.hpp"

namespace NewsGate
{
  AdServer::Type AdServer::Type::instance;

  static char ASPECT[] = "AdServer";
  
  //
  // NewsGate::AdServingPyModule class
  //
  class AdServingPyModule : public El::Python::ModuleImpl<AdServingPyModule>
  {
  public:
    static AdServingPyModule instance;

    AdServingPyModule() throw(El::Exception);

    virtual void initialized() throw(El::Exception);

    PyObject* py_create_server(PyObject* args) throw(El::Exception);
    PyObject* py_cleanup_server(PyObject* args) throw(El::Exception);
  
    PY_MODULE_METHOD_VARARGS(
      py_create_server,
      "create_server",
      "Creates AdServer object");

    PY_MODULE_METHOD_VARARGS(
      py_cleanup_server,
      "cleanup_server",
      "Cleanups AdServer object");  

    El::Python::Object_var not_found_ex;
  };

  //
  // NewsGate::AdServingPyModule class
  //

  AdServingPyModule AdServingPyModule::instance;
    
  AdServingPyModule::AdServingPyModule() throw(El::Exception)
      : El::Python::ModuleImpl<AdServingPyModule>(
        "newsgate.ad.serving",
        "Module containing AdServer factory method.",
        true)
  {
  }

  void
  AdServingPyModule::initialized() throw(El::Exception)
  {
    not_found_ex = create_exception("NotFound");
  }
  
  PyObject*
  AdServingPyModule::py_create_server(PyObject* args) throw(El::Exception)
  {
    return new AdServer(args);
  }

  PyObject*
  AdServingPyModule::py_cleanup_server(PyObject* args)
    throw(El::Exception)
  {
    PyObject* se = 0;
    
    if(!PyArg_ParseTuple(args,
                         "O:newsgate.ad.serving.cleanup_engine",
                         &se))
    {
      El::Python::handle_error(
        "NewsGate::AdServingPyModule::py_cleanup_engine");
    }

    if(!AdServer::Type::check_type(se))
    {
      El::Python::report_error(
        PyExc_TypeError,
        "1st argument of newsgate.ad.serving.AdServer",
        "NewsGate::AdServingPyModule::py_cleanup_engine");
    }

    AdServer* server = AdServer::Type::down_cast(se);    
    server->cleanup();
    
    return El::Python::add_ref(Py_None);
  }  

  //
  // NewsGate:AdServer class
  //
  AdServer::AdServer(PyTypeObject *type,
                     PyObject *args,
                     PyObject *kwds)
    throw(Exception, El::Exception)
      : El::Python::ObjectImpl(type),
        orb_adapter_(0)
  {
    throw Exception("NewsGate::AdServer::AdServer: "
                    "unforseen way of object creation");
  }
        
  AdServer::AdServer(PyObject* args)
    throw(Exception, El::Exception)
      : El::Python::ObjectImpl(&Type::instance),
        orb_adapter_(0)
  {
    PyObject* config = 0;
    PyObject* logger = 0;
    
    if(!PyArg_ParseTuple(args,
                         "OO:newsgate.ad.serving.AdServer",
                         &config,
                         &logger))
    {
      El::Python::handle_error("NewsGate::AdServer::AdServer");
    }

    if(!El::PSP::Config::Type::check_type(config))
    {
      El::Python::report_error(PyExc_TypeError,
                               "1st argument of el.psp.Config expected",
                               "NewsGate::AdServer::AdServer");
    }

    if(!El::Logging::Python::Logger::Type::check_type(logger))
    {
      El::Python::report_error(PyExc_TypeError,
                               "2nd argument of el.logging.Logger expected",
                               "NewsGate::AdServer::AdServer");
    }

    config_ = El::PSP::Config::Type::down_cast(config, true);
    logger_ = El::Logging::Python::Logger::Type::down_cast(logger, true);

    try
    {
/*      
      char* argv[] =
        {
          "--corba-thread-pool",
          "50"
        };
*/    
      /*
      char* argv[] =
        {
          "--corba-reactive"
        };
      */

      char* argv[] =
        {
        };

      orb_adapter_ = El::Corba::Adapter::orb_adapter(
        sizeof(argv) / sizeof(argv[0]), argv);
      
      orb_ = ::CORBA::ORB::_duplicate(orb_adapter_->orb());

      Ad::Transport::register_valuetype_factories(orb_.in());

      std::string ref = config_->string("server_ref");

      if(!ref.empty())
      {
        ad_server_ = AdServerRef(ref.c_str(), orb_.in());
      }
    }
    catch(const CORBA::Exception& e)
    {
      if(orb_adapter_)
      {
        El::Corba::Adapter::orb_adapter_cleanup();
        orb_adapter_ = 0;
      }

      std::ostringstream ostr;
      ostr << "NewsGate::AdServer::AdServer: "
        "CORBA::Exception caught. Description:\n" << e;

      throw Exception(ostr.str());
    }

    logger_->info("NewsGate::AdServer::AdServer: constructed", ASPECT);
  }

  AdServer::~AdServer() throw()
  {
    cleanup();

    logger_->info("NewsGate::AdServer::~AdServer: destructed", ASPECT);
  }

  void
  AdServer::cleanup() throw()
  {
    try
    {
      bool cleanup_corba_adapter = false;

      {
        WriteGuard guard(lock_);        
        cleanup_corba_adapter = orb_adapter_ != 0;
        orb_adapter_ = 0;
      }

      logger_->info("NewsGate::AdServer::cleanup: done", ASPECT);      
      
      if(cleanup_corba_adapter)
      {
        El::Corba::Adapter::orb_adapter_cleanup();
      }
    }
    catch(...)
    {
    }
  }  
  
  PyObject*
  AdServer::py_selection(PyObject* args) throw(El::Exception)
  {
    if(ad_server_.empty())
    {
      El::Python::report_error(
        AdServingPyModule::instance.not_found_ex.in(), "Ad module disabled");
    }
    
    unsigned long long id = 0;
    
    if(!PyArg_ParseTuple(args,
                         "K:newsgate.ad.serving.AdServer.selection",
                         &id))
    {
      El::Python::handle_error("NewsGate::AdServer::py_selection");
    }

    std::auto_ptr<Ad::Selection> sel;

    try
    {
      El::Python::AllowOtherThreads guard;
      Ad::AdServer_var ad_server = ad_server_.object();

      Ad::Transport::Selection_var result =
        ad_server->selection(Ad::AdServer::INTERFACE_VERSION, id);

      Ad::Transport::SelectionImpl::Type* selection =
        dynamic_cast<Ad::Transport::SelectionImpl::Type*>(result.in());
        
      if(selection == 0)
      {
        throw Exception(
          "NewsGate::AdServer::py_selection: dynamic_cast<"
          "Ad::Transport::SelectionImpl::Type*> failed");
      }

      sel.reset(selection->release());
    }
    catch(const Ad::NotFound& e)
    {
      El::Python::report_error(
        AdServingPyModule::instance.not_found_ex.in(), "Unexpected id");
    }
    catch(const Ad::ImplementationException& e)
    {
      std::ostringstream ostr;
      ostr << "NewsGate::AdServer::py_selection: "
        "NewsGate::Ad::ImplementationException caught. Description:\n"
           << e.description.in();
      
      throw Exception(ostr.str());
    }
    catch(const CORBA::Exception& e)
    {
      std::ostringstream ostr;
      ostr << "NewsGate::AdServer::py_selection: "
        "CORBA::Exception caught. Description:\n" << e;
      
      throw Exception(ostr.str());
    }

    return new Ad::Python::Selection(*sel);
  }
  
}
