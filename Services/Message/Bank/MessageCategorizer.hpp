/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Message/Bank/MessageCategorizer.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_MESSAGECATEGORIZER_HPP_
#define _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_MESSAGECATEGORIZER_HPP_

#include <stdint.h>

#include <string>
#include <vector>

#include <ext/hash_set>
#include <ext/hash_map>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/RefCount/All.hpp>
#include <El/Locale.hpp>
#include <El/CRC.hpp>

#include <xsd/Config.hpp>

#include <Commons/Message/StoredMessage.hpp>
#include <Commons/Search/SearchExpression.hpp>
#include <Commons/Message/Categorizer.hpp>

namespace NewsGate
{
  namespace Message
  {
    struct MessageCategorizer :
      public El::RefCount::DefaultImpl<El::Sync::ThreadPolicy>
    {
      const static Categorizer::Category::Id ROOT_CATEGORY_ID;
      
      typedef std::vector<std::string> CategoryArray;
      
      typedef __gnu_cxx::hash_map<Id, CategoryArray, MessageIdHash>
      IdCategoriesMap;
      
      struct Category
      {
        struct Path
        {
          StringConstPtr value;
          std::string lower;

          Path(const char* val = 0) throw(El::Exception);

          void write(El::BinaryOutStream& bstr) const throw(El::Exception);
          void read(El::BinaryInStream& bstr) throw(El::Exception);      
        };
        
        typedef std::vector<Path> PathArray;
        
        std::string name;
        uint8_t searcheable;
        PathArray paths;
        Categorizer::Category::IdArray   children;
        Categorizer::Category::IdArray   dependencies;
        Categorizer::Category::LocaleMap locales;
        Search::Condition_var condition;
        
        Category() throw(El::Exception) : searcheable(0) {}

//        const Categorizer::Category::Locale*
//        best_locale(const El::Locale& locale) const throw(El::Exception);

        void best_locale(const El::Locale& locale,
                         Categorizer::Category::Locale& result) const
          throw(El::Exception);

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);
      };
      
      typedef __gnu_cxx::hash_map<
        Categorizer::Category::Id,
        Category,
        El::Hash::Numeric<Categorizer::Category::Id> >
      CategoryMap;
      
      typedef __gnu_cxx::hash_map<StringConstPtr,
                                  StringConstPtr,
                                  StringConstPtrHash>
      NonSearcheablePathsMap;
      
      uint64_t stamp;
      std::string source;
      uint32_t hash;
      uint32_t dict_hash;
      CategoryMap categories;
      Categorizer::Category::IdArray calulation_order;
      NonSearcheablePathsMap non_searcheable_paths;

      typedef Server::Config::BankMessageManagerType::message_categorizer_type
      Config;
      
      MessageCategorizer(const Config& config) throw();
      ~MessageCategorizer() throw() {}
      
      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);
      
      void init() throw(El::Exception);

      void translate(Search::StringCounterMap& category_counter,
                     const El::Locale& locale) const
        throw(El::Exception);

      void locale(const char* path,
                  const El::Locale& locale,
                  Categorizer::Category::Locale& category_locale) const
        throw(El::Exception);
      
      typedef __gnu_cxx::hash_map<Id, Categories, MessageIdHash>
      MessageCategoryMap;
      
      void categorize(SearcheableMessageMap& messages,
                      const IdSet& ids,
                      bool no_hash_check,
                      MessageCategoryMap& old_categories) const
        throw(Exception, El::Exception);
      
      Categorizer::Category::Id find(const char* path) const
        throw(El::Exception);

      std::string name(Categorizer::Category::Id id) const
        throw(El::Exception);
        
      const char* path(Categorizer::Category::Id id) const
        throw(El::Exception);

    private:

      struct DependencyPair
      {
        Categorizer::Category::Id dependent;
        Categorizer::Category::Id dependee;
        bool child;

        DependencyPair(Categorizer::Category::Id dependent_val,
                       Categorizer::Category::Id dependee_val,
                       bool child_val) throw();

        bool operator==(const DependencyPair& val) const throw();
      };

      struct DependencyPairHash
      {
        size_t operator()(const DependencyPair& val) const throw();
      };

      typedef __gnu_cxx::hash_map<DependencyPair, size_t, DependencyPairHash>
      DependencyPairCounter;

      struct DependencyPairCount
      {
        DependencyPair pair;
        size_t count;

        DependencyPairCount(const DependencyPair& pair_val,
                            size_t count_val)
          throw();
      };
      
      typedef std::vector<DependencyPairCount> DependencyPairCountArray;

      void check_circular_deps(Categorizer::Category::Id dependent_cat,
                               Categorizer::Category::Id dependee_cat,
                               std::ostringstream& ostr,
                               size_t& circular_deps_count,
                               size_t& subordination_chains_count,
                               size_t& dependency_chains_count,
                               DependencyPairCounter& dep_counter)
        const throw(El::Exception);

      struct Dependency
      {
        Categorizer::Category::Id dependee;
        bool child;

        Dependency(Categorizer::Category::Id dependee_val, bool child_val)
          throw();
      };

      typedef std::vector<Dependency> DependencyStack;
      typedef std::vector<DependencyStack> DependencyStackArray;
      
      void find_dep(Categorizer::Category::Id from,
                    Categorizer::Category::Id whom,
                    bool child,
                    DependencyStack& stack,
                    DependencyStackArray& stack_array)
        const throw(El::Exception);
      
      Categorizer::Category::Id find(Categorizer::Category::Id id,
                                     const char* rel_path) const
        throw(El::Exception);

      void translate(Categorizer::Category::Id,
                     const El::Locale& locale,
                     const Search::StringCounterMap& category_counter,
                     Search::StringCounterMap& new_category_counter) const
        throw(El::Exception);

      typedef __gnu_cxx::hash_set<
        Categorizer::Category::Id,
        El::Hash::Numeric<Categorizer::Category::Id> >
      CategoryIdSet;

      typedef __gnu_cxx::hash_map<
        Categorizer::Category::Id,
        size_t,
        El::Hash::Numeric<Categorizer::Category::Id> >
      CategoryLevelMap;

      struct CheckDepsOrderCmp
      {
        MessageCategorizer::CategoryLevelMap child_levels;
          
        bool operator()(Categorizer::Category::Id a,
                        Categorizer::Category::Id b) const throw();          
      };

      struct CalculateOrderCmp : public CheckDepsOrderCmp
      {
        MessageCategorizer::CategoryLevelMap deps_levels;
          
        bool operator()(Categorizer::Category::Id a,
                        Categorizer::Category::Id b) const throw();
      };

      typedef __gnu_cxx::hash_map<
        Categorizer::Category::Id,
        Search::Condition::Result,
        El::Hash::Numeric<Categorizer::Category::Id> >
      ResultMap;

      
      Search::Condition::Result&
      search_category(const SearcheableMessageMap& messages,
                      Search::Condition::Context& context,
                      Categorizer::Category::Id id,
                      ResultMap& results) const
        throw(El::Exception);
      
      void get_category_dependencies(const Search::Condition* condition,
                                     CategoryIdSet& cat_ids) const
        throw(El::Exception);

      void set_full_paths(Categorizer::Category::Id id,
                          size_t level,
                          const char* path,
                          const char* searcheable_path,
                          CategoryLevelMap& levels)
        throw(El::Exception);

      void set_deps_level(Categorizer::Category::Id id,
                          size_t level,
                          CategoryIdSet& processed,
                          CategoryLevelMap& levels) const
        throw(El::Exception);

    private:
      const Config& config_;
    };

    typedef El::RefCount::SmartPtr<MessageCategorizer>
    MessageCategorizer_var;
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
    // NewsGate::MessageCategorizer class
    //
    inline
    MessageCategorizer::MessageCategorizer(const Config& config) throw()
        : stamp(0),
          hash(0),
          dict_hash(0),
          config_(config)
    {
    }

    inline
    void
    MessageCategorizer::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr << stamp << source << hash << dict_hash;
      
      bstr.write_map(categories);
      bstr.write_array(calulation_order);
      bstr.write_map(non_searcheable_paths);
    }

    inline
    void
    MessageCategorizer::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> stamp >> source >> hash >> dict_hash;
      
      bstr.read_map(categories);
      bstr.read_array(calulation_order);
      bstr.read_map(non_searcheable_paths);
    }

    //
    // NewsGate::MessageCategorizer::Category::Path class
    //
    inline
    void
    MessageCategorizer::Category::Path::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr << value << lower;
    }

    inline
    void
    MessageCategorizer::Category::Path::read(El::BinaryInStream& bstr)
      throw(El::Exception)
    {
      bstr >> value >> lower;
    }
    
    //
    // NewsGate::MessageCategorizer::Category class
    //
    inline
    void
    MessageCategorizer::Category::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr << name << searcheable;
      
      bstr.write_array(paths);
      bstr.write_array(children);
      bstr.write_array(dependencies);
      bstr.write_map(locales);
      
      bstr << condition;
    }
    
    inline
    void
    MessageCategorizer::Category::read(El::BinaryInStream& bstr)
      throw(El::Exception)
    {
      bstr >> name >> searcheable;
      
      bstr.read_array(paths);
      bstr.read_array(children);
      bstr.read_array(dependencies);
      bstr.read_map(locales);
      
      bstr >> condition;
    }

    inline
    void
    MessageCategorizer::Category::best_locale(
      const El::Locale& locale,
      Categorizer::Category::Locale& result)
      const throw(El::Exception)
    {
      Categorizer::Category::Locale loc;      
      Categorizer::Category::LocaleMap::const_iterator i = locales.find(locale);
          
      if(i != locales.end())
      {
        loc = i->second;
      }

      i = locales.find(El::Locale(locale.lang, El::Country::null));
        
      if(i != locales.end())
      {
        loc.absorb(i->second);
      }
        
      i = locales.find(El::Locale(El::Lang::null, locale.country));

      if(i != locales.end())
      {
        loc.absorb(i->second);
      }
          
      i = locales.find(El::Locale::null);

      if(i != locales.end())
      {
        loc.absorb(i->second);
      }

      result.swap(loc);
    }
/*    
    inline
    const Categorizer::Category::Locale*
    MessageCategorizer::Category::best_locale(const El::Locale& locale) const
      throw(El::Exception)
    {
      Categorizer::Category::LocaleMap::const_iterator i =
        locales.find(locale);
          
      if(i == locales.end())
      {
        i = locales.find(El::Locale(locale.lang, El::Country::null));
        
        if(i == locales.end())
        {
          i = locales.find(El::Locale(El::Lang::null, locale.country));

          if(i == locales.end())
          {
            i = locales.find(El::Locale::null);

            if(i == locales.end())
            {
              return 0;
            }
          }
        }
      }
      
      assert(i != locales.end());
      return &i->second;
    }
*/    
    //
    // NewsGate::MessageCategorizer::Dependency class
    //
    inline
    MessageCategorizer::Dependency::Dependency(
      Categorizer::Category::Id dependee_val,
      bool child_val) throw()
        : dependee(dependee_val),
          child(child_val)
    {
    }
    
    //
    // NewsGate::MessageCategorizer::DependencyPair class
    //
    inline
    MessageCategorizer::DependencyPair::DependencyPair(
      Categorizer::Category::Id dependent_val,
      Categorizer::Category::Id dependee_val,
      bool child_val) throw()
        : dependent(dependent_val),
          dependee(dependee_val),
          child(child_val)
    {
    }

    inline
    bool
    MessageCategorizer::DependencyPair::operator==(const DependencyPair& val)
      const throw()
    {
      return dependent == val.dependent && dependee == val.dependee &&
        child == val.child;
    }
    
    //
    // NewsGate::MessageCategorizer::DependencyPairHash class
    //
    inline
    size_t
    MessageCategorizer::DependencyPairHash::operator()(
      const DependencyPair& val) const throw()
    {
      size_t hash = 0;
      
      El::CRC(hash,
              (const unsigned char*)&val.dependent,
              sizeof(val.dependent));

      El::CRC(hash,
              (const unsigned char*)&val.dependee,
              sizeof(val.dependee));

      El::CRC(hash,
              (const unsigned char*)&val.child,
              sizeof(val.child));

      return hash;
    }

    //
    // NewsGate::MessageCategorizer::DependencyPairCount class
    //
    inline
    MessageCategorizer::DependencyPairCount::DependencyPairCount(
      const DependencyPair& pair_val,
      size_t count_val)
      throw()
        : pair(pair_val),
          count(count_val)
    {
    }
    
    //
    // NewsGate::MessageCategorizer::CheckDepsOrderCmp class
    //
    inline
    bool
    MessageCategorizer::CheckDepsOrderCmp::operator()(
      Categorizer::Category::Id a,
      Categorizer::Category::Id b) const throw()
    {
      size_t al = child_levels.find(a)->second;
      size_t bl = child_levels.find(b)->second;
      
      return al == bl ? a < b : al < bl;
    }
    
    //
    // NewsGate::MessageCategorizer::CalculateOrderCmp class
    //
    inline
    bool
    MessageCategorizer::CalculateOrderCmp::operator()(
      Categorizer::Category::Id a,
      Categorizer::Category::Id b) const throw()
    {
      size_t al = deps_levels.find(a)->second;
      size_t bl = deps_levels.find(b)->second;
      
      if(al != bl)
      {
        return al > bl;
      }
    
      al = child_levels.find(a)->second;
      bl = child_levels.find(b)->second;
    
      return al == bl ? a < b : al > bl;
    }  

    //
    // NewsGate::MessageCategorizer::Category::Path class
    //
    inline
    MessageCategorizer::Category::Path::Path(const char* val)
      throw(El::Exception)
        : value(val ? val : 0)
    {
      El::String::Manip::utf8_to_lower(val, lower);
    }    
    
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_MESSAGECATEGORIZER_HPP_
