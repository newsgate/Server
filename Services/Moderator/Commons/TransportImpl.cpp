/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Moderator/Commons/TransportImpl.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <Python.h>

#include <El/CORBA/Corba.hpp>

#include <string>
#include <sstream>
#include <iostream>

#include <El/Exception.hpp>

#include "TransportImpl.hpp"

namespace NewsGate
{
  namespace Moderation
  {
    namespace Transport
    {      
      void
      register_valuetype_factories(CORBA::ORB* orb)
        throw(CORBA::Exception, El::Exception)
      {
        CORBA::ValueFactoryBase_var factory =
          new NewsGate::Moderation::Transport::MsgAdjustmentContextImpl::
          Init();
      
        CORBA::ValueFactoryBase_var old = orb->register_value_factory(
          "IDL:NewsGate/Moderation/Transport/MsgAdjustmentContext:1.0",
          factory);

        factory =
          new NewsGate::Moderation::Transport::MsgAdjustmentResultImpl::
          Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Moderation/Transport/MsgAdjustmentResult:1.0",
          factory);

        factory =
          new NewsGate::Moderation::Transport::MsgAdjustmentContextPackImpl::
          Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Moderation/Transport/MsgAdjustmentContextPack:1.0",
          factory);        
        
        factory =
          new NewsGate::Moderation::Transport::MessagePackImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Moderation/Transport/MessagePack:1.0",
          factory);

        factory =
          new NewsGate::Moderation::Transport::GetHTMLItemsResultImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Moderation/Transport/GetHTMLItemsResult:1.0",
          factory);        
      }
    }
  }
}
