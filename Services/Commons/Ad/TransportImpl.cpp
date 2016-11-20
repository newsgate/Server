/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Commons/Ad/TransportImpl.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <El/CORBA/Corba.hpp>

#include <El/Exception.hpp>

#include "TransportImpl.hpp"

namespace NewsGate
{
  namespace Ad
  {
    namespace Transport
    {      
      void
      register_valuetype_factories(CORBA::ORB* orb)
        throw(CORBA::Exception, El::Exception)
      {
        CORBA::ValueFactoryBase_var factory = new SelectorImpl::Init();
      
        CORBA::ValueFactoryBase_var old = orb->register_value_factory(
          "IDL:NewsGate/Ad/Transport/Selector:1.0",
          factory);

        factory = new SelectionContextImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Ad/Transport/SelectionContext:1.0",
          factory);
        
        factory = new SelectionResultImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Ad/Transport/SelectionResult:1.0",
          factory);        
        
        factory = new SelectionImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Ad/Transport/Selection:1.0",
          factory);
      }
    }
  }
}
