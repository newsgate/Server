/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Frontends/Moderation/AdModeration.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_FRONTENDS_MODERATION_ADMODERATION_HPP_
#define _NEWSGATE_SERVER_FRONTENDS_MODERATION_ADMODERATION_HPP_

#include <string>

#include <ace/Synch.h>
#include <ace/Guard_T.h>

#include <El/Exception.hpp>

#include <El/Python/Exception.hpp>
#include <El/Python/Object.hpp>
#include <El/Python/RefCount.hpp>
#include <El/Python/Logger.hpp>

#include <Services/Moderator/Commons/AdManager.hpp>

namespace NewsGate
{  
  namespace AdModeration
  {
    EL_EXCEPTION(Exception, El::ExceptionBase);

    typedef El::Corba::SmartRef<NewsGate::Moderation::Ad::AdManager>
    ManagerRef;    
    
    class Manager : public El::Python::ObjectImpl
    {
    public:
      Manager(PyTypeObject *type, PyObject *args, PyObject *kwds)
        throw(Exception, El::Exception);      
    
      Manager(const ManagerRef& manager_ref) throw(Exception, El::Exception);

      virtual ~Manager() throw() {}

      PyObject* py_get_global(PyObject* args) throw(El::Exception);
      PyObject* py_get_pages(PyObject* args) throw(El::Exception);
      PyObject* py_get_page(PyObject* args) throw(El::Exception);
      PyObject* py_get_sizes() throw(El::Exception);
      PyObject* py_update_page(PyObject* args) throw(El::Exception);
      PyObject* py_update_global(PyObject* args) throw(El::Exception);
      PyObject* py_update_sizes(PyObject* args) throw(El::Exception);
      PyObject* py_get_ads(PyObject* args) throw(El::Exception);
      PyObject* py_get_ad(PyObject* args) throw(El::Exception);
      PyObject* py_update_ad(PyObject* args) throw(El::Exception);
      PyObject* py_create_ad(PyObject* args) throw(El::Exception);
      PyObject* py_get_counters(PyObject* args) throw(El::Exception);
      PyObject* py_get_counter(PyObject* args) throw(El::Exception);
      PyObject* py_update_counter(PyObject* args) throw(El::Exception);
      PyObject* py_create_counter(PyObject* args) throw(El::Exception);
      PyObject* py_get_conditions(PyObject* args) throw(El::Exception);
      PyObject* py_get_condition(PyObject* args) throw(El::Exception);
      PyObject* py_update_condition(PyObject* args) throw(El::Exception);
      PyObject* py_create_condition(PyObject* args) throw(El::Exception);
      PyObject* py_get_campaigns(PyObject* args) throw(El::Exception);
      PyObject* py_get_campaign(PyObject* args) throw(El::Exception);
      PyObject* py_update_campaign(PyObject* args) throw(El::Exception);
      PyObject* py_create_campaign(PyObject* args) throw(El::Exception);
      PyObject* py_get_group(PyObject* args) throw(El::Exception);
      PyObject* py_update_group(PyObject* args) throw(El::Exception);
      PyObject* py_create_group(PyObject* args) throw(El::Exception);
      PyObject* py_get_placement(PyObject* args) throw(El::Exception);
      PyObject* py_update_placement(PyObject* args) throw(El::Exception);
      PyObject* py_create_placement(PyObject* args) throw(El::Exception);
      PyObject* py_get_counter_placement(PyObject* args) throw(El::Exception);
      
      PyObject* py_update_counter_placement(PyObject* args)
        throw(El::Exception);
      
      PyObject* py_create_counter_placement(PyObject* args)
        throw(El::Exception);

      class Type : public El::Python::ObjectTypeImpl<Manager, Manager::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_METHOD_VARARGS(py_get_global,
                               "get_global",
                               "Gets ad global information");

        PY_TYPE_METHOD_VARARGS(py_get_pages,
                               "get_pages",
                               "Gets ad pages information");

        PY_TYPE_METHOD_NOARGS(py_get_sizes,
                               "get_sizes",
                               "Gets ad sizes information");

        PY_TYPE_METHOD_VARARGS(py_get_page,
                               "get_page",
                               "Gets page information");

        PY_TYPE_METHOD_VARARGS(py_update_page,
                               "update_page",
                               "Updates page information");
        
        PY_TYPE_METHOD_VARARGS(py_update_global,
                               "update_global",
                               "Updates global information");
        
        PY_TYPE_METHOD_VARARGS(py_update_sizes,
                               "update_sizes",
                               "Updates sizes information");
        
        PY_TYPE_METHOD_VARARGS(py_get_ads, "get_ads", "Get ads");
        PY_TYPE_METHOD_VARARGS(py_get_ad, "get_ad", "Get ad");
        PY_TYPE_METHOD_VARARGS(py_update_ad, "update_ad", "Update ad");
        PY_TYPE_METHOD_VARARGS(py_create_ad, "create_ad", "Create ad");

        PY_TYPE_METHOD_VARARGS(py_get_counters,"get_counters", "Get counters");
        PY_TYPE_METHOD_VARARGS(py_get_counter, "get_counter", "Get counter");
        
        PY_TYPE_METHOD_VARARGS(py_update_counter,
                               "update_counter",
                               "Update counter");
        
        PY_TYPE_METHOD_VARARGS(py_create_counter,
                               "create_counter",
                               "Create counter");

        PY_TYPE_METHOD_VARARGS(py_get_conditions,
                               "get_conditions",
                               "Get conditions");
        
        PY_TYPE_METHOD_VARARGS(py_get_condition,
                               "get_condition",
                               "Get condition");
        
        PY_TYPE_METHOD_VARARGS(py_update_condition,
                               "update_condition",
                               "Update condition");
        
        PY_TYPE_METHOD_VARARGS(py_create_condition,
                               "create_condition",
                               "Create condition");

        PY_TYPE_METHOD_VARARGS(py_get_campaigns,
                               "get_campaigns",
                               "Get campaigns");
        
        PY_TYPE_METHOD_VARARGS(py_get_campaign,
                               "get_campaign",
                               "Get campaign");
        
        PY_TYPE_METHOD_VARARGS(py_update_campaign,
                               "update_campaign",
                               "Update campaign");
        
        PY_TYPE_METHOD_VARARGS(py_create_campaign,
                               "create_campaign",
                               "Create campaign");
        
        PY_TYPE_METHOD_VARARGS(py_get_group, "get_group", "Get group");
        PY_TYPE_METHOD_VARARGS(py_update_group, "update_group","Update group");
        PY_TYPE_METHOD_VARARGS(py_create_group, "create_group","Create group");
        
        PY_TYPE_METHOD_VARARGS(py_get_placement,
                               "get_placement",
                               "Get placement");
        
        PY_TYPE_METHOD_VARARGS(py_update_placement,
                               "update_placement",
                               "Update placement");
        
        PY_TYPE_METHOD_VARARGS(py_create_placement,
                               "create_placement",
                               "Create placement");
        
        PY_TYPE_METHOD_VARARGS(py_get_counter_placement,
                               "get_counter_placement",
                               "Get counter placement");
        
        PY_TYPE_METHOD_VARARGS(py_update_counter_placement,
                               "update_counter_placement",
                               "Update counter placement");
        
        PY_TYPE_METHOD_VARARGS(py_create_counter_placement,
                               "create_counter_placement",
                               "Create counter placement");
      };

    private:
      ManagerRef manager_;
    };

    typedef El::Python::SmartPtr<Manager> Manager_var;

    class Global : public El::Python::ObjectImpl
    {
    public:
      Moderation::Ad::SelectorStatus selector_status;
      unsigned long adv_max_ads_per_page;
      unsigned long long update_number;
      double pcws_reduction_rate;
      unsigned long pcws_weight_zones;
    
    public:
      Global(PyTypeObject *type = 0,
             PyObject *args = 0,
             PyObject *kwds = 0)
        throw(Exception, El::Exception);

      Global(const ::NewsGate::Moderation::Ad::Global& selector)
        throw(El::Exception);
    
      virtual ~Global() throw() {}

      class Type : public El::Python::ObjectTypeImpl<Global,
                                                     Global::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ENUM(selector_status,
                            Moderation::Ad::SelectorStatus,
                            Moderation::Ad::RS_COUNT - 1,
                            "selector_status",
                            "Ad selector status");
        
        PY_TYPE_MEMBER_ULONG(adv_max_ads_per_page,
                             "adv_max_ads_per_page",
                             "Advertiser max ads per page count");
        
        PY_TYPE_MEMBER_ULONGLONG(update_number,
                                 "update_number",
                                 "Global update number");

        PY_TYPE_MEMBER_FLOAT(pcws_reduction_rate,
                             "pcws_reduction_rate",
                             "PCWS reduction rate");

        PY_TYPE_MEMBER_ULONG(pcws_weight_zones,
                             "pcws_weight_zones",
                             "PCWS weight zones");
      };
    };

    typedef El::Python::SmartPtr<Global> Global_var;

    class GlobalUpdate : public El::Python::ObjectImpl
    {
    public:
      
      unsigned long flags;
      Moderation::Ad::AdvertiserId advertiser;
      Moderation::Ad::SelectorStatus selector_status;
      unsigned long adv_max_ads_per_page;
      double pcws_reduction_rate;
      unsigned long pcws_weight_zones;
    
    public:
      GlobalUpdate(PyTypeObject *type = 0,
                     PyObject *args = 0,
                     PyObject *kwds = 0)
        throw(Exception, El::Exception);

      void init(::NewsGate::Moderation::Ad::GlobalUpdate& update) const
        throw(El::Exception);
    
      virtual ~GlobalUpdate() throw() {}

      class Type : public El::Python::ObjectTypeImpl<GlobalUpdate,
                                                     GlobalUpdate::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONG(flags, "flags", "Update flags");
        
        PY_TYPE_MEMBER_ULONGLONG(advertiser,
                                 "advertiser",
                                 "Advertiser identifier");

        PY_TYPE_MEMBER_ENUM(selector_status,
                            Moderation::Ad::SelectorStatus,
                            Moderation::Ad::RS_COUNT - 1,
                            "selector_status",
                            "Ad selector status");

        PY_TYPE_MEMBER_ULONG(adv_max_ads_per_page,
                             "adv_max_ads_per_page",
                             "Advertiser max ads per page count");

        PY_TYPE_MEMBER_FLOAT(pcws_reduction_rate,
                             "pcws_reduction_rate",
                             "PCWS reduction rate");

        PY_TYPE_MEMBER_ULONG(pcws_weight_zones,
                             "pcws_weight_zones",
                             "PCWS weight zones");
      };
    };

    typedef El::Python::SmartPtr<GlobalUpdate> GlobalUpdate_var;
    
    class AdvertiserMaxAdNum : public El::Python::ObjectImpl
    {
    public:
      Moderation::Ad::AdvertiserId id;
      std::string name;
      unsigned long max_ad_num;
    
    public:
      AdvertiserMaxAdNum(PyTypeObject *type = 0,
                         PyObject *args = 0,
                         PyObject *kwds = 0)
        throw(Exception, El::Exception);

      AdvertiserMaxAdNum(
        const ::NewsGate::Moderation::Ad::AdvertiserMaxAdNum& info)
        throw(El::Exception);

      void init(::NewsGate::Moderation::Ad::AdvertiserMaxAdNum& dest) const
        throw(El::Exception);      
    
      virtual ~AdvertiserMaxAdNum() throw() {}

      class Type : public El::Python::ObjectTypeImpl<AdvertiserMaxAdNum,
                                                     AdvertiserMaxAdNum::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Advertiser identifier");
        PY_TYPE_MEMBER_STRING(name, "name", "Advertiser name", false);
        PY_TYPE_MEMBER_ULONG(max_ad_num, "max_ad_num", "Max ad number");
      };
    };

    typedef El::Python::SmartPtr<AdvertiserMaxAdNum> AdvertiserMaxAdNum_var;
    
    class PageAdvertiserInfo : public El::Python::ObjectImpl
    {
    public:
      Moderation::Ad::AdvertiserId id;
      unsigned long max_ad_num;
      El::Python::Sequence_var adv_max_ad_nums;
    
    public:
      PageAdvertiserInfo(PyTypeObject *type = 0,
                         PyObject *args = 0,
                         PyObject *kwds = 0)
        throw(Exception, El::Exception);

      PageAdvertiserInfo(
        const ::NewsGate::Moderation::Ad::PageAdvertiserInfo& info)
        throw(El::Exception);
    
      virtual ~PageAdvertiserInfo() throw() {}

      class Type : public El::Python::ObjectTypeImpl<PageAdvertiserInfo,
                                                     PageAdvertiserInfo::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Advertiser identifier");

        PY_TYPE_MEMBER_ULONG(max_ad_num, "max_ad_num", "Page max ad number");

        PY_TYPE_MEMBER_OBJECT(adv_max_ad_nums,
                              El::Python::Sequence::Type,
                              "adv_max_ad_nums",
                              "Advertiser max ads number",
                              false);
      };
    };

    typedef El::Python::SmartPtr<PageAdvertiserInfo> PageAdvertiserInfo_var;
    
    class PageAdvertiserInfoUpdate : public El::Python::ObjectImpl
    {
    public:
      Moderation::Ad::AdvertiserId id;
      unsigned long max_ad_num;
      El::Python::Sequence_var adv_max_ad_nums;
    
    public:
      PageAdvertiserInfoUpdate(PyTypeObject *type = 0,
                               PyObject *args = 0,
                               PyObject *kwds = 0)
        throw(Exception, El::Exception);

      void init(::NewsGate::Moderation::Ad::PageAdvertiserInfoUpdate& update)
        const throw(El::Exception);
    
      virtual ~PageAdvertiserInfoUpdate() throw() {}

      class Type : public El::Python::ObjectTypeImpl<
        PageAdvertiserInfoUpdate,
        PageAdvertiserInfoUpdate::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Advertiser identifier");
        PY_TYPE_MEMBER_ULONG(max_ad_num, "max_ad_num", "Page max ad number");

        PY_TYPE_MEMBER_OBJECT(adv_max_ad_nums,
                              El::Python::Sequence::Type,
                              "adv_max_ad_nums",
                              "Advertiser max ads number",
                              false);
      };
    };

    typedef El::Python::SmartPtr<PageAdvertiserInfoUpdate>
    PageAdvertiserInfoUpdate_var;    
    
    class Page : public El::Python::ObjectImpl
    {
    public:
      Moderation::Ad::PageId id;
      std::string name;
      Moderation::Ad::PageStatus status;
      unsigned long max_ad_num;
      El::Python::Sequence_var slots;
      PageAdvertiserInfo_var advertiser_info;
    
    public:
      Page(PyTypeObject *type = 0,
           PyObject *args = 0,
           PyObject *kwds = 0)
        throw(Exception, El::Exception);

      Page(const ::NewsGate::Moderation::Ad::Page& page)
        throw(El::Exception);
    
      virtual ~Page() throw() {}

//      void init(::NewsGate::Moderation::Ad::Page& page) const
//        throw(El::Exception);
      
      class Type : public El::Python::ObjectTypeImpl<Page,
                                                     Page::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Page identifier");

        PY_TYPE_MEMBER_STRING(name,
                              "name",
                              "Page name",
                              false);

        PY_TYPE_MEMBER_ENUM(status,
                            Moderation::Ad::PageStatus,
                            Moderation::Ad::PS_COUNT - 1,
                            "status",
                            "Ad page status");
        
        PY_TYPE_MEMBER_ULONG(max_ad_num, "max_ad_num", "Page max ad number");

        PY_TYPE_MEMBER_OBJECT(slots,
                              El::Python::Sequence::Type,
                              "slots",
                              "Slots",
                              false);
        
        PY_TYPE_MEMBER_OBJECT(advertiser_info,
                              PageAdvertiserInfo::Type,
                              "advertiser_info",
                              "Advertiser Info",
                              false);
        
      };
    };

    typedef El::Python::SmartPtr<Page> Page_var;

    class PageUpdate : public El::Python::ObjectImpl
    {
    public:
      unsigned long flags;
      Moderation::Ad::PageId id;
      Moderation::Ad::PageStatus status;
      unsigned long max_ad_num;
      El::Python::Sequence_var slot_updates;
      PageAdvertiserInfoUpdate_var advertiser_info_update;
    
    public:
      PageUpdate(PyTypeObject *type = 0,
                 PyObject *args = 0,
                 PyObject *kwds = 0)
        throw(Exception, El::Exception);

      void init(::NewsGate::Moderation::Ad::PageUpdate& update) const
        throw(El::Exception);
    
      virtual ~PageUpdate() throw() {}

      class Type : public El::Python::ObjectTypeImpl<PageUpdate,
                                                     PageUpdate::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONG(flags, "flags", "Update flags");

        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Page identifier");

        PY_TYPE_MEMBER_ENUM(status,
                            Moderation::Ad::PageStatus,
                            Moderation::Ad::PS_COUNT - 1,
                            "status",
                            "Ad page status");
        
        PY_TYPE_MEMBER_ULONG(max_ad_num, "max_ad_num", "Page max ad number");

        PY_TYPE_MEMBER_OBJECT(slot_updates,
                              El::Python::Sequence::Type,
                              "slot_updates",
                              "Slot updates",
                              false);
        
        PY_TYPE_MEMBER_OBJECT(advertiser_info_update,
                              PageAdvertiserInfoUpdate::Type,
                              "advertiser_info_update",
                              "Page advertier info updates",
                              false);        
      };
    };

    typedef El::Python::SmartPtr<PageUpdate> PageUpdate_var;
    
    class Slot : public El::Python::ObjectImpl
    {
    public:
      Moderation::Ad::SlotId id;
      std::string name;
      Moderation::Ad::SlotStatus status;
      unsigned long max_width;
      unsigned long min_width;
      unsigned long max_height;
      unsigned long min_height;
    
    public:
      Slot(PyTypeObject *type = 0,
           PyObject *args = 0,
           PyObject *kwds = 0)
        throw(Exception, El::Exception);

      Slot(const ::NewsGate::Moderation::Ad::Slot& slot)
        throw(El::Exception);
    
      virtual ~Slot() throw() {}

      void init(::NewsGate::Moderation::Ad::Slot& slot) const
        throw(El::Exception);
      
      class Type : public El::Python::ObjectTypeImpl<Slot,
                                                     Slot::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Slot identifier");

        PY_TYPE_MEMBER_STRING(name,
                              "name",
                              "Slot name",
                              false);

        PY_TYPE_MEMBER_ENUM(status,
                            Moderation::Ad::SlotStatus,
                            Moderation::Ad::SS_COUNT - 1,
                            "status",
                            "Ad slot status");
        
        PY_TYPE_MEMBER_ULONG(max_width, "max_width", "Slot max width");
        PY_TYPE_MEMBER_ULONG(min_width, "min_width", "Slot min width");
        PY_TYPE_MEMBER_ULONG(max_height, "max_height", "Slot max height");
        PY_TYPE_MEMBER_ULONG(min_height, "min_height", "Slot min height");
      };
    };

    typedef El::Python::SmartPtr<Slot> Slot_var;

    class SlotUpdate : public El::Python::ObjectImpl
    {
    public:
      Moderation::Ad::SlotId id;
      Moderation::Ad::SlotStatus status;
    
    public:
      SlotUpdate(PyTypeObject *type = 0,
                 PyObject *args = 0,
                 PyObject *kwds = 0)
        throw(Exception, El::Exception);

      void init(::NewsGate::Moderation::Ad::SlotUpdate& update) const
        throw(El::Exception);
    
      virtual ~SlotUpdate() throw() {}

      class Type : public El::Python::ObjectTypeImpl<SlotUpdate,
                                                     SlotUpdate::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Slot identifier");

        PY_TYPE_MEMBER_ENUM(status,
                            Moderation::Ad::SlotStatus,
                            Moderation::Ad::SS_COUNT - 1,
                            "status",
                            "Ad slot status");        
      };
    };

    typedef El::Python::SmartPtr<SlotUpdate> SlotUpdate_var;

    class Size : public El::Python::ObjectImpl
    {
    public:
      Moderation::Ad::SizeId id;
      std::string name;
      Moderation::Ad::SizeStatus status;
      unsigned long width;
      unsigned long height;
    
    public:
      Size(PyTypeObject *type = 0,
           PyObject *args = 0,
           PyObject *kwds = 0)
        throw(Exception, El::Exception);

      Size(const ::NewsGate::Moderation::Ad::Size& slot)
        throw(El::Exception);
    
      virtual ~Size() throw() {}

      class Type : public El::Python::ObjectTypeImpl<Size,
                                                     Size::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Size identifier");

        PY_TYPE_MEMBER_STRING(name,
                              "name",
                              "Size name",
                              false);

        PY_TYPE_MEMBER_ENUM(status,
                            Moderation::Ad::SizeStatus,
                            Moderation::Ad::ZS_COUNT - 1,
                            "status",
                            "Ad size status");
        
        PY_TYPE_MEMBER_ULONG(width, "width", "Size width");
        PY_TYPE_MEMBER_ULONG(height, "height", "Size height");
      };
    };

    typedef El::Python::SmartPtr<Size> Size_var;

    class SizeUpdate : public El::Python::ObjectImpl
    {
    public:
      Moderation::Ad::SizeId id;
      Moderation::Ad::SizeStatus status;
    
    public:
      SizeUpdate(PyTypeObject *type = 0,
                 PyObject *args = 0,
                 PyObject *kwds = 0)
        throw(Exception, El::Exception);

      void init(::NewsGate::Moderation::Ad::SizeUpdate& update) const
        throw(El::Exception);
    
      virtual ~SizeUpdate() throw() {}

      class Type : public El::Python::ObjectTypeImpl<SizeUpdate,
                                                     SizeUpdate::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Size identifier");

        PY_TYPE_MEMBER_ENUM(status,
                            Moderation::Ad::SizeStatus,
                            Moderation::Ad::ZS_COUNT - 1,
                            "status",
                            "Ad size status");
        
      };
    };

    typedef El::Python::SmartPtr<SizeUpdate> SizeUpdate_var;

    class Advert : public El::Python::ObjectImpl
    {
    public:
      Moderation::Ad::AdvertId id;
      Moderation::Ad::AdvertiserId advertiser;
      std::string name;
      Moderation::Ad::AdvertStatus status;
      Moderation::Ad::SizeId size;
      std::string size_name;
      unsigned long width;
      unsigned long height;
      std::string text;
      El::Python::Sequence_var placements;
    
    public:
      Advert(PyTypeObject *type = 0,
             PyObject *args = 0,
             PyObject *kwds = 0)
        throw(Exception, El::Exception);

      Advert(const ::NewsGate::Moderation::Ad::Advert& ad)
        throw(El::Exception);
    
      virtual ~Advert() throw() {}

      void init(::NewsGate::Moderation::Ad::Advert& ad) const
        throw(El::Exception);
      
      class Type : public El::Python::ObjectTypeImpl<Advert,
                                                     Advert::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Advert identifier");

        PY_TYPE_MEMBER_ULONGLONG(advertiser,
                                 "advertiser",
                                 "Advertiser identifier");

        PY_TYPE_MEMBER_STRING(name,
                              "name",
                              "Advert name",
                              false);

        PY_TYPE_MEMBER_STRING(size_name,
                              "size_name",
                              "Advert size name",
                              false);

        PY_TYPE_MEMBER_ENUM(status,
                            Moderation::Ad::AdvertStatus,
                            Moderation::Ad::AS_COUNT - 1,
                            "status",
                            "Advert status");
        
        PY_TYPE_MEMBER_ULONG(size, "size", "Advert size");
        PY_TYPE_MEMBER_ULONG(width, "width", "Advert width");
        PY_TYPE_MEMBER_ULONG(height, "height", "Advert height");

        PY_TYPE_MEMBER_STRING(text,
                              "text",
                              "Advert text",
                              true);

        PY_TYPE_MEMBER_OBJECT(placements,
                              El::Python::Sequence::Type,
                              "placements",
                              "Placements",
                              false);        
      };
    };

    typedef El::Python::SmartPtr<Advert> Advert_var;

    class AdvertUpdate : public El::Python::ObjectImpl
    {
    public:
      Moderation::Ad::AdvertId id;
      Moderation::Ad::AdvertiserId advertiser;
      std::string name;
      Moderation::Ad::AdvertStatus status;
      Moderation::Ad::SizeId size;
      std::string text;
    
    public:
      AdvertUpdate(PyTypeObject *type = 0,
                   PyObject *args = 0,
                   PyObject *kwds = 0)
        throw(Exception, El::Exception);

      void init(::NewsGate::Moderation::Ad::AdvertUpdate& update) const
        throw(El::Exception);
    
      virtual ~AdvertUpdate() throw() {}

      class Type : public El::Python::ObjectTypeImpl<AdvertUpdate,
                                                     AdvertUpdate::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Advert identifier");

        PY_TYPE_MEMBER_ULONGLONG(advertiser,
                                 "advertiser",
                                 "Advertiser identifier");

        PY_TYPE_MEMBER_ENUM(status,
                            Moderation::Ad::AdvertStatus,
                            Moderation::Ad::AS_COUNT - 1,
                            "status",
                            "Advert status");        

        PY_TYPE_MEMBER_ULONGLONG(size, "size", "Size identifier");
        PY_TYPE_MEMBER_STRING(text, "text", "Ad text", true);
        PY_TYPE_MEMBER_STRING(name, "name", "Ad name", false);
      };
    };

    typedef El::Python::SmartPtr<AdvertUpdate> AdvertUpdate_var;

    class Counter : public El::Python::ObjectImpl
    {
    public:
      Moderation::Ad::CounterId id;
      Moderation::Ad::AdvertiserId advertiser;
      std::string name;
      Moderation::Ad::CounterStatus status;
      std::string text;
      El::Python::Sequence_var placements;
    
    public:
      Counter(PyTypeObject *type = 0,
              PyObject *args = 0,
              PyObject *kwds = 0)
        throw(Exception, El::Exception);

      Counter(const ::NewsGate::Moderation::Ad::Counter& counter)
        throw(El::Exception);
    
      virtual ~Counter() throw() {}

      void init(::NewsGate::Moderation::Ad::Counter& counter) const
        throw(El::Exception);
      
      class Type : public El::Python::ObjectTypeImpl<Counter,
                                                     Counter::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Counter identifier");

        PY_TYPE_MEMBER_ULONGLONG(advertiser,
                                 "advertiser",
                                 "Advertiser identifier");

        PY_TYPE_MEMBER_STRING(name,
                              "name",
                              "Counter name",
                              false);

        PY_TYPE_MEMBER_ENUM(status,
                            Moderation::Ad::CounterStatus,
                            Moderation::Ad::US_COUNT - 1,
                            "status",
                            "Counter status");
        
        PY_TYPE_MEMBER_STRING(text,
                              "text",
                              "Counter text",
                              true);

        PY_TYPE_MEMBER_OBJECT(placements,
                              El::Python::Sequence::Type,
                              "placements",
                              "Placements",
                              false);        
      };
    };

    typedef El::Python::SmartPtr<Counter> Counter_var;

    class CounterUpdate : public El::Python::ObjectImpl
    {
    public:
      Moderation::Ad::CounterId id;
      Moderation::Ad::AdvertiserId advertiser;
      std::string name;
      Moderation::Ad::CounterStatus status;
      std::string text;
    
    public:
      CounterUpdate(PyTypeObject *type = 0,
                    PyObject *args = 0,
                    PyObject *kwds = 0)
        throw(Exception, El::Exception);

      void init(::NewsGate::Moderation::Ad::CounterUpdate& update) const
        throw(El::Exception);
    
      virtual ~CounterUpdate() throw() {}

      class Type : public El::Python::ObjectTypeImpl<CounterUpdate,
                                                     CounterUpdate::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Counter identifier");

        PY_TYPE_MEMBER_ULONGLONG(advertiser,
                                 "advertiser",
                                 "Advertiser identifier");

        PY_TYPE_MEMBER_ENUM(status,
                            Moderation::Ad::CounterStatus,
                            Moderation::Ad::US_COUNT - 1,
                            "status",
                            "Counter status");        

        PY_TYPE_MEMBER_STRING(text, "text", "Counter text", true);
        PY_TYPE_MEMBER_STRING(name, "name", "Counter name", false);
      };
    };

    typedef El::Python::SmartPtr<CounterUpdate> CounterUpdate_var;

    class Condition : public El::Python::ObjectImpl
    {
    public:
      Moderation::Ad::ConditionId id;
      Moderation::Ad::AdvertiserId advertiser;
      std::string name;
      Moderation::Ad::ConditionStatus status;
      unsigned long rnd_mod;
      unsigned long rnd_mod_from;
      unsigned long rnd_mod_to;
      unsigned long group_freq_cap;
      unsigned long group_count_cap;
      unsigned long query_types;
      unsigned long query_type_exclusions;
      std::string page_sources;
      std::string page_source_exclusions;
      std::string message_sources;
      std::string message_source_exclusions;
      std::string page_categories;
      std::string page_category_exclusions;
      std::string message_categories;
      std::string message_category_exclusions;
      std::string search_engines;
      std::string search_engine_exclusions;
      std::string crawlers;
      std::string crawler_exclusions;
      std::string languages;
      std::string language_exclusions;
      std::string countries;
      std::string country_exclusions;
      std::string ip_masks;
      std::string ip_mask_exclusions;
      std::string tags;
      std::string tag_exclusions;
      std::string referers;
      std::string referer_exclusions;
      std::string content_languages;
      std::string content_language_exclusions;
    
    public:
      Condition(PyTypeObject *type = 0,
                PyObject *args = 0,
                PyObject *kwds = 0)
        throw(Exception, El::Exception);

      Condition(const ::NewsGate::Moderation::Ad::Condition& cond)
        throw(El::Exception);
      
      virtual ~Condition() throw() {}

      void init(::NewsGate::Moderation::Ad::Condition& cond) const
        throw(El::Exception);
      
      class Type : public El::Python::ObjectTypeImpl<Condition,
                                                     Condition::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Condition identifier");

        PY_TYPE_MEMBER_ULONGLONG(advertiser,
                                 "advertiser",
                                 "Advertiser identifier");

        PY_TYPE_MEMBER_STRING(name,
                              "name",
                              "Condition name",
                              false);

        PY_TYPE_MEMBER_ENUM(status,
                            Moderation::Ad::ConditionStatus,
                            Moderation::Ad::CS_COUNT - 1,
                            "status",
                            "Condition status");
        
        PY_TYPE_MEMBER_ULONG(rnd_mod, "rnd_mod", "Modulo for random number");
        
        PY_TYPE_MEMBER_ULONG(rnd_mod_from,
                             "rnd_mod_from",
                             "Random modulo result range lower point");
        
        PY_TYPE_MEMBER_ULONG(group_freq_cap,
                             "group_freq_cap",
                             "Group frequency cap(sec)");
        
        PY_TYPE_MEMBER_ULONG(group_count_cap,
                             "group_count_cap",
                             "Group count cap");
        
        PY_TYPE_MEMBER_ULONG(query_types,
                             "query_types",
                             "Query types");
        
        PY_TYPE_MEMBER_ULONG(query_type_exclusions,
                             "query_type_exclusions",
                             "Query type exceptions");
        
        PY_TYPE_MEMBER_ULONG(rnd_mod_to,
                             "rnd_mod_to",
                             "Random modulo result range upper point");
        
        PY_TYPE_MEMBER_STRING(page_sources,
                              "page_sources",
                              "Page sources",
                              true);
        
        PY_TYPE_MEMBER_STRING(page_source_exclusions,
                              "page_source_exclusions",
                              "Page source exclusions",
                              true);
        
        PY_TYPE_MEMBER_STRING(message_sources,
                              "message_sources",
                              "Message sources",
                              true);
        
        PY_TYPE_MEMBER_STRING(message_source_exclusions,
                              "message_source_exclusions",
                              "Message source exclusions",
                              true);
        
        PY_TYPE_MEMBER_STRING(page_categories,
                              "page_categories",
                              "Page categories",
                              true);
        
        PY_TYPE_MEMBER_STRING(page_category_exclusions,
                              "page_category_exclusions",
                              "Page category exclusions",
                              true);
        
        PY_TYPE_MEMBER_STRING(message_categories,
                              "message_categories",
                              "Message categories",
                              true);
        
        PY_TYPE_MEMBER_STRING(message_category_exclusions,
                              "message_category_exclusions",
                              "Message category exclusions",
                              true);
        
        PY_TYPE_MEMBER_STRING(search_engines,
                              "search_engines",
                              "Search engines",
                              true);
        
        PY_TYPE_MEMBER_STRING(search_engine_exclusions,
                              "search_engine_exclusions",
                              "Search engine exclusions",
                              true);
        
        PY_TYPE_MEMBER_STRING(crawlers,
                              "crawlers",
                              "Crawlers",
                              true);
        
        PY_TYPE_MEMBER_STRING(crawler_exclusions,
                              "crawler_exclusions",
                              "Crawler exclusions",
                              true);
        
        PY_TYPE_MEMBER_STRING(languages,
                              "languages",
                              "Languages",
                              true);
        
        PY_TYPE_MEMBER_STRING(language_exclusions,
                              "language_exclusions",
                              "Language exclusions",
                              true);
        
        PY_TYPE_MEMBER_STRING(countries,
                              "countries",
                              "Countries",
                              true);
        
        PY_TYPE_MEMBER_STRING(country_exclusions,
                              "country_exclusions",
                              "Country exclusions",
                              true);
        
        PY_TYPE_MEMBER_STRING(ip_masks,
                              "ip_masks",
                              "IP masks",
                              true);
        
        PY_TYPE_MEMBER_STRING(ip_mask_exclusions,
                              "ip_mask_exclusions",
                              "IP mask exclusions",
                              true);
        
        PY_TYPE_MEMBER_STRING(tags,
                              "tags",
                              "Tags",
                              true);
        
        PY_TYPE_MEMBER_STRING(tag_exclusions,
                              "tag_exclusions",
                              "Tag exclusions",
                              true);
        
        PY_TYPE_MEMBER_STRING(referers,
                              "referers",
                              "Referers",
                              true);
        
        PY_TYPE_MEMBER_STRING(referer_exclusions,
                              "referer_exclusions",
                              "Referer exclusions",
                              true);
        
        PY_TYPE_MEMBER_STRING(content_languages,
                              "content_languages",
                              "Content languages",
                              true);
        
        PY_TYPE_MEMBER_STRING(content_language_exclusions,
                              "content_language_exclusions",
                              "Content language exclusions",
                              true);
      };
    };

    typedef El::Python::SmartPtr<Condition> Condition_var;

    class ConditionUpdate : public El::Python::ObjectImpl
    {
    public:
      Moderation::Ad::ConditionId id;
      Moderation::Ad::AdvertiserId advertiser;
      std::string name;
      Moderation::Ad::ConditionStatus status;
      unsigned long rnd_mod;
      unsigned long rnd_mod_from;
      unsigned long rnd_mod_to;
      unsigned long group_freq_cap;
      unsigned long group_count_cap;
      unsigned long query_types;
      unsigned long query_type_exclusions;
      std::string page_sources;
      std::string page_source_exclusions;
      std::string message_sources;
      std::string message_source_exclusions;
      std::string page_categories;
      std::string page_category_exclusions;
      std::string message_categories;
      std::string message_category_exclusions;
      std::string search_engines;
      std::string search_engine_exclusions;
      std::string crawlers;
      std::string crawler_exclusions;
      std::string languages;
      std::string language_exclusions;
      std::string countries;
      std::string country_exclusions;
      std::string ip_masks;
      std::string ip_mask_exclusions;
      std::string tags;
      std::string tag_exclusions;
      std::string referers;
      std::string referer_exclusions;
      std::string content_languages;
      std::string content_language_exclusions;
    
    public:
      ConditionUpdate(PyTypeObject *type = 0,
                      PyObject *args = 0,
                      PyObject *kwds = 0)
        throw(Exception, El::Exception);

      ConditionUpdate(const ::NewsGate::Moderation::Ad::Condition& cond)
        throw(El::Exception);
      
      virtual ~ConditionUpdate() throw() {}

      void init(::NewsGate::Moderation::Ad::ConditionUpdate& update) const
        throw(El::Exception);
      
      class Type : public El::Python::ObjectTypeImpl<ConditionUpdate,
                                                     ConditionUpdate::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Condition identifier");

        PY_TYPE_MEMBER_ULONGLONG(advertiser,
                                 "advertiser",
                                 "Advertiser identifier");

        PY_TYPE_MEMBER_STRING(name,
                              "name",
                              "Condition name",
                              false);

        PY_TYPE_MEMBER_ENUM(status,
                            Moderation::Ad::ConditionStatus,
                            Moderation::Ad::CS_COUNT - 1,
                            "status",
                            "Condition status");
        
        PY_TYPE_MEMBER_ULONG(rnd_mod, "rnd_mod", "Modulo for random number");
        
        PY_TYPE_MEMBER_ULONG(rnd_mod_from,
                             "rnd_mod_from",
                             "Random modulo result range lower point");
        
        PY_TYPE_MEMBER_ULONG(rnd_mod_to,
                             "rnd_mod_to",
                             "Random modulo result range upper point");
        
        PY_TYPE_MEMBER_ULONG(group_freq_cap,
                             "group_freq_cap",
                             "Group frequency cap(sec)");
        
        PY_TYPE_MEMBER_ULONG(group_count_cap,
                             "group_count_cap",
                             "Group count cap");
        
        PY_TYPE_MEMBER_ULONG(query_types,
                             "query_types",
                             "Query types");
        
        PY_TYPE_MEMBER_ULONG(query_type_exclusions,
                             "query_type_exclusions",
                             "Query type exceptions");
        
        PY_TYPE_MEMBER_STRING(page_sources,
                              "page_sources",
                              "Page sources",
                              true);
        
        PY_TYPE_MEMBER_STRING(page_source_exclusions,
                              "page_source_exclusions",
                              "Page source exclusions",
                              true);
        
        PY_TYPE_MEMBER_STRING(message_sources,
                              "message_sources",
                              "Message sources",
                              true);
        
        PY_TYPE_MEMBER_STRING(message_source_exclusions,
                              "message_source_exclusions",
                              "Message source exclusions",
                              true);
        
        PY_TYPE_MEMBER_STRING(page_categories,
                              "page_categories",
                              "Page categories",
                              true);
        
        PY_TYPE_MEMBER_STRING(page_category_exclusions,
                              "page_category_exclusions",
                              "Page category exclusions",
                              true);
        
        PY_TYPE_MEMBER_STRING(message_categories,
                              "message_categories",
                              "Message categories",
                              true);
        
        PY_TYPE_MEMBER_STRING(message_category_exclusions,
                              "message_category_exclusions",
                              "Message category exclusions",
                              true);
        
        PY_TYPE_MEMBER_STRING(search_engines,
                              "search_engines",
                              "Search engines",
                              true);
        
        PY_TYPE_MEMBER_STRING(search_engine_exclusions,
                              "search_engine_exclusions",
                              "Search engine exclusions",
                              true);
        
        PY_TYPE_MEMBER_STRING(crawlers,
                              "crawlers",
                              "Crawlers",
                              true);
        
        PY_TYPE_MEMBER_STRING(crawler_exclusions,
                              "crawler_exclusions",
                              "Crawler exclusions",
                              true);
        
        PY_TYPE_MEMBER_STRING(languages,
                              "languages",
                              "Languages",
                              true);
        
        PY_TYPE_MEMBER_STRING(language_exclusions,
                              "language_exclusions",
                              "Language exclusions",
                              true);
        
        PY_TYPE_MEMBER_STRING(countries,
                              "countries",
                              "Countries",
                              true);
        
        PY_TYPE_MEMBER_STRING(country_exclusions,
                              "country_exclusions",
                              "Coutry exclusions",
                              true);
        
        PY_TYPE_MEMBER_STRING(ip_masks,
                              "ip_masks",
                              "IP masks",
                              true);
        
        PY_TYPE_MEMBER_STRING(ip_mask_exclusions,
                              "ip_mask_exclusions",
                              "IP mask exclusions",
                              true);
        
        PY_TYPE_MEMBER_STRING(tags,
                              "tags",
                              "Tags",
                              true);
        
        PY_TYPE_MEMBER_STRING(tag_exclusions,
                              "tag_exclusions",
                              "Tag exclusions",
                              true);
        
        PY_TYPE_MEMBER_STRING(referers,
                              "referers",
                              "Referers",
                              true);
        
        PY_TYPE_MEMBER_STRING(referer_exclusions,
                              "referer_exclusions",
                              "Referer exclusions",
                              true);
        
        PY_TYPE_MEMBER_STRING(content_languages,
                              "content_languages",
                              "Content languages",
                              true);
        
        PY_TYPE_MEMBER_STRING(content_language_exclusions,
                              "content_language_exclusions",
                              "Content language exclusions",
                              true);
      };
    };

    typedef El::Python::SmartPtr<ConditionUpdate> ConditionUpdate_var;
    
    class Placement : public El::Python::ObjectImpl
    {
    public:
      Moderation::Ad::PlacementId id;
      Moderation::Ad::GroupId group;
      std::string group_name;
      Moderation::Ad::CampaignId campaign;
      std::string campaign_name;
      Moderation::Ad::AdvertiserId advertiser;
      std::string name;
      Moderation::Ad::PlacementStatus status;
      Slot_var slot;
      Advert_var ad;
      double cpm;
      Moderation::Ad::PlacementInjection inject;
      std::string display_status;
    
    public:
      Placement(PyTypeObject *type = 0,
                PyObject *args = 0,
                PyObject *kwds = 0)
        throw(Exception, El::Exception);
      
      Placement(const ::NewsGate::Moderation::Ad::Placement& cmp)
        throw(El::Exception);
      
      virtual ~Placement() throw() {}
      
      void init(::NewsGate::Moderation::Ad::Placement& grp) const
        throw(El::Exception);
      
      class Type : public El::Python::ObjectTypeImpl<Placement,
                                                     Placement::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Placement identifier");

        PY_TYPE_MEMBER_ULONGLONG(group,
                                 "group",
                                 "Group identifier");

        PY_TYPE_MEMBER_ULONGLONG(campaign,
                                 "campaign",
                                 "Campaign identifier");

        PY_TYPE_MEMBER_OBJECT(slot, Slot::Type, "slot", "Slot", false);
        PY_TYPE_MEMBER_OBJECT(ad, Advert::Type, "ad", "Advert", false);

        PY_TYPE_MEMBER_ULONGLONG(advertiser,
                                 "advertiser",
                                 "Advertiser identifier");

        PY_TYPE_MEMBER_STRING(group_name,
                              "group_name",
                              "Placement group name",
                              false);

        PY_TYPE_MEMBER_STRING(campaign_name,
                              "campaign_name",
                              "Placement campaign name",
                              false);

        PY_TYPE_MEMBER_STRING(name,
                              "name",
                              "Placement name",
                              false);

        PY_TYPE_MEMBER_ENUM(status,
                            Moderation::Ad::PlacementStatus,
                            Moderation::Ad::TS_COUNT - 1,
                            "status",
                            "Placement status");

        PY_TYPE_MEMBER_FLOAT(cpm,
                             "cpm",
                             "Placement cpm");

        PY_TYPE_MEMBER_ENUM(inject,
                            Moderation::Ad::PlacementInjection,
                            Moderation::Ad::PI_COUNT - 1,
                            "inject",
                            "Placement injection");
        
        PY_TYPE_MEMBER_STRING(display_status,
                              "display_status",
                              "Display status",
                              false);
      };
    };

    typedef El::Python::SmartPtr<Placement> Placement_var;

    class PlacementUpdate : public El::Python::ObjectImpl
    {
    public:
      Moderation::Ad::PlacementId id;
      Moderation::Ad::AdvertiserId advertiser;
      std::string name;
      Moderation::Ad::PlacementStatus status;
      Moderation::Ad::SlotId slot;
      Moderation::Ad::AdvertId ad;
      double cpm;
      Moderation::Ad::PlacementInjection inject;
    
    public:
      PlacementUpdate(PyTypeObject *type = 0,
                      PyObject *args = 0,
                      PyObject *kwds = 0)
        throw(Exception, El::Exception);

      PlacementUpdate(const ::NewsGate::Moderation::Ad::Placement& plc)
        throw(El::Exception);
      
      virtual ~PlacementUpdate() throw() {}

      void init(::NewsGate::Moderation::Ad::PlacementUpdate& update) const
        throw(El::Exception);
      
      class Type : public El::Python::ObjectTypeImpl<PlacementUpdate,
                                                     PlacementUpdate::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Placement identifier");

        PY_TYPE_MEMBER_ULONGLONG(advertiser,
                                 "advertiser",
                                 "Advertiser identifier");

        PY_TYPE_MEMBER_STRING(name,
                              "name",
                              "Placement name",
                              false);

        PY_TYPE_MEMBER_ENUM(status,
                            Moderation::Ad::PlacementStatus,
                            Moderation::Ad::TS_COUNT - 1,
                            "status",
                            "Placement status");

        PY_TYPE_MEMBER_ULONGLONG(slot, "slot", "Placement slot identifier");
        PY_TYPE_MEMBER_ULONGLONG(ad, "ad", "Placement ad identifier");
        PY_TYPE_MEMBER_FLOAT(cpm, "cpm", "Placement cpm");        

        PY_TYPE_MEMBER_ENUM(inject,
                            Moderation::Ad::PlacementInjection,
                            Moderation::Ad::PI_COUNT - 1,
                            "inject",
                            "Placement injection");
      };
    };

    typedef El::Python::SmartPtr<PlacementUpdate> PlacementUpdate_var;    

    class CounterPlacement : public El::Python::ObjectImpl
    {
    public:
      Moderation::Ad::CounterPlacementId id;
      Moderation::Ad::GroupId group;
      std::string group_name;
      Moderation::Ad::CampaignId campaign;
      std::string campaign_name;
      Moderation::Ad::AdvertiserId advertiser;
      std::string name;
      Moderation::Ad::CounterPlacementStatus status;
      Page_var page;
      Counter_var counter;
      std::string display_status;
    
    public:
      CounterPlacement(PyTypeObject *type = 0,
                       PyObject *args = 0,
                       PyObject *kwds = 0)
        throw(Exception, El::Exception);
      
      CounterPlacement(const ::NewsGate::Moderation::Ad::CounterPlacement& src)
        throw(El::Exception);
      
      virtual ~CounterPlacement() throw() {}
      
      void init(::NewsGate::Moderation::Ad::CounterPlacement& dest) const
        throw(El::Exception);
      
      class Type : public El::Python::ObjectTypeImpl<CounterPlacement,
                                                     CounterPlacement::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Placement identifier");

        PY_TYPE_MEMBER_ULONGLONG(group,
                                 "group",
                                 "Group identifier");

        PY_TYPE_MEMBER_ULONGLONG(campaign,
                                 "campaign",
                                 "Campaign identifier");

        PY_TYPE_MEMBER_OBJECT(page, Page::Type, "page", "Page", false);
        
        PY_TYPE_MEMBER_OBJECT(counter,
                              Counter::Type,
                              "counter",
                              "Counter",
                              false);

        PY_TYPE_MEMBER_ULONGLONG(advertiser,
                                 "advertiser",
                                 "Advertiser identifier");

        PY_TYPE_MEMBER_STRING(group_name,
                              "group_name",
                              "Placement group name",
                              false);

        PY_TYPE_MEMBER_STRING(campaign_name,
                              "campaign_name",
                              "Placement campaign name",
                              false);

        PY_TYPE_MEMBER_STRING(name,
                              "name",
                              "Placement name",
                              false);

        PY_TYPE_MEMBER_ENUM(status,
                            Moderation::Ad::CounterPlacementStatus,
                            Moderation::Ad::OS_COUNT - 1,
                            "status",
                            "Placement status");
        
        PY_TYPE_MEMBER_STRING(display_status,
                              "display_status",
                              "Display status",
                              false);
      };
    };

    typedef El::Python::SmartPtr<CounterPlacement> CounterPlacement_var;

    class CounterPlacementUpdate : public El::Python::ObjectImpl
    {
    public:
      Moderation::Ad::CounterPlacementId id;
      Moderation::Ad::AdvertiserId advertiser;
      std::string name;
      Moderation::Ad::CounterPlacementStatus status;
      Moderation::Ad::PageId page;
      Moderation::Ad::CounterId counter;
    
    public:
      CounterPlacementUpdate(PyTypeObject *type = 0,
                             PyObject *args = 0,
                             PyObject *kwds = 0)
        throw(Exception, El::Exception);

      CounterPlacementUpdate(
        const ::NewsGate::Moderation::Ad::CounterPlacement& plc)
        throw(El::Exception);
      
      virtual ~CounterPlacementUpdate() throw() {}

      void init(::NewsGate::Moderation::Ad::CounterPlacementUpdate& update)
        const throw(El::Exception);
      
      class Type :
        public El::Python::ObjectTypeImpl<CounterPlacementUpdate,
                                          CounterPlacementUpdate::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Placement identifier");

        PY_TYPE_MEMBER_ULONGLONG(advertiser,
                                 "advertiser",
                                 "Advertiser identifier");

        PY_TYPE_MEMBER_STRING(name,
                              "name",
                              "Placement name",
                              false);

        PY_TYPE_MEMBER_ENUM(status,
                            Moderation::Ad::CounterPlacementStatus,
                            Moderation::Ad::OS_COUNT - 1,
                            "status",
                            "Counter placement status");

        PY_TYPE_MEMBER_ULONGLONG(page, "page", "Placement page identifier");
        
        PY_TYPE_MEMBER_ULONGLONG(counter,
                                 "counter",
                                 "Placement counter identifier");        
      };
    };

    typedef El::Python::SmartPtr<CounterPlacementUpdate>
    CounterPlacementUpdate_var;

    class Group : public El::Python::ObjectImpl
    {
    public:
      Moderation::Ad::GroupId id;
      Moderation::Ad::CampaignId campaign;
      std::string campaign_name;
      Moderation::Ad::AdvertiserId advertiser;
      std::string name;
      Moderation::Ad::GroupStatus status;
      double auction_factor;
      El::Python::Sequence_var conditions;
      El::Python::Sequence_var ad_placements;
      El::Python::Sequence_var counter_placements;
    
    public:
      Group(PyTypeObject *type = 0,
            PyObject *args = 0,
            PyObject *kwds = 0)
        throw(Exception, El::Exception);
      
      Group(const ::NewsGate::Moderation::Ad::Group& cmp)
        throw(El::Exception);
      
      virtual ~Group() throw() {}
      
      void init(::NewsGate::Moderation::Ad::Group& grp) const
        throw(El::Exception);
      
      class Type : public El::Python::ObjectTypeImpl<Group, Group::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Group identifier");

        PY_TYPE_MEMBER_ULONGLONG(campaign,
                                 "campaign",
                                 "Campaign identifier");

        PY_TYPE_MEMBER_ULONGLONG(advertiser,
                                 "advertiser",
                                 "Advertiser identifier");

        PY_TYPE_MEMBER_STRING(campaign_name,
                              "campaign_name",
                              "Group campaign name",
                              false);

        PY_TYPE_MEMBER_STRING(name,
                              "name",
                              "Group name",
                              false);

        PY_TYPE_MEMBER_ENUM(status,
                            Moderation::Ad::GroupStatus,
                            Moderation::Ad::GS_COUNT - 1,
                            "status",
                            "Group status");

        PY_TYPE_MEMBER_FLOAT(auction_factor,
                             "auction_factor",
                             "Group auction factor");
        
        PY_TYPE_MEMBER_OBJECT(conditions,
                              El::Python::Sequence::Type,
                              "conditions",
                              "Conditions",
                              false);
        
        PY_TYPE_MEMBER_OBJECT(ad_placements,
                              El::Python::Sequence::Type,
                              "ad_placements",
                              "Ad placements",
                              false);
        
        PY_TYPE_MEMBER_OBJECT(counter_placements,
                              El::Python::Sequence::Type,
                              "counter_placements",
                              "Counter placements",
                              false);
      };
    };

    typedef El::Python::SmartPtr<Group> Group_var;

    class GroupUpdate : public El::Python::ObjectImpl
    {
    public:
      Moderation::Ad::GroupId id;
      Moderation::Ad::AdvertiserId advertiser;
      std::string name;
      Moderation::Ad::GroupStatus status;
      double auction_factor;
      El::Python::Sequence_var conditions;
      bool reset_cap_min_time;
    
    public:
      GroupUpdate(PyTypeObject *type = 0,
                  PyObject *args = 0,
                  PyObject *kwds = 0)
        throw(Exception, El::Exception);

      GroupUpdate(const ::NewsGate::Moderation::Ad::Group& grp)
        throw(El::Exception);
      
      virtual ~GroupUpdate() throw() {}

      void init(::NewsGate::Moderation::Ad::GroupUpdate& update) const
        throw(El::Exception);
      
      class Type : public El::Python::ObjectTypeImpl<GroupUpdate,
                                                     GroupUpdate::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Group identifier");

        PY_TYPE_MEMBER_ULONGLONG(advertiser,
                                 "advertiser",
                                 "Advertiser identifier");

        PY_TYPE_MEMBER_STRING(name,
                              "name",
                              "Group name",
                              false);

        PY_TYPE_MEMBER_ENUM(status,
                            Moderation::Ad::GroupStatus,
                            Moderation::Ad::GS_COUNT - 1,
                            "status",
                            "Group status");

        PY_TYPE_MEMBER_FLOAT(auction_factor,
                             "auction_factor",
                             "Group auction factor");
        
        PY_TYPE_MEMBER_OBJECT(conditions,
                              El::Python::Sequence::Type,
                              "conditions",
                              "Conditions",
                              false);
        
        PY_TYPE_MEMBER_BOOL(reset_cap_min_time,
                            "reset_cap_min_time",
                            "Reset group cap min time");
      };
    };

    typedef El::Python::SmartPtr<GroupUpdate> GroupUpdate_var;    

    class Campaign : public El::Python::ObjectImpl
    {
    public:
      Moderation::Ad::CampaignId id;
      Moderation::Ad::AdvertiserId advertiser;
      std::string name;
      Moderation::Ad::CampaignStatus status;
      El::Python::Sequence_var conditions;
      El::Python::Sequence_var groups;
    
    public:
      Campaign(PyTypeObject *type = 0,
               PyObject *args = 0,
               PyObject *kwds = 0)
        throw(Exception, El::Exception);
      
      Campaign(const ::NewsGate::Moderation::Ad::Campaign& cmp)
        throw(El::Exception);
      
      virtual ~Campaign() throw() {}

      void init(::NewsGate::Moderation::Ad::Campaign& cmp) const
        throw(El::Exception);
      
      class Type : public El::Python::ObjectTypeImpl<Campaign,
                                                     Campaign::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Campaign identifier");

        PY_TYPE_MEMBER_ULONGLONG(advertiser,
                                 "advertiser",
                                 "Advertiser identifier");

        PY_TYPE_MEMBER_STRING(name,
                              "name",
                              "Campaign name",
                              false);

        PY_TYPE_MEMBER_ENUM(status,
                            Moderation::Ad::CampaignStatus,
                            Moderation::Ad::MS_COUNT - 1,
                            "status",
                            "Campaign status");

        PY_TYPE_MEMBER_OBJECT(conditions,
                              El::Python::Sequence::Type,
                              "conditions",
                              "Conditions",
                              false);
        
        PY_TYPE_MEMBER_OBJECT(groups,
                              El::Python::Sequence::Type,
                              "groups",
                              "Groups",
                              false);
      };
    };

    typedef El::Python::SmartPtr<Campaign> Campaign_var;

    class CampaignUpdate : public El::Python::ObjectImpl
    {
    public:
      Moderation::Ad::CampaignId id;
      Moderation::Ad::AdvertiserId advertiser;
      std::string name;
      Moderation::Ad::CampaignStatus status;
      El::Python::Sequence_var conditions;
    
    public:
      CampaignUpdate(PyTypeObject *type = 0,
                     PyObject *args = 0,
                     PyObject *kwds = 0)
        throw(Exception, El::Exception);

      CampaignUpdate(const ::NewsGate::Moderation::Ad::Campaign& cmp)
        throw(El::Exception);
      
      virtual ~CampaignUpdate() throw() {}

      void init(::NewsGate::Moderation::Ad::CampaignUpdate& update) const
        throw(El::Exception);
      
      class Type : public El::Python::ObjectTypeImpl<CampaignUpdate,
                                                     CampaignUpdate::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Campaign identifier");

        PY_TYPE_MEMBER_ULONGLONG(advertiser,
                                 "advertiser",
                                 "Advertiser identifier");

        PY_TYPE_MEMBER_STRING(name,
                              "name",
                              "Campaign name",
                              false);

        PY_TYPE_MEMBER_ENUM(status,
                            Moderation::Ad::CampaignStatus,
                            Moderation::Ad::MS_COUNT - 1,
                            "status",
                            "Campaign status");
        
        PY_TYPE_MEMBER_OBJECT(conditions,
                              El::Python::Sequence::Type,
                              "conditions",
                              "Conditions",
                              false);        
      };
    };

    typedef El::Python::SmartPtr<CampaignUpdate> CampaignUpdate_var;    
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{  
  namespace AdModeration
  {
    //
    // NewsGate::AdModeration::Manager::Type class
    //
    inline
    Manager::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<Manager, Manager::Type>(
          "newsgate.moderation.ad.Manager",
          "Object representing ad management functionality")
    {
      tp_new = 0;
    }

    //
    // NewsGate::AdModeration::Manager class
    //
    inline
    Manager::Manager(const ManagerRef& manager)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          manager_(manager)
    {
    }    

    //
    // NewsGate::AdModeration::Global class
    //
    inline
    Global::Global(PyTypeObject *type,
                       PyObject *args,
                       PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          selector_status(Moderation::Ad::RS_ENABLED),
          adv_max_ads_per_page(0),
          update_number(0),
          pcws_reduction_rate(0),
          pcws_weight_zones(0)
          
    {
    }

    inline
    Global::Global(const ::NewsGate::Moderation::Ad::Global& global)
      throw(El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          selector_status(global.selector_status),
          adv_max_ads_per_page(global.adv_max_ads_per_page),
          update_number(global.update_number),
          pcws_reduction_rate(global.pcws_reduction_rate),
          pcws_weight_zones(global.pcws_weight_zones)
    {
    }
    
    //
    // NewsGate::AdModeration::Global::Type class
    //
    inline
    Global::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<Global,
                                     Global::Type>(
        "newsgate.moderation.ad.Global",
        "Object encapsulating ad global options")
    {
    }

    //
    // NewsGate::AdModeration::GlobalUpdate class
    //
    inline
    GlobalUpdate::GlobalUpdate(PyTypeObject *type,
                               PyObject *args,
                               PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          flags(0),
          advertiser(0),
          selector_status(Moderation::Ad::RS_ENABLED),
          adv_max_ads_per_page(0),
          pcws_reduction_rate(0),
          pcws_weight_zones(0)
    {
    }

    inline
    void
    GlobalUpdate::init(::NewsGate::Moderation::Ad::GlobalUpdate& update)
      const throw(El::Exception)
    {
      update.flags = flags;
      update.advertiser = advertiser;
      update.selector_status = selector_status;
      update.adv_max_ads_per_page = adv_max_ads_per_page;
      update.pcws_reduction_rate = pcws_reduction_rate;
      update.pcws_weight_zones = pcws_weight_zones;
    }    
    
    //
    // NewsGate::AdModeration::GlobalUpdate::Type class
    //
    inline
    GlobalUpdate::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<GlobalUpdate,
                                     GlobalUpdate::Type>(
        "newsgate.moderation.ad.GlobalUpdate",
        "Object encapsulating ad global options update")
    {
    }
    
    //
    // NewsGate::AdModeration::Page class
    //
    inline
    Page::Page(PyTypeObject *type,
               PyObject *args,
               PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          status(Moderation::Ad::PS_ENABLED),
          max_ad_num(0),
          slots(new El::Python::Sequence()),
          advertiser_info(new PageAdvertiserInfo())
    {
    }

    inline
    Page::Page(const ::NewsGate::Moderation::Ad::Page& page)
      throw(El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          id(page.id),
          name(page.name.in()),
          status(page.status),
          max_ad_num(page.max_ad_num),
          slots(new El::Python::Sequence()),
          advertiser_info(new PageAdvertiserInfo(page.advertiser_info))
    {
      slots->resize(page.slots.length());
      
      for(size_t i = 0; i < page.slots.length(); ++i)
      {
        (*slots)[i] = new Slot(page.slots[i]);
      }
    }
/*
    inline
    void
    Page::init(::NewsGate::Moderation::Ad::Page& page) const
      throw(El::Exception)
    {
      page.id = id;
      page.name = name.c_str();
      page.status = status;
      page.max_ad_num = max_ad_num;
    }
*/  
    //
    // NewsGate::AdModeration::Page::Type class
    //
    inline
    Page::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<Page,
                                     Page::Type>(
        "newsgate.moderation.ad.Page",
        "Object encapsulating ad page options")
    {
    }
    
    //
    // NewsGate::AdModeration::PageUpdate class
    //
    inline
    PageUpdate::PageUpdate(PyTypeObject *type,
                           PyObject *args,
                           PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          flags(0),
          id(0),
          status(Moderation::Ad::PS_ENABLED),
          max_ad_num(0),
          slot_updates(new El::Python::Sequence()),
          advertiser_info_update(new PageAdvertiserInfoUpdate())
    {
    }

    inline
    void
    PageUpdate::init(::NewsGate::Moderation::Ad::PageUpdate& update) const
      throw(El::Exception)
    {
      update.flags = flags;
      update.id = id;
      update.status = status;
      update.max_ad_num = max_ad_num;

      update.slot_updates.length(slot_updates->size());

      for(size_t i = 0; i < slot_updates->size(); ++i)
      {
        SlotUpdate::Type::down_cast((*slot_updates)[i].in(), false)->
        init(update.slot_updates[i]);
      }

      advertiser_info_update->init(update.advertiser_info_update);
    }
    
    //
    // NewsGate::AdModeration::PageUpdate::Type class
    //
    inline
    PageUpdate::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<PageUpdate,
                                     PageUpdate::Type>(
        "newsgate.moderation.ad.PageUpdate",
        "Object encapsulating ad page update options")
    {
    }
    
    //
    // NewsGate::AdModeration::AdvertiserMaxAdNum class
    //
    inline
    AdvertiserMaxAdNum::AdvertiserMaxAdNum(PyTypeObject *type,
                                           PyObject *args,
                                           PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          max_ad_num(0)
    {
    }

    inline
    AdvertiserMaxAdNum::AdvertiserMaxAdNum(
      const ::NewsGate::Moderation::Ad::AdvertiserMaxAdNum& info)
      throw(El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          id(info.id),
          name(info.name.in()),
          max_ad_num(info.max_ad_num)
    {
    }
    
    inline
    void
    AdvertiserMaxAdNum::init(
      ::NewsGate::Moderation::Ad::AdvertiserMaxAdNum& dest) const
      throw(El::Exception)
    {
      dest.id = id;
      dest.max_ad_num = max_ad_num;
      dest.name = name.c_str();
    }
    
    //
    // NewsGate::AdModeration::AdvertiserMaxAdNum::Type class
    //
    inline
    AdvertiserMaxAdNum::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<AdvertiserMaxAdNum,
                                     AdvertiserMaxAdNum::Type>(
        "newsgate.moderation.ad.AdvertiserMaxAdNum",
        "Object encapsulating ad page advertiser options")
    {
    }
    
    //
    // NewsGate::AdModeration::PageAdvertiserInfo class
    //
    inline
    PageAdvertiserInfo::PageAdvertiserInfo(PyTypeObject *type,
                                           PyObject *args,
                                           PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          max_ad_num(Moderation::Ad::MAX_AD_NUM),
          adv_max_ad_nums(new El::Python::Sequence())
    {
    }

    inline
    PageAdvertiserInfo::PageAdvertiserInfo(
      const ::NewsGate::Moderation::Ad::PageAdvertiserInfo& info)
      throw(El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          id(info.id),
          max_ad_num(info.max_ad_num),
          adv_max_ad_nums(new El::Python::Sequence())
    {
      adv_max_ad_nums->resize(info.adv_max_ad_nums.length());
      
      for(size_t i = 0; i < info.adv_max_ad_nums.length(); ++i)
      {
        (*adv_max_ad_nums)[i] =
          new AdvertiserMaxAdNum(info.adv_max_ad_nums[i]);
      }
    }
    
    //
    // NewsGate::AdModeration::PageAdvertiserInfo::Type class
    //
    inline
    PageAdvertiserInfo::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<PageAdvertiserInfo,
                                     PageAdvertiserInfo::Type>(
        "newsgate.moderation.ad.PageAdvertiserInfo",
        "Object encapsulating ad page advertiser options")
    {
    }
    
    //
    // NewsGate::AdModeration::PageAdvertiserInfoUpdate class
    //
    inline
    PageAdvertiserInfoUpdate::PageAdvertiserInfoUpdate(PyTypeObject *type,
                                                       PyObject *args,
                                                       PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          max_ad_num(0),
          adv_max_ad_nums(new El::Python::Sequence())
    {
    }

    inline
    void
    PageAdvertiserInfoUpdate::init(
      ::NewsGate::Moderation::Ad::PageAdvertiserInfoUpdate& update) const
      throw(El::Exception)
    {
      update.id = id;
      update.max_ad_num = max_ad_num;

      update.adv_max_ad_nums.length(adv_max_ad_nums->size());
      
      for(size_t i = 0; i < adv_max_ad_nums->size(); ++i)
      {
        AdvertiserMaxAdNum::Type::down_cast(
          (*adv_max_ad_nums)[i].in(), false)->init(update.adv_max_ad_nums[i]);
      }
    }
    
    //
    // NewsGate::AdModeration::PageAdvertiserInfoUpdate::Type class
    //
    inline
    PageAdvertiserInfoUpdate::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<PageAdvertiserInfoUpdate,
                                     PageAdvertiserInfoUpdate::Type>(
        "newsgate.moderation.ad.PageAdvertiserInfoUpdate",
        "Object encapsulating ad page advertiser update options")
    {
    }

    //
    // NewsGate::AdModeration::Advert class
    //
    inline
    Advert::Advert(PyTypeObject *type,
                   PyObject *args,
                   PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          advertiser(0),
          status(Moderation::Ad::AS_ENABLED),
          size(0),
          width(0),
          height(0),
          placements(new El::Python::Sequence())
    {
    }

    inline
    Advert::Advert(const ::NewsGate::Moderation::Ad::Advert& ad)
      throw(El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          id(ad.id),
          advertiser(ad.advertiser),
          name(ad.name.in()),
          status(ad.status),
          size(ad.size),
          size_name(ad.size_name.in()),
          width(ad.width),
          height(ad.height),
          text(ad.text.in()),
          placements(new El::Python::Sequence())
    {
      placements->resize(ad.placements.length());

      for(size_t i = 0; i < ad.placements.length(); ++i)
      {
        (*placements)[i] = new Placement(ad.placements[i]);
      }
    }

    inline
    void
    Advert::init(::NewsGate::Moderation::Ad::Advert& ad) const
      throw(El::Exception)
    {
      ad.id = id;
      ad.advertiser = advertiser;
      ad.name = name.c_str();
      ad.status = status;
      ad.size = size;
      ad.size_name = size_name.c_str();
      ad.width = width;
      ad.height = height;
      ad.text = text.c_str();

      ad.placements.length(placements->size());

      for(size_t i = 0; i < ad.placements.length(); ++i)
      {
        Placement::Type::down_cast((*placements)[i].in(), false)->
          init(ad.placements[i]);
      }
    }
    
    //
    // NewsGate::AdModeration::Advert::Type class
    //
    inline
    Advert::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<Advert,
                                     Advert::Type>(
        "newsgate.moderation.ad.Advert",
        "Object encapsulating ads options")
    {
    }
    
    //
    // NewsGate::AdModeration::AdvertUpdate class
    //
    inline
    AdvertUpdate::AdvertUpdate(PyTypeObject *type,
                               PyObject *args,
                               PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          advertiser(0),
          status(Moderation::Ad::AS_ENABLED),
          size(0)
    {
    }

    inline
    void
    AdvertUpdate::init(::NewsGate::Moderation::Ad::AdvertUpdate& update) const
      throw(El::Exception)
    {
      update.id = id;
      update.advertiser = advertiser;
      update.name = name.c_str();
      update.status = status;
      update.size = size;
      update.text = text.c_str();
    }

    //
    // NewsGate::AdModeration::AdvertUpdate::Type class
    //
    inline
    AdvertUpdate::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<AdvertUpdate,
                                     AdvertUpdate::Type>(
        "newsgate.moderation.ad.AdvertUpdate",
        "Object encapsulating advert update options")
    {
    }

    //
    // NewsGate::AdModeration::Counter class
    //
    inline
    Counter::Counter(PyTypeObject *type,
                     PyObject *args,
                     PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          advertiser(0),
          status(Moderation::Ad::US_ENABLED),
          placements(new El::Python::Sequence())
    {
    }

    inline
    Counter::Counter(const ::NewsGate::Moderation::Ad::Counter& counter)
      throw(El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          id(counter.id),
          advertiser(counter.advertiser),
          name(counter.name.in()),
          status(counter.status),
          text(counter.text.in()),
          placements(new El::Python::Sequence())
    {
      placements->resize(counter.placements.length());

      for(size_t i = 0; i < counter.placements.length(); ++i)
      {
        (*placements)[i] = new CounterPlacement(counter.placements[i]);
      }
    }

    inline
    void
    Counter::init(::NewsGate::Moderation::Ad::Counter& counter) const
      throw(El::Exception)
    {
      counter.id = id;
      counter.advertiser = advertiser;
      counter.name = name.c_str();
      counter.status = status;
      counter.text = text.c_str();
/*
      counter.placements.length(placements->size());

      for(size_t i = 0; i < counter.placements.length(); ++i)
      {
        CounterPlacement::Type::down_cast((*placements)[i].in(), false)->
          init(counter.placements[i]);
      }
*/
    }

    //
    // NewsGate::AdModeration::Counter::Type class
    //
    inline
    Counter::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<Counter,
                                     Counter::Type>(
        "newsgate.moderation.ad.Counter",
        "Object encapsulating counter options")
    {
    }
    
    //
    // NewsGate::AdModeration::CounterUpdate class
    //
    inline
    CounterUpdate::CounterUpdate(PyTypeObject *type,
                                 PyObject *args,
                                 PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          advertiser(0),
          status(Moderation::Ad::US_ENABLED)
    {
    }

    inline
    void
    CounterUpdate::init(::NewsGate::Moderation::Ad::CounterUpdate& update)
      const throw(El::Exception)
    {
      update.id = id;
      update.advertiser = advertiser;
      update.name = name.c_str();
      update.status = status;
      update.text = text.c_str();
    }

    //
    // NewsGate::AdModeration::CounterUpdate::Type class
    //
    inline
    CounterUpdate::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<CounterUpdate,
                                     CounterUpdate::Type>(
        "newsgate.moderation.ad.CounterUpdate",
        "Object encapsulating counter update options")
    {
    }

    //
    // NewsGate::AdModeration::Condition class
    //
    inline
    Condition::Condition(PyTypeObject *type,
                         PyObject *args,
                         PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          advertiser(0),
          status(Moderation::Ad::CS_ENABLED),
          rnd_mod(0),
          rnd_mod_from(0),
          rnd_mod_to(0),
          group_freq_cap(0),
          group_count_cap(0),
          query_types(0),
          query_type_exclusions(0)
    {
    }

    inline
    Condition::Condition(const ::NewsGate::Moderation::Ad::Condition& cond)
      throw(El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          id(cond.id),
          advertiser(cond.advertiser),
          name(cond.name.in()),
          status(cond.status),
          rnd_mod(cond.rnd_mod),
          rnd_mod_from(cond.rnd_mod_from),
          rnd_mod_to(cond.rnd_mod_to),
          group_freq_cap(cond.group_freq_cap),
          group_count_cap(cond.group_count_cap),
          query_types(cond.query_types),
          query_type_exclusions(cond.query_type_exclusions),
          page_sources(cond.page_sources.in()),
          page_source_exclusions(cond.page_source_exclusions.in()),
          message_sources(cond.message_sources.in()),
          message_source_exclusions(cond.message_source_exclusions.in()),
          page_categories(cond.page_categories.in()),
          page_category_exclusions(cond.page_category_exclusions.in()),
          message_categories(cond.message_categories.in()),
          message_category_exclusions(cond.message_category_exclusions.in()),
          search_engines(cond.search_engines.in()),
          search_engine_exclusions(cond.search_engine_exclusions.in()),
          crawlers(cond.crawlers.in()),
          crawler_exclusions(cond.crawler_exclusions.in()),
          languages(cond.languages.in()),
          language_exclusions(cond.language_exclusions.in()),
          countries(cond.countries.in()),
          country_exclusions(cond.country_exclusions.in()),
          ip_masks(cond.ip_masks.in()),
          ip_mask_exclusions(cond.ip_mask_exclusions.in()),
          tags(cond.tags.in()),
          tag_exclusions(cond.tag_exclusions.in()),
          referers(cond.referers.in()),
          referer_exclusions(cond.referer_exclusions.in()),
          content_languages(cond.content_languages.in()),
          content_language_exclusions(cond.content_language_exclusions.in())
    {
    }

    inline
    void
    Condition::init(::NewsGate::Moderation::Ad::Condition& cond) const
      throw(El::Exception)
    {
      cond.id = id;
      cond.advertiser = advertiser;
      cond.name = name.c_str();
      cond.status = status;
      cond.rnd_mod = rnd_mod;
      cond.rnd_mod_from = rnd_mod_from;
      cond.rnd_mod_to = rnd_mod_to;
      cond.group_freq_cap = group_freq_cap;
      cond.group_count_cap = group_count_cap;
      cond.query_types = query_types;
      cond.query_type_exclusions = query_type_exclusions;
      cond.page_sources = page_sources.c_str();
      cond.page_source_exclusions = page_source_exclusions.c_str();
      cond.message_sources = message_sources.c_str();
      cond.message_source_exclusions = message_source_exclusions.c_str();
      cond.page_categories = page_categories.c_str();
      cond.page_category_exclusions = page_category_exclusions.c_str();
      cond.message_categories = message_categories.c_str();
      cond.message_category_exclusions = message_category_exclusions.c_str();
      cond.search_engines = search_engines.c_str();
      cond.search_engine_exclusions = search_engine_exclusions.c_str();
      cond.crawlers = crawlers.c_str();
      cond.crawler_exclusions = crawler_exclusions.c_str();
      cond.languages = languages.c_str();
      cond.language_exclusions = language_exclusions.c_str();
      cond.countries = countries.c_str();
      cond.country_exclusions = country_exclusions.c_str();
      cond.ip_masks = ip_masks.c_str();
      cond.ip_mask_exclusions = ip_mask_exclusions.c_str();
      cond.tags = tags.c_str();
      cond.tag_exclusions = tag_exclusions.c_str();
      cond.referers = referers.c_str();
      cond.referer_exclusions = referer_exclusions.c_str();
      cond.content_languages = content_languages.c_str();
      cond.content_language_exclusions = content_language_exclusions.c_str();
    }
    
    //
    // NewsGate::AdModeration::Condition::Type class
    //
    inline
    Condition::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<Condition,
                                     Condition::Type>(
        "newsgate.moderation.ad.Condition",
        "Object encapsulating conditions options")
    {
    }
    
    //
    // NewsGate::AdModeration::ConditionUpdate class
    //
    inline
    ConditionUpdate::ConditionUpdate(PyTypeObject *type,
                                     PyObject *args,
                                     PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          advertiser(0),
          status(Moderation::Ad::CS_ENABLED),
          rnd_mod(0),
          rnd_mod_from(0),
          rnd_mod_to(0),
          group_freq_cap(0),
          group_count_cap(0),
          query_types(0),
          query_type_exclusions(0)
    {
    }

    inline
    void
    ConditionUpdate::init(::NewsGate::Moderation::Ad::ConditionUpdate& update)
      const throw(El::Exception)
    {
      update.id = id;
      update.advertiser = advertiser;
      update.name = name.c_str();
      update.status = status;
      update.rnd_mod = rnd_mod;
      update.rnd_mod_from = rnd_mod_from;
      update.rnd_mod_to = rnd_mod_to;
      update.group_freq_cap = group_freq_cap;
      update.group_count_cap = group_count_cap;
      update.query_types = query_types;
      update.query_type_exclusions = query_type_exclusions;
      update.page_sources = page_sources.c_str();
      update.page_source_exclusions = page_source_exclusions.c_str();
      update.message_sources = message_sources.c_str();
      update.message_source_exclusions = message_source_exclusions.c_str();
      update.page_categories = page_categories.c_str();
      update.page_category_exclusions = page_category_exclusions.c_str();
      update.message_categories = message_categories.c_str();
      update.message_category_exclusions = message_category_exclusions.c_str();
      update.search_engines = search_engines.c_str();
      update.search_engine_exclusions = search_engine_exclusions.c_str();
      update.crawlers = crawlers.c_str();
      update.crawler_exclusions = crawler_exclusions.c_str();
      update.languages = languages.c_str();
      update.language_exclusions = language_exclusions.c_str();
      update.countries = countries.c_str();
      update.country_exclusions = country_exclusions.c_str();
      update.ip_masks = ip_masks.c_str();
      update.ip_mask_exclusions = ip_mask_exclusions.c_str();
      update.tags = tags.c_str();
      update.tag_exclusions = tag_exclusions.c_str();
      update.referers = referers.c_str();
      update.referer_exclusions = referer_exclusions.c_str();
      update.content_languages = content_languages.c_str();
      update.content_language_exclusions = content_language_exclusions.c_str();
    }
    
    //
    // NewsGate::AdModeration::ConditionUpdate::Type class
    //
    inline
    ConditionUpdate::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<ConditionUpdate,
                                     ConditionUpdate::Type>(
        "newsgate.moderation.ad.ConditionUpdate",
        "Object encapsulating condition update options")
    {
    }
    
    //
    // NewsGate::AdModeration::Campaign class
    //
    inline
    Campaign::Campaign(PyTypeObject *type,
                       PyObject *args,
                       PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          advertiser(0),
          status(Moderation::Ad::MS_ENABLED),
          conditions(new El::Python::Sequence()),
          groups(new El::Python::Sequence())
    {
    }

    inline
    Campaign::Campaign(const ::NewsGate::Moderation::Ad::Campaign& cmp)
      throw(El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          id(cmp.id),
          advertiser(cmp.advertiser),
          name(cmp.name.in()),
          status(cmp.status),
          conditions(new El::Python::Sequence()),
          groups(new El::Python::Sequence())
    {
      conditions->resize(cmp.conditions.length());
      
      for(size_t i = 0; i < cmp.conditions.length(); ++i)
      {
        (*conditions)[i] = new Condition(cmp.conditions[i]);
      }

      groups->resize(cmp.groups.length());
      
      for(size_t i = 0; i < cmp.groups.length(); ++i)
      {
        (*groups)[i] = new Group(cmp.groups[i]);
      }
    }

    inline
    void
    Campaign::init(::NewsGate::Moderation::Ad::Campaign& cmp) const
      throw(El::Exception)
    {
      cmp.id = id;
      cmp.advertiser = advertiser;
      cmp.name = name.c_str();
      cmp.status = status;

      cmp.conditions.length(conditions->size());

      for(size_t i = 0; i < cmp.conditions.length(); ++i)
      {
        Condition::Type::down_cast((*conditions)[i].in(), false)->
          init(cmp.conditions[i]);
      }

      cmp.groups.length(groups->size());

      for(size_t i = 0; i < cmp.groups.length(); ++i)
      {
        Group::Type::down_cast((*groups)[i].in(), false)->init(cmp.groups[i]);
      }
    }
    
    //
    // NewsGate::AdModeration::Campaign::Type class
    //
    inline
    Campaign::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<Campaign,
                                     Campaign::Type>(
        "newsgate.moderation.ad.Campaign",
        "Object encapsulating campaign options")
    {
    }
    
    //
    // NewsGate::AdModeration::CampaignUpdate class
    //
    inline
    CampaignUpdate::CampaignUpdate(PyTypeObject *type,
                                   PyObject *args,
                                   PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          advertiser(0),
          status(Moderation::Ad::MS_ENABLED),
          conditions(new El::Python::Sequence())
    {
    }

    inline
    void
    CampaignUpdate::init(::NewsGate::Moderation::Ad::CampaignUpdate& update)
      const throw(El::Exception)
    {
      update.id = id;
      update.advertiser = advertiser;
      update.name = name.c_str();
      update.status = status;

      update.conditions.length(conditions->size());

      for(size_t i = 0; i < update.conditions.length(); ++i)
      {
        PyObject* obj = (*conditions)[i].in();

        if(PyLong_Check(obj))
        {
          update.conditions[i] = PyLong_AsUnsignedLongLong(obj);
        }
      }      
    }
    
    //
    // NewsGate::AdModeration::CampaignUpdate::Type class
    //
    inline
    CampaignUpdate::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<CampaignUpdate,
                                     CampaignUpdate::Type>(
        "newsgate.moderation.ad.CampaignUpdate",
        "Object encapsulating campaign update options")
    {
    }
    
    //
    // NewsGate::AdModeration::Placement class
    //
    inline
    Placement::Placement(PyTypeObject *type,
                         PyObject *args,
                         PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          group(0),
          campaign(0),
          advertiser(0),
          status(Moderation::Ad::TS_ENABLED),
          slot(new Slot()),
          ad(new Advert()),
          cpm(0),
          inject(Moderation::Ad::PI_DIRECT)
    {
    }

    inline
    Placement::Placement(const ::NewsGate::Moderation::Ad::Placement& plc)
      throw(El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          id(plc.id),
          group(plc.group),
          group_name(plc.group_name.in()),
          campaign(plc.campaign),
          campaign_name(plc.campaign_name.in()),
          advertiser(plc.advertiser),
          name(plc.name.in()),
          status(plc.status),
          slot(new Slot(plc.placement_slot)),
          ad(new Advert(plc.ad)),
          cpm(plc.cpm),
          inject(plc.inject),
          display_status(plc.display_status.in())
    {
    }

    inline
    void
    Placement::init(::NewsGate::Moderation::Ad::Placement& plc) const
      throw(El::Exception)
    {
      plc.id = id;
      plc.group = group;
      plc.group_name = group_name.c_str();
      plc.campaign = campaign;
      plc.campaign_name = campaign_name.c_str();
      plc.advertiser = advertiser;
      plc.name = name.c_str();
      plc.status = status;
      slot->init(plc.placement_slot);
      ad->init(plc.ad);
      plc.cpm = cpm;
      plc.inject = inject;
      plc.display_status = display_status.c_str();
    }
    
    //
    // NewsGate::AdModeration::Placement::Type class
    //
    inline
    Placement::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<Placement,
                                     Placement::Type>(
        "newsgate.moderation.ad.Placement",
        "Object encapsulating placement options")
    {
    }
    
    //
    // NewsGate::AdModeration::PlacementUpdate class
    //
    inline
    PlacementUpdate::PlacementUpdate(PyTypeObject *type,
                                     PyObject *args,
                                     PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          advertiser(0),
          status(Moderation::Ad::TS_ENABLED),
          slot(0),
          ad(0),
          cpm(0),
          inject(Moderation::Ad::PI_DIRECT)
    {
    }

    inline
    void
    PlacementUpdate::init(::NewsGate::Moderation::Ad::PlacementUpdate& update)
      const throw(El::Exception)
    {
      update.id = id;
      update.advertiser = advertiser;
      update.name = name.c_str();
      update.status = status;
      update.slot = slot;
      update.ad = ad;
      update.cpm = cpm;
      update.inject = inject;      
    }
    
    //
    // NewsGate::AdModeration::PlacementUpdate::Type class
    //
    inline
    PlacementUpdate::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<PlacementUpdate,
                                     PlacementUpdate::Type>(
        "newsgate.moderation.ad.PlacementUpdate",
        "Object encapsulating placement update options")
    {
    }
    
    //
    // NewsGate::AdModeration::CounterPlacement class
    //
    inline
    CounterPlacement::CounterPlacement(PyTypeObject *type,
                                       PyObject *args,
                                       PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          group(0),
          campaign(0),
          advertiser(0),
          status(Moderation::Ad::OS_ENABLED),
          page(new Page()),
          counter(new Counter())
    {
    }

    inline
    CounterPlacement::CounterPlacement(
      const ::NewsGate::Moderation::Ad::CounterPlacement& plc)
      throw(El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          id(plc.id),
          group(plc.group),
          group_name(plc.group_name.in()),
          campaign(plc.campaign),
          campaign_name(plc.campaign_name.in()),
          advertiser(plc.advertiser),
          name(plc.name.in()),
          status(plc.status),
          page(new Page(plc.placement_page)),
          counter(new Counter(plc.placement_counter)),
          display_status(plc.display_status.in())
    {
    }

    inline
    void
    CounterPlacement::init(::NewsGate::Moderation::Ad::CounterPlacement& plc)
      const throw(El::Exception)
    {
      plc.id = id;
      plc.group = group;
      plc.group_name = group_name.c_str();
      plc.campaign = campaign;
      plc.campaign_name = campaign_name.c_str();
      plc.advertiser = advertiser;
      plc.name = name.c_str();
      plc.status = status;
      plc.placement_page.id = page->id;
      plc.placement_counter.id = counter->id;
      plc.display_status = display_status.c_str();
    }

    //
    // NewsGate::AdModeration::CounterPlacement::Type class
    //
    inline
    CounterPlacement::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<CounterPlacement,
                                     CounterPlacement::Type>(
        "newsgate.moderation.ad.CounterPlacement",
        "Object encapsulating placement options")
    {
    }
    
    //
    // NewsGate::AdModeration::CounterPlacementUpdate class
    //
    inline
    CounterPlacementUpdate::CounterPlacementUpdate(PyTypeObject *type,
                                                   PyObject *args,
                                                   PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          advertiser(0),
          status(Moderation::Ad::OS_ENABLED),
          page(0),
          counter(0)
    {
    }

    inline
    void
    CounterPlacementUpdate::init(
      ::NewsGate::Moderation::Ad::CounterPlacementUpdate& update)
      const throw(El::Exception)
    {
      update.id = id;
      update.advertiser = advertiser;
      update.name = name.c_str();
      update.status = status;
      update.page = page;
      update.counter = counter;
    }
    
    //
    // NewsGate::AdModeration::CounterPlacementUpdate::Type class
    //
    inline
    CounterPlacementUpdate::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<CounterPlacementUpdate,
                                     CounterPlacementUpdate::Type>(
        "newsgate.moderation.ad.CounterPlacementUpdate",
        "Object encapsulating placement update options")
    {
    }
    
    //
    // NewsGate::AdModeration::Group class
    //
    inline
    Group::Group(PyTypeObject *type,
                 PyObject *args,
                 PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          campaign(0),
          advertiser(0),
          status(Moderation::Ad::GS_ENABLED),
          auction_factor(1),
          conditions(new El::Python::Sequence()),
          ad_placements(new El::Python::Sequence()),
          counter_placements(new El::Python::Sequence())
    {
    }

    inline
    Group::Group(const ::NewsGate::Moderation::Ad::Group& grp)
      throw(El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          id(grp.id),
          campaign(grp.campaign),
          campaign_name(grp.campaign_name.in()),
          advertiser(grp.advertiser),
          name(grp.name.in()),
          status(grp.status),
          auction_factor(grp.auction_factor),
          conditions(new El::Python::Sequence()),
          ad_placements(new El::Python::Sequence()),
          counter_placements(new El::Python::Sequence())
    {
      conditions->resize(grp.conditions.length());
      
      for(size_t i = 0; i < grp.conditions.length(); ++i)
      {
        (*conditions)[i] = new Condition(grp.conditions[i]);
      }

      ad_placements->resize(grp.ad_placements.length());

      for(size_t i = 0; i < grp.ad_placements.length(); ++i)
      {
        (*ad_placements)[i] = new Placement(grp.ad_placements[i]);
      }

      counter_placements->resize(grp.counter_placements.length());

      for(size_t i = 0; i < grp.counter_placements.length(); ++i)
      {
        (*counter_placements)[i] =
          new CounterPlacement(grp.counter_placements[i]);
      }
    }

    inline
    void
    Group::init(::NewsGate::Moderation::Ad::Group& grp) const
      throw(El::Exception)
    {
      grp.id = id;
      grp.campaign = campaign;
      grp.campaign_name = campaign_name.c_str();
      grp.advertiser = advertiser;
      grp.name = name.c_str();
      grp.status = status;
      grp.auction_factor = auction_factor;

      grp.conditions.length(conditions->size());

      for(size_t i = 0; i < grp.conditions.length(); ++i)
      {
        Condition::Type::down_cast((*conditions)[i].in(), false)->
          init(grp.conditions[i]);
      }

      grp.ad_placements.length(ad_placements->size());

      for(size_t i = 0; i < grp.ad_placements.length(); ++i)
      {
        Placement::Type::down_cast((*ad_placements)[i].in(), false)->
          init(grp.ad_placements[i]);
      }

      grp.counter_placements.length(counter_placements->size());

      for(size_t i = 0; i < grp.counter_placements.length(); ++i)
      {
        CounterPlacement::Type::down_cast((*counter_placements)[i].in(),
                                         false)->
          init(grp.counter_placements[i]);
      }
    }
    
    //
    // NewsGate::AdModeration::Group::Type class
    //
    inline
    Group::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<Group,
                                     Group::Type>(
        "newsgate.moderation.ad.Group",
        "Object encapsulating campaign options")
    {
    }
    
    //
    // NewsGate::AdModeration::GroupUpdate class
    //
    inline
    GroupUpdate::GroupUpdate(PyTypeObject *type,
                             PyObject *args,
                             PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          advertiser(0),
          status(Moderation::Ad::GS_ENABLED),
          auction_factor(1),
          conditions(new El::Python::Sequence()),
          reset_cap_min_time(false)
    {
    }

    inline
    void
    GroupUpdate::init(::NewsGate::Moderation::Ad::GroupUpdate& update)
      const throw(El::Exception)
    {
      update.id = id;
      update.advertiser = advertiser;
      update.name = name.c_str();
      update.status = status;
      update.auction_factor = auction_factor;
      update.reset_cap_min_time = reset_cap_min_time;

      update.conditions.length(conditions->size());

      for(size_t i = 0; i < update.conditions.length(); ++i)
      {
        PyObject* obj = (*conditions)[i].in();

        if(PyLong_Check(obj))
        {
          update.conditions[i] = PyLong_AsUnsignedLongLong(obj);
        }
      }
    }
    
    //
    // NewsGate::AdModeration::GroupUpdate::Type class
    //
    inline
    GroupUpdate::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<GroupUpdate,
                                     GroupUpdate::Type>(
        "newsgate.moderation.ad.GroupUpdate",
        "Object encapsulating group update options")
    {
    }    
    //
    // NewsGate::AdModeration::Slot class
    //
    inline
    Slot::Slot(PyTypeObject *type,
               PyObject *args,
               PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          status(Moderation::Ad::SS_ENABLED),
          max_width(0),
          min_width(0),
          max_height(0),
          min_height(0)
    {
    }

    inline
    Slot::Slot(const ::NewsGate::Moderation::Ad::Slot& slot)
      throw(El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          id(slot.id),
          name(slot.name.in()),
          status(slot.status),
          max_width(slot.max_width),
          min_width(slot.min_width),
          max_height(slot.max_height),
          min_height(slot.min_height)
    {
    }
    
    inline
    void
    Slot::init(::NewsGate::Moderation::Ad::Slot& slot) const
      throw(El::Exception)
    {
      slot.id = id;
      slot.name = name.c_str();
      slot.status = status;
      slot.max_width = max_width;
      slot.min_width = min_width;
      slot.max_height = max_height;
      slot.min_height = min_height;      
    }
    
    //
    // NewsGate::AdModeration::Slot::Type class
    //
    inline
    Slot::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<Slot,
                                     Slot::Type>(
        "newsgate.moderation.ad.Slot",
        "Object encapsulating ad slot options")
    {
    }    
    
    //
    // NewsGate::AdModeration::SlotUpdate class
    //
    inline
    SlotUpdate::SlotUpdate(PyTypeObject *type,
                           PyObject *args,
                           PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          status(Moderation::Ad::SS_ENABLED)
    {
    }

    inline
    void
    SlotUpdate::init(::NewsGate::Moderation::Ad::SlotUpdate& update) const
      throw(El::Exception)
    {
      update.id = id;
      update.status = status;
    }

    //
    // NewsGate::AdModeration::SlotUpdate::Type class
    //
    inline
    SlotUpdate::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<SlotUpdate,
                                     SlotUpdate::Type>(
        "newsgate.moderation.ad.SlotUpdate",
        "Object encapsulating ad slot update options")
    {
    } 

    //
    // NewsGate::AdModeration::Size class
    //
    inline
    Size::Size(PyTypeObject *type,
               PyObject *args,
               PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          status(Moderation::Ad::ZS_ENABLED),
          width(0),
          height(0)
    {
    }

    inline
    Size::Size(const ::NewsGate::Moderation::Ad::Size& size)
      throw(El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          id(size.id),
          name(size.name.in()),
          status(size.status),
          width(size.width),
          height(size.height)
    {
    }
    
    //
    // NewsGate::AdModeration::Size::Type class
    //
    inline
    Size::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<Size,
                                     Size::Type>(
        "newsgate.moderation.ad.Size",
        "Object encapsulating ad size options")
    {
    }
    
    //
    // NewsGate::AdModeration::SizeUpdate class
    //
    inline
    SizeUpdate::SizeUpdate(PyTypeObject *type,
                           PyObject *args,
                           PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          status(Moderation::Ad::ZS_ENABLED)
    {
    }

    inline
    void
    SizeUpdate::init(::NewsGate::Moderation::Ad::SizeUpdate& update) const
      throw(El::Exception)
    {
      update.id = id;
      update.status = status;
    }

    //
    // NewsGate::AdModeration::SizeUpdate::Type class
    //
    inline
    SizeUpdate::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<SizeUpdate,
                                     SizeUpdate::Type>(
        "newsgate.moderation.ad.SizeUpdate",
        "Object encapsulating ad size update options")
    {
    } 
  }
}

#endif // _NEWSGATE_SERVER_FRONTENDS_MODERATION_ADMODERATION_HPP_
