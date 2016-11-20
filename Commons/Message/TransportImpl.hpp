/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Commons/Message/TransportImpl.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_COMMONS_MESSAGE_TRANSPORTIMPL_HPP_
#define _NEWSGATE_SERVER_COMMONS_MESSAGE_TRANSPORTIMPL_HPP_

#include <stdint.h>

#include <memory>
#include <vector>

#include <ext/hash_map>
#include <ext/hash_set>

#include <El/Exception.hpp>
#include <El/Hash/Hash.hpp>
#include <El/Luid.hpp>

#include <El/CORBA/Transport/EntityPack.hpp>

#include <Commons/Message/MessageTransport.hpp>
#include <Commons/Message/Message.hpp>
#include <Commons/Message/StoredMessage.hpp>
#include <Commons/Message/FetchFilter.hpp>
#include <Commons/Message/Categorizer.hpp>

namespace NewsGate
{
  namespace Message
  {
    namespace Transport
    {
      //
      // SetMessageSharingIdsRequest implementation
      //

      typedef El::SerializableSet<
        __gnu_cxx::hash_set<std::string, El::Hash::String> > SharingIdSet;

      struct MessageSharingSources
      {
        SharingIdSet ids;
        
        // Set in cluster mirror mode
        std::string mirrored_manager;

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);        
      };
      
      struct SetMessageSharingSourcesRequestImpl
      {
        typedef El::Corba::Transport::Entity<
          NewsGate::Message::Transport::SetMessageSharingSourcesRequest,
          MessageSharingSources,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::Entity_init<MessageSharingSources,
                                                  Type> Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };
      
      //
      // LocalCodePack implementation
      //

      struct LocalCodePackImpl
      {
        typedef El::Corba::Transport::EntityPack<
          LocalCodePack,
          LocalCode,
          LocalCodeArray,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::EntityPack_init<LocalCodePack,
                                                      LocalCodeArray,
                                                      Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueVar<Type> Out;
      };

      //
      // IdPack implementation
      //
      
      struct IdPackImpl
      {
        typedef El::Corba::Transport::EntityPack<
          IdPack,
          Id,
          IdArray,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::EntityPack_init<IdPack,
                                                      IdArray,
                                                      Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };

      //
      // RawMessagePack implementation
      //
    
      struct RawMessagePackImpl
      {
        typedef El::Corba::Transport::EntityPack<
          RawMessagePack,
          RawMessage,
          RawMessageArray,
          El::Corba::Transport::TE_GZIP> Type;
        
        typedef El::Corba::Transport::EntityPack_init<RawMessage,
                                                      RawMessageArray,
                                                      Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;

        typedef RawMessageArray MessageArray;
      };

      //
      // StoredMessagePack implementation
      //
    
      struct DebugInfo
      {
        uint8_t on;
        NewsGate::Message::WordsFreqInfo words_freq;
        uint32_t match_weight;        
        uint64_t feed_impressions;
        uint64_t feed_clicks;

        DebugInfo() throw(El::Exception);
        
        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);        

        void steal(DebugInfo& src) throw(El::Exception);
      };
      
      struct StoredMessageDebug
      {
        NewsGate::Message::StoredMessage message;
        DebugInfo debug_info;

        StoredMessageDebug() throw(El::Exception);
        
        StoredMessageDebug(const NewsGate::Message::StoredMessage& msg)
          throw(El::Exception);
        
        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);

        const NewsGate::Message::Id& get_id() const throw();

        void steal(StoredMessageDebug& src) throw(El::Exception);
      };

      class StoredMessageArray : public std::vector<StoredMessageDebug>
      {
      public:
        StoredMessageArray() throw(El::Exception);
        StoredMessageArray(size_t size) throw(El::Exception);
      };

      typedef std::auto_ptr<StoredMessageArray>
      StoredMessageArrayPtr;
      
      struct StoredMessagePackImpl
      {
        typedef El::Corba::Transport::EntityPack<
          StoredMessagePack,
          StoredMessage,
          StoredMessageArray,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::EntityPack_init<StoredMessage,
                                                      StoredMessageArray,
                                                      Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;

        typedef StoredMessageArray MessageArray;
      };

      struct StoredMessageImpl
      {
        typedef El::Corba::Transport::Entity<
          NewsGate::Message::Transport::StoredMessage,
          NewsGate::Message::Transport::StoredMessageDebug,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::Entity_init<
          NewsGate::Message::Transport::StoredMessageDebug, Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };

      struct IdImpl
      {
        typedef El::Corba::Transport::Entity<
          NewsGate::Message::Transport::Id,
          NewsGate::Message::Id,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::Entity_init<
          NewsGate::Message::Id, Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };

      //
      // MessageEventPack implementation
      //
    
      struct MessageEvent
      {
        NewsGate::Message::Id id;
        El::Luid event_id;
        uint32_t event_capacity;

        MessageEvent(const NewsGate::Message::Id& message_id_val,
                     const El::Luid& event_id_val,
                     uint32_t event_capacity_val) throw(El::Exception);
      
        MessageEvent() throw(El::Exception) : event_capacity(0) {}

        const NewsGate::Message::Id& get_id() const throw();
        
        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);        
      };

      typedef std::vector<MessageEvent> MessageEventArray;

      struct MessageEventPackImpl
      {
        typedef El::Corba::Transport::EntityPack<
          MessageEventPack,
          MessageEvent,
          MessageEventArray,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::EntityPack_init<MessageEventPack,
                                                      MessageEventArray,
                                                      Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;

        typedef MessageEventArray MessageArray;
      };

      //
      // MessageSharingInfoPack implementation
      //
    
      struct MessageSharingInfo
      {
        NewsGate::Message::Id message_id;
        El::Luid event_id;
        uint32_t event_capacity;
        uint64_t impressions;
        uint64_t clicks;
        uint64_t visited;
        Categories categories;
        uint64_t word_hash;

        MessageSharingInfo(const NewsGate::Message::Id& message_id_val,
                           const El::Luid& event_id_val,
                           uint32_t event_capacity_val,
                           uint64_t impressions_val,
                           uint64_t clicks_val,
                           uint64_t visited_val,
                           const Categories& categories_val,
                           uint64_t word_hash_val)
          throw(El::Exception);
      
        MessageSharingInfo() throw(El::Exception);
        
        const NewsGate::Message::Id& id() const throw() { return message_id; }
        
        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);        
      };

      typedef std::vector<MessageSharingInfo> MessageSharingInfoArray;

      struct MessageSharingInfoPackImpl
      {
        typedef El::Corba::Transport::EntityPack<
          MessageSharingInfoPack,
          MessageSharingInfo,
          MessageSharingInfoArray,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::EntityPack_init<MessageSharingInfoPack,
                                                      MessageSharingInfoArray,
                                                      Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };

      typedef std::vector<std::string> EndpointArray;
      
      struct ColoFrontend
      {
        enum Relation
        {
          RL_OWN,
          RL_MASTER,
          RL_MIRROR
        };

        Relation relation;
        std::string process_id;
        EndpointArray search_endpoints;
        EndpointArray limited_endpoints;

        ColoFrontend() throw() : relation(RL_OWN) {}

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);

        void clear() throw();
        bool empty() const throw() { return process_id.empty(); }

        static void append(std::vector<ColoFrontend>& array,
                           const ColoFrontend& cf)
          throw(El::Exception);
      };

      typedef std::vector<ColoFrontend> ColoFrontendArray;
      typedef std::auto_ptr<ColoFrontendArray> ColoFrontendArrayPtr;

      struct ColoFrontendPackImpl
      {
        typedef El::Corba::Transport::EntityPack<
          ColoFrontendPack,
          ColoFrontend,
          ColoFrontendArray,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::EntityPack_init<ColoFrontendPack,
                                                      ColoFrontendArray,
                                                      Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };

      struct EmptyStruct
      {
        void write(El::BinaryOutStream& bstr) const throw(El::Exception) {}
        void read(El::BinaryInStream& bstr) throw(El::Exception) {}
      };
      
      //
      // EmptyRequest implementation
      //

      struct EmptyRequestImpl
      {
        typedef El::Corba::Transport::Entity<
          NewsGate::Message::Transport::EmptyRequest,
          EmptyStruct,
          El::Corba::Transport::TE_IDENTITY> Type;
        
        typedef El::Corba::Transport::Entity_init<EmptyStruct, Type> Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };
      
      //
      // EmptyResponse implementation
      //

      struct EmptyResponseImpl
      {
        class EmptyResponseSemiImpl :
          public NewsGate::Message::Transport::EmptyResponse
        {
          //
          // IDL:NewsGate/Message/Transport/Response/absorb:1.0
          //
          virtual void absorb(Response* src)
            throw(Response::ImplementationException,
                  CORBA::SystemException) {}
        };
        
        typedef El::Corba::Transport::Entity<
          EmptyResponseSemiImpl,
          EmptyStruct,
          El::Corba::Transport::TE_IDENTITY> Type;

        typedef El::Corba::Transport::Entity_init<EmptyStruct, Type> Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };
      
      //
      // SetMessageFetchFilterRequest implementation
      //

      struct SetMessageFetchFilterRequestImpl
      {
        typedef El::Corba::Transport::Entity<
          NewsGate::Message::Transport::SetMessageFetchFilterRequest,
          Message::FetchFilter,
          El::Corba::Transport::TE_IDENTITY> Type;
        
        typedef El::Corba::Transport::Entity_init<Message::FetchFilter,
                                                  Type> Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };
      
      //
      // SetMessageCategorizerRequest implementation
      //

      struct SetMessageCategorizerRequestImpl
      {
        typedef El::Corba::Transport::Entity<
          NewsGate::Message::Transport::SetMessageCategorizerRequest,
          Message::Categorizer,
          El::Corba::Transport::TE_IDENTITY> Type;
        
        typedef El::Corba::Transport::Entity_init<Message::Categorizer,
                                                  Type> Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };
      
      //
      // MessageDigestRequest implementation
      //

      typedef std::vector<El::Luid> EventIdArray;

      struct MessageDigestRequestImpl
      {
        typedef El::Corba::Transport::EntityPack<
          MessageDigestRequest,
          El::Luid,
          EventIdArray,
          El::Corba::Transport::TE_IDENTITY> Type;
        
        typedef El::Corba::Transport::EntityPack_init<MessageDigestRequest,
                                                      EventIdArray,
                                                      Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };

      //
      // MessageDigestResponse implementation
      //
      struct MessageDigest
      {
        NewsGate::Message::Id message_id;
        El::Luid event_id;
        NewsGate::Message::CoreWords core_words;

        MessageDigest(const NewsGate::Message::Id& message_id_val,
                      const El::Luid& event_id_val,
                      const NewsGate::Message::CoreWords& core_words_val)
          throw(El::Exception);

        MessageDigest() throw(El::Exception);

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);        
      };
      
      typedef std::vector<MessageDigest> MessageDigestArray;
      
      struct MessageDigestResponseImpl
      {
        class MessageDigestResponseSemiImpl : public MessageDigestResponse
        {
          //
          // IDL:NewsGate/Message/Transport/Response/absorb:1.0
          //
          virtual void absorb(Response* src)
            throw(Response::ImplementationException,
                  CORBA::SystemException) {}
        };
        
        class Type :
          public El::Corba::Transport::EntityPack<
              MessageDigestResponseSemiImpl,
              MessageDigest,
              MessageDigestArray,
              El::Corba::Transport::TE_IDENTITY>
        {
        public:

          Type(MessageDigestArray* entities) throw(El::Exception);
          virtual ~Type() throw() {}
            
          //
          // IDL:NewsGate/Message/Transport/Response/absorb:1.0
          //
          virtual void absorb(Response* src)
            throw(Response::ImplementationException,
                  CORBA::SystemException);

          virtual CORBA::ValueBase* _copy_value() throw(CORBA::NO_IMPLEMENT);
          
        private:
          typedef ACE_Thread_Mutex Mutex;
          typedef ACE_Read_Guard<Mutex> ReadGuard;
          typedef ACE_Write_Guard<Mutex> WriteGuard;

          Mutex lock_;
        };

        typedef El::Corba::Transport::EntityPack_init<MessageDigestResponse,
                                                      MessageDigestArray,
                                                      Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;        
      };

      struct MessageStat
      {
        Message::Id id;
        uint32_t impressions;
        uint32_t clicks;
        uint64_t visited;

        MessageStat() throw() : impressions(0), clicks(0), visited(0) {}
        MessageStat(const Message::Id& id) throw();

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);          
      };

      typedef std::vector<MessageStat> MessageStatArray;

      struct EventStat
      {
        El::Luid id;
        uint64_t visited;
        
        EventStat() throw() : visited(0) {}
        EventStat(const El::Luid& id_val) throw() : id(id_val), visited(0) {}

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);          
      };

      typedef std::vector<EventStat> EventStatArray;

      struct MessageStatRequestInfo
      {
        MessageStatArray message_stat;
        EventStatArray event_stat;
        uint8_t require_feed_ids;

        MessageStatRequestInfo(uint8_t require_feed_ids_val = 0)
          throw(El::Exception);

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);
      };

      struct MessageStatRequestImpl
      {
        typedef El::Corba::Transport::Entity<
          MessageStatRequest,
          MessageStatRequestInfo,
          El::Corba::Transport::TE_IDENTITY> Type;
        
        typedef El::Corba::Transport::Entity_init<MessageStatRequestInfo,
                                                  Type> Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };
      
      struct MessageStatInfo
      {
        uint64_t count;
        NewsGate::Message::Id id;
        NewsGate::Message::SourceId source_id;

        MessageStatInfo() throw(El::Exception) : count(1), source_id(0) {}

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);          
      };
      
      typedef std::vector<MessageStatInfo> MessageStatInfoArray;
      
      struct MessageStatResponseImpl
      {
        class MessageStatResponseSemiImpl : public MessageStatResponse
        {
          //
          // IDL:NewsGate/Message/Transport/Response/absorb:1.0
          //
          virtual void absorb(Response* src)
            throw(Response::ImplementationException,
                  CORBA::SystemException) {}
        };
        
        class Type :
          public El::Corba::Transport::EntityPack<
              MessageStatResponseSemiImpl,
              MessageStatInfo,
              MessageStatInfoArray,
              El::Corba::Transport::TE_GZIP>
        {
        public:

          Type(MessageStatInfoArray* entities) throw(El::Exception);
          virtual ~Type() throw() {}
            
          //
          // IDL:NewsGate/Message/Transport/Response/absorb:1.0
          //
          virtual void absorb(Response* src)
            throw(Response::ImplementationException,
                  CORBA::SystemException);

          virtual CORBA::ValueBase* _copy_value() throw(CORBA::NO_IMPLEMENT);
          
        private:
          typedef ACE_Thread_Mutex Mutex;
          typedef ACE_Read_Guard<Mutex> ReadGuard;
          typedef ACE_Write_Guard<Mutex> WriteGuard;

          Mutex lock_;
        };

        typedef El::Corba::Transport::EntityPack_init<MessageStatResponse,
                                                      MessageStatInfoArray,
                                                      Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;        
      };

      //
      // CheckMirroredMessagesRequest implementation
      //

      struct CheckMirroredMessagesRequestImpl
      {
        typedef El::Corba::Transport::EntityPack<
          CheckMirroredMessagesRequest,
          Id,
          IdArray,
          El::Corba::Transport::TE_IDENTITY> Type;
        
        typedef El::Corba::Transport::EntityPack_init<
          CheckMirroredMessagesRequest,
          IdArray,
          Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };
      
      //
      // CheckMirroredMessagesResponse implementation
      //

      struct CheckMirroredMessagesResponseImpl
      {
        class CheckMirroredMessagesResponseSemiImpl :
          public CheckMirroredMessagesResponse
        {
          //
          // IDL:NewsGate/Message/Transport/Response/absorb:1.0
          //
          virtual void absorb(Response* src)
            throw(Response::ImplementationException,
                  CORBA::SystemException) {}
        };
        
        class Type : public El::Corba::Transport::EntityPack<
          CheckMirroredMessagesResponseSemiImpl,
          Id,
          IdArray,
          El::Corba::Transport::TE_IDENTITY>
        {
        public:
          
          Type(IdArray* ids) throw(El::Exception);
          virtual ~Type() throw() {}
            
          //
          // IDL:NewsGate/Message/Transport/Response/absorb:1.0
          //
          virtual void absorb(Response* src)
            throw(Response::ImplementationException,
                  CORBA::SystemException);

          virtual CORBA::ValueBase* _copy_value() throw(CORBA::NO_IMPLEMENT);
          
        private:
          typedef ACE_Thread_Mutex Mutex;
          typedef ACE_Read_Guard<Mutex> ReadGuard;
          typedef ACE_Write_Guard<Mutex> WriteGuard;

          Mutex lock_;
        };
        
        typedef El::Corba::Transport::EntityPack_init<
          CheckMirroredMessagesResponse,
          IdArray,
          Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };
      
      //
      // CategoryLocale implementation
      //

      struct CategoryLocaleImpl
      {
        typedef El::Corba::Transport::Entity<
          NewsGate::Message::Transport::CategoryLocale,
          Message::Categorizer::Category::Locale,
          El::Corba::Transport::TE_IDENTITY> Type;
        
        typedef El::Corba::Transport::Entity_init<
          Message::Categorizer::Category::Locale,
          Type> Init;

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
  namespace Message
  {
    namespace Transport
    {
      //
      // StoredMessageDebug struct
      //
      inline
      void
      MessageSharingSources::write(El::BinaryOutStream& bstr) const
        throw(El::Exception)
      {
        bstr << ids << mirrored_manager;
      }
      
      inline
      void
      MessageSharingSources::read(El::BinaryInStream& bstr)
        throw(El::Exception)
      {
        bstr >> ids >> mirrored_manager;
      }
      
      //
      // StoredMessageDebug struct
      //
      inline
      StoredMessageDebug::StoredMessageDebug() throw(El::Exception)
      {
      }
      
      inline
      StoredMessageDebug::StoredMessageDebug(
        const NewsGate::Message::StoredMessage& msg)
        throw(El::Exception)
          : message(msg)
      {
      }

      inline
      void
      StoredMessageDebug::write(El::BinaryOutStream& bstr) const
        throw(El::Exception)
      {
        bstr << message << debug_info;
      }
      
      inline
      void
      StoredMessageDebug::read(El::BinaryInStream& bstr)
        throw(El::Exception)
      {
        bstr >> message >> debug_info;
      }

      inline
      const NewsGate::Message::Id&
      StoredMessageDebug::get_id() const throw()
      {
        return message.id;
      }

      inline
      void
      StoredMessageDebug::steal(StoredMessageDebug& src) throw(El::Exception)
      {
        message.steal(src.message);
        debug_info.steal(src.debug_info);
      }
      
      //
      // DebugInfo struct
      //
      inline
      DebugInfo::DebugInfo() throw(El::Exception)
          : on(0),
            match_weight(0),
            feed_impressions(0),
            feed_clicks(0)
      {
      }
      
      inline
      void
      DebugInfo::write(El::BinaryOutStream& bstr) const throw(El::Exception)
      {
        bstr << on;

        if(on)
        {
          words_freq.write(bstr);
          
          bstr << match_weight << feed_impressions << feed_clicks;
        }
      }
      
      inline
      void
      DebugInfo::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        words_freq.clear();
        
        bstr >> on;
        
        if(on)
        {
          words_freq.read(bstr);
          bstr >> match_weight >> feed_impressions >> feed_clicks;
        }
      }
      
      inline
      void
      DebugInfo::steal(DebugInfo& src) throw(El::Exception)
      {
        on = src.on;
        words_freq.steal(src.words_freq);
        match_weight = src.match_weight;
        feed_impressions = src.feed_impressions;
        feed_clicks = src.feed_clicks;
      }
      
      //
      // StoredMessageArray struct
      //
      inline
      StoredMessageArray::StoredMessageArray()
        throw(El::Exception)
      {
      }
      
      inline
      StoredMessageArray::StoredMessageArray(size_t size)
        throw(El::Exception) : std::vector<StoredMessageDebug>(size)
      {
      }
      
      //
      // MessageEvent struct
      //

      inline
      MessageEvent::MessageEvent(const NewsGate::Message::Id& message_id_val,
                                 const El::Luid& event_id_val,
                                 uint32_t event_capacity_val)
        throw(El::Exception)
          : id(message_id_val),
            event_id(event_id_val),
            event_capacity(event_capacity_val)
      {
      }
      
      inline
      void
      MessageEvent::write(El::BinaryOutStream& bstr) const throw(El::Exception)
      {
        bstr << id << event_id << event_capacity;
      }
        
      inline
      void
      MessageEvent::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> id >> event_id >> event_capacity;
      }

      inline
      const NewsGate::Message::Id&
      MessageEvent::get_id() const throw()
      {
        return id;
      }
      
      //
      // MessageStatRequestInfo struct
      //
      
      inline
      MessageStatRequestInfo::MessageStatRequestInfo(
        uint8_t require_feed_ids_val) throw(El::Exception)
          : require_feed_ids(require_feed_ids_val)
      {
      }
      
      inline
      void
      MessageStatRequestInfo::write(El::BinaryOutStream& bstr) const
        throw(El::Exception)
      {
        bstr << (uint32_t)1 << require_feed_ids;
        bstr.write_array(message_stat);
        bstr.write_array(event_stat);
      }
      
      inline
      void
      MessageStatRequestInfo::read(El::BinaryInStream& bstr)
        throw(El::Exception)
      {
        uint32_t version = 0;
        bstr >> version >> require_feed_ids;
        bstr.read_array(message_stat);
        bstr.read_array(event_stat);
      }
      
      //
      // MessageSharingInfo struct
      //

      inline
      MessageSharingInfo::MessageSharingInfo(
        const NewsGate::Message::Id& message_id_val,
        const El::Luid& event_id_val,
        uint32_t event_capacity_val,
        uint64_t impressions_val,
        uint64_t clicks_val,
        uint64_t visited_val,
        const Categories& categories_val,
        uint64_t word_hash_val)
        throw(El::Exception)
          : message_id(message_id_val),
            event_id(event_id_val),
            event_capacity(event_capacity_val),
            impressions(impressions_val),
            clicks(clicks_val),
            visited(visited_val),
            categories(categories_val),
            word_hash(word_hash_val)
      {
      }                           

      inline
      MessageSharingInfo::MessageSharingInfo() throw(El::Exception)
          : event_capacity(0),
            impressions(0),
            clicks(0),
            visited(0),
            word_hash(0)
      {
      }
      
      inline
      void
      MessageSharingInfo::write(El::BinaryOutStream& bstr) const
        throw(El::Exception)
      {
        bstr << message_id << event_id << event_capacity << impressions
             << clicks << visited << categories << word_hash;
      }
        
      inline
      void
      MessageSharingInfo::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> message_id >> event_id >> event_capacity >> impressions
             >> clicks >> visited >> categories >> word_hash;
      }

      //
      // ColoFrontend struct
      //
      inline
      void
      ColoFrontend::write(El::BinaryOutStream& bstr) const throw(El::Exception)
      {
        bstr << (uint32_t)relation << process_id;
        bstr.write_array(search_endpoints);
        bstr.write_array(limited_endpoints);
      }
    
      inline
      void
      ColoFrontend::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        uint32_t rel = 0;
        bstr >> rel >> process_id;
        bstr.read_array(search_endpoints);
        bstr.read_array(limited_endpoints);
        
        relation = (Relation)rel;
      }

      inline
      void
      ColoFrontend::clear() throw()
      {
        relation = RL_OWN;
        process_id.clear();
        search_endpoints.clear();
        limited_endpoints.clear();
      }

      inline
      void
      ColoFrontend::append(std::vector<ColoFrontend>& array,
                           const ColoFrontend& cf)
        throw(El::Exception)
      {
        if(cf.empty())
        {
          return;
        }
        
        for(std::vector<ColoFrontend>::const_iterator i(array.begin()),
              e(array.end()); i != e; ++i)
        {
          if(i->process_id == cf.process_id)
          {
            return;
          }
        }

        array.push_back(cf);
      }

      //
      // MessageDigest struct
      //
      inline
      MessageDigest::MessageDigest() throw(El::Exception)
      {
      }

      inline
      MessageDigest::MessageDigest(
        const NewsGate::Message::Id& message_id_val,
        const El::Luid& event_id_val,
        const NewsGate::Message::CoreWords& core_words_val)
        throw(El::Exception)
          : message_id(message_id_val),
            event_id(event_id_val),
            core_words(core_words_val)
      {
      }
      
      inline
      void
      MessageDigest::write(El::BinaryOutStream& bstr) const
        throw(El::Exception)
      {
        bstr << message_id << event_id << core_words;
      }
      
      inline
      void
      MessageDigest::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> message_id >> event_id >> core_words;
      }

      //
      // MessageDigestResponseImpl class
      //

      inline
      MessageDigestResponseImpl::Type::Type(MessageDigestArray* digests)
        throw(El::Exception)
          : El::Corba::Transport::EntityPack<
              MessageDigestResponseSemiImpl,
              MessageDigest,
              MessageDigestArray,
              El::Corba::Transport::TE_IDENTITY>(digests)
      {
      }

      inline
      CORBA::ValueBase*
      MessageDigestResponseImpl::Type::_copy_value() throw(CORBA::NO_IMPLEMENT)
      {
        El::Corba::ValueVar<MessageDigestResponseImpl::Type> res(
          new MessageDigestResponseImpl::Type(0));

        if(serialized_)
        {
          res->packed_entities_ = new CORBA::OctetSeq();
          res->packed_entities_.inout() = packed_entities_.in();
        }
        else
        {
          res->entities_.reset(new MessageDigestArray());
          *res->entities_ = *entities_;
        }

        return res._retn();        
      }

      //
      // MessageStat struct
      //
      inline
      MessageStat::MessageStat(const Message::Id& id_val) throw()
          : id(id_val), impressions(0), clicks(0), visited(0)
      {
      }
      
      inline
      void
      MessageStat::write(El::BinaryOutStream& bstr) const throw(El::Exception)
      {
        bstr << id << impressions << clicks << visited;
      }

      inline
      void
      MessageStat::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> id >> impressions >> clicks >> visited;
      }
      
      //
      // EventStat struct
      //
      inline
      void
      EventStat::write(El::BinaryOutStream& bstr) const throw(El::Exception)
      {
        bstr << id << visited;
      }

      inline
      void
      EventStat::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> id >> visited;
      }
      
      //
      // MessageStatInfo struct
      //
      inline
      void
      MessageStatInfo::write(El::BinaryOutStream& bstr) const
        throw(El::Exception)
      {
        bstr << (uint32_t)1 << id << count << source_id;
      }

      inline
      void
      MessageStatInfo::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        uint32_t version = 0;
        bstr >> version >> id >> count >> source_id;
      }

      //
      // MessageStatResponseImpl class
      //
      inline
      MessageStatResponseImpl::Type::Type(MessageStatInfoArray* infos)
        throw(El::Exception)
          : El::Corba::Transport::EntityPack<
              MessageStatResponseSemiImpl,
              MessageStatInfo,
              MessageStatInfoArray,
              El::Corba::Transport::TE_GZIP>(infos)
      {
      }

      inline
      void
      MessageStatResponseImpl::Type::absorb(Response* src)
        throw(Response::ImplementationException,
              CORBA::SystemException)
      {
        Type* response = dynamic_cast<Type*>(src);

        if(response == 0)
        {
          Response::ImplementationException ex;
          ex.description = "MessageStatResponseImpl::Type::absorb: "
            "dynamic_cast<Type*> failed";
          
          throw ex;
        }
        
        WriteGuard guard(lock_);

        entities().insert(entities().begin(),
                          response->entities().begin(),
                          response->entities().end());
      }
          
      inline
      CORBA::ValueBase*
      MessageStatResponseImpl::Type::_copy_value() throw(CORBA::NO_IMPLEMENT)
      {
        El::Corba::ValueVar<MessageStatResponseImpl::Type> res(
          new MessageStatResponseImpl::Type(0));

        if(serialized_)
        {
          res->packed_entities_ = new CORBA::OctetSeq();
          res->packed_entities_.inout() = packed_entities_.in();
        }
        else
        {
          res->entities_.reset(new MessageStatInfoArray());
          *res->entities_ = *entities_;
        }

        return res._retn();        
      }

      //
      // CheckMirroredMessagesResponseImpl class
      //
      inline
      CheckMirroredMessagesResponseImpl::Type::Type(IdArray* ids)
        throw(El::Exception)
          : El::Corba::Transport::EntityPack<
              CheckMirroredMessagesResponseSemiImpl,
              Id,
              IdArray,
              El::Corba::Transport::TE_IDENTITY>(ids)
      {
      }

      inline
      void
      CheckMirroredMessagesResponseImpl::Type::absorb(Response* src)
        throw(Response::ImplementationException,
              CORBA::SystemException)
      {
        Type* response = dynamic_cast<Type*>(src);

        if(response == 0)
        {
          Response::ImplementationException ex;
          ex.description = "CheckMirroredMessagesResponseImpl::Type::absorb: "
            "dynamic_cast<Type*> failed";
          
          throw ex;
        }
        
        WriteGuard guard(lock_);

        entities().insert(entities().begin(),
                          response->entities().begin(),
                          response->entities().end());
      }
          
      inline
      CORBA::ValueBase*
      CheckMirroredMessagesResponseImpl::Type::_copy_value()
        throw(CORBA::NO_IMPLEMENT)
      {
        El::Corba::ValueVar<CheckMirroredMessagesResponseImpl::Type> res(
          new CheckMirroredMessagesResponseImpl::Type(0));

        if(serialized_)
        {
          res->packed_entities_ = new CORBA::OctetSeq();
          res->packed_entities_.inout() = packed_entities_.in();
        }
        else
        {
          res->entities_.reset(new IdArray());
          *res->entities_ = *entities_;
        }

        return res._retn();        
      }      
    }
  }
}

#endif // _NEWSGATE_SERVER_COMMONS_MESSAGE_TRANSPORTIMPL_HPP_
