/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Message/Bank/MessagePack.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_MESSAGEPACK_HPP_
#define _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_MESSAGEPACK_HPP_

#include <deque>

#include <El/Exception.hpp>
#include <El/BinaryStream.hpp>

#include <Commons/Message/TransportImpl.hpp>

namespace NewsGate
{
  namespace Message
  {
    struct PendingMessagePack
    {
      EL_EXCEPTION(Exception, El::ExceptionBase);

      enum Type
      {
        MPT_RAW,
        MPT_STORED
      };      
      
      Transport::MessagePack_var pack;
      PostMessageReason reason;

      PendingMessagePack(Transport::MessagePack* pack_val = 0,
                         PostMessageReason reason_val = PMR_NEW_MESSAGES)
        throw(El::Exception);

      void write(El::BinaryOutStream& bstr) const
        throw(Exception, El::Exception);
      
      void read(El::BinaryInStream& bstr)
        throw(Exception, El::Exception);
      
      size_t new_message_count() const throw();
      size_t shared_message_count() const throw();
      size_t pushed_message_count() const throw();
      size_t message_event_count() const throw();
      size_t message_count() const throw();
        
      static size_t message_count(Transport::MessagePack* messages) throw();

      static size_t new_message_count(Transport::MessagePack* messages,
                                      PostMessageReason reason)
        throw();

      static size_t shared_message_count(Transport::MessagePack* messages,
                                         PostMessageReason reason)
        throw();

      static size_t pushed_message_count(Transport::MessagePack* messages,
                                         PostMessageReason reason)
        throw();

      static size_t message_event_count(Transport::MessagePack* messages)
        throw();

      static bool is_message_event_pack(Transport::MessagePack* messages)
        throw();
        
      static bool is_new_message_pack(Transport::MessagePack* messages,
                                      PostMessageReason reason)
        throw();
        
      static bool is_shared_message_pack(Transport::MessagePack* messages,
                                         PostMessageReason reason)
        throw();        

      static bool is_pushed_message_pack(Transport::MessagePack* messages,
                                         PostMessageReason reason)
        throw();
    };

    struct PendingMessagePackQueue
    {
      PendingMessagePackQueue() throw();
        
      void push_back(const PendingMessagePack& pack) throw(El::Exception);
      void clear() throw();
      bool empty() const throw();        

      PendingMessagePack pop_front() throw(El::Exception);

      size_t pending_new_messages() const throw();
      size_t pending_shared_messages() const throw();
      size_t pending_pushed_messages() const throw();
      size_t pending_message_events() const throw();
      size_t pending_messages() const throw();
        
      size_t new_message_packs() const throw();
      size_t shared_message_packs() const throw();
      size_t pushed_message_packs() const throw();
      size_t message_event_packs() const throw();
      size_t message_packs() const throw();

    private:

      size_t pending_new_message_count_;
      size_t pending_shared_message_count_;
      size_t pending_pushed_message_count_;
      size_t pending_message_event_count_;
      size_t new_message_packs_;
      size_t shared_message_packs_;
      size_t pushed_message_packs_;
      size_t message_event_packs_;

      typedef std::deque<PendingMessagePack> Queue;
      Queue high_prio_queue_;
      Queue low_prio_queue_;
    };    
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Message
  {
    //
    // PendingMessagePack class
    //
    inline
    PendingMessagePack::PendingMessagePack(Transport::MessagePack* pack_val,
                                           PostMessageReason reason_val)
      throw(El::Exception)
        : pack(pack_val),
          reason(reason_val)
    {
      if(pack_val)
      {
        pack_val->_add_ref();
      }
    }

    inline
    void
    PendingMessagePack::write(El::BinaryOutStream& bstr) const
      throw(Exception, El::Exception)
    {
      const uint32_t version = 0;
      
      Transport::RawMessagePackImpl::Type* raw_msg_pack =
        dynamic_cast<Transport::RawMessagePackImpl::Type*>(pack.in());
        
      if(raw_msg_pack != 0)
      {
        bstr << version << (uint32_t)reason << (uint32_t)MPT_RAW;
        bstr.write_array(raw_msg_pack->entities());
        return;
      }
        
      Transport::StoredMessagePackImpl::Type* stored_msg_pack =
        dynamic_cast<Transport::StoredMessagePackImpl::Type*>(pack.in());
        
      if(stored_msg_pack != 0)
      {
        bstr << version << (uint32_t)reason << (uint32_t)MPT_STORED;
        bstr.write_array(stored_msg_pack->entities());
        return;
      }

      throw Exception("::NewsGate::Message::PendingMessagePack::write:"
                      "not implemented for that pack type");
    }

    inline
    void
    PendingMessagePack::read(El::BinaryInStream& bstr)
      throw(Exception, El::Exception)
    {
      uint32_t version = 0;
      uint32_t reason_val = 0;
      uint32_t type = 0;
            
      bstr >> version >> reason_val >> type;
            
      reason = (PostMessageReason)reason_val;
          
      switch(type)
      {
      case MPT_RAW:
        {
          Transport::RawMessagePackImpl::Var pack_impl =
            Transport::RawMessagePackImpl::Init::create(
              new RawMessageArray());
            
          bstr.read_array(pack_impl->entities());
          pack = pack_impl._retn();
          break;
        }
      case MPT_STORED:
        {
          Transport::StoredMessagePackImpl::Var pack_impl =
            Transport::StoredMessagePackImpl::Init::create(
              new Transport::StoredMessageArray());
              
          bstr.read_array(pack_impl->entities());
          pack = pack_impl._retn();
          break;
        }
      default:
        {
          throw Exception("::NewsGate::Message::PendingMessagePack::read:"
                          "not implemented for that pack type");
        }
      }
    }
    
    inline
    bool
    PendingMessagePack::is_message_event_pack(Transport::MessagePack* messages)
      throw()
    {
      return dynamic_cast<Transport::MessageEventPackImpl::Type*>(messages) !=
        0;
    }
        
    inline
    bool
    PendingMessagePack::is_new_message_pack(Transport::MessagePack* messages,
                                            PostMessageReason reason) throw()
    {
      return dynamic_cast<Transport::RawMessagePackImpl::Type*>(
        messages) != 0 ||
        (dynamic_cast<Transport::StoredMessagePackImpl::Type*>(messages) != 0 &&
         reason == PMR_NEW_MESSAGE_SHARING);
    }
        
    inline
    bool
    PendingMessagePack::is_shared_message_pack(
      Transport::MessagePack* messages,
      PostMessageReason reason) throw()
    {
      return dynamic_cast<Transport::StoredMessagePackImpl::Type*>(
        messages) != 0 && (reason == PMR_OLD_MESSAGE_SHARING ||
                           reason == PMR_SHARED_MESSAGE_SHARING ||
                           reason == PMR_LOST_MESSAGE_SHARING);
    }
    
    inline
    bool
    PendingMessagePack::is_pushed_message_pack(
      Transport::MessagePack* messages,
      PostMessageReason reason) throw()
    {
      return dynamic_cast<Transport::StoredMessagePackImpl::Type*>(
        messages) != 0 && reason == PMR_PUSH_OUT_FOREIGN_MESSAGES;
    }
    
    inline
    size_t
    PendingMessagePack::message_count(
      Transport::MessagePack* messages) throw()
    {
      Transport::RawMessagePackImpl::Type* raw_msg_pack =
        dynamic_cast<Transport::RawMessagePackImpl::Type*>(messages);
      
      if(raw_msg_pack != 0)
      {
        return raw_msg_pack->entities().size();
      }

      Transport::StoredMessagePackImpl::Type* stored_msg_pack =
        dynamic_cast<Transport::StoredMessagePackImpl::Type*>(messages);
      
      if(stored_msg_pack != 0)
      {
        return stored_msg_pack->entities().size();
      }

      return 0;
    }
    
    inline
    size_t
    PendingMessagePack::new_message_count(
      Transport::MessagePack* messages,
      PostMessageReason reason) throw()
    {
      Transport::RawMessagePackImpl::Type* raw_msg_pack =
        dynamic_cast<Transport::RawMessagePackImpl::Type*>(messages);
      
      if(raw_msg_pack != 0)
      {
        return raw_msg_pack->entities().size();
      }

      if(reason == PMR_NEW_MESSAGE_SHARING)
      {
        Transport::StoredMessagePackImpl::Type* stored_msg_pack =
          dynamic_cast<Transport::StoredMessagePackImpl::Type*>(messages);

        return stored_msg_pack->entities().size();
      }
      
      return 0;
    }
    
    inline
    size_t
    PendingMessagePack::shared_message_count(
      Transport::MessagePack* messages,
      PostMessageReason reason) throw()
    {
      Transport::StoredMessagePackImpl::Type* stored_msg_pack =
        dynamic_cast<Transport::StoredMessagePackImpl::Type*>(messages);
        
      if(stored_msg_pack != 0 && (reason == PMR_OLD_MESSAGE_SHARING ||
                                  reason == PMR_SHARED_MESSAGE_SHARING ||
                                  reason == PMR_LOST_MESSAGE_SHARING))
      {
        return stored_msg_pack->entities().size();
      }

      return 0;
    }
    
    inline
    size_t
    PendingMessagePack::pushed_message_count(
      Transport::MessagePack* messages,
      PostMessageReason reason) throw()
    {
      Transport::StoredMessagePackImpl::Type* stored_msg_pack =
        dynamic_cast<Transport::StoredMessagePackImpl::Type*>(messages);
        
      if(stored_msg_pack != 0 && reason == PMR_PUSH_OUT_FOREIGN_MESSAGES)
      {
        return stored_msg_pack->entities().size();
      }

      return 0;
    }
    
    inline
    size_t
    PendingMessagePack::message_event_count(
      Transport::MessagePack* messages) throw()
    {
      Transport::MessageEventPackImpl::Type* msg_event_pack =
        dynamic_cast<Transport::MessageEventPackImpl::Type*>(messages);
      
      if(msg_event_pack != 0)
      {
        return msg_event_pack->entities().size();
      }

      return 0;
    }
    
    inline
    size_t
    PendingMessagePack::new_message_count() const throw()
    {
      return new_message_count(pack.in(), reason);
    }
    
    inline
    size_t
    PendingMessagePack::shared_message_count() const throw()
    {
      return shared_message_count(pack.in(), reason);
    }
    
    inline
    size_t
    PendingMessagePack::pushed_message_count() const throw()
    {
      return pushed_message_count(pack.in(), reason);
    }
    
    inline
    size_t
    PendingMessagePack::message_event_count() const throw()
    {
      return message_event_count(pack.in());
    }
    
    inline
    size_t
    PendingMessagePack::message_count() const throw()
    {
      return message_count(pack.in());
    }
    
    //
    // PendingMessagePackQueue class
    //
    inline
    PendingMessagePackQueue::PendingMessagePackQueue()
      throw()
        : pending_new_message_count_(0),
          pending_shared_message_count_(0),
          pending_pushed_message_count_(0),
          pending_message_event_count_(0),
          new_message_packs_(0),
          shared_message_packs_(0),
          pushed_message_packs_(0),
          message_event_packs_(0)
    {
    }

    inline
    bool
    PendingMessagePackQueue::empty() const throw()
    {
      return high_prio_queue_.empty() && low_prio_queue_.empty();
    }
    
    inline
    void
    PendingMessagePackQueue::push_back(
      const PendingMessagePack& pack) throw(El::Exception)
    {
      size_t count = pack.message_event_count();
      
      if(count)
      {
        pending_message_event_count_ += count;
        ++message_event_packs_;
        low_prio_queue_.push_back(pack);
        return;
      }

      count = pack.new_message_count();

      if(count)
      {
        pending_new_message_count_ += count;
        ++new_message_packs_;
        low_prio_queue_.push_back(pack);
        return;
      }

      count = pack.shared_message_count();

      if(count)
      {
        pending_shared_message_count_ += count;
        ++shared_message_packs_;
        low_prio_queue_.push_back(pack);
      }

      count = pack.pushed_message_count();

      if(count)
      {
        pending_pushed_message_count_ += count;
        ++pushed_message_packs_;
        high_prio_queue_.push_back(pack);
      }
    }
    
    inline
    void
    PendingMessagePackQueue::clear() throw()
    {
      pending_new_message_count_ = 0;
      pending_shared_message_count_ = 0;
      pending_pushed_message_count_ = 0;
      pending_message_event_count_ = 0;
      new_message_packs_ = 0;
      shared_message_packs_ = 0;
      pushed_message_packs_ = 0;
      message_event_packs_ = 0;
      
      high_prio_queue_.clear();
      low_prio_queue_.clear();
    }
    
    inline
    PendingMessagePack
    PendingMessagePackQueue::pop_front() throw(El::Exception)
    {
      Queue& queue(high_prio_queue_.empty() ?
                   low_prio_queue_ : high_prio_queue_);
      
      PendingMessagePack pack = queue.front();
      queue.pop_front();
      
      size_t count = pack.message_event_count();
      
      if(count)
      { 
        assert(count <= pending_message_event_count_);
        pending_message_event_count_ -= count;

        assert(message_event_packs_ > 0);
        --message_event_packs_;
        
        return pack;
      }
      
      count = pack.new_message_count();

      if(count)
      {
        assert(count <= pending_new_message_count_);
        pending_new_message_count_ -= count;

        assert(new_message_packs_ > 0);
        --new_message_packs_;

        return pack;
      }
      
      count = pack.shared_message_count();

      if(count)
      {
        assert(count <= pending_shared_message_count_);
        pending_shared_message_count_ -= count;

        assert(shared_message_packs_ > 0);
        --shared_message_packs_;

        return pack;
      }

      count = pack.pushed_message_count();

      if(count)
      {
        assert(count <= pending_pushed_message_count_);
        pending_pushed_message_count_ -= count;

        assert(pushed_message_packs_ > 0);
        --pushed_message_packs_;

        return pack;
      }

      assert(false);
      return pack;
    }

    inline
    size_t
    PendingMessagePackQueue::pending_messages() const throw()
    {
      return pending_new_message_count_ + pending_pushed_message_count_ +
        pending_shared_message_count_;
    }
    
    inline
    size_t
    PendingMessagePackQueue::pending_new_messages() const throw()
    {
      return pending_new_message_count_;
    }
    
    inline
    size_t
    PendingMessagePackQueue::pending_pushed_messages() const throw()
    {
      return pending_pushed_message_count_;
    }
    
    inline
    size_t
    PendingMessagePackQueue::pending_shared_messages() const throw()
    {
      return pending_shared_message_count_;
    }
    
    inline
    size_t
    PendingMessagePackQueue::pending_message_events() const throw()
    {
      return pending_message_event_count_;
    }

    inline
    size_t
    PendingMessagePackQueue::message_packs() const throw()
    {
      return new_message_packs_ + shared_message_packs_ +
        pushed_message_packs_;
    }
    
    inline
    size_t
    PendingMessagePackQueue::new_message_packs() const throw()
    {
      return new_message_packs_;
    }
    
    inline
    size_t
    PendingMessagePackQueue::shared_message_packs() const throw()
    {
      return shared_message_packs_;
    }
    
    inline
    size_t
    PendingMessagePackQueue::pushed_message_packs() const throw()
    {
      return pushed_message_packs_;
    }
    
    inline
    size_t
    PendingMessagePackQueue::message_event_packs() const throw()
    {
      return message_event_packs_;
    }
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_MESSAGEPACK_HPP_
