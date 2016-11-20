/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Event/Commons/BankSessionImpl.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <El/CORBA/Corba.hpp>

#include <sstream>

#include <El/Exception.hpp>

#include <Services/Commons/Event/EventServices.hpp>

#include "BankSessionImpl.hpp"

namespace NewsGate
{
  namespace Event
  {
    //
    // BankSessionIdImpl class
    //
    char*
    BankSessionIdImpl::to_string()
    {
      std::ostringstream ostr;
      ostr << guid << "\t" << index << "\t" << banks_count;
      
      return CORBA::string_dup(ostr.str().c_str());
    }
    
    void
    BankSessionIdImpl::from_string(const char* str)
    {
      std::string guid_val;
      uint32_t index_val = 0;
      uint32_t banks_count_val = 0;
      
      std::istringstream istr(str);
      istr >> guid_val >> index_val >> banks_count_val;

      if(istr.fail() || istr.bad())
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Event::BankSessionIdImpl::from_string: "
          "invalid session id '" << str << "'";
          
        InvalidData e;
        e.description = str;

        throw e;
      }

      guid = guid_val;
      index = index_val;
      banks_count = banks_count_val;
    }
    
    void
    BankSessionIdImpl::marshal(CORBA::DataOutputStream* os)
    {
      os->write_string(guid.c_str());
      os->write_ulong(index);
      os->write_ulong(banks_count);
    }

    void
    BankSessionIdImpl::unmarshal(CORBA::DataInputStream* is)
    {
      CORBA::String_var guid_val = is->read_string();
      guid = guid_val.in();
      
      index = is->read_ulong();
      banks_count = is->read_ulong();
    }

    //
    // BankSessionImpl class
    //
    void
    BankSessionImpl::register_valuetype_factories(CORBA::ORB* orb)
      throw(CORBA::Exception, El::Exception)
    {
      CORBA::ValueFactoryBase_var factory =
        new NewsGate::Event::BankSessionIdImpl_init();
      
      CORBA::ValueFactoryBase_var old = orb->register_value_factory(
        "IDL:NewsGate/Event/BankSessionId:1.0",
        factory.in());

      factory = new NewsGate::Event::BankSessionImpl_init();
      
      old = orb->register_value_factory("IDL:NewsGate/Event/BankSession:1.0",
                                        factory.in());      
    }

    void
    BankSessionImpl::marshal(CORBA::DataOutputStream* os)
    {
      id_->marshal(os);
      os->write_Object(left_neighbour_.in());
      os->write_Object(right_neighbour_.in());
      os->write_ulong(bank_count_);
    }

    void
    BankSessionImpl::unmarshal(CORBA::DataInputStream* is)
    {
      id_ = new BankSessionIdImpl();
      id_->unmarshal(is);

      CORBA::Object_var object = is->read_Object();
      left_neighbour_ = Bank::_narrow(object.in());      

      object = is->read_Object();
      right_neighbour_ = Bank::_narrow(object.in());
      
      bank_count_ = is->read_ulong();
    }
  }
}
