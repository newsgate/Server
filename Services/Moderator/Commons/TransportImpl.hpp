/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Moderator/Commons/TransportImpl.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MODERATOR_COMMONS_TRANSPORTIMPL_IDL_
#define _NEWSGATE_SERVER_SERVICES_MODERATOR_COMMONS_TRANSPORTIMPL_IDL_

#include <El/Exception.hpp>
#include <El/BinaryStream.hpp>

#include <El/CORBA/Transport/EntityPack.hpp>

#include <Commons/Message/Automation/Automation.hpp>
#include <xsd/DataFeed/RSS/MsgAdjustment.hpp>
#include <Services/Moderator/Commons/FeedManager.hpp>

namespace NewsGate
{
  namespace Moderation
  {
    namespace Transport
    {
      //
      // MsgAdjustmentContextImpl implementation
      //
      struct MsgAdjustmentContextImpl
      {
        typedef El::Corba::Transport::Entity<
          NewsGate::Moderation::Transport::MsgAdjustmentContext,
          ::NewsGate::RSS::MsgAdjustment::Context,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::Entity_init<
          ::NewsGate::RSS::MsgAdjustment::Context,
          Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };

      //
      // MsgAdjustmentResultImpl implementation
      //
      struct MsgAdjustmentResultStruct
      {
        Message::Automation::Message message;
        std::string log;
        std::string error;

        void write(El::BinaryOutStream& ostr) const
          throw(El::Exception);

        void read(El::BinaryInStream& istr) throw(El::Exception);        
      };

      struct MsgAdjustmentResultImpl
      {
        typedef El::Corba::Transport::Entity<
          NewsGate::Moderation::Transport::MsgAdjustmentResult,
          MsgAdjustmentResultStruct,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::Entity_init<MsgAdjustmentResultStruct,
                                                  Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };

      //
      // MsgAdjustmentContextPackImpl implementation
      //
      struct MsgAdjustmentContextPackImpl
      {
        typedef El::Corba::Transport::EntityPack<
          MsgAdjustmentContextPack,
          ::NewsGate::RSS::MsgAdjustment::Context,
          ::NewsGate::RSS::MsgAdjustment::ContextArray,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::EntityPack_init<
          MsgAdjustmentContextPack,
          ::NewsGate::RSS::MsgAdjustment::ContextArray,
          Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueVar<Type> Out;
      };
      
      //
      // MessagePackImpl implementation
      //
      struct MessagePackImpl
      {
        typedef El::Corba::Transport::EntityPack<
          MessagePack,
          ::NewsGate::Message::Automation::Message,
          ::NewsGate::Message::Automation::MessageArray,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::EntityPack_init<
          MessagePack,
          ::NewsGate::Message::Automation::MessageArray,
          Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueVar<Type> Out;
      };


      //
      // GetHTMLItemsResultImpl implementation
      //
      struct GetHTMLItemsResultStruct
      {
        ::NewsGate::Message::Automation::MessageArray messages;
        std::string log;
        std::string error;
        std::string cache;
        uint8_t interrupted;

        GetHTMLItemsResultStruct() throw() : interrupted(0) {}

        void write(El::BinaryOutStream& ostr) const
          throw(El::Exception);

        void read(El::BinaryInStream& istr) throw(El::Exception);        
      };

      struct GetHTMLItemsResultImpl
      {
        typedef El::Corba::Transport::Entity<
          NewsGate::Moderation::Transport::GetHTMLItemsResult,
          GetHTMLItemsResultStruct,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::Entity_init<GetHTMLItemsResultStruct,
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
  namespace Moderation
  {
    namespace Transport
    {
      //
      // MsgAdjustmentResultStruct struct
      //
      inline
      void
      MsgAdjustmentResultStruct::write(El::BinaryOutStream& ostr) const
        throw(El::Exception)
      {
        ostr << message << log << error;
      }        

      inline
      void
      MsgAdjustmentResultStruct::read(El::BinaryInStream& istr)
        throw(El::Exception)
      {
        istr >> message >> log >> error;
      }

      //
      // GetHTMLItemsResultStruct struct
      //
      inline
      void
      GetHTMLItemsResultStruct::write(El::BinaryOutStream& ostr) const
        throw(El::Exception)
      {
        ostr.write_array(messages);        
        ostr << log << error << cache << interrupted;
      }

      inline
      void
      GetHTMLItemsResultStruct::read(El::BinaryInStream& istr)
        throw(El::Exception)
      {
        istr.read_array(messages);        
        istr >> log >> error >> cache >> interrupted;
      }
    }
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_MODERATOR_COMMONS_TRANSPORTIMPL_IDL_
