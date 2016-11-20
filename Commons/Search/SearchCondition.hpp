/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Commons/Search/SearchCondition.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_COMMONS_SEARCH_SEARCHCONDITION_HPP_
#define _NEWSGATE_SERVER_COMMONS_SEARCH_SEARCHCONDITION_HPP_

#include <stdint.h>
#include <limits.h>

#include <memory>
#include <iostream>
#include <list>
#include <vector>
#include <string>
#include <stack>

#include <ext/hash_map>

#include <google/dense_hash_map>
#include <google/dense_hash_set>

#include <El/Exception.hpp>
#include <El/RefCount/All.hpp>
#include <El/String/HashedString.hpp>
#include <El/String/Manip.hpp>
#include <El/Stat.hpp>
#include <El/Lang.hpp>
#include <El/Country.hpp>
#include <El/BinaryStream.hpp>
#include <El/LightArray.hpp>
#include <El/Dictionary/Morphology.hpp>
#include <El/Luid.hpp>

#include <Commons/Feed/Types.hpp>
#include <Commons/Search/SearchExpressionExceptions.hpp>
#include <Commons/Message/Message.hpp>
#include <Commons/Message/StoredMessage.hpp>

namespace NewsGate
{
  namespace Search
  {
    EL_EXCEPTION(Exception, El::ExceptionBase);
    EL_EXCEPTION(InvalidArg, Exception);

    class Condition;
    class Filter;
    
    typedef El::RefCount::SmartPtr<Condition> Condition_var;
    typedef El::RefCount::SmartPtr<Filter> Filter_var;
    typedef std::vector<Condition_var> ConditionArray;
    
    class Condition : public virtual El::RefCount::Interface
    {
    public:

      typedef Message::MessageMap Result;
      typedef std::auto_ptr<Result> ResultPtr;
      typedef std::list<const Result*> ResultList;

      class Context;
      
      class MessageFilter
      {
      public:
        virtual bool satisfy(const Message::StoredMessage& msg,
                             Context& context) const
          throw(El::Exception) = 0;

        virtual ~MessageFilter() throw() {}
      };
      
      typedef std::list<const MessageFilter*> MessageFilterList;

      struct Context
      {
        ResultList intersect_list;
        ResultList skip_list;
        MessageFilterList filters;
        uint64_t time;
        const Message::SearcheableMessageMap& messages;

        Context(const Message::SearcheableMessageMap& messages_val) throw();
      };

      static bool message_in_list(Message::Number number,
                                  ResultList::const_iterator& begin,
                                  ResultList::const_iterator& end)
        throw(El::Exception);
      
      static bool message_not_in_list(Message::Number number,
                                      ResultList::const_iterator& begin,
                                      ResultList::const_iterator& end)
        throw(El::Exception);
      
      class PositionSet :
        public google::dense_hash_set<Message::WordPosition,
                                      El::Hash::Numeric<
                                        Message::WordPosition> >
      {
      public:
        PositionSet() throw(El::Exception);
        PositionSet(unsigned long size) throw(El::Exception);
      };
      
      typedef std::auto_ptr<PositionSet> PositionSetPtr;
      
      typedef __gnu_cxx::hash_map<Message::Number,
                                  PositionSet,
                                  El::Hash::Numeric<Message::Number> >
      WordPositionSets;
      
      typedef std::auto_ptr<WordPositionSets> WordPositionSetsPtr;

      class CoreWordsSet :
        public google::dense_hash_set<uint32_t,
                                      El::Hash::Numeric<uint32_t> >
      {
      public:
        CoreWordsSet() throw(El::Exception);
        CoreWordsSet(unsigned long size) throw(El::Exception);
      };
      
      typedef std::auto_ptr<CoreWordsSet> CoreWordsSetPtr;

      struct MessageMatchInfo
      {
        PositionSetPtr positions;
        CoreWordsSetPtr core_words; // matched core words

        MessageMatchInfo() throw(El::Exception);
        
        MessageMatchInfo(const MessageMatchInfo& mmi) throw(El::Exception);
        
        MessageMatchInfo& operator=(const MessageMatchInfo& mmi)
          throw(El::Exception);

        void insert(const MessageMatchInfo& mmi) throw(El::Exception);
        void steal(MessageMatchInfo& mmi) throw(El::Exception);

        bool empty() throw(El::Exception);
      };

      struct MessageMatchInfoMap :
        public __gnu_cxx::hash_map<Message::Number,
                                   MessageMatchInfo,
                                   El::Hash::Numeric<Message::Number> >
      {
        void unite(const MessageMatchInfoMap& src) throw(El::Exception);
      };

      struct Time
      {
        EL_EXCEPTION(Exception, NewsGate::Search::Exception);
        EL_EXCEPTION(InvalidArg, Exception);
        
        enum OffsetType
        {
          OT_FROM_NOW,
          OT_SINCE_EPOCH
        };

        uint64_t offset;
        OffsetType type;

        Time(uint64_t offset_val = 0,
             OffsetType type_val = OT_SINCE_EPOCH) throw();

        Time(const wchar_t* val) throw(InvalidArg, El::Exception);

        uint64_t value(const Context& context) const throw();
        
        void print(std::ostream& ostr) const throw(El::Exception);
        void dump(std::wostream& ostr) const throw(El::Exception);

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);
      };

      enum EvaluateFlags
      {
        EF_FILL_POSITIONS = 0x1,
        EF_FILL_CORE_WORDS = 0x2,
        EF_SEARCH_HIDDEN = 0x4
      };

      Condition() throw();
      virtual ~Condition() throw();      
      
      virtual Result* evaluate_simple(Context& context,
                                      MessageMatchInfoMap& match_info,
                                      unsigned long flags) const
        throw(El::Exception) = 0;

      enum Type
      {
        TP_ALL,
        TP_ANY,
        TP_SITE,
        TP_URL,
        TP_MSG,
        TP_EVENT,
        TP_EVERY,
        TP_OR,
        TP_AND,
        TP_EXCEPT,
        TP_PUB_DATE,
        TP_FETCHED,
        TP_LANG,
        TP_COUNTRY,
        TP_SPACE,
        TP_DOMAIN,
        TP_CAPACITY,
        TP_CATEGORY,
        TP_WITH,
        TP_SIGNATURE,
        TP_NONE,
        TP_IMPRESSIONS,
        TP_CLICKS,
        TP_CTR,
        TP_RCTR,
        TP_FEED_IMPRESSIONS,
        TP_FEED_CLICKS,
        TP_FEED_CTR,
        TP_FEED_RCTR,
        TP_VISITED
      };

      struct LangSet :
        public google::dense_hash_set<El::Lang, El::Hash::Lang>
      {
        LangSet() throw() { set_empty_key(El::Lang::nonexistent); }
      };

      struct CountrySet :
        public google::dense_hash_set<El::Country, El::Hash::Country>
      {
        CountrySet() throw() { set_empty_key(El::Country::nonexistent); }
      };
     
      struct FeedSpaceSet :
        public google::dense_hash_set<
          ::NewsGate::Feed::Space,
          El::Hash::Numeric< ::NewsGate::Feed::Space> >
      {
        FeedSpaceSet() throw()
        { set_empty_key(::NewsGate::Feed::SP_SPACES_COUNT); }
      };

      struct FeedTypeSet :
        public google::dense_hash_set<
          ::NewsGate::Feed::Type,
          El::Hash::Numeric< ::NewsGate::Feed::Type> >
      {
        FeedTypeSet() throw()
        { set_empty_key(::NewsGate::Feed::TP_TYPES_COUNT); }
      };

      struct StringConstPtrSet :
        public google::dense_hash_set<El::String::StringConstPtr,
                                      El::Hash::StringConstPtr>
      {
        StringConstPtrSet() throw() { set_empty_key(0); }
      };

      typedef std::vector<std::string> StringArray;
      typedef std::vector<size_t> SizeArray;

      static Condition* create(Type type) throw(El::Exception);

      virtual Type type() const throw() = 0;

      virtual void print(std::ostream& ostr) const throw(El::Exception) = 0;

      virtual void dump(std::wostream& ostr, std::wstring& ident) const
        throw(El::Exception) = 0;

      virtual void write(El::BinaryOutStream& bstr) const
        throw(El::Exception);
      
      virtual void read(El::BinaryInStream& bstr) throw(El::Exception);

      virtual const Result* const_evaluate(Context& context,
                                           unsigned long flags) const
        throw(El::Exception);
      
      virtual Result* evaluate(Context& context,
                               MessageMatchInfoMap& match_info,
                               unsigned long flags) const
        throw(El::Exception) = 0;

      virtual void normalize(
        const El::Dictionary::Morphology::WordInfoManager& word_info_manager)
        throw(El::Exception) = 0;

      virtual Condition* optimize() throw(El::Exception);
      virtual void relax(size_t level) throw(El::Exception);
      
      virtual Condition* intersect(Condition_var& condition)
        throw(El::Exception);

      virtual Condition* filter(Filter_var& condition) throw(El::Exception);
      
      virtual Condition* exclude(Condition_var& condition)
        throw(El::Exception);

      virtual ConditionArray subconditions() const throw(El::Exception);

      uint64_t cleanup_opt_info() throw(El::Exception);
      
    protected:

      struct OptimizationInfo
      {
        virtual ~OptimizationInfo() throw() {}
      };

      std::auto_ptr<OptimizationInfo> optimization_info_;

    private:
      void operator=(const Condition&);
      Condition(const Condition&);
    };

    class None : public virtual Condition,
                 public virtual El::RefCount::DefaultImpl<
                   El::Sync::ThreadPolicy>
    {
    public:

      virtual ~None() throw() {}
      None() throw() {}
      
      virtual void print(std::ostream& ostr) const throw(El::Exception);

      virtual void dump(std::wostream& ostr, std::wstring& ident) const 
        throw(El::Exception);
      
      virtual Type type() const throw() { return TP_NONE; }

      virtual void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      virtual void read(El::BinaryInStream& bstr) throw(El::Exception);
      
      virtual Result* evaluate(
        Context& context,
        MessageMatchInfoMap& match_info,
        unsigned long flags) const
        throw(El::Exception);

      virtual Result* evaluate_simple(Context& context,
                                      MessageMatchInfoMap& match_info,
                                      unsigned long flags) const
        throw(El::Exception);

      virtual void normalize(
        const El::Dictionary::Morphology::WordInfoManager& word_info_manager)
        throw(El::Exception);
      
      static El::Stat::TimeMeter evaluate_meter;

    private:
      void operator=(const None&);
      None(const None&);
    };
    
    typedef El::LightArray<El::Dictionary::Morphology::WordId, uint32_t>
    WordIdArray;
    
    struct Word
    {
      enum WordFlags
      {
        WF_GROUP  = 0x3,
        WF_EXACT  = 0x4,
        WF_HAS_NF = 0x8
      };

      uint8_t flags;
      Message::StringConstPtr text;
      WordIdArray norm_forms;

      enum Relation
      {
        WR_DIFERENT,
        WR_SAME,
        WR_MORE_SPECIFIC,
        WR_LESS_SPECIFIC
      };

      Word() throw(El::Exception);

      size_t common_norm_forms(const Word& w) const throw();
      
      bool same_text(const Word& w) const throw();
      Relation relation(const Word& w) const throw();

      unsigned char group() const throw();
      void group(unsigned char val) throw();

      bool exact() const throw();
      void exact(bool val) throw();

      void set_norm_forms_flag() throw();
      bool use_norm_forms() const throw();

      void write(El::BinaryOutStream& ostr) const
        throw(Exception, El::Exception);

      void read(El::BinaryInStream& istr) throw(Exception, El::Exception);
    };

    typedef std::vector<Word> WordList;
    
    class Words : public virtual Condition,
                  public virtual El::RefCount::DefaultImpl<
                    El::Sync::ThreadPolicy>
    {
    public:

      Words() throw(El::Exception) : word_flags(0) {}
      
      virtual void dump(std::wostream& ostr, std::wstring& ident) const 
        throw(El::Exception);
      
      virtual ~Words() throw();

      virtual void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      virtual void read(El::BinaryInStream& bstr) throw(El::Exception);

      virtual void normalize(
        const El::Dictionary::Morphology::WordInfoManager& word_info_manager)
        throw(El::Exception);
      
      static void print_phrase(std::ostream& ostr,
                               WordList::const_iterator i,
                               WordList::const_iterator e,
                               size_t len) throw();
      
      enum Flags
      {
        WF_CORE = 0x1,
        WF_TITLE = 0x2,
        WF_DESC = 0x4,
        WF_IMG_ALT = 0x8,
        WF_KEYWORDS = 0x10
      };
      
      WordList words;
      uint32_t word_flags;
 
    protected:

      WordList::const_iterator next_phrase(WordList::const_iterator i,
                                           const WordList::const_iterator& e,
                                           size_t& len) const
        throw();
      
      WordList::iterator next_phrase(WordList::iterator i,
                                     const WordList::iterator& e,
                                     size_t& len) const
        throw();
      
      static Word::Relation relation(WordList::const_iterator wi1,
                                     const WordList::const_iterator& we1,
                                     size_t len1,
                                     WordList::const_iterator wi2,
                                     const WordList::const_iterator& we2,
                                     size_t len2,
                                     bool use_norm_forms)
        throw();
      
      static Word::Relation relation_assymetrical(
        WordList::const_iterator si,
        const WordList::const_iterator& se,
        WordList::const_iterator li,
        size_t iterations,
        bool use_norm_forms)
        throw();
      
      static void add_match_info(MessageMatchInfoMap& match_info,
                                 unsigned long flags,
                                 Message::Number number,
                                 const Message::StoredMessage& msg,
                                 const Word& word)
        throw(El::Exception);
      
      static void add_match_info(MessageMatchInfo& match_info,
                                 unsigned long flags,
                                 const Message::StoredMessage& msg,
                                 const Word& word)
        throw(El::Exception);
      
      bool satisfy(Message::WordPosition pos,
                   const Message::StoredMessage& msg) const throw();
        
      bool satisfy(const Message::WordPositions& positions,
                   const Message::StoredMessage& msg) const throw();
      
    protected:

      class SeqSet :
        public google::dense_hash_set<
          Message::WordPosition,
          El::Hash::Numeric<Message::WordPosition> >
      {
      public:
        SeqSet() throw(El::Exception);
      };

      typedef __gnu_cxx::hash_map<Message::WordPosition,
                                  SeqSet,
                                  El::Hash::Numeric<Message::WordPosition> >
      PositionSeqSet;

      typedef std::auto_ptr<PositionSeqSet> PositionSeqSetPtr;

    private:
      void operator=(const Words&);
      Words(const Words&);
    };
    
    class AllWords : public Words
    {
    public:

      AllWords() throw() {}
      virtual ~AllWords() throw() {}
      
      virtual void print(std::ostream& ostr) const throw(El::Exception);
      virtual void relax(size_t level) throw(El::Exception);

      virtual void dump(std::wostream& ostr,
                        std::wstring& ident) const throw(El::Exception);      

      virtual Type type() const throw();
        
      virtual Result* evaluate(Context& context,
                               MessageMatchInfoMap& match_info,
                               unsigned long flags) const
        throw(El::Exception);
      
      virtual Result* evaluate_simple(Context& context,
                                      MessageMatchInfoMap& match_info,
                                      unsigned long flags) const
        throw(El::Exception);

      Result* evaluate(Context& context,
                       Result* res,
                       MessageMatchInfoMap& match_info,
                       unsigned long flags) const
        throw(El::Exception);

      static El::Stat::TimeMeter evaluate_meter;

    private:
      
      bool precheck_word_sequence(const Message::StoredMessage& msg,
                                  unsigned long word_group,
                                  unsigned long prev_word_group,
                                  unsigned long flags,
                                  WordList::const_iterator wit,
                                  WordList::const_iterator& group_wit,
                                  unsigned long& word_seq_in_group,
                                  PositionSeqSetPtr& group_positions,
                                  MessageMatchInfo& match_info)
        const throw(El::Exception);
      
      static void postcheck_word_sequence(unsigned long word_group,
                                          unsigned long prev_word_group,
                                          unsigned long& word_seq_in_group)
        throw(El::Exception);

      static bool check_word_sequence(
        unsigned long word_group,
        unsigned long prev_word_group,
        unsigned long& word_seq_in_group,
        const Message::WordPositionArray& positions,
        const Message::WordPositions& word_position,
        PositionSeqSetPtr& group_positions)
        throw(El::Exception);

    private:
      void operator=(const AllWords&);
      AllWords(const AllWords&);
    };
    
    class AnyWords : public Words
    {
    public:
      
      AnyWords() throw(El::Exception);
      virtual ~AnyWords() throw() {}

      virtual void print(std::ostream& ostr) const throw(El::Exception);
      virtual void relax(size_t level) throw(El::Exception);

      virtual void dump(std::wostream& ostr,
                        std::wstring& ident) const throw(El::Exception);      

      virtual Type type() const throw();

      virtual void normalize(
        const El::Dictionary::Morphology::WordInfoManager& word_info_manager)
        throw(El::Exception);

      virtual Result* evaluate(Context& context,
                               MessageMatchInfoMap& match_info,
                               unsigned long flags) const
        throw(El::Exception);

      virtual Result* evaluate_simple(Context& context,
                                      MessageMatchInfoMap& match_info,
                                      unsigned long flags) const
        throw(El::Exception);

      virtual void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      virtual void read(El::BinaryInStream& bstr) throw(El::Exception);

      void remove_duplicates(bool use_norm_forms) throw();

      uint32_t match_threshold;

      static El::Stat::TimeMeter evaluate_meter;

    private:
      
      static const Message::StoredMessage*
      find_msg(Message::Number number,
               const Message::StoredMessageMap& stored_message,
               Context& context,
               bool search_hidden,
               ResultList::const_iterator& intersect_list_begin,
               ResultList::const_iterator& intersect_list_end,
               ResultList::const_iterator& skip_list_begin,
               ResultList::const_iterator& skip_list_end,
               MessageFilterList::const_iterator& filters_begin,
               MessageFilterList::const_iterator& filters_end)
        throw(El::Exception);
      
      Result* evaluate_one(Context& context,
                           MessageMatchInfoMap& match_info,
                           unsigned long flags) const
        throw(El::Exception);

      Result* evaluate_many(Context& context,
                            MessageMatchInfoMap& match_info,
                            unsigned long flags) const
        throw(El::Exception);

    private:
      void operator=(const AnyWords&);
      AnyWords(const AnyWords&);
    };
    
    class Site : public virtual Condition,
                 public virtual El::RefCount::DefaultImpl<
                   El::Sync::ThreadPolicy>
    {
    public:

      Site() throw() {}
      virtual ~Site() throw() {}

      virtual void print(std::ostream& ostr) const throw(El::Exception);

      virtual void dump(std::wostream& ostr, std::wstring& ident) const 
        throw(El::Exception);      

      virtual Type type() const throw();

      virtual Condition* intersect(Condition_var& condition)
        throw(El::Exception);
      
      virtual Condition* filter(Filter_var& condition) throw(El::Exception);
      
      virtual Condition* exclude(Condition_var& condition)
        throw(El::Exception);

      virtual void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      virtual void read(El::BinaryInStream& bstr) throw(El::Exception);
      
      virtual Result* evaluate(Context& context,
                               MessageMatchInfoMap& match_info,
                               unsigned long flags) const
        throw(El::Exception);

      virtual Result* evaluate_simple(Context& context,
                                      MessageMatchInfoMap& match_info,
                                      unsigned long flags) const
        throw(El::Exception);

      virtual void normalize(
        const El::Dictionary::Morphology::WordInfoManager& word_info_manager)
        throw(El::Exception) {}
      
//      typedef std::list<Message::StringConstPtr> HostNameList;
      typedef std::vector<Message::StringConstPtr> HostNameList;
      HostNameList hostnames;

      struct SiteOptimizationInfo : public OptimizationInfo
      {
        StringConstPtrSet hosts;
        SizeArray host_lengths;

      protected:
        ~SiteOptimizationInfo() throw() {}
      };

      SiteOptimizationInfo* optimization_info()
        throw(Exception, El::Exception);
      
      static El::Stat::TimeMeter evaluate_meter;

    private:
      void operator=(const Site&);
      Site(const Site&);
    };
    
    class Url : public virtual Condition,
                public virtual El::RefCount::DefaultImpl<
                  El::Sync::ThreadPolicy>
    {
    public:
      
      Url() throw() {}
      virtual ~Url() throw() {}
      
      virtual void print(std::ostream& ostr) const throw(El::Exception);

      virtual void dump(std::wostream& ostr, std::wstring& ident) const 
        throw(El::Exception);

      virtual Type type() const throw();

      virtual Condition* intersect(Condition_var& condition)
        throw(El::Exception);
      
      virtual Condition* filter(Filter_var& condition) throw(El::Exception);

      virtual Condition* exclude(Condition_var& condition)
        throw(El::Exception);

      virtual void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      virtual void read(El::BinaryInStream& bstr) throw(El::Exception);
      
      virtual Result* evaluate(Context& context,
                               MessageMatchInfoMap& match_info,
                               unsigned long flags) const
        throw(El::Exception);

      virtual Result* evaluate_simple(Context& context,
                                      MessageMatchInfoMap& match_info,
                                      unsigned long flags) const
        throw(El::Exception);

      virtual void normalize(
        const El::Dictionary::Morphology::WordInfoManager& word_info_manager)
        throw(El::Exception) {}
      
//      typedef std::list<Message::StringConstPtr> UrlList;
      typedef std::vector<Message::StringConstPtr> UrlList;
      UrlList urls;

      struct UrlOptimizationInfo : public OptimizationInfo
      {
        struct UrlSite
        {
          std::string name;
          size_t len;

          UrlSite() : len(0) {}
        };

        typedef std::vector<UrlSite> UrlSiteArray;
        
        StringConstPtrSet urls;
        UrlSiteArray sites;

      protected:
        ~UrlOptimizationInfo() throw() {}
      };

      UrlOptimizationInfo* optimization_info() throw(Exception, El::Exception);

      static El::Stat::TimeMeter evaluate_meter;      

    private:
      void operator=(const Url&);
      Url(const Url&);
    };
    
    class Category : public virtual Condition,
                     public virtual El::RefCount::DefaultImpl<
                       El::Sync::ThreadPolicy>
    {
    public:

      Category() throw() {}
      virtual ~Category() throw() {}

      virtual void print(std::ostream& ostr) const throw(El::Exception);

      virtual void dump(std::wostream& ostr, std::wstring& ident) const 
        throw(El::Exception);
      
      virtual Type type() const throw();

      virtual void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      virtual void read(El::BinaryInStream& bstr) throw(El::Exception);
      
      virtual const Result* const_evaluate(Context& context,
                                           unsigned long flags) const
        throw(El::Exception);
      
      virtual Result* evaluate(Context& context,
                               MessageMatchInfoMap& match_info,
                               unsigned long flags) const
        throw(El::Exception);

      virtual Result* evaluate_simple(Context& context,
                                      MessageMatchInfoMap& match_info,
                                      unsigned long flags) const
        throw(El::Exception);

      virtual void normalize(
        const El::Dictionary::Morphology::WordInfoManager& word_info_manager)
        throw(El::Exception) {}
      
      typedef El::LightArray<Message::StringConstPtr, uint32_t> CategoryArray;
      CategoryArray categories;

      static El::Stat::TimeMeter evaluate_meter;

    private:
      
      void get_messages(Context& context,
                        bool search_hidden,
                        const Message::Category* category,
                        Result& result) const
        throw(El::Exception);

    private:
      void operator=(const Category&);
      Category(const Category&);
    };
    
    class Msg : public virtual Condition,
                public virtual El::RefCount::DefaultImpl<
                  El::Sync::ThreadPolicy>
    {
    public:

      Msg() throw() {}
      virtual ~Msg() throw() {}
      
      virtual void print(std::ostream& ostr) const throw(El::Exception);

      virtual void dump(std::wostream& ostr, std::wstring& ident) const 
        throw(El::Exception);      

      virtual Type type() const throw();

      virtual void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      virtual void read(El::BinaryInStream& bstr) throw(El::Exception);
      
      virtual Result* evaluate(Context& context,
                               MessageMatchInfoMap& match_info,
                               unsigned long flags) const
        throw(El::Exception);

      virtual Result* evaluate_simple(Context& context,
                                      MessageMatchInfoMap& match_info,
                                      unsigned long flags) const
        throw(El::Exception);

      virtual void normalize(
        const El::Dictionary::Morphology::WordInfoManager& word_info_manager)
        throw(El::Exception) {}
      
      Message::IdArray ids;

      static El::Stat::TimeMeter evaluate_meter;      

    private:
      void operator=(const Msg&);
      Msg(const Msg&);
    };

    typedef El::RefCount::SmartPtr<Msg> Msg_var;    

    typedef std::vector<El::Luid> LuidArray;
    
    class Event : public virtual Condition,
                  public virtual El::RefCount::DefaultImpl<
                    El::Sync::ThreadPolicy>
    {
    public:

      Event() throw() {}
      virtual ~Event() throw() {}

      virtual void print(std::ostream& ostr) const throw(El::Exception);

      virtual void dump(std::wostream& ostr, std::wstring& ident) const 
        throw(El::Exception);
      
      virtual Type type() const throw();

      virtual void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      virtual void read(El::BinaryInStream& bstr) throw(El::Exception);
      
      virtual Result* evaluate(Context& context,
                               MessageMatchInfoMap& match_info,
                               unsigned long flags) const
        throw(El::Exception);

      virtual Result* evaluate_simple(Context& context,
                                      MessageMatchInfoMap& match_info,
                                      unsigned long flags) const
        throw(El::Exception);

      virtual void normalize(
        const El::Dictionary::Morphology::WordInfoManager& word_info_manager)
        throw(El::Exception) {}
      
      LuidArray ids;

      static El::Stat::TimeMeter evaluate_meter;      

    private:
      void operator=(const Event&);
      Event(const Event&);
    };

    typedef El::RefCount::SmartPtr<Event> Event_var;    
    
    class Every : public virtual Condition,
                  public virtual El::RefCount::DefaultImpl<
                    El::Sync::ThreadPolicy>
    {
    public:

      Every() throw() {}
      virtual ~Every() throw() {}

      virtual void print(std::ostream& ostr) const throw(El::Exception);

      virtual void dump(std::wostream& ostr, std::wstring& ident) const 
        throw(El::Exception);      

      virtual Type type() const throw();

      virtual Condition* intersect(Condition_var& condition)
        throw(El::Exception);

      virtual void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      virtual void read(El::BinaryInStream& bstr) throw(El::Exception);
      
      virtual const Result* const_evaluate(Context& context,
                                           unsigned long flags) const
        throw(El::Exception);
      
      virtual Result* evaluate(Context& context,
                               MessageMatchInfoMap& match_info,
                               unsigned long flags) const
        throw(El::Exception);

      virtual Result* evaluate_simple(Context& context,
                                      MessageMatchInfoMap& match_info,
                                      unsigned long flags) const
        throw(El::Exception);

      virtual void normalize(
        const El::Dictionary::Morphology::WordInfoManager& word_info_manager)
        throw(El::Exception) {}
      
      static El::Stat::TimeMeter evaluate_meter;      

    private:
      void operator=(const Every&);
      Every(const Every&);
    };
    
    class Or : public virtual Condition,
               public virtual El::RefCount::DefaultImpl<
                 El::Sync::ThreadPolicy>
    {
    public:

      Or() throw() {}
      virtual ~Or() throw() {}

      virtual void print(std::ostream& ostr) const throw(El::Exception);

      virtual void dump(std::wostream& ostr,
                        std::wstring& ident) const throw(El::Exception);      

      virtual Type type() const throw();

      virtual Condition* optimize() throw(El::Exception);
      virtual Condition* filter(Filter_var& condition) throw(El::Exception);
      
      virtual Condition* intersect(Condition_var& condition)
        throw(El::Exception);
      
      virtual Condition* exclude(Condition_var& condition)
        throw(El::Exception);

      Condition* check_trivial() throw(El::Exception);
      
      virtual void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      virtual void read(El::BinaryInStream& bstr) throw(El::Exception);
      
      virtual Result* evaluate(Context& context,
                               MessageMatchInfoMap& match_info,
                               unsigned long flags) const
        throw(El::Exception);

      virtual Result* evaluate_simple(Context& context,
                                      MessageMatchInfoMap& match_info,
                                      unsigned long flags) const
        throw(El::Exception);

      virtual void normalize(
        const El::Dictionary::Morphology::WordInfoManager& word_info_manager)
        throw(El::Exception);
      
      virtual ConditionArray subconditions() const throw(El::Exception);
      
    public:
      
      ConditionArray operands;

      static El::Stat::TimeMeter evaluate_meter;
      static El::Stat::TimeMeter evaluate_simple_meter;
      
    private:
      void operator=(const Or&);
      Or(const Or&);
    };

    class And : public virtual Condition,
                public virtual El::RefCount::DefaultImpl<
                  El::Sync::ThreadPolicy>
    {
    public:

      And() throw() {}
      virtual ~And() throw() {}

      virtual void print(std::ostream& ostr) const throw(El::Exception);

      virtual void dump(std::wostream& ostr,
                        std::wstring& ident) const throw(El::Exception);
      
      virtual Type type() const throw();

      virtual Condition* optimize() throw(El::Exception);
      virtual Condition* filter(Filter_var& condition) throw(El::Exception);

      virtual Condition* intersect(Condition_var& condition)
        throw(El::Exception);
      
      virtual Condition* exclude(Condition_var& condition)
        throw(El::Exception);

      virtual void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      virtual void read(El::BinaryInStream& bstr) throw(El::Exception);
      
      virtual Result* evaluate(Context& context,
                               MessageMatchInfoMap& match_info,
                               unsigned long flags) const
        throw(El::Exception);

      virtual Result* evaluate_simple(Context& context,
                                      MessageMatchInfoMap& match_info,
                                      unsigned long flags) const
        throw(El::Exception);

      virtual void normalize(
        const El::Dictionary::Morphology::WordInfoManager& word_info_manager)
        throw(El::Exception);
      
      virtual ConditionArray subconditions() const throw(El::Exception);

    public:
      
      ConditionArray operands;

      static El::Stat::TimeMeter evaluate_meter;
      static El::Stat::TimeMeter evaluate_simple_meter;

    protected:
      
      static void intersect_results(Result& result,
                                    MessageMatchInfoMap& match_info,
                                    unsigned long flags,
                                    const Result& op_result,
                                    const MessageMatchInfoMap& op_match_info)
        throw(El::Exception);
      
    private:
      void operator=(const And&);
      And(const And&);
    };

    class Except : public virtual Condition,
                   public virtual El::RefCount::DefaultImpl<
                     El::Sync::ThreadPolicy>
    {
    public:

      Except() throw() {}
      virtual ~Except() throw() {}

      virtual void print(std::ostream& ostr) const throw(El::Exception);
      virtual void relax(size_t level) throw(El::Exception);

      virtual void dump(std::wostream& ostr, std::wstring& ident) const
        throw(El::Exception);
      
      virtual Type type() const throw();

      virtual Condition* optimize() throw(El::Exception);
      virtual Condition* filter(Filter_var& condition) throw(El::Exception);

      virtual Condition* intersect(Condition_var& condition)
        throw(El::Exception);
      
      virtual Condition* exclude(Condition_var& condition)
        throw(El::Exception);

      virtual void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      virtual void read(El::BinaryInStream& bstr) throw(El::Exception);
      
      virtual Result* evaluate(Context& context,
                               MessageMatchInfoMap& match_info,
                               unsigned long flags) const
        throw(El::Exception);

      virtual Result* evaluate_simple(Context& context,
                                      MessageMatchInfoMap& match_info,
                                      unsigned long flags) const
        throw(El::Exception);

      virtual void normalize(
        const El::Dictionary::Morphology::WordInfoManager& word_info_manager)
        throw(El::Exception);
      
      virtual ConditionArray subconditions() const throw(El::Exception);
      
    public:
      Condition_var left;
      Condition_var right;

      static El::Stat::TimeMeter evaluate_meter;
      static El::Stat::TimeMeter evaluate_simple_meter;      
      
    private:
      void operator=(const Except&);
      Except(const Except&);
    };

    typedef El::RefCount::SmartPtr<Except> Except_var;
    
    class Filter : public virtual Condition::MessageFilter,
                   public virtual Condition,
                   public virtual El::RefCount::DefaultImpl<
                     El::Sync::ThreadPolicy>
    {
    public:
      
      Filter() throw() : reversed(0) {}
      virtual ~Filter() throw() {}
      
      virtual Condition* optimize() throw(El::Exception);
      virtual Condition* filter(Filter_var& condition) throw(El::Exception);

      virtual Condition* intersect(Condition_var& condition)
        throw(El::Exception);      
      
      virtual Condition* exclude(Condition_var& condition)
        throw(El::Exception);

      virtual void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      virtual void read(El::BinaryInStream& bstr) throw(El::Exception);
      
      virtual Result* evaluate(Context& context,
                               MessageMatchInfoMap& match_info,
                               unsigned long flags) const
        throw(El::Exception);

      virtual Condition::Result* evaluate_simple(
        Context& context,
        MessageMatchInfoMap& match_info,
        unsigned long flags) const
        throw(El::Exception);
      
      virtual void normalize(
        const El::Dictionary::Morphology::WordInfoManager& word_info_manager)
        throw(El::Exception);
      
      virtual ConditionArray subconditions() const throw(El::Exception);
      
    public:
      Condition_var condition;
      uint8_t reversed;
      
    private:
      void operator=(const Filter&);
      Filter(const Filter&);
    };

    template<const Condition::Type condition_type,
             typename VALUE_LIST,
             typename VALUE_SET>
    class FilterValueList : public virtual Filter
    {
    public:

      typedef VALUE_LIST ValueList;
      typedef VALUE_SET  ValueSet;

      typedef FilterValueList<condition_type, VALUE_LIST, VALUE_SET>
      FilterValueListType;

      ~FilterValueList() throw() {}
      
      virtual Type type() const throw();
      
      virtual Condition* filter(Filter_var& condition) throw(El::Exception);

      virtual void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      virtual void read(El::BinaryInStream& bstr) throw(El::Exception);      
      
      struct OptimizationInfo : public Condition::OptimizationInfo
      {
        ValueSet values;

      protected:
        ~OptimizationInfo() throw() {}
      };

      OptimizationInfo* optimization_info() throw(Exception, El::Exception);
      
    public:
      
      ValueList values;
    };
    
    class Lang : public virtual FilterValueList<Condition::TP_LANG,
                                                std::vector<El::Lang>,
                                                Condition::LangSet >
    {
    public:
      Lang() throw() {}
      virtual ~Lang() throw() {}
      
      virtual void print(std::ostream& ostr) const throw(El::Exception);

      virtual void dump(std::wostream& ostr,
                        std::wstring& ident) const throw(El::Exception);      

      virtual Result* evaluate(Context& context,
                               MessageMatchInfoMap& match_info,
                               unsigned long flags) const
        throw(El::Exception);

      virtual Result* evaluate_simple(Context& context,
                                      MessageMatchInfoMap& match_info,
                                      unsigned long flags) const
        throw(El::Exception);

      virtual bool satisfy(const Message::StoredMessage& msg,
                           Context& context) const
        throw(El::Exception);
      
    public:
      
      static El::Stat::TimeMeter evaluate_meter;
      static El::Stat::TimeMeter evaluate_simple_meter;      
      
    private:
      void operator=(const Lang&);
      Lang(const Lang&);
    };

    typedef El::RefCount::SmartPtr<Lang> Lang_var;

    class Country : public virtual FilterValueList<Condition::TP_COUNTRY,
                                                   std::vector<El::Country>,
                                                   Condition::CountrySet >
    {
    public:
      Country() throw() {}
      virtual ~Country() throw() {}
      
      virtual void print(std::ostream& ostr) const throw(El::Exception);

      virtual void dump(std::wostream& ostr,
                        std::wstring& ident) const throw(El::Exception);      

      virtual Result* evaluate(Context& context,
                               MessageMatchInfoMap& match_info,
                               unsigned long flags) const
        throw(El::Exception);

      virtual Result* evaluate_simple(Context& context,
                                      MessageMatchInfoMap& match_info,
                                      unsigned long flags) const
        throw(El::Exception);

      virtual bool satisfy(const Message::StoredMessage& msg,
                           Context& context) const
        throw(El::Exception);      
      
    public:
      static El::Stat::TimeMeter evaluate_meter;
      static El::Stat::TimeMeter evaluate_simple_meter;      
      
    private:
      void operator=(const Country&);
      Country(const Country&);
    };

    typedef El::RefCount::SmartPtr<Country> Country_var;
    
    class Space :
      public virtual FilterValueList<Condition::TP_SPACE,
                                     std::vector< ::NewsGate::Feed::Space >,
                                     Condition::FeedSpaceSet >
    {
    public:
      Space() throw() {}
      virtual ~Space() throw() {}
      
      virtual void print(std::ostream& ostr) const throw(El::Exception);

      virtual void dump(std::wostream& ostr,
                        std::wstring& ident) const throw(El::Exception);      

      virtual Result* evaluate(Context& context,
                               MessageMatchInfoMap& match_info,
                               unsigned long flags) const
        throw(El::Exception);

      virtual Result* evaluate_simple(Context& context,
                                      MessageMatchInfoMap& match_info,
                                      unsigned long flags) const
        throw(El::Exception);

      virtual bool satisfy(const Message::StoredMessage& msg,
                           Context& context) const
        throw(El::Exception);
      
    public:
      static El::Stat::TimeMeter evaluate_meter;
      static El::Stat::TimeMeter evaluate_simple_meter;      
      
    private:
      void operator=(const Space&);
      Space(const Space&);
    };

    class Fetched : public virtual Filter
    {
    public:
      Fetched() throw() {}
      virtual ~Fetched() throw() {}
      
      virtual Type type() const throw();

      virtual void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      virtual void read(El::BinaryInStream& bstr) throw(El::Exception);
      
      virtual void print(std::ostream& ostr) const throw(El::Exception);

      virtual void dump(std::wostream& ostr,
                        std::wstring& ident) const throw(El::Exception);      

      virtual Result* evaluate(Context& context,
                               MessageMatchInfoMap& match_info,
                               unsigned long flags) const
        throw(El::Exception);

      virtual Result* evaluate_simple(Context& context,
                                      MessageMatchInfoMap& match_info,
                                      unsigned long flags) const
        throw(El::Exception);

      virtual bool satisfy(const Message::StoredMessage& msg,
                           Context& context) const
        throw(El::Exception);
      
    public:
      Time time;

      static El::Stat::TimeMeter evaluate_meter;
      static El::Stat::TimeMeter evaluate_simple_meter;
      
    private:
      void operator=(const Fetched&);
      Fetched(const Fetched&);
    };

    typedef El::RefCount::SmartPtr<Fetched> Fetched_var;    

    class Visited : public virtual Filter
    {
    public:
      Visited() throw() {}
      virtual ~Visited() throw() {}
      
      virtual Type type() const throw();

      virtual void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      virtual void read(El::BinaryInStream& bstr) throw(El::Exception);
      
      virtual void print(std::ostream& ostr) const throw(El::Exception);

      virtual void dump(std::wostream& ostr,
                        std::wstring& ident) const throw(El::Exception);      

      virtual Result* evaluate(Context& context,
                               MessageMatchInfoMap& match_info,
                               unsigned long flags) const
        throw(El::Exception);

      virtual Result* evaluate_simple(Context& context,
                                      MessageMatchInfoMap& match_info,
                                      unsigned long flags) const
        throw(El::Exception);

      virtual bool satisfy(const Message::StoredMessage& msg,
                           Context& context) const
        throw(El::Exception);
      
    public:
      Time time;

      static El::Stat::TimeMeter evaluate_meter;
      static El::Stat::TimeMeter evaluate_simple_meter;
      
    private:
      void operator=(const Visited&);
      Visited(const Visited&);
    };

    class PubDate : public virtual Filter
    {
    public:
      PubDate() throw() {}
      virtual ~PubDate() throw() {}
      
      virtual Type type() const throw();

      virtual void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      virtual void read(El::BinaryInStream& bstr) throw(El::Exception);
      
      virtual void print(std::ostream& ostr) const throw(El::Exception);

      virtual void dump(std::wostream& ostr,
                        std::wstring& ident) const throw(El::Exception);      

      virtual Result* evaluate(Context& context,
                               MessageMatchInfoMap& match_info,
                               unsigned long flags) const
        throw(El::Exception);

      virtual Result* evaluate_simple(Context& context,
                                      MessageMatchInfoMap& match_info,
                                      unsigned long flags) const
        throw(El::Exception);

      virtual bool satisfy(const Message::StoredMessage& msg,
                           Context& context) const
        throw(El::Exception);
      
    public:
      Time time;

      static El::Stat::TimeMeter evaluate_meter;
      static El::Stat::TimeMeter evaluate_simple_meter;      
      
    private:
      void operator=(const PubDate&);
      PubDate(const PubDate&);
    };

    class With : public virtual Filter
    {
    public:
      With() throw() {}
      virtual ~With() throw() {}
      
      virtual Type type() const throw();

      virtual void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      virtual void read(El::BinaryInStream& bstr) throw(El::Exception);
      
      virtual void print(std::ostream& ostr) const throw(El::Exception);

      virtual void dump(std::wostream& ostr,
                        std::wstring& ident) const throw(El::Exception);      

      virtual Result* evaluate(Context& context,
                               MessageMatchInfoMap& match_info,
                               unsigned long flags) const
        throw(El::Exception);

      virtual Result* evaluate_simple(Context& context,
                                      MessageMatchInfoMap& match_info,
                                      unsigned long flags) const
        throw(El::Exception);

      virtual bool satisfy(const Message::StoredMessage& msg,
                           Context& context) const
        throw(El::Exception);
      
    public:
      
      enum Feature
      {
        FT_IMAGE
      };

      typedef std::vector<Feature> FeatureArray;
      
      FeatureArray features;

      static El::Stat::TimeMeter evaluate_meter;
      static El::Stat::TimeMeter evaluate_simple_meter;      
      
    private:
      void operator=(const With&);
      With(const With&);
    };

    class Domain : public virtual Filter
    {
    public:
      Domain() throw() {}
      virtual ~Domain() throw() {}
      
      virtual Type type() const throw();

      virtual void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      virtual void read(El::BinaryInStream& bstr) throw(El::Exception);
      
      virtual void print(std::ostream& ostr) const throw(El::Exception);

      virtual void dump(std::wostream& ostr,
                        std::wstring& ident) const throw(El::Exception);      

      virtual Result* evaluate(Context& context,
                               MessageMatchInfoMap& match_info,
                               unsigned long flags) const
        throw(El::Exception);

      virtual Result* evaluate_simple(Context& context,
                                      MessageMatchInfoMap& match_info,
                                      unsigned long flags) const
        throw(El::Exception);

      virtual bool satisfy(const Message::StoredMessage& msg,
                           Context& context) const
        throw(El::Exception);
      
    public:
      
      struct DomainRec
      {
        std::string name;
        unsigned char len;

        DomainRec() : len(0) {}

        void write(El::BinaryOutStream& ostr) const
          throw(Exception, El::Exception);
        
        void read(El::BinaryInStream& istr) throw(Exception, El::Exception);
      };

      typedef std::vector<DomainRec> DomainList;
      DomainList domains;
      
      static El::Stat::TimeMeter evaluate_meter;
      static El::Stat::TimeMeter evaluate_simple_meter;      
      
    private:
      void operator=(const Domain&);
      Domain(const Domain&);
    };

    template<const Condition::Type condition_type, typename VALUE_TYPE>
    class FilterValue : public virtual Filter
    {
    public:

      typedef VALUE_TYPE ValueType;

      FilterValue() throw(El::Exception) {}
      virtual ~FilterValue() throw() {}
      
      virtual Type type() const throw();

      virtual void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      virtual void read(El::BinaryInStream& bstr) throw(El::Exception);

      virtual const char* operation() const throw() = 0;
      virtual const char* negation() const throw() { return "NOT"; }
      
      virtual void print(std::ostream& ostr) const throw(El::Exception);

      virtual void dump(std::wostream& ostr,
                        std::wstring& ident) const throw(El::Exception);
      
      virtual void print_value(std::ostream& ostr) const throw(El::Exception);
      
      virtual void dump_value(std::wostream& ostr,
                              std::wstring& ident) const throw(El::Exception);
      
      virtual Result* evaluate(Context& context,
                               MessageMatchInfoMap& match_info,
                               unsigned long flags) const
        throw(El::Exception);

      virtual Result* evaluate_simple(Context& context,
                                      MessageMatchInfoMap& match_info,
                                      unsigned long flags) const
        throw(El::Exception);

    public:
      ValueType value;      

      static El::Stat::TimeMeter evaluate_meter;
      static El::Stat::TimeMeter evaluate_simple_meter;      
      
    private:
      void operator=(const FilterValue&);
      FilterValue(const FilterValue&);
    };    

    typedef FilterValue<Condition::TP_CAPACITY, uint32_t> CapacityBase;
    
    class Capacity : public CapacityBase
    {
    public:
      Capacity() throw() { value = 0; }

      virtual const char* operation() const throw() { return "CAPACITY"; }
      
      virtual bool satisfy(const Message::StoredMessage& msg,
                           Context& context) const
        throw(El::Exception);
    };

    typedef FilterValue<Condition::TP_IMPRESSIONS, uint64_t> ImpressionsBase;
    
    class Impressions : public ImpressionsBase
    {
    public:
      Impressions() throw() { value = 0; }

      virtual const char* operation() const throw() { return "IMPRESSIONS"; }
      
      virtual bool satisfy(const Message::StoredMessage& msg,
                           Context& context) const
        throw(El::Exception);
    };

    typedef FilterValue<Condition::TP_FEED_IMPRESSIONS, uint64_t>
    FeedImpressionsBase;
    
    class FeedImpressions : public FeedImpressionsBase
    {
    public:
      FeedImpressions() throw() { value = 0; }

      virtual const char* operation() const throw() { return "F-IMPRESSIONS"; }
      
      virtual bool satisfy(const Message::StoredMessage& msg,
                           Context& context) const
        throw(El::Exception);
    };

    typedef FilterValue<Condition::TP_CLICKS, uint64_t> ClicksBase;
    
    class Clicks : public ClicksBase
    {
    public:
      Clicks() throw() { value = 0; }

      virtual const char* operation() const throw() { return "CLICKS"; }
      
      virtual bool satisfy(const Message::StoredMessage& msg,
                           Context& context) const
        throw(El::Exception);
    };

    typedef FilterValue<Condition::TP_FEED_CLICKS, uint64_t>
    FeedClicksBase;
    
    class FeedClicks : public FeedClicksBase
    {
    public:
      FeedClicks() throw() { value = 0; }

      virtual const char* operation() const throw() { return "F-CLICKS"; }
      
      virtual bool satisfy(const Message::StoredMessage& msg,
                           Context& context) const
        throw(El::Exception);
    };    

    typedef FilterValue<Condition::TP_CTR, float> CTR_Base;
    
    class CTR : public CTR_Base
    {
    public:
      CTR() throw() { value = 0; }

      virtual const char* operation() const throw() { return "CTR"; }
      
      virtual bool satisfy(const Message::StoredMessage& msg,
                           Context& context) const
        throw(El::Exception);
    };

    typedef FilterValue<Condition::TP_FEED_CTR, float> FeedCTR_Base;
    
    class FeedCTR : public FeedCTR_Base
    {
    public:
      FeedCTR() throw() { value = 0; }

      virtual const char* operation() const throw() { return "F-CTR"; }
      
      virtual bool satisfy(const Message::StoredMessage& msg,
                           Context& context) const
        throw(El::Exception);
    };

    typedef FilterValue<Condition::TP_RCTR, float> RCTR_Base;
    
    class RCTR : public RCTR_Base
    {
    public:
      RCTR() throw();
      
      virtual const char* operation() const throw() { return "RCTR"; }
      
      virtual bool satisfy(const Message::StoredMessage& msg,
                           Context& context) const
        throw(El::Exception);

      virtual void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      virtual void read(El::BinaryInStream& bstr) throw(El::Exception);

      virtual void print_value(std::ostream& ostr) const throw(El::Exception);
      
      virtual void dump_value(std::wostream& ostr,
                              std::wstring& ident) const throw(El::Exception);
      
    public:
      uint8_t ril_is_default;
      uint32_t respected_impression_level;
    };

    typedef FilterValue<Condition::TP_FEED_RCTR, float> FeedRCTR_Base;
    
    class FeedRCTR : public FeedRCTR_Base
    {
    public:
      FeedRCTR() throw();
      
      virtual const char* operation() const throw() { return "F-RCTR"; }
      
      virtual bool satisfy(const Message::StoredMessage& msg,
                           Context& context) const
        throw(El::Exception);

      virtual void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      virtual void read(El::BinaryInStream& bstr) throw(El::Exception);

      virtual void print_value(std::ostream& ostr) const throw(El::Exception);
      
      virtual void dump_value(std::wostream& ostr,
                              std::wstring& ident) const throw(El::Exception);
      
    public:
      uint8_t ril_is_default;
      uint32_t respected_impression_level;
    };

    class Signature : public virtual Filter
    {
    public:
      Signature() throw() {}
      virtual ~Signature() throw() {}
      
      virtual Type type() const throw();

      virtual void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      virtual void read(El::BinaryInStream& bstr) throw(El::Exception);
      
      virtual void print(std::ostream& ostr) const throw(El::Exception);

      virtual void dump(std::wostream& ostr,
                        std::wstring& ident) const throw(El::Exception);      

      virtual Result* evaluate(Context& context,
                               MessageMatchInfoMap& match_info,
                               unsigned long flags) const
        throw(El::Exception);

      virtual Result* evaluate_simple(Context& context,
                                      MessageMatchInfoMap& match_info,
                                      unsigned long flags) const
        throw(El::Exception);

      virtual bool satisfy(const Message::StoredMessage& msg,
                           Context& context) const
        throw(El::Exception);
      
    public:
      
      typedef El::LightArray<uint64_t, uint32_t> SignatureArray;
      SignatureArray values;
      
      static El::Stat::TimeMeter evaluate_meter;
      static El::Stat::TimeMeter evaluate_simple_meter;      
      
    private:
      void operator=(const Signature&);
      Signature(const Signature&);
    };
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

inline
El::BinaryOutStream&
operator<<(El::BinaryOutStream& ostr,
           const NewsGate::Search::Condition_var& condition)
  throw(El::BinaryOutStream::Exception, El::Exception)
{
  ostr << (uint8_t)condition->type() << *condition;
  return ostr;
}

inline
El::BinaryInStream&
operator>>(El::BinaryInStream& istr,
           NewsGate::Search::Condition_var& condition)
  throw(El::BinaryInStream::Exception, El::Exception)
{
  uint8_t type = 0;
  istr >> type;
      
  condition = NewsGate::Search::Condition::create(
    (NewsGate::Search::Condition::Type)type);
  
  istr >> *condition;
  return istr;
}

inline
El::BinaryOutStream&
operator<<(El::BinaryOutStream& ostr, NewsGate::Search::With::Feature feature)
  throw(El::BinaryOutStream::Exception, El::Exception)
{
  ostr << (uint32_t)feature;
  return ostr;
}

inline
El::BinaryInStream&
operator>>(El::BinaryInStream& istr, NewsGate::Search::With::Feature& feature)
  throw(El::BinaryInStream::Exception, El::Exception)
{
  uint32_t ft = 0;
  istr >> ft;
  
  feature = (NewsGate::Search::With::Feature)ft;
  return istr;
}

namespace NewsGate
{
  namespace Search
  {
    //
    // Condition::Context class
    //
    inline
    Condition::Context::Context(
      const Message::SearcheableMessageMap& messages_val) throw()
        : time(ACE_OS::gettimeofday().sec()),
          messages(messages_val)
    {
    }
    
    //
    // Condition::PositionSet class
    //
    inline
    Condition::PositionSet::PositionSet() throw(El::Exception)
    {
      set_empty_key(Message::WORD_POSITION_MAX);
      set_deleted_key(Message::WORD_POSITION_MAX - 1);
    }
    
    inline
    Condition::PositionSet::PositionSet(unsigned long size)
      throw(El::Exception)
        : google::dense_hash_set<Message::WordPosition,
                                 El::Hash::Numeric<
                                   Message::WordPosition> >(size)
    {
      set_empty_key(Message::WORD_POSITION_MAX);
      set_deleted_key(Message::WORD_POSITION_MAX - 1);
    }

    //
    // Condition::Time class
    //
    inline
    Condition::Time::Time(uint64_t offset_val, OffsetType type_val) throw()
        : offset(offset_val),
          type(type_val)
    {
    }
    
    inline
    void
    Condition::Time::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr << offset << (uint32_t)type;
    }
    
    inline
    void
    Condition::Time::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      uint32_t tp = 0;
      bstr >> offset >> tp;
      type = (OffsetType)tp;
    }
    
    inline
    uint64_t
    Condition::Time::value(const Context& context) const throw()
    {
      return type == OT_SINCE_EPOCH ? offset : (context.time - offset);
    }

    //
    // Condition::MessageMatchInfo class
    //
    inline
    Condition::MessageMatchInfo::MessageMatchInfo() throw(El::Exception)
    {
    }
    
    inline
    Condition::MessageMatchInfo::MessageMatchInfo(const MessageMatchInfo& mmi)
      throw(El::Exception)
    {
      *this = mmi;
    }

    inline
    Condition::MessageMatchInfo&
    Condition::MessageMatchInfo::operator=(const MessageMatchInfo& mmi)
      throw(El::Exception)
    {
      if(mmi.positions.get() && mmi.positions->size())
      {
        if(positions.get() == 0)
        {
          positions.reset(new PositionSet(mmi.positions->size()));
        }
        
        *positions = *mmi.positions;
      }
      else
      {
        positions.reset(0);
      }

      if(mmi.core_words.get() && mmi.core_words->size())
      {
        if(core_words.get() == 0)
        {
          core_words.reset(new CoreWordsSet(mmi.core_words->size()));
        }
        
        *core_words = *mmi.core_words;
      }
      else
      {
        core_words.reset(0);
      }

      return *this;
    }

    inline
    bool
    Condition::MessageMatchInfo::empty() throw(El::Exception)
    {
      return (core_words.get() == 0 || core_words->empty()) && 
        (positions.get() == 0 || positions->empty());
    }
    
    inline
    void
    Condition::MessageMatchInfo::insert(const MessageMatchInfo& mmi)
      throw(El::Exception)
    {
      if(mmi.core_words.get() && mmi.core_words->size())
      {
        if(core_words.get() == 0)
        {
          core_words.reset(new CoreWordsSet(mmi.core_words->size()));
        }
        
        core_words->insert(mmi.core_words->begin(),
                           mmi.core_words->end());
      }

      if(mmi.positions.get() && mmi.positions->size())
      {
        if(positions.get() == 0)
        {
          positions.reset(new PositionSet(mmi.positions->size()));
        }
          
        positions->insert(mmi.positions->begin(), mmi.positions->end());
      }
    }

    inline
    void
    Condition::MessageMatchInfo::steal(MessageMatchInfo& mmi)
      throw(El::Exception)
    {
      if(mmi.core_words.get() && mmi.core_words->size())
      {
        if(core_words.get() == 0)
        {
          core_words.reset(mmi.core_words.release());
        }
        else
        {
          core_words->insert(mmi.core_words->begin(),
                             mmi.core_words->end());
        }
      }

      if(mmi.positions.get() && mmi.positions->size())
      {
        if(positions.get() == 0)
        {
          positions.reset(mmi.positions.release());
        }
        else
        {
          positions->insert(mmi.positions->begin(), mmi.positions->end());
        }
      }
    }

    //
    // Condition::MessageMatchInfo class
    //
    inline
    void
    Condition::MessageMatchInfoMap::unite(const MessageMatchInfoMap& src)
      throw(El::Exception)
    {
      for(MessageMatchInfoMap::const_iterator i(src.begin()), e(src.end());
          i != e; ++i)
      {
        (*this)[i->first].insert(i->second);
      }
    }

    //
    // Condition::CoreWordsSet class
    //
    inline
    Condition::CoreWordsSet::CoreWordsSet() throw(El::Exception)
    {
      set_empty_key(0);
      set_deleted_key(INT32_MAX);
    }
    
    inline
    Condition::CoreWordsSet::CoreWordsSet(unsigned long size)
      throw(El::Exception)
        : google::dense_hash_set<uint32_t,
                                 El::Hash::Numeric<uint32_t> >(size)
    {
      set_empty_key(0);
      set_deleted_key(INT32_MAX);
    }

    //
    // Condition class
    //

    inline
    Condition::Condition() throw()
    {
    }

    inline
    const Condition::Result*
    Condition::const_evaluate(Context& context,
                             unsigned long flags) const
      throw(El::Exception)
    {
      return 0;
    }

    inline
    Condition*
    Condition::filter(Filter_var& condition) throw(El::Exception)
    {
      return El::RefCount::add_ref(this);      
    }

    inline
    Condition*
    Condition::exclude(Condition_var& condition) throw(El::Exception)
    {
      return El::RefCount::add_ref(this);      
    }    

    inline
    void
    Condition::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
    }
      
    inline
    void
    Condition::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      optimization_info_.reset(0);
    }

    inline
    Condition*
    Condition::optimize() throw(El::Exception)
    {
      return El::RefCount::add_ref(this);
    }

    inline
    void
    Condition::relax(size_t level) throw(El::Exception)
    {
      ConditionArray subs = subconditions();

      for(ConditionArray::iterator i(subs.begin()), e(subs.end()); i != e; ++i)
      {
        i->in()->relax(level);
      }
    }

    inline
    Condition*
    Condition::intersect(Condition_var& condition)
      throw(El::Exception)
    {
      return El::RefCount::add_ref(this);
    }

    inline
    uint64_t
    Condition::cleanup_opt_info() throw(El::Exception)
    {
      uint64_t condition_flags = ((uint64_t)1) << type();
      optimization_info_.reset(0);

      ConditionArray sub_cond = subconditions();
      
      for(ConditionArray::iterator i(sub_cond.begin()), e(sub_cond.end());
          i != e; ++i)
      {
        condition_flags |= (*i)->cleanup_opt_info();
      }

      return condition_flags;
    }
    
    inline
    Condition*
    Condition::create(Type type) throw(El::Exception)
    {
      switch(type)
      {
      case TP_ALL: return new AllWords();        
      case TP_ANY: return new AnyWords();
      case TP_SITE: return new Site();
      case TP_URL: return new Url();
      case TP_MSG: return new Msg();
      case TP_EVENT: return new Event();
      case TP_EVERY: return new Every();
      case TP_NONE: return new None();
      case TP_OR: return new Or();
      case TP_AND: return new And();
      case TP_EXCEPT: return new Except();
      case TP_PUB_DATE: return new PubDate();
      case TP_FETCHED: return new Fetched();
      case TP_VISITED: return new Visited();
      case TP_LANG: return new Lang();
      case TP_COUNTRY: return new Country();
      case TP_SPACE: return new Space();
      case TP_DOMAIN: return new Domain();
      case TP_CAPACITY: return new Capacity();
      case TP_IMPRESSIONS: return new Impressions();
      case TP_FEED_IMPRESSIONS: return new FeedImpressions();
      case TP_CLICKS: return new Clicks();
      case TP_FEED_CLICKS: return new FeedClicks();
      case TP_CTR: return new CTR();
      case TP_FEED_CTR: return new FeedCTR();
      case TP_RCTR: return new RCTR();
      case TP_FEED_RCTR: return new FeedRCTR();
      case TP_SIGNATURE: return new Signature();
      case TP_CATEGORY: return new Category();
      case TP_WITH: return new With();
      }

      std::ostringstream ostr;
      ostr << "NewsGate::Search::Condition::create: unexpected type code "
           << type;
      
      throw Exception(ostr.str());
    }

    inline
    bool
    Condition::message_in_list(Message::Number number,
                               ResultList::const_iterator& begin,
                               ResultList::const_iterator& end)
      throw(El::Exception)
    {   
      for(ResultList::const_iterator it = begin; it != end; ++it)
      {
        const Result& res = **it;
        
        if(res.find(number) != res.end())
        {
          return true;
        }
      }

      return false;
    }
    
    inline
    bool
    Condition::message_not_in_list(Message::Number number,
                                   ResultList::const_iterator& begin,
                                   ResultList::const_iterator& end)
      throw(El::Exception)
    {   
      for(ResultList::const_iterator it = begin; it != end; ++it)
      {
        const Result& res = **it;
        
        if(res.find(number) == res.end())
        {
          return true;
        }
      }

      return false;
    }

    inline
    ConditionArray
    Condition::subconditions() const throw(El::Exception)
    {
      return ConditionArray();
    }
    
    //
    // Word class
    //
    inline
    Word::Word() throw(El::Exception) : flags(0)
    {
    }

    inline
    bool
    Word::same_text(const Word& w) const throw()
    {
      return text == w.text && exact() == w.exact();
    }

    inline
    unsigned char
    Word::group() const throw()
    {
      return flags & WF_GROUP;
    }
    
    inline
    void
    Word::group(unsigned char val) throw()
    {
      flags = (flags & ~WF_GROUP) | (val & WF_GROUP);
    }

    inline
    bool
    Word::use_norm_forms() const throw()
    {
      return (flags & WF_HAS_NF) != 0 && (flags & WF_EXACT) == 0;
    }
    
    inline
    bool
    Word::exact() const throw()
    {
      return flags & WF_EXACT;
    }

    inline
    void
    Word::exact(bool val) throw()
    {
      if(val)
      {
        flags |= WF_EXACT;
      }
      else
      {
        flags &= ~WF_EXACT;
      }
    }

    inline
    void
    Word::set_norm_forms_flag() throw()
    {
      if(norm_forms.empty())
      {
        flags &= ~WF_HAS_NF;
      }
      else
      {
        flags |= WF_HAS_NF;
      }
    }

    inline
    void
    Word::write(El::BinaryOutStream& ostr) const
      throw(Exception, El::Exception)
    {
      ostr << flags << text;
      ostr.write_array(norm_forms);
    }

    inline
    void
    Word::read(El::BinaryInStream& istr) throw(Exception, El::Exception)
    {
      istr >> flags >> text;
      istr.read_array(norm_forms);
    }
    
    inline
    size_t
    Word::common_norm_forms(const Word& w) const throw()
    {
      size_t count = 0;
      
      WordIdArray::const_iterator j(norm_forms.begin());
      WordIdArray::const_iterator je(norm_forms.end());
        
      for(WordIdArray::const_iterator i(w.norm_forms.begin()),
            e(w.norm_forms.end()); i != e; ++i)
      {
        El::Dictionary::Morphology::WordId id = *i;
          
        for(; j != je && *j < id; ++j);

        if(j == je)
        {
          break;
        }

        if(*j == id)
        {
          ++count;
        }
      }

      return count;
    }
    
    inline
    Word::Relation
    Word::relation(const Word& w) const throw()
    {
      // Returns: WR_DIFERENT, WR_SAME, WR_LESS_SPECIFIC OR WR_MORE_SPECIFIC

      bool diff_norm_forms_use = use_norm_forms() != w.use_norm_forms();

      if(text == w.text)
      {
        return diff_norm_forms_use ?
          (use_norm_forms() ? WR_LESS_SPECIFIC : WR_MORE_SPECIFIC) : WR_SAME;
      }

      // Here word texts are different

      if(!use_norm_forms() && !w.use_norm_forms())
      {
        return WR_DIFERENT;
      }

      // Here word texts are different and at least one of them use norm forms

      size_t common_forms = common_norm_forms(w);

      if(!common_forms)
      {
        return WR_DIFERENT;
      }

      // Here word texts are different and have some common norm forms
      
      if(diff_norm_forms_use)
      {
        return use_norm_forms() ? WR_LESS_SPECIFIC : WR_MORE_SPECIFIC;
      }

      // Here word texts are different and both them use norm forms

      size_t nf_count = norm_forms.size();
      size_t w_nf_count = w.norm_forms.size();
      
      if(common_forms != nf_count && common_forms != w_nf_count)
      {
        return WR_DIFERENT;
      }

      // Here one word norm forms are subset of another word norm forms

      return common_forms == nf_count ?
        (common_forms == w_nf_count ? WR_SAME : WR_MORE_SPECIFIC) :
        WR_LESS_SPECIFIC;
    }

    //
    // Words class
    //
    inline
    Words::~Words() throw()
    {
    }

    inline
    void
    Words::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      Condition::write(bstr);      
      bstr.write_array(words);
      bstr << word_flags;
    }
    
    inline
    void
    Words::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      Condition::read(bstr);
      bstr.read_array(words);
      bstr >> word_flags;
    }

    inline
    bool
    Words::satisfy(Message::WordPosition pos,
                   const Message::StoredMessage& msg) const throw()
    {
      return (pos >= msg.description_pos && pos < msg.img_alt_pos &&
              (word_flags & WF_DESC) != 0) ||
        (pos < msg.description_pos && (word_flags & WF_TITLE) != 0) ||
        (pos >= msg.img_alt_pos && pos < msg.keywords_pos &&
         (word_flags & WF_IMG_ALT) != 0) ||
        (pos >= msg.keywords_pos && (word_flags & WF_KEYWORDS) != 0);
    }
  
    inline
    bool
    Words::satisfy(const Message::WordPositions& positions,
                   const Message::StoredMessage& msg) const throw()
    {
      if((word_flags & WF_CORE) != 0 &&
         (positions.flags & Message::WordPositions::FL_CORE_WORD) == 0)
      {
        return false;
      }

      bool check_title = (word_flags & WF_TITLE) != 0;
      bool check_desc = (word_flags & WF_DESC) != 0;
      bool check_img_alt = (word_flags & WF_IMG_ALT) != 0;
      bool check_keywords = (word_flags & WF_KEYWORDS) != 0;

      Message::WordPosition description_pos = msg.description_pos;
      Message::WordPosition img_alt_pos = msg.img_alt_pos;
      Message::WordPosition keywords_pos = msg.keywords_pos;
      
      const Message::WordPositionArray& word_positions = msg.positions;
      size_t positions_count = positions.position_count();

      for(size_t i = 0; i < positions_count; i++)
      {
        Message::WordPosition pos = positions.position(word_positions, i);
        
        if((pos >= description_pos && pos < img_alt_pos && check_desc) ||
           (pos < description_pos && check_title) ||
           (pos >= img_alt_pos && pos < keywords_pos && check_img_alt) ||
           (pos >= keywords_pos && check_keywords))
        {
          return true;
        }
      }
      
      return false;
    }
    
    inline
    void
    Words::add_match_info(MessageMatchInfoMap& match_info,
                          unsigned long flags,
                          Message::Number number,
                          const Message::StoredMessage& msg,
                          const Word& word)
      throw(El::Exception)
    {
      if(flags)
      {
        add_match_info(match_info[number], flags, msg, word);
      }
    }
    
    inline
    void
    Words::add_match_info(MessageMatchInfo& match_info,
                          unsigned long flags,
                          const Message::StoredMessage& msg,
                          const Word& word)
      throw(El::Exception)
    {
      if(!flags)
      {
        return;
      }
      
      const WordIdArray& norm_forms = word.norm_forms;
      const Message::WordPositionArray& word_positions = msg.positions;

      bool fill_core_words = (flags & EF_FILL_CORE_WORDS) != 0;
      bool fill_positions = (flags & EF_FILL_POSITIONS) != 0;
      
      if(word.use_norm_forms())
      {
        const Message::NormFormPosition& norm_form_positions =
          msg.norm_form_positions;

        for(size_t i = 0; i < norm_forms.size(); i++)
        {
          El::Dictionary::Morphology::WordId id = norm_forms[i];
                  
          const Message::NormFormPosition::KeyValue* nwpos =
            norm_form_positions.find(id);

          if(nwpos)
          {
            const Message::WordPositions& wpos = nwpos->second;

            if((wpos.flags & Message::WordPositions::FL_CORE_WORD) != 0)
            {
              if(fill_core_words)
              {
                if(match_info.core_words.get() == 0)
                {
                  match_info.core_words.reset(new CoreWordsSet());
                }
              
                match_info.core_words->insert(id);
              }
            }
            
            if(fill_positions)
            {
              Message::WordPositionNumber positions_count =
                wpos.position_count();
            
              if(match_info.positions.get() == 0)
              {
                match_info.positions.reset(new PositionSet(positions_count));
              }
              
              for(unsigned long i = 0; i < positions_count; i++)
              {
                match_info.positions->insert(wpos.position(word_positions, i));
              }
            }
            
          }
        }
      }
      else
      {
        const Message::WordPositions& wpos =
          msg.word_positions.find(word.text.c_str(),
                                  word.text.hash())->second;

        if((wpos.flags & Message::WordPositions::FL_CORE_WORD) != 0 &&
           fill_core_words)
        {
          if(match_info.core_words.get() == 0)
          {
            match_info.core_words.reset(new CoreWordsSet());
          }

          if(norm_forms.empty())
          {
            match_info.core_words->insert(
              El::Dictionary::Morphology::pseudo_id(word.text.c_str()));
          }
          else
          {
            const Message::NormFormPosition& norm_form_positions =
              msg.norm_form_positions;

            for(size_t i = 0; i < norm_forms.size(); i++)
            {
              El::Dictionary::Morphology::WordId id = norm_forms[i];
                  
              const Message::NormFormPosition::KeyValue* nwpos =
                norm_form_positions.find(id);

              if(nwpos)
              {
                const Message::WordPositions& wpos = nwpos->second;

                if((wpos.flags & Message::WordPositions::FL_CORE_WORD) != 0)
                {
                  match_info.core_words->insert(id);
                }
              }
            }
          }
        }
        
        if(fill_positions)
        {
          unsigned long wpos_count = wpos.position_count();

          if(match_info.positions.get() == 0)
          {
            match_info.positions.reset(new PositionSet(wpos_count));
          }

          for(size_t i = 0; i < wpos_count; i++)
          {
            match_info.positions->insert(wpos.position(word_positions, i));
          }
        }
      }
    }
      
    inline
    Word::Relation
    Words::relation_assymetrical(WordList::const_iterator si, // short word
                                 const WordList::const_iterator& se,
                                 WordList::const_iterator li, // long word
                                 size_t iterations,
                                 bool use_norm_forms) throw()
    {      
      WordList::const_iterator lb(li);
      
      if(use_norm_forms)
      {
      // Returns: WR_DIFERENT, WR_SAME, WR_LESS_SPECIFIC or WR_MORE_SPECIFIC

        for(; iterations--; ++li)
        {
          WordList::const_iterator lii = li;
          WordList::const_iterator sii = si;

          Word::Relation rel = Word::WR_DIFERENT;
        
          for(; sii != se; ++lii, ++sii)
          {
            Word::Relation r = sii->relation(*lii);
            
            if(r == Word::WR_DIFERENT ||
               (r == Word::WR_LESS_SPECIFIC && rel == Word::WR_MORE_SPECIFIC) ||
               (r == Word::WR_MORE_SPECIFIC && rel == Word::WR_LESS_SPECIFIC))
            {
              break;
            }

            if(rel == Word::WR_DIFERENT || rel == Word::WR_SAME)
            {
              rel = r;
            }
          }

          if(rel != Word::WR_DIFERENT)
          {
            if(li == lb && !iterations)
            {
              return sii == se ? rel : Word::WR_DIFERENT;
            }
            
            if(rel != Word::WR_MORE_SPECIFIC)
            {
              return Word::WR_LESS_SPECIFIC;
            }
          }
        }        
      }
      else
      {      
      // Returns: WR_DIFERENT, WR_SAME or WR_LESS_SPECIFIC
        
        for(; iterations--; ++li)
        {
          WordList::const_iterator lii = li;
          WordList::const_iterator sii = si;
        
          for(; sii != se && sii->same_text(*lii); ++lii, ++sii);
        
          if(sii == se)
          {
            return li == lb && !iterations ?
              Word::WR_SAME : Word::WR_LESS_SPECIFIC;
          }
        }
      }
      
      return Word::WR_DIFERENT;
    }

    inline
    Word::Relation
    Words::relation(WordList::const_iterator wi1,
                    const WordList::const_iterator& we1,
                    size_t len1,
                    WordList::const_iterator wi2,
                    const WordList::const_iterator& we2,
                    size_t len2,
                    bool use_norm_forms) throw()
    {
      Word::Relation rel;
      
      if(len1 <= len2)
      {
        rel = relation_assymetrical(wi1,
                                    we1,
                                    wi2,
                                    len2 - len1 + 1,
                                    use_norm_forms);
      }
      else
      {
        rel = relation_assymetrical(wi2,
                                    we2,
                                    wi1,
                                    len1 - len2 + 1,
                                    use_norm_forms);
        
        switch(rel)
        {
        case Word::WR_LESS_SPECIFIC:
          {
            rel = Word::WR_MORE_SPECIFIC;
            break;
          }
        case Word::WR_MORE_SPECIFIC:
          {
            rel = Word::WR_LESS_SPECIFIC;
            break;
          }
        default: break;
        }
      }

      return rel;
    }
    
    inline
    WordList::const_iterator
    Words::next_phrase(WordList::const_iterator i,
                       const WordList::const_iterator& e,
                       size_t& len)
      const throw()
    {
      len = 1;
      unsigned char group = i++->group();
      
      if(group)
      {
        for(; i != e && i->group() == group; ++i, ++len);
      }

      return i;
    }
    
    inline
    WordList::iterator
    Words::next_phrase(WordList::iterator i,
                       const WordList::iterator& e,
                       size_t& len)
      const throw()
    {
      len = 1;
      unsigned char group = i++->group();
      
      if(group)
      {
        for(; i != e && i->group() == group; ++i, ++len);
      }

      return i;
    }    
    
    //
    // Words::SeqSet struct
    //
    inline
    Words::SeqSet::SeqSet() throw(El::Exception)
    {
      set_empty_key(Message::WORD_POSITION_MAX);
      set_deleted_key(Message::WORD_POSITION_MAX - 1);
    }
    
    //
    // AllWords class
    //
    
    inline
    void
    AllWords::dump(std::wostream& ostr, std::wstring& ident) const
      throw(El::Exception)
    {
      ostr << ident << L"ALL ";
      Words::dump(ostr, ident);
    }
    
    inline
    Condition::Type
    AllWords::type() const throw()
    {
      return TP_ALL;
    }

    inline
    Condition::Result*
    AllWords::evaluate(Context& context,
                       MessageMatchInfoMap& match_info,
                       unsigned long flags) const
      throw(El::Exception)
    {
      return evaluate(context, 0, match_info, flags);
    }

    inline
    Condition::Result*
    AllWords::evaluate_simple(Context& context,
                              MessageMatchInfoMap& match_info,
                              unsigned long flags)
      const throw(El::Exception)
    {
      return evaluate(context, match_info, flags);
    }

    inline
    bool
    AllWords::precheck_word_sequence(const Message::StoredMessage& msg,
                                     unsigned long word_group,
                                     unsigned long prev_word_group,
                                     unsigned long flags,
                                     WordList::const_iterator wit,
                                     WordList::const_iterator& group_wit,
                                     unsigned long& word_seq_in_group,
                                     PositionSeqSetPtr& group_positions,
                                     MessageMatchInfo& match_info) const
      throw(El::Exception)
    {
      if(word_group == prev_word_group)
      {
        return true;
      }

      if(prev_word_group != 0 && word_seq_in_group)
      {
        // Closing group

        bool fill_core_words = (flags & EF_FILL_CORE_WORDS) != 0;
        bool fill_positions = (flags & EF_FILL_POSITIONS) != 0;
        bool core = (word_flags & WF_CORE) != 0;
        
        if(fill_core_words || core)
        {
          bool has_core_words = false;
          
          for(WordList::const_iterator it = group_wit; it != wit; ++it)
          {
            const Word& word = *it;
            const WordIdArray& norm_forms = word.norm_forms;

            if(!word.use_norm_forms())
            {
              const Message::WordPositions& wpos =
                msg.word_positions.find(word.text.c_str(),
                                        word.text.hash())->second;
                
              if((wpos.flags & Message::WordPositions::FL_CORE_WORD) != 0)
              {
                has_core_words = true;
                  
                if(fill_core_words)
                {
                  if(match_info.core_words.get() == 0)
                  {
                    match_info.core_words.reset(new CoreWordsSet());
                  }

                  if(norm_forms.empty())
                  {
                    match_info.core_words->insert(
                      El::Dictionary::Morphology::pseudo_id(
                        word.text.c_str()));
                  }
                  else
                  {
                    const Message::NormFormPosition& norm_form_positions =
                      msg.norm_form_positions;

                    for(size_t i = 0; i < norm_forms.size(); i++)
                    {
                      El::Dictionary::Morphology::WordId id = norm_forms[i];
                  
                      const Message::NormFormPosition::KeyValue* nwpos =
                        norm_form_positions.find(id);

                      if(nwpos)
                      {
                        const Message::WordPositions& wpos = nwpos->second;

                        if((wpos.flags & Message::WordPositions::FL_CORE_WORD)
                           != 0)
                        {
                          match_info.core_words->insert(id);
                        }            
                      }
                    }
                  }
                }
              }
            }
            else
            {
              const Message::NormFormPosition& norm_form_positions =
                msg.norm_form_positions;

              for(size_t i = 0; i < norm_forms.size(); i++)
              {
                El::Dictionary::Morphology::WordId id = norm_forms[i];
                  
                const Message::NormFormPosition::KeyValue* nwpos =
                  norm_form_positions.find(id);

                if(nwpos)
                {
                  const Message::WordPositions& wpos = nwpos->second;

                  if((wpos.flags & Message::WordPositions::FL_CORE_WORD) !=
                     0)
                  {
                    has_core_words = true;
                    
                    if(fill_core_words)
                    {
                      if(match_info.core_words.get() == 0)
                      {
                        match_info.core_words.reset(new CoreWordsSet());
                      }
              
                      match_info.core_words->insert(id);
                    }
                  }            
                }
              }
            }
          }

          if(core && !has_core_words)
          {
            return false;
          }
        }
      
        if(group_positions.get() != 0)
        {
          PositionSeqSet::const_iterator b(group_positions->begin());
          PositionSeqSet::const_iterator e(group_positions->end());

          PositionSeqSet::const_iterator it = b;
          
          for(; it != e &&
                (it->second.find(word_seq_in_group) == it->second.end() ||
                 !satisfy(it->first, msg)); ++it);

          if(it == e)
          {
            match_info.core_words.reset(0);
            return false;
          }
          
          if(fill_positions)
          {
            for(it = b; it != e; ++it)
            {
              if(it->second.find(word_seq_in_group) != it->second.end())
              {
                if(match_info.positions.get() == 0)
                {
                  match_info.positions.reset(
                    new PositionSet(word_seq_in_group));
                }
                
                for(Message::WordPosition i = 0; i < word_seq_in_group; ++i)
                {
                  match_info.positions->insert(it->first - i);
                }
              }
            }
          }
        }
      }

      if(word_group)
      {
        //
        // Opening group
        //
        group_positions.reset(0);              
        word_seq_in_group = 1;
        group_wit = wit;
      }

      return true;
    }

    inline
    void
    AllWords::postcheck_word_sequence(unsigned long word_group,
                                      unsigned long prev_word_group,
                                      unsigned long& word_seq_in_group)
      throw(El::Exception)
    {
      if(word_group == prev_word_group && word_group)
      {
        word_seq_in_group++;
      }
    }

    inline
    bool
    AllWords::check_word_sequence(unsigned long word_group,
                                  unsigned long prev_word_group,
                                  unsigned long& word_seq_in_group,
                                  const Message::WordPositionArray& positions,
                                  const Message::WordPositions& word_positions,
                                  PositionSeqSetPtr& group_positions)
      throw(El::Exception)
    {
      unsigned long positions_count = word_positions.position_count();
        
      if(word_group != prev_word_group)
      {            
        if(word_group)
        {
          if(group_positions.get() == 0)
          {
            group_positions.reset(new PositionSeqSet());
          }
          
          for(size_t i = 0; i < positions_count; i++)
          {
            (*group_positions)[word_positions.position(positions, i)].
              insert(1);
          }
        }
      }
      else if(word_group)
      {
        //
        // Continuing group
        //

        bool seq_present = false;            
        unsigned long next_word_seq_in_group = word_seq_in_group + 1;
            
        for(size_t i = 0; i < positions_count; i++)
        {
          unsigned long pos = word_positions.position(positions, i);
          
          PositionSeqSet::iterator it;
              
          if(pos > 0 && (it = group_positions->find(pos - 1)) !=
             group_positions->end() &&
             it->second.find(word_seq_in_group) != it->second.end())
          {
            (*group_positions)[pos].insert(next_word_seq_in_group);
            seq_present = true;
          }
        }

        if(!seq_present)
        {
          return false;
        }
      }

      return true;
    }
    
    //
    // AnyWords class
    //

    inline
    AnyWords::AnyWords() throw(El::Exception) : match_threshold(1)
    {
    }

    inline
    void
    AnyWords::dump(std::wostream& ostr, std::wstring& ident) const
      throw(El::Exception)
    {
      ostr << ident << L"ANY ";

      assert(words.begin() != words.end());

      bool enforce_quoting = true;

      if(match_threshold > 1)
      {
        ostr << match_threshold << " ";
        enforce_quoting = false;
      }

      if(word_flags & WF_CORE)
      {
        ostr << L"CORE ";
//        enforce_quoting = false;
      }

      if(enforce_quoting)
      {
        const Word& word = *words.begin();
        unsigned long tmp;
        
        enforce_quoting = word.group() == 0 && !word.exact() &&
          El::String::Manip::numeric(word.text.c_str(), tmp);         
      }
      
      unsigned char group = 0;
      bool exact = false;

      for(WordList::const_iterator b(words.begin()), i(b), e(words.end());
          i != e; ++i)
      {
        const Word& word = *i;

        if(group && group != word.group())
        {
          ostr << (exact ? "\"" : "'");
        }

        if(i != b)
        {
          ostr << " ";
        }

        //
        // For optimization purposes for double quoted single word phrases
        // group is set to 0, so need to
        // check for word.group() == 0 && word.exact() cases specifically
        //

        if((word.group() && word.group() != group) ||
           (word.group() == 0 && word.exact()) || enforce_quoting)
        {
          ostr << ((enforce_quoting || word.exact()) ? "\"" : "'");
        }          

        std::wstring text;
        El::String::Manip::utf8_to_wchar(word.text.c_str(), text);

        ostr << text;
        
        group = ((word.group() == 0 && word.exact()) || enforce_quoting) ?
          3 : word.group();
        
        exact = word.exact() || enforce_quoting;
        enforce_quoting = false;
      }
      
      if(group)
      {
        ostr << (exact ? "\"" : "'");
      }

      ostr << std::endl;  
    } 
    
    inline
    Condition::Type
    AnyWords::type() const throw()
    {
      return TP_ANY;
    }

    inline
    void
    AnyWords::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      Words::write(bstr);
      bstr << match_threshold;
    }
    
    inline
    void
    AnyWords::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      Words::read(bstr);
      bstr >> match_threshold;
    }
    
    inline
    Condition::Result*
    AnyWords::evaluate_simple(Context& context,
                              MessageMatchInfoMap& match_info,
                              unsigned long flags)
      const throw(El::Exception)
    {
      return evaluate(context, match_info, flags);
    }

    inline
    Condition::Result*
    AnyWords::evaluate(Context& context,
                       MessageMatchInfoMap& match_info,
                       unsigned long flags) const
      throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_meter);

      return match_threshold > 1 ?
        evaluate_many(context, match_info, flags) :
        evaluate_one(context, match_info, flags);
    }

    inline
    const Message::StoredMessage*
    AnyWords::find_msg(Message::Number number,
                       const Message::StoredMessageMap& stored_message,
                       Context& context,
                       bool search_hidden,
                       ResultList::const_iterator& intersect_list_begin,
                       ResultList::const_iterator& intersect_list_end,
                       ResultList::const_iterator& skip_list_begin,
                       ResultList::const_iterator& skip_list_end,
                       MessageFilterList::const_iterator& filters_begin,
                       MessageFilterList::const_iterator& filters_end)
      throw(El::Exception)
    {
      if(message_not_in_list(number,
                             intersect_list_begin,
                             intersect_list_end) ||
         message_in_list(number, skip_list_begin, skip_list_end))
      {
        // Found in skip list
        return 0;
      }

      Message::StoredMessageMap::const_iterator mit  =
        stored_message.find(number);
          
      const Message::StoredMessage* msg = mit->second;

      if(msg->hidden() != search_hidden)
      {
        return 0;
      }
      
      MessageFilterList::const_iterator fit = filters_begin;
          
      for(; fit != filters_end && (*fit)->satisfy(*msg, context); ++fit);

      if(fit != filters_end)
      {
        // Filtered out
        return 0;
      }

      return msg;
    }

    //
    // Or class
    //
    
    inline
    Condition*
    Or::optimize() throw(El::Exception)
    {
      for(ConditionArray::iterator i(operands.begin()), e(operands.end());
          i != e; )
      {
        Condition_var& condition = *i;
        condition = condition->optimize();

        if(condition->type() == TP_NONE)
        {
          i = operands.erase(i);
          e = operands.end();
        }
        else
        {
          ++i;
        }
      }

      return check_trivial();
    }

    inline
    Condition*
    Or::filter(Filter_var& cond) throw(El::Exception)
    {
      bool null_cond = true;
        
      for(ConditionArray::iterator i(operands.begin()), e(operands.end());
          i != e; )
      {
        Condition_var& condition = *i;
        Filter_var cond_dup = cond; // Not change original var-ptr
        
        condition = condition->filter(cond_dup);

        if(cond_dup.in() != 0)
        {
          null_cond = false;
        }

        if(condition->type() == TP_NONE)
        {
          i = operands.erase(i);
          e = operands.end();
        }
        else
        {
          ++i;
        }        
      }

      if(null_cond)
      {
        cond = 0;
      }
      
      return check_trivial();
    }
    
    inline
    Condition*
    Or::intersect(Condition_var& cond) throw(El::Exception)
    {
      bool null_cond = true;
        
      for(ConditionArray::iterator i(operands.begin()), e(operands.end());
          i != e; )
      {
        Condition_var& condition = *i;
        Condition_var cond_dup = cond; // Not change original var-ptr
        
        condition = condition->intersect(cond_dup);

        if(cond_dup.in() != 0)
        {
          null_cond = false;
        }

        if(condition->type() == TP_NONE)
        {
          i = operands.erase(i);
          e = operands.end();
        }
        else
        {
          ++i;
        }        
      }
      
      if(null_cond)
      {
        cond = 0;
      }
      
      return check_trivial();
    }

    inline
    Condition*
    Or::exclude(Condition_var& cond) throw(El::Exception)
    {
      bool null_cond = true;
        
      for(ConditionArray::iterator i(operands.begin()), e(operands.end());
          i != e; )
      {
        Condition_var& condition = *i;
        Condition_var cond_dup = cond; // Not change original var-ptr
        
        condition = condition->exclude(cond_dup);

        if(cond_dup.in() != 0)
        {
          null_cond = false;
        }

        if(condition->type() == TP_NONE)
        {
          i = operands.erase(i);
          e = operands.end();
        }
        else
        {
          ++i;
        }        
      }
      
      if(null_cond)
      {
        cond = 0;
      }
      
      return check_trivial();
    }

    inline
    Condition*
    Or::check_trivial() throw(El::Exception)
    {
      if(operands.empty())
      {
        return new None();
      }

      if(operands.size() == 1)
      {
        Condition_var& condition = *operands.begin();
        return El::RefCount::add_ref(condition.in());
      }

      return El::RefCount::add_ref(this);
    }
    
    inline
    void
    Or::dump(std::wostream& ostr, std::wstring& ident) const
      throw(El::Exception)
    {
      ostr << ident << L"OR\n";

      ident += L"  ";

      for(ConditionArray::const_iterator it(operands.begin()),
            ie(operands.end()); it != ie; ++it)
      {
        Condition* condition = it->in();
        condition->dump(ostr, ident);
      }

      ident.resize(ident.length() - 2);
    }
    
    inline
    Condition::Type
    Or::type() const throw()
    {
      return TP_OR;
    }

    inline
    void
    Or::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      Condition::write(bstr);
      bstr.write_array(operands);
    }
    
    inline
    void
    Or::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      Condition::read(bstr);
      bstr.read_array(operands);
    }
      
    inline
    void
    Or::normalize(const El::Dictionary::Morphology::WordInfoManager&
                  word_info_manager) throw(El::Exception)
    {
      for(ConditionArray::const_iterator it(operands.begin()),
            ie(operands.end()); it != ie; ++it)
      {
        Condition* condition = it->in();
        condition->normalize(word_info_manager);
      }
    }
    
    inline
    ConditionArray
    Or::subconditions() const throw(El::Exception)
    {
      return operands;
    }
    
    //
    // And class
    //
    inline
    Condition*
    And::optimize() throw(El::Exception)
    {
      bool changed = false;

      do
      {
        changed = false;
        
        for(ConditionArray::iterator i(operands.begin()), e(operands.end());
            i != e; )
        {
          {
            Condition_var& condition = *i;
            condition = condition->optimize();

            if(condition->type() == TP_NONE)
            {
              return condition.retn();
            }
          }

          bool absorbed = false;
        
          for(ConditionArray::iterator j(operands.begin()); j != i; )
          {
            Condition_var& condition = *i;
            Condition_var& cond = *j;
          
            Condition* prev_cond = cond.in();
            Condition* prev_condition = condition.in();
          
            cond = cond->intersect(condition);

            if(cond->type() == TP_NONE)
            {
              return cond.retn();
            }
              
            if(condition.in() && condition->type() == TP_NONE)
            {
              return condition.retn();
            }

            changed |= cond.in() != prev_cond ||
              prev_condition != condition.in();
          
            if(condition.in() == 0)
            {
              absorbed = true;
              break;
            }

            prev_cond = cond.in();
            prev_condition = condition.in();

            condition = condition->intersect(cond);

            if(condition->type() == TP_NONE)
            {
              return condition.retn();
            }
            
            if(cond.in() && cond->type() == TP_NONE)
            {
              return cond.retn();
            }

            changed |= cond.in() != prev_cond ||
              prev_condition != condition.in();
          
            if(cond.in() == 0)
            {
              size_t offset = i - operands.begin();
            
              j = operands.erase(j);
              i = operands.begin() + offset - 1;
              e = operands.end();
            }
            else
            {
              ++j;
            }
          }

          if(absorbed)
          {
            i = operands.erase(i);
            e = operands.end();
          }
          else
          {
            ++i;
          }
        }

        if(operands.size() == 1)
        {
          Condition_var& condition = *operands.begin();
          return El::RefCount::add_ref(condition.in());
        }
      }
      while(changed);
      
      return El::RefCount::add_ref(this);
    }

    inline
    Condition*
    And::filter(Filter_var& cond) throw(El::Exception)
    {
      bool null_cond = true;
        
      for(ConditionArray::iterator i(operands.begin()), e(operands.end());
          i != e; ++i)
      {
        Condition_var& condition = *i;
        Filter_var cond_dup = cond; // Not change original var-ptr
        
        condition = condition->filter(cond_dup);

        if(condition->type() == TP_NONE)
        {
          return condition.retn();
        }

        if(cond_dup.in() != 0)
        {
          null_cond = false;
        }
      }
      
      if(null_cond)
      {
        cond = 0;
      }
      
      return El::RefCount::add_ref(this);
    }    
    
    inline
    Condition*
    And::intersect(Condition_var& cond) throw(El::Exception)
    {
      bool null_cond = true;
        
      for(ConditionArray::iterator i(operands.begin()), e(operands.end());
          i != e; ++i)
      {
        Condition_var& condition = *i;
        Condition_var cond_dup = cond; // Not change original var-ptr
        
        condition = condition->intersect(cond_dup);

        if(condition->type() == TP_NONE)
        {
          return condition.retn();
        }

        if(cond_dup.in() != 0)
        {
          null_cond = false;
        }        
      }
      
      if(null_cond)
      {
        cond = 0;
      }
      
      return El::RefCount::add_ref(this);
    }

    inline
    Condition*
    And::exclude(Condition_var& cond) throw(El::Exception)
    {
      bool null_cond = true;
        
      for(ConditionArray::iterator i(operands.begin()), e(operands.end());
          i != e; ++i)
      {
        Condition_var& condition = *i;
        Condition_var cond_dup = cond; // Not change original var-ptr
        
        condition = condition->exclude(cond_dup);

        if(condition->type() == TP_NONE)
        {
          return condition.retn();
        }
        
        if(cond_dup.in() != 0)
        {
          null_cond = false;
        }
      }
      
      if(null_cond)
      {
        cond = 0;
      }
      
      return El::RefCount::add_ref(this);
    }
    
    inline
    void
    And::dump(std::wostream& ostr, std::wstring& ident) const
      throw(El::Exception)
    {
      ostr << ident << L"AND\n";

      ident += L"  ";

      for(ConditionArray::const_iterator it(operands.begin()),
            ie(operands.end()); it != ie; ++it)
      {
        Condition* condition = it->in();
        condition->dump(ostr, ident);
      }

      ident.resize(ident.length() - 2);
    }
    
    inline
    Condition::Type
    And::type() const throw()
    {
      return TP_AND;
    }

    inline
    void
    And::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      Condition::write(bstr);
      bstr.write_array(operands);
    }
    
    inline
    void
    And::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      Condition::read(bstr);
      bstr.read_array(operands);
    }
      
    inline
    void
    And::normalize(const El::Dictionary::Morphology::WordInfoManager&
                   word_info_manager) throw(El::Exception)
    {
      for(ConditionArray::const_iterator it(operands.begin()),
            ie(operands.end()); it != ie; ++it)
      {
        Condition* condition = it->in();
        condition->normalize(word_info_manager);
      }
    }
    
    inline
    void
    And::intersect_results(Result& result,
                           MessageMatchInfoMap& match_info,
                           unsigned long flags,
                           const Result& op_result,
                           const MessageMatchInfoMap& op_match_info)
      throw(El::Exception)
    {
      
      for(Result::iterator it = result.begin(); it != result.end(); ++it)
      {
        Message::Number number = it->first;
        bool erase = op_result.find(number) == op_result.end();
          
        if(erase)
        {
          result.erase(it);
          match_info.erase(number);
        }
        else
        {
          MessageMatchInfoMap::const_iterator mit = op_match_info.find(number);
          
          if(mit != op_match_info.end())
          {
            match_info[number].insert(mit->second);
          }          
        }

      }
    }

    inline
    ConditionArray
    And::subconditions() const throw(El::Exception)
    {
      return operands;
    }
    
    //
    // Except class
    //
    
    inline
    Condition*
    Except::optimize() throw(El::Exception)
    {
      right = right->optimize();

      if(right->type() == TP_NONE)
      {
        left = left->optimize();
        return El::RefCount::add_ref(left.in());
      }

      left = left->exclude(right);
      left = left->optimize();

      if(left->type() == TP_NONE || right.in() == 0)
      {
        return left.retn();
      }

      return El::RefCount::add_ref(this);
    }

    inline
    void
    Except::relax(size_t level) throw(El::Exception)
    {
      left->relax(level);
    }
    
    inline
    Condition*
    Except::filter(Filter_var& cond) throw(El::Exception)
    {
      left = left->filter(cond);

      if(left->type() == TP_NONE)
      {
        return left.retn();
      }

      return El::RefCount::add_ref(this);      
    }

    inline
    Condition*
    Except::exclude(Condition_var& cond) throw(El::Exception)
    {
      left = left->exclude(cond);

      if(left->type() == TP_NONE)
      {
        return left.retn();
      }

      return El::RefCount::add_ref(this);      
    }

    inline
    Condition*
    Except::intersect(Condition_var& cond) throw(El::Exception)
    {
//      Condition_var cond_dup = cond; // Not change original var-ptr
//      left = left->intersect(cond_dup);
      left = left->intersect(cond);

      if(left->type() == TP_NONE)
      {
        return left.retn();
      }

      return El::RefCount::add_ref(this);      
    }
      
    inline
    void
    Except::dump(std::wostream& ostr, std::wstring& ident) const
      throw(El::Exception)
    {
      ostr << ident << L"EXCEPT\n";

      ident += L"  ";

      left->dump(ostr, ident);
      right->dump(ostr, ident);
      
      ident.resize(ident.length() - 2);
    }
    
    inline
    Condition::Type
    Except::type() const throw()
    {
      return TP_EXCEPT;
    }

    inline
    void
    Except::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      Condition::write(bstr);
      bstr << left << right;
    }
    
    inline
    void
    Except::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      Condition::read(bstr);
      bstr >> left >> right;
    }
      
    inline
    void
    Except::normalize(const El::Dictionary::Morphology::WordInfoManager&
                      word_info_manager) throw(El::Exception)
    {
      left->normalize(word_info_manager);
      right->normalize(word_info_manager);
    }
    
    inline
    ConditionArray
    Except::subconditions() const throw(El::Exception)
    {
      ConditionArray result(2);
      result[0] = left;
      result[1] = right;
      
      return result;
    }
    
    //
    // Filter class
    //
    inline
    Condition*    
    Filter::optimize() throw(El::Exception)
    {
      Filter_var cond = El::RefCount::add_ref(this);
  
      condition = condition->filter(cond);
      condition = condition->optimize();

      if(condition->type() == TP_NONE || cond.in() == 0)
      {
        return condition.retn();
      }
      
      return El::RefCount::add_ref(this);      
    }

    inline
    Condition*
    Filter::filter(Filter_var& cond) throw(El::Exception)
    {
      condition = condition->filter(cond);

      if(condition->type() == TP_NONE)
      {
        return condition.retn();
      }

      return El::RefCount::add_ref(this);      
    }
    
    inline
    Condition*
    Filter::intersect(Condition_var& cond) throw(El::Exception)
    {
//      Condition_var cond_dup = cond; // Not change original var-ptr
//      condition = condition->intersect(cond_dup);

      condition = condition->intersect(cond);
      
      if(condition->type() == TP_NONE)
      {
        return condition.retn();
      }

      return El::RefCount::add_ref(this);      
    }
    
    inline
    Condition*
    Filter::exclude(Condition_var& cond) throw(El::Exception)
    {
      condition = condition->exclude(cond);

      if(condition->type() == TP_NONE)
      {
        return condition.retn();
      }
      
      return El::RefCount::add_ref(this);      
    }

    inline
    Condition::Result*
    Filter::evaluate(Context& context,
                     MessageMatchInfoMap& match_info,
                     unsigned long flags) const
      throw(El::Exception)
    {
      context.filters.push_front(this);
      
      ResultPtr res(condition->evaluate(context, match_info, flags));
      
      context.filters.erase(context.filters.begin());
      return res.release();
    }
    
    inline
    Condition::Result*
    Filter::evaluate_simple(Context& context,
                            MessageMatchInfoMap& match_info,
                            unsigned long flags)
      const throw(El::Exception)
    {
      ResultPtr res(condition->evaluate_simple(context, match_info, flags));
      Result& result = *res;

      for(Result::iterator it = result.begin(); it != result.end(); ++it)
      {
        if(!satisfy(*it->second, context))
        {
          match_info.erase(it->first);          
          result.erase(it);
        }
      }
      
      return res.release();
    }

    inline
    void
    Filter::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      Condition::write(bstr);
      bstr << condition << reversed;
    }
    
    inline
    void
    Filter::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      Condition::read(bstr);
      bstr >> condition >> reversed;
    }
      
    inline
    void
    Filter::normalize(const El::Dictionary::Morphology::WordInfoManager&
                      word_info_manager) throw(El::Exception)
    {
      condition->normalize(word_info_manager);
    }
    
    inline
    ConditionArray
    Filter::subconditions() const throw(El::Exception)
    {
      ConditionArray result(1);
      result[0] = condition;
      
      return result;
    }

    //
    // Fetched class
    //
    inline
    void
    Fetched::dump(std::wostream& ostr, std::wstring& ident) const
      throw(El::Exception)
    {
      ostr << ident << (reversed ? L"FETCHED BEFORE\n" : L"FETCHED\n");

      ident += L"  ";

      condition->dump(ostr, ident);
      ostr << ident;
      
      time.dump(ostr);
      ostr << std::endl;
      
      ident.resize(ident.length() - 2);
    }
      
    inline
    Condition::Type
    Fetched::type() const throw()
    {
      return TP_FETCHED;
    }

    inline
    void
    Fetched::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      Filter::write(bstr);
      bstr << time;
    }
    
    inline
    void
    Fetched::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      Filter::read(bstr);
      bstr >> time;
    }
      
    inline
    bool
    Fetched::satisfy(const Message::StoredMessage& msg,
                     Context& context) const
      throw(El::Exception)
    {
      return reversed ?
        msg.fetched < time.value(context) : msg.fetched >= time.value(context);
    }
      
    inline
    Condition::Result*
    Fetched::evaluate(Context& context,
                      MessageMatchInfoMap& match_info,
                      unsigned long flags) const
      throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_meter);
      return Filter::evaluate(context, match_info, flags);
    }

    //
    // Visited class
    //
    inline
    void
    Visited::dump(std::wostream& ostr, std::wstring& ident) const
      throw(El::Exception)
    {
      ostr << ident << (reversed ? L"VISITED BEFORE\n" : L"VISITED\n");

      ident += L"  ";

      condition->dump(ostr, ident);
      ostr << ident;
      
      time.dump(ostr);
      ostr << std::endl;
      
      ident.resize(ident.length() - 2);
    }
      
    inline
    Condition::Type
    Visited::type() const throw()
    {
      return TP_VISITED;
    }

    inline
    void
    Visited::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      Filter::write(bstr);
      bstr << time;
    }
    
    inline
    void
    Visited::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      Filter::read(bstr);
      bstr >> time;
    }
      
    inline
    bool
    Visited::satisfy(const Message::StoredMessage& msg,
                     Context& context) const
      throw(El::Exception)
    {
      return reversed ?
        msg.visited < time.value(context) : msg.visited >= time.value(context);
    }
      
    inline
    Condition::Result*
    Visited::evaluate(Context& context,
                      MessageMatchInfoMap& match_info,
                      unsigned long flags) const
      throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_meter);
      return Filter::evaluate(context, match_info, flags);
    }

    //
    // PubDate class
    //
    inline
    void
    PubDate::dump(std::wostream& ostr, std::wstring& ident) const
      throw(El::Exception)
    {
      ostr << ident << (reversed ? L"DATE BEFORE\n" : L"DATE\n");

      ident += L"  ";

      condition->dump(ostr, ident);
      ostr << ident;
      
      time.dump(ostr);
      ostr << std::endl;
      
      ident.resize(ident.length() - 2);
    }
      
    inline
    Condition::Type
    PubDate::type() const throw()
    {
      return TP_PUB_DATE;
    }

    inline
    void
    PubDate::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      Filter::write(bstr);
      bstr << time;
    }
    
    inline
    void
    PubDate::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      Filter::read(bstr);
      bstr >> time;
    }
      
    inline
    bool
    PubDate::satisfy(const Message::StoredMessage& msg,
                     Context& context) const
      throw(El::Exception)
    {
      return reversed ?
        msg.published < time.value(context) :
        msg.published >= time.value(context);
    }
      
    inline
    Condition::Result*
    PubDate::evaluate(Context& context,
                      MessageMatchInfoMap& match_info,
                      unsigned long flags) const
      throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_meter);
      return Filter::evaluate(context, match_info, flags);
    }

    //
    // With class
    //
    inline
    void
    With::dump(std::wostream& ostr, std::wstring& ident) const
      throw(El::Exception)
    {
      ostr << ident << (reversed ? L"WITH NO\n" : L"WITH\n");

      ident += L"  ";

      condition->dump(ostr, ident);
      
      for(FeatureArray::const_iterator it(features.begin()),
            ie(features.end()); it != ie; ++it)
      {
        ostr << ident;

        switch(*it)
        {
        case FT_IMAGE: ostr << "IMAGE"; break;
        }

        ostr << std::endl;
      }
      
      ident.resize(ident.length() - 2);
    }
      
    inline
    Condition::Type
    With::type() const throw()
    {
      return TP_WITH;
    }

    inline
    void
    With::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      Filter::write(bstr);
      bstr.write_array(features);
    }
    
    inline
    void
    With::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      Filter::read(bstr);
      bstr.read_array(features);
    }
      
    inline
    bool
    With::satisfy(const Message::StoredMessage& msg, Context& context) const
      throw(El::Exception)
    {
      for(FeatureArray::const_iterator it(features.begin()),
            ie(features.end()); it != ie; ++it)
      {
        switch(*it)
        {
        case FT_IMAGE:
          {
            bool has_image =
              (msg.flags & Message::StoredMessage::MF_HAS_IMAGES) != 0;

            if((reversed && has_image) || (!reversed && !has_image))
            {
              return false;
            }
          }
        }        
      }

      return true;
    }
      
    inline
    Condition::Result*
    With::evaluate(Context& context,
                   MessageMatchInfoMap& match_info,
                   unsigned long flags) const
      throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_meter);
      return Filter::evaluate(context, match_info, flags);
    }

    inline
    Condition::Result*
    With::evaluate_simple(Context& context,
                          MessageMatchInfoMap& match_info,
                          unsigned long flags) const
      throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_simple_meter);
      return Filter::evaluate_simple(context, match_info, flags);
    }

    //
    // FilterValueList class
    //
    template<const Condition::Type condition_type,
             typename VALUE_LIST,
             typename VALUE_SET>
    Condition::Type
    FilterValueList<condition_type, VALUE_LIST, VALUE_SET>::type() const
      throw()
    {
      return condition_type;
    }
    
    template<const Condition::Type condition_type,
             typename VALUE_LIST,
             typename VALUE_SET>
    void
    FilterValueList<condition_type, VALUE_LIST, VALUE_SET>::write(
      El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      Filter::write(bstr);
      bstr.write_array(values);
    }
    
    template<const Condition::Type condition_type,
             typename VALUE_LIST,
             typename VALUE_SET>
    void
    FilterValueList<condition_type, VALUE_LIST, VALUE_SET>::read(
      El::BinaryInStream& bstr) throw(El::Exception)
    {
      Filter::read(bstr);
      bstr.read_array(values);
    }
      
    template<const Condition::Type condition_type,
             typename VALUE_LIST,
             typename VALUE_SET>
    typename FilterValueList<condition_type, VALUE_LIST, VALUE_SET>::OptimizationInfo*
    FilterValueList<condition_type, VALUE_LIST, VALUE_SET>::optimization_info()
      throw(Exception, El::Exception)
    {
      if(optimization_info_.get() == 0)
      {
        optimization_info_.reset(new OptimizationInfo());

        OptimizationInfo* opt =
          dynamic_cast<OptimizationInfo*>(optimization_info_.get());

        opt->values.resize(values.size());
        
        for(typename ValueList::const_iterator i(values.begin()),
              e(values.end()); i != e; ++i)
        {
          opt->values.insert(*i);
        }
      }

      OptimizationInfo* opt =
        dynamic_cast<OptimizationInfo*>(optimization_info_.get());
      
      assert(opt != 0);
      return opt;      
    }
    
    template<const Condition::Type condition_type,
             typename VALUE_LIST,
             typename VALUE_SET>
    Condition*
    FilterValueList<condition_type, VALUE_LIST, VALUE_SET>::filter(
      Filter_var& cond) throw(El::Exception)
    {
      if(cond->type() != type())
      {
        condition = condition->filter(cond);
        return El::RefCount::add_ref(this);
      }
      
      FilterValueListType* param =
        dynamic_cast<FilterValueListType*>(cond.in());
      
      assert(param != 0);
      
      bool param_non_reversed = !param->reversed;
      bool changed = false;
        
      if(reversed)
      {
        const ValueSet& own_values = optimization_info()->values;
        ValueList new_values;

        if(!param_non_reversed)
        {
          values.swap(new_values);
        }

        new_values.reserve(param->values.size() + new_values.size());
            
        for(typename ValueList::iterator i(param->values.begin()),
              e(param->values.end()); i != e; ++i)
        {
          const typename ValueList::value_type& v = *i;
          
          if(own_values.find(v) == own_values.end())
          {
            new_values.push_back(v);
          }
        }

        values.swap(new_values);
          
        if(param_non_reversed)
        {
          reversed = false;
        }

        changed = true;
      }
      else
      {
        const ValueSet& param_values = param->optimization_info()->values;
        typename ValueSet::const_iterator param_values_end(param_values.end());

        for(typename ValueList::iterator i(values.begin()), e(values.end());
            i != e; )
        {
          if((param_values.find(*i) == param_values_end) == param_non_reversed)
          {
            i = values.erase(i);
            e = values.end();
            changed = true;
          }
          else
          {
            ++i;
          }  
        }
      }

      if(changed)
      {
        optimization_info_.reset(0);
      }
      
      if(values.empty())
      {
        if(!reversed)
        {
          cond = 0;
          return new None();
        }
          
        condition = condition->filter(cond);
        cond = 0;
          
        return condition.retn();
      }

      condition = condition->filter(cond);
      cond = 0;

      if(condition->type() == TP_NONE)
      {
        return condition.retn();
      }

      return El::RefCount::add_ref(this);
    }

    inline
    void
    Lang::dump(std::wostream& ostr, std::wstring& ident) const
      throw(El::Exception)
    {
      ostr << ident << (reversed ? L"LANGUAGE NOT\n" : L"LANGUAGE\n");

      ident += L"  ";

      condition->dump(ostr, ident);
      ostr << ident;

      for(ValueList::const_iterator i(values.begin()), e(values.end());
          i != e; ++i)
      {
        std::wstring language;
        El::String::Manip::utf8_to_wchar(i->l3_code(), language);
      
        if(i != values.begin())
        {
          ostr << L" ";
        }
        
        ostr << language; 
      }

      ostr << std::endl;
      
      ident.resize(ident.length() - 2);
    }

    inline
    Condition::Result*
    Lang::evaluate(Context& context,
                   MessageMatchInfoMap& match_info,
                   unsigned long flags) const
      throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_meter);
      return Filter::evaluate(context, match_info, flags);
    }

    inline
    Condition::Result*
    Lang::evaluate_simple(Context& context,
                          MessageMatchInfoMap& match_info,
                          unsigned long flags) const
      throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_simple_meter);
      return Filter::evaluate_simple(context, match_info, flags);
    }

    inline
    bool
    Lang::satisfy(const Message::StoredMessage& msg,
                  Context& context) const throw(El::Exception)
    {
      for(ValueList::const_iterator i(values.begin()), e(values.end());
          i != e; ++i)
      {
        if(msg.lang == *i)
        {
          return !reversed;
        }
      }

      return reversed;
    }
      
    //
    // Country class
    //
    inline
    void
    Country::dump(std::wostream& ostr, std::wstring& ident) const
      throw(El::Exception)
    {
      ostr << ident << (reversed ? L"COUNTRY NOT\n" : L"COUNTRY\n");

      ident += L"  ";

      condition->dump(ostr, ident);
      ostr << ident;

      for(ValueList::const_iterator b(values.begin()), i(b), e(values.end());
          i != e; ++i)
      {
        std::wstring country;
        El::String::Manip::utf8_to_wchar(i->l3_code(), country);
      
        if(i != b)
        {
          ostr << L" ";
        }
        
        ostr << country; 
      }

      ostr << std::endl;
      ident.resize(ident.length() - 2);
    }
      
    inline
    Condition::Result*
    Country::evaluate(Context& context,
                      MessageMatchInfoMap& match_info,
                      unsigned long flags) const
      throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_meter);
      return Filter::evaluate(context, match_info, flags);
    }

    inline
    Condition::Result*
    Country::evaluate_simple(Context& context,
                             MessageMatchInfoMap& match_info,
                             unsigned long flags)
      const throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_simple_meter);
      return Filter::evaluate_simple(context, match_info, flags);
    }

    inline
    bool
    Country::satisfy(const Message::StoredMessage& msg,
                     Context& context)
      const throw(El::Exception)
    {
      for(ValueList::const_iterator i(values.begin()), e(values.end());
          i != e; ++i)
      {
//        std::cerr << "comparing " << msg.country.el_code() << " ? "
//                  << it->el_code() << std::endl;
        
        if(msg.country == *i)
        {
//          std::cerr << "TRUE\n";
          return !reversed;
        }
      }

//      std::cerr << "FALSE\n";
      return reversed;
    }
      
    //
    // Space class
    //
    inline
    void
    Space::dump(std::wostream& ostr, std::wstring& ident) const
      throw(El::Exception)
    {
      ostr << ident << (reversed ? L"SPACE NOT\n" : L"SPACE\n");

      ident += L"  ";

      condition->dump(ostr, ident);
      ostr << ident;

      for(ValueList::const_iterator b(values.begin()), i(b), e(values.end());
          i != e; ++i)
      {
        if(i != b)
        {
          ostr << L" ";
        }
        
        ostr << *i;
      }

      ostr << std::endl;
      
      ident.resize(ident.length() - 2);
    }
      
    inline
    Condition::Result*
    Space::evaluate(Context& context,
                    MessageMatchInfoMap& match_info,
                    unsigned long flags) const
      throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_meter);
      return Filter::evaluate(context, match_info, flags);
    }

    inline
    Condition::Result*
    Space::evaluate_simple(Context& context,
                           MessageMatchInfoMap& match_info,
                           unsigned long flags)
      const throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_simple_meter);
      return Filter::evaluate_simple(context, match_info, flags);
    }

    inline
    bool
    Space::satisfy(const Message::StoredMessage& msg, Context& context) const
      throw(El::Exception)
    {
      for(ValueList::const_iterator i(values.begin()), e(values.end());
          i != e; ++i)
      {
        if(msg.space == *i)
        {
          return !reversed;
        }
      }

      return reversed;
    }
      
    //
    // Domain class
    //
    inline
    void
    Domain::dump(std::wostream& ostr, std::wstring& ident) const
      throw(El::Exception)
    {
      ostr << ident << (reversed ? L"DOMAIN NOT\n" : L"DOMAIN\n");

      ident += L"  ";

      condition->dump(ostr, ident);
      ostr << ident;
      
      for(DomainList::const_iterator b(domains.begin()), i(b),
            e(domains.end()); i != e; ++i)
      {
        std::wstring dom;
        El::String::Manip::utf8_to_wchar(i->name.c_str(), dom);

        if(i != b)
        {
          ostr << L" ";
        }
        
        ostr << dom; 
      }

      ostr << std::endl;
      ident.resize(ident.length() - 2);
    }
      
    inline
    void
    Domain::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      Filter::write(bstr);
      bstr.write_container(domains);
    }
    
    inline
    void
    Domain::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      Filter::read(bstr);
      bstr.read_container(domains);
    }
      
    inline
    Condition::Result*
    Domain::evaluate(Context& context,
                     MessageMatchInfoMap& match_info,
                     unsigned long flags) const
      throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_meter);
      return Filter::evaluate(context, match_info, flags);
    }

    inline
    Condition::Result*
    Domain::evaluate_simple(Context& context,
                            MessageMatchInfoMap& match_info,
                            unsigned long flags)
      const throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_simple_meter);
      return Filter::evaluate_simple(context, match_info, flags);
    }

    inline
    Condition::Type
    Domain::type() const throw()
    {
      return TP_DOMAIN;
    }

    inline
    bool
    Domain::satisfy(const Message::StoredMessage& msg,
                    Context& context) const
      throw(El::Exception)
    {
      for(DomainList::const_iterator i(domains.begin()), e(domains.end());
          i != e; ++i)
      {
        bool satisfy = msg.in_domain(i->name.c_str(), i->len);
        
        if(satisfy)
        {
          return !reversed;
        }
      }

      return reversed;
    }

    //
    // FilterValue class template
    // 
    template<const Condition::Type condition_type, typename VALUE_TYPE>
    Condition::Type
    FilterValue<condition_type, VALUE_TYPE>::type() const throw()
    {
      return condition_type;
    }

    template<const Condition::Type condition_type, typename VALUE_TYPE>
    void
    FilterValue<condition_type, VALUE_TYPE>::write(El::BinaryOutStream& bstr)
      const throw(El::Exception)
    {
      Filter::write(bstr);
      bstr << value;      
    }
    
    template<const Condition::Type condition_type, typename VALUE_TYPE>
    void
    FilterValue<condition_type, VALUE_TYPE>::read(El::BinaryInStream& bstr)
      throw(El::Exception)
    {
      Filter::read(bstr);
      bstr >> value;      
    }

    template<const Condition::Type condition_type, typename VALUE_TYPE>
    Condition::Result*
    FilterValue<condition_type, VALUE_TYPE>::evaluate(
      Context& context,
      MessageMatchInfoMap& match_info,
      unsigned long flags) const
      throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_meter);
      return Filter::evaluate(context, match_info, flags);
    }

    template<const Condition::Type condition_type, typename VALUE_TYPE>
    Condition::Result*
    FilterValue<condition_type, VALUE_TYPE>::evaluate_simple(
      Context& context,
      MessageMatchInfoMap& match_info,
      unsigned long flags) const
      throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_simple_meter);
      return Filter::evaluate_simple(context, match_info, flags);
    }
    
    template<const Condition::Type condition_type, typename VALUE_TYPE>
    void
    FilterValue<condition_type, VALUE_TYPE>::dump(std::wostream& ostr,
                                                  std::wstring& ident) const
      throw(El::Exception)
    {
      ostr << ident;
      El::String::Manip::latin1_to_wchar(operation(), ostr);

      if(reversed)
      {
        ostr << L" ";
        El::String::Manip::latin1_to_wchar(negation(), ostr);
      }

      ostr << L"\n";
      ident += L"  ";

      condition->dump(ostr, ident);
      ostr << ident;
      dump_value(ostr, ident);
      ostr << std::endl;
      
      ident.resize(ident.length() - 2);
    }
      
    template<const Condition::Type condition_type, typename VALUE_TYPE>
    void
    FilterValue<condition_type, VALUE_TYPE>::print(std::ostream& ostr) const
      throw(El::Exception)
    {
      condition->print(ostr);

      ostr << " " << operation();

      if(reversed)
      {
        ostr << " " << negation();
      }

      ostr << " ";
      print_value(ostr);
    }

    template<const Condition::Type condition_type, typename VALUE_TYPE>
    void
    FilterValue<condition_type, VALUE_TYPE>::print_value(std::ostream& ostr)
      const throw(El::Exception)
    {
      ostr << value;
    }
    
    template<const Condition::Type condition_type, typename VALUE_TYPE>
    void
    FilterValue<condition_type, VALUE_TYPE>::dump_value(std::wostream& ostr,
                                                        std::wstring& ident)
      const throw(El::Exception)
    {
      ostr << value;
    }

    //
    // Capacity class
    //
    inline
    bool
    Capacity::satisfy(const Message::StoredMessage& msg,
                      Context& context) const
      throw(El::Exception)
    {
      return reversed ? (msg.event_capacity < value) :
        (msg.event_capacity >= value);
    }

    //
    // Impressions class
    //
    inline
    bool
    Impressions::satisfy(const Message::StoredMessage& msg,
                         Context& context) const
      throw(El::Exception)
    {
      return reversed ? (msg.impressions < value) : (msg.impressions >= value);
    }

    //
    // FeedImpressions class
    //
    inline
    bool
    FeedImpressions::satisfy(const Message::StoredMessage& msg,
                             Context& context) const
      throw(El::Exception)
    {
      const Message::FeedInfoMap& feeds = context.messages.feeds;
      
      Message::FeedInfoMap::const_iterator i =
        feeds.find(msg.source_url.c_str());
      
      uint64_t impressions = i == feeds.end() ? 0 : i->second->impressions;
      return reversed ? (impressions < value) : (impressions >= value);
    }

    //
    // Clicks class
    //
    inline
    bool
    Clicks::satisfy(const Message::StoredMessage& msg, Context& context) const
      throw(El::Exception)
    {
      return reversed ? (msg.clicks < value) : (msg.clicks >= value);
    }

    //
    // FeedClicks class
    //
    inline
    bool
    FeedClicks::satisfy(const Message::StoredMessage& msg,
                        Context& context) const
      throw(El::Exception)
    {
      const Message::FeedInfoMap& feeds = context.messages.feeds;
      
      Message::FeedInfoMap::const_iterator i =
        feeds.find(msg.source_url.c_str());
      
      uint64_t clicks = i == feeds.end() ? 0 : i->second->clicks;
      return reversed ? (clicks < value) : (clicks >= value);
    }

    //
    // CTR class
    //
    inline
    bool
    CTR::satisfy(const Message::StoredMessage& msg, Context& context) const
      throw(El::Exception)
    {
      float ctr = msg.impressions ?
        (float)(std::min(msg.clicks, msg.impressions) * 100) /
        msg.impressions : (float)0;
        
      return reversed ? (ctr < value) : (ctr >= value);
    }

    //
    // FeedCTR class
    //
    inline
    bool
    FeedCTR::satisfy(const Message::StoredMessage& msg, Context& context) const
      throw(El::Exception)
    {
      const Message::FeedInfoMap& feeds = context.messages.feeds;
      
      Message::FeedInfoMap::const_iterator i =
        feeds.find(msg.source_url.c_str());

      float ctr = 0;

      if(i != feeds.end())
      {
        uint64_t impressions = i->second->impressions;

        if(impressions)
        {
          ctr = (float)(std::min(i->second->clicks, impressions) * 100) /
            impressions;
        }
      }
        
      return reversed ? (ctr < value) : (ctr >= value);
    }

    //
    // RCTR class
    //
    inline
    RCTR::RCTR() throw()
        : ril_is_default(0),
          respected_impression_level(0)
    {
      value = 0;
    }
    
    inline
    bool
    RCTR::satisfy(const Message::StoredMessage& msg, Context& context) const
      throw(El::Exception)
    {
      uint64_t respected_impressions =
        std::max((uint64_t)respected_impression_level, msg.impressions);
        
      float ctr = respected_impressions ?
        (float)(std::min(msg.clicks, msg.impressions) * 100) /
        respected_impressions : (float)0;
        
      return reversed ? (ctr < value) : (ctr >= value);
    }

    inline
    void
    RCTR::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      RCTR_Base::write(bstr);
      bstr << ril_is_default << respected_impression_level;
    }

    inline
    void
    RCTR::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      RCTR_Base::read(bstr);
      bstr >> ril_is_default >> respected_impression_level;
    }

    inline
    void
    RCTR::print_value(std::ostream& ostr) const throw(El::Exception)
    {
      ostr << value;

      if(!ril_is_default)
      {
        ostr << " " << respected_impression_level;
      }
    }
    
    inline
    void
    RCTR::dump_value(std::wostream& ostr,
                     std::wstring& ident) const throw(El::Exception)
    {
      ostr << value;

      if(!ril_is_default)
      {
        ostr << L"\n" << ident << respected_impression_level;
      }
    }
    
    //
    // FeedRCTR class
    //
    inline
    FeedRCTR::FeedRCTR() throw()
        : ril_is_default(0),
          respected_impression_level(0)
    {
      value = 0;
    }
    
    inline
    bool
    FeedRCTR::satisfy(const Message::StoredMessage& msg,
                      Context& context) const
      throw(El::Exception)
    {
      const Message::FeedInfoMap& feeds = context.messages.feeds;
      
      Message::FeedInfoMap::const_iterator i =
        feeds.find(msg.source_url.c_str());

      float ctr = 0;

      if(i != feeds.end())
      {
        uint64_t impressions = i->second->impressions;

        if(impressions)
        {
          uint64_t respected_impressions =
            std::max((uint64_t)respected_impression_level, impressions);
        
          ctr = (float)(std::min(i->second->clicks, impressions) * 100) /
            respected_impressions;
        }
      }
      
      return reversed ? (ctr < value) : (ctr >= value);
    }

    inline
    void
    FeedRCTR::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      FeedRCTR_Base::write(bstr);
      bstr << ril_is_default << respected_impression_level;
    }

    inline
    void
    FeedRCTR::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      FeedRCTR_Base::read(bstr);
      bstr >> ril_is_default >> respected_impression_level;
    }

    inline
    void
    FeedRCTR::print_value(std::ostream& ostr) const throw(El::Exception)
    {
      ostr << value;

      if(!ril_is_default)
      {
        ostr << " " << respected_impression_level;
      }
    }
    
    inline
    void
    FeedRCTR::dump_value(std::wostream& ostr,
                         std::wstring& ident) const throw(El::Exception)
    {
      ostr << value;

      if(!ril_is_default)
      {
        ostr << L"\n" << ident << respected_impression_level;
      }
    }
    
    //
    // Signature class
    //
    inline
    void
    Signature::dump(std::wostream& ostr, std::wstring& ident) const
      throw(El::Exception)
    {
      ostr << ident << (reversed ? L"SIGNATURE NOT\n" : L"SIGNATURE\n");

      ident += L"  ";

      condition->dump(ostr, ident);

      for(SignatureArray::const_iterator i(values.begin()), e(values.end());
          i != e; ++i)
      {
        ostr << ident << std::uppercase << std::hex << *i << std::nouppercase
             << std::dec << std::endl;
      }

      ident.resize(ident.length() - 2);
    }
      
    inline
    void
    Signature::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      Filter::write(bstr);
      bstr << values;
    }
    
    inline
    void
    Signature::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      Filter::read(bstr);
      bstr >> values;
    }
      
    inline
    Condition::Result*
    Signature::evaluate(Context& context,
                        MessageMatchInfoMap& match_info,
                        unsigned long flags) const
      throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_meter);
      return Filter::evaluate(context, match_info, flags);
    }

    inline
    Condition::Result*
    Signature::evaluate_simple(Context& context,
                               MessageMatchInfoMap& match_info,
                               unsigned long flags)
      const throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_simple_meter);
      return Filter::evaluate_simple(context, match_info, flags);
    }

    inline
    Condition::Type
    Signature::type() const throw()
    {
      return TP_SIGNATURE;
    }

    inline
    bool
    Signature::satisfy(const Message::StoredMessage& msg,
                       Context& context) const
      throw(El::Exception)
    {
      for(SignatureArray::const_iterator i(values.begin()), e(values.end());
          i != e; ++i)
      {
        if(msg.signature == *i)
        {
          return !reversed;
        }
      }
      
      return reversed;
    }

    //
    // Domain::DomainRec class
    //
    inline
    void
    Domain::DomainRec::write(El::BinaryOutStream& ostr) const
      throw(Exception, El::Exception)
    {
      ostr << name << len;
    }

    inline
    void
    Domain::DomainRec::read(El::BinaryInStream& istr)
      throw(Exception, El::Exception)
    {
      istr >> name >> len;
    }
      
    //
    // Condition class
    //
    inline
    Condition::~Condition() throw()
    {
    }

    //
    // Site class
    //

    inline
    Condition*
    Site::intersect(Condition_var& condition) throw(El::Exception)
    {
      switch(condition->type())
      {
      case TP_SITE:
        {
          Site* param = dynamic_cast<Site*>(condition.in());
          assert(param != 0);

          const StringConstPtrSet& sites = param->optimization_info()->hosts;

          bool changed = false;

          for(HostNameList::iterator i(hostnames.begin()), e(hostnames.end());
              i != e; )
          {
            if(sites.find(i->c_str()) == sites.end())
            {
              i = hostnames.erase(i);
              e = hostnames.end();
              changed = true;
            }
            else
            {
              ++i;
            }
          }

          if(changed)
          {
            optimization_info_.reset(0);
          }
          
          condition = 0;

          if(hostnames.empty())
          {
            return new None();
          }

          break;
        }
      default:
        break;
      }
 
      return El::RefCount::add_ref(this);
    }    

    inline
    Condition*
    Site::exclude(Condition_var& condition) throw(El::Exception)
    {
      switch(condition->type())
      {
      case TP_SITE:
        {
          Site* param = dynamic_cast<Site*>(condition.in());
          assert(param != 0);

          const StringConstPtrSet& sites = param->optimization_info()->hosts;

          bool changed = false;

          for(HostNameList::iterator i(hostnames.begin()), e(hostnames.end());
              i != e; )
          {
            if(sites.find(i->c_str()) != sites.end())
            {
              i = hostnames.erase(i);
              e = hostnames.end();
              changed = true;
            }
            else
            {
              ++i;
            }
          }

          if(changed)
          {
            optimization_info_.reset(0);
          }

          condition = 0;
          
          if(hostnames.empty())
          {
            return new None();
          }
          
          break;
        }
      default:
        break;
      }
 
      return El::RefCount::add_ref(this);
    }
    
    inline
    Condition*
    Site::filter(Filter_var& cond) throw(El::Exception)
    {
      if(cond->type() != TP_DOMAIN)
      {
        return El::RefCount::add_ref(this);
      }
      
      Domain* param = dynamic_cast<Domain*>(cond.in());
      assert(param != 0);

      Domain::DomainList::const_iterator db(param->domains.begin()),
        de(param->domains.end());

      SizeArray& host_lengths = optimization_info()->host_lengths;
      SizeArray::iterator hli = host_lengths.begin();
          
      bool changed = false;
        
      for(HostNameList::iterator i(hostnames.begin()), e(hostnames.end());
          i != e; )
      {
        size_t host_len = *hli;
        Domain::DomainList::const_iterator j(db);
          
        for(; j != de; ++j)
        {
          size_t domain_len = j->len;
              
          if(domain_len > host_len)
          {
            continue;
          }

          size_t offset = host_len - domain_len;
          const char* host = i->c_str();
          const char* domain = j->name.c_str();
            
          if(strcmp(host + offset, domain) == 0 &&
             (!offset || host[offset - 1] == '.'))
          {
            break;
          }
        }

        if((j != de) == param->reversed)
        {
          i = hostnames.erase(i);
          e = hostnames.end();
          hli = host_lengths.erase(hli);
          changed = true;
        }
        else
        {
          ++i;
          ++hli;
        }
      }

      if(changed)
      {
        optimization_info_.reset(0);
      }

      cond = 0;
      
      if(hostnames.empty())
      {
        return new None();
      }
      
      return El::RefCount::add_ref(this);      
    }
    
    inline
    void
    Site::dump(std::wostream& ostr, std::wstring& ident) const
      throw(El::Exception)
    {
      ostr << ident << L"SITE";

      for(HostNameList::const_iterator i(hostnames.begin()),
            e(hostnames.end()); i != e; ++i)
      {
        std::wstring host;
        El::String::Manip::utf8_to_wchar(i->c_str(), host);
        ostr << L" " << host;
      }
      
      ostr << std::endl;
    }

    inline
    Condition::Result*
    Site::evaluate_simple(Context& context,
                          MessageMatchInfoMap& match_info,
                          unsigned long flags) const
      throw(El::Exception)
    {
      return evaluate(context, match_info, flags);
    }

    inline
    Condition::Type
    Site::type() const throw()
    {
      return TP_SITE;
    }

    inline
    void
    Site::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      Condition::write(bstr);
      bstr.write_container(hostnames);
    }
    
    inline
    void
    Site::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      Condition::read(bstr);
      bstr.read_container(hostnames);
    }
      
    //
    // Url class
    //

    inline
    Condition*
    Url::intersect(Condition_var& condition) throw(El::Exception)
    {
      switch(condition->type())
      {
      case TP_URL:
        {
          Url* param = dynamic_cast<Url*>(condition.in());
          assert(param != 0);

          const StringConstPtrSet& url_set = param->optimization_info()->urls;
          bool changed = false;

          for(UrlList::iterator i(urls.begin()), e(urls.end()); i != e; )
          {
            if(url_set.find(i->c_str()) == url_set.end())
            {
              i = urls.erase(i);
              e = urls.end();
              changed = true;
            }
            else
            {
              ++i;
            }
          }

          if(changed)
          {
            optimization_info_.reset(0);
          }
          
          condition = 0;
          
          if(urls.empty())
          {
            return new None();
          }
          
          break;
        }
        
      case TP_SITE:
        {
          Site* param = dynamic_cast<Site*>(condition.in());
          assert(param != 0);

          const StringConstPtrSet& hosts = param->optimization_info()->hosts;
          
          UrlOptimizationInfo::UrlSiteArray& sites =
            optimization_info()->sites;

          bool changed = false;
          
          for(size_t i = 0; i < urls.size(); )
          {
            if(hosts.find(sites[i].name.c_str()) == hosts.end())
            {
              urls.erase(urls.begin() + i);
              sites.erase(sites.begin() + i);
              changed = true;
            }
            else
            {
              ++i;
            }
          }

          if(changed)
          {
            optimization_info_.reset(0);
          }
          
          condition = 0;
          
          if(urls.empty())
          {
            return new None();
          }
          
          break;
        }
        
      default:
        break;
      }
 
      return El::RefCount::add_ref(this);
    }

    inline
    Condition*
    Url::exclude(Condition_var& condition) throw(El::Exception)
    {
      switch(condition->type())
      {
      case TP_URL:
        {
          Url* param = dynamic_cast<Url*>(condition.in());
          assert(param != 0);

          const StringConstPtrSet& url_set = param->optimization_info()->urls;
          bool changed = false;
          
          for(UrlList::iterator i(urls.begin()), e(urls.end()); i != e; )
          {
            if(url_set.find(i->c_str()) != url_set.end())
            {
              i = urls.erase(i);
              e = urls.end();
              changed = true;
            }
            else
            {
              ++i;
            }
          }

          if(changed)
          {
            optimization_info_.reset(0);
          }
          
          condition = 0;
          
          if(urls.empty())
          {
            return new None();
          }
          
          break;
        }
        
      case TP_SITE:
        {
          Site* param = dynamic_cast<Site*>(condition.in());
          assert(param != 0);

          const StringConstPtrSet& hosts = param->optimization_info()->hosts;
          
          UrlOptimizationInfo::UrlSiteArray& sites =
            optimization_info()->sites;

          bool changed = false;

          for(size_t i = 0; i < urls.size(); )
          {
            if(hosts.find(sites[i].name.c_str()) != hosts.end())
            {
              urls.erase(urls.begin() + i);
              sites.erase(sites.begin() + i);
              changed = true;
            }
            else
            {
              ++i;
            }
          }

          if(changed)
          {
            optimization_info_.reset(0);
          }
          
          condition = 0;
          
          if(urls.empty())
          {
            return new None();
          }
          
          break;
        }
        
      default:
        break;
      }
 
      return El::RefCount::add_ref(this);
    }
    
    inline
    Condition*
    Url::filter(Filter_var& cond) throw(El::Exception)
    {
      if(cond->type() != TP_DOMAIN)
      {
        return El::RefCount::add_ref(this);
      }
      
      Domain* param = dynamic_cast<Domain*>(cond.in());
      assert(param != 0);

      Domain::DomainList::const_iterator db(param->domains.begin()),
        de(param->domains.end());

      UrlOptimizationInfo::UrlSiteArray& sites = optimization_info()->sites;
      UrlList::iterator ui(urls.begin());
          
      bool changed = false;
        
      for(UrlOptimizationInfo::UrlSiteArray::iterator i(sites.begin()),
            e(sites.end()); i != e; )
      {
        size_t site_len = i->len;
        Domain::DomainList::const_iterator j(db);
          
        for(; j != de; ++j)
        {
          size_t domain_len = j->len;
              
          if(domain_len > site_len)
          {
            continue;
          }

          size_t offset = site_len - domain_len;
          const char* host = i->name.c_str();
          const char* domain = j->name.c_str();
            
          if(strcmp(host + offset, domain) == 0 &&
             (!offset || host[offset - 1] == '.'))
          {
            break;
          }
        }

        if((j != de) == param->reversed)
        {
          i = sites.erase(i);
          e = sites.end();
          ui = urls.erase(ui);
          changed = true;
        }
        else
        {
          ++i;
          ++ui;
        }
      }

      if(changed)
      {
        optimization_info_.reset(0);
      }

      cond = 0;
      
      if(urls.empty())
      {
        return new None();
      }
      
      return El::RefCount::add_ref(this);
    }
    
    inline
    void
    Url::dump(std::wostream& ostr, std::wstring& ident) const
      throw(El::Exception)
    {
      ostr << ident << L"URL";

      for(UrlList::const_iterator i(urls.begin()), e(urls.end());
          i != e; ++i)
      {
        std::wstring url;
        El::String::Manip::utf8_to_wchar(i->c_str(), url);
        ostr << L" " << url;
      }
      
      ostr << std::endl;
    }

    inline
    Condition::Result*
    Url::evaluate_simple(Context& context,
                         MessageMatchInfoMap& match_info,
                         unsigned long flags) const
      throw(El::Exception)
    {
      return evaluate(context, match_info, flags);
    }

    inline
    Condition::Type
    Url::type() const throw()
    {
      return TP_URL;
    }

    inline
    void
    Url::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      Condition::write(bstr);
      bstr.write_container(urls);
    }
    
    inline
    void
    Url::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      Condition::read(bstr);
      bstr.read_container(urls);
    }

    //
    // Category class
    //    
    inline
    void
    Category::dump(std::wostream& ostr, std::wstring& ident) const
      throw(El::Exception)
    {
      ostr << ident << L"CATEGORY";

      for(unsigned long i = 0; i < categories.size(); i++)
      {
        std::wstring cat;
        El::String::Manip::utf8_to_wchar(categories[i].c_str(), cat);

        
        ostr << L" \"" << cat << L"\"";
      }
      
      ostr << std::endl;
    }

    inline
    const Condition::Result*
    Category::const_evaluate(Context& context, unsigned long flags) const
      throw(El::Exception)
    {
      if(categories.size() != 1)
      {
        return 0;
      }

      const Message::Category* category =
        context.messages.root_category->find(categories[0].c_str() + 1);
      
      return category ? &category->messages : 0;
    }
      
    inline
    Condition::Result*
    Category::evaluate_simple(Context& context,
                              MessageMatchInfoMap& match_info,
                              unsigned long flags) const
      throw(El::Exception)
    {
      return evaluate(context, match_info, flags);
    }

    inline
    Condition::Type
    Category::type() const throw()
    {
      return TP_CATEGORY;
    }

    inline
    void
    Category::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      Condition::write(bstr);
      bstr << categories;
    }
    
    inline
    void
    Category::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      Condition::read(bstr);
      bstr >> categories;
    }

    //
    // None class
    //
    inline
    void
    None::dump(std::wostream& ostr, std::wstring& ident) const
      throw(El::Exception)
    {
      ostr << ident << L"NONE" << std::endl;
    }

    inline
    void
    None::print(std::ostream& ostr) const throw(El::Exception)
    {
      ostr << "NONE";
    }
    
    inline
    Condition::Result*
    None::evaluate_simple(Context& context,
                          MessageMatchInfoMap& match_info,
                          unsigned long flags) const
      throw(El::Exception)
    {
      return new Result();
    }

    inline
    Condition::Result*
    None::evaluate(Context& context,
                   MessageMatchInfoMap& match_info,
                   unsigned long flags) const
      throw(El::Exception)
    {
      return new Result();
    }
    
    inline
    void
    None::normalize(
      const El::Dictionary::Morphology::WordInfoManager& word_info_manager)
      throw(El::Exception)
    {
    }
    
    inline
    void
    None::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      Condition::write(bstr);
    }
    
    inline
    void
    None::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      Condition::read(bstr);
    }
    
    //
    // Msg class
    //
    
    inline
    void
    Msg::dump(std::wostream& ostr, std::wstring& ident) const
      throw(El::Exception)
    {
      ostr << ident << L"MSG";

      for(Message::IdArray::const_iterator i(ids.begin()),
            e(ids.end()); i != e; ++i)
      {
        std::wstring id;
        El::String::Manip::utf8_to_wchar(i->string().c_str(), id);
        ostr << L" " << id;
      }
      
      ostr << std::endl;
    }

    inline
    Condition::Result*
    Msg::evaluate_simple(Context& context,
                         MessageMatchInfoMap& match_info,
                         unsigned long flags) const
      throw(El::Exception)
    {
      return evaluate(context, match_info, flags);
    }

    inline
    Condition::Type
    Msg::type() const throw()
    {
      return TP_MSG;
    }

    inline
    void
    Msg::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      Condition::write(bstr);
      bstr.write_array(ids);
    }
    
    inline
    void
    Msg::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      Condition::read(bstr);
      bstr.read_array(ids);
    }

    //
    // Event class
    //
    inline
    void
    Event::dump(std::wostream& ostr, std::wstring& ident) const
      throw(El::Exception)
    {
      ostr << ident << L"EVENT";

      for(LuidArray::const_iterator i(ids.begin()), e(ids.end());
          i != e; ++i)
      {
        std::wstring id;
        El::String::Manip::utf8_to_wchar(i->string().c_str(), id);
        ostr << L" " << id;
      }
      
      ostr << std::endl;
    }

    inline
    Condition::Result*
    Event::evaluate_simple(Context& context,
                           MessageMatchInfoMap& match_info,
                           unsigned long flags) const
      throw(El::Exception)
    {
      return evaluate(context, match_info, flags);
    }

    inline
    Condition::Type
    Event::type() const throw()
    {
      return TP_EVENT;
    }

    inline
    void
    Event::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      Condition::write(bstr);
      bstr.write_array(ids);
    }
    
    inline
    void
    Event::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      Condition::read(bstr);
      bstr.read_array(ids);
    }

    //
    // Every class
    //
    inline
    Condition*
    Every::intersect(Condition_var& condition) throw(El::Exception)
    {
      return condition.retn();
    }

    inline
    void
    Every::dump(std::wostream& ostr, std::wstring& ident) const
      throw(El::Exception)
    {
      ostr << ident << L"EVERY" << std::endl;
    }

    inline
    void
    Every::print(std::ostream& ostr) const throw(El::Exception)
    {
      ostr << "EVERY";
    }
    
    inline
    const Condition::Result*
    Every::const_evaluate(Context& context, unsigned long flags) const
      throw(El::Exception)
    {
      return &context.messages.messages;
    }

    inline
    Condition::Result*
    Every::evaluate_simple(Context& context,
                           MessageMatchInfoMap& match_info,
                           unsigned long flags) const
      throw(El::Exception)
    {
      return evaluate(context, match_info, flags);
    }

    inline
    Condition::Type
    Every::type() const throw()
    {
      return TP_EVERY;
    }

    inline
    void
    Every::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      Condition::write(bstr);
    }
    
    inline
    void
    Every::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      Condition::read(bstr);
    }
  }
}

#endif // _NEWSGATE_SERVER_COMMONS_SEARCH_SEARCHCONDITION_HPP_
