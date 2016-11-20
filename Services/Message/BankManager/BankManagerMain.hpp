/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Message/BankManager/BankManagerMain.hpp
 * @author Karen Arutyunov
 * $Id: $
 *
 * File contains declaration of classes responsible for application
 * initialization and configuration.
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MESSAGE_BANKMANAGER_BANKMANAGERMAIN_HPP_
#define _NEWSGATE_SERVER_SERVICES_MESSAGE_BANKMANAGER_BANKMANAGERMAIN_HPP_

#include <memory>

#include <ace/OS.h>
#include <ace/Singleton.h>

#include <El/Exception.hpp>
#include <El/MySQL/DB.hpp>
#include <El/Service/Service.hpp>

#include <El/Logging/Logger.hpp>
#include <El/CORBA/Process/ControlImpl.hpp>

#include <xsd/Config.hpp>

#include "BankManagerImpl.hpp"

class BankManagerApp
  : public virtual El::Service::Callback,
    public virtual El::Corba::ProcessCtlImpl
{
  friend class Application;
  
public:
  EL_EXCEPTION(Exception, El::ExceptionBase);
  EL_EXCEPTION(InvalidArgument, Exception);
  
public:
  BankManagerApp() throw (El::Exception);
  virtual ~BankManagerApp() throw () {}
  
  void main(int& argc, char** argv) throw ();

  virtual bool notify(El::Service::Event* event) throw(El::Exception);
  virtual void terminate_process() throw(El::Exception);

  NewsGate::Message::BankManager_ptr bank_manager_corba_ref() const throw();
  
public:
  const Server::Config::MessageBankManagerType& config() const throw ();

  El::MySQL::DB* dbase() throw(El::Exception);
  El::MySQL::DB* message_managing_dbase() const throw(El::Exception);

  const char* bank_manager_id() const throw(El::Exception);

private:
  El::Logging::LoggerPtr logger_;

  typedef std::auto_ptr<class Server::Config::ConfigType> ConfigPtr;
  ConfigPtr config_;

  El::MySQL::DB_var dbase_;
  El::MySQL::DB_var message_managing_dbase_;
  
  NewsGate::Message::BankManagerImpl_var bank_manager_impl_;
  NewsGate::Message::BankManager_var bank_manager_corba_ref_;
  
  std::string bank_manager_id_;

  typedef ACE_RW_Thread_Mutex     Mutex_;
  typedef ACE_Read_Guard<Mutex_>  ReadGuard;
  typedef ACE_Write_Guard<Mutex_> WriteGuard;
     
  mutable Mutex_ lock_;
};

class Application :
  public ACE_Singleton<BankManagerApp, ACE_Thread_Mutex>
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
// BankManagerApp class
// 
inline
const Server::Config::MessageBankManagerType&
BankManagerApp::config() const throw ()
{
  return config_->message_service().bank_manager();
}

inline
El::MySQL::DB*
BankManagerApp::dbase() throw(El::Exception)
{
  return dbase_.in();
}

inline
El::MySQL::DB*
BankManagerApp::message_managing_dbase() const throw(El::Exception)
{
  return message_managing_dbase_.in();
}

inline
const char*
BankManagerApp::bank_manager_id() const throw(El::Exception)
{
  return bank_manager_id_.c_str();
}

inline
NewsGate::Message::BankManager_ptr
BankManagerApp::bank_manager_corba_ref() const throw()
{
  return bank_manager_corba_ref_.in();
}

#endif // _NEWSGATE_SERVER_SERVICES_MESSAGE_BANKMANAGER_BANKMANAGERMAIN_HPP_
