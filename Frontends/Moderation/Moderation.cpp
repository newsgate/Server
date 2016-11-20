/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Frontends/SearchEngine/SearchEngine.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <Python.h>
#include <El/CORBA/Corba.hpp>

#include <string>
#include <sstream>

#include <El/Exception.hpp>

#include <El/Net/HTTP/Params.hpp>
#include <El/Net/HTTP/Cookies.hpp>
#include <El/Apache/Request.hpp>

#include <El/Python/Object.hpp>
#include <El/Python/Module.hpp>
#include <El/Python/Utility.hpp>
#include <El/Python/RefCount.hpp>
#include <El/Python/Sequence.hpp>

#include <El/PSP/Config.hpp>
#include <El/PSP/Request.hpp>

#include <Commons/Feed/Types.hpp>

#include <Commons/Search/SearchExpression.hpp>

#include <Services/Moderator/Commons/TransportImpl.hpp>
#include <Services/Moderator/Commons/ModeratorManager.hpp>
#include <Services/Moderator/Commons/FeedManager.hpp>
#include <Services/Moderator/Commons/CategoryManager.hpp>

#include "Moderation.hpp"
#include "CategoryModeration.hpp"
#include "MessageModeration.hpp"

namespace
{
  static char ASPECT[] = "Moderation";
}

namespace NewsGate
{
  ModeratorConnector::Type ModeratorConnector::Type::instance;
  Moderator::Type Moderator::Type::instance;

  //
  // NewsGate::ModeratorConnector class
  //
  Moderation::CreatorIdSeq*
  ModeratorConnector::get_creator_ids(PyObject* creator_ids)
    throw(Exception, El::Exception)
  {
    int len = PySequence_Size(creator_ids);

    if(len < 0)
    {
      El::Python::handle_error(
        "NewsGate::ModeratorConnector::get_creator_ids");
    }

    Moderation::CreatorIdSeq_var seq = new Moderation::CreatorIdSeq();
    seq->length(len);
    
    for(CORBA::ULong i = 0; i < (CORBA::ULong)len; i++)
    {
      El::Python::Object_var item = PySequence_GetItem(creator_ids, i);

      seq[i] = El::Python::ulonglong_from_number(
        item.in(),
        "NewsGate::ModeratorConnector::get_creator_ids");
    }

    return seq._retn();
  }

  //
  // NewsGate::ModerationPyModule class
  //
  ModerationPyModule ModerationPyModule::instance;
  
  ModerationPyModule::ModerationPyModule() throw(El::Exception)
      : El::Python::ModuleImpl<ModerationPyModule>(
        "newsgate.moderation",
        "Module containing ModeratorConnector factory method.",
        true)
  {
  }

  void
  ModerationPyModule::initialized() throw(El::Exception)
  {
    not_ready_ex = create_exception("NotReady");
    invalid_session_ex = create_exception("InvalidSession");    
    invalid_user_name_ex = create_exception("InvalidUsername");
    account_disabled_ex = create_exception("AccountDisabled");
  }
  
  PyObject*
  ModerationPyModule::py_create_moderator_connector(PyObject* args)
    throw(El::Exception)
  {
    return new ModeratorConnector(args);
  }
 
  PyObject*
  ModerationPyModule::py_cleanup_moderator_connector(PyObject* args)
    throw(El::Exception)
  {
    PyObject* se = 0;
    
    if(!PyArg_ParseTuple(args,
                         "O:newsgate.moderation.cleanup_moderator_connector",
                         &se))
    {
      El::Python::handle_error(
        "NewsGate::ModerationPyModule::py_cleanup_moderator_connector");
    }

    if(!ModeratorConnector::Type::check_type(se))
    {
      El::Python::report_error(
        PyExc_TypeError,
        "1st argument of newsgate.moderation.ModeratorConnector",
        "NewsGate::WordPyModule::py_cleanup_moderator_connector");
    }

    ModeratorConnector* connector = ModeratorConnector::Type::down_cast(se);
    connector->cleanup();
    
    return El::Python::add_ref(Py_None);
  }  

  //
  // NewsGate::ModeratorConnector class
  //
  ModeratorConnector::ModeratorConnector(PyTypeObject *type,
                                         PyObject *args,
                                         PyObject *kwds)
    throw(Exception, El::Exception)
      : El::Python::ObjectImpl(type),
        orb_adapter_(0)
  {
    throw Exception("NewsGate::ModeratorConnector::ModeratorConnector: "
                    "unforseen way of object creation");
  }
        
  ModeratorConnector::ModeratorConnector(PyObject* args)
    throw(Exception, El::Exception)
      : El::Python::ObjectImpl(&Type::instance),
        orb_adapter_(0)
  {
    PyObject* config = 0;
    PyObject* logger = 0;
    
    if(!PyArg_ParseTuple(
         args,
         "OO:newsgate.moderation.ModeratorConnector.ModeratorConnector",
         &config,
         &logger))
    {
      El::Python::handle_error(
        "NewsGate::ModeratorConnector::ModeratorConnector");
    }

    if(!El::PSP::Config::Type::check_type(config))
    {
      El::Python::report_error(
        PyExc_TypeError,
        "1st argument of el.psp.Config expected",
        "NewsGate::ModeratorConnector::ModeratorConnector");
    }

    if(!El::Logging::Python::Logger::Type::check_type(logger))
    {
      El::Python::report_error(
        PyExc_TypeError,
        "2nd argument of el.logging.Logger expected",
        "NewsGate::ModeratorConnector::ModeratorConnector");
    }

    config_ = El::PSP::Config::Type::down_cast(config, true);
    logger_ = El::Logging::Python::Logger::Type::down_cast(logger, true);

    try
    {
      orb_adapter_ = El::Corba::Adapter::orb_adapter(0, 0);
      orb_ = ::CORBA::ORB::_duplicate(orb_adapter_->orb());

      Moderation::Transport::register_valuetype_factories(orb_.in());      
      
      moderator_manager_ref_ = ModeratorModeration::ManagerRef(
        config_->string("moderator_manager").c_str(), orb_.in());
      
      FeedModeration::ManagerRef feed_man_ref(
        config_->string("feed_manager").c_str(), orb_.in());      

      message_manager_ = new MessageModeration::Manager(feed_man_ref);
      feed_manager_ = new FeedModeration::Manager(feed_man_ref);
      
      moderator_manager_ =
        new ModeratorModeration::Manager(moderator_manager_ref_, this);
      
      CategoryModeration::ManagerRef cat_manager(
        config_->string("category_manager").c_str(), orb_.in());

      category_manager_ = new CategoryModeration::Manager(cat_manager);

      CustomerModeration::ManagerRef cust_manager(
        config_->string("customer_manager").c_str(), orb_.in());

      customer_manager_ = new CustomerModeration::Manager(cust_manager);

      AdModeration::ManagerRef ad_manager(
        config_->string("ad_manager").c_str(), orb_.in());

      ad_manager_ = new AdModeration::Manager(ad_manager);
    }
    catch(const CORBA::Exception& e)
    {
      if(orb_adapter_)
      {
        El::Corba::Adapter::orb_adapter_cleanup();
        orb_adapter_ = 0;
      }

      std::ostringstream ostr;
      ostr << "NewsGate::ModeratorConnector::ModeratorConnector: "
        "CORBA::Exception caught. Description:\n" << e;

      throw Exception(ostr.str());
    }

    logger_->info("NewsGate::ModeratorConnector::ModeratorConnector: "
                  "moderation objects constructed",
                  ASPECT);
  }

  ModeratorConnector::~ModeratorConnector() throw()
  {
    cleanup();
    
    logger_->info("NewsGate::ModeratorConnector::~ModeratorConnector: "
                  "moderation objects destructed",
                  ASPECT);
  }

  void
  ModeratorConnector::cleanup() throw()
  {
    try
    {
      bool cleanup_corba_adapter = false;

      {
        WriteGuard guard(lock_);        
        cleanup_corba_adapter = orb_adapter_ != 0;
        orb_adapter_ = 0;
      }

      logger_->info("NewsGate::ModeratorConnector::cleanup: done", ASPECT);
      
      if(cleanup_corba_adapter)
      {
        El::Corba::Adapter::orb_adapter_cleanup();
      }
    }
    catch(...)
    {
    }
  }

  PyObject*
  ModeratorConnector::py_connect(PyObject* args) throw(El::Exception)
  {
    PyObject* request = 0;
    unsigned char can_login = false;
    unsigned char advance_sess_timeout = true;
    
    if(!PyArg_ParseTuple(args,
                         "O|bb:newsgate.moderation.ModeratorConnector.connect",
                         &request,
                         &can_login,
                         &advance_sess_timeout))
    {
      El::Python::handle_error("NewsGate::ModeratorConnector::py_connect");
    }

    if(!El::PSP::Request::Type::check_type(request))
    {
      El::Python::report_error(
        PyExc_TypeError,
        "1st argument expected to be of type el.psp.Request",
        "NewsGate::ModeratorConnector::py_connect");
    }

    El::Apache::Request* req =
      El::PSP::Request::Type::down_cast(request)->request();

    std::string uname;
    std::string password;

    if(can_login)
    {
      const El::Net::HTTP::ParamList& params = req->in().parameters();
      
      for(El::Net::HTTP::ParamList::const_iterator it = params.begin();
          it != params.end(); it++)
      {
        const std::string& nm = it->name;
        const std::string& val = it->value;
        
        if(nm == "n")
        {
          uname = val;
        }
        else if(nm == "p")
        {
          password = val;
        }
      }
    }

    std::string session_id;

    try
    {
      if(uname.empty())
      {
        const char* psess = req->in().cookies().most_specific("ms");

        if(psess)
        {
          session_id = psess;
        }
        else
        {
          throw Moderation::InvalidSession();
        }
      }
      else
      {
        El::Python::AllowOtherThreads guard;
        
        NewsGate::Moderation::ModeratorManager_var moderator_manager =
          moderator_manager_ref_.object();
        
        Moderation::SessionId_var sessid =
          moderator_manager->login(uname.c_str(),
                                   password.c_str(),
                                   req->remote_ip());
        
        session_id = sessid.in();
      }

      Moderation::ModeratorInfo_var moderator_info;
      
      {
        El::Python::AllowOtherThreads guard;
        
        NewsGate::Moderation::ModeratorManager_var moderator_manager =
          moderator_manager_ref_.object();

        moderator_info = moderator_manager->authenticate(session_id.c_str(),
                                                         req->remote_ip(),
                                                         advance_sess_timeout);
      }
      
      return connect_moderator(*moderator_info, session_id.c_str());
    }
    catch(const Moderation::InvalidUsername& )
    {
      El::Python::report_error(
        ModerationPyModule::instance.invalid_user_name_ex.in(),
        "Invalid user name or password");      
    }
    catch(const Moderation::AccountDisabled& )
    {
      El::Python::report_error(
        ModerationPyModule::instance.account_disabled_ex.in(),
        "Account disabled");
    }
    catch(const Moderation::NotReady& e)
    {
      std::ostringstream ostr;
      ostr << "System not ready. Reason:\n" << e.reason.in();
        
      El::Python::report_error(ModerationPyModule::instance.not_ready_ex.in(),
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
      ostr << "NewsGate::ModeratorConnector::py_connect: "
        "ImplementationException exception caught. Description:\n"
           << e.description.in();
        
      throw Exception(ostr.str());
    }
    catch(const CORBA::Exception& e)
    {
      std::ostringstream ostr;
      ostr << "NewsGate::ModeratorConnector::py_connect: "
        "CORBA::Exception caught. Description:\n" << e;

      throw Exception(ostr.str());
    }

    throw Exception("NewsGate::ModeratorConnector::py_connect: "
                    "unexpected execution path");
  }

  PyObject*
  ModeratorConnector::py_logout(PyObject* args) throw(El::Exception)
  {    
    PyObject* request = 0;
    
    if(!PyArg_ParseTuple(args,
                         "O:newsgate.moderation.ModeratorConnector.logout",
                         &request))
    {
      El::Python::handle_error("NewsGate::ModeratorConnector::py_logout");
    }

    if(!El::PSP::Request::Type::check_type(request))
    {
      El::Python::report_error(
        PyExc_TypeError,
        "1st argument expected to be of type el.psp.Request",
        "NewsGate::ModeratorConnector::py_logout");
    }

    El::Apache::Request* req =
      El::PSP::Request::Type::down_cast(request)->request();

    const char* psess = req->in().cookies().most_specific("ms");

    if(psess)
    {
      try
      {
        El::Python::AllowOtherThreads guard;
        
        NewsGate::Moderation::ModeratorManager_var moderator_manager =
          moderator_manager_ref_.object();
        
        moderator_manager->logout(psess, req->remote_ip());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::ModeratorConnector::py_logout: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::ModeratorConnector::py_logout: "
          "CORBA::Exception caught. Description:\n" << e;
        
        throw Exception(ostr.str());
      } 
    }

    return El::Python::add_ref(Py_None);
  }

  Moderator*
  ModeratorConnector::connect_moderator(
    const Moderation::ModeratorInfo& moderator_info,
    const char* session) const throw(El::Exception)
  {
    try
    {
      NewsGate::Moderation::ModeratorManager_var moderator_manager =
        moderator_manager_ref_.object();

      bool customer_moderating = config_->string("customer_moderating") == "1";
      
      return session && *session != '\0' ?
        new Moderator(moderator_info,
                      session,
                      customer_moderating,
                      moderator_manager.in(),
                      category_manager_.in(),
                      message_manager_.in(),
                      feed_manager_.in(),
                      moderator_manager_.in(),
                      customer_manager_.in(),
                      ad_manager_.in()) :
        new Moderator(moderator_info,
                      "",
                      customer_moderating,
                      0,
                      0,
                      0,
                      0,
                      0,
                      0,
                      0);
    }
    catch(const CORBA::Exception& e)
    {
      std::ostringstream ostr;
      ostr << "NewsGate::ModeratorConnector::connect_moderator: "
        "CORBA::Exception caught. Description:\n" << e;

      throw Exception(ostr.str());
    }
  }
  
  //
  // NewsGate::Moderator class
  //
  Moderator::Moderator(PyTypeObject *type,
                       PyObject *args,
                       PyObject *kwds)
    throw(Exception, El::Exception)
      : El::Python::ObjectImpl(type ? type : &Type::instance),
        id(0),
        advertiser_id(0),
        status(Moderation::MS_DISABLED),
        show_deleted(false),
        privileges(new El::Python::Sequence())
  {
  }

  Moderator::Moderator(
    const Moderation::ModeratorInfo& moderator_info,
    const char* session,
    bool customer_moderating,
    ::NewsGate::Moderation::ModeratorManager* mod_manager,
    CategoryModeration::Manager* category_manager_val,
    MessageModeration::Manager* message_manager_val,
    FeedModeration::Manager* feed_manager_val,
    ModeratorModeration::Manager* moderator_manager_val,
    CustomerModeration::Manager* customer_manager_val,
    AdModeration::Manager* ad_manager_val)
    throw(Exception, El::Exception)
      : El::Python::ObjectImpl(&Type::instance),
        session_id(session),
        id(moderator_info.id),
        advertiser_id(moderator_info.advertiser_id),
        name(moderator_info.name.in()),
        advertiser_name(moderator_info.advertiser_name.in()),
        email(moderator_info.email.in()),
        updated(moderator_info.updated.in()),
        created(moderator_info.created.in()),
        creator(moderator_info.creator.in()),
        superior(moderator_info.superior.in()),
        comment(moderator_info.comment.in()),
        status(moderator_info.status),
        show_deleted(moderator_info.show_deleted),
        privileges(new El::Python::Sequence()),
        category_manager(El::Python::add_ref(category_manager_val)),
        message_manager(El::Python::add_ref(message_manager_val)),
        feed_manager(El::Python::add_ref(feed_manager_val)),
        moderator_manager(El::Python::add_ref(moderator_manager_val)),
        customer_manager(El::Python::add_ref(customer_manager_val)),
        ad_manager(El::Python::add_ref(ad_manager_val)),
        moderator_manager_ref_(
          ::NewsGate::Moderation::ModeratorManager::_duplicate(mod_manager)),
        customer_moderating_(customer_moderating)
  {
    if(moderator_manager.in())
    {
      moderator_manager->session_id(session);
    }

    const Moderation::GrantedPrivilegeSeq& priv = moderator_info.privileges;
      
    for(unsigned long i = 0; i < priv.length(); i++)
    {
      const Moderation::GrantedPrivilege& p = priv[i];
      
      ModeratorModeration::GrantedPrivilege_var gp =
        new ModeratorModeration::GrantedPrivilege();
      
      gp->priv->id = p.priv.id;
      gp->priv->args = p.priv.args;
      gp->name = p.name.in();
      gp->granted_by = p.granted_by.in();
      
      privileges->push_back(gp);
    }      
  }

  PyObject*
  Moderator::py_is_customer() throw(El::Exception)
  {
    if(customer_moderating_)
    {
      for(unsigned long i = 0; i < privileges->size(); i++)
      {
        ModeratorModeration::GrantedPrivilege* gp =
          ModeratorModeration::GrantedPrivilege::Type::down_cast(
            (*privileges)[i].in());
        
        if(gp->priv->id == Moderation::PV_CUSTOMER)
        {
          return El::Python::add_ref(Py_True);
        }
      }
    }
    
    return El::Python::add_ref(Py_False);
  }
  
  PyObject*
  Moderator::py_has_privilege(PyObject* args) throw(El::Exception)
  {
    unsigned long priv = 0;
    
    if(!PyArg_ParseTuple(args,
                         "k:newsgate.moderation.Moderator.has_privilege",
                         &priv))
    {
      El::Python::handle_error(
        "NewsGate::Moderator::py_has_privilege");
    }

    for(unsigned long i = 0; i < privileges->size(); i++)
    {
      ModeratorModeration::GrantedPrivilege* gp =
        ModeratorModeration::GrantedPrivilege::Type::down_cast(
          (*privileges)[i].in());
        
      if(gp->priv->id == priv)
      {
        return El::Python::add_ref(Py_True);
      }
    }
    
    return El::Python::add_ref(Py_False);
  }

}
