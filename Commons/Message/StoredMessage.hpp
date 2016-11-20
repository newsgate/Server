/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Commons/Message/StoredMessage.hpp
 * @Author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_COMMONS_MESSAGE_STOREDMESSAGE_HPP_
#define _NEWSGATE_SERVER_COMMONS_MESSAGE_STOREDMESSAGE_HPP_

#include <stdint.h>
#include <limits.h>

#include <list>
#include <utility>
#include <stack>
#include <vector>
#include <iostream>

#include <ext/hash_map>
#include <ext/hash_set>
#include <google/sparse_hash_map>
#include <google/sparse_hash_set>
#include <google/dense_hash_map>

#include <El/Exception.hpp>
#include <El/RefCount/All.hpp>
#include <El/SyncPolicy.hpp>
#include <El/BinaryStream.hpp>
#include <El/Hash/Hash.hpp>
#include <El/ArrayPtr.hpp>
#include <El/LightArray.hpp>
#include <El/String/Manip.hpp>
#include <El/String/LightString.hpp>
#include <El/String/StringPtr.hpp>
#include <El/Stat.hpp>
#include <El/Counter.hpp>
#include <El/CRC.hpp>
#include <El/Hash/Map.hpp>
#include <El/Country.hpp>
#include <El/Lang.hpp>
#include <El/Locale.hpp>
#include <El/Luid.hpp>
#include <El/Dictionary/Morphology.hpp>

#include <Commons/Message/Message.hpp>

namespace NewsGate
{ 
  namespace Message
  {
    typedef uint16_t WordPosition;
    const uint16_t WORD_POSITION_MAX = UINT16_MAX;
    
    typedef uint16_t WordPositionNumber;
    typedef El::LightArray<WordPosition, WordPositionNumber> WordPositionArray;

    struct WordComplement
    {
      enum Type
      {
        TP_UNDEFINED,
        TP_STANDALONE,
        TP_REPLACEMENT,
        TP_PREFIX,
        TP_SUFFIX,
        TP_SEGMENTATION
      };
      
      WordPosition position;
      uint8_t type;
      StringConstPtr text;

      WordComplement(WordPosition pos = 0,
                     uint8_t tp = TP_UNDEFINED,
                     const char* txt = 0)
        throw(El::Exception);

      void write(El::BinaryOutStream& ostr) const
        throw(Exception, El::Exception);

      void read(El::BinaryInStream& istr) throw(Exception, El::Exception);
    };

    typedef El::LightArray<WordComplement, WordPositionNumber>
    WordComplementLighArray;

    struct StoredImage : public Image
    {
      WordPosition alt_base;
      ImageThumbArray thumbs;

      StoredImage() throw(El::Exception);
      StoredImage(const StoredImage& src) throw(El::Exception);

      StoredImage& operator=(const StoredImage& src) throw(El::Exception);

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };
    
    typedef El::LightArray<StoredImage, WordPositionNumber>
    StoredImageArray;

    typedef std::auto_ptr<StoredImageArray> StoredImageArrayPtr;
    
    struct StoredContent :
      public virtual El::RefCount::DefaultImpl<El::Sync::ThreadPolicy>
    {
      StringConstPtr           source_html_link;
      El::String::LightString  url;
      uint32_t                 dict_hash;
      WordComplementLighArray  word_complements;
      StoredImageArrayPtr      images;
      
      StoredContent() throw(El::Exception);
      virtual ~StoredContent() throw();

      uint64_t timestamp() const throw();
      void timestamp(uint64_t val) throw();

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      
      void read(El::BinaryInStream& bstr,
                WordPosition& description_base) throw(El::Exception);

      void write_complements(std::ostream& ostr) const
        throw(Exception, El::Exception);

      void write_complements(El::BinaryOutStream& bin_str) const
        throw(Exception, El::Exception);
      
      void read_complements(std::istream& istr,
                            WordPosition& description_base)
        throw(Exception, El::Exception);

      void read_complements(El::BinaryInStream& bstr,
                            WordPosition& description_base)
        throw(Exception, El::Exception);

      void copy(const StoredContent& src) throw(El::Exception);

    private:
      uint64_t timestamp_;

    private:
      void operator=(const StoredContent&);
      StoredContent(const StoredContent&);
    };                     
    
    typedef El::RefCount::SmartPtr<StoredContent> StoredContent_var;

    struct WordPositions
    {
      enum Flags
      {
        FL_TITLE = 0x1,
        FL_DESC = 0x2,
        FL_ALT = 0x4,
        FL_LOCATION_MASK=0x7,
        FL_POS_ONE  = 0x8,
        FL_POS_TWO  = 0x10,
        FL_POS_MANY = 0x00,
        FL_POS_MASK = 0x18,
        FL_CORE_WORD = 0x20,
        FL_CAPITAL_WORD = 0x40,
        FL_TOKEN_TYPE_MASK = 0x780, // 4 bits 7-10,
        FL_PROPER_NAME = 0x800,
        FL_SENTENCE_PROPER_NAME = 0x1000
      };

      enum TokenType
      {
        TT_UNDEFINED = 0,
        TT_WORD = 1,
        TT_NUMBER = 2,
        TT_SURROGATE = 3,
        TT_FEED_STOP_WORD = 0xD,
        TT_KNOWN_WORD = 0xE,
        TT_STOP_WORD = 0xF
      };
      
      struct PositionRef
      {
        WordPositionNumber offset;
        WordPositionNumber count;

// Can't have a constructor for struct used in union        
//      PositionRef() throw();
        
        void write(El::BinaryOutStream& bin_str) const throw(El::Exception);
        void read(El::BinaryInStream& bin_str) throw(El::Exception);
      };

      uint16_t flags;
      El::Lang lang;
      
      union
      {
        PositionRef pos_ref;
        WordPosition pos[2];
      };

      WordPositions() throw();

      const WordPosition& position(const WordPositionArray& positions,
                                   WordPositionNumber index) const
        throw(El::Exception);

      WordPosition& position(WordPositionArray& positions,
                             WordPositionNumber index)
        throw(El::Exception);

      WordPositionNumber position_count() const throw();
      bool position_offset(WordPositionNumber& offset) const throw();

      void set_positions(WordPositionNumber offset,
                         WordPositionNumber count)
        throw();

      void insert_position(WordPositionArray& positions,
                           WordPositionNumber& global_index,
                           WordPosition position)
        throw();
      
      void write(El::BinaryOutStream& bin_str) const throw(El::Exception);
      void read(El::BinaryInStream& bin_str) throw(El::Exception);

      void read_1(El::BinaryInStream& bin_str) throw(El::Exception);
      void read_2(El::BinaryInStream& bin_str) throw(El::Exception);

      bool is_core() const throw() { return (flags & FL_CORE_WORD) != 0; }

      TokenType token_type() const throw();
      void token_type(TokenType type) throw();
    };
    
    typedef El::Hash::Map<StringConstPtr,
                          WordPositions,
                          StringConstPtrHash,
                          uint16_t>
    MessageWordPosition;

    typedef El::Hash::Map<
      El::Dictionary::Morphology::WordId,
      WordPositions,
      El::Hash::Numeric<El::Dictionary::Morphology::WordId>,
      uint16_t>
    NormFormPosition;

    typedef __gnu_cxx::hash_set<uint32_t> SegMarkerPositionSet;
    
    struct SegmentationInfo
    {
      SegMarkerPositionSet title;
      SegMarkerPositionSet description;
      SegMarkerPositionSet keywords;

      typedef std::vector<SegMarkerPositionSet> ImagesInfo;
      ImagesInfo images;
    };

    class CoreWords : public El::LightArray<uint32_t, uint8_t>
    {
    public:
      ~CoreWords() throw() {}
      
      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);      
    };

    struct Categories
    {
      typedef El::LightArray<StringConstPtr, uint16_t> CategoryArray;

      uint32_t      categorizer_hash;
      CategoryArray array;

      Categories() throw(El::Exception) : categorizer_hash(0) {}

      void write(std::ostream& ostr) const throw(Exception, El::Exception);

      void write(El::BinaryOutStream& bin_str) const
        throw(Exception, El::Exception);
      
      void read(std::istream& istr) throw(Exception, El::Exception);
      void read(El::BinaryInStream& bstr) throw(Exception, El::Exception);

      void swap(Categories& src) throw(El::Exception);
      void clear() throw() { array.clear(); categorizer_hash = 0; }
    };
    
    typedef uint64_t Signature;

    class SignatureSet :
      public google::sparse_hash_set<Signature,
                                     El::Hash::Numeric<Signature> >
    {
    public:
      SignatureSet() throw(El::Exception) { set_deleted_key(0); }
    };
    
    struct StoredMessage
    {
      enum MessageFlags
      {
        MF_HAS_IMAGES = 0x1,
        MF_HAS_THUMBS = 0x2,
        MF_HIDDEN = 0x40,
        MF_DIRTY = 0x80,
        MF_PERSISTENT_FLAGS = 0x3
      };
      
      // Memebers
      
      uint8_t flags; // 1
      uint8_t space; // 1
      
      // if 0, then hostname is not a DNS name
      uint8_t hostname_len; // 1

      El::Country   country; // 2
      El::Lang      lang; // 2

      WordPosition description_pos; // 2
      WordPosition img_alt_pos; // 2
      WordPosition keywords_pos; // 2
      
      uint32_t event_capacity; // 4
      uint32_t search_weight;

      uint32_t* feed_search_weight;
      
      StoredContent_var  content;  // 8
      StringConstPtr source_url; // 8
      StringConstPtr source_title; // 8
      StringConstPtr hostname; // 8

      CoreWords core_words; // 5=4+1
      WordPositionArray positions; // 6=4+2

      uint64_t           impressions; // 8
      uint64_t           clicks; // 8
      uint64_t           published; // 8
      uint64_t           fetched; // 8
      uint64_t           visited; // 8
      Id                 id; // 8
      Signature          signature; // 8
      Signature          url_signature; // 8
      SourceId           source_id; // 8
      El::Luid           event_id; // 8

      MessageWordPosition word_positions; // 10=(4+2)+4
      NormFormPosition norm_form_positions; // 10=(4+2)+4

      Categories categories;
      Categories::CategoryArray category_paths;
    
      static El::Stat::TimeMeter break_down_meter;
      static El::Stat::Counter object_counter;
      static uint32_t zero_feed_search_weight;      
      
      // Methods
      
      StoredMessage() throw();
      StoredMessage(const StoredMessage& src) throw(El::Exception);

      ~StoredMessage() throw();

      StoredMessage& operator=(const StoredMessage& src) throw(El::Exception);
      bool operator<(const StoredMessage& val) const throw();

      bool hidden() const throw() { return (flags & MF_HIDDEN) == MF_HIDDEN; }
      bool visible() const throw() { return (flags & MF_HIDDEN) != MF_HIDDEN; }
      
      void hide() throw() { flags |= MF_HIDDEN; }
      void show() throw() { flags &= ~MF_HIDDEN; }
      
      static void normalize_word(std::wstring& word) throw(El::Exception);
      static void normalize_char(wchar_t& chr) throw(El::Exception);
      
      void break_down(const char* title,
                      const char* description,
                      const RawImageArray* images,
                      const char* keywords,
                      const SegmentationInfo* segmentation_info = 0)
        throw(Exception, El::Exception);

      bool in_domain(const char* domain, uint8_t domain_len) const throw();
      void set_source_url(const char* val) throw(El::Exception);

      static Signature msg_url_signature(const char* url) throw();
      void set_url_signature(const char* url) throw();
      
      void steal(StoredMessage& src) throw(El::Exception);

      std::string image_thumb_name(const char* dir_path,
                                   size_t img_index,
                                   size_t thumb_index,
                                   bool create_dir) const
        throw(El::Exception);

      std::string image_thumbs_name(const char* dir_path,
                                    bool create_dir) const
        throw(El::Exception);

      void write_image_thumbs(const char* dir_path,
                              bool drop) const
        throw(Exception, El::Exception);

      void read_image_thumbs(const char* dir_path,
                             int32_t img_index,
                             int32_t thumb_index,
                             size_t* loaded_thumbs = 0)
        throw(Exception, El::Exception);

      static void cleanup_thumb_files(const char* dir_path,
                                      time_t max_age)
        throw(Exception, El::Exception);

      struct MessageBuilder
      {
        virtual bool word(const char* text, WordPosition position)
          throw(El::Exception) = 0;

        virtual bool interword(const char* text) throw(El::Exception) = 0;
        virtual bool segmentation() throw(El::Exception) = 0;

        virtual ~MessageBuilder() throw() {}
      };

      bool assemble_title(MessageBuilder& builder) const
        throw(Exception, El::Exception);      
      
      bool assemble_description(MessageBuilder& builder) const
        throw(Exception, El::Exception);      

      bool assemble_image_alt(MessageBuilder& builder,
                              WordPosition index) const
        throw(Exception, El::Exception);
      
      bool assemble_keywords(MessageBuilder& builder) const
        throw(Exception, El::Exception);      

      class DefaultMessageBuilder : public MessageBuilder
      {
      public:
        DefaultMessageBuilder(std::ostream& ostr, size_t max_len = SIZE_MAX)
          throw(El::Exception);

        virtual ~DefaultMessageBuilder() throw() {}

        virtual bool word(const char* text, WordPosition position)
          throw(El::Exception);

        virtual bool interword(const char* text) throw(El::Exception);
        virtual bool segmentation() throw(El::Exception);

        size_t length() const throw() { return length_; }

      private:
        std::ostream& output_;
        size_t max_length_;
        size_t length_;
      };      

      void adjust_positions(uint16_t version,
                            WordPosition description_base)
        throw(El::Exception);
      
      void write_broken_down(std::ostream& ostr) const
        throw(Exception, El::Exception);

      void write_broken_down(El::BinaryOutStream& bin_str) const
        throw(Exception, El::Exception);
      
      uint16_t read_broken_down(std::istream& istr)
        throw(Exception, El::Exception);

      uint16_t read_broken_down(El::BinaryInStream& bstr)
        throw(Exception, El::Exception);

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);

      static void set_normal_forms(
        const El::Dictionary::Morphology::WordInfoArray& word_infos,
        const MessageWordPosition& word_positions,
        const WordPositionArray& positions,
        NormFormPosition& norm_form_positions,
        WordPositionArray& resulted_positions)
        throw(El::Exception);

      uint64_t word_hash(bool core_word_flag) const throw();
      
    protected:
      
      void clear() throw();

      bool assemble(MessageBuilder& builder,
                    WordPosition min_pos,
                    WordPosition max_pos) const
        throw(Exception, El::Exception);      

      struct WordComplementElem
      {
        WordPosition position;
        uint8_t      type;
        El::String::LightString text;

        void set_text(const wchar_t* txt) throw(El::Exception);

        WordComplementElem(WordPosition pos = 0,
                           uint8_t tp = WordComplement::TP_UNDEFINED,
                           const char* txt = "")
          throw(El::Exception);
      };

      class WordComplementElemArray :
        public std::vector<WordComplementElem>
      {
      public:
        void add(WordComplementElem& wc) throw(El::Exception);
      };

      struct MsPs : public std::vector<WordPosition>
      {
        uint16_t flags;

        MsPs() throw() : flags(0) {}
      };
    
      struct MsWdPs :
        public __gnu_cxx::hash_map<std::string, MsPs, El::Hash::String>
      {
        struct Quote
        {
          wchar_t chr;
          std::wstring close_chrs;
        };

        void push_word(std::wstring& word,
                       uint16_t flags,
                       WordPosition& position,
                       Signature* psignature,
                       WordComplementElemArray& word_complements,
                       bool& end_of_sentence)
          throw(El::Exception);
      };

      static void parse_text(const char* text,
                             uint16_t flags,
                             WordPosition& position,
                             MsWdPs& word_pos,
                             WordComplementElemArray& word_complements,
                             Signature* psignature,
                             const SegMarkerPositionSet* seg_markers)
        throw(El::Exception);

    private:
      static const wchar_t FUNKY_QUOTES[];
      static const wchar_t REPLACEMENT_QUOTES[];
    };

    typedef std::list<StoredMessage> StoredMessageList;
    typedef std::vector<StoredMessage> StoredMessageArray;
    
    typedef uint32_t Number;
    const uint32_t NUMBER_MAX = UINT32_MAX;
    
    typedef uint32_t WordHash;

    class MessageMap :
      public google::dense_hash_map<Number,
                                    const StoredMessage*,
                                    El::Hash::Numeric<Number> >
    {
    public:
      MessageMap() throw(El::Exception);
      MessageMap(unsigned long size) throw(El::Exception);
    };    

    class StoredMessageMap : public MessageMap
    {
    public:
      ~StoredMessageMap() throw();
    };

    class NumberSet :
      public google::sparse_hash_set<Number, El::Hash::Numeric<Number> >
    {
    public:
      
      NumberSet() throw(El::Exception);      
      NumberSet(const NumberSet& src) throw(El::Exception);

      ~NumberSet() { object_counter.decrement(); }

      static El::Stat::Counter object_counter;      
    };

    struct WordLangInfo
    {
      uint32_t messages;
      uint32_t capitalized;
      
      WordLangInfo() throw() : messages(0), capitalized(0) {}
      WordLangInfo(uint32_t msgs, uint32_t caps) throw();      
    };

    class LangCounterMap :
      public google::sparse_hash_map<El::Lang, WordLangInfo, El::Hash::Lang>
    {
    public:
      LangCounterMap() throw(El::Exception);      
    };

    struct WordMessages
    {
      uint32_t capitalized;
      NumberSet messages;
      std::auto_ptr<LangCounterMap> lang_counter;

      WordMessages() throw(El::Exception) : capitalized(0) {}

      void add_message(const El::Lang& lang,
                       bool cap,
                       Number number,
                       const StoredMessageMap& msg_map,
                       bool viewer) throw(El::Exception);
      
      void remove_message(const El::Lang& lang,
                          bool cap,
                          Number number,
                          bool viewer)
        throw(El::Exception);
      
      size_t lang_message_count(const El::Lang& lang,
                                const StoredMessageMap& msg_map) const throw();

      bool proper_name(const El::Lang& lang) const throw();
      
    private:
      WordMessages(const WordMessages&);
      void operator=(const WordMessages&);
    };

    struct SmartStringConstPtr : public El::String::StringConstPtr
    {
      SmartStringConstPtr(const char* str = 0) throw(El::Exception);
      bool operator==(const SmartStringConstPtr& str) const throw();

    private:
      void operator=(const std::string& str) throw(El::Exception);
    };

    struct SmartStringConstPtrHash
    {
      size_t operator()(const SmartStringConstPtr& str) const
        throw(El::Exception);
    };    

    struct WordIdToMessageNumberMap :
      public google::sparse_hash_map<
      El::Dictionary::Morphology::WordId,
      WordMessages*,
      El::Hash::Numeric<El::Dictionary::Morphology::WordId> >
    {
      WordIdToMessageNumberMap() throw(El::Exception);
      ~WordIdToMessageNumberMap() throw();

    private:
      WordIdToMessageNumberMap(const WordIdToMessageNumberMap&);
      void operator=(const WordIdToMessageNumberMap&);
    };
    
    struct WordToMessageNumberMap :
      public google::sparse_hash_map<SmartStringConstPtr,
                                     WordMessages*,
                                     SmartStringConstPtrHash>
    {
      WordToMessageNumberMap() throw(El::Exception);
      ~WordToMessageNumberMap() throw();

    private:
      WordToMessageNumberMap(const WordToMessageNumberMap&);
      void operator=(const WordToMessageNumberMap&);
    };

    struct SiteToMessageNumberMap :
      public google::sparse_hash_map<
      SmartStringConstPtr,
      NumberSet*,
      SmartStringConstPtrHash>
    {
      SiteToMessageNumberMap() throw(El::Exception);
      ~SiteToMessageNumberMap() throw();

    private:
      SiteToMessageNumberMap(const SiteToMessageNumberMap&);
      void operator=(const SiteToMessageNumberMap&);
    };

    struct WordIdToCountMap :
      public google::sparse_hash_map<
      El::Dictionary::Morphology::WordId,
      uint32_t,
      El::Hash::Numeric<El::Dictionary::Morphology::WordId> >
    {
      WordIdToCountMap() throw(El::Exception);
    };

    class MessageWordMap :
      public google::sparse_hash_map<Number,
                                     El::Dictionary::Morphology::WordId*,
                                     El::Hash::Numeric<Number> >
    {
    public:
      MessageWordMap() throw(El::Exception);
      ~MessageWordMap() throw();

    private:
      MessageWordMap(const MessageWordMap&);
      void operator=(const MessageWordMap&);
    };    
    
    struct FeedInfo
    {
      MessageWordMap messages;
      WordIdToCountMap words;

      uint64_t impressions;
      uint64_t clicks;
      uint32_t search_weight;

      FeedInfo() throw() : impressions(0), clicks(0), search_weight(0) {}
      
      void optimize_mem_usage() throw(El::Exception);
      void calc_search_weight(uint32_t impression_respected_level) throw();
    };

    struct FeedInfoMap :
      public google::dense_hash_map<
      SmartStringConstPtr,
      FeedInfo*,
      SmartStringConstPtrHash>
    {
      FeedInfoMap() throw(El::Exception);
      ~FeedInfoMap() throw();

    private:
      FeedInfoMap(const FeedInfoMap&);
      void operator=(const FeedInfoMap&);
    };

    class EventToNumberMap :
      public google::dense_hash_map<El::Luid, NumberSet*, El::Hash::Luid>
    {
    public:
      EventToNumberMap() throw(El::Exception);
      ~EventToNumberMap() throw();

    private:
      EventToNumberMap(const EventToNumberMap&);
      void operator=(const EventToNumberMap&);
    };

    class IdToNumberMap :
      public google::dense_hash_map<Id, Number, MessageIdHash>
    {
    public:
      IdToNumberMap() throw(El::Exception);
    };
    
    struct WordsFreqInfo
    {
      struct PositionInfo
      {
        float word_frequency; // in %
        float norm_form_frequency; // in %
//        float wp_frequency;

        El::Dictionary::Morphology::WordId norm_form;
        StringConstPtr word;
        El::Lang lang;

        enum PositionFlags
        {
          PF_CAPITAL = 0x1,
          PF_TITLE = 0x2,
          PF_ALTERNATE_ONLY = 0x4, // Word do not appear not in title,
                                   // nor in description
          PF_META_INFO = 0x8, // Word comes exclusivelly from message meta info
                              // like meta keyword html tag
          PF_PROPER_NAME = 0x10
        };

        WordPositionNumber word_position_count;
        WordPositionNumber norm_form_position_count;
        
        uint16_t position_flags;
        uint16_t norm_form_flags;
        uint16_t word_flags;
        
        WordPositions::TokenType token_type;

        PositionInfo() throw();

        void set_position_flag(PositionFlags flag, bool value) throw();
        bool get_position_flag(PositionFlags flag) const throw();
        
        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);
      };

      typedef __gnu_cxx::hash_map<WordPosition,
                                  PositionInfo,
                                  El::Hash::Numeric<WordPosition> >
      PositionInfoMap;

      struct WordInfo
      {
        uint16_t position_flags;
        El::Dictionary::Morphology::WordId norm_form;
        El::Dictionary::Morphology::WordId pseudo_id;
        StringConstPtr text;
        El::Lang lang;
        float cw_weight;
        float wp_weight;
        float weight;
        WordPositions::TokenType token_type;
        bool feed_countable;

        WordInfo() throw(El::Exception);

        El::Dictionary::Morphology::WordId id() const throw();

        bool operator==(const WordInfo& wi) const throw(El::Exception);

        void write(El::BinaryOutStream& ostr) const
          throw(Exception, El::Exception);

        void read(El::BinaryInStream& istr) throw(Exception, El::Exception);
      };

      struct WordInfoHash
      {
        size_t operator()(const WordInfo& wi) const throw(El::Exception);
      };        

      typedef __gnu_cxx::hash_set<WordInfo, WordInfoHash> WordInfoHashSet;
      typedef std::vector<WordInfo> WordInfoArray;

      PositionInfoMap word_positions;
      WordInfoArray word_infos;

      void clear() throw(El::Exception);
      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);
      void steal(WordsFreqInfo& src) throw(El::Exception);
    };

    class Category;
    
    typedef __gnu_cxx::hash_map<StringConstPtr,
                                El::RefCount::SmartPtr<Category>,
                                StringConstPtrHash>
    CategoryMap;

    struct Category :
      public virtual El::RefCount::DefaultImpl<El::Sync::ThreadPolicy>
    {
      MessageMap messages;
      CategoryMap children;

      Category() throw(El::Exception) {}
      ~Category() throw() {}
      
      void insert(const char* path, Number number, const StoredMessage* msg)
        throw(InvalidArg, El::Exception);

      void remove(const char* path, Number number) throw(El::Exception);
      
      bool empty() const throw();
      const Category* find(const char* path) const throw(El::Exception);
      NumberSet* all_messages() const throw(El::Exception);
      
      void optimize_mem_usage() throw(El::Exception);

    private:
      void get_messages(NumberSet& cat_messages) const throw(El::Exception);
    };

    typedef El::RefCount::SmartPtr<Category> Category_var;

    class CategoryCounter :
      public google::dense_hash_map<SmartStringConstPtr,
                                    uint32_t,
                                    SmartStringConstPtrHash>
    {
    public:
      CategoryCounter() throw(El::Exception);
    };

    class LocaleCategoryCounter :
      public google::dense_hash_map<El::Locale,
                                    CategoryCounter*,
                                    El::Hash::Locale>
    {
    public:
      LocaleCategoryCounter() throw(El::Exception);
      ~LocaleCategoryCounter() throw();

      void increment(const El::Lang& lang,
                     const El::Country& country,
                     SmartStringConstPtr category) throw(El::Exception);

      void decrement(const El::Lang& lang,
                     const El::Country& country,
                     SmartStringConstPtr category) throw(El::Exception);

    private:

      void increment(const El::Locale& locale,
                     SmartStringConstPtr category) throw(El::Exception);      

      void decrement(const El::Locale& locale,
                     SmartStringConstPtr category) throw(El::Exception);
      
    private:
      LocaleCategoryCounter(const LocaleCategoryCounter&);
      void operator=(const LocaleCategoryCounter&);
    };

    struct WordPair
    {
      uint32_t word1;
      uint32_t word2;

      WordPair() throw() : word1(0), word2(0) {}
      WordPair(uint32_t w1, uint32_t w2) throw();

      bool operator==(const WordPair& wp) const throw();      
    };

    struct WordPairHash
    {
      size_t operator()(const WordPair& wp) const throw();
    };        

    class TimeMessageCounter :
      public google::sparse_hash_map<uint32_t,
                                     uint32_t,
                                     El::Hash::Numeric<uint32_t> >
    {
    public:
      TimeMessageCounter() throw(El::Exception) { set_deleted_key(0); }
    };

    struct WFW
    {
      float factor;
      float weight_max;
      float weight_sum;
      size_t count;

      WFW() throw() : factor(1), weight_max(0), weight_sum(0), count(0) {}
      WFW(float wm) throw() : factor(1),weight_max(wm),weight_sum(wm),count(1){}
      
      float weight_average() const throw() {return count ? weight_sum/count:0;}
    };
    
    class WordFreqWeight:
      public google::dense_hash_map<El::Dictionary::Morphology::WordId,
                                    WFW,
                                    El::Hash::Numeric<uint32_t> >
    {
    public:
      WordFreqWeight() throw();

      void absorb(El::Dictionary::Morphology::WordId id, float weight)
        throw(El::Exception);
    };

    struct LangInfo
    {
      uint32_t message_count;
//      TimeMessageCounter time_message_counter;

      LangInfo() throw(El::Exception) : message_count(0) {}
    };
    
    class LangInfoMap :
      public google::sparse_hash_map<El::Lang, LangInfo*, El::Hash::Lang>
    {
    public:
      LangInfoMap() throw(El::Exception) { set_deleted_key(El::Lang::null); }
      ~LangInfoMap() throw();
    };

    struct WordPairManager
    {
      virtual ~WordPairManager() throw() {}
      
      virtual void wp_increment_counter(const El::Lang& lang,
                                        uint32_t time_index,
                                        const WordPair& wp)
        throw(El::Exception) = 0;
      
      virtual void wp_decrement_counter(const El::Lang& lang,
                                        uint32_t time_index,
                                        const WordPair& wp)
        throw(El::Exception) = 0;
      
      virtual uint32_t wp_get(const El::Lang& lang,
                              uint32_t time_index,
                              const WordPair& wp)
        throw(El::Exception) = 0;
    };

    class SearcheableMessageMap
    {
    public:
      WordToMessageNumberMap   words;
      WordIdToMessageNumberMap norm_forms;
      SiteToMessageNumberMap   sites;
      FeedInfoMap              feeds;
      StoredMessageMap         messages;
      IdToNumberMap            id_to_number;
      EventToNumberMap         event_to_number;
      Category_var             root_category;
      LangInfoMap              lang_info;
      LocaleCategoryCounter    category_counter;

      static El::Stat::TimeMeter insert_meter;
      static El::Stat::TimeMeter remove_meter;

    public:
      SearcheableMessageMap(bool suppress_dups,
                            bool viewer,
                            uint32_t impression_respected_level,
                            WordPairManager* word_pair_manager)
        throw(El::Exception);
      
      StoredMessage* insert(const StoredMessage& msg,
                            unsigned long core_words_prc,
                            unsigned long max_core_words
                            /*bool sort_cwords*/)
        throw(Exception, El::Exception);

      void remove(const Id& id) throw(Exception, El::Exception);
      
      void optimize_mem_usage() throw(El::Exception);

      StoredMessage* find(const Id& id) throw(El::Exception);
      const StoredMessage* find(const Id& id) const throw(El::Exception);

      unsigned long total_words() const throw();
      unsigned long long total_word_positions() const throw();

      unsigned long total_norm_forms() const throw();
      unsigned long long total_norm_form_positions() const throw();

      void calc_words_freq(const StoredMessage& msg,
                           bool new_msg,
                           WordsFreqInfo& result)
         const throw(El::Exception);
/*
      void sort_core_words(StoredMessage& msg,
                           WordsFreqInfo* word_freq) const
        throw(Exception, El::Exception);
*/

      void calc_word_pairs_freq(const StoredMessage& msg,
                                WordsFreqInfo& wfi)
         const throw(El::Exception);

      void calc_search_weight(StoredMessage& msg) throw();

      void set_event_id(StoredMessage& msg, const El::Luid& event_id)
        throw(El::Exception);

      void set_categories(StoredMessage& msg, Categories& categories)
        throw(El::Exception);

      void set_impressions(StoredMessage& msg, uint64_t impressions)
        throw(El::Exception);

      void set_clicks(StoredMessage& msg, uint64_t clicks)
        throw(El::Exception);

      void add_category(StoredMessage& msg,
                        const StringConstPtr& category,
                        const char* lower_category = 0)
        throw(El::Exception);

      size_t lang_message_count(const El::Lang& lang) const throw();

      bool viewer() const throw() { return viewer_; }
      
      void dump(std::ostream& ostr) const throw(El::Exception);

    private:
      Number assign_number() throw(Exception, El::Exception);
      void recycle_number(Number number) throw(El::Exception);

      void remove_categories(StoredMessage& msg, Number number)
        throw(El::Exception);
      
      void add_categories(StoredMessage& msg, Number number)
        throw(El::Exception);

      Signature message_signature(const StoredMessage& msg) const throw();

      typedef __gnu_cxx::hash_set<StringConstPtr, StringConstPtrHash>
      StringConstPtrSet;

      static void save_category_paths(const char* category,
                                      size_t category_len,
                                      char* buffer,
                                      StringConstPtrSet& path_set)
        throw(El::Exception);
      
      void set_category_paths(StoredMessage& msg,
                              StringConstPtrSet& path_set,
                              Number number)
        throw(El::Exception);

      void sort_cw(StoredMessage& msg, WordsFreqInfo& word_freq) const
        throw(Exception, El::Exception);      
      
    private:
      std::stack<Number> recycled_numbers_;
      Number next_number_;
      unsigned long total_words_;
      unsigned long long total_word_positions_;
      unsigned long total_norm_forms_;
      unsigned long long total_norm_form_positions_;

      bool suppress_duplicates_;
      bool viewer_;
      SignatureSet signatures_;
      SignatureSet url_signatures_;
      uint32_t impression_respected_level_;
      WordPairManager* word_pair_manager_;

    private:
      
      SearcheableMessageMap(const SearcheableMessageMap&);
      void operator=(const SearcheableMessageMap&);
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
    // WordPositions struct
    //
    inline
    WordPositions::WordPositions() throw() : flags(0)
    {
      pos_ref.offset = 0;
      pos_ref.count = 0;
    }

    inline
    void
    WordPositions::write(El::BinaryOutStream& bin_str) const
      throw(El::Exception)
    {
      bin_str << flags << lang << pos_ref;
    }

    inline
    void
    WordPositions::read(El::BinaryInStream& bin_str) throw(El::Exception)
    {
      bin_str >> flags >> lang >> pos_ref;
    }

    inline
    void
    WordPositions::read_1(El::BinaryInStream& bin_str) throw(El::Exception)
    {
      uint8_t fl = 0;
      
      bin_str >> fl >> pos_ref;
      flags = fl;
    }

    inline
    void
    WordPositions::read_2(El::BinaryInStream& bin_str) throw(El::Exception)
    {
      uint8_t fl = 0;
      
      bin_str >> fl >> lang >> pos_ref;
      flags = fl;
    }

    inline
    WordPositionNumber
    WordPositions::position_count() const throw()
    {
      unsigned long multiplicity = flags & FL_POS_MASK;
      
      return multiplicity == FL_POS_ONE ? 1 : (multiplicity == FL_POS_TWO ?
        2 : pos_ref.count);
    }

    inline
    bool
    WordPositions::position_offset(WordPositionNumber& offset) const throw()
    {
      switch(flags & FL_POS_MASK)
      {
      case FL_POS_ONE:
      case FL_POS_TWO: return false;
      default: offset = pos_ref.offset; return true;
      }
    }
    
    inline
    const WordPosition&
    WordPositions::position(const WordPositionArray& positions,
                            WordPositionNumber index) const
      throw(El::Exception)
    {
      return (flags & FL_POS_MASK) == FL_POS_MANY ?
        positions[pos_ref.offset + index] : pos[index];
    }

    inline
    WordPosition&
    WordPositions::position(WordPositionArray& positions,
                            WordPositionNumber index)
      throw(El::Exception)
    {
      return (flags & FL_POS_MASK) == FL_POS_MANY ?
        positions[pos_ref.offset + index] : pos[index];
    }

    inline
    void
    WordPositions::set_positions(WordPositionNumber offset,
                                 WordPositionNumber count)
      throw()
    {
      switch(count)
      {
      case 1: flags &= ~FL_POS_MASK; flags |= FL_POS_ONE; break;
      case 2:
      {
        flags &= ~FL_POS_MASK;
        flags |= FL_POS_TWO;
        pos[0] = WORD_POSITION_MAX;
        break;
      }
      default:
        {
          flags &= ~FL_POS_MASK;
          flags |= FL_POS_MANY;
          pos_ref.offset = offset;
          pos_ref.count = count;
          break;
        }
      }
    }

    inline
    void
    WordPositions::insert_position(WordPositionArray& positions,
                                   WordPositionNumber& global_index,
                                   WordPosition position)
      throw()
    {
      switch(flags & FL_POS_MASK)
      {
      case FL_POS_ONE: pos[0] = position; break;
      case FL_POS_TWO:
        {
          pos[pos[0] == WORD_POSITION_MAX ? 0 : 1] = position;
          break;
        }
      default:
        {
          positions[global_index++] = position;
          break;
        }
      }
    }

    inline
    WordPositions::TokenType
    WordPositions::token_type() const throw()
    {
      return (WordPositions::TokenType)((flags & FL_TOKEN_TYPE_MASK) >> 7);
    }
    
    inline
    void
    WordPositions::token_type(TokenType type) throw()
    {
      flags = (flags & ~FL_TOKEN_TYPE_MASK) | (type << 7);
    }
    
    //
    // WordPositions::PositionRef struct
    //
    inline
    void
    WordPositions::PositionRef::write(El::BinaryOutStream& bin_str) const
      throw(El::Exception)
    {
      bin_str << offset << count;
    }

    inline
    void
    WordPositions::PositionRef::read(El::BinaryInStream& bin_str)
      throw(El::Exception)
    {
      bin_str >> offset >> count;
    }

    //
    // WordComplement struct
    //
    inline
    WordComplement::WordComplement(WordPosition pos,
                                   uint8_t tp,
                                   const char* txt)
      throw(El::Exception)
        : position(pos),
          type(tp),
          text(txt)
    {
    }

    inline
    void
    WordComplement::write(El::BinaryOutStream& ostr) const
      throw(Exception, El::Exception)
    {
      ostr << position << type << text;
    }

    inline
    void
    WordComplement::read(El::BinaryInStream& istr)
      throw(Exception, El::Exception)
    {
      istr >> position >> type >> text;
    }
    
    //
    // StoredImage class
    //
    inline
    StoredImage::StoredImage() throw(El::Exception) : alt_base(0)
    {
    }

    inline
    StoredImage::StoredImage(const StoredImage& src) throw(El::Exception)
        : alt_base(src.alt_base),
          thumbs(src.thumbs)
    {
    }

    inline
    StoredImage&
    StoredImage::operator=(const StoredImage& src) throw(El::Exception)
    {
      Image::operator=(src);
      
      alt_base = src.alt_base;
      thumbs = src.thumbs;

      return *this;
    }

    inline
    void
    StoredImage::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      Image::write(bstr);      
      bstr << alt_base << thumbs;
    }

    inline
    void
    StoredImage::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      Image::read(bstr);
      bstr >> alt_base >> thumbs;
    }

    //
    // StoredContent class
    //
    inline
    StoredContent::StoredContent() throw(El::Exception)
        : dict_hash(0),
          timestamp_(0)
    {
    }
    
    inline
    StoredContent::~StoredContent() throw()
    {
    }
 
    inline
    uint64_t
    StoredContent::timestamp() const throw()
    {
      ReadGuard_ guard(lock_i());
      return timestamp_;
    }
    
    inline
    void
    StoredContent::timestamp(uint64_t val) throw()
    {
      WriteGuard_ guard(lock_i());
      timestamp_ = val;
    }

    inline
    void
    StoredContent::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << source_html_link << url << dict_hash;
      write_complements(bstr);
    }
    
    inline
    void
    StoredContent::read(El::BinaryInStream& bstr,
                        WordPosition& description_base) throw(El::Exception)
    {
      bstr >> source_html_link >> url >> dict_hash;
      read_complements(bstr, description_base);
    }

    inline
    void
    StoredContent::write_complements(std::ostream& ostr) const
      throw(Exception, El::Exception)
    {
      El::BinaryOutStream bin_str(ostr);
      write_complements(bin_str);
    }

    inline
    void
    StoredContent::write_complements(El::BinaryOutStream& bstr) const
      throw(Exception, El::Exception)
    {
      // Version
      bstr << (uint32_t)2;
      
      bstr.write_array(word_complements);

      WordPosition img_count = images.get() ? images->size() : 0;
      bstr << img_count;

      if(img_count)
      {
        for(WordPosition i = 0; i < images->size(); i++)
        {
          (*images)[i].write(bstr);
        }
      }
    }
      
    inline
    void
    StoredContent::read_complements(std::istream& istr,
                                    WordPosition& description_base)
      throw(Exception, El::Exception)
    {
      El::BinaryInStream bin_str(istr);
      read_complements(bin_str, description_base);
    }

    inline
    void
    StoredContent::read_complements(El::BinaryInStream& bstr,
                                    WordPosition& description_base)
      throw(Exception, El::Exception)
    {
      word_complements.clear();

      uint32_t version = 0;
      bstr >> version;

      WordPosition img_count = 0;
      bstr >> word_complements;

      if(version < 2)
      {
        // Can remove description_base notion completelly after
        // 1.5.95.0 running all colos longer than a month
        bstr >> description_base;
      }
      else
      {
        description_base = WORD_POSITION_MAX;
      }

      bstr >> img_count;

      if(img_count)
      {
        images.reset(new StoredImageArray(img_count));

        for(WordPosition i = 0; i < img_count; i++)
        {
          bstr >> (*images)[i];
        }        
      }
      else
      {
        images.reset(0);
      }
    }

    inline
    void
    StoredContent::copy(const StoredContent& src) throw(El::Exception)
    {
      source_html_link = src.source_html_link;
      url = src.url;
      dict_hash = src.dict_hash;
      word_complements = src.word_complements;

      if(src.images.get())
      {
        images.reset(new StoredImageArray(*src.images.get()));
      }
      else
      {
        images.reset(0);
      }
    }
    
    //
    // CoreWords class
    //
    inline
    void
    CoreWords::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << (uint8_t)size();
      
      for(unsigned long i = 0; i < size(); i++)
      {
        bstr << (*this)[i];
      }
    }
    
    inline
    void
    CoreWords::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      uint8_t uc_size = 0;
      
      bstr >> uc_size;
      resize(uc_size);
        
      for(uint8_t i = 0; i < uc_size; i++)
      {
        bstr >> (*this)[i];
      }
    }

    //
    // StoredMessage class
    //
    inline
    StoredMessage::StoredMessage() throw()
        : flags(0),
          space(Feed::SP_UNDEFINED),
          hostname_len(0),
          description_pos(0),
          img_alt_pos(0),
          keywords_pos(0),
          event_capacity(0),
          search_weight(0),
          feed_search_weight(&zero_feed_search_weight),
          impressions(0),
          clicks(0),
          published(0),
          fetched(0),
          visited(0),
          signature(0),
          url_signature(0),
          source_id(0),
          word_positions(StringConstPtr::null),
          norm_form_positions(0)
    {
      object_counter.increment();
    }
    
    inline
    StoredMessage::StoredMessage(const StoredMessage& src)
      throw(El::Exception)
        : flags(0),
          space(Feed::SP_UNDEFINED),
          hostname_len(0),
          description_pos(0),
          img_alt_pos(0),
          keywords_pos(0),
          event_capacity(0),
          search_weight(0),
          feed_search_weight(&zero_feed_search_weight),
          impressions(0),
          clicks(0),
          published(0),
          fetched(0),
          visited(0),
          signature(0),
          url_signature(0),
          source_id(0),
          word_positions(StringConstPtr::null),
          norm_form_positions(0)
    {
      *this = src;
      
      object_counter.increment();
    }
    
    inline
    StoredMessage::~StoredMessage() throw()
    {
      clear();
      
      object_counter.decrement();
    }

    inline
    void
    StoredMessage::normalize_char(wchar_t& chr) throw(El::Exception)
    {
      const wchar_t* ptr = wcschr(FUNKY_QUOTES, chr);

      if(ptr)
      {
        chr = REPLACEMENT_QUOTES[ptr - FUNKY_QUOTES];
      }
    }

    inline
    Signature
    StoredMessage::msg_url_signature(const char* url) throw()
    {
      Signature url_signature = 0; 

      if(url)
      {
        El::CRC(url_signature, (const unsigned char*)url, strlen(url));
      }

      return url_signature;
   }
    
    inline
    void
    StoredMessage::set_url_signature(const char* url) throw()
    {
      url_signature = msg_url_signature(url);
    }

    inline
    bool
    StoredMessage::operator<(const StoredMessage& val) const throw()
    {
      return published == val.published ? id < val.id :
        published < val.published;
    }
    
    inline
    StoredMessage&
    StoredMessage::operator=(const StoredMessage& src)
      throw(El::Exception)
    {
      id = src.id;
      event_id = src.event_id;
      description_pos = src.description_pos;
      img_alt_pos = src.img_alt_pos;
      keywords_pos = src.keywords_pos;
      event_capacity = src.event_capacity;
      flags = src.flags;
      impressions = src.impressions;
      clicks = src.clicks;
      published = src.published;
      fetched = src.fetched;
      visited = src.visited;
      signature = src.signature;
      url_signature = src.url_signature;
      source_id = src.source_id;
      space = src.space;
      country = src.country;
      lang = src.lang;
      
      content = src.content;
      source_title = src.source_title;
      
      set_source_url(src.source_url.c_str());

      word_positions = src.word_positions;      
      norm_form_positions = src.norm_form_positions;
      positions = src.positions;
      core_words = src.core_words;

      categories = src.categories;

// Filled by SearcheableMessageMap methods only      
      category_paths.clear();
      search_weight = 0;
      feed_search_weight = &zero_feed_search_weight;
      
      return *this;
    }
    
    inline
    bool
    StoredMessage::assemble_title(MessageBuilder& builder) const
      throw(Exception, El::Exception)
    {
      if(content.in() == 0)
      {
        throw Exception("NewsGate::Message::StoredMessage::assemble_title: "
                        "no content");
      }

      return assemble(builder, 0, description_pos);
    }
      
    inline
    bool
    StoredMessage::assemble_description(MessageBuilder& builder) const
      throw(Exception, El::Exception)
    {
      if(content.in() == 0)
      {
        throw Exception("NewsGate::Message::StoredMessage::"
                        "assemble_description: no content");
      }

      return assemble(builder, description_pos, img_alt_pos);
    }

    inline
    bool
    StoredMessage::assemble_image_alt(MessageBuilder& builder,
                                      WordPosition index) const
      throw(Exception, El::Exception)
    {
      if(content.in() == 0)
      {
        throw Exception("NewsGate::Message::StoredMessage::"
                        "assemble_image_alt: no content");
      }

      const StoredImageArray* images = content->images.get();
      
      if(images == 0 || index >= images->size())
      {
        throw Exception("NewsGate::Message::StoredMessage::"
                        "assemble_image_alt: out of image range");
      }

      return assemble(builder,
                      (*images)[index].alt_base,
                      index < images->size() - 1 ?
                      (*images)[index + 1].alt_base : keywords_pos);
    }

    inline
    bool
    StoredMessage::assemble_keywords(MessageBuilder& builder) const
      throw(Exception, El::Exception)
    {
      if(content.in() == 0)
      {
        throw Exception("NewsGate::Message::StoredMessage::"
                        "assemble_keywords: no content");
      }

      return assemble(builder, keywords_pos, WORD_POSITION_MAX);
    }

    inline
    bool
    StoredMessage::in_domain(const char* domain, uint8_t domain_len)
      const throw()
    {
      if(hostname_len == 0 || domain_len == 0)
      {
        return false;
      }

      return domain_len <= hostname_len &&
        strcmp(domain, hostname.c_str() + hostname_len - domain_len) == 0 &&
        (domain_len == hostname_len ||
         hostname.c_str()[hostname_len - domain_len - 1] == '.');
    }

    inline
    void
    StoredMessage::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << id << event_id << event_capacity
           << (uint8_t)(flags & MF_PERSISTENT_FLAGS) << impressions << clicks
           << published << fetched << visited << signature << url_signature
           << source_id << space << country << lang << source_title;

      content->write(bstr);
      write_broken_down(bstr);
      bstr << categories;
    }

    inline
    void
    StoredMessage::adjust_positions(uint16_t version,
                                    WordPosition description_base)
      throw(El::Exception)
    {
      if(version < 5)
      {
        description_pos = description_base;

        const StoredImageArray* images = content->images.get();
        
        img_alt_pos = images && images->size() ?
          (*images)[0].alt_base : WORD_POSITION_MAX;
        
        keywords_pos = WORD_POSITION_MAX;
      }
    }
    
    inline
    void
    StoredMessage::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> id >> event_id >> event_capacity >> flags >> impressions
           >> clicks >> published >> fetched >> visited >> signature
           >> url_signature >> source_id >> space >> country >> lang
           >> source_title;

      content = new StoredContent();

      WordPosition description_base = 0;
      content->read(bstr, description_base);

      uint16_t version = read_broken_down(bstr);
      bstr >> categories;

      adjust_positions(version, description_base);

// Filled by SearcheableMessageMap methods only      
      category_paths.clear();
      search_weight = 0;
      feed_search_weight = &zero_feed_search_weight;
    }

    inline
    void
    StoredMessage::steal(StoredMessage& src) throw(El::Exception)
    {
      id = src.id;
      event_id = src.event_id;
      description_pos = src.description_pos;
      img_alt_pos = src.img_alt_pos;
      keywords_pos = src.keywords_pos;
      event_capacity = src.event_capacity;
      flags = src.flags;
      impressions = src.impressions;
      clicks = src.clicks;
      published  = src.published;
      fetched = src.fetched;
      visited = src.visited;
      signature = src.signature;
      url_signature = src.url_signature;
      source_id = src.source_id;
      space = src.space;
      country = src.country;
      lang = src.lang;

      content = src.content.retn();
      source_title.swap(src.source_title);
      
      source_url.swap(src.source_url);      
      hostname.swap(src.hostname);
      hostname_len = src.hostname_len;
      src.hostname_len = 0;

      word_positions.swap(src.word_positions);
      norm_form_positions.swap(src.norm_form_positions);
      positions.swap(src.positions);
      core_words.swap(src.core_words);
      
      categories.swap(src.categories);
      
      category_paths.clear();
      search_weight = 0;
      feed_search_weight = &zero_feed_search_weight;
    }

    //
    // WordsFreqInfo struct
    //
    inline
    void
    WordsFreqInfo::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << (uint32_t)word_positions.size();

      for(PositionInfoMap::const_iterator it = word_positions.begin();
          it != word_positions.end(); it++)
      {
        bstr << it->first;
        it->second.write(bstr);
      }

      bstr.write_array(word_infos);
    }

    inline
    void
    WordsFreqInfo::clear() throw(El::Exception)
    {
      word_positions.clear();
      word_infos.clear();  
    }
      
    inline
    void
    WordsFreqInfo::read(El::BinaryInStream& bstr)
      throw(El::Exception)
    {
      clear();
    
      uint32_t word_positions_count = 0;
      bstr >> word_positions_count;

      for(unsigned long i = 0; i < word_positions_count; i++)
      {
        WordPosition pos;
        bstr >> pos;
        word_positions[pos].read(bstr);
      }

      bstr.read_array(word_infos);
    }

    inline
    void
    WordsFreqInfo::steal(WordsFreqInfo& src) throw(El::Exception)
    {
      word_positions.swap(src.word_positions);
      word_infos.swap(src.word_infos);
    }
    
    //
    // WordsFreqInfo::WordInfo struct
    //    
    inline
    size_t
    WordsFreqInfo::WordInfoHash::operator()(const WordInfo& wi) const
      throw(El::Exception)
    {
      unsigned long crc = El::CRC32_init();
      unsigned char t = wi.norm_form ? 1 : 2;
      
      El::CRC32(crc, &t, 1);
            
      if(wi.norm_form)
      {
        El::CRC32(crc,
                  (const unsigned char*)&wi.norm_form,
                  sizeof(wi.norm_form));
      }
      else
      {
        El::CRC32(crc,
                  (const unsigned char*)&wi.pseudo_id,
                  sizeof(wi.pseudo_id));
      }

//      return wi.norm_form;
      return crc;
    }
      
    //
    // WordPair struct
    //    
    inline
    WordPair::WordPair(uint32_t w1, uint32_t w2) throw()
        : word1(std::min(w1, w2)),
          word2(std::max(w1, w2))
    {
    }

    inline
    bool
    WordPair::operator==(const WordPair& wp) const throw()
    {
      return word1 == wp.word1 && word2 == wp.word2;
    }
    
    //
    // WordPairHash struct
    //    
    inline
    size_t
    WordPairHash::operator()(const WordPair& wp) const throw()
    {
      unsigned long crc = El::CRC32_init();
      El::CRC32(crc, (const unsigned char*)&wp.word1, sizeof(wp.word1));
      El::CRC32(crc, (const unsigned char*)&wp.word2, sizeof(wp.word2));
      return crc;
    }
    
    //
    // WordFreqWeight class
    //
    inline
    WordFreqWeight::WordFreqWeight() throw()
    {
      set_empty_key(UINT32_MAX);
      set_deleted_key(UINT32_MAX - 1);
    }

    inline
    void
    WordFreqWeight::absorb(El::Dictionary::Morphology::WordId id, float weight)
      throw(El::Exception)
    {
      iterator i = find(id);
      
      if(i == end())
      {
        insert(std::make_pair(id, WFW(weight)));
      }
      else
      {
        WFW& wfw = i->second;
        
        wfw.weight_max = std::max(wfw.weight_max, weight);
        wfw.weight_sum += weight;
        ++wfw.count;
      }
    }
    
    //
    // WordsFreqInfo::WordInfo struct
    //
    inline
    WordsFreqInfo::WordInfo::WordInfo() throw(El::Exception)
        : position_flags(0),
          norm_form(0),
          pseudo_id(0),
          cw_weight(0),
          wp_weight(0),
          weight(0),
          token_type(WordPositions::TT_UNDEFINED),
          feed_countable(false)
    {
    }      
      
    inline
    El::Dictionary::Morphology::WordId
    WordsFreqInfo::WordInfo::id() const throw()
    {
      return norm_form ? norm_form : pseudo_id;
    }
      
    inline
    bool
    WordsFreqInfo::WordInfo::operator==(const WordInfo& wi) const
      throw(El::Exception)
    {
      if(norm_form != wi.norm_form)
      {
        return false;
      }

      if(norm_form)
      {
        return true;
      }
        
      return text == wi.text;
    }

    inline
    void
    WordsFreqInfo::WordInfo::write(El::BinaryOutStream& ostr) const
      throw(Exception, El::Exception)
    {
      ostr << position_flags << norm_form << pseudo_id << cw_weight
           << wp_weight << weight << text << lang << (uint32_t)token_type
           << (uint8_t)feed_countable;
    }

    inline
    void
    WordsFreqInfo::WordInfo::read(El::BinaryInStream& istr)
      throw(Exception, El::Exception)
    {
      uint32_t tt = 0;
      uint8_t fc = 0;
      
      istr >> position_flags >> norm_form >> pseudo_id >> cw_weight
           >> wp_weight >> weight >> text >> lang >> tt >> fc;
      
      token_type = (WordPositions::TokenType)tt;
      feed_countable = fc != 0;
    }

    //
    // WordsFreqInfo::PositionInfo struct
    //
    inline
    WordsFreqInfo::PositionInfo::PositionInfo() throw()
        : word_frequency(0),
          norm_form_frequency(0),
//          wp_frequency(0),
          norm_form(0),
          word_position_count(0),
          norm_form_position_count(0),
          position_flags(0),
          norm_form_flags(0),
          word_flags(0),
          token_type(WordPositions::TT_UNDEFINED)
    {
    }

    inline
    bool
    WordsFreqInfo::PositionInfo::get_position_flag(PositionFlags flag) const
      throw()
    {
      return (position_flags & flag) != 0;
    }
    
    inline
    void
    WordsFreqInfo::PositionInfo::set_position_flag(PositionFlags flag,
                                                   bool value)
      throw()
    {
      if(value)
      {
        position_flags |= flag;
      }
      else
      {
        position_flags &= ~flag;
      }
    }
      
    inline
    void
    WordsFreqInfo::PositionInfo::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr << word_position_count << word_frequency
           << norm_form_position_count << norm_form_frequency
           << norm_form_flags << word_flags << position_flags
           << (uint32_t)token_type << lang << norm_form << word/*
                                                                 << wp_frequency*/;
    }
      
    inline
    void
    WordsFreqInfo::PositionInfo::read(El::BinaryInStream& bstr)
      throw(El::Exception)
    {
      uint32_t tt = 0;
        
      bstr >> word_position_count >> word_frequency
           >> norm_form_position_count >> norm_form_frequency
           >> norm_form_flags >> word_flags >> position_flags >> tt >> lang
           >> norm_form >> word/* >> wp_frequency*/;

      token_type = (WordPositions::TokenType)tt;
    }

    //
    // Number class
    //
    inline
    NumberSet::NumberSet() throw(El::Exception)
    {
      set_deleted_key(UINT32_MAX);
      object_counter.increment();
    }

    inline
    NumberSet::NumberSet(const NumberSet& src) throw(El::Exception)
        : google::sparse_hash_set<Number, El::Hash::Numeric<Number> >(src)
    {
      object_counter.increment();
    }

    //
    // Number class
    //
    inline
    WordLangInfo::WordLangInfo(uint32_t msgs, uint32_t caps) throw()
        : messages(msgs),
          capitalized(caps)
    {
    }

    //
    // LangCounterMap class
    //
    inline
    LangCounterMap::LangCounterMap() throw(El::Exception)
    {
      set_deleted_key(El::Lang::nonexistent);
    }    

    //
    // WordMessages class
    //
    inline
    size_t
    WordMessages::lang_message_count(const El::Lang& lang,
                                     const StoredMessageMap& msg_map) const
      throw()
    {
      if(lang == El::Lang::null || messages.empty())
      {
        return messages.size();
      }
      
      if(lang_counter.get() == 0)
      {
        StoredMessageMap::const_iterator mi =
          msg_map.find(*messages.begin());
          
        assert(mi != msg_map.end());

        return mi->second->lang == lang ? messages.size() : 0;
      }
      
      LangCounterMap::iterator i = lang_counter->find(lang);
      return i == lang_counter->end() ? 0 : i->second.messages;
    }    
    
    inline
    bool
    WordMessages::proper_name(const El::Lang& lang) const throw()
    {
      if(messages.empty() || lang == El::Lang::null || lang_counter.get() == 0)
      {
        return capitalized > 1 && capitalized * 100 / messages.size() >= 90;
      }

      LangCounterMap::iterator i = lang_counter->find(lang);
      
      if(i == lang_counter->end())
      {
        return capitalized > 1 && capitalized * 100 / messages.size() >= 90;
      }
      
      const WordLangInfo& wli = i->second;
      
      return  wli.capitalized > 1 &&
        wli.capitalized * 100 / wli.messages >= 90;
    }    
    
    inline
    void
    WordMessages::add_message(const El::Lang& lang,
                              bool cap,
                              Number number,
                              const StoredMessageMap& msg_map,
                              bool viewer)
      throw(El::Exception)
    {
      if(!viewer)
      {
        if(/*lang != El::Lang::null &&*/ !messages.empty())
        {
          if(lang_counter.get() == 0)
          {        
            StoredMessageMap::const_iterator mi =
              msg_map.find(*messages.begin());
          
            assert(mi != msg_map.end());

            if(mi->second->lang != lang)
            {
              lang_counter.reset(new LangCounterMap());
              lang_counter->resize(2);
            
              lang_counter->insert(
                std::make_pair(mi->second->lang,
                               WordLangInfo(messages.size(), capitalized)));

//            std::cerr << "AAA: " << capitalized << std::endl;
            }
          }
          
          if(lang_counter.get() != 0)
          {
            LangCounterMap::iterator i = lang_counter->find(lang);

            if(i == lang_counter->end())
            {
              lang_counter->insert(std::make_pair(lang, WordLangInfo(1, cap)));
            }
            else
            {
              WordLangInfo& wli = i->second;

              ++wli.messages;
            
              if(cap)
              {
                ++wli.capitalized;

//              std::cerr << "BBB: " << wli.capitalized << std::endl;
              }
            
              assert(wli.capitalized <= wli.messages);
            }
          }
        }

        if(cap)
        {
          ++capitalized;
        }
      }
      
      messages.insert(number);

      assert(viewer || capitalized <= messages.size());
    }

    inline
    void
    WordMessages::remove_message(const El::Lang& lang,
                                 bool cap,
                                 Number number,
                                 bool viewer)
      throw(El::Exception)
    {
      if(!viewer)
      {
        if(/*lang != El::Lang::null &&*/ lang_counter.get() != 0)
        {
          LangCounterMap::iterator i = lang_counter->find(lang);
          assert(i != lang_counter->end());

          WordLangInfo& wli = i->second;
        
          if(--wli.messages == 0)
          {          
            lang_counter->erase(i);

            size_t langs = lang_counter->size();
            assert(langs > 0);

            if(langs == 1)
            {
              lang_counter.reset(0);
            }
          }
          else
          {
            if(cap)
            {
              assert(wli.capitalized);
              --wli.capitalized;

//            std::cerr << "CCC: " << wli.capitalized << std::endl;
            }

            assert(wli.capitalized <= wli.messages);
          }
        }

        if(cap)
        {
          assert(capitalized);
          --capitalized;
        }
      }
      
      messages.erase(number);

      assert(viewer || capitalized <= messages.size());      
    }

    //
    // MessageMap class
    //
    inline
    MessageMap::MessageMap() throw(El::Exception)
    {
      set_empty_key(NUMBER_MAX);
      set_deleted_key(NUMBER_MAX - 1);
    }
    
    inline
    MessageMap::MessageMap(unsigned long size) throw(El::Exception)
        : google::dense_hash_map<Number,
                                 const StoredMessage*,
                                 El::Hash::Numeric<Number> >(size)
    {
      set_empty_key(NUMBER_MAX);
      set_deleted_key(NUMBER_MAX - 1);
    }
    
    //
    // SearcheableMessageMap class
    //
    inline
    SearcheableMessageMap::SearcheableMessageMap(
      bool suppress_dups,
      bool viewer,
      uint32_t impression_respected_level,
      WordPairManager* word_pair_manager)
      throw(El::Exception)
        : root_category(new Category()),
          next_number_(0),
          total_words_(0),
          total_word_positions_(0),
          total_norm_forms_(0),
          total_norm_form_positions_(0),
          suppress_duplicates_(suppress_dups),
          viewer_(viewer),
          impression_respected_level_(impression_respected_level),
          word_pair_manager_(!viewer ? word_pair_manager : 0)
    {
    }

    inline
    unsigned long
    SearcheableMessageMap::total_words() const throw()
    {
      return total_words_;
    }

    inline
    unsigned long long
    SearcheableMessageMap::total_word_positions() const throw()
    {
      return total_word_positions_;
    }
    
    inline
    unsigned long
    SearcheableMessageMap::total_norm_forms() const throw()
    {
      return total_norm_forms_;
    }
    
    inline
    unsigned long long
    SearcheableMessageMap::total_norm_form_positions() const throw()
    {
      return total_norm_form_positions_;
    }

    inline
    Signature
    SearcheableMessageMap::message_signature(const StoredMessage& msg) const
      throw()
    {
      Signature msg_signature = msg.signature;

      if(!suppress_duplicates_)
      {
        El::CRC(msg_signature, (const unsigned char*)"|", 1);
        
        El::CRC(msg_signature,
                (const unsigned char*)msg.source_url.c_str(),
                msg.source_url.length());
      }

      return msg_signature;
    }

    inline
    size_t
    SearcheableMessageMap::lang_message_count(const El::Lang& lang) const
      throw()
    {
      if(lang == El::Lang::null)
      {
        return messages.size();
      }
      
      LangInfoMap::const_iterator i = lang_info.find(lang);
      return i == lang_info.end() ? 0 : i->second->message_count;
    }
    
    inline
    Number
    SearcheableMessageMap::assign_number() throw(Exception, El::Exception)
    {
      if(!recycled_numbers_.empty())
      {
        Number number = recycled_numbers_.top();
        recycled_numbers_.pop();

        return number;
      }
      
      if(next_number_ == NUMBER_MAX - 1)
      {
        throw Exception("NewsGate::Message::SearcheableMessageMap::"
                        "assign_number: no more numbers");
      }

      return next_number_++;
    }

    inline
    void
    SearcheableMessageMap::recycle_number(Number number) throw(El::Exception)
    {
      recycled_numbers_.push(number);
    }

    inline
    StoredMessage*
    SearcheableMessageMap::find(const Id& id) throw(El::Exception)
    {
      IdToNumberMap::iterator it = id_to_number.find(id);

      if(it == id_to_number.end())
      {
        return 0;
      }

      return const_cast<StoredMessage*>(messages.find(it->second)->second);
    }
    
    inline
    const StoredMessage*
    SearcheableMessageMap::find(const Id& id) const throw(El::Exception)
    {
      IdToNumberMap::const_iterator it = id_to_number.find(id);

      if(it == id_to_number.end())
      {
        return 0;
      }

      return messages.find(it->second)->second;
    }

    inline
    void
    SearcheableMessageMap::set_event_id(StoredMessage& msg,
                                        const El::Luid& event_id)
      throw(El::Exception)
    {
      if(msg.event_id == event_id)
      {
        return;
      }
      
      Number number = id_to_number.find(msg.id)->second;
        
      if(msg.event_id != El::Luid::null)
      {
        NumberSet* number_set = event_to_number.find(msg.event_id)->second;

        if(number_set->size() == 1)
        {
          delete number_set;
          event_to_number.erase(msg.event_id);
        }
        else
        {
          number_set->erase(number);
        }
      }
      
      msg.event_id = event_id;

      if(event_id != El::Luid::null)
      {
        EventToNumberMap::iterator it = event_to_number.find(event_id);

        if(it == event_to_number.end())
        {
          it = event_to_number.insert(
            std::make_pair(event_id, new NumberSet())).first;
        }
        
        it->second->insert(number);
      }
    }

    //
    // StoredMessage::DefaultMessageBuilder class
    //
    inline
    StoredMessage::DefaultMessageBuilder::DefaultMessageBuilder(
      std::ostream& ostr,
      size_t max_len)
      throw(El::Exception)
        : output_(ostr),
          max_length_(max_len),
          length_(0)
    {
    }

    //
    // Category class
    //
    inline
    bool
    Category::empty() const throw()
    {
      return messages.empty() && children.empty();
    }
    
    inline
    NumberSet*
    Category::all_messages() const throw(El::Exception)
    {
      std::auto_ptr<NumberSet> msg(new NumberSet());
      get_messages(*msg);
      return msg.release();
    }
    
    inline
    void
    Category::get_messages(NumberSet& cat_messages) const throw(El::Exception)
    {
      for(MessageMap::const_iterator i(messages.begin()), e(messages.end());
          i != e; ++i)
      {
        cat_messages.insert(i->first);
      }
      
/*      
      cat_messages.insert(messages.begin(), messages.end());
      
      for(CategoryMap::const_iterator it = children.begin();
          it != children.end(); it++)
      {
        it->second->get_messages(cat_messages);
      }
*/
    }
    
    //
    // Categories class
    //
    inline
    void
    Categories::swap(Categories& src) throw(El::Exception)
    {
      std::swap(categorizer_hash, src.categorizer_hash);
      array.swap(src.array);
    }
    
    //
    // WordToMessageNumberMap class
    //
    inline
    WordToMessageNumberMap::WordToMessageNumberMap() throw(El::Exception)
    {
      set_deleted_key(0);
    }

    inline
    WordToMessageNumberMap::~WordToMessageNumberMap() throw()
    {
      for(iterator it = begin(); it != end(); it++)
      {
        StringConstPtr::string_manager.remove(it->first.c_str());
        delete it->second;
      }
    }

    //
    // WordIdToMessageNumberMap class
    //
    inline
    WordIdToMessageNumberMap::WordIdToMessageNumberMap() throw(El::Exception)
    {
      set_deleted_key(0);
    }
    
    inline
    WordIdToMessageNumberMap::~WordIdToMessageNumberMap() throw()
    {
      for(iterator it = begin(); it != end(); it++)
      {
        delete it->second;
      }
    }

    //
    // WordIdToCountMap class
    //
    inline
    WordIdToCountMap::WordIdToCountMap() throw(El::Exception)
    {
      set_deleted_key(0);
    }
    
    //
    // SiteToMessageNumberMap class
    //
    inline
    SiteToMessageNumberMap::SiteToMessageNumberMap() throw(El::Exception)
    {
      set_deleted_key(0);
    }
    
    inline
    SiteToMessageNumberMap::~SiteToMessageNumberMap() throw()
    {
      for(iterator it = begin(); it != end(); it++)
      {
        StringConstPtr::string_manager.remove(it->first.c_str());
        delete it->second;
      }
    }

    //
    // FeedInfo class
    //
    inline
    void
    FeedInfo::optimize_mem_usage() throw(El::Exception)
    {
      messages.resize(0);
      words.resize(0);
    }
    
    //
    // FeedInfoMap class
    //
    inline
    FeedInfoMap::FeedInfoMap() throw(El::Exception)
    {
      set_deleted_key(0);
      set_empty_key(SmartStringConstPtr((const char*)1));
    }
    
    inline
    FeedInfoMap::~FeedInfoMap() throw()
    {
      for(iterator i(begin()), e(end()); i != e; ++i)
      {
        StringConstPtr::string_manager.remove(i->first.c_str());
        delete i->second;
      }
    }

    //
    // MessageWordMap class
    //
    inline
    MessageWordMap::MessageWordMap() throw(El::Exception)
    {
      set_deleted_key(UINT32_MAX);
    }
    
    inline
    MessageWordMap::~MessageWordMap() throw()
    {
      for(iterator i(begin()), e(end()); i != e; ++i)
      {
        delete [] i->second;
      }
    }

    //
    // IdToNumberMap class
    //
    inline
    IdToNumberMap::IdToNumberMap() throw(El::Exception)
    {
      set_deleted_key(Id::zero);
      set_empty_key(Id::nonexistent);
    }
    
    //
    // EventToNumberMap class
    //
    inline
    EventToNumberMap::EventToNumberMap() throw(El::Exception)
    {
      set_deleted_key(El::Luid::null);
      set_empty_key(El::Luid::nonexistent);
    }
    
    inline
    EventToNumberMap::~EventToNumberMap() throw()
    {
      for(iterator it = begin(); it != end(); it++)
      {
        delete it->second;
      }
    }

    //
    // StoredMessageMap class
    //
    inline
    StoredMessageMap::~StoredMessageMap() throw()
    {
      for(iterator i(begin()), e(end()); i != e; ++i)
      {
        delete i->second;
      }
    }

    //
    // MessageCategoryMap class
    //
/*
    inline
    MessageCategoryMap::MessageCategoryMap() throw(El::Exception)
    {
      set_deleted_key(NUMBER_MAX);
      set_empty_key(NUMBER_MAX - 1);
    }
    
    inline
    MessageCategoryMap::~MessageCategoryMap() throw()
    {
      for(iterator i(begin()), e(end()); i != e; ++i)
      {
        delete [] i->second;
      }
    }    
*/    
    //
    // CategoryCounter class
    //
    inline
    CategoryCounter::CategoryCounter() throw(El::Exception)
    {
      set_deleted_key(0);
      set_empty_key(SmartStringConstPtr((const char*)1));
    }

    //
    // LangInfoMap class
    //    
    inline
    LangInfoMap::~LangInfoMap() throw()
    {
      for(iterator i(begin()), e(end()); i != e; ++i)
      {
        delete i->second;
      }
    }
    
    //
    // LocaleCategoryCounter class
    //
    inline
    LocaleCategoryCounter::LocaleCategoryCounter() throw(El::Exception)
    {
      set_deleted_key(El::Locale::nonexistent);
      set_empty_key(El::Locale::nonexistent2);
    }
    
    inline
    LocaleCategoryCounter::~LocaleCategoryCounter() throw()
    {
      for(iterator i(begin()), e(end()); i != e; ++i)
      {
        delete i->second;
      }
    }

    inline
    void
    LocaleCategoryCounter::increment(const El::Lang& lang,
                                     const El::Country& country,
                                     SmartStringConstPtr category)
      throw(El::Exception)
    {
      increment(El::Locale::null, category);

      if(lang != El::Lang::null)
      {
        increment(El::Locale(lang, El::Country::null), category);
      }

      if(country != El::Country::null)
      {
        increment(El::Locale(El::Lang::null, country), category);
      }

      if(lang != El::Lang::null && country != El::Country::null)
      {
        increment(El::Locale(lang, country), category);
      }
    }

    inline
    void
    LocaleCategoryCounter::increment(const El::Locale& locale,
                                     SmartStringConstPtr category)
      throw(El::Exception)
    {
      LocaleCategoryCounter::iterator i = find(locale);

      if(i == end())
      {
        i = insert(std::make_pair(locale, new CategoryCounter())).first;
      }

      i->second->insert(std::make_pair(category, 0)).first->second += 1;
    }
    
    inline
    void
    LocaleCategoryCounter::decrement(const El::Lang& lang,
                                     const El::Country& country,
                                     SmartStringConstPtr category)
      throw(El::Exception)
    {
      decrement(El::Locale::null, category);

      if(lang != El::Lang::null)
      {
        decrement(El::Locale(lang, El::Country::null), category);
      }

      if(country != El::Country::null)
      {
        decrement(El::Locale(El::Lang::null, country), category);
      }

      if(lang != El::Lang::null && country != El::Country::null)
      {
        decrement(El::Locale(lang, country), category);
      }
    }

    inline
    void
    LocaleCategoryCounter::decrement(const El::Locale& locale,
                                     SmartStringConstPtr category)
      throw(El::Exception)
    {
      LocaleCategoryCounter::iterator i = find(locale);
      assert(i != end());

      CategoryCounter& cc = *i->second;
      CategoryCounter::iterator ci = cc.find(category);
      assert(ci != cc.end());
      
      uint32_t& count = ci->second;
      assert(count > 0);
      
      if(--count == 0)
      {
        cc.erase(ci);
      }

      if(cc.empty())
      {
        delete i->second;
        erase(i);
      }
    }
    
    //
    // SmartStringConstPtr class
    //
    inline
    SmartStringConstPtr::SmartStringConstPtr(const char* str)
      throw(El::Exception)
        : StringConstPtr(str)
    {
    }

    inline
    bool
    SmartStringConstPtr::operator==(const SmartStringConstPtr& str) const
      throw()
    {
      return str_ == str.str_;
    }
      
    //
    // SmartStringConstPtrHash class
    //
    inline
    size_t
    SmartStringConstPtrHash::operator()(const SmartStringConstPtr& str) const
      throw(El::Exception)
    {
      return (unsigned long)str.c_str();
    }
  }
}

#endif // _NEWSGATE_SERVER_COMMONS_MESSAGE_STOREDMESSAGE_HPP_
