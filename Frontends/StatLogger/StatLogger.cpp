/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Frontends/StatLogger/StatLogger.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <Python.h>
#include <El/CORBA/Corba.hpp>

#include <iostream>
#include <string>
#include <sstream>
#include <utility>
#include <vector>

#include <El/Exception.hpp>
#include <El/Guid.hpp>
#include <El/Net/HTTP/URL.hpp>

#include <El/Python/Object.hpp>
#include <El/Python/Module.hpp>
#include <El/Python/Utility.hpp>
#include <El/Python/RefCount.hpp>

#include <El/PSP/Config.hpp>

#include <Services/Commons/Statistics/StatLogger.hpp>
#include <Services/Commons/FraudPrevention/LimitCheck.hpp>

#include "StatLogger.hpp"

namespace NewsGate
{
  StatLogger::Type StatLogger::Type::instance;

  static char ASPECT[] = "StatLogger";
  
  //
  // NewsGate::StatPyModule class
  //
  class StatPyModule : public El::Python::ModuleImpl<StatPyModule>
  {
  public:
    static StatPyModule instance;

    StatPyModule() throw(El::Exception);

    PyObject* py_create_logger(PyObject* args) throw(El::Exception);
    PyObject* py_cleanup_logger(PyObject* args) throw(El::Exception);
  
    PY_MODULE_METHOD_VARARGS(
      py_create_logger,
      "create_logger",
      "Creates StatLogger object");

    PY_MODULE_METHOD_VARARGS(
      py_cleanup_logger,
      "cleanup_logger",
      "Cleanups StatLogger object");
  };

  //
  // NewsGate::StatLogger class
  //
  StatLogger::StatLogger(PyTypeObject *type,
                         PyObject *args,
                         PyObject *kwds)
    throw(Exception, El::Exception)
      : El::Python::ObjectImpl(type),
        orb_adapter_(0)
  {
    throw Exception("NewsGate::StatLogger::StatLogger: "
                    "unforseen way of object creation");
  }
        
  StatLogger::StatLogger(PyObject* args)
    throw(Exception, El::Exception)
      : El::Python::ObjectImpl(&Type::instance),
        orb_adapter_(0)
  {
    PyObject* config = 0;
    PyObject* logger = 0;
    
    if(!PyArg_ParseTuple(args,
                         "OO:newsgate.stat.StatLogger.StatLogger",
                         &config,
                         &logger))
    {
      El::Python::handle_error("NewsGate::StatLogger::StatLogger");
    }

    if(!El::PSP::Config::Type::check_type(config))
    {
      El::Python::report_error(PyExc_TypeError,
                               "1st argument of el.psp.Config expected",
                               "NewsGate::StatLogger::StatLogger");
    }

    if(!El::Logging::Python::Logger::Type::check_type(logger))
    {
      El::Python::report_error(PyExc_TypeError,
                               "2nd argument of el.logging.Logger expected",
                               "NewsGate::StatLogger::StatLogger");
    }

    config_ = El::PSP::Config::Type::down_cast(config, true);
    logger_ = El::Logging::Python::Logger::Type::down_cast(logger, true);

    try
    {
      char* argv[] =
        {
        };
      
      orb_adapter_ = El::Corba::Adapter::orb_adapter(
        sizeof(argv) / sizeof(argv[0]), argv);
      
      orb_ = ::CORBA::ORB::_duplicate(orb_adapter_->orb());

      std::string ref = config_->string("processor_ref");
      std::string limit_checker_ref = config_->string("limit_checker_ref");

      if(!ref.empty())
      {
        FraudPrevention::EventLimitCheckDescArray check_descriptors;

        check_descriptors.add_check_descriptors(
          FraudPrevention::ET_CLICK,
          true,
          false,
          false,
          config_->string("fraud_prevention_click_user").c_str());
        
        
          check_descriptors.add_check_descriptors(
          FraudPrevention::ET_CLICK,
          true,
          false,
          true,
          config_->string("fraud_prevention_click_user_msg").c_str());
        
        check_descriptors.add_check_descriptors(
          FraudPrevention::ET_CLICK,
          false,
          true,
          false,
          config_->string("fraud_prevention_click_ip").c_str());
        
        check_descriptors.add_check_descriptors(
          FraudPrevention::ET_CLICK,
          false,
          true,
          true,
          config_->string("fraud_prevention_click_ip_msg").c_str());

        Statistics::StatLogger::IpSet ip_whitelist;

        El::String::ListParser parser(
          config_->string("xsearch.client_ips").c_str());
        
        const char* item = 0;
    
        while((item = parser.next_item()) != 0)
        {
          if(strcmp(item, "*") == 0)
          {
            ip_whitelist.clear();
            check_descriptors.clear();
            break;
          }

          ip_whitelist.insert(item);
        }

        stat_logger_ =
          new Statistics::StatLogger(
            ref.c_str(),
            limit_checker_ref.c_str(),
            check_descriptors,
            ip_whitelist,
            orb_.in(),
            config_->number("flush_period"),
            this);
        
        stat_logger_->start();
      }
    }
    catch(const CORBA::Exception& e)
    {
      if(orb_adapter_)
      {
        El::Corba::Adapter::orb_adapter_cleanup();
        orb_adapter_ = 0;
      }

      std::ostringstream ostr;
      ostr << "NewsGate::StatLogger::StatLogger: "
        "CORBA::Exception caught. Description:\n" << e;

      throw Exception(ostr.str());
    }

    logger_->info("NewsGate::StatLogger::StatLogger: "
                  "stat logger constructed",
                  ASPECT);
  }

  StatLogger::~StatLogger() throw()
  {
    cleanup();

    logger_->info("NewsGate::StatLogger::~StatLogger: "
                  "stat logger destructed",
                  ASPECT);
  }

  void
  StatLogger::cleanup() throw()
  {
    try
    {
      Statistics::StatLogger_var stat_logger;
      bool cleanup_corba_adapter = false;

      {
        WriteGuard guard(lock_);
        
        stat_logger = stat_logger_.retn();        
        cleanup_corba_adapter = orb_adapter_ != 0;
        orb_adapter_ = 0;
      }

      if(stat_logger.in())
      {
        stat_logger->stop();
        stat_logger->wait();
      }

      logger_->info("NewsGate::StatLogger::cleanup: done", ASPECT);      
      
      if(cleanup_corba_adapter)
      {
        El::Corba::Adapter::orb_adapter_cleanup();
      }
    }
    catch(...)
    {
    }
  }

  PyObject*
  StatLogger::py_page_impression(PyObject* args) throw(El::Exception)
  {
    if(stat_logger_.in() == 0)
    {
      return El::Python::add_ref(Py_False);
    }
    
    const char* request_id = 0;
    const char* protocol = 0;
    const char* referer = 0;
    const char* host = 0;
    
    if(!PyArg_ParseTuple(args,
                         "ssss:newsgate.stat.StatLogger.page_impression",
                         &request_id,
                         &protocol,
                         &referer,
                         &host))
    {
      El::Python::handle_error("NewsGate::StatLogger::py_page_impression");
    }

    Statistics::PageImpressionInfo ii;
    Statistics::StatLogger::guid(request_id, ii.id);
    ii.time.time(ACE_OS::gettimeofday());

    switch(protocol[0])
    {
    case 'h':
    case 'x':
    case 'j': ii.protocol = toupper(protocol[0]);
    default: break;
    }

    try
    {
      ii.referer.referer(referer, host);
    }
    catch(const El::Net::URL::Exception&)
    {
    }
    
    stat_logger_->page_impression(ii);
    
    return El::Python::add_ref(Py_True);
  }
  
  PyObject*
  StatLogger::py_message_impression(PyObject* args) throw(El::Exception)
  {
    if(stat_logger_.in() == 0)
    {
      return El::Python::add_ref(Py_False);
    }
    
//    std::cerr << "StatLogger::py_message_impression\n";

    const char* request_id = 0;
    const char* protocol = 0;
    const char* client_id = 0;
    const char* message_ids = 0;
    
    if(!PyArg_ParseTuple(args,
                         "ssss:newsgate.stat.StatLogger.message_impression",
                         &request_id,
                         &protocol,
                         &client_id,
                         &message_ids))
    {
      El::Python::handle_error("NewsGate::StatLogger::py_message_impression");
    }

    char proto = 'H';

    switch(protocol[0])
    {
    case 'h':
    case 'x': proto = toupper(protocol[0]);
    default: break;
    }
    
    Statistics::MessageImpressionInfo ii;
    Statistics::StatLogger::guid(request_id, ii.id);    
    Statistics::StatLogger::guid(client_id, ii.client_id);
    
    ii.time.time(ACE_OS::gettimeofday());

    El::String::ListParser parser(message_ids);

    const char* item = 0;

    while((item = parser.next_item()) != 0)
    {
      uint32_t impressions = 1;
      const char* delim = strchr(item, ':');
      std::string msg_id;

      if(delim)
      {
        if(proto == 'X' && El::String::Manip::numeric(delim + 1, impressions))
        {
          msg_id.assign(item, delim - item);
          item = msg_id.c_str();
        }
        else
        {
          std::ostringstream ostr;
          ostr << "NewsGate::StatLogger::py_message_impression: unexpected "
            "message spec '" << item << "' for protocol '" << proto << "'";

          throw Exception(ostr.str());
        }
      }
      
      size_t len = strlen(item);

      if(len)
      {
        Message::Transport::MessageStatInfo stat;
        
        stat.id = Message::Id(item, item[len - 1] == '=');
        stat.count = impressions;
        
        ii.messages.push_back(stat);
      }
    }

    stat_logger_->message_impression(ii);
    
    return El::Python::add_ref(Py_True);
  }
  
  PyObject*
  StatLogger::py_message_click(PyObject* args) throw(El::Exception)
  {
    if(stat_logger_.in() == 0)
    {
      return El::Python::add_ref(Py_False);
    }
    
    const char* request_id = 0;
    const char* protocol = 0;
    const char* client_id = 0;
    const char* message_ids = 0;
    const char* remote_ip = 0;
    
    if(!PyArg_ParseTuple(args,
                         "sssss:newsgate.stat.StatLogger.message_click",
                         &request_id,
                         &protocol,
                         &client_id,
                         &message_ids,
                         &remote_ip))
    {
      El::Python::handle_error("NewsGate::StatLogger::py_message_click");
    }

    char proto = 'H';

    switch(protocol[0])
    {
    case 'h':
    case 'x': proto = toupper(protocol[0]);
    default: break;
    }
    
    Statistics::MessageClickInfo ci;
    Statistics::StatLogger::guid(request_id, ci.id);
    Statistics::StatLogger::guid(client_id, ci.client_id);
    
    strcpy(ci.ip, remote_ip);
    
    ci.time.time(ACE_OS::gettimeofday());

    El::String::ListParser parser(message_ids);

    const char* item = 0;
    
    while((item = parser.next_item()) != 0)
    {
      uint32_t clicks = 1;
      const char* delim = strchr(item, ':');
      std::string msg_id;

      if(delim)
      {
        if(proto == 'X' && El::String::Manip::numeric(delim + 1, clicks))
        {
          msg_id.assign(item, delim - item);
          item = msg_id.c_str();
        }
        else
        {
          std::ostringstream ostr;
          ostr << "NewsGate::StatLogger::py_message_click: unexpected "
            "message spec '" << item << "' for protocol '" << proto << "'";

          throw Exception(ostr.str());
        }
      }
      
      size_t len = strlen(item);

      if(len)
      {
        Message::Transport::MessageStatInfo stat;
        
        stat.id = Message::Id(item, item[len - 1] == '=');
        stat.count = clicks;
        
        ci.messages.push_back(stat);        
      }
    }
    
    stat_logger_->message_click(ci);
    
    return El::Python::add_ref(Py_True);
  }
  
  PyObject*
  StatLogger::py_message_visit(PyObject* args) throw(El::Exception)
  {
    if(stat_logger_.in() == 0)
    {
      return El::Python::add_ref(Py_False);
    }
    
    const char* request_id = 0;
    const char* protocol = 0;
    const char* client_id = 0;
    const char* message_ids = 0;
    const char* event_ids = 0;
    
    if(!PyArg_ParseTuple(args,
                         "sssss:newsgate.stat.StatLogger.message_visit",
                         &request_id,
                         &protocol,
                         &client_id,
                         &message_ids,
                         &event_ids))
    {
      El::Python::handle_error("NewsGate::StatLogger::py_message_visit");
    }

    Statistics::MessageVisitInfo vi;
    
    vi.time = ACE_OS::gettimeofday().sec();
    
    {
      El::String::ListParser parser(message_ids);
      const char* item = 0;
    
      while((item = parser.next_item()) != 0)
      {      
        size_t len = strlen(item);
        
        if(len)
        {        
          vi.messages.push_back(Message::Id(item, item[len - 1] == '='));
        }
      }
    }
    
    {
      El::String::ListParser parser(event_ids);
      const char* item = 0;
    
      while((item = parser.next_item()) != 0)
      {      
        size_t len = strlen(item);
        
        if(len)
        {        
          vi.events.push_back(El::Luid(item, item[len - 1] == '='));
        }
      }
    }

    stat_logger_->message_visit(vi);
    
    return El::Python::add_ref(Py_True);
  }
  
  bool
  StatLogger::notify(El::Service::Event* event) throw(El::Exception)
  {
    El::Service::log(event,
                     "NewsGates::StatLogger::notify: ",
                     logger_.in(),
                     ASPECT);
    
    return true;
  }

  //
  // NewsGate::StatPyModule class
  //
  StatPyModule StatPyModule::instance;
    
  StatPyModule::StatPyModule() throw(El::Exception)
      : El::Python::ModuleImpl<StatPyModule>(
        "newsgate.stat",
        "Module containing StatLogger factory method.",
        true)
  {
  }

  PyObject*
  StatPyModule::py_create_logger(PyObject* args) throw(El::Exception)
  {
    return new StatLogger(args);
  }

  PyObject*
  StatPyModule::py_cleanup_logger(PyObject* args)
    throw(El::Exception)
  {
    PyObject* se = 0;
    
    if(!PyArg_ParseTuple(args,
                         "O:newsgate.stat.cleanup_logger",
                         &se))
    {
      El::Python::handle_error(
        "NewsGate::StatPyModule::py_cleanup_logger");
    }

    if(!StatLogger::Type::check_type(se))
    {
      El::Python::report_error(
        PyExc_TypeError,
        "1st argument of newsgate.stat.StatLogger",
        "NewsGate::StatPyModule::py_cleanup_logger");
    }

    StatLogger* logger = StatLogger::Type::down_cast(se);    
    logger->cleanup();
    
    return El::Python::add_ref(Py_None);
  }  
}
