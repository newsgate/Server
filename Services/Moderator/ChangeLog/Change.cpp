/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/ChangeLog/Change.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <string>
#include <sstream>

#include <El/Exception.hpp>
#include <El/BinaryStream.hpp>
#include <El/MySQL/DB.hpp>

#include "Change.hpp"

namespace NewsGate
{
  namespace Moderation
  {
    void
    Change::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr << (uint32_t)1 << moderator_id << moderator_name << ip;      
    }
      
    void
    Change::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      uint32_t version = 0;
      bstr >> version;
      
      if(version != 1)
      {
        std::ostringstream ostr;
        ostr << "::NewsGate::Moderation::Change::read: unexpected version "
             << version;
        
        throw Exception(ostr.str());
      }

      bstr >> moderator_id >> moderator_name >> ip;  
    }

    void
    Change::save(El::MySQL::Connection* connection) const throw(El::Exception)
    {
      std::string data;
          
      {
        std::ostringstream ostr;
        
        {
          El::BinaryOutStream bstr(ostr);
          write(bstr);
        } 
            
        data = ostr.str();
        data = connection->escape(data.c_str(), data.length());
      }
        
      std::ostringstream ostr;
      ostr << "insert into ModerationChangeLog set type=" << type()
           << ", subtype=" << subtype()
           << ", ip='" << ip
           << "', moderator=" << moderator_id << ", moderator_name='"
           << connection->escape(moderator_name.c_str()) << "', url='"
           << connection->escape(url().c_str()) << "', summary='"
           << connection->escape(summary().c_str()) << "', details='"
           << connection->escape(details().c_str()) << "'";

      El::MySQL::Result_var result = connection->query(ostr.str().c_str());
    }
  }
}
