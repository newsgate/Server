/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Frontends/Moderation/MessageModeration.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <Python.h>
#include <El/CORBA/Corba.hpp>

#include <string>
#include <sstream>

#include <El/Exception.hpp>

#include <El/Python/Object.hpp>
#include <El/Python/Module.hpp>
#include <El/Python/Utility.hpp>
#include <El/Python/RefCount.hpp>
#include <El/Python/Sequence.hpp>

#include <Commons/Search/SearchExpression.hpp>

#include <Services/Moderator/Commons/FeedManager.hpp>

#include "MessageModeration.hpp"

namespace
{
//  static char ASPECT[] = "MessageModeration";
  static const unsigned long MAX_MSG_FETCH_FILTER_EXPR_LEN = 0x1000000 - 1;
}

namespace NewsGate
{
  namespace MessageModeration
  {
    Manager::Type Manager::Type::instance;
    MessageFetchFilterRule::Type
    MessageFetchFilterRule::Type::instance;
    MessageFetchFilterRuleError::Type
    MessageFetchFilterRuleError::Type::instance;
    
    //
    // NewsGate::MessageModeration::MessageModerationPyModule class
    //
    
    class MessageModerationPyModule :
      public El::Python::ModuleImpl<MessageModerationPyModule>
    {
    public:
      static MessageModerationPyModule instance;

      MessageModerationPyModule() throw(El::Exception);
      
      virtual void initialized() throw(El::Exception);

    El::Python::Object_var message_fetch_filter_rule_error_ex;
    };
  
    MessageModerationPyModule MessageModerationPyModule::instance;
  
    MessageModerationPyModule::MessageModerationPyModule()
      throw(El::Exception)
        : El::Python::ModuleImpl<MessageModerationPyModule>(
        "newsgate.moderation.message",
        "Module containing Message Moderation types.",
        true)
    {
    }

    void
    MessageModerationPyModule::initialized() throw(El::Exception)
    {
      message_fetch_filter_rule_error_ex =
        create_exception("MessageFetchFilterRuleError");
    }
    
    //
    // NewsGate::MessageModeration::Manager class
    //
    
    Manager::Manager(PyTypeObject *type, PyObject *args, PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type)
    {
      throw Exception(
        "NewsGate::MessageModeration::Manager::Manager: unforseen way "
        "of object creation");
    }
    
    Manager::Manager(const FeedModeration::ManagerRef& manager)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          feed_manager_(manager)
    {
    }    

    PyObject*
    Manager::py_set_message_fetch_filter(PyObject* args)
      throw(El::Exception)
    {
      PyObject* rules = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "O:newsgate.moderation.message.Manager.set_message_fetch_filter",
           &rules))
      {
        El::Python::handle_error(
          "NewsGate::MessageModeration::Manager::"
          "py_set_message_fetch_filter");
      }

      if(!PySequence_Check(rules))
      {
        El::Python::report_error(PyExc_TypeError,
                                 "argument expected to be of sequence type",
                                 "NewsGate::MessageModeration::Manager::"
                                 "py_set_message_fetch_filter");
      }

      int len = PySequence_Size(rules);

      if(len < 0)
      {
        El::Python::handle_error("NewsGate::MessageModeration::Manager::"
                                 "py_set_message_fetch_filter");
      }

      Moderation::MessageFetchFilterRuleSeq_var seq =
        new Moderation::MessageFetchFilterRuleSeq();

      seq->length(len);
      
      for(CORBA::ULong i = 0; i < (CORBA::ULong)len; i++)
      {
        El::Python::Object_var item = PySequence_GetItem(rules, i);

        MessageFetchFilterRule_var rule =
          MessageFetchFilterRule::Type::down_cast(item.in(), true);

        std::wstring query;
        El::String::Manip::utf8_to_wchar(rule->expression.c_str(), query);
        
        if(query.length() > MAX_MSG_FETCH_FILTER_EXPR_LEN)
        {
          MessageFetchFilterRuleError_var error =
            new MessageFetchFilterRuleError();
        
          error->id = rule->id;
          error->position = MAX_MSG_FETCH_FILTER_EXPR_LEN;

          std::ostringstream ostr;
          ostr << "max expression lenght (" << MAX_MSG_FETCH_FILTER_EXPR_LEN
               << ") exceeded: " << query.length() << " chars";
          
          error->description = ostr.str();
        
          PyErr_SetObject(
            MessageModerationPyModule::instance.
            message_fetch_filter_rule_error_ex.in(),
            error.in());
        
          return 0;        
        }

        std::string rule_expr;
        El::String::Manip::suppress(rule->expression.c_str(), rule_expr, "\r");
        El::String::Manip::utf8_to_wchar(rule_expr.c_str(), query);
      
        Search::ExpressionParser parser;
        std::wistringstream istr(query);

        try
        {
          parser.parse(istr);
        }
        catch(const Search::ParseError& e)
        {
          MessageFetchFilterRuleError_var error =
            new MessageFetchFilterRuleError();
        
          error->id = rule->id;
          error->position = e.position;

          const char* desc = strstr(e.what(), ": ");
          error->description = desc ? desc + 2 : e.what();
        
          PyErr_SetObject(
            MessageModerationPyModule::instance.
            message_fetch_filter_rule_error_ex.in(),
            error.in());
        
          return 0;
        }

        Moderation::MessageFetchFilterRule& r = seq[i];
      
        r.expression = rule->expression.c_str();
        r.description = rule->description.c_str();
      }

      try
      {
        El::Python::AllowOtherThreads guard;

        ::NewsGate::Moderation::FeedManager_var feed_manager =
            feed_manager_.object();
        
        feed_manager->set_message_fetch_filter(seq.in());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::ModeratorConnector::py_set_message_fetch_filter: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::ModeratorConnector::py_set_message_fetch_filter: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }

      return El::Python::add_ref(Py_None);
    }

    PyObject*
    Manager::py_add_message_filter(PyObject* args)
      throw(El::Exception)
    {
      PyObject* ids = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "O:newsgate.moderation.message.Manager.add_message_filter",
           &ids))
      {
        El::Python::handle_error(
          "NewsGate::MessageModeration::Manager::py_add_message_filter");
      }

      if(!PySequence_Check(ids))
      {
        El::Python::report_error(PyExc_TypeError,
                                 "argument expected to be of sequence type",
                                 "NewsGate::MessageModeration::Manager::"
                                 "py_add_message_filter");
      }

      int len = PySequence_Size(ids);

      if(len < 0)
      {
        El::Python::handle_error("NewsGate::MessageModeration::Manager::"
                                 "py_add_message_filter");
      }

      Moderation::MessageIdSeq_var seq = new Moderation::MessageIdSeq();

      seq->length(len);
      
      for(CORBA::ULong i = 0; i < (CORBA::ULong)len; i++)
      {
        El::Python::Object_var item = PySequence_GetItem(ids, i);

        size_t slen = 0;
        const char* encoded_id =
          El::Python::string_from_string(
            item.in(),
            slen,
            "NewsGate::MessageModeration::Manager::"
            "py_add_message_filter");      

        unsigned long long id = 0;
      
        El::String::Manip::base64_decode(encoded_id,
                                         (unsigned char*)&id,
                                         sizeof(id));      

        seq[i] = id;
      }

      try
      {
        El::Python::AllowOtherThreads guard;

        ::NewsGate::Moderation::FeedManager_var feed_manager =
            feed_manager_.object();
        
        feed_manager->add_message_filter(seq.in());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::ModeratorConnector::py_add_message_filter: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::ModeratorConnector::py_add_message_filter: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }

      return El::Python::add_ref(Py_None);
    }

    PyObject*
    Manager::py_get_message_fetch_filter()
      throw(El::Exception)
    {
      Moderation::MessageFetchFilterRuleSeq_var rules;

      try
      {
        El::Python::AllowOtherThreads guard;

        ::NewsGate::Moderation::FeedManager_var feed_manager =
            feed_manager_.object();
            
        rules = feed_manager->get_message_fetch_filter();
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::ModeratorConnector::py_get_message_fetch_filter: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::ModeratorConnector::py_get_message_fetch_filter: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }

      El::Python::Sequence_var result = new El::Python::Sequence();
      result->resize(rules->length());

      for(unsigned long i = 0; i < rules->length(); i++)
      {
        const Moderation::MessageFetchFilterRule& rule = rules[i];
        
        MessageFetchFilterRule_var r = new MessageFetchFilterRule();

        r->id = i;
        r->expression = rule.expression.in();
        r->description = rule.description.in();
      
        (*result)[i] = r.retn();
      }

      return result.retn();
    }
    
    //
    // NewsGate::MessageModeration::MessageFetchFilterRule class
    //
    MessageFetchFilterRule::MessageFetchFilterRule(
      PyTypeObject *type,
      PyObject *args,
      PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0)
    {
    }  
  
    //
    // NewsGate::MessageModeration::MessageFetchFilterRuleError class
    //
    MessageFetchFilterRuleError::MessageFetchFilterRuleError(
      PyTypeObject *type,
      PyObject *args,
      PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          position(0)
    {
    }
    
  }
}
