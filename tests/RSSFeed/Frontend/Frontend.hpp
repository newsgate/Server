/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/tests/RSSFeed/Frontend/Frontend.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_TESTS_RSSFEED_FRONTEND_FRONTEND_HPP_
#define _NEWSGATE_SERVER_TESTS_RSSFEED_FRONTEND_FRONTEND_HPP_

#include <memory>
#include <string>

#include <El/Exception.hpp>
#include <El/Apache/Module.hpp>

#include <El/Logging/Logger.hpp>
#include <El/Logging/StreamLogger.hpp>

#include <tests/RSSFeed/Service/RSSFeed.hpp>

namespace NewsGate
{
  namespace RSS
  {
    struct FrontendConfig
    {
      static El::Apache::ModuleRef mod_ref;
  
      unsigned long log_level;
      std::string rss_feed_ior;

      FrontendConfig() throw();
      FrontendConfig(const FrontendConfig& cf_base,
                     const FrontendConfig& cf_new) throw();
    };
    
    class Frontend : public El::Apache::Module<FrontendConfig>
    {
    public:
      Frontend(El::Apache::ModuleRef& mod_ref) throw(El::Exception);
  
      virtual void directive(const El::Directive& directive,
                             Config& config)
        throw(El::Exception);

      virtual int handler(Context& context) throw(El::Exception);

      virtual void child_init(Config& conf) throw(El::Exception);      
      virtual void child_cleanup() throw(El::Exception);
    
    private:
    
      EL_EXCEPTION(Exception, El::ExceptionBase);

      El::Logging::Logger* logger() const throw();

    public:
      virtual ~Frontend() throw();
      
    private:    
      CORBA::ORB_var orb_;
      FeedService_var rss_feed_;

      El::Logging::LoggerPtr logger_;
    };
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace RSS
  { 
    //
    // FrontendConfig class
    //
    inline
    FrontendConfig::FrontendConfig() throw()
        : log_level(0)
    {
    }
    
    inline
    FrontendConfig::FrontendConfig(const FrontendConfig& cf_base,
                                   const FrontendConfig& cf_new) throw()
        : log_level(cf_new.log_level ? cf_new.log_level : cf_base.log_level),
          rss_feed_ior(cf_new.rss_feed_ior.empty() ?
                       cf_base.rss_feed_ior : cf_new.rss_feed_ior)
    {
    }
    
    //
    // Frontend class
    //
    inline
    Frontend::~Frontend() throw()
    {
    }

    inline
    El::Logging::Logger*
    Frontend::logger() const throw()
    {
      return logger_.get();
    }
  
  }
}
  
#endif // _NEWSGATE_SERVER_TESTS_RSSFEED_FRONTEND_FRONTEND_HPP_
