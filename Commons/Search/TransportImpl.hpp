/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Commons/Search/TransportImpl.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_COMMONS_SEARCH_TRANSPORTIMPL_HPP_
#define _NEWSGATE_SERVER_COMMONS_SEARCH_TRANSPORTIMPL_HPP_

#include <memory>
#include <vector>

#include <El/Exception.hpp>

#include <El/BinaryStream.hpp>
#include <El/CORBA/Transport/EntityPack.hpp>
#include <Commons/Search/SearchTransport.hpp>

#include <Commons/Search/SearchExpression.hpp>

namespace NewsGate
{ 
  namespace Search
  {
    namespace Transport
    {
      struct ExpressionHolder
      {
        NewsGate::Search::Expression_var expression;

        ExpressionHolder(NewsGate::Search::Expression* expr = 0) throw();
        
        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);
      };
      
      struct ExpressionImpl
      {
        typedef El::Corba::Transport::Entity<
          NewsGate::Search::Transport::Expression,
          ExpressionHolder,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::Entity_init<ExpressionHolder, Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };

      struct StrategyImpl
      {
        typedef El::Corba::Transport::Entity<
          NewsGate::Search::Transport::Strategy,
          NewsGate::Search::Strategy,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::Entity_init<NewsGate::Search::Strategy,
                                                  Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };

      struct ResultImpl
      {
        typedef El::Corba::Transport::Entity<
          NewsGate::Search::Transport::Result,
          NewsGate::Search::Result,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::Entity_init<NewsGate::Search::Result,
                                                  Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };
      
      struct StatImpl
      {
        typedef El::Corba::Transport::Entity<
          NewsGate::Search::Transport::Stat,
          NewsGate::Search::Stat,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::Entity_init<NewsGate::Search::Stat,
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
  namespace Search
  {
    namespace Transport
    {
      //
      // ExpressionHolder struct
      //

      inline
      ExpressionHolder::ExpressionHolder(NewsGate::Search::Expression* expr)
        throw() : expression(expr)
      {
      }
      
      inline
      void
      ExpressionHolder::write(El::BinaryOutStream& bstr) const
        throw(El::Exception)
      {
        bstr << *expression;
      }
      
      inline
      void
      ExpressionHolder::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        expression = new NewsGate::Search::Expression();
        bstr >> *expression;
      }

    }
  }
}

#endif // _NEWSGATE_SERVER_COMMONS_SEARCH_TRANSPORTIMPL_HPP_
