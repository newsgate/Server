/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/FeedManager/FeedManagerImpl.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <Python.h>

#include <El/CORBA/Corba.hpp>

#include <El/Exception.hpp>

#include "FeedManagerImpl.hpp"
#include "FeedManagerMain.hpp"
#include "FeedRecord.hpp"

namespace NewsGate
{
  namespace Moderation
  {
    //
    // FeedManagerImpl class
    //
    FeedManagerImpl::FeedManagerImpl(El::Service::Callback* callback)
      throw(Exception, El::Exception)
        : El::Service::CompoundService<FeedCrawler>(callback,
                                                    "FeedManagerImpl")
    {
      FeedCrawler_var fc = new FeedCrawler(this);
      state(fc.in());
    }

    FeedManagerImpl::~FeedManagerImpl() throw()
    {
      // Check if state is active, then deactivate and log error
    }

    void
    FeedManagerImpl::wait() throw(Exception, El::Exception)
    {
      El::Service::CompoundService<FeedCrawler>::wait();
      
      message_adjustment_.stop();
    }
    
    ::NewsGate::Moderation::ValidationTaskInfoSeq*
    FeedManagerImpl::task_infos(
      const ::NewsGate::Moderation::CreatorIdSeq& creator_ids)
      throw(NewsGate::Moderation::NotReady,
            NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      FeedCrawler_var st = state();
      return st->task_infos(creator_ids);
    }

    void
    FeedManagerImpl::stop_validation(
      const char* task_id,
      const ::NewsGate::Moderation::CreatorIdSeq& creator_ids)
      throw(NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      FeedCrawler_var st = state();
      st->stop_validation(task_id, creator_ids);
    }
      
    void
    FeedManagerImpl::delete_validation(
      const char* task_id,
      const ::NewsGate::Moderation::CreatorIdSeq& creator_ids)
      throw(NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      FeedCrawler_var st = state();
      st->delete_validation(task_id, creator_ids);
    } 
    
    ::NewsGate::Moderation::ResourceValidationResultSeq*
    FeedManagerImpl::validation_result(
      const char* task_id,
      const ::NewsGate::Moderation::CreatorIdSeq& creator_ids,
      CORBA::Boolean all_results,
      CORBA::ULong_out total_results)
      throw(NewsGate::Moderation::TaskNotFound,
            NewsGate::Moderation::NotReady,
            NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      FeedCrawler_var st = state();
      return st->validation_result(task_id,
                                   creator_ids,
                                   all_results,
                                   total_results);
    }
    
    ::NewsGate::Moderation::ValidationToken
    FeedManagerImpl::validate_async(
      const ::NewsGate::Moderation::UrlSeq& urls,
      ::NewsGate::Moderation::ProcessingType processing_type,
      ::NewsGate::Moderation::CreatorId creator_id,
      ::CORBA::ULong req_period)
      throw(NewsGate::Moderation::NotReady,
            NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      FeedCrawler_var st = state();
      return st->validate_async(urls,
                                processing_type,
                                creator_id,
                                req_period);
    }    
    
    ::NewsGate::Moderation::ResourceValidationResultSeq*
    FeedManagerImpl::validate(
      const ::NewsGate::Moderation::UrlSeq& urls,
      ::NewsGate::Moderation::ProcessingType processing_type,
      ::NewsGate::Moderation::CreatorId creator_id)
      throw(NewsGate::Moderation::NotReady,
            NewsGate::Moderation::ImplementationException,
            CORBA::SystemException)
    {
      FeedCrawler_var st = state();
      return st->validate(urls, processing_type, creator_id);
    }

    ::NewsGate::Moderation::ResourceValidationResultSeq*
    FeedManagerImpl::add_feeds(
      const ::NewsGate::Moderation::UrlSeq& feeds,
      const ::NewsGate::Moderation::FeedSourceSeq& feed_sources,
      ::NewsGate::Moderation::CreatorType creator_type,
      ::NewsGate::Moderation::CreatorId creator_id)
      throw(NewsGate::Moderation::NotReady,
            NewsGate::Moderation::ImplementationException,
            CORBA::SystemException)
    {
      FeedCrawler_var st = state();
      return st->add_feeds(feeds, feed_sources, creator_type, creator_id);
    }
    
    bool
    FeedManagerImpl::notify(El::Service::Event* event)
      throw(El::Exception)
    {
      if(El::Service::CompoundService<FeedCrawler>::notify(event))
      {
        return true;
      }
      
      return true;
    }

    void
    FeedManagerImpl::set_message_fetch_filter(
      const ::NewsGate::Moderation::MessageFetchFilterRuleSeq& rules)
      throw(NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      message_filter_.set_message_fetch_filter(rules);
    }
        
    ::NewsGate::Moderation::MessageFetchFilterRuleSeq*
    FeedManagerImpl::get_message_fetch_filter()
      throw(NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      return message_filter_.get_message_fetch_filter();
    }

    void
    FeedManagerImpl::add_message_filter(
      const ::NewsGate::Moderation::MessageIdSeq& ids)
      throw(NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      message_filter_.add_message_filter(ids);
    }
    
    void
    FeedManagerImpl::feed_update_info(
      const ::NewsGate::Moderation::FeedUpdateInfoSeq& feed_infos)
      throw(NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      feed_management_.feed_update_info(feed_infos);
    }
    
    ::NewsGate::Moderation::FeedInfoSeq*
    FeedManagerImpl::feed_info_seq(
      const ::NewsGate::Moderation::FeedIdSeq& ids,
      ::CORBA::Boolean get_stat,
      ::CORBA::ULong stat_from_date,
      ::CORBA::ULong stat_to_date)
      throw(NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      return feed_management_.feed_info_seq(ids,
                                            get_stat,
                                            stat_from_date,
                                            stat_to_date);
    }

    ::NewsGate::Moderation::FeedInfoResult*
    FeedManagerImpl::feed_info_range(
      ::CORBA::ULong start_from,
      ::CORBA::ULong results,
      ::CORBA::Boolean get_stat,
      ::CORBA::ULong stat_from_date,
      ::CORBA::ULong stat_to_date,
      const ::NewsGate::Moderation::SortInfo& sort,
      const ::NewsGate::Moderation::FilterInfo& filter)
      throw(NewsGate::Moderation::FilterRuleError,
            NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      return feed_management_.feed_info_range(start_from,
                                              results,
                                              get_stat,
                                              stat_from_date,
                                              stat_to_date,
                                              sort,
                                              filter);
    }    

    char*
    FeedManagerImpl::xpath_url(const char * xpath,
                               const char * url)
      throw(NewsGate::Moderation::OperationFailed,
            NewsGate::Moderation::ImplementationException,
            CORBA::SystemException)
    {
      return message_adjustment_.xpath_url(xpath, url);
    }
    
    void
    FeedManagerImpl::adjust_message(
      const char* adjustment_script,
      ::NewsGate::Moderation::Transport::MsgAdjustmentContext* ctx,
      ::NewsGate::Moderation::Transport::MsgAdjustmentResult_out result)
      throw(NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      message_adjustment_.adjust_message(adjustment_script, ctx, result);
    }    

    ::NewsGate::Moderation::Transport::MsgAdjustmentContextPack*
    FeedManagerImpl::get_feed_items(const char* url,
                                    ::CORBA::ULong type,
                                    ::CORBA::ULong space,
                                    ::CORBA::ULong country,
                                    ::CORBA::ULong lang,
                                    const char* encoding)
      throw(NewsGate::Moderation::OperationFailed,
            NewsGate::Moderation::ImplementationException,
            CORBA::SystemException)
    {
      return message_adjustment_.get_feed_items(url,
                                                type,
                                                space,
                                                country,
                                                lang,
                                                encoding);
    }

    ::NewsGate::Moderation::Transport::GetHTMLItemsResult*
    FeedManagerImpl::get_html_items(
      const char* url,
      const char* script,
      ::CORBA::ULong type,
      ::CORBA::ULong space,
      ::CORBA::ULong country,
      ::CORBA::ULong lang,
      const ::NewsGate::Moderation::KeywordsSeq& keywords,
      const char* cache,
      const char* encoding)
      throw(NewsGate::Moderation::OperationFailed,
            NewsGate::Moderation::ImplementationException,
            CORBA::SystemException)
    {
      return message_adjustment_.get_html_items(url,
                                                script,
                                                type,
                                                space,
                                                country,
                                                lang,
                                                keywords,
                                                cache,
                                                encoding);
    }
  }  
}
