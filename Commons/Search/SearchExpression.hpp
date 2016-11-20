/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Commons/Search/SearchExpression.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_COMMONS_SEARCH_SEARCHEXPRESSION_HPP_
#define _NEWSGATE_SERVER_COMMONS_SEARCH_SEARCHEXPRESSION_HPP_

#include <stdint.h>
#include <limits.h>

#include <iostream>
#include <string>
#include <memory>
#include <vector>

#include <google/sparse_hash_map>
#include <google/sparse_hash_set>
#include <google/dense_hash_set>
#include <google/dense_hash_map>

#include <El/Exception.hpp>
#include <El/RefCount/All.hpp>
#include <El/Stat.hpp>
#include <El/BinaryStream.hpp>
#include <El/Hash/Hash.hpp>
#include <El/Luid.hpp>
#include <El/Lang.hpp>
#include <El/Country.hpp>
#include <El/String/Unicode.hpp>
#include <El/GCC_Version.hpp>

#include <Commons/Feed/Types.hpp>
#include <Commons/Message/Message.hpp>
#include <Commons/Message/StoredMessage.hpp>
#include <Commons/Search/SearchCondition.hpp>

//#define TRACE_SEARCH_TIME

namespace NewsGate
{
  namespace Search
  {
    struct Strategy
    {
      enum SortingMode
      {
        SM_NONE,
        SM_BY_RELEVANCE_DESC,
        SM_BY_PUB_DATE_DESC,
        SM_BY_PUB_DATE_ASC,
        SM_BY_FETCH_DATE_DESC,
        SM_BY_FETCH_DATE_ASC,
        SM_BY_RELEVANCE_ASC,
        SM_BY_EVENT_CAPACITY_DESC,
        SM_BY_EVENT_CAPACITY_ASC,
        SM_BY_POPULARITY_DESC,
        SM_BY_POPULARITY_ASC,
        SM_COUNT
      };

      struct Sorting
      {        
        virtual ~Sorting() throw() {}

        virtual SortingMode type() throw() = 0;
        virtual Sorting* clone() throw(El::Exception) = 0;
        
        virtual void write(El::BinaryOutStream& bstr) const
          throw(El::Exception) {}
        
        virtual void read(El::BinaryInStream& bstr) throw(El::Exception) {}
      };

      struct SortNone : public Sorting
      {
        virtual ~SortNone() throw() {}
        virtual SortingMode type() throw() { return SM_NONE; }
        virtual Sorting* clone() throw(El::Exception);        
      };

      struct SortUseTime : public Sorting
      {
        uint32_t message_max_age;

        virtual void write(El::BinaryOutStream& bstr) const
          throw(El::Exception);
        
        virtual void read(El::BinaryInStream& bstr) throw(El::Exception);

      protected:
        SortUseTime(uint32_t message_max_age_val) throw();
        virtual ~SortUseTime() throw() {}
      };      
      
      struct SortByPubDateDesc : public SortUseTime
      {
        SortByPubDateDesc(uint32_t message_max_age_val = 86400) throw();
        
        virtual ~SortByPubDateDesc() throw() {}
        virtual SortingMode type() throw() { return SM_BY_PUB_DATE_DESC; }
        virtual Sorting* clone() throw(El::Exception);        
      };      
      
      struct SortByPubDateAcs : public SortUseTime
      {
        SortByPubDateAcs(uint32_t message_max_age_val = 86400) throw();

        virtual ~SortByPubDateAcs() throw() {}
        virtual SortingMode type() throw() { return SM_BY_PUB_DATE_ASC; }
        virtual Sorting* clone() throw(El::Exception);        
      };      
      
      struct SortByFetchDateDesc : public SortUseTime
      {
        SortByFetchDateDesc(uint32_t message_max_age_val = 86400) throw();

        virtual ~SortByFetchDateDesc() throw() {}
        virtual SortingMode type() throw() { return SM_BY_FETCH_DATE_DESC; }
        virtual Sorting* clone() throw(El::Exception);        
      };      
      
      struct SortByFetchDateAcs : public SortUseTime
      {
        SortByFetchDateAcs(uint32_t message_max_age_val = 86400) throw();

        virtual ~SortByFetchDateAcs() throw() {}
        virtual SortingMode type() throw() { return SM_BY_FETCH_DATE_ASC; }
        virtual Sorting* clone() throw(El::Exception);        
      };      
      
      struct SortByRelevance : public SortUseTime
      {
        uint32_t max_core_words;
        uint32_t impression_respected_level;

        virtual void write(El::BinaryOutStream& bstr) const
          throw(El::Exception);
        
        virtual void read(El::BinaryInStream& bstr) throw(El::Exception);

      protected:
        SortByRelevance(uint32_t message_max_age_val,
                        uint32_t max_core_words_val,
                        uint32_t impression_respected_level_val) throw();
        
        virtual ~SortByRelevance() throw() {}
      };
      
      struct SortByRelevanceDesc : public SortByRelevance
      {
        SortByRelevanceDesc(uint32_t message_max_age_val = 86400,
                            uint32_t max_core_words_val = 20,
                            uint32_t impression_respected_level_val = 100)
          throw();
        
        virtual ~SortByRelevanceDesc() throw() {}

        virtual SortingMode type() throw() { return SM_BY_RELEVANCE_DESC; }

        virtual Sorting* clone() throw(El::Exception);
      };
      
      struct SortByRelevanceAsc : public SortByRelevance
      {
        SortByRelevanceAsc(uint32_t message_max_age_val = 86400,
                           uint32_t max_core_words_val = 20,
                           uint32_t impression_respected_level_val = 100)
          throw();
        
        virtual ~SortByRelevanceAsc() throw() {}
        virtual SortingMode type() throw() { return SM_BY_RELEVANCE_ASC; }
        virtual Sorting* clone() throw(El::Exception);
      };
      
      struct SortByEventCapacity : public SortUseTime
      {
        uint32_t event_max_size;
        uint32_t impression_respected_level;
        
      protected:
        SortByEventCapacity(uint32_t message_max_age_val,
                            uint32_t event_max_size_val,
                            uint32_t impression_respected_level_val = 100)
          throw();
        
        virtual ~SortByEventCapacity() throw() {}

        virtual void write(El::BinaryOutStream& bstr) const
          throw(El::Exception);
        
        virtual void read(El::BinaryInStream& bstr) throw(El::Exception);
      };

      struct SortByEventCapacityDesc : public SortByEventCapacity
      {
        SortByEventCapacityDesc(uint32_t message_max_age_val = 86400,
                                uint32_t event_max_size_val = 10000,
                                uint32_t impression_respected_level_val = 100)
          throw();
        
        virtual ~SortByEventCapacityDesc() throw() {}
        
        virtual SortingMode type() throw() { return SM_BY_EVENT_CAPACITY_DESC;}
        virtual Sorting* clone() throw(El::Exception);        
      };      
      
      struct SortByEventCapacityAsc : public SortByEventCapacity
      {
        SortByEventCapacityAsc(uint32_t message_max_age_val = 86400,
                               uint32_t event_max_size_val = 10000,
                               uint32_t impression_respected_level_val = 100)
          throw();
        
        virtual ~SortByEventCapacityAsc() throw() {}
        virtual SortingMode type() throw() { return SM_BY_EVENT_CAPACITY_ASC; }
        virtual Sorting* clone() throw(El::Exception);        
      };      
      
      struct SortByPopularity : public SortUseTime
      {
        uint32_t impression_respected_level;
        
      protected:
        SortByPopularity(uint32_t message_max_age_val,
                         uint32_t impression_respected_level_val) throw();
        virtual ~SortByPopularity() throw() {}

        virtual void write(El::BinaryOutStream& bstr) const
          throw(El::Exception);
        
        virtual void read(El::BinaryInStream& bstr) throw(El::Exception);
      };

      struct SortByPopularityDesc : public SortByPopularity
      {
        SortByPopularityDesc(uint32_t message_max_age_val = 86400,
                             uint32_t impression_respected_level_val = 100)
          throw();
        
        virtual ~SortByPopularityDesc() throw() {}
        virtual SortingMode type() throw() { return SM_BY_POPULARITY_DESC;}
        virtual Sorting* clone() throw(El::Exception);        
      };      
      
      struct SortByPopularityAsc : public SortByPopularity
      {
        SortByPopularityAsc(uint32_t message_max_age_val = 86400,
                            uint32_t impression_respected_level_val = 100)
          throw();
        
        virtual ~SortByPopularityAsc() throw() {}
        virtual SortingMode type() throw() { return SM_BY_POPULARITY_ASC; }
        virtual Sorting* clone() throw(El::Exception);        
      };      
      
      typedef std::auto_ptr<Sorting> SortingPtr;      

      enum SuppressionType
      {
        ST_NONE,
        ST_DUPLICATES,
        ST_SIMILAR,
        ST_COLLAPSE_EVENTS,
        ST_COUNT
      };

      struct Suppression
      {        
        virtual ~Suppression() throw() {}

        virtual SuppressionType type() throw() = 0;
        virtual Suppression* clone() throw(El::Exception) = 0;
        
        virtual void write(El::BinaryOutStream& bstr) const
          throw(El::Exception) {}
        
        virtual void read(El::BinaryInStream& bstr) throw(El::Exception) {}
      };

      struct SuppressNone : public Suppression
      {
        virtual ~SuppressNone() throw() {}
        virtual SuppressionType type() throw() { return ST_NONE; }
        virtual Suppression* clone() throw(El::Exception);        
      };
      
      struct SuppressDuplicates : public Suppression
      {
        virtual ~SuppressDuplicates() throw() {}
        virtual SuppressionType type() throw() { return ST_DUPLICATES; }
        virtual Suppression* clone() throw(El::Exception);        
      };      
      
      struct SuppressSimilar : public Suppression
      {
        uint32_t similarity_threshold; // 1-100%

        // % of core words of smaller message to be subset of bigger
        // message core words to be suppressed
        uint32_t containment_level;

        // minimal number of words in message to participate in suppression
        uint32_t min_core_words;

        SuppressSimilar(uint32_t similarity_threshold_val = 75,
                        uint32_t containment_level_val = 90,
                        uint32_t min_core_words_val = 4) throw();

        virtual ~SuppressSimilar() throw() {}

        virtual SuppressionType type() throw() { return ST_SIMILAR; }

        virtual Suppression* clone() throw(El::Exception);        

        virtual void write(El::BinaryOutStream& bstr) const
          throw(El::Exception);
        
        virtual void read(El::BinaryInStream& bstr) throw(El::Exception);
      };
      
      struct CollapseEvents : public SuppressSimilar
      {
        uint32_t msg_per_event;
        
        CollapseEvents(uint32_t similarity_threshold_val = 75,
                       uint32_t containment_level_val = 90,
                       uint32_t min_core_words_val = 4,
                       uint32_t msg_per_event_val = 1) throw();

        virtual ~CollapseEvents() throw() {}

        virtual SuppressionType type() throw() { return ST_COLLAPSE_EVENTS; }

        virtual Suppression* clone() throw(El::Exception);        

        virtual void write(El::BinaryOutStream& bstr) const
          throw(El::Exception);
        
        virtual void read(El::BinaryInStream& bstr) throw(El::Exception);
      };
      
      typedef std::auto_ptr<Suppression> SuppressionPtr;
/*
      class SpaceSet :
        public google::dense_hash_set<
        uint8_t,
        El::Hash::Numeric<uint8_t> >
      {
      public:
        SpaceSet() throw(El::Exception);
      };
*/
        
      struct Filter
      {
        El::Lang lang;
        El::Country country;
        Message::StringConstPtr feed;
        Message::StringConstPtr category;
        El::Luid event;
//        SpaceSet spaces;

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);
      };

      enum ResultFlags
      {
        RF_MESSAGES = 0x1,      // Message info should be included into result
        RF_LANG_STAT = 0x2,     // Message-per-language statistics should
                                // be included into result
        RF_COUNTRY_STAT = 0x4,  // Message-per-country statistics should
                                // be included into result
        RF_FEED_STAT = 0x8,     // Message-per-feed statistics should
                                // be included into result
        RF_CATEGORY_STAT = 0x10 // Message-per-category statistics should
                                // be included into result
      };
      
      SortingPtr sorting;
      SuppressionPtr suppression;
      bool search_hidden;
      Filter filter;
      uint32_t result_flags;

      Strategy(Sorting* sorting_val = new SortByPubDateDesc(),
               Suppression* suppression_val = new SuppressDuplicates(),
               bool search_hidden_val = false,
               const Filter& filter_val = Filter(),
               uint32_t result_flags_val =
                 RF_MESSAGES | RF_LANG_STAT | RF_COUNTRY_STAT)
        throw();

      Strategy(const Strategy& val) throw(El::Exception);
      Strategy& operator=(const Strategy& val) throw(El::Exception);

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };

    struct WeightedId
    {
      Message::Id id;
      uint32_t weight;

      static WeightedId zero;
      static WeightedId nonexistent;
        
      WeightedId(const Message::Id& id_val,
                 uint32_t weight_val = 0) throw();

      WeightedId() throw();

      bool operator==(const WeightedId& id) const throw();
      bool operator!=(const WeightedId& id) const throw();
      bool operator<(const WeightedId& id) const throw();

      bool heavier(const WeightedId& val) const throw();

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };

    struct WeightedIdHash
    {
      size_t operator()(const WeightedId& val) const throw();
    };
    
    struct MessageInfo
    {
    private:

      enum MessageInfoFlags
      {
        MIF_OWN_CORE_WORDS = 0x1,
        MIF_HAS_EXTRAS = 0x2
      };
      
      uint8_t flags_;
      uint8_t core_words_count_;
      
    private:
      uint32_t* core_words_;
      
    public:

      struct Extras
      {
        uint32_t state_hash;
        uint64_t published;

        Extras() throw();
        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);
      };

      std::auto_ptr<Extras> extras;
      WeightedId wid;
      El::Luid   event_id;
      Message::Signature signature;
      Message::Signature url_signature;

      MessageInfo() throw();
      MessageInfo(const MessageInfo& src) throw(El::Exception);
      
      ~MessageInfo() throw();

      MessageInfo& operator=(const MessageInfo& src) throw(El::Exception);

      bool operator<(const MessageInfo& mi) const throw();

      void core_words(const Message::CoreWords& core_words, bool copy)
        throw(El::Exception);

      void core_words(const uint32_t* core_words,
                      uint8_t core_words_count,
                      bool copy)
        throw(El::Exception);

      const uint32_t* core_words() const throw();
      unsigned char core_words_count() const throw();

      void own_core_words() throw(El::Exception);

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);

      void steal(MessageInfo& src) throw(El::Exception);
    };
    
    class MessageInfoArray : public std::vector<MessageInfo>
    {
    public:
      MessageInfoArray(unsigned long size) throw(El::Exception);
      MessageInfoArray() throw(El::Exception);
    };

    typedef std::auto_ptr<MessageInfoArray> MessageInfoArrayPtr;

    struct LangHash
    {
      size_t operator()(const El::Lang& val) const throw();      
    };    

    class LangCounterMap :
      public google::dense_hash_map<El::Lang, uint32_t, LangHash>
    {
    public:
      LangCounterMap() throw(El::Exception);
    };
    
    struct CountryHash
    {
      size_t operator()(const El::Country& val) const throw();      
    };    

    class CountryCounterMap :
      public google::dense_hash_map<El::Country, uint32_t, CountryHash>
    {
    public:
      CountryCounterMap() throw(El::Exception);
    };
/*
    class SpaceCounterMap :
      public google::dense_hash_map<
        uint8_t,
        uint32_t,
        El::Hash::Numeric<uint8_t> >
    {
    public:
      SpaceCounterMap() throw(El::Exception);
    };
*/

    struct Counter
    {
      uint32_t count1;
      uint32_t count2;
      Message::SmartStringConstPtr name;
      uint64_t id;

      Counter() throw() : count1(0), count2(0), id(0) {}
      Counter(uint32_t c1, uint32_t c2, const char* n, uint64_t i) throw();

      static const Counter null;
    };
    
    class StringCounterMap :
      public google::dense_hash_map<
      Message::SmartStringConstPtr,
      Counter,
      Message::SmartStringConstPtrHash>
    {
    public:
      StringCounterMap() throw(El::Exception);
      StringCounterMap(const StringCounterMap& val) throw(El::Exception);

      ~StringCounterMap() throw();
      
      void clear() throw(El::Exception);

      StringCounterMap& operator=(const StringCounterMap& val)
        throw(El::Exception);

      void absorb(const StringCounterMap& counter) throw(El::Exception);
      
      void absorb(const Message::StringConstPtr& str,
                  const Message::StringConstPtr& name,
                  uint64_t id,
                  bool inc_sec)
        throw(El::Exception);

      void absorb(const Message::StringConstPtr& str, bool inc_sec)
        throw(El::Exception);

      void absorb(const char* str, uint32_t count) throw(El::Exception);
//      void absorb_sec(const Message::StringConstPtr& str) throw(El::Exception);

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };

    struct Stat
    {
      LangCounterMap lang_counter;
      CountryCounterMap country_counter;
      StringCounterMap feed_counter;
      StringCounterMap category_counter;
//      SpaceCounterMap space_counter;
      
      uint32_t total_messages;
      uint32_t space_filtered;

      Stat() throw(El::Exception) : total_messages(0)/*, space_filtered(0)*/ {}

      void absorb(const Stat& stat) throw(El::Exception);

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };

    struct Result
    {
      MessageInfoArrayPtr message_infos;
      
      uint32_t max_weight;
      uint32_t min_weight;

      Stat stat;
      
      Result() throw(El::Exception);
      Result(unsigned long size) throw(El::Exception);

      void take_top(size_t start_from,
                    size_t results_count,
                    const Strategy& strategy,
                    size_t* suppressed = 0)
        throw(El::Exception);

      enum StateHashFlag
      {
        SHF_STAT = 0x1,
        SHF_EVENT = 0x2,
        SHF_CATEGORY = 0x4,
        SHF_UPDATED = 0x8,
        SHF_ALL = SHF_STAT | SHF_EVENT | SHF_CATEGORY | SHF_UPDATED
      };
      
      void set_extras(const Message::SearcheableMessageMap& messages,
                      unsigned long flags,
                      uint32_t crc_init)
        throw(El::Exception);
      
      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);

    private:

      class SignatureMap :
        public google::sparse_hash_map<Message::Signature,
                                       WeightedId,
                                       El::Hash::Numeric<Message::Signature> >
      {
      public:
        SignatureMap() throw(El::Exception);
      };
      
      class SignatureSet :
        public google::sparse_hash_set<Message::Signature,
                                       El::Hash::Numeric<Message::Signature> >
      {
      public:
        SignatureSet() throw(El::Exception);
      };

      class IdCounterMap :
        public google::dense_hash_map<Message::Id,
                                      uint32_t,
                                      Message::MessageIdHash>
      {
      public:
        IdCounterMap() throw(El::Exception);
      };

      class IdSet :
        public google::dense_hash_set<Message::Id, Message::MessageIdHash>
      {
      public:
        IdSet() throw(El::Exception);
      };

      class WordIdMap :
        public google::dense_hash_map<uint32_t,
                                      IdSet*,
                                      El::Hash::Numeric<uint32_t> >
      {
      public:
        WordIdMap() throw(El::Exception);
        ~WordIdMap() throw();
      };

      class MessageInfoPtrMap :
        public google::dense_hash_map<Message::Id,
                                      const MessageInfo*,
                                      Message::MessageIdHash>
      {
      public:
        MessageInfoPtrMap() throw(El::Exception);
      };

      class MessageInfoConstPtrMap :
        public google::dense_hash_map<Message::Id,
                                      const MessageInfo*,
                                      Message::MessageIdHash>
      {
      public:
        MessageInfoConstPtrMap() throw(El::Exception);
      };

      typedef std::vector<MessageInfoConstPtrMap> MessageInfoMapArray;

      class EventIdCounter :
        public google::dense_hash_map<El::Luid, uint32_t, El::Hash::Luid>
      {
      public:
        EventIdCounter() throw(El::Exception);
      };

      unsigned long get_index(const WeightedId& weighted_id,
                              size_t range_count,
                              uint32_t diapason) const
        throw();

      MessageInfoArray* sort_skip_cut(size_t start_from,
                                      size_t results_count,
                                      const Strategy& strategy,
                                      size_t* suppressed)
        throw(El::Exception);
      
      MessageInfoArray*
      sort_and_cut(size_t start_from,
                   size_t results_count,
                   const Strategy& strategy,
                   size_t* suppressed,
                   IdSet* suppressed_messages) const
        throw(El::Exception);

      bool check_uniqueness(const WeightedId& wid,
                            Message::Signature msg_signature,
                            const SignatureMap& signatures,
                            size_t range_count,
                            unsigned long diapason,
                            MessageInfoMapArray& ranges,
                            size_t* suppressed,
                            IdSet* suppressed_messages) const
        throw(El::Exception);

      static size_t remove_similar(MessageInfoArray* message_infos,
                                   size_t total_required,
                                   const Strategy& strategy,
                                   size_t* suppressed,
                                   IdSet* suppressed_messages)
        throw(InvalidArg, El::Exception);

      static size_t collapse_events(MessageInfoArray* message_infos,
                                    size_t total_required,
                                    const Strategy& strategy,
                                    size_t* suppressed,
                                    IdSet* suppressed_messages)
        throw(InvalidArg, El::Exception);
      
      bool remove_from_ranges(const WeightedId& weighted_id,
                              unsigned long range_count,
                              unsigned long diapason,
                              MessageInfoMapArray& ranges) const
        throw(El::Exception);
      
      bool present_in_ranges(const WeightedId& weighted_id,
                             unsigned long range_count,
                             unsigned long diapason,
                             const MessageInfoMapArray& ranges) const
        throw(El::Exception);
    };

    typedef std::auto_ptr<Result> ResultPtr;

    typedef std::vector<Message::WordPosition> WordPositionArray;

    typedef __gnu_cxx::hash_map<Message::Id,
                                WordPositionArray,
                                Message::MessageIdHash>
    MessageWordPositionMap;
    
    class Expression :
      public virtual El::RefCount::DefaultImpl<El::Sync::ThreadPolicy>
    {
      friend class ExpressionParser;

    public:
      Condition_var condition;
      
    public:
      Expression() throw();
      Expression(Condition* condition) throw();
        
      Result* search(const Message::SearcheableMessageMap& messages,
                     bool copy_msg_struct,
                     const Strategy& strategy = Strategy(),
                     MessageWordPositionMap* mwp_map = 0,
                     time_t* current_time = 0)
        const throw(El::Exception);

      Result* search_simple(
        const Message::SearcheableMessageMap& messages,
        bool copy_msg_struct,
        const Strategy& strategy = Strategy(),
        MessageWordPositionMap* mwp_map = 0,
        time_t* current_time = 0) const
        throw(El::Exception);

      void normalize(
        const El::Dictionary::Morphology::WordInfoManager& word_info_manager)
        throw(El::Exception);

      void dump(std::ostream& ostr) const throw(El::Exception);
      void dump(std::wostream& ostr) const throw(El::Exception);

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);
      
    public:
      virtual ~Expression() throw();

      static El::Stat::TimeMeter search_meter;
      static El::Stat::TimeMeter search_simple_meter;
      static El::Stat::TimeMeter take_top_meter;
      static El::Stat::TimeMeter sort_and_cut_meter;
      static El::Stat::TimeMeter sort_skip_cut_meter;
      static El::Stat::TimeMeter remove_similar_meter;
      static El::Stat::TimeMeter collapse_events_meter;
      static El::Stat::TimeMeter copy_range_meter;

      static El::Stat::TimeMeter remove_similar_p1_meter;
      static El::Stat::TimeMeter remove_similar_p2_meter;
      
      static El::Stat::TimeMeter search_p1_meter;
      static El::Stat::TimeMeter search_p2_meter;

      CONSTEXPR static const float SORT_BY_RELEVANCE_TOPICALITY_WEIGHT = 7;

      static const unsigned long TOPICALITY_DATE_RANGE = 86400 * 7;
      static const uint32_t topicality_[Expression::TOPICALITY_DATE_RANGE];
      
    private:

      Result* search_result(const Condition::Context& context,
                            bool copy_msg_struct,
                            const Condition::Result* cond_res,
                            const Strategy& strategy,
                            const Condition::MessageMatchInfoMap& match_info,
                            MessageWordPositionMap* mwp_map,
                            time_t* current_time)
        const throw(El::Exception);

      Condition* search_condition(const Strategy& strategy) const
        throw(El::Exception);
    };
    
    typedef El::RefCount::SmartPtr<Expression> Expression_var;

    class ExpressionParser
    {
    public:
      ExpressionParser(uint32_t respected_impression_level = 100) throw();
      ~ExpressionParser() throw();

      void parse(std::wistream& istr)
        throw(ParseError, Exception, El::Exception);

      Expression_var& expression() throw();  
      const Expression_var& expression() const throw();
      
      bool contain(Condition::Type type) const throw();

      void optimize() throw(El::Exception);
      
    private:

      enum TokenType
      {
        TT_UNDEFINED,
        TT_REGULAR,
        TT_ALL,
        TT_ANY,
        TT_SITE,
        TT_URL,
        TT_CATEGORY,
        TT_MSG,
        TT_EVENT,
        TT_EVERY,
        TT_AND,
        TT_EXCEPT,
        TT_OR,
        TT_OPEN,
        TT_CLOSE,
        TT_LANG,
        TT_COUNTRY,
        TT_SPACE,
        TT_DOMAIN,
        TT_CAPACITY,
        TT_PUB_DATE,
        TT_FETCHED,
        TT_NOT,
        TT_BEFORE,
        TT_WITH,
        TT_NO,
        TT_IMAGE,
        TT_SIGNATURE,
        TT_NONE,
        TT_IMPRESSIONS,
        TT_CLICKS,
        TT_CTR,
        TT_RCTR,
        TT_FEED_IMPRESSIONS,
        TT_FEED_CLICKS,
        TT_FEED_CTR,
        TT_FEED_RCTR,
        TT_CORE,
        TT_TITLE,
        TT_DESC,
        TT_IMG_ALT,
        TT_KEYWORDS,
        TT_VISITED
      };

      static const TokenType SPECIAL_WORD_TYPE[];
      
      Condition* read_condition(std::wistream& istr, Condition* first_operand)
        throw(ParseError, Exception, El::Exception);

      TokenType read_token(std::wistream& istr)
        throw(ParseError, Exception, El::Exception);
      
      void read_word_modifiers(std::wistream& istr,
                               uint32_t& flags,
                               uint32_t* match_threshold)
        throw(ParseError, Exception, El::Exception);
      
      bool read_words(std::wistream& istr, WordList& words)
        throw(ParseError, Exception, El::Exception);

      bool push_word(std::wstring& word,
                     WordList& words,
                     unsigned char group,
                     bool exact)
        throw(El::Exception);
      
      void put_back() throw(Exception, El::Exception);
      
      Condition* read_operand(std::wistream& istr)
        throw(ParseError, Exception, El::Exception);
      
      Condition* read_expression(std::wistream& istr, bool nested)
        throw(ParseError, Exception, El::Exception);

      Condition* read_all(std::wistream& istr)
        throw(ParseError, Exception, El::Exception);
      
      Condition* read_any(std::wistream& istr)
        throw(ParseError, Exception, El::Exception);
      
      Condition* read_site(std::wistream& istr)
        throw(ParseError, Exception, El::Exception);

      Condition* read_url(std::wistream& istr)
        throw(ParseError, Exception, El::Exception);
      
      Condition* read_category(std::wistream& istr)
        throw(ParseError, Exception, El::Exception);
      
      Condition* read_msg(std::wistream& istr)
        throw(ParseError, Exception, El::Exception);
      
      Condition* read_event(std::wistream& istr)
        throw(ParseError, Exception, El::Exception);
      
      Condition* read_every(std::wistream& istr)
        throw(ParseError, Exception, El::Exception);
      
      Condition* read_none(std::wistream& istr)
        throw(ParseError, Exception, El::Exception);
      
      Condition* read_and(std::wistream& istr, Condition* left_op)
        throw(ParseError, Exception, El::Exception);
      
      Condition* read_except(std::wistream& istr, Condition* left_op)
        throw(ParseError, Exception, El::Exception);
      
      Condition* read_or(std::wistream& istr, Condition* left_op)
        throw(ParseError, Exception, El::Exception);      

      Condition* read_fetched(std::wistream& istr, Condition* left_op)
        throw(ParseError, Exception, El::Exception);

      Condition* read_visited(std::wistream& istr, Condition* left_op)
        throw(ParseError, Exception, El::Exception);

      Condition* read_pub_date(std::wistream& istr, Condition* left_op)
        throw(ParseError, Exception, El::Exception);

      Condition* read_lang(std::wistream& istr, Condition* left_op)
        throw(ParseError, Exception, El::Exception);

      Condition* read_country(std::wistream& istr, Condition* left_op)
        throw(ParseError, Exception, El::Exception);

      Condition* read_space(std::wistream& istr,
                            Condition* left_op)
        throw(ParseError, Exception, El::Exception);

      Condition* read_domain(std::wistream& istr, Condition* left_op)
        throw(ParseError, Exception, El::Exception);

      Condition* read_capacity(std::wistream& istr, Condition* left_op)
        throw(ParseError, Exception, El::Exception);

      Condition* read_impressions(std::wistream& istr, Condition* left_op)
        throw(ParseError, Exception, El::Exception);

      Condition* read_feed_impressions(std::wistream& istr, Condition* left_op)
        throw(ParseError, Exception, El::Exception);

      Condition* read_clicks(std::wistream& istr, Condition* left_op)
        throw(ParseError, Exception, El::Exception);

      Condition* read_feed_clicks(std::wistream& istr, Condition* left_op)
        throw(ParseError, Exception, El::Exception);

      Condition* read_ctr(std::wistream& istr, Condition* left_op)
        throw(ParseError, Exception, El::Exception);

      Condition* read_feed_ctr(std::wistream& istr, Condition* left_op)
        throw(ParseError, Exception, El::Exception);

      Condition* read_rctr(std::wistream& istr, Condition* left_op)
        throw(ParseError, Exception, El::Exception);

      Condition* read_feed_rctr(std::wistream& istr, Condition* left_op)
        throw(ParseError, Exception, El::Exception);

      Condition* read_signature(std::wistream& istr, Condition* left_op)
        throw(ParseError, Exception, El::Exception);

      Condition* read_with(std::wistream& istr, Condition* left_op)
        throw(ParseError, Exception, El::Exception);

      static bool is_operation(TokenType type) throw();
      static bool is_operand(TokenType type) throw();
      static bool is_filter(TokenType type) throw();

    private:
      
      size_t position_;
      size_t last_token_begins_;
      std::wstring last_token_;
      TokenType last_token_type_;
      wchar_t last_token_quoted_;
      bool buffered_;
      uint64_t condition_flags_;
      uint32_t respected_impression_level_;

      Expression_var expression_;
    };
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Search
  {
    //
    // Strategy::SortNone struct
    //
    inline
    Strategy::Sorting*
    Strategy::SortNone::clone() throw(El::Exception)
    {
      return new SortNone();
    }
        
    //
    // Strategy::SortByPubDateDesc struct
    //
    inline
    Strategy::SortByPubDateDesc::SortByPubDateDesc(
      uint32_t message_max_age_val) throw()
        : SortUseTime(message_max_age_val)
    {
    }
    
    inline
    Strategy::Sorting* 
    Strategy::SortByPubDateDesc::clone() throw(El::Exception)
    {
      return new SortByPubDateDesc(message_max_age);
    }
        
    //
    // Strategy::SortByPubDateAcs struct
    //
    inline
    Strategy::SortByPubDateAcs::SortByPubDateAcs(uint32_t message_max_age_val)
      throw() : SortUseTime(message_max_age_val)
    {
    }
    
    inline
    Strategy::Sorting* 
    Strategy::SortByPubDateAcs::clone() throw(El::Exception)
    {
      return new SortByPubDateAcs(message_max_age);
    }
        
    //
    // Strategy::SortByFetchDateDesc struct
    //
    inline
    Strategy::SortByFetchDateDesc::SortByFetchDateDesc(
      uint32_t message_max_age_val) throw()
        : SortUseTime(message_max_age_val)
    {
    }

    inline
    Strategy::Sorting* 
    Strategy::SortByFetchDateDesc::clone() throw(El::Exception)
    {
      return new SortByFetchDateDesc(message_max_age);
    }
        
    //
    // Strategy::SortByFetchDateAcs struct
    //
    inline
    Strategy::SortByFetchDateAcs::SortByFetchDateAcs(
      uint32_t message_max_age_val) throw()
        : SortUseTime(message_max_age_val)
    {
    }

    inline
    Strategy::Sorting* 
    Strategy::SortByFetchDateAcs::clone() throw(El::Exception)
    {
      return new SortByFetchDateAcs(message_max_age);
    }
    
    //
    // Strategy::SortUseTime struct
    //
    inline
    Strategy::SortUseTime::SortUseTime(uint32_t message_max_age_val) throw()
        : message_max_age(message_max_age_val)
    {
    }

    inline
    void
    Strategy::SortUseTime::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr << message_max_age;
    }
        
    inline
    void
    Strategy::SortUseTime::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> message_max_age;
    }
    
    //
    // Strategy::SortByRelevance struct
    //
    inline
    Strategy::SortByRelevance::SortByRelevance(
      uint32_t message_max_age_val,
      uint32_t max_core_words_val,
      uint32_t impression_respected_level_val) throw()
        : SortUseTime(message_max_age_val),
          max_core_words(max_core_words_val),
          impression_respected_level(impression_respected_level_val)
    {
    }

    inline
    void
    Strategy::SortByRelevance::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      SortUseTime::write(bstr);
      bstr << max_core_words << impression_respected_level;
    }
        
    inline
    void
    Strategy::SortByRelevance::read(El::BinaryInStream& bstr)
      throw(El::Exception)
    {
      SortUseTime::read(bstr);
      bstr >> max_core_words >> impression_respected_level;
    }
    
    //
    // Strategy::SortByRelevanceDesc struct
    //
    inline
    Strategy::SortByRelevanceDesc::SortByRelevanceDesc(
      uint32_t message_max_age_val,
      uint32_t max_core_words_val,
      uint32_t impression_respected_level_val)
      throw()
        : SortByRelevance(message_max_age_val,
                          max_core_words_val,
                          impression_respected_level_val)
    {
    }
        
    inline
    Strategy::Sorting*
    Strategy::SortByRelevanceDesc::clone() throw(El::Exception)
    {
      return new SortByRelevanceDesc(message_max_age,
                                     max_core_words,
                                     impression_respected_level);
    }

    //
    // Strategy::SortByRelevanceAsc struct
    //
    inline
    Strategy::SortByRelevanceAsc::SortByRelevanceAsc(
      uint32_t message_max_age_val,
      uint32_t max_core_words_val,
      uint32_t impression_respected_level_val)
      throw()
        : SortByRelevance(message_max_age_val,
                          max_core_words_val,
                          impression_respected_level)
    {
    }
    
    inline
    Strategy::Sorting*
    Strategy::SortByRelevanceAsc::clone() throw(El::Exception)
    {
      return new SortByRelevanceAsc(message_max_age,
                                    max_core_words,
                                    impression_respected_level);
    }

    //
    // Strategy::SortByEventCapacity struct
    //
    inline
    Strategy::SortByEventCapacity::SortByEventCapacity(
      uint32_t message_max_age_val,
      uint32_t event_max_size_val,
      uint32_t impression_respected_level_val) throw()
        : SortUseTime(message_max_age_val),
          event_max_size(event_max_size_val),
          impression_respected_level(impression_respected_level_val)
    {
    }

    inline
    void
    Strategy::SortByEventCapacity::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      SortUseTime::write(bstr);
      bstr << event_max_size << impression_respected_level;
    }

    inline
    void
    Strategy::SortByEventCapacity::read(El::BinaryInStream& bstr)
      throw(El::Exception)
    {
      SortUseTime::read(bstr);
      bstr >> event_max_size >> impression_respected_level;
    }
    
    //
    // Strategy::SortByEventCapacityDesc struct
    //
    inline
    Strategy::SortByEventCapacityDesc::SortByEventCapacityDesc(
      uint32_t message_max_age_val,
      uint32_t event_max_size_val,
      uint32_t impression_respected_level_val) throw()
        : SortByEventCapacity(message_max_age_val,
                              event_max_size_val,
                              impression_respected_level_val)
    {
    }
    
    inline
    Strategy::Sorting*
    Strategy::SortByEventCapacityDesc::clone() throw(El::Exception)
    {
      return new SortByEventCapacityDesc(message_max_age, event_max_size);
    }

    //
    // Strategy::SortByEventCapacityAsc struct
    //
    inline
    Strategy::SortByEventCapacityAsc::SortByEventCapacityAsc(
      uint32_t message_max_age_val,
      uint32_t event_max_size_val,
      uint32_t impression_respected_level_val) throw()
        : SortByEventCapacity(message_max_age_val,
                              event_max_size_val,
                              impression_respected_level_val)
    {
    }
    
    inline
    Strategy::Sorting*
    Strategy::SortByEventCapacityAsc::clone() throw(El::Exception)
    {
      return new SortByEventCapacityAsc(message_max_age, event_max_size);
    }

    //
    // Strategy::SortByPopularity struct
    //
    inline
    Strategy::SortByPopularity::SortByPopularity(
      uint32_t message_max_age_val,
      uint32_t impression_respected_level_val) throw()
        : SortUseTime(message_max_age_val),
          impression_respected_level(impression_respected_level_val)
    {
    }
    
    inline
    void
    Strategy::SortByPopularity::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      SortUseTime::write(bstr);
      bstr << impression_respected_level;
    }

    inline
    void
    Strategy::SortByPopularity::read(El::BinaryInStream& bstr)
      throw(El::Exception)
    {
      SortUseTime::read(bstr);
      bstr >> impression_respected_level;
    }
    
    //
    // Strategy::SortByPopularityDesc struct
    //
    inline
    Strategy::SortByPopularityDesc::SortByPopularityDesc(
      uint32_t message_max_age_val,
      uint32_t impression_respected_level_val) throw()
        : SortByPopularity(message_max_age_val,
                           impression_respected_level_val)
    {
    }
    
    inline
    Strategy::Sorting*
    Strategy::SortByPopularityDesc::clone() throw(El::Exception)
    {
      return new SortByPopularityDesc(message_max_age,
                                      impression_respected_level);
    }

    //
    // Strategy::SortByPopularityAsc struct
    //
    inline
    Strategy::SortByPopularityAsc::SortByPopularityAsc(
      uint32_t message_max_age_val,
      uint32_t impression_respected_level_val) throw()
        : SortByPopularity(message_max_age_val,
                           impression_respected_level_val)
    {
    }
    
    inline
    Strategy::Sorting*
    Strategy::SortByPopularityAsc::clone() throw(El::Exception)
    {
      return new SortByPopularityAsc(message_max_age,
                                     impression_respected_level);
    }

    //
    // Strategy::SuppressNone struct
    //
    inline
    Strategy::Suppression* 
    Strategy::SuppressNone::clone() throw(El::Exception)
    {
      return new SuppressNone();
    }
        
    //
    // Strategy::SuppressDuplicates struct
    //
    inline
    Strategy::Suppression* 
    Strategy::SuppressDuplicates::clone() throw(El::Exception)
    {
      return new SuppressDuplicates();
    }
        
    //
    // Strategy::SuppressSimilar struct
    //
    inline
    Strategy::SuppressSimilar::SuppressSimilar(
      uint32_t similarity_threshold_val,
      uint32_t containment_level_val,
      uint32_t min_core_words_val) throw()
        : similarity_threshold(similarity_threshold_val),
          containment_level(containment_level_val),
          min_core_words(min_core_words_val)
    {
    }

    inline
    void
    Strategy::SuppressSimilar::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr << similarity_threshold << containment_level << min_core_words;
    }
        
    inline
    void
    Strategy::SuppressSimilar::read(El::BinaryInStream& bstr)
      throw(El::Exception)
    {
      bstr >> similarity_threshold >> containment_level >> min_core_words;
    }
    
    inline
    Strategy::Suppression*
    Strategy::SuppressSimilar::clone() throw(El::Exception)
    {
      return new SuppressSimilar(similarity_threshold,
                                 containment_level,
                                 min_core_words);
    }    
    
    //
    // Strategy::CollapseEvents struct
    //
    inline
    Strategy::CollapseEvents::CollapseEvents(
      uint32_t similarity_threshold_val,
      uint32_t containment_level_val,
      uint32_t min_core_words_val,
      uint32_t msg_per_event_val) throw()
        : SuppressSimilar(similarity_threshold_val,
                          containment_level_val,
                          min_core_words_val),
          msg_per_event(msg_per_event_val)
    {
    }

    inline
    void
    Strategy::CollapseEvents::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      SuppressSimilar::write(bstr);
      bstr << msg_per_event;
    }
        
    inline
    void
    Strategy::CollapseEvents::read(El::BinaryInStream& bstr)
      throw(El::Exception)
    {
      SuppressSimilar::read(bstr);      
      bstr >> msg_per_event;
    }
    
    inline
    Strategy::Suppression*
    Strategy::CollapseEvents::clone() throw(El::Exception)
    {
      return new CollapseEvents(similarity_threshold,
                                containment_level,
                                min_core_words,
                                msg_per_event);
    }
/*
    //
    // Strategy::SpaceSet struct
    //
    inline
    Strategy::SpaceSet::SpaceSet() throw(El::Exception)
    {
      set_empty_key(::NewsGate::Feed::SP_SPACES_COUNT);
      set_deleted_key(::NewsGate::Feed::SP_NONEXISTENT);
    }
*/  
    //
    // Strategy::Filter struct
    //
    inline
    void
    Strategy::Filter::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr << lang << country << feed << category << event;
//      bstr.write_set(spaces);
    }

    inline
    void
    Strategy::Filter::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> lang >> country >> feed >> category >> event;
//      bstr.read_set(spaces);
    }
    
    //
    // Strategy struct
    //
    inline
    Strategy::Strategy(Sorting* sorting_val,
                       Suppression* suppression_val,
                       bool search_hidden_val,
                       const Filter& filter_val,
                       uint32_t result_flags_val) throw()
        : sorting(sorting_val),
          suppression(suppression_val),
          search_hidden(search_hidden_val),
          filter(filter_val),
          result_flags(result_flags_val)
    {
    }
    
    inline
    Strategy::Strategy(const Strategy& val) throw(El::Exception)
        : sorting(val.sorting->clone()),
          suppression(val.suppression->clone()),
          search_hidden(val.search_hidden),
          filter(val.filter),
          result_flags(val.result_flags)
    {
    }
    
    inline
    Strategy&
    Strategy::operator=(const Strategy& val) throw(El::Exception)
    {
      sorting.reset(val.sorting->clone());
      suppression.reset(val.suppression->clone());
      search_hidden = val.search_hidden;
      filter = val.filter;
      result_flags = val.result_flags;
      return *this;
    }

    inline
    void
    Strategy::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << (uint8_t)sorting->type() << *sorting
           << (uint8_t)suppression->type() << *suppression
           << (uint8_t)search_hidden << filter << result_flags;
    }
    
    inline
    void
    Strategy::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      uint8_t sm;
      bstr >> sm;

      switch(sm)
      {
      case SM_BY_RELEVANCE_DESC:
        {
          sorting.reset(new SortByRelevanceDesc());
          break;
        }
      case SM_BY_RELEVANCE_ASC:
        {
          sorting.reset(new SortByRelevanceAsc());
          break;
        }
      case SM_BY_PUB_DATE_DESC:
        {
          sorting.reset(new SortByPubDateDesc());
          break;
        }
      case SM_BY_FETCH_DATE_DESC:
        {
          sorting.reset(new SortByFetchDateDesc());
          break;
        }
      case SM_BY_PUB_DATE_ASC:
        {
          sorting.reset(new SortByPubDateAcs());
          break;
        }
      case SM_BY_FETCH_DATE_ASC:
        {
          sorting.reset(new SortByFetchDateAcs());
          break;
        }
      case SM_BY_EVENT_CAPACITY_DESC:
        {
          sorting.reset(new SortByEventCapacityDesc());
          break;
        }
      case SM_BY_EVENT_CAPACITY_ASC:
        {
          sorting.reset(new SortByEventCapacityAsc());
          break;
        }
      case SM_BY_POPULARITY_DESC:
        {
          sorting.reset(new SortByPopularityDesc());
          break;
        }
      case SM_BY_POPULARITY_ASC:
        {
          sorting.reset(new SortByPopularityAsc());
          break;
        }
      case SM_NONE: 
        {
          sorting.reset(new SortNone());
          break;
        }
      }
      
      bstr >> *sorting >> sm;

      switch(sm)
      {
      case ST_NONE: 
        {
          suppression.reset(new SuppressNone());
          break;
        }
      case ST_DUPLICATES: 
        {
          suppression.reset(new SuppressDuplicates());
          break;
        }
      case ST_SIMILAR: 
        {
          suppression.reset(new SuppressSimilar());
          break;
        }
      case ST_COLLAPSE_EVENTS: 
        {
          suppression.reset(new CollapseEvents());
          break;
        }
      }
      
      uint8_t hw;

      bstr >> *suppression >> hw >> filter >> result_flags;
      search_hidden = hw;
    }
    
    //
    // MessageInfo class
    //

    inline
    MessageInfo::MessageInfo() throw()
        : flags_(0),
          core_words_count_(0),
          core_words_(0),
          signature(0),
          url_signature(0)
    {
    }

    inline
    MessageInfo::MessageInfo(const MessageInfo& src) throw(El::Exception)
        : flags_(0),
          core_words_count_(0),
          core_words_(0),
          extras(src.extras.get() ? new Extras(*src.extras) : 0),
          wid(src.wid),
          event_id(src.event_id),
          signature(src.signature),
          url_signature(src.url_signature)
    {
      if(src.core_words_)
      {
        core_words(src.core_words_,
                   src.core_words_count_,
                   (src.flags_ & MIF_OWN_CORE_WORDS) != 0);
      }
    }

    inline
    MessageInfo::~MessageInfo() throw()
    {
      if(flags_ & MIF_OWN_CORE_WORDS)
      {
        delete [] core_words_;
      }
    }
    
    inline
    MessageInfo&
    MessageInfo::operator=(const MessageInfo& src)
      throw(El::Exception)
    {
      event_id = src.event_id;
      wid = src.wid;
      signature = src.signature;
      url_signature = src.url_signature;

      core_words(src.core_words_,
                 src.core_words_count_,
                 (src.flags_ & MIF_OWN_CORE_WORDS) != 0);

      extras.reset(src.extras.get() ? new Extras(*src.extras) : 0);
      return *this;
    }

    inline
    bool
    MessageInfo::operator<(const MessageInfo& val) const throw()
    {
      return wid.weight > val.wid.weight ||
        (wid.weight == val.wid.weight && wid.id > val.wid.id);
    }

    inline
    void
    MessageInfo::steal(MessageInfo& src) throw(El::Exception)
    {
      wid = src.wid;
      event_id = src.event_id;
      signature = src.signature;
      url_signature = src.url_signature;

      if(flags_ & MIF_OWN_CORE_WORDS)
      {
        delete [] core_words_;
      }

      flags_ = src.flags_;
      core_words_count_ = src.core_words_count_;
      core_words_ = src.core_words_;

      src.core_words_ = 0;
      src.core_words_count_ = 0;

      extras.reset(src.extras.release());
    }
    
    inline
    void
    MessageInfo::core_words(const Message::CoreWords& words, bool copy)
      throw(El::Exception)
    {
      core_words(words.elements(), words.size(), copy);
    }
    
    inline
    void
    MessageInfo::core_words(const uint32_t* core_words,
                            unsigned char core_words_count,
                            bool copy)
      throw(El::Exception)
    {
      if(flags_ & MIF_OWN_CORE_WORDS)
      {
        delete [] core_words_;
      }

      core_words_count_ = core_words_count;

      if(core_words_count)
      {
        if(copy)
        {    
          core_words_ = new uint32_t[core_words_count];
          
          memcpy(core_words_,
                 core_words,
                 ((uint32_t)core_words_count) * sizeof(uint32_t));
        
          flags_ |= MIF_OWN_CORE_WORDS;
        }
        else
        {
          core_words_ = const_cast<uint32_t*>(core_words);
          flags_ &= ~MIF_OWN_CORE_WORDS;
        }
      }
      else
      {
        core_words_ = 0;
        flags_ |= MIF_OWN_CORE_WORDS;
      }
    }

    inline
    const uint32_t*
    MessageInfo::core_words() const throw()
    {
      return core_words_;
    }
    
    inline
    unsigned char
    MessageInfo::core_words_count() const throw()
    {
      return core_words_count_;
    }

    inline
    void
    MessageInfo::own_core_words() throw(El::Exception)
    {
      if((flags_ & MIF_OWN_CORE_WORDS) == 0)
      {
        core_words(core_words_, core_words_count_, true);
      }
    }

    inline
    void
    MessageInfo::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      uint8_t flags = flags_;

      if(extras.get())
      {
        flags |= MIF_HAS_EXTRAS;
      }
      
      bstr << wid << signature << url_signature << event_id << flags;

      if(flags & MIF_HAS_EXTRAS)
      {
        bstr << *extras;
      }
      
      bstr << core_words_count_;

      for(size_t i = 0; i < core_words_count_; ++i)
      {
        bstr << core_words_[i];
      }
    }
    
      
    inline
    void
    MessageInfo::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      if(flags_ & MIF_OWN_CORE_WORDS)
      {
        delete [] core_words_;
        core_words_ = 0;
      }

      uint8_t flags = 0;
      bstr >> wid >> signature >> url_signature >> event_id >> flags;
      
      if(flags & MIF_HAS_EXTRAS)
      {
        flags &= ~MIF_HAS_EXTRAS;

        if(extras.get() == 0)
        {
          extras.reset(new Extras());
        }
        
        bstr >> *extras;
      }
      else
      {
        extras.reset(0);
      }

      flags_ = flags | MIF_OWN_CORE_WORDS;
      bstr >> core_words_count_;
      
      core_words_ =
        core_words_count_ ? new uint32_t[core_words_count_] : 0;

      for(size_t i = 0; i < core_words_count_; ++i)
      {
        bstr >> core_words_[i];
      }
    }

    //
    // MessageInfo::Extras class
    //
    inline
    MessageInfo::Extras::Extras() throw() : state_hash(0), published(0)
    {
    }
    
    inline
    void
    MessageInfo::Extras::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr << state_hash << published;
    }

    inline
    void
    MessageInfo::Extras::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> state_hash >> published;
    }
    
    //
    // MessageInfo class
    //
    inline
    MessageInfoArray::MessageInfoArray(unsigned long size)
      throw(El::Exception)
        : std::vector<MessageInfo>(size)
    {
    }
    
    inline
    MessageInfoArray::MessageInfoArray() throw(El::Exception)
    {
    }

    //
    // LangHash class
    //
    inline
    size_t
    LangHash::operator()(const El::Lang& val) const throw()
    {
      return val.el_code();
    }

    //
    // LangCounterMap class
    //
    inline
    LangCounterMap::LangCounterMap() throw(El::Exception)
    {
      set_empty_key(El::Lang::nonexistent);
      set_deleted_key(El::Lang::nonexistent2);
    }

    //
    // CountryHash class
    //
    inline
    size_t
    CountryHash::operator()(const El::Country& val) const throw()
    {
      return val.el_code();
    }

    //
    // CountryCounterMap class
    //
    inline
    CountryCounterMap::CountryCounterMap() throw(El::Exception)
    {
      set_empty_key(El::Country::nonexistent);
      set_deleted_key(El::Country::nonexistent2);
    }

    //
    // CountryCounterMap class
    //
/*    
    inline
    SpaceCounterMap::SpaceCounterMap() throw(El::Exception)
    {
      set_empty_key(::NewsGate::Feed::SP_SPACES_COUNT);
      set_deleted_key(::NewsGate::Feed::SP_NONEXISTENT);
    }
*/

    //
    // CountryCounterMap class
    //

    inline
    void
    Stat::absorb(const Stat& stat) throw(El::Exception)
    {
      total_messages += stat.total_messages;
//      space_filtered += stat.space_filtered;

      if(lang_counter.empty())
      {
        lang_counter.insert(stat.lang_counter.begin(),
                            stat.lang_counter.end());
      }
      else
      {
        for(Search::LangCounterMap::const_iterator
              i(stat.lang_counter.begin()), e(stat.lang_counter.end());
            i != e; ++i)
        {
          Search::LangCounterMap::iterator j = lang_counter.find(i->first);
          
          if(j == lang_counter.end())
          {
            lang_counter.insert(*i);
          }
          else
          {
            j->second += i->second;
          }
        }
      }

      if(country_counter.empty())
      {
        country_counter.insert(stat.country_counter.begin(),
                               stat.country_counter.end());
      }
      else
      {
        for(Search::CountryCounterMap::const_iterator
              i(stat.country_counter.begin()), e(stat.country_counter.end());
            i != e; ++i)
        {
          Search::CountryCounterMap::iterator j =
            country_counter.find(i->first);
          
          if(j == country_counter.end())
          {
            country_counter.insert(*i);
          }
          else
          {
            j->second += i->second;
          }
        }
      }
/*
      if(space_counter.empty())
      {
        space_counter.insert(stat.space_counter.begin(),
                             stat.space_counter.end());
      }
      else
      {
        for(Search::SpaceCounterMap::const_iterator
              i(stat.space_counter.begin()), e(stat.space_counter.end());
            i != e; ++i)
        {
          Search::SpaceCounterMap::iterator j = space_counter.find(i->first);
          
          if(j == space_counter.end())
          {
            space_counter.insert(*i);
          }
          else
          {
            j->second += i->second;
          }
        }
      }
*/
      
      feed_counter.absorb(stat.feed_counter);        
      category_counter.absorb(stat.category_counter);
    }
    
    inline
    void
    Stat::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr.write_map(lang_counter);
      bstr.write_map(country_counter);
//      bstr.write_map(space_counter);
      
      bstr << feed_counter << category_counter << total_messages/*
                                                                  << space_filtered*/;
    }
    
    inline
    void
    Stat::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr.read_map(lang_counter);
      bstr.read_map(country_counter);
//      bstr.read_map(space_counter);
      
      bstr >> feed_counter >> category_counter >> total_messages/*
                                                                  >> space_filtered*/;
    }

    //
    // Counter class
    //
    inline
    Counter::Counter(uint32_t c1, uint32_t c2, const char* n, uint64_t i)
      throw() : count1(c1), count2(c2), name(n), id(i)
    {
    }
    
    //
    // StringCounterMap class
    //
    inline
    StringCounterMap::StringCounterMap() throw(El::Exception)
    {
      set_deleted_key(0);
      set_empty_key((const char*)0x1);
    }
    
    inline
    StringCounterMap::StringCounterMap(const StringCounterMap& val)
      throw(El::Exception)
    {
      set_deleted_key(0);
      set_empty_key((const char*)0x1);
      *this = val;
    }
    
    inline
    StringCounterMap::~StringCounterMap() throw()
    {
      clear();
    }

    inline
    void
    StringCounterMap::absorb(const Message::StringConstPtr& str,
                             const Message::StringConstPtr& name,
                             uint64_t id,
                             bool inc_sec)
      throw(El::Exception)
    {
      std::pair<StringCounterMap::iterator, bool> res =
        insert(std::make_pair(str.c_str(), Counter(0, 0, name.c_str(), id)));

      if(res.second)
      {
        str.add_ref();
        name.add_ref();
      }
      
      Counter& counter = res.first->second;
      
      ++counter.count1;

      if(inc_sec)
      {
        ++counter.count2;
      }
    }

    inline
    void
    StringCounterMap::absorb(const Message::StringConstPtr& str, bool inc_sec)
      throw(El::Exception)
    {
      std::pair<StringCounterMap::iterator, bool> res =
        insert(std::make_pair(str.c_str(), Counter::null));

      if(res.second)
      {
        str.add_ref();
      }
      
      Counter& counter = res.first->second;
      
      ++counter.count1;

      if(inc_sec)
      {
        ++counter.count2;
      }
    }

    inline
    void
    StringCounterMap::absorb(const char* str, uint32_t count)
      throw(El::Exception)
    {
      std::pair<StringCounterMap::iterator, bool> res =
        insert(std::make_pair(str, Counter::null));

      if(res.second)
      {
        Message::StringConstPtr::string_manager.add_ref(str);
      }
      
      res.first->second.count1 += count;      
    }
/*
    inline
    void
    StringCounterMap::absorb_sec(const Message::StringConstPtr& str)
      throw(El::Exception)
    {
      std::pair<StringCounterMap::iterator, bool> res =
        insert(std::make_pair(str.c_str(), Counter::null));

      if(res.second)
      {
        str.add_ref();
      }
      
      ++(res.first->second.count2);
    }
*/
    inline
    void
    StringCounterMap::absorb(const StringCounterMap& counter)
      throw(El::Exception)
    {
      for(StringCounterMap::const_iterator i(counter.begin()),
            e(counter.end()); i != e; ++i)
      {
        StringCounterMap::iterator cit = find(i->first);
        const Counter cnt = i->second;

        if(cit == end())
        {
          insert(
            std::make_pair(
              Message::StringConstPtr::string_manager.add_ref(
                i->first.c_str()),
              Counter(cnt.count1,
                      cnt.count2,
                      Message::StringConstPtr::string_manager.add_ref(
                        cnt.name.c_str()),
                      cnt.id)));
        }
        else
        {
          cit->second.count1 += cnt.count1;
          cit->second.count2 += cnt.count2;
        }
      }
    }

    inline
    void
    StringCounterMap::clear() throw(El::Exception)
    {
      for(iterator i(begin()), e(end()); i != e; ++i)
      {
        Message::StringConstPtr::string_manager.remove(i->first.c_str());
        Message::StringConstPtr::string_manager.remove(i->second.name.c_str());
      }

      google::dense_hash_map<Message::SmartStringConstPtr,
        Counter,
        Message::SmartStringConstPtrHash>::clear();
    }

    inline
    StringCounterMap&
    StringCounterMap::operator=(const StringCounterMap& val)
      throw(El::Exception)
    {
      clear();

      for(const_iterator i(val.begin()), e(val.end()); i != e; ++i)
      {
        const Counter& counter = i->second;
        
        insert(
          std::make_pair(
            Message::StringConstPtr::string_manager.add_ref(i->first.c_str()),
            Counter(counter.count1,
                    counter.count2,
                    Message::StringConstPtr::string_manager.add_ref(
                      counter.name.c_str()),
                    counter.id)));
      }

      return *this;
    }
    
    inline
    void
    StringCounterMap::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr << (uint32_t)size();

      for(const_iterator i(begin()), e(end()); i != e; ++i)
      {
        Message::StringConstPtr::string_manager.write_string(bstr,
                                                             i->first.c_str());
        
        const Counter& counter = i->second;
        bstr << counter.count1 << counter.count2;

        Message::StringConstPtr::string_manager.write_string(
          bstr,
          counter.name.c_str());

        bstr << counter.id;
      }
    }
    
    inline
    void
    StringCounterMap::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      uint32_t count = 0;
      bstr >> count;

      for(uint32_t i = 0; i < count; i++)
      {
        Message::StringConstPtr key;
        bstr >> key;

        uint32_t counter1 = 0;
        uint32_t counter2 = 0;
        Message::StringConstPtr name;
        uint64_t id = 0;
        
        bstr >> counter1 >> counter2 >> name >> id;
        
        insert(std::make_pair(key.release(),
                              Counter(counter1,
                                      counter2,
                                      name.release(),
                                      id)));
      }
    }
    
    //
    // WeightedId class
    //
    inline
    WeightedId::WeightedId(const Message::Id& id_val,
                           uint32_t weight_val) throw()
        : id(id_val), weight(weight_val)
    {
    }
    
    inline
    WeightedId::WeightedId() throw() : weight(0)
    {
    }
    
    inline
    bool
    WeightedId::operator==(const WeightedId& val) const throw()
    {
      return id == val.id;
    }
    
    inline
    bool
    WeightedId::operator!=(const WeightedId& val) const throw()
    {
      return id != val.id;
    }

    inline
    bool
    WeightedId::operator<(const WeightedId& val) const throw()
    {
      return id < val.id;
    }

    inline
    bool
    WeightedId::heavier(const WeightedId& val) const throw()
    {
      return weight > val.weight || (weight == val.weight && id > val.id);
    }

    inline
    void
    WeightedId::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << id << weight;
    }
    
    inline
    void
    WeightedId::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> id >> weight;
    }
    
    //
    // Results::WeightedIdHash class
    //
    inline
    size_t
    WeightedIdHash::operator()(const WeightedId& val) const throw()
    {
      return val.id.data;
    }

    //
    // Results::IdSet class
    //
    inline
    Result::IdSet::IdSet() throw(El::Exception)
    {
      set_deleted_key(Message::Id::zero);
      set_empty_key(Message::Id::nonexistent);
    }

    //
    // Results::SignatureMap class
    //
    inline
    Result::SignatureMap::SignatureMap() throw(El::Exception)
    {
      set_deleted_key(0);
    }

    //
    // Results::SignatureSet class
    //
    inline
    Result::SignatureSet::SignatureSet() throw(El::Exception)
    {
      set_deleted_key(0);
    }    
    
    //
    // Results::IdCounterMap class
    //
    inline
    Result::IdCounterMap::IdCounterMap() throw(El::Exception)
    {
      set_deleted_key(Message::Id::zero);
      set_empty_key(Message::Id::nonexistent);
    }

    //
    // Results::WordIdMap class
    //
    inline
    Result::WordIdMap::WordIdMap() throw(El::Exception)
    {
      set_deleted_key(0);
      set_empty_key(INT32_MAX);
    }
    
    inline
    Result::WordIdMap::~WordIdMap() throw()
    {
      for(WordIdMap::iterator it = begin(); it != end(); it++)
      {
        delete it->second;
      }
    }
    
    //
    // Results::MessageInfoPtrMap class
    //
    inline
    Result::MessageInfoPtrMap::MessageInfoPtrMap() throw(El::Exception)
    {
      set_deleted_key(Message::Id::zero);
      set_empty_key(Message::Id::nonexistent);
    }
    
    //
    // Results::MessageInfoConstPtrMap class
    //
    inline
    Result::MessageInfoConstPtrMap::MessageInfoConstPtrMap()
      throw(El::Exception)
    {
      set_deleted_key(Message::Id::zero);
      set_empty_key(Message::Id::nonexistent);
    }
    
    //
    // Results::EventIdCounter class
    //
    
    inline
    Result::EventIdCounter::EventIdCounter() throw(El::Exception)
    {
      set_empty_key(El::Luid::null);
      set_deleted_key(El::Luid::nonexistent);
    }

    //
    // Results class
    //
    inline
    Result::Result() throw(El::Exception)
        : message_infos(new MessageInfoArray()),
          max_weight(0),
          min_weight(UINT32_MAX)
    {
    }

    inline
    Result::Result(unsigned long size) throw(El::Exception)
        : message_infos(new MessageInfoArray(size)),
          max_weight(0),
          min_weight(UINT32_MAX)
    {
    }

    inline
    unsigned long
    Result::get_index(const WeightedId& weighted_id,
                      size_t range_count,
                      uint32_t diapason) const
      throw()
    {
      return (unsigned long long)(max_weight - weighted_id.weight) *
        range_count / diapason;
    }

    inline
    bool
    Result::remove_from_ranges(const WeightedId& weighted_id,
                               unsigned long range_count,
                               unsigned long diapason,
                               MessageInfoMapArray& ranges) const
      throw(El::Exception)
    {
      unsigned long index = get_index(weighted_id, range_count, diapason);

      if(index >= ranges.size())
      {
        return false;
      }

      MessageInfoConstPtrMap& msg_map = ranges[index];
      MessageInfoConstPtrMap::iterator it = msg_map.find(weighted_id.id);

      if(it == msg_map.end())
      {
        return false;
      }      

      msg_map.erase(it);
      return true;
    }

    inline
    bool
    Result::present_in_ranges(const WeightedId& weighted_id,
                              unsigned long range_count,
                              unsigned long diapason,
                              const MessageInfoMapArray& ranges) const
      throw(El::Exception)
    {
      unsigned long index = get_index(weighted_id, range_count, diapason);

      if(index >= ranges.size())
      {
        return false;
      }

      const MessageInfoConstPtrMap& msg_map = ranges[index];
      return msg_map.find(weighted_id.id) != msg_map.end();
    }    

    inline
    bool
    Result::check_uniqueness(const WeightedId& wid,
                             Message::Signature msg_signature,
                             const SignatureMap& signatures,
                             size_t range_count,
                             unsigned long diapason,
                             MessageInfoMapArray& ranges,
                             size_t* suppressed,
                             IdSet* suppressed_messages) const
      throw(El::Exception)
    {
      if(!msg_signature)
      {
        return true;
      }

      SignatureMap::const_iterator sit = signatures.find(msg_signature);
        
      if(sit == signatures.end())
      {
        return true;
      }
      
      const WeightedId& existing_wid = sit->second;
              
      if(present_in_ranges(existing_wid, range_count, diapason, ranges))
      {
        if(suppressed)
        {
          ++(*suppressed);
        }
                
        if(existing_wid.heavier(wid))
        {
          // Skip current message weighted id

          if(suppressed_messages)
          {
            suppressed_messages->insert(wid.id);
          }
                
          return false;
        }
        else
        {
          if(suppressed_messages)
          {
            suppressed_messages->insert(existing_wid.id);
          }

          remove_from_ranges(existing_wid,
                             range_count,
                             diapason,
                             ranges);
        }
      }

      return true;
    }
    
    inline
    void
    Result::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr.write_array(*message_infos);
      bstr << max_weight << min_weight;
      stat.write(bstr);
    }

    inline
    void
    Result::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr.read_array(*message_infos);
      bstr >> max_weight >> min_weight;
      stat.read(bstr);
    }
    
    //
    // Expression class
    //
    inline
    Expression::Expression() throw()
    {
    }
    
    inline
    Expression::Expression(Condition* cond) throw() : condition(cond)
    {
    }

    inline
    Expression::~Expression() throw()
    {
    }
     
    inline
    void
    Expression::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr << condition;
    }
      
    inline
    void
    Expression::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> condition;
    }

    inline
    void
    Expression::normalize(
      const El::Dictionary::Morphology::WordInfoManager& word_info_manager)
      throw(El::Exception)
    {
      if(condition.in())
      {
        condition->normalize(word_info_manager);
      }
    }
    
    inline
    Result*
    Expression::search(const Message::SearcheableMessageMap& messages,
                       bool copy_msg_struct,
                       const Strategy& strategy,
                       MessageWordPositionMap* mwp_map,
                       time_t* current_time)
      const throw(El::Exception)
    {
#     ifdef TRACE_SEARCH_TIME
      ACE_High_Res_Timer timer;
      timer.start();
#     endif

      El::Stat::TimeMeasurement measurement(search_meter);
      El::Stat::TimeMeasurement measurement1(search_p1_meter);
      
      Condition::MessageMatchInfoMap match_info;

      unsigned long flags = 0;

      if(mwp_map)
      {
        flags |= Condition::EF_FILL_POSITIONS;
      }

      Strategy::SortingMode mode = Strategy::SM_NONE;
      
      if((strategy.result_flags & Strategy::RF_MESSAGES) != 0 &&
         ((mode = strategy.sorting->type()) == Strategy::SM_BY_RELEVANCE_DESC
          || mode == Strategy::SM_BY_RELEVANCE_ASC))
      {
        flags |= Condition::EF_FILL_CORE_WORDS;
      }

      if(strategy.search_hidden)
      {
        flags |= Condition::EF_SEARCH_HIDDEN;
      }
      
      Condition::Context context(messages);

      if(current_time)
      {
        context.time = *current_time;
      }

#     ifdef TRACE_SEARCH_TIME
      timer.stop();
      ACE_Time_Value pr_tm;
      timer.elapsed_time(pr_tm);
      timer.start();
#     endif

      Condition::ResultPtr result;
      
      const Condition::Result* cres =
        condition->const_evaluate(context, flags);

      if(cres == 0)
      {
        Condition_var cond = search_condition(strategy);
        result.reset(cond->evaluate(context, match_info, flags));
        cres = result.get();
      }

#     ifdef TRACE_SEARCH_TIME
      timer.stop();
      ACE_Time_Value ev_tm;
      timer.elapsed_time(ev_tm);
      timer.start();
#     endif

      measurement1.stop();
      El::Stat::TimeMeasurement measurement2(search_p2_meter);
      
      Result* res = search_result(context,
                                  copy_msg_struct,
                                  cres,
                                  strategy,
                                  match_info,
                                  mwp_map,
                                  current_time);

#     ifdef TRACE_SEARCH_TIME
      timer.stop();
      ACE_Time_Value sr_tm;
      timer.elapsed_time(sr_tm);

      std::cerr << "Search::search: pr_tm " << El::Moment::time(pr_tm)
                << "; ev_tm " << El::Moment::time(ev_tm)
                << "; sr_tm " << El::Moment::time(sr_tm)
                << std::endl;
#     endif

      return res;
    }

    inline
    Result*
    Expression::search_simple(
      const Message::SearcheableMessageMap& messages,
      bool copy_msg_struct,
      const Strategy& strategy,
      MessageWordPositionMap* mwp_map,
      time_t* current_time) const
      throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(search_simple_meter);
      
      Condition::MessageMatchInfoMap match_info;
      
      unsigned long flags = 0;

      if(mwp_map)
      {
        flags |= Condition::EF_FILL_POSITIONS;
      }

      Strategy::SortingMode mode = Strategy::SM_NONE;
      
      if((strategy.result_flags & Strategy::RF_MESSAGES) != 0 &&
         ((mode = strategy.sorting->type()) == Strategy::SM_BY_RELEVANCE_DESC
          || mode == Strategy::SM_BY_RELEVANCE_ASC))
      {
        flags |= Condition::EF_FILL_CORE_WORDS;
      }
      
      if(strategy.search_hidden)
      {
        flags |= Condition::EF_SEARCH_HIDDEN;
      }
      
      Condition::Context context(messages);

      if(current_time)
      {
        context.time = *current_time;
      }
      
      Condition::ResultPtr result(
        condition->evaluate_simple(context, match_info, flags));
      
      return search_result(context,
                           copy_msg_struct,
                           result.get(),
                           strategy,
                           match_info,
                           mwp_map,
                           current_time);
    }

    inline
    Condition*
    Expression::search_condition(const Strategy& strategy) const
      throw(El::Exception)
    {
      Condition_var cond = condition;

      if(strategy.filter.lang != El::Lang::null &&
         (strategy.result_flags & Strategy::RF_LANG_STAT) == 0)
      {
        Lang_var lang = new Lang();
        lang->values.push_back(strategy.filter.lang);
        lang->condition = cond;
        cond = lang.retn();
      }
      
      if(strategy.filter.country != El::Country::null &&
         (strategy.result_flags & Strategy::RF_COUNTRY_STAT) == 0)
      {
        Country_var country = new Country();
        country->values.push_back(strategy.filter.country);
        country->condition = cond;
        cond = country.retn();
      }

      return cond.retn();
    }

    //
    // ExpressionParser class
    //
    inline
    ExpressionParser::~ExpressionParser() throw()
    {
    }

    inline
    Expression_var&
    ExpressionParser::expression() throw()
    {
      return expression_;
    }
    
    inline
    const Expression_var&
    ExpressionParser::expression() const throw()
    {
      return expression_;
    }
    
    inline
    bool
    ExpressionParser::contain(Condition::Type type) const throw()
    {
      return (condition_flags_ & (((uint64_t)1) << type)) != 0;
    }
    
    inline
    void
    ExpressionParser::optimize() throw(El::Exception)
    {
      condition_flags_ = 0;
      
      if(expression_.in())
      {
        Condition_var& condition = expression_->condition;
        
        if(condition.in())
        {
          condition = condition->optimize();
          condition_flags_ = condition->cleanup_opt_info();
        }
      }      
    }

    inline
    void
    ExpressionParser::put_back() throw(Exception, El::Exception)
    {
      if(buffered_)
      {
        throw Exception(
          "NewsGate::Search::ExpressionParser::put_back: buffer not empty");
      }

      buffered_ = true;
    }

    inline
    bool
    ExpressionParser::is_operation(TokenType type) throw()
    {
      switch(type)
      {
      case TT_AND:
      case TT_OR:
      case TT_EXCEPT: return true;
      default: break;
      }
      
      return false;
    }
    
    inline
    bool
    ExpressionParser::is_filter(TokenType type) throw()
    {
      switch(type)
      {
      case TT_LANG:
      case TT_COUNTRY:
      case TT_SPACE:
      case TT_DOMAIN:
      case TT_CAPACITY:
      case TT_IMPRESSIONS:
      case TT_FEED_IMPRESSIONS:
      case TT_CLICKS:
      case TT_FEED_CLICKS:
      case TT_CTR:
      case TT_FEED_CTR:
      case TT_RCTR:
      case TT_FEED_RCTR:
      case TT_SIGNATURE:
      case TT_WITH:
      case TT_PUB_DATE:
      case TT_FETCHED:
      case TT_VISITED: return true;
      default: break;
      }
      
      return false;
    }
    
    inline
    bool
    ExpressionParser::is_operand(TokenType type) throw()
    {
      switch(type)
      {
      case TT_ALL:
      case TT_ANY:
      case TT_SITE:
      case TT_URL:
      case TT_CATEGORY:
      case TT_MSG:
      case TT_EVENT:
      case TT_EVERY:
      case TT_NONE:
      case TT_OPEN: return true;
      default: break;
      }
      
      return false;
    }
    
    inline
    bool
    ExpressionParser::push_word(std::wstring& word,
                                WordList& words,
                                unsigned char group,
                                bool exact)
      throw(El::Exception)
    {
      unsigned long cat_flags = El::String::Unicode::EC_QUOTE |
        El::String::Unicode::EC_BRACKET | El::String::Unicode::EC_STOP;
      
      long start = 0;

      for(; El::String::Unicode::CharTable::el_categories(word[start]) &
            cat_flags; start++);

      if(start)
      {
        word = word.substr(start);
      }

      if(word[0] == L'\0')
      {
        return false;
      }
      
      start = word.length() - 1;
      long i = start;
        
      for(; i >= 0 && (El::String::Unicode::CharTable::el_categories(
                         word[i]) & cat_flags); i--);

      if(i < 0)
      {
        word.clear();
        return false;
      }
      else if(i != start)
      {
        word.resize(i + 1);
      }
        
      words.push_back(Word());

      WordList::reverse_iterator it = words.rbegin();

      std::string sword;
      El::String::Manip::wchar_to_utf8(word.c_str(), sword);

      it->text = sword;
      it->group(group);
      it->exact(exact);
        
      word.clear();
      return true;
    }
  }
}

#endif // _NEWSGATE_SERVER_COMMONS_SEARCH_SEARCHEXPRESSION_HPP_
