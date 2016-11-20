/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   xsd/DataFeed/RSS/SAXParser.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

//#define RSS_PARSER_TRACE

#ifndef _NEWSGATE_SERVER_XSD_DATAFEED_RSS_SAXPARSER_HPP_
#define _NEWSGATE_SERVER_XSD_DATAFEED_RSS_SAXPARSER_HPP_

#include <iostream>
#include <sstream>
#include <fstream>
#include <memory>

#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>
#include <xercesc/sax2/ContentHandler.hpp>
//#include <xercesc/util/TranscodingException.hpp>

#include <El/String/Utf16Char.hpp>
#include <El/XML/Use.hpp>
#include <El/XML/InputStream.hpp>
#include <El/XML/EntityResolver.hpp>

#include <xsd/DataFeed/RSS/Parser.hpp>

namespace NewsGate
{ 
  namespace RSS
  {
    class SAXParser : public xercesc::ContentHandler,
                      public Parser
    {
    public:
      EL_EXCEPTION(Exception, Parser::Exception);

    public:
      SAXParser(Parser::Interceptor* interceptor,
                Channel* delegated_channel) throw(El::Exception);
      
      virtual ~SAXParser() throw() {}

      virtual void parse_file(
        const char* name,
        bool validate,
        El::XML::EntityResolver* entity_resolver)
        throw(EncodingError, Exception, El::Exception, xsd::cxx::exception);

      virtual void parse(
        std::istream& is,
        const char* feed_url,
        const char* charset, 
        unsigned long max_read_portion,
        bool validate,
        El::XML::EntityResolver* entity_resolver)
        throw(EncodingError, Exception, El::Exception, xsd::cxx::exception);

      void feed_url(El::Net::HTTP::URL* url) throw(El::Exception);      

      virtual void characters(const XMLCh* const chars,
                              const XMLSize_t length);

    protected:
      virtual void endDocument() {}
      
      virtual void ignorableWhitespace(const XMLCh* const chars,
                                       const XMLSize_t length) {}
      
      virtual void processingInstruction(const XMLCh* const target,
                                         const XMLCh* const data) {}
      
      virtual void setDocumentLocator(const xercesc::Locator* const locator) {}
      
      virtual void startDocument() {}
      
      virtual void startPrefixMapping(const XMLCh* const prefix,
                                      const XMLCh* const uri) {}

      virtual void endPrefixMapping(const XMLCh* const prefix) {}

      virtual void skippedEntity(const XMLCh* const name) {}

    protected:
      
      El::Net::HTTP::URL_var feed_url_;

      typedef std::auto_ptr<std::uostringstream> uostringstream_ptr;      
      uostringstream_ptr chars_;
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
    inline
    SAXParser::SAXParser(Parser::Interceptor* interceptor,
                         Channel* delegated_channel)
      throw(El::Exception)
        : Parser(interceptor, delegated_channel)
    {
    }
    
    inline
    void
    SAXParser::parse_file(
      const char* name,
      bool validate,
      El::XML::EntityResolver* entity_resolver)
      throw(EncodingError, Exception, El::Exception, xsd::cxx::exception)
    {
      std::fstream file(name, std::ios::in);

      if(!file.is_open())
      {
        std::ostringstream ostr;
        ostr << "NewsGate::RSS::SAXParser::parse_file: can't open file "
             << name;

        throw Exception(ostr.str());
      }

      std::string url = std::string("file://") + name;
      
      parse(file,
            url.c_str(),
            0,
            1024 * 1024,
            validate,
            entity_resolver);
    }

    inline
    void
    SAXParser::parse(
      std::istream& is,
      const char* feed_url,
      const char* charset,
      unsigned long max_read_portion,
      bool validate,
      El::XML::EntityResolver* entity_resolver)
      throw(EncodingError, Exception, El::Exception, xsd::cxx::exception)
    {
      if(validate)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::RSS::SAXParser::parse: "
          "sorry, validation is not supported at the moment";

        throw Exception(ostr.str());        
      }

      feed_url_ = feed_url && strncmp(feed_url, "file://", 7) ?
        new El::Net::HTTP::URL(feed_url) : 0;      
      
      El::XML::Use use;
        
      ErrorHandler err_handler;
      El::XML::InputSource input_source(is, max_read_portion, charset);

      std::auto_ptr<xercesc::SAX2XMLReader> sax(
        xercesc::XMLReaderFactory::createXMLReader());

      sax->setFeature(xercesc::XMLUni::fgSAX2CoreNameSpaces, true);
      sax->setFeature(xercesc::XMLUni::fgSAX2CoreNameSpacePrefixes, true);
      sax->setFeature(xercesc::XMLUni::fgXercesValidationErrorAsFatal, true);
      sax->setFeature(xercesc::XMLUni::fgSAX2CoreValidation, false);
      sax->setFeature(xercesc::XMLUni::fgXercesSchema, false);
      sax->setFeature(xercesc::XMLUni::fgXercesSchemaFullChecking, false);
      sax->setFeature(xercesc::XMLUni::fgXercesLoadExternalDTD, false);
      sax->setFeature(xercesc::XMLUni::fgXercesSkipDTDValidation, true);

      sax->setErrorHandler(&err_handler);
      sax->setContentHandler(this);

      if(entity_resolver)
      {
        sax->setEntityResolver(entity_resolver);
      }

      try
      {
        sax->parse(input_source);
      }
/*      
      catch(const xercesc::TranscodingException& e)
      {
        std::string msg = xsd::cxx::xml::transcode<char>(e.getMessage());

        std::ostringstream ostr;
        ostr << "NewsGate::RSS::SAXParser::parse: transcoding failed for '"
             << (feed_url ? feed_url : "unknown") << "'. Description:\n"
             << msg;

        throw EncodingError(ostr.str());
      }
*/
      catch(const xercesc::SAXException& e)
      {
        std::string msg = xsd::cxx::xml::transcode<char>(e.getMessage());

        std::ostringstream ostr;
        ostr << "NewsGate::RSS::SAXParser::parse: parse failed for '"
             << (feed_url ? feed_url : "unknown") << "'. Description:\n"
             << msg;
        
        throw Exception(ostr.str());
      }
    }

    inline
    void
    SAXParser::characters(const XMLCh* const chars, const XMLSize_t length)
    {
      if(chars_.get() != 0)
      {
        chars_->write(chars, length);
      }  
    }

    inline
    void
    SAXParser::feed_url(El::Net::HTTP::URL* url) throw(El::Exception)
    {
      feed_url_ = El::RefCount::add_ref(url);
    }
  }
}

#endif // _NEWSGATE_SERVER_XSD_DATAFEED_RSS_SAXPARSER_HPP_
