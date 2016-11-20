/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Frontends/Moderation/MessageModeration.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_FRONTENDS_MODERATION_MESSAGEMODERATION_HPP_
#define _NEWSGATE_SERVER_FRONTENDS_MODERATION_MESSAGEMODERATION_HPP_

#include <string>

#include <El/Exception.hpp>

#include <El/Python/Exception.hpp>
#include <El/Python/Object.hpp>
#include <El/Python/RefCount.hpp>
#include <El/Python/Logger.hpp>

#include <Services/Moderator/Commons/FeedManager.hpp>

#include "FeedModeration.hpp"

namespace NewsGate
{  
  namespace MessageModeration
  {
    EL_EXCEPTION(Exception, El::ExceptionBase);
    
    class Manager : public El::Python::ObjectImpl
    {
    public:
      Manager(PyTypeObject *type, PyObject *args, PyObject *kwds)
        throw(Exception, El::Exception);
    
      Manager(const FeedModeration::ManagerRef& feed_manager)
        throw(Exception, El::Exception);

      virtual ~Manager() throw() {}

      PyObject* py_set_message_fetch_filter(PyObject* args)
        throw(El::Exception);
      
      PyObject* py_get_message_fetch_filter() throw(El::Exception);
      PyObject* py_add_message_filter(PyObject* args) throw(El::Exception);

      class Type : public El::Python::ObjectTypeImpl<Manager, Manager::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_METHOD_VARARGS(py_set_message_fetch_filter,
                               "set_message_fetch_filter",
                               "Set message fetch filter");

        PY_TYPE_METHOD_NOARGS(py_get_message_fetch_filter,
                              "get_message_fetch_filter",
                              "Get message fetch filter");

        
        PY_TYPE_METHOD_VARARGS(py_add_message_filter,
                               "add_message_filter",
                               "Add ids to message filter");
      };

    private:
      FeedModeration::ManagerRef feed_manager_;
    };

    typedef El::Python::SmartPtr<Manager> Manager_var;

    class MessageFetchFilterRule : public El::Python::ObjectImpl
    {
    public:
      unsigned long id;
      std::string expression;
      std::string description;
    
    public:
      MessageFetchFilterRule(PyTypeObject *type = 0,
                             PyObject *args = 0,
                             PyObject *kwds = 0)
        throw(Exception, El::Exception);

      virtual ~MessageFetchFilterRule() throw() {}

      class Type :
        public El::Python::ObjectTypeImpl<MessageFetchFilterRule,
                                          MessageFetchFilterRule::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;
        
        PY_TYPE_MEMBER_ULONG(id, "id", "Rule id");

        PY_TYPE_MEMBER_STRING(expression,
                              "expression",
                              "Rule expression",
                              true);

        PY_TYPE_MEMBER_STRING(description,
                              "description",
                              "Rule description",
                              true);
      };
    };

    typedef El::Python::SmartPtr<MessageFetchFilterRule>
    MessageFetchFilterRule_var;    

    class MessageFetchFilterRuleError : public El::Python::ObjectImpl
    {
    public:
      unsigned long id;
      unsigned long position;
      std::string description;
    
    public:
      MessageFetchFilterRuleError(PyTypeObject *type = 0,
                                  PyObject *args = 0,
                                  PyObject *kwds = 0)
        throw(Exception, El::Exception);

      virtual ~MessageFetchFilterRuleError() throw() {}

      class Type :
        public El::Python::ObjectTypeImpl<MessageFetchFilterRuleError,
                                          MessageFetchFilterRuleError::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;
        
        PY_TYPE_MEMBER_ULONG(id, "id", "Rule id");

        PY_TYPE_MEMBER_ULONG(position,
                             "position",
                             "Expression error position");

        PY_TYPE_MEMBER_STRING(description,
                              "description",
                              "Rule error description",
                              true);
      };
    };

    typedef El::Python::SmartPtr<MessageFetchFilterRuleError>
    MessageFetchFilterRuleError_var;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{  
  namespace MessageModeration
  {
    //
    // NewsGate::MessageModeration::Manager::Type class
    //
    inline
    Manager::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<Manager, Manager::Type>(
          "newsgate.moderation.message.Manager",
          "Object representing message management functionality")
    {
      tp_new = 0;
    }

    //
    // NewsGate::MessageModeration::MessageFetchFilterRule::Type class
    //
    inline
    MessageFetchFilterRule::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<
      MessageFetchFilterRule,
      MessageFetchFilterRule::Type>(
        "newsgate.moderation.message.MessageFetchFilterRule",
        "Object representing message fetch filtering rule")
    {
    }
  
    //
    // NewsGate::MessageModeration::MessageFetchFilterRuleError::Type class
    //
    inline
    MessageFetchFilterRuleError::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<MessageFetchFilterRuleError,
                                     MessageFetchFilterRuleError::Type>(
                                       "newsgate.moderation.message.MessageFetchFilterRuleError",
                                       "Object representing message fetch filtering rule error")
    {
    }
  }
}

#endif // _NEWSGATE_SERVER_FRONTENDS_MODERATION_MESSAGEMODERATION_HPP_
