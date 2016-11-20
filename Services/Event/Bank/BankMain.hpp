/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Event/Bank/BankMain.hpp
 * @author Karen Arutyunov
 * $Id: $
 *
 * File contains declaration of classes responsible for application
 * initialization and configuration.
 */

#ifndef _NEWSGATE_SERVER_SERVICES_EVENT_BANK_BANKMAIN_HPP_
#define _NEWSGATE_SERVER_SERVICES_EVENT_BANK_BANKMAIN_HPP_

#include <memory>

#include <ace/OS.h>
#include <ace/Singleton.h>

#include <El/Exception.hpp>
#include <El/MySQL/DB.hpp>
#include <El/Service/Service.hpp>

#include <El/Logging/Logger.hpp>
#include <El/CORBA/Process/ControlImpl.hpp>

#include <xsd/Config.hpp>

//#include <Services/Commons/Event/EventServices.hpp>
#include "BankImpl.hpp"

/**
 * Responsible for configuration, objects lifetime management.
 */
class BankApp
  : public virtual El::Service::Callback,
    public virtual El::Corba::ProcessCtlImpl
{
public:
  EL_EXCEPTION(Exception, El::ExceptionBase);
  EL_EXCEPTION(InvalidArgument, Exception);

  friend class Application;
  
public:
  BankApp() throw (El::Exception);
  virtual ~BankApp() throw () {}
  
  /**
   * Parses command line, opens config file,
   * creates corba objects, initialize.
   */
  void main(int& argc, char** argv) throw ();

  virtual bool notify(El::Service::Event* event) throw(El::Exception);
  virtual void terminate_process() throw(El::Exception);

public:

  const Server::Config::EventBankType& config() const throw ();

  El::MySQL::DB* dbase() throw(El::Exception);

  NewsGate::Event::BankManager_ptr bank_manager()
    throw(CORBA::SystemException, Exception, El::Exception);

  const char* bank_ior() const throw(El::Exception);
  
private:

  typedef ACE_RW_Thread_Mutex     Mutex_;
  typedef ACE_Read_Guard<Mutex_>  ReadGuard;
  typedef ACE_Write_Guard<Mutex_> WriteGuard;
     
  mutable Mutex_ lock_;

  El::Logging::LoggerPtr logger_;

  typedef std::auto_ptr<class Server::Config::ConfigType> ConfigPtr;
  ConfigPtr config_;

  El::MySQL::DB_var dbase_;

  std::string bank_manager_ref_;
  NewsGate::Event::BankManager_var bank_manager_;
  NewsGate::Event::BankImpl_var bank_impl_;
  std::string bank_ior_;
};

class Application :
  public ACE_Singleton<BankApp, ACE_Thread_Mutex>
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
// BankApp class
// 
inline
const Server::Config::EventBankType&
BankApp::config() const throw ()
{
  return config_->event_service().bank();
}

inline
El::MySQL::DB*
BankApp::dbase() throw(El::Exception)
{
  return dbase_.in();
}

inline
const char*
BankApp::bank_ior() const throw(El::Exception)
{
  return bank_ior_.c_str();
}

#endif // _NEWSGATE_SERVER_SERVICES_EVENT_BANK_BANKMAIN_HPP_
