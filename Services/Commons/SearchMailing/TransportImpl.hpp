/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Commons/SearchMailing/TransportImpl.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_SERVICES_COMMONS_SEARCHMAILING_TRANSPORTIMPL_HPP_
#define _NEWSGATE_SERVER_SERVICES_COMMONS_SEARCHMAILING_TRANSPORTIMPL_HPP_

#include <stdint.h>

#include <El/Exception.hpp>
#include <El/CORBA/Transport/EntityPack.hpp>

#include <Services/Commons/SearchMailing/SearchMailing.hpp>

namespace NewsGate
{
  namespace SearchMailing
  {
    namespace Transport
    {

      struct SubscriptionImpl
      {
        typedef El::Corba::Transport::Entity<
          Subscription,
          SearchMailing::Subscription,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::Entity_init<SearchMailing::Subscription,
                                                  Type> Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };

      struct SubscriptionPackImpl
      {
        typedef El::Corba::Transport::EntityPack<
          SubscriptionPack,
          Subscription,
          SubscriptionArray,
          El::Corba::Transport::TE_GZIP>
        Type;
        
        typedef El::Corba::Transport::EntityPack_init<SubscriptionPack,
                                                      SubscriptionArray,
                                                      Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };

      void register_valuetype_factories(CORBA::ORB* orb)
        throw(CORBA::Exception, El::Exception);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace SearchMailing
  {
    namespace Transport
    {
    }
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_COMMONS_SEARCHMAILING_TRANSPORTIMPL_HPP_
