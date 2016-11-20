/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Server/Commons/Event/TransportImpl.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <El/CORBA/Corba.hpp>

#include <El/Exception.hpp>

#include <Commons/Event/EventTransport.hpp>

#include "TransportImpl.hpp"

namespace NewsGate
{
  namespace Event
  {
    namespace Transport
    {
      void
      register_valuetype_factories(CORBA::ORB* orb)
        throw(CORBA::Exception, El::Exception)
      {
        CORBA::ValueFactoryBase_var factory =
          new MessageDigestPackImpl::Init();
      
        CORBA::ValueFactoryBase_var old = orb->register_value_factory(
          "IDL:NewsGate/Event/Transport/MessageDigestPack:1.0",
          factory);

        factory = new EventPushInfoPackImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Event/Transport/EventPushInfoPack:1.0",
          factory);
        
        factory = new EventIdPackImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Event/Transport/EventIdPack:1.0",
          factory);
        
        factory = new EventIdRelPackImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Event/Transport/EventIdRelPack:1.0",
          factory);
        
        factory = new EventObjectPackImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Event/Transport/EventObjectPack:1.0",
          factory);
        
        factory = new EventObjectRelPackImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Event/Transport/EventObjectRelPack:1.0",
          factory);
        
      }
    }
  }
}
