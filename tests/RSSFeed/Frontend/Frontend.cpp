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

#include <El/CORBA/Corba.hpp>

#include <sstream>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/Net/HTTP/Headers.hpp>
#include <El/Net/HTTP/StatusCodes.hpp>

#include <El/Logging/StreamLogger.hpp>

#include "Frontend.hpp"

EL_APACHE_MODULE_INSTANCE(NewsGate::RSS::Frontend, rss_frontend_module);

namespace Aspect
{
  const char FRONTEND[] = "RSSFrontend";
}

namespace NewsGate
{
  namespace RSS
  {

    Frontend::Frontend(El::Apache::ModuleRef& mod_ref) throw(El::Exception)
        : El::Apache::Module<Config>(mod_ref, true, APR_HOOK_FIRST, "RSS_URI")
    {
      register_directive("RSS_LogLevel",
                         "numeric:7,10",
                         "RSS_LogLevel <log level - numeric:7,10>.",
                         1,
                         1);
      
      register_directive("RSS_FeedIOR",
                         "string:nws",
                         "RSS_FeedIOR <str>.",
                         1,
                         1);
    }

    void
    Frontend::directive(const El::Directive& directive,
                         Config& config)
      throw(El::Exception)
    {
      const El::Directive::Arg& arg = directive.arguments[0];
        
      if(directive.name == "RSS_LogLevel")
      {
        config.log_level = arg.numeric();
      }
      else
      {
        config.rss_feed_ior = arg.string();
      } 
    }

    void
    Frontend::child_init(Config& conf) throw(El::Exception)
    {
      if(logger_.get())
      {
        throw Exception(
          "Frontend::child_init: only one configuration expected");
      }
      
      try
      {
        logger_.reset(
          new El::Logging::StreamLogger(std::cerr, conf.log_level));
      
        try
        {
          orb_ = ::CORBA::ORB::_duplicate(
            El::Corba::Adapter::orb_adapter(0, 0)->orb());      

          CORBA::Object_var obj = orb_->string_to_object(
            conf.rss_feed_ior.c_str());
        
          if (CORBA::is_nil(obj.in()))
          {
            std::ostringstream ostr;
            ostr << "string_to_object gives null reference. Ior: "
                 << conf.rss_feed_ior;
        
            throw Exception(ostr.str());
          }
        
          rss_feed_ = FeedService::_narrow (obj.in ());
        
          if (CORBA::is_nil(rss_feed_.in()))
          {
            std::ostringstream ostr;
            ostr << "_narrow() gives null reference. Ior: "
                 << conf.rss_feed_ior;
        
            throw Exception(ostr.str());
          }

          logger()->info("Frontend::init: frontend is running ...",
                         Aspect::FRONTEND);        
        }
        catch (const CORBA::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "Frontend::child_init: got CORBA::Exception. Description: "
               << e;
        
          throw Exception(ostr.str());
        }
      } 
      catch (const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "Frontend::child_init: El::Exception caught. Description:"
               << std::endl << e;
        
          logger()->emergency(ostr.str(), Aspect::FRONTEND);
        }
        catch(...)
        {
        }
      }
      
    }
      
    void
    Frontend::child_cleanup() throw(El::Exception)
    {
      try
      {
        El::Corba::Adapter::orb_adapter_cleanup();
        
        logger()->info("Frontend::child_cleanup: frontend terminated",
                       Aspect::FRONTEND);
      }
      catch(...)
      {
      }
    }    
    
    int
    Frontend::handler(Context& context) throw(El::Exception)
    {
      try
      {        
        try
        {
          logger()->trace("Frontend::handler: entered",
                          Aspect::FRONTEND,
                          El::Logging::MIDDLE);
        
          NewsGate::RSS::FeedService::Request req;

          const char* feed_name = strrchr(context.request.uri(), '/');

          if(feed_name == 0)
          {
            std::ostringstream ostr;
            ostr << "NewsGate::RSS::Frontend::handler: invalid url "
                 << context.request.uri();
            
            throw Exception(ostr.str());
          }
            
          req.feed_name = CORBA::string_dup(++feed_name);
          
          const El::Net::HTTP::HeaderList& headers =
            context.request.in().headers();

          for(El::Net::HTTP::HeaderList::const_iterator it =
                headers.begin(); it != headers.end(); it++)
          {
            if(!strcasecmp(it->name.c_str(),
                           El::Net::HTTP::HD_IF_MODIFIED_SINCE))
            {
              req.if_modified_since = it->value.c_str();
            }
            else if(!strcasecmp(it->name.c_str(),
                                El::Net::HTTP::HD_IF_NONE_MATCH))
            {
              req.if_none_match = it->value.c_str();
            }
          }
          
          NewsGate::RSS::FeedService::Response_var res;
          rss_feed_->get(req, res.out());

          const char* body = res->body.in();
          unsigned long len = strlen(body);

          El::Apache::Request::Out& out = context.request.out();

          out.content_type("text/xml");

          if(*res->etag.in() != '\0')
          {
            out.send_header(El::Net::HTTP::HD_ETAG, res->etag.in());
          }

          if(*res->last_modified.in() != '\0')
          {
            out.send_header(El::Net::HTTP::HD_LAST_MODIFIED,
                            res->last_modified.in());
          }

          std::ostream& out_stream = out.stream();

          if(res->chunked)
          {
            out.send_header(El::Net::HTTP::HD_TRANSFER_ENCODING,
                            "chunked");

            unsigned long write_bytes = 0;
            
            do
            {
              write_bytes = len < res->chunked ? len : res->chunked;

              out_stream << std::hex << write_bytes << std::dec
                         << ";A=1; B=2\r\n";

              if(write_bytes)
              {
                out_stream.write(body, write_bytes);
                out_stream << "\r\n";
              }
              
              body += write_bytes;
              len -= write_bytes;
            }
            while(write_bytes > 0);

            out_stream << "Trailer1:010\r\nTrailer2:020\r\n"
              "Trailer3:030\r\n\r\n";
          }
          else
          {
            if(res->content_length)
            {
              std::ostringstream ostr;
              ostr << len;
              out.send_header(El::Net::HTTP::HD_CONTENT_LENGTH,
                              ostr.str().c_str());
            }

            out_stream.write(body, len);
          }

          return res->status_code == El::Net::HTTP::SC_OK ?
            OK : res->status_code;
        }
        catch (const NewsGate::RSS::FeedService::UnknownFeed& e)
        {
          return HTTP_NOT_FOUND;
        }
        catch (const CORBA::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::RSS::Frontend::handle: got CORBA::Exception. "
            "Description: " << e;
        
          logger()->emergency(ostr.str(), Aspect::FRONTEND);
        
          return HTTP_INTERNAL_SERVER_ERROR;
        }
        catch (const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::RSS::Frontend::handle_request: "
            "got El::Exception. Description: " << e;
        
          logger()->emergency(ostr.str(), Aspect::FRONTEND);
        
          return HTTP_INTERNAL_SERVER_ERROR;
        }
      }
      catch(...)
      {
        logger()->emergency("NewsGate::RSS::Frontend::handle_request: "
                            "unknown exception caught",
                            Aspect::FRONTEND);

        return HTTP_INTERNAL_SERVER_ERROR;
      }      
    }
  
  }
}
