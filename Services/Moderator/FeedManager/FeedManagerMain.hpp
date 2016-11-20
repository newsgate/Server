/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/FeedManager/FeedManagerMain.hpp
 * @author Karen Arutyunov
 * $Id: $
 *
 * File contains declaration of classes responsible for application
 * initialization and configuration.
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MODERATOR_FEEDMANAGER_FEEDMANAGERMAIN_HPP_
#define _NEWSGATE_SERVER_SERVICES_MODERATOR_FEEDMANAGER_FEEDMANAGERMAIN_HPP_

#include <memory>

#include <ace/OS.h>
#include <ace/Singleton.h>

#include <El/Exception.hpp>
#include <El/Logging/Logger.hpp>
#include <El/MySQL/DB.hpp>
#include <El/Service/Service.hpp>
#include <El/Net/HTTP/Robots.hpp>
#include <El/CORBA/Process/ControlImpl.hpp>

#include <xsd/Config.hpp>

#include "FeedManagerImpl.hpp"
#include "FeedStatSinkImpl.hpp"

class FeedManagerApp
  : public virtual El::Service::Callback,
    public virtual El::Corba::ProcessCtlImpl
{
  friend class Application;
  
public:
  EL_EXCEPTION(Exception, El::ExceptionBase);
  EL_EXCEPTION(InvalidArgument, Exception);
  
public:
  FeedManagerApp() throw (El::Exception);
  virtual ~FeedManagerApp() throw () {}
  
  void main(int& argc, char** argv) throw ();

  virtual bool notify(El::Service::Event* event) throw(El::Exception);
  virtual void terminate_process() throw(El::Exception);

public:
  const Server::Config::FeedManagerType& config() const throw ();
  El::MySQL::DB* dbase() throw(El::Exception);
  El::Net::HTTP::RobotsChecker& robots_checker() throw();

private:
  El::Logging::LoggerPtr logger_;
  std::auto_ptr<El::Net::HTTP::RobotsChecker> robots_checker_;

  typedef std::auto_ptr<class Server::Config::ConfigType> ConfigPtr;
  ConfigPtr config_;

  El::MySQL::DB_var dbase_;
  
  NewsGate::Moderation::FeedManagerImpl_var feed_manager_impl_;
  NewsGate::Statistics::FeedSinkImpl_var feed_stat_sink_impl_;
  
  typedef ACE_RW_Thread_Mutex     Mutex_;
  typedef ACE_Read_Guard<Mutex_>  ReadGuard;
  typedef ACE_Write_Guard<Mutex_> WriteGuard;
     
  mutable Mutex_ lock_;
};

class Application :
  public ACE_Singleton<FeedManagerApp, ACE_Thread_Mutex>
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
// FeedManagerApp class
//


inline
const Server::Config::FeedManagerType&
FeedManagerApp::config() const throw ()
{
  return config_->moderation().feed_manager();
}


inline
El::MySQL::DB*
FeedManagerApp::dbase() throw(El::Exception)
{
  return dbase_.in();
}

inline
El::Net::HTTP::RobotsChecker&
FeedManagerApp::robots_checker() throw()
{
  return *robots_checker_;
}

#endif // _NEWSGATE_SERVER_SERVICES_MODERATOR_FEEDMANAGER_FEEDMANAGERMAIN_HPP_
