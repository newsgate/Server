/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Frontends/AdModule/AdModule.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_FRONTENDS_ADMODULE_ADMODULE_HPP_
#define _NEWSGATE_SERVER_FRONTENDS_ADMODULE_ADMODULE_HPP_

#include <string>

#include <El/Exception.hpp>

#include <El/Python/Exception.hpp>
#include <El/Python/Object.hpp>
#include <El/Python/RefCount.hpp>
#include <El/Python/Logger.hpp>
#include <El/Python/Lang.hpp>
#include <El/Python/Country.hpp>
#include <El/Python/Sequence.hpp>
#include <El/Python/Map.hpp>

#include <El/PSP/Config.hpp>

#include <Commons/Ad/Ad.hpp>
#include <Commons/Ad/Python/Ad.hpp>

#include <Services/Commons/Ad/AdServices.hpp>

namespace NewsGate
{
  class AdServer : public El::Python::ObjectImpl
  {
  public:
    EL_EXCEPTION(Exception, El::ExceptionBase);
    
  public:
    AdServer(PyTypeObject *type, PyObject *args, PyObject *kwds)
      throw(Exception, El::Exception);
    
    AdServer(PyObject* args) throw(Exception, El::Exception);
      
    virtual ~AdServer() throw();

    void cleanup() throw();
    
    PyObject* py_selection(PyObject* args) throw(El::Exception);      

    class Type : public El::Python::ObjectTypeImpl<AdServer,
                                                   AdServer::Type>
    {
    public:
      Type() throw(El::Python::Exception, El::Exception);
      static Type instance;
      
      PY_TYPE_METHOD_VARARGS(py_selection, "selection", "Get selection");
    };

  private:
    El::PSP::Config_var config_;
    El::Logging::Python::Logger_var logger_;
    El::Corba::OrbAdapter* orb_adapter_;
    CORBA::ORB_var orb_;

    typedef El::Corba::SmartRef<Ad::AdServer> AdServerRef;
    AdServerRef ad_server_;

    typedef ACE_RW_Thread_Mutex Mutex;
    typedef ACE_Read_Guard<Mutex> ReadGuard;
    typedef ACE_Write_Guard<Mutex> WriteGuard;

    Mutex lock_;    
  };  
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  //
  // NewsGate::AdServer::Type class
  //
  inline
  AdServer::Type::Type() throw(El::Python::Exception, El::Exception)
      : El::Python::ObjectTypeImpl<AdServer, AdServer::Type>(
        "newsgate.ad.serving.AdServer",
        "Object providing ad retreival functionality")
  {
    tp_new = 0;
  }

}
  
#endif // _NEWSGATE_SERVER_FRONTENDS_ADMODULE_ADMODULE_HPP_
