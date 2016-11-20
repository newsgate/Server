/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Statistics/StatProcessor/StatProcessorMain.hpp
 * @author Karen Arutyunov
 * $Id: $
 *
 * File contains declaration of classes responsible for application
 * initialization and configuration.
 */

#ifndef _NEWSGATE_SERVER_SERVICES_STATISTICS_STATPROCESSOR_STATPROCESSORMAIN_HPP_
#define _NEWSGATE_SERVER_SERVICES_STATISTICS_STATPROCESSOR_STATPROCESSORMAIN_HPP_

#include <memory>

#include <ace/OS.h>
#include <ace/Singleton.h>

#include <El/Exception.hpp>
#include <El/MySQL/DB.hpp>
#include <El/Service/Service.hpp>

#include <El/Logging/Logger.hpp>
#include <El/CORBA/Process/ControlImpl.hpp>

#include <xsd/Config.hpp>

#include "StatProcessorImpl.hpp"

class StatProcessorApp : public virtual El::Service::Callback,
                         public virtual El::Corba::ProcessCtlImpl
{
  friend class Application;
  
public:
  EL_EXCEPTION(Exception, El::ExceptionBase);
  EL_EXCEPTION(InvalidArgument, Exception);
  
public:
  StatProcessorApp() throw (El::Exception);
  virtual ~StatProcessorApp() throw () {}
  
  void main(int& argc, char** argv) throw ();

  virtual bool notify(El::Service::Event* event) throw(El::Exception);
  virtual void terminate_process() throw(El::Exception);
  
public:
  const Server::Config::StatProcessorType& config() const throw ();

  El::MySQL::DB* dbase() throw(El::Exception);

private:
  El::Logging::LoggerPtr logger_;

  typedef std::auto_ptr<class Server::Config::ConfigType> ConfigPtr;
  ConfigPtr config_;

  El::MySQL::DB_var dbase_;  
  NewsGate::Statistics::StatProcessorImpl_var stat_processor_impl_;
};

class Application :
  public ACE_Singleton<StatProcessorApp, ACE_Thread_Mutex>
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
// StatProcessorApp class
// 
inline
const Server::Config::StatProcessorType&
StatProcessorApp::config() const throw ()
{
  return config_->statistics().stat_processor();
}

inline
El::MySQL::DB*
StatProcessorApp::dbase() throw(El::Exception)
{
  return dbase_.in();
}

#endif // _NEWSGATE_SERVER_SERVICES_STATISTICS_STATPROCESSOR_STATPROCESSORMAIN_HPP_
