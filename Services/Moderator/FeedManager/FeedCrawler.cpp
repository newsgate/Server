/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/FeedManager/FeedCrawler.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <Python.h>
#include <stdint.h>

#include <El/CORBA/Corba.hpp>

#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <memory>
#include <set>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/Moment.hpp>
#include <El/String/ListParser.hpp>
#include <El/Net/HTTP/Session.hpp>
#include <El/Net/HTTP/StatusCodes.hpp>
#include <El/Net/HTTP/Headers.hpp>
#include <El/String/Manip.hpp>
#include <El/MySQL/DB.hpp>
#include <El/Guid.hpp>

#include <xsd/ConfigParser.hpp>
#include <xsd/DataFeed/RSS/ParserFactory.hpp>

#include <Commons/Feed/Types.hpp>

#include "FeedCrawler.hpp"
#include "FeedManagerMain.hpp"
#include "LinkGrabber.hpp"
#include "FeedRecord.hpp"

namespace Aspect
{
  const char FEEDS_VALIDATION[] = "FeedsValidation";
}

typedef std::auto_ptr<El::Net::HTTP::Session>  HTTPSessionPtr;

namespace NewsGate
{
  namespace Moderation
  {
    const size_t MAX_URL_LEN = 255;
    const unsigned long FeedCrawler::ValidateFeeds::LOOK_AROUND_DEPTH = 2;

    const char HTML_FEED_SCRIPT[] =
      "for a in context.html_doc().find('//a'):\n\
  context.new_message(url = a.attr('href'),\n\
                      title = None,\n\
                      description = None,\n\
                      images = None,\n\
                      source = None,\n\
                      space = None,\n\
                      lang = None,\n\
                      country = None,\n\
                      keywords = None,\n\
                      feed_domain_only = True,\n\
                      unique_message_url = True,\n\
                      unique_message_doc = True,\n\
                      unique_message_title = False,\n\
                      title_required = True,\n\
                      description_required = True,\n\
                      max_image_count = 1,\n\
                      drop_url_anchor = True,\n\
                      save = True,\n\
                      encoding=\"\")\n";
    
    //
    // CompValidationRes class
    //
    bool
    CompValidationRes(const ResourceValidationResult& r1,
                      const ResourceValidationResult& r2)
    {
      if(r1.result != r2.result)
      {
        return r1.result < r2.result;
      }

      if(r1.type != r2.type)
      {
        return r1.type < r2.type;
      }

      if(r1.feed_reference_count != r2.feed_reference_count)
      {
        return r1.feed_reference_count > r2.feed_reference_count;
      }

      return strcmp(r1.url.in(), r2.url.in()) < 0;
    }

    bool
    CompValidationTaskInfo(const ValidationTaskInfo& t1,
                           const ValidationTaskInfo& t2)
    {
      return t1.started > t2.started;
    }
    
    //
    // FeedCrawler class
    //

    FeedCrawler::FeedCrawler(El::Service::Callback* callback)
      throw(InvalidArgument, Exception, El::Exception)
        : El::Service::CompoundService<>(
          callback,
          "FeedCrawler",
          Application::instance()->config().threads(),
          1024 * 1000,
          SIZE_MAX,
          El::Service::ThreadPool::TaskQueue::ES_RANDOM)
    {
      std::string skip_ext =
        Application::instance()->config().parsing().skip_extensions();

      El::String::ListParser parser(skip_ext.c_str());

      const char* ext;
      while((ext = parser.next_item()) != 0)
      {
        exclude_extensions_.insert(ext);
      }
    }

    FeedCrawler::~FeedCrawler() throw()
    {
      // Check if state is active, then deactivate and log error
    }
    
    bool
    FeedCrawler::stop() throw(Exception, El::Exception)
    {
      bool res = El::Service::CompoundService<>::stop();

      if(res)
      {
        ReadGuard guard(srv_lock_);
        
        for(ValidateFeedMap::iterator it = validate_tasks_.begin();
            it != validate_tasks_.end(); it++)
        {
          it->second->awake();
        }
      }
      
      return res;
    }

    void
    FeedCrawler::creator_id_seq_to_set(
      const ::NewsGate::Moderation::CreatorIdSeq& creator_id_seq,
      CreatorIdSet& creator_id_set)
      throw(El::Exception)
    {
      for(unsigned long i = 0; i < creator_id_seq.length(); i++)
      {
        creator_id_set.insert(creator_id_seq[i]);
      }
    }

    ::NewsGate::Moderation::ValidationTaskInfoSeq*
    FeedCrawler::task_infos(
      const ::NewsGate::Moderation::CreatorIdSeq& creator_ids)
      throw(NewsGate::Moderation::NotReady,
            NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      Moderation::ValidationTaskInfoSeq_var infos =
        new Moderation::ValidationTaskInfoSeq();

      try
      {
        CreatorIdSet creator_id_set;
        creator_id_seq_to_set(creator_ids, creator_id_set);
      
        ReadGuard guard(srv_lock_);
      
        for(ValidateFeedMap::const_iterator it = validate_tasks_.begin();
            it != validate_tasks_.end(); it++)
        {
          const ValidateFeeds_var task = it->second;

          if(creator_id_set.find(task->creator_id) != creator_id_set.end())
          {
            unsigned long len = infos->length();
            infos->length(len + 1);
          
            Moderation::ValidationTaskInfo& info = infos[len];
          
            info.id = task->id.c_str();
            info.title = task->title.c_str();
            info.creator_id = task->creator_id;
            info.started = task->started();
            info.feeds = task->feeds_count();
            info.pending_urls = task->pending_tasks();
            info.received_bytes = task->received_bytes();
            info.processed_urls = task->processed_urls();
            info.status = task->status();
          }
        }
      
        ValidationTaskInfo* res = infos->get_buffer();    
        std::sort(res, res + infos->length(), CompValidationTaskInfo);
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Moderation::FeedCrawler::task_infos: "
          "El::Exception caught. Description: " << e;
        
        NewsGate::Moderation::ImplementationException ex;
        ex.description = ostr.str().c_str();
        
        throw ex;
      }
      
      return infos._retn();
    }

    void
    FeedCrawler::stop_validation(
      const char* task_id,
      const ::NewsGate::Moderation::CreatorIdSeq& creator_ids)
      throw(NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      try
      {
        CreatorIdSet creator_id_set;
        creator_id_seq_to_set(creator_ids, creator_id_set);
        
        ValidateFeeds_var task;
        
        {
          ReadGuard guard(srv_lock_);
          ValidateFeedMap::const_iterator it = validate_tasks_.find(task_id);

          if(it != validate_tasks_.end())
          {
            task = it->second;          
          }
        }
        
        if(task.in() &&
           creator_id_set.find(task->creator_id) != creator_id_set.end())
        {
          task->interrupt();
        }
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Moderation::FeedCrawler::stop_validation: "
          "El::Exception caught. Description: " << e;
        
        NewsGate::Moderation::ImplementationException ex;
        ex.description = ostr.str().c_str();
        
        throw ex;
      }
    }
      
    void
    FeedCrawler::delete_validation(
      const char* task_id,
      const ::NewsGate::Moderation::CreatorIdSeq& creator_ids)
      throw(NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      try
      {
        CreatorIdSet creator_id_set;
        creator_id_seq_to_set(creator_ids, creator_id_set);
        
        ValidateFeeds_var task;
        
        {
          ReadGuard guard(srv_lock_);
          ValidateFeedMap::const_iterator it = validate_tasks_.find(task_id);

          if(it != validate_tasks_.end())
          {
            task = it->second;          
          }
        }
        
        if(task.in() &&
           creator_id_set.find(task->creator_id) != creator_id_set.end())
        {
          task->interrupt();

          WriteGuard guard(srv_lock_);
          validate_tasks_.erase(task_id);
        }
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Moderation::FeedCrawler::delete_validation: "
          "El::Exception caught. Description: " << e;
        
        NewsGate::Moderation::ImplementationException ex;
        ex.description = ostr.str().c_str();
        
        throw ex;
      }
    } 
    
    ::NewsGate::Moderation::ResourceValidationResultSeq*
    FeedCrawler::validation_result(
      const char* task_id,
      const ::NewsGate::Moderation::CreatorIdSeq& creator_ids,
      CORBA::Boolean all_results,
      CORBA::ULong_out total_results)
      throw(NewsGate::Moderation::TaskNotFound,
            NewsGate::Moderation::NotReady,
            NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      ResourceValidationResultSeq_var result;

      try
      {
        CreatorIdSet creator_id_set;
        creator_id_seq_to_set(creator_ids, creator_id_set);

        ValidateFeeds_var task;
        
        {
          ReadGuard guard(srv_lock_);
          ValidateFeedMap::const_iterator it = validate_tasks_.find(task_id);
          
          if(it == validate_tasks_.end())
          {
            throw Moderation::TaskNotFound();
          }
          else
          {
            task = it->second;
            
            if(creator_id_set.find(task->creator_id) == creator_id_set.end())
            {
              throw Moderation::TaskNotFound();
            }

            result = new ResourceValidationResultSeq();
          }
        }
        
        ValidationStatus status = task->status();
        
        if(status != VS_SUCCESS && status != VS_INTERRUPTED)
        {
          Moderation::NotReady ex;
          
          ex.reason = "NewsGate::Moderation::FeedCrawler::"
            "validation_result: validation task didn't successfully "
            "completed";

          throw ex;
        }

        total_results = task->result->length();

        if(all_results)
        {
          *result = *task->result;
        }
        else
        {
          const ResourceValidationResultSeq& seq = task->result;
          unsigned long count = 0;

          for(unsigned long i = 0; i < seq.length(); ++i)
          {
            const ResourceValidationResult& res = seq[i];
            
            if((res.type >= RT_RSS && res.type <= RT_HTML_FEED) ||
               (res.result == VR_VALID && res.type == RT_HTML &&
                res.feed_reference_count))
            {
              ++count;
            }
          }

          result->length(count);

          for(unsigned long i = 0, j = 0; i < seq.length(); ++i)
          {
            const ResourceValidationResult& res = seq[i];
            
            if((res.type >= RT_RSS && res.type <= RT_HTML_FEED) ||
               (res.result == VR_VALID &&
                res.type == RT_HTML && res.feed_reference_count))
            {
              (*result)[j++] = res;
            }
          }
        }
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Moderation::FeedCrawler::validation_result: "
          "El::Exception caught. Description: " << e;
        
        NewsGate::Moderation::ImplementationException ex;
        ex.description = ostr.str().c_str();
        
        throw ex;
      }

      return result._retn();
    }
    
    ::NewsGate::Moderation::ValidationToken
    FeedCrawler::validate_async(
      const ::NewsGate::Moderation::UrlSeq& urls,
      ::NewsGate::Moderation::ProcessingType processing_type,
      ::NewsGate::Moderation::CreatorId creator_id,
      ::CORBA::ULong req_period)
      throw(NewsGate::Moderation::NotReady,
            NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      try
      {
        ValidateFeeds_var validate_task =
          create_task(urls, processing_type, false, creator_id, req_period);
          
        {
          WriteGuard guard(srv_lock_);
          validate_tasks_[validate_task->id] = validate_task;
        }

        try
        {
          do_validate(validate_task.in(), true);
          return CORBA::string_dup(validate_task->id.c_str());
        }
        catch(...)
        {
          WriteGuard guard(srv_lock_);
          validate_tasks_.erase(validate_task->id);

          throw;
        }
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Moderation::FeedCrawler::validate_async: "
          "El::Exception caught. Description: " << e;
        
        NewsGate::Moderation::ImplementationException ex;
        ex.description = ostr.str().c_str();
        
        throw ex;
      }
    }
    
    ::NewsGate::Moderation::FeedCrawler::ValidateFeeds*
    FeedCrawler::create_task(
      const ::NewsGate::Moderation::UrlSeq& urls,
      ::NewsGate::Moderation::ProcessingType processing_type,
      bool urgent,
      ::NewsGate::Moderation::CreatorId creator_id,
      unsigned long req_period)
      throw(El::Exception)
    {
      ::NewsGate::Moderation::UrlSeq parsed_urls;
      StringList meta_urls;

      for(size_t i = 0; i < urls.length(); i++)
      {          
        parse_meta_url(urls[i], parsed_urls, &meta_urls);          
      }
        
      ValidateFeeds_var validate_task =
        new ValidateFeeds(parsed_urls,
                          processing_type,
                          urgent,
                          creator_id,
                          req_period,
                          this);

      if(!meta_urls.empty())
      {
        ResourceValidationResult result;
        
        result.type = RT_META_URL;
        result.result = VR_VALID;
        result.processing_type = processing_type;
        result.feed_reference_count = 0;
        result.description = "Meta-url";
        result.feed_id = 0;

        for(StringList::const_iterator it = meta_urls.begin();
            it != meta_urls.end(); it++)
        {
          result.url = it->c_str();
          validate_task->set_result(result, false);
        }
      }

      return validate_task.retn();
    }
    
    ::NewsGate::Moderation::ResourceValidationResultSeq*
    FeedCrawler::validate(
      const ::NewsGate::Moderation::UrlSeq& urls,
      ::NewsGate::Moderation::ProcessingType processing_type,
      ::NewsGate::Moderation::CreatorId creator_id)
      throw(NewsGate::Moderation::NotReady,
            NewsGate::Moderation::ImplementationException,
            CORBA::SystemException)
    {
      try
      {
        ValidateFeeds_var validate_task =
          create_task(urls, processing_type, true, creator_id, 0);
        
        {
          WriteGuard guard(srv_lock_);
          validate_tasks_[validate_task->id] = validate_task;
        }
        
        try
        {
          Moderation::ResourceValidationResultSeq_var res =
            do_validate(validate_task.in(), false);
          
          {
            WriteGuard guard(srv_lock_);
            validate_tasks_.erase(validate_task->id);
          }
          
          return res._retn();
        }
        catch(...)
        {
          WriteGuard guard(srv_lock_);
          validate_tasks_.erase(validate_task->id);

          throw;
        }
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Moderation::FeedCrawler::validate: "
          "El::Exception caught. Description: " << e;
        
        NewsGate::Moderation::ImplementationException ex;
        ex.description = ostr.str().c_str();
        
        throw ex;
      }
    }

    void
    FeedCrawler::parse_meta_url(const UrlDesc& mod_url,
                                    UrlSeq& parsed_urls,
                                    StringList* meta_urls)
      throw(El::Exception)
    {
      const char* url = mod_url.url.in();
      
      const char* token_begin = 0;
      const char* delim = 0;
      const char* token_end = 0;

      bool urls_added = false;

      if((token_begin = strstr(url, "{{")) != 0 &&
         (delim = strchr(token_begin += 2, '-')) != 0 &&
         (token_end = strstr(delim + 1, "}}")) != 0)
      {
        std::string from;
        from.assign(token_begin, delim - token_begin);
          
        std::string to;
        to.assign(delim + 1, token_end - delim - 1);

        unsigned long f = 0;
        unsigned long t = 0;
          
        if(El::String::Manip::numeric(from.c_str(), f) &&
           El::String::Manip::numeric(to.c_str(), t) && f <= t)
        {
          urls_added = true;
            
          std::string prefix;
          std::string suffix;
            
          prefix.assign(url, token_begin - url - 2);
          suffix = token_end + 2;
              
          for(; f <= t; f++)
          {
            std::ostringstream ostr;
            ostr << prefix << f << suffix;

            UrlDesc derived_url;
            derived_url.url = ostr.str().c_str();
            
            parse_meta_url(derived_url, parsed_urls, 0);
          }
        }
      }
      
      if(urls_added)
      {
        if(meta_urls)
        {
          meta_urls->push_back(url);
        }
      }
      else
      {
        unsigned long len = parsed_urls.length();
        parsed_urls.length(len + 1);
        parsed_urls[len] = mod_url;
      }
    }
    
    ::NewsGate::Moderation::ResourceValidationResultSeq*
    FeedCrawler::do_validate(ValidateFeeds* validate_task, bool async)
      throw(NewsGate::Moderation::NotReady,
            NewsGate::Moderation::ImplementationException,
            CORBA::SystemException)
    {    
      try
      {
        validate_task->run(async);

        if(async)
        {
          return 0;
        }
        
        if(!validate_task->error.empty())
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::FeedCrawler::do_validate: "
            "validation task failed. Reason: " << validate_task->error;

          throw Exception(ostr.str());
        }

        if(!started())
        {
          Moderation::NotReady ex;
          ex.reason = "NewsGate::Moderation::FeedCrawler::do_validate: "
            "system is shutting down";

          throw ex;
        }

        return validate_task->result._retn();
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Moderation::FeedCrawler::do_validate: "
          "El::Exception caught. Description: " << e;
        
        NewsGate::Moderation::ImplementationException ex;
        ex.description = ostr.str().c_str();
        
        throw ex;
      }
    }
    
    bool
    FeedCrawler::notify(El::Service::Event* event)
      throw(El::Exception)
    {
      if(El::Service::CompoundService<>::notify(event))
      {
        return true;
      }
      
      return true;
    }

    ::NewsGate::Moderation::ResourceValidationResultSeq*
    FeedCrawler::add_feeds(
      const ::NewsGate::Moderation::UrlSeq& feeds,
      const ::NewsGate::Moderation::FeedSourceSeq& feed_sources,
      ::NewsGate::Moderation::CreatorType creator_type,
      ::NewsGate::Moderation::CreatorId creator_id)
      throw(NewsGate::Moderation::NotReady,
            NewsGate::Moderation::ImplementationException,
            CORBA::SystemException)
    {
      std::string filename;

      try
      {      
        ValidateFeeds_var validate_task =
          new ValidateFeeds(feeds, PT_CHECK_FEEDS, true, creator_id, 0, this);
        
        ResourceValidationResultSeq_var result =
          do_validate(validate_task.in(), false);

        {
          ACE_Time_Value ctime = ACE_OS::gettimeofday();
        
          std::ostringstream ostr;
          ostr << Application::instance()->config().temp_dir()
               << "/FeedManager.cache."
               << El::Moment(ctime).dense_format() << "." << rand();

          filename = ostr.str();
        }

        //
        // Saving feeds
        //

        const char* creator_type_str = "";
        
        switch(creator_type)
        {
        case CT_MODERATOR: creator_type_str = "M"; break;
        case CT_USER: creator_type_str = "U"; break;
        case CT_CRAWLER: creator_type_str = "C"; break;
        default:
          {
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::FeedCrawler::add_feeds: "
            "unexpected creator type " << creator_type;
          
          throw Exception(ostr.str());
          }
        }
          
        std::fstream file(filename.c_str(), ios::out);

        if(!file.is_open())
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::FeedCrawler::add_feeds: "
            "failed to open file '" << filename << "' for write access";
        
          throw Exception(ostr.str());
        }

        bool first_line = true;
        
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();

        std::string html_feed_script;
        El::MySQL::Result_var qresult;
        
        for(size_t i = 0; i < result->length(); i++)
        {
          const ResourceValidationResult& res = result[i];

          if(res.result != VR_VALID)
          {
            continue;
          }

          Feed::Type tp = Feed::TP_UNDEFINED;

          switch(res.type)
          {
          case RT_RSS:
            {
              tp = Feed::TP_RSS;
              break;
            }
          case RT_ATOM:
            {
              tp = Feed::TP_ATOM;
              break;
            }
          case RT_RDF:
            {
              tp = Feed::TP_RDF;
              break;
            }
          case RT_HTML_FEED:
            {
              if(html_feed_script.empty())
              {
                html_feed_script =
                  connection->escape_for_load(HTML_FEED_SCRIPT);
              }
              
              tp = Feed::TP_HTML;
              break;
            }
          default:
            {
              continue;
            }
          }

          if(first_line)
          {
            first_line = false;
          }
          else
          {
            file << std::endl;
          }

          file << tp << "\t" << connection->escape_for_load(res.url.in())
               << "\t" << creator_type_str << "\t" << creator_id
               << "\t" << (res.type == RT_HTML_FEED ?
                           html_feed_script.c_str() : "");
        }

        file.close();

        if(!first_line)
        {
//          qresult = connection->query("lock tables FeedUpdateNum write, "
//                                      "Feed write, FeedBuff write");

          qresult = connection->query("begin");

          try
          {
            qresult = connection->query(
              "select update_num from FeedUpdateNum for update");
          
            FeedUpdateNum feeds_stamp(qresult.in());
            
            if(!feeds_stamp.fetch_row())
            {
              throw Exception(
                "NewsGate::Moderation::FeedCrawler::add_feeds: "
                "failed to get latest feed update number");
            }

            uint64_t update_num = (uint64_t)feeds_stamp.update_num() + 1;

            {
              std::ostringstream ostr;
              ostr << "update FeedUpdateNum set update_num=" << update_num;
              
              qresult = connection->query(ostr.str().c_str());
              qresult = connection->query("delete from FeedBuff");
            }
            
            {
              std::ostringstream ostr; 
              ostr << "LOAD DATA INFILE '"
                   << connection->escape(filename.c_str())
                   << "' INTO TABLE FeedBuff "
                "character set binary (type, url, creator_type, "
                "creator, adjustment_script)";
              
              qresult = connection->query(ostr.str().c_str());
            }

            qresult = connection->query(
              "INSERT IGNORE INTO Feed (type, url, update_num, created, "
              "creator_type, creator, status, adjustment_script) "
              "SELECT * FROM FeedBuff");
          
            {
              std::ostringstream ostr;
              ostr << "update Feed set update_num=" << update_num
                   << " where update_num=0";
              
              qresult = connection->query(ostr.str().c_str());
            }
            
            {
              std::ostringstream ostr;
              ostr << "select id, url from Feed where update_num="
                   << update_num;
              
              qresult = connection->query(ostr.str().c_str());
            }

            typedef std::map<std::string, FeedId> UrlIdMap;
            UrlIdMap url_ids;
            
            FeedIdAndUrl feed_id_url(qresult.in());
            
            while(feed_id_url.fetch_row())
            {
              url_ids[feed_id_url.url().c_str()] = feed_id_url.id();
            }
            
            qresult = connection->query("commit");    
//            qresult = connection->query("unlock tables");

            for(size_t i = 0; i < result->length(); i++)
            {
              ResourceValidationResult& res = result[i];

              UrlIdMap::const_iterator it = url_ids.find(res.url.in());
              res.feed_id = it == url_ids.end() ? 0 : it->second;
            }
          }
          catch(...)
          {
            qresult = connection->query("rollback");
//            qresult = connection->query("unlock tables");
            throw;
          }
        }
        
        //
        // Saving feed sources
        //

        file.open(filename.c_str(), ios::out);

        if(!file.is_open())
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::FeedCrawler::add_feeds: "
            "failed to open file '" << filename << "' for write access";
        
          throw Exception(ostr.str());
        }

        first_line = true;
        
        for(unsigned long i = 0; i < feed_sources.length(); i++)
        {
          const Moderation::FeedSource& fs = feed_sources[i];
        
          char type = 'S';
          
          switch(fs.type)
          {
          case FST_SINGLE_FEED_HTML: type = 'S'; break;
          case FST_MULTI_FEED_HTML: type = 'M'; break;
          case FST_META_URL: type = 'T'; break;
          default:
            {
              std::ostringstream ostr;
              ostr << "NewsGate::Moderation::FeedCrawler::add_feeds: "
                "unexpected feed source type " << fs.type;
              
              throw Exception(ostr.str());
            }
          }
          char processing_type = 'X';
          
          switch(fs.processing_type)
          {
          case PT_CHECK_FEEDS: processing_type = 'C'; break;
          case PT_PARSE_PAGE_FOR_FEEDS: processing_type = 'P'; break;
          case PT_LOOK_AROUND_PAGE_FOR_FEEDS: processing_type = 'A'; break;
          case PT_PARSE_DOMAIN_FOR_FEEDS: processing_type = 'D'; break;
          default:
            {
              std::ostringstream ostr;
              ostr << "NewsGate::Moderation::FeedCrawler::add_feeds: "
                "unexpected feed processing type " << fs.processing_type;
              
              throw Exception(ostr.str());
            }
          }

          if(first_line)
          {
            first_line = false;
          }
          else
          {
            file << std::endl;
          }
              
          file << connection->escape_for_load(fs.url.in()) << "\t"
               << type << "\t" << processing_type << "\t" << creator_type_str
               << "\t" << creator_id;
        }

        file.close();

        if(feed_sources.length())
        {
          qresult = connection->query("begin");
          
          try
          {
//            qresult = connection->query("lock tables FeedSource write, "
//                                        "FeedSourceBuff write");

            // Just for the sake of locking for protection of FeedSourceBuff
            qresult = connection->query(
              "select update_num from FeedUpdateNum for update");
          
            qresult = connection->query("delete from FeedSourceBuff");
            
            {
              std::ostringstream ostr; 
              ostr << "LOAD DATA INFILE '"
                   << connection->escape(filename.c_str())
                   << "' IGNORE INTO TABLE FeedSourceBuff "
                "character set binary (url, type, "
                "processing_type, creator_type, creator)";
              
              qresult = connection->query(ostr.str().c_str());
            }
          
            qresult = connection->query(
              "INSERT IGNORE INTO FeedSource (url, type, processing_type, "
              "created, creator_type, creator) SELECT * FROM FeedSourceBuff"
              " on duplicate key update requests=requests+1");

            qresult = connection->query("commit");    
//            qresult = connection->query("unlock tables");
          }
          catch(...)
          {
            qresult = connection->query("rollback");
//            qresult = connection->query("unlock tables");
            throw;
          }
          
//          qresult = connection->query("DROP TABLE IF EXISTS FeedSourceBuff");
        }
        
        unlink(filename.c_str());
        return result._retn();
      }
      catch(const El::Exception& e)
      {
        if(!filename.empty())
        {
          unlink(filename.c_str());
        }
        
        std::ostringstream ostr;
        ostr << "NewsGate::Moderation::FeedCrawler::add_feeds: "
          "El::Exception caught. Description: " << e;
        
        NewsGate::Moderation::ImplementationException ex;
        ex.description = ostr.str().c_str();
        
        throw ex;
      }      
    }
    
    //
    // FeedCrawler::ValidateFeeds class
    //

    FeedCrawler::ValidateFeeds::ValidateFeeds(
      const UrlSeq& urls,
      ProcessingType processing_type,
      bool urgent,
      CreatorId creator_id_val,
      unsigned long req_period,
      FeedCrawler* service)
      throw(El::Exception)
        : creator_id(creator_id_val),
          result(new ::NewsGate::Moderation::ResourceValidationResultSeq()),
          completed_(lock_),
          status_(VS_ACTIVE),
          processing_type_(processing_type),
          urgent_(urgent),
          req_period_(req_period),
          current_feed_(0),
          pending_tasks_(0),
          feed_crawler_(service),
          received_bytes_(0),
          started_(ACE_OS::gettimeofday().sec()),
          feeds_count_(0),
          next_request_time_(ACE_OS::gettimeofday())
    {
      El::Guid guid;
      guid.generate();
        
      id = guid.string();

      std::stringstream ostr;

      switch(processing_type)
      {
      case PT_CHECK_FEEDS:
        {
          ostr << "Check feeds (rp " << req_period << " ms) for:";
          break;
        }
      case PT_PARSE_PAGE_FOR_FEEDS:
        {
          ostr << "Parse WEB pages, grab & check feeds (rp " << req_period
               << " ms) for:";
          break;
        }
      case PT_LOOK_AROUND_PAGE_FOR_FEEDS:
        {
          ostr << "Look around WEB pages, grab & check feeds (rp "
               << req_period << " ms) for:";
          break;
        }
      case PT_PARSE_DOMAIN_FOR_FEEDS:
        {
          ostr << "Traverse company domain, grab & check feeds (rp "
               << req_period << " ms) for:";
          break;
        }
      default:
        {
          ostr << "FeedCrawler::ValidateFeeds::ValidateFeeds: unexpected "
            "processing type " << processing_type;
          throw Exception(ostr.str());
        }
      }
      
      
      for(size_t i = 0; i < urls.length(); i++)
      {
        const Moderation::UrlDesc& udesc = urls[i];
        
        add_feed(udesc.url.in(),
                 processing_type == PT_CHECK_FEEDS ?
                 UT_FEED : UT_FEED_OR_HTML,
                 true,
                 false,
                 0,
                 processing_type == PT_LOOK_AROUND_PAGE_FOR_FEEDS ?
                 LOOK_AROUND_DEPTH : SIZE_MAX);
        
        if(i < 5)
        {
          ostr << " \n" << udesc.url.in();
        }
      }

      if(urls.length() > 5)
      {
        ostr << " \n...";
      }

      title = ostr.str();
    }

    void
    FeedCrawler::ValidateFeeds::run(bool async) throw(El::Exception)
    {
      {
        Guard guard(lock_);

        if(urls_.size() == 0)
        {
          status_ = VS_SUCCESS;
          return;
        }
        
        for(size_t i = 0; i < urls_.size(); i++)
        {
          self_schedule();
        }
      }

      if(async)
      {
        return;
      }
      
      while(true)
      {
        if(!feed_crawler_->started())
        {
          return;
        }
        
        Guard guard(lock_);
        
        if(status_ != VS_ACTIVE)
        {
          return;
        }
        
        if(completed_.wait(0))
        {
          int error = ACE_OS::last_error();
          
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::FeedCrawler::ValidateFeeds::"
            "run: completed.wait() failed. Errno " << error
               << ". Description:" << std::endl << ACE_OS::strerror(error);
          
          throw Exception(ostr.str());
        }
      }

    }

    void
    FeedCrawler::ValidateFeeds::interrupt() throw(El::Exception)
    {
      Guard guard(lock_);

      if(status_ == VS_ACTIVE)
      {
        status_ = VS_INTERRUPTED;
        finalize_result();
        awake();
      }
    }
      
    void
    FeedCrawler::ValidateFeeds::set_result(
      const ResourceValidationResult& res,
      bool task_result) throw(Exception, El::Exception)
    {
      Guard guard(lock_);

      if(status_ != VS_ACTIVE)
      {
        return;
      }
      
      size_t len = result->length();
      result->length(len + 1);
      
      result[len] = res;

      if(res.result == VR_VALID)
      {
        switch(res.type)
        {
        case RT_RSS:
        case RT_ATOM:
        case RT_RDF:
        case RT_HTML_FEED: feeds_count_++; break;
        default: break;
        }
      }

      if(task_result)
      {
        pending_tasks_--;

        if(pending_tasks_ == 0)
        {
          status_ = VS_SUCCESS;
          finalize_result();
        }
        
        awake();
      }
    }

    void
    FeedCrawler::ValidateFeeds::finalize_result()
      throw(Exception, El::Exception)
    {
      for(size_t i = 0; i < result->length(); i++)
      {
        ResourceValidationResult& res = result[i];
        
        UrlMap::iterator it = url_map_.find(res.url.in());
        
        res.feed_reference_count = it == url_map_.end() ?
          0 : it->second.reference_counter;

        if(res.result == VR_VALID &&
           ((res.type >= RT_RSS && res.type <= RT_HTML_FEED) ||
            (res.type == RT_HTML && res.feed_reference_count)))
        {
          try
          {
            std::wstring feed_wurl;
            El::String::Manip::utf8_to_wchar(res.url.in(), feed_wurl);

            if(feed_wurl.length() > MAX_URL_LEN)
            {
              res.result = VR_TOO_LONG_URL;
            
              std::ostringstream ostr;
              ostr << "max " << MAX_URL_LEN << " chars allowed for feed url";
              res.description = ostr.str().c_str();
            }            
          }
          catch(const El::Exception& e)
          {
            res.result = VR_UNRECOGNIZED_RESOURCE_TYPE;
            res.description = e.what();
          }
        }
        
        if(res.result == VR_VALID && res.feed_reference_count)
        {
          res.description = "Feeds source";  
        }        
      }
      
      ResourceValidationResult* res = result->get_buffer();    
      std::sort(res, res + result->length(), CompValidationRes);
    }
    
    void
    FeedCrawler::ValidateFeeds::add_feed_i(const char* url,
                                               UrlType type,
                                               bool user_provided,
                                               bool add_task,
                                               const char* referencer,
                                               size_t go_futher)
      throw(Exception, El::Exception)
    {
      if(!feed_crawler_->started() || status_ != VS_ACTIVE)
      {
        return;
      }
      
      try
      {
        El::Net::HTTP::URL_var http_url =
          new El::Net::HTTP::URL(url);

        UrlDesc url_desc;
        url_desc.url = http_url->string();
        url_desc.referencer = referencer ? referencer : "";
        url_desc.user_provided = user_provided;
        
        url_desc.processing_type = user_provided ?
          processing_type_ : PT_PARSE_PAGE_FOR_FEEDS; //PT_CHECK_FEEDS;        

        if(url_map_.find(url_desc.url) == url_map_.end())
        {
          url_desc.type = type;
          url_desc.go_futher = go_futher;
            
          urls_.push_back(url_desc);
          url_map_[url_desc.url] = UrlInfo();
/*
          std::cerr << "ValidateFeeds::add_feed: " << url_desc.url
                    << std::endl;
*/
          if(add_task)
          {
            self_schedule();            
          }

          add_urls_in_url(http_url.in(),
                          type,
                          user_provided,
                          add_task,
                          referencer,
                          go_futher);
        }
      }
      catch(const El::Net::HTTP::URL::InvalidArg& e)
      {
        if(type == UT_FEED || type == UT_FEED_OR_HTML)
        {
          unsigned long len = result->length();
          result->length(len + 1);
          
          ResourceValidationResult& res = result[len];
          
          res.url = url;
          res.type = RT_UNKNOWN;
          res.result = VR_UNRECOGNIZED_RESOURCE_TYPE;
          
          std::ostringstream ostr;
          ostr << "Parsing URL error: " << e;
          res.description = ostr.str().c_str();
        }
      }
    }

    void
    FeedCrawler::ValidateFeeds::self_schedule() throw(El::Exception)
    {
      pending_tasks_++;

      El::Service::CompoundServiceMessage_var msg =
        new ValidateFeedsMsg(this, feed_crawler_);

      if(urgent_)
      {
        feed_crawler_->deliver_now(
          msg.in(),
          0,
          El::Service::ThreadPool::TaskQueue::ES_FRONT);
      }
      else
      {
        feed_crawler_->deliver_at_time(msg.in(), next_request_time_);
      }
      
      unsigned long url_fetch_period =
        std::max(
          (unsigned long)Application::instance()->config().url_fetch_period(),
          req_period_);

      ACE_Time_Value inc(url_fetch_period / 1000,
                         (url_fetch_period % 1000) * 1000);
      
      next_request_time_ += inc;
    }
    
    void
    FeedCrawler::ValidateFeeds::add_urls_in_url(El::Net::HTTP::URL* url,
                                                    UrlType type,
                                                    bool user_provided,
                                                    bool add_task,
                                                    const char* referencer,
                                                    size_t go_futher)
      throw(Exception, El::Exception)
    {
      El::String::ListParser parser(url->params(), "&");

      const char* item = 0;
      while((item = parser.next_item()) != 0)
      {
        const char* val = strchr(item, '=');

        if(val++)
        {
          std::string embed_url;
          
          if(strncmp(val, "http://", 7) == 0 ||
             strncmp(val, "feed://", 7) == 0)
          {
            embed_url = val;
          }
          else if(strncasecmp(val, "http%3a", 7) == 0 ||
                  strncasecmp(val, "feed%3a", 7) == 0)
          {
            try
            {
              El::String::Manip::mime_url_decode(val, embed_url);
            }
            catch(...)
            {
              continue;
            }
          }

          if(!embed_url.empty())
          {
            add_feed_i(embed_url.c_str(),
                       type,
                       user_provided,
                       add_task,
                       referencer,
                       go_futher);
          }
        }
        
      }
       
    }
    
    void
    FeedCrawler::ValidateFeeds::execute() throw(El::Exception)
    {
      std::string processed_url;
        
      try
      {
        if(!feed_crawler_->started() || status() != VS_ACTIVE)
        {
          return;
        }

        ResourceValidationResult result;

        ValidateFeeds::UrlDesc url_desc = get_feed();
          
        result.url = url_desc.url.c_str();
        result.type = RT_UNKNOWN;
        result.result = VR_UNRECOGNIZED_RESOURCE_TYPE;
        result.processing_type = url_desc.processing_type;
        result.feed_id = 0;

        processed_url = result.url;

        validate(url_desc, result);

        if(!url_desc.referencer.empty() && result.result == VR_VALID &&
           result.type != RT_HTML && result.type != RT_UNKNOWN)
        {
          Guard guard(lock_);
            
          UrlMap::iterator it = url_map_.find(url_desc.referencer);
          
          if(it != url_map_.end())
          {
            it->second.reference_counter++;
          }
          
        }        
       
        set_result(result, true);
/*
        Guard guard(lock_);
        std::cerr << "ValidateFeeds::execute: processed " << result.url.in()
                  << ", so far " << received_bytes_ << " bytes, pending urls "
                  << pending_tasks_ << std::endl;
*/
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "FeedCrawler::ValidateFeeds::execute: for url "
             << processed_url << " El::Exception caught. Description:\n" << e;

        std::string error = ostr.str();
          
        Application::logger()->emergency(error.c_str(),
                                         Aspect::FEEDS_VALIDATION);
        
        set_error(error.c_str());
      }
    }

    void
    FeedCrawler::ValidateFeeds::received_bytes(unsigned long long bytes)
      throw()
    {
      Guard guard(lock_);

      if(status_ == VS_ACTIVE)
      {
        received_bytes_ += bytes;
      }
    }
    
    void
    FeedCrawler::ValidateFeeds::validate(
      const ValidateFeeds::UrlDesc& url_desc,
      ResourceValidationResult& result)
      throw(El::Exception)
    {
      El::MySQL::Connection_var connection;

      try
      {
        std::ostringstream query_ostr;

        connection = Application::instance()->dbase()->connect();
        
        query_ostr << "select type from Feed where url = '" << 
          connection->escape(url_desc.url.c_str()) << "'";

        El::MySQL::Result_var qresult;
//          connection->query("lock tables Feed read");        
        
        qresult = connection->query(query_ostr.str().c_str());

        FeedTypeRecord record(qresult.in());

        if(record.fetch_row())
        {
          unsigned long type = record.type();

          switch(type)
          {
          case ::NewsGate::Feed::TP_RSS:
            {
              result.type = RT_RSS;
              break;
            }
          case ::NewsGate::Feed::TP_ATOM:
            {
              result.type = RT_ATOM;
              break;
            }
          case ::NewsGate::Feed::TP_RDF:
            {
              result.type = RT_RDF;
              break;
            }
          case ::NewsGate::Feed::TP_HTML:
            {
              result.type = RT_HTML_FEED;
              break;
            }
          default: break;
          }
          
//          qresult = connection->query("unlock tables");
          connection = 0;

          result.result = VR_ALREADY_IN_THE_SYSTEM;
          
          {
            Guard guard(lock_);

            UrlMap::iterator it = url_map_.find(url_desc.referencer);

            if(it != url_map_.end())
            {
              it->second.reference_counter++;
            }
          }
          
          return;
        }

//        qresult = connection->query("unlock tables");
//        connection = 0;
      }
      catch(...)
      {
/*        
        if(connection.in() != 0)
        {
          El::MySQL::Result_var qresult =
            connection->query("unlock tables");
        }
*/

        throw;
      }
      
      try
      {
        std::string company_domain;
        std::string new_permanent_location;
        bool is_html = false;
        std::string charset;
        std::string error;

        El::Net::HTTP::URL_var url =
          new El::Net::HTTP::URL(url_desc.url.c_str());
        
        HTTPSessionPtr session(
          start_session(url.in(),
                        company_domain,
                        new_permanent_location,
                        is_html,
                        charset,
                        error));
        
        if(session.get() == 0)
        {
          result.description = error.c_str();
          return;
        }

        received_bytes(session->received_bytes(true));

        bool redirect = *url != *(session->url());
        bool permanent_redirect = redirect && !new_permanent_location.empty();

        bool relocation_to_different_company = false;
        
        if(redirect && !permanent_redirect)
        {
          if(strncmp(session->url()->string(), "http://", 7))
          {
            std::ostringstream ostr;
            ostr << "can't perform temporary redirection to "
                 << session->url()->string()
                 << ", http:// protocol expected";

            result.description = ostr.str().c_str();
            return;
          }

          std::string new_location_company_domain;
          
          try
          {
            if(El::Net::ip(session->url()->host()))
            {
              new_location_company_domain = session->url()->host();
            }
            else if(!El::Net::company_domain(session->url()->host(),
                                             &new_location_company_domain))
            {
              std::ostringstream ostr;
              ostr << "can't perform temporary redirection to "
                   << session->url()->host()
                   << "unexpected hostname " << session->url()->host();

              result.description = ostr.str().c_str();
              
              return;
            }
          }
          catch(...)
          {
          }          
          
          if(strcasecmp(new_location_company_domain.c_str(),
                        company_domain.c_str()))
          {
            relocation_to_different_company = true;
          }
          else
          {
            Guard guard(lock_);
              
            {
              TempRedirectMap::iterator it =
                tmp_redirect_map_.find(session->url()->string());
          
              if(it != tmp_redirect_map_.end())
              {
                std::ostringstream ostr;
                ostr << "temporary redirection to "
                     << session->url()->string()
                     << " have already been performed from " << it->second;
                
                result.description = ostr.str().c_str();
                return;
              }
            }
              
            {
              UrlMap::iterator it = url_map_.find(session->url()->string());
          
              if(it != url_map_.end())
              {
                std::ostringstream ostr;
                ostr << "request to " << session->url()->string()
                     << " have already been made";
                  
                result.description = ostr.str().c_str();
                return;
              }
            }
              
            tmp_redirect_map_[session->url()->string()] = url->string();
            
            std::ostringstream ostr;
            ostr << "temporarily redirected to " << session->url()->string();
            result.description = ostr.str().c_str();
          }
        }
        
        if(permanent_redirect || relocation_to_different_company)
        {
          std::string new_location;
          
          if(relocation_to_different_company)
          {
            new_location = session->url()->string();
            
            std::ostringstream ostr;
            ostr << "temporary redirected to other domain "
                 << new_location;
            
            result.description = ostr.str().c_str();
          }
          else
          {
            new_location = new_permanent_location;
            
            std::ostringstream ostr;
            ostr << "permanently redirected to " << new_location;
            result.description = ostr.str().c_str();
          }
            
          if(strncmp(new_location.c_str(), "http://", 7) == 0)
          {
            add_url_for_processing(new_location.c_str(),
                                   url_desc.type,
                                   url_desc.user_provided,
                                   company_domain.c_str(),
                                   url_desc.referencer.c_str(),
                                   url_desc.go_futher);
          }

          return;
        }

        std::string body;
        std::istream& body_str = session->response_body();

        while(!body_str.fail())
        {
          char buff[1024];

          body_str.read(buff, sizeof(buff));
          body.append(buff, body_str.gcount());
        }
            
        received_bytes(session->received_bytes(true));
        session->test_completion();
        session->close();
          
        std::string feed_url = session->url()->string();
        std::auto_ptr<std::ostringstream> ostr(new std::ostringstream());

        if(valid_rdf(body, feed_url.c_str(), charset.c_str(), *ostr))
        {
          result.type = RT_RDF;
          result.result = VR_VALID;
          return;
        }
            
        if(valid_atom(body, feed_url.c_str(), charset.c_str(), *ostr))
        {
          result.type = RT_ATOM;
          result.result = VR_VALID;
          return;
        }
            
        if(valid_rss(body, feed_url.c_str(), charset.c_str(), *ostr))
        {
          result.type = RT_RSS;
          result.result = VR_VALID;
          return;
        }
            
        if(is_html)
        {
          if(url_desc.processing_type == PT_CHECK_FEEDS)
          {
            result.type = RT_HTML_FEED;
            result.result = VR_VALID;
            return;
          }
          
          ostr.reset(new std::ostringstream());
          *ostr << "non XML content type";
        }
           
        if(url_desc.type == UT_FEED)
        {
          result.description = ostr->str().c_str();
          return;
        }

        if(url_desc.type == UT_PROBABLY_FEED)
        {
          return;
        }
            
        if(url_desc.type == UT_FEED_OR_HTML)
        {
          if(is_html)
          {
            LinkGrabber parser;
            parser.parse(body.c_str(),
                         url_desc.url.c_str(),
                         charset.c_str());

            typedef LinkGrabber::StringSet StringSet;

            if(url_desc.go_futher > 0)
            {
              for(StringSet::const_iterator
                    it = parser.normalized_refs.begin();
                  it != parser.normalized_refs.end(); it++)
              {
//                std::cerr << "Predecode " << *it << std::endl;
                
                std::string decoded = html_decode_url(it->c_str());

/*                
                std::cerr << "Preadding " << decoded << std::endl;

                if(decoded == "http://vk.com/away.php?to=http%3A%2F%2F%E1%EE%EB%FC%F8%EE%E5%EF%F0%E0%E2%E8%F2%E5%EB%FC%F1%F2%E2%EE.%F0%F4%2Fevents%2F833%2F&h=99301a903a94750874&post=53083705_50355")
                {
                  std::cerr << "Found\n";
                }
*/
                  
                add_url_for_processing(
                  decoded.c_str(),
                  UT_UNKNOWN,
                  false,
                  company_domain.c_str(),
                  url_desc.url.c_str(),
                  url_desc.go_futher - 1);

//                std::cerr << "Postadding " << decoded << std::endl;
              }
            }
                
            for(StringSet::const_iterator
                  it = parser.normalized_frames.begin();
                it != parser.normalized_frames.end(); it++)
            {
              add_url_for_processing(html_decode_url(it->c_str()).c_str(),
                                     UT_FEED_OR_HTML,
                                     false,
                                     company_domain.c_str(),
                                     url_desc.url.c_str(),
                                     url_desc.go_futher);
            }
                
            result.type = RT_HTML;
            result.result = VR_VALID;
            return;
          }
        }
      }
      catch(const LinkGrabber::Exception& e)
      {
        result.result = VR_NO_VALID_RESPONSE;
        
        std::ostringstream ostr;
        ostr << "HTML page parsing error: " << e;
        result.description = ostr.str().c_str();
      }
      catch(const El::Net::Exception& e)
      {
        result.result = VR_NO_VALID_RESPONSE;
        
        std::ostringstream ostr;
        ostr << "HTTP request error: " << e;
        result.description = ostr.str().c_str();
      }
    }

    El::Net::HTTP::Session*
    FeedCrawler::ValidateFeeds::start_session(
      El::Net::HTTP::URL* url,
      std::string& company_domain,
      std::string& new_permanent_location,
      bool& is_html,
      std::string& charset,
      std::string& error)
      throw(El::Net::Exception, El::Exception)
    {
      const Server::Config::FeedManagerType& config =
        Application::instance()->config();

      company_domain.clear();
      new_permanent_location.clear();
      is_html = false;
      charset.clear();
      error.clear();    

      if(El::Net::ip(url->host()))
      {
        company_domain = url->host();
      }
      else if(!El::Net::company_domain(url->host(), &company_domain))
      {
        std::ostringstream ostr;
        ostr << "unexpected hostname " << url->host();
        error = ostr.str();
        return 0;
      }
        
      HTTPSessionPtr session(new El::Net::HTTP::Session(url));
      ACE_Time_Value timeout(config.request_timeout());
        
      session->open(&timeout, &timeout, &timeout);

      El::Net::HTTP::ParamList params;
      El::Net::HTTP::HeaderList headers;

      headers.add(El::Net::HTTP::HD_ACCEPT,
                  "text/xml,application/xml,application/atom+xml,"
                  "application/rss+xml,application/rdf+xml,"
                  "application/xhtml+xml,text/html,*/*;q=0.1");

      headers.add(El::Net::HTTP::HD_ACCEPT_LANGUAGE, "en-us,*");
      headers.add(El::Net::HTTP::HD_ACCEPT_ENCODING, "gzip, deflate");
      headers.add(El::Net::HTTP::HD_USER_AGENT, config.user_agent().c_str());

      new_permanent_location =
        session->send_request(El::Net::HTTP::GET,
                              params,
                              headers,
                              0,
                              0,
                              config.redirects_to_follow());

      if(session->status_code() != El::Net::HTTP::SC_OK)
      {
        received_bytes(session->received_bytes(true));
        
        std::ostringstream ostr;
        ostr << "status " << session->status_code() << ", "
             << session->status_text();
        
        throw El::Net::Exception(ostr.str());
      }

      El::Net::HTTP::Header header;
      
      while(session->recv_response_header(header))
      {
        const char* name = header.name.c_str();
        const char* value = header.value.c_str();
          
        if(strcasecmp(name, El::Net::HTTP::HD_CONTENT_TYPE) == 0)
        {
          if(strncmp(value, "text/html", 9) == 0 ||
             strncmp(value, "application/xhtml", 17) == 0)
          {
            is_html = true;
          }
          else if(strstr(value, "xml") == 0 &&
                  strstr(value, "text/plain") == 0)
          {
            received_bytes(session->received_bytes(true));
                  
            std::ostringstream ostr;
            ostr << "unexpected " << El::Net::HTTP::HD_CONTENT_TYPE
                 << " value " << value;
                  
            error = ostr.str();
            return 0;
          }

          El::Net::HTTP::content_type(value, charset);
        }
      }
    
      return session.release();
    }

    void
    FeedCrawler::ValidateFeeds::add_url_for_processing(
      const char* s_url,
      UrlType type,
      bool user_provided,
      const char* company_domain,
      const char* referencer,
      size_t go_futher)
      throw(El::Net::Exception, El::Exception)
    {
      const FeedCrawler::StringSet& exclude_extensions =
        feed_crawler_->exclude_extensions();
    
      try
      {
        El::Net::HTTP::URL_var url = new El::Net::HTTP::URL(s_url);

//        std::cerr << "ADD: " << url->string() << std::endl;

        if(type == UT_UNKNOWN)
        {
          std::string url_company_domain;
          if(El::Net::ip(url->host()))
          {
            url_company_domain = url->host();
          }
          else if(!El::Net::company_domain(url->host(), &url_company_domain))
          {
            return;
          }

          const char* ext = strchr(url->path(), '.');
          
          if(ext && exclude_extensions.find(ext + 1) !=
             exclude_extensions.end())
          {
            return;
          }

          type = processing_type_ == PT_CHECK_FEEDS ? UT_FEED :
            ((processing_type_ == PT_PARSE_DOMAIN_FOR_FEEDS &&
              url_company_domain == company_domain) ||
             (processing_type_ == PT_LOOK_AROUND_PAGE_FOR_FEEDS &&
              go_futher) ?
             UT_FEED_OR_HTML : UT_PROBABLY_FEED);
        }
        
        add_feed(url->string(),
                 type,
                 user_provided,
                 true,
                 referencer,
                 go_futher);
      }
      catch(const ValidateFeeds::Exception& )
      {
      }
      catch(const El::Net::URL::Exception& )
      {
      }
    }
    
    std::string
    FeedCrawler::ValidateFeeds::html_decode_url(const char* url)
      throw(El::Exception)
    {
      //
      // Encoding is not known here, so all we can do to convert &amp;
      // to &. To perform actual XML decoding need to pull out document
      // encoding information from HTML.
      //

      std::ostringstream result;
      const char* ptr = url;
      
      while(true)
      {
        const char* next = strstr(ptr, "&amp;");

        if(next == 0)
        {
          break;
        }
        
        std::string str;
        str.assign(ptr, next - ptr);
        result << str << "&";
        ptr = next + 5;
      }

      result << ptr;
      return result.str();
    }

    bool
    FeedCrawler::ValidateFeeds::valid_rss(const std::string& feed,
                                          const char* feed_url,
                                          const char* charset,
                                          std::ostream& error)
      throw(El::Exception)
    {
      ACE_Time_Value timeout(
        Application::instance()->config().request_timeout());
      
      if(charset && *charset)
      {
        std::istringstream istr(feed);
//        RSS::RSSOldParser parser;
        RSS::RSSParser parser;

        try
        {
          std::auto_ptr<El::XML::EntityResolver> entity_resolver;
          
          const Server::Config::FeedManagerParsingType& cfg =
            Application::instance()->config().parsing();

          if(cfg.entity_resolver().present())
          {
            entity_resolver.reset(
              NewsGate::Config::create_entity_resolver(
                *cfg.entity_resolver()));
          }
          
          parser.parse(istr,
                       feed_url,
                       charset,
                       1024 * 10,
                       false,
                       entity_resolver.get());
          
          return true;
        }
        catch(const xsd::cxx::exception& e)
        {
          error << "RSS validator error: " << e << std::endl;
          return false;
        }
        catch(const RSS::Parser::EncodingError& e)
        {
          // Will try without HTTP-level encoding specification
        }
        catch(const RSS::Parser::Exception& e)
        {
          error << "RSS validator error: " << e << std::endl;
          return false;
        }
      }
      
      std::istringstream istr(feed);
//      RSS::RSSOldParser parser;
      RSS::RSSParser parser;

      try
      {
        std::auto_ptr<El::XML::EntityResolver> entity_resolver;
          
        const Server::Config::FeedManagerParsingType& cfg =
          Application::instance()->config().parsing();
        
        if(cfg.entity_resolver().present())
        {
          entity_resolver.reset(
            NewsGate::Config::create_entity_resolver(*cfg.entity_resolver()));
        }
        
        parser.parse(istr,
                     feed_url,
                     0,
                     1024 * 10,
                     false,
                     entity_resolver.get());
        
        return true;
      }
      catch(const xsd::cxx::exception& e)
      {
        error << "RSS validator error: " << e << std::endl;
      }
      catch(const RSS::Parser::EncodingError& e)
      {
        error << "RSS encoding error: " << e << std::endl;
      }
      catch(const RSS::Parser::Exception& e)
      {
        error << "RSS validator error: " << e << std::endl;
      }
      
      return false;
    }
    
    bool
    FeedCrawler::ValidateFeeds::valid_atom(const std::string& feed,
                                               const char* feed_url,
                                               const char* charset,
                                               std::ostream& error)
      throw(El::Exception)
    {
      ACE_Time_Value timeout(
        Application::instance()->config().request_timeout());
      
      if(charset && *charset)
      {
        std::istringstream istr(feed);
        RSS::AtomParser parser;
        
        try
        {
          std::auto_ptr<El::XML::EntityResolver> entity_resolver;
          
          const Server::Config::FeedManagerParsingType& cfg =
            Application::instance()->config().parsing();
        
          if(cfg.entity_resolver().present())
          {
            entity_resolver.reset(
              NewsGate::Config::create_entity_resolver(
                *cfg.entity_resolver()));
          }
        
          parser.parse(istr,
                       feed_url,
                       charset,
                       1024 * 10,
                       false,
                       entity_resolver.get());
          
          return true;
        }
        catch(const RSS::Parser::EncodingError& e)
        {
          // Will try without HTTP-level encoding specification
        }
        catch(const RSS::Parser::Exception& e)
        {
          error << "Atom validator error: " << e << std::endl;
          return false;
        }
      }
      
      std::istringstream istr(feed);
      RSS::AtomParser parser;

      try
      {
        std::auto_ptr<El::XML::EntityResolver> entity_resolver;
          
        const Server::Config::FeedManagerParsingType& cfg =
          Application::instance()->config().parsing();
        
        if(cfg.entity_resolver().present())
        {
          entity_resolver.reset(
            NewsGate::Config::create_entity_resolver(*cfg.entity_resolver()));
        }
        
        parser.parse(istr,
                     feed_url,
                     0,
                     1024 * 10,
                     false,
                     entity_resolver.get());
        
        return true;
      }
      catch(const RSS::Parser::EncodingError& e)
      {
        error << "Atom encoding error: " << e << std::endl;
      }
      catch(const RSS::Parser::Exception& e)
      {
        error << "Atom validator error: " << e << std::endl;
      }

      return false;
    }
    
    bool
    FeedCrawler::ValidateFeeds::valid_rdf(const std::string& feed,
                                          const char* feed_url,
                                          const char* charset,
                                          std::ostream& error)
      throw(El::Exception)
    {
      ACE_Time_Value timeout(
        Application::instance()->config().request_timeout());
      
      if(charset && *charset)
      {
        std::istringstream istr(feed);
        RSS::RDFParser parser;

        try
        {
          std::auto_ptr<El::XML::EntityResolver> entity_resolver;
          
          const Server::Config::FeedManagerParsingType& cfg =
            Application::instance()->config().parsing();
        
          if(cfg.entity_resolver().present())
          {
            entity_resolver.reset(
              NewsGate::Config::create_entity_resolver(
                *cfg.entity_resolver()));
          }
        
          parser.parse(istr,
                       feed_url,
                       charset,
                       1024 * 10,
                       false,
                       entity_resolver.get());
          
          return true;
        }
        catch(const RSS::Parser::EncodingError& e)
        {
          // Will try without HTTP-level encoding specification
        }
        catch(const RSS::Parser::Exception& e)
        {
          error << "RDF validator error: " << e << std::endl;
          return false;
        }
      }
      
      std::istringstream istr(feed);
      RSS::RDFParser parser;

      try
      {
        std::auto_ptr<El::XML::EntityResolver> entity_resolver;
          
        const Server::Config::FeedManagerParsingType& cfg =
          Application::instance()->config().parsing();
        
        if(cfg.entity_resolver().present())
        {
          entity_resolver.reset(
            NewsGate::Config::create_entity_resolver(*cfg.entity_resolver()));
        }
        
        parser.parse(istr,
                     feed_url,
                     0,
                     1024 * 10,
                     false,
                     entity_resolver.get());
        
        return true;
      }
      catch(const RSS::Parser::EncodingError& e)
      {
        error << "RDF encoding error: " << e << std::endl;
      }
      catch(const RSS::Parser::Exception& e)
      {
        error << "RDF validator error: " << e << std::endl;
      }
      
      return false;
    }

    //
    // FeedCrawler::ValidateFeedsMsg struct
    //
    FeedCrawler::ValidateFeedsMsg::ValidateFeedsMsg(
      ValidateFeeds* validate_feeds,
      FeedCrawler* service) throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(service, service, false),
          El::Service::CompoundServiceMessage(service, service),
          validate_feeds_task(El::RefCount::add_ref(validate_feeds))
    {
    }    

    void
    FeedCrawler::ValidateFeedsMsg::execute() throw(El::Exception)
    {
      validate_feeds_task->execute();
    }
  }  
}
