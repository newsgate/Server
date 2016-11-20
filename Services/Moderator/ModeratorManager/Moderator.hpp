/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/ModeratorManager/Moderator.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MODERATOR_MODERATORMANAGER_MODERATOR_HPP_
#define _NEWSGATE_SERVER_SERVICES_MODERATOR_MODERATORMANAGER_MODERATOR_HPP_

#include <iostream>
#include <memory>

#include <El/Exception.hpp>
#include <El/RefCount/All.hpp>
#include <El/SyncPolicy.hpp>
#include <El/String/LightString.hpp>
#include <El/Guid.hpp>

#include <Services/Moderator/Commons/ModeratorManager.hpp>

namespace NewsGate
{
  namespace Moderation
  {
    struct Moderator :
      public virtual El::RefCount::DefaultImpl<El::Sync::ThreadPolicy>
    {
      typedef unsigned long long Id;

      typedef std::auto_ptr<El::Guid> SessionIdPtr;

      struct GrantedPrivilege
      {
        Privilege privilege;
        Id granted_by;
      };

      typedef std::vector<GrantedPrivilege> GrantedPrivilegeVector;
    
      Id id;
      Id advertiser_id;
      El::String::LightString name;
      El::String::LightString advertiser_name;
      unsigned char password_digest[16];
      El::String::LightString email;
      ACE_Time_Value updated;
      ACE_Time_Value created;
      Id creator;
      Id superior;
      ModeratorStatus status;
      bool show_deleted;
      El::String::LightString comment;
      GrantedPrivilegeVector privileges;
      SessionIdPtr session_id;
      El::String::LightString last_ip;
      
      Moderator(Id id_val,
                const char* name_val,
                const unsigned char* password_digest_val,
                const char* email_val,
                const ACE_Time_Value& updated_val,
                const ACE_Time_Value& created_val,
                Id creator_val,
                Id superior_val,
                ModeratorStatus status_val,
                bool show_deleted_val,
                const char* comment_val) throw(El::Exception);
      
      virtual ~Moderator() throw();

      std::ostream& dump(std::ostream& ostr) const throw(El::Exception);

      bool has_privilege(PrivilegeId privilege_id) const throw();

      Moderator* deep_copy() const throw(El::Exception);

    private:
      void operator=(const Moderator&);
      Moderator(const Moderator&);
    };
    
    typedef El::RefCount::SmartPtr<Moderator> Moderator_var;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Moderation
  {
    inline
    Moderator::Moderator(Id id_val,
                         const char* name_val,
                         const unsigned char* password_digest_val,
                         const char* email_val,
                         const ACE_Time_Value& updated_val,
                         const ACE_Time_Value& created_val,
                         Id creator_val,
                         Id superior_val,
                         ModeratorStatus status_val,
                         bool show_deleted_val,
                         const char* comment_val) throw(El::Exception)
        : id(id_val),
          advertiser_id(0),
          name(name_val),
          email(email_val),
          updated(updated_val),
          created(created_val),
          creator(creator_val),
          superior(superior_val),
          status(status_val),
          show_deleted(show_deleted_val),
          comment(comment_val)
    {
      memcpy(password_digest, password_digest_val, sizeof(password_digest));
      privileges.reserve(1);
    }
      
    inline
    Moderator::~Moderator() throw()
    {
    }

    inline
    Moderator*
    Moderator::deep_copy() const throw(El::Exception)
    {
      Moderator_var moderator = new Moderator(id,
                                              name.c_str(),
                                              password_digest,
                                              email.c_str(),
                                              updated,
                                              created,
                                              creator,
                                              superior,
                                              status,
                                              show_deleted,
                                              comment.c_str());

      moderator->privileges = privileges;
      moderator->advertiser_id = advertiser_id;
      moderator->advertiser_name = advertiser_name;

      if(session_id.get())
      {
        moderator->session_id.reset(new El::Guid(*session_id));
      }

      return moderator.retn();
    }
    
    inline
    std::ostream&
    Moderator::dump(std::ostream& ostr) const throw(El::Exception)
    {
      ostr << id << " " << name << " " << email << " " << superior
           << " " << status << " " << show_deleted;

      return ostr;
    }
    
    inline
    bool
    Moderator::has_privilege(PrivilegeId privilege_id) const throw()
    {
      for(GrantedPrivilegeVector::const_iterator it = privileges.begin();
          it != privileges.end(); it++)
      {
        if(it->privilege.id == privilege_id)
        {
          return true;
        }
      }
            
      return false;
    }
    
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_MODERATOR_MODERATORMANAGER_MODERATOR_HPP_
