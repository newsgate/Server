/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Commons/Statistics/TransportImpl.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <El/CORBA/Corba.hpp>

#include <El/Exception.hpp>

#include "TransportImpl.hpp"

namespace NewsGate
{
  namespace Statistics
  {
    namespace Transport
    {
      void
      register_valuetype_factories(CORBA::ORB* orb)
        throw(CORBA::Exception, El::Exception)
      {
        CORBA::ValueFactoryBase_var factory = new RequestInfoPackImpl::Init();
      
        CORBA::ValueFactoryBase_var old = orb->register_value_factory(
          "IDL:NewsGate/Statistics/Transport/RequestInfoPack:1.0",
          factory);

        factory = new PageImpressionInfoPackImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Statistics/Transport/PageImpressionInfoPack:1.0",
          factory);

        factory = new MessageImpressionInfoPackImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Statistics/Transport/MessageImpressionInfoPack:1.0",
          factory);

        factory = new MessageClickInfoPackImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Statistics/Transport/MessageClickInfoPack:1.0",
          factory);

        factory = new MessageImpressionClickCounterImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Statistics/Transport/MessageImpressionClickCounter:1.0",
          factory);
      }
    }
  }
}
