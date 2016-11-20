/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/ChangeLog/ModeratorChange.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MODERATOR_CHANGELOG_MODERATORCHANGE_HPP_
#define _NEWSGATE_SERVER_SERVICES_MODERATOR_CHANGELOG_MODERATORCHANGE_HPP_

#include <stdint.h>

#include <string>
//#include <vector>
//#include <set>

#include <El/Exception.hpp>
#include <El/String/Manip.hpp>
#include <El/BinaryStream.hpp>

#include <Services/Moderator/ChangeLog/Change.hpp>
//#include <Services/Moderator/Commons/CategoryManager.hpp>

namespace NewsGate
{
  namespace Moderation
  {
    struct ModeratorLogin : public Change
    {
      enum LoginResult
      {
        LR_SUCCESS,
        LR_NO_USER,
        LR_WRONG_PASS,
        LR_DISABLED
      };

      uint32_t result;
      
      ModeratorLogin(uint64_t mod_id,
                     const char* mod_name,
                     const char* ip,
                     LoginResult res)
        throw(El::Exception);

      ModeratorLogin() throw(El::Exception);

      virtual ~ModeratorLogin() throw() {}

      virtual Type type() const throw() { return CT_MODERATOR_LOGIN; }
      virtual uint32_t subtype() const throw() { return result; }
        
      virtual void write(El::BinaryOutStream& bstr) const
        throw(El::Exception);
        
      virtual void read(El::BinaryInStream& bstr) throw(El::Exception);
      virtual std::string url() const throw(El::Exception);
        
      virtual void summary(std::ostream& ostr) const
        throw(Exception, El::Exception);
        
      virtual void details(std::ostream& ostr) const
        throw(Exception, El::Exception);
    };

    struct ModeratorLogout : public Change
    {
      enum LogoutType
      {
        LT_MANUAL,
        LT_RELOGIN,
        LT_DISABLED,
        LT_TIMEOUT
      };

      uint32_t logout_type;
      
      ModeratorLogout(uint64_t mod_id,
                      const char* mod_name,
                      const char* ip,
                      LogoutType tp)
        throw(El::Exception);

      ModeratorLogout() throw(El::Exception);

      virtual ~ModeratorLogout() throw() {}

      virtual Type type() const throw() { return CT_MODERATOR_LOGOUT; }
      virtual uint32_t subtype() const throw() { return logout_type; }
        
      virtual void write(El::BinaryOutStream& bstr) const
        throw(El::Exception);
        
      virtual void read(El::BinaryInStream& bstr) throw(El::Exception);
      virtual std::string url() const throw(El::Exception);
        
      virtual void summary(std::ostream& ostr) const
        throw(Exception, El::Exception);
        
      virtual void details(std::ostream& ostr) const
        throw(Exception, El::Exception);
    };
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Moderation
  {
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_MODERATOR_CHANGELOG_MODERATORCHANGE_HPP_
