/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/FeedManager/MessageAdjustment.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <Python.h>

#include <El/CORBA/Corba.hpp>

#include <string>
#include <sstream>
#include <memory>

#include <ace/OS.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/HTMLparser.h>
#include <libxml/HTMLtree.h>
#include <libxml/xpath.h>

#include <El/Exception.hpp>
#include <El/Guid.hpp>
#include <El/Net/HTTP/Session.hpp>
#include <El/Net/HTTP/StatusCodes.hpp>
#include <El/Net/HTTP/Headers.hpp>
#include <El/Logging/StreamLogger.hpp>
#include <El/HTML/LightParser.hpp>

#include <El/LibXML/HTMLParser.hpp>
#include <El/LibXML/Traverser.hpp>

#include <xsd/DataFeed/RSS/Data.hpp>
#include <xsd/DataFeed/RSS/ParserFactory.hpp>
#include <xsd/DataFeed/RSS/HTMLFeedParser.hpp>

#include <Services/Moderator/Commons/TransportImpl.hpp>

#include <xsd/ConfigParser.hpp>

#include "FeedManagerMain.hpp"
#include "MessageAdjustment.hpp"

namespace Aspect
{
  const char MSG_ADJUSTMENT[] = "MsgAdjustment";
}

namespace NewsGate
{
  namespace Moderation
  {
    typedef std::auto_ptr<El::Net::HTTP::Session> HTTPSessionPtr;

    //
    // MessageAdjustment class
    //
    MessageAdjustment::MessageAdjustment() throw(El::Exception)
    {
      const Server::Config::FeedManagerType& config =
        Application::instance()->config();

      {
        std::string extensions =
          config.image().extension_whitelist();

        El::String::ListParser parser(extensions.c_str());
        
        const char* ext;
        while((ext = parser.next_item()) != 0)
        {
          std::string file_ext;
          El::String::Manip::to_lower(ext, file_ext);

          if(!file_ext.empty())
          {
            image_extension_whitelist_.insert(file_ext);
          }
        }        
      }
      
      {
        std::string prefixes = config.image().prefix_blacklist();

        El::String::ListParser parser(prefixes.c_str());
        
        const char* prefix;
        while((prefix = parser.next_item()) != 0)
        {
          image_prefix_blacklist_.push_back(prefix);
        }
      }

      size_t timeout = config.python().sandbox().timeout();
      
      sandbox_service_ =
        new El::Python::SandboxService(
          this,
          "FeedManagerSandboxService",
          config.python().sandbox().processes(),
//        timeout ? (timeout + 1) * 1000 : 0,
          // Process-level timeout doubled to ensure python-level timeout
          // works before
          timeout ? timeout * 2000 : 0,
          "libFeedParsing.so");

      sandbox_service_->start();
    }

    MessageAdjustment::~MessageAdjustment() throw()
    {
      stop();
    }

    void
    MessageAdjustment::stop() throw()
    {
      sandbox_service_->stop();
      sandbox_service_->wait();
    }
    
    bool
    MessageAdjustment::notify(El::Service::Event* event) throw(El::Exception)
    {
      El::Service::log(event, "MessageAdjustment::notify: ",
                       Application::logger(),
                       Aspect::MSG_ADJUSTMENT);

      return true;
    }
      
    bool
    MessageAdjustment::interrupt_execution(std::string& reason)
      throw(El::Exception)
    {
      return false;
    }
      
    
    El::Net::HTTP::Session*
    MessageAdjustment::start_session(const char* url,
                                     std::string* permanent_url)
      throw(El::Exception)
    {
      const Server::Config::FeedManagerType& config =
        Application::instance()->config();
      
      El::Net::HTTP::HeaderList headers;

      headers.add(El::Net::HTTP::HD_ACCEPT, "*/*");
      headers.add(El::Net::HTTP::HD_ACCEPT_ENCODING, "gzip,deflate");
      headers.add(El::Net::HTTP::HD_ACCEPT_LANGUAGE, "en-us,*");
      
      headers.add(El::Net::HTTP::HD_USER_AGENT,
                  config.user_agent().c_str());
            
      HTTPSessionPtr session(new El::Net::HTTP::Session(url));
            
      ACE_Time_Value timeout(config.request_timeout());
      session->open(&timeout, &timeout, &timeout);
            
      std::string perm_url =
        session->send_request(El::Net::HTTP::GET,
                              El::Net::HTTP::ParamList(),
                              headers,
                              0,
                              0,
                              config.redirects_to_follow());

      if(permanent_url)
      {
        *permanent_url = perm_url;
      }

      session->recv_response_status();
      return session.release();
    }
   
    char*
    MessageAdjustment::xpath_url(const char * xpath,
                                 const char * url)
      throw(NewsGate::Moderation::OperationFailed,
            NewsGate::Moderation::ImplementationException,
            CORBA::SystemException)
    { 
      const Server::Config::FeedManagerType& config =
        Application::instance()->config();
        
      std::string result;
      std::string temp_file_path;
      
      try
      {
        HTTPSessionPtr session(start_session(url));
      
        if(session->status_code() != El::Net::HTTP::SC_OK)
        {
          std::ostringstream ostr;
          ostr << "Unexpected response status " << session->status_code()
               <<  " for " << url;
        
          throw Exception(ostr.str());        
        }

        std::string encoding;
        
        El::Net::HTTP::Header header;
        while(session->recv_response_header(header))
        {
          if(encoding.empty() &&
             strcasecmp(header.name.c_str(),
                        El::Net::HTTP::HD_CONTENT_TYPE) == 0)
          {
            El::Net::HTTP::content_type(header.value.c_str(), encoding);
          }
        }
 
        {
          El::Guid guid;
          guid.generate();
          
          std::ostringstream ostr;
          ostr << config.temp_dir() << "/XPath."
               << guid.string(El::Guid::GF_DENSE);
          
          temp_file_path = ostr.str();
        }

        session->save_body(temp_file_path.c_str(),
                           config.xpath().max_size(),
                           0,
                           encoding.c_str());

        El::LibXML::ErrorRecorderHandler error_handler;
        El::LibXML::HTMLParser parser;

        htmlDocPtr doc = parser.parse_file(temp_file_path.c_str(),
                                           encoding.c_str(),
                                           &error_handler,
                                           HTML_PARSE_NONET);

        unlink(temp_file_path.c_str());
        temp_file_path.clear();
        
        xmlXPathContextPtr xpc = xmlXPathNewContext(doc);
      
        if(xpc == 0)
        {
          std::ostringstream ostr;
          ostr << "xmlXPathNewContext failed";
          throw Exception(ostr.str());      
        }
      
        xmlXPathObjectPtr xpath_result =
          xmlXPathEvalExpression((xmlChar*)xpath, xpc);
      
        xmlXPathFreeContext(xpc);
      
        if(xpath_result == 0)
        {
          std::ostringstream ostr;
          ostr << "xmlXPathEvalExpression failed";
          throw Exception(ostr.str());      
        }
      
        std::ostringstream ostr;
        xmlNodeSetPtr node_set = xpath_result->nodesetval;

        try
        {
          if(xmlXPathNodeSetIsEmpty(node_set))
          {
            ostr << "Empty result set for XPath '" << xpath << "'\n";
          }
          else
          {
            for(int i = 0; i < node_set->nodeNr; ++i)
            {
              xmlNodePtr node = node_set->nodeTab[i];
            
              ostr << i + 1 << ") ";
            
              El::LibXML::Traverser traverser;
              
              El::LibXML::TextBuilder builder(ostr,
                                              El::LibXML::TextBuilder::OT_XML);
              
              traverser.traverse(node, builder);
            
              ostr << std::endl;
            }
          }
        }
        catch(...)
        {        
          xmlXPathFreeObject(xpath_result);
          throw;
        }
      
        xmlXPathFreeObject(xpath_result);

        ostr << "\nRobots checker: access "
             << (Application::instance()->robots_checker().allowed(
                   session->url(),
                   config.user_agent().c_str()) ? "allowed" : "disallowed");

        ostr << "\nHTML Parsing Errors:\n\n";
        error_handler.dump(ostr);
          
        result = ostr.str();
      }
      catch(const El::Exception& e)
      {
        if(!temp_file_path.empty())
        {
          unlink(temp_file_path.c_str());
        }
        
        NewsGate::Moderation::OperationFailed ex;
        ex.reason = CORBA::string_dup(e.what());
        throw ex;
      }
      
      return CORBA::string_dup(result.c_str());
    }
    
    void
    MessageAdjustment::adjust_message(
      const char* adjustment_script,
      ::NewsGate::Moderation::Transport::MsgAdjustmentContext* ctx,
      ::NewsGate::Moderation::Transport::MsgAdjustmentResult_out result)
      throw(NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      try
      {
        Moderation::Transport::MsgAdjustmentContextImpl::Type*
          context = dynamic_cast<
          Moderation::Transport::MsgAdjustmentContextImpl::Type*>(ctx);
        
        if(context == 0)
        {
          throw Exception(
            "MessageAdjustment::adjust_message: dynamic_cast<"
            "NewsGate::Moderation::Transport::MsgAdjustmentContextImpl::"
            "Type*> failed");
        }

        const Server::Config::FeedManagerType& config =
          Application::instance()->config();

        Moderation::Transport::MsgAdjustmentResultImpl::Var res =
          Moderation::Transport::MsgAdjustmentResultImpl::Init::create(
          new Moderation::Transport::MsgAdjustmentResultStruct());

        Moderation::Transport::MsgAdjustmentResultStruct& adjustment_result =
          res->entity();

        adjustment_result.message = context->entity().message;
        
        RSS::MsgAdjustment::Context ctx(
          adjustment_result.message,
          context->entity().item,
          false,
          context->entity().encoding.c_str());
        
        El::Net::HTTP::RequestParams& rp = ctx.request_params;
        rp.user_agent = config.user_agent();
        rp.referer = config.image().referer();
        rp.timeout = config.request_timeout();
        rp.redirects_to_follow = config.redirects_to_follow();
        rp.recv_buffer_size = config.image().socket_recv_buf_size();

        ctx.message_restrictions.max_title_len =
          config.parsing().limits().message_title();
        
        ctx.message_restrictions.max_desc_len =
          config.parsing().limits().message_description();

        ctx.message_restrictions.max_desc_chars =
          config.parsing().limits().message_description_chars();

        ctx.message_restrictions.max_image_count =
          config.parsing().limits().max_image_count();
        
        Message::Automation::ImageRestrictions& ir =
          ctx.message_restrictions.image_restrictions;
        
        ir.min_width = config.image().min_width();
        ir.min_height = config.image().min_height();
        ir.black_list = image_prefix_blacklist_;
        ir.extensions = image_extension_whitelist_;        

        std::ostringstream ostr;
        
        El::Logging::StreamLogger
          logger(ostr, El::Logging::TRACE + El::Logging::HIGH);

        ::NewsGate::Feed::Automation::Item& item = ctx.item;
        
        item.article_max_size = config.full_article().max_size();
        item.cache_dir = config.temp_dir();

        Message::Automation::Message& message = ctx.message;
        
        try
        {
          RSS::MsgAdjustment::Code code;

          std::string trimmed;
          El::String::Manip::trim(adjustment_script, trimmed);
            
          if(!trimmed.empty())
          {
            try
            {
              code.compile(adjustment_script,
                           ctx.item.feed_id,
                           ctx.message.source.url.c_str());
            }
            catch(...)
            {
              // As didn't managed to check the validity yet will
              // consider as valid
              message.valid = true;
              throw;
            }
          }
          
          try
          {
            std::ostringstream ostr;
              
            item.article_session.reset(
              ctx.normalize_message_url(message.url, 0, ostr));

            if(item.article_session.get())
            {
              message.valid = true;

              // Session can be created but closed if schema (like https) is
              // unsupported. This way normalize_message_url reports that link
              // is ok, but we can't go inside the document
              if(!item.article_session->opened())
              {
                item.article_session.reset(0);
              }
            }
            else
            {
              std::string log;
              El::String::Manip::trim(ostr.str().c_str(), log);

              std::ostringstream ostr;
              ostr << "MessageAdjustment::adjust_message: "
                "normalize_message_url failed. Reason:\n" << log;
                
              throw Exception(ostr.str());
            }              
            
            if(!code.compiled() || (config.full_article().obey_robots_txt() &&
               item.article_session.get() &&
               !Application::instance()->robots_checker().allowed(
                 item.article_session->url(),
                 config.user_agent().c_str())))
            {
              item.article_session.reset(0);
            }
          }
          catch(const El::Exception& e)
          {
            item.article_session.reset(0);
            
            std::ostringstream ostr;
            ostr << "Checking article availability error:\n"
                 << e;
              
            logger.warning(ostr.str(), Aspect::MSG_ADJUSTMENT);
          }

//          if(message.valid)
// Run regardles of message validity to check if there are
// runtime errors on the cover
          if(code.compiled())
          {
            std::auto_ptr<El::Python::Sandbox> sandbox(
              Config::create_python_sandbox(
                Application::instance()->config().python().sandbox()));
            
//            code.run(ctx, sandbox.get(), &logger);
            code.run(ctx, sandbox_service_.in(), sandbox.get(), &logger);
          }
        }
        catch(const El::Exception& e)
        {
          adjustment_result.error = e.what();
        }

        Message::Automation::ImageArray& images = message.images;
        size_t image_count = ctx.message_restrictions.max_image_count;

        for(Message::Automation::ImageArray::iterator i(images.begin()),
              e(images.end()); i != e; ++i)
        {
          Message::Automation::Image& img = *i;
          
          if(img.status == ::NewsGate::Message::Automation::Image::IS_VALID)
          {
            for(Message::Automation::ImageArray::iterator j(images.begin());
                j != i; ++j)
            {
              Message::Automation::Image& img2 = *j;
              
              if(img.src == img2.src &&
                 img2.status == Message::Automation::Image::IS_VALID)
              {
                img.status = Message::Automation::Image::IS_DUPLICATE;
                break;
              }
            }
          }
          
          if(img.status != Message::Automation::Image::IS_VALID)
          {
            continue;
          }

          if(!image_count)
          {
            img.status = Message::Automation::Image::IS_TOO_MANY;
            continue;
          }          

          if(img.height == Message::Automation::Image::DIM_UNDEFINED ||
             img.width == Message::Automation::Image::DIM_UNDEFINED)
          {
            img.read_size(rp);
          }
          
          if(img.height < ir.min_height || img.width < ir.min_width)
          {
            img.status = Message::Automation::Image::IS_TOO_SMALL;
            continue;
          }
          
          --image_count;
        }
/*
        for(Message::Automation::ImageArray::iterator i(images.begin()),
              e(images.end()); i != e; ++i)
        {
          Message::Automation::Image& img = *i;
          std::cerr << img.status << " : " << img.src << std::endl;
        }

        std::cerr << std::endl;
*/
        std::wstring wlog;

        El::String::Manip::utf8_to_wchar(ostr.str().c_str(),
                                         wlog,
                                         true,
                                         El::String::Manip::UAC_XML_1_0);
        
        El::String::Manip::wchar_to_utf8(wlog.c_str(), adjustment_result.log);

        result = res._retn();
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "MessageAdjustment::adjust_message: El::Exception caught. "
          "Description: " << e;
        
        Moderation::ImplementationException ex;        
        ex.description = ostr.str().c_str();

        throw ex;
      }
    }

    ::NewsGate::Moderation::Transport::MsgAdjustmentContextPack*
    MessageAdjustment::get_feed_items(const char* url,
                                      ::CORBA::ULong type_val,
                                      ::CORBA::ULong space_val,
                                      ::CORBA::ULong country,
                                      ::CORBA::ULong lang,
                                      const char* encoding)
      throw(NewsGate::Moderation::OperationFailed,
            NewsGate::Moderation::ImplementationException,
            CORBA::SystemException)
    {
      NewsGate::Feed::Type tp;
      NewsGate::Feed::Space sp;
      El::Country ct;
      El::Lang lg;

      try
      {
        tp = NewsGate::Feed::type(type_val);
        sp = NewsGate::Feed::space(space_val);
        ct = El::Country(El::Country::el_code(country));
        lg = El::Lang(El::Lang::el_code(lang));
      }
      catch(const El::Exception& e)
      {
        NewsGate::Moderation::OperationFailed ex;
        ex.reason = CORBA::string_dup(e.what());
        throw ex;
      }

      try
      {
        if(encoding && *encoding != '\0')
        {
          return get_feed_items(url, tp, sp, ct, lg, false, encoding);
        }
        else
        {
          try
          {
            return get_feed_items(url, tp, sp, ct, lg, true, 0);
          }
          catch(const RSS::Parser::EncodingError& e)
          {
            // Will try without HTTP-level encoding specification
          }

          return get_feed_items(url, tp, sp, ct, lg, false, 0);
        }
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "MessageAdjustment::get_feed_items: El::Exception caught. "
          "Description: " << e;
        
        Moderation::ImplementationException ex;        
        ex.description = ostr.str().c_str();

        throw ex;
      }      
    }
    
    ::NewsGate::Moderation::Transport::MsgAdjustmentContextPack*
    MessageAdjustment::get_feed_items(const char* feed_url,
                                      NewsGate::Feed::Type feed_type,
                                      NewsGate::Feed::Space feed_space,
                                      const El::Country& feed_country,
                                      const El::Lang& feed_lang,
                                      bool use_http_charset,
                                      const char* encoding)
      throw(Exception,
            RSS::Parser::EncodingError,
            El::Exception,
            NewsGate::Moderation::OperationFailed,
            NewsGate::Moderation::ImplementationException,
            CORBA::SystemException)
    {
      const Server::Config::FeedManagerType& config =
        Application::instance()->config();
      
      RSS::ParserPtr parser(RSS::ParserFactory::create(Feed::TP_UNDEFINED, 0));

      std::string url;
      std::string new_permanent_url;
      
      try
      {
        HTTPSessionPtr session(start_session(feed_url, &new_permanent_url));
      
        if(session->status_code() != El::Net::HTTP::SC_OK)
        {
          std::ostringstream ostr;
          ostr << "Unexpected response status " << session->status_code()
               <<  " for " << feed_url;
        
          throw Exception(ostr.str());        
        }

        if(!new_permanent_url.empty())
        {
          feed_url = new_permanent_url.c_str();
        }
        
        url = session->url()->string();

        std::string charset;
        
        El::Net::HTTP::Header header;              
        while(session->recv_response_header(header))
        {
          if(strcasecmp(header.name.c_str(),
                        El::Net::HTTP::HD_CONTENT_TYPE) == 0)
          {
            El::Net::HTTP::content_type(header.value.c_str(), charset);
          }      
        }
        
        std::auto_ptr<El::XML::EntityResolver> entity_resolver;
        
        if(config.parsing().entity_resolver().present())
        {
          entity_resolver.reset(
            NewsGate::Config::create_entity_resolver(
              *config.parsing().entity_resolver()));
        }
            
        parser->parse(session->response_body(),
                      url.c_str(),
                      encoding && *encoding != '\0' ? encoding :
                      (use_http_charset ? charset.c_str() : 0),
                      1024 * 10,
                      false,
                      entity_resolver.get());
      }
      catch(const RSS::Parser::EncodingError& e)
      {
        if(use_http_charset)
        {
          throw;
        }

        NewsGate::Moderation::OperationFailed ex;
        ex.reason = CORBA::string_dup(e.what());
        throw ex;
      }
      catch(const El::Exception& e)
      {
        NewsGate::Moderation::OperationFailed ex;
        ex.reason = CORBA::string_dup(e.what());
        throw ex;
      }

      RSS::Channel& channel = parser->rss_channel();
      
      try
      {
        std::wstring title;
        El::String::Manip::utf8_to_wchar(channel.title.c_str(), title);
            
        El::HTML::LightParser html_parser;
        
        html_parser.parse(title.c_str(),
                          url.c_str(),
                          El::HTML::LightParser::PF_LAX,
                          config.parsing().limits().channel_title());
        
        El::String::Manip::compact(html_parser.text.c_str(),
                                   channel.title);
      }
      catch(const El::Exception& e)
      {
        channel.title.clear();
      }

      Moderation::Transport::MsgAdjustmentContextPackImpl::Var res =
        Moderation::Transport::MsgAdjustmentContextPackImpl::Init::create(
          new ::NewsGate::RSS::MsgAdjustment::ContextArray());
      
      ::NewsGate::RSS::MsgAdjustment::ContextArray& result =
          res->entities();

      channel.adjust(feed_url,
                     url.c_str(),
                     feed_country,
                     feed_lang,
                     &address_info_);
      
      for(RSS::ItemList::iterator i(channel.items.begin()),
            e(channel.items.end()); i != e; ++i)
      {
        result.push_back(RSS::MsgAdjustment::Context());
          
        RSS::MsgAdjustment::Context& context = *result.rbegin();        
        
        context.fill(*i,
                     channel,
                     feed_url,
                     encoding,
                     feed_space,
                     ACE_Time_Value(config.request_timeout()),
                     config.user_agent().c_str(),
                     config.redirects_to_follow(),
                     0,
                     config.image().referer().c_str(),
                     config.image().socket_recv_buf_size(),
                     config.image().min_width(),
                     config.image().min_height(),
                     image_prefix_blacklist_,
                     image_extension_whitelist_,
                     config.parsing().limits().message_title(),
                     config.parsing().limits().message_description(),
                     config.parsing().limits().message_description_chars(),
                     config.parsing().limits().max_image_count());
        
        context.item.article_session.reset(0);
      }
      
      return res._retn();
    }

    ::NewsGate::Moderation::Transport::GetHTMLItemsResult*
    MessageAdjustment::get_html_items(
      const char* url,
      const char* script,
      ::CORBA::ULong type_val,
      ::CORBA::ULong space_val,
      ::CORBA::ULong country,
      ::CORBA::ULong lang,
      const ::NewsGate::Moderation::KeywordsSeq& keywords,
      const char* cache,
      const char* encoding)
      throw(NewsGate::Moderation::OperationFailed,
            NewsGate::Moderation::ImplementationException,
            CORBA::SystemException)
    {
//      NewsGate::Feed::Type tp;
      NewsGate::Feed::Space sp;
      El::Country ct;
      El::Lang lg;

      try
      {
//        tp = NewsGate::Feed::type(type_val);
        sp = NewsGate::Feed::space(space_val);
        ct = El::Country(El::Country::el_code(country));
        lg = El::Lang(El::Lang::el_code(lang));
      }
      catch(const El::Exception& e)
      {
        NewsGate::Moderation::OperationFailed ex;
        ex.reason = CORBA::string_dup(e.what());
        throw ex;
      }
        
      try
      {
        Moderation::Transport::GetHTMLItemsResultImpl::Var res =
          Moderation::Transport::GetHTMLItemsResultImpl::Init::create(
            new Moderation::Transport::GetHTMLItemsResultStruct());

        Moderation::Transport::GetHTMLItemsResultStruct& result =
          res->entity();

        const Server::Config::FeedManagerType& config =
          Application::instance()->config();
        
        RSS::HTMLFeed::Context ctx;

        ctx.fill(url,
                 sp,
                 ct,
                 lg,
                 El::String::Array(),
                 &address_info_,
                 ACE_Time_Value(config.request_timeout()),
                 config.user_agent().c_str(),
                 config.redirects_to_follow(),
                 config.image().referer().c_str(),
                 config.image().socket_recv_buf_size(),
                 config.image().min_width(),
                 config.image().min_height(),
                 image_prefix_blacklist_,
                 image_extension_whitelist_,
                 config.parsing().limits().message_title(),
                 config.parsing().limits().message_description(),
                 config.parsing().limits().message_description_chars(),
                 config.parsing().limits().max_image_count(),
                 config.full_article().max_size(),
                 config.temp_dir().c_str(),
                 cache,
                 config.html_cache().max_size(),
                 config.html_cache().timeout(),
                 encoding);

        for(size_t i = 0; i < keywords.length(); ++i)
        {
          ctx.keywords.push_back(keywords[i].in());
        }
        
        std::ostringstream ostr;
        
        El::Logging::StreamLogger
          logger(ostr, El::Logging::TRACE + El::Logging::HIGH);

        try
        {
          RSS::HTMLFeed::Code code;

          std::string trimmed;
          El::String::Manip::trim(script, trimmed);
            
          if(trimmed.empty())
          {
            throw Exception("No script provided");
          }

          code.compile(script, 0, url);

          std::auto_ptr<El::Python::Sandbox> sandbox(
            Config::create_python_sandbox(
              Application::instance()->config().python().sandbox()));
            
          code.run(ctx, 0, sandbox_service_.in(), sandbox.get(), &logger);
        }
        catch(const El::Exception& e)
        {
          result.error = e.what();
        }

        std::wstring wlog;

        El::String::Manip::utf8_to_wchar(ostr.str().c_str(),
                                         wlog,
                                         true,
                                         El::String::Manip::UAC_XML_1_0);

        El::String::Manip::wchar_to_utf8(wlog.c_str(), result.log);

        for(Message::Automation::MessageArray::const_iterator
              i(ctx.messages.begin()), e(ctx.messages.end()); i != e; ++i)
        {
          result.messages.push_back(*i);
        }

        result.cache = ctx.cache.stringify().c_str();
        result.interrupted = ctx.interrupted;
        
        return res._retn();
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "MessageAdjustment::get_html_items: El::Exception caught. "
          "Description: " << e;
        
        Moderation::ImplementationException ex;        
        ex.description = ostr.str().c_str();

        throw ex;
      }      
    }    
  }  
}
