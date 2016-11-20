/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Server/Commons/Message/TransportImpl.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <El/CORBA/Corba.hpp>

#include <wchar.h>

#include <string>
#include <sstream>

#include <El/Exception.hpp>
#include <El/String/Manip.hpp>

#include <Commons/Message/MessageTransport.hpp>
#include <Commons/Message/Message.hpp>

#include "TransportImpl.hpp"

namespace NewsGate
{
  namespace Message
  {
    namespace Transport
    {
      //
      // Valutypes factory register
      //
      void
      register_valuetype_factories(CORBA::ORB* orb)
        throw(CORBA::Exception, El::Exception)
      {
        CORBA::ValueFactoryBase_var factory =
          new NewsGate::Message::Transport::RawMessagePackImpl::Init();
      
        CORBA::ValueFactoryBase_var old = orb->register_value_factory(
          "IDL:NewsGate/Message/Transport/RawMessagePack:1.0",
          factory);

        factory =
          new NewsGate::Message::Transport::StoredMessagePackImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Message/Transport/StoredMessagePack:1.0",
          factory);

        factory = new NewsGate::Message::Transport::IdPackImpl::Init();
        
        old = orb->register_value_factory(
          "IDL:NewsGate/Message/Transport/IdPack:1.0",
          factory);

        factory = new NewsGate::Message::Transport::LocalCodePackImpl::Init();
      
        old = orb->register_value_factory (
          "IDL:NewsGate/Message/Transport/LocalCodePack:1.0",
          factory);

        factory =
          new NewsGate::Message::Transport::MessageEventPackImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Message/Transport/MessageEventPack:1.0",
          factory);

        factory =
          new NewsGate::Message::Transport::MessageSharingInfoPackImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Message/Transport/MessageSharingInfoPack:1.0",
          factory);

        factory =
          new NewsGate::Message::Transport::ColoFrontendPackImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Message/Transport/ColoFrontendPack:1.0",
          factory);

        factory =
          new NewsGate::Message::Transport::EmptyResponseImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Message/Transport/EmptyResponse:1.0",
          factory);

        factory = new NewsGate::Message::Transport::
          SetMessageFetchFilterRequestImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Message/Transport/SetMessageFetchFilterRequest:1.0",
          factory);

        factory = new NewsGate::Message::Transport::
          SetMessageCategorizerRequestImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Message/Transport/SetMessageCategorizerRequest:1.0",
          factory);

        factory =
          new NewsGate::Message::Transport::EmptyRequestImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Message/Transport/EmptyRequest:1.0",
          factory);

        factory =
          new NewsGate::Message::Transport::MessageDigestRequestImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Message/Transport/MessageDigestRequest:1.0",
          factory);

        factory =
          new NewsGate::Message::Transport::MessageDigestResponseImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Message/Transport/MessageDigestResponse:1.0",
          factory);

        factory =
          new NewsGate::Message::Transport::MessageStatRequestImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Message/Transport/MessageStatRequest:1.0",
          factory);

        factory =
          new NewsGate::Message::Transport::MessageStatResponseImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Message/Transport/MessageStatResponse:1.0",
          factory);

        factory = new NewsGate::Message::Transport::IdImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Message/Transport/Id:1.0",
          factory);

        factory = new NewsGate::Message::Transport::StoredMessageImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Message/Transport/StoredMessage:1.0",
          factory);

        factory = new NewsGate::Message::Transport::
          SetMessageSharingSourcesRequestImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Message/Transport/SetMessageSharingSourcesRequest:1.0",
          factory);

        factory = new NewsGate::Message::Transport::
          CheckMirroredMessagesRequestImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Message/Transport/CheckMirroredMessagesRequest:1.0",
          factory);        
        
        factory = new NewsGate::Message::Transport::
          CheckMirroredMessagesResponseImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Message/Transport/CheckMirroredMessagesResponse:1.0",
          factory);

        factory = new NewsGate::Message::Transport::CategoryLocaleImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Message/Transport/CategoryLocale:1.0",
          factory);
      }

      //
      // MessageDigestResponseImpl::Type class
      //
      
      void
      MessageDigestResponseImpl::Type::absorb(Response* src)
        throw(Response::ImplementationException,
              CORBA::SystemException)
      {
        Type* response = dynamic_cast<Type*>(src);

        if(response == 0)
        {
          Response::ImplementationException ex;
          ex.description = "MessageDigestResponseImpl::Type::absorb: "
            "dynamic_cast<Type*> failed";
          
          throw ex;
        }
        
        WriteGuard guard(lock_);

        entities().insert(entities().begin(),
                          response->entities().begin(),
                          response->entities().end());
      }

    }
  }
}
