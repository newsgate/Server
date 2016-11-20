/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   xsd/DataFeed/RSS/Parser.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_XSD_DATAFEED_RSS_PARSER_HPP_
#define _NEWSGATE_SERVER_XSD_DATAFEED_RSS_PARSER_HPP_

#include <iostream>
#include <sstream>
#include <memory>

#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/SAXParseException.hpp>

#include <El/XML/EntityResolver.hpp>

#include <xsd/cxx/exceptions.hxx>
#include <xsd/cxx/xml/string.hxx>

#include <xsd/DataFeed/RSS/Data.hpp>

namespace NewsGate
{ 
  namespace RSS
  {
    class Parser
    {
    public:
      EL_EXCEPTION(Exception, RSS::Exception);
      EL_EXCEPTION(EncodingError, Exception);

      class Interceptor
      {
      public:
        virtual void post_last_build_date(Channel& channel)
          throw(El::Exception) = 0;
        
        virtual void post_item(Channel& channel) throw(El::Exception) = 0;

        virtual ~Interceptor() throw() {}
      };
      
    public:

      Parser(Interceptor* interceptor,
             Channel* delegated_channel) throw(El::Exception);
      
      virtual ~Parser() throw() {}

      virtual void parse_file(
        const char* name,
        bool validate,
        El::XML::EntityResolver* entity_resolver)
        throw(EncodingError,
              Exception,
              El::Exception,
              xsd::cxx::exception) = 0;

      virtual void parse(
        std::istream& is,
        const char* feed_url,
        const char* charset,
        unsigned long max_read_portion,
        bool validate,
        El::XML::EntityResolver* entity_resolver)
        throw(EncodingError,
              Exception,
              El::Exception,
              xsd::cxx::exception) = 0;

      const Channel& rss_channel() const throw();
      Channel& rss_channel() throw();

      void last_build_date(const El::Moment& moment) throw(El::Exception);
      void item_pub_date(const El::Moment& time) throw(El::Exception);

    protected:
      
      class ErrorHandler : public xercesc::ErrorHandler
      {
      public:
        EL_EXCEPTION(Exception, RSS::Parser::Exception);
      
      public:
        virtual void warning(const xercesc::SAXParseException& exc) throw ();
      
        virtual void error(const xercesc::SAXParseException& exc)
          throw(EncodingError, Exception, El::Exception);
      
        virtual void fatalError(const xercesc::SAXParseException& exc)
          throw(EncodingError, Exception, El::Exception);
      
        virtual void resetErrors() throw ();

      private:

        static void throw_exception(const xercesc::SAXParseException& exc)
          throw(EncodingError, Exception);
      };
    
    protected:
      Interceptor* interceptor_;
      Channel own_channel_;
      Channel* delegated_channel_;
      Item item_;
      uint64_t time_;
    };

    typedef std::auto_ptr<Parser> ParserPtr;
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
    // Parser class
    //
  
    inline
    Parser::Parser(Interceptor* interceptor,
                   Channel* delegated_channel) throw(El::Exception)
        : interceptor_(interceptor),
          delegated_channel_(delegated_channel),
          time_(0)
    {
    }

    inline
    const Channel&
    Parser::rss_channel() const throw()
    {
      return delegated_channel_ ? *delegated_channel_ : own_channel_;
    }
    
    inline
    Channel&
    Parser::rss_channel() throw()
    {
      return delegated_channel_ ? *delegated_channel_ : own_channel_;
    }

    inline
    void
    Parser::last_build_date(const El::Moment& moment) throw(El::Exception)
    {
      if(time_ == 0)
      {
        time_ = ACE_OS::gettimeofday().sec();
      }

      uint64_t lbd = std::min((uint64_t)((ACE_Time_Value)moment).sec(), time_);
      Channel& channel = rss_channel();
      
      for(ItemList::const_iterator i(channel.items.begin()),
            e(channel.items.end()); i != e; ++i)
      {
        uint64_t pub = i->code.published;
          
        if(lbd < pub)
        {
          lbd = pub;
        }
      }

      channel.last_build_date = El::Moment(ACE_Time_Value(lbd));      
    }
    
    inline
    void
    Parser::item_pub_date(const El::Moment& time) throw(El::Exception)
    {
      if(time_ == 0)
      {
        time_ = ACE_OS::gettimeofday().sec();
      }

      item_.code.published =
        std::min((uint64_t)((ACE_Time_Value)time).sec(), time_);

      Channel& channel = rss_channel();
      
      if(channel.last_build_date != El::Moment::null)
      {
        uint64_t lbd = ((ACE_Time_Value)channel.last_build_date).sec();

        if(lbd < item_.code.published)
        {
          channel.last_build_date =
            El::Moment(ACE_Time_Value(item_.code.published));
        }
      }
    }
    
    //
    // Parser::ErrorHandler class
    //

    inline
    void
    Parser::ErrorHandler::warning(const xercesc::SAXParseException& exc)
      throw ()
    {
    }
  
    inline
    void
    Parser::ErrorHandler::error(const xercesc::SAXParseException& e)
      throw(EncodingError, Exception, El::Exception)
    {
      throw_exception(e);      
    }  

    inline
    void
    Parser::ErrorHandler::fatalError(const xercesc::SAXParseException& e)
      throw(EncodingError, Exception, El::Exception)
    {
      throw_exception(e);      
    }

    inline
    void
    Parser::ErrorHandler::throw_exception(
      const xercesc::SAXParseException& e)
      throw(EncodingError, Exception)
    {
      std::string msg = xsd::cxx::xml::transcode<char>(e.getMessage());
      
      std::ostringstream ostr;
      ostr << "Error: " << msg;

      if(msg.find("UTFDataFormatException") != std::string::npos ||
         msg.find("TranscodingException") != std::string::npos ||
         msg.find("encoding") != std::string::npos)
      {
        throw EncodingError(ostr.str());
      }
      else
      {
        throw Exception(ostr.str());
      } 
    }
    
    inline
    void
    Parser::ErrorHandler::resetErrors() throw ()
    {
    }    
  }
}

#endif // _NEWSGATE_SERVER_XSD_DATAFEED_RSS_PARSER_HPP_
