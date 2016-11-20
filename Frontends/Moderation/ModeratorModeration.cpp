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

#include <Services/Moderator/Commons/ModeratorManager.hpp>

#include "ModeratorModeration.hpp"
#include "Moderation.hpp"

namespace NewsGate
{
  namespace ModeratorModeration
  {
    Manager::Type Manager::Type::instance;

    Privilege::Type Privilege::Type::instance;

    GrantedPrivilege::Type
    GrantedPrivilege::Type::instance;

    ModeratorUpdateInfo::Type
    ModeratorUpdateInfo::Type::instance;
    
    ModeratorCreationInfo::Type
    ModeratorCreationInfo::Type::instance;
    
    //
    // NewsGate::ModeratorModeration::ModeratorModerationPyModule class
    //
    class ModeratorModerationPyModule :
      public El::Python::ModuleImpl<ModeratorModerationPyModule>
    {
    public:
      static ModeratorModerationPyModule instance;

      ModeratorModerationPyModule() throw(El::Exception);
      
      virtual void initialized() throw(El::Exception);

      El::Python::Object_var account_not_exist_ex;
      El::Python::Object_var account_already_exist_ex;
      El::Python::Object_var new_name_occupied_ex;
      El::Python::Object_var permission_denied_ex;
   };
  
    ModeratorModerationPyModule ModeratorModerationPyModule::instance;
  
    ModeratorModerationPyModule::ModeratorModerationPyModule()
      throw(El::Exception)
        : El::Python::ModuleImpl<ModeratorModerationPyModule>(
        "newsgate.moderation.moderator",
        "Module containing Moderator Moderation types.",
        true)
    {
    }

    void
    ModeratorModerationPyModule::initialized() throw(El::Exception)
    {
      account_not_exist_ex = create_exception("AccountNotExist");
      account_already_exist_ex = create_exception("AccountAlreadyExist");
      new_name_occupied_ex = create_exception("NewNameOccupied");
      permission_denied_ex = create_exception("PermissionDenied");

      add_member(PyLong_FromLong(Moderation::PV_FEED_CREATOR),
                 "PV_FEED_CREATOR");
    
      add_member(PyLong_FromLong(Moderation::PV_FEED_MANAGER),
                 "PV_FEED_MANAGER");
    
      add_member(PyLong_FromLong(Moderation::PV_ACCOUNT_MANAGER),
                 "PV_ACCOUNT_MANAGER");

      add_member(PyLong_FromLong(Moderation::PV_CATEGORY_MANAGER),
                 "PV_CATEGORY_MANAGER");

      add_member(PyLong_FromLong(Moderation::PV_CLIENT_MANAGER),
                 "PV_CLIENT_MANAGER");

      add_member(PyLong_FromLong(Moderation::PV_AD_MANAGER),
                 "PV_AD_MANAGER");

      add_member(PyLong_FromLong(Moderation::PV_ADVERTISER),
                 "PV_ADVERTISER");

      add_member(PyLong_FromLong(Moderation::PV_CUSTOMER_MANAGER),
                 "PV_CUSTOMER_MANAGER");

      add_member(PyLong_FromLong(Moderation::PV_CUSTOMER),
                 "PV_CUSTOMER");

      add_member(PyLong_FromLong(Moderation::PV_CATEGORY_EDITOR),
                 "PV_CATEGORY_EDITOR");

      add_member(PyLong_FromLong(Moderation::MS_ENABLED), "MS_ENABLED");
      add_member(PyLong_FromLong(Moderation::MS_DISABLED), "MS_DISABLED");    
    }
    
    //
    // NewsGate::ModeratorModeration::Manager class
    //
    
    Manager::Manager(PyTypeObject *type, PyObject *args, PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type),
          connector_(0)
    {
      
      throw Exception(
        "NewsGate::ModeratorModeration::Manager::Manager: unforseen way "
        "of object creation");
    }
    
    PyObject*
    Manager::py_set_advertiser(PyObject* args) throw(El::Exception)
    {
      unsigned long long id = 0;
      unsigned long long advertiser_id = 0;      
    
      if(!PyArg_ParseTuple(
           args,
           "KK:newsgate.moderation.moderator.Manager.set_advertiser",
           &id,
           &advertiser_id))
      {
        El::Python::handle_error(
          "NewsGate::ModeratorModeration::Manager::py_set_advertiser");
      }

      try
      {
        Moderation::ModeratorInfo_var result_info;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::ModeratorManager_var moderator_manager =
            moderator_manager_ref_.object();
          
          result_info = moderator_manager->set_advertiser(session_id().c_str(),
                                                          id,
                                                          advertiser_id);
        }
      
        return
          connector_->connect_moderator(*result_info, session_id().c_str());
      }
      catch(const Moderation::PermissionDenied& )
      {
        El::Python::report_error(
          ModeratorModerationPyModule::instance.permission_denied_ex.in(),
          "Permission denied");
      }
      catch(const Moderation::NotReady& e)
      {
        std::ostringstream ostr;
        ostr << "System not ready. Reason:\n" << e.reason.in();
        
        El::Python::report_error(
          ModerationPyModule::instance.not_ready_ex.in(),
          ostr.str().c_str());
      }
      catch(const Moderation::InvalidSession& )
      {
        El::Python::report_error(
          ModerationPyModule::instance.invalid_session_ex.in(),
          "Invalid session");
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::ModeratorModeration::Manager::py_set_advertiser: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::ModeratorModeration::Manager::py_set_advertiser: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::ModeratorModeration::Manager::py_set_advertiser: "
        "unexpected execution path");      
    }

    PyObject*
    Manager::py_update_moderator(PyObject* args)
      throw(El::Exception)
    {
      PyObject* ui = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "O:newsgate.moderation.moderator.Manager.update_moderator",
           &ui))
      {
        El::Python::handle_error(
          "NewsGate::ModeratorModeration::Manager::py_update_moderator");
      }

      ModeratorUpdateInfo* update = ModeratorUpdateInfo::Type::down_cast(ui);

      if(update->status >= Moderation::MS_COUNT)
      {
        El::Python::report_error(PyExc_TypeError,
                                 "Unexpected status value",
                                 "NewsGate::ModeratorModeration::Manager::"
                                 "py_update_moderator");
      }
    
      Moderation::ModeratorUpdateInfo_var mui =
        new Moderation::ModeratorUpdateInfo();

      mui->id = update->id;
      mui->name = update->name.c_str();
      mui->email = update->email.c_str();
      mui->password = update->password.c_str();
      mui->status = (Moderation::ModeratorStatus)update->status;
      mui->show_deleted = update->show_deleted;

      const El::Python::Sequence& privs = *update->privileges;
      mui->privileges.length(privs.size());

      unsigned long i = 0;
      for(El::Python::Sequence::const_iterator it = privs.begin();
          it != privs.end(); it++)
      {
        Privilege* val = Privilege::Type::down_cast(it->in());          

        if(val->id >= Moderation::PV_COUNT || val->id == Moderation::PV_NULL)
        {
          El::Python::report_error(PyExc_TypeError,
                                   "Unexpected privelege value",
                                   "NewsGate::ModeratorModeration::Manager::"
                                   "py_update_moderator");
        }

        Moderation::Privilege& privilege = mui->privileges[i++];        

        privilege.id = (Moderation::PrivilegeId)val->id;
        privilege.args = val->args.c_str();
      }

      try
      {
        Moderation::ModeratorInfo_var result_info;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::ModeratorManager_var moderator_manager =
            moderator_manager_ref_.object();

          result_info = moderator_manager->update_moderator(
            session_id().c_str(),
            mui.in());
        }
      
        return
          connector_->connect_moderator(*result_info, session_id().c_str());
      }
      catch(const Moderation::AccountNotExist& )
      {
        El::Python::report_error(
          ModeratorModerationPyModule::instance.account_not_exist_ex.in(),
          "Account not exist");
      }
      catch(const Moderation::NewNameOccupied& )
      {
        El::Python::report_error(
          ModeratorModerationPyModule::instance.new_name_occupied_ex.in(),
          "New name occupied");
      }
      catch(const Moderation::PermissionDenied& )
      {
        El::Python::report_error(
          ModeratorModerationPyModule::instance.permission_denied_ex.in(),
          "Permission denied");
      }
      catch(const Moderation::NotReady& e)
      {
        std::ostringstream ostr;
        ostr << "System not ready. Reason:\n" << e.reason.in();
        
        El::Python::report_error(
          ModerationPyModule::instance.not_ready_ex.in(),
          ostr.str().c_str());
      }
      catch(const Moderation::InvalidSession& )
      {
        El::Python::report_error(
          ModerationPyModule::instance.invalid_session_ex.in(),
          "Invalid session");
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::ModeratorModeration::Manager::py_update_moderator: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::ModeratorModeration::Manager::py_update_moderator: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::ModeratorModeration::Manager::py_update_moderator: "
        "unexpected execution path");
    }
        
    PyObject*
    Manager::py_create_moderator(PyObject* args)
      throw(El::Exception)
    {
      PyObject* ci = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "O:newsgate.moderation.moderator.Manager.create_moderator",
           &ci))
      {
        El::Python::handle_error(
          "NewsGate::ModeratorModeration::Manager::py_create_moderator");
      }

      ModeratorCreationInfo* info = ModeratorCreationInfo::Type::down_cast(ci);

      if(info->status >= Moderation::MS_COUNT)
      {
        El::Python::report_error(PyExc_TypeError,
                                 "Unexpected status value",
                                 "NewsGate::ModeratorModeration::Manager::"
                                 "py_create_moderator");
      }
    
      Moderation::ModeratorCreationInfo_var mci =
        new Moderation::ModeratorCreationInfo();

      mci->name = info->name.c_str();
      mci->email = info->email.c_str();
      mci->password = info->password.c_str();
      mci->status = (Moderation::ModeratorStatus)info->status;
      mci->show_deleted = info->show_deleted;

      const El::Python::Sequence& privs = *info->privileges;
      mci->privileges.length(privs.size());

      unsigned long i = 0;
      for(El::Python::Sequence::const_iterator it = privs.begin();
          it != privs.end(); it++)
      {
        Privilege* val = Privilege::Type::down_cast(it->in());          

        if(val->id >= Moderation::PV_COUNT || val->id == Moderation::PV_NULL)
        {
          El::Python::report_error(PyExc_TypeError,
                                   "Unexpected privelege value",
                                   "NewsGate::ModeratorModeration::Manager::"
                                   "py_create_moderator");
        }

        Moderation::Privilege& privilege = mci->privileges[i++];        
        
        privilege.id = (Moderation::PrivilegeId)val->id;
        privilege.args = val->args.c_str();
      }

      Moderation::ModeratorInfo_var result_info;
      
      try
      {
        {
          El::Python::AllowOtherThreads guard;
          
          NewsGate::Moderation::ModeratorManager_var moderator_manager =
            moderator_manager_ref_.object();

          result_info =
            moderator_manager->create_moderator(session_id().c_str(),
                                                mci.in());
        }
      
        return connector_->connect_moderator(*result_info, 0);
      }
      catch(const Moderation::AccountAlreadyExist& )
      {
        El::Python::report_error(
          ModeratorModerationPyModule::instance.account_already_exist_ex.in(),
          "Account already exist");
      }
      catch(const Moderation::NewNameOccupied& )
      {
        El::Python::report_error(
          ModeratorModerationPyModule::instance.new_name_occupied_ex.in(),
          "New name occupied");
      }
      catch(const Moderation::PermissionDenied& )
      {
        El::Python::report_error(
          ModeratorModerationPyModule::instance.permission_denied_ex.in(),
          "Permission denied");
      }
      catch(const Moderation::NotReady& e)
      {
        std::ostringstream ostr;
        ostr << "System not ready. Reason:\n" << e.reason.in();
        
        El::Python::report_error(
          ModerationPyModule::instance.not_ready_ex.in(),
          ostr.str().c_str());
      }
      catch(const Moderation::InvalidSession& )
      {
        El::Python::report_error(
          ModerationPyModule::instance.invalid_session_ex.in(),
          "Invalid session");
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::ModeratorModeration::Manager::py_create_moderator: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Moderator::py_create_moderator: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception("NewsGate::ModeratorModeration::Manager::"
                      "py_create_moderator: unexpected execution path");
    }
    
    PyObject*
    Manager::py_get_subordinates(PyObject* args) throw(El::Exception)
    {
      try
      {
        Moderation::ModeratorInfoSeq_var subordinates;
      
        {
          El::Python::AllowOtherThreads guard;
        
          NewsGate::Moderation::ModeratorManager_var moderator_manager =
            moderator_manager_ref_.object();

          subordinates =
            moderator_manager->get_subordinates(session_id().c_str());
        }

        El::Python::Sequence_var res = new El::Python::Sequence();
        res->resize(subordinates->length());
      
        for(unsigned long i = 0; i < subordinates->length(); i++)
        {
          (*res)[i] = connector_->connect_moderator((*subordinates)[i], 0);
        }

        return res.retn();
      }
      catch(const Moderation::NotReady& e)
      {
        std::ostringstream ostr;
        ostr << "System not ready. Reason:\n" << e.reason.in();
        
        El::Python::report_error(
          ModerationPyModule::instance.not_ready_ex.in(),
          ostr.str().c_str());
      }
      catch(const Moderation::InvalidSession& )
      {
        El::Python::report_error(
          ModerationPyModule::instance.invalid_session_ex.in(),
          "Invalid session");
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::ModeratorModeration::Manager::py_get_subordinates: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::ModeratorModeration::Manager::py_get_subordinates: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception("NewsGate::ModeratorModeration::Manager::"
                      "py_get_subordinates: unexpected execution path");
    }
  
    PyObject*
    Manager::py_get_moderators(PyObject* args) throw(El::Exception)
    {
      PyObject* moderator_ids = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "O:newsgate.moderation.moderator.Manager.py_get_moderators",
           &moderator_ids))
      {
        El::Python::handle_error(
          "NewsGate::ModeratorModeration::Manager::py_get_moderators");
      }

      if(!PySequence_Check(moderator_ids))
      {
        El::Python::report_error(PyExc_TypeError,
                                 "argument expected to be of sequence type",
                                 "NewsGate::ModeratorModeration::Manager::"
                                 "py_get_moderators");
      }

      int len = PySequence_Size(moderator_ids);

      if(len < 0)
      {
        El::Python::handle_error(
          "NewsGate::ModeratorModeration::Manager::py_get_moderators");
      }
      
      try
      {
        Moderation::ModeratorIdSeq_var seq = new Moderation::ModeratorIdSeq();
        seq->length(len);
      
        for(CORBA::ULong i = 0; i < (CORBA::ULong)len; i++)
        {
          El::Python::Object_var item = PySequence_GetItem(moderator_ids, i);
        
          seq[i] = El::Python::ulonglong_from_number(
            item.in(),
            "NewsGate::ModeratorModeration::Manager::py_get_moderators");
        }

        Moderation::ModeratorInfoSeq_var moderators;
      
        {
          El::Python::AllowOtherThreads guard;
        
          NewsGate::Moderation::ModeratorManager_var moderator_manager =
            moderator_manager_ref_.object();

          moderators =
            moderator_manager->get_moderators(session_id().c_str(), seq.in());
        }

        El::Python::Sequence_var res = new El::Python::Sequence();
        res->resize(moderators->length());
      
        for(unsigned long i = 0; i < moderators->length(); i++)
        {
          (*res)[i] = connector_->connect_moderator((*moderators)[i], 0);
//          (*res)[i] = new Moderator((*moderators)[i], "", 0, 0, 0, 0, 0);
        }

        return res.retn();
      }
      catch(const Moderation::NotReady& e)
      {
        std::ostringstream ostr;
        ostr << "System not ready. Reason:\n" << e.reason.in();
        
        El::Python::report_error(
          ModerationPyModule::instance.not_ready_ex.in(),
          ostr.str().c_str());
      }
      catch(const Moderation::InvalidSession& )
      {
        El::Python::report_error(
          ModerationPyModule::instance.invalid_session_ex.in(),
          "Invalid session");
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::ModeratorModeration::Manager::py_get_moderators: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::ModeratorModeration::Manager::py_get_moderators: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception("NewsGate::ModeratorModeration::Manager::"
                      "py_get_moderators: unexpected execution path");
    }
  
    PyObject*
    Manager::py_get_advertisers() throw(El::Exception)
    {
      try
      {
        Moderation::ModeratorInfoSeq_var moderators;
      
        {
          El::Python::AllowOtherThreads guard;
        
          NewsGate::Moderation::ModeratorManager_var moderator_manager =
            moderator_manager_ref_.object();

          moderators =
            moderator_manager->get_advertisers(session_id().c_str());
        }

        El::Python::Sequence_var res = new El::Python::Sequence();
        res->resize(moderators->length());
      
        for(unsigned long i = 0; i < moderators->length(); i++)
        {
          (*res)[i] = connector_->connect_moderator((*moderators)[i], 0);
        }

        return res.retn();
      }
      catch(const Moderation::NotReady& e)
      {
        std::ostringstream ostr;
        ostr << "System not ready. Reason:\n" << e.reason.in();
        
        El::Python::report_error(
          ModerationPyModule::instance.not_ready_ex.in(),
          ostr.str().c_str());
      }
      catch(const Moderation::InvalidSession& )
      {
        El::Python::report_error(
          ModerationPyModule::instance.invalid_session_ex.in(),
          "Invalid session");
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::ModeratorModeration::Manager::py_get_advertisers: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::ModeratorModeration::Manager::py_get_advertisers: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception("NewsGate::ModeratorModeration::Manager::"
                      "py_get_advertisers: unexpected execution path");
    }
  
    //
    // NewsGate::MessageModeration::Privilege class
    //
    Privilege::Privilege(PyTypeObject *type, PyObject *args, PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0)
    {
    }

    //
    // NewsGate::MessageModeration::GrantedPrivilege class
    //
    GrantedPrivilege::GrantedPrivilege(PyTypeObject *type,
                                       PyObject *args,
                                       PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          priv(new Privilege())
    {
    }

    //
    // NewsGate::MessageModeration::ModeratorUpdateInfo class
    //
    ModeratorUpdateInfo::ModeratorUpdateInfo(
      PyTypeObject *type,
      PyObject *args,
      PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          status(Moderation::MS_DISABLED),
          show_deleted(false),
          privileges(new El::Python::Sequence())
    {
    }

    //
    // NewsGate::MessageModeration::ModeratorCreationInfo class
    //
    ModeratorCreationInfo::ModeratorCreationInfo(
      PyTypeObject *type,
      PyObject *args,
      PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          status(Moderation::MS_DISABLED),
          show_deleted(false),
          privileges(new El::Python::Sequence())
    {
    }    
  }
}
