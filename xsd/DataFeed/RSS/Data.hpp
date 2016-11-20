/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/xsd/DataFeed/RSS/Data.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_XSD_DATAFEED_RSS_DATA_HPP_
#define _NEWSGATE_SERVER_XSD_DATAFEED_RSS_DATA_HPP_

#include <list>
#include <vector>
#include <string>
#include <iostream>

#include <El/Exception.hpp>
#include <El/Moment.hpp>
#include <El/Lang.hpp>
#include <El/Country.hpp>
#include <El/Geography/AddressInfo.hpp>
#include <El/Net/HTTP/URL.hpp>

#include <Commons/Feed/Types.hpp>
#include <Commons/Message/Message.hpp>

namespace NewsGate
{
  namespace RSS
  {
    EL_EXCEPTION(Exception, El::ExceptionBase);

    struct Enclosure
    {
      std::string url;
      std::string type;

      void clear() throw() { url.clear(); type.clear(); }
    };

    typedef std::vector<Enclosure> EnclosureArray;

    struct Guid
    {
      bool is_permalink;
      std::string value;

      Guid() throw() : is_permalink(true) {}
      void clear() throw() { is_permalink = true; value.clear(); }
    };

    struct Item
    {
      Message::LocalCode code;
      std::string title;
      std::string description;
      std::string url;
      EnclosureArray enclosures;
      std::string keywords;
      Guid guid;

      Item() throw(El::Exception);
      void clear() throw(El::Exception);

      void dump(std::ostream& ostr) const throw(El::Exception);
    };
  
    typedef std::list<Item> ItemList;

    struct Channel
    {
      Feed::Type type;
      std::string title;
      std::string description;
      std::string html_link;
      El::Moment last_build_date;
      unsigned long ttl;
      El::Lang lang;
      El::Country country;
      ItemList items;

      Channel() throw(El::Exception);
      
      void clear() throw(El::Exception);
      void set_lang_and_country(const std::string& lc) throw(El::Exception);

      void adjust(const char* feed_url,
                  const char* feed_immediate_url,
                  const El::Country& feed_country,
                  const El::Lang& feed_lang,
                  El::Geography::AddressInfo* address_info)
        throw(Exception, El::Exception);

      void dump(std::ostream& ostr) const throw(El::Exception);
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
    // Item class
    //

    inline
    Item::Item() throw(El::Exception)
    {
    }
  
    inline
    void 
    Item::clear() throw(El::Exception)
    {
      code.clear();
      title.clear();
      description.clear();
      url.clear();
      enclosures.clear();
      guid.clear();
    }

    inline
    void
    Item::dump(std::ostream& ostr) const throw(El::Exception)
    {
      ostr << "title='" << title << "', description='"
           << description << "', up_date=" << code.pub_date().rfc0822()
           << ", msg_code=" << code.string() << ", url='" << url
           << "', guid=" << guid.is_permalink << "/'" << guid.value << "'";
    }
    
    //
    // Channel class
    //
    inline
    Channel::Channel() throw(El::Exception) : type(Feed::TP_UNDEFINED), ttl(0)
    {
    }

    inline
    void
    Channel::set_lang_and_country(const std::string& lc) throw(El::Exception)
    {
      std::string::size_type pos = lc.find('-');

      std::string cnt;
      std::string lng;
      
      if(pos != std::string::npos)
      {
        cnt = lc.substr(pos + 1);
        lng = lc.substr(0, pos);
      }
      else
      {
        lng = lc;
      }

      if(!lng.empty())
      {
        try
        {
          lang = El::Lang(lng.c_str());
        }
        catch(const El::Lang::InvalidArg& )
        {
        }
      }
      
      if(!cnt.empty())
      {
        try
        {
          country = El::Country(cnt.c_str());
        }
        catch(const El::Country::InvalidArg& )
        {
        }
      }
    }
    
    inline
    void
    Channel::clear() throw(El::Exception)
    {
      type = Feed::TP_UNDEFINED;
      title.clear();
      description.clear();
      html_link.clear();
      last_build_date = El::Moment();
      ttl = 0;
      items.clear();      
    }

    inline
    void
    Channel::dump(std::ostream& ostr) const throw(El::Exception)
    {
      ostr << "type=" << type << ", title='" << title << "', description='"
           << description << "', html_link='" << html_link
           << "', last_build_date=" << last_build_date.rfc0822()
           << ", ttl=" << ttl << ", lang=" << lang << ", country=" << country
           << ", items:" << std::endl;
      
      for(ItemList::const_iterator it = items.begin(); it != items.end(); it++)
      {
        ostr << "  ";
        it->dump(ostr);
        ostr << std::endl;
      }      
    }

    inline
    void
    Channel::adjust(const char* feed_url,
                    const char* feed_immediate_url,
                    const El::Country& feed_country,
                    const El::Lang& feed_lang,
                    El::Geography::AddressInfo* address_info)
      throw(Exception, El::Exception)
    {
      if(feed_country != El::Country::nonexistent)
      {
        country = feed_country;
      }
      else
      {
        El::Country domain_country;

        El::Net::HTTP::URL_var immediate_url =
          new El::Net::HTTP::URL(feed_immediate_url);
            
        El::Net::HTTP::URL_var url = new El::Net::HTTP::URL(feed_url);
          
        try
        {
          std::string domain = std::string(".") +
            El::Net::domain(immediate_url->host(), 1);
          
          domain_country = El::Country(domain.c_str());
        }
        catch(const El::Country::InvalidArg& )
        {
        }

        if(domain_country == El::Country::null)
        {
          try
          {          
            std::string domain = std::string(".") +
              El::Net::domain(url->host(), 1);
            
            domain_country = El::Country(domain.c_str());
          }
          catch(const El::Country::InvalidArg& )
          {
          }
        }
          
        if(domain_country != El::Country::null)
        {
          country = domain_country;
        }
        else if(country == El::Country::null && address_info)
        {
          country = address_info->country(immediate_url->host());

          if(country == El::Country::null)
          {
            country = address_info->country(url->host());
          }
        }
      }

      if(feed_lang != El::Lang::nonexistent)
      {
        lang = feed_lang;
      }
      else if(lang == El::Lang::null && country != El::Country::null)
      {
        lang = country.lang();
      }       
    }
  }
}

#endif // _NEWSGATE_SERVER_XSD_DATAFEED_RSS_DATA_HPP_
