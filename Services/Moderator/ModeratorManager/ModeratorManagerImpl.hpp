/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/ModeratorManager/ModeratorManagerImpl.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MODERATOR_MODERATORMANAGER_MODERATORMANAGERIMPL_HPP_
#define _NEWSGATE_SERVER_SERVICES_MODERATOR_MODERATORMANAGER_MODERATORMANAGERIMPL_HPP_

#include <string>

#include <ext/hash_map>
#include <google/sparse_hash_map>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/Service/CompoundService.hpp>
#include <El/Hash/Hash.hpp>
#include <El/String/StringPtr.hpp>
#include <El/Guid.hpp>

#include <Services/Moderator/Commons/ModeratorManager_s.hpp>
#include <Services/Moderator/ModeratorManager/Moderator.hpp>
#include <Services/Moderator/ChangeLog/ModeratorChange.hpp>

namespace NewsGate
{
  namespace Moderation
  {
    class ModeratorManagerImpl :
      public virtual POA_NewsGate::Moderation::ModeratorManager,
      public virtual El::Service::CompoundService<>
    {
    public:
      EL_EXCEPTION(Exception, El::ExceptionBase);
      EL_EXCEPTION(InvalidArgument, Exception);

    public:

      ModeratorManagerImpl(El::Service::Callback* callback)
        throw(InvalidArgument, Exception, El::Exception);

      virtual ~ModeratorManagerImpl() throw();

    protected:
      
      //
      // IDL:NewsGate/Moderation/ModeratorManager/authenticate:1.0
      //
      virtual ::NewsGate::Moderation::ModeratorInfo* authenticate(
        const char* session,
        const char* ip,
        CORBA::Boolean advance_sess_timeout)
        throw(NewsGate::Moderation::InvalidSession,
              NewsGate::Moderation::AccountDisabled,
              NewsGate::Moderation::NotReady,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);
      
      //
      // IDL:NewsGate/Moderation/ModeratorManager/login:1.0
      //
      virtual ::NewsGate::Moderation::SessionId login(const char* username,
                                                      const char* password,
                                                      const char* ip)
        throw(NewsGate::Moderation::InvalidUsername,
              NewsGate::Moderation::AccountDisabled,
              NewsGate::Moderation::NotReady,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);

      //
      // IDL:NewsGate/Moderation/ModeratorManager/logout:1.0
      //
      virtual void logout(const char* session, const char* ip)
        throw(NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);

      //
      // IDL:NewsGate/Moderation/ModeratorManager/create_moderator:1.0
      //
      virtual ::NewsGate::Moderation::ModeratorInfo* create_moderator(
        const char* session,
        const ::NewsGate::Moderation::ModeratorCreationInfo& moderator_info)
        throw(NewsGate::Moderation::InvalidSession,
              NewsGate::Moderation::PermissionDenied,
              NewsGate::Moderation::AccountAlreadyExist,
              NewsGate::Moderation::NotReady,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);

      //
      // IDL:NewsGate/Moderation/ModeratorManager/update_moderator:1.0
      //
      virtual ::NewsGate::Moderation::ModeratorInfo* update_moderator(
        const char* session,
        const ::NewsGate::Moderation::ModeratorUpdateInfo& moderator_info)
        throw(NewsGate::Moderation::InvalidSession,
              NewsGate::Moderation::PermissionDenied,
              NewsGate::Moderation::AccountNotExist,
              NewsGate::Moderation::NewNameOccupied,
              NewsGate::Moderation::NotReady,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);
      
      //
      // IDL:NewsGate/Moderation/ModeratorManager/get_subordinates:1.0
      //
      virtual ::NewsGate::Moderation::ModeratorInfoSeq* get_subordinates(
        const char* session)
        throw(NewsGate::Moderation::InvalidSession,
              NewsGate::Moderation::NotReady,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);
      
      //
      // IDL:NewsGate/Moderation/ModeratorManager/get_moderators:1.0
      //
      virtual ::NewsGate::Moderation::ModeratorInfoSeq*
      get_moderators(const char* session,
                     const ::NewsGate::Moderation::ModeratorIdSeq& ids)
        throw(NewsGate::Moderation::InvalidSession,
              NewsGate::Moderation::NotReady,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);

      //
      // IDL:NewsGate/Moderation/ModeratorManager/get_advertisers:1.0
      //
      virtual ::NewsGate::Moderation::ModeratorInfoSeq* get_advertisers(
        const char* session)
        throw(NewsGate::Moderation::InvalidSession,
              NewsGate::Moderation::NotReady,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);

      //
      // IDL:NewsGate/Moderation/ModeratorManager/set_advertiser:1.0
      //
      virtual ::NewsGate::Moderation::ModeratorInfo* set_advertiser(
        const char* session,
        ::NewsGate::Moderation::ModeratorId id,
        ::NewsGate::Moderation::ModeratorId advertiser_id)
        throw(NewsGate::Moderation::InvalidSession,
              NewsGate::Moderation::PermissionDenied,
              NewsGate::Moderation::NotReady,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);
      
      virtual bool notify(El::Service::Event* event) throw(El::Exception);

    private:
      void load_moderators() throw(El::Exception);
      void timeout_sessions() throw(El::Exception);

      void logout(const char* session,
                  const char* ip,
                  ModeratorLogout::LogoutType type)
        throw(Exception, El::Exception);
      
      Moderator* find(const char* session, bool advance_sess_timeout)
        throw(NewsGate::Moderation::InvalidSession,
              NewsGate::Moderation::NotReady,
              NewsGate::Moderation::ImplementationException);
      
      std::string moderator_name(Moderator::Id id)
        throw(El::Exception);
      
      void set_moderator_info(const Moderator* moderator,
                              ModeratorInfo& moderator_info)
        throw(El::Exception);

      static std::string make_digest(const char* text,
                                     unsigned char* digest)
        throw(El::Exception);

      static void set_customer_status(Moderator::Id id, bool status)
        throw(El::Exception);
      
      static void set_advertiser_status(Moderator::Id id,
                                        bool status,
                                        const char* name)
        throw(El::Exception);
      
    private:

      struct LoadModerators : public El::Service::CompoundServiceMessage
      {
        LoadModerators(ModeratorManagerImpl* state) throw(El::Exception);
      };      

      struct TimeoutSessions : public El::Service::CompoundServiceMessage
      {
        TimeoutSessions(ModeratorManagerImpl* state) throw(El::Exception);
      };      

      typedef __gnu_cxx::hash_map<ModeratorId,
                                  Moderator_var,
                                  El::Hash::Numeric<ModeratorId> >
      ModeratorMap;

      class NameToIdMap :
        public google::sparse_hash_map<El::String::StringConstPtr,
                                       ModeratorId,
                                       El::Hash::StringConstPtr>
      {
      public:
        NameToIdMap() throw(El::Exception);
      };
      
      struct SessionRecord
      {
        ModeratorId moderator;
        ACE_Time_Value timeout;
      };

      class SessionToIdMap :
        public google::sparse_hash_map<El::Guid,
                                       SessionRecord,
                                       El::Hash::Guid>
      {
      public:
        SessionToIdMap() throw(El::Exception);
      };

      typedef std::list<El::Guid> GuidList;
      
      ModeratorMap moderators_;
      NameToIdMap name_map_;
      SessionToIdMap sessions_;
      bool ready_;
      
    };

    typedef El::RefCount::SmartPtr<ModeratorManagerImpl>
    ModeratorManagerImpl_var;

  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Moderation
  {
    //
    // NewsGate::Moderation::ModeratorManagerImpl::LoadModerators
    //
    inline
    ModeratorManagerImpl::LoadModerators::LoadModerators(
      ModeratorManagerImpl* state)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }
    
    //
    // NewsGate::Moderation::ModeratorManagerImpl::TimeoutSessions
    //
    inline
    ModeratorManagerImpl::TimeoutSessions::TimeoutSessions(
      ModeratorManagerImpl* state)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }

    //
    // NewsGate::Moderation::ModeratorManagerImpl::NameToIdMap
    //
    inline
    ModeratorManagerImpl::NameToIdMap::NameToIdMap() throw(El::Exception)
    {
      set_deleted_key(0);
    }
    
    //
    // NewsGate::Moderation::ModeratorManagerImpl::SessionToIdMap
    //
    inline
    ModeratorManagerImpl::SessionToIdMap::SessionToIdMap() throw(El::Exception)
    {
      set_deleted_key(El::Guid::null);
    }
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_MODERATOR_MODERATORMANAGER_MODERATORMANAGERIMPL_HPP_
