/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/CategoryManager/CategoryManagerMain.hpp
 * @author Karen Arutyunov
 * $Id: $
 *
 * File contains declaration of classes responsible for application
 * initialization and configuration.
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MODERATOR_CATEGORYMANAGER_CATEGORYMANAGERMAIN_HPP_
#define _NEWSGATE_SERVER_SERVICES_MODERATOR_CATEGORYMANAGER_CATEGORYMANAGERMAIN_HPP_

#include <memory>

#include <ace/OS.h>
#include <ace/Singleton.h>

#include <El/Exception.hpp>
#include <El/MySQL/DB.hpp>
#include <El/Service/Service.hpp>

#include <El/Logging/Logger.hpp>
#include <El/CORBA/Process/ControlImpl.hpp>

#include <xsd/Config.hpp>

#include <Services/Dictionary/Commons/DictionaryServices.hpp>
#include <Services/Segmentation/Commons/SegmentationServices.hpp>

#include "CategoryManagerImpl.hpp"

class CategoryManagerApp
  : public virtual El::Service::Callback,
    public virtual El::Corba::ProcessCtlImpl
{
  friend class Application;
  
public:
  EL_EXCEPTION(Exception, El::ExceptionBase);
  EL_EXCEPTION(InvalidArgument, Exception);
  
public:
  CategoryManagerApp() throw (El::Exception);
  virtual ~CategoryManagerApp() throw () {}
  
  void main(int& argc, char** argv) throw ();

  virtual bool notify(El::Service::Event* event) throw(El::Exception);
  virtual void terminate_process() throw(El::Exception);

public:
  const Server::Config::CategoryManagerType& config() const throw ();
  El::MySQL::DB* dbase() throw(El::Exception);

  NewsGate::Dictionary::WordManager_ptr word_manager()
    throw(CORBA::SystemException, Exception, El::Exception);

  NewsGate::Segmentation::Segmentor_ptr segmentor()
    throw(CORBA::SystemException, Exception, El::Exception);

private:
  El::Logging::LoggerPtr logger_;

  typedef std::auto_ptr<class Server::Config::ConfigType> ConfigPtr;
  ConfigPtr config_;

  El::MySQL::DB_var dbase_;

  NewsGate::Dictionary::WordManager_var word_manager_;
  NewsGate::Segmentation::Segmentor_var segmentor_;  
  
  NewsGate::Moderation::Category::ManagerImpl_var category_manager_impl_;
  
  typedef ACE_RW_Thread_Mutex     Mutex_;
  typedef ACE_Read_Guard<Mutex_>  ReadGuard;
  typedef ACE_Write_Guard<Mutex_> WriteGuard;
     
  mutable Mutex_ lock_;
};

class Application :
  public ACE_Singleton<CategoryManagerApp, ACE_Thread_Mutex>
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
const Server::Config::CategoryManagerType&
CategoryManagerApp::config() const throw ()
{
  return config_->moderation().category_manager();
}

inline
El::MySQL::DB*
CategoryManagerApp::dbase() throw(El::Exception)
{
  return dbase_.in();
}

inline
NewsGate::Dictionary::WordManager_ptr
CategoryManagerApp::word_manager()
  throw(CORBA::SystemException, Exception, El::Exception)
{
  return NewsGate::Dictionary::WordManager::_duplicate(word_manager_.in());
}

inline
NewsGate::Segmentation::Segmentor_ptr
CategoryManagerApp::segmentor()
  throw(CORBA::SystemException, Exception, El::Exception)
{
  return NewsGate::Segmentation::Segmentor::_duplicate(segmentor_.in());
}

#endif // _NEWSGATE_SERVER_SERVICES_MODERATOR_CATEGORYMANAGER_CATEGORYMANAGERMAIN_HPP_
