/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Frontends/StatLogger/StatLogger.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_FRONTENDS_STATLOGGER_STATLOGGER_HPP_
#define _NEWSGATE_SERVER_FRONTENDS_STATLOGGER_STATLOGGER_HPP_

#include <string>

#include <El/Exception.hpp>

#include <El/Python/Exception.hpp>
#include <El/Python/Object.hpp>
#include <El/Python/RefCount.hpp>
#include <El/Python/Logger.hpp>

#include <El/PSP/Config.hpp>

#include <Services/Commons/Statistics/StatLogger.hpp>

namespace NewsGate
{
  class StatLogger : public El::Python::ObjectImpl,
                     public El::Service::Callback
  {
  public:
    EL_EXCEPTION(Exception, El::ExceptionBase);
    
  public:
    StatLogger(PyTypeObject *type, PyObject *args, PyObject *kwds)
      throw(Exception, El::Exception);
    
    StatLogger(PyObject* args) throw(Exception, El::Exception);
      
    virtual ~StatLogger() throw();

    void cleanup() throw();    

    PyObject* py_page_impression(PyObject* args) throw(El::Exception);      
    PyObject* py_message_impression(PyObject* args) throw(El::Exception);      
    PyObject* py_message_click(PyObject* args) throw(El::Exception);      
    PyObject* py_message_visit(PyObject* args) throw(El::Exception);      

    class Type : public El::Python::ObjectTypeImpl<StatLogger,
                                                   StatLogger::Type>
    {
    public:
      Type() throw(El::Python::Exception, El::Exception);
      static Type instance;
      
      PY_TYPE_METHOD_VARARGS(py_page_impression,
                             "page_impression",
                             "Record page impression statistics");
      
      PY_TYPE_METHOD_VARARGS(py_message_impression,
                             "message_impression",
                             "Record message impression statistics");
      
      PY_TYPE_METHOD_VARARGS(py_message_click,
                             "message_click",
                             "Record message click statistics");
      
      PY_TYPE_METHOD_VARARGS(py_message_visit,
                             "message_visit",
                             "Record message visit statistics");
    };

  private:
    virtual bool notify(El::Service::Event* event) throw(El::Exception);
    
  private:
    El::PSP::Config_var config_;
    El::Logging::Python::Logger_var logger_;
    El::Corba::OrbAdapter* orb_adapter_;
    CORBA::ORB_var orb_;

    Statistics::StatLogger_var stat_logger_;

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
  // NewsGate::StatLogger::Type class
  //
  inline
  StatLogger::Type::Type() throw(El::Python::Exception, El::Exception)
      : El::Python::ObjectTypeImpl<StatLogger, StatLogger::Type>(
        "newsgate.stat.StatLogger",
        "Object providing statistics recording functionality")
  {
    tp_new = 0;
  }

}
  
#endif // _NEWSGATE_SERVER_FRONTENDS_STATLOGGER_STATLOGGER_HPP_
