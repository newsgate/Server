/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Message/Bank/BankMain.hpp
 * @author Karen Arutyunov
 * $Id: $
 *
 * File contains declaration of classes responsible for application
 * initialization and configuration.
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_BANKMAIN_HPP_
#define _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_BANKMAIN_HPP_

#include <memory>

#include <ace/OS.h>
#include <ace/Singleton.h>

#include <El/Exception.hpp>
#include <El/MySQL/DB.hpp>
#include <El/Service/Service.hpp>

#include <El/Logging/Logger.hpp>
#include <El/CORBA/Process/ControlImpl.hpp>

#include <xsd/Config.hpp>

#include <Services/Commons/Message/BankSessionImpl.hpp>
#include <Services/Commons/Message/MessageServices.hpp>
#include <Services/Commons/Event/EventServices.hpp>
#include <Services/Commons/Event/BankClientSessionImpl.hpp>

#include <Services/Dictionary/Commons/DictionaryServices.hpp>
#include <Services/Segmentation/Commons/SegmentationServices.hpp>

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
  void main(int& argc, char** argv) throw();

  virtual bool notify(El::Service::Event* event) throw(El::Exception);
  virtual void terminate_process() throw(El::Exception);

public:

  const Server::Config::MessageBankType& config() const throw ();

  El::MySQL::DB* dbase() throw(El::Exception);

  NewsGate::Message::BankSessionId* create_session_id() throw(El::Exception);

  NewsGate::Message::BankManager_ptr bank_manager()
    throw(CORBA::SystemException, Exception, El::Exception);

  NewsGate::Dictionary::WordManager_ptr word_manager()
    throw(CORBA::SystemException, Exception, El::Exception);

  NewsGate::Segmentation::Segmentor_ptr segmentor()
    throw(CORBA::SystemException, Exception, El::Exception);

  NewsGate::Event::BankClientSession* event_bank_client_session()
    throw(CORBA::SystemException, Exception, El::Exception);

  const char* bank_ior() const throw(El::Exception);
  size_t preload_mem_usage() const { return preload_mem_usage_; }

private:
  typedef ACE_RW_Thread_Mutex     Mutex_;
  typedef ACE_Read_Guard<Mutex_>  ReadGuard;
  typedef ACE_Write_Guard<Mutex_> WriteGuard;
     
  mutable Mutex_ lock_;

  El::Logging::LoggerPtr logger_;

  typedef std::auto_ptr<class Server::Config::ConfigType> ConfigPtr;
  ConfigPtr config_;

  El::MySQL::DB_var dbase_;

  NewsGate::Dictionary::WordManager_var word_manager_;
  NewsGate::Segmentation::Segmentor_var segmentor_;
  NewsGate::Event::BankClientSessionImpl_var event_bank_client_session_;
  
  std::string bank_manager_ref_;
  NewsGate::Message::BankManager_var bank_manager_;
  NewsGate::Message::BankImpl_var bank_impl_;
  std::string bank_ior_;
  size_t preload_mem_usage_;
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
const Server::Config::MessageBankType&
BankApp::config() const throw ()
{
  return config_->message_service().bank();
}

inline
El::MySQL::DB*
BankApp::dbase() throw(El::Exception)
{
  return dbase_.in();
}

inline
NewsGate::Message::BankSessionId*
BankApp::create_session_id() throw(El::Exception)
{
  return NewsGate::Message::BankSessionIdImpl_init::create();
}

inline
const char*
BankApp::bank_ior() const throw(El::Exception)
{
  return bank_ior_.c_str();
}

inline
NewsGate::Dictionary::WordManager_ptr
BankApp::word_manager() throw(CORBA::SystemException, Exception, El::Exception)
{
  return NewsGate::Dictionary::WordManager::_duplicate(word_manager_.in());
}

inline
NewsGate::Segmentation::Segmentor_ptr
BankApp::segmentor() throw(CORBA::SystemException, Exception, El::Exception)
{
  return NewsGate::Segmentation::Segmentor::_duplicate(segmentor_.in());
}

inline
NewsGate::Event::BankClientSession*
BankApp::event_bank_client_session()
  throw(CORBA::SystemException, Exception, El::Exception)
{
  if(event_bank_client_session_.in() == 0)
  {
    return 0;
  }

  event_bank_client_session_->_add_ref();
  return event_bank_client_session_.in();
}

#endif // _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_BANKMAIN_HPP_
