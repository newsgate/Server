/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/ModeratorManager/ModeratorManagerImpl.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <string>
#include <sstream>

#include <md5.h>
#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/Guid.hpp>
#include <El/String/Manip.hpp>

#include <Services/Moderator/ChangeLog/ModeratorChange.hpp>

#include "ModeratorManagerImpl.hpp"
#include "ModeratorManagerMain.hpp"
#include "ModeratorRecord.hpp"

namespace Aspect
{
  const char STATE[] = "State";
}

namespace NewsGate
{
  namespace Moderation
  {
    //
    // ModeratorManagerImpl class
    //
    ModeratorManagerImpl::ModeratorManagerImpl(El::Service::Callback* callback)
      throw(InvalidArgument, Exception, El::Exception)
        : El::Service::CompoundService<>(callback, "ModeratorManagerImpl"),
          ready_(false)
    {
      El::Service::CompoundServiceMessage_var task = new LoadModerators(this);
      deliver_now(task.in());
    }

    ModeratorManagerImpl::~ModeratorManagerImpl() throw()
    {
      // Check if state is active, then deactivate and log error
    }

    Moderator*
    ModeratorManagerImpl::find(const char* session, bool advance_sess_timeout)
      throw(NewsGate::Moderation::InvalidSession,
            NewsGate::Moderation::NotReady,
            NewsGate::Moderation::ImplementationException)
    {
      if(moderators_.empty())
      {
        NewsGate::Moderation::NotReady ex;

        ex.reason = "NewsGate::Moderation::ModeratorManagerImpl::"
          "find: moderators not loaded";
        
        throw ex;
      }
      
      El::Guid guid;

      try
      {
        guid = session;
      }
      catch(const El::Guid::InvalidArg& e)
      {
        throw NewsGate::Moderation::InvalidSession();
      }
      
      SessionToIdMap::iterator it = sessions_.find(guid);

      if(it == sessions_.end())
      {
        throw NewsGate::Moderation::InvalidSession();
      }

      if(advance_sess_timeout)
      {
        it->second.timeout = ACE_OS::gettimeofday() +
          ACE_Time_Value(Application::instance()->config().session_timeout());
      }
      
      ModeratorMap::iterator mit = moderators_.find(it->second.moderator);

      if(mit == moderators_.end())
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Moderation::ModeratorManagerImpl::"
          "find: moderator with id " << it->second.moderator << " not found";

        NewsGate::Moderation::ImplementationException ex;
        ex.description = ostr.str().c_str();
        throw ex;
      }

      return El::RefCount::add_ref(mit->second.in());
    }

    ::NewsGate::Moderation::SessionId
    ModeratorManagerImpl::login(const char* username,
                                const char* password,
                                const char* ip)
      throw(NewsGate::Moderation::InvalidUsername,
            NewsGate::Moderation::AccountDisabled,
            NewsGate::Moderation::NotReady,
            NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      try
      {
        El::MySQL::Connection_var connection =
          Application::instance()->client_management_dbase()->connect();
        
        WriteGuard guard(srv_lock_);

        if(moderators_.empty())
        {
          NewsGate::Moderation::NotReady ex;

          ex.reason = "NewsGate::Moderation::ModeratorManagerImpl::"
            "login: moderators not loaded";

          throw ex;
        }

        NameToIdMap::const_iterator it = name_map_.find(username);

        if(it == name_map_.end())
        {
          ModeratorLogin
            mod_login(0, username, ip, ModeratorLogin::LR_NO_USER);
          
          mod_login.save(connection);
          
          throw NewsGate::Moderation::InvalidUsername();
        }

        ModeratorMap::iterator mit = moderators_.find(it->second);

        if(mit == moderators_.end())
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::ModeratorManagerImpl::"
            "login: moderator with name " << username << ", id "
               << it->second << " not found";

          NewsGate::Moderation::ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }

        Moderator_var moderator = mit->second;
        moderator->last_ip = ip;
        
        unsigned char password_digest[16];
        make_digest(password, password_digest);

        if(memcmp(moderator->password_digest,
                  password_digest,
                  sizeof(password_digest)) != 0)
        {
          ModeratorLogin mod_login(moderator->id,
                                   username,
                                   ip,
                                   ModeratorLogin::LR_WRONG_PASS);
          
          mod_login.save(connection);

          throw NewsGate::Moderation::InvalidUsername();
        }

        if(moderator->status == MS_DISABLED)
        {
          ModeratorLogin mod_login(moderator->id,
                                   username,
                                   ip,
                                   ModeratorLogin::LR_DISABLED);
          
          mod_login.save(connection);

          throw NewsGate::Moderation::AccountDisabled();
        }

        if(moderator->session_id.get() != 0)
        {
          sessions_.erase(*moderator->session_id);

          ModeratorLogout mod_login(moderator->id,
                                    username,
                                    ip,
                                    ModeratorLogout::LT_RELOGIN);
          
          mod_login.save(connection);          
        }
        else
        {
          moderator->session_id.reset(new El::Guid());
        }

        moderator->session_id->generate();

        SessionRecord sess_rec;
        
        sess_rec.moderator = moderator->id;
        
        sess_rec.timeout = ACE_OS::gettimeofday() +
          ACE_Time_Value(Application::instance()->config().session_timeout());
        
        sessions_[*moderator->session_id] = sess_rec;

        CORBA::String_var res = CORBA::string_dup(
          moderator->session_id->string(El::Guid::GF_DENSE).c_str());

        ModeratorLogin mod_login(moderator->id,
                                 username,
                                 ip,
                                 ModeratorLogin::LR_SUCCESS);
        
        mod_login.save(connection);
        
        return res._retn();
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Moderation::ModeratorManagerImpl::login: "
          "El::Exception caught. Description: " << e;

        NewsGate::Moderation::ImplementationException ex;
        ex.description = ostr.str().c_str();
        throw ex;
      }
    }
      
    void
    ModeratorManagerImpl::logout(const char* session, const char* ip)
      throw(NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      try
      {
        logout(session, ip, ModeratorLogout::LT_MANUAL);
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Moderation::ModeratorManagerImpl::login: "
          "El::Exception caught. Description: " << e;

        NewsGate::Moderation::ImplementationException ex;
        ex.description = ostr.str().c_str();
        throw ex;
      }
    }

    void
    ModeratorManagerImpl::logout(const char* session,
                                 const char* ip,
                                 ModeratorLogout::LogoutType type)
      throw(Exception, El::Exception)
    {
      El::MySQL::Connection_var connection =
        Application::instance()->client_management_dbase()->connect();
        
      WriteGuard guard(srv_lock_);

      El::Guid guid;

      try
      {
        guid = session;
      }
      catch(const El::Guid::InvalidArg& e)
      {
        return;
      }
      
      SessionToIdMap::iterator it = sessions_.find(guid);

      if(it == sessions_.end())
      {
        return;
      }

      ModeratorMap::iterator mit = moderators_.find(it->second.moderator);

      Moderator_var moderator;
            
      if(mit != moderators_.end())
      {
        moderator = mit->second;
        moderator->last_ip = ip;
          
        moderator->session_id.reset(0);
      }

      sessions_.erase(it);

      if(moderator.in())
      {
        ModeratorLogout mod_login(moderator->id,
                                  moderator->name.c_str(),
                                  ip,
                                  type);
          
        mod_login.save(connection);
      }
    }
    
    bool
    ModeratorManagerImpl::notify(El::Service::Event* event)
      throw(El::Exception)
    {
      if(El::Service::CompoundService<>::notify(event))
      {
        return true;
      }

      if(dynamic_cast<LoadModerators*>(event) != 0)
      {
        load_moderators();
        return true;
      }

      if(dynamic_cast<TimeoutSessions*>(event) != 0)
      {
        timeout_sessions();
        return true;
      }
      
      return true;
    }

    void
    ModeratorManagerImpl::timeout_sessions() throw(El::Exception)
    {
      GuidList expired_sessions;
      
      ACE_Time_Value now = ACE_OS::gettimeofday();
      
      {  
        ReadGuard guard(srv_lock_);
        
        for(SessionToIdMap::const_iterator it = sessions_.begin();
            it != sessions_.end(); ++it)
        {
          if(it->second.timeout <= now)
          {
            expired_sessions.push_back(it->first);
          }
        }
      }

      if(!expired_sessions.empty())
      {
        El::MySQL::Connection_var connection =
          Application::instance()->client_management_dbase()->connect();
        
        WriteGuard  guard(srv_lock_);
        
        for(GuidList::const_iterator it = expired_sessions.begin();
            it != expired_sessions.end(); it++)
        {
          SessionToIdMap::iterator sit = sessions_.find(*it);

          if(sit != sessions_.end() && sit->second.timeout <= now)
          {
            ModeratorMap::iterator mit =
              moderators_.find(sit->second.moderator);

            Moderator_var moderator;
            
            if(mit != moderators_.end())
            {
              moderator = mit->second;
              moderator->session_id.reset(0);
            }
            
            sessions_.erase(*it);

            if(moderator.in())
            {
              ModeratorLogout mod_login(moderator->id,
                                        moderator->name.c_str(),
                                        moderator->last_ip.c_str(),
                                        ModeratorLogout::LT_TIMEOUT);
          
              mod_login.save(connection);
            }
          }
        }
      }
      
      El::Service::CompoundServiceMessage_var task = new TimeoutSessions(this);
      
      deliver_at_time(task.in(),
                      ACE_OS::gettimeofday() +
                      ACE_Time_Value(Application::instance()->config().
                                     timeout_sessions_period()));      
    }
    
    void
    ModeratorManagerImpl::load_moderators() throw(El::Exception)
    {
      std::auto_ptr<std::ostringstream> users_ostr;

      if(Application::will_trace(El::Logging::HIGH))
      {
        users_ostr.reset(new std::ostringstream());
        *users_ostr << "NewsGate::Moderation::ModeratorManagerImpl::"
          "load_moderators:\n";
      }
            
      try
      {
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();
      
        El::MySQL::Result_var result =
          connection->query(
            "select id, name, password_digest, email, updated, "
            "created, creator, superior, status, show_deleted, comment "
            "from Moderator where status != 'L'",
            0,
            false);        
        
        {
          ModeratorRecord record(result.in());
          
          WriteGuard guard(srv_lock_);  
          
          while(record.fetch_row())
          {
            unsigned char digest[16];
            El::String::Manip::base64_decode(record.password_digest().c_str(),
                                             digest,
                                             sizeof(digest));
              
            Moderator_var moderator =
              new Moderator(record.id(),
                            record.name().c_str(),
                            digest,
                            record.email().c_str(),
                            record.created().moment(),
                            record.updated().moment(),
                            record.creator(),
                            record.superior(),
                            strcmp(record.status().c_str(), "E") == 0 ?
                            MS_ENABLED : MS_DISABLED,
                            strcmp(record.show_deleted().c_str(), "Y") == 0,
                            record.comment().is_null() ?
                            "" : record.comment().c_str());
            
            if(users_ostr.get() != 0)
            {
              moderator->dump(*users_ostr) << std::endl;
            }

            moderators_[moderator->id] = moderator;
            name_map_[moderator->name.c_str()] = moderator->id;
          }
        }

        result = connection->query(
          "select moderator, privilege, granted_by, args from "
          "ModeratorPrivileges",
          0,
          false);
  
        {
          ModeratorPrivilegeRecord record(result.in());
          
          WriteGuard guard(srv_lock_);  
          
          while(record.fetch_row())
          {
            ModeratorMap::iterator it = moderators_.find(record.moderator());

            if(it == moderators_.end())
            {
              std::ostringstream ostr;
              ostr << "NewsGate::ModeratorManagerImpl::load_moderators: "
                "privileges specified for non-existent moderator "
                   << record.moderator();
        
              El::Service::Error error(ostr.str(), this);
              callback_->notify(&error);

              continue;
            }

            Moderator* moderator = it->second.in();

            Moderator::GrantedPrivilege privilege;
            privilege.granted_by = record.granted_by();
            
            privilege.privilege.id =
              (Moderation::PrivilegeId)record.privilege().value();

            privilege.privilege.args =
              record.args().is_null() ? "" : record.args().value();

            moderator->privileges.push_back(privilege);

            if(privilege.privilege.id == PV_ADVERTISER)
            {
              moderator->advertiser_id = moderator->id;
              moderator->advertiser_name = moderator->name;
            }
          }
        }

        El::Service::CompoundServiceMessage_var task =
          new TimeoutSessions(this);
        
        deliver_at_time(task.in(),
                        ACE_OS::gettimeofday() +
                        ACE_Time_Value(Application::instance()->config().
                                       timeout_sessions_period()));
        
        WriteGuard guard(srv_lock_);  
        ready_ = true;
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::ModeratorManagerImpl::load_moderators: "
          "El::Exception caught. Description:" << std::endl << e;
        
        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);

        El::Service::CompoundServiceMessage_var task =
          new LoadModerators(this);
        
        deliver_at_time(task.in(),
                        ACE_OS::gettimeofday() + ACE_Time_Value(10));
      }

      if(users_ostr.get() != 0)
      {
        Application::logger()->trace(users_ostr->str(),
                                     Aspect::STATE,
                                     El::Logging::HIGH);
      }      
    }

    ::NewsGate::Moderation::ModeratorInfo*
    ModeratorManagerImpl::create_moderator(
      const char* session,
      const ::NewsGate::Moderation::ModeratorCreationInfo& moderator_info)
      throw(NewsGate::Moderation::InvalidSession,
            NewsGate::Moderation::PermissionDenied,
            NewsGate::Moderation::AccountAlreadyExist,
            NewsGate::Moderation::NotReady,
            NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      ::NewsGate::Moderation::ModeratorInfo_var result_info =
          new ::NewsGate::Moderation::ModeratorInfo();
      
      try
      {
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();
        
        WriteGuard guard(srv_lock_);

        if(moderators_.empty())
        {
          NewsGate::Moderation::NotReady ex;

          ex.reason = "NewsGate::Moderation::ModeratorManagerImpl::"
            "create_moderator: moderators not loaded";

          throw ex;
        }

        Moderator_var moderator = find(session, true);

        if(!moderator->has_privilege(PV_ACCOUNT_MANAGER))
        {
          throw NewsGate::Moderation::PermissionDenied();
        }
        
        NameToIdMap::const_iterator it =
          name_map_.find(moderator_info.name.in());

        if(it != name_map_.end())
        {
          throw NewsGate::Moderation::AccountAlreadyExist();
        }

        ModeratorStatus status = moderator_info.status;

        unsigned char password_digest[16];
        
        std::string encoded_password_digest =
          make_digest(moderator_info.password.in(), password_digest);
        
        Moderator_var new_moderator =
          new Moderator(0,
                        moderator_info.name.in(),
                        password_digest,
                        moderator_info.email.in(),
                        ACE_Time_Value::zero,
                        ACE_Time_Value::zero,
                        moderator->id,
                        moderator->id,
                        status,
                        moderator_info.show_deleted,
                        "");

        const PrivilegeSeq& privileges = moderator_info.privileges;

        for(size_t i = 0; i < privileges.length(); i++)
        {
          Moderator::GrantedPrivilege priv;

          priv.privilege = privileges[i];

          if(!moderator->has_privilege(priv.privilege.id))
          {
            // Moderator can set for descendants only the priviliges he has
            throw NewsGate::Moderation::PermissionDenied();
          }
          
          if(!new_moderator->has_privilege(priv.privilege.id))
          {
            priv.granted_by = moderator->id;
            new_moderator->privileges.push_back(priv);
          } 
        }
        
        El::MySQL::Result_var result;

        std::string escaped_name =
          connection->escape(new_moderator->name.c_str());

        std::string escaped_password_digest =
          connection->escape(encoded_password_digest.c_str());

        {
          std::ostringstream ostr;
          ostr << "insert ignore into Moderator (name, "
            "password_digest, email, created, creator, superior, status, "
            "show_deleted) values ('" << escaped_name << "', '"
               << escaped_password_digest << "', '"
               << connection->escape(new_moderator->email.c_str())
               << "', NOW(), " << moderator->id << ", " << moderator->id
               << ", '" << (status == MS_ENABLED ? "E" : "D")
               << "', '" << (moderator_info.show_deleted ? "Y" : "N")
               << "')";

          result = connection->query(ostr.str().c_str());
        }
        
        {
          std::ostringstream ostr;
          ostr << "select id, name, password_digest, email, "
            "updated, created, creator, superior, status, show_deleted, "
            "comment from Moderator where name='" << escaped_name << "'";

          result = connection->query(ostr.str().c_str());
        }

        Moderator::Id id = 0;
        
        {
          ModeratorRecord record(result.in());
          
          if(!record.fetch_row())
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Moderation::ModeratorManagerImpl::"
              "create_moderator: can't find newly inserted user '"
                 << escaped_name << "' in the DB";
            
            throw Exception(ostr.str());
          }
          
          id = record.id();
          new_moderator->created = record.created().moment();
          new_moderator->updated = record.updated().moment();
        }
        
        new_moderator->id = id;

        if(new_moderator->has_privilege(PV_ADVERTISER))
        {
          new_moderator->advertiser_id = id;
          new_moderator->advertiser_name = new_moderator->name;
        }
        
        const Moderator::GrantedPrivilegeVector new_mode_privileges =
          new_moderator->privileges;

        bool customer_status = false;
        bool advertiser_status = false;
        
        if(!new_mode_privileges.empty())
        {
          std::ostringstream ostr;
          ostr << "insert ignore into ModeratorPrivileges "
            "(moderator, privilege, granted_by, args) values";

          for(Moderator::GrantedPrivilegeVector::const_iterator it =
                new_mode_privileges.begin(); it != new_mode_privileges.end();
              it++)
          {
            if(it != new_mode_privileges.begin())
            {
              ostr << ",";
            }
          
            ostr << " (" << id << ", " << it->privilege.id << ", "
                 << it->granted_by << ", '"
                 << connection->escape(it->privilege.args) << "')";

            switch(it->privilege.id)
            {
            case PV_CUSTOMER:
              {
                customer_status = true;
                break;
              }
            case PV_ADVERTISER:
              {
                advertiser_status = true;
                break;
              }
            default: break;
            }
          }
          
          result = connection->query(ostr.str().c_str());
        }

        customer_status &= status == MS_ENABLED;
        advertiser_status &= status == MS_ENABLED;

        set_moderator_info(new_moderator.in(), *result_info);

        if(new_moderator->has_privilege(PV_CUSTOMER))
        {
          set_customer_status(id, customer_status);
        }
        
        if(new_moderator->has_privilege(PV_ADVERTISER))
        {
          set_advertiser_status(id,
                                advertiser_status,
                                new_moderator->name.c_str());
        }
        
        moderators_[id] = new_moderator;
        name_map_[new_moderator->name.c_str()] = id;
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Moderation::ModeratorManagerImpl::"
          "create_moderator: El::Exception caught. Description: " << e;

        NewsGate::Moderation::ImplementationException ex;
        ex.description = ostr.str().c_str();
        throw ex;
      }
    
      return result_info._retn();
    }

    void
    ModeratorManagerImpl::set_customer_status(Moderator::Id id, bool status)
      throw(El::Exception)
    {
      El::MySQL::Connection_var connection =
        Application::instance()->client_management_dbase()->connect();

      char st = status ? 'E' : 'D';

      std::ostringstream ostr;
      ostr << "insert into Customer "
        "(id, status) values (" << id << ", '" << st
           << "') on duplicate key update status='" << st << "'";

      El::MySQL::Result_var result = connection->query(ostr.str().c_str());
    }
    
    void
    ModeratorManagerImpl::set_advertiser_status(Moderator::Id id,
                                                bool status,
                                                const char* name)
      throw(El::Exception)
    {
      El::MySQL::Connection_var connection =
        Application::instance()->client_management_dbase()->connect();

      std::string escaped_name = connection->escape(name);
      
      char st = status ? 'E' : 'D';

      std::ostringstream ostr;
      ostr << "insert into Advertiser (id, name, status) values (" << id
           << ", '" << escaped_name << "', '" << st
           << "') on duplicate key update name='" << escaped_name
           << "', status='" << st << "'";

      El::MySQL::Result_var result = connection->query(ostr.str().c_str());

      result = connection->query(
        "update AdSelector set update_num=update_num+1");
    }
    
    ::NewsGate::Moderation::ModeratorInfo*
    ModeratorManagerImpl::set_advertiser(
      const char* session,
      ::NewsGate::Moderation::ModeratorId id,
      ::NewsGate::Moderation::ModeratorId advertiser_id)
      throw(NewsGate::Moderation::InvalidSession,
            NewsGate::Moderation::PermissionDenied,
            NewsGate::Moderation::NotReady,
            NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      ::NewsGate::Moderation::ModeratorInfo_var result_info =
          new ::NewsGate::Moderation::ModeratorInfo();

      try
      {
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();
        
        WriteGuard guard(srv_lock_);

        // Checking if moderators are loaded
        
        if(moderators_.empty())
        {
          NewsGate::Moderation::NotReady ex;

          ex.reason = "NewsGate::Moderation::ModeratorManagerImpl::"
            "set_advertiser: moderators not loaded";

          throw ex;
        }

        Moderator_var moderator = find(session, true);

        if(id != moderator->id || !moderator->has_privilege(PV_AD_MANAGER))
        {
          throw NewsGate::Moderation::PermissionDenied();
        }

        if(advertiser_id)
        {
          ModeratorMap::iterator mit = moderators_.find(advertiser_id);

          if(mit == moderators_.end() ||
             !mit->second->has_privilege(PV_ADVERTISER))
          {
            throw NewsGate::Moderation::PermissionDenied();
          }
          
          moderator->advertiser_id = advertiser_id;
          moderator->advertiser_name = mit->second->name;
        }
        else if(moderator->has_privilege(PV_ADVERTISER))
        {
          moderator->advertiser_id = moderator->id;
          moderator->advertiser_name = moderator->name;
        }
        else
        {
          moderator->advertiser_id = 0;
          moderator->advertiser_name.clear();
        }
        
        set_moderator_info(moderator.in(), *result_info);
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Moderation::ModeratorManagerImpl::"
          "set_advertiser: El::Exception caught. Description: " << e;

        NewsGate::Moderation::ImplementationException ex;
        ex.description = ostr.str().c_str();
        throw ex;
      }
    
      return result_info._retn();
    }
        
    ::NewsGate::Moderation::ModeratorInfo*
    ModeratorManagerImpl::update_moderator(
      const char* session,
      const ::NewsGate::Moderation::ModeratorUpdateInfo& moderator_info)
      throw(NewsGate::Moderation::InvalidSession,
            NewsGate::Moderation::PermissionDenied,
            NewsGate::Moderation::AccountNotExist,
            NewsGate::Moderation::NewNameOccupied,
            NewsGate::Moderation::NotReady,
            NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      ::NewsGate::Moderation::ModeratorInfo_var result_info =
          new ::NewsGate::Moderation::ModeratorInfo();
      
      try
      {
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();
        
        WriteGuard guard(srv_lock_);

        // Checking if moderators are loaded
        
        if(moderators_.empty())
        {
          NewsGate::Moderation::NotReady ex;

          ex.reason = "NewsGate::Moderation::ModeratorManagerImpl::"
            "update_moderator: moderators not loaded";

          throw ex;
        }

        Moderator_var moderator = find(session, true);

        bool self_update = moderator_info.id == moderator->id;

        // Checking if moderator has privilege to update other moderators
        
        if(!self_update && !moderator->has_privilege(PV_ACCOUNT_MANAGER))
        {
          throw NewsGate::Moderation::PermissionDenied();
        }

        // Searching for moderator to be updated

        ModeratorMap::const_iterator mit = moderators_.find(moderator_info.id);

        if(mit == moderators_.end())
        {
          throw NewsGate::Moderation::AccountNotExist();
        }

        Moderator_var updated_moderator = mit->second->deep_copy();
        std::string current_name = updated_moderator->name.c_str();
        
        if(strcmp(current_name.c_str(), moderator_info.name.in()))
        {
          NameToIdMap::const_iterator it =
            name_map_.find(moderator_info.name.in());

          if(it != name_map_.end())
          {
            throw NewsGate::Moderation::NewNameOccupied();
          }
        }

        // Setting options new values

        updated_moderator->status = moderator_info.status;
        updated_moderator->show_deleted = moderator_info.show_deleted;

        if(*moderator_info.name.in() != '\0')
        {
          updated_moderator->name = moderator_info.name.in();
        }

        if(*moderator_info.password.in() != '\0')
        {
          make_digest(moderator_info.password.in(),
                      updated_moderator->password_digest);
        }
        
        if(*moderator_info.email.in() != '\0')
        {
          updated_moderator->email = moderator_info.email.in();
        }

        if(!self_update)
        {
          // Adding new priveleges
          
          const PrivilegeSeq& privileges = moderator_info.privileges;

          Moderator::GrantedPrivilegeVector& updated_moderator_privileges =
            updated_moderator->privileges;

          updated_moderator_privileges.clear();

          for(size_t i = 0; i < privileges.length(); i++)
          {
            Moderator::GrantedPrivilege priv;

            priv.privilege = privileges[i];

            if(!moderator->has_privilege(priv.privilege.id))
            {
              // Moderator can set for descendants only the priviliges he has
              throw NewsGate::Moderation::PermissionDenied();
            }

            priv.granted_by = moderator->id;
            updated_moderator_privileges.push_back(priv);
/*
            if(priv.privilege.id == PV_ADVERTISER)
            {
              priv.privilege.id = PV_CUSTOMER;
              updated_moderator_privileges.push_back(priv);
            }
*/
          }
        }
        
        El::MySQL::Result_var result;

        std::string escaped_name =
          connection->escape(updated_moderator->name.c_str());

        std::string encoded_password_digest;
        
        El::String::Manip::base64_encode(
          updated_moderator->password_digest,
          sizeof(updated_moderator->password_digest),
          encoded_password_digest);
        
        {
          std::ostringstream ostr;
          ostr << "update Moderator set name='"
               << escaped_name << "', password_digest='"
               << connection->escape(encoded_password_digest.c_str())
               << "', email='"
               << connection->escape(updated_moderator->email.c_str())
               << "', status='" << (updated_moderator->status ==
                                    MS_ENABLED ? "E" : "D")
               << "', show_deleted='"
               << (updated_moderator->show_deleted ? "Y" : "N")
               << "' where id=" << updated_moderator->id;

          result = connection->query(ostr.str().c_str());
        }
        
        {
          std::ostringstream ostr;
          ostr << "select id, name, password_digest, email, "
            "updated, created, creator, superior, status, show_deleted, "
            "comment from Moderator where id=" << updated_moderator->id;

          result = connection->query(ostr.str().c_str());
        }

        {
          ModeratorRecord record(result.in());
          
          if(!record.fetch_row())
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Moderation::ModeratorManagerImpl::"
              "create_moderator: can't find just updated user '"
                 << escaped_name << "' in the DB";
            
            throw Exception(ostr.str());
          }
          
          updated_moderator->updated = record.updated().moment();
        }

        bool customer_status = false;
        bool advertiser_status = false;
        
        if(!self_update)
        {
          {
            std::ostringstream ostr;
            ostr << "delete from ModeratorPrivileges where moderator="
                 << updated_moderator->id;
            
            result = connection->query(ostr.str().c_str());          
          }
        
          Moderator::GrantedPrivilegeVector& updated_moderator_privileges =
            updated_moderator->privileges;
          
          if(!updated_moderator_privileges.empty())
          {
            std::ostringstream ostr;
            
            ostr << "insert ignore into ModeratorPrivileges "
              "(moderator, privilege, granted_by, args) values";

            for(Moderator::GrantedPrivilegeVector::const_iterator it =
                  updated_moderator_privileges.begin();
                it != updated_moderator_privileges.end(); it++)
            {
              if(it != updated_moderator_privileges.begin())
              {
                ostr << ",";
              }
          
              ostr << " (" << updated_moderator->id << ", " << it->privilege.id
                   << ", " << it->granted_by << ", '"
                   << connection->escape(it->privilege.args) << "')";

              switch(it->privilege.id)
              {
              case PV_CUSTOMER:
                {
                  customer_status = true;
                  break;
                }
              case PV_ADVERTISER:
                {
                  advertiser_status = true;
                  break;
                }
              default: break;
              }
            }

            std::string query = ostr.str();
            result = connection->query(query.c_str());
          }

          customer_status &= updated_moderator->status == MS_ENABLED;
          advertiser_status &= updated_moderator->status == MS_ENABLED;
        }
        
        name_map_.erase(current_name.c_str());
        name_map_[updated_moderator->name.c_str()] = updated_moderator->id;
        moderators_[updated_moderator->id] = updated_moderator;

        if(!self_update)
        {
          if(moderator->has_privilege(PV_CUSTOMER))
          {
            set_customer_status(updated_moderator->id, customer_status);
          }
          
          if(moderator->has_privilege(PV_ADVERTISER))
          {
            set_advertiser_status(updated_moderator->id,
                                  advertiser_status,
                                  updated_moderator->name.c_str());
          }

          if(updated_moderator->has_privilege(PV_ADVERTISER))
          {
            updated_moderator->advertiser_id = updated_moderator->id;
            updated_moderator->advertiser_name = updated_moderator->name;
          }
          else
          {
            updated_moderator->advertiser_id = 0;
            
            for(ModeratorMap::iterator i(moderators_.begin()),
                  e(moderators_.end()); i != e; ++i)
            {
              Moderator* m = i->second.in();

              if(m->advertiser_id == updated_moderator->id)
              {
                if(m->has_privilege(PV_ADVERTISER))
                {
                  m->advertiser_id = m->id;
                  m->advertiser_name = m->name;
                }
                else
                {
                  m->advertiser_id = 0;
                  m->advertiser_name.clear();
                }
              }
            }
          }
        }

        set_moderator_info(updated_moderator.in(), *result_info);        
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Moderation::ModeratorManagerImpl::"
          "update_moderator: El::Exception caught. Description: " << e;

        NewsGate::Moderation::ImplementationException ex;
        ex.description = ostr.str().c_str();
        throw ex;
      }
    
      return result_info._retn();
    }

    void
    ModeratorManagerImpl::set_moderator_info(const Moderator* moderator,
                                             ModeratorInfo& moderator_info)
      throw(El::Exception)
    {
      moderator_info.id = moderator->id;
      moderator_info.advertiser_id = moderator->advertiser_id;
      moderator_info.advertiser_name = moderator->advertiser_name.c_str();
      moderator_info.name = moderator->name.c_str();
      moderator_info.email = moderator->email.c_str();
      
      moderator_info.updated =
        El::Moment(moderator->updated).rfc0822().c_str();
        
      moderator_info.created =
        El::Moment(moderator->created).rfc0822().c_str();
        
      moderator_info.status = moderator->status;
      moderator_info.show_deleted = moderator->show_deleted;

      moderator_info.creator = moderator_name(moderator->creator).c_str();
      moderator_info.superior = moderator_name(moderator->superior).c_str();

      moderator_info.comment = moderator->comment.c_str();
          
      size_t len = moderator->privileges.size();
      moderator_info.privileges.length(len);

      for(size_t i = 0; i < len; i++)
      {
        NewsGate::Moderation::Privilege priv =
          moderator->privileges[i].privilege;
        
        std::string priv_name;
        
        switch(priv.id)
        {
        case PV_FEED_CREATOR:
          {
            priv_name = "FeedCreator";
            break;
          }
        case PV_FEED_MANAGER:
          {
            priv_name = "FeedManager";
            break;
          }
        case PV_ACCOUNT_MANAGER:
          {
            priv_name = "AccountManager";
            break;
          }
        case PV_CATEGORY_MANAGER:
          {
            priv_name = "CategoryManager";
            break;
          }
        case PV_CLIENT_MANAGER:
          {
            priv_name = "ClientManager";
            break;
          }
        case PV_CUSTOMER_MANAGER:
          {
            priv_name = "CustomerManager";
            break;
          }
        case PV_CUSTOMER:
          {
            priv_name = "Customer";
            break;
          }
        case PV_CATEGORY_EDITOR:
          {
            priv_name = "CategoryEditor";
            break;
          }
        case PV_AD_MANAGER:
          {
            priv_name = "AdManager";
            break;
          }
        case PV_ADVERTISER:
          {
            priv_name = "Advertiser";
            break;
          }
        default:
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Moderation::ModeratorManagerImpl::"
              "set_moderator_info: unexpected privilege " << priv.id;

            throw Exception(ostr.str());
          }
        }
        
        moderator_info.privileges[i].priv = priv;
        moderator_info.privileges[i].name = priv_name.c_str();

        moderator_info.privileges[i].granted_by =
          moderator_name(moderator->privileges[i].granted_by).c_str();
      }
      
    } 
    
    ::NewsGate::Moderation::ModeratorInfo*
    ModeratorManagerImpl::authenticate(const char* session,
                                       const char* ip,
                                       CORBA::Boolean advance_sess_timeout)
      throw(NewsGate::Moderation::InvalidSession,
            NewsGate::Moderation::AccountDisabled,
            NewsGate::Moderation::NotReady,
            NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      ::NewsGate::Moderation::ModeratorInfo_var moderator_info =
        new ::NewsGate::Moderation::ModeratorInfo();
        
      try
      {
        WriteGuard guard(srv_lock_);
        
        Moderator_var moderator = find(session, advance_sess_timeout);
        moderator->last_ip = ip;

        if(moderator->status == MS_DISABLED)
        {
          logout(session, ip, ModeratorLogout::LT_DISABLED);
          throw NewsGate::Moderation::AccountDisabled();
        }
        
        set_moderator_info(moderator.in(), *moderator_info);
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Moderation::ModeratorManagerImpl::"
          "authenticate: El::Exception caught. Description: " << e;

        NewsGate::Moderation::ImplementationException ex;
        ex.description = ostr.str().c_str();
        throw ex;
      }

      return moderator_info._retn();
    }

    ::NewsGate::Moderation::ModeratorInfoSeq*
    ModeratorManagerImpl::get_moderators(
      const char* session,
      const ::NewsGate::Moderation::ModeratorIdSeq& ids)
      throw(NewsGate::Moderation::InvalidSession,
            NewsGate::Moderation::NotReady,
            NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      ::NewsGate::Moderation::ModeratorInfoSeq_var moderator_infos =
          new ::NewsGate::Moderation::ModeratorInfoSeq();

      try
      {
        ReadGuard guard(srv_lock_);
      
        Moderator_var moderator = find(session, false);

        for(size_t i = 0; i < ids.length(); i++)
        {
          ModeratorMap::const_iterator it = moderators_.find(ids[i]);

          if(it != moderators_.end())
          {
            unsigned long len = moderator_infos->length();
            moderator_infos->length(len + 1);
            set_moderator_info(it->second.in(), moderator_infos[len]);
          }
        }        
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Moderation::ModeratorManagerImpl::"
          "get_moderators: El::Exception caught. Description: " << e;
        
        NewsGate::Moderation::ImplementationException ex;
        ex.description = ostr.str().c_str();
        throw ex;
      }
      
      return moderator_infos._retn();
    }

    ::NewsGate::Moderation::ModeratorInfoSeq*
    ModeratorManagerImpl::get_advertisers(const char* session)
      throw(NewsGate::Moderation::InvalidSession,
            NewsGate::Moderation::NotReady,
            NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      ::NewsGate::Moderation::ModeratorInfoSeq_var moderator_infos =
          new ::NewsGate::Moderation::ModeratorInfoSeq();

      try
      {
        ReadGuard guard(srv_lock_);
      
        Moderator_var moderator = find(session, false);

        for(ModeratorMap::const_iterator i(moderators_.begin()),
              e(moderators_.end()); i != e; ++i)
        {
          Moderator* m = i->second.in();

          if(m->has_privilege(PV_ADVERTISER))
          {
            unsigned long len = moderator_infos->length();
            moderator_infos->length(len + 1);
            set_moderator_info(m, moderator_infos[len]);
          }
        }
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Moderation::ModeratorManagerImpl::"
          "get_advertisers: El::Exception caught. Description: " << e;
        
        NewsGate::Moderation::ImplementationException ex;
        ex.description = ostr.str().c_str();
        throw ex;
      }
      
      return moderator_infos._retn();
    }

    ::NewsGate::Moderation::ModeratorInfoSeq*
    ModeratorManagerImpl::get_subordinates(const char* session)
      throw(NewsGate::Moderation::InvalidSession,
            NewsGate::Moderation::NotReady,
            NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      ::NewsGate::Moderation::ModeratorInfoSeq_var moderator_infos =
          new ::NewsGate::Moderation::ModeratorInfoSeq();

      try
      {
        ReadGuard guard(srv_lock_);
      
        Moderator_var moderator = find(session, false);
        Moderator::Id moderator_id = moderator->id;

        for(ModeratorMap::const_iterator it = moderators_.begin();
            it != moderators_.end(); it++)
        {
          Moderator::Id id = it->second->superior;

          while(id && id != moderator_id)
          {
            ModeratorMap::const_iterator mit = moderators_.find(id);

            if(mit == moderators_.end())
            {
              std::ostringstream ostr;
              ostr << "NewsGate::Moderation::ModeratorManagerImpl::"
                "get_subordinates: moderator with id " << id
                   << " not found";
          
              El::Service::Error error(ostr.str(), this);
              callback_->notify(&error);
            
              id = 0;
              break;
            }

            id = mit->second->superior;
          }

          if(id == moderator_id)
          {
            size_t len = moderator_infos->length();
            moderator_infos->length(len + 1);
            set_moderator_info(it->second.in(), moderator_infos[len]);
          }
        }
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Moderation::ModeratorManagerImpl::"
          "get_subordinates: El::Exception caught. Description: " << e;
        
        NewsGate::Moderation::ImplementationException ex;
        ex.description = ostr.str().c_str();
        throw ex;
      }

      return moderator_infos._retn();
    }

    std::string
    ModeratorManagerImpl::moderator_name(Moderator::Id id)
      throw(El::Exception)
    {
      std::string name;
      
      if(id == 0)
      {
        name = "God";
      }
      else
      {
        ModeratorMap::iterator mit = moderators_.find(id);

        if(mit == moderators_.end())
        {
          name = "Unknown";
              
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::ModeratorManagerImpl::"
            "moderator_name: moderator with id " << id
               << " not found";
          
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        else
        {
          name = mit->second->name.c_str();
        }    
      }
      
      return name;
    }

    std::string
    ModeratorManagerImpl::make_digest(const char* text, unsigned char* digest)
      throw(El::Exception)
    {
      MD5_CTX c;
      MD5_Init(&c);
      MD5_Update(&c, (unsigned char*)text, strlen(text));
      MD5_Final((unsigned char*)digest, &c);

      std::string encoded;
      El::String::Manip::base64_encode(digest, 16, encoded);

      return encoded;
    }
  }  
}
