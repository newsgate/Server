/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   xsd/DataFeed/RSS/UniParser.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_XSD_DATAFEED_RSS_UNIPARSER_HPP_
#define _NEWSGATE_SERVER_XSD_DATAFEED_RSS_UNIPARSER_HPP_

//#define RSS_PARSER_TRACE

#include <iostream>
#include <sstream>
#include <string>

#include <xercesc/sax2/Attributes.hpp>

#include <El/String/Manip.hpp>

#include <xsd/DataFeed/RSS/SAXParser.hpp>
#include <xsd/DataFeed/RSS/AtomParser.hpp>
#include <xsd/DataFeed/RSS/RDFParser.hpp>
#include <xsd/DataFeed/RSS/RSSParser.hpp>

namespace NewsGate
{ 
  namespace RSS
  {
    class UniParser : public SAXParser
    {
    public:
      EL_EXCEPTION(Exception, SAXParser::Exception);

    public:
      UniParser(Parser::Interceptor* interceptor = 0) throw(El::Exception);
      virtual ~UniParser() throw() {}

    private:
      virtual void endElement(const XMLCh* const uri,
                              const XMLCh* const localname,
                              const XMLCh* const qname);
      
      virtual void startElement(const XMLCh* const uri,
                                const XMLCh* const localname,
                                const XMLCh* const qname,
                                const xercesc::Attributes& attrs);
      
      virtual void characters(const XMLCh* const chars,
                              const XMLSize_t length);

      bool set_backend(SAXParser* parser,
                       const XMLCh* const uri,
                       const XMLCh* const localname,
                       const XMLCh* const qname,
                       const xercesc::Attributes& attrs)
        throw(El::Exception);
      
    private:
      std::auto_ptr<SAXParser> backend_;

    private:
      UniParser(UniParser&);
      void operator=(UniParser&);
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
    UniParser::UniParser(Parser::Interceptor* interceptor)
      throw(El::Exception)
        : SAXParser(interceptor, 0)
    {
    }

    inline
    bool
    UniParser::set_backend(SAXParser* parser,
                           const XMLCh* const uri,
                           const XMLCh* const localname,
                           const XMLCh* const qname,
                           const xercesc::Attributes& attrs)
      throw(El::Exception)
    {
      std::auto_ptr<SAXParser> backend;

      try
      {
        backend.reset(parser);
        backend->startElement(uri, localname, qname, attrs);
        backend_.reset(backend.release());
        backend_->feed_url(feed_url_.in());          
        return true;
      }
      catch(const SAXParser::Exception&)
      {
      }

      return false;
    }
    
    inline
    void
    UniParser::startElement(const XMLCh* const uri,
                            const XMLCh* const localname,
                            const XMLCh* const qname,
                            const xercesc::Attributes& attrs)
    {
      if(backend_.get() == 0)
      {
        if(set_backend(new RSSParser(interceptor_, &own_channel_),
                       uri,
                       localname,
                       qname,
                       attrs) ||
           set_backend(new AtomParser(interceptor_, &own_channel_),
                       uri,
                       localname,
                       qname,
                       attrs) ||
           set_backend(new RDFParser(interceptor_, &own_channel_),
                       uri,
                       localname,
                       qname,
                       attrs))
        {
          return;
        }

        std::string name = xsd::cxx::xml::transcode<char>(localname);
        
        std::ostringstream ostr;
        ostr << "NewsGate::RSS::UniParser::startElement: "
          "unexpected root element '" << name << "'";

        throw Exception(ostr.str());
      }
      
      backend_->startElement(uri, localname, qname, attrs);
    }

    inline
    void
    UniParser::endElement(const XMLCh* const uri,
                          const XMLCh* const localname,
                          const XMLCh* const qname)
    {
      if(backend_.get() == 0)
      {
        std::string name = xsd::cxx::xml::transcode<char>(localname);
        
        std::ostringstream ostr;
        ostr << "NewsGate::RSS::RSSParser::endElement: "
          "unmatched endElement for '" << name << "'";
        
        throw Exception(ostr.str());
      }
      
      backend_->endElement(uri, localname, qname);
    }

    inline
    void
    UniParser::characters(const XMLCh* const chars, const XMLSize_t length)
    {
      if(backend_.get())
      {
        backend_->characters(chars, length);
      }
      else
      {
        SAXParser::characters(chars, length);
      }      
    }
  }
}

#endif // _NEWSGATE_SERVER_XSD_DATAFEED_RSS_UNIPARSER_HPP_
