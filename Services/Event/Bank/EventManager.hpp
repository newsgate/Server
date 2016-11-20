/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Event/Bank/EventManager.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_EVENT_BANK_EVENTMANAGER_HPP_
#define _NEWSGATE_SERVER_SERVICES_EVENT_BANK_EVENTMANAGER_HPP_

#include <stdint.h>
#include <limits.h>

#include <list>
#include <vector>
#include <set>
#include <iostream>
#include <sstream>
#include <memory>
#include <utility>

#include <ext/hash_map>

#include <google/dense_hash_map>
#include <google/sparse_hash_map>
#include <google/sparse_hash_set>
#include <google/dense_hash_set>

#include <El/Exception.hpp>
#include <El/Luid.hpp>
#include <El/Hash/Hash.hpp>
#include <El/Hash/Map.hpp>
#include <El/BinaryStream.hpp>
#include <El/MySQL/DB.hpp>
#include <El/RefCount/All.hpp>
#include <El/Service/CompoundService.hpp>
#include <El/Stat.hpp>

#include <xsd/Config.hpp>

#include <Commons/Message/StoredMessage.hpp>
#include <Commons/Message/TransportImpl.hpp>
#include <Commons/Event/Event.hpp>

#include <Services/Commons/Message/MessageServices.hpp>

namespace NewsGate
{
  namespace Event
  {
    class EventManagerCallback : public virtual El::Service::Callback
    {
    };

    class EventManager :
      public El::Service::CompoundService<El::Service::Service,
                                          EventManagerCallback>
    {
    public:
      EL_EXCEPTION(Exception, El::ExceptionBase);

    public:

      struct EventCardinality
      {
        El::Lang lang;
        uint64_t cardinality;
        uint64_t published;
        El::Luid id;

        EventCardinality(const EventObject& event) throw();
        EventCardinality() throw() : cardinality(0), published(0) {}

        EventCardinality(El::Lang lang_val,
                         uint64_t cardinality_val,
                         uint64_t published_val,
                         const El::Luid& id_val) throw();

        bool operator<(const EventCardinality& val) const throw();

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);
      };

      typedef std::set<EventCardinality> EventCardinalitySet;

      struct EventCardinalityUpdated
      {
        uint64_t cardinality;
        uint64_t published;

        EventCardinalityUpdated(uint64_t cardinality_val = 0,
                                uint64_t published_val = 0) throw();
      };

      class EventCardinalityMap :
        public google::sparse_hash_map<El::Luid,
                                       EventCardinalityUpdated,
                                       El::Hash::Luid>
      {
      public:
        EventCardinalityMap() throw(El::Exception);
      };

      class EventCardinalities
      {
      public:

        size_t size() const throw();
        bool empty() const throw();
        bool contain(const El::Luid& id) const throw();
        
        void swap(EventCardinalities& val) throw();
        void clear() throw();
        EventCardinality pop() throw(El::Exception);
        void insert(const EventCardinality& cardinality) throw(El::Exception);
        void optimize_mem_usage() throw(El::Exception);

        void read(El::BinaryInStream& bstr) throw(El::Exception);
        void write(El::BinaryOutStream& bstr) const throw(El::Exception);

      private:

        EventCardinalitySet cardinalities_;
        EventCardinalityMap events_;
      };

      struct HashPair
      {
        static const HashPair zero;
        static const HashPair unexistent;

        El::Lang lang;

        HashPair() throw() : first_(0), second_(0) {}

        HashPair(uint32_t first_val,
                 uint32_t second_val,
                 El::Lang lang_val) throw();

        uint32_t first() const throw() { return first_; }
        uint32_t second() const throw() { return second_; }

        bool operator==(const HashPair& val) const throw();

        struct Hash
        {
          size_t operator()(const HashPair& val) const throw();
        };

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);
        
      private:
        uint32_t first_;
        uint32_t second_;
      };
      
      struct MergeDenialInfo
      {
        El::Luid event_id;
        uint64_t timeout;

        MergeDenialInfo() throw() : timeout(0) {}
        MergeDenialInfo(uint64_t t) throw() : timeout(t) {}
        
        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);        
      };
/*
      struct MergeRelOverlap
      {
        float value;
        uint64_t timeout;

        MergeRelOverlap() throw() : value(0), timeout(0) {}
        MergeRelOverlap(float v, uint64_t t) throw() : value(v), timeout(t) {}
      };      
        
      class MergeRelOverlapMap :
        public google::dense_hash_map<HashPair,
                                      MergeRelOverlap,
                                      HashPair::Hash>
      {
      public:
        MergeRelOverlapMap() throw(El::Exception);
      };
*/
      class MergeDenialMap :
        public google::dense_hash_map<HashPair,
                                      MergeDenialInfo,
                                      HashPair::Hash>
      {
      public:
        MergeDenialMap() throw(El::Exception);
      };

      class LangSet : public google::dense_hash_set<El::Lang, El::Hash::Lang>
      {
      public:
        LangSet() throw(El::Exception);
        std::string string() const throw(El::Exception);
      };      

      struct State
      {
        LangSet  languages;
        bool     loaded;
        uint64_t events;
        uint64_t messages;
        uint64_t changed_events;
        uint64_t merge_blacklist_size;
        uint64_t task_queue_size;

        State() throw(El::Exception);
        
        void dump(std::ostream& ostr) const throw(El::Exception);
      };
      
    public:
      
      EventManager(Event::BankSession* session,
                   EventManagerCallback* callback,
                   bool load,
                   bool all_events_changed,
                   const Server::Config::BankEventManagerType& config)
        throw(El::Exception);
      
      virtual ~EventManager() throw();

      void give(EventCardinalities& changed_events,
                MergeDenialMap& merge_blacklist)
        throw(El::Exception);
        
      void take(EventCardinalities& changed_events,
                MergeDenialMap& merge_blacklist)
        throw(El::Exception);

      bool insert(const Transport::MessageDigestArray& message_digests,
                  Message::Transport::MessageEventArray& message_events,
                  bool save_messages,
                  std::ostringstream* log_stream)
        throw(Exception, El::Exception);

      bool delete_messages(const Message::IdArray& ids,
                           std::ostringstream* log_stream)
        throw(Exception, El::Exception);
      
      bool get_message_events(
        const Message::IdArray& message_ids,
        Message::Transport::MessageEventArray& message_events,
        size_t& found_messages,
        size_t& total_message_count,
        std::ostringstream* log_stream)
        throw(Exception, El::Exception);
      
      void get_events(const Transport::EventIdRelArray& ids,
                      Transport::EventObjectRelArray& events)
        throw(Exception, El::Exception);

      void event_releations(EventNumber event_number,
                            const El::Luid& related_event,
                            Transport::EventRel& event_rel,
                            El::MySQL::Connection* connection)
        throw(El::Exception);

      void split_event(EventNumber event_number,
                       const Message::Id& split_id,
                       Transport::EventParts& parts,
                       El::MySQL::Connection* connection)
        throw(El::Exception);
      
      void separate_event(EventNumber event_number,
                          uint32_t separate,
                          bool narrow_separation,
                          const El::Luid& rel_event_id,
                          Transport::EventParts& parts,
                          El::MySQL::Connection* connection)
        throw(El::Exception);
      
      bool accept_events(
        const Transport::MessageDigestArray& digests,
        const Transport::EventPushInfoArray& events,
        std::ostringstream* log_stream)
        throw(Exception, El::Exception);
      
      virtual bool notify(El::Service::Event* event) throw(El::Exception);
      virtual void wait() throw(Exception, El::Exception);
      virtual bool start() throw(Exception, El::Exception);

      bool loaded() const throw();
      std::string lang_string() const throw(El::Exception);

      State state() const throw(El::Exception);

      void add_lang(const El::Lang& lang) throw(El::Exception);
      size_t event_count() const throw();
      
    private:

      struct MessageCoreWords
      {
        Message::CoreWords words;
        uint64_t timestamp;

        MessageCoreWords() throw(El::Exception) : timestamp(0) {}
      };
      
      struct MessageCoreWordsMap :
        public google::sparse_hash_map<Message::Id,
                                       MessageCoreWords*,
                                       Message::MessageIdHash>
      {
        MessageCoreWordsMap() throw(El::Exception);
        ~MessageCoreWordsMap() throw();
      };
      
      struct EventInfo
      {
        uint32_t capacity;
        El::Luid id;

        EventInfo(const El::Luid& id_val = El::Luid::null,
                  uint32_t capacity_val = 0)
          throw(El::Exception);

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);        
      };
      
      class MessageIdToEventInfoMap :
        public google::sparse_hash_map<Message::Id,
                                       EventInfo,
                                       Message::MessageIdHash>
      {
      public:
        MessageIdToEventInfoMap() throw(El::Exception);
      };

      typedef std::vector<const Transport::MessageDigest*>
      MessageDigestPtrArray;
      
      typedef __gnu_cxx::hash_map<El::Luid,
                                  MessageDigestPtrArray,
                                  El::Hash::Luid>
      EventMessageDigestMap;

      EventObject* insert(
        const Event::Transport::MessageDigest& message_digest,
        Message::Transport::MessageEvent& message_event,
        uint64_t expire_time,
        std::ostream* log_stream)
        throw(Exception, El::Exception);

      El::Luid get_event_id() throw(El::Exception);
      uint32_t get_event_number() throw(El::Exception);

      void traverse_events() throw();
      void remake_traverse_events() throw();
      void load_events() throw();
      void delete_obsolete_messages() throw();
      
      void delete_messages(const Message::IdArray& ids)
        throw(El::Exception);
      
      void merge_events() throw();

      void set_words(EventObject& event,
                     const EventWordWeightMap& event_words_pw)
        throw(Exception, El::Exception);

      void set_messages(EventObject& event,
                        MessageInfoArray& messages,
                        const EventWordWeightMap* event_words_pw = 0,
                        std::ostream* log_stream = 0,
                        bool verbose = false)
        throw(El::Exception);
      
      void get_message_word_weights(const EventObject& event,
                                    EventWordWeightMap& event_words_pw,
                                    std::ostream* log_stream,
                                    bool verbose)
        const throw(El::Exception);
      
      void unreference_words(const EventObject& event)
        throw(Exception, El::Exception);

      typedef std::list<std::string> StringList;
      
      void merge_events(const EventObject& src,
                        EventObject& dest,
                        El::MySQL::Connection* connection,
                        StringList& defered_queries,
                        std::ostream& log_stream,
                        bool verbose)
        throw(Exception, El::Exception);

      void assert_can_merge(const EventObject& e,
                            const char* point,
                            bool print) const throw();
      
      EventNumber find_best_overlap(
        EventNumber event_number,
        El::MySQL::Connection* connection,
        uint64_t now,
        size_t min_size,
        EventNumber& max_denied_overlap_event_number,
        uint32_t& merge_deny_timeout,
        std::ostringstream* log_stream)
        throw(Exception, El::Exception);

      EventNumber find_best_overlap(
        const EventObject& event,
        EventNumber event_number,
        El::MySQL::Connection* connection,
        uint64_t now,
        size_t min_size,
        EventNumber& max_denied_overlap_event_number,
        uint32_t& merge_deny_timeout,
        std::ostringstream* log_stream)
        throw(Exception, El::Exception);
      
      size_t event_merge_level(size_t event_size,
                               size_t strain,
                               uint64_t time_diff,
                               uint64_t time_range) const throw();
      
      bool set_merge(EventObject& event) const throw();
      
      void post_message_event_updates(
        MessageIdToEventInfoMap* message_event_updates)
        throw(Exception, El::Exception);
      
      void cleanup_obsolete_events(const EventNumberSet& obsolete_events,
                                   time_t expire_time,
                                   bool delete_messages,
                                   El::MySQL::Connection* connection,
                                   StringList& defered_queries,
                                   std::ostringstream* log_stream,
                                   bool verbose)
        throw(El::Exception);

      typedef std::auto_ptr<std::ostringstream> SStreamPtr;
      
      void cleanup_event(EventObject& event,
                         EventNumber number,
                         time_t expire_time,
                         El::MySQL::Connection* connection,
                         SStreamPtr& remove_events_query,
                         SStreamPtr* delete_msg_query,
                         std::ostringstream* log_stream,
                         bool verbose)
        throw(El::Exception);

      void erase_message_core_words(const Message::Id& id)
        throw(El::Exception);
      
      void load_message_core_words(const EventNumberSet& events,
                                   El::MySQL::Connection* connection)
        throw(El::Exception);

      void load_message_core_words(EventNumber event_number,
                                   El::MySQL::Connection* connection)
        throw(El::Exception);

      void load_message_core_words(SStreamPtr& ostr,
                                   time_t current_time,
                                   El::MySQL::Connection* connection)
        throw(El::Exception);

      void flush_dirty_events(const EventNumberSet& events_to_flush,
                              bool lock,
                              std::ostringstream* log_stream)
        throw(El::Exception);
      
      void push_events_to_neigbours(const EventNumberSet& events_to_push,
                                    El::MySQL::Connection* connection,
                                    std::ostringstream* log_stream)
        throw(El::Exception);
      
      bool push_events(Transport::MessageDigestPackImpl::Type* message_digest,
                       Transport::EventPushInfoPackImpl::Type* event_pack,
                       Event::Bank_ptr bank,
                       El::MySQL::Connection* connection,
                       std::string& error_desc)
        throw(Exception, El::Exception);
      
      size_t revise_events(const EventNumberSet& events_to_revise,
                           time_t now,
                           time_t expire_time,
                           El::MySQL::Connection* connection,
                           std::ostringstream* log_stream,
                           bool verbose)
        throw(El::Exception);

      void execute_queries(El::MySQL::Connection* connection,
                           const StringList& queries,
                           const char* caller)
        throw(El::Exception);

      bool detach_message_candidate(const EventObject& event,
                                    El::MySQL::Connection* connection = 0,
                                    time_t expire_time = 0,
                                    size_t* detach_message_index = 0,
                                    uint32_t* dissenters = 0)
        throw(El::Exception);
      
      void optimize_mem_usage(std::ostringstream* log_stream)
        throw(El::Exception);
      
      bool revise_event(
        EventNumber event_number,
        time_t expire_time,
        EventNumberSet& revised_events,
        Transport::MessageDigestArray& detached_message_digests)
        throw(El::Exception);

      void write_msg_line(const Event::Transport::MessageDigest& msg,
                          std::ostream& stream,
                          bool first_line)
      throw(Exception, El::Exception);

      void write_messages_to_db(const char* cache_filename)
        throw(Exception, El::Exception);      
      
      void write_event_line(const EventObject& event,
                            std::ostream& stream,
                            bool first_line)
        throw(Exception, El::Exception);
      
      void write_events_to_db(const char* cache_filename)
        throw(Exception, El::Exception);

      uint64_t merge_deny_timeout(const EventObject& e1, const EventObject& e2)
        throw(El::Exception);

      void schedule_merge(time_t delay) throw(El::Exception);
      
      void do_merge_events(El::MySQL::Connection* connection,
                           StringList& defered_queries)
        throw(El::Exception);

      size_t words_overlap(const EventObject& event,
                           Message::Id msg_id,
                           const EventWordWeightMap* ewwm = 0,
                           size_t* common_word_count = 0) const
        throw(El::Exception);

      bool split_event(EventNumber event_number,
                       time_t expire_time,
                       EventNumberSet& changed_events,
                       std::ostringstream* log_stream)
        throw(El::Exception);

      bool separate_event(EventNumber event_number,
                          time_t expire_time,
                          EventNumberSet& changed_events,
                          std::ostringstream* log_stream)
        throw(El::Exception);

      bool remake_event(EventNumber event_number,
                        time_t now,
                        size_t remake_min_size,
                         El::MySQL::Connection* connection,
                        std::ostringstream* log_stream,
                        const char* ident)
        throw(El::Exception);

      typedef std::vector<MessageInfo> MessageInfoVector;
      
      EventObject*
      find_remake_candidate(uint32_t word_id,
                            bool is_event_word,
                            EventNumber event_number,
                            const EventObject& event1,
                            const MessageInfoVector& msgs1,
                            const EventObject& event2,
                            const MessageInfoVector& msgs2,
                            float rel_overlap,
                            time_t now,
                            float& max_improve_ln,
                            unsigned long& candidate_improve,
                            unsigned long& candidate_size,
                            uint32_t& remake_word_id,
                            float& remake_rel_overlap,
                            float& remake_rel_candidate_overlap,
                            MessageInfoVector& new_event_messages,
                            MessageInfoVector& old_event_messages,
                            El::MySQL::Connection* connection,
                            std::ostringstream* log_stream)
        throw(El::Exception);

      EventObject* detach_event(
        EventObject& event,
        MessageInfoVector::const_iterator new_event_msg_begin,
        MessageInfoVector::const_iterator new_event_msg_end,
        MessageInfoVector::const_iterator old_event_msg_begin,
        MessageInfoVector::const_iterator old_event_msg_end,
        EventNumberSet& changed_events)
        throw(El::Exception);
      
      void create_event(EventObject& event,
                        MessageInfoVector::const_iterator from,
                        MessageInfoVector::const_iterator to,
                        const El::Lang& lang)
        throw(El::Exception);

      void create_event(EventObject& event,
                        MessageInfoArray& messages,
                        const El::Lang& lang)
        throw(El::Exception);
      
    private:

      class EventNumberToEventMap :
        public google::dense_hash_map<EventNumber,
                                      EventObject*,
                                      El::Hash::Numeric<EventNumber> >
      {
      public:
        EventNumberToEventMap() throw(El::Exception)
        {
          set_deleted_key(NUMBER_ZERO);
          set_empty_key(NUMBER_UNEXISTENT);
        }
        
        ~EventNumberToEventMap() throw();
      };
      
      class EventIdToEventNumberMap :
        public google::sparse_hash_map<El::Luid, EventNumber, El::Hash::Luid>
      {
      public:
        EventIdToEventNumberMap() throw(El::Exception);
      };

      struct TraverseEvents :
        public El::Service::CompoundServiceMessage
      {
        TraverseEvents(EventManager* state) throw(El::Exception);
      };      
      
      struct RemakeTraverseEvents :
        public El::Service::CompoundServiceMessage
      {
        RemakeTraverseEvents(EventManager* state) throw(El::Exception);
      };      
      
      struct MergeEvents : public El::Service::CompoundServiceMessage
      {
        MergeEvents(EventManager* state) throw(El::Exception);
      };      
      
      struct LoadEvents :
        public El::Service::CompoundServiceMessage
      {
        LoadEvents(EventManager* state) throw(El::Exception);        
      };

      struct DeleteObsoleteMessages :
        public El::Service::CompoundServiceMessage
      {
        DeleteObsoleteMessages(EventManager* state) throw(El::Exception);
      };

      struct DeleteMessages : public El::Service::CompoundServiceMessage
      {
        DeleteMessages(EventManager* state,
                       const Message::IdArray& ids_val) throw(El::Exception);

        virtual ~DeleteMessages() throw() { }

        Message::IdArray ids;
      };

      class MessageIdToEventNumberMap :
        public google::sparse_hash_map<Message::Id,
                                       EventNumber,
                                       Message::MessageIdHash>
      {
      public:
        MessageIdToEventNumberMap() throw(El::Exception);
      };

      class WordIdSet :
        public google::sparse_hash_set<uint32_t,
                                       El::Hash::Numeric<uint32_t> >
      {
      public:
        WordIdSet() throw(El::Exception) { set_deleted_key(0); }
      };

      typedef ACE_Thread_Mutex DBMutex;
      typedef ACE_Guard<DBMutex> DBGuard;

      static DBMutex event_buff_lock_;
      static DBMutex message_buff_lock_;

      typedef ACE_Thread_Mutex StatusMutex;
      typedef ACE_Guard<StatusMutex> StatusGuard;

      mutable StatusMutex status_lock_;

      const Server::Config::BankEventManagerType& config_;
      size_t max_core_words_;
      size_t max_message_core_words_;
      size_t max_size_;
      size_t max_time_range_;
      size_t merge_level_base_;
      size_t merge_level_min_;
      float merge_level_size_based_decrement_step_;
      size_t merge_max_strain_;
      uint64_t merge_max_time_diff_;
      uint64_t min_rift_time_;
      uint64_t merge_deny_size_factor_;
      uint64_t merge_deny_max_time_;
      float merge_level_time_based_increment_step_;
      float merge_level_range_based_increment_step_;
      float merge_level_strain_based_increment_step_;
      size_t remake_min_improve_;
      size_t remake_min_size_;
      size_t remake_min_size_revise_;
      size_t remake_min_part_;
      
      Event::BankSession_var session_;
       
      bool events_loaded_;
      bool events_unloaded_;
      bool all_events_changed_;
      time_t message_time_lower_boundary_;
      time_t msg_core_words_next_preemt_;
      uint64_t merge_blacklist_cleanup_time_;

      LangSet langs_;
      WordToEventNumberMap word_map_;
      EventNumberToEventMap events_;
      EventIdToEventNumberMap id_to_number_map_;
      MessageIdToEventNumberMap message_events_;
      EventCardinalities changed_events_;      
      MergeDenialMap merge_blacklist_;
      MessageCoreWordsMap message_core_words_;
      
      EventNumber last_event_number_;
      ACE_Time_Value next_revision_time_;

      Message::BankClientSession_var bank_client_session_;

      typedef std::auto_ptr<MessageIdToEventInfoMap>
      MessageIdToEventInfoMapPtr;

      MessageIdToEventInfoMapPtr message_event_updates_;
      ACE_Time_Value next_message_event_update_time_;

      typedef std::vector<EventNumber> EventNumberArray;
      
      EventNumberArray traverse_event_;
      EventNumberArray::const_iterator traverse_event_it_;

      EventNumberArray remake_traverse_event_;
      EventNumberArray::const_iterator remake_traverse_event_it_;

      size_t changed_events_counters_[20];
      size_t changed_events_sum_;
      size_t changed_events_avg_;
      size_t traverse_period_;
      size_t dissenters_;

      El::Luid event_load_last_id_;
      Message::Id msg_load_last_id_;
//      bool cleanup_allowed_;
//      bool traverse_period_increased_;
/*
      El::Stat::TimeMeter fbo_meter1;
      El::Stat::TimeMeter fbo_meter11;
      El::Stat::TimeMeter fbo_meter12;
      El::Stat::TimeMeter fbo_meter13;
      El::Stat::TimeMeter fbo_meter14;
      El::Stat::TimeMeter fbo_meter15;
      El::Stat::TimeMeter fbo_meter16;
      El::Stat::TimeMeter fbo_meter17;
      El::Stat::TimeMeter fbo_meter2;
      El::Stat::TimeMeter fbo_meter3;
*/
    };
    
    typedef El::RefCount::SmartPtr<EventManager> EventManager_var;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Event
  {
    //
    // NewsGate::Event::EventManager::State class
    //
    inline
    EventManager::State::State() throw(El::Exception)
        : loaded(false),
          events(0),
          messages(0),
          changed_events(0),
          merge_blacklist_size(0),
          task_queue_size(0)
    {
    }

    inline
    void
    EventManager::State::dump(std::ostream& ostr) const throw(El::Exception)
    {
      ostr << languages.string() 
           << ":\n  events: " << events << " / " << changed_events
           << "\n  messages: " << messages
           << "\n  merge blacklist size: " << merge_blacklist_size
           << "\n  loaded: " << (loaded ? "yes" : "no")
           << "\n  tasks: " << task_queue_size;
    }
    
    //
    // NewsGate::Event::EventManager class
    //
    inline
    bool
    EventManager::loaded() const throw()
    {
      StatusGuard guard(status_lock_);
      return events_loaded_;
    }

    inline
    size_t
    EventManager::event_merge_level(size_t event_size,
                                    size_t strain,
                                    uint64_t time_diff,
                                    uint64_t time_range) const throw()
    {
      return EventObject::event_merge_level(
        event_size,
        strain,
        time_diff,
        time_range,
        merge_level_base_,
        merge_level_min_,
        min_rift_time_,
        merge_level_size_based_decrement_step_,
        merge_level_time_based_increment_step_,
        merge_level_range_based_increment_step_,
        merge_level_strain_based_increment_step_);
    }

    inline
    bool
    EventManager::set_merge(EventObject& event) const throw()
    {
      if(event.strain() > merge_max_strain_)
      {
        event.flags |= EventObject::EF_DISSENTERS_CLEANUP;
      }
        
      bool res =
        event.can_merge(merge_max_strain_, max_time_range_, max_size_);

      if(res)
      {
        event.flags |= EventObject::EF_CAN_MERGE;
      }
      else
      {
        event.flags &= ~EventObject::EF_CAN_MERGE;
      }

      return res;
    }

    inline
    void
    EventManager::create_event(EventObject& event,
                               MessageInfoVector::const_iterator from,
                               MessageInfoVector::const_iterator to,
                               const El::Lang& lang)
      throw(El::Exception)
    {
      MessageInfoArray messages;
      messages.init<MessageInfoVector::const_iterator>(from, to);

      create_event(event, messages, lang);
    }
    
    inline
    void
    EventManager::create_event(EventObject& event,
                               MessageInfoArray& messages,
                               const El::Lang& lang)
      throw(El::Exception)
    {
      event.lang = lang;
      event.messages(messages);

      EventWordWeightMap event_words_pw;
        
      get_message_word_weights(event,
                               event_words_pw,
                               0,
                               false);
          
      event.set_words(event_words_pw, max_core_words_, true);

      uint32_t dis_val = 0;
      detach_message_candidate(event, 0, 0, 0, &dis_val);
      event.dissenters(dis_val);
    }
    
    //
    // NewsGate::Event::EventManager::LoadEvents class
    //
    inline
    EventManager::LoadEvents::LoadEvents(EventManager* state)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {  
    }
    
    //
    // NewsGate::Event::EventManager::DeleteObsoleteMessages class
    //
    inline
    EventManager::DeleteObsoleteMessages::DeleteObsoleteMessages(
      EventManager* state)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {  
    }

    //
    // NewsGate::Event::EventManager::DeleteMessages class
    //
    inline
    EventManager::DeleteMessages::DeleteMessages(
      EventManager* state,
      const Message::IdArray& ids_val)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, true),
          El::Service::CompoundServiceMessage(state, state),
          ids(ids_val)
    {
    }

    //
    // LangSet class
    //
    inline
    EventManager::LangSet::LangSet() throw(El::Exception)
    {
      set_empty_key(El::Lang::nonexistent);
      set_deleted_key(El::Lang::nonexistent2);
    }

    inline
    std::string
    EventManager::LangSet::string() const throw(El::Exception)
    {
      std::ostringstream ostr;
      
      for(LangSet::const_iterator it = begin(); it != end(); ++it)
      {
        if(it != begin())
        {
          ostr << " ";
        }
        
        ostr << it->l3_code();
      }
      
      return ostr.str();
    }
    
    //
    // EventCardinalityMap class
    //
    inline
    EventManager::MessageCoreWordsMap::MessageCoreWordsMap()
      throw(El::Exception)
    {
      set_deleted_key(Message::Id::zero);
    }
    
    inline
    EventManager::MessageCoreWordsMap::~MessageCoreWordsMap() throw()
    {
      for(iterator it = begin(); it != end(); it++)
      {
        delete it->second;
      }
    }
    
    //
    // EventCardinalityMap class
    //
    inline
    EventManager::EventCardinalityMap::EventCardinalityMap()
      throw(El::Exception)
    {
      set_deleted_key(El::Luid::null);
    }

    //
    // EventCardinalityUpdated class
    //
    inline
    EventManager::EventCardinalityUpdated::EventCardinalityUpdated(
      uint64_t cardinality_val,
      uint64_t published_val) throw()
        : cardinality(cardinality_val),
          published(published_val)
    {
    }

    //
    // EventCardinalityMap class
    //

    inline
    void
    EventManager::EventCardinalities::swap(EventCardinalities& val) throw()
    {
      cardinalities_.swap(val.cardinalities_);
      events_.swap(val.events_);
    }
    
    inline
    void
    EventManager::EventCardinalities::clear() throw()
    {
      cardinalities_.clear();
      events_.clear();
    }

    inline
    size_t
    EventManager::EventCardinalities::size() const throw()
    {
      return cardinalities_.size();
    }
    
    inline
    bool
    EventManager::EventCardinalities::empty() const throw()
    {
      return cardinalities_.empty();
    }
    
    inline
    bool
    EventManager::EventCardinalities::contain(const El::Luid& id) const throw()
    {
      return events_.find(id) != events_.end();
    }
      
    inline
    EventManager::EventCardinality
    EventManager::EventCardinalities::pop() throw(El::Exception)
    {
      assert(!cardinalities_.empty());

      EventCardinalitySet::iterator it = cardinalities_.begin();

      EventCardinality res = *it;
      events_.erase(it->id);
      cardinalities_.erase(it);

      return res;
    }

    inline
    void
    EventManager::EventCardinalities::insert(
      const EventCardinality& cardinality) throw(El::Exception)
    {
      EventCardinalityMap::iterator it = events_.find(cardinality.id);

      if(it != events_.end())
      {
        cardinalities_.erase(EventCardinality(cardinality.lang,
                                              it->second.cardinality,
                                              it->second.published,
                                              it->first));
        events_.erase(it);
      }

      cardinalities_.insert(cardinality);
      
      events_[cardinality.id] =
        EventCardinalityUpdated(cardinality.cardinality,
                                cardinality.published);
    }
      
    inline
    void
    EventManager::EventCardinalities::optimize_mem_usage() throw(El::Exception)
    {
      events_.resize(0);
    }
    
    inline
    void
    EventManager::EventCardinalities::read(El::BinaryInStream& bstr)
      throw(El::Exception)
    {
      clear();
      
      bstr.read_set(cardinalities_);
          
      for(EventCardinalitySet::const_iterator it =
            cardinalities_.begin(); it != cardinalities_.end(); ++it)
      {
        events_.insert(std::make_pair(it->id, it->cardinality));
      }
    }

    inline
    void
    EventManager::EventCardinalities::write(El::BinaryOutStream& bstr)
      const throw(El::Exception)
    {
      bstr.write_set(cardinalities_);
    }
    
    //
    // EventManager::EventCardinality struct
    //
    inline
    EventManager::EventCardinality::EventCardinality(const EventObject& event)
      throw()
        : lang(event.lang),
          cardinality(event.cardinality()),
          published(event.published_max),
          id(event.id)
    {
    }

    inline
    EventManager::EventCardinality::EventCardinality(
      El::Lang lang_val,
      uint64_t cardinality_val,
      uint64_t published_val,
      const El::Luid& id_val) throw()
        : lang(lang_val),
          cardinality(cardinality_val),
          published(published_val),
          id(id_val)
    {
    }

    inline
    bool
    EventManager::EventCardinality::operator<(const EventCardinality& val)
      const throw()
    {
      if(cardinality != val.cardinality)
      {
        return 
//          cardinality > val.cardinality;
          cardinality < val.cardinality;
      }

      if(published != val.published)
      {
        return published > val.published;
      }
      
      return id < val.id;
    }
      
    inline
    void
    EventManager::EventCardinality::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr << id << cardinality << published << lang;
    }
      
    inline
    void
    EventManager::EventCardinality::read(El::BinaryInStream& bstr)
      throw(El::Exception)
    {
      bstr >> id >> cardinality >> published >> lang;
    }
    
    //
    // EventManager::EventIdToEventNumberMap struct
    //
    inline
    EventManager::EventIdToEventNumberMap::EventIdToEventNumberMap()
      throw(El::Exception)
    {
      set_deleted_key(El::Luid::null);
    }

    //
    // EventManager::EventNumberToEventMap struct
    //
    inline
    EventManager::EventNumberToEventMap::~EventNumberToEventMap() throw()
    {
      for(iterator it = begin(); it != end(); ++it)
      {
        delete it->second;
      }
    }
/*    
    //
    // EventManager::MergeRelOverlapMap struct
    //  
    inline
    EventManager::MergeRelOverlapMap::MergeRelOverlapMap() throw(El::Exception)
    {
      set_empty_key(HashPair::unexistent);
      set_deleted_key(HashPair::zero);
    }
*/
    //
    // EventManager::MergeDenialMap struct
    //  
    inline
    EventManager::MergeDenialMap::MergeDenialMap() throw(El::Exception)
    {
      set_empty_key(HashPair::unexistent);
      set_deleted_key(HashPair::zero);
    }
    
    //
    // EventManager::EventInfo struct
    //
  
    inline
    EventManager::EventInfo::EventInfo(const El::Luid& id_val,
                                       uint32_t capacity_val)
      throw(El::Exception)
        : capacity(capacity_val),
          id(id_val)
    {
    }
    
    inline
    void 
    EventManager::EventInfo::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr << id << capacity;
    }
    
    inline
    void 
    EventManager::EventInfo::read(El::BinaryInStream& bstr)
      throw(El::Exception)
    {
      bstr >> id >> capacity;
    }
    
    //
    // EventManager::MessageIdToEventInfoMap struct
    //
  
    inline
    EventManager::MessageIdToEventInfoMap::MessageIdToEventInfoMap()
      throw(El::Exception)
    {
      set_deleted_key(Message::Id::zero);
    }
    
    //
    // EventManager::MessageIdToEventNumberMap struct
    //
  
    inline
    EventManager::MessageIdToEventNumberMap::MessageIdToEventNumberMap()
      throw(El::Exception)
    {
      set_deleted_key(Message::Id::zero);
    }
    
    //
    // NewsGate::EventManager::TraverseEvents class
    //
    inline
    EventManager::TraverseEvents::TraverseEvents(EventManager* state)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }
    
    //
    // NewsGate::EventManager::RemakeTraverseEvents class
    //
    inline
    EventManager::RemakeTraverseEvents::RemakeTraverseEvents(
      EventManager* state)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }
    
    //
    // NewsGate::EventManager::MergeEvents class
    //
    inline
    EventManager::MergeEvents::MergeEvents(EventManager* state)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }

    //
    // EventManager::MergeDenialInfo struct
    //
    inline
    void
    EventManager::MergeDenialInfo::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr << event_id << timeout;
    }
      
    inline
    void
    EventManager::MergeDenialInfo::read(El::BinaryInStream& bstr)
      throw(El::Exception)
    {
      bstr >> event_id >> timeout;
    }
    
    //
    // EventManager::HashPair struct
    //
    inline  
    EventManager::HashPair::HashPair(uint32_t first_val,
                                     uint32_t second_val,
                                     El::Lang lang_val) throw()
        : lang(lang_val),
          first_(std::min(first_val, second_val)),
          second_(std::max(first_val, second_val))
    {
    }

    inline  
    bool
    EventManager::HashPair::operator==(const HashPair& val) const throw()
    {
      return first_ == val.first_ && second_ == val.second_ &&
        lang == val.lang;
    }
        
    inline
    void
    EventManager::HashPair::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr << first_ << second_ << lang;
    }
      
    inline
    void
    EventManager::HashPair::read(El::BinaryInStream& bstr)
      throw(El::Exception)
    {
      bstr >> first_ >> second_ >> lang;
    }
    
//
// EventManager::HashPair::Hash struct
//
    inline  
    size_t
    EventManager::HashPair::Hash::operator()(const HashPair& val) const
      throw()
    {
#ifdef M32
      return val.first() ^ val.second();
#else
      return (((uint64_t)val.first()) << 32) | ((uint64_t)val.second());
#endif      
    }    
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_EVENT_BANK_EVENTMANAGER_HPP_
