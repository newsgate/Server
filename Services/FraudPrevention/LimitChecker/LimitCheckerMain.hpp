/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/FraudPrevention/LimitChecker/LimitCheckerMain.hpp
 * @author Karen Arutyunov
 * $Id: $
 *
 * File contains declaration of classes responsible for application
 * initialization and configuration.
 */

#ifndef _NEWSGATE_SERVER_SERVICES_FRAUDPREVENTION_LIMITCHECKER_LIMITCHECKERMAIN_HPP_
#define _NEWSGATE_SERVER_SERVICES_FRAUDPREVENTION_LIMITCHECKER_LIMITCHECKERMAIN_HPP_

#include <memory>

#include <ace/OS.h>
#include <ace/Singleton.h>

#include <El/Exception.hpp>
#include <El/MySQL/DB.hpp>
#include <El/Service/Service.hpp>

#include <El/CORBA/Process/ControlImpl.hpp>

#include <xsd/Config.hpp>

#include "LimitCheckerImpl.hpp"

class LimitCheckerApp : public virtual El::Service::Callback,
                        public virtual El::Corba::ProcessCtlImpl
{
  friend class Application;
  
public:
  EL_EXCEPTION(Exception, El::ExceptionBase);
  EL_EXCEPTION(InvalidArgument, Exception);
  
public:
  LimitCheckerApp() throw (El::Exception);
  virtual ~LimitCheckerApp() throw () {}
  
  void main(int& argc, char** argv) throw ();

  virtual bool notify(El::Service::Event* event) throw(El::Exception);
  virtual void terminate_process() throw(El::Exception);
  
public:
  const Server::Config::LimitCheckerType& config() const throw ();

  El::MySQL::DB* dbase() throw(El::Exception);

private:
  El::Logging::LoggerPtr logger_;

  typedef std::auto_ptr<class Server::Config::ConfigType> ConfigPtr;
  ConfigPtr config_;

  El::MySQL::DB_var dbase_;  
  NewsGate::FraudPrevention::LimitCheckerImpl_var limit_checker_impl_;
};

class Application :
  public ACE_Singleton<LimitCheckerApp, ACE_Thread_Mutex>
{
public:
  static El::Logging::Logger* logger() throw ();
  static bool will_trace(unsigned long trace_level) throw();
};


//////////////////////////////////////////////////////////////////////////////
// Inlines
//////////////////////////////////////////////////////////////////////////////

//
// Application class
//

inline
El::Logging::Logger*
Application::logger() throw ()
{
  return instance()->logger_.get();
}

inline
bool
Application::will_trace(unsigned long trace_level) throw()
{
  return instance()->logger_->will_trace(trace_level);
}

//
// LimitCheckerApp class
// 
inline
const Server::Config::LimitCheckerType&
LimitCheckerApp::config() const throw ()
{
  return config_->fraud_prevention().limit_checker();
}

inline
El::MySQL::DB*
LimitCheckerApp::dbase() throw(El::Exception)
{
  return dbase_.in();
}

#endif // _NEWSGATE_SERVER_SERVICES_FRAUDPREVENTION_LIMITCHECKER_LIMITCHECKERMAIN_HPP_
