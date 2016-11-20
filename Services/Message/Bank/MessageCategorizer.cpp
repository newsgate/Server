/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Message/Bank/MessageCategorizer.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <sstream>
#include <string>
#include <algorithm>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/RefCount/All.hpp>
#include <El/String/Manip.hpp>
#include <El/Logging/Logger.hpp>

#include <Commons/Message/StoredMessage.hpp>

#include <Commons/Search/SearchCondition.hpp>

#include "BankMain.hpp"
#include "MessageCategorizer.hpp"

namespace NewsGate
{
  namespace Message
  {
    const Categorizer::Category::Id MessageCategorizer::ROOT_CATEGORY_ID = 1;

    void
    MessageCategorizer::categorize(
      SearcheableMessageMap& messages,
      const IdSet& ids,
      bool no_hash_check,
      MessageCategoryMap& old_categories) const
      throw(Exception, El::Exception)
    {
      old_categories.clear();
      
      if(ids.empty())
      {
        return; 
      }

      Search::Condition::Context search_context(messages);
      Search::Condition::Result search_context_intersect_list;

      search_context_intersect_list.resize(ids.size());
      search_context.intersect_list.push_back(&search_context_intersect_list);

      const IdToNumberMap& id_to_number = messages.id_to_number;

      for(IdSet::const_iterator it = ids.begin(); it != ids.end(); it++)
      {
        Id id = *it;
        
        StoredMessage* msg = messages.find(id);
        
        if(msg && (no_hash_check || msg->categories.categorizer_hash != hash))
        {          
          old_categories.insert(std::make_pair(id, Categories())).
            first->second = msg->categories;

          Categories categories;
          categories.categorizer_hash = hash;
          
          messages.set_categories(*msg, categories);

          IdToNumberMap::const_iterator nit = id_to_number.find(id);
          assert(nit != id_to_number.end());
          
          search_context_intersect_list.insert(
            std::make_pair(nit->second, msg));
        }
      }

      if(search_context_intersect_list.empty())
      {
        return;
      }

      ResultMap results;
        
      for(Categorizer::Category::IdArray::const_iterator
            it = calulation_order.begin(); it != calulation_order.end(); it++)
      {
        Categorizer::Category::Id id = *it;
        
        results.erase(id);

        const Search::Condition::Result& result =
          search_category(messages,
                          search_context,
                          id,
                          results);

        CategoryMap::const_iterator cit = categories.find(id);
        assert(cit != categories.end());
        
        const Category::PathArray& paths = cit->second.paths;

        for(Search::Condition::Result::const_iterator mit = result.begin();
            mit != result.end(); mit++)
        {
          StoredMessage* msg = messages.find(mit->second->id);
          assert(msg != 0);

          for(Category::PathArray::const_iterator pit = paths.begin();
              pit != paths.end(); pit++)
          {
            messages.add_category(*msg, pit->value, pit->lower.c_str());  
          }
        }
      }

      for(Search::Condition::Result::iterator
            it(search_context_intersect_list.begin()),
            eit(search_context_intersect_list.end()); it != eit; ++it)
      {
        StoredMessage* msg = const_cast<StoredMessage*>(it->second);
        
        Categories categories;
        categories.array = msg->categories.array;

        StringConstPtr* begin = categories.array.elements();
        StringConstPtr* end = begin + categories.array.size();

        bool path_modified = false;

        for(StringConstPtr* cat = begin; cat != end; cat++)
        {
          NonSearcheablePathsMap::const_iterator nit =
            non_searcheable_paths.find(*cat);

          if(nit != non_searcheable_paths.end())
          {
            *cat = nit->second;
            path_modified = true;
          }
        }

        if(categories.array.size() == 1)
        {
          if(!path_modified)
          {
            continue;
          }
        }
        else
        {
          std::sort(begin, end, std::greater<StringConstPtr>());

          typedef std::vector<StringConstPtr> CategoryPtrArray;

          CategoryPtrArray optimized_categories;
          optimized_categories.reserve(categories.array.size());
            
          for(const StringConstPtr* cat = begin; cat != end; cat++)
          {
            const StringConstPtr& path = *cat;
            unsigned long path_len = path.length();
              
            CategoryPtrArray::iterator oit = optimized_categories.begin();
              
            for(; oit != optimized_categories.end() &&
                  strncmp(path.c_str(), oit->c_str(), path_len); oit++);
              
              
            if(oit != optimized_categories.end())
            {
              continue;
            }
            
            for(oit = optimized_categories.begin();
                oit != optimized_categories.end() && *oit < path; oit++);
              
            optimized_categories.insert(oit, path);
          }

          categories.array.init(optimized_categories);
        }
          
        categories.categorizer_hash = hash;
        messages.set_categories(*msg, categories);
      }

    }

    Search::Condition::Result&
    MessageCategorizer::search_category(const SearcheableMessageMap& messages,
                                        Search::Condition::Context& context,
                                        Categorizer::Category::Id id,
                                        ResultMap& results) const
      throw(El::Exception)
    {
      ResultMap::iterator it = results.find(id);

      if(it != results.end())
      {
        return it->second;
      }
      
      CategoryMap::const_iterator cit = categories.find(id);
      assert(cit != categories.end());

      const Category& category = cit->second;
      
      Search::Condition::MessageMatchInfoMap match_info;

      Search::Condition::ResultPtr res(
        category.condition->evaluate(context, match_info, 0));
      
      Search::Condition::Result& result =
        results.insert(std::make_pair(id, Search::Condition::Result())).
        first->second;
      
      result = *res;
      res.reset(0);

      const Categorizer::Category::IdArray& children = category.children;

      for(Categorizer::Category::IdArray::const_iterator it = children.begin();
          it != children.end(); it++)
      {
        Search::Condition::Result& res = search_category(messages,
                                                         context,
                                                         *it,
                                                         results);
        result.insert(res.begin(), res.end());
      }

      return result;
    }

    void
    MessageCategorizer::locale(const char* path,
                               const El::Locale& locale,
                               Categorizer::Category::Locale& category_locale)
      const throw(El::Exception)
    {
      Categorizer::Category::Id id = find(path);

      if(!id)
      {
        return;
      }

      CategoryMap::const_iterator it = categories.find(id);
      assert(it != categories.end());

      it->second.best_locale(locale, category_locale);
    }    
    
/*    
    bool
    MessageCategorizer::locale(const char* path,
                               const El::Locale& locale,
                               Categorizer::Category::Locale& category_locale)
      const throw(El::Exception)
    {
      Categorizer::Category::Id id = find(path);

      if(!id)
      {
        return false;
      }

      CategoryMap::const_iterator it = categories.find(id);
      assert(it != categories.end());

      const Categorizer::Category::Locale* loc = it->second.best_locale(locale);

      if(loc)
      {
        category_locale = *loc;
      }
      
      return loc != 0;
    }
*/  
    Categorizer::Category::Id
    MessageCategorizer::find(const char* path) const
      throw(El::Exception)
    {
      return find(ROOT_CATEGORY_ID, path);
      
    }
    
    Categorizer::Category::Id
    MessageCategorizer::find(Categorizer::Category::Id id,
                             const char* rel_path) const
      throw(El::Exception)
    {
      const char* subpath = strchr(rel_path, '/');

      if(subpath == 0)
      {
        return 0;
      }

      CategoryMap::const_iterator it = categories.find(id);
      assert(it != categories.end());

      const Category& cat = it->second;

      std::string lower;
      El::String::Manip::utf8_to_lower(cat.name.c_str(), lower);

      if(std::string(rel_path, subpath - rel_path) != lower)
      {
        return 0;
      }

      if(*(++subpath) == '\0')
      {
        return id;
      }
      
      const Categorizer::Category::IdArray& children = cat.children;

      for(Categorizer::Category::IdArray::const_iterator it = children.begin();
          it != children.end(); it++)
      {
        Categorizer::Category::Id res = find(*it, subpath);

        if(res)
        {
          return res;
        }
        
      }

      return 0;
    }

    void
    MessageCategorizer::init() throw(El::Exception)
    {
      calulation_order.clear();
      calulation_order.reserve(categories.size());

      std::ostringstream ostr;

      if(Application::will_trace(El::Logging::HIGH))
      {
        ostr << "MessageCategorizer::init:\n* Dependencies:\n";
      }
          
      for(MessageCategorizer::CategoryMap::iterator it = categories.begin();
          it != categories.end(); it++)
      {
        calulation_order.push_back(it->first);
        
        MessageCategorizer::Category& cat = it->second;
        
        CategoryIdSet cat_ids;
        get_category_dependencies(cat.condition.in(), cat_ids);

        cat.dependencies.clear();
        cat.dependencies.reserve(cat_ids.size());

        for(CategoryIdSet::const_iterator cit = cat_ids.begin();
            cit != cat_ids.end(); cit++)
        {
          cat.dependencies.push_back(*cit);

          if(Application::will_trace(El::Logging::HIGH))
          {
            ostr << "  " << name(it->first) << " (" << it->first << ") -> "
                 << name(*cit) << " (" << *cit << ")" << std::endl;
          }
        }
      }

      CalculateOrderCmp order_cmp;
      set_full_paths(ROOT_CATEGORY_ID, 0, "", 0, order_cmp.child_levels);

      if(Application::will_trace(El::Logging::LOW))
      {
        ostr << "\n* Circular Deps Check:\n";

        size_t circular_deps_count = 0;
        size_t subordination_chains_count = 0;
        size_t dependency_chains_count = 0;
        DependencyPairCounter dep_counter;
          
        for(MessageCategorizer::CategoryMap::iterator i(categories.begin()),
              e(categories.end()); i != e; ++i)
        {
          MessageCategorizer::Category& cat = i->second;
          
          for(Categorizer::Category::IdArray::const_iterator
                j(cat.dependencies.begin()), e(cat.dependencies.end());
              j != e; ++j)
          {
            check_circular_deps(i->first,
                                *j,
                                ostr,
                                circular_deps_count,
                                subordination_chains_count,
                                dependency_chains_count,
                                dep_counter);
          }
        }

        if(circular_deps_count)
        {
          ostr << "\n  Conclusion:\n    circular dependencies "
               << circular_deps_count << "\n    subordination chains "
               << subordination_chains_count << "\n    dependency chains "
               << dependency_chains_count << "\n    dependency frequency:\n";

          DependencyPairCountArray dep_pair_counts;
          dep_pair_counts.reserve(dep_counter.size());
            
          for(DependencyPairCounter::const_iterator i(dep_counter.begin()),
                e(dep_counter.end()); i != e; ++i)
          {
            const DependencyPair& dep_pair(i->first);
            size_t count(i->second);

            DependencyPairCountArray::iterator
              ai(dep_pair_counts.begin()),
              ae(dep_pair_counts.end());
            
            for(; ai != ae && ai->count >= count; ++ai);
            dep_pair_counts.insert(ai, DependencyPairCount(dep_pair, count));
          }

          for(DependencyPairCountArray::const_iterator
                i(dep_pair_counts.begin()), e(dep_pair_counts.end()); i != e;
              ++i)
          {
            const DependencyPair& pair(i->pair);
            
            ostr << "      " << i->count << " "
                 << path(pair.dependent) << " (" << pair.dependent << ") -> "
                 << path(pair.dependee) << " (" << pair.dependee << ") "
                 << (pair.child ? "child" : "dep") << std::endl;
          }
        }
      }
      
      order_cmp.deps_levels = order_cmp.child_levels;
      
      if(Application::will_trace(El::Logging::HIGH))
      {
        ostr << "\n* Child Levels:\n";

        for(CategoryLevelMap::const_iterator
              it = order_cmp.child_levels.begin();
            it != order_cmp.child_levels.end(); it++)
        {
          ostr << "  " << path(it->first) << " (" << it->first << ") : "
               << it->second << std::endl;        
        }

        ostr << "\n* Check deps order:\n";
      }
      
      std::sort(calulation_order.begin(),
                calulation_order.end(),
                (const CheckDepsOrderCmp&)order_cmp);
      
      for(Categorizer::Category::IdArray::const_iterator
            it = calulation_order.begin(); it != calulation_order.end(); it++)
      {
        Categorizer::Category::Id id = *it;

        if(Application::will_trace(El::Logging::HIGH))
        {
          ostr << "  " << path(id) << " (" << id << ")" << std::endl;
        }

        unsigned long level = order_cmp.deps_levels.find(id)->second;

        MessageCategorizer::CategoryMap::iterator cit = categories.find(id);
        assert(cit != categories.end());

        const Categorizer::Category::IdArray& dependencies =
          cit->second.dependencies;

        CategoryIdSet processed;
        processed.insert(id);
        
        for(Categorizer::Category::IdArray::const_iterator
              it = dependencies.begin(); it != dependencies.end(); it++)
        {            
          set_deps_level(*it,
                         level + 1,
                         processed,
                         order_cmp.deps_levels);
        }
      }

      if(Application::will_trace(El::Logging::HIGH))
      {
        ostr << "\n* Deps Levels:\n";

        for(CategoryLevelMap::const_iterator
              it = order_cmp.deps_levels.begin();
            it != order_cmp.deps_levels.end(); it++)
        {
          ostr << "  " << path(it->first) << " (" << it->first << ") : "
               << it->second << std::endl;        
        }        
      }
      
      std::sort(calulation_order.begin(), calulation_order.end(), order_cmp);
      
      if(Application::will_trace(El::Logging::HIGH))
      {
        ostr << "\n* Calculation order:\n";
        
        for(Categorizer::Category::IdArray::const_iterator
              it = calulation_order.begin(); it != calulation_order.end();
            it++)
        {
          ostr << "  " << path(*it) << " (" << *it << ")" << std::endl;
        }
      }

      if(Application::will_trace(El::Logging::HIGH))
      {
        Application::logger()->trace(ostr.str(),
                                     Aspect::MSG_CATEGORIZATION,
                                     El::Logging::HIGH);
      }
    }

    void
    MessageCategorizer::check_circular_deps(
      Categorizer::Category::Id dependent_cat,
      Categorizer::Category::Id dependee_cat,
      std::ostringstream& ostr,
      size_t& circular_deps_count,
      size_t& subordination_chains_count,
      size_t& dependency_chains_count,
      DependencyPairCounter& dep_counter)
      const throw(El::Exception)
    {
      DependencyStack stack;
      DependencyStackArray stack_array;

      find_dep(dependee_cat, dependent_cat, false, stack, stack_array);
      
      if(!stack_array.empty())
      {
        ++circular_deps_count;
        
        ostr << "\n  " << path(dependent_cat) << " (" << dependent_cat
             << ") circulary depends from " << path(dependee_cat) << " ("
             << dependee_cat << ") through " << stack_array.size()
             << " chain(s):\n";

        size_t number = 0;
        
        for(DependencyStackArray::const_iterator i(stack_array.begin()),
              e(stack_array.end()); i != e; ++i)
        {          
          const DependencyStack& stack = *i;
          std::string chain_type;
            
          for(DependencyStack::const_iterator i(stack.begin() + 1),
                e(stack.end()); i != e; ++i)
          {
            const Dependency& dep = *i;
            
            if(!dep.child)
            {
              if(chain_type.empty())
              {
                ++dependency_chains_count;
                chain_type = "dependency";
              }
            }

            DependencyPair dep_pair((i - 1)->dependee,
                                    dep.dependee,
                                    dep.child);
            
            dep_counter.insert(std::make_pair(dep_pair, 0)).first->second++;
          }

          if(chain_type.empty())
          {
            ++subordination_chains_count;
            chain_type = "subordination";
          }
        
          ostr << "    [" << number++ << " - " << chain_type << "]\n";
          
          for(DependencyStack::const_iterator i(stack.begin()),
                e(stack.end()); i != e; ++i)
          {
            ostr << "      " << path(i->dependee) << " ("
                 << (i->child ? "child" : "dep") << ")\n";
          }
        }
      }
    }
    
    void
    MessageCategorizer::find_dep(Categorizer::Category::Id from,
                                 Categorizer::Category::Id whom,
                                 bool child,
                                 DependencyStack& stack,
                                 DependencyStackArray& stack_array)
      const throw(El::Exception)
    {
      for(DependencyStack::const_iterator i(stack.begin()), e(stack.end());
          i != e; ++i)
      {
        if(i->dependee == from)
        {
          return;
        }
      }
      
      stack.push_back(Dependency(from, child));
        
      const MessageCategorizer::Category& cat = categories.find(from)->second;

      for(Categorizer::Category::IdArray::const_iterator
            j(cat.dependencies.begin()), e(cat.dependencies.end());
          j != e; ++j)
      {
        if(*j == whom)
        {
          stack_array.push_back(stack);
          stack_array.rbegin()->push_back(Dependency(whom, false));
        }
        else
        {
          find_dep(*j, whom, false, stack, stack_array);
        }
      }
      
      for(Categorizer::Category::IdArray::const_iterator
            j(cat.children.begin()), e(cat.children.end());
          j != e; ++j)
      {
        if(*j == whom)
        {
          stack_array.push_back(stack);
          stack_array.rbegin()->push_back(Dependency(whom, true));
        }
        else
        {
          find_dep(*j, whom, true, stack, stack_array);
        }
      }

      stack.pop_back();
    }
    
    void
    MessageCategorizer::translate(Search::StringCounterMap& category_counter,
                                  const El::Locale& locale)
      const throw(El::Exception)
    {
      Search::StringCounterMap new_category_counter;

      translate(ROOT_CATEGORY_ID,
                locale,
                category_counter,
                new_category_counter);
      
      category_counter.swap(new_category_counter);
    }

    void
    MessageCategorizer::translate(
      Categorizer::Category::Id cat_id,
      const El::Locale& locale,
      const Search::StringCounterMap& category_counter,
      Search::StringCounterMap& new_category_counter) const
      throw(El::Exception)
    {
      CategoryMap::const_iterator it = categories.find(cat_id);
      assert(it != categories.end());

      const Category& category = it->second;
      const Category::PathArray& paths = category.paths;
      
      for(Category::PathArray::const_iterator it = paths.begin();
          it != paths.end(); it++)
      {
        const StringConstPtr& path = it->value;
        
        if(new_category_counter.find(path.c_str()) !=
           new_category_counter.end())
        {
          continue;
        }

        Message::StringConstPtr lname;

        if(!category.name.empty())
        {
          Categorizer::Category::Locale loc;
          category.best_locale(locale, loc);

          if(!loc.name.empty())
          {
            lname = loc.name;
          }
          else if(config_.category_name_as_last_resort())
          {
            lname = category.name;
          }          
        }
        
        Search::StringCounterMap::iterator nit =
          new_category_counter.insert(
            std::make_pair(path.add_ref(),
                           Search::Counter(0, 0, lname.add_ref(), cat_id))).
          first;

        Search::StringCounterMap::const_iterator cit =
          category_counter.find(path.c_str());

        if(cit != category_counter.end())
        {
          nit->second.count1 = cit->second.count1;
          nit->second.count2 = cit->second.count2;
        }
      }

      const Categorizer::Category::IdArray& children = category.children;
      
      for(Categorizer::Category::IdArray::const_iterator it = children.begin();
          it != children.end(); it++)
      {
        translate(*it, locale, category_counter, new_category_counter);
      }
    }
    
    void
    MessageCategorizer::set_deps_level(
      Categorizer::Category::Id id,
      size_t level,
      CategoryIdSet& processed,
      CategoryLevelMap& levels) const
      throw(El::Exception)
    {
      if(processed.find(id) != processed.end())
      {
        return;
      }
      
      MessageCategorizer::CategoryMap::const_iterator cit =
        categories.find(id);
      
      assert(cit != categories.end());

      const Categorizer::Category::IdArray& dependencies =
        cit->second.dependencies;
      
      for(Categorizer::Category::IdArray::const_iterator
            it = dependencies.begin(); it != dependencies.end(); it++)
      {
        if(processed.find(*it) != processed.end())
        {
          return;
        }
      }

      CategoryLevelMap::iterator it = levels.find(id);
      assert(it != levels.end());

      level = std::max(level, it->second);
      it->second = level;

      processed.insert(id);

      for(Categorizer::Category::IdArray::const_iterator
            it = dependencies.begin(); it != dependencies.end(); it++)
      {
        set_deps_level(*it, level + 1, processed, levels);
      }

      const Categorizer::Category::IdArray& children =
        cit->second.children;
      
      for(Categorizer::Category::IdArray::const_iterator
            it = children.begin(); it != children.end(); it++)
      {
        set_deps_level(*it, level + 1, processed, levels);
      }

      processed.erase(id);
    }
    
    void
    MessageCategorizer::set_full_paths(
      Categorizer::Category::Id id,
      size_t level,
      const char* path,
      const char* searcheable_path,
      CategoryLevelMap& levels)
      throw(El::Exception)
    {
      CategoryLevelMap::iterator lit = levels.find(id);

      if(lit == levels.end())
      {
        levels[id] = level;
      }
      else
      {
        lit->second = std::max(lit->second, level);
      }
      
      level++;
      
      CategoryMap::iterator it = categories.find(id);
      assert(it != categories.end());
      
      Category& cat = it->second;      
      Category::Path cur_path((std::string(path) + cat.name + "/").c_str());
        
      bool searcheable = cat.searcheable && searcheable_path == 0;

      if(!searcheable)
      {
        if(searcheable_path == 0)
        {
          searcheable_path = path;
        }
        
        non_searcheable_paths[cur_path.value] = searcheable_path;
      }
      
      cat.paths.push_back(cur_path);

      const Categorizer::Category::IdArray& children = cat.children;
      
      for(Categorizer::Category::IdArray::const_iterator it = children.begin();
          it != children.end(); it++)
      {
        set_full_paths(*it,
                       level,
                       cur_path.value.c_str(),
                       searcheable_path,
                       levels);
      }
    }
      
    void
    MessageCategorizer::get_category_dependencies(
      const Search::Condition* condition,
      CategoryIdSet& cat_ids) const
      throw(El::Exception)
    {
      const Search::Category* category =
        dynamic_cast<const Search::Category*>(condition);

      if(category)
      {
        const Search::Category::CategoryArray& categories =
          category->categories;

        for(size_t i = 0; i < categories.size(); i++)
        {
          Categorizer::Category::Id id = find(categories[i].c_str());

          if(id)
          {
            cat_ids.insert(id);
          }
        }
      }
      else
      { 
        Search::ConditionArray subconditions = condition->subconditions();

        for(Search::ConditionArray::const_iterator it = subconditions.begin();
            it != subconditions.end(); it++)
        {
          get_category_dependencies(it->in(), cat_ids);
        }
      }
    }
    
    std::string
    MessageCategorizer::name(Categorizer::Category::Id id)
      const throw(El::Exception)
    {
      CategoryMap::const_iterator it = categories.find(id);
      assert(it != categories.end());

      return std::string(it->second.name.c_str()) + "/";
    }
    
    const char*
    MessageCategorizer::path(Categorizer::Category::Id id)
      const throw(El::Exception)
    {
      CategoryMap::const_iterator it = categories.find(id);
      
      assert(it != categories.end());
      assert(!it->second.paths.empty());

      return it->second.paths[0].value.c_str();
    }
    
  }
}
