/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Message/Bank/MessageManager.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_MESSAGEMANAGER_HPP_
#define _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_MESSAGEMANAGER_HPP_

#include <stdint.h>

#include <string>
#include <list>
#include <utility>
#include <memory>
#include <map>
#include <set>
#include <vector>
#include <sstream>

#include <ext/hash_set>
#include <ext/hash_map>

#include <google/dense_hash_map>
#include <google/sparse_hash_set>
#include <google/sparse_hash_map>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/RefCount/All.hpp>
#include <El/Service/CompoundService.hpp>
#include <El/String/StringPtr.hpp>
#include <El/String/LightString.hpp>
#include <El/Locale.hpp>
#include <El/TokyoCabinet/DBM.hpp>
#include <El/Mutex.hpp>

#include <xsd/Config.hpp>

#include <Commons/Message/StoredMessage.hpp>
#include <Commons/Message/TransportImpl.hpp>
#include <Commons/Search/SearchExpression.hpp>
#include <Commons/Event/TransportImpl.hpp>
#include <Commons/Message/Categorizer.hpp>

#include <Services/Commons/Message/MessageServices.hpp>
#include <Services/Commons/Event/EventServices.hpp>

#include "ContentCache.hpp"
#include "MessageCategorizer.hpp"
#include "MessageLoader.hpp"
#include "MessagePack.hpp"
#include "WordPairManager.hpp"

namespace NewsGate
{
  namespace Message
  {
    struct MessageSink
    {
      ::NewsGate::Message::BankClientSession_var message_sink;
      unsigned long flags;
      ACE_Time_Value expiration_time;

      MessageSink() throw() : flags(0) {}
    };

    class MessageSinkMap : public __gnu_cxx::hash_map<std::string,
                           MessageSink,
                           El::Hash::String>
    { 
    public:
      MessageSinkMap() throw(El::Exception) {}
    };      
      
    class MessageManagerCallback : public virtual El::Service::Callback
    {
    public:
      virtual MessageSinkMap message_sink_map(unsigned long flags)
        throw(El::Exception) = 0;

      virtual void dictionary_hash_changed() throw(El::Exception) = 0;
    };

    class MessageManager :
      public El::Service::CompoundService<MessageLoader,
                                          MessageManagerCallback>,
      public virtual MessageLoaderCallback,
      public virtual WordPairManager
    {
    public:
      EL_EXCEPTION(Exception, El::ExceptionBase);
      EL_EXCEPTION(WordManagerNotReady, Exception);
      EL_EXCEPTION(SegmentorNotReady, Exception);

      enum AssignResult
      {
        AR_SUCCESS,
        AR_NOT_CHANGED,
        AR_NOT_READY
      };

      struct LTWPBuff
      {
        unsigned char buff[sizeof(uint16_t) + 3 *sizeof(uint32_t)];
        
        LTWPBuff() throw() { memset(buff, 0, sizeof(buff)); }
        LTWPBuff(const El::Lang& l, uint32_t ti, const WordPair& wp) throw();

        size_t size() const throw() { return sizeof(buff); }

        bool operator==(const LTWPBuff& val) const throw();
        bool operator<(const LTWPBuff& val) const throw();        
      };

      struct LTWPBuffHash
      {
        size_t operator()(const LTWPBuff& ltwp) const throw();
      };
      
      struct LT
      {
        El::Lang lang;
        uint32_t time_index;

        LT() throw() : time_index(0) {}
        LT(const El::Lang& l, uint32_t ti) throw();
        bool operator==(const LT& val) const throw();

        std::string string() const throw(El::Exception);
      };

      struct LTHash
      {
        size_t operator()(const LT& lt) const throw();
      };
      
      class WordPairCounter :
        public google::sparse_hash_map<WordPair, uint32_t, WordPairHash>
      {
      public:
        WordPairCounter() throw(El::Exception) { set_deleted_key(WordPair()); }
      };

      class WordPairSet :
        public google::sparse_hash_set<WordPair, WordPairHash>
      {
      public:
        WordPairSet() throw(El::Exception) { set_deleted_key(WordPair()); }
      };

      class LTWPCounter :
        public google::sparse_hash_map<LT, WordPairCounter*, LTHash>
      {
      public:
        LTWPCounter() throw(El::Exception) { set_deleted_key(LT()); }
        ~LTWPCounter() throw();
      };

      class LTWPSet :
        public google::sparse_hash_map<LT, WordPairSet*, LTHash>
      {
      public:
        LTWPSet() throw(El::Exception) { set_deleted_key(LT()); }
        ~LTWPSet() throw();
      };      
      
      struct LTWPBuffCounterHashMap :
        public google::dense_hash_map<LTWPBuff, int, LTWPBuffHash>
      {
        LTWPBuffCounterHashMap() throw(El::Exception);
      };
      
    public:
      MessageManager(
        MessageManagerCallback* callback,
        const Server::Config::BankMessageManagerType& config,
        Message::BankSession* session,
        BankClientSession* bank_client_session)
        throw(El::Exception);
      
      virtual ~MessageManager() throw();

      void session(Message::BankSession* value)
        throw(Exception, El::Exception);
      
      void insert(const StoredMessageList& messages,
                  ::NewsGate::Message::PostMessageReason reason)
        throw(WordManagerNotReady, Exception, El::Exception);

      void process_message_events(
        Transport::MessageEventPackImpl::Type* pack,
        ::NewsGate::Message::PostMessageReason reason)
        throw(Exception, El::Exception);

      AssignResult set_message_fetch_filter(const FetchFilter& filter)
        throw(Exception, El::Exception);
      
      AssignResult assign_message_fetch_filter(const FetchFilter& filter)
        throw(Exception, El::Exception);

      AssignResult assign_message_categorizer(const Categorizer& categorizer)
        throw(Exception, El::Exception);

      bool reset_categorizer(const std::string& new_cat_source,
                             uint64_t new_cat_stamp,
                             bool enforce) throw();

      void set_mirrored_manager(const char* mirrored_manager,
                                const char* sharing_id)
        throw(El::Exception);

      struct MessageFetchFilter
      {
        uint64_t stamp;
        Search::Condition_var condition;

        MessageFetchFilter(uint64_t stamp_val,
                           Search::Condition* condition_val)
          throw(El::Exception);

        MessageFetchFilter() : stamp(0) {}
        ~MessageFetchFilter() throw() {}

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);
      };

      struct MessageFetchFilterMap :
        public __gnu_cxx::hash_map<std::string,
                                   MessageFetchFilter,
                                   El::Hash::String>,
        public El::RefCount::DefaultImpl<El::Sync::ThreadPolicy>
      {
        uint32_t dict_hash;
        
        MessageFetchFilterMap() throw() : dict_hash(0) {}

        MessageFetchFilterMap(const MessageFetchFilterMap& src)
          throw(El::Exception);
        
        ~MessageFetchFilterMap() throw() {}

        MessageFetchFilterMap& operator=(const MessageFetchFilterMap& src)
          throw(El::Exception);
        
        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);
      };
      
      typedef El::RefCount::SmartPtr<MessageFetchFilterMap>
      MessageFetchFilterMap_var;

      struct IdTimeMap :
        public google::sparse_hash_map<Id, uint64_t, MessageIdHash>
      {
        IdTimeMap() throw(El::Exception) { set_deleted_key(Id::zero); }
      };

      struct PreInsertInfo
      {
        Message::StoredMessage msg;
        WordsFreqInfo word_freq_info;
      };

      typedef __gnu_cxx::hash_map<Id,
                                  PreInsertInfo,
                                  MessageIdHash> PreInsertInfoMap;
      
      size_t apply_message_fetch_filters(
        SearcheableMessageMap& messages,
        MessageFetchFilterMap* filters,
        Search::Expression* capacity_filter,
        IdTimeMap& removed_msg,
        size_t max_remove_count)
        throw(Exception, El::Exception);

      void delete_messages(const IdTimeMap& ids,
                           El::MySQL::Connection* connection,
                           bool event_bank_notify = true)
        throw(Exception, El::Exception);

      void categorize(SearcheableMessageMap& messages,
                      MessageCategorizer* categorizer,
                      const IdTimeMap& ids,
                      bool no_hash_check,
                      MessageCategorizer::MessageCategoryMap& old_categories)
        throw(Exception, El::Exception);

      Search::Condition* category_message_condition(
        const Categorizer::Category::RelMsgArray& messages,
        IdSet& recent_msg_ids) const
        throw(Exception, El::Exception);
      
      void post_categorize_and_save(
        SearcheableMessageMap* temp_messages,
        const IdTimeMap& ids,
        const MessageCategorizer::MessageCategoryMap& old_categories,
        bool extended_save,
        ACE_Time_Value& filtering_time,
        ACE_Time_Value& applying_change_time,
        ACE_Time_Value& deletion_time,
        ACE_Time_Value& saving_time,
        ACE_Time_Value& db_insertion_time)
        throw(Exception, El::Exception);

      void categorize_and_save(
        MessageCategorizer* categorizer,
        const IdTimeMap& ids,
        bool extended_save,
        Transport::MessageSharingInfoPackImpl::Var* share_messages)
        throw(Exception, El::Exception);

      MessageFetchFilterMap* get_message_fetch_filters() const throw();
      MessageCategorizer* get_message_categorizer() const throw();
      
      void update_events(const Transport::MessageEventArray& message_events)
        throw(Exception, El::Exception);

      void message_sharing_offer(
        Transport::MessageSharingInfoArray* offered_msg,
        IdArray& requested_msg)
        throw(Exception, El::Exception);

      ::NewsGate::Search::Result* search(
        const ::NewsGate::Search::Expression* expression,
        size_t start_from,
        size_t results_count,
        const ::NewsGate::Search::Strategy& strategy,
        const El::Locale& locale,
        unsigned long long gm_flags,
        const char* request_category_locale,
        Categorizer::Category::Locale& category_locale,
        size_t& total_matched_messages,
        size_t& suppressed_messages) const
        throw(Exception, El::Exception);

      Transport::StoredMessageArray*
      get_messages(const IdArray& ids,
                   uint64_t gm_flags,
                   int32_t img_index,
                   int32_t thumb_index,
                   IdArray& notfound_msg_ids)
        throw(El::Exception);

      Transport::Response* get_message_digests(
        Transport::EventIdArray& event_ids) const   
        throw(El::Exception);

      Transport::Response* check_mirrored_messages(IdArray& message_ids,
                                                   bool ready) const
        throw(/*NewsGate::Message::NotReady,*/
              El::Exception);

      Transport::Response* message_stat(
        Transport::MessageStatRequestInfo* stat) throw(El::Exception);

      virtual bool notify(El::Service::Event* event) throw(El::Exception);
      virtual bool start() throw(Exception, El::Exception);
      virtual void wait() throw(Exception, El::Exception);

      void flush_messages() throw(Exception, El::Exception);

      bool loaded() const throw();
      size_t message_count() const throw();

      void adjust_capacity_threshold(size_t pack_message_count) throw();
      
      std::string save_pack(const PendingMessagePack& pack, bool temporary)
        const throw(Exception, El::Exception);

      virtual void wp_increment_counter(const El::Lang& lang,
                                        uint32_t time_index,
                                        const WordPair& wp)
        throw(El::Exception);
      
      virtual void wp_decrement_counter(const El::Lang& lang,
                                        uint32_t time_index,
                                        const WordPair& wp)
        throw(El::Exception);
      
      virtual uint32_t wp_get(const El::Lang& lang,
                              uint32_t time_index,
                              const WordPair& wp)
        throw(El::Exception);
      
    private:

      std::string process_event(El::Service::Event* event)
        throw(El::Exception);
      
      uint32_t dictionary_hash()
        throw(WordManagerNotReady, Exception, El::Exception);
      
      Search::Condition* segment_search_expression(const char* exp)
        throw(SegmentorNotReady, Exception, El::Exception);
      
      bool normalize_search_condition(Search::Condition_var& condition)
        throw(WordManagerNotReady, Exception, El::Exception);
      
      bool normalize_words(StoredMessageList& messages,
                           uint32_t dict_hash,
                           IdSet* changed_norm_forms = 0)
        throw(WordManagerNotReady, Exception, El::Exception);

      bool renormalize_words(StoredMessageList& messages,
                             uint32_t dict_hash)
        throw(WordManagerNotReady, Exception, El::Exception);
      
      void insert_loaded_messages() throw(El::Exception);
      
      void insert_loaded_messages(StoredMessageArray& messages,
                                  uint32_t dict_hash,
                                  StoredMessageList& messages_to_renormalize)
        throw(Exception, El::Exception);

      void traverse_cache() throw();
      void apply_message_fetch_filters() throw(Exception, El::Exception);
      void reapply_message_fetch_filters() throw(Exception, El::Exception);

      void delete_obsolete_messages(const char* table_name)
        throw(El::Exception);

      void save_message_stat(Transport::MessageStatRequestInfo* stat_val)
        throw(El::Exception);

      void save_message_sharing_info(
        Transport::MessageSharingInfoArray& info) throw(El::Exception);
      
      struct ImportMsg : public El::Service::CompoundServiceMessage
      {
        ImportMsg(MessageManager* state) throw(El::Exception);
      };

      struct InsertLoadedMsg : public El::Service::CompoundServiceMessage
      {
        InsertLoadedMsg(MessageManager* state) throw(El::Exception);
      };

      struct TraverseCache : public El::Service::CompoundServiceMessage
      {
        TraverseCache(MessageManager* state) throw(El::Exception);
      };

      struct ApplyMsgFetchFilters :
        public El::Service::CompoundServiceMessage
      {
        ApplyMsgFetchFilters(MessageManager* state) throw(El::Exception);
      };

      struct ReapplyMsgFetchFilters :
        public El::Service::CompoundServiceMessage
      {
        ReapplyMsgFetchFilters(MessageManager* state) throw(El::Exception);
      };

      struct MsgDeleteNotification : public El::Service::CompoundServiceMessage
      {
        MsgDeleteNotification(MessageManager* state) throw(El::Exception);
      };

      struct SetMirroredManager : public El::Service::CompoundServiceMessage
      {
        std::string mirrored_manager_ref;
        std::string sharing_id;
        
        SetMirroredManager(MessageManager* state,
                           const char* mirrored_manager_val,
                           const char* sharing_id_val)
          throw(El::Exception);

        ~SetMirroredManager() throw() {}
      };

      struct SaveMsgStat : public El::Service::CompoundServiceMessage
      {
        std::auto_ptr<Transport::MessageStatRequestInfo> stat;
        
        SaveMsgStat(MessageManager* state,
                    Transport::MessageStatRequestInfo* stat_val)
          throw(El::Exception);

        ~SaveMsgStat() throw() {}
      };

      struct SaveMsgSharingInfo : public El::Service::CompoundServiceMessage
      {
        std::auto_ptr<Transport::MessageSharingInfoArray> info;
        
        SaveMsgSharingInfo(MessageManager* state,
                           Transport::MessageSharingInfoArray* info_val)
          throw(El::Exception);

        ~SaveMsgSharingInfo() throw() {}
      };

      struct DeleteObsoleteMessages :
        public El::Service::CompoundServiceMessage
      {
        std::string table_name;
        
        DeleteObsoleteMessages(MessageManager* state,
                               const char* tbl_name) throw(El::Exception);

        ~DeleteObsoleteMessages() throw() {}
      };

      typedef std::set<StoredMessage> StoredMessageSet;
      
      struct InsertChangedMessages :
        public El::Service::CompoundServiceMessage
      {
        StoredMessageSet message_set;
        
        InsertChangedMessages(MessageManager* state,
                              StoredMessageSet& msg_set) throw(El::Exception);

        ~InsertChangedMessages() throw() {}
      };

      void msg_delete_notification(MsgDeleteNotification* mdn)
        throw(Exception, El::Exception);

      void msg_delete_event_bank_notify(const IdTimeMap& ids)
        throw(El::Exception);

      void insert_changed_messages(InsertChangedMessages* icm)
        throw(Exception, El::Exception);
      
      enum MessageOperation
      {
        MO_PREEMPT,
        MO_DELETE,
        MO_FLUSH_STATE
      };

      typedef std::list<std::pair<Message::Id, MessageOperation> >
      MessageOperationList;

      struct IdToSizeMap :
        public google::dense_hash_map<Id, size_t, MessageIdHash>
      {
        IdToSizeMap() throw(El::Exception);
      };
      
      void exec_msg_operations(const MessageOperationList& operations,
                               time_t cur_time,
                               time_t preempt_time,
                               time_t expire_time,
                               std::ostream& log_ostr)
        throw(Exception, El::Exception);
      
      void move_messages_to_owners(
        Transport::StoredMessageArrayPtr& messages,
        const IdToSizeMap* it_to_index_map,
        const char* query)
        throw(Exception, El::Exception);

      bool post_to_owners(
        Transport::StoredMessageArray* messages,
        IdTimeMap& posted) throw();
      
      void share_requested_messages(
        Transport::MessageSharingInfoPackImpl::Type* messages_to_share)
        throw(Exception, El::Exception);

      void share_messages(const IdArray& ids,
                          ::NewsGate::Message::BankClientSession* session,
                          ::NewsGate::Message::PostMessageReason reason,
                          const char* sharing_id)
        throw(CORBA::Exception, El::Exception);

      void check_mirrored_messages(
        BankClientSession* mirrored_banks,
        const char* mirrored_sharing_id,
        Transport::CheckMirroredMessagesRequestImpl::Type* messages_to_check)
        throw(Exception, El::Exception);
      
      void share_all_messages(const StoredMessageList& share_msg,
                              MessageSinkMap& message_sink,
                              PostMessageReason sharing_reason)
        throw(Exception, El::Exception);
      
      bool own_message(const Id& id, bool check_bank_validity = true) const
        throw();

      void record_word_freq_distribution(const StoredMessage& msg)
        throw(Exception, El::Exception);
      
      void dump_mem_usage() const throw(El::Exception);
      
      void dump_message_size_distribution() const
        throw(Exception, El::Exception);

/*      
      void record_word_mem_distribution(const StoredMessage& msg)
        throw(Exception, El::Exception);
      
      void dump_word_mem_distribution() throw(Exception, El::Exception);
*/
      template<typename TYPE>
      void dump_word_freq_distribution(const TYPE& word_messages,
                                       const char* prefix)
        const throw(Exception, El::Exception);

      struct WordLenStat
      {
        size_t count;

        WordLenStat() : count(0) {}
      };

      typedef std::map<size_t, WordLenStat> WordLenStatMap;
      typedef std::set<std::string> WordSet;

      typedef std::auto_ptr<WordLenStatMap> WordLenStatMapPtr;
      typedef std::auto_ptr<WordSet> WordSetPtr;

/*      
      void dump_word_mem_distribution(WordLenStatMapPtr& message_word_len_stat,
                                      const char* stat_name)
        throw(Exception, El::Exception);
*/

      void dump_word_pair_freq_distribution()
        throw(Exception, El::Exception);

      void fill_debug_info(Transport::StoredMessageArray& message,
                           uint64_t gm_flags) const
        throw(El::Exception);
      
      void flush_content_cache(bool skip_if_busy) throw(El::Exception);
      
      void load_msg_content(const IdArray& ids,
                            bool update_storage,
                            Transport::StoredMessageArray& result,
                            IdArray& notfound_msg_ids)
        throw(El::Exception);
      
      void load_img_thumb(const char* context,
                          Transport::StoredMessageArray& result,
                          int32_t img_index = -1,
                          int32_t thumb_index = -1)
        throw(El::Exception);
      
      bool insert(const StoredMessageList& messages,
                  bool replace,
                  bool colo_local_msg,
                  bool mirror,
                  Event::Transport::MessageDigestArray* message_digests,
                  StoredMessageList* share_messages)
        throw(WordManagerNotReady, Exception, El::Exception);

      void post_digests(
        Event::BankClientSession* event_bank_client_session,
        Event::Transport::MessageDigestPackImpl::Type* message_digest_pack)
        throw(Exception, El::Exception);
      
      void save_messages(const char* msg_filename,
                         const char* cat_filename,
                         const char* dic_filename,
                         size_t& loaded_rows,
                         size_t& inserted_rows,
                         ACE_Time_Value& time) const
        throw(El::Exception);

      void set_mirrored_banks(const char* mirrored_manager,
                              const char* sharing_id)
        throw(El::Exception);

      void schedule_traverse() throw(El::Exception);
      void schedule_load() throw(El::Exception);

      void traverse_messages(bool prio_msg) throw(El::Exception);
      void import_messages(ImportMsg* im) throw(El::Exception);

      enum ImportMessagesResult
      {
        IMR_NONE,
        IMR_ERROR,
        IMR_SUCCESS
      };
      
      ImportMessagesResult import_messages(const char* directory,
                                           PostMessageReason pack_reason)
        throw(El::Exception);

      typedef std::vector<std::string> StringArray;
      
      void import_messages(El::BinaryInStream& bstr,
                           PostMessageReason pack_reason,
                           size_t& messages,
                           size_t& thumbnails,
                           StringArray& pack_files)
        throw(Exception, El::Exception);      

      static void flush_msg_stat(std::fstream& file,
                                 StoredMessage& msg,
                                 bool newline)
        throw(El::Exception);

      void upload_flushed_msg_stat(El::MySQL::Connection* connection,
                                   const char* filename,
                                   ACE_Time_Value& tm)
        throw(El::Exception);
      
      bool normalize_categorizer(uint32_t dict_hash)
        throw(Exception, El::Exception);
      
      bool normalize_filters(uint32_t dict_hash)
        throw(Exception, El::Exception);
      
      MessageCategorizer* load_message_categorizer(const char* filename)
        throw(Exception, El::Exception);
      
      bool save_message_categorizer(
        const MessageCategorizer& categorizer,
        const char* filename)
        throw(Exception, El::Exception);

      MessageFetchFilterMap* load_message_filters(const char* filename)
        throw(Exception, El::Exception);
      
      bool save_message_filters(
        const MessageFetchFilterMap& message_filters,
        const char* filename)
        throw(Exception, El::Exception);

      void recent_msg_deletions(const IdTimeMap& ids) throw(El::Exception);
      
      void save_msg_time_map(IdTimeMap& msg_map, const char* ext)
        throw(Exception, El::Exception);
      
      void load_msg_time_map(IdTimeMap& msg_map, const char* ext)
        throw(Exception, El::Exception);

      static void dump_buff(ostream& ostr,
                            const unsigned char* buff,
                            size_t size)
        throw(El::Exception);

      void inc_wp_freq_distr(int count, int incr) throw();

      TCWordPairManager& word_pair_manager(const LT& lt) const throw();
      
    private:

      typedef El::Service::CompoundService<MessageLoader,
                                           MessageManagerCallback>
      BaseClass;

      typedef ACE_Thread_Mutex ThreadMutex;
      typedef ACE_Guard<ThreadMutex> Guard;

      mutable ThreadMutex db_lock_;
      
      typedef El::Sync::AdaptingReadGuard MgrReadGuard;
      typedef El::Sync::AdaptingWriteGuard MgrWriteGuard;

      El::Sync::AdaptingMutex mgr_lock_;
      
      const Server::Config::BankMessageManagerType& config_;
      Message::BankSession_var session_;
      BankClientSession_var bank_client_session_;
      
      BankClientSession_var mirrored_banks_;
      std::string mirrored_manager_;
      std::string mirrored_sharing_id_;

      std::string wp_counter_type_;
      size_t wp_intervals_;
      
      SearcheableMessageMap messages_;
      ContentCache content_cache_;
      
      std::string cache_filename_;
      ACE_Time_Value next_sharing_time_;
      ACE_Time_Value optimize_mem_time_;      
      uint32_t dict_hash_;
      size_t preload_mem_usage_;
      
      size_t capacity_threshold_;
      Search::Expression_var capacity_filter_;

      unsigned long message_word_count_distribution_[3];
      unsigned long long message_word_freq_distribution_[3];
      unsigned long long norm_forms_freq_distribution_[3];

/*      
      WordLenStatMapPtr message_word_len_stat_;
      WordLenStatMapPtr message_unique_word_len_stat_;
      WordSetPtr message_unique_words_;
*/

      NumberSet traverse_message_;
      NumberSet::const_iterator traverse_message_it_;

      NumberSet traverse_prio_message_;
      NumberSet::const_iterator traverse_prio_message_it_;
      
      MessageFetchFilterMap_var message_filters_;
      MessageCategorizer_var message_categorizer_;

      bool loaded_;
      bool flushed_;

      StoredMessageSet changed_messages_;
      
      IdTimeMap del_msg_notifications_;
      IdTimeMap recent_msg_deletions_;
      Id msg_dom_last_id_;

      mutable ThreadMutex ltwp_counter_lock_;
      
      typedef __gnu_cxx::hash_map<LT, TCWordPairManager_var, LTHash>
      TCWordPairManagerMap;
      
      LTWPCounter ltwp_counter_;
      LTWPSet ltwp_set_;
      std::auto_ptr<TCWordPairManagerMap> word_pair_managers_;

      unsigned long message_wp_freq_distribution_[100];
    };
    
    typedef El::RefCount::SmartPtr<MessageManager> MessageManager_var;
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
    // MessageManager class
    //

    inline
    void
    MessageManager::inc_wp_freq_distr(int count, int incr) throw()
    {
      if(count--)
      {
        int len = sizeof(message_wp_freq_distribution_) /
          sizeof(message_wp_freq_distribution_[0]);
          
        message_wp_freq_distribution_[count < len ? count : (len - 1)] += incr;
      }    
    }

    //
    // LT struct
    //
    inline
    MessageManager::LT::LT(const El::Lang& l, uint32_t ti) throw()
        : lang(l),
          time_index(ti)
    {
    }

    inline
    bool
    MessageManager::LT::operator==(const LT& val) const throw()
    {
      return lang == val.lang && time_index == val.time_index;
    }

    inline
    std::string
    MessageManager::LT::string() const throw(El::Exception)
    {
      std::ostringstream ostr;
      ostr << lang.l3_code(true) << "_" << time_index;
      return ostr.str();
    }

    inline
    size_t
    MessageManager::LTHash::operator()(const LT& lt) const throw()
    {
      unsigned long crc = El::CRC32_init();
      uint16_t lcode = lt.lang.el_code();  

      El::CRC32(crc,
                (const unsigned char*)&lcode,
                sizeof(lcode));
      
      El::CRC32(crc,
                (const unsigned char*)&lt.time_index,
                sizeof(lt.time_index));
      
      return crc;
    }
    
    //
    // LTWPCounter class
    //
    inline
    MessageManager::LTWPCounter::~LTWPCounter() throw()
    { 
      for(iterator i(begin()), e(end()); i != e; ++i)
      {
        delete i->second;
      }
    }
    
    //
    // LTWPSet class
    //
    inline
    MessageManager::LTWPSet::~LTWPSet() throw()
    { 
      for(iterator i(begin()), e(end()); i != e; ++i)
      {
        delete i->second;
      }
    }
    
    //
    // LTWPBuff struct
    //
    inline
    MessageManager::LTWPBuff::LTWPBuff(const El::Lang& l,
                                       uint32_t ti,
                                       const WordPair& wp) throw()
    {
      unsigned char* p = buff;
      uint16_t lcode = l.el_code();

      assert(sizeof(buff) ==
             sizeof(lcode) + sizeof(ti) + sizeof(wp.word1) + sizeof(wp.word2));
      
      memcpy(p, &lcode, sizeof(lcode));
      p += sizeof(lcode);

      memcpy(p, &ti, sizeof(ti));
      p += sizeof(ti);

      memcpy(p, &wp.word1, sizeof(wp.word1));
      p += sizeof(wp.word1);
      
      memcpy(p, &wp.word2, sizeof(wp.word2));
    }

    inline
    bool
    MessageManager::LTWPBuff::operator==(const LTWPBuff& val) const throw()
    {
      return memcmp(buff, val.buff, sizeof(buff)) == 0;
    }

    inline
    bool
    MessageManager::LTWPBuff::operator<(const LTWPBuff& val) const throw()
    {
      return memcmp(buff, val.buff, sizeof(buff)) < 0;
    }

    inline
    size_t
    MessageManager::LTWPBuffHash::operator()(const LTWPBuff& ltwp) const
      throw()
    {
      unsigned long crc = El::CRC32_init();
      El::CRC32(crc, (const unsigned char*)ltwp.buff, sizeof(ltwp.buff));
      return crc;
    }

    //
    // NewsGate::Message::MessageManager::LTWPBuffCounterHashMap struct
    //
    inline
    MessageManager::LTWPBuffCounterHashMap::LTWPBuffCounterHashMap()
      throw(El::Exception)
    {
      set_deleted_key(LTWPBuff(El::Lang::nonexistent, 0, WordPair()));
      set_empty_key(LTWPBuff(El::Lang::nonexistent2, 0, WordPair()));
    }    

    //
    // NewsGate::Message::MessageManager::MessageFetchFilter struct
    //
    inline
    void
    MessageManager::MessageFetchFilter::write(El::BinaryOutStream& bstr)
      const throw(El::Exception)
    {
      bstr << stamp << condition;
    }

    inline
    void
    MessageManager::MessageFetchFilter::read(El::BinaryInStream& bstr)
      throw(El::Exception)
    {
      bstr >> stamp >> condition;
    }
    
    //
    // NewsGate::Message::MessageManager::MessageFetchFilterMap struct
    //

    inline
    MessageManager::MessageFetchFilterMap::MessageFetchFilterMap(
      const MessageFetchFilterMap& src) throw(El::Exception)
    {
      *this = src;
    }
    
    inline
    MessageManager::MessageFetchFilterMap&
    MessageManager::MessageFetchFilterMap::operator=(
      const MessageFetchFilterMap& src) throw(El::Exception)
    {
      dict_hash = src.dict_hash;
      
      clear();
      insert(src.begin(), src.end());
      return *this;
    }
    
    inline
    void
    MessageManager::MessageFetchFilterMap::write(El::BinaryOutStream& bstr)
      const throw(El::Exception)
    {
      bstr << dict_hash;
      bstr.write_map(*this);
    }

    inline
    void
    MessageManager::MessageFetchFilterMap::read(El::BinaryInStream& bstr)
      throw(El::Exception)
    {
      bstr >> dict_hash;
      bstr.read_map(*this);
    }

    //
    // NewsGate::Message::MessageManager::IdToSizeMap struct
    //
    inline
    MessageManager::IdToSizeMap::IdToSizeMap() throw(El::Exception)
    {
      set_deleted_key(Id::zero);
      set_empty_key(Id::nonexistent);
    }
    
    //
    // NewsGate::Message::MessageManager class
    //
    inline
    bool
    MessageManager::loaded() const throw()
    {
      MgrReadGuard guard(mgr_lock_);
      return loaded_ && !flushed_;
    }

    inline
    size_t
    MessageManager::message_count() const throw()
    {
      MgrReadGuard guard(mgr_lock_);
      return messages_.messages.size();
    }
    
    inline
    bool
    MessageManager::own_message(const Id& id, bool check_bank_validity) const
      throw()
    {
      CORBA::ULong index = 0;
      CORBA::ULong banks_count = 0;
      
      session_->location(index, banks_count);
      
      if(banks_count == 0 || (id.src_id() % banks_count) == index)
      {
        return true;
      }
      
      return check_bank_validity ?
        (bank_client_session_->owner_bank_state(id.src_id()) ==
         BankClientSession::BS_INVALIDATED) : false;
    }

    inline
    TCWordPairManager&
    MessageManager::word_pair_manager(const LT& lt) const throw()
    {
      TCWordPairManagerMap::const_iterator i = word_pair_managers_->find(lt);

      if(i == word_pair_managers_->end())
      {
        i = word_pair_managers_->find(LT(El::Lang::null, 0));
      }

      assert(i != word_pair_managers_->end());
      
      return *i->second.in();
      
//      return *word_pair_manager_.get();
    }

    //
    // NewsGate::Message::MessageManager::MessageFetchFilter class
    //
    inline
    MessageManager::MessageFetchFilter::MessageFetchFilter(
      uint64_t stamp_val,
      Search::Condition* condition_val)
      throw(El::Exception)
        : stamp(stamp_val),
          condition(condition_val)
    {
    }

    //
    // NewsGate::Message::MessageManager::ImportMsg class
    //
    inline
    MessageManager::ImportMsg::ImportMsg(MessageManager* state)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }
    
    //
    // NewsGate::Message::MessageManager::InsertLoadedMsg class
    //
    inline
    MessageManager::InsertLoadedMsg::InsertLoadedMsg(MessageManager* state)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }
    
    //
    // NewsGate::Message::MessageManager::TraverseCache class
    //
    inline
    MessageManager::TraverseCache::TraverseCache(MessageManager* state)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }

    //
    // NewsGate::Message::MessageManager::MsgDeleteNotification class
    //
    inline
    MessageManager::MsgDeleteNotification::MsgDeleteNotification(
      MessageManager* state)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }

    //
    // NewsGate::Message::SetMirroredManager::SetMirroredManager class
    //
    inline
    MessageManager::SetMirroredManager::SetMirroredManager(
      MessageManager* state,
      const char* mirrored_manager_val,
      const char* sharing_id_val)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, true),
          El::Service::CompoundServiceMessage(state, state),
          mirrored_manager_ref(mirrored_manager_val ?
                               mirrored_manager_val : ""),
          sharing_id(sharing_id_val ? sharing_id_val : "")
    {
    }
    
    //
    // NewsGate::Message::SaveMsgStat::SaveMsgStat class
    //
    inline
    MessageManager::SaveMsgStat::SaveMsgStat(
      MessageManager* state,
      Transport::MessageStatRequestInfo* stat_val)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, true),
          El::Service::CompoundServiceMessage(state, state),
          stat(stat_val)
    {
    }

    //
    // NewsGate::Message::SaveMsgSharingInfo::SaveMsgSharingInfo class
    //
    inline
    MessageManager::SaveMsgSharingInfo::SaveMsgSharingInfo(
      MessageManager* state,
      Transport::MessageSharingInfoArray* info_val)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, true),
          El::Service::CompoundServiceMessage(state, state),
          info(info_val)
    {
    }
    
    //
    // NewsGate::Message::DeleteObsoleteMessages::DeleteObsoleteMessages class
    //
    inline
    MessageManager::DeleteObsoleteMessages::DeleteObsoleteMessages(
      MessageManager* state,
      const char* tbl_name)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, true),
          El::Service::CompoundServiceMessage(state, state),
          table_name(tbl_name)
    {
    }

    //
    // NewsGate::Message::InsertChangedMessages::InsertChangedMessages class
    //
    inline
    MessageManager::InsertChangedMessages::InsertChangedMessages(
      MessageManager* state,
      StoredMessageSet& msg_set)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, true),
          El::Service::CompoundServiceMessage(state, state)
    {
      message_set.swap(msg_set);
    }    
    
    //
    // NewsGate::Message::MessageManager::ApplyMsgFetchFilters class
    //
    inline
    MessageManager::ApplyMsgFetchFilters::ApplyMsgFetchFilters(
      MessageManager* state)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }

    //
    // NewsGate::Message::MessageManager::ReapplyMsgFetchFilters class
    //
    inline
    MessageManager::ReapplyMsgFetchFilters::ReapplyMsgFetchFilters(
      MessageManager* state)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }

  }
}

#endif // _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_MESSAGEMANAGER_HPP_
