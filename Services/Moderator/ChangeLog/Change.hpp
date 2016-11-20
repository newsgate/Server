/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/ChangeLog/Change.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MODERATOR_CHANGELOG_CHANGE_HPP_
#define _NEWSGATE_SERVER_SERVICES_MODERATOR_CHANGELOG_CHANGE_HPP_

#include <stdint.h>
#include <string>
#include <iostream>
#include <sstream>

#include <El/Exception.hpp>
#include <El/String/Manip.hpp>
#include <El/BinaryStream.hpp>
#include <El/MySQL/DB.hpp>

namespace NewsGate
{
  namespace Moderation
  {
    struct Change
    {
      uint64_t moderator_id;
      std::string moderator_name;
      std::string ip;

      EL_EXCEPTION(Exception, El::ExceptionBase);

      Change(uint64_t mod_id,
             const char* mod_name,
             const char* ip_address)
        throw(El::Exception);

      Change() throw(El::Exception);
      virtual ~Change() throw() {}

      enum Type
      {
        CT_MODERATOR_LOGIN,
        CT_MODERATOR_LOGOUT,
        CT_CATEGORY_CHANGE
      };

      virtual Type type() const throw() = 0;
      virtual uint32_t subtype() const throw() = 0;
      virtual std::string url() const throw(El::Exception) = 0;
      
      virtual void summary(std::ostream& ostr) const
        throw(Exception, El::Exception) = 0;
      
      virtual void details(std::ostream& ostr) const
        throw(Exception, El::Exception) = 0;

      virtual void write(El::BinaryOutStream& bstr) const
        throw(El::Exception);
      
      virtual void read(El::BinaryInStream& bstr) throw(El::Exception);

      std::string summary() const throw(El::Exception);
      std::string details() const throw(El::Exception);
      void save(El::MySQL::Connection* connection) const throw(El::Exception);
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
    //
    // Change struct
    //
    inline
    Change::Change(uint64_t mod_id,
                   const char* mod_name,
                   const char* ip_address)
      throw(El::Exception)
        : moderator_id(mod_id),
          moderator_name(mod_name),
          ip(ip_address)
    {
    }

    inline
    Change::Change() throw(El::Exception) : moderator_id(0)
    {
    }

    inline
    std::string
    Change::summary() const throw(El::Exception)
    {
      std::ostringstream ostr;
      summary(ostr);
      return ostr.str();
    }

    inline
    std::string
    Change::details() const throw(El::Exception)
    {
      std::ostringstream ostr;
      details(ostr);
      return ostr.str();
    }
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_MODERATOR_CHANGELOG_CHANGE_HPP_
