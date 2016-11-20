/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Commons/Ad/TransportImpl.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_SERVICES_COMMONS_AD_TRANSPORTIMPL_HPP_
#define _NEWSGATE_SERVER_SERVICES_COMMONS_AD_TRANSPORTIMPL_HPP_

#include <stdint.h>

#include <vector>

#include <El/Exception.hpp>
#include <El/Hash/Hash.hpp>
#include <El/CRC.hpp>
#include <El/CORBA/Transport/EntityPack.hpp>

#include <Commons/Ad/Ad.hpp>
#include <Services/Commons/Ad/AdServices.hpp>

namespace NewsGate
{
  namespace Ad
  {
    namespace Transport
    {
      struct SelectorImpl
      {
        typedef El::Corba::Transport::Entity<
          Selector,
          ::NewsGate::Ad::Selector,
          El::Corba::Transport::TE_GZIP>
        Type;
        
        typedef El::Corba::Transport::Entity_init< ::NewsGate::Ad::Selector,
                                                   Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };

      struct SelectionContextImpl
      {
        typedef El::Corba::Transport::Entity<
          SelectionContext,
          ::NewsGate::Ad::SelectionContext,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::Entity_init<
          ::NewsGate::Ad::SelectionContext, Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };

      struct SelectionResultImpl
      {
        typedef El::Corba::Transport::Entity<
          SelectionResult,
          ::NewsGate::Ad::SelectionResult,
          El::Corba::Transport::TE_GZIP>
        Type;
        
        typedef El::Corba::Transport::Entity_init<
          ::NewsGate::Ad::SelectionResult, Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };

      struct SelectionImpl
      {
        typedef El::Corba::Transport::Entity<
          Selection,
          ::NewsGate::Ad::Selection,
          El::Corba::Transport::TE_GZIP>
        Type;
        
        typedef El::Corba::Transport::Entity_init<
          ::NewsGate::Ad::Selection, Type>
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
  namespace Ad
  {
    namespace Transport
    {
    }
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_COMMONS_AD_TRANSPORTIMPL_HPP_
