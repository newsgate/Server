/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Ad/AdServer/AdServerImpl.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_AD_ADSERVER_ADSERVERIMPL_HPP_
#define _NEWSGATE_SERVER_SERVICES_AD_ADSERVER_ADSERVERIMPL_HPP_

#include <string>
#include <memory>

#include <ext/hash_set>

#include <google/dense_hash_map>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/Hash/Hash.hpp>
#include <El/Service/CompoundService.hpp>
#include <El/Stat.hpp>

#include <Commons/Ad/Ad.hpp>
#include <Services/Commons/Ad/TransportImpl.hpp>
#include <Services/Commons/Ad/AdServices_s.hpp>

#include "DB_Record.hpp"

namespace NewsGate
{
  namespace Ad
  {
    class AdServerImpl :
      public virtual POA_NewsGate::Ad::AdServer,
      public virtual El::Service::CompoundService<> 
    {
    public:
      EL_EXCEPTION(Exception, El::ExceptionBase);

    public:

      AdServerImpl(El::Service::Callback* callback)
        throw(Exception, El::Exception);

      virtual ~AdServerImpl() throw();

      virtual void wait() throw(Exception, El::Exception);

    private:

      //
      // IDL:NewsGate/Ad/AdServer/selector_changed:1.0
      //
      virtual ::CORBA::Boolean selector_changed(
        ::CORBA::ULong interface_version,
        ::CORBA::ULongLong update_number)
        throw(::NewsGate::Ad::IncompatibleVersion,
              NewsGate::Ad::ImplementationException,
              CORBA::SystemException);      
      
      //
      // IDL:NewsGate/Ad/AdServer/get_selector:1.0
      //
      virtual ::NewsGate::Ad::Transport::Selector* get_selector(
        ::CORBA::ULong interface_version)
        throw(::NewsGate::Ad::IncompatibleVersion,
              ::NewsGate::Ad::ImplementationException,
              CORBA::SystemException);

      //
      // IDL:NewsGate/Ad/AdServer/select:1.0
      //
      virtual ::NewsGate::Ad::Transport::SelectionResult* select(
        ::CORBA::ULong interface_version,
        ::NewsGate::Ad::Transport::SelectionContext* ctx)
        throw(::NewsGate::Ad::IncompatibleVersion,
              ::NewsGate::Ad::ImplementationException,
              CORBA::SystemException);
      
      //
      // IDL:NewsGate/Ad/AdServer/selection:1.0
      //
      virtual ::NewsGate::Ad::Transport::Selection* selection(
        ::CORBA::ULong interface_version,
        ::CORBA::ULongLong id)
        throw(::NewsGate::Ad::NotFound,
              ::NewsGate::Ad::IncompatibleVersion,
              ::NewsGate::Ad::ImplementationException,
              CORBA::SystemException);
      
      virtual bool notify(El::Service::Event* event) throw(El::Exception);
      
      struct CheckForUpdate : public El::Service::CompoundServiceMessage
      {
        CheckForUpdate(AdServerImpl* service) throw(El::Exception);
        ~CheckForUpdate() throw() {}
      };

      void check_for_update(CheckForUpdate* cfu) throw(El::Exception);
      void check_db_update() throw(El::Exception);
      bool check_external_update() throw(Exception, El::Exception);

      static void row_to_condition(DB::Condition& row, Condition& condition)
        throw(El::Exception);
      
      static void add_condition(DB::Condition& row,
                                Creative& creative,
                                Selector& selector)
        throw(El::Exception);

      static void add_condition(DB::Condition& row,
                                Counter& counter,
                                Selector& selector)
        throw(El::Exception);

      static void load_page(El::MySQL::Connection* connection,
                            PageId id,
                            Page& page,
                            Selector& selector)
        throw(Exception, El::Exception);

      static El::MySQL::Result* query_placement_group_conditions(
        uint64_t id,
        El::MySQL::Connection* connection,
        const char* table_name)
        throw(El::Exception);        
      
      static El::MySQL::Result* query_placement_campaign_conditions(
        uint64_t id,
        El::MySQL::Connection* connection,
        const char* table_name)
        throw(El::Exception);        
      
      static void text_to_url_array(const char* text,
                                    Ad::StringArray& array)
        throw(El::Exception);

      static void text_to_string_array(const char* text,
                                       Ad::StringArray& array)
        throw(El::Exception);

      typedef __gnu_cxx::hash_set<std::string, El::Hash::String> StringSet;
      
      static void text_to_set(const char* text, StringSet& set)
        throw(El::Exception);
      
      static void text_to_lang_set(const char* text, Ad::LangSet& set)
        throw(El::Exception);

      static void text_to_lang_array(const char* text, Ad::LangArray& array)
        throw(El::Exception);      
      
      static void text_to_country_set(const char* text, Ad::CountrySet& set)
        throw(El::Exception);
      
      static void text_to_ipmask_array(const char* text,
                                       std::vector<El::Net::IpMask>& array)
        throw(El::Exception);

      struct CreativePtrMap:
        public google::dense_hash_map<CreativeId,
                                      const Creative*,
                                      El::Hash::Numeric<CreativeId> >
      {
        CreativePtrMap() throw() { set_empty_key(0); }
      };
      
      static CreativePtrMap* selector_creatives(const Selector& selector)
        throw(El::Exception);

    private:
      
      typedef std::auto_ptr<Selector> SelectorPtr;
      SelectorPtr selector_;

      typedef std::auto_ptr<CreativePtrMap> CreativePtrMapPtr;
      CreativePtrMapPtr creatives_;
      
      El::Stat::TimeMeter meters_[PI_COUNT];
      std::string selector_cache_;
    };

    typedef El::RefCount::SmartPtr<AdServerImpl> AdServerImpl_var;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Ad
  {
    //
    // AdServerImpl::CheckForUpdate class
    //
    inline
    AdServerImpl::CheckForUpdate::CheckForUpdate(AdServerImpl* service)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(service, service, false),
          El::Service::CompoundServiceMessage(service, service)
    {
    }
  }
}

#endif //_NEWSGATE_SERVER_SERVICES_AD_ADSERVER_ADSERVERIMPL_HPP_
