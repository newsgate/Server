/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

#include <El/CORBA/Corba.hpp>

#include "TransportImpl.hpp"

namespace NewsGate
{ 
  namespace Search
  {
    namespace Transport
    {
      void
      register_valuetype_factories(CORBA::ORB* orb)
        throw(CORBA::Exception, El::Exception)
      {
        CORBA::ValueFactoryBase_var factory =
          new NewsGate::Search::Transport::ExpressionImpl::Init();
      
        CORBA::ValueFactoryBase_var old = orb->register_value_factory(
          "IDL:NewsGate/Search/Transport/Expression:1.0",
          factory);

        factory =
          new NewsGate::Search::Transport::StrategyImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Search/Transport/Strategy:1.0",
          factory);

        factory =
          new NewsGate::Search::Transport::ResultImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Search/Transport/Result:1.0",
          factory);

        factory =
          new NewsGate::Search::Transport::StatImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Search/Transport/Stat:1.0",
          factory);
      }
      
    }
  }
}
