/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Message/Bank/ManagingMessages.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_MANAGINGMESSAGES_HPP_
#define _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_MANAGINGMESSAGES_HPP_

#include <string>
#include <ext/hash_map>
#include <memory>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/Hash/Hash.hpp>
#include <El/Stat.hpp>

#include <Commons/Message/TransportImpl.hpp>

#include <Services/Commons/Message/MessageServices.hpp>
#include <Services/Dictionary/Commons/TransportImpl.hpp>
#include <Services/Segmentation/Commons/TransportImpl.hpp>

#include "SubService.hpp"
#include "MessageManager.hpp"
#include "MessagePack.hpp"

namespace NewsGate
{
  namespace Message
  {
    //
    // ManagingMessages class
    //
    class ManagingMessages : public virtual BankState,
                             public virtual MessageManagerCallback
    {
    public:        
      EL_EXCEPTION(Exception, BankState::Exception);

      ManagingMessages(Message::BankSession* session,
                       const ACE_Time_Value& report_presence_period,
                       BankStateCallback* callback)
        throw(Exception, El::Exception);

      virtual ~ManagingMessages() throw();

      void session(Message::BankSession* value)
        throw(Exception, El::Exception);
      
      virtual void post_messages(
        ::NewsGate::Message::Transport::MessagePack* messages,
        ::NewsGate::Message::PostMessageReason reason,
        const char* validation_id)
        throw(NewsGate::Message::NotReady,
              NewsGate::Message::ImplementationException,
              CORBA::SystemException);

      virtual void message_sharing_register(
        const char* manager_persistent_id,
        ::NewsGate::Message::BankClientSession* message_sink,
        CORBA::ULong flags,
        CORBA::ULong expiration_time)
        throw(NewsGate::Message::NotReady,
              NewsGate::Message::ImplementationException,
              CORBA::SystemException);

      virtual void message_sharing_unregister(
        const char* manager_persistent_id)
        throw(NewsGate::Message::ImplementationException,
              CORBA::SystemException);

      virtual void message_sharing_offer(
        ::NewsGate::Message::Transport::MessageSharingInfoPack*
          offered_messages,
        const char* validation_id,
        ::NewsGate::Message::Transport::IdPack_out requested_messages)
        throw(NewsGate::Message::ImplementationException,
              CORBA::SystemException);
      
      virtual void search(const ::NewsGate::Message::SearchRequest& request,
                          ::NewsGate::Message::MatchedMessages_out result)
        throw(NewsGate::Message::NotReady,
              NewsGate::Message::ImplementationException,
              ::CORBA::SystemException);
      
      virtual ::NewsGate::Message::Transport::StoredMessagePack* get_messages(
        ::NewsGate::Message::Transport::IdPack* message_ids,
        ::CORBA::ULongLong gm_flags,
        ::CORBA::Long img_index,
        ::CORBA::Long thumb_index,
        ::NewsGate::Message::Transport::IdPack_out notfound_message_ids)
        throw(NewsGate::Message::NotReady,
              NewsGate::Message::ImplementationException,
              ::CORBA::SystemException);
      
      virtual void send_request(
        ::NewsGate::Message::Transport::Request* req,
        ::NewsGate::Message::Transport::Response_out resp)
        throw(NewsGate::Message::NotReady,
              NewsGate::Message::InvalidData,
              NewsGate::Message::ImplementationException,
              CORBA::SystemException);      

      virtual MessageSinkMap message_sink_map(unsigned long flags)
        throw(El::Exception);
      
      virtual void dictionary_hash_changed() throw(El::Exception);
      
      virtual void wait() throw(Exception, El::Exception);
      virtual bool notify(El::Service::Event* event) throw(El::Exception);
      virtual bool start() throw(Exception, El::Exception);
      
      void flush_messages() throw(Exception, El::Exception);
      
    private:
      
      std::string process_event(El::Service::Event* event)
        throw(El::Exception);
      
      void report_presence() throw();
      
      typedef std::auto_ptr<FetchFilter> FetchFilterPtr;
      typedef std::auto_ptr<Categorizer> CategorizerPtr;

      void set_message_fetch_filter(FetchFilterPtr& filter,
                                    size_t retry) throw();
      
      void set_message_categorizer(CategorizerPtr& categorizer,
                                   size_t retry) throw();

      struct ExitApplication : public El::Service::CompoundServiceMessage
      {
        ExitApplication(ManagingMessages* state) throw(El::Exception);
      };
      
      struct ReportPresence : public El::Service::CompoundServiceMessage
      {
        ReportPresence(ManagingMessages* state) throw(El::Exception);

        virtual El::Service::ThreadPool::TaskQueue::EnqueueStrategy
        enqueue_strategy() throw();
      };
      
      struct AcceptMessages : public El::Service::CompoundServiceMessage
      {
        AcceptMessages(ManagingMessages* state) throw(El::Exception);
      };

      struct AcceptCachedMessages :
        public El::Service::CompoundServiceMessage
      {
        AcceptCachedMessages(ManagingMessages* state) throw(El::Exception);
        ~AcceptCachedMessages() throw() {}

        std::string pending_filename;
        PendingMessagePack pending_pack;
      };

      struct SetMessageFetchFilter :
        public El::Service::CompoundServiceMessage
      {
        FetchFilterPtr message_filter;
        size_t retry;
        
        SetMessageFetchFilter(ManagingMessages* state,
                              FetchFilter* filter,
                              size_t retry_val)
          throw(El::Exception);
        
        ~SetMessageFetchFilter() throw() {}
      };

      struct SetMessageCategorizer :
        public El::Service::CompoundServiceMessage
      {
        CategorizerPtr message_categorizer;
        size_t retry;
        
        SetMessageCategorizer(ManagingMessages* state,
                              Categorizer* categorizer,
                              size_t retry_val)
          throw(El::Exception);
        
        ~SetMessageCategorizer() throw() {}
      };

      void accept_messages(AcceptMessages* am) throw();

      void accept_cached_messages(AcceptCachedMessages* acm)
        throw(Exception, El::Exception);      

      void process_messages(Transport::RawMessagePackImpl::Type* pack,
                            ::NewsGate::Message::PostMessageReason reason)
        throw(Exception, El::Exception, CORBA::Exception);
      
      void process_messages(Transport::StoredMessagePackImpl::Type* pack,
                            ::NewsGate::Message::PostMessageReason reason)
        throw(Exception, El::Exception, CORBA::Exception);

      void process_messages(Transport::MessageEventPackImpl::Type* pack,
                            ::NewsGate::Message::PostMessageReason reason)
        throw(Exception, El::Exception, CORBA::Exception);

      MessageManager* message_manager() const throw(Exception, El::Exception);
      
      Segmentation::Transport::SegmentedMessagePackImpl::Type*
      segment_message_components(
        const Transport::RawMessagePackImpl::MessageArray& entities,
        bool& segmentation_enabled)
        throw(Exception, El::Exception);

      bool check_validation_id(bool exception,
                               const char* validation_id) const
        throw(El::Exception, NewsGate::Message::NotReady);
      
      bool check_accept_package_readiness(bool exception,
                                          Transport::MessagePack* messages,
                                          PostMessageReason reason)
        throw(El::Exception, NewsGate::Message::NotReady);
      
      bool check_message_manager_readiness(bool exception,
                                           Transport::MessagePack* messages)
        throw(El::Exception, NewsGate::Message::NotReady);
      
      bool check_message_pack_queue_readiness(bool exception,
                                              Transport::MessagePack* messages,
                                              PostMessageReason reason)
        throw(El::Exception, NewsGate::Message::NotReady);
      
      bool check_word_manager_readiness(bool exception,
                                        Transport::MessagePack* messages,
                                        PostMessageReason reason)
        throw(El::Exception, NewsGate::Message::NotReady);
      
      bool check_segmentor_readiness(bool exception,
                                     Transport::MessagePack* messages,
                                     PostMessageReason reason)
        throw(El::Exception, NewsGate::Message::NotReady);
      
      static void set_fetched_time(StoredMessageList& messages)
        throw(El::Exception);
      
    private:

      ACE_Time_Value report_presence_period_;
      Message::BankSession_var session_;
      BankClientSession_var bank_client_session_;
      MessageSinkMap message_sink_map_;
      bool word_manager_failed_;
      bool segmentor_failed_;
      unsigned long long search_counter_;
      PendingMessagePackQueue pending_message_packs_;
      std::auto_ptr<Message::Transport::SharingIdSet> sharing_ids_;
      bool has_event_bank_;
      ACE_Time_Value trace_search_duration_;

      El::Stat::TimeMeter search_meter_;
      El::Stat::TimeMeter get_messages_meter_;
    };
    
    typedef El::RefCount::SmartPtr<ManagingMessages> ManagingMessages_var;
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
    // ManagingMessages class
    //
    inline
    MessageManager*
    ManagingMessages::message_manager() const throw(Exception, El::Exception)
    {
      El::Service::Service_var st = state();
      MessageManager* message_manager = dynamic_cast<MessageManager*>(st.in());
      
      if(message_manager == 0)
      {
        throw Exception(
          "NewsGate::Message::ManagingMessages::message_manager: "
          "dynamic_cast<MessageManager*> failed");
      }
      
      return El::RefCount::add_ref(message_manager);
    }
 
    //
    // ManagingMessages::ExitApplication class
    //
    inline
    ManagingMessages::ExitApplication::ExitApplication(
      ManagingMessages* state) throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }

    //
    // ManagingMessages::ReportPresence class
    //
    inline
    ManagingMessages::ReportPresence::ReportPresence(
      ManagingMessages* state) throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }

    inline
    El::Service::ThreadPool::TaskQueue::EnqueueStrategy
    ManagingMessages::ReportPresence::enqueue_strategy() throw()
    {
      return El::Service::ThreadPool::TaskQueue::ES_FRONT;
    }

    //
    // ManagingMessages::AcceptCachedMessages class
    //
    inline
    ManagingMessages::AcceptCachedMessages::AcceptCachedMessages(
      ManagingMessages* state) throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }

    //
    // ManagingMessages::AcceptMessages class
    //
    inline
    ManagingMessages::AcceptMessages::AcceptMessages(
      ManagingMessages* state) throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }

    //
    // ManagingMessages::SetMessageFetchFilter class
    //
    inline
    ManagingMessages::SetMessageFetchFilter::SetMessageFetchFilter(
      ManagingMessages* state,
      FetchFilter* filter,
      size_t retry_val) throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state),
          message_filter(filter),
          retry(retry_val)
    {
    }

    //
    // ManagingMessages::SetMessageCategorizer class
    //
    inline
    ManagingMessages::SetMessageCategorizer::SetMessageCategorizer(
      ManagingMessages* state,
      Categorizer* categorizer,
      size_t retry_val) throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state),
          message_categorizer(categorizer),
          retry(retry_val)
    {
    }

  }
}

#endif // _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_MANAGINGMESSAGES_HPP_
