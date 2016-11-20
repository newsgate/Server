/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Commons/Event/TransportImpl.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_COMMONS_EVENT_TRANSPORTIMPL_HPP_
#define _NEWSGATE_SERVER_COMMONS_EVENT_TRANSPORTIMPL_HPP_

#include <stdint.h>

#include <vector>

#include <El/Exception.hpp>
#include <El/Luid.hpp>
#include <El/Lang.hpp>

#include <El/CORBA/Transport/EntityPack.hpp>

#include <Commons/Event/Event.hpp>
#include <Commons/Event/EventTransport.hpp>

#include <Commons/Message/StoredMessage.hpp>
#include <Commons/Message/MessageTransport.hpp>

namespace NewsGate
{
  namespace Event
  {
    namespace Transport
    {
      //
      // MessageDigestPack implementation
      //
      struct MessageDigest
      {
        Message::Id id;
        uint64_t published;
        El::Lang lang;
        Message::CoreWords core_words;
        El::Luid event_id;

        MessageDigest(const Message::Id& id_val,
                      uint64_t published_val,
                      const El::Lang& lang_val,
                      const Message::CoreWords& core_words_val,
                      const El::Luid& event_id_val)
          throw(El::Exception);

        MessageDigest() throw(El::Exception);

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);        
      };
      
      typedef std::vector<MessageDigest> MessageDigestArray;

      struct MessageDigestPackImpl
      {
        typedef El::Corba::Transport::EntityPack<
          MessageDigestPack,
          MessageDigest,
          MessageDigestArray,
          El::Corba::Transport::TE_GZIP>
        Type;
        
        typedef El::Corba::Transport::EntityPack_init<MessageDigestPack,
                                                      MessageDigestArray,
                                                      Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };

      //
      // EventPushInfoPack implementation
      //

      struct EventPushInfo
      {
        uint8_t  flags;
        int32_t  spin;
        uint32_t dissenters;
        El::Luid id;

        EventPushInfo(const El::Luid& id_val = El::Luid::null,
                      int32_t spin_val = 0,
                      uint8_t flags_val = 0,
                      uint32_t dissenters_val = 0) throw(El::Exception);

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);        
      };
      
      typedef std::vector<EventPushInfo> EventPushInfoArray;

      struct EventPushInfoPackImpl
      {
        typedef El::Corba::Transport::EntityPack<
          EventPushInfoPack,
          EventPushInfo,
          EventPushInfoArray,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::EntityPack_init<EventPushInfoPack,
                                                      EventPushInfoArray,
                                                      Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };

      //
      // EventIdPack implementation
      //

      typedef std::vector<El::Luid> EventIdArray;

      struct EventIdPackImpl
      {
        typedef El::Corba::Transport::EntityPack<
          EventIdPack,
          El::Luid,
          EventIdArray,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::EntityPack_init<EventIdPack,
                                                      EventIdArray,
                                                      Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };

      //
      // EventIdRelPack implementation
      //

      struct EventIdRel
      {
        El::Luid id;
        El::Luid rel;
        Message::Id split;
        uint32_t separate; // Word id
        uint8_t narrow_separation;

        EventIdRel() throw(El::Exception) {}
        
        EventIdRel(El::Luid id_val,
                   El::Luid rel_val = El::Luid::null,
                   Message::Id split_val = Message::Id::zero,
                   uint32_t separate_val = 0,
                   uint8_t narrow_separation_val = 0)
          throw(El::Exception);

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);        
      };

      typedef std::vector<EventIdRel> EventIdRelArray;

      struct EventIdRelPackImpl
      {
        typedef El::Corba::Transport::EntityPack<
          EventIdRelPack,
          EventIdRel,
          EventIdRelArray,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::EntityPack_init<EventIdRelPack,
                                                      EventIdRelArray,
                                                      Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };

      //
      // EventObjectRelPack implementation
      //

      struct EventRel
      {
        EventObject object1;
        EventObject object2;
        uint8_t colocated;
        uint64_t merge_blacklist_timeout;
        EventObject merge_result;

        EventRel() throw(El::Exception);

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);        
      };
      
      struct EventParts
      {
        EventObject part1;
        EventObject part2;
        EventRel merge1;
        EventRel merge2;

        EventParts() throw(El::Exception) {}

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);        
      };
      
      struct EventObjectRel
      {
        uint8_t changed;
        EventRel rel;
        EventParts split;
        EventParts separate;

        EventObjectRel() throw(El::Exception) : changed(0) {}
        
        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);        
      };      

      typedef std::vector<EventObjectRel> EventObjectRelArray;

      struct EventObjectRelPackImpl
      {
        typedef El::Corba::Transport::EntityPack<
          EventObjectRelPack,
          EventObjectRel,
          EventObjectRelArray,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::EntityPack_init<EventObjectRelPack,
                                                      EventObjectRelArray,
                                                      Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };

      //
      // EventObjectPack implementation
      //

      typedef std::vector<EventObject> EventObjectArray;

      struct EventObjectPackImpl
      {
        typedef El::Corba::Transport::EntityPack<
          EventObjectPack,
          EventObject,
          EventObjectArray,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::EntityPack_init<EventObjectPack,
                                                      EventObjectArray,
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
  namespace Event
  {
    namespace Transport
    {      
      //
      // MessageDigest struct
      //
      inline
      MessageDigest::MessageDigest() throw(El::Exception) : published(0)
      {
      }

      inline
      MessageDigest::MessageDigest(const Message::Id& id_val,
                                   uint64_t published_val,
                                   const El::Lang& lang_val,
                                   const Message::CoreWords& core_words_val,
                                   const El::Luid& event_id_val)
        throw(El::Exception)
          : id(id_val),
            published(published_val),
            lang(lang_val),
            core_words(core_words_val),
            event_id(event_id_val)
      {
      }
      
      inline
      void
      MessageDigest::write(El::BinaryOutStream& bstr) const
        throw(El::Exception)
      {
        bstr << id << published << lang << core_words << event_id;
      }
      
      inline
      void
      MessageDigest::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> id >> published >> lang >> core_words >> event_id;
      }

      //
      // EventPushInfo struct
      //
      inline
      EventPushInfo::EventPushInfo(const El::Luid& id_val,
                                   int32_t spin_val,
                                   uint8_t flags_val,
                                   uint32_t dissenters_val)
        throw(El::Exception) :
          flags(flags_val),
          spin(spin_val),
          dissenters(dissenters_val),
          id(id_val)
      {
      }
      
      inline
      void
      EventPushInfo::write(El::BinaryOutStream& bstr) const
        throw(El::Exception)
      {
        bstr << id << spin << flags << dissenters;
      }
      
      inline
      void
      EventPushInfo::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> id >> spin >> flags >> dissenters;
      }

      //
      // EventIdRel struct
      //

      inline
      EventIdRel::EventIdRel(El::Luid id_val,
                             El::Luid rel_val,
                             Message::Id split_val,
                             uint32_t separate_val,
                             uint8_t narrow_separation_val)
        throw(El::Exception)
          : id(id_val),
            rel(rel_val),
            split(split_val),
            separate(separate_val),
            narrow_separation(narrow_separation_val)
      {
      }
      
      inline
      void
      EventIdRel::write(El::BinaryOutStream& bstr) const throw(El::Exception)
      {
        bstr << id << rel << split << separate << narrow_separation;
      }
      
      inline
      void
      EventIdRel::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> id >> rel >> split >> separate >> narrow_separation;
      }      

      //
      // EventRel struct
      //
      inline
      EventRel::EventRel() throw(El::Exception)
          : colocated(0),
            merge_blacklist_timeout(0)
      {
      }
      
      inline
      void
      EventRel::write(El::BinaryOutStream& bstr) const throw(El::Exception)
      {
        bstr << object1 << object2 << colocated << merge_blacklist_timeout
             << merge_result;
      }
      
      inline
      void
      EventRel::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> object1 >> object2 >> colocated >> merge_blacklist_timeout
             >> merge_result;
      }

      //
      // EventParts struct
      //
      inline
      void
      EventParts::write(El::BinaryOutStream& bstr) const throw(El::Exception)
      {
        bstr << part1 << part2 << merge1 << merge2;
      }
      
      inline
      void
      EventParts::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> part1 >> part2 >> merge1 >> merge2;
      }      

      //
      // EventObjectRel struct
      //
      inline
      void
      EventObjectRel::write(El::BinaryOutStream& bstr) const
        throw(El::Exception)
      {
        bstr << changed << rel << split << separate;
      }
      
      inline
      void
      EventObjectRel::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> changed >> rel >> split >> separate;
      }
    }
  }
}

#endif // _NEWSGATE_SERVER_COMMONS_EVENT_TRANSPORTIMPL_HPP_
