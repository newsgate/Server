/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Commons/Statistics/StatLogger.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_SERVICES_COMMONS_STATISTICS_STATLOGGER_HPP_
#define _NEWSGATE_SERVER_SERVICES_COMMONS_STATISTICS_STATLOGGER_HPP_

#include <stdint.h>

#include <string>
#include <vector>
#include <memory>

#include <ext/hash_set>

#include <ace/Synch.h>
#include <ace/Guard_T.h>

#include <El/Exception.hpp>
#include <El/Moment.hpp>
#include <El/BinaryStream.hpp>
#include <El/Guid.hpp>
#include <El/Luid.hpp>
#include <El/Hash/Hash.hpp>
#include <El/Service/CompoundService.hpp>

#include <Commons/Message/Message.hpp>
#include <Commons/Message/TransportImpl.hpp>

#include <Services/Commons/Statistics/StatisticsServices.hpp>
#include <Services/Commons/FraudPrevention/LimitCheck.hpp>
#include <Services/Commons/FraudPrevention/FraudPreventionServices.hpp>
#include <Services/Commons/Message/BankClientSessionImpl.hpp>

namespace NewsGate
{
  namespace Statistics
  {
    struct ClientInfo
    {
      uint8_t type;
      uint8_t device_type;
      char lang[4];
      char country[4];
      char ip[16];
      char name[21];
      char device[21];
      char os[21];
      char id[33];
      char user_agent[257];

      void agent(const char* val) throw(El::Exception);

      ClientInfo() throw();
      
      void write(El::BinaryOutStream& bstr) const throw(El::Exception);      
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };

    struct RefererInfo
    {
      uint8_t internal;
      
      char site[257];
      char company_domain[257];
      char url[1025];
      char page[1025];

      RefererInfo() throw();

      void referer(const char* ref, const char* site_host)
        throw(El::Exception);
      
      void write(El::BinaryOutStream& bstr) const throw(El::Exception);      
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };

    struct Filter
    {
      char lang[4];
      char country[4];
      uint64_t event;
      char feed[513];
      char category[1025];
      
      Filter() throw();
      
      void write(El::BinaryOutStream& bstr) const throw(El::Exception);      
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };
    
    struct SearchModifier
    {
      uint8_t type;
      char value[1025];
      
      SearchModifier() throw();
      
      void write(El::BinaryOutStream& bstr) const throw(El::Exception);      
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };
    
    struct Locale
    {
      char lang[4];
      char country[4];
      
      Locale() throw();
      
      void write(El::BinaryOutStream& bstr) const throw(El::Exception);      
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };
    
    struct RequestParams
    {
      uint8_t protocol;
      uint8_t create_informer;
      uint8_t columns;
      uint8_t sorting_type;
      uint8_t suppression_type;
      uint8_t message_view;
      uint8_t print_left_bar;

      char translate_def_lang[4];
      char translate_lang[4];
      
      uint32_t start_item;
      uint32_t item_count;
      int32_t  annotation_len;
      uint32_t sr_flags;
      uint64_t gm_flags;
      
      Locale locale;
      char query[1025];
      char informer_params[1025];
      SearchModifier modifier;
      Filter filter;

      RequestParams() throw();

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };

    struct ResponseInfo
    {
      uint8_t messages_loaded;
      uint32_t total_matched_messages;
      uint32_t suppressed_messages;

      char optimized_query[1025];

      ResponseInfo() throw();

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };

    struct Time
    {
      uint32_t usec;
      uint64_t sec;
      char datetime[20];

      Time() throw();

      void time(const ACE_Time_Value& val) throw(El::Exception);

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);      
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };
    
    struct RequestInfo
    {
      uint32_t request_duration; // milliseconds
      uint32_t search_duration; // milliseconds

      Time time;      
      ResponseInfo response;

      char id[33];
      char host[257];
      
      ClientInfo client;
      RefererInfo referer;
      RequestParams params;

      RequestInfo() throw();
      
      void write(El::BinaryOutStream& bstr) const throw(El::Exception);      
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };

    typedef std::vector<RequestInfo> RequestInfoArray;
    
    struct PageImpressionInfo
    {
      uint8_t protocol;
      Time time;      
      char id[33];
      RefererInfo referer;

      PageImpressionInfo() throw();

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);      
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };

    typedef std::vector<PageImpressionInfo> PageImpressionInfoArray;
    
    struct MessageImpressionInfo
    {
      Time time;      
      Message::Transport::MessageStatInfoArray messages;
      char id[33];
      char client_id[33];

      MessageImpressionInfo() throw();

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);      
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };

    typedef std::vector<MessageImpressionInfo> MessageImpressionInfoArray;
    
    struct MessageClickInfo
    {
      char ip[16];
      Time time;      
      Message::Transport::MessageStatInfoArray messages;
      char id[33];
      char client_id[33];

      MessageClickInfo() throw();

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);      
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };

    typedef std::vector<MessageClickInfo> MessageClickInfoArray;

    struct MessageVisitInfo
    {
      uint64_t time;
      NewsGate::Message::IdArray messages;

      typedef std::vector<El::Luid> EventIdArray;
      EventIdArray events;

      MessageVisitInfo() throw() : time(0) {}      

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);      
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };
    
    typedef std::vector<MessageVisitInfo> MessageVisitInfoArray;

    class StatLogger : public virtual El::Service::CompoundService<>
    {
    public:
      EL_EXCEPTION(Exception, El::ExceptionBase);

      typedef __gnu_cxx::hash_set<std::string, El::Hash::String> IpSet;

    public:

      StatLogger(
        const char* stat_processor_ref,
        const char* limit_checker_ref,
        const FraudPrevention::EventLimitCheckDescArray&
          limit_check_descriptors,
        const IpSet& limit_check_ip_whitelist,
        CORBA::ORB_ptr orb,
        time_t flush_period,
        El::Service::Callback* callback)
        throw(Exception, El::Exception);

      virtual ~StatLogger() throw() {}

      void search_request(const RequestInfo& request)
        throw(Exception, El::Exception);
      
      void page_impression(const PageImpressionInfo& impression)
        throw(Exception, El::Exception);
      
      void message_impression(const MessageImpressionInfo& impression)
        throw(Exception, El::Exception);
      
      void message_click(const MessageClickInfo& click)
        throw(Exception, El::Exception);
      
      void message_visit(const MessageVisitInfo& visit)
        throw(Exception, El::Exception);
      
      void flush(const char* caller) throw(Exception, El::Exception);
      
      virtual void wait() throw(Exception, El::Exception);

      static void guid(const char* src, char* dest) throw();

    private:
      
      virtual bool notify(El::Service::Event* event) throw(El::Exception);

      Message::BankClientSession* bank_client_session()
        throw(Exception, El::Exception);    

    private:

      struct Flush : public El::Service::CompoundServiceMessage
      {
        Flush(StatLogger* logger) throw(El::Exception);
        ~Flush() throw() {}
      };

      typedef El::Corba::SmartRef<StatProcessor> StatProcessorRef;
      
      typedef El::Corba::SmartRef< ::NewsGate::FraudPrevention::LimitChecker >
      LimitCheckerRef;
      
      typedef El::Corba::SmartRef<Message::BankManager> MessageBankManagerRef;

      typedef ACE_Thread_Mutex Mutex;
      typedef ACE_Guard<Mutex> Guard;
      
      Mutex lock_;

      StatProcessorRef stat_processor_;
      LimitCheckerRef limit_checker_;
      FraudPrevention::EventLimitCheckDescArray limit_check_descriptors_;
      IpSet limit_check_ip_whitelist_;
      
      Message::BankClientSessionImpl_var bank_client_session_;
      
      ACE_Time_Value flush_period_;

      typedef std::auto_ptr<RequestInfoArray> RequestInfoArrayPtr;

      typedef std::auto_ptr<PageImpressionInfoArray>
      PageImpressionInfoArrayPtr;
      
      typedef std::auto_ptr<MessageImpressionInfoArray>
      MessageImpressionInfoArrayPtr;
      
      typedef std::auto_ptr<MessageClickInfoArray> MessageClickInfoArrayPtr;
      typedef std::auto_ptr<MessageVisitInfoArray> MessageVisitInfoArrayPtr;
      
      RequestInfoArrayPtr requests_;
      PageImpressionInfoArrayPtr page_impressions_;
      MessageImpressionInfoArrayPtr message_impressions_;
      MessageClickInfoArrayPtr message_clicks_;
      MessageVisitInfoArrayPtr message_visits_;
    };

    typedef El::RefCount::SmartPtr<StatLogger> StatLogger_var;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Statistics
  {
    //
    // ClientInfo struct
    //

    inline
    ClientInfo::ClientInfo() throw() : type('U'), device_type('U')
    {
      *lang = '\0';
      lang[sizeof(lang) - 1] = '\0';
        
      *country = '\0';
      country[sizeof(country) - 1] = '\0';

      *ip = '\0';
      ip[sizeof(ip) - 1] = '\0';
        
      *name = '\0';
      name[sizeof(name) - 1] = '\0';
        
      *device = '\0';
      device[sizeof(device) - 1] = '\0';
        
      *os = '\0';
      os[sizeof(os) - 1] = '\0';
        
      *id = '\0';      
      id[sizeof(id) - 1] = '\0';
        
      *user_agent = '\0';
      user_agent[sizeof(user_agent) - 1] = '\0';    
    }
    
    inline
    void
    ClientInfo::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << type << device_type;
      
      bstr.write_string_buff(lang);
      bstr.write_string_buff(country);
      bstr.write_string_buff(ip);
      bstr.write_string_buff(name);
      bstr.write_string_buff(device);
      bstr.write_string_buff(os);
      bstr.write_string_buff(id);
      bstr.write_string_buff(user_agent);
    }
    
    inline
    void
    ClientInfo::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> type >> device_type;
      
      bstr.read_string_buff(lang, sizeof(lang));
      bstr.read_string_buff(country, sizeof(country));
      bstr.read_string_buff(ip, sizeof(ip));
      bstr.read_string_buff(name, sizeof(name));
      bstr.read_string_buff(device, sizeof(device));
      bstr.read_string_buff(os, sizeof(os));
      bstr.read_string_buff(id, sizeof(id));
      bstr.read_string_buff(user_agent, sizeof(user_agent));
    }
    
    //
    // RefererInfo struct
    //

    inline
    RefererInfo::RefererInfo() throw() : internal(0)
    {
      *site = '\0';
      site[sizeof(site) - 1] = '\0';
        
      *company_domain = '\0';
      company_domain[sizeof(company_domain) - 1] = '\0';
        
      *url = '\0';
      url[sizeof(url) - 1] = '\0';
        
      *page = '\0';
      page[sizeof(page) - 1] = '\0';        
    }
    
    inline
    void
    RefererInfo::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << internal;
      
      bstr.write_string_buff(site);
      bstr.write_string_buff(company_domain);
      bstr.write_string_buff(url);
      bstr.write_string_buff(page);
    }
    
    inline
    void
    RefererInfo::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> internal;

      bstr.read_string_buff(site, sizeof(site));
      bstr.read_string_buff(company_domain, sizeof(company_domain));
      bstr.read_string_buff(url, sizeof(url));
      bstr.read_string_buff(page, sizeof(page));
    }

    //
    // SearchModifier struct
    //
    inline
    SearchModifier::SearchModifier() throw() : type('N')
    {
      *value = '\0';
      value[sizeof(value) - 1] = '\0';
    }
    
    inline
    void
    SearchModifier::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << type;
      bstr.write_string_buff(value);
    }

    inline
    void
    SearchModifier::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> type;
      bstr.read_string_buff(value, sizeof(value));      
    }
      
    //
    // Filter struct
    //
    inline
    Filter::Filter() throw() : event(0)
    {
      *lang = '\0';
      lang[sizeof(lang) - 1] = '\0';

      *country = '\0';
      country[sizeof(country) - 1] = '\0';

      *feed = '\0';
      feed[sizeof(feed) - 1] = '\0';

      *category = '\0';
      category[sizeof(category) - 1] = '\0';
    }
    
    inline
    void
    Filter::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr.write_string_buff(lang);
      bstr.write_string_buff(country);

      bstr << event;
      
      bstr.write_string_buff(feed);
      bstr.write_string_buff(category);
    }

    inline
    void
    Filter::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr.read_string_buff(lang, sizeof(lang));
      bstr.read_string_buff(country, sizeof(country));
      
      bstr >> event;

      bstr.read_string_buff(feed, sizeof(feed));
      bstr.read_string_buff(category, sizeof(category));
    }

    //
    // Locale struct
    //
    inline
    Locale::Locale() throw()
    {
      *lang = '\0';
      lang[sizeof(lang) - 1] = '\0';

      *country = '\0';
      country[sizeof(country) - 1] = '\0';
    }
    
    inline
    void
    Locale::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr.write_string_buff(lang);
      bstr.write_string_buff(country);
    }

    inline
    void
    Locale::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr.read_string_buff(lang, sizeof(lang));
      bstr.read_string_buff(country, sizeof(country));
    }

    //
    // RequestParams struct
    //
    inline
    RequestParams::RequestParams() throw()
        : protocol('H'),
          create_informer(0),
          columns(0),
          sorting_type(0),
          suppression_type(0),
          message_view('P'),
          print_left_bar(0),
          start_item(0),
          item_count(0),
          annotation_len(-1),
          sr_flags(0),
          gm_flags(0)
    {
      *translate_def_lang = '\0';
      translate_def_lang[sizeof(translate_def_lang) - 1] = '\0';
      
      *translate_lang = '\0';
      translate_lang[sizeof(translate_lang) - 1] = '\0';

      *query = '\0';
      query[sizeof(query) - 1] = '\0';

      *informer_params = '\0';
      informer_params[sizeof(informer_params) - 1] = '\0';
    }
    
    inline
    void
    RequestParams::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << protocol << create_informer << columns << sorting_type
           << suppression_type << message_view << print_left_bar
           << start_item << item_count << annotation_len << sr_flags
           << gm_flags << locale;
      
      bstr.write_string_buff(translate_def_lang);
      bstr.write_string_buff(translate_lang);        
      bstr.write_string_buff(query);
      bstr.write_string_buff(informer_params);
      bstr << modifier << filter;
    }

    inline
    void
    RequestParams::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> protocol >> create_informer >> columns >> sorting_type
           >> suppression_type >> message_view >> print_left_bar
           >> start_item >> item_count >> annotation_len >> sr_flags
           >> gm_flags >> locale;

      bstr.read_string_buff(translate_def_lang, sizeof(translate_def_lang));
      bstr.read_string_buff(translate_lang, sizeof(translate_lang));      
      bstr.read_string_buff(query, sizeof(query));
      bstr.read_string_buff(informer_params, sizeof(informer_params));
      bstr >> modifier >> filter;
    }

    //
    // ResponseInfo struct
    //
    inline
    ResponseInfo::ResponseInfo() throw()
        : messages_loaded(0),
          total_matched_messages(0),
          suppressed_messages(0)
    {
      *optimized_query = '\0';
      optimized_query[sizeof(optimized_query) - 1] = '\0';
    }
    
    inline
    void
    ResponseInfo::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << messages_loaded << total_matched_messages << suppressed_messages;
      bstr.write_string_buff(optimized_query);
    }
    
    inline
    void
    ResponseInfo::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> messages_loaded >> total_matched_messages >> suppressed_messages;
      bstr.read_string_buff(optimized_query, sizeof(optimized_query));
    }
    
    //
    // Time struct
    //
    inline
    Time::Time() throw() : usec(0), sec(0)
    {
      *datetime = '\0';      
      datetime[sizeof(datetime) - 1] = '\0';
    }

    inline
    void
    Time::time(const ACE_Time_Value& val) throw(El::Exception)
    {
      usec = val.usec();
      sec = val.sec();
      
      strcpy(datetime,
             El::Moment(ACE_Time_Value(sec)).iso8601(false, true).c_str());
    }
    
    inline
    void
    Time::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << sec << usec;
      bstr.write_string_buff(datetime);
    }
    
    inline
    void
    Time::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> sec >> usec;
      bstr.read_string_buff(datetime, sizeof(datetime));
    }
    
    //
    // RequestInfo struct
    //
    inline
    RequestInfo::RequestInfo() throw()
        : request_duration(0),
          search_duration(0)
    {
      *id = '\0';
      id[sizeof(id) - 1] = '\0';
        
      *host = '\0';
      host[sizeof(host) - 1] = '\0';        
    }
    
    inline
    void
    RequestInfo::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << request_duration << search_duration << time << response;
      bstr.write_string_buff(id);
      bstr.write_string_buff(host);
      bstr << client << referer << params;
    }
    
    inline
    void
    RequestInfo::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> request_duration >> search_duration >> time >> response;
      bstr.read_string_buff(id, sizeof(id));
      bstr.read_string_buff(host, sizeof(host));
      bstr >> client;
      bstr >> referer;
      bstr >> params;
    }

    //
    // PageImpressionInfo struct
    //
    
    inline
    PageImpressionInfo::PageImpressionInfo() throw()
        : protocol('H')
    {
      *id = '\0';
      id[sizeof(id) - 1] = '\0';
    }

    inline
    void
    PageImpressionInfo::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr.write_string_buff(id);
      bstr << protocol << time << referer;
    }

    inline
    void
    PageImpressionInfo::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr.read_string_buff(id, sizeof(id));
      bstr >> protocol >> time >> referer;
    }
    
    //
    // MessageImpressionInfo struct
    //
    
    inline
    MessageImpressionInfo::MessageImpressionInfo() throw()
    {
      *id = '\0';
      id[sizeof(id) - 1] = '\0';
      
      *client_id = '\0';
      client_id[sizeof(client_id) - 1] = '\0';
    }

    inline
    void
    MessageImpressionInfo::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr << time;
      bstr.write_string_buff(id);
      bstr.write_string_buff(client_id);
      bstr.write_array(messages);
    }

    inline
    void
    MessageImpressionInfo::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> time;
      bstr.read_string_buff(id, sizeof(id));
      bstr.read_string_buff(client_id, sizeof(client_id));
      bstr.read_array(messages);
    }
    
    //
    // MessageClickInfo struct
    //
    
    inline
    MessageClickInfo::MessageClickInfo() throw()
    {
      *ip = '\0';
      ip[sizeof(ip) - 1] = '\0';

      *id = '\0';
      id[sizeof(id) - 1] = '\0';

      *client_id = '\0';
      client_id[sizeof(client_id) - 1] = '\0';
    }

    inline
    void
    MessageClickInfo::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr.write_string_buff(ip);
      bstr << time;
      bstr.write_string_buff(id);
      bstr.write_string_buff(client_id);
      bstr.write_array(messages);
    }

    inline
    void
    MessageClickInfo::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr.read_string_buff(ip, sizeof(ip));
      bstr >> time;
      bstr.read_string_buff(id, sizeof(id));
      bstr.read_string_buff(client_id, sizeof(client_id));
      bstr.read_array(messages);
    }

    //
    // MessageVisitInfo struct
    //
    
    inline
    void
    MessageVisitInfo::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr << time;
      bstr.write_array(messages);
      bstr.write_array(events);
    }
    
    inline
    void
    MessageVisitInfo::read(El::BinaryInStream& bstr) throw(El::Exception)
    {  
      bstr >> time;
      bstr.read_array(messages);
      bstr.read_array(events);
    }
    
    //
    // StatLogger class
    //
    inline
    void 
    StatLogger::guid(const char* src, char* dest) throw()
    {
      if(src)
      {
        try
        {
          strcpy(dest, El::Guid(src).string(El::Guid::GF_DENSE).c_str());
          return;
        }
        catch(const El::Guid::Exception&)
        {
        }
      }

      *dest = '\0';  
    }
    
    //
    // StatLogger::Flush class
    //
    inline
    StatLogger::Flush::Flush(StatLogger* logger) throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(logger, logger, false),
          El::Service::CompoundServiceMessage(logger, logger)
    {
    }
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_COMMONS_STATISTICS_STATLOGGER_HPP_
