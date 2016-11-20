/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   xsd/DataFeed/RSS/RDFParser.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

//#define RSS_PARSER_TRACE

#ifndef _NEWSGATE_SERVER_XSD_DATAFEED_RSS_RDFPARSER_HPP_
#define _NEWSGATE_SERVER_XSD_DATAFEED_RSS_RDFPARSER_HPP_

#include <iostream>
#include <sstream>
#include <string>

#include <xercesc/sax2/Attributes.hpp>

#include <El/String/Manip.hpp>

#include <xsd/DataFeed/RSS/SAXParser.hpp>

namespace NewsGate
{ 
  namespace RSS
  {
    class RDFParser : public SAXParser
    {
    public:
      EL_EXCEPTION(Exception, SAXParser::Exception);

    public:
      RDFParser(Parser::Interceptor* interceptor = 0,
                Channel* delegated_channel = 0) throw(El::Exception);
      
      virtual ~RDFParser() throw() {}

    private:
      virtual void endElement(const XMLCh* const uri,
                              const XMLCh* const localname,
                              const XMLCh* const qname);
      
      virtual void startElement(const XMLCh* const uri,
                                const XMLCh* const localname,
                                const XMLCh* const qname,
                                const xercesc::Attributes& attrs);

      bool is_rss_namespace(const char* namespace_val)
        throw(El::Exception);
      
    private:
      std::string rdf_namespace_;
      std::string rss_namespace_prefix_;
      unsigned long rss_namespace_prefix_len_;

      enum ParsingPosition
      {
        PP_START,
        PP_WAIT_CHANNEL,
        PP_TAG_CHANNEL,
        PP_WAIT_ITEM,
        PP_TAG_ITEM,
        PP_END
      };

      ParsingPosition position_;      
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
    RDFParser::RDFParser(Parser::Interceptor* interceptor,
                         Channel* delegated_channel)
      throw(El::Exception)
        : SAXParser(interceptor, delegated_channel),
          rdf_namespace_("http://www.w3.org/1999/02/22-rdf-syntax-ns#"),
          rss_namespace_prefix_("http://purl.org/"),
          rss_namespace_prefix_len_(rss_namespace_prefix_.length()),
          position_(PP_START)
    {
    }

    inline
    bool
    RDFParser::is_rss_namespace(const char* namespace_val) throw(El::Exception)
    {
      return strncasecmp(namespace_val,
                         rss_namespace_prefix_.c_str(),
                         rss_namespace_prefix_.length()) == 0;
    }
    
/*    
  CHANNEL:
  
  <title>kottke.org remaindered links</title> 
  <link>http://www.kottke.org/remainder/</link> 
  <description>Many fun, interesting links updated many times a day. Many.</description> 
  <dc:language>en-us</dc:language> 
  <dc:date>2007-01-22T11:58:44-05:00</dc:date>

  ITEM:
  
  <title>For the Designing the City of the Future contest held by the History Channel, New York-based architecture firm ARO developed "a vision of New York recovering from massive flooding in low lying areas of New York as a result of global warming"</title> 
  <link>http://polisnyc.wordpress.com/2007/01/17/new-york-2106/</link> 
- <description>- <![CDATA[ <a href="http://polisnyc.wordpress.com/2007/01/17/new-york-2106/">For the Designing the City of the Future contest held by the History Channel, New York-based architecture firm ARO developed "a vision of New York recovering from massive flooding in low lying areas of New York as a result of global warming"</a>. <a href="http://www.flickr.com/photos/54112970@N00/sets/72157594483012847/">Photos of their entry are available on Flickr</a>. "In order to co-exist with fluctuating sea levels, ARO proposed a new building type called a "vane." Part skyscraper, part viaduct, 'vanes' are built in, on, and over flooded streets, reconnecting to the classic street grid and making up for lost square footage."  ]]> </description>
  <dc:date>2007-01-22T11:58:44-05:00</dc:date> 

*/  
    inline
    void
    RDFParser::startElement(const XMLCh* const uri,
                            const XMLCh* const localname,
                            const XMLCh* const qname,
                            const xercesc::Attributes& attrs)
    {
      switch(position_)
      {
      case PP_START:
        {
          if(strcasecmp(xsd::cxx::xml::transcode<char>(localname).c_str(),
                        "RDF") == 0 &&
             strcasecmp(xsd::cxx::xml::transcode<char>(uri).c_str(),
                        rdf_namespace_.c_str()) == 0)
          {
            position_ = PP_WAIT_CHANNEL;
            rss_channel().type = Feed::TP_RDF;
            return;
          }
            
          break;
        }
      case PP_WAIT_CHANNEL:
        {
          std::string name = xsd::cxx::xml::transcode<char>(localname);
            
          if(strcasecmp(name.c_str(), "channel") == 0)
          {
            if(is_rss_namespace(xsd::cxx::xml::transcode<char>(uri).c_str()))
            {
              position_ = PP_TAG_CHANNEL;
            }
          }
          else if(strcasecmp(name.c_str(), "item") == 0)
          {
            if(is_rss_namespace(xsd::cxx::xml::transcode<char>(uri).c_str()))
            {
              position_ = PP_TAG_ITEM;
            }              
          }
            
          return;
        }
      case PP_WAIT_ITEM:
        {
          std::string name = xsd::cxx::xml::transcode<char>(localname);
            
          if(strcasecmp(name.c_str(), "channel") == 0)
          {
            if(is_rss_namespace(xsd::cxx::xml::transcode<char>(uri).c_str()))
            {
              break;
            }
          }
          else if(strcasecmp(name.c_str(), "item") == 0)
          {
            if(is_rss_namespace(xsd::cxx::xml::transcode<char>(uri).c_str()))
            {
              position_ = PP_TAG_ITEM;
            }              
          }
            
          return;
        }
      case PP_TAG_CHANNEL:
        {
          std::string name = xsd::cxx::xml::transcode<char>(localname);

          if((strcasecmp(name.c_str(), "title") == 0 ||
              strcasecmp(name.c_str(), "description") == 0 ||
              strcasecmp(name.c_str(), "link") == 0 ||
              strcasecmp(name.c_str(), "language") == 0 ||
              strcasecmp(name.c_str(), "date") == 0) &&
             is_rss_namespace(xsd::cxx::xml::transcode<char>(uri).c_str()))
          {
            chars_.reset(new std::uostringstream());
          }
          
          return;
        }
      case PP_TAG_ITEM:
        {
          std::string name = xsd::cxx::xml::transcode<char>(localname);

          if((strcasecmp(name.c_str(), "title") == 0 ||
              strcasecmp(name.c_str(), "link") == 0 ||
              strcasecmp(name.c_str(), "description") == 0 ||
              strcasecmp(name.c_str(), "encoded") == 0 ||
              strcasecmp(name.c_str(), "date") == 0) &&
             is_rss_namespace(xsd::cxx::xml::transcode<char>(uri).c_str()))
          {
            chars_.reset(new std::uostringstream());
          }
          
          return;
        }
      default: break;
      }

      std::ostringstream ostr;
      ostr << "NewsGate::RSS::RDFParser::startElement: unexpected element '"
           << xsd::cxx::xml::transcode<char>(localname) << "' encountered in "
        "namespace '" << xsd::cxx::xml::transcode<char>(uri)
           << "' during phase " << position_;
        
      throw Exception(ostr.str());
    }
      
    inline
    void
    RDFParser::endElement(const XMLCh* const uri,
                          const XMLCh* const localname,
                          const XMLCh* const qname)
    {
      switch(position_)
      {
      case PP_WAIT_CHANNEL:
      case PP_WAIT_ITEM:
        {
          if(strcasecmp(xsd::cxx::xml::transcode<char>(localname).c_str(),
                        "RDF") == 0 &&
             strcasecmp(xsd::cxx::xml::transcode<char>(uri).c_str(),
                        rdf_namespace_.c_str()) == 0)
          {
            position_ = PP_END;
          }
          
          return;
        }
      case PP_TAG_CHANNEL:
        {
          std::string name = xsd::cxx::xml::transcode<char>(localname);
          
          if(strcasecmp(name.c_str(), "channel") == 0 &&
             is_rss_namespace(xsd::cxx::xml::transcode<char>(uri).c_str()))
          {
            position_ = PP_WAIT_ITEM;
          }
          else 
          {
            if(chars_.get() != 0)
            {
              Channel& channel = rss_channel();
              
              std::string chars =
                xsd::cxx::xml::transcode<char>(chars_->str().c_str());
              
              std::string nmspace = xsd::cxx::xml::transcode<char>(uri);
            
              if(strcasecmp(name.c_str(), "title") == 0 &&
                 is_rss_namespace(nmspace.c_str()))
              {
                channel.title = chars;

#     ifdef RSS_PARSER_TRACE          
                std::cerr << "ChannelParser::title: " << channel.title
                          << std::endl;
#     endif
              }
              else if(strcasecmp(name.c_str(), "description") == 0 &&
                      is_rss_namespace(nmspace.c_str()))
              {
                  channel.description = chars;
          
#     ifdef RSS_PARSER_TRACE          
                std::cerr << "ChannelParser::description: "
                          << channel.description << std::endl;
#     endif
              }
              else if(strcasecmp(name.c_str(), "link") == 0 &&
                      is_rss_namespace(nmspace.c_str()))
              {
                try
                {
                  El::Net::HTTP::URL_var url =
                    new El::Net::HTTP::URL(chars.c_str());
                  
                  channel.html_link = url->string();
                }
                catch(...)
                {
                  try
                  {
                    if(feed_url_.in())
                    {
                      channel.html_link = feed_url_->abs_url(chars.c_str());
                    }
                  }
                  catch(...)
                  {
                  }                
                } 
          
#     ifdef RSS_PARSER_TRACE          
                std::cerr << "ChannelParser::html_link: "
                          << channel.html_link << std::endl;
#     endif
              }
              else if(strcasecmp(name.c_str(), "language") == 0 &&
                      is_rss_namespace(nmspace.c_str()))
              {
                channel.set_lang_and_country(chars.c_str());
        
#           ifdef RSS_PARSER_TRACE          
                std::cerr << "ChannelParser::language: " << channel.lang
                          << std::endl << "ChannelParser::country: "
                          << channel.country << std::endl;
#           endif
              }
              else if(strcasecmp(name.c_str(), "date") == 0 &&
                      is_rss_namespace(nmspace.c_str()))
              {
                try
                {
//                  channel.last_build_date.set_iso8601(chars.c_str());

                  last_build_date(
                    El::Moment(chars.c_str(), El::Moment::TF_ISO_8601));
                  
            
#     ifdef RSS_PARSER_TRACE          
                  std::cerr << "ChannelParser::lastBuildDate: " <<
                    channel.last_build_date.rfc0822() << std::endl;
#     endif
                }
                catch(...)
                {
                }
                
                if(interceptor_)
                {
                  interceptor_->post_last_build_date(channel);
                }          
              }

              chars_.reset(0);
            }
          }

          return;
        }
      case PP_TAG_ITEM:
        {
          std::string name = xsd::cxx::xml::transcode<char>(localname);
          
          if(strcasecmp(name.c_str(), "item") == 0 &&
             is_rss_namespace(xsd::cxx::xml::transcode<char>(uri).c_str()))
          {
            Channel& channel = rss_channel();
            channel.items.push_back(item_);
        
            if(interceptor_)
            {
              interceptor_->post_item(channel);
            }
#     ifdef RSS_PARSER_TRACE          
            std::cerr << "ItemParser::post: id=" << item_.code.string()
                      << std::endl << std::endl;
#     endif
            
            item_.clear();
            position_ = PP_WAIT_ITEM;
          }          
          else 
          {
            if(chars_.get() != 0)
            {
              std::string ch =
                xsd::cxx::xml::transcode<char>(chars_->str().c_str());
              
              std::string chars;
              El::String::Manip::trim(ch.c_str(), chars);
              
              std::string nmspace = xsd::cxx::xml::transcode<char>(uri);
            
              if(strcasecmp(name.c_str(), "title") == 0 &&
                 is_rss_namespace(nmspace.c_str()))
              {
                item_.title = chars;

#     ifdef RSS_PARSER_TRACE          
                std::cerr << "ItemParser::title: " << item_.title
                          << std::endl;
#     endif
              }
              else if(strcasecmp(name.c_str(), "link") == 0 &&
                      is_rss_namespace(nmspace.c_str()))
              {                
                try
                {
                  if(feed_url_.in())
                  {
                    chars = feed_url_->abs_url(chars.c_str());
                  }
                }
                catch(...)
                {
                }
                  
                try
                {
                  El::Net::HTTP::URL_var url =
                    new El::Net::HTTP::URL(chars.c_str());
                  
                  item_.url = url->string();
                }
                catch(...)
                {
                }
                
                item_.code.id = 0;
                
                El::CRC(item_.code.id,
                        (const unsigned char*)chars.c_str(),
                        chars.size());
                
#     ifdef RSS_PARSER_TRACE          
                std::cerr << "ItemParser::link: " << item_.url
                          << std::endl;
#     endif 
              }
              else if(strcasecmp(name.c_str(), "description") == 0 &&
                      is_rss_namespace(nmspace.c_str()))
              {
                if(item_.description.empty())
                {
                  item_.description = chars;

#     ifdef RSS_PARSER_TRACE
                  std::cerr << "ItemParser::description: "
                            << item_.description
                            << std::endl;
#     endif
                }
              }
              else if(strcasecmp(name.c_str(), "encoded") == 0 &&
                      is_rss_namespace(nmspace.c_str()))
              {
                item_.description = chars;
          
#     ifdef RSS_PARSER_TRACE          
                std::cerr << "ChannelParser::description (from encoded): "
                          << channel.description << std::endl;
#     endif
              }
              else if(strcasecmp(name.c_str(), "date") == 0 &&
                      is_rss_namespace(nmspace.c_str()))
              {
                try
                {
                  El::Moment time(chars.c_str(), El::Moment::TF_ISO_8601);
//                  item_.code.published = ACE_Time_Value(time).sec();

                  item_pub_date(time);
                  
#     ifdef RSS_PARSER_TRACE          
                  std::cerr << "ItemParser::published: "
                            << item_.code.pub_date().rfc0822() << std::endl;
#     endif
                }
                catch(...)
                {
                }
              }
              
              chars_.reset(0);
            }
          }
          
          return;
        }
      default: break;
      }
      
    }
  }
}

#endif // _NEWSGATE_SERVER_XSD_DATAFEED_RSS_RDFPARSER_HPP_
