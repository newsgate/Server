/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Segmentation/Commons/TransportImpl.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_SERVICES_SEGMENTATION_COMMONS_TRANSPORTIMPL_IDL_
#define _NEWSGATE_SERVER_SERVICES_SEGMENTATION_COMMONS_TRANSPORTIMPL_IDL_

#include <stdint.h>
#include <vector>

#include <El/Exception.hpp>
#include <El/BinaryStream.hpp>
#include <El/LightArray.hpp>
#include <El/String/LightString.hpp>

#include <El/CORBA/Transport/EntityPack.hpp>

#include <Commons/Message/StoredMessage.hpp>
#include <Services/Segmentation/Commons/SegmentationServices.hpp>

namespace NewsGate
{
  namespace Segmentation
  {
    namespace Transport
    {
      EL_EXCEPTION(Exception, El::ExceptionBase);
      EL_EXCEPTION(InvalidArgument, Exception);
      
      //
      // Text implementation
      //

      struct Text
      {
        El::String::LightString value;
        Message::SegMarkerPositionSet inserted_spaces;

        void set(const char* result, const char* src)
          throw(InvalidArgument, Exception, El::Exception);
        
        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);
      };

      //
      // MessagePack implementation
      //

      struct Message
      {
        struct Image
        {
          El::String::LightString alt;
          
          void write(El::BinaryOutStream& bstr) const throw(El::Exception);
          void read(El::BinaryInStream& bstr) throw(El::Exception);
        };

        typedef El::LightArray<Image, ::NewsGate::Message::WordPositionNumber>
        ImageArray;
        
        El::String::LightString title;
        El::String::LightString description;
        El::String::LightString keywords;
        ImageArray images;

        Message() throw(El::Exception) {}

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);        
      };

      typedef std::vector<Message> MessageArray;
      
      struct MessagePackImpl
      {
        typedef El::Corba::Transport::EntityPack<
          MessagePack,
          Message,
          MessageArray,
          El::Corba::Transport::TE_GZIP>
        Type;
        
        typedef El::Corba::Transport::EntityPack_init<MessagePack,
                                                      MessageArray,
                                                      Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };
      
      //
      // SegmentedMessagePack implementation
      //

      struct SegmentedMessage
      {        
        struct Image
        {
          Text alt;

          void write(El::BinaryOutStream& bstr) const throw(El::Exception);
          void read(El::BinaryInStream& bstr) throw(El::Exception);
        };

        typedef El::LightArray<Image, ::NewsGate::Message::WordPositionNumber>
        ImageArray;        
        
        Text title;
        Text description;
        Text keywords;
        ImageArray images;

        SegmentedMessage() throw(El::Exception) {}

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);        
      };

      typedef std::vector<SegmentedMessage> SegmentedMessageArray;
      
      struct SegmentedMessagePackImpl
      {
        typedef El::Corba::Transport::EntityPack<
          SegmentedMessagePack,
          SegmentedMessage,
          SegmentedMessageArray,
          El::Corba::Transport::TE_GZIP>
        Type;
        
        typedef El::Corba::Transport::EntityPack_init<SegmentedMessagePack,
                                                      SegmentedMessageArray,
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
  namespace Segmentation
  {
    namespace Transport
    {
      //
      // Message struct
      //
      inline
      void
      Message::write(El::BinaryOutStream& bstr) const throw(El::Exception)
      {
        bstr << title << description << images << keywords;
      }
      
      inline
      void
      Message::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> title >> description >> images >> keywords;
      }

      //
      // Message::Image struct
      //
      inline
      void
      Message::Image::write(El::BinaryOutStream& bstr) const
        throw(El::Exception)
      {
        bstr << alt;
      }
      
      inline
      void
      Message::Image::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> alt;
      }

      //
      // SegmentedMessage struct
      //
      inline
      void
      SegmentedMessage::write(El::BinaryOutStream& bstr) const
        throw(El::Exception)
      {
        bstr << title << description << images << keywords;
      }
      
      inline
      void
      SegmentedMessage::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> title >> description >> images >> keywords;
      }

      //
      // SegmentedMessage::Image struct
      //
      inline
      void
      SegmentedMessage::Image::write(El::BinaryOutStream& bstr) const
        throw(El::Exception)
      {
        bstr << alt;
      }
      
      inline
      void
      SegmentedMessage::Image::read(El::BinaryInStream& bstr)
        throw(El::Exception)
      {
        bstr >> alt;
      }

      //
      // Text struct
      //
      inline
      void
      Text::write(El::BinaryOutStream& bstr) const throw(El::Exception)
      {
        bstr << value;
        bstr.write_set(inserted_spaces);
      }
      
      inline
      void
      Text::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> value;
        bstr.read_set(inserted_spaces);
      }
    }
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_SEGMENTATION_COMMONS_TRANSPORTIMPL_IDL_
