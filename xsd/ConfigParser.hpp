/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/xsd/ConfigParser.hpp
 * @author Karen Arutyunov
 * $Id: $
 *
 */

#ifndef _NEWSGATE_SERVER_XSD_CONFIGPARSER_HPP_
#define _NEWSGATE_SERVER_XSD_CONFIGPARSER_HPP_

#include <ace/OS.h>

#include <xercesc/sax2/SAX2XMLReader.hpp>

#include <El/Exception.hpp>
#include <El/MySQL/DB.hpp>

#include <El/Logging/Logger.hpp>
#include <El/Logging/FileLogger.hpp>

#include <El/XML/EntityResolver.hpp>
#include <El/Net/HTTP/Robots.hpp>

#include <El/Python/Sandbox.hpp>

#include <xsd/Config.hpp>

namespace NewsGate
{
  namespace Config
  {
    El::MySQL::DB* create_db(const Server::Config::DataBaseType& cfg,
                             const char* charset = 0)
      throw(El::MySQL::Exception, El::Exception);

    El::Logging::Logger* create_logger(
      const Server::Config::LoggerType& cfg) throw(El::Exception);

    El::XML::EntityResolver* create_entity_resolver(
      const Server::Config::EntityResolverType& resolver)
      throw(El::Exception);

    El::Net::HTTP::RobotsChecker* create_robots_checker(
      const Server::Config::RobotsCheckerType& cfg)
      throw(El::Exception);

    El::Python::Sandbox* create_python_sandbox(
      const Server::Config::PythonSandboxOptionsType& cfg,
      El::Python::Sandbox::Callback* callback = 0)
      throw(El::Exception);
  }
}

//////////////////////////////////////////////////////////////////////////////
// Inlines
//////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Config
  {
    inline
    El::MySQL::DB*
    create_db(const Server::Config::DataBaseType& cfg,
              const char* charset)
      throw(El::MySQL::Exception, El::Exception)
    {
      El::MySQL::ConnectionFactory* connection_factory = 0;

      if(cfg.connection_pool().present())
      {
        connection_factory = new El::MySQL::ConnectionPoolFactory(
          cfg.connection_pool()->min_connections(),
          cfg.connection_pool()->max_connections(),
          0,
          charset);
      }
      else
      {
        connection_factory = new El::MySQL::NewConnectionsFactory(0, charset);
      }
        
      return cfg.unix_socket().empty() ?
        new El::MySQL::DB(cfg.user().c_str(),
                          cfg.passwd().c_str(),
                          cfg.dbname().c_str(),
                          cfg.port(),
                          cfg.host().c_str(),
                          cfg.client_flags(),
                          connection_factory) :
        new El::MySQL::DB(cfg.user().c_str(),
                          cfg.passwd().c_str(),
                          cfg.dbname().c_str(),
                          cfg.unix_socket().c_str(),
                          cfg.client_flags(),
                          connection_factory);      
    }

    inline
    El::Logging::Logger*
    create_logger(const Server::Config::LoggerType& cfg) throw(El::Exception)
    {
      El::Logging::FileLogger::RotatingPolicyList logger_policies;

      if(cfg.time_span_policy().present())
      {
        logger_policies.push_back(
          new El::Logging::FileLogger::RotatingByTimePolicy(
            ACE_Time_Value(cfg.time_span_policy()->time())));
      }

      if(cfg.size_span_policy().present())
      {
        logger_policies.push_back(
          new El::Logging::FileLogger::RotatingBySizePolicy(
            cfg.size_span_policy()->size()));
      }

      return new El::Logging::FileLogger(cfg.filename().c_str(),
                                         cfg.log_level(),
                                         cfg.aspects().c_str(),
                                         &logger_policies);
    }

    inline
    El::XML::EntityResolver*
    create_entity_resolver(const Server::Config::EntityResolverType& resolver)
      throw(El::Exception)
    {
      El::XML::EntityResolver::NetStrategy net_strategy;

      if(resolver.net_strategy().present())
      {
        const Server::Config::EntityResolverType::net_strategy_type& ns =
          *resolver.net_strategy();

        net_strategy.connect_timeout.reset(
          new ACE_Time_Value(ns.connect_timeout()));
        
        net_strategy.send_timeout.reset(
          new ACE_Time_Value(ns.send_timeout()));
        
        net_strategy.recv_timeout.reset(
            new ACE_Time_Value(ns.recv_timeout()));
        
        net_strategy.redirects_to_follow = ns.redirects_to_follow();
      }
        
      El::XML::EntityResolver::FileStrategy file_strategy;

      if(resolver.file_strategy().present())
      {
        const Server::Config::EntityResolverType::file_strategy_type& fs =
          *resolver.file_strategy();

        file_strategy.cache_dir = fs.cache_dir();
        file_strategy.cache_timeout = fs.cache_timeout();
        file_strategy.cache_retry_period = fs.cache_retry_period();
        file_strategy.max_file_size = fs.max_file_size();
        file_strategy.cache_clean_period = fs.cache_clean_period();
        file_strategy.cache_file_expire = fs.cache_file_expire();
      }

      return new El::XML::EntityResolver(net_strategy, file_strategy); 
    }

    inline
    El::Net::HTTP::RobotsChecker*
    create_robots_checker(const Server::Config::RobotsCheckerType& cfg)
      throw(El::Exception)
    {
      return
        new El::Net::HTTP::RobotsChecker(ACE_Time_Value(cfg.timeout()),
                                         cfg.redirects_to_follow(),
                                         ACE_Time_Value(cfg.entry_timeout()),
                                         ACE_Time_Value(cfg.cleanup_period()));
    }

    inline
    El::Python::Sandbox*
    create_python_sandbox(const Server::Config::PythonSandboxOptionsType& cfg,
                          El::Python::Sandbox::Callback* callback)
      throw(El::Exception)
    {
      std::string safe_modules = cfg.safe_modules().present() ?
        *cfg.safe_modules() : El::Python::Sandbox::SAFE_MODULES;

      std::string safe_builtins = cfg.safe_builtins().present() ?
        *cfg.safe_builtins() : El::Python::Sandbox::SAFE_BUILTINS;
         
      return new El::Python::Sandbox(
        safe_modules.c_str(),
        safe_builtins.c_str(),
        cfg.max_ticks(),
        ACE_Time_Value(cfg.timeout()),
        cfg.call_max_depth(),
        cfg.max_memory(),
        callback);
    }
  }
}

#endif // _NEWSGATE_SERVER_XSD_CONFIGPARSER_HPP_
