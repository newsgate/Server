/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Commons/FraudPrevention/TransportImpl.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_SERVICES_COMMONS_FRAUDPREVENTION_TRANSPORTIMPL_HPP_
#define _NEWSGATE_SERVER_SERVICES_COMMONS_FRAUDPREVENTION_TRANSPORTIMPL_HPP_

#include <stdint.h>

#include <vector>

#include <El/Exception.hpp>
#include <El/Hash/Hash.hpp>
#include <El/CRC.hpp>
#include <El/CORBA/Transport/EntityPack.hpp>

#include <Services/Commons/FraudPrevention/FraudPreventionServices.hpp>
#include <Services/Commons/FraudPrevention/LimitCheck.hpp>

namespace NewsGate
{
  namespace FraudPrevention
  {
    namespace Transport
    {
      struct EventLimitCheckPackImpl
      {
        typedef El::Corba::Transport::EntityPack<
          EventLimitCheckPack,
          EventLimitCheck,
          EventLimitCheckArray,
          El::Corba::Transport::TE_GZIP>
        Type;
        
        typedef El::Corba::Transport::EntityPack_init<EventLimitCheckPack,
                                                      EventLimitCheckArray,
                                                      Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };

      struct EventLimitCheckResultPackImpl
      {
        typedef El::Corba::Transport::EntityPack<
          EventLimitCheckResultPack,
          EventLimitCheckResult,
          EventLimitCheckResultArray,
          El::Corba::Transport::TE_GZIP>
        Type;
        
        typedef El::Corba::Transport::EntityPack_init<
          EventLimitCheckResultPack,
          EventLimitCheckResultArray,
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
  namespace FraudPrevention
  {
    namespace Transport
    {
    }
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_COMMONS_FRAUDPREVENTION_TRANSPORTIMPL_HPP_
