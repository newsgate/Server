/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   xsd/DataFeed/RSS/AtomParser.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_XSD_DATAFEED_RSS_ATOMPARSER_HPP_
#define _NEWSGATE_SERVER_XSD_DATAFEED_RSS_ATOMPARSER_HPP_

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
    class AtomParser : public SAXParser
    {
    public:
      EL_EXCEPTION(Exception, SAXParser::Exception);

    public:
      AtomParser(Parser::Interceptor* interceptor = 0,
                 Channel* delegated_channel = 0) throw(El::Exception);
      
      virtual ~AtomParser() throw() {}

    private:
      virtual void endElement(const XMLCh* const uri,
                              const XMLCh* const localname,
                              const XMLCh* const qname);
      
      virtual void startElement(const XMLCh* const uri,
                                const XMLCh* const localname,
                                const XMLCh* const qname,
                                const xercesc::Attributes& attrs);
      
    private:

      void start_entry_subelement(const char* name,
                                  const xercesc::Attributes& attrs)
        throw(El::Exception);
      
      void start_feed_subelement(const char* name,
                                 const xercesc::Attributes& attrs)
        throw(El::Exception);
      
      void end_entry_subelement(const char* name) throw(El::Exception);
      
      void end_feed_subelement(const char* name) throw(El::Exception);      

    private:      
      std::string namespace_;
      size_t entry_;
      size_t source_;
      bool entry_link_html_;
      bool inside_feed_elem_;
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
    AtomParser::AtomParser(Parser::Interceptor* interceptor,
                           Channel* delegated_channel)
      throw(El::Exception)
        : SAXParser(interceptor, delegated_channel),
          namespace_("http://www.w3.org/2005/Atom"),
          entry_(0),
          source_(0),
          entry_link_html_(false),
          inside_feed_elem_(false)
    {
    }
    
    inline
    void
    AtomParser::startElement(const XMLCh* const uri,
                             const XMLCh* const localname,
                             const XMLCh* const qname,
                             const xercesc::Attributes& attrs)
    {

      if(!inside_feed_elem_)
      {
        std::string namespace_val = xsd::cxx::xml::transcode<char>(uri);
        std::string name_val = xsd::cxx::xml::transcode<char>(localname);
          
        if(strcasecmp(namespace_val.c_str(),  namespace_.c_str()) ||
           strcasecmp(name_val.c_str(), "feed"))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::RSS::AtomParser::startElement: "
            "unexpected root element '" << name_val << "' in namespace '"
               << namespace_val << "'";

          throw Exception(ostr.str());
        }

        inside_feed_elem_ = true;
        rss_channel().type = Feed::TP_ATOM;
        return;
      }
        
      std::string name = xsd::cxx::xml::transcode<char>(localname);
      
      if(strcasecmp(xsd::cxx::xml::transcode<char>(uri).c_str(),
                    namespace_.c_str()) == 0)
      {
        if(strcasecmp(name.c_str(), "entry") == 0)
        {
          if(!entry_)
          {
            item_.clear();
            entry_link_html_ = false;
          }
          
          entry_++;
        }

        if(!source_)
        {
          if(entry_)
          {
            start_entry_subelement(name.c_str(), attrs);
          }
          else
          {
            start_feed_subelement(name.c_str(), attrs);
          }
        }
      }
      else if(entry_)
      {
        if(strcasecmp(name.c_str(), "img") == 0)
        {
          // Improvement for feed://ruformator.ru/export/newsAtom.asp
          // NG-28
          
          if(chars_.get() != 0)
          {
            // Inside summary or content subelements of entry
            
            std::wostringstream ostr;
            ostr << L"<img";

            unsigned long len = attrs.getLength();

            for(unsigned long i = 0; i < len; i++)
            {
              std::string attr_name =
                xsd::cxx::xml::transcode<char>(attrs.getLocalName(i));
            
              if(strcasecmp(attr_name.c_str(), "src") == 0 ||
                 strcasecmp(attr_name.c_str(), "alt") == 0)
              {
                ostr << L" ";
                El::String::Manip::utf8_to_wchar(attr_name.c_str(), ostr);
                ostr << "=\"";

                std::string encoded_val;
              
                El::String::Manip::xml_encode(
                  xsd::cxx::xml::transcode<char>(attrs.getValue(i)).c_str(),
                  encoded_val,
                  El::String::Manip::XE_ATTRIBUTE_ENCODING);

                El::String::Manip::utf8_to_wchar(encoded_val.c_str(), ostr);
              
                ostr << L"\"";
              }
            }
          
            ostr << L"/>";

            std::ustring str;
            El::String::Manip::wchar_to_utf16(ostr.str().c_str(), str);
            *chars_ << str;
          }
        }
        else if(strcasecmp(name.c_str(), "a") == 0)
        {
          // Improvement for http://vo.astronet.ru/planet/atom.xml
          // NG-28
          
          if(chars_.get() != 0)
          {
            // Inside summary or content subelements of entry
            
            std::wostringstream ostr;
            ostr << L"<a";

            unsigned long len = attrs.getLength();

            for(unsigned long i = 0; i < len; i++)
            {
              std::string attr_name =
                xsd::cxx::xml::transcode<char>(attrs.getLocalName(i));
            
              if(strcasecmp(attr_name.c_str(), "href") == 0)
              {
                ostr << L" ";
                El::String::Manip::utf8_to_wchar(attr_name.c_str(), ostr);
                ostr << "=\"";

                std::string encoded_val;
              
                El::String::Manip::xml_encode(
                  xsd::cxx::xml::transcode<char>(attrs.getValue(i)).c_str(),
                  encoded_val,
                  El::String::Manip::XE_ATTRIBUTE_ENCODING);

                El::String::Manip::utf8_to_wchar(encoded_val.c_str(), ostr);
              
                ostr << L"\"";
              }
            }
          
            ostr << L"/>";

            std::ustring str;
            El::String::Manip::wchar_to_utf16(ostr.str().c_str(), str);
            *chars_ << str;
          }
        }        
        else if(strcasecmp(name.c_str(), "thumbnail") == 0)
        {
          // Improvement for http://www.bbc.co.uk/russian/life/index.xml
          // NG-27

          Enclosure enclosure;

          enclosure.type = "image/";
          unsigned long len = attrs.getLength();

          for(unsigned long i = 0; i < len; i++)
          {
            std::string attr_name =
              xsd::cxx::xml::transcode<char>(attrs.getLocalName(i));

            if(strcasecmp(attr_name.c_str(), "url") == 0)
            {
              enclosure.url =
                xsd::cxx::xml::transcode<char>(attrs.getValue(i));
            }
          }

          item_.enclosures.push_back(enclosure);
        }
        
/*

        
        Enclosure enclosure;
        
        unsigned long len = attrs.getLength();

        for(unsigned long i = 0; i < len; i++)
        {
          std::string attr_name =
            xsd::cxx::xml::transcode<char>(attrs.getLocalName(i));

          if(strcasecmp(attr_name.c_str(), "src") == 0)
          {
            enclosure.url = xsd::cxx::xml::transcode<char>(attrs.getValue(i));
          }
          else if(strcasecmp(attr_name.c_str(), "alt") == 0)
          {
//            enclosure.desc = xsd::cxx::xml::transcode<char>(attrs.getValue(i));
          }
        }
*/
        
//        item_.enclosures
      }
      
    }

    inline
    void
    AtomParser::endElement(const XMLCh* const uri,
                           const XMLCh* const localname,
                           const XMLCh* const qname)
    {
      if(xsd::cxx::xml::transcode<char>(uri) == namespace_)
      {
        std::string name = xsd::cxx::xml::transcode<char>(localname);

        if(source_)
        {
          if(strcasecmp(name.c_str(), "source") == 0)
          {
            source_--;
          }
        }
        else
        {
          if(entry_)
          {
            end_entry_subelement(name.c_str());
          }
          else
          {
            end_feed_subelement(name.c_str());
          }

          if(strcasecmp(name.c_str(), "entry") == 0)
          {
            if(entry_)
            {
              entry_--;
            }
            else
            {
              std::ostringstream ostr;
              ostr << "NewsGate::RSS::AtomParser::parse: "
                "document is not well formed";

              throw Exception(ostr.str());            
            }
          }
          else if(strcasecmp(name.c_str(), "feed") == 0)
          {
            if(!entry_)
            {
              inside_feed_elem_ = false;
            }
          }
        }
      }
      
    }

    inline
    void
    AtomParser::start_entry_subelement(const char* name,
                                       const xercesc::Attributes& attrs)
      throw(El::Exception)
    {
//      std::cerr << "start_entry_subelement: " << name << std::endl;
      
      if(strcasecmp(name, "updated") == 0 ||
         strcasecmp(name, "published") == 0 ||
         strcasecmp(name, "title") == 0 ||
         strcasecmp(name, "summary") == 0 ||
         strcasecmp(name, "content") == 0)
      {
        chars_.reset(new std::uostringstream());
      }
      else if(strcasecmp(name, "link") == 0)
      {
        unsigned long len = attrs.getLength();

        std::string rel;
        std::string link;
        std::string type;

        for(unsigned long i = 0; i < len; i++)
        {
          std::string attr_name =
            xsd::cxx::xml::transcode<char>(attrs.getLocalName(i));

          if(strcasecmp(attr_name.c_str(), "rel") == 0)
          {
            rel = xsd::cxx::xml::transcode<char>(attrs.getValue(i));
          }
          else if(strcasecmp(attr_name.c_str(), "href") == 0)
          {
            std::string l = xsd::cxx::xml::transcode<char>(attrs.getValue(i));
            El::String::Manip::trim(l.c_str(), link);
          }
          else if(strcasecmp(attr_name.c_str(), "type") == 0)
          {
            type = xsd::cxx::xml::transcode<char>(attrs.getValue(i));
          }
        }

        if(strcasecmp(rel.c_str(), "enclosure") == 0)
        {
          Enclosure enclosure;
          enclosure.type = type;
          enclosure.url = link;
          item_.enclosures.push_back(enclosure);
          
          return;
        }

        if(entry_link_html_ ||
           (!rel.empty() && strcasecmp(rel.c_str(), "alternate") &&
            strcasecmp(rel.c_str(), "self")))
        {
          return;
        }

        bool is_html =
          strcasecmp(type.c_str(), "application/xhtml+xml") == 0 ||
          strcasecmp(type.c_str(), "text/html") == 0;

        if(!is_html && !item_.url.empty())
        {
          return;
        }

        entry_link_html_ = is_html;

        try
        {
          if(feed_url_.in())
          {
            link = feed_url_->abs_url(link.c_str());
          }
        }
        catch(...)
        {
        }
        
        try
        {
          El::Net::HTTP::URL_var url = new El::Net::HTTP::URL(link.c_str());
          item_.url = url->string();
        }
        catch(...)
        {
        }
        
        item_.code.id = 0;
        
        El::CRC(item_.code.id,
                (const unsigned char*)link.c_str(),
                link.size());        
          
#     ifdef RSS_PARSER_TRACE          
        std::cerr << "ItemParser::link: " << item_.url
                  << std::endl;
#     endif 
      }
      else if(strcasecmp(name, "source") == 0)
      {
        source_++;
      }
    }
      
    inline
    void
    AtomParser::start_feed_subelement(const char* name,
                                      const xercesc::Attributes& attrs)
      throw(El::Exception)
    {
      Channel& channel = rss_channel();
      
      if(strcasecmp(name, "feed") == 0)
      {
        unsigned long len = attrs.getLength();

        for(unsigned long i = 0; i < len; i++)
        {
          std::string attr_name =
            xsd::cxx::xml::transcode<char>(attrs.getLocalName(i));

          if(strcasecmp(attr_name.c_str(), "lang") == 0)
          {
            std::string value =
              xsd::cxx::xml::transcode<char>(attrs.getValue(i));

            channel.set_lang_and_country(value.c_str());
        
#           ifdef RSS_PARSER_TRACE          
            std::cerr << "ChannelParser::language: " << channel.lang
                      << std::endl << "ChannelParser::country: "
                      << channel.country << std::endl;
#           endif      
            
          }
        }
      }
      else if(strcasecmp(name, "updated") == 0 ||
              strcasecmp(name, "title") == 0 ||
              strcasecmp(name, "subtitle") == 0)
      {
        chars_.reset(new std::uostringstream());
      }
      else if(strcasecmp(name, "link") == 0)
      {
        std::string href;
        std::string rel;
        std::string type;
        
        unsigned long len = attrs.getLength();
        
        for(unsigned long i = 0; i < len; i++)
        {
          std::string attr_name =
            xsd::cxx::xml::transcode<char>(attrs.getLocalName(i));

          std::string value =
            xsd::cxx::xml::transcode<char>(attrs.getValue(i));

          if(strcasecmp(attr_name.c_str(), "href") == 0)
          {
            href = value;
          }
          else if(strcasecmp(attr_name.c_str(), "rel") == 0)
          {
            rel = value;
          }
          else if(strcasecmp(attr_name.c_str(), "type") == 0)
          {
            type = value;
          }
        }

        if(!href.empty() &&
           (rel.empty() || strcasecmp(rel.c_str(), "alternate") == 0) &&
           (type.empty() || strcasestr(type.c_str(), "html") != 0))
        {                
          try
          {
            El::Net::HTTP::URL_var url =
              new El::Net::HTTP::URL(href.c_str());
                  
            channel.html_link = url->string();
          }
          catch(...)
          {
            try
            {
              if(feed_url_.in())
              {
                channel.html_link = feed_url_->abs_url(href.c_str());
              }
            }
            catch(...)
            {
            }
          }
          
#         ifdef RSS_PARSER_TRACE          
          std::cerr << "ChannelParser::html_link: " << channel.html_link
                    << std::endl;
#         endif
        }
      }
    }
      
    inline
    void
    AtomParser::end_entry_subelement(const char* name) throw(El::Exception)
    {
      if(chars_.get() != 0)
      {        
        std::string chars =
          xsd::cxx::xml::transcode<char>(chars_->str().c_str());
        
        if(strcasecmp(name, "updated") == 0)
        {
          try
          {
            El::Moment time(chars.c_str(), El::Moment::TF_ISO_8601);
            
            if(item_.code.published == 0)
            {
//              item_.code.published = ACE_Time_Value(time).sec();
              item_pub_date(time);
            }
            
#     ifdef RSS_PARSER_TRACE          
        std::cerr << "ItemParser::updated: " << time.rfc0822() << std::endl;
#     endif
          }
          catch(...)
          {
          }
        }
        else if(strcasecmp(name, "published") == 0)
        {
          try
          {
            El::Moment time(chars.c_str(), El::Moment::TF_ISO_8601);
//            item_.code.published = ACE_Time_Value(time).sec();
            
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
        else if(strcasecmp(name, "title") == 0)
        {
          item_.title = chars;

#     ifdef RSS_PARSER_TRACE          
          std::cerr << "ItemParser::title: " << item_.title
                    << std::endl;
#     endif
        }
        else if(strcasecmp(name, "summary") == 0)
        {
          if(item_.description.empty())
          {
            item_.description = chars;

#     ifdef RSS_PARSER_TRACE          
            std::cerr << "ItemParser::description: "
                      << item_.description << std::endl;
#     endif
          }
        }
        else if(strcasecmp(name, "content") == 0)
        {
          item_.description = chars;

#     ifdef RSS_PARSER_TRACE          
          std::cerr << "ItemParser::description: " << item_.description
                    << std::endl;
#     endif
        }

        chars_.reset(0);
      }
      else if(strcasecmp(name, "entry") == 0)
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
      }
        
    }
      
    inline
    void
    AtomParser::end_feed_subelement(const char* name) throw(El::Exception)
    {
      if(chars_.get() != 0)
      {
        Channel& channel = rss_channel();
        
        std::string chars =
          xsd::cxx::xml::transcode<char>(chars_->str().c_str());
        
        if(strcasecmp(name, "updated") == 0)
        {
          try
          {
//            channel.last_build_date.set_iso8601(chars.c_str());
            
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
        else if(strcasecmp(name, "title") == 0)
        {
          channel.title = chars;

#     ifdef RSS_PARSER_TRACE          
          std::cerr << "ChannelParser::title: " << channel.title << std::endl;
#     endif
        }
        else if(strcasecmp(name, "subtitle") == 0)
        {
          channel.description = chars;
          
#     ifdef RSS_PARSER_TRACE          
          std::cerr << "ChannelParser::description: " << channel.description
                    << std::endl;
#     endif
        }

        chars_.reset(0);
      }
    }
    
  }
}

#endif // _NEWSGATE_SERVER_XSD_DATAFEED_RSS_ATOMPARSER_HPP_
