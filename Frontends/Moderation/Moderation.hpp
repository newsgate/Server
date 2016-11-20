/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Frontends/Moderation/Moderation.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_FRONTENDS_MODERATION_MODERATION_HPP_
#define _NEWSGATE_SERVER_FRONTENDS_MODERATION_MODERATION_HPP_

#include <string>

#include <El/Exception.hpp>

#include <El/Python/Exception.hpp>
#include <El/Python/Object.hpp>
#include <El/Python/RefCount.hpp>
#include <El/Python/Logger.hpp>

#include <El/PSP/Config.hpp>

#include <Services/Moderator/Commons/ModeratorManager.hpp>
#include <Services/Moderator/Commons/FeedManager.hpp>

#include "CategoryModeration.hpp"
#include "MessageModeration.hpp"
#include "FeedModeration.hpp"
#include "ModeratorModeration.hpp"
#include "CustomerModeration.hpp"
#include "AdModeration.hpp"

namespace NewsGate
{  
  class Moderator : public El::Python::ObjectImpl
  {
  public:
    EL_EXCEPTION(Exception, El::ExceptionBase);

  public:
    std::string session_id;
    Moderation::ModeratorId id;
    Moderation::ModeratorId advertiser_id;
    std::string name;
    std::string advertiser_name;
    std::string email;
    std::string updated;
    std::string created;
    std::string creator;
    std::string superior;
    std::string comment;
    unsigned long status;
    bool show_deleted;
    El::Python::Sequence_var privileges;
    
    CategoryModeration::Manager_var category_manager;
    MessageModeration::Manager_var message_manager;
    FeedModeration::Manager_var feed_manager;
    ModeratorModeration::Manager_var moderator_manager;
    CustomerModeration::Manager_var customer_manager;
    AdModeration::Manager_var ad_manager;
    
  public:
    Moderator(PyTypeObject *type = 0, PyObject *args = 0, PyObject *kwds = 0)
      throw(Exception, El::Exception);
    
    Moderator(const Moderation::ModeratorInfo& moderator_info,
              const char* session,
              bool customer_moderating,
              Moderation::ModeratorManager* moderator_manager_ref,
              CategoryModeration::Manager* category_manager_val,
              MessageModeration::Manager* message_manager_val,
              FeedModeration::Manager* feed_manager_val,
              ModeratorModeration::Manager* moderator_manager_val,
              CustomerModeration::Manager* customer_manager_val,
              AdModeration::Manager* ad_manager_val)
      throw(Exception, El::Exception);

    virtual ~Moderator() throw() {}

    PyObject* py_has_privilege(PyObject* args) throw(El::Exception);
    PyObject* py_is_customer() throw(El::Exception);

    class Type : public El::Python::ObjectTypeImpl<Moderator,
                                                   Moderator::Type>
    {
    public:
      Type() throw(El::Python::Exception, El::Exception);
      static Type instance;

      PY_TYPE_MEMBER_STRING(session_id, "session_id", "Session id", false);
      PY_TYPE_MEMBER_ULONGLONG(id, "id", "Moderator identifier");
      
      PY_TYPE_MEMBER_ULONGLONG(advertiser_id,
                               "advertiser_id",
                               "Moderator advertiser identifier");
      
      PY_TYPE_MEMBER_STRING(name, "name", "Account name", false);
      
      PY_TYPE_MEMBER_STRING(advertiser_name,
                            "advertiser_name",
                            "Advertiser name",
                            true);
      
      PY_TYPE_MEMBER_STRING(email, "email", "Email", false);
        
      PY_TYPE_MEMBER_STRING(updated,
                            "updated",
                            "Time of account last update",
                            false);
        
      PY_TYPE_MEMBER_STRING(created,
                            "created",
                            "Time of account creation",
                            false);
        
      PY_TYPE_MEMBER_STRING(creator,
                            "creator",
                            "Creator moderator",
                            false);
        
      PY_TYPE_MEMBER_STRING(superior,
                            "superior",
                            "Superior moderator",
                            false);
        
      PY_TYPE_MEMBER_STRING(comment, "comment", "Comment", true);

      PY_TYPE_MEMBER_ULONG(status, "status", "Moderator status");

      PY_TYPE_MEMBER_BOOL(show_deleted,
                          "show_deleted",
                          "Moderator show_deleted flag");

      PY_TYPE_MEMBER_OBJECT(privileges,
                            El::Python::Sequence::Type,
                            "privileges",
                            "Moderator priviliges",
                            false);

      PY_TYPE_MEMBER_OBJECT(category_manager,
                            CategoryModeration::Manager::Type,
                            "category_manager",
                            "Category manager",
                            false);

      PY_TYPE_MEMBER_OBJECT(message_manager,
                            MessageModeration::Manager::Type,
                            "message_manager",
                            "Message manager",
                            false);

      PY_TYPE_MEMBER_OBJECT(feed_manager,
                            FeedModeration::Manager::Type,
                            "feed_manager",
                            "Feed manager",
                            false);

      PY_TYPE_MEMBER_OBJECT(moderator_manager,
                            ModeratorModeration::Manager::Type,
                            "moderator_manager",
                            "Moderator manager",
                            false);

      PY_TYPE_MEMBER_OBJECT(customer_manager,
                            CustomerModeration::Manager::Type,
                            "customer_manager",
                            "Customer manager",
                            false);

      PY_TYPE_MEMBER_OBJECT(ad_manager,
                            AdModeration::Manager::Type,
                            "ad_manager",
                            "Ad manager",
                            false);

      PY_TYPE_METHOD_VARARGS(py_has_privilege,
                             "has_privilege",
                             "Checks for presence of specific privilege");

      PY_TYPE_METHOD_NOARGS(py_is_customer,
                            "is_customer",
                            "Checks if account represents a customer");
    };
      
  private:
    ::NewsGate::Moderation::ModeratorManager_var moderator_manager_ref_;
    bool customer_moderating_;
  };

  typedef El::Python::SmartPtr<Moderator> Moderator_var;

  class ModeratorConnector : public El::Python::ObjectImpl
  {
  public:
    EL_EXCEPTION(Exception, El::ExceptionBase);
    
  public:
    ModeratorConnector(PyTypeObject *type, PyObject *args, PyObject *kwds)
      throw(Exception, El::Exception);
    
    ModeratorConnector(PyObject* args) throw(Exception, El::Exception);
      
    static Moderation::CreatorIdSeq* get_creator_ids(PyObject* creator_ids)
      throw(Exception, El::Exception);

    virtual ~ModeratorConnector() throw();

    void cleanup() throw();

    Moderator*
    connect_moderator(const Moderation::ModeratorInfo& moderator_info,
                      const char* session) const throw(El::Exception);
    
    PyObject* py_connect(PyObject* args) throw(El::Exception);      
    PyObject* py_logout(PyObject* args) throw(El::Exception);      
      
    class Type : public El::Python::ObjectTypeImpl<ModeratorConnector,
                                                   ModeratorConnector::Type>
    {
    public:
      Type() throw(El::Python::Exception, El::Exception);
      static Type instance;

      PY_TYPE_METHOD_VARARGS(py_connect,
                             "connect",
                             "Connects moderator to data source");
      
      PY_TYPE_METHOD_VARARGS(py_logout,
                             "logout",
                             "Logout moderator");
    };

  private:
    El::PSP::Config_var config_;
    El::Logging::Python::Logger_var logger_;
    El::Corba::OrbAdapter* orb_adapter_;
    CORBA::ORB_var orb_;

    ModeratorModeration::ManagerRef moderator_manager_ref_;
    
    CategoryModeration::Manager_var category_manager_;
    MessageModeration::Manager_var message_manager_;
    FeedModeration::Manager_var feed_manager_;
    ModeratorModeration::Manager_var moderator_manager_;
    CustomerModeration::Manager_var customer_manager_;
    AdModeration::Manager_var ad_manager_;

    typedef ACE_RW_Thread_Mutex Mutex;
    typedef ACE_Read_Guard<Mutex> ReadGuard;
    typedef ACE_Write_Guard<Mutex> WriteGuard;

    Mutex lock_;    
  };

  class ModerationPyModule : public El::Python::ModuleImpl<ModerationPyModule>
  {
  public:
    static ModerationPyModule instance;

    ModerationPyModule() throw(El::Exception);

    virtual void initialized() throw(El::Exception);
    
    PyObject* py_create_moderator_connector(PyObject* args)
      throw(El::Exception);

    PyObject* py_cleanup_moderator_connector(PyObject* args)
      throw(El::Exception);
    
    PY_MODULE_METHOD_VARARGS(
      py_create_moderator_connector,
      "create_moderator_connector",
      "Creates ModeratorConnector object");

    PY_MODULE_METHOD_VARARGS(
      py_cleanup_moderator_connector,
      "cleanup_moderator_connector",
      "Cleanups ModeratorConnector object");    

    El::Python::Object_var invalid_session_ex;
    El::Python::Object_var not_ready_ex;
    El::Python::Object_var invalid_user_name_ex;
    El::Python::Object_var account_disabled_ex;
  };
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  //
  // NewsGate::ModeratorConnector::Type class
  //
  inline
  ModeratorConnector::Type::Type() throw(El::Python::Exception, El::Exception)
      : El::Python::ObjectTypeImpl<ModeratorConnector,
                                   ModeratorConnector::Type>(
        "newsgate.moderation.ModeratorConnector",
        "Object intended for establishing moderation sessions")
  {
    tp_new = 0;
  }

  //
  // NewsGate::Moderator::Type class
  //
  inline
  Moderator::Type::Type() throw(El::Python::Exception, El::Exception)
      : El::Python::ObjectTypeImpl<Moderator, Moderator::Type>(
        "newsgate.moderation.Moderator",
        "Object providing feeds and accounts moderation functionality")
  {
    tp_new = 0;
  }
}

#endif // _NEWSGATE_SERVER_FRONTENDS_MODERATION_MODERATION_HPP_
