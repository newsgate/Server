/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Frontends/Moderation/ModeratorModeration.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_FRONTENDS_MODERATION_MODERATORMODERATION_HPP_
#define _NEWSGATE_SERVER_FRONTENDS_MODERATION_MODERATORMODERATION_HPP_

#include <string>

#include <ace/Synch.h>
#include <ace/Guard_T.h>

#include <El/Exception.hpp>

#include <El/Python/Exception.hpp>
#include <El/Python/Object.hpp>
#include <El/Python/RefCount.hpp>
#include <El/Python/Logger.hpp>

#include <Services/Moderator/Commons/ModeratorManager.hpp>

namespace NewsGate
{
  class ModeratorConnector;
  
  namespace ModeratorModeration
  {
    EL_EXCEPTION(Exception, El::ExceptionBase);

    typedef El::Corba::SmartRef<NewsGate::Moderation::ModeratorManager>
    ManagerRef;    
    
    class Manager : public El::Python::ObjectImpl
    {
    public:
      Manager(PyTypeObject *type, PyObject *args, PyObject *kwds)
        throw(Exception, El::Exception);      
    
      Manager(const ManagerRef& moderator_manager_ref,
              const ::NewsGate::ModeratorConnector* connector)
        throw(Exception, El::Exception);

      virtual ~Manager() throw() {}

      PyObject* py_update_moderator(PyObject* args) throw(El::Exception);
      PyObject* py_create_moderator(PyObject* args) throw(El::Exception);
      PyObject* py_get_subordinates(PyObject* args) throw(El::Exception);
      PyObject* py_get_moderators(PyObject* args) throw(El::Exception);
      PyObject* py_set_advertiser(PyObject* args) throw(El::Exception);
      PyObject* py_get_advertisers() throw(El::Exception);

      void session_id(const char* val) throw(El::Exception);
      std::string session_id() const throw(El::Exception);
      
      class Type : public El::Python::ObjectTypeImpl<Manager, Manager::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_METHOD_VARARGS(py_update_moderator,
                               "update_moderator",
                               "Updates moderator information");
        
        PY_TYPE_METHOD_VARARGS(py_create_moderator,
                               "create_moderator",
                               "Creates new moderator account");
        
        PY_TYPE_METHOD_VARARGS(py_get_subordinates,
                               "get_subordinates",
                               "Get subordinate moderators info");
        
        PY_TYPE_METHOD_VARARGS(py_get_moderators,
                               "get_moderators",
                               "Get moderators info");
        
        PY_TYPE_METHOD_VARARGS(py_set_advertiser,
                               "set_advertiser",
                               "Set advertiser id");
        
        PY_TYPE_METHOD_NOARGS(py_get_advertisers,
                              "get_advertisers",
                              "Get advertisers");
      };

    private:
      typedef ACE_RW_Thread_Mutex    Mutex;
      typedef ACE_Write_Guard<Mutex> WriteGuard;      
      typedef ACE_Read_Guard<Mutex>  ReadGuard;      

      mutable Mutex lock_;
      std::string session_id_;
      ManagerRef moderator_manager_ref_;
      const ::NewsGate::ModeratorConnector* connector_;
    };

    typedef El::Python::SmartPtr<Manager> Manager_var;

    class Privilege : public El::Python::ObjectImpl
    {
    public:
      unsigned long id;
      std::string args;
    
    public:
      Privilege(PyTypeObject *type = 0,
                PyObject *args = 0,
                PyObject *kwds = 0)
        throw(Exception, El::Exception);
    
      virtual ~Privilege() throw() {}

      class Type : public El::Python::ObjectTypeImpl<Privilege,
                                                     Privilege::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONG(id, "id", "Privilege id");
        PY_TYPE_MEMBER_STRING(args, "args", "Privilege args", true);        
      };
    };

    typedef El::Python::SmartPtr<Privilege> Privilege_var;    

    class GrantedPrivilege : public El::Python::ObjectImpl
    {
    public:
      Privilege_var priv;
      std::string name;      
      std::string granted_by;      
    
    public:
      GrantedPrivilege(PyTypeObject *type = 0,
                       PyObject *args = 0,
                       PyObject *kwds = 0)
        throw(Exception, El::Exception);
    
      virtual ~GrantedPrivilege() throw() {}

      class Type : public El::Python::ObjectTypeImpl<GrantedPrivilege,
                                                     GrantedPrivilege::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_OBJECT(priv,
                              Privilege::Type,
                              "priv",
                              "Privilege info",
                              false);
        
        PY_TYPE_MEMBER_STRING(name, "name", "Privilege name", false);
        
        PY_TYPE_MEMBER_STRING(granted_by,
                              "granted_by",
                              "Name of moderator has granted this privilege",
                              false);
      };
    };

    typedef El::Python::SmartPtr<GrantedPrivilege> GrantedPrivilege_var;
    
    class ModeratorUpdateInfo : public El::Python::ObjectImpl
    {
    public:
      Moderation::ModeratorId id;
      std::string name;
      std::string email;
      std::string password;
      unsigned long status;
      bool show_deleted;
      El::Python::Sequence_var privileges;
    
    public:
      ModeratorUpdateInfo(PyTypeObject *type = 0,
                          PyObject *args = 0,
                          PyObject *kwds = 0)
        throw(Exception, El::Exception);
    
      virtual ~ModeratorUpdateInfo() throw() {}

      class Type : public El::Python::ObjectTypeImpl<ModeratorUpdateInfo,
                                                     ModeratorUpdateInfo::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Moderator identifier");

        // If empty moderator name will not be changed
        PY_TYPE_MEMBER_STRING(name, "name", "Moderator new name", true);

        // If empty moderator email will not be changed
        PY_TYPE_MEMBER_STRING(email, "email", "Moderator new email", true);

        // If empty moderator password will not be changed
        PY_TYPE_MEMBER_STRING(password,
                              "password",
                              "Moderator new password",
                              true);
        
        PY_TYPE_MEMBER_ULONG(status, "status", "Moderator new status");
        
        PY_TYPE_MEMBER_BOOL(show_deleted,
                            "show_deleted",
                            "Moderator show_deleted flag");
        
        PY_TYPE_MEMBER_OBJECT(privileges,
                              El::Python::Sequence::Type,
                              "privileges",
                              "Moderator new priviliges",
                              false);
      };
    };

    typedef El::Python::SmartPtr<ModeratorUpdateInfo> ModeratorUpdateInfo_var;

    class ModeratorCreationInfo : public El::Python::ObjectImpl
    {
    public:
      std::string name;
      std::string email;
      std::string password;
      unsigned long status;
      bool show_deleted;
      El::Python::Sequence_var privileges;
    
    public:
      ModeratorCreationInfo(PyTypeObject *type = 0,
                            PyObject *args = 0,
                            PyObject *kwds = 0)
        throw(Exception, El::Exception);
    
      virtual ~ModeratorCreationInfo() throw() {}

      class Type : public El::Python::ObjectTypeImpl<
        ModeratorCreationInfo,
        ModeratorCreationInfo::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;
        
        PY_TYPE_MEMBER_STRING(name, "name", "Moderator name", true);

        PY_TYPE_MEMBER_STRING(email, "email", "Moderator email", true);

        PY_TYPE_MEMBER_STRING(password,
                              "password",
                              "Moderator password",
                              true);
        
        PY_TYPE_MEMBER_ULONG(status, "status", "Moderator status");
        
        PY_TYPE_MEMBER_BOOL(show_deleted,
                            "show_deleted",
                            "Moderator show_deleted flag");
        
        PY_TYPE_MEMBER_OBJECT(privileges,
                              El::Python::Sequence::Type,
                              "privileges",
                              "Moderator priviliges",
                              false);
      };
    };

    typedef El::Python::SmartPtr<ModeratorCreationInfo>
    ModeratorCreationInfo_var;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{  
  namespace ModeratorModeration
  {
    //
    // NewsGate::ModeratorModeration::Manager class
    //
    inline
    Manager::Manager(const ManagerRef& manager,
                     const ::NewsGate::ModeratorConnector* connector)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          moderator_manager_ref_(manager),
          connector_(connector)
    {
    }    

    inline
    void
    Manager::session_id(const char* val) throw(El::Exception)
    {
      WriteGuard guard(lock_);
      session_id_ = val;
    }
    
    inline
    std::string
    Manager::session_id() const throw(El::Exception)
    {
      ReadGuard guard(lock_);
      return session_id_;
    }
    
    //
    // NewsGate::ModeratorModeration::Manager::Type class
    //
    inline
    Manager::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<Manager, Manager::Type>(
          "newsgate.moderation.moderator.Manager",
          "Object representing moderator management functionality")
    {
      tp_new = 0;
    }

    //
    // NewsGate::ModeratorModeration::Privilege::Type class
    //
    inline
    Privilege::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<Privilege, Privilege::Type>(
          "newsgate.moderation.moderator.Privilege",
          "Object encapsulating privilege info")
    {
    }

    //
    // NewsGate::ModeratorModeration::GrantedPrivilege::Type class
    //
    inline
    GrantedPrivilege::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<GrantedPrivilege,
                                     GrantedPrivilege::Type>(
          "newsgate.moderation.moderator.GrantedPrivilege",
          "Object encapsulating granted privilege options")
    {
    }

    //
    // NewsGate::ModeratorModeration::ModeratorUpdateInfo::Type class
    //
    inline
    ModeratorUpdateInfo::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<
      ModeratorUpdateInfo,
      ModeratorUpdateInfo::Type>(
        "newsgate.moderation.moderator.ModeratorUpdateInfo",
        "Object encapsulating moderator update options")
    {
    }

    //
    // NewsGate::ModeratorModeration::ModeratorCreationInfo::Type class
    //
    inline
    ModeratorCreationInfo::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<
      ModeratorCreationInfo,
      ModeratorCreationInfo::Type>(
        "newsgate.moderation.moderator.ModeratorCreationInfo",
        "Object encapsulating moderator creation options")
    {
    }
  }
}

#endif // _NEWSGATE_SERVER_FRONTENDS_MODERATION_MODERATORMODERATION_HPP_
