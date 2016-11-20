/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Frontends/Moderation/AdModeration.cpp
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

#include <Services/Moderator/Commons/AdManager.hpp>

#include "AdModeration.hpp"
#include "Moderation.hpp"

namespace NewsGate
{
  namespace AdModeration
  {
    Manager::Type Manager::Type::instance;
    Page::Type Page::Type::instance;
    Global::Type Global::Type::instance;
    Slot::Type Slot::Type::instance;
    Size::Type Size::Type::instance;
    PageUpdate::Type PageUpdate::Type::instance;
    GlobalUpdate::Type GlobalUpdate::Type::instance;
    SlotUpdate::Type SlotUpdate::Type::instance;
    SizeUpdate::Type SizeUpdate::Type::instance;
    Advert::Type Advert::Type::instance;
    AdvertUpdate::Type AdvertUpdate::Type::instance;
    Counter::Type Counter::Type::instance;
    CounterUpdate::Type CounterUpdate::Type::instance;
    Condition::Type Condition::Type::instance;
    ConditionUpdate::Type ConditionUpdate::Type::instance;
    PageAdvertiserInfo::Type PageAdvertiserInfo::Type::instance;
    PageAdvertiserInfoUpdate::Type PageAdvertiserInfoUpdate::Type::instance;
    Campaign::Type Campaign::Type::instance;
    CampaignUpdate::Type CampaignUpdate::Type::instance;
    Group::Type Group::Type::instance;
    GroupUpdate::Type GroupUpdate::Type::instance;
    Placement::Type Placement::Type::instance;
    PlacementUpdate::Type PlacementUpdate::Type::instance;
    CounterPlacement::Type CounterPlacement::Type::instance;
    CounterPlacementUpdate::Type CounterPlacementUpdate::Type::instance;
    AdvertiserMaxAdNum::Type AdvertiserMaxAdNum::Type::instance;
    
    //
    // NewsGate::AdModeration::AdModerationPyModule class
    //
    class AdModerationPyModule :
      public El::Python::ModuleImpl<AdModerationPyModule>
    {
    public:
      static AdModerationPyModule instance;

      AdModerationPyModule() throw(El::Exception);
      
      virtual void initialized() throw(El::Exception);

      El::Python::Object_var object_not_found_ex;
      El::Python::Object_var object_already_exist_ex;
   };
  
    AdModerationPyModule AdModerationPyModule::instance;
  
    AdModerationPyModule::AdModerationPyModule()
      throw(El::Exception)
        : El::Python::ModuleImpl<AdModerationPyModule>(
        "newsgate.moderation.ad",
        "Module containing Ad Moderation types.",
        true)
    {
    }

    void
    AdModerationPyModule::initialized() throw(El::Exception)
    {
      object_not_found_ex = create_exception("ObjectNotFound");
      object_already_exist_ex = create_exception("ObjectAlreadyExist");
//      invalid_object_ex = create_exception("InvalidObject");

      add_member(PyLong_FromLong(Moderation::Ad::RS_ENABLED), "RS_ENABLED");
      add_member(PyLong_FromLong(Moderation::Ad::RS_DISABLED), "RS_DISABLED");
      add_member(PyLong_FromLong(Moderation::Ad::PS_ENABLED), "PS_ENABLED");
      add_member(PyLong_FromLong(Moderation::Ad::PS_DISABLED), "PS_DISABLED");
      add_member(PyLong_FromLong(Moderation::Ad::SS_ENABLED), "SS_ENABLED");
      add_member(PyLong_FromLong(Moderation::Ad::SS_DISABLED), "SS_DISABLED");
      add_member(PyLong_FromLong(Moderation::Ad::ZS_ENABLED), "ZS_ENABLED");
      add_member(PyLong_FromLong(Moderation::Ad::ZS_DISABLED), "ZS_DISABLED");
      add_member(PyLong_FromLong(Moderation::Ad::AS_ENABLED), "AS_ENABLED");
      add_member(PyLong_FromLong(Moderation::Ad::AS_DISABLED), "AS_DISABLED");
      add_member(PyLong_FromLong(Moderation::Ad::AS_DELETED), "AS_DELETED");
      add_member(PyLong_FromLong(Moderation::Ad::CS_ENABLED), "CS_ENABLED");
      add_member(PyLong_FromLong(Moderation::Ad::CS_DISABLED), "CS_DISABLED");
      add_member(PyLong_FromLong(Moderation::Ad::CS_DELETED), "CS_DELETED");
      add_member(PyLong_FromLong(Moderation::Ad::MS_ENABLED), "MS_ENABLED");
      add_member(PyLong_FromLong(Moderation::Ad::MS_DISABLED), "MS_DISABLED");
      add_member(PyLong_FromLong(Moderation::Ad::MS_DELETED), "MS_DELETED");

      add_member(PyLong_FromLong(Moderation::Ad::MAX_AD_NUM), "MAX_AD_NUM");
      
      add_member(PyLong_FromLong(Moderation::Ad::PU_AD_MANAGEMENT_INFO),
                 "PU_AD_MANAGEMENT_INFO");
      
      add_member(PyLong_FromLong(Moderation::Ad::PU_ADVERTISER_INFO),
                 "PU_ADVERTISER_INFO");

      add_member(PyLong_FromLong(Moderation::Ad::GS_ENABLED), "GS_ENABLED");
      add_member(PyLong_FromLong(Moderation::Ad::GS_DISABLED), "GS_DISABLED");
      add_member(PyLong_FromLong(Moderation::Ad::GS_DELETED), "GS_DELETED"); 

      add_member(PyLong_FromLong(Moderation::Ad::TS_ENABLED), "TS_ENABLED");
      add_member(PyLong_FromLong(Moderation::Ad::TS_DISABLED), "TS_DISABLED");
      add_member(PyLong_FromLong(Moderation::Ad::TS_DELETED), "TS_DELETED");

      add_member(PyLong_FromLong(Moderation::Ad::PI_DIRECT), "PI_DIRECT");
      add_member(PyLong_FromLong(Moderation::Ad::PI_FRAME), "PI_FRAME");

      add_member(PyLong_FromLong(Moderation::Ad::US_ENABLED), "US_ENABLED");
      add_member(PyLong_FromLong(Moderation::Ad::US_DISABLED), "US_DISABLED");
      add_member(PyLong_FromLong(Moderation::Ad::US_DELETED), "US_DELETED"); 

      add_member(PyLong_FromLong(Moderation::Ad::OS_ENABLED), "OS_ENABLED");
      add_member(PyLong_FromLong(Moderation::Ad::OS_DISABLED), "OS_DISABLED");
      add_member(PyLong_FromLong(Moderation::Ad::OS_DELETED), "OS_DELETED");
    }
    
    //
    // NewsGate::AdModeration::Manager class
    //
    
    Manager::Manager(PyTypeObject *type, PyObject *args, PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type)
    {
      throw Exception(
        "NewsGate::AdModeration::Manager::Manager: unforseen way "
        "of object creation");
    }
    
    PyObject*
    Manager::py_get_global(PyObject* args) throw(El::Exception)
    {
      unsigned long long advertiser_id = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "K:newsgate.moderation.ad.Manager.get_global",
           &advertiser_id))
      {
        El::Python::handle_error(
          "NewsGate::AdModeration::Manager::py_get_global");
      }

      try
      {
        Moderation::Ad::Global_var global;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          global = ad_manager->get_global(advertiser_id);
        }
        
        return new Global(*global);
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_get_global: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_get_global: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_get_global: "
        "unexpected execution path");
    }        

    PyObject*
    Manager::py_get_pages(PyObject* args) throw(El::Exception)
    {
      unsigned long long advertiser_id = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "K:newsgate.moderation.ad.Manager.get_pages",
           &advertiser_id))
      {
        El::Python::handle_error(
          "NewsGate::AdModeration::Manager::py_get_pages");
      }

      try
      {
        Moderation::Ad::PageSeq_var pages;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          pages = ad_manager->get_pages(advertiser_id);
        }

        El::Python::Sequence_var res = new El::Python::Sequence();
        res->reserve(pages->length());

        for(size_t i = 0; i < pages->length(); ++i)
        {
          res->push_back(new Page((*pages)[i]));
        }
        
        return res.retn();
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_get_pages: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_get_pages: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_get_pages: "
        "unexpected execution path");
    }        

    PyObject*
    Manager::py_get_page(PyObject* args) throw(El::Exception)
    {
      unsigned long id = 0;
      unsigned long long advertiser_id = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "kK:newsgate.moderation.ad.Manager.get_page",
           &id,
           &advertiser_id))
      {
        El::Python::handle_error(
          "NewsGate::AdModeration::Manager::py_get_page");
      }

      try
      {
        Moderation::Ad::Page_var page;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          page = ad_manager->get_page(id, advertiser_id);
        }
      
        return new Page(*page);
      }
      catch(const Moderation::ObjectNotFound& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_not_found_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_get_page: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_get_page: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_get_page: "
        "unexpected execution path");      
    }
    
    PyObject*
    Manager::py_get_sizes() throw(El::Exception)
    {
      try
      {
        Moderation::Ad::SizeSeq_var sizes;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          sizes = ad_manager->get_sizes();
        }

        El::Python::Sequence_var res = new El::Python::Sequence();
        res->reserve(sizes->length());

        for(size_t i = 0; i < sizes->length(); ++i)
        {
          res->push_back(new Size((*sizes)[i]));
        }
        
        return res.retn();
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_get_sizes: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_get_sizes: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_get_sizes: "
        "unexpected execution path");
    }        

    PyObject*
    Manager::py_get_conditions(PyObject* args) throw(El::Exception)
    {
      unsigned long long id = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "K:newsgate.moderation.ad.Manager.get_conditions",
           &id))
      {
        El::Python::handle_error(
          "NewsGate::AdModeration::Manager::py_get_conditions");
      }

      try
      {
        Moderation::Ad::ConditionSeq_var conditions;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          conditions = ad_manager->get_conditions(id);
        }

        El::Python::Sequence_var res = new El::Python::Sequence();
        res->reserve(conditions->length());

        for(size_t i = 0; i < conditions->length(); ++i)
        {
          res->push_back(new Condition((*conditions)[i]));
        }
        
        return res.retn();
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_get_conditions: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_get_conditions: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_get_conditions: "
        "unexpected execution path");
    }
    
    PyObject*
    Manager::py_get_condition(PyObject* args) throw(El::Exception)
    {
      unsigned long long id = 0;
      unsigned long long advertiser_id = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "KK:newsgate.moderation.ad.Manager.get_condition",
           &id,
           &advertiser_id))
      {
        El::Python::handle_error(
          "NewsGate::AdModeration::Manager::py_get_condition");
      }

      try
      {
        Moderation::Ad::Condition_var cond;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          cond = ad_manager->get_condition(id, advertiser_id);
        }
      
        return new Condition(*cond);
      }
      catch(const Moderation::ObjectNotFound& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_not_found_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_get_condition: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_get_condition: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_get_condition: "
        "unexpected execution path");      
    }
    
    PyObject*
    Manager::py_update_condition(PyObject* args) throw(El::Exception)
    {
      PyObject* update = 0;

      if(!PyArg_ParseTuple(
           args,
           "O:newsgate.moderation.ad.Manager.update_condition",
           &update))
      {
        El::Python::handle_error(
          "NewsGate::AdModeration::Manager::py_update_condition");
      }

      ::NewsGate::Moderation::Ad::ConditionUpdate u;
      ConditionUpdate::Type::down_cast(update, false)->init(u);
      
      try
      {
        Moderation::Ad::Condition_var cond;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          cond = ad_manager->update_condition(u);
        }
      
        return new Condition(*cond);
      }
      catch(const Moderation::ObjectNotFound& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_not_found_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ObjectAlreadyExist& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_already_exist_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_update_condition: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_update_condition: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_update_condition: "
        "unexpected execution path");      
    }
  
    PyObject*
    Manager::py_update_page(PyObject* args) throw(El::Exception)
    {
      PyObject* update = 0;

      if(!PyArg_ParseTuple(
           args,
           "O:newsgate.moderation.ad.Manager.update_page",
           &update))
      {
        El::Python::handle_error(
          "NewsGate::AdModeration::Manager::py_update_page");
      }

      ::NewsGate::Moderation::Ad::PageUpdate pu;
      PageUpdate::Type::down_cast(update, false)->init(pu);
      
      try
      {
        Moderation::Ad::Page_var page;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          page = ad_manager->update_page(pu);
        }
      
        return new Page(*page);
      }
      catch(const Moderation::ObjectNotFound& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_not_found_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_update_page: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_update_page: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_update_page: "
        "unexpected execution path");      
    }
  
    PyObject*
    Manager::py_update_global(PyObject* args) throw(El::Exception)
    {
      PyObject* update = 0;

      if(!PyArg_ParseTuple(
           args,
           "O:newsgate.moderation.ad.Manager.update_global",
           &update))
      {
        El::Python::handle_error(
          "NewsGate::AdModeration::Manager::py_update_global");
      }

      ::NewsGate::Moderation::Ad::GlobalUpdate su;
      GlobalUpdate::Type::down_cast(update, false)->init(su);
      
      try
      {
        Moderation::Ad::Global_var global;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          global = ad_manager->update_global(su);
        }
      
        return new Global(*global);
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_update_global: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_update_global: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_update_global: "
        "unexpected execution path");      
    }

    PyObject*
    Manager::py_update_sizes(PyObject* args) throw(El::Exception)
    {
      PyObject* update = 0;

      if(!PyArg_ParseTuple(
           args,
           "O:newsgate.moderation.ad.Manager.update_sizes",
           &update))
      {
        El::Python::handle_error(
          "NewsGate::AdModeration::Manager::py_update_sizes");
      }

      ::NewsGate::Moderation::Ad::SizeUpdateSeq su;
      
      El::Python::Sequence* seq =
        El::Python::Sequence::Type::down_cast(update, false);
      
      su.length(seq->size());

      for(size_t i = 0; i < seq->size(); ++i)
      {
        SizeUpdate* update =
          SizeUpdate::Type::down_cast((*seq)[i].in(), false);

        update->init(su[i]);
      }
      
      try
      {
        Moderation::Ad::SizeSeq_var sizes;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          sizes = ad_manager->update_sizes(su);
        }

        El::Python::Sequence_var res = new El::Python::Sequence();
        res->resize(sizes->length());

        for(size_t i = 0; i < sizes->length(); ++i)
        {
          (*res)[i] = new Size((*sizes)[i]);
        }
      
        return res.retn();
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_update_sizes: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_update_sizes: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_update_sizes: "
        "unexpected execution path");      
    }

    PyObject*
    Manager::py_get_ads(PyObject* args) throw(El::Exception)
    {
      unsigned long long id = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "K:newsgate.moderation.ad.Manager.get_ads",
           &id))
      {
        El::Python::handle_error(
          "NewsGate::AdModeration::Manager::py_get_ads");
      }

      try
      {
        Moderation::Ad::AdvertSeq_var ads;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          ads = ad_manager->get_ads(id);
        }

        El::Python::Sequence_var res = new El::Python::Sequence();
        res->reserve(ads->length());

        for(size_t i = 0; i < ads->length(); ++i)
        {
          res->push_back(new Advert((*ads)[i]));
        }
        
        return res.retn();
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_get_ads: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_get_ads: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_get_ads: "
        "unexpected execution path");
    }
    
    PyObject*
    Manager::py_get_ad(PyObject* args) throw(El::Exception)
    {
      unsigned long long id = 0;
      unsigned long long advertiser_id = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "KK:newsgate.moderation.ad.Manager.get_ad",
           &id,
           &advertiser_id))
      {
        El::Python::handle_error("NewsGate::AdModeration::Manager::py_get_ad");
      }

      try
      {
        Moderation::Ad::Advert_var ad;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          ad = ad_manager->get_ad(id, advertiser_id);
        }
      
        return new Advert(*ad);
      }
      catch(const Moderation::ObjectNotFound& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_not_found_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_get_ad: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_get_ad: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_get_ad: "
        "unexpected execution path");      
    }
    
    PyObject*
    Manager::py_update_ad(PyObject* args) throw(El::Exception)
    {
      PyObject* update = 0;

      if(!PyArg_ParseTuple(
           args,
           "O:newsgate.moderation.ad.Manager.update_ad",
           &update))
      {
        El::Python::handle_error(
          "NewsGate::AdModeration::Manager::py_update_ad");
      }

      ::NewsGate::Moderation::Ad::AdvertUpdate au;
      AdvertUpdate::Type::down_cast(update, false)->init(au);
      
      try
      {
        Moderation::Ad::Advert_var ad;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          ad = ad_manager->update_ad(au);
        }
      
        return new Advert(*ad);
      }
      catch(const Moderation::ObjectNotFound& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_not_found_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ObjectAlreadyExist& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_already_exist_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_update_ad: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_update_ad: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_update_ad: "
        "unexpected execution path");      
    }
    
    PyObject*
    Manager::py_create_ad(PyObject* args) throw(El::Exception)
    {
      PyObject* ad = 0;

      if(!PyArg_ParseTuple(
           args,
           "O:newsgate.moderation.ad.Manager.create_ad",
           &ad))
      {
        El::Python::handle_error(
          "NewsGate::AdModeration::Manager::py_create_ad");
      }

      ::NewsGate::Moderation::Ad::Advert advert;
      Advert::Type::down_cast(ad, false)->init(advert);
      
      try
      {
        Moderation::Ad::Advert_var ad;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          ad = ad_manager->create_ad(advert);
        }
      
        return new Advert(*ad);
      }
      catch(const Moderation::ObjectNotFound& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_not_found_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ObjectAlreadyExist& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_already_exist_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_create_ad: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_create_ad: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_create_ad: "
        "unexpected execution path");      
    }

    PyObject*
    Manager::py_get_counters(PyObject* args) throw(El::Exception)
    {
      unsigned long long id = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "K:newsgate.moderation.ad.Manager.get_counters",
           &id))
      {
        El::Python::handle_error(
          "NewsGate::AdModeration::Manager::py_get_counters");
      }

      try
      {
        Moderation::Ad::CounterSeq_var counters;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          counters = ad_manager->get_counters(id);
        }

        El::Python::Sequence_var res = new El::Python::Sequence();
        res->reserve(counters->length());

        for(size_t i = 0; i < counters->length(); ++i)
        {
          res->push_back(new Counter((*counters)[i]));
        }
        
        return res.retn();
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_get_counters: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_get_counters: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_get_counters: "
        "unexpected execution path");
    }
    
    PyObject*
    Manager::py_get_counter(PyObject* args) throw(El::Exception)
    {
      unsigned long long id = 0;
      unsigned long long advertiser_id = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "KK:newsgate.moderation.ad.Manager.get_counter",
           &id,
           &advertiser_id))
      {
        El::Python::handle_error(
          "NewsGate::AdModeration::Manager::py_get_counter");
      }

      try
      {
        Moderation::Ad::Counter_var counter;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          counter = ad_manager->get_counter(id, advertiser_id);
        }
      
        return new Counter(*counter);
      }
      catch(const Moderation::ObjectNotFound& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_not_found_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_get_counter: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_get_counter: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_get_counter: "
        "unexpected execution path");      
    }
    
    PyObject*
    Manager::py_update_counter(PyObject* args) throw(El::Exception)
    {
      PyObject* update = 0;

      if(!PyArg_ParseTuple(
           args,
           "O:newsgate.moderation.ad.Manager.update_counter",
           &update))
      {
        El::Python::handle_error(
          "NewsGate::AdModeration::Manager::py_update_counter");
      }

      ::NewsGate::Moderation::Ad::CounterUpdate cu;
      CounterUpdate::Type::down_cast(update, false)->init(cu);
      
      try
      {
        Moderation::Ad::Counter_var counter;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          counter = ad_manager->update_counter(cu);
        }
      
        return new Counter(*counter);
      }
      catch(const Moderation::ObjectNotFound& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_not_found_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ObjectAlreadyExist& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_already_exist_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_update_counter: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_update_counter: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_update_counter: "
        "unexpected execution path");      
    }
    
    PyObject*
    Manager::py_create_counter(PyObject* args) throw(El::Exception)
    {
      PyObject* cn = 0;

      if(!PyArg_ParseTuple(
           args,
           "O:newsgate.moderation.ad.Manager.create_counter",
           &cn))
      {
        El::Python::handle_error(
          "NewsGate::AdModeration::Manager::py_create_counter");
      }

      ::NewsGate::Moderation::Ad::Counter cntr;
      Counter::Type::down_cast(cn, false)->init(cntr);
      
      try
      {
        Moderation::Ad::Counter_var counter;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          counter = ad_manager->create_counter(cntr);
        }
      
        return new Counter(*counter);
      }
      catch(const Moderation::ObjectNotFound& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_not_found_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ObjectAlreadyExist& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_already_exist_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_create_counter: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_create_counter: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_create_counter: "
        "unexpected execution path");      
    }
    
    PyObject*
    Manager::py_create_condition(PyObject* args) throw(El::Exception)
    {
      PyObject* cond = 0;

      if(!PyArg_ParseTuple(
           args,
           "O:newsgate.moderation.ad.Manager.create_condition",
           &cond))
      {
        El::Python::handle_error(
          "NewsGate::AdModeration::Manager::py_create_condition");
      }

      ::NewsGate::Moderation::Ad::Condition condition;
      Condition::Type::down_cast(cond, false)->init(condition);
      
      try
      {
        Moderation::Ad::Condition_var cond;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          cond = ad_manager->create_condition(condition);
        }
      
        return new Condition(*cond);
      }
      catch(const Moderation::ObjectNotFound& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_not_found_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ObjectAlreadyExist& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_already_exist_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_create_condition: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_create_condition: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_create_condition: "
        "unexpected execution path");      
    }
    
    PyObject*
    Manager::py_get_campaigns(PyObject* args) throw(El::Exception)
    {
      unsigned long long id = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "K:newsgate.moderation.ad.Manager.get_campaigns",
           &id))
      {
        El::Python::handle_error(
          "NewsGate::AdModeration::Manager::py_get_campaigns");
      }

      try
      {
        Moderation::Ad::CampaignSeq_var campaigns;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          campaigns = ad_manager->get_campaigns(id);
        }

        El::Python::Sequence_var res = new El::Python::Sequence();
        res->reserve(campaigns->length());

        for(size_t i = 0; i < campaigns->length(); ++i)
        {
          res->push_back(new Campaign((*campaigns)[i]));
        }
        
        return res.retn();
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_get_campaigns: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_get_campaigns: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_get_campaigns: "
        "unexpected execution path");
    }
    
    PyObject*
    Manager::py_get_campaign(PyObject* args) throw(El::Exception)
    {
      unsigned long long id = 0;
      unsigned long long advertiser_id = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "KK:newsgate.moderation.ad.Manager.get_campaign",
           &id,
           &advertiser_id))
      {
        El::Python::handle_error(
          "NewsGate::AdModeration::Manager::py_get_campaign");
      }

      try
      {
        Moderation::Ad::Campaign_var cmp;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          cmp = ad_manager->get_campaign(id, advertiser_id);
        }
      
        return new Campaign(*cmp);
      }
      catch(const Moderation::ObjectNotFound& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_not_found_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_get_campaign: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_get_campaign: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_get_campaign: "
        "unexpected execution path");      
    }
    
    PyObject*
    Manager::py_update_campaign(PyObject* args) throw(El::Exception)
    {
      PyObject* update = 0;

      if(!PyArg_ParseTuple(
           args,
           "O:newsgate.moderation.ad.Manager.update_campaign",
           &update))
      {
        El::Python::handle_error(
          "NewsGate::AdModeration::Manager::py_update_campaign");
      }

      ::NewsGate::Moderation::Ad::CampaignUpdate u;
      CampaignUpdate::Type::down_cast(update, false)->init(u);
      
      try
      {
        Moderation::Ad::Campaign_var cmp;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          cmp = ad_manager->update_campaign(u);
        }
      
        return new Campaign(*cmp);
      }
      catch(const Moderation::ObjectNotFound& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_not_found_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ObjectAlreadyExist& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_already_exist_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_update_campaign: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_update_campaign: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_update_campaign: "
        "unexpected execution path");      
    }
  
    PyObject*
    Manager::py_create_campaign(PyObject* args) throw(El::Exception)
    {
      PyObject* cond = 0;

      if(!PyArg_ParseTuple(
           args,
           "O:newsgate.moderation.ad.Manager.create_campaign",
           &cond))
      {
        El::Python::handle_error(
          "NewsGate::AdModeration::Manager::py_create_campaign");
      }

      ::NewsGate::Moderation::Ad::Campaign campaign;
      Campaign::Type::down_cast(cond, false)->init(campaign);
      
      try
      {
        Moderation::Ad::Campaign_var cmp;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          cmp = ad_manager->create_campaign(campaign);
        }
      
        return new Campaign(*cmp);
      }
      catch(const Moderation::ObjectNotFound& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_not_found_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ObjectAlreadyExist& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_already_exist_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_create_campaign: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_create_campaign: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_create_campaign: "
        "unexpected execution path");      
    }
    
    PyObject*
    Manager::py_get_placement(PyObject* args) throw(El::Exception)
    {
      unsigned long long id = 0;
      unsigned long long advertiser_id = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "KK:newsgate.moderation.ad.Manager.get_placement",
           &id,
           &advertiser_id))
      {
        El::Python::handle_error(
          "NewsGate::AdModeration::Manager::py_get_placement");
      }

      try
      {
        Moderation::Ad::Placement_var plc;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          plc = ad_manager->get_placement(id, advertiser_id);
        }
      
        return new Placement(*plc);
      }
      catch(const Moderation::ObjectNotFound& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_not_found_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_get_placement: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_get_placement: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_get_placement: "
        "unexpected execution path");      
    }
    
    PyObject*
    Manager::py_update_placement(PyObject* args) throw(El::Exception)
    {
      PyObject* update = 0;

      if(!PyArg_ParseTuple(
           args,
           "O:newsgate.moderation.ad.Manager.update_placement",
           &update))
      {
        El::Python::handle_error(
          "NewsGate::AdModeration::Manager::py_update_placement");
      }

      ::NewsGate::Moderation::Ad::PlacementUpdate u;
      PlacementUpdate::Type::down_cast(update, false)->init(u);
      
      try
      {
        Moderation::Ad::Placement_var plc;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          plc = ad_manager->update_placement(u);
        }
      
        return new Placement(*plc);
      }
      catch(const Moderation::ObjectNotFound& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_not_found_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ObjectAlreadyExist& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_already_exist_ex.in(),
          e.reason.in());
      }
/*      
      catch(const Moderation::InvalidObject& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.invalid_object_ex.in(),
          e.reason.in());
      }
*/
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_update_placement: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_update_placement: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_update_placement: "
        "unexpected execution path");      
    }
  
    PyObject*
    Manager::py_create_placement(PyObject* args) throw(El::Exception)
    {
      PyObject* pl = 0;

      if(!PyArg_ParseTuple(
           args,
           "O:newsgate.moderation.ad.Manager.create_placement",
           &pl))
      {
        El::Python::handle_error(
          "NewsGate::AdModeration::Manager::py_create_placement");
      }

      ::NewsGate::Moderation::Ad::Placement placement;
      Placement::Type::down_cast(pl, false)->init(placement);
      
      try
      {
        Moderation::Ad::Placement_var plc;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          plc = ad_manager->create_placement(placement);
        }
      
        return new Placement(*plc);
      }
      catch(const Moderation::ObjectNotFound& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_not_found_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ObjectAlreadyExist& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_already_exist_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_create_placement: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_create_placement: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_create_placement: "
        "unexpected execution path");      
    }
    
    PyObject*
    Manager::py_get_counter_placement(PyObject* args) throw(El::Exception)
    {
      unsigned long long id = 0;
      unsigned long long advertiser_id = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "KK:newsgate.moderation.ad.Manager.get_counter_placement",
           &id,
           &advertiser_id))
      {
        El::Python::handle_error(
          "NewsGate::AdModeration::Manager::py_get_counter_placement");
      }

      try
      {
        Moderation::Ad::CounterPlacement_var plc;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          plc = ad_manager->get_counter_placement(id, advertiser_id);
        }
      
        return new CounterPlacement(*plc);
      }
      catch(const Moderation::ObjectNotFound& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_not_found_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_get_counter_placement: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_get_counter_placement: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_get_counter_placement: "
        "unexpected execution path");      
    }
    
    PyObject*
    Manager::py_update_counter_placement(PyObject* args) throw(El::Exception)
    {
      PyObject* update = 0;

      if(!PyArg_ParseTuple(
           args,
           "O:newsgate.moderation.ad.Manager.update_counter_placement",
           &update))
      {
        El::Python::handle_error(
          "NewsGate::AdModeration::Manager::py_update_counter_placement");
      }

      ::NewsGate::Moderation::Ad::CounterPlacementUpdate u;
      CounterPlacementUpdate::Type::down_cast(update, false)->init(u);
      
      try
      {
        Moderation::Ad::CounterPlacement_var plc;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          plc = ad_manager->update_counter_placement(u);
        }
      
        return new CounterPlacement(*plc);
      }
      catch(const Moderation::ObjectNotFound& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_not_found_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ObjectAlreadyExist& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_already_exist_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::"
          "py_update_counter_placement: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::"
          "py_update_counter_placement: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_update_counter_placement: "
        "unexpected execution path");      
    }
  
    PyObject*
    Manager::py_create_counter_placement(PyObject* args) throw(El::Exception)
    {
      PyObject* pl = 0;

      if(!PyArg_ParseTuple(
           args,
           "O:newsgate.moderation.ad.Manager.create_counter_placement",
           &pl))
      {
        El::Python::handle_error(
          "NewsGate::AdModeration::Manager::py_create_counter_placement");
      }

      ::NewsGate::Moderation::Ad::CounterPlacement placement;
      CounterPlacement::Type::down_cast(pl, false)->init(placement);
      
      try
      {
        Moderation::Ad::CounterPlacement_var plc;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          plc = ad_manager->create_counter_placement(placement);
        }
      
        return new CounterPlacement(*plc);
      }
      catch(const Moderation::ObjectNotFound& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_not_found_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ObjectAlreadyExist& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_already_exist_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::"
          "py_create_counter_placement: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::"
          "py_create_counter_placement: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_create_counter_placement: "
        "unexpected execution path");
    }

    PyObject*
    Manager::py_get_group(PyObject* args) throw(El::Exception)
    {
      unsigned long long id = 0;
      unsigned long long advertiser_id = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "KK:newsgate.moderation.ad.Manager.get_group",
           &id,
           &advertiser_id))
      {
        El::Python::handle_error(
          "NewsGate::AdModeration::Manager::py_get_group");
      }

      try
      {
        Moderation::Ad::Group_var grp;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          grp = ad_manager->get_group(id, advertiser_id);
        }
      
        return new Group(*grp);
      }
      catch(const Moderation::ObjectNotFound& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_not_found_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_get_group: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_get_group: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_get_group: "
        "unexpected execution path");      
    }
    
    PyObject*
    Manager::py_update_group(PyObject* args) throw(El::Exception)
    {
      PyObject* update = 0;

      if(!PyArg_ParseTuple(
           args,
           "O:newsgate.moderation.ad.Manager.update_group",
           &update))
      {
        El::Python::handle_error(
          "NewsGate::AdModeration::Manager::py_update_group");
      }

      ::NewsGate::Moderation::Ad::GroupUpdate u;
      GroupUpdate::Type::down_cast(update, false)->init(u);
      
      try
      {
        Moderation::Ad::Group_var grp;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          grp = ad_manager->update_group(u);
        }
      
        return new Group(*grp);
      }
      catch(const Moderation::ObjectNotFound& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_not_found_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ObjectAlreadyExist& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_already_exist_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_update_group: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_update_group: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_update_group: "
        "unexpected execution path");      
    }
  
    PyObject*
    Manager::py_create_group(PyObject* args) throw(El::Exception)
    {
      PyObject* gr = 0;

      if(!PyArg_ParseTuple(
           args,
           "O:newsgate.moderation.ad.Manager.create_group",
           &gr))
      {
        El::Python::handle_error(
          "NewsGate::AdModeration::Manager::py_create_group");
      }

      ::NewsGate::Moderation::Ad::Group group;
      Group::Type::down_cast(gr, false)->init(group);
      
      try
      {
        Moderation::Ad::Group_var grp;
      
        {
          El::Python::AllowOtherThreads guard;

          NewsGate::Moderation::Ad::AdManager_var ad_manager =
            manager_.object();
          
          grp = ad_manager->create_group(group);
        }
      
        return new Group(*grp);
      }
      catch(const Moderation::ObjectNotFound& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_not_found_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ObjectAlreadyExist& e)
      {
        El::Python::report_error(
          AdModerationPyModule::instance.object_already_exist_ex.in(),
          e.reason.in());
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_create_group: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::AdModeration::Manager::py_create_group: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    
      throw Exception(
        "NewsGate::AdModeration::Manager::py_create_group: "
        "unexpected execution path");      
    }
  }
}
