/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Commons/Search/SearchCondition.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <limits.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <sstream>
#include <utility>
#include <memory>
#include <list>

#include <El/Exception.hpp>
#include <El/Stat.hpp>
#include <El/String/Manip.hpp>
#include <El/String/HashedString.hpp>
#include <El/Net/HTTP/URL.hpp>

#include "SearchCondition.hpp"

namespace
{
//  const unsigned long ANY_WORD_RESULT_RESERVE = 100000;
  const size_t ANY_WORD_POS_RESERVE = 10000;
}

namespace NewsGate
{
  namespace Search
  {
    El::Stat::TimeMeter AllWords::evaluate_meter("AllWords::evaluate", false);
    El::Stat::TimeMeter AnyWords::evaluate_meter("AnyWords::evaluate", false);
    
    El::Stat::TimeMeter Site::evaluate_meter("Site::evaluate", false);
    El::Stat::TimeMeter Url::evaluate_meter("Url::evaluate", false);
    El::Stat::TimeMeter Category::evaluate_meter("Category::evaluate", false);
    El::Stat::TimeMeter Msg::evaluate_meter("Msg::evaluate", false);
    El::Stat::TimeMeter Event::evaluate_meter("Event::evaluate", false);
    El::Stat::TimeMeter Every::evaluate_meter("Every::evaluate", false);
    El::Stat::TimeMeter None::evaluate_meter("None::evaluate", false);
    El::Stat::TimeMeter And::evaluate_meter("And::evaluate", false);
    
    El::Stat::TimeMeter And::evaluate_simple_meter("And::evaluate_simple",
                                                   false);
    
    El::Stat::TimeMeter Or::evaluate_meter("Or::evaluate", false);
    
    El::Stat::TimeMeter Or::evaluate_simple_meter("Or::evaluate_simple",
                                                  false);
    
    El::Stat::TimeMeter Except::evaluate_meter("Except::evaluate", false);
    
    El::Stat::TimeMeter Except::evaluate_simple_meter(
      "Except::evaluate_simple",
      false);
    
    El::Stat::TimeMeter Fetched::evaluate_meter("Fetched::evaluate", false);
    
    El::Stat::TimeMeter Fetched::evaluate_simple_meter(
      "Fetched::evaluate_simple",
      false);
    
    El::Stat::TimeMeter Visited::evaluate_meter("Visited::evaluate", false);
    
    El::Stat::TimeMeter Visited::evaluate_simple_meter(
      "Visited::evaluate_simple",
      false);
    
    El::Stat::TimeMeter PubDate::evaluate_meter("PubDate::evaluate", false);
    
    El::Stat::TimeMeter PubDate::evaluate_simple_meter(
      "PubDate::evaluate_simple",
      false);
    
    El::Stat::TimeMeter With::evaluate_meter("With::evaluate", false);
    
    El::Stat::TimeMeter With::evaluate_simple_meter("With::evaluate_simple",
                                                    false);

    El::Stat::TimeMeter Lang::evaluate_meter("Lang::evaluate", false);
    
    El::Stat::TimeMeter Lang::evaluate_simple_meter("Lang::evaluate_simple",
                                                     false);
    
    El::Stat::TimeMeter Country::evaluate_meter("Country::evaluate", false);
    
    El::Stat::TimeMeter Country::evaluate_simple_meter(
      "Country::evaluate_simple",
      false);
    
    El::Stat::TimeMeter Space::evaluate_meter("Space::evaluate", false);
    
    El::Stat::TimeMeter Space::evaluate_simple_meter("Space::evaluate_simple",
                                                     false);
    
    El::Stat::TimeMeter Domain::evaluate_meter("Domain::evaluate", false);
    
    El::Stat::TimeMeter Domain::evaluate_simple_meter(
      "Domain::evaluate_simple",
      false);

    El::Stat::TimeMeter Signature::evaluate_meter("Signature::evaluate",
                                                  false);
    
    El::Stat::TimeMeter Signature::evaluate_simple_meter(
      "Signature::evaluate_simple",
      false);
    
    template<>
    El::Stat::TimeMeter
    CapacityBase::evaluate_meter("Capacity::evaluate", false);
    
    template<>
    El::Stat::TimeMeter
    CapacityBase::evaluate_simple_meter("Capacity::evaluate_simple", false);
    
    template<>
    El::Stat::TimeMeter
    ImpressionsBase::evaluate_meter("Impressions::evaluate", false);
    
    template<>
    El::Stat::TimeMeter
    ImpressionsBase::evaluate_simple_meter("Impressions::evaluate_simple",
                                           false);
    
    template<>
    El::Stat::TimeMeter
    FeedImpressionsBase::evaluate_meter("FeedImpressions::evaluate", false);
    
    template<>
    El::Stat::TimeMeter
    FeedImpressionsBase::evaluate_simple_meter(
      "FeedImpressions::evaluate_simple",
      false);
    
    template<>
    El::Stat::TimeMeter
    ClicksBase::evaluate_meter("Clicks::evaluate", false);
    
    template<>
    El::Stat::TimeMeter
    ClicksBase::evaluate_simple_meter("Clicks::evaluate_simple", false);
    
    template<>
    El::Stat::TimeMeter
    FeedClicksBase::evaluate_meter("FeedClicks::evaluate", false);
    
    template<>
    El::Stat::TimeMeter
    FeedClicksBase::evaluate_simple_meter("FeedClicks::evaluate_simple",
                                          false);
    
    template<>
    El::Stat::TimeMeter CTR_Base::evaluate_meter("CTR::evaluate", false);
    
    template<>
    El::Stat::TimeMeter
    CTR_Base::evaluate_simple_meter("CTR::evaluate_simple", false);
    
    template<>
    El::Stat::TimeMeter FeedCTR_Base::evaluate_meter("FeedCTR::evaluate",
                                                     false);
    
    template<>
    El::Stat::TimeMeter
    FeedCTR_Base::evaluate_simple_meter("FeedCTR::evaluate_simple", false);
    
    template<>
    El::Stat::TimeMeter RCTR_Base::evaluate_meter("RCTR::evaluate", false);
    
    template<>
    El::Stat::TimeMeter
    RCTR_Base::evaluate_simple_meter("RCTR::evaluate_simple", false);
    
    template<>
    El::Stat::TimeMeter FeedRCTR_Base::evaluate_meter("FeedRCTR::evaluate",
                                                      false);
    
    template<>
    El::Stat::TimeMeter
    FeedRCTR_Base::evaluate_simple_meter("FeedRCTR::evaluate_simple", false);
    
    //
    // Words struct
    //

    void
    Words::dump(std::wostream& ostr, std::wstring& ident) const
      throw(El::Exception)
    {
      unsigned char group = 0;
      bool exact = false;

      if(word_flags & WF_CORE)
      {
        ostr << "CORE ";
      }
      
      for(WordList::const_iterator it = words.begin(); it != words.end(); it++)
      {
        const Word& word = *it;

        if(group && group != word.group())
        {
          ostr << (exact ? L"\"" : L"'");
        }

        if(it != words.begin())
        {
          ostr << L" ";
        }

        //
        // For optimization purposes for double quoted single word phrases
        // group is set to 0, so need to
        // check for word.group() == 0 && word.exact() cases specifically
        //
        if((word.group() && word.group() != group) ||
           (word.group() == 0 && word.exact()))
        {
          ostr << (word.exact() ? L"\"" : L"'");
        }
          
        std::wstring text;
        El::String::Manip::utf8_to_wchar(word.text.c_str(), text);

        ostr << text;
/*
        if(!word.norm_forms.empty())
        {
          ostr << " [ ";
          for(unsigned long i = 0; i < word.norm_forms.size(); i++)
          {
            ostr << word.norm_forms[i] << " ";
          }
          ostr << "]";
        }
*/
        
        group = word.group() == 0 && word.exact() ? 3 : word.group();
        exact = word.exact();
      }

      if(group)
      {
        ostr << (exact ? L"\"" : L"'");
      }
/*
      if(words.rbegin() != words.rend() && words.rbegin()->group() != 0)
      {
        ostr << (words.rbegin()->exact() ? L"\"" : L"'");
      }
*/
      ostr << std::endl;
    }

    inline
    void
    Words::normalize(const El::Dictionary::Morphology::WordInfoManager&
                     word_info_manager) throw(El::Exception)
    {
      El::Dictionary::Morphology::WordArray words_in;
      words_in.reserve(100);
      
      for(WordList::const_iterator i(words.begin()), e(words.end()); i != e;
          ++i)
      {
        const Word& word = *i;
        words_in.push_back(word.text.c_str());
      }
      
      El::Dictionary::Morphology::WordInfoArray word_infos;      
      word_info_manager.normal_form_ids(words_in, word_infos, 0, false);

      size_t i = 0;
      for(WordList::iterator it(words.begin()),ie(words.end()); it != ie; ++it)
      {
        Word& word = *it;

        const El::Dictionary::Morphology::WordFormArray& w_forms =
          word_infos[i++].forms;
          
        WordIdArray& norm_forms = word.norm_forms;
        norm_forms.resize(w_forms.size());
        word.set_norm_forms_flag();

        size_t j = 0;

        for(El::Dictionary::Morphology::WordFormArray::const_iterator
              fit(w_forms.begin()), fie(w_forms.end()); fit != fie; ++fit, ++j)
        {
          norm_forms[j] = fit->id;
        }
      }
    }
    
    void
    Words::print_phrase(std::ostream& ostr,
                        WordList::const_iterator i,
                        WordList::const_iterator e,
                        size_t len) throw()
    {
      if(len)
      {
        std::string quote = i->exact() ? "\"" : (len > 1 ? "'" : "");

        ostr << quote;
        
        for(WordList::const_iterator j(i); j != e; ++j)
        {
          if(j != i)
          {
            ostr << " ";
          }
          
          ostr << j->text.c_str();
        }

        ostr << quote;
      }
    }
    
    //
    // AnyWords class
    //
    
    void
    AnyWords::normalize(const El::Dictionary::Morphology::WordInfoManager&
                        word_info_manager) throw(El::Exception)
    {
      Words::normalize(word_info_manager);

      if(match_threshold > 1 && words.size() > 1)
      {
        for(WordList::iterator it(words.begin()),ie(words.end()); it != ie;
            ++it)
        {
          it->norm_forms.sort();
        }

        remove_duplicates(true);
      }
    }

    void
    AnyWords::remove_duplicates(bool use_norm_forms) throw()
    {
      if(match_threshold < 2 || words.size() < 2)
      {
        return;
      }
      
      WordList new_words;
      new_words.reserve(words.size());
      
      for(WordList::const_iterator i(words.begin()), e(words.end()); i != e; )
      {
        size_t len1 = 0;
        WordList::const_iterator n(next_phrase(i, e, len1));

        bool skip = false;

        for(WordList::iterator j(new_words.begin()), je(new_words.end());
            j != je; )
        {
          size_t len2 = 0;
          WordList::iterator jn(next_phrase(j, je, len2));

          Word::Relation rel =
            relation(i, n, len1, j, jn, len2, use_norm_forms);

          if(rel == Word::WR_DIFERENT)
          {  
            j = jn;
          }
          else if(rel == Word::WR_LESS_SPECIFIC)
          {
/*            
            print_phrase(std::cout, j, jn, len2);
            std::cout << " -> ";
            print_phrase(std::cout, i, n, len1);
            std::cout << std::endl;
*/          
            copy(jn, je, j);
            new_words.resize(new_words.size() - len2);
            je = new_words.end();
          }          
          else
          {
            // rel == Word::WR_SAME || rel == Word::WR_MORE_SPECIFIC
            skip = true;
/*
            print_phrase(std::cout, i, n, len1);
            std::cout << " -> ";
            print_phrase(std::cout, j, jn, len2);
            std::cout << std::endl;
*/          
            break;
          }
        }

        if(skip)
        {
          i = n;
        }
        else if(len1 == 1)
        {
          new_words.push_back(*i++);
        }
        else
        {
          unsigned char group = 1;

          if(new_words.rbegin() != new_words.rend())
          {
            group = new_words.rbegin()->group() == 1 ? 2 : 1;
          }
          
          for(; i != n; ++i)
          {
            new_words.push_back(*i);
            new_words.rbegin()->group(group);
          }
        }
      }

      words.swap(new_words);
    }
    
    void
    AnyWords::relax(size_t level) throw(El::Exception)
    {
      for(; level; --level)
      {
        if(match_threshold > 1)
        {
          --match_threshold;
        }
        else if(word_flags & WF_CORE)
        {
          word_flags &= ~WF_CORE;
        }
        else
        {
          return;
        }
      }
    }
    
    void
    AnyWords::print(std::ostream& ostr) const throw(El::Exception)
    {      
      ostr << "ANY ";

      assert(words.begin() != words.end());

      bool enforce_quoting = true;

      if(match_threshold > 1)
      {
        ostr << match_threshold << " ";
        enforce_quoting = false;
      }

      if(word_flags & WF_CORE)
      {
        ostr << "CORE ";
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

        ostr << word.text;
        
        group = ((word.group() == 0 && word.exact()) || enforce_quoting) ?
          3 : word.group();
        
        exact = word.exact() || enforce_quoting;
        enforce_quoting = false;
      }
      
      if(group)
      {
        ostr << (exact ? "\"" : "'");
      }
    }
    
    Condition::Result*
    AnyWords::evaluate_one(Context& context,
                           MessageMatchInfoMap& match_info,
                           unsigned long flags) const
      throw(El::Exception)
    {
      const Message::WordToMessageNumberMap& msg_words =
        context.messages.words;
      
      const Message::StoredMessageMap& stored_message =
        context.messages.messages;

      const Message::WordIdToMessageNumberMap& norm_forms =
        context.messages.norm_forms;
      
      ResultList::const_iterator intersect_list_begin =
        context.intersect_list.begin();
      
      ResultList::const_iterator intersect_list_end =
        context.intersect_list.end();

      ResultList::const_iterator skip_list_begin =
        context.skip_list.begin();
      
      ResultList::const_iterator skip_list_end =
        context.skip_list.end();

      MessageFilterList::const_iterator filters_begin =
        context.filters.begin();
      
      MessageFilterList::const_iterator filters_end =
        context.filters.end();

      //
      // Forecasting result hash set size
      //
      
      ResultPtr presult(new Result());
      Result& result = *presult;
      
      match_info.resize(ANY_WORD_POS_RESERVE);

      //
      // Processing standalone words (not in phrase). Just need to 
      // grab messages really containing the word.
      //

      bool search_hidden = (flags & EF_SEARCH_HIDDEN) == EF_SEARCH_HIDDEN;
      bool has_phrases = false;

      for(WordList::const_iterator it(words.begin()), ie(words.end());
          it != ie; ++it)
      {
        const Word& word = *it;

        if(word.use_norm_forms())
        {
          if(word.group() != 0)
          {
            has_phrases = true;
            continue;
          }

          const WordIdArray& nf = word.norm_forms;
          
          for(WordIdArray::const_iterator i(nf.begin()), e(nf.end());
              i < e; ++i)
          {
            El::Dictionary::Morphology::WordId word_id = *i;
            
            Message::WordIdToMessageNumberMap::const_iterator nit =
              norm_forms.find(word_id);

            if(nit == norm_forms.end())
            {
              continue;
            }
            
            const Message::NumberSet& numbers = nit->second->messages;

            for(Message::NumberSet::const_iterator nmit(numbers.begin()),
                  nmit_end(numbers.end()); nmit != nmit_end; ++nmit)
            {
              Message::Number number = *nmit;
            
              const Message::StoredMessage* msg =
                find_msg(number,
                         stored_message,
                         context,
                         search_hidden,
                         intersect_list_begin,
                         intersect_list_end,
                         skip_list_begin,
                         skip_list_end,
                         filters_begin,
                         filters_end);
            
              if(msg != 0 &&
                 satisfy(msg->norm_form_positions.find(word_id)->second, *msg))
              {
                result.insert(std::make_pair(number, msg));
                add_match_info(match_info, flags, number, *msg, *it);
              }
            }
          }          
        }
        else
        {
          Message::WordToMessageNumberMap::const_iterator mit =
            msg_words.find(word.text.c_str());

          if(mit == msg_words.end())
          {
            continue;
          }
        
          if(word.group() != 0)
          {
            has_phrases = true;
            continue;
          }
        
          const Message::NumberSet& numbers = mit->second->messages;

          for(Message::NumberSet::const_iterator nmit(numbers.begin()),
                nmit_end(numbers.end()); nmit != nmit_end; ++nmit)
          {
            Message::Number number = *nmit;
            
            const Message::StoredMessage* msg =
              find_msg(number,
                       stored_message,
                       context,
                       search_hidden,
                       intersect_list_begin,
                       intersect_list_end,
                       skip_list_begin,
                       skip_list_end,
                       filters_begin,
                       filters_end);
            
            if(msg != 0 &&
               satisfy(msg->word_positions.find(word.text)->second, *msg))
            {
              result.insert(std::make_pair(number, msg));
              add_match_info(match_info, flags, number, *msg, *it);
            }
          }
        }
      }

      if(has_phrases)
      {
        //
        // Processing word in phrases.
        //

        AllWords* all = new AllWords();
        Condition_var condition = all;
        all->word_flags = word_flags;

        for(WordList::const_iterator it(words.begin()), ie(words.end());
            it != ie; )
        {        
          unsigned long group = it->group();

          if(group == 0)
          {
            ++it;
            continue;
          }
        
          all->words.clear();
        
          for( ; it != ie && it->group() == group; ++it)
          {
            all->words.push_back(*it);
          }
          
          all->evaluate(context, &result, match_info, flags);
        }
      }
    
      return presult.release();
    }

    struct MatchCounter
    {
      uint32_t last_word;
      uint32_t counter;

      MatchCounter(uint32_t last_word_val = 0) throw();

      MatchCounter(const MatchCounter& src) throw();
      MatchCounter& operator=(const MatchCounter& src) throw();
    };

    inline
    MatchCounter::MatchCounter(uint32_t last_word_val) throw() 
        : last_word(last_word_val),
          counter(1)
    {
    } 

    inline
    MatchCounter::MatchCounter(const MatchCounter& src) throw()
        : last_word(src.last_word),
          counter(src.counter)
    {
    }
    
    inline
    MatchCounter&
    MatchCounter::operator=(const MatchCounter& src) throw()
    {
      last_word = src.last_word;
      counter = src.counter;
      return *this;
    }

    class MatchCounterMap :
      public google::dense_hash_map<Message::Number,
                                    MatchCounter,
                                    El::Hash::Numeric<Message::Number> >
    {
    public:
      MatchCounterMap(unsigned long size) throw(El::Exception);
      MatchCounterMap() throw(El::Exception);
    };

    MatchCounterMap::MatchCounterMap(unsigned long size) throw(El::Exception)
        : google::dense_hash_map<Message::Number,
                                 MatchCounter,
                                 El::Hash::Numeric<Message::Number> >(size)
    {
      set_empty_key(Message::NUMBER_MAX);
      set_deleted_key(Message::NUMBER_MAX - 1);
    }
    
    MatchCounterMap::MatchCounterMap() throw(El::Exception)
    {
      set_empty_key(Message::NUMBER_MAX);
      set_deleted_key(Message::NUMBER_MAX - 1);
    }
    
    Condition::Result*
    AnyWords::evaluate_many(Context& context,
                            MessageMatchInfoMap& match_info,
                            unsigned long flags) const
      throw(El::Exception)
    {
      const Message::WordToMessageNumberMap& msg_words =
        context.messages.words;
      
      const Message::StoredMessageMap& stored_message =
        context.messages.messages;

      const Message::WordIdToMessageNumberMap& norm_forms =
        context.messages.norm_forms;
      
      ResultList::const_iterator intersect_list_begin =
        context.intersect_list.begin();
      
      ResultList::const_iterator intersect_list_end =
        context.intersect_list.end();

      ResultList::const_iterator skip_list_begin =
        context.skip_list.begin();
      
      ResultList::const_iterator skip_list_end =
        context.skip_list.end();

      MessageFilterList::const_iterator filters_begin =
        context.filters.begin();
      
      MessageFilterList::const_iterator filters_end =
        context.filters.end();

      MatchCounterMap match_counter;

      bool search_hidden = (flags & EF_SEARCH_HIDDEN) == EF_SEARCH_HIDDEN;
      bool has_phrases = false;
      size_t word_index = 0;
      
      for(WordList::const_iterator it(words.begin()), ie(words.end());
          it != ie; ++it, ++word_index)
      {
        const Word& word = *it;

        if(word.use_norm_forms())
        {
          if(word.group() != 0)
          {
            has_phrases = true;
            continue;
          }

          const WordIdArray& nf = word.norm_forms;
          
          for(WordIdArray::const_iterator i(nf.begin()), e(nf.end());
              i != e; ++i)
          {
            El::Dictionary::Morphology::WordId word_id = *i;

            Message::WordIdToMessageNumberMap::const_iterator nit =
              norm_forms.find(word_id);

            if(nit == norm_forms.end())
            {
              continue;
            }
            
            const Message::NumberSet& numbers = nit->second->messages;

            for(Message::NumberSet::const_iterator nmit(numbers.begin()),
                  nmit_end(numbers.end()); nmit != nmit_end; ++nmit)
            {
              Message::Number number = *nmit;
              
              const Message::StoredMessage* msg =
                find_msg(number,
                         stored_message,
                         context,
                         search_hidden,
                         intersect_list_begin,
                         intersect_list_end,
                         skip_list_begin,
                         skip_list_end,
                         filters_begin,
                         filters_end);
              
              if(msg != 0 &&
                 satisfy(msg->norm_form_positions.find(word_id)->second, *msg))
              {
                MatchCounterMap::iterator it = match_counter.find(number);
                
                if(it == match_counter.end())
                {
                  match_counter.insert(
                    std::make_pair(number, MatchCounter(word_index)));
                }
                else
                {
                  uint32_t& lw = it->second.last_word;
                  
                  if(lw != word_index)
                  {
                    it->second.counter++;
                    lw = word_index;
                  }
                }              
              }
            }
          }
        }
        else
        {
          Message::WordToMessageNumberMap::const_iterator mit =
            msg_words.find(word.text.c_str());

          if(mit == msg_words.end())
          {
            continue;
          }
        
          if(word.group() != 0)
          {
            has_phrases = true;
            continue;
          }
        
          const Message::NumberSet& numbers = mit->second->messages;

          for(Message::NumberSet::const_iterator nmit(numbers.begin()),
                nmit_end(numbers.end()); nmit != nmit_end; ++nmit)
          {
            Message::Number number = *nmit;

            const Message::StoredMessage* msg =
              find_msg(number,
                       stored_message,
                       context,
                       search_hidden,
                       intersect_list_begin,
                       intersect_list_end,
                       skip_list_begin,
                       skip_list_end,
                       filters_begin,
                       filters_end);
            
            if(msg != 0 &&
               satisfy(msg->word_positions.find(word.text)->second, *msg))
            {
              MatchCounterMap::iterator it = match_counter.find(number);

              if(it == match_counter.end())
              {
                match_counter.insert(
                  std::make_pair(number, MatchCounter(word_index)));
              }
              else
              {
                uint32_t& lw = it->second.last_word;
                
                if(lw != word_index)
                {
                  it->second.counter++;
                  lw = word_index;
                }
              }
            }
          }
        }
      }      
      
      ResultPtr presult(new Result());

      if(has_phrases)
      {
        match_info.resize(std::max((size_t)match_counter.size(),
                                   ANY_WORD_POS_RESERVE));
        
        //
        // Processing word in phrases.
        //

        AllWords* all = new AllWords();
        Condition_var condition = all;
        all->word_flags = word_flags;
        
        Result local_result;
        
        for(WordList::const_iterator it(words.begin()), ie(words.end());
            it != ie; )
        {        
          unsigned long group = it->group();

          if(group == 0)
          {
            ++it;
            continue;
          }
        
          all->words.clear();
        
          for( ; it != ie && it->group() == group; ++it)
          {
            all->words.push_back(*it);
          }

          local_result.clear();
          
          all->evaluate(context,
                        &local_result,
                        match_info,
                        flags);

          for(Result::const_iterator rit(local_result.begin()),
                rit_end(local_result.end()); rit != rit_end; ++rit)
          {
            unsigned long number = rit->first;
            
            presult->insert(*rit);

            MatchCounterMap::iterator it = match_counter.find(number);

            if(it == match_counter.end())
            {
              match_counter.insert(
                std::make_pair(number, MatchCounter()));
            }
            else
            {
              it->second.counter++;
            }
          }
        }

        for(Result::iterator rit(presult->begin()), rit_end(presult->end());
            rit != rit_end; ++rit)
        {
          unsigned long number = rit->first;
          MatchCounterMap::iterator it = match_counter.find(number);

          assert(it != match_counter.end());

          if(it->second.counter < match_threshold)
          {
            match_info.erase(number);
            presult->erase(rit);
          }
        }
      }
      else
      {
        match_info.resize(match_counter.size());
      }

      MatchCounterMap::const_iterator match_counter_end = match_counter.end();

      for(WordList::const_iterator it(words.begin()), ie(words.end());
          it != ie; ++it)
      {
        const Word& word = *it;

        if(word.group() != 0)
        {
          continue;
        }        
        
        if(word.use_norm_forms())
        {
          const WordIdArray& nf = word.norm_forms;
          
          for(WordIdArray::const_iterator i(nf.begin()), e(nf.end());
              i != e; ++i)
          {
            El::Dictionary::Morphology::WordId word_id = *i;

            Message::WordIdToMessageNumberMap::const_iterator nit =
              norm_forms.find(word_id);

            if(nit == norm_forms.end())
            {
              continue;
            }
            
            const Message::NumberSet& numbers = nit->second->messages;

            for(Message::NumberSet::const_iterator nmit(numbers.begin()),
                  nmit_end(numbers.end()); nmit != nmit_end; ++nmit)
            {
              Message::Number number = *nmit;

              {
                MatchCounterMap::const_iterator mit =
                  match_counter.find(number);
                
                if(mit == match_counter_end ||
                   mit->second.counter < match_threshold)
                {
                  continue;
                }
              }
              
              Message::StoredMessageMap::const_iterator mit =
                stored_message.find(number);
          
              assert(mit != stored_message.end());
              const Message::StoredMessage* msg = mit->second;

              if(msg != 0 &&
                 satisfy(msg->norm_form_positions.find(word_id)->second, *msg))
              {
                presult->insert(std::make_pair(number, msg));
                add_match_info(match_info, flags, number, *msg, *it);
              }
            }
          }          
        }
        else
        {
          Message::WordToMessageNumberMap::const_iterator mit =
            msg_words.find(word.text.c_str());

          if(mit == msg_words.end())
          {
            continue;
          }
        
          const Message::NumberSet& numbers = mit->second->messages;

          for(Message::NumberSet::const_iterator nmit(numbers.begin()),
                nmit_end(numbers.end()); nmit != nmit_end; ++nmit)
          {
            Message::Number number = *nmit;

            {
              MatchCounterMap::const_iterator mit =
                match_counter.find(number);

              if(mit == match_counter_end ||
                 mit->second.counter < match_threshold)
              {
                continue;
              }
            }
              
            Message::StoredMessageMap::const_iterator mit =
              stored_message.find(number);
            
            assert(mit != stored_message.end());
            const Message::StoredMessage* msg = mit->second;

            if(msg != 0 &&
               satisfy(msg->word_positions.find(word.text)->second, *msg))
            {
              presult->insert(std::make_pair(number, msg));
              add_match_info(match_info, flags, number, *msg, *it);
            }
          }
        }
      }
      
      return presult.release();
    }

    struct MessageNumberSetList : std::list<const Message::NumberSet*>
    {
      uint32_t size;
      bool has_norm_form;
      
      MessageNumberSetList() : size(0), has_norm_form(false) {}
    };
    
    //
    // AllWords class
    //
    void
    AllWords::relax(size_t level) throw(El::Exception)
    {      
      if(level && (word_flags & WF_CORE))
      {
        word_flags &= ~WF_CORE;
      }
    }
    
    void
    AllWords::print(std::ostream& ostr) const throw(El::Exception)
    {
      if(word_flags & WF_CORE)
      {
        ostr << "ALL CORE ";
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
           (word.group() == 0 && word.exact()))
        {
          ostr << (word.exact() ? "\"" : "'");
        }          

        ostr << word.text;
        
        group = word.group() == 0 && word.exact() ? 3 : word.group();
        exact = word.exact();
      }
      
      if(group)
      {
        ostr << (exact ? "\"" : "'");
      }
    }
    
    Condition::Result*
    AllWords::evaluate(Context& context,
                       Result* res,
                       MessageMatchInfoMap& match_info,
                       unsigned long flags) const
      throw(El::Exception)
    {    
      El::Stat::TimeMeasurement measurement(evaluate_meter);

      typedef std::list<MessageNumberSetList> NumberSetListList;
      NumberSetListList number_sets;
      
      const Message::WordToMessageNumberMap& msg_words =
        context.messages.words;
      
      const Message::StoredMessageMap& stored_message =
        context.messages.messages;
      
      const Message::WordIdToMessageNumberMap& norm_forms =
        context.messages.norm_forms;

      ResultList::const_iterator intersect_list_begin =
        context.intersect_list.begin();
      
      ResultList::const_iterator intersect_list_end =
        context.intersect_list.end();

      ResultList::const_iterator skip_list_begin =
        context.skip_list.begin();
      
      ResultList::const_iterator skip_list_end =
        context.skip_list.end();

      MessageFilterList::const_iterator filters_begin =
        context.filters.begin();
      
      MessageFilterList::const_iterator filters_end =
        context.filters.end();

      bool search_hidden = (flags & EF_SEARCH_HIDDEN) == EF_SEARCH_HIDDEN;
      
      //
      // For each word in ALL condition there is a message number set(s)
      // potentially containing this word. Here we are building a list of
      // such sets ordered by set size so smallest set go first. If for
      // some word the set is empty then the condition do not match.
      //
      for(WordList::const_iterator it = words.begin(); it != words.end(); it++)
      {
        const Word& word = *it;
        MessageNumberSetList number_set_list;

        if(word.use_norm_forms())
        {
          const WordIdArray& nf = word.norm_forms;
          
          for(size_t i = 0; i < nf.size(); i++)
          {
            Message::WordIdToMessageNumberMap::const_iterator nit =
              norm_forms.find(nf[i]);

            if(nit != norm_forms.end())
            {
              const Message::NumberSet& numbers = nit->second->messages;
              unsigned long size = numbers.size();
              
              if(size)
              {
                number_set_list.push_back(&numbers);
                number_set_list.size += size;
              }
            }
          }

          number_set_list.has_norm_form = true;
        }
        else
        {
          Message::WordToMessageNumberMap::const_iterator mit =
            msg_words.find(word.text.c_str());

          if(mit != msg_words.end())
          {
            const Message::NumberSet& numbers = mit->second->messages;
            unsigned long size = numbers.size();
            
            if(size)
            {
              number_set_list.push_back(&numbers);
              number_set_list.size += size;
            }
          }
        }

        if(!number_set_list.size)
        {
          return res ? res : new Result();
        }

        NumberSetListList::iterator nsit = number_sets.begin();
        
        for(; nsit != number_sets.end() &&
              nsit->size < number_set_list.size; nsit++);

        number_sets.insert(nsit, number_set_list);
      }
      
      NumberSetListList::const_iterator nsit = number_sets.begin();

      if(nsit == number_sets.end())
      {
        return res ? res : new Result();
      }

      const MessageNumberSetList& number_set_list = *nsit++;

      ResultPtr presult(res ? 0 : new Result(number_set_list.size));
      Result& result = res ? *res : *presult;

      match_info.resize(number_set_list.size);
      
      //
      // Building list of messages containing all words in
      // ALL condition. We are iterating through the smallest message set
      // (the one related to most rare word) checking if condition evaluates
      // to TRUE on each message.

      for(MessageNumberSetList::const_iterator nit = number_set_list.begin();
          nit != number_set_list.end(); nit++)
      {
        const Message::NumberSet& numbers = **nit;
  
        for(Message::NumberSet::const_iterator it = numbers.begin();
            it != numbers.end(); ++it)
        {          
          Message::Number number = *it;

          if(message_not_in_list(number,
                                 intersect_list_begin,
                                 intersect_list_end) ||
             message_in_list(number, skip_list_begin, skip_list_end))
          {
            continue;
          }

          Message::StoredMessageMap::const_iterator mit  =
            stored_message.find(number);
        
          const Message::StoredMessage* msg = mit->second;

          if(msg->hidden() != search_hidden)
          {
            continue;
          }
          
          MessageFilterList::const_iterator fit = filters_begin;        
          for(; fit != filters_end && (*fit)->satisfy(*msg, context); fit++);
        
          if(fit != filters_end)
          {
            // Filtered out
            continue;
          }

          //
          // Check if message with "number" is contained in other
          // words related message sets
          //
          NumberSetListList::const_iterator i = nsit;

          for(; i != number_sets.end(); i++)
          {
            const MessageNumberSetList& mn_set_list = *i;
            MessageNumberSetList::const_iterator it = mn_set_list.begin();
              
            for(; it != mn_set_list.end(); it++)
            {
              const Message::NumberSet& numbers = **it;
              
              if(numbers.find(number) != numbers.end())
              {
                break;
              }
            }

            if(it == mn_set_list.end())
            {
              // Went through the number set list and didn't find the one
              // containing "number"
              break;
            }  
          }
        
          if(i != number_sets.end())
          {
            // Not all words contained in message
            continue;
          }
        
//          El::Stat::TimeMeasurement measurement2(evaluate_p2_meter);
          
          const Message::MessageWordPosition& msg_word_positions =
            msg->word_positions;

          const Message::WordPositionArray& word_positions = msg->positions;

          const Message::NormFormPosition& norm_form_positions =
            msg->norm_form_positions;

          unsigned long prev_word_group = 0;
          unsigned long word_group = 0;
          unsigned long word_seq_in_group = 0;
            
          std::auto_ptr<PositionSeqSet> group_positions;
          WordList::const_iterator group_wit = words.begin();
          WordList::const_iterator wit = group_wit;
          WordList::const_iterator wit_end = words.end();

          MessageMatchInfo mi;

          //
          // Now as we ensured that message contain all words of ALL operator,
          // we need:
          // * to check if grouped words (those which taken in quotes)
          //   follows in message in specified order;
          // * if core flag set - to check that at least 1 word in phrase
          //   is a core one;
          // * to check that matched words belongs to specified message parts
          //
          // Also we fill message match and position infos.
          //
          
          for(; wit != wit_end; prev_word_group = word_group, ++wit)
          {            
            const Message::MessageWordPosition::KeyValue* wpos = 0;
            
            if(!wit->use_norm_forms())
            {
              const Message::StringConstPtr& word_text = wit->text;
              
              wpos =
                msg_word_positions.find(word_text.c_str(), word_text.hash());

              if(wpos == 0)
              {   
                break;
              }
            }

            const WordIdArray& norm_forms = wit->norm_forms;
            
            word_group = wit->group();
            
            if(word_group == 0)
            {
              if(wpos)
              {
                if(!satisfy(wpos->second, *msg))
                {
                  break;
                }
              }
              else
              {
                WordIdArray::const_iterator i = norm_forms.begin();
                WordIdArray::const_iterator nf_end = norm_forms.end();
                
                for(; i != nf_end; ++i)
                {
                  const Message::NormFormPosition::KeyValue* kw =
                    norm_form_positions.find(*i);

                  if(kw && satisfy(kw->second, *msg))
                  {
                    break;
                  }                  
                }

                if(i == nf_end)
                {
                  break;
                }
              }
              
              add_match_info(mi, flags, *msg, *wit);
              
              if(prev_word_group == 0)
              {
                continue;
              }
            }

            if(!precheck_word_sequence(*msg,
                                       word_group,
                                       prev_word_group,
                                       flags,
                                       wit,
                                       group_wit,
                                       word_seq_in_group,
                                       group_positions,
                                       mi))
            {
              break;
            }

            if(word_group == 0)
            {
              continue;
            }
              
            if(wpos)
            {
              if(!check_word_sequence(word_group,
                                      prev_word_group,
                                      word_seq_in_group,
                                      word_positions,
                                      wpos->second,
                                      group_positions))
              {
                break;
              }
            }
            else
            {
              size_t sequences = 0;
              
              for(WordIdArray::const_iterator i(norm_forms.begin()),
                    e(norm_forms.end()); i != e; ++i)
              {
                const Message::NormFormPosition::KeyValue* nwpos =
                  norm_form_positions.find(*i);

                if(nwpos)
                {                  
                  if(check_word_sequence(
                       word_group,
                       prev_word_group,
                       word_seq_in_group,
                       word_positions,
                       nwpos->second,
                       group_positions))
                  {
                    sequences++;
                  } 
                }
              }

              if(!sequences)
              {
                break;
              }
            }

            postcheck_word_sequence(word_group,
                                    prev_word_group,
                                    word_seq_in_group);
          }

//          measurement2.stop();

//          El::Stat::TimeMeasurement measurement2(evaluate_p2_meter);
          
          if(wit == wit_end && precheck_word_sequence(*msg,
                                                      0,
                                                      prev_word_group,
                                                      flags,
                                                      wit,
                                                      group_wit,
                                                      word_seq_in_group,
                                                      group_positions,
                                                      mi))
          {
            if(!mi.empty())
            {
              match_info[number].steal(mi);
            }
            
            result.insert(std::make_pair(number, msg));
          }
        }        
      }
      
      return res ? res : presult.release();
    }

    //
    // And class
    //
    void
    And::print(std::ostream& ostr) const throw(El::Exception)
    {
      for(ConditionArray::const_iterator b(operands.begin()), i(b),
            e(operands.end()); i != e; ++i)
      {
        if(i != b)
        {
          ostr << " AND ";
        }

        ostr << "( ";
        (*i)->print(ostr);
        ostr << " )";
      }
    }

    Condition::Result*
    And::evaluate_simple(Context& context,
                         MessageMatchInfoMap& match_info,
                         unsigned long flags) const
      throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_simple_meter);
      
      ConditionArray::const_iterator it = operands.begin();

      if(it == operands.end())
      {
        return new Result();
      }

      ResultPtr presult(it->in()->evaluate_simple(context, match_info, flags));
        
      for(++it; it != operands.end(); ++it)
      {
        MessageMatchInfoMap mi;
        
        ResultPtr pres(it->in()->evaluate_simple(context, mi, flags));
        intersect_results(*presult, match_info, flags, *pres, mi);
      }
      
      return presult.release();
    }
    
    Condition::Result*
    And::evaluate(Context& context,
                  MessageMatchInfoMap& match_info,
                  unsigned long flags) const
      throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_meter);
      
      ConditionArray::const_iterator it = operands.begin();

      if(it == operands.end())
      {
        return new Result();
      }

      ResultPtr presult(it->in()->evaluate(context, match_info, flags));

      if(presult->empty())
      {
        return presult.release();
      }
      
      Result& result = *presult;

      for(++it; it != operands.end(); ++it)
      {
        result.resize(0);
        
        ResultList::iterator iwit = context.intersect_list.begin();
        
        for(; iwit != context.intersect_list.end() &&
              (*iwit)->size() < result.size(); iwit++);

        iwit = context.intersect_list.insert(iwit, &result);
       
        MessageMatchInfoMap mi;

        ResultPtr pres(it->in()->evaluate(context, mi, flags));
        intersect_results(result, match_info, flags, *pres, mi);        

        context.intersect_list.erase(iwit);

        if(result.empty())
        {
          return presult.release();
        }        
      }

      return presult.release();
    }

    //
    // Or class
    //
    void
    Or::print(std::ostream& ostr) const throw(El::Exception)
    {
      for(ConditionArray::const_iterator b(operands.begin()), i(b),
            e(operands.end()); i != e; ++i)
      {
        if(i != b)
        {
          ostr << " OR ";
        }

        ostr << "( ";
        (*i)->print(ostr);
        ostr << " )";
      }
    }
    
    Condition::Result*
    Or::evaluate_simple(Context& context,
                        MessageMatchInfoMap& match_info,
                        unsigned long flags) const
      throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_simple_meter);
      
      ConditionArray::const_iterator it = operands.begin();

      if(it == operands.end())
      {
        return new Result();
      }

      ResultPtr presult(
        it->in()->evaluate_simple(context, match_info, flags));
      
      Result& result = *presult;
        
      for(++it; it != operands.end(); ++it)
      {
        MessageMatchInfoMap mi;
        ResultPtr pres(it->in()->evaluate_simple(context, mi, flags));
        
        Result& res = *pres;

        result.resize(result.size() + res.size());

        for(Result::iterator it = res.begin(); it != res.end(); ++it)
        {
          result.insert(*it);
        }

        match_info.unite(mi);
      }
      
      return presult.release();
    }

    Condition::Result*
    Or::evaluate(Context& context,
                 MessageMatchInfoMap& match_info,
                 unsigned long flags) const
      throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_meter);
      
      ConditionArray::const_iterator it = operands.begin();

      if(it == operands.end())
      {
        return new Result();
      }

      ResultPtr presult(it->in()->evaluate(context, match_info, flags));
      Result& result = *presult;
        
      for(++it; it != operands.end(); ++it)
      {
        MessageMatchInfoMap mi;

        ResultPtr pres(it->in()->evaluate(context, mi, flags));
        
        Result& res = *pres;
        result.resize(result.size() + res.size());

        for(Result::iterator rit = res.begin(); rit != res.end(); ++rit)
        {
          result.insert(*rit);
        }

        match_info.unite(mi);
      }
      
      return presult.release();
    }

    //
    // Except class
    //
    void
    Except::print(std::ostream& ostr) const throw(El::Exception)
    {
      ostr << "( ";
      left->print(ostr);
      ostr << " ) EXCEPT ( ";
      right->print(ostr);
      ostr << " )";
    }

    Condition::Result*
    Except::evaluate(Context& context,
                     MessageMatchInfoMap& match_info,
                     unsigned long flags) const
      throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_meter);

      MessageMatchInfoMap mi;
      ResultPtr res2(right->evaluate(context, mi, 0));

      Result& result = *res2;

      ResultList::iterator exit = context.skip_list.begin();
      for(; exit != context.skip_list.end() && (*exit)->size() > result.size();
          exit++);

      exit = context.skip_list.insert(exit, &result);

      ResultPtr res1(left->evaluate(context, match_info, flags));
        
      context.skip_list.erase(exit);
      return res1.release();
    }
    
    Condition::Result*
    Except::evaluate_simple(Context& context,
                            MessageMatchInfoMap& match_info,
                            unsigned long flags)
      const throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_simple_meter);
      
      ResultPtr res1(left->evaluate_simple(context, match_info, flags));

      MessageMatchInfoMap mi;
      ResultPtr res2(right->evaluate_simple(context, mi, 0));

      for(Result::const_iterator it = res2->begin(); it != res2->end(); ++it)
      {
        match_info.erase(it->first);
        res1->erase(it->first);
      }

      return res1.release();
    }

    //
    // Fetched class
    //
    void
    Fetched::print(std::ostream& ostr) const throw(El::Exception)
    {
      condition->print(ostr);

      ostr << " FETCHED ";

      if(reversed)
      {
        ostr << "BEFORE ";
      }

      time.print(ostr);
    }

    Condition::Result*
    Fetched::evaluate_simple(Context& context,
                             MessageMatchInfoMap& match_info,
                             unsigned long flags)
      const throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_simple_meter);
      
      ResultPtr res(
        condition->evaluate_simple(context, match_info, flags));
      
      Result& result = *res;
      unsigned long tm = time.value(context);

      for(Result::iterator it = result.begin(); it != result.end(); ++it)
      {
        unsigned long msg_fetched = it->second->fetched;
        
        if(reversed ? msg_fetched >= tm : msg_fetched < tm)
        {
          match_info.erase(it->first);
          result.erase(it);
        }
      }
      
      return res.release();
    }

    //
    // Visited class
    //
    void
    Visited::print(std::ostream& ostr) const throw(El::Exception)
    {
      condition->print(ostr);

      ostr << " VISITED ";

      if(reversed)
      {
        ostr << "BEFORE ";
      }

      time.print(ostr);
    }

    Condition::Result*
    Visited::evaluate_simple(Context& context,
                             MessageMatchInfoMap& match_info,
                             unsigned long flags)
      const throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_simple_meter);
      
      ResultPtr res(
        condition->evaluate_simple(context, match_info, flags));
      
      Result& result = *res;
      unsigned long tm = time.value(context);

      for(Result::iterator it = result.begin(); it != result.end(); ++it)
      {
        unsigned long msg_visited = it->second->visited;
        
        if(reversed ? msg_visited >= tm : msg_visited < tm)
        {
          match_info.erase(it->first);
          result.erase(it);
        }
      }
      
      return res.release();
    }

    //
    // PubDate class
    //
    void
    PubDate::print(std::ostream& ostr) const throw(El::Exception)
    {
      condition->print(ostr);

      ostr << " DATE ";

      if(reversed)
      {
        ostr << "BEFORE ";
      }

      time.print(ostr);
    }

    Condition::Result*
    PubDate::evaluate_simple(Context& context,
                             MessageMatchInfoMap& match_info,
                             unsigned long flags)
      const throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_simple_meter);
      
      ResultPtr res(condition->evaluate_simple(context, match_info, flags));
      
      Result& result = *res;
      uint32_t tm = time.value(context);

      for(Result::iterator it = result.begin(); it != result.end(); ++it)
      {
        uint32_t msg_published = it->second->published;
        
        if(reversed ? msg_published >= tm : msg_published < tm)
        {
          match_info.erase(it->first);
          result.erase(it);
        }
      }
      
      return res.release();
    }

    //
    // With class
    //
    void
    With::print(std::ostream& ostr) const throw(El::Exception)
    {
      condition->print(ostr);

      ostr << " WITH";

      if(reversed)
      {
        ostr << " NO";
      }

      for(FeatureArray::const_iterator i(features.begin()), e(features.end());
          i != e; ++i)
      {
        ostr << " ";

        switch(*i)
        {
        case FT_IMAGE: ostr << "IMAGE"; break;
        }
      }
    }

    //
    // Lang class
    //
    void
    Lang::print(std::ostream& ostr) const throw(El::Exception)
    {
      condition->print(ostr);

      ostr << " LANGUAGE";

      if(reversed)
      {
        ostr << " NOT";
      }

      for(ValueList::const_iterator i(values.begin()), e(values.end());
          i != e; ++i)
      {
        ostr << " " << i->l3_code();
      }
    }

    //
    // Country class
    //
    void
    Country::print(std::ostream& ostr) const throw(El::Exception)
    {
      condition->print(ostr);

      ostr << " COUNTRY";

      if(reversed)
      {
        ostr << " NOT";
      }

      for(ValueList::const_iterator i(values.begin()), e(values.end());
          i != e; ++i)
      {
        ostr << " " << i->l3_code();
      }
    }

    //
    // Space class
    //
    void
    Space::print(std::ostream& ostr) const throw(El::Exception)
    {
      condition->print(ostr);

      ostr << " SPACE";

      if(reversed)
      {
        ostr << " NOT";
      }

      for(ValueList::const_iterator i(values.begin()), e(values.end());
          i != e; ++i)
      {
        ostr << " ";

        switch(*i)
        {
        case NewsGate::Feed::SP_UNDEFINED: ostr << "undefined"; break;
        case NewsGate::Feed::SP_NEWS: ostr << "news"; break;
        case NewsGate::Feed::SP_TALK: ostr << "talk"; break;
        case NewsGate::Feed::SP_AD: ostr << "ad"; break;
        case NewsGate::Feed::SP_BLOG: ostr << "blog"; break;
        case NewsGate::Feed::SP_ARTICLE: ostr << "article"; break;
        case NewsGate::Feed::SP_PHOTO: ostr << "photo"; break;
        case NewsGate::Feed::SP_VIDEO: ostr << "video"; break;
        case NewsGate::Feed::SP_AUDIO: ostr << "audio"; break;
        case NewsGate::Feed::SP_PRINTED: ostr << "printed"; break;
        default:
          {
            assert(false);
            break;
          }
        }
      }
    }

    //
    // Domain class
    //
    void
    Domain::print(std::ostream& ostr) const throw(El::Exception)
    {
      condition->print(ostr);

      ostr << " DOMAIN";

      if(reversed)
      {
        ostr << " NOT";
      }

      for(DomainList::const_iterator i(domains.begin()), e(domains.end());
          i != e; ++i)
      {
        ostr << " " << i->name;
      }
    }

    //
    // Signature class
    //
    void
    Signature::print(std::ostream& ostr) const throw(El::Exception)
    {
      condition->print(ostr);

      ostr << " SIGNATURE";

      if(reversed)
      {
        ostr << " NOT";
      }

      for(SignatureArray::const_iterator i(values.begin()), e(values.end());
          i != e; ++i)
      {
        ostr << " " << std::uppercase << std::hex << *i << std::nouppercase
             << std::dec;
      }
    }

    //
    // Site class
    //
    void
    Site::print(std::ostream& ostr) const throw(El::Exception)
    {
      ostr << "SITE";

      for(HostNameList::const_iterator i(hostnames.begin()),
            e(hostnames.end()); i != e; ++i)
      {
        ostr << " " << i->c_str();
      }
    }

    Site::SiteOptimizationInfo*
    Site::optimization_info() throw(Exception, El::Exception)
    {
      if(optimization_info_.get() == 0)
      {
        optimization_info_.reset(new SiteOptimizationInfo());

        SiteOptimizationInfo* opt =
          dynamic_cast<SiteOptimizationInfo*>(optimization_info_.get());

        opt->hosts.resize(hostnames.size());
        opt->host_lengths.reserve(hostnames.size());
        
        for(HostNameList::const_iterator i(hostnames.begin()),
              e(hostnames.end()); i != e; ++i)
        {
          const char* host = i->c_str();
          opt->hosts.insert(host);
          opt->host_lengths.push_back(El::Net::ip(host) ? 0 : strlen(host));
        }
      }

      SiteOptimizationInfo* opt =
        dynamic_cast<SiteOptimizationInfo*>(optimization_info_.get());
      
      assert(opt != 0);
      return opt;
    }
    
    Condition::Result*
    Site::evaluate(Context& context,
                   MessageMatchInfoMap& match_info,
                   unsigned long flags) const
      throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_meter);

      const Message::SiteToMessageNumberMap& sites = context.messages.sites;

      size_t size_forecast = 0;

      for(HostNameList::const_iterator it = hostnames.begin();
          it != hostnames.end(); it++)
      {
        Message::SiteToMessageNumberMap::const_iterator sit =
          sites.find(it->c_str());

        if(sit == sites.end())
        {
          continue;
        }

        size_forecast += sit->second->size();
      }

      if(!size_forecast)
      {
        return new Result();
      }
      
      const Message::StoredMessageMap& stored_message =
        context.messages.messages;

      ResultList::const_iterator intersect_list_begin =
        context.intersect_list.begin();
      
      ResultList::const_iterator intersect_list_end =
        context.intersect_list.end();

      ResultList::const_iterator skip_list_begin =
        context.skip_list.begin();
      
      ResultList::const_iterator skip_list_end =
        context.skip_list.end();

      MessageFilterList::const_iterator filters_begin =
        context.filters.begin();
      
      MessageFilterList::const_iterator filters_end =
        context.filters.end();

      bool search_hidden = (flags & EF_SEARCH_HIDDEN) == EF_SEARCH_HIDDEN;
      
      ResultPtr presult(new Result(size_forecast));
      Result& result = *presult;

      for(HostNameList::const_iterator it = hostnames.begin();
          it != hostnames.end(); it++)
      {
        const char* hostname = it->c_str();

        Message::SiteToMessageNumberMap::const_iterator sit =
          sites.find(hostname);

        if(sit == sites.end())
        {
          continue;
        }
        
        const Message::NumberSet& numbers = *sit->second;

        for(Message::NumberSet::const_iterator nmit = numbers.begin();
            nmit != numbers.end(); ++nmit)
        {
          Message::Number number = *nmit;

          if(message_not_in_list(number,
                                 intersect_list_begin,
                                 intersect_list_end) ||
             message_in_list(number, skip_list_begin, skip_list_end))
          {
            continue;
          }

          Message::StoredMessageMap::const_iterator mit  =
            stored_message.find(number);
          
          if(mit == stored_message.end())
          {      
            continue;
          }

          const Message::StoredMessage* msg = mit->second;

          if(msg->hidden() != search_hidden)
          {
            continue;
          }
          
          MessageFilterList::const_iterator fit = filters_begin;
          
          for(; fit != filters_end && (*fit)->satisfy(*msg, context); fit++);

          if(fit != filters_end)
          {
            // Filtered out
            continue;
          }

          if(strcmp(hostname, msg->hostname.c_str()) == 0)
          {   
            result.insert(std::make_pair(number, msg));
          }
        }
      }
      
      return presult.release();
    }
    
    //
    // Url class
    //
    void
    Url::print(std::ostream& ostr) const throw(El::Exception)
    {
      ostr << "URL";

      for(UrlList::const_iterator i(urls.begin()), e(urls.end()); i != e; ++i)
      {
        ostr << " " << i->c_str();
      }
    }

    Url::UrlOptimizationInfo*
    Url::optimization_info() throw(Exception, El::Exception)
    {
      if(optimization_info_.get() == 0)
      {
        optimization_info_.reset(new UrlOptimizationInfo());

        UrlOptimizationInfo* opt =
          dynamic_cast<UrlOptimizationInfo*>(optimization_info_.get());

        opt->urls.resize(urls.size());
        opt->sites.reserve(urls.size());
        
        for(UrlList::const_iterator i(urls.begin()), e(urls.end()); i != e;
            ++i)
        {
          const char* url = i->c_str();
          opt->urls.insert(url);

          El::Net::HTTP::URL_var u = new El::Net::HTTP::URL(url);

          UrlOptimizationInfo::UrlSite us;
          us.name = u->host();
          us.len = El::Net::ip(us.name.c_str()) ? 0 : us.name.length();
            
          opt->sites.push_back(us);
        }
      }

      UrlOptimizationInfo* opt =
        dynamic_cast<UrlOptimizationInfo*>(optimization_info_.get());
      
      assert(opt != 0);
      return opt;
    }
    
    Condition::Result*
    Url::evaluate(Context& context,
                  MessageMatchInfoMap& match_info,
                  unsigned long flags) const
      throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_meter);

      const Message::FeedInfoMap& msg_feeds = context.messages.feeds;

      size_t size_forecast = 0;

      for(UrlList::const_iterator it = urls.begin(); it != urls.end(); it++)
      {
        Message::FeedInfoMap::const_iterator fit = msg_feeds.find(it->c_str());

        if(fit == msg_feeds.end())
        {
          continue;
        }

        size_forecast += fit->second->messages.size();
      }

      if(!size_forecast)
      {
        return new Result();
      }
      
      const Message::StoredMessageMap& stored_message =
        context.messages.messages;

      ResultList::const_iterator intersect_list_begin =
        context.intersect_list.begin();
      
      ResultList::const_iterator intersect_list_end =
        context.intersect_list.end();

      ResultList::const_iterator skip_list_begin =
        context.skip_list.begin();
      
      ResultList::const_iterator skip_list_end =
        context.skip_list.end();

      MessageFilterList::const_iterator filters_begin =
        context.filters.begin();
      
      MessageFilterList::const_iterator filters_end =
        context.filters.end();

      bool search_hidden = (flags & EF_SEARCH_HIDDEN) == EF_SEARCH_HIDDEN;
      
      ResultPtr presult(new Result(size_forecast));
      Result& result = *presult;

      for(UrlList::const_iterator it = urls.begin(); it != urls.end(); it++)
      {
        const char* url = it->c_str();
        
        Message::FeedInfoMap::const_iterator fit = msg_feeds.find(url);

        if(fit == msg_feeds.end())
        {
          continue;
        }

        const Message::MessageWordMap& numbers = fit->second->messages;

        for(Message::MessageWordMap::const_iterator nmit = numbers.begin();
            nmit != numbers.end(); ++nmit)
        {
          Message::Number number = nmit->first;

          if(message_not_in_list(number,
                                 intersect_list_begin,
                                 intersect_list_end) ||
             message_in_list(number, skip_list_begin, skip_list_end))
          {
            continue;
          }

          Message::StoredMessageMap::const_iterator mit  =
            stored_message.find(number);
          
          if(mit == stored_message.end())
          {      
            continue;
          }

          const Message::StoredMessage* msg = mit->second;

          if(msg->hidden() != search_hidden)
          {
            continue;
          }
          
          MessageFilterList::const_iterator fit = filters_begin;
          
          for(; fit != filters_end && (*fit)->satisfy(*msg, context); fit++);

          if(fit != filters_end)
          {
            // Filtered out
            continue;
          }

          if(strcmp(url, msg->source_url.c_str()) == 0)
          {   
            result.insert(std::make_pair(number, msg));
          }
        }
      }
      
      return presult.release();
    }

    //
    // Category class
    //
    void
    Category::print(std::ostream& ostr) const throw(El::Exception)
    {
      ostr << "CATEGORY";

      for(CategoryArray::const_iterator i(categories.begin()),
            e(categories.end()); i != e; ++i)
      {
        ostr << " \"" << i->c_str() << "\"";
      }
    }

    Condition::Result*
    Category::evaluate(Context& context,
                       MessageMatchInfoMap& match_info,
                       unsigned long flags) const
      throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_meter);

      bool search_hidden = (flags & EF_SEARCH_HIDDEN) == EF_SEARCH_HIDDEN;
      ResultPtr presult(new Result());

      for(size_t i = 0; i < categories.size(); i++)
      {
        const Message::Category* category =
          context.messages.root_category->find(categories[i].c_str() + 1);

        if(category)
        {
          get_messages(context, search_hidden, category, *presult);
        }
      }
      
      return presult.release();
    }

    void
    Category::get_messages(Context& context,
                           bool search_hidden,
                           const Message::Category* category,
                           Result& result) const
      throw(El::Exception)
    {
      const Message::StoredMessageMap& stored_message =
        context.messages.messages;
      
      ResultList::const_iterator intersect_list_begin =
        context.intersect_list.begin();
      
      ResultList::const_iterator intersect_list_end =
        context.intersect_list.end();

      ResultList::const_iterator skip_list_begin =
        context.skip_list.begin();
      
      ResultList::const_iterator skip_list_end =
        context.skip_list.end();

      MessageFilterList::const_iterator filters_begin =
        context.filters.begin();
      
      MessageFilterList::const_iterator filters_end =
        context.filters.end();

//      const Message::NumberSet& numbers = category->messages;
      const Message::MessageMap& numbers = category->messages;
      
//      for(Message::NumberSet::const_iterator nmit = numbers.begin();
      for(Message::MessageMap::const_iterator nmit = numbers.begin();
          nmit != numbers.end(); ++nmit)
      {
//        Message::Number number = *nmit;
        Message::Number number = nmit->first;

        if(message_not_in_list(number,
                               intersect_list_begin,
                               intersect_list_end) ||
           message_in_list(number, skip_list_begin, skip_list_end))
        {
          continue;
        }

        Message::StoredMessageMap::const_iterator mit  =
          stored_message.find(number);
          
        if(mit == stored_message.end())
        {      
          continue;
        }

        const Message::StoredMessage* msg = mit->second;

        if(msg->hidden() != search_hidden)
        {
          continue;
        }
        
        MessageFilterList::const_iterator fit = filters_begin;          
        for(; fit != filters_end && (*fit)->satisfy(*msg, context); fit++);

        if(fit != filters_end)
        {
          // Filtered out
          continue;
        }

        result.insert(std::make_pair(number, msg));
      }
/*
      const Message::CategoryMap& children = category->children;
      
      for(Message::CategoryMap::const_iterator it = children.begin();
          it != children.end(); it++)
      {
        get_messages(context, search_hidden, it->second.in(), result);
      }
*/
    }

    //
    // Msg class
    //
    void
    Msg::print(std::ostream& ostr) const throw(El::Exception)
    {
      ostr << "MSG";

      for(Message::IdArray::const_iterator i(ids.begin()), e(ids.end());
          i != e; ++i)
      {
        ostr << " " << i->string();
      }
    }

    Condition::Result*
    Msg::evaluate(Context& context,
                  MessageMatchInfoMap& match_info,
                  unsigned long flags) const
      throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_meter);

      const Message::IdToNumberMap& id_to_number =
        context.messages.id_to_number;

      size_t size_forecast = 0;

      for(Message::IdArray::const_iterator it = ids.begin(); it != ids.end();
          it++)
      {
        if(id_to_number.find(*it) != id_to_number.end())
        {
          size_forecast++;
        }
      }

      if(!size_forecast)
      {
        return new Result();
      }
      
      const Message::StoredMessageMap& stored_message =
        context.messages.messages;

      ResultList::const_iterator intersect_list_begin =
        context.intersect_list.begin();
      
      ResultList::const_iterator intersect_list_end =
        context.intersect_list.end();

      ResultList::const_iterator skip_list_begin =
        context.skip_list.begin();
      
      ResultList::const_iterator skip_list_end =
        context.skip_list.end();

      MessageFilterList::const_iterator filters_begin =
        context.filters.begin();
      
      MessageFilterList::const_iterator filters_end =
        context.filters.end();

      bool search_hidden = (flags & EF_SEARCH_HIDDEN) == EF_SEARCH_HIDDEN;
      
      ResultPtr presult(new Result(size_forecast));
      Result& result = *presult;

      for(Message::IdArray::const_iterator it = ids.begin(); it != ids.end();
          it++)
      {
        Message::IdToNumberMap::const_iterator iit = id_to_number.find(*it);
        
        if(iit == id_to_number.end())
        {
          continue;
        }

        Message::Number number = iit->second;

        if(message_not_in_list(number,
                               intersect_list_begin,
                               intersect_list_end) ||
           message_in_list(number, skip_list_begin, skip_list_end))
        {
          continue;
        }

        Message::StoredMessageMap::const_iterator mit  =
          stored_message.find(number);
          
        if(mit == stored_message.end())
        {      
          continue;
        }

        const Message::StoredMessage* msg = mit->second;

        if(msg->hidden() != search_hidden)
        {
          continue;
        }
        
        MessageFilterList::const_iterator fit = filters_begin;  
        for(; fit != filters_end && (*fit)->satisfy(*msg, context); fit++);

        if(fit != filters_end)
        {
          // Filtered out
          continue;
        }

        result.insert(std::make_pair(number, msg));
      }
      
      return presult.release();
    }

    //
    // Event class
    //
    void
    Event::print(std::ostream& ostr) const throw(El::Exception)
    {
      ostr << "EVENT";

      for(LuidArray::const_iterator i(ids.begin()), e(ids.end()); i != e; ++i)
      {
        ostr << " " << i->string();
      }
    }

    Condition::Result*
    Event::evaluate(Context& context,
                    MessageMatchInfoMap& match_info,
                    unsigned long flags) const
      throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_meter);

      const Message::EventToNumberMap& event_to_number =
        context.messages.event_to_number;

      size_t size_forecast = 0;

      for(LuidArray::const_iterator it = ids.begin(); it != ids.end(); it++)
      {
        Message::EventToNumberMap::const_iterator eit =
          event_to_number.find(*it);
        
        if(eit != event_to_number.end())
        {
          size_forecast += eit->second->size();
        }
      }

      if(!size_forecast)
      {
        return new Result();
      }
      
      const Message::StoredMessageMap& stored_message =
        context.messages.messages;

      ResultList::const_iterator intersect_list_begin =
        context.intersect_list.begin();
      
      ResultList::const_iterator intersect_list_end =
        context.intersect_list.end();

      ResultList::const_iterator skip_list_begin =
        context.skip_list.begin();
      
      ResultList::const_iterator skip_list_end =
        context.skip_list.end();

      MessageFilterList::const_iterator filters_begin =
        context.filters.begin();
      
      MessageFilterList::const_iterator filters_end =
        context.filters.end();

      bool search_hidden = (flags & EF_SEARCH_HIDDEN) == EF_SEARCH_HIDDEN;
      
      ResultPtr presult(new Result(size_forecast));
      Result& result = *presult;

      for(LuidArray::const_iterator it = ids.begin(); it != ids.end(); it++)
      {
        Message::EventToNumberMap::const_iterator iit =
          event_to_number.find(*it);
        
        if(iit == event_to_number.end())
        {
          continue;
        }

        const Message::NumberSet& numbers = *iit->second;

        for(Message::NumberSet::const_iterator nit = numbers.begin();
            nit != numbers.end(); nit++)
        {
          Message::Number number = *nit;

          if(message_not_in_list(number,
                                 intersect_list_begin,
                                 intersect_list_end) ||
             message_in_list(number, skip_list_begin, skip_list_end))
          {
            continue;
          }

          Message::StoredMessageMap::const_iterator mit  =
            stored_message.find(number);
          
          if(mit == stored_message.end())
          {      
            continue;
          }

          const Message::StoredMessage* msg = mit->second;

          if(msg->hidden() != search_hidden)
          {
            continue;
          }
          
          MessageFilterList::const_iterator fit = filters_begin;          
          for(; fit != filters_end && (*fit)->satisfy(*msg, context); fit++);

          if(fit != filters_end)
          {
            // Filtered out
            continue;
          }

          result.insert(std::make_pair(number, msg));
        }
      }
      
      return presult.release();
    }

    //
    // Event class
    //
    Condition::Result*
    Every::evaluate(Context& context,
                    MessageMatchInfoMap& match_info,
                    unsigned long flags) const
      throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(evaluate_meter);

      ResultList::const_iterator intersect_list_begin =
        context.intersect_list.begin();
      
      ResultList::const_iterator intersect_list_end =
        context.intersect_list.end();

      ResultList::const_iterator skip_list_begin =
        context.skip_list.begin();
      
      ResultList::const_iterator skip_list_end =
        context.skip_list.end();

      MessageFilterList::const_iterator filters_begin =
        context.filters.begin();
      
      MessageFilterList::const_iterator filters_end =
        context.filters.end();

      const Message::StoredMessageMap& stored_message =
        context.messages.messages;
      
      bool search_hidden = (flags & EF_SEARCH_HIDDEN) == EF_SEARCH_HIDDEN;
      
      ResultPtr presult(new Result(stored_message.size()));

      Message::StoredMessageMap::const_iterator end = stored_message.end();

      for(Message::StoredMessageMap::const_iterator it =
            stored_message.begin(); it != end; ++it)
      {
        Message::Number number = it->first;

        if(message_not_in_list(number,
                               intersect_list_begin,
                               intersect_list_end) ||
           message_in_list(number, skip_list_begin, skip_list_end))
        {
          continue;
        }

        const Message::StoredMessage* msg = it->second;

        if(msg->hidden() != search_hidden)
        {
          continue;
        }
        
        MessageFilterList::const_iterator fit = filters_begin;  
        for(; fit != filters_end && (*fit)->satisfy(*msg, context); fit++);

        if(fit != filters_end)
        {
          // Filtered out
          continue;
        }

        presult->insert(std::make_pair(number, msg));
      }
      
      return presult.release();
    }

    //
    // Condition::Time struct
    //
    Condition::Time::Time(const wchar_t* val) throw(InvalidArg, El::Exception)
    {
      std::string tm;
      El::String::Manip::wchar_to_utf8(val, tm);
      
      if(tm[0] == '\0')
      {
        throw InvalidArg(
          "NewsGate::Search::Condition::Time::Time: date/time expected");
      }

      char* end = 0;
      time_t tval = strtol(tm.c_str(), &end, 10);

      if(*end == '\0')
      {
        offset = tval;
        type = OT_SINCE_EPOCH;
        return;
      }

      unsigned long scale = 0;

      switch(*end)
      {
      case 'D': case 'd': scale = 86400; break;
      case 'H': case 'h': scale = 3600; break;
      case 'M': case 'm': scale = 60; break;
      case 'S': case 's': scale = 1; break;
      }
        
      end++;
      
      if(*end == '\0' && scale)
      {
        offset = tval * scale;
        type = OT_FROM_NOW;
        return;
      }

      std::string date;
      size_t pos = tm.find('.');
      
      if(pos == std::string::npos)
      {
        date = tm;
      }
      else
      {
        date.assign(tm.c_str(), pos);
        std::string time(tm.c_str() + pos + 1);

        if(time.empty())
        {
          date.clear();
        }
        else
        {
          date += std::string(" ") + time;
        }
      }

      size_t len = date.length();

      if(len == 10 || len == 19)
      {
        try
        {
          El::Moment m(date.c_str(), El::Moment::TF_ISO_8601);
          
          offset = ((ACE_Time_Value)m).sec();
          type = OT_SINCE_EPOCH;
          return;
        }
        catch(const El::Moment::InvalidArg&)
        {
        }
      }
      
      std::ostringstream ostr;
      ostr << "NewsGate::Search::Condition::Time::Time: unexpected time "
        "format for '" << tm << "'";
        
      throw InvalidArg(ostr.str());
    }
    
    void
    Condition::Time::print(std::ostream& ostr) const throw(El::Exception)
    {
      switch(type)
      {
      case OT_SINCE_EPOCH:
        {
          El::Moment m((ACE_Time_Value)offset);
          
          ostr << m.tm_year + 1900 << "-" << std::setw(2)
               << std::setfill('0') << m.tm_mon + 1 << "-" << std::setw(2)
               << m.tm_mday;

          if(m.tm_hour || m.tm_min || m.tm_sec)
          {
            ostr << "." << std::setw(2) << m.tm_hour << ":" << std::setw(2)
                 << m.tm_min << ":" << std::setw(2) << m.tm_sec;
          }
          
          break;
        }
      case OT_FROM_NOW:
        {
          if((offset % 86400) == 0)
          {
            ostr << offset / 86400 << "D";
            return;
          }
          
          if((offset % 3600) == 0)
          {
            ostr << offset / 3600 << "H";
            return;
          }
            
          if((offset % 60) == 0)
          {
            ostr << offset / 60 << "M";
            return;
          }

          ostr << offset << "S";
          break;
        }
      }
    }

    void
    Condition::Time::dump(std::wostream& ostr) const throw(El::Exception)
    {
      switch(type)
      {
      case OT_SINCE_EPOCH:
        {
          El::Moment m((ACE_Time_Value)offset);
          
          ostr << m.tm_year + 1900 << L"-" << std::setw(2)
               << std::setfill(L'0') << m.tm_mon + 1 << L"-" << std::setw(2)
               << m.tm_mday;

          if(m.tm_hour || m.tm_min || m.tm_sec)
          {
            ostr << L"." << std::setw(2) << m.tm_hour << L":" << std::setw(2)
                 << m.tm_min << L":" << std::setw(2) << m.tm_sec;
          }
          
          break;
        }
      case OT_FROM_NOW:
        {
          if((offset % 86400) == 0)
          {
            ostr << offset / 86400 << L"D";
            return;
          }
          
          if((offset % 3600) == 0)
          {
            ostr << offset / 3600 << L"H";
            return;
          }
            
          if((offset % 60) == 0)
          {
            ostr << offset / 60 << L"M";
            return;
          }

          ostr << offset << L"S";
          break;
        }
      }
    }

  }
}
