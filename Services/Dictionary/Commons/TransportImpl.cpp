/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Dictionary/Commons/TransportImpl.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <El/CORBA/Corba.hpp>

#include <string>
#include <sstream>
#include <iostream>

#include <El/Exception.hpp>

#include "TransportImpl.hpp"

namespace NewsGate
{
  namespace Dictionary
  {
    namespace Transport
    {      
      void
      register_valuetype_factories(CORBA::ORB* orb)
        throw(CORBA::Exception, El::Exception)
      {
        CORBA::ValueFactoryBase_var factory =
          new NewsGate::Dictionary::Transport::MessageWordsPackImpl::Init();
      
        CORBA::ValueFactoryBase_var old = orb->register_value_factory(
          "IDL:NewsGate/Dictionary/Transport/MessageWordsPack:1.0",
          factory);

        factory = new NewsGate::Dictionary::Transport::
          NormalizedMessageWordsPackImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Dictionary/Transport/NormalizedMessageWordsPack:1.0",
          factory);

        factory = new NewsGate::Dictionary::Transport::
          NormalizedWordsPackImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Dictionary/Transport/NormalizedWordsPack:1.0",
          factory);

        factory =
          new NewsGate::Dictionary::Transport::GetLemmasParamsImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Dictionary/Transport/GetLemmasParams:1.0",
          factory);

        factory = new NewsGate::Dictionary::Transport::LemmaPackImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Dictionary/Transport/LemmaPack:1.0",
          factory);
      }
    }
  }
}
