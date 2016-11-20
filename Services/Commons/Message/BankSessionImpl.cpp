/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Message/Commons/BankSessionImpl.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <El/CORBA/Corba.hpp>

#include <sstream>

#include <El/Exception.hpp>

#include <Services/Commons/Message/MessageServices.hpp>

#include "BankSessionImpl.hpp"

namespace NewsGate
{
  namespace Message
  {
    //
    // BankSessionIdImpl class
    //
    char*
    BankSessionIdImpl::to_string()
    {
      std::ostringstream ostr;
      ostr << index << "\t" << banks_count;
      
      return CORBA::string_dup(ostr.str().c_str());
    }
    
    void
    BankSessionIdImpl::from_string(const char* str)
    {
      uint32_t index_val = 0;
      uint32_t banks_count_val = 0;
      
      std::istringstream istr(str);
      istr >> index_val >> banks_count_val;

      if(istr.fail() || istr.bad())
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::BankSessionIdImpl::from_string: "
          "invalid session id '" << str << "'";
          
        InvalidData e;
        e.description = str;

        throw e;
      }

      index = index_val;
      banks_count = banks_count_val;
    }
    
    void
    BankSessionIdImpl::marshal(CORBA::DataOutputStream* os)
    {
      os -> write_ulong(index);
      os -> write_ulong(banks_count);
    }

    void
    BankSessionIdImpl::unmarshal(CORBA::DataInputStream* is)
    {
      index = is -> read_ulong();
      banks_count = is -> read_ulong();
    }

    //
    // BankSessionImpl class
    //
    void
    BankSessionImpl::register_valuetype_factories(CORBA::ORB* orb)
      throw(CORBA::Exception, El::Exception)
    {
      CORBA::ValueFactoryBase_var factory =
        new NewsGate::Message::BankSessionIdImpl_init();
      
      CORBA::ValueFactoryBase_var old = orb->register_value_factory(
        "IDL:NewsGate/Message/BankSessionId:1.0",
        factory.in());

      factory = new NewsGate::Message::BankSessionImpl_init();
      
      old = orb->register_value_factory("IDL:NewsGate/Message/BankSession:1.0",
                                        factory.in());      
    }

    void
    BankSessionImpl::marshal(CORBA::DataOutputStream* os)
    {
      id_->marshal(os);
      os->write_string(sharing_id_.in());
      os->write_boolean(mirror_);
    }

    void
    BankSessionImpl::unmarshal(CORBA::DataInputStream* is)
    {
      id_ = new BankSessionIdImpl();
      id_->unmarshal(is);
      
      sharing_id_ = is->read_string();
      mirror_ = is->read_boolean();
    }
  }
}
