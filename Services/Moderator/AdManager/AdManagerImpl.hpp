/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/AdManager/AdManagerImpl.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MODERATOR_ADMANAGER_ADMANAGERIMPL_HPP_
#define _NEWSGATE_SERVER_SERVICES_MODERATOR_ADMANAGER_ADMANAGERIMPL_HPP_

#include <string>

#include <ext/hash_map>
#include <google/sparse_hash_map>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/Service/CompoundService.hpp>
#include <El/Hash/Hash.hpp>
#include <El/String/StringPtr.hpp>
#include <El/Guid.hpp>
#include <El/MySQL/DB.hpp>

#include <Services/Moderator/Commons/AdManager_s.hpp>

#include "DB_Record.hpp"

namespace NewsGate
{
  namespace Moderation
  {
    namespace Ad
    {
      class AdManagerImpl :
        public virtual POA_NewsGate::Moderation::Ad::AdManager,
        public virtual El::Service::CompoundService<>
      {
      public:
        EL_EXCEPTION(Exception, El::ExceptionBase);
        EL_EXCEPTION(InvalidArgument, Exception);

      public:

        AdManagerImpl(El::Service::Callback* callback)
          throw(InvalidArgument, Exception, El::Exception);

        virtual ~AdManagerImpl() throw();

      protected:

        //
        // IDL:NewsGate/Moderation/AdManager/get_global:1.0
        //
        virtual ::NewsGate::Moderation::Ad::Global* get_global(
          ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id)
          throw(NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);      
        
        //
        // IDL:NewsGate/Moderation/AdManager/update_global:1.0
        //
        virtual ::NewsGate::Moderation::Ad::Global* update_global(
          const ::NewsGate::Moderation::Ad::GlobalUpdate& update)
          throw(NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);      
        
        //
        // IDL:NewsGate/Moderation/AdManager/get_pages:1.0
        //
        virtual ::NewsGate::Moderation::Ad::PageSeq* get_pages(
          ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id)
          throw(NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);      

        //
        // IDL:NewsGate/Moderation/AdManager/get_page:1.0
        //
        virtual ::NewsGate::Moderation::Ad::Page* get_page(
          ::NewsGate::Moderation::Ad::PageId id,
          ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id)
          throw(NewsGate::Moderation::ObjectNotFound,
                NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);      

        //
        // IDL:NewsGate/Moderation/AdManager/update_page:1.0
        //
        virtual ::NewsGate::Moderation::Ad::Page* update_page(
          const ::NewsGate::Moderation::Ad::PageUpdate& page)
          throw(NewsGate::Moderation::ObjectNotFound,
                NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);

        //
        // IDL:NewsGate/Moderation/AdManager/get_sizes:1.0
        //
        virtual ::NewsGate::Moderation::Ad::SizeSeq* get_sizes()
          throw(NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);
        
        //
        // IDL:NewsGate/Moderation/AdManager/update_sizes:1.0
        //
        virtual ::NewsGate::Moderation::Ad::SizeSeq* update_sizes(
          const ::NewsGate::Moderation::Ad::SizeUpdateSeq& sizes)
          throw(NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);

        //
        // IDL:NewsGate/Moderation/AdManager/get_ads:1.0
        //
        virtual ::NewsGate::Moderation::Ad::AdvertSeq* get_ads(
          ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id)
          throw(NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);
          
        //
        // IDL:NewsGate/Moderation/AdManager/get_ad:1.0
        //
        virtual ::NewsGate::Moderation::Ad::Advert* get_ad(
          const ::NewsGate::Moderation::Ad::AdvertId id,
          const ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id)
          throw(NewsGate::Moderation::ObjectNotFound,
                NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);
        
        //
        // IDL:NewsGate/Moderation/AdManager/update_ad:1.0
        //
        virtual ::NewsGate::Moderation::Ad::Advert* update_ad(
          const ::NewsGate::Moderation::Ad::AdvertUpdate& ad)
          throw(NewsGate::Moderation::ObjectNotFound,
                NewsGate::Moderation::ObjectAlreadyExist,
                NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);

        //
        // IDL:NewsGate/Moderation/AdManager/create_ad:1.0
        //
        virtual ::NewsGate::Moderation::Ad::Advert* create_ad(
          const ::NewsGate::Moderation::Ad::Advert& ad)
          throw(NewsGate::Moderation::ObjectAlreadyExist,
                NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);        
        
        //
        // IDL:NewsGate/Moderation/AdManager/get_counters:1.0
        //
        virtual ::NewsGate::Moderation::Ad::CounterSeq* get_counters(
          ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id)
          throw(NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);
          
        //
        // IDL:NewsGate/Moderation/AdManager/get_counter:1.0
        //
        virtual ::NewsGate::Moderation::Ad::Counter* get_counter(
          const ::NewsGate::Moderation::Ad::CounterId id,
          const ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id)
          throw(NewsGate::Moderation::ObjectNotFound,
                NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);
        
        //
        // IDL:NewsGate/Moderation/AdManager/update_counter:1.0
        //
        virtual ::NewsGate::Moderation::Ad::Counter* update_counter(
          const ::NewsGate::Moderation::Ad::CounterUpdate& counter)
          throw(NewsGate::Moderation::ObjectNotFound,
                NewsGate::Moderation::ObjectAlreadyExist,
                NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);

        //
        // IDL:NewsGate/Moderation/AdManager/create_counter:1.0
        //
        virtual ::NewsGate::Moderation::Ad::Counter* create_counter(
          const ::NewsGate::Moderation::Ad::Counter& counter)
          throw(NewsGate::Moderation::ObjectAlreadyExist,
                NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);
        
        //
        // IDL:NewsGate/Moderation/AdManager/get_conditions:1.0
        //
        virtual ::NewsGate::Moderation::Ad::ConditionSeq* get_conditions(
            ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id)
          throw(NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);

        //
        // IDL:NewsGate/Moderation/AdManager/get_condition:1.0
        //
        virtual ::NewsGate::Moderation::Ad::Condition* get_condition(
            ::NewsGate::Moderation::Ad::ConditionId id,
            ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id)
          throw(NewsGate::Moderation::ObjectNotFound,
                NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);
        
        //
        // IDL:NewsGate/Moderation/AdManager/update_condition:1.0
        //
        virtual ::NewsGate::Moderation::Ad::Condition* update_condition(
          const ::NewsGate::Moderation::Ad::ConditionUpdate& update)
          throw(NewsGate::Moderation::ObjectNotFound,
                NewsGate::Moderation::ObjectAlreadyExist,
                NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);
        
        //
        // IDL:NewsGate/Moderation/AdManager/create_condition:1.0
        //
        virtual ::NewsGate::Moderation::Ad::Condition* create_condition(
          const ::NewsGate::Moderation::Ad::Condition& condition)
          throw(NewsGate::Moderation::ObjectAlreadyExist,
                NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);
        
        //
        // IDL:NewsGate/Moderation/AdManager/get_campaigns:1.0
        //
        virtual ::NewsGate::Moderation::Ad::CampaignSeq* get_campaigns(
            ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id)
          throw(NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);

        //
        // IDL:NewsGate/Moderation/AdManager/get_campaign:1.0
        //
        virtual ::NewsGate::Moderation::Ad::Campaign* get_campaign(
            ::NewsGate::Moderation::Ad::CampaignId id,
            ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id)
          throw(NewsGate::Moderation::ObjectNotFound,
                NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);
        
        //
        // IDL:NewsGate/Moderation/AdManager/update_campaign:1.0
        //
        virtual ::NewsGate::Moderation::Ad::Campaign* update_campaign(
          const ::NewsGate::Moderation::Ad::CampaignUpdate& update)
          throw(NewsGate::Moderation::ObjectNotFound,
                NewsGate::Moderation::ObjectAlreadyExist,
                NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);
        
        //
        // IDL:NewsGate/Moderation/AdManager/create_campaign:1.0
        //
        virtual ::NewsGate::Moderation::Ad::Campaign* create_campaign(
          const ::NewsGate::Moderation::Ad::Campaign& campaign)
          throw(NewsGate::Moderation::ObjectAlreadyExist,
                NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);        

        //
        // IDL:NewsGate/Moderation/AdManager/get_group:1.0
        //
        virtual ::NewsGate::Moderation::Ad::Group* get_group(
            ::NewsGate::Moderation::Ad::GroupId id,
            ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id)
          throw(NewsGate::Moderation::ObjectNotFound,
                NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);
        
        //
        // IDL:NewsGate/Moderation/AdManager/update_group:1.0
        //
        virtual ::NewsGate::Moderation::Ad::Group* update_group(
          const ::NewsGate::Moderation::Ad::GroupUpdate& update)
          throw(NewsGate::Moderation::ObjectNotFound,
                NewsGate::Moderation::ObjectAlreadyExist,
                NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);
        
        //
        // IDL:NewsGate/Moderation/AdManager/create_group:1.0
        //
        virtual ::NewsGate::Moderation::Ad::Group* create_group(
          const ::NewsGate::Moderation::Ad::Group& grp)
          throw(NewsGate::Moderation::ObjectAlreadyExist,
                NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);        
        
        //
        // IDL:NewsGate/Moderation/AdManager/get_placement:1.0
        //
        virtual ::NewsGate::Moderation::Ad::Placement* get_placement(
            ::NewsGate::Moderation::Ad::GroupId id,
            ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id)
          throw(NewsGate::Moderation::ObjectNotFound,
                NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);
        
        //
        // IDL:NewsGate/Moderation/AdManager/update_placement:1.0
        //
        virtual ::NewsGate::Moderation::Ad::Placement* update_placement(
          const ::NewsGate::Moderation::Ad::PlacementUpdate& update)
          throw(NewsGate::Moderation::ObjectNotFound,
                NewsGate::Moderation::ObjectAlreadyExist,
//                NewsGate::Moderation::InvalidObject,
                NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);
        
        //
        // IDL:NewsGate/Moderation/AdManager/create_placement:1.0
        //
        virtual ::NewsGate::Moderation::Ad::Placement* create_placement(
          const ::NewsGate::Moderation::Ad::Placement& plc)
          throw(NewsGate::Moderation::ObjectAlreadyExist,
                NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);        
        
        //
        // IDL:NewsGate/Moderation/AdManager/get_counter_placement:1.0
        //
        virtual ::NewsGate::Moderation::Ad::CounterPlacement*
        get_counter_placement(
            ::NewsGate::Moderation::Ad::GroupId id,
            ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id)
          throw(NewsGate::Moderation::ObjectNotFound,
                NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);
        
        //
        // IDL:NewsGate/Moderation/AdManager/update_counter_placement:1.0
        //
        virtual ::NewsGate::Moderation::Ad::CounterPlacement*
        update_counter_placement(
          const ::NewsGate::Moderation::Ad::CounterPlacementUpdate& update)
          throw(NewsGate::Moderation::ObjectNotFound,
                NewsGate::Moderation::ObjectAlreadyExist,
                NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);
        
        //
        // IDL:NewsGate/Moderation/AdManager/create_counter_placement:1.0
        //
        virtual ::NewsGate::Moderation::Ad::CounterPlacement*
        create_counter_placement(
          const ::NewsGate::Moderation::Ad::CounterPlacement& plc)
          throw(NewsGate::Moderation::ObjectAlreadyExist,
                NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);        
        
        virtual bool notify(El::Service::Event* event) throw(El::Exception);

        static void load_slots(PageId page_id,
                               El::MySQL::Connection* connection,
                               SlotSeq& slots)
          throw(El::Exception);

        static void load_adv_restrictions(
          PageId page_id,
          AdvertiserId advertiser_id,
          El::MySQL::Connection* connection,
          AdvertiserMaxAdNumSeq& adv_max_ad_nums)
          throw(El::Exception);
        
        static Page* load_page(PageId id,
                               AdvertiserId advertiser_id,
                               El::MySQL::Connection* connection)
          throw(NewsGate::Moderation::ObjectNotFound,
                El::Exception);

        static Global* load_global(
          ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
          El::MySQL::Connection* connection)
          throw(NewsGate::Moderation::ImplementationException,
                El::Exception);

        static SizeSeq* load_sizes(El::MySQL::Connection* connection)
          throw(El::Exception);

        static AdvertSeq* load_ads(
          ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
          El::MySQL::Connection* connection)
          throw(El::Exception);

        static Advert* load_ad(
          ::NewsGate::Moderation::Ad::AdvertId id,
          ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
          El::MySQL::Connection* connection)
          throw(NewsGate::Moderation::ObjectNotFound, El::Exception);

        static CounterSeq* load_counters(
          ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
          El::MySQL::Connection* connection)
          throw(El::Exception);

        static Counter* load_counter(
          ::NewsGate::Moderation::Ad::CounterId id,
          ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
          El::MySQL::Connection* connection)
          throw(NewsGate::Moderation::ObjectNotFound, El::Exception);

        static ConditionSeq* load_conditions(
          ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
          El::MySQL::Connection* connection)
          throw(El::Exception);

        static Condition* load_condition(
          ::NewsGate::Moderation::Ad::ConditionId id,
          ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
          El::MySQL::Connection* connection)
          throw(NewsGate::Moderation::ObjectNotFound, El::Exception);

        static CampaignSeq* load_campaigns(
          ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
          El::MySQL::Connection* connection)
          throw(El::Exception);

        static Campaign* load_campaign(
          ::NewsGate::Moderation::Ad::CampaignId id,
          ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
          El::MySQL::Connection* connection)
          throw(NewsGate::Moderation::ObjectNotFound, El::Exception);        

        static void load_groups(
          GroupSeq& groups,
          ::NewsGate::Moderation::Ad::CampaignId id,
          ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
          El::MySQL::Connection* connection)
          throw(El::Exception);        

        static Group* load_group(
          ::NewsGate::Moderation::Ad::GroupId id,
          ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
          El::MySQL::Connection* connection)
          throw(NewsGate::Moderation::ObjectNotFound, El::Exception);        

        static void load_group_conditions(
          ConditionSeq& conditions,
          ::NewsGate::Moderation::Ad::GroupId id,
          ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
          El::MySQL::Connection* connection)
          throw(El::Exception);
        
        static void load_campaign_conditions(
          ConditionSeq& conditions,
          ::NewsGate::Moderation::Ad::CampaignId id,
          ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
          El::MySQL::Connection* connection)
          throw(El::Exception);
        
        static void load_placement(
          Placement& plc,
          ::NewsGate::Moderation::Ad::PlacementId id,
          ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
          El::MySQL::Connection* connection)
          throw(NewsGate::Moderation::ObjectNotFound,
                El::Exception);

        static void load_counter_placement(
          CounterPlacement& plc,
          ::NewsGate::Moderation::Ad::CounterPlacementId id,
          ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
          El::MySQL::Connection* connection)
          throw(NewsGate::Moderation::ObjectNotFound,
                El::Exception);

        static void load_group_placements(
          PlacementSeq& placements,
          ::NewsGate::Moderation::Ad::GroupId id,
          ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
          El::MySQL::Connection* connection)
          throw(El::Exception);

        static void load_group_counter_placements(
          CounterPlacementSeq& placements,
          ::NewsGate::Moderation::Ad::GroupId id,
          ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
          El::MySQL::Connection* connection)
          throw(El::Exception);

        static void load_ad_placements(
          ::NewsGate::Moderation::Ad::Advert& ad,
          ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
          El::MySQL::Connection* connection)
          throw(El::Exception);
        
        static void load_counter_placements(
          ::NewsGate::Moderation::Ad::Counter& counter,
          ::NewsGate::Moderation::Ad::AdvertiserId advertiser_id,
          El::MySQL::Connection* connection)
          throw(El::Exception);
        
        static std::string placement_display_status(DB::Placement& row)
          throw(El::Exception);

        static std::string counter_placement_display_status(
          DB::CounterPlacement& row)
          throw(El::Exception);
      };

      typedef El::RefCount::SmartPtr<AdManagerImpl>
      AdManagerImpl_var;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Moderation
  {
    namespace Ad
    {
    }
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_MODERATOR_ADMANAGER_ADMANAGERIMPL_HPP_
