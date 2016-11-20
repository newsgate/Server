/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Feeds/RSS/Commons/TransportImpl.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <El/CORBA/Corba.hpp>

#include <string>
#include <sstream>
#include <iostream>

#include <El/Exception.hpp>
#include <El/Moment.hpp>
#include <El/Lang.hpp>
#include <El/Country.hpp>

#include <Services/Feeds/RSS/Commons/RSSFeedServices.hpp>

#include "TransportImpl.hpp"

namespace NewsGate
{
  namespace RSS
  {
    namespace Transport
    {
      //
      // FeedStat class
      //
      FeedStat::FeedStat() throw()
          : id(0),
            requests(0),
            failed(0),
            unchanged(0),
            not_modified(0),
            presumably_unchanged(0),
            has_changes(0),
            wasted(0),
            outbound(0),
            inbound(0),
            requests_duration(0),
            messages(0),
            messages_size(0),
            messages_delay(0),
            max_message_delay(0),
            mistiming(0)
      {
      }

      void
      FeedStat::write(El::BinaryOutStream& bstr) const throw(El::Exception)
      {
        bstr << id << requests << failed << unchanged << not_modified
             << presumably_unchanged << has_changes << wasted << outbound
             << inbound << requests_duration << messages << messages_size
             << messages_delay << max_message_delay << mistiming;
      }
      
      void
      FeedStat::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> id >> requests >> failed >> unchanged >> not_modified
             >> presumably_unchanged >> has_changes >> wasted >> outbound
             >> inbound >> requests_duration >> messages >> messages_size
             >> messages_delay >> max_message_delay >> mistiming;
      }

      //
      // FeedsStatistics class
      //
      
      void
      FeedsStatistics::write(El::BinaryOutStream& bstr) const
        throw(El::Exception)
      {
        bstr << date;
        bstr.write_array(feeds_stat);
      }
      
      void
      FeedsStatistics::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> date;
        bstr.read_array(feeds_stat);
      }
      
      //
      // FeedStateUpdate class
      //
      void
      FeedStateUpdate::write(El::BinaryOutStream& bstr) const
        throw(El::Exception)
      {
        bstr << id << updated_fields;

        if(updated_fields & UF_HTTP_LAST_MODIFIED_HDR)
        {
          bstr << http.last_modified_hdr;
        }

        if(updated_fields & UF_HTTP_ETAG_HDR)
        {
          bstr << http.etag_hdr;
        }

        if(updated_fields & UF_HTTP_CONTENT_LENGTH_HDR)
        {
          bstr << http.content_length_hdr;
        }

        if(updated_fields & UF_HTTP_SINGLE_CHUNKED)
        {
          bstr << http.single_chunked;
        }
      
        if(updated_fields & UF_HTTP_FIRST_CHUNK_SIZE)
        {
          bstr << http.first_chunk_size;
        }
        
        if(updated_fields & UF_HTTP_NEW_LOCATION)
        {
          bstr << http.new_location;
        }

        if(updated_fields & UF_CHANNEL_TITLE)
        {
          bstr << channel.title;
        }
      
        if(updated_fields & UF_CHANNEL_DESCRIPTION)
        {
          bstr << channel.description;
        }
      
        if(updated_fields & UF_CHANNEL_HTML_LINK)
        {
          bstr << channel.html_link;
        }
      
        if(updated_fields & UF_CHANNEL_LANG)
        {
          bstr << channel.lang;
        }
      
        if(updated_fields & UF_CHANNEL_COUNTRY)
        {
          bstr << channel.country;
        }
      
        if(updated_fields & UF_CHANNEL_TTL)
        {
          bstr << channel.ttl;
        }
      
        if(updated_fields & UF_CHANNEL_TYPE)
        {
          bstr << channel.type;
        }
      
        if(updated_fields & UF_CHANNEL_LAST_BUILD_DATE)
        {
          bstr << channel.last_build_date;
        }
      
        if(updated_fields & UF_LAST_REQUEST_DATE)
        {
          bstr << last_request_date;
        }
      
        if(updated_fields & UF_ENTROPY)
        {
          bstr << entropy;
        }
      
        if(updated_fields & UF_ENTROPY_UPDATED_DATE)
        {
          bstr << entropy_updated_date;
        }
      
        if(updated_fields & UF_SIZE)
        {
          bstr << size;
        }
      
        if(updated_fields & UF_HEURISTIC_COUNTER)
        {
          bstr << heuristics_counter;
        }

        if(updated_fields & UF_EXPIRED_MESSAGES)
        {
          bstr.write_array(expired_messages);
        }

        if(updated_fields & UF_UPDATED_MESSAGES)
        {
          bstr.write_array(updated_messages);
        }

        if(updated_fields & UF_NEW_MESSAGES)
        {
          bstr.write_array(new_messages);
        }

        if(updated_fields & UF_CACHE)
        {
          bstr << cache;
        }
      }
    
      void
      FeedStateUpdate::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> id >> updated_fields;

        if(updated_fields & UF_HTTP_LAST_MODIFIED_HDR)
        {
          bstr >> http.last_modified_hdr;
        }

        if(updated_fields & UF_HTTP_ETAG_HDR)
        {
          bstr >> http.etag_hdr;
        }

        if(updated_fields & UF_HTTP_CONTENT_LENGTH_HDR)
        {
          bstr >> http.content_length_hdr;
        }

        if(updated_fields & UF_HTTP_SINGLE_CHUNKED)
        {
          bstr >> http.single_chunked;
        }
      
        if(updated_fields & UF_HTTP_FIRST_CHUNK_SIZE)
        {
          bstr >> http.first_chunk_size;
        }

        if(updated_fields & UF_HTTP_NEW_LOCATION)
        {
          bstr >> http.new_location;
        }

        if(updated_fields & UF_CHANNEL_TITLE)
        {
          bstr >> channel.title;
        }
      
        if(updated_fields & UF_CHANNEL_DESCRIPTION)
        {
          bstr >> channel.description;
        }
      
        if(updated_fields & UF_CHANNEL_HTML_LINK)
        {
          bstr >> channel.html_link;
        }
      
        if(updated_fields & UF_CHANNEL_LANG)
        {
          bstr >> channel.lang;
        }
      
        if(updated_fields & UF_CHANNEL_COUNTRY)
        {
          bstr >> channel.country;
        }
      
        if(updated_fields & UF_CHANNEL_TTL)
        {
          bstr >> channel.ttl;
        }
      
        if(updated_fields & UF_CHANNEL_TYPE)
        {
          bstr >> channel.type;
        }
      
        if(updated_fields & UF_CHANNEL_LAST_BUILD_DATE)
        {
          bstr >> channel.last_build_date;
        }
      
        if(updated_fields & UF_LAST_REQUEST_DATE)
        {
          bstr >> last_request_date;
        }
      
        if(updated_fields & UF_ENTROPY)
        {
          bstr >> entropy;
        }
      
        if(updated_fields & UF_ENTROPY_UPDATED_DATE)
        {
          bstr >> entropy_updated_date;
        }
      
        if(updated_fields & UF_SIZE)
        {
          bstr >> size;
        }
      
        if(updated_fields & UF_HEURISTIC_COUNTER)
        {
          bstr >> heuristics_counter;
        }

        if(updated_fields & UF_EXPIRED_MESSAGES)
        {
          bstr.read_array(expired_messages);
        }

        if(updated_fields & UF_UPDATED_MESSAGES)
        {
          bstr.read_array(updated_messages);
        }

        if(updated_fields & UF_NEW_MESSAGES)
        {
          bstr.read_array(new_messages);
        }

        if(updated_fields & UF_CACHE)
        {
          bstr >> cache;
        }
      }

      void
      FeedStateUpdate::dump(std::ostream& ostr) const throw(El::Exception)
      {      
        ostr << "id: " << id << "\n  updated_fields: 0x" << std::hex
             << updated_fields << std::dec;

        if(updated_fields & UF_HTTP_LAST_MODIFIED_HDR)
        {
          ostr << "\n  HTTP_LAST_MODIFIED_HDR: " << http.last_modified_hdr;
        }

        if(updated_fields & UF_HTTP_ETAG_HDR)
        {
          ostr << "\n  HTTP_ETAG_HDR: " << http.etag_hdr;
        }

        if(updated_fields & UF_HTTP_CONTENT_LENGTH_HDR)
        {
          ostr << "\n  HTTP_CONTENT_LENGTH_HDR: " << http.content_length_hdr;
        }

        if(updated_fields & UF_HTTP_SINGLE_CHUNKED)
        {
          ostr << "\n  HTTP_SINGLE_CHUNKED: " << (long)http.single_chunked;
        }

        if(updated_fields & UF_HTTP_FIRST_CHUNK_SIZE)
        {
          ostr << "\n  HTTP_FIRST_CHUNK_SIZE: " << http.first_chunk_size;
        }

        if(updated_fields & UF_HTTP_NEW_LOCATION)
        {
          ostr << "\n  HTTP_NEW_LOCATION: " << http.new_location;
        }

        if(updated_fields & UF_CHANNEL_TITLE)
        {
          ostr << "\n  CHANNEL_TITLE: " << channel.title;
        }

        if(updated_fields & UF_CHANNEL_DESCRIPTION)
        {
          ostr << "\n  CHANNEL_DESCRIPTION: " << channel.description;
        }

        if(updated_fields & UF_CHANNEL_HTML_LINK)
        {
          ostr << "\n  CHANNEL_HTML_LINK: " << channel.html_link;
        }

        if(updated_fields & UF_CHANNEL_LANG)
        {
          ostr << "\n  CHANNEL_LANG: "
               << El::Lang((El::Lang::ElCode)channel.lang);
        }

        if(updated_fields & UF_CHANNEL_COUNTRY)
        {
          ostr << "\n  CHANNEL_COUNTRY: "
               << El::Country((El::Country::ElCode)channel.country);
        }

        if(updated_fields & UF_CHANNEL_TTL)
        {
          ostr << "\n  CHANNEL_TTL: " << channel.ttl;
        }

        if(updated_fields & UF_CHANNEL_TYPE)
        {
          ostr << "\n  CHANNEL_TYPE: " << channel.type;
        }

        if(updated_fields & UF_CHANNEL_LAST_BUILD_DATE)
        {
          ostr << "\n  CHANNEL_LAST_BUILD_DATE: "
               << El::Moment(ACE_Time_Value(
                               channel.last_build_date)).rfc0822();
        }

        if(updated_fields & UF_LAST_REQUEST_DATE)
        {
          ostr << "\n  LAST_REQUEST_DATE: "
               << El::Moment(ACE_Time_Value(last_request_date)).rfc0822();
        }

        if(updated_fields & UF_ENTROPY)
        {
          ostr << "\n  ENTROPY: " << entropy;
        }

        if(updated_fields & UF_ENTROPY_UPDATED_DATE)
        {
          ostr << "\n  ENTROPY_UPDATED_DATE: "
               << El::Moment(ACE_Time_Value(entropy_updated_date)).rfc0822();
        }

        if(updated_fields & UF_SIZE)
        {
          ostr << "\n  SIZE: " << size;
        }
      
        if(updated_fields & UF_HEURISTIC_COUNTER)
        {
          ostr << "\n  HEURISTIC_COUNTER: " << heuristics_counter;
        }
      
        if(updated_fields & UF_EXPIRED_MESSAGES)
        {
          ostr << "\n  EXPIRED_MESSAGES (" << expired_messages.size() << "): "
               << expired_messages.string();        
        }
      
        if(updated_fields & UF_UPDATED_MESSAGES)
        {
          ostr << "\n  UPDATED_MESSAGES: (" << updated_messages.size() << "): "
               << updated_messages.string();
        }
      
        if(updated_fields & UF_NEW_MESSAGES)
        {
          ostr << "\n  NEW_MESSAGES: (" << new_messages.size() << "): "
               << new_messages.string();
        }

        if(updated_fields & UF_CACHE)
        {
          ostr << "\n  CACHE (" << cache.size() << ")";
        }
      
        ostr << std::endl;
      }

      void
      register_valuetype_factories(CORBA::ORB* orb)
        throw(CORBA::Exception, El::Exception)
      {
        CORBA::ValueFactoryBase_var factory =
          new NewsGate::RSS::Transport::FeedPackImpl::Init();
      
        CORBA::ValueFactoryBase_var old = orb->register_value_factory(
          "IDL:NewsGate/RSS/FeedPack:1.0",
          factory);

        factory = new NewsGate::RSS::Transport::FeedsStatisticsImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/RSS/FeedsStatistics:1.0",
          factory);

        factory =
          new NewsGate::RSS::Transport::FeedStateUpdatePackImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/RSS/FeedStateUpdatePack:1.0",
          factory);
      }
      
    }
  }
}
