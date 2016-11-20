/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Frontends/Moderation/FeedModeration.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <Python.h>
#include <El/CORBA/Corba.hpp>

#include <string>
#include <sstream>

#include <El/Exception.hpp>
#include <El/String/Manip.hpp>

#include <El/Python/Object.hpp>
#include <El/Python/Module.hpp>
#include <El/Python/Utility.hpp>
#include <El/Python/RefCount.hpp>
#include <El/Python/Sequence.hpp>

#include <Commons/Search/SearchExpression.hpp>

#include <xsd/DataFeed/RSS/MsgAdjustment.hpp>
#include <Services/Moderator/Commons/ModeratorManager.hpp>
#include <Services/Moderator/Commons/TransportImpl.hpp>
#include <Services/Moderator/Commons/FeedManager.hpp>

#include "Moderation.hpp"
#include "FeedModeration.hpp"

namespace NewsGate
{
  namespace FeedModeration
  {
    Manager::Type Manager::Type::instance;
    
    UrlDesc::Type UrlDesc::Type::instance;
    FeedSource::Type FeedSource::Type::instance;
    ResourceValidationResult::Type ResourceValidationResult::Type::instance;
    ValidationTaskInfo::Type ValidationTaskInfo::Type::instance;
    FeedInfo::Type FeedInfo::Type::instance;
    FeedInfoResult::Type FeedInfoResult::Type::instance;
    FeedValidationResult::Type FeedValidationResult::Type::instance;
    SortInfo::Type SortInfo::Type::instance;
    FilterRule::Type FilterRule::Type::instance;
    FilterInfo::Type FilterInfo::Type::instance;
    FilterRuleError::Type FilterRuleError::Type::instance;
    OperationFailed::Type OperationFailed::Type::instance;
    FeedUpdateInfo::Type FeedUpdateInfo::Type::instance;
    MsgAdjustmentResult::Type MsgAdjustmentResult::Type::instance;
    GetHTMLItemsResult::Type GetHTMLItemsResult::Type::instance;
    
    //
    // NewsGate::MessageModeration::MessageModerationPyModule class
    //
    
    class FeedModerationPyModule :
      public El::Python::ModuleImpl<FeedModerationPyModule>
    {
    public:
      static FeedModerationPyModule instance;

      FeedModerationPyModule() throw(El::Exception);
      
      virtual void initialized() throw(El::Exception);

      El::Python::Object_var task_not_found_ex;
      El::Python::Object_var filter_rule_error_ex;
      El::Python::Object_var opration_failed_ex;
    };
  
    FeedModerationPyModule FeedModerationPyModule::instance;
  
    FeedModerationPyModule::FeedModerationPyModule()
      throw(El::Exception)
        : El::Python::ModuleImpl<FeedModerationPyModule>(
        "newsgate.moderation.feed",
        "Module containing Feed Moderation types.",
        true)
    {
    }

    void
    FeedModerationPyModule::initialized() throw(El::Exception)
    {
      task_not_found_ex = create_exception("TaskNotFound");
      filter_rule_error_ex = create_exception("FilterRuleError");
      opration_failed_ex = create_exception("OperationFailed");

      add_member(PyLong_FromLong(Moderation::PT_CHECK_FEEDS),
                 "PT_CHECK_FEEDS");
    
      add_member(PyLong_FromLong(Moderation::PT_PARSE_PAGE_FOR_FEEDS),
                 "PT_PARSE_PAGE_FOR_FEEDS");
    
      add_member(PyLong_FromLong(Moderation::PT_LOOK_AROUND_PAGE_FOR_FEEDS),
                 "PT_LOOK_AROUND_PAGE_FOR_FEEDS");
    
      add_member(PyLong_FromLong(Moderation::PT_PARSE_DOMAIN_FOR_FEEDS),
                 "PT_PARSE_DOMAIN_FOR_FEEDS");

      add_member(PyLong_FromLong(Moderation::VR_VALID), "VR_VALID");

      add_member(PyLong_FromLong(Moderation::VR_ALREADY_IN_THE_SYSTEM),
                 "VR_ALREADY_IN_THE_SYSTEM");
    
      add_member(PyLong_FromLong(Moderation::VR_UNRECOGNIZED_RESOURCE_TYPE),
                 "VR_UNRECOGNIZED_RESOURCE_TYPE");
    
      add_member(PyLong_FromLong(Moderation::VR_NO_VALID_RESPONSE),
                 "VR_NO_VALID_RESPONSE");

      add_member(PyLong_FromLong(Moderation::VR_TOO_LONG_URL),
                 "VR_TOO_LONG_URL");

      add_member(PyLong_FromLong(Moderation::RT_UNKNOWN), "RT_UNKNOWN");
      add_member(PyLong_FromLong(Moderation::RT_RSS), "RT_RSS");
      add_member(PyLong_FromLong(Moderation::RT_ATOM), "RT_ATOM");
      add_member(PyLong_FromLong(Moderation::RT_RDF), "RT_RDF");
      add_member(PyLong_FromLong(Moderation::RT_HTML_FEED), "RT_HTML_FEED");
      add_member(PyLong_FromLong(Moderation::RT_HTML), "RT_HTML");
      add_member(PyLong_FromLong(Moderation::RT_META_URL), "RT_META_URL");

      add_member(PyLong_FromLong(Moderation::CT_CRAWLER), "CT_CRAWLER");
      add_member(PyLong_FromLong(Moderation::CT_MODERATOR), "CT_MODERATOR");
      add_member(PyLong_FromLong(Moderation::CT_USER), "CT_USER");
      add_member(PyLong_FromLong(Moderation::CT_ADMIN), "CT_ADMIN");

      add_member(PyLong_FromLong(Moderation::FST_SINGLE_FEED_HTML),
                 "FST_SINGLE_FEED_HTML");
    
      add_member(PyLong_FromLong(Moderation::FST_MULTI_FEED_HTML),
                 "FST_MULTI_FEED_HTML");
    
      add_member(PyLong_FromLong(Moderation::FST_META_URL), "FST_META_URL");

      add_member(PyLong_FromLong(Moderation::VS_ACTIVE), "VS_ACTIVE");
      add_member(PyLong_FromLong(Moderation::VS_SUCCESS), "VS_SUCCESS");
      add_member(PyLong_FromLong(Moderation::VS_ERROR), "VS_ERROR");
      
      add_member(PyLong_FromLong(Moderation::VS_INTERRUPTED),
                 "VS_INTERRUPTED");

      add_member(PyLong_FromLong(Feed::TP_UNDEFINED), "TP_UNDEFINED");
      add_member(PyLong_FromLong(Feed::TP_RSS), "TP_RSS");
      add_member(PyLong_FromLong(Feed::TP_ATOM), "TP_ATOM");
      add_member(PyLong_FromLong(Feed::TP_RDF), "TP_RDF");
      add_member(PyLong_FromLong(Feed::TP_HTML), "TP_HTML");
    
      add_member(PyLong_FromLong(Feed::SP_UNDEFINED), "SP_UNDEFINED");
      add_member(PyLong_FromLong(Feed::SP_NEWS), "SP_NEWS");
      add_member(PyLong_FromLong(Feed::SP_TALK), "SP_TALK");
      add_member(PyLong_FromLong(Feed::SP_AD), "SP_AD");
      add_member(PyLong_FromLong(Feed::SP_BLOG), "SP_BLOG");
      add_member(PyLong_FromLong(Feed::SP_ARTICLE), "SP_ARTICLE");
      add_member(PyLong_FromLong(Feed::SP_PHOTO), "SP_PHOTO");
      add_member(PyLong_FromLong(Feed::SP_VIDEO), "SP_VIDEO");
      add_member(PyLong_FromLong(Feed::SP_AUDIO), "SP_AUDIO");
      add_member(PyLong_FromLong(Feed::SP_PRINTED), "SP_PRINTED");

      add_member(PyLong_FromLong(Feed::ST_ENABLED), "ST_ENABLED");
      add_member(PyLong_FromLong(Feed::ST_DISABLED), "ST_DISABLED");
      add_member(PyLong_FromLong(Feed::ST_PENDING), "ST_PENDING");
      add_member(PyLong_FromLong(Feed::ST_DELETED), "ST_DELETED");

      add_member(PyLong_FromLong(Moderation::FS_NONE), "FS_NONE");
      add_member(PyLong_FromLong(Moderation::FS_ID), "FS_ID");
      add_member(PyLong_FromLong(Moderation::FS_TYPE), "FS_TYPE");
      add_member(PyLong_FromLong(Moderation::FS_URL), "FS_URL");
      add_member(PyLong_FromLong(Moderation::FS_ENCODING), "FS_ENCODING");
      add_member(PyLong_FromLong(Moderation::FS_KEYWORDS), "FS_KEYWORDS");
      
      add_member(PyLong_FromLong(Moderation::FS_ADJUSTMENT_SCRIPT),
                 "FS_ADJUSTMENT_SCRIPT");
      
      add_member(PyLong_FromLong(Moderation::FS_SPACE), "FS_SPACE");
      add_member(PyLong_FromLong(Moderation::FS_LANG), "FS_LANG");
      add_member(PyLong_FromLong(Moderation::FS_COUNTRY), "FS_COUNTRY");
      add_member(PyLong_FromLong(Moderation::FS_STATUS), "FS_STATUS");
      add_member(PyLong_FromLong(Moderation::FS_CREATOR), "FS_CREATOR");
    
      add_member(PyLong_FromLong(Moderation::FS_CREATOR_TYPE),
                 "FS_CREATOR_TYPE");
    
      add_member(PyLong_FromLong(Moderation::FS_COMMENT), "FS_COMMENT");
      add_member(PyLong_FromLong(Moderation::FS_CREATED), "FS_CREATED");
      add_member(PyLong_FromLong(Moderation::FS_UPDATED), "FS_UPDATED");
    
      add_member(PyLong_FromLong(Moderation::FS_CHANNEL_TITLE),
                 "FS_CHANNEL_TITLE");
    
      add_member(PyLong_FromLong(Moderation::FS_CHANNEL_DESCRIPTION),
                 "FS_CHANNEL_DESCRIPTION");
    
      add_member(PyLong_FromLong(Moderation::FS_CHANNEL_HTML_LINK),
                 "FS_CHANNEL_HTML_LINK");
    
      add_member(PyLong_FromLong(Moderation::FS_CHANNEL_LANG),
                 "FS_CHANNEL_LANG");
    
      add_member(PyLong_FromLong(Moderation::FS_CHANNEL_COUNTRY),
                 "FS_CHANNEL_COUNTRY");
    
      add_member(PyLong_FromLong(Moderation::FS_CHANNEL_TTL),
                 "FS_CHANNEL_TTL");
    
      add_member(PyLong_FromLong(Moderation::FS_CHANNEL_LAST_BUILD_DATE),
                 "FS_CHANNEL_LAST_BUILD_DATE");
    
      add_member(PyLong_FromLong(Moderation::FS_LAST_REQUEST_DATE),
                 "FS_LAST_REQUEST_DATE");
    
      add_member(PyLong_FromLong(Moderation::FS_LAST_MODIFIED_HDR),
                 "FS_LAST_MODIFIED_HDR");
    
      add_member(PyLong_FromLong(Moderation::FS_ETAG_HDR), "FS_ETAG_HDR");
    
      add_member(PyLong_FromLong(Moderation::FS_CONTENT_LENGTH_HDR),
                 "FS_CONTENT_LENGTH_HDR");
    
      add_member(PyLong_FromLong(Moderation::FS_ENTROPY), "FS_ENTROPY");
    
      add_member(PyLong_FromLong(Moderation::FS_ENTROPY_UPDATED_DATE),
                 "FS_ENTROPY_UPDATED_DATE");
    
      add_member(PyLong_FromLong(Moderation::FS_SIZE), "FS_SIZE");
    
      add_member(PyLong_FromLong(Moderation::FS_SINGLE_CHUNKED),
                 "FS_SINGLE_CHUNKED");
    
      add_member(PyLong_FromLong(Moderation::FS_FIRST_CHUNK_SIZE),
                 "FS_FIRST_CHUNK_SIZE");
    
      add_member(PyLong_FromLong(Moderation::FS_HEURISTICS_COUNTER),
                 "FS_HEURISTICS_COUNTER");
    
      add_member(PyLong_FromLong(Moderation::FS_REQUESTS), "FS_REQUESTS");
      add_member(PyLong_FromLong(Moderation::FS_FAILED), "FS_FAILED");
      add_member(PyLong_FromLong(Moderation::FS_UNCHANGED), "FS_UNCHANGED");
    
      add_member(PyLong_FromLong(Moderation::FS_NOT_MODIFIED),
                 "FS_NOT_MODIFIED");
    
      add_member(PyLong_FromLong(Moderation::FS_PRESUMABLY_UNCHANGED),
                 "FS_PRESUMABLY_UNCHANGED");
    
      add_member(PyLong_FromLong(Moderation::FS_HAS_CHANGES),
                 "FS_HAS_CHANGES");
      
      add_member(PyLong_FromLong(Moderation::FS_WASTED), "FS_WASTED");
      add_member(PyLong_FromLong(Moderation::FS_OUTBOUND), "FS_OUTBOUND");
      add_member(PyLong_FromLong(Moderation::FS_INBOUND), "FS_INBOUND");
    
      add_member(PyLong_FromLong(Moderation::FS_REQUESTS_DURATION),
                 "FS_REQUESTS_DURATION");
    
      add_member(PyLong_FromLong(Moderation::FS_MESSAGES), "FS_MESSAGES");
    
      add_member(PyLong_FromLong(Moderation::FS_MESSAGES_SIZE),
                 "FS_MESSAGES_SIZE");
    
      add_member(PyLong_FromLong(Moderation::FS_MESSAGES_DELAY),
                 "FS_MESSAGES_DELAY");
    
      add_member(PyLong_FromLong(Moderation::FS_MAX_MESSAGE_DELAY),
                 "FS_MAX_MESSAGE_DELAY");

      add_member(PyLong_FromLong(Moderation::FS_MSG_IMPRESSIONS),
                 "FS_MSG_IMPRESSIONS");

      add_member(PyLong_FromLong(Moderation::FS_MSG_CLICKS), "FS_MSG_CLICKS");

      add_member(PyLong_FromLong(Moderation::FS_MSG_CTR), "FS_MSG_CTR");

      add_member(PyLong_FromLong(Moderation::FO_LIKE), "FO_LIKE");
      add_member(PyLong_FromLong(Moderation::FO_NOT_LIKE), "FO_NOT_LIKE");
      add_member(PyLong_FromLong(Moderation::FO_REGEXP), "FO_REGEXP");
      add_member(PyLong_FromLong(Moderation::FO_NOT_REGEXP), "FO_NOT_REGEXP");
      add_member(PyLong_FromLong(Moderation::FO_EQ), "FO_EQ");
      add_member(PyLong_FromLong(Moderation::FO_NE), "FO_NE");
      add_member(PyLong_FromLong(Moderation::FO_LT), "FO_LT");
      add_member(PyLong_FromLong(Moderation::FO_LE), "FO_LE");
      add_member(PyLong_FromLong(Moderation::FO_GT), "FO_GT");
      add_member(PyLong_FromLong(Moderation::FO_GE), "FO_GE");
      add_member(PyLong_FromLong(Moderation::FO_ANY_OF), "FO_ANY_OF");
      add_member(PyLong_FromLong(Moderation::FO_NONE_OF), "FO_NONE_OF");
    }
    
    //
    // NewsGate::MessageModeration::Manager class
    //
    
    Manager::Manager(PyTypeObject *type, PyObject *args, PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type)
    {
      throw Exception(
        "NewsGate::FeedModeration::Manager::Manager: unforseen way "
        "of object creation");
    }
    
    Manager::Manager(const ManagerRef& manager)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          feed_manager_(manager)
    {
    }    

    PyObject*
    Manager::py_adjust_message(PyObject* args) throw(El::Exception)
    {
      char* adjustment_script = 0;
      PyObject* ctx_obj = 0;
      
      if(!PyArg_ParseTuple(
           args,
           "sO:newsgate.moderation.feed.Manager.adjust_message",
           &adjustment_script,
           &ctx_obj))
      {
        El::Python::handle_error(
          "NewsGate::FeedModeration::Manager::py_adjust_message");
      }

      RSS::MsgAdjustment::Python::Context_var context =
          RSS::MsgAdjustment::Python::Context::Type::down_cast(
            ctx_obj,
            true);

      Moderation::Transport::MsgAdjustmentContextImpl::Var
        context_transport =
        Moderation::Transport::MsgAdjustmentContextImpl::Init::create(
          new RSS::MsgAdjustment::Context());
      
      context->save(context_transport->entity());

      try
      {
        El::Python::AllowOtherThreads guard;
        
        Moderation::FeedManager_var feed_manager =
            feed_manager_.object();

        Moderation::Transport::MsgAdjustmentResult_var res;
        
//        std::cerr << "SCRIPT:\n'" << adjustment_script << "'\n";
      
        feed_manager->adjust_message(adjustment_script,
                                     context_transport.in(),
                                     res.out());

        Moderation::Transport::MsgAdjustmentResultImpl::Type*
            result = dynamic_cast<
            Moderation::Transport::MsgAdjustmentResultImpl::Type*>(res.in());
        
        if(result == 0)
        {
          throw Exception(
            "NewsGate::FeedModeration::Manager::py_adjust_message: "
            "dynamic_cast<NewsGate::Moderation::Transport::"
            "MsgAdjustmentResultImpl::Type*> failed");            
        }

        const Moderation::Transport::MsgAdjustmentResultStruct& r =
          result->entity();

        return new MsgAdjustmentResult(r.message,
                                       r.log.c_str(),
                                       r.error.c_str());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FeedModeration::Manager::adjust_message: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FeedModeration::Manager::adjust_message: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    }
    
    PyObject*
    Manager::py_get_feed_items(PyObject* args) throw(El::Exception)
    {
      const char* feed_url = 0;
      unsigned long feed_type = 0;
      unsigned long feed_space = 0;
      unsigned long feed_country = 0;
      unsigned long feed_lang = 0;
      const char* encoding = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "skkkks:newsgate.moderation.feed.Manager.get_feed_items",
           &feed_url,
           &feed_type,
           &feed_space,
           &feed_country,
           &feed_lang,
           &encoding))
      {
        El::Python::handle_error(
          "NewsGate::FeedModeration::Manager::py_get_feed_items");
      }

      try
      {
        std::auto_ptr< RSS::MsgAdjustment::ContextArray > contexts;
        
        {
          El::Python::AllowOtherThreads guard;
        
          ::NewsGate::Moderation::FeedManager_var feed_manager =
              feed_manager_.object();
        
          Moderation::Transport::MsgAdjustmentContextPack_var res =
            feed_manager->get_feed_items(feed_url,
                                         feed_type,
                                         feed_space,
                                         feed_country,
                                         feed_lang,
                                         encoding);

          Moderation::Transport::MsgAdjustmentContextPackImpl::Type*
            result = dynamic_cast<
            Moderation::Transport::MsgAdjustmentContextPackImpl::Type*>(
              res.in());
        
          if(result == 0)
          {
            throw Exception(
              "NewsGate::FeedModeration::Manager::py_get_feed_items: "
              "dynamic_cast<NewsGate::Moderation::Transport::"
              "MsgAdjustmentContextPackImpl::Type*> failed");            
          }

          contexts.reset(result->release());
        }      

        El::Python::Sequence_var items = new El::Python::Sequence();
        items->reserve(contexts->size());
          
        for(RSS::MsgAdjustment::ContextArray::const_iterator
              i(contexts->begin()), e(contexts->end()); i != e; ++i)
        {
          RSS::MsgAdjustment::Python::Context_var ctx =
            new RSS::MsgAdjustment::Python::Context(*i);
          
          items->push_back(ctx);
        }
        
        return items.retn();
      }
      catch(const NewsGate::Moderation::OperationFailed& e)
      {
        OperationFailed_var error = new OperationFailed();
        error->reason = e.reason.in();
        
        PyErr_SetObject(
          FeedModerationPyModule::instance.opration_failed_ex.in(),
          error.in());
          
        return 0;
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FeedModeration::Manager::py_get_feed_items: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FeedModeration::Manager::py_get_feed_items: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    }
    
    PyObject*
    Manager::py_get_html_items(PyObject* args) throw(El::Exception)
    {
      const char* feed_url = 0;
      const char* script = 0;
      unsigned long feed_type = 0;
      unsigned long feed_space = 0;
      unsigned long feed_country = 0;
      unsigned long feed_lang = 0;
      PyObject* keywords = 0;
      const char* cache = 0;
      const char* encoding = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "sskkkkOss:newsgate.moderation.feed.Manager.get_html_items",
           &feed_url,
           &script,
           &feed_type,
           &feed_space,
           &feed_country,
           &feed_lang,
           &keywords,
           &cache,
           &encoding))
      {
        El::Python::handle_error(
          "NewsGate::FeedModeration::Manager::py_get_html_items");
      }

      El::Python::Sequence* kwds =
        El::Python::Sequence::Type::down_cast(keywords);

      ::NewsGate::Moderation::KeywordsSeq keyword_seq;
      keyword_seq.length(kwds->size());

      size_t index = 0;
      
      for(El::Python::Sequence::const_iterator i(kwds->begin()),
            e(kwds->end()); i != e; ++i, ++index)
      {
        size_t slen = 0;
        
        std::string kwd =
          El::Python::string_from_string(
            i->in(),
            slen,
            "NewsGate::FeedModeration::Manager::py_get_html_items");

        keyword_seq[index] = kwd.c_str();
      }

      try
      {        
        std::auto_ptr< Moderation::Transport::GetHTMLItemsResultStruct >
          op_res;
        
        {
          El::Python::AllowOtherThreads guard;
        
          ::NewsGate::Moderation::FeedManager_var feed_manager =
              feed_manager_.object();
        
          Moderation::Transport::GetHTMLItemsResult_var res =
            feed_manager->get_html_items(feed_url,
                                         script,
                                         feed_type,
                                         feed_space,
                                         feed_country,
                                         feed_lang,
                                         keyword_seq,
                                         cache ? cache : "",
                                         encoding);

          Moderation::Transport::GetHTMLItemsResultImpl::Type*
            result = dynamic_cast<
              Moderation::Transport::GetHTMLItemsResultImpl::Type*>(res.in());
        
          if(result == 0)
          {
            throw Exception(
              "NewsGate::FeedModeration::Manager::py_get_html_items: "
              "dynamic_cast<NewsGate::Moderation::Transport::"
              "GetHTMLItemsResultImpl::Type*> failed");            
          }

          op_res.reset(result->release());
        }
        
        return new GetHTMLItemsResult(op_res->messages,
                                      op_res->log.c_str(),
                                      op_res->error.c_str(),
                                      op_res->cache.c_str(),
                                      op_res->interrupted);
      }
      catch(const NewsGate::Moderation::OperationFailed& e)
      {
        OperationFailed_var error = new OperationFailed();
        error->reason = e.reason.in();
        
        PyErr_SetObject(
          FeedModerationPyModule::instance.opration_failed_ex.in(),
          error.in());
          
        return 0;
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FeedModeration::Manager::py_get_html_items: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FeedModeration::Manager::py_get_html_items: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    }
    
    PyObject*
    Manager::py_xpath_url(PyObject* args) throw(El::Exception)
    {
      char* xpath = 0;
      char* url = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "ss:newsgate.moderation.feed.Manager.xpath_url",
           &xpath,
           &url))
      {
        El::Python::handle_error(
          "NewsGate::FeedModeration::Manager::py_xpath_url");
      }

      CORBA::String_var result;
      
      try
      {
        El::Python::AllowOtherThreads guard;
        
        ::NewsGate::Moderation::FeedManager_var feed_manager =
            feed_manager_.object();
        
        result = feed_manager->xpath_url(xpath, url);
      }
      catch(const NewsGate::Moderation::OperationFailed& e)
      {
        std::ostringstream ostr;
        ostr << "Error:\n" << e.reason.in();
        result = CORBA::string_dup(ostr.str().c_str());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FeedModeration::Manager::py_xpath_url: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FeedModeration::Manager::py_xpath_url: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }

      std::ostringstream dest;

      dest << "<pre class='result'>";
      
      El::String::Manip::xml_encode(result.in(),
                                    dest,
                                    El::String::Manip::XE_TEXT_ENCODING |
                                    El::String::Manip::XE_PRESERVE_UTF8);

      dest << "</pre>";      
      return PyString_FromString(dest.str().c_str());
    }
    
    PyObject*
    Manager::py_validate_feeds(PyObject* args)
      throw(El::Exception)
    {
      PyObject* feed_infos = 0;
      unsigned long processing_type = 0;
      unsigned long long creator_id = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "OkK:newsgate.moderation.feed.Manager.validate_feeds",
           &feed_infos,
           &processing_type,
           &creator_id))
      {
        El::Python::handle_error(
          "NewsGate::FeedModeration::Manager::py_validate_feeds");
      }

      if(!PySequence_Check(feed_infos))
      {
        El::Python::report_error(
          PyExc_TypeError,
          "1st argument expected to be of sequence type",
          "NewsGate::FeedModeration::Manager::"
          "py_validate_feeds");
      }

      if(processing_type >= Moderation::PT_COUNT)
      {
        El::Python::report_error(
          PyExc_TypeError,
          "2nd argument expected to be of ProcessingType type",
          "NewsGate::FeedModeration::Manager::py_validate_feeds");
      }
    
      int len = PySequence_Size(feed_infos);

      if(len < 0)
      {
        El::Python::handle_error(
          "NewsGate::FeedModeration::Manager::py_validate_feeds");      
      }

      Moderation::UrlSeq_var seq = new Moderation::UrlSeq();
      seq->length(len);
    
      for(CORBA::ULong i = 0; i < (CORBA::ULong)len; i++)
      {
        El::Python::Object_var item = PySequence_GetItem(feed_infos, i);
        UrlDesc* desc = UrlDesc::Type::down_cast(item.in());
        seq[i].url = desc->url.c_str();
      }
    
      try
      {
        Moderation::ResourceValidationResultSeq_var validation_results;
      
        {
          El::Python::AllowOtherThreads guard;
        
          ::NewsGate::Moderation::FeedManager_var feed_manager =
              feed_manager_.object();

          validation_results =
            feed_manager->validate(
              seq.in(),
              (Moderation::ProcessingType)processing_type,
              creator_id);
        }

        El::Python::Sequence_var result = new El::Python::Sequence();

        unsigned long len = validation_results->length();
        result->resize(len);

        for(unsigned long i = 0; i < len; i++)
        {
          (*result)[i] =
            new ResourceValidationResult((*validation_results)[i]);
        }

        return result.retn();
      }
      catch(const Moderation::NotReady& e)
      {
        std::ostringstream ostr;
        ostr << "System not ready. Reason:\n" << e.reason.in();
        
        El::Python::report_error(
          ModerationPyModule::instance.not_ready_ex.in(),
          ostr.str().c_str());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FeedModeration::Manager::py_validate_feeds: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FeedModeration::Manager::py_validate_feeds: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception("NewsGate::FeedModeration::Manager::py_validate_feeds: "
                      "unexpected execution path");
    }
  
    PyObject*
    Manager::py_validation_result(PyObject* args)
      throw(El::Exception)
    {
      char* task_id = 0;
      PyObject* creator_ids = 0;
      unsigned long all_results = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "sOk:newsgate.moderation.feed.Manager.validation_result",
           &task_id,
           &creator_ids,
           &all_results))
      {
        El::Python::handle_error(
          "NewsGate::FeedModeration::Manager::py_validation_result");
      }

      if(!PySequence_Check(creator_ids))
      {
        El::Python::report_error(
          PyExc_TypeError,
          "2nd argument expected to be of sequence type",
          "NewsGate::FeedModeration::Manager::py_validation_result");
      }

      Moderation::CreatorIdSeq_var seq =
        ModeratorConnector::get_creator_ids(creator_ids);

      try
      {
        CORBA::ULong total_results = 0;
        Moderation::ResourceValidationResultSeq_var validation_results;
      
        {
          El::Python::AllowOtherThreads guard;
        
          ::NewsGate::Moderation::FeedManager_var feed_manager =
              feed_manager_.object();
          
          validation_results =
            feed_manager->validation_result(task_id,
                                            seq.in(),
                                            all_results,
                                            total_results);
        }


        FeedValidationResult_var result = new FeedValidationResult();
        result->total_results = total_results;
        
        El::Python::Sequence_var results = result->results;

        unsigned long len = validation_results->length();
        results->resize(len);

        for(unsigned long i = 0; i < len; i++)
        {
          (*results)[i] =
            new ResourceValidationResult((*validation_results)[i]);
        }

        return result.retn();
      }
      catch(const Moderation::TaskNotFound&)
      {
        El::Python::report_error(
          FeedModerationPyModule::instance.task_not_found_ex.in(),
          "Validation task not exist");
      }
      catch(const Moderation::NotReady& e)
      {
        std::ostringstream ostr;
        ostr << "System not ready. Reason:\n" << e.reason.in();
        
        El::Python::report_error(
          ModerationPyModule::instance.not_ready_ex.in(),
          ostr.str().c_str());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FeedModeration::Manager::py_validation_result: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FeedModeration::Manager::py_validation_result: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception("NewsGate::FeedModeration::Manager::"
                      "py_validation_result: unexpected execution path");
    }
  
    PyObject*
    Manager::py_validate_feeds_async(PyObject* args)
      throw(El::Exception)
    {
      PyObject* feed_infos = 0;
      unsigned long processing_type = 0;
      unsigned long long creator_id = 0;
      unsigned long req_period = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "OkKk:newsgate.moderation.feed.Manager.validate_feeds_async",
           &feed_infos,
           &processing_type,
           &creator_id,
           &req_period))
      {
        El::Python::handle_error(
          "NewsGate::FeedModeration::Manager::py_validate_feeds_async");
      }

      if(!PySequence_Check(feed_infos))
      {
        El::Python::report_error(
          PyExc_TypeError,
          "1st argument expected to be of sequence type",
          "NewsGate::FeedModeration::Manager::py_validate_feeds_async");
      }

      if(processing_type >= Moderation::PT_COUNT)
      {
        El::Python::report_error(
          PyExc_TypeError,
          "2nd argument expected to be of ProcessingType type",
          "NewsGate::FeedModeration::Manager::py_validate_feeds_async");
      }
    
      int len = PySequence_Size(feed_infos);

      if(len < 0)
      {
        El::Python::handle_error(
          "NewsGate::FeedModeration::Manager::py_validate_feeds_async");
      }

      Moderation::UrlSeq_var seq = new Moderation::UrlSeq();
      seq->length(len);
    
      for(CORBA::ULong i = 0; i < (CORBA::ULong)len; i++)
      {
        El::Python::Object_var item = PySequence_GetItem(feed_infos, i);
        UrlDesc* desc = UrlDesc::Type::down_cast(item.in());
        seq[i].url = desc->url.c_str();
      }
    
      try
      {
        Moderation::ValidationToken_var validation_token;
      
        {
          El::Python::AllowOtherThreads guard;
        
          ::NewsGate::Moderation::FeedManager_var feed_manager =
              feed_manager_.object();

          validation_token =
            feed_manager->validate_async(
              seq.in(),
              (Moderation::ProcessingType)processing_type,
              creator_id,
              req_period);
        }
      
        return PyString_FromString(validation_token.in());
      }
      catch(const Moderation::NotReady& e)
      {
        std::ostringstream ostr;
        ostr << "System not ready. Reason:\n" << e.reason.in();
        
        El::Python::report_error(
          ModerationPyModule::instance.not_ready_ex.in(),
          ostr.str().c_str());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FeedModeration::Manager::"
          "py_validate_feeds_async: ImplementationException exception caught. "
          "Description:\n" << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FeedModeration::Manager::py_validate_feeds_async: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception("NewsGate::FeedModeration::Manager::"
                      "py_validate_feeds_async: unexpected execution path");
    }
  
    PyObject*
    Manager::py_add_feeds(PyObject* args)
      throw(El::Exception)
    {
      PyObject* feed_infos = 0;
      PyObject* feed_sources = 0;
      unsigned long creator_type = 0;
      unsigned long long creator_id = 0;
    
      if(!PyArg_ParseTuple(args,
                           "OOkK:newsgate.moderation.feed.Manager.add_feeds",
                           &feed_infos,
                           &feed_sources,
                           &creator_type,
                           &creator_id))
      {
        El::Python::handle_error(
          "NewsGate::FeedModeration::Manager::py_add_feeds");
      }

      if(!PySequence_Check(feed_infos) || !PySequence_Check(feed_sources))
      {
        El::Python::report_error(
          PyExc_TypeError,
          "1st and 2nd arguments expected to be of sequence type",
          "NewsGate::FeedModeration::Manager::py_add_feeds");
      }

      if(creator_type >= Moderation::CT_COUNT)
      {
        El::Python::report_error(
          PyExc_TypeError,
          "3rd argument expected to be of CreatorType type",
          "NewsGate::FeedModeration::Manager::py_add_feeds");
      }
    
      int len = PySequence_Size(feed_infos);

      if(len < 0)
      {
        El::Python::handle_error(
          "NewsGate::FeedModeration::Manager::py_add_feeds");      
      }

      Moderation::UrlSeq_var url_seq = new Moderation::UrlSeq();
      url_seq->length(len);
    
      for(CORBA::ULong i = 0; i < (CORBA::ULong)len; i++)
      {
        El::Python::Object_var item = PySequence_GetItem(feed_infos, i);
        UrlDesc* desc = UrlDesc::Type::down_cast(item.in());
        url_seq[i].url = desc->url.c_str();
      }
    
      len = PySequence_Size(feed_sources);

      if(len < 0)
      {
        El::Python::handle_error(
          "NewsGate::FeedModeration::Manager::py_add_feeds");      
      }

      Moderation::FeedSourceSeq_var sources_seq =
        new Moderation::FeedSourceSeq();
    
      sources_seq->length(len);
    
      for(CORBA::ULong i = 0; i < (CORBA::ULong)len; i++)
      {
        El::Python::Object_var item = PySequence_GetItem(feed_sources, i);
        FeedSource* src = FeedSource::Type::down_cast(item.in());

        if(src->type >= Moderation::FST_COUNT)
        {
          El::Python::report_error(
            PyExc_TypeError,
            "FeedSource.type should be of FeesSourceType",
            "NewsGate::FeedModeration::Manager::py_add_feeds");
        }
      
        if(src->processing_type >= Moderation::PT_COUNT)
        {
          El::Python::report_error(
            PyExc_TypeError,
            "FeedSource.processing_type should be of ProcessingType",
            "NewsGate::FeedModeration::Manager::py_add_feeds");
        }
      
        sources_seq[i].url = src->url.c_str();
        sources_seq[i].type = (Moderation::FeedSourceType)src->type;
      
        sources_seq[i].processing_type =
          (Moderation::ProcessingType)src->processing_type;
      }
    
      try
      {
        Moderation::ResourceValidationResultSeq_var validation_results;
      
        {
          El::Python::AllowOtherThreads guard;

          ::NewsGate::Moderation::FeedManager_var feed_manager =
              feed_manager_.object();
          
          validation_results =
            feed_manager->add_feeds(url_seq.in(),
                                    sources_seq.in(),
                                    (Moderation::CreatorType)creator_type,
                                    creator_id);
        }
      
        El::Python::Sequence_var result = new El::Python::Sequence();

        unsigned long len = validation_results->length();
        result->resize(len);

        for(unsigned long i = 0; i < len; i++)
        {
          (*result)[i] =
            new ResourceValidationResult((*validation_results)[i]);
        }

        return result.retn();
      }
      catch(const Moderation::NotReady& e)
      {
        std::ostringstream ostr;
        ostr << "System not ready. Reason:\n" << e.reason.in();
        
        El::Python::report_error(
          ModerationPyModule::instance.not_ready_ex.in(),
          ostr.str().c_str());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FeedModeration::Manager::py_add_feeds: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FeedModeration::Manager::py_add_feeds: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception("NewsGate::FeedModeration::Manager::py_add_feeds: "
                      "unexpected execution path");
    }
  
    PyObject*
    Manager::py_delete_validation(PyObject* args)
      throw(El::Exception)
    {
      char* task_id = 0;
      PyObject* creator_ids = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "sO:newsgate.moderation.feed.Manager.delete_validation",
           &task_id,
           &creator_ids))
      {
        El::Python::handle_error(
          "NewsGate::FeedModeration::Manager::py_delete_validation");
      }

      if(!PySequence_Check(creator_ids))
      {
        El::Python::report_error(
          PyExc_TypeError,
          "2nd argument expected to be of sequence type",
          "NewsGate::FeedModeration::Manager::py_delete_validation");
      }

      Moderation::CreatorIdSeq_var seq =
        ModeratorConnector::get_creator_ids(creator_ids);

      try
      {
        El::Python::AllowOtherThreads guard;

        ::NewsGate::Moderation::FeedManager_var feed_manager =
            feed_manager_.object();
        
        feed_manager->delete_validation(task_id, seq.in());
        
        return El::Python::add_ref(Py_None);
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FeedModeration::Manager::py_delete_validation: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FeedModeration::Manager::py_delete_validation: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception("NewsGate::FeedModeration::Manager::"
                      "py_delete_validation: unexpected execution path");
    }
  
    PyObject*
    Manager::py_stop_validation(PyObject* args)
      throw(El::Exception)
    {
      char* task_id = 0;
      PyObject* creator_ids = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "sO:newsgate.moderation.feed.Manager.stop_validation",
           &task_id,
           &creator_ids))
      {
        El::Python::handle_error(
          "NewsGate::FeedModeration::Manager::py_stop_validation");
      }

      if(!PySequence_Check(creator_ids))
      {
        El::Python::report_error(
          PyExc_TypeError,
          "2nd argument expected to be of sequence type",
          "NewsGate::FeedModeration::Manager::py_stop_validation");
      }

      Moderation::CreatorIdSeq_var seq =
        ModeratorConnector::get_creator_ids(creator_ids);

      try
      {
        El::Python::AllowOtherThreads guard;

        ::NewsGate::Moderation::FeedManager_var feed_manager =
            feed_manager_.object();
        
        feed_manager->stop_validation(task_id, seq.in());
        
        return El::Python::add_ref(Py_None);
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FeedModeration::Manager::py_stop_validation: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FeedModeration::Manager::py_stop_validation: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception("NewsGate::FeedModeration::Manager::py_stop_validation: "
                      "unexpected execution path");
    }
  
    PyObject*
    Manager::py_feed_update_info(PyObject* args)
      throw(El::Exception)
    {
      PyObject* feed_updates = 0;
    
      if(!PyArg_ParseTuple(args,
                           "O:newsgate.moderation.feed.Manager.feed_info_seq",
                           &feed_updates))
      {
        El::Python::handle_error(
          "NewsGate::FeedModeration::Manager::py_feed_update_info");
      }

      if(!PySequence_Check(feed_updates))
      {
        El::Python::report_error(PyExc_TypeError,
                                 "Argument expected to be of sequence type",
                                 "NewsGate::FeedModeration::Manager::"
                                 "py_feed_update_info");
      }

      int len = PySequence_Size(feed_updates);

      if(len < 0)
      {
        El::Python::handle_error(
          "NewsGate::FeedModeration::Manager::py_feed_update_info");
      }
      
      try
      {
        Moderation::FeedUpdateInfoSeq_var seq =
          new Moderation::FeedUpdateInfoSeq();
      
        seq->length(len);
      
        for(CORBA::ULong i = 0; i < (CORBA::ULong)len; i++)
        {
          El::Python::Object_var item = PySequence_GetItem(feed_updates, i);

          const FeedUpdateInfo* fu =
            FeedUpdateInfo::Type::down_cast(item.in());
        
          if(fu->space >= Feed::SP_SPACES_COUNT)
          {
            El::Python::report_error(
              PyExc_TypeError,
              "Invalid newsgate.moderation.FeedUpdateInfo.space value",
              "NewsGate::FeedModeration::Manager::py_feed_update_info");
          }

          bool valid = fu->lang <= El::Lang::languages_count() ||
            fu->lang == El::Lang::EC_NONEXISTENT;

          if(!valid)
          {
            El::Python::report_error(
              PyExc_TypeError,
              "Invalid newsgate.moderation.FeedUpdateInfo.lang value",
              "NewsGate::FeedModeration::Manager::py_feed_update_info");
          }
        
          valid = fu->country <= El::Country::countries_count() ||
            fu->country == El::Country::EC_NONEXISTENT;

          if(!valid)
          {
            El::Python::report_error(
              PyExc_TypeError,
              "Invalid newsgate.moderation.FeedUpdateInfo.country value",
              "NewsGate::FeedModeration::Manager::py_feed_update_info");
          }
        
          if(fu->status >= Feed::ST_STATUSES_COUNT)
          {
            El::Python::report_error(
              PyExc_TypeError,
              "Invalid newsgate.moderation.FeedUpdateInfo.status value",
              "NewsGate::FeedModeration::Manager::py_feed_update_info");
          }
        
          Moderation::FeedUpdateInfo& dest_fu = seq[i];

          dest_fu.id = fu->id;
          dest_fu.space = fu->space;
          dest_fu.lang = fu->lang;
          dest_fu.country = fu->country;
          dest_fu.encoding = fu->encoding.c_str();

          switch(fu->status)
          {
          case Feed::ST_ENABLED: dest_fu.status = 'E'; break;
          case Feed::ST_DISABLED: dest_fu.status = 'D'; break;
          case Feed::ST_PENDING: dest_fu.status = 'P'; break;
          case Feed::ST_DELETED: dest_fu.status = 'L'; break;
          }

          dest_fu.keywords = fu->keywords.c_str();
          dest_fu.adjustment_script = fu->adjustment_script.c_str();
          dest_fu.comment = fu->comment.c_str();
        }

        {
          El::Python::AllowOtherThreads guard;

          ::NewsGate::Moderation::FeedManager_var feed_manager =
              feed_manager_.object();
          
          feed_manager->feed_update_info(seq);
        }

        return El::Python::add_ref(Py_None);
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FeedModeration::Manager::"
          "py_feed_update_info: ImplementationException exception "
          "caught. Description:\n" << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FeedModeration::Manager::py_feed_update_info: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception("NewsGate::FeedModeration::Manager::"
                      "py_feed_update_info: unexpected execution path");    

    }

    PyObject*
    Manager::py_feed_info_seq(PyObject* args)
      throw(El::Exception)
    {
      PyObject* feed_ids = 0;
      unsigned char get_stat = false;
      PyObject* stat_from_date = 0;
      PyObject* stat_to_date = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "ObOO:newsgate.moderation.feed.Manager.feed_info_seq",
           &feed_ids,
           &get_stat,
           &stat_from_date,
           &stat_to_date))
      {
        El::Python::handle_error(
          "NewsGate::FeedModeration::Manager::py_feed_info_seq");
      }

      if(!PySequence_Check(feed_ids))
      {
        El::Python::report_error(
          PyExc_TypeError,
          "1st argument expected to be of sequence type",
          "NewsGate::FeedModeration::Manager::py_feed_info_seq");
      }

      if(!El::Python::Moment::Type::check_type(stat_from_date) ||
         !El::Python::Moment::Type::check_type(stat_to_date))
      {
        El::Python::report_error(
          PyExc_TypeError,
          "3rd and 4th arguments expected to be of el.Moment type",
          "NewsGate::FeedModeration::Manager::py_feed_info_seq");
      }

      int len = PySequence_Size(feed_ids);

      if(len < 0)
      {
        El::Python::handle_error(
          "NewsGate::FeedModeration::Manager::py_feed_info_seq");
      }
      
      try
      {
        Moderation::FeedIdSeq_var seq = new Moderation::FeedIdSeq();
        seq->length(len);
      
        for(CORBA::ULong i = 0; i < (CORBA::ULong)len; i++)
        {
          El::Python::Object_var item = PySequence_GetItem(feed_ids, i);
        
          seq[i] = El::Python::ulonglong_from_number(
            item.in(),
            "NewsGate::FeedModeration::Manager::py_feed_info_seq");
        }

        Moderation::FeedInfoSeq_var infos;
      
        {
          El::Python::AllowOtherThreads guard;
        
          ::NewsGate::Moderation::FeedManager_var feed_manager =
              feed_manager_.object();

          infos = feed_manager->feed_info_seq(
            seq,
            get_stat,
            ACE_Time_Value(
              *El::Python::Moment::Type::down_cast(stat_from_date)).sec(),
            ACE_Time_Value(
              *El::Python::Moment::Type::down_cast(stat_to_date)).sec());
        }

        El::Python::Sequence_var result = new El::Python::Sequence();
        result->reserve(infos->length());
      
        for(unsigned long i = 0; i < infos->length(); i++)
        {
          result->push_back(new FeedInfo(infos[i]));
        }

        return result.retn();
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FeedModeration::Manager::"
          "py_feed_info_seq: ImplementationException exception "
          "caught. Description:\n" << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FeedModeration::Manager::py_feed_info_seq: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception("NewsGate::FeedModeration::Manager::py_feed_info_seq: "
                      "unexpected execution path");
    }
  
    PyObject*
    Manager::py_feed_info_range(PyObject* args)
      throw(El::Exception)
    {
      unsigned long start_from = 0;
      unsigned long results = 0;
      unsigned char get_stat = false;
      PyObject* stat_from_date = 0;
      PyObject* stat_to_date = 0;
      PyObject* si = 0;
      PyObject* fi = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "kkbOO|OO:newsgate.moderation.feed.Manager.feed_info_range",
           &start_from,
           &results,
           &get_stat,
           &stat_from_date,
           &stat_to_date,
           &si,
           &fi))
      {
        El::Python::handle_error(
          "NewsGate::FeedModeration::Manager::py_feed_info_range");
      }

      if(!El::Python::Moment::Type::check_type(stat_from_date) ||
         !El::Python::Moment::Type::check_type(stat_to_date))
      {
        El::Python::report_error(
          PyExc_TypeError,
          "4th and 5th arguments expected to be of el.Moment type",
          "NewsGate::FeedModeration::Manager::py_feed_info_range");
      }

      if(si && !SortInfo::Type::check_type(si))
      {
        El::Python::report_error(
          PyExc_TypeError,
          "6th argument expected to be of newsgate.moderation.SortInfo type",
          "NewsGate::FeedModeration::Manager::py_feed_info_range");
      }

      SortInfo* sort_info = SortInfo::Type::down_cast(si);
    
      if(fi && !FilterInfo::Type::check_type(fi))
      {
        El::Python::report_error(
          PyExc_TypeError,
          "7th argument expected to be of newsgate.moderation.FilterInfo type",
          "NewsGate::FeedModeration::Manager::py_feed_info_range");
      }

      FilterInfo* filter_info = FilterInfo::Type::down_cast(fi);
    
      try
      {
        Moderation::SortInfo sort;
      
        if(sort_info)
        {
          if(sort_info->field >= Moderation::FS_COUNT)
          {
            El::Python::report_error(
              PyExc_TypeError,
              "Unexpected field value for sort argument",
              "NewsGate::FeedModeration::Manager::py_feed_info_range");
          }

          sort.field = (Moderation::FieldSelector)sort_info->field;
          sort.descending = sort_info->descending;
        }
        else
        {
          sort.field = Moderation::FS_NONE;
          sort.descending = false;
        }
    
        Moderation::FilterInfo filter;

        if(filter_info)
        {          
          unsigned long len = filter_info->rules->size();

//          filter.consider_deleted = filter_info->consider_deleted;

          if(!filter_info->consider_deleted)
          {
            ++len;
          }
          
          filter.rules.length(len);
          size_t i = 0;

          if(!filter_info->consider_deleted)
          {
            Moderation::FilterRule& dest_rule = filter.rules[i++];

            dest_rule.id = UINT32_MAX;
            dest_rule.field = Moderation::FS_STATUS;
            dest_rule.operation = Moderation::FO_NONE_OF;
            
            Moderation::ArgSeq& dest_rule_args = dest_rule.args;
            dest_rule_args.length(1);
            dest_rule_args[0] = (const char*)"L";
          }

          for(size_t j = 0; i < len; ++i, ++j)
          {
            PyObject* item = (*filter_info->rules)[j];
          
            if(!FilterRule::Type::check_type(item))
            {
              El::Python::report_error(
                PyExc_TypeError,
                "not FilterRule object in rules sequence of filter info "
                "(7th) argument",
                "NewsGate::FeedModeration::Manager::py_feed_info_range");
            }

            FilterRule* src_rule = FilterRule::Type::down_cast(item);
            Moderation::FilterRule& dest_rule = filter.rules[i];
          
            dest_rule.id = src_rule->id;

            if(src_rule->field >= Moderation::FS_COUNT ||
               src_rule->field == Moderation::FS_NONE)
            {
              El::Python::report_error(
                PyExc_TypeError,
                "invalid filter rule field specification in filter info "
                "(7th) argument",
                "NewsGate::FeedModeration::Manager::py_feed_info_range");
            }
             
            dest_rule.field = (Moderation::FieldSelector)src_rule->field;

            if(src_rule->operation >= Moderation::FO_COUNT)
            {
              El::Python::report_error(
                PyExc_TypeError,
                "invalid filter rule operation specification in filter info "
                "(7th) argument",
                "NewsGate::FeedModeration::Manager::py_feed_info_range");
            }
             
            dest_rule.operation =
              (Moderation::FilterOperation)src_rule->operation;

            El::Python::Sequence& src_rule_args = *src_rule->args.in();
            Moderation::ArgSeq& dest_rule_args = dest_rule.args;

            unsigned long args_count = src_rule_args.size();
            dest_rule_args.length(args_count);

            for(size_t i = 0; i < args_count; i++)
            {
              El::Python::Object_var str =
                El::Python::string_from_object(
                  src_rule_args[i].in(),
                  "NewsGate::FeedModeration::Manager::py_feed_info_range");

              size_t slen = 0;
              dest_rule_args[i] =
                El::Python::string_from_string(
                  str.in(),
                  slen,
                  "NewsGate::FeedModeration::Manager::py_feed_info_range");
            
            }
          }
        }
      
        Moderation::FeedInfoResult_var infos;
      
        try
        {
          El::Python::AllowOtherThreads guard;
        
          ::NewsGate::Moderation::FeedManager_var feed_manager =
              feed_manager_.object();

          infos = feed_manager->feed_info_range(
            start_from,
            results,
            get_stat,
            ACE_Time_Value(
              *El::Python::Moment::Type::down_cast(stat_from_date)).sec(),
            ACE_Time_Value(
              *El::Python::Moment::Type::down_cast(stat_to_date)).sec(),
            sort,
            filter);
        }
        catch(const ::NewsGate::Moderation::FilterRuleError& e)
        {
          FilterRuleError_var error = new FilterRuleError();
          error->id = e.id;
          error->description = e.description.in();
        
          PyErr_SetObject(
            FeedModerationPyModule::instance.filter_rule_error_ex.in(),
            error.in());
          
          return 0;
        }

        return new FeedInfoResult(infos.in());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FeedModeration::Manager::"
          "py_feed_info_range: ImplementationException exception "
          "caught. Description:\n" << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FeedModeration::Manager::py_feed_info_range: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception("NewsGate::FeedModeration::Manager::py_feed_info_range: "
                      "unexpected execution path");
    }
  
    PyObject*
    Manager::py_validation_task_infos(PyObject* args)
      throw(El::Exception)
    {
      PyObject* creator_ids = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "O:newsgate.moderation.feed.Manager.validation_task_infos",
           &creator_ids))
      {
        El::Python::handle_error(
          "NewsGate::FeedModeration::Manager::py_validation_task_infos");
      }

      if(!PySequence_Check(creator_ids))
      {
        El::Python::report_error(
          PyExc_TypeError,
          "1st argument expected to be of sequence type",
          "NewsGate::FeedModeration::Manager::py_validation_task_infos");
      }

      Moderation::CreatorIdSeq_var seq =
        ModeratorConnector::get_creator_ids(creator_ids);

      try
      {
        Moderation::ValidationTaskInfoSeq_var task_infos;
      
        {
          El::Python::AllowOtherThreads guard;

          ::NewsGate::Moderation::FeedManager_var feed_manager =
              feed_manager_.object();
          
          task_infos = feed_manager->task_infos(seq);
        }

        El::Python::Sequence_var result = new El::Python::Sequence();

        unsigned long len = task_infos->length();
        result->resize(len);

        for(unsigned long i = 0; i < len; i++)
        {
          (*result)[i] = new ValidationTaskInfo((*task_infos)[i]);
        }

        return result.retn();
      }
      catch(const Moderation::NotReady& e)
      {
        std::ostringstream ostr;
        ostr << "System not ready. Reason:\n" << e.reason.in();
        
        El::Python::report_error(
          ModerationPyModule::instance.not_ready_ex.in(),
          ostr.str().c_str());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FeedModeration::Manager::"
          "py_validation_task_infos: ImplementationException exception "
          "caught. Description:\n" << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FeedModeration::Manager::py_validation_task_infos: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception("NewsGate::FeedModeration::Manager::"
                      "py_validation_task_infos: unexpected execution path");
    }
  
    
    //
    // NewsGate::FeedModeration::UrlDesc class
    //
    UrlDesc::UrlDesc(PyTypeObject *type,
                     PyObject *args,
                     PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance)
    {
    }

    //
    // NewsGate::ResourceValidationResult class
    //
    ResourceValidationResult::ResourceValidationResult(
      PyTypeObject *type,
      PyObject *args,
      PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          type(Moderation::RT_UNKNOWN),
          processing_type(Moderation::PT_CHECK_FEEDS),
          result(Moderation::VR_VALID),
          feed_reference_count(0),
          feed_id(0)
    {
    }

    ResourceValidationResult::ResourceValidationResult(
      const Moderation::ResourceValidationResult& src)
      throw(Exception, El::Exception) :
        El::Python::ObjectImpl(&Type::instance),
        url(src.url.in()),
        type(src.type),
        processing_type(src.processing_type),
        result(src.result),
        feed_reference_count(src.feed_reference_count),
        description(src.description.in()),
        feed_id(src.feed_id)
    {
    }
   
    //
    // NewsGate::FeedSource class
    //
    FeedSource::FeedSource(
      PyTypeObject *type,
      PyObject *args,
      PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          type(Moderation::FST_SINGLE_FEED_HTML),
          processing_type(Moderation::PT_CHECK_FEEDS)
    {
    }
  
    //
    // NewsGate::ValidationTaskInfo class
    //
    ValidationTaskInfo::ValidationTaskInfo(
      PyTypeObject *type,
      PyObject *args,
      PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          creator_id(0),
          status(Moderation::VS_ACTIVE),
          started(0),
          feeds(0),
          pending_urls(0),
          received_bytes(0),
          processed_urls(0)
    {
    }
  
    ValidationTaskInfo::ValidationTaskInfo(
      const Moderation::ValidationTaskInfo& src)
      throw(El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          id(src.id.in()),
          title(src.title.in()),
          creator_id(src.creator_id),
          status(src.status),
          started(src.started),
          feeds(src.feeds),
          pending_urls(src.pending_urls),
          received_bytes(src.received_bytes),
          processed_urls(src.processed_urls)
    {
    }

    //
    // NewsGate::SortInfo class
    //
    SortInfo::SortInfo(PyTypeObject *type,
                       PyObject *args,
                       PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          field(Moderation::FS_NONE),
          descending(false)
    {
      unsigned long fld = Moderation::FS_NONE;
      unsigned char desc = false;
    
      if(!PyArg_ParseTuple(args,
                           "|kb:newsgate.moderation.feed.SortInfo.SortInfo",
                           &fld,
                           &desc))
      {
        El::Python::handle_error(
          "NewsGate::FeedModeration::SortInfo::SortInfo");
      }

      field = fld;
      descending = desc;
    }  

    //
    // NewsGate::FilterRule class
    //
    FilterRule::FilterRule(PyTypeObject *type,
                           PyObject *args,
                           PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          field(Moderation::FS_NONE),
          operation(0),
          args(new El::Python::Sequence())
    {
    }
  
    //
    // NewsGate::FilterInfo class
    //
    FilterInfo::FilterInfo(PyTypeObject *type,
                           PyObject *args,
                           PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          consider_deleted(false),
          rules(new El::Python::Sequence())
    {
    }
  
    //
    // NewsGate::FilterRuleError class
    //
    FilterRuleError::FilterRuleError(PyTypeObject *type,
                                     PyObject *args,
                                     PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0)
    {
    }
  
    //
    // NewsGate::OperationFailed class
    //
    OperationFailed::OperationFailed(PyTypeObject *type,
                                     PyObject *args,
                                     PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance)
    {
    }
  
    //
    // NewsGate::FeedValidationResult class
    //
    FeedValidationResult::FeedValidationResult(PyTypeObject *type,
                                               PyObject *args,
                                               PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          total_results(0),
          results(new El::Python::Sequence())
    {
    }

    //
    // NewsGate::FeedInfoResult class
    //
    FeedInfoResult::FeedInfoResult(PyTypeObject *type,
                                   PyObject *args,
                                   PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::Sequence(type ? type : &Type::instance, args, kwds),
          feed_count(0)
    {
    }

    FeedInfoResult::FeedInfoResult(
      const Moderation::FeedInfoResult& src)
      throw(El::Exception)
        : El::Python::Sequence(&Type::instance, 0, 0),
          feed_count(src.feed_count)
    {
      unsigned long len = src.feed_infos.length();
      resize(len);
    
      for(unsigned long i = 0; i < len; i++)
      {
        (*this)[i] = new FeedInfo(src.feed_infos[i]);
      }
    }
  
    //
    // NewsGate::FeedInfo class
    //
    FeedInfo::FeedInfo(PyTypeObject *type,
                       PyObject *args,
                       PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          type(Feed::TP_UNDEFINED),
          space(Feed::SP_UNDEFINED),
          lang(new El::Python::Lang(El::Lang::EC_NONEXISTENT)),
          country(new El::Python::Country(El::Country::EC_NONEXISTENT)),
          status(Feed::ST_DISABLED),
          creator(0),
          creator_type(Moderation::CT_MODERATOR),
          created(new El::Python::Moment()),
          updated(new El::Python::Moment()),        
          channel_lang(new El::Python::Lang()),
          channel_country(new El::Python::Country()),
          channel_ttl(-1),
          channel_last_build_date(new El::Python::Moment()),
          last_request_date(new El::Python::Moment()),
          content_length_hdr(0),
          entropy(0),
          entropy_updated_date(new El::Python::Moment()),
          size(0),
          single_chunked(-1),
          first_chunk_size(-1),
          heuristics_counter(-1),
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
          msg_impressions(0),
          msg_clicks(0),
          msg_ctr(0)
    {
    }
  
    FeedInfo::FeedInfo(const Moderation::FeedInfo& src)
      throw(El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          id(src.id),
          type(src.type),
          space(src.space),
          lang(new El::Python::Lang((El::Lang::ElCode)src.lang)),
          country(new El::Python::Country((El::Country::ElCode)src.country)),
          status(Feed::ST_DISABLED),
          url(src.url.in()),
          encoding(src.encoding.in()),
          keywords(src.keywords.in()),
          adjustment_script(src.adjustment_script.in()),
          creator(src.creator),
          creator_type(Moderation::CT_MODERATOR),
          created(new El::Python::Moment(ACE_Time_Value(src.created))),
          updated(new El::Python::Moment(ACE_Time_Value(src.updated))),
          comment(src.comment.in()),
          channel_title(src.channel_title.in()),
          channel_description(src.channel_description.in()),
          channel_html_link(src.channel_html_link.in()),

          channel_lang(
            new El::Python::Lang(
              El::Lang((El::Lang::ElCode)src.channel_lang))),

          channel_country(
            new El::Python::Country(
              El::Country((El::Country::ElCode)src.channel_country))),

          channel_ttl(src.channel_ttl),

          channel_last_build_date(
            new El::Python::Moment(
              ACE_Time_Value(src.channel_last_build_date))),
          
          last_request_date(
            new El::Python::Moment(ACE_Time_Value(src.last_request_date))),
        
          last_modified_hdr(src.last_modified_hdr.in()),
          etag_hdr(src.etag_hdr.in()),
          content_length_hdr(src.content_length_hdr),
          entropy(src.entropy),
  
          entropy_updated_date(
            new El::Python::Moment(ACE_Time_Value(src.entropy_updated_date))),
  
          size(src.size),
          single_chunked(src.single_chunked),
          first_chunk_size(src.first_chunk_size),        
          heuristics_counter(src.heuristics_counter),
          requests(src.requests),
          failed(src.failed),
          unchanged(src.unchanged),
          not_modified(src.not_modified),
          presumably_unchanged(src.presumably_unchanged),
          has_changes(src.has_changes),
          wasted(src.wasted),
          outbound(src.outbound),
          inbound(src.inbound),
          requests_duration(src.requests_duration),
          messages(src.messages),
          messages_size(src.messages_size),
          messages_delay(src.messages_delay),
          max_message_delay(src.max_message_delay),
          msg_impressions(src.msg_impressions),
          msg_clicks(src.msg_clicks),
          msg_ctr(src.msg_ctr)
    {
      switch(src.status)
      {
      case 'E': status = Feed::ST_ENABLED; break;
      case 'D': status = Feed::ST_DISABLED; break;
      case 'P': status = Feed::ST_PENDING; break;
      case 'L': status = Feed::ST_DELETED; break;
      }

      switch(src.creator_type)
      {
      case 'C': creator_type = Moderation::CT_CRAWLER; break;
      case 'M': creator_type = Moderation::CT_MODERATOR; break;
      case 'U': creator_type = Moderation::CT_USER; break;
      case 'A': creator_type = Moderation::CT_ADMIN; break;
      }
    }

    //
    // NewsGate::FeedUpdateInfo class
    //
    FeedUpdateInfo::FeedUpdateInfo(PyTypeObject *type,
                                   PyObject *args,
                                   PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          space(Feed::SP_UNDEFINED),
          lang(El::Lang::EC_NONEXISTENT),
          country(El::Country::EC_NONEXISTENT),
          status(Feed::ST_DISABLED)
    {
    }  
    
  }
}
