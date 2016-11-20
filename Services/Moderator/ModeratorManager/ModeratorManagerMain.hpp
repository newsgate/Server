/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/ModeratorManager/ModeratorManagerMain.hpp
 * @author Karen Arutyunov
 * $Id: $
 *
 * File contains declaration of classes responsible for application
 * initialization and configuration.
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MODERATOR_MODERATORMANAGER_MODERATORMANAGERMAIN_HPP_
#define _NEWSGATE_SERVER_SERVICES_MODERATOR_MODERATORMANAGER_MODERATORMANAGERMAIN_HPP_

#include <memory>

#include <ace/OS.h>
#include <ace/Singleton.h>

#include <El/Exception.hpp>
#include <El/MySQL/DB.hpp>
#include <El/Service/Service.hpp>

#include <El/Logging/Logger.hpp>
#include <El/CORBA/Process/ControlImpl.hpp>

#include <xsd/Config.hpp>

#include "ModeratorManagerImpl.hpp"

class ModeratorManagerApp
  : public virtual El::Service::Callback,
    public virtual El::Corba::ProcessCtlImpl
{
  friend class Application;
  
public:
  EL_EXCEPTION(Exception, El::ExceptionBase);
  EL_EXCEPTION(InvalidArgument, Exception);
  
public:
  ModeratorManagerApp() throw (El::Exception);
  virtual ~ModeratorManagerApp() throw () {}
  
  void main(int& argc, char** argv) throw ();

  virtual bool notify(El::Service::Event* event) throw(El::Exception);
  virtual void terminate_process() throw(El::Exception);

public:
  const Server::Config::ModeratorManagerType& config() const throw ();
  El::MySQL::DB* dbase() throw(El::Exception);
  El::MySQL::DB* client_management_dbase() throw(El::Exception);

private:
  El::Logging::LoggerPtr logger_;

  typedef std::auto_ptr<class Server::Config::ConfigType> ConfigPtr;
  ConfigPtr config_;

  El::MySQL::DB_var dbase_;
  El::MySQL::DB_var client_management_dbase_;
  
  NewsGate::Moderation::ModeratorManagerImpl_var moderator_manager_impl_;  

  typedef ACE_RW_Thread_Mutex     Mutex_;
  typedef ACE_Read_Guard<Mutex_>  ReadGuard;
  typedef ACE_Write_Guard<Mutex_> WriteGuard;
     
  mutable Mutex_ lock_;
};

class Application :
  public ACE_Singleton<ModeratorManagerApp, ACE_Thread_Mutex>
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
// ModeratorManagerApp class
//

inline
const Server::Config::ModeratorManagerType&
ModeratorManagerApp::config() const throw ()
{
  return config_->moderation().moderator_manager();
}

inline
El::MySQL::DB*
ModeratorManagerApp::dbase() throw(El::Exception)
{
  return dbase_.in();
}

inline
El::MySQL::DB*
ModeratorManagerApp::client_management_dbase() throw(El::Exception)
{
  return client_management_dbase_.in();
}

#endif // _NEWSGATE_SERVER_SERVICES_MODERATOR_MODERATORMANAGER_MODERATORMANAGERMAIN_HPP_
