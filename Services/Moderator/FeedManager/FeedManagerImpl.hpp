/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/FeedManager/FeedManagerImpl.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MODERATOR_FEEDMANAGER_FEEDMANAGERIMPL_HPP_
#define _NEWSGATE_SERVER_SERVICES_MODERATOR_FEEDMANAGER_FEEDMANAGERIMPL_HPP_

#include <El/Exception.hpp>
#include <El/Service/CompoundService.hpp>

#include <Services/Moderator/Commons/FeedManager.hpp>
#include <Services/Moderator/Commons/FeedManager_s.hpp>

#include "FeedCrawler.hpp"
#include "MessageFilter.hpp"
#include "MessageAdjustment.hpp"
#include "FeedManagement.hpp"

namespace NewsGate
{
  namespace Moderation
  {
    class FeedManagerImpl :
      public virtual POA_NewsGate::Moderation::FeedManager,
      public virtual El::Service::CompoundService<FeedCrawler>
    {
    public:
      EL_EXCEPTION(Exception, El::ExceptionBase);

    public:
      FeedManagerImpl(El::Service::Callback* callback)
        throw(Exception, El::Exception);

      virtual ~FeedManagerImpl() throw();

      virtual void wait() throw(Exception, El::Exception);

    protected:
      
      //
      // IDL:NewsGate/Moderation/FeedManager/xpath_url:1.0
      //
      virtual char* xpath_url(const char * xpath,
                               const char * url)
        throw(NewsGate::Moderation::OperationFailed,
              NewsGate::Moderation::ImplementationException,
              CORBA::SystemException);
      
      //
      // IDL:NewsGate/Moderation/FeedManager/validate:1.0
      //
      virtual ::NewsGate::Moderation::ResourceValidationResultSeq* validate(
        const ::NewsGate::Moderation::UrlSeq& urls,
        ::NewsGate::Moderation::ProcessingType processing_type,
        ::NewsGate::Moderation::CreatorId creator_id)
        throw(NewsGate::Moderation::NotReady,
              NewsGate::Moderation::ImplementationException,
              CORBA::SystemException);
      
      //
      //
      // IDL:NewsGate/Moderation/FeedManager/validate_async:1.0
      //
      virtual ::NewsGate::Moderation::ValidationToken validate_async(
        const ::NewsGate::Moderation::UrlSeq& urls,
        ::NewsGate::Moderation::ProcessingType processing_type,
        ::NewsGate::Moderation::CreatorId creator_id,
        ::CORBA::ULong req_period)
        throw(NewsGate::Moderation::NotReady,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);
      
      // IDL:NewsGate/Moderation/FeedManager/add_feeds:1.0
      //
      virtual ::NewsGate::Moderation::ResourceValidationResultSeq* add_feeds(
        const ::NewsGate::Moderation::UrlSeq& feeds,
        const ::NewsGate::Moderation::FeedSourceSeq& feed_sources,
        ::NewsGate::Moderation::CreatorType creator_type,
        ::NewsGate::Moderation::CreatorId creator_id)
        throw(NewsGate::Moderation::NotReady,
              NewsGate::Moderation::ImplementationException,
              CORBA::SystemException);
      
      //
      // IDL:NewsGate/Moderation/FeedManager/task_infos:1.0
      //
      virtual ::NewsGate::Moderation::ValidationTaskInfoSeq* task_infos(
        const ::NewsGate::Moderation::CreatorIdSeq& creator_ids)
        throw(NewsGate::Moderation::NotReady,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);
      
      //
      // IDL:NewsGate/Moderation/FeedManager/validation_result:1.0
      //
      virtual ::NewsGate::Moderation::ResourceValidationResultSeq*
      validation_result(
        const char* task_id,
        const ::NewsGate::Moderation::CreatorIdSeq& creator_ids,
        CORBA::Boolean all_results,
        CORBA::ULong_out total_results)
        throw(NewsGate::Moderation::TaskNotFound,
              NewsGate::Moderation::NotReady,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);
      
      //
      // IDL:NewsGate/Moderation/FeedManager/stop_validation:1.0
      //
      virtual void stop_validation(
        const char* task_id,
        const ::NewsGate::Moderation::CreatorIdSeq& creator_ids)
        throw(NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);
      
      //
      // IDL:NewsGate/Moderation/FeedManager/delete_validation:1.0
      //
      virtual void delete_validation(
        const char* task_id,
        const ::NewsGate::Moderation::CreatorIdSeq& creator_ids)
        throw(NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);
      
      //
      // IDL:NewsGate/Moderation/FeedManager/feeds_info:1.0
      //
      virtual ::NewsGate::Moderation::FeedInfoResult*
      feed_info_range(::CORBA::ULong start_from,
                      ::CORBA::ULong results,
                      ::CORBA::Boolean get_stat,
                      ::CORBA::ULong stat_from_date,
                      ::CORBA::ULong stat_to_date,
                      const ::NewsGate::Moderation::SortInfo& sort,
                      const ::NewsGate::Moderation::FilterInfo& filter)
        throw(NewsGate::Moderation::FilterRuleError,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);
      
    //
    // IDL:NewsGate/Moderation/FeedManager/feed_info_seq:1.0
    //
    virtual ::NewsGate::Moderation::FeedInfoSeq* feed_info_seq(
      const ::NewsGate::Moderation::FeedIdSeq& ids,
      ::CORBA::Boolean get_stat,
      ::CORBA::ULong stat_from_date,
      ::CORBA::ULong stat_to_date)
      throw(NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException);

      //
      // IDL:NewsGate/Moderation/FeedManager/feed_update_info:1.0
      //
      virtual void feed_update_info(
        const ::NewsGate::Moderation::FeedUpdateInfoSeq& feed_infos)
        throw(NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);
      
      //
      // IDL:NewsGate/Moderation/FeedManager/set_message_fetch_filter:1.0
      //
      virtual void set_message_fetch_filter(
        const ::NewsGate::Moderation::MessageFetchFilterRuleSeq& rules)
        throw(NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);
      
      //
      // IDL:NewsGate/Moderation/FeedManager/get_message_fetch_filter:1.0
      //
      virtual ::NewsGate::Moderation::MessageFetchFilterRuleSeq*
      get_message_fetch_filter()
        throw(NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);
      
      //
      // IDL:NewsGate/Moderation/FeedManager/add_message_filter:1.0
      //
      virtual void add_message_filter(
        const ::NewsGate::Moderation::MessageIdSeq& ids)
        throw(NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);
      
      //
      // IDL:NewsGate/Moderation/FeedManager/adjust_message:1.0
      //
      virtual void adjust_message(
          const char * adjustment_script,
          ::NewsGate::Moderation::Transport::MsgAdjustmentContext * ctx,
          ::NewsGate::Moderation::Transport::MsgAdjustmentResult_out result)
        throw(NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);

      //
      // IDL:NewsGate/Moderation/FeedManager/get_feed_items:1.0
      //
      virtual ::NewsGate::Moderation::Transport::MsgAdjustmentContextPack*
      get_feed_items(const char* url,
                     ::CORBA::ULong type,
                     ::CORBA::ULong space,
                     ::CORBA::ULong country,
                     ::CORBA::ULong lang,
                     const char* encoding)
        throw(NewsGate::Moderation::OperationFailed,
              NewsGate::Moderation::ImplementationException,
              CORBA::SystemException);      

      //
      // IDL:NewsGate/Moderation/FeedManager/get_html_items:1.0
      //
      virtual ::NewsGate::Moderation::Transport::GetHTMLItemsResult*
      get_html_items(const char* url,
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
              CORBA::SystemException);      

      virtual bool notify(El::Service::Event* event) throw(El::Exception);
      
    private:
      
      MessageFilter message_filter_;
      MessageAdjustment message_adjustment_;
      FeedManagement feed_management_;
    };

    typedef El::RefCount::SmartPtr<FeedManagerImpl> FeedManagerImpl_var;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Moderation
  {
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_MODERATOR_FEEDMANAGER_FEEDMANAGERIMPL_HPP_
