/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/tests/RSSFeed/Service/RSSFeedMain.hpp
 * @author Karen Arutyunov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_TESTS_RSSFEED_SERVICE_RSSFEEDMAIN_HPP_
#define _NEWSGATE_SERVER_TESTS_RSSFEED_SERVICE_RSSFEEDMAIN_HPP_

#include <memory>

#include <ace/OS.h>
#include <ace/Singleton.h>

#include <El/Exception.hpp>

#include <El/Logging/Logger.hpp>
#include <El/CORBA/Process/ControlImpl.hpp>

#include <xsd/TestRSSFeed/Config.hpp>

#include "RSSFeedServiceImpl.hpp"

class RSSFeedApp : public virtual El::Service::Callback,
                   public virtual El::Corba::ProcessCtlImpl
{
  friend class Application;
  
public:
  EL_EXCEPTION(Exception, El::ExceptionBase);
  EL_EXCEPTION(InvalidArgument, Exception);
  
public:
  RSSFeedApp() throw (El::Exception);
  virtual ~RSSFeedApp() throw () {}
  
  void main(int& argc, char** argv) throw ();

  virtual bool notify(El::Service::Event* event) throw(El::Exception);
  virtual void terminate_process() throw(El::Exception);

public:

  const Server::TestRSSFeed::Config::ConfigType& config() const throw ();

private:
  typedef ACE_RW_Thread_Mutex     Mutex_;
  typedef ACE_Read_Guard<Mutex_>  ReadGuard;
  typedef ACE_Write_Guard<Mutex_> WriteGuard;
     
  mutable Mutex_ lock_;

  El::Logging::LoggerPtr logger_;

  typedef std::auto_ptr<class Server::TestRSSFeed::Config::ConfigType>
  ConfigPtr;
  
  ConfigPtr config_;  

  NewsGate::RSS::FeedServiceImpl_var feed_impl_;

  typedef ACE_Thread_Mutex         ShutdownMutex;
  typedef ACE_Guard<ShutdownMutex> ShutdownGuard;

  ShutdownMutex shutdown_lock_;
};

class Application : public ACE_Singleton<RSSFeedApp, ACE_Thread_Mutex>
{
public:
  static El::Logging::Logger* logger() throw();
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
// RSSFeedApp class
//


inline
const Server::TestRSSFeed::Config::ConfigType&
RSSFeedApp::config() const throw ()
{
  return *config_.get();
}


#endif // _NEWSGATE_SERVER_TESTS_RSSFEED_SERVICE_RSSFEEDMAIN_HPP_
