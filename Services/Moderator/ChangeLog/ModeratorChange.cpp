/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/ChangeLog/ModeratorChange.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <string>

#include <El/Exception.hpp>
#include <El/String/Manip.hpp>

#include "ModeratorChange.hpp"

namespace NewsGate
{
  namespace Moderation
  {
    //
    // ModeratorLogin struct
    //
    ModeratorLogin::ModeratorLogin() throw(El::Exception)
        : Change(),
          result(LR_SUCCESS)
    {
    }

    ModeratorLogin::ModeratorLogin(uint64_t mod_id,
                                   const char* mod_name,
                                   const char* ip,
                                   LoginResult res)
      throw(El::Exception)
        : Change(mod_id, mod_name, ip),
          result(res)
    {
    }

    std::string
    ModeratorLogin::url() const throw(El::Exception)
    {
      std::ostringstream ostr;
      ostr << "/psp/account/update?i=" << moderator_id;
      return ostr.str();
    }
      
    void
    ModeratorLogin::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      Change::write(bstr);
      bstr << (uint32_t)1 << result;
    }
      
    void
    ModeratorLogin::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      Change::read(bstr);
        
      uint32_t version = 0;
      bstr >> version;

      if(version != 1)
      {
        std::ostringstream ostr;
        ostr << "::NewsGate::Moderation::ModeratorLogin::read: "
          "unexpected version " << version;
            
        throw Exception(ostr.str());
      }

      bstr >> result;
    }

    void
    ModeratorLogin::summary(std::ostream& ostr) const
      throw(Exception, El::Exception)
    {
      ostr << "Login ";
      
      switch(result)
      {
      case LR_SUCCESS:
        {
          ostr << "successfull";
          break;
        }
      case LR_NO_USER:
        {
          ostr << "failed - wrong user";
          break;
        }
      case LR_WRONG_PASS:
        {
          ostr << "failed - wrong password";
          break;
        }
      case LR_DISABLED:
        {
          ostr << "failed - account disabled";
          break;
        }
      }
    }
    
    void
    ModeratorLogin::details(std::ostream& ostr) const
      throw(Exception, El::Exception)
    {
    }

    //
    // ModeratorLogout struct
    //
    ModeratorLogout::ModeratorLogout() throw(El::Exception)
        : Change(),
          logout_type(LT_MANUAL)
    {
    }

    ModeratorLogout::ModeratorLogout(uint64_t mod_id,
                                     const char* mod_name,
                                     const char* ip,
                                     LogoutType tp)
      throw(El::Exception)
        : Change(mod_id, mod_name, ip),
          logout_type(tp)
    {
    }

    std::string
    ModeratorLogout::url() const throw(El::Exception)
    {
      std::ostringstream ostr;
      ostr << "/psp/account/update?i=" << moderator_id;
      return ostr.str();
    }
      
    void
    ModeratorLogout::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      Change::write(bstr);
      bstr << (uint32_t)1 << logout_type;
    }
      
    void
    ModeratorLogout::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      Change::read(bstr);
        
      uint32_t version = 0;
      bstr >> version;

      if(version != 1)
      {
        std::ostringstream ostr;
        ostr << "::NewsGate::Moderation::ModeratorLogout::read: "
          "unexpected version " << version;
            
        throw Exception(ostr.str());
      }

      bstr >> logout_type;
    }

    void
    ModeratorLogout::summary(std::ostream& ostr) const
      throw(Exception, El::Exception)
    {
      ostr << "Logout ";
      
      switch(logout_type)
      {
      case LT_MANUAL:
        {
          ostr << "manual";
          break;
        }
      case LT_TIMEOUT:
        {
          ostr << "by timeout";
          break;
        }
      }
    }
    
    void
    ModeratorLogout::details(std::ostream& ostr) const
      throw(Exception, El::Exception)
    {
    }      
  }
}
