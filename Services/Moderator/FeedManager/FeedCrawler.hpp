/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/FeedManager/FeedCrawler.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MODERATOR_FEEDMANAGER_FEEDCRAWLER_HPP_
#define _NEWSGATE_SERVER_SERVICES_MODERATOR_FEEDMANAGER_FEEDCRAWLER_HPP_

#include <string>
#include <list>
#include <iostream>
#include <ext/hash_map>
#include <ext/hash_set>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/Hash/Hash.hpp>
#include <El/Net/HTTP/URL.hpp>
#include <El/Net/HTTP/Session.hpp>
#include <El/Service/CompoundService.hpp>

#include <Services/Moderator/Commons/FeedManager.hpp>

namespace NewsGate
{
  namespace Moderation
  {
    class FeedCrawler : public virtual El::Service::CompoundService<>
    {
    public:
      EL_EXCEPTION(Exception, El::ExceptionBase);
      EL_EXCEPTION(InvalidArgument, Exception);

    public:
      FeedCrawler(El::Service::Callback* callback)
        throw(InvalidArgument, Exception, El::Exception);

      virtual ~FeedCrawler() throw();

      typedef __gnu_cxx::hash_set<std::string, El::Hash::String> StringSet;
      
      const StringSet& exclude_extensions() const throw();
      
      virtual bool stop() throw(Exception, El::Exception);

      ::NewsGate::Moderation::ResourceValidationResultSeq* validate(
        const ::NewsGate::Moderation::UrlSeq& urls,
        ::NewsGate::Moderation::ProcessingType processing_type,
        ::NewsGate::Moderation::CreatorId creator_id)
        throw(NewsGate::Moderation::NotReady,
              NewsGate::Moderation::ImplementationException,
              CORBA::SystemException);
      
      ::NewsGate::Moderation::ValidationToken validate_async(
        const ::NewsGate::Moderation::UrlSeq& urls,
        ::NewsGate::Moderation::ProcessingType processing_type,
        ::NewsGate::Moderation::CreatorId creator_id,
        ::CORBA::ULong req_period)
        throw(NewsGate::Moderation::NotReady,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);

      ::NewsGate::Moderation::ResourceValidationResultSeq* add_feeds(
        const ::NewsGate::Moderation::UrlSeq& feeds,
        const ::NewsGate::Moderation::FeedSourceSeq& feed_sources,
        ::NewsGate::Moderation::CreatorType creator_type,
        ::NewsGate::Moderation::CreatorId creator_id)
        throw(NewsGate::Moderation::NotReady,
              NewsGate::Moderation::ImplementationException,
              CORBA::SystemException);
      
      ::NewsGate::Moderation::ValidationTaskInfoSeq* task_infos(
        const ::NewsGate::Moderation::CreatorIdSeq& creator_ids)
        throw(NewsGate::Moderation::NotReady,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);
      
      ::NewsGate::Moderation::ResourceValidationResultSeq*
      validation_result(
        const char* task_id,
        const ::NewsGate::Moderation::CreatorIdSeq& creator_ids,
        CORBA::Boolean all_results,
        CORBA::ULong_out total_results)
        throw(NewsGate::Moderation::TaskNotFound,
              NewsGate::Moderation::NotReady,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);
      
      void stop_validation(
        const char* task_id,
        const ::NewsGate::Moderation::CreatorIdSeq& creator_ids)
        throw(NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);
      
      void delete_validation(
        const char* task_id,
        const ::NewsGate::Moderation::CreatorIdSeq& creator_ids)
        throw(NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);
      
    protected:
      
      virtual bool notify(El::Service::Event* event) throw(El::Exception);
      
    private:

      struct ValidateFeeds :
        public El::RefCount::DefaultImpl<El::Sync::ThreadPolicy>
      {
        EL_EXCEPTION(Exception, El::ExceptionBase);

        std::string id;
        std::string title;
        CreatorId creator_id;
        ResourceValidationResultSeq_var result;        
        std::string error;
        
        ValidateFeeds(const UrlSeq& urls,
                      ProcessingType processing_type,
                      bool urgent,
                      CreatorId creator_id_val,
                      unsigned long req_period,
                      FeedCrawler* service) throw(El::Exception);

        virtual ~ValidateFeeds() throw();

        void run(bool async) throw(El::Exception);

        void set_result(const ResourceValidationResult& result,
                        bool task_result)
          throw(Exception, El::Exception);

        void execute() throw(El::Exception);
        
        void awake() throw() { completed_.signal(); }
        void finalize_result() throw(Exception, El::Exception);
  
        void interrupt() throw(El::Exception);

        ValidationStatus status() const throw();
        unsigned long long received_bytes() const throw();
        
        size_t started() const throw();        
        size_t pending_tasks() const throw();
        size_t feeds_count() const throw();
        size_t processed_urls() const throw();        

      private:
        
        enum UrlType
        {
          UT_UNKNOWN,
          UT_FEED,            // Must be feed, register it
          UT_FEED_OR_HTML,    // Must be feed or HTML page, if feed then
                              // register, parse for feed urls or pages
                              // otherwise
          UT_PROBABLY_FEED    // If feed, then register; skip otherwise
        };

        static const unsigned long LOOK_AROUND_DEPTH;
        
        struct UrlDesc
        {
          std::string url;
          UrlType type;
          ProcessingType processing_type;
          std::string referencer;
          size_t go_futher;
          bool user_provided;

          bool require_error_reporting() const throw();

          UrlDesc() throw(El::Exception);
        };

        void validate(const ValidateFeeds::UrlDesc& url_desc,
                      ResourceValidationResult& result) throw(El::Exception);

        void set_error(const char* err) throw();

        UrlDesc get_feed() throw(Exception, El::Exception);
        
        void add_feed(const char* url,
                      UrlType type,
                      bool user_provided,
                      bool add_task,
                      const char* referencer,
                      size_t go_futher)
          throw(Exception, El::Exception);

        void self_schedule() throw(El::Exception);

        void add_feed_i(const char* url,
                        UrlType type,
                        bool user_provided,
                        bool add_task,
                        const char* referencer,
                        size_t go_futher)
          throw(Exception, El::Exception);

        void add_urls_in_url(El::Net::HTTP::URL* url,
                             UrlType type,
                             bool user_provided,
                             bool add_task,
                             const char* referencer,
                             size_t go_futher)
          throw(Exception, El::Exception);
        
        void received_bytes(unsigned long long bytes) throw();
          
        void add_url_for_processing(const char* url,
                                    UrlType type,
                                    bool user_provided,
                                    const char* company_domain,
                                    const char* referencer,
                                    size_t go_futher)
          throw(El::Net::Exception, El::Exception);
        
        El::Net::HTTP::Session* start_session(
          El::Net::HTTP::URL* url,
          std::string& company_domain,
          std::string& new_permanent_location,
          bool& is_html,
          std::string& charset,
          std::string& error)
          throw(El::Net::Exception, El::Exception);
        
        static bool valid_rss(const std::string& feed,
                              const char* feed_url,
                              const char* charset,
                              std::ostream& error)
          throw(El::Exception);
      
        static bool valid_atom(const std::string& feed,
                               const char* feed_url,
                               const char* charset,
                               std::ostream& error)
          throw(El::Exception);
      
        static bool valid_rdf(const std::string& feed,
                              const char* feed_url,
                              const char* charset,
                              std::ostream& error)
          throw(El::Exception);
      
        static std::string html_decode_url(const char* url)
          throw(El::Exception);
        
      private:
        typedef std::vector<UrlDesc> UrlDescArray;

        struct UrlInfo
        {
          size_t reference_counter;

          UrlInfo(size_t reference_counter_val = 0) throw();
        };
        
        typedef __gnu_cxx::hash_map<std::string, UrlInfo, El::Hash::String>
        UrlMap;
        
        typedef __gnu_cxx::hash_map<std::string, std::string, El::Hash::String>
        TempRedirectMap;

      private:
        typedef ACE_Thread_Mutex                Mutex;
        typedef ACE_Guard<Mutex>                Guard;
        typedef ACE_Condition<ACE_Thread_Mutex> Condition;

        mutable Mutex lock_;
        Condition completed_;

        ValidationStatus status_;
        
        ProcessingType processing_type_;
        bool urgent_;
        
        unsigned long req_period_;
        size_t current_feed_;
        size_t pending_tasks_;

        UrlDescArray urls_;

        // Maps urls to number of feeds referenced by content of the url
        UrlMap url_map_;

        // Maps temporary redirects to originating urls.
        // Will not repeat such redirects even originating from different url.
        TempRedirectMap tmp_redirect_map_;

        FeedCrawler* feed_crawler_;

        unsigned long long received_bytes_;
        size_t started_;
        size_t feeds_count_;
        ACE_Time_Value next_request_time_;
      };

      typedef El::RefCount::SmartPtr<ValidateFeeds> ValidateFeeds_var;
      
      struct ValidateFeedsMsg : public El::Service::CompoundServiceMessage
      {
        ValidateFeeds_var validate_feeds_task;

        ValidateFeedsMsg(ValidateFeeds* validate_feeds,
                         FeedCrawler* service) throw(El::Exception);
        
        virtual void execute() throw(El::Exception);
      };
      
    private:
      
      typedef std::list<std::string> StringList;
      
      void parse_meta_url(const UrlDesc& mod_url,
                          UrlSeq& parsed_urls,
                          StringList* meta_urls)
        throw(El::Exception);

      ValidateFeeds*
      create_task(const ::NewsGate::Moderation::UrlSeq& urls,
                  ::NewsGate::Moderation::ProcessingType processing_type,
                  bool urgent,
                  ::NewsGate::Moderation::CreatorId creator_id,
                  unsigned long req_period)
        throw(El::Exception);
      
      ::NewsGate::Moderation::ResourceValidationResultSeq* do_validate(
        ValidateFeeds* validate_task, bool async)
        throw(NewsGate::Moderation::NotReady,
              NewsGate::Moderation::ImplementationException,
              CORBA::SystemException);

      typedef __gnu_cxx::hash_set<Moderation::CreatorId,
                                  El::Hash::Numeric<Moderation::CreatorId> >
      CreatorIdSet;
      
      void creator_id_seq_to_set(
        const ::NewsGate::Moderation::CreatorIdSeq& creator_id_seq,
        CreatorIdSet& creator_id_set)
        throw(El::Exception);

      typedef __gnu_cxx::hash_map<std::string,
                                  ValidateFeeds_var,
                                  El::Hash::String>
      ValidateFeedMap;

      ValidateFeedMap validate_tasks_;
      StringSet exclude_extensions_;
    };

    typedef El::RefCount::SmartPtr<FeedCrawler> FeedCrawler_var;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Moderation
  {
    //
    // NewsGate::Moderation::FeedCrawler
    //
    inline
    const FeedCrawler::StringSet&
    FeedCrawler::exclude_extensions() const throw()
    {
      return exclude_extensions_;
    }
    
    //
    // NewsGate::Moderation::FeedCrawler::ValidateFeeds
    //
    inline
    FeedCrawler::ValidateFeeds::~ValidateFeeds() throw()
    {
    }

    inline
    size_t
    FeedCrawler::ValidateFeeds::started() const throw()
    {
      Guard guard(lock_);
      return started_;
    }
    
    inline
    size_t
    FeedCrawler::ValidateFeeds::pending_tasks() const throw()
    {
      Guard guard(lock_);
      return pending_tasks_;
    }
    
    inline
    size_t
    FeedCrawler::ValidateFeeds::feeds_count() const throw()
    {
      Guard guard(lock_);
      return feeds_count_;
    }
    
    inline
    unsigned long long
    FeedCrawler::ValidateFeeds::received_bytes() const throw()
    {
      Guard guard(lock_);
      return received_bytes_;
    }
    
    inline
    size_t
    FeedCrawler::ValidateFeeds::processed_urls() const throw()
    {
      Guard guard(lock_);
      return result->length();
    }

    inline
    ValidationStatus
    FeedCrawler::ValidateFeeds::status() const throw()
    {
      Guard guard(lock_);
      return status_;
    }
    
    inline
    void
    FeedCrawler::ValidateFeeds::add_feed(const char* url,
                                             UrlType type,
                                             bool user_provided,
                                             bool add_task,
                                             const char* referencer,
                                             size_t go_futher)
      throw(Exception, El::Exception)
    {
      Guard guard(lock_);
      add_feed_i(url, type, user_provided, add_task,referencer, go_futher);
    }
    
    inline
    FeedCrawler::ValidateFeeds::UrlDesc
    FeedCrawler::ValidateFeeds::get_feed() throw(Exception, El::Exception)
    {
      Guard guard(lock_);
      
      if(current_feed_ >= urls_.size())
      {
        throw Exception("NewsGate::Moderation::FeedCrawler::"
                        "ValidateFeeds::get_feed: unexpected call");
      }

      return urls_[current_feed_++];
    }
    
    inline
    void
    FeedCrawler::ValidateFeeds::set_error(const char* err) throw()
    {
      try
      {
        Guard guard(lock_);

        if(error.empty())
        {
          error = err;
          status_ = VS_ERROR;
          awake();
        }
      }
      catch(...)
      {
      }
    }

    //
    // FeedCrawler::ValidateFeeds::UrlInfo struct
    //

    inline
    FeedCrawler::ValidateFeeds::UrlInfo::UrlInfo(
      size_t reference_counter_val) throw()
        : reference_counter(reference_counter_val)
    {
    }
    
    //
    // FeedCrawler::ValidateFeeds::UrlDesc struct
    //

    inline
    FeedCrawler::ValidateFeeds::UrlDesc::UrlDesc() throw(El::Exception)
        : type(UT_UNKNOWN),
          processing_type(PT_CHECK_FEEDS),
          go_futher(0),
          user_provided(false)
    {
    }
    
    inline
    bool
    FeedCrawler::ValidateFeeds::UrlDesc::require_error_reporting() const
      throw()
    {
      return type == UT_FEED || type == UT_FEED_OR_HTML;
    }    
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_MODERATOR_FEEDMANAGER_FEEDCRAWLER_HPP_
