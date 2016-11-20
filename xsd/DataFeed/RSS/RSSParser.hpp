/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   xsd/DataFeed/RSS/RSSParser.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_XSD_DATAFEED_RSS_RSSPARSER_HPP_
#define _NEWSGATE_SERVER_XSD_DATAFEED_RSS_RSSPARSER_HPP_

//#define RSS_PARSER_TRACE

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
    class RSSParser : public SAXParser
    {
    public:
      EL_EXCEPTION(Exception, SAXParser::Exception);

    public:
      RSSParser(Parser::Interceptor* interceptor = 0,
                Channel* delegated_channel = 0) throw(El::Exception);
      virtual ~RSSParser() throw() {}

    private:
      virtual void endElement(const XMLCh* const uri,
                              const XMLCh* const localname,
                              const XMLCh* const qname);
      
      virtual void startElement(const XMLCh* const uri,
                                const XMLCh* const localname,
                                const XMLCh* const qname,
                                const xercesc::Attributes& attrs);
      
    private:

      void start_item_subelement(const char* name,
                                 const char* nmspace,
                                 const xercesc::Attributes& attrs)
        throw(El::Exception);
      
      void start_channel_subelement(const char* name,
                                    const xercesc::Attributes& attrs)
        throw(El::Exception);
      
      void end_item_subelement(const char* name) throw(El::Exception);
      void end_channel_subelement(const char* name) throw(El::Exception);      

      void set_url(const char* val) throw(El::Exception);
      
    private:
      size_t inside_channel_;
      size_t inside_item_;
      bool inside_rss_elem_;
      size_t parsing_depth_;
      std::string item_link_;
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
    RSSParser::RSSParser(Parser::Interceptor* interceptor,
                         Channel* delegated_channel)
      throw(El::Exception)
        : SAXParser(interceptor, delegated_channel),
          inside_channel_(0),
          inside_item_(0),
          inside_rss_elem_(false),
          parsing_depth_(0)
    {
    }
    
    inline
    void
    RSSParser::startElement(const XMLCh* const uri,
                             const XMLCh* const localname,
                             const XMLCh* const qname,
                             const xercesc::Attributes& attrs)
    {
      parsing_depth_++;

      std::string name = xsd::cxx::xml::transcode<char>(localname);

      if(!inside_rss_elem_)
      {
        if(strcasecmp(name.c_str(), "rss"))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::RSS::RSSParser::startElement: "
            "unexpected root element '" << name << "'";

          throw Exception(ostr.str());
        }

        inside_rss_elem_ = true;
        rss_channel().type = Feed::TP_RSS;
        return;
      }
      
      if(strcasecmp(name.c_str(), "channel") == 0)
      {
        inside_channel_++;
      }
      else if(strcasecmp(name.c_str(), "item") == 0)
      {
        item_.clear();
        item_link_.clear();
        inside_item_++;
      }
      else if(inside_item_)
      {
        std::string nmspace = xsd::cxx::xml::transcode<char>(uri);

        if(parsing_depth_ == 4)
        {
          start_item_subelement(name.c_str(), nmspace.c_str(), attrs);
        }
      }
      else if(inside_channel_)
      {
        if(parsing_depth_ == 3)
        {
          start_channel_subelement(name.c_str(), attrs);
        }
      }

//      std::cerr << "startElement " << name << " " << parsing_depth_
//                << std::endl;      
    }

    inline
    void
    RSSParser::endElement(const XMLCh* const uri,
                          const XMLCh* const localname,
                          const XMLCh* const qname)
    {
      std::string name = xsd::cxx::xml::transcode<char>(localname);

//      std::cerr << "endElement " << name << " " << parsing_depth_
//                << std::endl;      

      if(inside_item_)
      {
        if(strcasecmp(name.c_str(), "item") == 0)
        {
          //
          // Calculate right item url
          //
          
          if(item_.guid.is_permalink)
          {
            const char* url = item_.guid.value.c_str();

            if(strncmp(url, "http://", 7) == 0)
            {
              //
              // Some feeds thinks that by default isPermaLink=false.
              // Still want to download them properly.
              //
              set_url(url);
            }
          }
            
          if(item_.url.empty())
          {
            set_url(item_link_.c_str());
          }

          if(item_.url.empty())
          {
            // If even <link> is absent or empty consider <guid> as a link
            // unconditionally.
            set_url(item_.guid.value.c_str());
          }
          
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
          item_link_.clear();
          inside_item_--;
        }
        else
        {
          end_item_subelement(name.c_str());
        }
      }
      else if(inside_channel_)
      {
        if(strcasecmp(name.c_str(), "channel") == 0)
        {
          inside_channel_--;
        }
        else
        {
          end_channel_subelement(name.c_str());
        }
      }

      parsing_depth_--;
    }

    inline
    void
    RSSParser::start_item_subelement(const char* name,
                                     const char* nmspace,
                                     const xercesc::Attributes& attrs)
      throw(El::Exception)
    {
//      std::cerr << "start_item_subelement: " << name << std::endl;

      if(strcasecmp(name, "pubDate") == 0 ||
         (strcasecmp(name, "title") == 0 &&
          strcasecmp(nmspace, "http://search.yahoo.com/mrss/")) ||
         strcasecmp(name, "description") == 0 ||
         strcasecmp(name, "encoded") == 0 ||
         strcasecmp(name, "full-text") == 0 ||
         strcasecmp(name, "link") == 0 ||
         strcasecmp(name, "guid") == 0)
      {
        chars_.reset(new std::uostringstream());
      }

      if(strcasecmp(name, "enclosure") == 0 ||
         strcasecmp(name, "thumbnail") == 0 ||
         strcasecmp(name, "content") == 0)
      {
        Enclosure enclosure;

        if(strcasecmp(name, "thumbnail") == 0)
        {
          enclosure.type = "image/";
        }

        unsigned long len = attrs.getLength();
        for(unsigned long i = 0; i < len; i++)
        {
          std::string attr_name =
            xsd::cxx::xml::transcode<char>(attrs.getLocalName(i));
          
          std::string v = xsd::cxx::xml::transcode<char>(attrs.getValue(i));
          
          std::string attr_val;
          El::String::Manip::trim(v.c_str(), attr_val);

          if(strcasecmp(attr_name.c_str(), "url") == 0)
          {
            enclosure.url = attr_val;
          }
          else if(strcasecmp(attr_name.c_str(), "type") == 0)
          {
            if(enclosure.type.empty())
            {
              enclosure.type = attr_val;
            }
          }
          else if(strcasecmp(attr_name.c_str(), "medium") == 0)
          {
            if(enclosure.type.empty())
            {
              enclosure.type = attr_val + "/";
            }
          }
        }

        if(!enclosure.url.empty())
        {
          item_.enclosures.push_back(enclosure);

#     ifdef RSS_PARSER_TRACE
        std::cerr << "ItemParser::enclosure: url=" << enclosure.url
                  << ", type=" << enclosure.type << std::endl;
#     endif          
        }
      }
      else if(strcasecmp(name, "guid") == 0)
      {
        unsigned long len = attrs.getLength();
        for(unsigned long i = 0; i < len; i++)
        {
          std::string attr_name =
            xsd::cxx::xml::transcode<char>(attrs.getLocalName(i));
          
          std::string v = xsd::cxx::xml::transcode<char>(attrs.getValue(i));
          
          std::string attr_val;
          El::String::Manip::trim(v.c_str(), attr_val);

          if(strcasecmp(attr_name.c_str(), "isPermaLink") == 0 &&
             strcasecmp(attr_val.c_str(), "false") == 0)
          {
            item_.guid.is_permalink = false;
            break;
          }
        }
      }      
    }
      
    inline
    void
    RSSParser::start_channel_subelement(const char* name,
                                        const xercesc::Attributes& attrs)
      throw(El::Exception)
    {
//      std::cerr << "start_channel_subelement: " << name << std::endl;

      if(strcasecmp(name, "title") == 0 ||
         strcasecmp(name, "description") == 0 ||
         strcasecmp(name, "link") == 0 ||
         strcasecmp(name, "language") == 0 ||
         strcasecmp(name, "lastBuildDate") == 0 ||
         strcasecmp(name, "ttl") == 0)
      {
        chars_.reset(new std::uostringstream());
      }      
    }
      
    inline
    void
    RSSParser::end_item_subelement(const char* name) throw(El::Exception)
    {
//      std::cerr << "end_item_subelement: " << name << std::endl;

      if(chars_.get() != 0)
      {
        std::string chars =
          xsd::cxx::xml::transcode<char>(chars_->str().c_str());

        if(strcasecmp(name, "pubDate") == 0)
        {
          try
          {
            El::Moment pub_date;
            pub_date.set_rfc0822(chars.c_str());

//            item_.code.published = ACE_Time_Value(pub_date).sec();

            item_pub_date(pub_date);
      
#     ifdef RSS_PARSER_TRACE          
            std::cerr << "ItemParser::pubDate: "
                      << item_.code.up_date().rfc0822() << std::endl;
#     endif
          }
          catch(...)
          {
          }          
        }
        else if(strcasecmp(name, "title") == 0)
        {
          item_.title = chars;
          
#     ifdef RSS_PARSER_TRACE
          std::cerr << "ItemParser::title: " << item_.title << std::endl;
#     endif
        }
        else if(strcasecmp(name, "description") == 0)
        {
          if(item_.description.empty())
          {
            item_.description = chars;
      
#     ifdef RSS_PARSER_TRACE          
            std::cerr << "ItemParser::description: " << item_.description
                      << std::endl;
#     endif
          }
        }
        else if(strcasecmp(name, "encoded") == 0 ||
                strcasecmp(name, "full-text") == 0)
        {
          item_.description = chars;

#     ifdef RSS_PARSER_TRACE 
          std::cerr << "ItemParser::encoded: " << item_.description
                    << std::endl;
#     endif
        }
        else if(strcasecmp(name, "link") == 0)
        {
          El::String::Manip::trim(chars.c_str(), item_link_);

#     ifdef RSS_PARSER_TRACE
          std::cerr << "ItemParser::link: " << item_link_ << std::endl;
#     endif
        }
        else if(strcasecmp(name, "guid") == 0)
        {
          El::String::Manip::trim(chars.c_str(), item_.guid.value);
          
#     ifdef RSS_PARSER_TRACE
          std::cerr << "ItemParser::guid: " << item_.guid.is_permalink << "; '"
                    << item_.guid.value << "'\n";
#     endif
        }

        chars_.reset(0);
      }
    }
      
    inline
    void
    RSSParser::end_channel_subelement(const char* name) throw(El::Exception)
    {
//      std::cerr << "end_channel_subelement: " << name << std::endl;

      if(chars_.get() != 0)
      {
        Channel& channel = rss_channel();
        
        std::string chars =
          xsd::cxx::xml::transcode<char>(chars_->str().c_str());
        
        if(strcasecmp(name, "lastBuildDate") == 0)
        {
          try
          {
//            channel.last_build_date.set_rfc0822(chars.c_str());

            last_build_date(
              El::Moment(chars.c_str(), El::Moment::TF_RFC_0822));
            
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
        else if(strcasecmp(name, "title") == 0)
        {
          channel.title = chars;

#     ifdef RSS_PARSER_TRACE          
          std::cerr << "ChannelParser::title: " << channel.title << std::endl;
#     endif
        }
        else if(strcasecmp(name, "description") == 0)
        {
          channel.description = chars;
          
#     ifdef RSS_PARSER_TRACE          
          std::cerr << "ChannelParser::description: " << channel.description
                    << std::endl;
#     endif
        }
        else if(strcasecmp(name, "link") == 0)
        {
          try
          {
            El::Net::HTTP::URL_var url = new El::Net::HTTP::URL(chars.c_str());
            channel.html_link = url->string();
          }
          catch(...)
          {
            if(feed_url_.in())
            {
              try
              {
                channel.html_link = feed_url_->abs_url(chars.c_str());
              }
              catch(...)
              {
              } 
            }
          }

#     ifdef RSS_PARSER_TRACE          
          std::cerr << "ChannelParser::link: " << channel.html_link
                    << std::endl;
#     endif      
        }
        else if(strcasecmp(name, "language") == 0)
        {
          channel.set_lang_and_country(chars.c_str());
        
#     ifdef RSS_PARSER_TRACE          
          std::cerr << "ChannelParser::language: " << channel.lang
                    << std::endl << "ChannelParser::country: "
                    << channel.country << std::endl;
#     endif      
        }
        else if(strcasecmp(name, "ttl") == 0)
        {
          unsigned long long val = 0;

          if(El::String::Manip::numeric(chars.c_str(), val))
          {
            channel.ttl = val;
      
#     ifdef RSS_PARSER_TRACE          
            std::cerr << "ChannelParser::ttl: " << channel.ttl << std::endl;
#     endif
          }
        }
        
        chars_.reset(0);
      }
    }
    
    inline
    void
    RSSParser::set_url(const char* val) throw(El::Exception)
    {
      std::string trimmed;
      El::String::Manip::trim(val, trimmed);

      if(feed_url_.in())
      {
        try
        {
          trimmed = feed_url_->abs_url(trimmed.c_str());
        }
        catch(...)
        {
        } 
      }

      try
      {
        El::Net::HTTP::URL_var url = new El::Net::HTTP::URL(trimmed.c_str());
        item_.url = url->string();

        item_.code.id = 0;
      
        El::CRC(item_.code.id,
                (const unsigned char*)item_.url.c_str(),
                item_.url.size());
      }
      catch(...)
      {
      }      
    }
    
  }
}

#endif // _NEWSGATE_SERVER_XSD_DATAFEED_RSS_RSSPARSER_HPP_
