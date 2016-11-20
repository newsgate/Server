/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Commons/FraudPrevention/TransportImpl.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <El/CORBA/Corba.hpp>

#include <El/Exception.hpp>

#include "TransportImpl.hpp"

namespace NewsGate
{
  namespace FraudPrevention
  {
    namespace Transport
    {      
      void
      register_valuetype_factories(CORBA::ORB* orb)
        throw(CORBA::Exception, El::Exception)
      {
        CORBA::ValueFactoryBase_var factory =
          new EventLimitCheckPackImpl::Init();
      
        CORBA::ValueFactoryBase_var old = orb->register_value_factory(
          "IDL:NewsGate/FraudPrevention/Transport/EventLimitCheckPack:1.0",
          factory);

        factory = new EventLimitCheckResultPackImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/FraudPrevention/Transport/"
          "EventLimitCheckResultPack:1.0",
          factory);
      }
    }
  }
}
