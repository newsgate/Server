/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Frontends/Moderation/FeedModeration.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_FRONTENDS_MODERATION_FEEDMODERATION_HPP_
#define _NEWSGATE_SERVER_FRONTENDS_MODERATION_FEEDMODERATION_HPP_

#include <string>

#include <El/Exception.hpp>

#include <El/Python/Exception.hpp>
#include <El/Python/Object.hpp>
#include <El/Python/RefCount.hpp>
#include <El/Python/Logger.hpp>
#include <El/Python/Moment.hpp>
#include <El/Python/Lang.hpp>
#include <El/Python/Country.hpp>

#include <Commons/Message/Automation/Automation.hpp>
#include <Services/Moderator/Commons/FeedManager.hpp>

namespace NewsGate
{  
  namespace FeedModeration
  {
    EL_EXCEPTION(Exception, El::ExceptionBase);
    
    typedef El::Corba::SmartRef<NewsGate::Moderation::FeedManager>
    ManagerRef;

    class Manager : public El::Python::ObjectImpl
    {
    public:
      Manager(PyTypeObject *type, PyObject *args, PyObject *kwds)
        throw(Exception, El::Exception);
    
      Manager(const ManagerRef& feed_manager) throw(Exception, El::Exception);

      virtual ~Manager() throw() {}

      PyObject* py_validate_feeds(PyObject* args) throw(El::Exception);
      PyObject* py_validate_feeds_async(PyObject* args) throw(El::Exception);
      PyObject* py_add_feeds(PyObject* args) throw(El::Exception);
      PyObject* py_validation_task_infos(PyObject* args) throw(El::Exception);
      PyObject* py_validation_result(PyObject* args) throw(El::Exception);
      PyObject* py_stop_validation(PyObject* args) throw(El::Exception);
      PyObject* py_delete_validation(PyObject* args) throw(El::Exception);
      PyObject* py_feed_info_range(PyObject* args) throw(El::Exception);
      PyObject* py_feed_info_seq(PyObject* args) throw(El::Exception);
      PyObject* py_feed_update_info(PyObject* args) throw(El::Exception);
      PyObject* py_xpath_url(PyObject* args) throw(El::Exception);
      PyObject* py_adjust_message(PyObject* args) throw(El::Exception);
      PyObject* py_get_feed_items(PyObject* args) throw(El::Exception);
      PyObject* py_get_html_items(PyObject* args) throw(El::Exception);
      
      class Type : public El::Python::ObjectTypeImpl<Manager, Manager::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_METHOD_VARARGS(py_get_feed_items,
                               "get_feed_items",
                               "Get feed items");
        
        PY_TYPE_METHOD_VARARGS(py_get_html_items,
                               "get_html_items",
                               "Get html items");
        
        PY_TYPE_METHOD_VARARGS(py_adjust_message,
                               "adjust_message",
                               "Adjust message with a script");
        
        PY_TYPE_METHOD_VARARGS(py_xpath_url,
                               "xpath_url",
                               "Apply XPath expression to a document at url");
        
        PY_TYPE_METHOD_VARARGS(py_validate_feeds,
                               "validate_feeds",
                               "Validate feeds");
        
        PY_TYPE_METHOD_VARARGS(py_validate_feeds_async,
                               "validate_feeds_async",
                               "Validate feeds asynchronously");
        
        PY_TYPE_METHOD_VARARGS(py_add_feeds, "add_feeds", "Add feeds");

        PY_TYPE_METHOD_VARARGS(py_validation_task_infos,
                               "validation_task_infos",
                               "Validation task infos");
        
        PY_TYPE_METHOD_VARARGS(py_validation_result,
                               "validation_result",
                               "Returns validation task result");
        
        PY_TYPE_METHOD_VARARGS(py_stop_validation,
                               "stop_validation",
                               "Stop validation task");
        
        PY_TYPE_METHOD_VARARGS(py_delete_validation,
                               "delete_validation",
                               "Delete validation task");
        
        PY_TYPE_METHOD_VARARGS(py_feed_info_range,
                               "feed_info_range",
                               "Feeds info range");
        
        PY_TYPE_METHOD_VARARGS(py_feed_info_seq,
                               "feed_info_seq",
                               "Feeds info sequence");
        
        PY_TYPE_METHOD_VARARGS(py_feed_update_info,
                               "feed_update_info",
                               "Feeds update info");        
      };

    private:
      ManagerRef feed_manager_;
    };

    typedef El::Python::SmartPtr<Manager> Manager_var;

    class UrlDesc : public El::Python::ObjectImpl
    {
    public:
      std::string url;
    
    public:
      UrlDesc(PyTypeObject *type = 0, PyObject *args = 0, PyObject *kwds = 0)
        throw(Exception, El::Exception);
    
      virtual ~UrlDesc() throw() {}

      class Type : public El::Python::ObjectTypeImpl<UrlDesc, UrlDesc::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;
        
        PY_TYPE_MEMBER_STRING(url, "url", "Url", false);
      };
    };

    typedef El::Python::SmartPtr<UrlDesc> UrlDesc_var;

    class ResourceValidationResult : public El::Python::ObjectImpl
    {
    public:
      std::string url;
      unsigned long type;
      unsigned long processing_type;
      unsigned long result;
      unsigned long feed_reference_count;
      
      std::string description;
      unsigned long long feed_id;
    
    public:
      ResourceValidationResult(PyTypeObject *type = 0,
                               PyObject *args = 0,
                               PyObject *kwds = 0)
        throw(Exception, El::Exception);
    
      virtual ~ResourceValidationResult() throw() {}

      ResourceValidationResult(const Moderation::ResourceValidationResult& src)
        throw(Exception, El::Exception);
      
      class Type :
        public El::Python::ObjectTypeImpl<ResourceValidationResult,
                                          ResourceValidationResult::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;
        
        PY_TYPE_MEMBER_STRING(url, "url", "Url", false);
        PY_TYPE_MEMBER_ULONG(type, "type", "Resource type");
        
        PY_TYPE_MEMBER_ULONG(processing_type,
                             "processing_type",
                             "Processing type");
        
        PY_TYPE_MEMBER_ULONG(result, "result", "Validation result");
        
        PY_TYPE_MEMBER_ULONG(feed_reference_count,
                             "feed_reference_count",
                             "Feed reference type");
        
        PY_TYPE_MEMBER_STRING(description,
                              "description",
                              "Description",
                              false);

        PY_TYPE_MEMBER_ULONGLONG(feed_id, "feed_id", "Feed id");
      };
    };

    typedef El::Python::SmartPtr<ResourceValidationResult>
    ResourceValidationResult_var;

    class FeedSource : public El::Python::ObjectImpl
    {
    public:
      std::string url;
      unsigned long type;
      unsigned long processing_type;
    
    public:
      FeedSource(PyTypeObject *type = 0,
                 PyObject *args = 0,
                 PyObject *kwds = 0)
        throw(Exception, El::Exception);
    
      virtual ~FeedSource() throw() {}

      class Type :
        public El::Python::ObjectTypeImpl<FeedSource, FeedSource::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;
        
        PY_TYPE_MEMBER_STRING(url, "url", "Url", false);
        PY_TYPE_MEMBER_ULONG(type, "type", "Source type");
        
        PY_TYPE_MEMBER_ULONG(processing_type,
                             "processing_type",
                             "Processing type");
      };
    };

    typedef El::Python::SmartPtr<FeedSource> FeedSource_var;

    class ValidationTaskInfo : public El::Python::ObjectImpl
    {
    public:
      std::string id;
      std::string title;
      unsigned long long creator_id;
      unsigned long status;
      unsigned long started;
      unsigned long feeds;
      unsigned long pending_urls;
      unsigned long long received_bytes;
      unsigned long processed_urls;
    
    public:
      ValidationTaskInfo(PyTypeObject *type = 0,
                         PyObject *args = 0,
                         PyObject *kwds = 0)
        throw(Exception, El::Exception);

      ValidationTaskInfo(const Moderation::ValidationTaskInfo& src)
        throw(El::Exception);
    
      virtual ~ValidationTaskInfo() throw() {}

      class Type :
        public El::Python::ObjectTypeImpl<ValidationTaskInfo,
                                          ValidationTaskInfo::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;
        
        PY_TYPE_MEMBER_STRING(id, "id", "Task id", false);
        PY_TYPE_MEMBER_STRING(title, "title", "Task title", true);
        PY_TYPE_MEMBER_ULONGLONG(creator_id, "creator_id", "Creator id");
        PY_TYPE_MEMBER_ULONG(status, "status", "Task status");
        PY_TYPE_MEMBER_ULONG(started, "started", "Time task started");
        PY_TYPE_MEMBER_ULONG(feeds, "feeds", "Feeds count");
        
        PY_TYPE_MEMBER_ULONG(pending_urls,
                             "pending_urls",
                             "Pending urls count");
        
        PY_TYPE_MEMBER_ULONGLONG(received_bytes,
                                 "received_bytes",
                                 "Bytes received");

        PY_TYPE_MEMBER_ULONG(processed_urls,
                             "processed_urls",
                             "Processed urls count");
      };
    };

    typedef El::Python::SmartPtr<ValidationTaskInfo> ValidationTaskInfo_var;

    class FeedInfoResult : public El::Python::Sequence
    {
    public:
      unsigned long feed_count;
    
    public:
      FeedInfoResult(PyTypeObject *type = 0,
                     PyObject *args = 0,
                     PyObject *kwds = 0)
        throw(Exception, El::Exception);

      FeedInfoResult(const Moderation::FeedInfoResult& src)
        throw(El::Exception);
    
      virtual ~FeedInfoResult() throw() {}

      class Type :
        public El::Python::ObjectTypeImpl<FeedInfoResult,
                                          FeedInfoResult::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;
        
        PY_TYPE_MEMBER_ULONG(feed_count, "feed_count", "Total feed count");
      };
    };

    typedef El::Python::SmartPtr<FeedInfoResult> FeedInfoResult_var;

    class FeedValidationResult : public El::Python::ObjectImpl
    {
    public:
      unsigned long total_results; 
      El::Python::Sequence_var results;
   
    public:
      FeedValidationResult(PyTypeObject *type = 0,
                           PyObject *args = 0,
                           PyObject *kwds = 0)
        throw(Exception, El::Exception);
    
      virtual ~FeedValidationResult() throw() {}

      class Type :
        public El::Python::ObjectTypeImpl<FeedValidationResult,
                                          FeedValidationResult::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;
        
        PY_TYPE_MEMBER_ULONG(total_results,
                             "total_results",
                             "Total validation results count");

        PY_TYPE_MEMBER_OBJECT(results,
                              El::Python::Sequence::Type,
                              "results",
                              "Total validation results",
                              false);
      };
    };

    typedef El::Python::SmartPtr<FeedValidationResult>
    FeedValidationResult_var;    

    class FeedUpdateInfo : public El::Python::ObjectImpl
    {
    public:
      unsigned long long id;
      std::string encoding;
      unsigned long space;
      unsigned long lang;
      unsigned long country;
      unsigned long status;
      std::string keywords;
      std::string adjustment_script;
      std::string comment;
    
    public:
      FeedUpdateInfo(PyTypeObject *type = 0,
                     PyObject *args = 0,
                     PyObject *kwds = 0)
        throw(Exception, El::Exception);

      virtual ~FeedUpdateInfo() throw() {}

      class Type :
        public El::Python::ObjectTypeImpl<FeedUpdateInfo, FeedUpdateInfo::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;
        
        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Feed id");
        PY_TYPE_MEMBER_STRING(encoding, "encoding", "Feed encoding", true);
        PY_TYPE_MEMBER_ULONG(space, "space", "Feed space");
        PY_TYPE_MEMBER_ULONG(lang, "lang", "Feed language");
        PY_TYPE_MEMBER_ULONG(country, "country", "Feed country");
        PY_TYPE_MEMBER_ULONG(status, "status", "Feed status");
        PY_TYPE_MEMBER_STRING(keywords, "keywords", "Feed keywords", true);
        
        PY_TYPE_MEMBER_STRING(adjustment_script,
                              "adjustment_script",
                              "Message adjustment script",
                              true);
        
        PY_TYPE_MEMBER_STRING(comment, "comment", "Feed comment", true);
      };
    };

    typedef El::Python::SmartPtr<FeedUpdateInfo> FeedUpdateInfo_var;

    class FeedInfo : public El::Python::ObjectImpl
    {
    public:
      unsigned long long id;
      unsigned long type;
      unsigned long space;
      El::Python::Lang_var lang;
      El::Python::Country_var country;
      unsigned long status;
      std::string url;
      std::string encoding;
      std::string keywords;
      std::string adjustment_script;
      unsigned long long creator;
      unsigned long creator_type;
      El::Python::Moment_var created;
      El::Python::Moment_var updated;      
      std::string comment;
      std::string channel_title;
      std::string channel_description;
      std::string channel_html_link;
      El::Python::Lang_var channel_lang;
      El::Python::Country_var channel_country;
      long channel_ttl;
      El::Python::Moment_var channel_last_build_date;
      El::Python::Moment_var last_request_date;
      std::string last_modified_hdr;
      std::string etag_hdr;
      long long content_length_hdr;
      unsigned long entropy;
      El::Python::Moment_var entropy_updated_date;
      unsigned long size;
      int single_chunked;
      long long first_chunk_size;
      long heuristics_counter;
      unsigned long requests;
      unsigned long failed;
      unsigned long unchanged;
      unsigned long not_modified;
      unsigned long presumably_unchanged;
      unsigned long has_changes;
      float wasted;
      unsigned long long outbound;
      unsigned long long inbound;
      unsigned long long requests_duration;
      unsigned long long messages;
      unsigned long long messages_size;
      unsigned long long messages_delay;
      unsigned long long max_message_delay;
      unsigned long long msg_impressions;
      unsigned long long msg_clicks;
      float msg_ctr;
    
    public:
      FeedInfo(PyTypeObject *type = 0,
               PyObject *args = 0,
               PyObject *kwds = 0)
        throw(Exception, El::Exception);

      FeedInfo(const Moderation::FeedInfo& src) throw(El::Exception);
    
      virtual ~FeedInfo() throw() {}

      class Type :
        public El::Python::ObjectTypeImpl<FeedInfo, FeedInfo::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Feed id");
        PY_TYPE_MEMBER_ULONG(type, "type", "Feed type");
        PY_TYPE_MEMBER_STRING(encoding, "encoding", "Feed encoding", true);
        PY_TYPE_MEMBER_ULONG(space, "space", "Feed space");

        PY_TYPE_MEMBER_OBJECT(lang,
                              El::Python::Lang::Type,
                              "lang",
                              "Feed language",
                              false);

        PY_TYPE_MEMBER_OBJECT(country,
                              El::Python::Country::Type,
                              "country",
                              "Feed country",
                              false);
        
        PY_TYPE_MEMBER_ULONG(status, "status", "Feed status");
        PY_TYPE_MEMBER_STRING(url, "url", "Feed source url", false);
        PY_TYPE_MEMBER_STRING(keywords, "keywords", "Feed keywords", true);
        
        PY_TYPE_MEMBER_STRING(adjustment_script,
                              "adjustment_script",
                              "Message adjustment script",
                              true);
        
        PY_TYPE_MEMBER_ULONGLONG(creator, "creator", "Feed creator id");
        
        PY_TYPE_MEMBER_ULONG(creator_type,
                             "creator_type",
                             "Feed creator type");

        PY_TYPE_MEMBER_OBJECT(created,
                              El::Python::Moment::Type,
                              "created",
                              "Feed creation date",
                              false);

        PY_TYPE_MEMBER_OBJECT(updated,
                              El::Python::Moment::Type,
                              "updated",
                              "Feed update date",
                              false);

        PY_TYPE_MEMBER_STRING(comment,
                              "comment",
                              "Feed comment",
                              true);

        PY_TYPE_MEMBER_STRING(channel_title,
                              "channel_title",
                              "Feed channel title",
                              true);

        PY_TYPE_MEMBER_STRING(channel_description,
                              "channel_description",
                              "Feed channel description",
                              true);

        PY_TYPE_MEMBER_STRING(channel_html_link,
                              "channel_html_link",
                              "Feed channel html link",
                              true);

        PY_TYPE_MEMBER_OBJECT(channel_lang,
                              El::Python::Lang::Type,
                              "channel_lang",
                              "Feed channel language",
                              false);

        PY_TYPE_MEMBER_OBJECT(channel_country,
                              El::Python::Country::Type,
                              "channel_country",
                              "Feed channel country",
                              false);

        PY_TYPE_MEMBER_LONG(channel_ttl,
                            "channel_ttl",
                            "Feed channel ttl");

        PY_TYPE_MEMBER_OBJECT(channel_last_build_date,
                              El::Python::Moment::Type,
                              "channel_last_build_date",
                              "Feed channel last_build date",
                              false);

        PY_TYPE_MEMBER_OBJECT(last_request_date,
                              El::Python::Moment::Type,
                              "last_request_date",
                              "Feed last request date",
                              false);

        PY_TYPE_MEMBER_STRING(last_modified_hdr,
                              "last_modified_hdr",
                              "Feed Last-Modified header value",
                              true);

        PY_TYPE_MEMBER_STRING(etag_hdr,
                              "etag_hdr",
                              "Feed ETag header value",
                              true);

        PY_TYPE_MEMBER_LONGLONG(content_length_hdr,
                                "content_length_hdr",
                                "Feed Content-Length header value");

        PY_TYPE_MEMBER_ULONG(entropy,
                             "entropy",
                             "Feed channel entropy value");

       PY_TYPE_MEMBER_OBJECT(entropy_updated_date,
                              El::Python::Moment::Type,
                              "entropy_updated_date",
                              "Feed channel entropy update date",
                              false);

        PY_TYPE_MEMBER_ULONG(size,
                             "size",
                             "Feed channel max entries per request");

        PY_TYPE_MEMBER_INT(single_chunked,
                           "single_chunked",
                           "Feed response single-chunked flag");

        PY_TYPE_MEMBER_LONGLONG(first_chunk_size,
                                "first_chunk_size",
                                "Feed HTTP-response first chunk size");

        PY_TYPE_MEMBER_LONG(heuristics_counter,
                            "heuristics_counter",
                            "Feed request heuristics counter"); 

        PY_TYPE_MEMBER_ULONG(requests, "requests", "Feed requests count"); 
        PY_TYPE_MEMBER_ULONG(failed, "failed", "Feed failed requests count");
        
        PY_TYPE_MEMBER_ULONG(unchanged,
                             "unchanged",
                             "Feed unchanged times on request");
        
        PY_TYPE_MEMBER_ULONG(not_modified,
                             "not_modified",
                             "Feed mot modified times on request");
        
        PY_TYPE_MEMBER_ULONG(presumably_unchanged,
                             "presumably_unchanged",
                             "Number of times system presumes feed unchanged");
        
        PY_TYPE_MEMBER_ULONG(has_changes,
                             "has_changes",
                             "Number of times feed has changes");
        
        PY_TYPE_MEMBER_FLOAT(wasted, "wasted", "Feed wasted request count");
        
        PY_TYPE_MEMBER_ULONGLONG(outbound,
                                 "outbound",
                                 "Feed outbound traffic");
        
        PY_TYPE_MEMBER_ULONGLONG(inbound, "inbound", "Feed inbound traffic");
        
        PY_TYPE_MEMBER_ULONGLONG(requests_duration,
                                 "requests_duration",
                                 "Feed requests cumulative duration");
        
        PY_TYPE_MEMBER_ULONGLONG(messages, "messages", "Feed message count");
        
        PY_TYPE_MEMBER_ULONGLONG(messages_size,
                                 "messages_size",
                                 "Feed messages cumulative size");
        
        PY_TYPE_MEMBER_ULONGLONG(messages_delay,
                                 "messages_delay",
                                 "Feed messages cumulative delay");
        
        PY_TYPE_MEMBER_ULONGLONG(max_message_delay,
                                 "max_message_delay",
                                 "Feed messages maximum delay");
        
        PY_TYPE_MEMBER_ULONGLONG(msg_impressions,
                                 "msg_impressions",
                                 "Feed messages impressions");
        
        PY_TYPE_MEMBER_ULONGLONG(msg_clicks,
                                 "msg_clicks",
                                 "Feed messages clicks");
        
        PY_TYPE_MEMBER_FLOAT(msg_ctr,
                             "msg_ctr",
                             "Feed messages CTR");
      };
    };

    typedef El::Python::SmartPtr<FeedInfo> FeedInfo_var;

    class SortInfo : public El::Python::ObjectImpl
    {
    public:
      unsigned long field;
      bool descending;
    
    public:
      SortInfo(PyTypeObject *type = 0,
               PyObject *args = 0,
               PyObject *kwds = 0)
        throw(Exception, El::Exception);

      virtual ~SortInfo() throw() {}

      class Type :
        public El::Python::ObjectTypeImpl<SortInfo, SortInfo::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;
        
        PY_TYPE_MEMBER_ULONG(field, "field", "Field result to be sorted by");
        
        PY_TYPE_MEMBER_BOOL(descending,
                            "descending",
                            "To sort in descending order");
      };
    };

    typedef El::Python::SmartPtr<SortInfo> SortInfo_var;

    class FilterRule : public El::Python::ObjectImpl
    {
    public:
      unsigned long id;
      unsigned long field;
      unsigned long operation;
      El::Python::Sequence_var args;
    
    public:
      FilterRule(PyTypeObject *type = 0,
                 PyObject *args = 0,
                 PyObject *kwds = 0)
        throw(Exception, El::Exception);

      virtual ~FilterRule() throw() {}

      class Type :
        public El::Python::ObjectTypeImpl<FilterRule, FilterRule::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;
        
        PY_TYPE_MEMBER_ULONG(id, "id", "Rule id");
        PY_TYPE_MEMBER_ULONG(field, "field", "Field rule to be applied to");
        
        PY_TYPE_MEMBER_ULONG(operation, "operation", "Filter operation");
        
        PY_TYPE_MEMBER_OBJECT(args,
                              El::Python::Sequence::Type,
                              "args",
                              "Filter rule arguments",
                              false);
      };
    };

    typedef El::Python::SmartPtr<FilterRule> FilterRule_var;

    class FilterInfo : public El::Python::ObjectImpl
    {
    public:
      bool consider_deleted;
      El::Python::Sequence_var rules;
    
    public:
      FilterInfo(PyTypeObject *type = 0,
                 PyObject *args = 0,
                 PyObject *kwds = 0)
        throw(Exception, El::Exception);

      virtual ~FilterInfo() throw() {}

      class Type :
        public El::Python::ObjectTypeImpl<FilterInfo, FilterInfo::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_BOOL(consider_deleted,
                            "consider_deleted",
                            "If to consider deleted feeds");
        
        PY_TYPE_MEMBER_OBJECT(rules,
                              El::Python::Sequence::Type,
                              "rules",
                              "Filter rules",
                              false);
      };
    };

    typedef El::Python::SmartPtr<FilterInfo> FilterInfo_var;

    class FilterRuleError : public El::Python::ObjectImpl
    {
    public:
      unsigned long id;
      std::string description;
    
    public:
      FilterRuleError(PyTypeObject *type = 0,
                      PyObject *args = 0,
                      PyObject *kwds = 0)
        throw(Exception, El::Exception);

      virtual ~FilterRuleError() throw() {}

      class Type :
        public El::Python::ObjectTypeImpl<FilterRuleError,
                                          FilterRuleError::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;
        
        PY_TYPE_MEMBER_ULONG(id, "id", "Rule id");

        PY_TYPE_MEMBER_STRING(description,
                              "description",
                              "Rule error description",
                              true);
      };
    };

    typedef El::Python::SmartPtr<FilterRuleError> FilterRuleError_var;

    class OperationFailed : public El::Python::ObjectImpl
    {
    public:
      std::string reason;
    
    public:
      OperationFailed(PyTypeObject *type = 0,
                      PyObject *args = 0,
                      PyObject *kwds = 0)
        throw(Exception, El::Exception);

      virtual ~OperationFailed() throw() {}

      class Type :
        public El::Python::ObjectTypeImpl<OperationFailed,
                                          OperationFailed::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;
        
        PY_TYPE_MEMBER_STRING(reason,
                              "reason",
                              "Operation failure reason",
                              true);
      };
    };

    typedef El::Python::SmartPtr<OperationFailed> OperationFailed_var;

    class MsgAdjustmentResult : public El::Python::ObjectImpl
    {
    public:
      MsgAdjustmentResult(PyTypeObject *type = 0,
                          PyObject *args = 0,
                          PyObject *kwds = 0)
        throw(El::Exception);
      
      MsgAdjustmentResult(const ::NewsGate::Message::Automation::Message& msg,
                          const char* lg,
                          const char* err)
        throw(El::Exception);
      
      virtual ~MsgAdjustmentResult() throw() {}

      class Type :
        public El::Python::ObjectTypeImpl<MsgAdjustmentResult,
                                          MsgAdjustmentResult::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;
        
        PY_TYPE_MEMBER_OBJECT(
          message,
          ::NewsGate::Message::Automation::Python::Message::Type,
          "message",
          "Adjusted message",
          false);

        PY_TYPE_MEMBER_STRING(log, "log", "Adjustment log", true);
        PY_TYPE_MEMBER_STRING(error, "error", "Adjustment error", true);
      };
    
      ::NewsGate::Message::Automation::Python::Message_var message;
      std::string log;
      std::string error;
    };
      
    typedef El::Python::SmartPtr<MsgAdjustmentResult> MsgAdjustmentResult_var;

    class GetHTMLItemsResult : public El::Python::ObjectImpl
    {
    public:
      GetHTMLItemsResult(PyTypeObject *type = 0,
                         PyObject *args = 0,
                         PyObject *kwds = 0)
        throw(El::Exception);
      
      GetHTMLItemsResult(
        const ::NewsGate::Message::Automation::MessageArray& msgs,
        const char* lg,
        const char* err,
        const char* cache,
        bool interrupted)
        throw(El::Exception);
      
      virtual ~GetHTMLItemsResult() throw() {}

      class Type :
        public El::Python::ObjectTypeImpl<GetHTMLItemsResult,
                                          GetHTMLItemsResult::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;
        
        PY_TYPE_MEMBER_OBJECT(
          messages,
          El::Python::Sequence::Type,
          "messages",
          "Resulted message",
          false);

        PY_TYPE_MEMBER_STRING(log, "log", "Operation log", true);
        PY_TYPE_MEMBER_STRING(error, "error", "Operation error", true);
        PY_TYPE_MEMBER_STRING(cache, "cache", "Request cache", true);
        
        PY_TYPE_MEMBER_BOOL(interrupted,
                            "interrupted",
                            "Script interruption flag");
      };
    
      El::Python::Sequence_var messages;
      std::string log;
      std::string error;
      std::string cache;
      bool interrupted;
    };
      
    typedef El::Python::SmartPtr<GetHTMLItemsResult> GetHTMLItemsResult_var;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{  
  namespace FeedModeration
  {
    //
    // NewsGate::FeedModeration::Manager::Type class
    //
    inline
    Manager::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<Manager, Manager::Type>(
          "newsgate.moderation.feed.Manager",
          "Object representing feed management functionality")
    {
      tp_new = 0;
    }

    //
    // NewsGate::FeedModeration::UrlDesc::Type class
    //
    inline
    UrlDesc::Type::Type() throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<UrlDesc, UrlDesc::Type>(
          "newsgate.moderation.feed.UrlDesc",
          "Object representing url descriptor")
    {
    }

    //
    // NewsGate::FeedModeration::ResourceValidationResult::Type class
    //
    inline
    ResourceValidationResult::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<
      ResourceValidationResult,
      ResourceValidationResult::Type>(
        "newsgate.moderation.feed.ResourceValidationResult",
        "Object representing resource validation result")
    {
    }

    //
    // NewsGate::FeedModeration::FeedSource::Type class
    //
    inline
    FeedSource::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<FeedSource, FeedSource::Type>(
          "newsgate.moderation.feed.FeedSource",
          "Object representing feed source")
    {
    }

    //
    // NewsGate::FeedModeration::ValidationTaskInfo::Type class
    //
    inline
    ValidationTaskInfo::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<
      ValidationTaskInfo,
      ValidationTaskInfo::Type>(
        "newsgate.moderation.feed.ValidationTaskInfo",
        "Object representing validation task info")
    {
    }

    //
    // NewsGate::FeedModeration::FeedInfo::Type class
    //
    inline
    FeedInfo::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<FeedInfo, FeedInfo::Type>(
          "newsgate.moderation.feed.FeedInfo",
          "Object representing feed info")
    {
    }

    //
    // NewsGate::FeedModeration::FeedUpdateInfo::Type class
    //
    inline
    FeedUpdateInfo::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<FeedUpdateInfo, FeedUpdateInfo::Type>(
          "newsgate.moderation.feed.FeedUpdateInfo",
          "Object representing feed update info")
    {
    }

    //
    // NewsGate::FeedModeration::FeedInfoResult::Type class
    //
    inline
    FeedInfoResult::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<FeedInfoResult, FeedInfoResult::Type>(
          "newsgate.moderation.feed.FeedInfoResult",
          "Object representing Moderator.feeds_info result",
          "el.Sequence")
    {
    }

    //
    // NewsGate::FeedModeration::FeedValidationResult::Type class
    //
    inline
    FeedValidationResult::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<FeedValidationResult,
                                     FeedValidationResult::Type>(
          "newsgate.moderation.feed.FeedValidationResult",
          "Object representing result of validate_feeds call")
    {
    }

    //
    // NewsGate::FeedModeration::SortInfo::Type class
    //
    inline
    SortInfo::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<SortInfo, SortInfo::Type>(
          "newsgate.moderation.feed.SortInfo",
          "Object representing sorting info")
    {
    }

    //
    // NewsGate::FeedModeration::FilterRule::Type class
    //
    inline
    FilterRule::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<FilterRule, FilterRule::Type>(
          "newsgate.moderation.feed.FilterRule",
          "Object representing filtering rule")
    {
    }

    //
    // NewsGate::FeedModeration::FilterInfo::Type class
    //
    inline
    FilterInfo::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<FilterInfo, FilterInfo::Type>(
          "newsgate.moderation.feed.FilterInfo",
          "Object representing filter info")
    {
    }

    //
    // NewsGate::FeedModeration::FilterRuleError::Type class
    //
    inline
    FilterRuleError::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<FilterRuleError, FilterRuleError::Type>(
          "newsgate.moderation.feed.FilterRuleError",
          "Object representing filter rule error")
    {
    }
    
    //
    // NewsGate::FeedModeration::OperationFailed::Type class
    //
    inline
    OperationFailed::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<OperationFailed, OperationFailed::Type>(
          "newsgate.moderation.feed.OperationFailed",
          "Object representing operation failure exception")
    {
    }

    //
    // MsgAdjustmentResult class
    //
    inline
    MsgAdjustmentResult::MsgAdjustmentResult(PyTypeObject *type,
                                             PyObject *args,
                                             PyObject *kwds)
      throw(El::Exception) :
        El::Python::ObjectImpl(type ? type : &Type::instance),
        message(new ::NewsGate::Message::Automation::Python::Message())
    {
    }

    inline
    MsgAdjustmentResult::MsgAdjustmentResult(
      const ::NewsGate::Message::Automation::Message& msg,
      const char* lg,
      const char* err)
      throw(El::Exception) :
        El::Python::ObjectImpl(&Type::instance),
        message(new ::NewsGate::Message::Automation::Python::Message(msg)),
        log(lg ? lg : ""),
        error(err ? err : "")
    {
    }

    //
    // MsgAdjustmentResult::Type class
    //
    inline
    MsgAdjustmentResult::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<MsgAdjustmentResult,
                                     MsgAdjustmentResult::Type>(
          "newsgate.MsgAdjustmentResult",
          "Object representing adjustment script execution result")
    {
    }

    //
    // GetHTMLItemsResult class
    //
    inline
    GetHTMLItemsResult::GetHTMLItemsResult(PyTypeObject *type,
                                           PyObject *args,
                                           PyObject *kwds)
      throw(El::Exception) :
        El::Python::ObjectImpl(type ? type : &Type::instance),
        messages(new El::Python::Sequence()),
        interrupted(0)
    {
    }

    inline
    GetHTMLItemsResult::GetHTMLItemsResult(
      const ::NewsGate::Message::Automation::MessageArray& msgs,
      const char* lg,
      const char* err,
      const char* ch,
      bool intr)
      throw(El::Exception) :
        El::Python::ObjectImpl(&Type::instance),
        messages(new El::Python::Sequence()),
        log(lg ? lg : ""),
        error(err ? err : ""),
        cache(ch ? ch : ""),
        interrupted(intr)
    {
      messages->from_container<
        ::NewsGate::Message::Automation::Python::Message,
        ::NewsGate::Message::Automation::MessageArray>(msgs);
    }

    //
    // GetHTMLItemsResult::Type class
    //
    inline
    GetHTMLItemsResult::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<GetHTMLItemsResult,
                                     GetHTMLItemsResult::Type>(
          "newsgate.GetHTMLItemsResult",
          "Object representing HTML feed script execution result")
    {
    }
  }
}

#endif // _NEWSGATE_SERVER_FRONTENDS_MODERATION_FEEDMODERATION_HPP_
