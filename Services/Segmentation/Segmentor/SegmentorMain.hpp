/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Segmentation/Segmentor/SegmentorMain.hpp
 * @author Karen Arutyunov
 * $Id: $
 *
 * File contains declaration of classes responsible for application
 * initialization and configuration.
 */

#ifndef _NEWSGATE_SERVER_SERVICES_SEGMENTATION_SEGMENTOR_SEGMENTORMAIN_HPP_
#define _NEWSGATE_SERVER_SERVICES_SEGMENTATION_SEGMENTOR_SEGMENTORMAIN_HPP_

#include <memory>

#include <ace/OS.h>
#include <ace/Singleton.h>

#include <El/Exception.hpp>

#include <El/Service/Service.hpp>

#include <El/Logging/Logger.hpp>
#include <El/CORBA/Process/ControlImpl.hpp>

#include <xsd/Config.hpp>

#include "SegmentorImpl.hpp"

/**
 * Responsible for configuration, objects lifetime management.
 */
class SegmentorApp
  : public virtual El::Service::Callback,
    public virtual El::Corba::ProcessCtlImpl
{
public:
  EL_EXCEPTION(Exception, El::ExceptionBase);
  EL_EXCEPTION(InvalidArgument, Exception);
  
public:
  SegmentorApp() throw (El::Exception);
  virtual ~SegmentorApp() throw () {}
  
  /**
   * Parses command line, opens config file,
   * creates corba objects, initialize.
   */
  void main(int& argc, char** argv) throw ();

  virtual bool notify(El::Service::Event* event) throw(El::Exception);
  virtual void terminate_process() throw(El::Exception);

public:

  const Server::Config::SegmentorType& config() const throw ();

  El::Logging::LoggerPtr logger_;

private:
  typedef std::auto_ptr<class Server::Config::ConfigType> ConfigPtr;
  ConfigPtr config_;

  NewsGate::Segmentation::SegmentorImpl_var segmentor_impl_;

  typedef ACE_RW_Thread_Mutex     Mutex_;
  typedef ACE_Read_Guard<Mutex_>  ReadGuard;
  typedef ACE_Write_Guard<Mutex_> WriteGuard;
     
  mutable Mutex_ lock_;
};

class Application : public ACE_Singleton<SegmentorApp, ACE_Thread_Mutex>
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
// SegmentorApp class
// 
inline
const Server::Config::SegmentorType&
SegmentorApp::config() const throw ()
{
  return config_->segmentation().segmentor();
}
  
#endif // _NEWSGATE_SERVER_SERVICES_SEGMENTATION_SEGMENTOR_SEGMENTORMAIN_HPP_
