/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Feeds/RSS/Commons/TransportImpl.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_COMMONS_TRANSPORTIMPL_IDL_
#define _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_COMMONS_TRANSPORTIMPL_IDL_

#include <stdint.h>

#include <vector>
#include <iostream>

#include <El/Exception.hpp>

#include <El/CORBA/Transport/EntityPack.hpp>

#include <Commons/Message/Message.hpp>
#include <Services/Feeds/RSS/Commons/RSSFeedServices.hpp>

namespace NewsGate
{
  namespace RSS
  {
    namespace Transport
    {
      struct ChannelState
      {
        std::string title;
        std::string description;
        std::string html_link;
        uint16_t type;
        uint16_t lang;
        uint16_t country;
        uint16_t ttl;
        uint64_t last_build_date;

        ChannelState() throw();

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);        
      };

      struct HTTPState
      {
        std::string last_modified_hdr;
        std::string etag_hdr;
        int64_t content_length_hdr;      
        int8_t  single_chunked;
        int64_t first_chunk_size;
        std::string new_location;

        HTTPState() throw();

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);        
      };

      struct FeedStateUpdate
      {
        enum UpdatedFields
        {
          UF_HTTP_LAST_MODIFIED_HDR = 0x1,
          UF_HTTP_ETAG_HDR = 0x2,
          UF_HTTP_CONTENT_LENGTH_HDR = 0x4,
          UF_HTTP_SINGLE_CHUNKED = 0x8,
          UF_HTTP_FIRST_CHUNK_SIZE = 0x10,          
          UF_HTTP_NEW_LOCATION = 0x20,
          UF_CHANNEL_TITLE = 0x40,
          UF_CHANNEL_DESCRIPTION = 0x80,
          UF_CHANNEL_LANG = 0x100,
          UF_CHANNEL_COUNTRY = 0x200,
          UF_CHANNEL_TTL = 0x400,
          UF_CHANNEL_LAST_BUILD_DATE = 0x800,
          UF_LAST_REQUEST_DATE = 0x1000,
          UF_ENTROPY = 0x2000,
          UF_ENTROPY_UPDATED_DATE = 0x4000,
          UF_SIZE = 0x8000,
          UF_HEURISTIC_COUNTER = 0x10000,
          UF_EXPIRED_MESSAGES = 0x20000,
          UF_UPDATED_MESSAGES = 0x40000,
          UF_NEW_MESSAGES = 0x80000,
          UF_CHANNEL_HTML_LINK = 0x100000,
          UF_CHANNEL_TYPE = 0x200000,
          UF_CACHE = 0x400000
        };
      
        uint32_t updated_fields;
        
        Feed::Id id;
        HTTPState http;
        ChannelState channel;
        uint64_t last_request_date;
        uint32_t entropy;
        uint64_t entropy_updated_date;
        uint32_t size;
        int32_t  heuristics_counter;
        Message::LocalIdArray   expired_messages;
        Message::LocalCodeArray updated_messages;
        Message::LocalCodeArray new_messages;
        std::string cache;

        FeedStateUpdate() throw();

        void channel_title(const char* val) throw(El::Exception);
        void channel_description(const char* val) throw(El::Exception);
        void channel_html_link(const char* val) throw(El::Exception);
        void channel_lang(uint32_t val) throw();
        void channel_country(uint32_t val) throw();
        void channel_ttl(uint32_t val) throw();
        void channel_last_build_date(uint64_t val) throw();
        void channel_type(uint32_t val) throw();

        void http_last_modified_hdr(const char* val) throw(El::Exception);
        void http_etag_hdr(const char* val) throw(El::Exception);
        void http_content_length_hdr(uint64_t val) throw();
        void http_single_chunked(uint8_t val) throw();
        void http_first_chunk_size(uint64_t val) throw();
        void http_new_location(const char* val) throw(El::Exception);

        void set_last_request_date(uint64_t val) throw();
        void set_entropy(uint32_t val) throw();
        void set_entropy_updated_date(uint64_t val) throw();
        void set_size(uint32_t val) throw();
        void set_heuristics_counter(int32_t val) throw();

        void set_cache(const std::string& val) throw();

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);        

        void dump(std::ostream& ostr) const throw(El::Exception);
      };

      typedef std::vector<FeedStateUpdate> FeedStateUpdateArray;

      struct FeedStateUpdatePackImpl
      {
        typedef El::Corba::Transport::EntityPack<
          FeedStateUpdatePack,
          FeedStateUpdate,
          FeedStateUpdateArray,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::EntityPack_init<FeedStateUpdatePack,
                                                      FeedStateUpdateArray,
                                                      Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };

      struct FeedStat
      {
        Feed::Id id;
        uint32_t requests;
        uint32_t failed;
        uint32_t unchanged;
        uint32_t not_modified;
        uint32_t presumably_unchanged;
        uint32_t has_changes;
        float wasted;
        uint32_t outbound;
        uint32_t inbound;
        uint32_t requests_duration;
        uint32_t messages;
        uint32_t messages_size;
        uint32_t messages_delay;
        uint32_t max_message_delay;
        uint64_t mistiming;

        FeedStat() throw();

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);        
      };
      
      typedef std::vector<FeedStat> FeedStatArray;

      struct FeedsStatistics
      {
        uint64_t date;
        FeedStatArray feeds_stat;

        FeedsStatistics() throw() : date(0) {}

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);        
      };

      struct FeedsStatisticsImpl
      {
        typedef El::Corba::Transport::Entity<
          NewsGate::RSS::FeedsStatistics,
          NewsGate::RSS::Transport::FeedsStatistics,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::Entity_init<
          NewsGate::RSS::Transport::FeedsStatistics,
          Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };

      struct FeedState
      {
        Feed::Id id;
        HTTPState http;
        ChannelState channel;
        uint64_t last_request_date;
        uint32_t entropy;
        uint64_t entropy_updated_date;
        uint32_t size;
        int32_t  heuristics_counter;
        Message::LocalCodeArray last_messages;
        std::string cache;

        FeedState() throw();

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);        
      };

      struct Feed
      {
        std::string url;
        std::string encoding;
        uint16_t space;
        uint16_t lang;
        uint16_t country;
        int8_t status;
        std::string keywords;
        std::string adjustment_script;
        FeedState state;

        Feed() throw();
          
        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);        
      };

      typedef std::vector<Feed> FeedArray;

      struct FeedPackImpl
      {
        typedef El::Corba::Transport::EntityPack<
          FeedPack,
          Feed,
          FeedArray,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::EntityPack_init<FeedPack,
                                                      FeedArray,
                                                      Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };
      
      void register_valuetype_factories(CORBA::ORB* orb)
        throw(CORBA::Exception, El::Exception);
    }
  }
}


///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace RSS
  {
    namespace Transport
    {
      
      //
      // FeedStateUpdate struct
      //
      inline
      FeedStateUpdate::FeedStateUpdate() throw()
          : updated_fields(0),
            id(0),
            last_request_date(0),
            entropy(0),
            entropy_updated_date(0),
            size(0),
            heuristics_counter(0)
      {
        http.content_length_hdr = 0;
        http.single_chunked = 0;
        http.first_chunk_size = 0;
        channel.lang = 0;
        channel.country = 0;
        channel.ttl = 0;
        channel.last_build_date = 0;
      }
    
      inline
      void
      FeedStateUpdate::channel_title(const char* val) throw(El::Exception)
      {
        channel.title = val;
        updated_fields |= UF_CHANNEL_TITLE;
      }
    
      inline
      void
      FeedStateUpdate::channel_description(const char* val)
        throw(El::Exception)
      {
        channel.description = val;
        updated_fields |= UF_CHANNEL_DESCRIPTION;
      }

      inline
      void
      FeedStateUpdate::channel_html_link(const char* val)
        throw(El::Exception)
      {
        channel.html_link = val;
        updated_fields |= UF_CHANNEL_HTML_LINK;
      }

      inline
      void
      FeedStateUpdate::channel_lang(uint32_t val) throw()
      {
        channel.lang = val;
        updated_fields |= UF_CHANNEL_LANG;
      }
    
      inline
      void
      FeedStateUpdate::channel_country(uint32_t val) throw()
      {
        channel.country = val;
        updated_fields |= UF_CHANNEL_COUNTRY;
      }
    
      inline
      void
      FeedStateUpdate::channel_ttl(uint32_t val) throw()
      {
        channel.ttl = val;
        updated_fields |= UF_CHANNEL_TTL;
      }
    
      inline
      void
      FeedStateUpdate::channel_type(uint32_t val) throw()
      {
        channel.type = val;
        updated_fields |= UF_CHANNEL_TYPE;
      }
    
      inline
      void
      FeedStateUpdate::channel_last_build_date(uint64_t val) throw()
      {
        channel.last_build_date = val;
        updated_fields |= UF_CHANNEL_LAST_BUILD_DATE;
      }

      inline
      void
      FeedStateUpdate::http_last_modified_hdr(const char* val)
        throw(El::Exception)
      {
        http.last_modified_hdr = val;
        updated_fields |= UF_HTTP_LAST_MODIFIED_HDR;
      }
    
      inline
      void
      FeedStateUpdate::http_etag_hdr(const char* val) throw(El::Exception)
      {
        http.etag_hdr = val;
        updated_fields |= UF_HTTP_ETAG_HDR;
      }
    
      inline
      void
      FeedStateUpdate::http_content_length_hdr(uint64_t val) throw()
      {
        http.content_length_hdr = val;
        updated_fields |= UF_HTTP_CONTENT_LENGTH_HDR;
      }
    
      inline
      void
      FeedStateUpdate::http_single_chunked(uint8_t val) throw()
      {
        http.single_chunked = val;
        updated_fields |= UF_HTTP_SINGLE_CHUNKED;
      }
    
      inline
      void
      FeedStateUpdate::http_first_chunk_size(uint64_t val) throw()
      {
        http.first_chunk_size = val;
        updated_fields |= UF_HTTP_FIRST_CHUNK_SIZE;
      }
    
      inline
      void
      FeedStateUpdate::http_new_location(const char* val) throw(El::Exception)
      {
        http.new_location = val;
        updated_fields |= UF_HTTP_NEW_LOCATION;        
      }
      
      inline
      void
      FeedStateUpdate::set_last_request_date(uint64_t val) throw()
      {
        last_request_date = val;
        updated_fields |= UF_LAST_REQUEST_DATE;
      }
    
      inline
      void
      FeedStateUpdate::set_entropy(uint32_t val) throw()
      {
        entropy = val;
        updated_fields |= UF_ENTROPY;
      }

      inline
      void
      FeedStateUpdate::set_entropy_updated_date(uint64_t val) throw()
      {
        entropy_updated_date = val;
        updated_fields |= UF_ENTROPY_UPDATED_DATE;
      }

      inline
      void
      FeedStateUpdate::set_size(uint32_t val) throw()
      {
        size = val;
        updated_fields |= UF_SIZE;
      }

      inline
      void
      FeedStateUpdate::set_heuristics_counter(int32_t val) throw()
      {
        heuristics_counter = val;
        updated_fields |= UF_HEURISTIC_COUNTER;
      }

      inline
      void
      FeedStateUpdate::set_cache(const std::string& val) throw()
      {
        cache = val;
        updated_fields |= UF_CACHE;        
      }

      //
      // ChannelState class
      //
      inline
      ChannelState::ChannelState() throw()
          : type(0),
            lang(0),
            country(0),
            ttl(0),
            last_build_date(0)
      {
      }
      
      inline
      void
      ChannelState::write(El::BinaryOutStream& bstr) const throw(El::Exception)
      {
        bstr << type << title << description << html_link << lang << country
             << ttl << last_build_date;
      }

      inline
      void
      ChannelState::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> type >> title >> description >> html_link >> lang >> country
             >> ttl >> last_build_date;
      }

      //
      // HTTPState class
      //
      inline
      HTTPState::HTTPState() throw()
          : content_length_hdr(0),
            single_chunked(0),
            first_chunk_size(0)
      {
      }
      
      inline
      void
      HTTPState::write(El::BinaryOutStream& bstr) const throw(El::Exception)
      {
        bstr << last_modified_hdr << etag_hdr << content_length_hdr
             << single_chunked << first_chunk_size << new_location;
      }      

      inline
      void
      HTTPState::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> last_modified_hdr >> etag_hdr >> content_length_hdr
             >> single_chunked >> first_chunk_size >> new_location;
      }

      //
      // FeedState class
      //
      inline
      FeedState::FeedState() throw()
          : id(0),
            last_request_date(0),
            entropy(0),
            entropy_updated_date(0),
            size(0),
            heuristics_counter(0)
      {
      }
      
      inline
      void
      FeedState::write(El::BinaryOutStream& bstr) const throw(El::Exception)
      {
        bstr << id << http << channel << last_request_date
             << entropy << entropy_updated_date << size << heuristics_counter
             << cache;
        
        bstr.write_array(last_messages);
      }

      inline
      void
      FeedState::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> id >> http >> channel >> last_request_date
             >> entropy >> entropy_updated_date >> size >> heuristics_counter
             >> cache;
        
        bstr.read_array(last_messages);
      }

      //
      // Feed class
      //
      inline
      Feed::Feed() throw()
          : space(0),
            lang(0),
            country(0),
            status(0)
      {
      }
      
      inline
      void
      Feed::write(El::BinaryOutStream& bstr) const throw(El::Exception)
      {
        bstr << url << encoding << space << lang << country << status
             << keywords << adjustment_script << state;
      }
      
      inline
      void
      Feed::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> url >> encoding >> space >> lang >> country >> status
             >> keywords >> adjustment_script >> state;
      }
    }
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_COMMONS_TRANSPORTIMPL_IDL_
