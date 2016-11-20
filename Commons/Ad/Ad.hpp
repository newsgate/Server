/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   Commons/Ad/Ad.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_COMMONS_AD_AD_HPP_
#define _NEWSGATE_SERVER_COMMONS_AD_AD_HPP_

#include <stdint.h>
#include <stdlib.h>

#include <string>
#include <vector>
#include <stack>
#include <iostream>
#include <sstream>

#include <ext/hash_map>
#include <ext/hash_set>

#include <google/dense_hash_map>
#include <google/dense_hash_set>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/BinaryStream.hpp>
#include <El/Hash/Hash.hpp>
#include <El/Lang.hpp>
#include <El/Country.hpp>
#include <El/Net/URL.hpp>

namespace NewsGate
{
  namespace Ad
  {
    EL_EXCEPTION(Exception, El::ExceptionBase);

    typedef uint64_t GroupId;
    typedef uint64_t ConditionId;
    typedef uint64_t CounterId;
    typedef uint64_t CreativeId;
    typedef uint32_t PageId;
    typedef uint32_t SlotId;
    typedef uint64_t AdvertiserId;
    typedef uint64_t SelectorUpdateNumber;

    typedef __gnu_cxx::hash_set<std::string, El::Hash::String> StringSet;    
    typedef std::vector<std::string> StringArray;

    typedef std::vector<SlotId> SlotIdArray;
    
    typedef std::string Category;
    typedef std::vector<Category> CategoryArray;
    typedef __gnu_cxx::hash_set<Category, El::Hash::String> CategorySet;    

    typedef std::string Source;
    typedef std::vector<Source> SourceArray;
    typedef __gnu_cxx::hash_set<Source, El::Hash::String> SourceSet;    

    typedef std::string SearchEngine;
    typedef std::vector<SearchEngine> SearchEngineArray;
    
    typedef __gnu_cxx::hash_set<SearchEngine, El::Hash::String>
    SearchEngineSet;    
    
    typedef std::string Crawler;
    typedef std::vector<Crawler> CrawlerArray;
    typedef __gnu_cxx::hash_set<Crawler, El::Hash::String> CrawlerSet;

    typedef std::vector<El::Net::IpMask> IpMaskArray;

    typedef std::string Tag;
    typedef std::vector<Tag> TagArray;
    typedef __gnu_cxx::hash_set<Tag, El::Hash::String> TagSet;
    
    typedef std::string Referer;
    typedef std::vector<Referer> RefererArray;
    typedef __gnu_cxx::hash_set<Referer, El::Hash::String> RefererSet;
    
    struct GroupIdSet:
      public google::dense_hash_set<GroupId, El::Hash::Numeric<GroupId> >
    {
      GroupIdSet() throw() { set_empty_key(0); set_deleted_key(UINT64_MAX); }
    };
    
    struct LangSet: public google::dense_hash_set<El::Lang, El::Hash::Lang>
    {
      LangSet() throw() { set_empty_key(El::Lang::nonexistent); }
    };
    
    typedef std::vector<El::Lang> LangArray;

    struct CountrySet:
      public google::dense_hash_set<El::Country, El::Hash::Country>
    {
      CountrySet() throw() { set_empty_key(El::Country::nonexistent); }
    };
    
    struct CapMinTimeMap:
      public google::dense_hash_map<GroupId,
                                    uint64_t,
                                    El::Hash::Numeric<GroupId> >
    {
      CapMinTimeMap() throw() {set_empty_key(0); set_deleted_key(UINT64_MAX);}
    };

    enum QueryType
    {
      QT_NONE = 0x1,
      QT_ANY = 0x2,
      QT_SEARCH = 0x4,
      QT_EVENT = 0x8,
      QT_CATEGORY = 0x10,
      QT_SOURCE = 0x20,
      QT_MESSAGE = 0x40
    };
    
    enum PageIdValue
    {
      PI_UNKNOWN,
      PI_DESK_PAPER,
      PI_DESK_NLINE,
      PI_DESK_COLUMN,
      PI_TAB_PAPER,
      PI_TAB_NLINE,
      PI_TAB_COLUMN,
      PI_MOB_NLINE,
      PI_MOB_COLUMN,
      PI_DESK_MESSAGE,
      PI_TAB_MESSAGE,
      PI_MOB_MESSAGE,
      PI_COUNT
    };
    
    enum SlotIdValue
    {
      SI_UNKNOWN = 0,
      SI_DESK_PAPER_FIRST = 1,
      SI_DESK_PAPER_MSG1 = SI_DESK_PAPER_FIRST,
      SI_DESK_PAPER_MSG2,
      SI_DESK_PAPER_MSA1,
      SI_DESK_PAPER_MSA2,
      SI_DESK_PAPER_RTB1,
      SI_DESK_PAPER_RTB2,
      SI_DESK_PAPER_ROOF,
      SI_DESK_PAPER_BASEMENT,
      SI_DESK_PAPER_LAST = SI_DESK_PAPER_BASEMENT,
      SI_DESK_NLINE_FIRST = 1000,
      SI_DESK_NLINE_MSA1 = SI_DESK_NLINE_FIRST,
      SI_DESK_NLINE_RTB1,
      SI_DESK_NLINE_RTB2,
      SI_DESK_NLINE_ROOF,
      SI_DESK_NLINE_BASEMENT,
      SI_DESK_NLINE_LAST = SI_DESK_NLINE_BASEMENT,
      SI_DESK_COLUMN_FIRST = 2000,
      SI_DESK_COLUMN_MSG1 = SI_DESK_COLUMN_FIRST,
      SI_DESK_COLUMN_MSG2,
      SI_DESK_COLUMN_MSA1,
      SI_DESK_COLUMN_RTB1,
      SI_DESK_COLUMN_RTB2,
      SI_DESK_COLUMN_ROOF,
      SI_DESK_COLUMN_BASEMENT,
      SI_DESK_COLUMN_LAST = SI_DESK_COLUMN_BASEMENT,
      SI_TAB_PAPER_FIRST = 3000,
      SI_TAB_PAPER_MSG1 = SI_TAB_PAPER_FIRST,
      SI_TAB_PAPER_MSG2,
      SI_TAB_PAPER_MSA1,
      SI_TAB_PAPER_MSA2,
      SI_TAB_PAPER_RTB1,
      SI_TAB_PAPER_RTB2,
      SI_TAB_PAPER_ROOF,
      SI_TAB_PAPER_BASEMENT,
      SI_TAB_PAPER_LAST = SI_TAB_PAPER_BASEMENT,
      SI_TAB_NLINE_FIRST = 4000,
      SI_TAB_NLINE_MSA1 = SI_TAB_NLINE_FIRST,
      SI_TAB_NLINE_RTB1,
      SI_TAB_NLINE_RTB2,
      SI_TAB_NLINE_ROOF,
      SI_TAB_NLINE_BASEMENT,
      SI_TAB_NLINE_LAST = SI_TAB_NLINE_BASEMENT,
      SI_TAB_COLUMN_FIRST = 5000,
      SI_TAB_COLUMN_MSG1 = SI_TAB_COLUMN_FIRST,
      SI_TAB_COLUMN_MSG2,
      SI_TAB_COLUMN_MSA1,
      SI_TAB_COLUMN_RTB1,
      SI_TAB_COLUMN_RTB2,
      SI_TAB_COLUMN_ROOF,
      SI_TAB_COLUMN_BASEMENT,
      SI_TAB_COLUMN_LAST = SI_TAB_COLUMN_BASEMENT,
      SI_MOB_NLINE_FIRST = 6000,
      SI_MOB_NLINE_MSA1 = SI_MOB_NLINE_FIRST,
      SI_MOB_NLINE_ROOF,
      SI_MOB_NLINE_BASEMENT,
      SI_MOB_NLINE_LAST = SI_MOB_NLINE_BASEMENT,
      SI_MOB_COLUMN_FIRST = 7000,
      SI_MOB_COLUMN_MSG1 = SI_MOB_COLUMN_FIRST,
      SI_MOB_COLUMN_MSG2,
      SI_MOB_COLUMN_MSA1,
      SI_MOB_COLUMN_ROOF,
      SI_MOB_COLUMN_BASEMENT,
      SI_MOB_COLUMN_LAST = SI_MOB_COLUMN_BASEMENT,
      SI_DESK_MESSAGE_FIRST = 8000,
      SI_DESK_MESSAGE_IMG = SI_DESK_MESSAGE_FIRST,
      SI_DESK_MESSAGE_MSG1,
      SI_DESK_MESSAGE_MSG2,
      SI_DESK_MESSAGE_RTB1,
      SI_DESK_MESSAGE_RTB2,
      SI_DESK_MESSAGE_ROOF,
      SI_DESK_MESSAGE_BASEMENT,
      SI_DESK_MESSAGE_LAST = SI_DESK_MESSAGE_BASEMENT,
      SI_TAB_MESSAGE_FIRST = 9000,
      SI_TAB_MESSAGE_IMG = SI_TAB_MESSAGE_FIRST,
      SI_TAB_MESSAGE_MSG1,
      SI_TAB_MESSAGE_MSG2,
      SI_TAB_MESSAGE_RTB1,
      SI_TAB_MESSAGE_RTB2,
      SI_TAB_MESSAGE_ROOF,
      SI_TAB_MESSAGE_BASEMENT,
      SI_TAB_MESSAGE_LAST = SI_TAB_MESSAGE_BASEMENT,
      SI_MOB_MESSAGE_FIRST = 10000,
      SI_MOB_MESSAGE_IMG = SI_MOB_MESSAGE_FIRST,
      SI_MOB_MESSAGE_MSG1,
      SI_MOB_MESSAGE_MSG2,
      SI_MOB_MESSAGE_ROOF,
      SI_MOB_MESSAGE_BASEMENT,
      SI_MOB_MESSAGE_LAST = SI_MOB_MESSAGE_BASEMENT
    };

    struct GroupCap
    {
      uint32_t count;
      uint64_t time;
      
      GroupCap() throw() : count(0), time(0) {}
      GroupCap(uint32_t c, uint64_t t) throw() : count(c), time(t) {}
    };

    struct GroupCapMap:
      public google::dense_hash_map<GroupId,
                                    GroupCap,
                                    El::Hash::Numeric<GroupId> >
    {
      GroupCapMap() throw() { set_empty_key(0); set_deleted_key(UINT64_MAX); }
    };    
    
    struct GroupCaps
    {
      GroupCapMap caps;
      uint64_t current_time;

      GroupCaps() throw() : current_time(0) {}
      void set_current_time() throw();

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);

      void from_string(const char* str) throw(El::Exception);
      void to_string(std::string& str) const throw(El::Exception);

      void swap(GroupCaps& val) throw();
    };

    struct SelectionContext;
    
    struct Condition
    {
      ConditionId id;
      uint8_t rnd_mod;
      uint8_t rnd_mod_from;
      uint8_t rnd_mod_to;
      uint32_t group_freq_cap;
      uint32_t group_count_cap;
      uint64_t query_types;
      uint64_t query_type_exclusions;
      SourceArray message_sources;
      SourceArray message_source_exclusions;
      SourceArray page_sources;
      SourceArray page_source_exclusions;
      CategoryArray message_categories;
      CategoryArray message_category_exclusions;
      CategoryArray page_categories;
      CategoryArray page_category_exclusions;
      SearchEngineSet search_engines;
      SearchEngineSet search_engine_exclusions;
      CrawlerSet crawlers;
      CrawlerSet crawler_exclusions;
      LangSet languages;
      LangSet language_exclusions;
      CountrySet countries;
      CountrySet country_exclusions;
      IpMaskArray ip_masks;
      IpMaskArray ip_mask_exclusions;
      TagArray tags;
      TagArray tag_exclusions;
      RefererArray referers;
      RefererArray referer_exclusions;
      LangArray content_languages;
      LangArray content_language_exclusions;

      static std::string NONE;
      static std::string ANY;
      static El::Lang NONE_LANG;
      static El::Lang ANY_LANG;

      Condition() throw();
      
      Condition(ConditionId id_val,
                uint8_t rnd_mod_val,
                uint8_t rnd_mod_from_val,
                uint8_t rnd_mod_to_val,
                uint32_t group_freq_cap_val,
                uint32_t group_count_cap_val) throw();

      bool match(const SelectionContext& context,
                 GroupId group,
                 bool ad) const
        throw();

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);

      void add_message_source(const char* src) throw(El::Exception);
      
      void add_message_source_exclusion(const char* src)
        throw(El::Exception);
      
      void add_page_source(const char* src) throw(El::Exception);
      void add_page_source_exclusion(const char* src) throw(El::Exception);
      
      void add_referer(const char* ref) throw(El::Exception);
      void add_referer_exclusion(const char* ref) throw(El::Exception);
      
      static void add_url(const char* url, StringArray& urls)
        throw(El::Exception);

      void add_message_category(const char* cat) throw(El::Exception);
      
      void add_message_category_exclusion(const char* cat)
        throw(El::Exception);
      
      void add_page_category(const char* cat) throw(El::Exception);
      void add_page_category_exclusion(const char* cat) throw(El::Exception);
      
      void add_tag(const char* tag) throw(El::Exception);
      void add_tag_exclusion(const char* tag) throw(El::Exception);
      
      static void add_string(const char* str, StringArray& strings)
        throw(El::Exception);
    };

    typedef std::vector<Condition> ConditionArray;
    typedef std::vector<const Condition*> ConditionPtrArray;
    
    typedef __gnu_cxx::hash_map<ConditionId,
                                Condition,
                                El::Hash::Numeric<ConditionId> >
    ConditionMap;    
    
    enum CreativeInjection
    {
      CI_DIRECT,
      CI_FRAME,
      CI_COUNT
    };

    class Selector;

    struct Counter
    {
      CounterId id;
      GroupId group;
      uint8_t group_cap;
      uint64_t group_cap_min_time;
      AdvertiserId advertiser;
      std::string text;
      ConditionPtrArray conditions;

      Counter() throw();
      
      Counter(CounterId id_val,
              GroupId group_val,
              uint64_t group_cap_min_time_val,
              AdvertiserId advertiser_val,
              const char* text_val) throw();

      bool match(const SelectionContext& context) const throw();
      
      void finalize(Selector& selector) throw(El::Exception);
      
      void dump(std::ostream& ostr, const char* ident = "") const
        throw(El::Exception);

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      
      void read(El::BinaryInStream& bstr,
                const ConditionMap& condition_map) throw(El::Exception);

      void add_condition(const Condition* cond) throw(El::Exception);
    };

    typedef std::vector<Counter> CounterArray;

    struct Creative
    {
      CreativeId id;
      GroupId group;
      uint8_t group_cap;
      uint64_t group_cap_min_time;
      uint32_t width;
      uint32_t height;
      double weight;
      AdvertiserId advertiser;
      std::string text;
      ConditionPtrArray conditions;
      CreativeInjection inject;

      Creative() throw();
      
      Creative(CreativeId id_val,
               GroupId group_val,
               uint64_t group_cap_min_time_val,
               uint32_t width_val,
               uint32_t height_val,
               double weight_val,
               AdvertiserId advertiser_val,
               const char* text_val,
               CreativeInjection inject_val) throw();

      bool match(const SelectionContext& context) const throw();
      
      void finalize(Selector& selector) throw(El::Exception);
      
      void dump(std::ostream& ostr, const char* ident = "") const
        throw(El::Exception);

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      
      void read(El::BinaryInStream& bstr,
                const ConditionMap& condition_map) throw(El::Exception);

      void add_condition(const Condition* cond) throw(El::Exception);
    };

    typedef std::vector<Creative> CreativeArray;

    struct CreativeWeight
    {
      const Creative* creative;
      double weight;
      
      CreativeWeight() : creative(0), weight(0) {}
      CreativeWeight(const Creative* c, double w) : creative(c), weight(w) {}

      bool operator<(const CreativeWeight& v) const throw();
    };  
    
    typedef std::vector<CreativeWeight> CreativeWeightArray;
    
    struct SelectionContext
    {
      PageId page;
      SlotIdArray slots;
      SourceSet page_sources;
      SourceSet message_sources;
      CategorySet page_categories;
      CategorySet message_categories;
      SearchEngine search_engine;
      Crawler crawler;
      uint32_t rnd;
      El::Lang language;
      El::Country country;
      LangSet content_languages;
      uint32_t ip;
      GroupCaps ad_caps;
      GroupCaps counter_caps;
      TagSet tags;
      RefererSet referers;
      uint64_t query_types;

      SelectionContext() throw();
      SelectionContext(PageId page_val, uint32_t rnd_val = 0) throw();      

      void rand() throw();
      uint32_t rnd_mod(uint32_t mod) const throw();

      void add_message_category(const char* category) throw(El::Exception);
      void add_page_category(const char* category) throw(El::Exception);
      
      static void add_category(const char* category, CategorySet& categories)
        throw(El::Exception);
      
      void add_message_source(const char* source) throw(El::Exception);
      void add_page_source(const char* source) throw(El::Exception);

      void add_tag(const char* tag) throw(El::Exception);
      void add_content_lang(const El::Lang& lang) throw(El::Exception);
      void set_referer(const char* referer) throw(El::Exception);
        
      static void add_url(const char* url, StringSet& urls)
        throw(El::Exception);
      
      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };

    struct Slot
    {
      CreativeArray creatives;

      Slot() throw() {}
      
      void finalize(Selector& selector) throw(El::Exception);

      void dump(std::ostream& ostr, const char* ident = "") const
        throw(El::Exception);

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      
      void read(El::BinaryInStream& bstr,
                const ConditionMap& conditions) throw(El::Exception);      
    };

    typedef __gnu_cxx::hash_map<SlotId, Slot, El::Hash::Numeric<SlotId> >
    SlotMap;

    struct AdvMaxAdNum
    {
      AdvertiserId advertiser;
      uint32_t max_ad_num;

      AdvMaxAdNum() throw() : advertiser(0), max_ad_num(0) {}
      
      AdvMaxAdNum(AdvertiserId advertiser_val, uint32_t max_ad_num_val)
        throw();      

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };

    typedef std::vector<AdvMaxAdNum> AdvMaxAdNumArray;

    struct AdvRestriction
    {
      uint32_t max_ad_num;
      AdvMaxAdNumArray adv_max_ad_nums;
      
      AdvRestriction() throw() : max_ad_num(0) {}
      AdvRestriction(uint32_t max_ad_num_val) throw();

      void dump(std::ostream& ostr, const char* ident = "") const
        throw(El::Exception);

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };

    typedef __gnu_cxx::hash_map<AdvertiserId,
                                AdvRestriction,
                                El::Hash::Numeric<AdvertiserId> >
    AdvRestrictionMap;

    struct Page
    {
      SlotMap slots;
      CounterArray counters;
      AdvRestrictionMap adv_restrictions;
      uint32_t max_ad_num;

      Page(uint32_t max_ad_num_val) throw() : max_ad_num(max_ad_num_val) {}
      Page() throw() : max_ad_num(0) {}

      void finalize(Selector& selector) throw(El::Exception);

      void add_max_ad_num(AdvertiserId advertiser1,
                          AdvertiserId advertiser2,
                          uint32_t max_ad_num)
        throw(El::Exception);

      void add_max_ad_num(AdvertiserId advertiser, uint32_t max_ad_num)
        throw(El::Exception);

      void dump(std::ostream& ostr, const char* ident = "") const
        throw(El::Exception);

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      
      void read(El::BinaryInStream& bstr,
                const ConditionMap& conditions) throw(El::Exception);

    private:
      
      void add_max_ad_num_asym(AdvertiserId advertiser1,
                               AdvertiserId advertiser2,
                               uint32_t max_ad_num)
        throw(El::Exception);
    };
    
    typedef __gnu_cxx::hash_map<PageId, Page, El::Hash::Numeric<PageId> >
    PageMap;

    struct Selection
    {
      CreativeId id;
      SlotId slot;
      uint32_t width;
      uint32_t height;
      std::string text;
      CreativeInjection inject;

      Selection() throw() :id(0),slot(0),width(0),height(0),inject(CI_DIRECT){}
      
      Selection(CreativeId id_val,
                SlotId slot_val,
                uint32_t width_val,
                uint32_t height_val,
                const char* text_val,
                CreativeInjection inject_val) throw();

      void dump(std::ostream& ostr) const throw(El::Exception);

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);
      
      bool operator==(const Selection& val) const throw();
    };

    typedef std::vector<Selection> SelectionArray;
    
    struct SelectedCounter
    {
      CounterId id;
      std::string text;

      SelectedCounter() throw() : id(0) {}
      
      SelectedCounter(CounterId id_val,
                      const char* text_val) throw();

      void dump(std::ostream& ostr) const throw(El::Exception);

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);
      
      bool operator==(const SelectedCounter& val) const throw();
    };

    typedef std::vector<SelectedCounter> SelectedCounterArray;
    
    struct SelectionResult
    {
      SelectionArray ads;
      SelectedCounterArray counters;
      GroupCaps ad_caps;
      GroupCaps counter_caps;

      void dump(std::ostream& ostr) const throw(El::Exception);

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };
    
    class Selector
    {
    public:

      struct CreativeWeightStrategy
      {
        enum StrategyType
        {
          ST_NONE,
          ST_PROBABILISTIC
        };

        virtual ~CreativeWeightStrategy() throw() {}
        virtual StrategyType type() const throw() = 0;
        virtual CreativeWeightStrategy* clone() const throw() = 0;

        virtual void write(El::BinaryOutStream& bstr) const
          throw(El::Exception) = 0;
        
        virtual void read(El::BinaryInStream& bstr) throw(El::Exception) = 0;

        virtual void set_weights(CreativeWeightArray& creative_weights)
          throw(El::Exception) = 0;
        
        static CreativeWeightStrategy* create(StrategyType type)
          throw(Exception, El::Exception);
      };

      typedef std::auto_ptr<CreativeWeightStrategy> CreativeWeightStrategyPtr;

      struct NoneCreativeWeightStrategy : CreativeWeightStrategy
      {
        virtual ~NoneCreativeWeightStrategy() throw() {}
        virtual StrategyType type() const throw() { return ST_NONE; }
        virtual CreativeWeightStrategy* clone() const throw();
        
        virtual void set_weights(CreativeWeightArray& creative_weights)
          throw(El::Exception);

        virtual void write(El::BinaryOutStream& bstr) const
          throw(El::Exception) {}
        
        virtual void read(El::BinaryInStream& bstr) throw(El::Exception) {}
      };
      
      struct ProbabilisticCreativeWeightStrategy : CreativeWeightStrategy
      {
        float reduction_rate;
        uint32_t weight_zones;
        
        ProbabilisticCreativeWeightStrategy() throw();
        ProbabilisticCreativeWeightStrategy(float reduct_rate,
                                            uint32_t wght_zones) throw();
        
        ProbabilisticCreativeWeightStrategy(
          const ProbabilisticCreativeWeightStrategy& src) throw();
        
        virtual ~ProbabilisticCreativeWeightStrategy() throw() {}
        virtual StrategyType type() const throw() { return ST_PROBABILISTIC; }
        virtual CreativeWeightStrategy* clone() const throw();

        virtual void set_weights(CreativeWeightArray& creative_weights)
          throw(El::Exception);

        virtual void write(El::BinaryOutStream& bstr) const
          throw(El::Exception);
        
        virtual void read(El::BinaryInStream& bstr) throw(El::Exception);

      private:

        struct CWG
        {
          double total_weight;
          double cumulative_weight;
          CreativeWeightArray creative_weights;

          CWG() throw();

          void multiply(double factor) throw();
        };

        typedef std::vector<CWG> CWGArray;

        void dump(const CWGArray& cwga) const throw();

        void downgrade_weights(CWGArray::reverse_iterator from,
                               CWGArray::reverse_iterator to) const
          throw(El::Exception);

        CreativeWeight random_take(CWGArray& cwga) const throw(El::Exception);
      };      
      
    public:

      uint64_t group_cap_timeout;
      uint64_t group_cap_max_count;
      uint64_t counter_cap_timeout;
      uint64_t counter_cap_max_count;
      SelectorUpdateNumber update_number;
      
      ConditionMap conditions;
      CapMinTimeMap capped_groups_min_times;
      
      PageMap pages;
      CreativeWeightStrategyPtr creative_weight_strategy;

      Selector(uint64_t group_cap_tm,
               uint64_t group_cap_max_cnt,
               uint64_t counter_cap_tm,
               uint64_t counter_cap_max_cnt,
               Ad::SelectorUpdateNumber update_num) throw(El::Exception);

      Selector() throw(El::Exception);

      Selector(const Selector& src) throw(El::Exception);
      Selector& operator=(const Selector& src) throw(El::Exception);

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);

      void dump(std::ostream& ostr, const char* ident = "") const
        throw(El::Exception);
      
    public:

      void add_creative(PageIdValue page,
                        SlotIdValue slot,
                        const Creative& creative)
        throw(El::Exception);

      void add_counter(PageIdValue page, const Counter& counter)
        throw(El::Exception);

      void finalize() throw(El::Exception);
        
      void select(SelectionContext& context, SelectionResult& result)
        throw(Exception, El::Exception);

    private:
      
      void add_page(PageIdValue page_id,
                    SlotIdValue slot_first,
                    SlotIdValue slot_last)
        throw(El::Exception);

      bool select_ads(const Page& page,
                      const SelectionContext& context,
                      SelectionResult& result)
        throw(Exception, El::Exception);
      
      bool select_counters(const Page& page,
                           const SelectionContext& context,
                           SelectionResult& result)
        throw(Exception, El::Exception);

    void cleanup_caps(GroupCaps& caps) const throw(El::Exception);
      
    private:

      struct SelectedSlot
      {
        SlotId id;
        CreativeWeightArray creatives;

        // Add to speedup iterating
        // creatives_begin, creatives_end

        SelectedSlot() throw() : id(0) {}
        SelectedSlot(SlotId id_val) throw() : id(id_val) {}        
      };

      typedef std::vector<SelectedSlot> SelectedSlotArray;

      struct SelectedPlacement
      {
        SlotId id;
        const Creative* creative;

        SelectedPlacement() throw() : id(0), creative(0) {}
        SelectedPlacement(SlotId id_val, const Creative* creative_val) throw();
      };
      
      typedef std::stack<SelectedPlacement> SelectedPlacementStack;

      struct AdvCountMap:
        public google::dense_hash_map<AdvertiserId,
                                      uint32_t,
                                      El::Hash::Numeric<AdvertiserId> >
      {
        AdvCountMap() throw() { set_empty_key(0); }
      };

      struct SelTravContext
      {
        SelectedPlacementStack best;
        double best_weight;
        
        SelectedPlacementStack current;
        double current_weight;

        AdvCountMap adv_counter;
        GroupIdSet group_capped;

        SelTravContext() throw() : best_weight(0), current_weight(0) {}
      };

      void select_best_placement(SelectedSlotArray::const_iterator slot_b,
                                 SelectedSlotArray::const_iterator slot_e,
                                 const AdvRestrictionMap& adv_restrictions,
                                 uint32_t level,
                                 SelTravContext& context) const
        throw(El::Exception);
    };

    void normalize_url(const char* url, std::string& normalized)
      throw(El::Exception);
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Ad
  {
    //
    // AdvRestriction struct
    //
    inline
    AdvRestriction::AdvRestriction(uint32_t max_ad_num_val) throw()
        : max_ad_num(max_ad_num_val)
    {
    }

    inline
    void
    AdvRestriction::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << max_ad_num;
      bstr.write_array(adv_max_ad_nums);
    }

    inline
    void
    AdvRestriction::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> max_ad_num;
      bstr.read_array(adv_max_ad_nums);
    }

    //
    // AdvRestriction struct
    //
    inline
    AdvMaxAdNum::AdvMaxAdNum(AdvertiserId advertiser_val,
                             uint32_t max_ad_num_val)
      throw()
        : advertiser(advertiser_val),
          max_ad_num(max_ad_num_val)
    {
    }
    
    inline
    void
    AdvMaxAdNum::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << advertiser << max_ad_num;
    }
    
    inline
    void
    AdvMaxAdNum::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> advertiser >> max_ad_num;
    }
    
    //
    // Selection struct
    //

    inline
    Condition::Condition() throw()
        : id(0),
          rnd_mod(0),
          rnd_mod_from(0),
          rnd_mod_to(0),
          group_freq_cap(0),
          group_count_cap(0),
          query_types(0),
          query_type_exclusions(0)
    {
    }
    
    inline
    Condition::Condition(ConditionId id_val,
                         uint8_t rnd_mod_val,
                         uint8_t rnd_mod_from_val,
                         uint8_t rnd_mod_to_val,
                         uint32_t group_freq_cap_val,
                         uint32_t group_count_cap_val) throw()
        : id(id_val),
          rnd_mod(rnd_mod_val),
          rnd_mod_from(rnd_mod_from_val),
          rnd_mod_to(rnd_mod_to_val),
          group_freq_cap(group_freq_cap_val),
          group_count_cap(group_count_cap_val),
          query_types(0),
          query_type_exclusions(0)
    {
    }

    inline
    void
    Condition::add_message_source(const char* src) throw(El::Exception)
    {
      add_url(src, message_sources);
    }
      
    inline
    void
    Condition::add_message_source_exclusion(const char* src)
      throw(El::Exception)
    {
      add_url(src, message_source_exclusions);
    }
      
    inline
    void
    Condition::add_page_source(const char* src) throw(El::Exception)
    {
      add_url(src, page_sources);
    }
    
    inline
    void
    Condition::add_page_source_exclusion(const char* src)
      throw(El::Exception)
    {
      add_url(src, page_source_exclusions);
    }
    
    inline
    void
    Condition::add_referer(const char* ref) throw(El::Exception)
    {
      add_url(ref, referers);
    }
    
    inline
    void
    Condition::add_referer_exclusion(const char* ref) throw(El::Exception)
    {
      add_url(ref, referer_exclusions);
    }
    
    inline
    void
    Condition::add_url(const char* url, StringArray& urls)
      throw(El::Exception)
    {
      if(strcasecmp(url, "[any]") == 0 || strcasecmp(url, "[none]") == 0)
      {
        std::string lowered;
        El::String::Manip::to_lower(url, lowered);
        urls.push_back(lowered);
        return;
      }
      
      try
      {
        std::string norm;
        normalize_url(url, norm);
        urls.push_back(norm);
      }
      catch(...)
      {
      }
    }

    inline
    void
    Condition::add_message_category(const char* cat) throw(El::Exception)
    {
      add_string(cat, message_categories);
    }
      
    inline
    void
    Condition::add_message_category_exclusion(const char* cat)
      throw(El::Exception)
    {
      add_string(cat, message_category_exclusions);
    }
      
    inline
    void
    Condition::add_page_category(const char* cat) throw(El::Exception)
    {
      add_string(cat, page_categories);
    }
    
    inline
    void
    Condition::add_page_category_exclusion(const char* cat)
      throw(El::Exception)
    {
      add_string(cat, page_category_exclusions);
    }
      
    inline
    void
    Condition::add_tag(const char* tag) throw(El::Exception)
    {
      add_string(tag, tags);
    }
    
    inline
    void
    Condition::add_tag_exclusion(const char* tag) throw(El::Exception)
    {
      add_string(tag, tag_exclusions);
    }
      
    inline
    void
    Condition::add_string(const char* str, StringArray& strings)
      throw(El::Exception)
    {
      std::string lowered;
      El::String::Manip::to_lower(str, lowered);
      strings.push_back(lowered);
    }

    inline
    void
    Condition::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << id << rnd_mod << rnd_mod_from << rnd_mod_to << query_types
           << query_type_exclusions << group_freq_cap << group_count_cap;

      bstr.write_array(message_sources);
      bstr.write_array(message_source_exclusions);
      bstr.write_array(page_sources);
      bstr.write_array(page_source_exclusions);
      bstr.write_array(message_categories);
      bstr.write_array(message_category_exclusions);
      bstr.write_array(page_categories);
      bstr.write_array(page_category_exclusions);
      bstr.write_set(search_engines);
      bstr.write_set(search_engine_exclusions);
      bstr.write_set(crawlers);
      bstr.write_set(crawler_exclusions);
      bstr.write_set(languages);
      bstr.write_set(language_exclusions);
      bstr.write_set(countries);
      bstr.write_set(country_exclusions);
      bstr.write_array(ip_masks);
      bstr.write_array(ip_mask_exclusions);
      bstr.write_array(tags);
      bstr.write_array(tag_exclusions);
      bstr.write_array(referers);
      bstr.write_array(referer_exclusions);
      bstr.write_array(content_languages);
      bstr.write_array(content_language_exclusions);
    }
    
    inline
    void
    Condition::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> id >> rnd_mod >> rnd_mod_from >> rnd_mod_to >> query_types
           >> query_type_exclusions >> group_freq_cap >> group_count_cap;

      bstr.read_array(message_sources);
      bstr.read_array(message_source_exclusions);
      bstr.read_array(page_sources);
      bstr.read_array(page_source_exclusions);
      bstr.read_array(message_categories);
      bstr.read_array(message_category_exclusions);
      bstr.read_array(page_categories);
      bstr.read_array(page_category_exclusions);
      bstr.read_set(search_engines);
      bstr.read_set(search_engine_exclusions);
      bstr.read_set(crawlers);
      bstr.read_set(crawler_exclusions);
      bstr.read_set(languages);
      bstr.read_set(language_exclusions);
      bstr.read_set(countries);
      bstr.read_set(country_exclusions);
      bstr.read_array(ip_masks);
      bstr.read_array(ip_mask_exclusions);
      bstr.read_array(tags);
      bstr.read_array(tag_exclusions);
      bstr.read_array(referers);
      bstr.read_array(referer_exclusions);
      bstr.read_array(content_languages);
      bstr.read_array(content_language_exclusions);
    }

    inline
    bool
    Condition::match(const SelectionContext& context, GroupId group, bool ad)
      const throw()
    {
      if(rnd_mod)
      {
        uint8_t val = context.rnd_mod(rnd_mod);

        if(val < rnd_mod_from || val > rnd_mod_to)
        {
          return false;
        }
      }

      if(group_freq_cap || group_count_cap)
      {
        const GroupCaps& cgc = ad ? context.ad_caps : context.counter_caps;
          
        const GroupCapMap& caps = cgc.caps;
        GroupCapMap::const_iterator i = caps.find(group);

        if(i != caps.end())
        {
          const GroupCap& cap = i->second;
          
          if((group_freq_cap &&
              cgc.current_time < cap.time + group_freq_cap) ||
             (group_count_cap && cap.count >= group_count_cap))
          {
            return false;
          }
        }
      }

      if(!crawlers.empty())
      {
        if(context.crawler.empty())
        {
          if(crawlers.find(NONE) == crawlers.end())
          {
            return false;
          }
        }
        else
        {
          if(crawlers.find(context.crawler) == crawlers.end() &&
             crawlers.find(ANY) == crawlers.end())
          {
            return false;
          }
        }
      }

      if(!crawler_exclusions.empty())
      {
        if(context.crawler.empty())
        {
          if(crawler_exclusions.find(NONE) != crawler_exclusions.end())
          {
            return false;
          }
        }
        else
        {
          if(crawler_exclusions.find(context.crawler) !=
             crawler_exclusions.end() ||
             crawler_exclusions.find(ANY) != crawler_exclusions.end())
          {
            return false;
          }
        }
      }

      if(!languages.empty())
      {
        if(languages.find(context.language) == languages.end())
        {
          return false;
        }
      }

      if(context.language != El::Lang::null)
      {
        if(language_exclusions.find(context.language) !=
           language_exclusions.end())
        {
          return false;
        }
      }

      if(!tags.empty())
      {
        if(context.tags.empty())
        {
          TagArray::const_iterator i(tags.begin());
          TagArray::const_iterator e(tags.end());
          
          for(; i != e && *i != NONE; ++i);
          
          if(i == e)
          {
            return false;
          }
        }
        else
        {
          TagArray::const_iterator i(tags.begin());
          TagArray::const_iterator e(tags.end());
          bool any = false;
          
          for(; i != e && context.tags.find(*i) ==
                context.tags.end(); ++i)
          {
            if(*i == ANY)
            {
              any = true;
            }
          }
          
          if(i == e && !any)
          {
            return false;
          }
        }
      }

      if(!tag_exclusions.empty())
      {
        if(context.tags.empty())
        {
          for(CategoryArray::const_iterator
                i(tag_exclusions.begin()),
                e(tag_exclusions.end()); i != e; ++i)
          {
            if(*i == NONE)
            {
              return false;
            }
          }
        }
        else
        {
          for(CategoryArray::const_iterator
                i(tag_exclusions.begin()),
                e(tag_exclusions.end()); i != e; ++i)
          {        
            if(context.tags.find(*i) !=
               context.tags.end() || *i == ANY)
            {
              return false;
            }
          }
        } 
      }
      
      if(!content_languages.empty())
      {
        if(context.content_languages.empty())
        {
          LangArray::const_iterator i(content_languages.begin());
          LangArray::const_iterator e(content_languages.end());
          
          for(; i != e && *i != NONE_LANG; ++i);
          
          if(i == e)
          {
            return false;
          }
        }
        else
        {
          LangArray::const_iterator i(content_languages.begin());
          LangArray::const_iterator e(content_languages.end());
          bool any = false;
          
          for(; i != e && context.content_languages.find(*i) ==
                context.content_languages.end(); ++i)
          {
            if(*i == ANY_LANG)
            {
              any = true;
            }
          }
          
          if(i == e && !any)
          {
            return false;
          }
        }
      }

      if(!content_language_exclusions.empty())
      {
        if(context.content_languages.empty())
        {
          for(LangArray::const_iterator
                i(content_language_exclusions.begin()),
                e(content_language_exclusions.end()); i != e; ++i)
          {
            if(*i == NONE_LANG)
            {
              return false;
            }
          }
        }
        else
        {
          for(LangArray::const_iterator
                i(content_language_exclusions.begin()),
                e(content_language_exclusions.end()); i != e; ++i)
          {        
            if(context.content_languages.find(*i) !=
               context.content_languages.end() || *i == ANY_LANG)
            {
              return false;
            }
          }
        } 
      }

      if(query_types)
      {
        if(!context.query_types)
        {
          if((query_types & QT_NONE) == 0)
          {
            return false;
          }
        }
        else if((query_types & context.query_types) == 0 &&
                (query_types & QT_ANY) == 0)
        {
          return false;
        }
      }

      if(query_type_exclusions)
      {
        if(!context.query_types)
        {
          if(query_type_exclusions & QT_NONE)
          {
            return false;
          }
        }
        else if((query_type_exclusions & context.query_types) ||
                (query_type_exclusions & QT_ANY))
        {
          return false;
        } 
      }

      if(!referers.empty())
      {        
        if(context.referers.empty())
        {
          CategoryArray::const_iterator i(referers.begin());
          CategoryArray::const_iterator e(referers.end());
          
          for(; i != e && *i != NONE; ++i);
          
          if(i == e)
          {
            return false;
          }
        }
        else
        {
          CategoryArray::const_iterator i(referers.begin());
          CategoryArray::const_iterator e(referers.end());
          bool any = false;
          
          for(; i != e && context.referers.find(*i) ==
                context.referers.end(); ++i)
          {
            if(*i == ANY)
            {
              any = true;
            }
          }
          
          if(i == e && !any)
          {
            return false;
          }
        }
      }

      if(!referer_exclusions.empty())
      {          
        if(context.referers.empty())
        {
          for(CategoryArray::const_iterator
                i(referer_exclusions.begin()),
                e(referer_exclusions.end()); i != e; ++i)
          {
            if(*i == NONE)
            {
              return false;
            }
          }
        }
        else
        {
          for(CategoryArray::const_iterator
                i(referer_exclusions.begin()),
                e(referer_exclusions.end()); i != e; ++i)
          {        
            if(context.referers.find(*i) !=
               context.referers.end() || *i == ANY)
            {
              return false;
            }
          }
        } 
      }
      
      if(!page_sources.empty())
      {        
        if(context.page_sources.empty())
        {
          SourceArray::const_iterator i(page_sources.begin());
          SourceArray::const_iterator e(page_sources.end());
          
          for(; i != e && *i != NONE; ++i);
          
          if(i == e)
          {
            return false;
          }
        }
        else
        {
          SourceArray::const_iterator i(page_sources.begin());
          SourceArray::const_iterator e(page_sources.end());
          bool any = false;
          
          for(; i != e && context.page_sources.find(*i) ==
                context.page_sources.end(); ++i)
          {
            if(*i == ANY)
            {
              any = true;
            }
          }
          
          if(i == e && !any)
          {
            return false;
          }
        }
      }

      if(!page_source_exclusions.empty())
      {
        if(context.page_sources.empty())
        {
          for(SourceArray::const_iterator
                i(page_source_exclusions.begin()),
                e(page_source_exclusions.end()); i != e; ++i)
          {
            if(*i == NONE)
            {
              return false;
            }
          }
        }
        else
        {
          for(SourceArray::const_iterator
                i(page_source_exclusions.begin()),
                e(page_source_exclusions.end()); i != e; ++i)
          {        
            if(context.page_sources.find(*i) !=
               context.page_sources.end() || *i == ANY)
            {
              return false;
            }
          }
        } 
      }
 
      if(!message_sources.empty())
      {        
        if(context.message_sources.empty())
        {
          SourceArray::const_iterator i(message_sources.begin());
          SourceArray::const_iterator e(message_sources.end());
          
          for(; i != e && *i != NONE; ++i);
          
          if(i == e)
          {
            return false;
          }
        }
        else
        {
          SourceArray::const_iterator i(message_sources.begin());
          SourceArray::const_iterator e(message_sources.end());
          bool any = false;
          
          for(; i != e && context.message_sources.find(*i) ==
                context.message_sources.end(); ++i)
          {
            if(*i == ANY)
            {
              any = true;
            }
          }
          
          if(i == e && !any)
          {
            return false;
          }
        }
      }

      if(!message_source_exclusions.empty())
      {
        if(context.message_sources.empty())
        {
          for(SourceArray::const_iterator
                i(message_source_exclusions.begin()),
                e(message_source_exclusions.end()); i != e; ++i)
          {
            if(*i == NONE)
            {
              return false;
            }
          }
        }
        else
        {
          for(SourceArray::const_iterator
                i(message_source_exclusions.begin()),
                e(message_source_exclusions.end()); i != e; ++i)
          {        
            if(context.message_sources.find(*i) !=
               context.message_sources.end() || *i == ANY)
            {
              return false;
            }
          }
        }
      }
 
      if(!page_categories.empty())
      {        
        if(context.page_categories.empty())
        {
          CategoryArray::const_iterator i(page_categories.begin());
          CategoryArray::const_iterator e(page_categories.end());
          
          for(; i != e && *i != NONE; ++i);
          
          if(i == e)
          {
            return false;
          }
        }
        else
        {
          CategoryArray::const_iterator i(page_categories.begin());
          CategoryArray::const_iterator e(page_categories.end());
          bool any = false;
          
          for(; i != e && context.page_categories.find(*i) ==
                context.page_categories.end(); ++i)
          {
            if(*i == ANY)
            {
              any = true;
            }
          }
          
          if(i == e && !any)
          {
            return false;
          }
        }
      }

      if(!page_category_exclusions.empty())
      {
        if(context.page_categories.empty())
        {
          for(CategoryArray::const_iterator
                i(page_category_exclusions.begin()),
                e(page_category_exclusions.end()); i != e; ++i)
          {
            if(*i == NONE)
            {
              return false;
            }
          }
        }
        else
        {
          for(CategoryArray::const_iterator
                i(page_category_exclusions.begin()),
                e(page_category_exclusions.end()); i != e; ++i)
          {        
            if(context.page_categories.find(*i) !=
               context.page_categories.end() || *i == ANY)
            {
              return false;
            }
          }
        } 
      }
 
      if(!message_categories.empty())
      {        
        if(context.message_categories.empty())
        {
          CategoryArray::const_iterator i(message_categories.begin());
          CategoryArray::const_iterator e(message_categories.end());
          
          for(; i != e && *i != NONE; ++i);
          
          if(i == e)
          {
            return false;
          }
        }
        else
        {
          CategoryArray::const_iterator i(message_categories.begin());
          CategoryArray::const_iterator e(message_categories.end());
          bool any = false;
          
          for(; i != e && context.message_categories.find(*i) ==
                context.message_categories.end(); ++i)
          {
            if(*i == ANY)
            {
              any = true;
            }
          }
          
          if(i == e && !any)
          {
            return false;
          }
        }
      }

      if(!message_category_exclusions.empty())
      {
        if(context.message_categories.empty())
        {
          for(CategoryArray::const_iterator
                i(message_category_exclusions.begin()),
                e(message_category_exclusions.end()); i != e; ++i)
          {
            if(*i == NONE)
            {
              return false;
            }
          }
        }
        else
        {
          for(CategoryArray::const_iterator
                i(message_category_exclusions.begin()),
                e(message_category_exclusions.end()); i != e; ++i)
          {        
            if(context.message_categories.find(*i) !=
               context.message_categories.end() || *i == ANY)
            {
              return false;
            }
          }
        } 
      }
 
      if(!search_engines.empty())
      {
        if(context.search_engine.empty())
        {
          if(search_engines.find(NONE) == search_engines.end())
          {
            return false;
          }
        }
        else
        {
          if(search_engines.find(context.search_engine) ==
             search_engines.end() &&
             search_engines.find(ANY) == search_engines.end())
          {
            return false;
          }
        }
      }

      if(!search_engine_exclusions.empty())
      {
        if(context.search_engine.empty())
        {
          if(search_engine_exclusions.find(NONE) !=
             search_engine_exclusions.end())
          {
            return false;
          }
        }
        else
        {
          if(search_engine_exclusions.find(context.search_engine) !=
             search_engine_exclusions.end() ||
             search_engine_exclusions.find(ANY) !=
             search_engine_exclusions.end())
          {
            return false;
          }
        }
      }
     
      if(!countries.empty())
      {
        if(countries.find(context.country) == countries.end())
        {
          return false;
        }
      }

      if(context.country != El::Country::null)
      {
        if(country_exclusions.find(context.country) !=
           country_exclusions.end())
        {
          return false;
        }
      }

      if(!ip_masks.empty())
      {
        IpMaskArray::const_iterator i(ip_masks.begin());
        IpMaskArray::const_iterator e(ip_masks.end());
        
        for(; i != e && !i->match(context.ip); ++i);

        if(i == e)
        {
          return false;
        }
      }
      
      if(!ip_mask_exclusions.empty())
      {
        IpMaskArray::const_iterator i(ip_mask_exclusions.begin());
        IpMaskArray::const_iterator e(ip_mask_exclusions.end());
        
        for(; i != e && !i->match(context.ip); ++i);

        if(i != e)
        {
          return false;
        }
      }
            
      return true;
    }    
    
    //
    // Counter struct
    //

    inline
    Counter::Counter() throw()
        : id(0),
          group(0),
          group_cap(0),
          group_cap_min_time(0),
          advertiser(0)
    {
    }
    
    inline
    Counter::Counter(CounterId id_val,
                     GroupId group_val,
                     uint64_t group_cap_min_time_val,
                     AdvertiserId advertiser_val,
                     const char* text_val) throw()
        : id(id_val),
          group(group_val),
          group_cap(0),
          group_cap_min_time(group_cap_min_time_val),
          advertiser(advertiser_val),
          text(text_val)
    {
    }

    inline
    void
    Counter::add_condition(const Condition* cond) throw(El::Exception)
    {
      for(ConditionPtrArray::const_iterator i(conditions.begin()),
            e(conditions.end()); i != e; ++i)
      {
        if((*i)->id == cond->id)
        {
          return;
        }
      }

      conditions.push_back(cond);
    }

    inline
    bool
    Counter::match(const SelectionContext& context) const throw()
    {
      for(ConditionPtrArray::const_iterator i(conditions.begin()),
            e(conditions.end()); i != e; ++i)
      {
        if(!(*i)->match(context, group, false))
        {
          return false;
        }
      }
        
      return true;
    }

    inline
    void
    Counter::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << id << group << group_cap << group_cap_min_time
           << advertiser << text << (uint32_t)conditions.size();

      for(ConditionPtrArray::const_iterator i(conditions.begin()),
            e(conditions.end()); i != e; ++i)
      {
        bstr << (*i)->id;
      }
    }

    inline
    void
    Counter::read(El::BinaryInStream& bstr,
                  const ConditionMap& condition_map) throw(El::Exception)
    {
      uint32_t count = 0;      
      bstr >> id >> group >> group_cap >> group_cap_min_time
           >> advertiser >> text >> count;

      conditions.resize(count);
      
      for(uint32_t i = 0; i < count; ++i)
      {
        ConditionId id = 0;
        bstr >> id;

        ConditionMap::const_iterator ci = condition_map.find(id);

        if(ci == condition_map.end())
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Ad::Counter::read: not found condition " << id;
          throw Exception(ostr.str());
        }

        conditions[i] = &ci->second;
      }
    }

    //
    // Creative struct
    //
    
    inline
    Creative::Creative() throw()
        : id(0),
          group(0),
          group_cap(0),
          group_cap_min_time(0),
          width(0),
          height(0),
          weight(0),
          advertiser(0),
          inject(CI_DIRECT)
    {
    }
    
    inline
    Creative::Creative(CreativeId id_val,
                       GroupId group_val,
                       uint64_t group_cap_min_time_val,
                       uint32_t width_val,
                       uint32_t height_val,
                       double weight_val,
                       AdvertiserId advertiser_val,
                       const char* text_val,
                       CreativeInjection inject_val) throw()
        : id(id_val),
          group(group_val),
          group_cap(0),
          group_cap_min_time(group_cap_min_time_val),
          width(width_val),
          height(height_val),
          weight(weight_val),
          advertiser(advertiser_val),
          text(text_val),
          inject(inject_val)
    {
    }

    inline
    void
    Creative::add_condition(const Condition* cond) throw(El::Exception)
    {
      for(ConditionPtrArray::const_iterator i(conditions.begin()),
            e(conditions.end()); i != e; ++i)
      {
        if((*i)->id == cond->id)
        {
          return;
        }
      }

      conditions.push_back(cond);
    }

    inline
    bool
    Creative::match(const SelectionContext& context) const throw()
    {
      for(ConditionPtrArray::const_iterator i(conditions.begin()),
            e(conditions.end()); i != e; ++i)
      {
        if(!(*i)->match(context, group, true))
        {
          return false;
        }
      }
        
      return true;
    }

    inline
    void
    Creative::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << id << group << group_cap << group_cap_min_time
           << width << height << weight << advertiser << text
           << (uint32_t)inject << (uint32_t)conditions.size();

      for(ConditionPtrArray::const_iterator i(conditions.begin()),
            e(conditions.end()); i != e; ++i)
      {
        bstr << (*i)->id;
      }
    }

    inline
    void
    Creative::read(El::BinaryInStream& bstr,
                   const ConditionMap& condition_map) throw(El::Exception)
    {
      uint32_t count = 0;
      uint32_t inj = 0;
      
      bstr >> id >> group >> group_cap >> group_cap_min_time
           >> width >> height >> weight >> advertiser >> text >> inj >> count;

      inject = (CreativeInjection)inj;

      conditions.resize(count);
      
      for(uint32_t i = 0; i < count; ++i)
      {
        ConditionId id = 0;
        bstr >> id;

        ConditionMap::const_iterator ci = condition_map.find(id);

        if(ci == condition_map.end())
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Ad::Creative::read: not found condition " << id;
          throw Exception(ostr.str());
        }

        conditions[i] = &ci->second;
      }
    }

    //
    // Slot struct
    //
    inline
    void
    Slot::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << (uint32_t)creatives.size();
      
      for(CreativeArray::const_iterator i(creatives.begin()),
            e(creatives.end()); i != e; ++i)
      {
        i->write(bstr);
      }
    }

    inline
    void
    Slot::read(El::BinaryInStream& bstr,
               const ConditionMap& conditions) throw(El::Exception)
    {
      uint32_t count = 0;
      bstr >> count;

      creatives.resize(count);

      for(uint32_t i = 0; i < count; ++i)
      {
        creatives[i].read(bstr, conditions);
      }      
    }

    //
    // Page struct
    //
    inline
    void
    Page::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << max_ad_num;
      bstr.write_map(adv_restrictions);

      bstr << (uint32_t)slots.size();

      for(SlotMap::const_iterator i(slots.begin()), e(slots.end()); i != e;
          ++i)
      {
        bstr << i->first;
        i->second.write(bstr);
      }

      bstr << (uint32_t)counters.size();
      
      for(CounterArray::const_iterator i(counters.begin()),
            e(counters.end()); i != e; ++i)
      {
        i->write(bstr);
      }
    }

    inline
    void
    Page::read(El::BinaryInStream& bstr,
               const ConditionMap& conditions) throw(El::Exception)
    {
      bstr >> max_ad_num;
      bstr.read_map(adv_restrictions);

      uint32_t count = 0;
      bstr >> count;

      slots.clear();

      while(count--)
      {
        SlotId id = 0;
        bstr >> id;

        slots.insert(std::make_pair(id, Slot())).first->second.
          read(bstr, conditions);
      }
      
      bstr >> count;
      counters.resize(count);

      for(uint32_t i = 0; i < count; ++i)
      {
        counters[i].read(bstr, conditions);
      }      
    }

    //
    // FreqCaps struct
    //
    inline
    void
    GroupCaps::set_current_time() throw()
    {
      current_time = ACE_OS::gettimeofday().sec();
    }

    inline
    void
    GroupCaps::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {      
      uint64_t tm = std::max((uint64_t)ACE_OS::gettimeofday().sec(),
                             current_time);
      
      bstr << (uint32_t)1 << current_time << tm << (uint32_t)caps.size();
      
      for(GroupCapMap::const_iterator i(caps.begin()), e(caps.end());
          i != e; ++i)
      {
        const GroupCap& cap = i->second;
        
        bstr << i->first << (uint32_t)(tm > cap.time ? tm - cap.time : 0)
             << cap.count;
      }
    }
    
    inline
    void
    GroupCaps::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      caps.clear();
      current_time = 0;
      
      uint32_t v = 0;
      uint32_t sz = 0;
      uint64_t tm = 0;
      
      bstr >> v >> current_time >> tm >> sz;

      GroupId id = 0;
      uint32_t t = 0;
      uint32_t c = 0;
      
      while(sz--)
      {
        bstr >> id >> t >> c;
        caps.insert(std::make_pair(id, GroupCap(c, tm - t)));
      }
    }

    inline
    void
    GroupCaps::from_string(const char* str) throw(El::Exception)
    {
      caps.clear();
      current_time = 0;
      
      if(str)
      {
        try
        {
          std::string val;
          El::String::Manip::base64_decode(str, val);
          
          std::istringstream istr(val);
          El::BinaryInStream bstr(istr);
          bstr >> *this;
        }
        catch(...)
        {
        }
      }
    }

    inline
    void
    GroupCaps::to_string(std::string& str) const throw(El::Exception)
    {
      std::ostringstream ostr;
      
      {
        El::BinaryOutStream bstr(ostr);
        bstr << *this;
      }

      std::string gfc = ostr.str();

      El::String::Manip::base64_encode((unsigned char*)gfc.c_str(),
                                       gfc.length(),
                                       str);
    }

    inline
    void
    GroupCaps::swap(GroupCaps& val) throw()
    {
      caps.swap(val.caps);
      std::swap(current_time, val.current_time);
    }

    //
    // SelectionContext struct
    //
    inline 
    bool
    CreativeWeight::operator<(const CreativeWeight& v) const throw()
    {
      return weight < v.weight;
    }  
    
    //
    // SelectionContext struct
    //
    inline
    SelectionContext::SelectionContext() throw()
        : page(0),
          rnd(0),
          ip(0),
          query_types(0)
    {
    }
    
    inline
    SelectionContext::SelectionContext(PageId page_val,
                                       uint32_t rnd_val) throw()
        : page(page_val),
          rnd(rnd_val),
          ip(0),
          query_types(0)
    {
      ad_caps.set_current_time();
      counter_caps.current_time = ad_caps.current_time;
    }

    inline
    void
    SelectionContext::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr << page << search_engine << crawler << rnd << language << country
           << ip << ad_caps << counter_caps << query_types;
      
      bstr.write_set(message_categories);
      bstr.write_set(page_categories);
      bstr.write_set(message_sources);
      bstr.write_set(page_sources);
      bstr.write_array(slots);
      bstr.write_set(tags);
      bstr.write_set(referers);
      bstr.write_set(content_languages);
    }
    
    inline
    void
    SelectionContext::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> page >> search_engine >> crawler >> rnd >> language >> country
           >> ip >> ad_caps >> counter_caps >> query_types;
      
      bstr.read_set(message_categories);
      bstr.read_set(page_categories);
      bstr.read_set(message_sources);
      bstr.read_set(page_sources);
      bstr.read_array(slots);
      bstr.read_set(tags);
      bstr.read_set(referers);
      bstr.read_set(content_languages);
    }

    inline
    void
    SelectionContext::rand() throw()
    {
      rnd = ::rand();
    }

    inline
    uint32_t
    SelectionContext::rnd_mod(uint32_t mod) const throw()
    {
      return (unsigned long long)rnd * mod /
        ((unsigned long long)RAND_MAX + 1);
    }

    inline
    void
    SelectionContext::add_message_category(const char* category)
      throw(El::Exception)
    {
      add_category(category, message_categories);
    }
    
    inline
    void
    SelectionContext::add_page_category(const char* category)
      throw(El::Exception)
    {
      if(*category != '\0')
      {
        add_category(category, page_categories);
        query_types |= QT_CATEGORY;
      }
    }

    inline
    void
    SelectionContext::add_message_source(const char* source)
      throw(El::Exception)
    {
      add_url(source, message_sources);
    }
    
    inline
    void
    SelectionContext::add_page_source(const char* source) throw(El::Exception)
    {
      if(*source != '\0')
      {
        add_url(source, page_sources);
      }
    }

    inline
    void
    SelectionContext::add_tag(const char* tag) throw(El::Exception)
    {
      tags.insert(tag);
    }

    inline
    void
    SelectionContext::add_content_lang(const El::Lang& lang)
      throw(El::Exception)
    {
      content_languages.insert(lang);
    }

    inline
    void
    SelectionContext::set_referer(const char* referer) throw(El::Exception)
    {
      referers.clear();
      add_url(referer, referers);
    }

    //
    // Selector::CreativeWeightStrategy struct
    //
    inline
    Selector::CreativeWeightStrategy*
    Selector::NoneCreativeWeightStrategy::clone() const throw()
    {
      return new NoneCreativeWeightStrategy();
    }

    inline
    void
    Selector::NoneCreativeWeightStrategy::set_weights(
      CreativeWeightArray& creative_weights)
      throw(El::Exception)
    {
    }

    //
    // Selector::ProbabilisticCreativeWeightStrategy struct
    //
    inline
    Selector::ProbabilisticCreativeWeightStrategy::
    ProbabilisticCreativeWeightStrategy()
      throw()
        : reduction_rate(1),
          weight_zones(10)
    {
    }

    inline
    Selector::ProbabilisticCreativeWeightStrategy::
    ProbabilisticCreativeWeightStrategy(float reduct_rate,
                                        uint32_t wght_zones) throw()
      : reduction_rate(reduct_rate),
          weight_zones(wght_zones)
    {
    }
    
    inline
    Selector::ProbabilisticCreativeWeightStrategy::
    ProbabilisticCreativeWeightStrategy(
      const Selector::ProbabilisticCreativeWeightStrategy& src)
      throw()
        : reduction_rate(src.reduction_rate),
          weight_zones(src.weight_zones)
    {
    }
    
    inline
    Selector::CreativeWeightStrategy*
    Selector::ProbabilisticCreativeWeightStrategy::clone() const
      throw()
    {
      return new ProbabilisticCreativeWeightStrategy(*this);
    }

    inline
    void
    Selector::ProbabilisticCreativeWeightStrategy::write(
      El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr << reduction_rate << weight_zones;
    }
        
    inline
    void
    Selector::ProbabilisticCreativeWeightStrategy::read(
      El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> reduction_rate >> weight_zones;
    }
    
    inline
    CreativeWeight
    Selector::ProbabilisticCreativeWeightStrategy::random_take(
      CWGArray& cwga) const throw(El::Exception)
    {
      CreativeWeight cw;

      double cumulative_weight = cwga.rbegin()->cumulative_weight;
      int r = ::rand();
      
      double weight =
        ((double) r / ((double)RAND_MAX + 1)) * cumulative_weight;

      cumulative_weight = 0;

      CWGArray::iterator i(cwga.begin());
      CWGArray::iterator e(cwga.end());

      CreativeWeightArray::iterator j;
      
      for(; i != e; ++i)
      {
        CWG& cwg = *i;
        
        if(weight < cwg.cumulative_weight)
        {
          j = cwg.creative_weights.begin();
          
          CreativeWeightArray::iterator e(cwg.creative_weights.end());
            
          for(; j != e; ++j)
          {
            cumulative_weight += j->weight;
            
            if(weight < cumulative_weight)
            {
              break;
            }
          }

          if(j == e)
          {
            j = cwg.creative_weights.begin() +
              (cwg.creative_weights.size() - 1);
          }

          break;
        }

        cumulative_weight = cwg.cumulative_weight;
      }

      if(i == e)
      {
        i = cwga.begin() + (cwga.size() - 1);

        CreativeWeightArray& creative_weights = i->creative_weights;
        j = creative_weights.begin() + (creative_weights.size() - 1);
      }

      cw = *j;
      
      CWG& cwg = *i;
      cwg.creative_weights.erase(j);

      if(cwg.creative_weights.empty())
      {
        i = cwga.erase(i);
      }

      e = cwga.end();
      
      for(; i != e; ++i)
      {
        CWG& cwg = *i;
        cwg.total_weight -= cw.weight;
        cwg.cumulative_weight -= cw.weight;
      }
      
      return cw;
    }
    
    inline
    void
    Selector::ProbabilisticCreativeWeightStrategy::downgrade_weights(
      CWGArray::reverse_iterator from,
      CWGArray::reverse_iterator to)
      const throw(El::Exception)
    {
      CWGArray::reverse_iterator next = from + 1;

      if(next == to)
      {
        return;
      }

      downgrade_weights(next, to);      
      
      double max_weight = from->total_weight * reduction_rate;

      if(next->cumulative_weight > max_weight)
      {
        double factor = max_weight / next->cumulative_weight;
        
        for(CWGArray::reverse_iterator i(next); i != to; ++i)
        {
          i->multiply(factor);
        }
      }

      from->cumulative_weight = next->cumulative_weight + from->total_weight;
    }
    
    //
    // Selector::ProbabilisticCreativeWeightStrategy::CWG struct
    //
    inline
    Selector::ProbabilisticCreativeWeightStrategy::CWG::CWG() throw()
        : total_weight(0),
          cumulative_weight(0)
    {
    }

    inline
    void
    Selector::ProbabilisticCreativeWeightStrategy::CWG::multiply(
      double factor) throw()
    {
      total_weight *= factor;
      cumulative_weight *= factor;
      
      for(CreativeWeightArray::iterator i(creative_weights.begin()),
            e(creative_weights.end()); i != e; ++i)
      {
        i->weight *= factor;
      }
    }
    
    //
    // SelectionResult struct
    //

    inline
    void
    SelectionResult::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr.write_array(ads);
      bstr.write_array(counters);
      bstr << ad_caps << counter_caps;
    }
    
    inline
    void
    SelectionResult::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr.read_array(ads);
      bstr.read_array(counters);
      bstr >> ad_caps >> counter_caps;
    }
    
    //
    // Selection struct
    //
    inline
    Selection::Selection(CreativeId id_val,
                         SlotId slot_val,
                         uint32_t width_val,
                         uint32_t height_val,
                         const char* text_val,
                         CreativeInjection inject_val) throw()
        : id(id_val),
          slot(slot_val),
          width(width_val),
          height(height_val),
          text(text_val),
          inject(inject_val)
    {
    }

    inline
    void
    Selection::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << id << slot << width << height << text << (uint32_t)inject;
    }

    inline
    void
    Selection::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      uint32_t inj = 0;
      bstr >> id >> slot >> width >> height >> text >> inj;
      inject = (CreativeInjection)inj;
    }
    
    inline
    bool
    Selection::operator==(const Selection& val) const throw()
    {
      return id == val.id && slot == val.slot && width == val.width &&
        height == val.height && text == val.text && inject == val.inject;
    }
    //
    // SelectedCounter struct
    //
    inline
    SelectedCounter::SelectedCounter(CounterId id_val,
                                     const char* text_val) throw()
        : id(id_val),
          text(text_val)
    {
    }

    inline
    void
    SelectedCounter::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr << id << text;
    }

    inline
    void
    SelectedCounter::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> id >> text;
    }
    
    inline
    bool
    SelectedCounter::operator==(const SelectedCounter& val) const throw()
    {
      return id == val.id && text == val.text;
    }    
    
    //
    // SelectedPlacement struct
    //
    inline
    Selector::SelectedPlacement::SelectedPlacement(
      SlotId id_val,
      const Creative* creative_val) throw()
        : id(id_val),
          creative(creative_val)
    {
    }

    //
    // Selector struct
    //
    inline
    Selector::Selector() throw(El::Exception)
        : group_cap_timeout(0),
          group_cap_max_count(0),
          counter_cap_timeout(0),
          counter_cap_max_count(0),
          update_number(0),
          creative_weight_strategy(new NoneCreativeWeightStrategy())
    {
    }

    inline
    Selector::Selector(const Selector& src) throw(El::Exception)
    {
      *this = src;
    }
    
    inline
    Selector&
    Selector::operator=(const Selector& src) throw(El::Exception)
    {
      std::ostringstream ostr;
      
      {
        El::BinaryOutStream bstr(ostr);
        bstr << src;
      }

      std::istringstream istr(ostr.str());
      El::BinaryInStream bstr(istr);
      bstr >> *this;

      return *this;
    }    
  }
}

#endif // _NEWSGATE_SERVER_COMMONS_FEED_TYPES_HPP_
