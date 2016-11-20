/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Feeds/RSS/Puller/PullerMain.hpp
 * @author Karen Arutyunov
 * $Id: $
 *
 * File contains declaration of classes responsible for application
 * initialization and configuration.
 */

#ifndef _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_PULLER_PULLERMAIN_HPP_
#define _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_PULLER_PULLERMAIN_HPP_

#include <memory>

#include <ace/OS.h>
#include <ace/Singleton.h>

#include <El/Exception.hpp>
#include <El/Service/Service.hpp>

#include <El/Logging/Logger.hpp>
#include <El/Net/HTTP/MimeTypeMap.hpp>
#include <El/Net/HTTP/Robots.hpp>
#include <El/CORBA/Process/ControlImpl.hpp>

#include <xsd/Config.hpp>

#include <Services/Feeds/RSS/Commons/RSSFeedServices.hpp>

#include "PullerImpl.hpp"

class PullerApp
  : public virtual El::Service::Callback,
    public virtual El::Corba::ProcessCtlImpl
{
  friend class Application;
  
public:
  EL_EXCEPTION(Exception, El::ExceptionBase);
  EL_EXCEPTION(InvalidArgument, Exception);
  
public:
  PullerApp() throw (El::Exception);
  virtual ~PullerApp() throw () {}
  
  void main(int& argc, char** argv) throw ();

  virtual bool notify(El::Service::Event* event) throw(El::Exception);
  
  virtual void terminate_process() throw(El::Exception);
  
public:

  const Server::Config::RSSFeedPullerType& config() const throw ();

  NewsGate::RSS::PullerManager_ptr puller_manager()
    throw(CORBA::SystemException, Exception, El::Exception);

  NewsGate::RSS::Puller_ptr puller_corba_ref() const throw();

  const El::Net::HTTP::MimeTypeMap& mime_types() const throw();

  El::Net::HTTP::RobotsChecker& robots_checker() throw();
  
private:
  typedef ACE_RW_Thread_Mutex     Mutex_;
  typedef ACE_Read_Guard<Mutex_>  ReadGuard;
  typedef ACE_Write_Guard<Mutex_> WriteGuard;
     
  mutable Mutex_ lock_;

  El::Logging::LoggerPtr logger_;
  std::auto_ptr<El::Net::HTTP::RobotsChecker> robots_checker_;

  typedef std::auto_ptr<class Server::Config::ConfigType> ConfigPtr;
  ConfigPtr config_;  

  NewsGate::RSS::PullerManager_var puller_manager_;
  NewsGate::RSS::PullerImpl_var puller_impl_;
  NewsGate::RSS::Puller_var puller_corba_ref_;

  std::auto_ptr<El::Net::HTTP::MimeTypeMap> mime_types_;
};

class Application : public ACE_Singleton<PullerApp, ACE_Thread_Mutex>
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
// PullerApp class
//

inline
const El::Net::HTTP::MimeTypeMap&
PullerApp::mime_types() const throw()
{
  return *mime_types_;
}
  
inline
const Server::Config::RSSFeedPullerType&
PullerApp::config() const throw ()
{
  return config_->rss_feed().puller();
}
  
inline
NewsGate::RSS::Puller_ptr
PullerApp::puller_corba_ref() const throw()
{
  return puller_corba_ref_.in();
}

inline
El::Net::HTTP::RobotsChecker&
PullerApp::robots_checker() throw()
{
  return *robots_checker_;
}

#endif // _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_PULLER_PULLERMAIN_HPP_
