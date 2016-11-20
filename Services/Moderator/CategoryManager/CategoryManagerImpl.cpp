
/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/CategoryManager/CategoryManagerImpl.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <ctype.h>

#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <memory>
#include <set>
#include <map>

#include <ace/OS.h>
#include <mysql/mysqld_error.h>

#include <El/Exception.hpp>
#include <El/Moment.hpp>
#include <El/Lang.hpp>
#include <El/String/Manip.hpp>
#include <El/String/ListParser.hpp>
#include <El/Luid.hpp>

#include <Commons/Search/SearchExpression.hpp>

#include <Services/Commons/Message/BankClientSessionImpl.hpp>

#include <Services/Commons/Category/WordListResolver.hpp>
#include <Services/Moderator/ChangeLog/CategoryChange.hpp>
#include <Services/Dictionary/Commons/TransportImpl.hpp>

#include "CategoryManagerImpl.hpp"
#include "CategoryManagerMain.hpp"
#include "DB_Record.hpp"

namespace NewsGate
{
  namespace Moderation
  {    
    namespace Category
    {
      const uint64_t ROOT_CATEGORY_ID = 1;
      
      //
      // ManagerImpl class
      //
      ManagerImpl::ManagerImpl(El::Service::Callback* callback)
        throw(InvalidArgument, Exception, El::Exception)
          : El::Service::CompoundService<>(
            callback,
            "CategoryManagerImpl")
      {
      }

      inline
      bool
      po_cmp(const Phrase& a, const Phrase& b)
      {
        if(a.occurances_freq_excess != b.occurances_freq_excess)
        {
          return a.occurances_freq_excess > b.occurances_freq_excess;
        }

        return a.words.size() < b.words.size();
      } 
      
      ::NewsGate::Moderation::Category::RelevantPhrases*
      ManagerImpl::category_relevant_phrases(
        ::NewsGate::Moderation::Category::CategoryId id,
        const char* lang,
        const ::NewsGate::Moderation::Category::PhraseIdSeq& skip_phrase_ids)
      throw(NewsGate::Moderation::Category::CategoryNotFound,
            NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
      {
        ACE_Time_Value total_time = ACE_OS::gettimeofday();
        
        const Server::Config::CategoryManagerType::relevant_phrases_type&
          config = Application::instance()->config().relevant_phrases();

        ACE_Time_Value phrase_sorting_time;
        ACE_Time_Value search_time;
        ACE_Time_Value phrase_counting_time;
        ACE_Time_Value usefulness_calc_time;
        ACE_Time_Value category_parsing_time = ACE_OS::gettimeofday();

        size_t relevant_message_count = 0;
        size_t irrelevant_message_count = 0;
        
        RelevantPhrases_var result = new RelevantPhrases();
        
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();            
          
          CategoryDescriptor_var category;

          try
          {
            category = get_category(id, connection);
          }
          catch(const CategoryNotFound&)
          {
            throw;
          }
          catch(const NoPath& e)
          {
            std::ostringstream ostr;
            ostr << "unexpected NoPath (" << e.id << "/" <<
              e.name <<") caught for category " << id;

            throw Exception(ostr.str());
          }
          catch(const Cycle& e)
          {
            std::ostringstream ostr;
            ostr << "unexpected Cycle (" << e.id << "/" <<
              e.name <<") caught for category " << id;

            throw Exception(ostr.str());
          }

          PhraseSet any_phrases;
          PhraseSet any2_phrases;
          PhraseSet skip_phrases;
          PhraseSet skip2_phrases;
          CategoryIdSet processed_categories;
          
          category_phrases(id,
                           lang,
                           &any_phrases,
                           &any2_phrases,
                           &skip_phrases,
                           &skip2_phrases,
                           connection.in(),
                           processed_categories);

          std::string query = category_expression(id,
                                                  connection.in(),
                                                  category->children);

          result->query = query.c_str();

//          std::cerr << "!!!\n" << query << "\n???\n" << result->query.in();

//          std::cerr << query << std::endl;

          category_parsing_time =
            ACE_OS::gettimeofday() - category_parsing_time;
          
          phrase_counting_time = ACE_OS::gettimeofday();
          PhraseCounter relevant_phrase_counter;
          
          result->relevant_message_count =
            count_phrases(query.c_str(),
                          lang,
                          relevant_phrase_counter,
                          relevant_message_count,
                          search_time,
                          false);

          PhraseCounter irrelevant_phrase_counter;

          {
            std::ostringstream ostr;
            ostr << "EVERY EXCEPT ( " << query << " )";

            result->irrelevant_message_count =
              count_phrases(ostr.str().c_str(),
                            lang,
                            irrelevant_phrase_counter,
                            irrelevant_message_count,
                            search_time,
                            true);
          }
/*
          for(PhraseCounter::const_iterator
                i(relevant_phrase_counter.begin()),
                e(relevant_phrase_counter.end()); i != e; ++i)
          {
            const Phrase& p = i->first;

//            if(strcasestr(p.text().c_str(), "video monitor"))
            {
              std::cerr << "!!! ";
              p.dump(std::cerr);
              std::cerr << std::endl;
            }
          }
*/
          phrase_counting_time = ACE_OS::gettimeofday() - phrase_counting_time -
            search_time;
          
          if(!irrelevant_message_count)
          {
            return result._retn();
          }

          phrase_sorting_time = ACE_OS::gettimeofday();

          remove_general_phrases(any_phrases, relevant_phrase_counter);
          remove_general_phrases(any2_phrases, relevant_phrase_counter);

          PhraseIdSet skip_set;

          for(size_t i = 0; i < skip_phrase_ids.length(); ++i)
          {
            skip_set.insert(skip_phrase_ids[i]);
          }

          PhraseArray phrases;
          phrases.reserve(relevant_phrase_counter.size() / 2);

          for(PhraseCounter::const_iterator
                i(relevant_phrase_counter.begin()),
                e(relevant_phrase_counter.end()); i != e; ++i)
          {
            const Phrase& p = i->first;

            if(skip_set.find(p.id) != skip_set.end())
            {
              continue;
            }
/*
            if(strcasestr(p.text().c_str(), "video monitor"))
            {
              std::cerr << "!!! ";
              p.dump(std::cerr);
              std::cerr << std::endl;
            }
*/

            PhraseArray subs;
            p.subphrases(subs, true);

            bool found = false;
            bool found2 = false;

            for(PhraseArray::const_iterator j(subs.begin()), je(subs.end());
                j != je && (!found || !found2); ++j)
            {
              const Phrase& p = *j;
              
              found |= any_phrases.find(p) != any_phrases.end() ||
                skip_phrases.find(p) != skip_phrases.end() ||
                ((p.flags & Phrase::PF_TRIVIAL_COMPLEMENT) &&
                 (any2_phrases.find(p) != any2_phrases.end() ||
                  skip2_phrases.find(p) != skip2_phrases.end()));

              found2 |= any2_phrases.find(p) != any2_phrases.end();
            }

            if(found)
            {
/*              
              std::cerr << "--- ";
              p.dump(std::cerr);
              std::cerr << std::endl;
*/
              // Do not suggest phrase if more general already present
              continue;
            }

            if(any2_phrases.find(p) != any2_phrases.end() ||
               skip2_phrases.find(p) != skip2_phrases.end())
            {
/*              
              std::cerr << "||| ";
              p.dump(std::cerr);
              std::cerr << std::endl;
*/
              continue;
            }

            size_t ro = i->second;
              
            if(ro > 1)
            {              
              PhraseCounter::const_iterator j =
                irrelevant_phrase_counter.find(p);
              
              size_t iro = j == irrelevant_phrase_counter.end() ?
                0 : j->second;

              double rf = (double)ro / relevant_message_count;
              double irf = (double)iro / irrelevant_message_count;

              if(rf > irf)
              {
                Phrase po(p);
                  
                po.occurances = ro;
                po.occurances_irrelevant = iro;
//                po.occurances_freq = rf;
//                po.occurances_irrelevant_freq = irf;
                  
                po.occurances_freq_excess = //log(ro) *
                  log(iro + 2) *
                  rf / (irf > 0 ? irf :
                        (0.5 / irrelevant_message_count));

                if(found2)
                {
//                Insistently suggest more specific phrase for a general one
                  po.occurances_freq_excess *= 1.2;
                }

                phrases.push_back(po);
              }
            }
          }
            
          std::sort(phrases.begin(), phrases.end(), &po_cmp);

          {
            PhraseArray phrases2;
            phrases2.reserve(phrases.size());
            
            for(PhraseArray::const_iterator b(phrases.begin()), i(b),
                  e(phrases.end()); i != e; ++i)
            {
              const Phrase& p = *i;
              bool add = true;
              
              if(i != b)
              {
                PhraseArray::const_iterator j(b);
                for(; j != i && !p.contains(*j); ++j);

                add = j == i;
              }
              
              if(add)
              {
                phrases2.push_back(p);
              }
            }

            phrases.swap(phrases2);
          }
            
          size_t phrase_count =
            std::min(phrases.size(), (size_t)config.max_phrase_count());
            
          PhraseOccuranceSeq& res_phrases = result->phrases;
          res_phrases.length(phrase_count);

          PhraseArray::const_iterator j(phrases.begin());
            
          for(size_t i = 0; i < phrase_count; ++i, ++j)
          {
            const Phrase& p = *j;
            PhraseOccurance& po = res_phrases[i];
                
            po.phrase = p.text().c_str();
            po.id = p.id;
            po.occurances = p.occurances;
            po.total_irrelevant_occurances = 0;
            po.occurances_irrelevant = p.occurances_irrelevant;
//            po.occurances_freq = p.occurances_freq;
//            po.occurances_irrelevant_freq = p.occurances_irrelevant_freq;
            po.occurances_freq_excess = p.occurances_freq_excess;
          }

          phrase_sorting_time = ACE_OS::gettimeofday() - phrase_sorting_time;

          usefulness_calc_time = ACE_OS::gettimeofday();

          set_phrase_usefulness_flag(query.c_str(), lang, res_phrases);

          usefulness_calc_time = ACE_OS::gettimeofday() - usefulness_calc_time;
          
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Category::ManagerImpl::"
                "category_relevant_phrases: El::Exception caught. "
            "Description:\n" << e;

          ImplementationException ex;
          ex.description = CORBA::string_dup(ostr.str().c_str());

          throw ex;
        }
        catch(const NewsGate::Moderation::Category::ExpressionParseError& e)
        {
          ImplementationException ex;
          ex.description = e.description;

          throw ex;
        }

        total_time = ACE_OS::gettimeofday() - total_time;

        result->category_parsing_time = category_parsing_time.sec();
        result->total_time = total_time.sec();
        result->phrase_counting_time = phrase_counting_time.sec();
        result->phrase_sorting_time = phrase_sorting_time.sec();
        result->search_time = search_time.sec();
        result->usefulness_calc_time = usefulness_calc_time.sec();
        
        return result._retn();
      }

      void
      ManagerImpl::remove_general_phrases(const PhraseSet& phrases,
                                          PhraseCounter& relevant_phrases)
        throw(El::Exception)
      { 
        for(PhraseSet::const_iterator i(phrases.begin()),
              e(phrases.end()); i != e; ++i)
        {
          const Phrase& p = *i;

          PhraseArray subs;
          p.subphrases(subs);
            
          for(PhraseArray::const_iterator j(subs.begin()), je(subs.end());
              j != je; ++j)
          {
            PhraseCounter::iterator r = relevant_phrases.find(*j);

            if(r != relevant_phrases.end())
            {
              // Do not suggest phrase if more specific already exist
              relevant_phrases.erase(r);
            }
          }
        }
      }
      
      void
      ManagerImpl::category_phrases(Message::Categorizer::Category::Id id,
                                    const char* lang,
                                    PhraseSet* any_phrases,
                                    PhraseSet* any2_phrases,
                                    PhraseSet* skip_phrases,
                                    PhraseSet* skip2_phrases,
                                    El::MySQL::Connection* connection,
                                    CategoryIdSet& processed_categories)
        throw(Exception, El::Exception)
      {
        if(!processed_categories.insert(id).second)
        {
          return;
        }
        
        El::Lang language;

        if(*lang != '\0')
        {
          language = El::Lang(lang);
        }

//        std::cerr << "*" << lang << " " << language.l3_code() << std::endl;
        
        std::ostringstream ostr;
        ostr << "select name, words from "
          "CategoryWordList where category_id=" << id;
              
        El::MySQL::Result_var qresult =
          connection->query(ostr.str().c_str());            
              
        CategoryWordListDesc wl(qresult.in(), 2);
              
        while(wl.fetch_row())
        {
          std::string name = wl.name();
          std::string::size_type pos = name.find('-');

          El::Lang wl_lang;

          if(pos != std::string::npos)
          {
            try
            {
              std::string lg(name, 0, pos);
              wl_lang = El::Lang(lg.c_str());
              name = name.c_str() + pos + 1;
            }
            catch(...)
            {
            }
          }

          if((wl_lang != El::Lang::null && language != El::Lang::null &&
              wl_lang != language) || name == "URL" || name == "SITE")
          {
            continue;
          }

          size_t len = name.length();
          bool class2 = len > 1 && isdigit(name[len - 1]);

          if(class2)
          {
            name = std::string(name.c_str(), len - 1);
          }
            
          PhraseSet* res_phrases =
            name == "ANY" ? (class2 ? any2_phrases : any_phrases) :
            (name == "SKIP" ? (class2 ? skip2_phrases : skip_phrases) : 0);
                                      
          if(res_phrases == 0)
          {
            continue;
          }

//            PhraseSet& res_phrases = class2 ? cat_phrases2 : cat_phrases;
//            PhraseSet& res_phrases = cat_phrases;

          PhraseArray phrases;
          WordInfoMap words;
            
          {
            std::istringstream istr(wl.words().c_str());
            std::string line;
              
            while(std::getline(istr, line))
            {
              std::string trimmed;
              El::String::Manip::trim(line.c_str(), trimmed);

              size_t len = trimmed.length();

              if(len > 2)
              {
                char first = trimmed[0];
                char last = trimmed[len - 1];

                if((first == '"' && last == '"') ||
                   (first == '\'' && last == '\''))
                {
                  line = std::string(trimmed.c_str() + 1, len - 2);
                  El::String::Manip::trim(line.c_str(), trimmed);
                }     
              }

              if(trimmed.empty())
              {
                continue;
              }

              std::string uniformed;
              El::String::Manip::utf8_to_uniform(trimmed.c_str(), uniformed);

              El::String::ListParser parser(uniformed.c_str());
              const char* item = 0;
              WordArray wrd;
                
              while((item = parser.next_item()) != 0)
              {
                wrd.push_back(Word(item));
                  
                words.insert(
                  std::make_pair(
                    item,
                    ::El::Dictionary::Morphology::WordInfo()));
              }

              phrases.push_back(Phrase(wrd));
            }
          }

          ::NewsGate::Dictionary::WordManager::WordSeq word_seq;
          word_seq.length(words.size());
          size_t j = 0;

          for(WordInfoMap::const_iterator i(words.begin()), e(words.end());
              i != e; ++i)
          {
            word_seq[j++] = CORBA::string_dup(i->first.c_str());            
          }

          try
          {
            Dictionary::WordManager_var word_manager =
              Application::instance()->word_manager();

            ::NewsGate::Dictionary::Transport::NormalizedWordsPack_var result;
              
            word_manager->normalize_words(word_seq, ""/*lang*/, result.out());

            ::NewsGate::Dictionary::Transport::NormalizedWordsPackImpl::Type*
                res = dynamic_cast< ::NewsGate::Dictionary::Transport::
            NormalizedWordsPackImpl::Type* >(
              result.in());
              
            if(res == 0)
            {
              throw Exception(
                "NewsGate::Moderation::Category::ManagerImpl::"
                "category_phrases: dynamic_cast<"
                "::NewsGate::Dictionary::Transport::NormalizedWordsPackImpl::"
                "Type*> failed");
            }

            ::El::Dictionary::Morphology::WordInfoArray& norm_words =
                res->entities();

            assert(norm_words.size() == words.size());

            j = 0;

            for(WordInfoMap::iterator i(words.begin()), e(words.end());
                i != e; ++i)
            {
              i->second = norm_words[j++];

              ::El::Dictionary::Morphology::WordInfo& wi = i->second;

              if(wi.forms.empty())
              {
                wi.forms.push_back(
                  ::El::Dictionary::Morphology::WordForm(
                    El::Dictionary::Morphology::pseudo_id(i->first.c_str()),
                    El::Lang::null,
                    false));
              }
/*                
                  std::cerr << i->first << " " << wi.lang.l3_code();
                
                  for(::El::Dictionary::Morphology::WordFormArray::const_iterator
                  i(wi.forms.begin()), e(wi.forms.end()); i != e; ++i)
                  {
                  std::cerr << " " << i->id;
                  }

                  std::cerr << std::endl;
*/
            }
          }
          catch(const Dictionary::NotReady& e)
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Moderation::Category::ManagerImpl::"
              "category_phrases: Dictionary::NotReady caught. "
              "Reason:\n" << e.reason.in();

            throw Exception(ostr.str());
          }
          catch(const Dictionary::ImplementationException& e)
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Moderation::Category::ManagerImpl::"
              "category_phrases: Dictionary::ImplementationException "
              "caught. Description:\n" << e.description.in();

            throw Exception(ostr.str());
          }
          catch(const CORBA::Exception& e)
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Moderation::Category::ManagerImpl::"
              "category_phrases: CORBA::Exception caught. "
              "Description:\n" << e;

            throw Exception(ostr.str());
          }
              
          for(PhraseArray::const_iterator i(phrases.begin()),
                e(phrases.end()); i != e; ++i)
          {
/*
  std::cerr << "*** ";
  i->dump(std::cerr);
  std::cerr << std::endl;
*/            
            PhraseArray all_phrases;
              
            possible_phrases(i->words.begin(),
                             i->words.end(),
                             words,
                             all_phrases);
              
            for(PhraseArray::const_iterator i(all_phrases.begin()),
                  e(all_phrases.end()); i != e; ++i)
            {
              res_phrases->insert(*i);
            }
/*              
                for(PhraseArray::const_iterator i(all_phrases.begin()),
                e(all_phrases.end()); i != e; ++i)
                {
                i->dump(std::cerr);
                std::cerr << std::endl;
                }
*/
          }            
        }

        CategoryDescriptor_var category;

        try
        {
          category = get_category(id, connection);
        }
        catch(const CategoryNotFound&)
        {
          throw;
        }
        catch(const NoPath& e)
        {
          std::ostringstream ostr;
          ostr << "unexpected NoPath (" << e.id << "/" <<
            e.name <<") caught for category " << id;

          throw Exception(ostr.str());
        }
        catch(const Cycle& e)
        {
          std::ostringstream ostr;
          ostr << "unexpected Cycle (" << e.id << "/" <<
            e.name <<") caught for category " << id;

          throw Exception(ostr.str());
        }

        const CategoryDescriptorSeq& children = category->children;

        for(size_t i = 0; i < children.length(); ++i)
        {
          category_phrases(children[i].id,
                           lang,
                           any_phrases,
                           0,
                           0,
                           0,
                           connection,
                           processed_categories);
        }
        
      }

      void
      ManagerImpl::possible_phrases(WordArray::const_iterator begin,
                                    WordArray::const_iterator end,
                                    const WordInfoMap& words,
                                    PhraseArray& phrases)
        throw(El::Exception)
      {
        if(begin == end)
        {
          return;
        }
        
        PhraseArray sub_phrases;        
        possible_phrases(begin + 1, end, words, sub_phrases);

        const Word& word = *begin;
        const char* word_text = word.text.c_str();
        
        WordInfoMap::const_iterator wi = words.find(word.text);
        assert(wi != words.end());

        const ::El::Dictionary::Morphology::WordFormArray& wfa =
            wi->second.forms;
        
        for(::El::Dictionary::Morphology::WordFormArray::const_iterator
              i(wfa.begin()), e(wfa.end()); i != e; ++i)
        {
          const ::El::Dictionary::Morphology::WordForm& wf = *i;

          if(sub_phrases.empty())
          {
            WordArray word_array;
//            word_array.push_front(Word(word_text, wf.id));              
            word_array.insert(word_array.begin(), Word(word_text, wf.id));
            phrases.push_back(Phrase(word_array));
          }
          else
          {
            for(PhraseArray::const_iterator j(sub_phrases.begin()),
                  je(sub_phrases.end()); j != je; ++j)
            {
              WordArray word_array(j->words);
//              word_array.push_front(Word(word_text, wf.id));
              word_array.insert(word_array.begin(), Word(word_text, wf.id));
              
              phrases.push_back(Phrase(word_array));
            }
          }
        }
      }
      
      std::string
      ManagerImpl::category_expression(Message::Categorizer::Category::Id id,
                                       El::MySQL::Connection* connection,
                                       const CategoryDescriptorSeq& children)
        throw(Exception, El::Exception)
      {        
        El::MySQL::Result_var qresult;
        std::string expression = "";
        {
          {
            std::ostringstream ostr;

            ostr << "select expression from CategoryExpression "
              "where category_id=" << id;
            
            qresult = connection->query(ostr.str().c_str());
          }

          Message::Categorizer::Category::ExpressionArray expressions;
          CategoryExpressionDesc exp_desc(qresult.in(), 1);

          while(exp_desc.fetch_row())
          {
            expressions.push_back(exp_desc.expression());
          }

          std::ostringstream ostr;
          
          for(Message::Categorizer::Category::ExpressionArray::iterator
                b(expressions.begin()), i(b), e(expressions.end()); i != e; ++i)
          {
            std::string& exp = *i;
            
            Message::Categorization::WordListResolver::instantiate_expression(
              exp,
              id,
              connection);

            if(i != b)
            {
              ostr << " OR ";
            }

            ostr << "( " << exp << " )";
          }

          expression = ostr.str();
        }

        if(expression.empty())
        {
          expression = "NONE";
        }

        {
          std::ostringstream ostr;

          ostr << "select message_id, relation from CategoryMessage "
            "where category_id=" << id;
        
          qresult = connection->query(ostr.str().c_str());

          bool has_excluded = false;
          bool has_included = false;

          std::ostringstream estr;
          std::ostringstream istr;
          
          CategoryMessage msg(qresult.in());
          
          while(msg.fetch_row())
          {
            Message::Id msg_id(msg.message_id());          

            if(msg.relation().value()[0] == 'E')
            {
              if(!has_excluded)
              {
                has_excluded = true;
                estr << "MSG";                
              }

              estr << " " << msg_id.string();
            }
            else
            {
              if(!has_included)
              {
                has_included = true;
                istr << "MSG";                
              }

              istr << " " << msg_id.string();
            }
          }

          if(has_excluded)
          {
            expression = "( " + expression + " ) EXCEPT " + estr.str();
          }
          
          if(has_included)
          {
            expression = "( " + expression + " ) OR " + istr.str();
          }
        }

        if(children.length())
        {
          std::ostringstream ostr;
          ostr << " OR CATEGORY";
            
          for(size_t i = 0; i < children.length(); ++i)
          {
            ostr << " \"" << children[i].paths[0].path.in() << "\"";
          }

          expression += ostr.str();
        }
        
        return expression;
      }

      Message::BankClientSession*
      ManagerImpl::bank_client_session() throw(El::Exception)
      {
        {
          ReadGuard guard(srv_lock_);

          if(bank_client_session_.in() != 0)
          {
            bank_client_session_->_add_ref();
            return bank_client_session_.in();
          }

          std::string message_bank_manager_ref =
            Application::instance()->config().message_bank_manager_ref();

          if(message_bank_manager_ref.empty())
          {
            return 0;
          }
        }

        WriteGuard guard(srv_lock_);

        if(bank_client_session_.in() != 0)
        {
          bank_client_session_->_add_ref();
          return bank_client_session_.in();
        }
        
        try
        {
          std::string message_bank_manager_ref =
            Application::instance()->config().message_bank_manager_ref();

          CORBA::Object_var obj =
            Application::instance()->orb()->string_to_object(
              message_bank_manager_ref.c_str());
      
          if(CORBA::is_nil(obj.in()))
          {
            std::ostringstream ostr;
            ostr << "::NewsGate::Moderation::Category::ManagerImpl::"
              "bank_client_session: string_to_object() "
              "gives nil reference for "
                 << message_bank_manager_ref;
        
            throw Exception(ostr.str().c_str());
          }

          Message::BankManager_var manager =
            Message::BankManager::_narrow(obj.in());
        
          if (CORBA::is_nil(manager.in()))
          {
            std::ostringstream ostr;
            ostr << "::NewsGate::Moderation::Category::ManagerImpl::"
              "bank_client_session: _narrow gives nil reference for "
                 << message_bank_manager_ref;
      
            throw Exception(ostr.str().c_str());
          }

          bank_client_session_ = manager->bank_client_session();

          Message::BankClientSessionImpl* bank_client_session =
            dynamic_cast<Message::BankClientSessionImpl*>(
              bank_client_session_.in());
      
          if(bank_client_session == 0)
          {
            throw Exception(
              "::NewsGate::Moderation::Category::ManagerImpl::"
              "bank_client_session: "
              "dynamic_cast<Message::BankClientSessionImpl*> failed");
          }

          bank_client_session->init_threads(this);
        }
        catch(const CORBA::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Category::ManagerImpl::"
                "bank_client_session: retrieving message bank client "
            "session failed. Reason:\n" << e;

          throw Exception(ostr.str());
        }

        bank_client_session_->_add_ref();
        return bank_client_session_.in();
      }

      void
      ManagerImpl::set_phrase_usefulness_flag(const char* query,
                                              const char* lang,
                                              PhraseOccuranceSeq& phrases)
        throw(NewsGate::Moderation::Category::ExpressionParseError,
              El::Exception)
      {
        Message::BankClientSession_var session = bank_client_session();
          
        if(session.in() == 0)
        {
          throw Exception(
            "::NewsGate::Moderation::Category::ManagerImpl::"
            "set_phrase_usefulness_flag: message bank manager not available");
        }

        Search::Expression_var expression = segment_search_expression(query);
        expression = normalize_search_expression(expression.in());

        Search::Msg_var msg_cond = new Search::Msg();
        Search::Except_var except = new Search::Except();
        
        except->left = expression->condition;
        except->right = msg_cond;

        expression->condition = except;
        
        Search::Strategy::Filter search_filter;

        if(*lang != '\0')
        {
          search_filter.lang = El::Lang(lang);
        }

        const Server::Config::CategoryManagerType::relevant_phrases_type&
          config = Application::instance()->config().relevant_phrases();        

        while(true)
        {
          expression->add_ref();

          Search::Transport::ExpressionImpl::Var
            expression_transport =
            Search::Transport::ExpressionImpl::Init::create(
              new Search::Transport::ExpressionHolder(expression));

          Search::Transport::StrategyImpl::Var strategy_transport = 
            Search::Transport::StrategyImpl::Init::create(
              new Search::Strategy(new Search::Strategy::SortNone(),
                                   new Search::Strategy::SuppressNone(),
                                   false,
                                   search_filter,
                                   Search::Strategy::RF_MESSAGES));

          Message::SearchRequest_var search_request =
            new Message::SearchRequest();

          search_request->gm_flags = Message::Bank::GM_ID;
            
          search_request->expression = expression_transport._retn();
          search_request->strategy = strategy_transport._retn();
          search_request->results_count = config.search_results();

          Message::SearchResult_var search_result;
          ACE_Time_Value tm = ACE_OS::gettimeofday();

          try
          {
            session->search(search_request.in(), search_result.out());
          }
          catch(const NewsGate::Message::ImplementationException& e)
          {
            std::ostringstream ostr;
            ostr << "::NewsGate::Moderation::Category::ManagerImpl::"
              "set_phrase_usefulness_flag: ImplementationException caught. "
              "Description:\n" << e.description.in();
            
            throw Exception(ostr.str());
          }
          catch(const CORBA::Exception& e)
          {
            std::ostringstream ostr;
            ostr << "::NewsGate::Moderation::Category::ManagerImpl::"
              "set_phrase_usefulness_flag: CORBA::Exception caught. "
              "Description:\n" << e;
            
            throw Exception(ostr.str());
          }

          Message::Transport::StoredMessagePackImpl::Type* msg_transport =
            dynamic_cast<Message::Transport::StoredMessagePackImpl::Type*>(
              search_result->messages.in());
      
          if(msg_transport == 0)
          {
            throw Exception(
              "::NewsGate::Moderation::Category::ManagerImpl::"
              "relevant_phrases: dynamic_cast<const Message::Transport::"
              "StoredMessagePackImpl::Type*> failed");
          }

          Message::Transport::StoredMessageArrayPtr messages;
          messages.reset(msg_transport->release());

          if(messages->empty())
          {
            break;
          }

          for(Message::Transport::StoredMessageArray::const_iterator
                i(messages->begin()), e(messages->end()); i != e; ++i)
          {
            const Message::StoredMessage& msg = i->message;
            msg_cond->ids.push_back(msg.id);
          }

        }

        for(size_t i = 0; i < phrases.length(); ++i)
        {
          PhraseOccurance& p = phrases[i];
/*
          if(p.occurances_irrelevant)
          {
            continue;
          }
*/
          std::string phrase_query = std::string("CORE '") +
            p.phrase.in() + "'";

          try
          {
            expression = segment_search_expression(phrase_query.c_str());
          }
          catch(const NewsGate::Moderation::Category::ExpressionParseError&)
          {
            continue;
          }
          
          expression = normalize_search_expression(expression.in());

          except->left = expression->condition;

          expression->condition = except;
          expression->add_ref();

          Search::Transport::ExpressionImpl::Var
            expression_transport =
            Search::Transport::ExpressionImpl::Init::create(
              new Search::Transport::ExpressionHolder(expression));

          Search::Transport::StrategyImpl::Var strategy_transport = 
            Search::Transport::StrategyImpl::Init::create(
              new Search::Strategy(new Search::Strategy::SortNone(),
                                   new Search::Strategy::SuppressNone(),
                                   false,
                                   search_filter,
                                   0));
          
          Message::SearchRequest_var search_request =
            new Message::SearchRequest();

          search_request->gm_flags = 0;
          search_request->expression = expression_transport._retn();
          search_request->strategy = strategy_transport._retn();
          search_request->results_count = 0;

          Message::SearchResult_var search_result;
          
          try
          {
            session->search(search_request.in(), search_result.out());
          }
          catch(const NewsGate::Message::ImplementationException& e)
          {
            std::ostringstream ostr;
            ostr << "::NewsGate::Moderation::Category::ManagerImpl::"
              "set_phrase_usefulness_flag: ImplementationException caught. "
              "Description:\n" << e.description.in();
            
            throw Exception(ostr.str());
          }
          catch(const CORBA::Exception& e)
          {
            std::ostringstream ostr;
            ostr << "::NewsGate::Moderation::Category::ManagerImpl::"
              "set_phrase_usefulness_flag: CORBA::Exception caught. "
              "Description:\n" << e;
            
            throw Exception(ostr.str());
          }

          p.total_irrelevant_occurances =
            search_result->total_matched_messages;
        }
      }
      
      size_t
      ManagerImpl::count_phrases(const char* query,
                                 const char* lang,
                                 PhraseCounter& phrase_counter,
                                 size_t& message_count,
                                 ACE_Time_Value& search_time,
                                 bool collapse_events,
                                 const char* log_name)
        throw(NewsGate::Moderation::Category::ExpressionParseError,
              El::Exception)
      {
        Message::BankClientSession_var session = bank_client_session();
          
        if(session.in() == 0)
        {
          throw Exception(
            "::NewsGate::Moderation::Category::ManagerImpl::"
            "relevant_phrases: message bank manager not available");
        }

        const Server::Config::CategoryManagerType::relevant_phrases_type&
          config = Application::instance()->config().relevant_phrases();

        size_t total_message_count = 0;
        message_count = 0;

        Search::Expression_var expression = segment_search_expression(query);
        expression = normalize_search_expression(expression.in());

        Search::Msg_var msg_cond = new Search::Msg();
        Search::Event_var event_cond = new Search::Event();

        Search::Except_var except1 = new Search::Except();
        except1->left = expression->condition;
        except1->right = event_cond;
        
        Search::Except_var except2 = new Search::Except();
        except2->left = except1;
        except2->right = msg_cond;

        expression->condition = except2;
        
        Search::Strategy::Filter search_filter;

        if(*lang != '\0')
        {
          search_filter.lang = El::Lang(lang);
        }

        for(size_t i = config.search_count(); i > 0 ; --i)
        {
          expression->add_ref();

          Search::Transport::ExpressionImpl::Var
            expression_transport =
            Search::Transport::ExpressionImpl::Init::create(
              new Search::Transport::ExpressionHolder(expression));

          Search::Strategy::SortingPtr sorting(
            new Search::Strategy::SortByPubDateDesc(config.message_max_age()));
          
          Search::Strategy::SuppressionPtr suppression(
            collapse_events ?
            new Search::Strategy::CollapseEvents(
              config.supress_cw().intersection(),
              config.supress_cw().containment_level(),
              config.supress_cw().min_count(),
              config.message_per_event()) :
            new Search::Strategy::SuppressSimilar(
              config.supress_cw().intersection(),
              config.supress_cw().containment_level(),
              config.supress_cw().min_count()));

          Search::Transport::StrategyImpl::Var strategy_transport = 
            Search::Transport::StrategyImpl::Init::create(
              new Search::Strategy(sorting.release(),
                                   suppression.release(),
                                   false,
                                   search_filter,
                                   Search::Strategy::RF_MESSAGES));

          Message::SearchRequest_var search_request =
            new Message::SearchRequest();

          search_request->gm_flags =
            Message::Bank::GM_ID | Message::Bank::GM_EVENT;
            
          search_request->expression = expression_transport._retn();
          search_request->strategy = strategy_transport._retn();
          search_request->results_count = config.search_results();

          Message::SearchResult_var search_result;
          ACE_Time_Value tm = ACE_OS::gettimeofday();

          try
          {
            session->search(search_request.in(), search_result.out());
          }
          catch(const NewsGate::Message::ImplementationException& e)
          {
            std::ostringstream ostr;
            ostr << "::NewsGate::Moderation::Category::ManagerImpl::"
              "relevant_phrases: ImplementationException caught. "
              "Description:\n" << e.description.in();
            
            throw Exception(ostr.str());
          }
          catch(const CORBA::Exception& e)
          {
            std::ostringstream ostr;
            ostr << "::NewsGate::Moderation::Category::ManagerImpl::"
              "relevant_phrases: CORBA::Exception caught. "
              "Description:\n" << e;
            
            throw Exception(ostr.str());
          }

          ACE_Time_Value dt = ACE_OS::gettimeofday() - tm;

          search_time += dt;

          if(!total_message_count)
          {
            total_message_count = search_result->total_matched_messages +
              search_result->suppressed_messages;
          }

          Message::Transport::StoredMessagePackImpl::Type* msg_transport =
            dynamic_cast<Message::Transport::StoredMessagePackImpl::Type*>(
              search_result->messages.in());
      
          if(msg_transport == 0)
          {
            throw Exception(
              "::NewsGate::Moderation::Category::ManagerImpl::"
              "relevant_phrases: dynamic_cast<const Message::Transport::"
              "StoredMessagePackImpl::Type*> failed");
          }

          Message::Transport::StoredMessageArrayPtr messages;
          messages.reset(msg_transport->release());

//          std::cerr << "IDS: " << messages->size() << "/ "
//                    << El::Moment::time(dt) << std::endl;
          
          if(messages->empty())
          {
            break;
          }

          if(log_name)
          {
            std::cerr << log_name << " ===========================\n";
          }

          MessagePhraseMap message_phrases;
          uint64_t time_threshold = tm.sec() - config.message_phrases_timeout();

          Search::Msg_var msg_cond2 = new Search::Msg();
          
          Search::Expression_var expression2 = new Search::Expression();
          expression2->condition = msg_cond2;
          
          {
            WriteGuard guard(srv_lock_);

            for(MessagePhraseMap::iterator i(message_phrases_.begin()),
                  e(message_phrases_.end()); i != e;)
            {
              MessagePhraseMap::iterator cur = i++;
              
              if(cur->second.time < time_threshold)
              {
                message_phrases_.erase(cur);
              }
            }
              
            for(Message::Transport::StoredMessageArray::const_iterator
                  i(messages->begin()), e(messages->end()); i != e; ++i)
            {
              const Message::StoredMessage& msg = i->message;
              msg_cond->ids.push_back(msg.id);

              if(msg.event_id != El::Luid::null)
              {
                event_cond->ids.push_back(msg.event_id);
              }

              MessagePhraseMap::const_iterator mi =
                message_phrases_.find(msg.id);

              if(mi == message_phrases_.end())
              {
                msg_cond2->ids.push_back(msg.id);
              }
              else
              {
                message_phrases[msg.id] = mi->second;
              }
            }
          }

          if(!msg_cond2->ids.empty())
          {
            expression2->add_ref();
          
            expression_transport =
              Search::Transport::ExpressionImpl::Init::create(
                new Search::Transport::ExpressionHolder(expression2));

            sorting.reset(new Search::Strategy::SortNone());
            suppression.reset(new Search::Strategy::SuppressNone());

            strategy_transport = 
              Search::Transport::StrategyImpl::Init::create(
                new Search::Strategy(sorting.release(),
                                     suppression.release(),
                                     false,
                                     Search::Strategy::Filter(),
                                     Search::Strategy::RF_MESSAGES));
          
            search_request = new Message::SearchRequest();

            search_request->gm_flags =
              Message::Bank::GM_ID | Message::Bank::GM_TITLE |
              Message::Bank::GM_DESC | Message::Bank::GM_DEBUG_INFO |
              Message::Bank::GM_CORE_WORDS;
            
            search_request->expression = expression_transport._retn();
            search_request->strategy = strategy_transport._retn();
            search_request->results_count = config.search_results();

            search_result = 0;
          
            tm = ACE_OS::gettimeofday();

            try
            {
              session->search(search_request.in(), search_result.out());
            }
            catch(const NewsGate::Message::ImplementationException& e)
            {
              std::ostringstream ostr;
              ostr << "::NewsGate::Moderation::Category::ManagerImpl::"
                "relevant_phrases: ImplementationException caught. "
                "Description:\n" << e.description.in();
            
              throw Exception(ostr.str());
            }
            catch(const CORBA::Exception& e)
            {
              std::ostringstream ostr;
              ostr << "::NewsGate::Moderation::Category::ManagerImpl::"
                "relevant_phrases: CORBA::Exception caught. "
                "Description:\n" << e;
            
              throw Exception(ostr.str());
            }

            dt = ACE_OS::gettimeofday() - tm;
            search_time += dt;
            
            msg_transport =
              dynamic_cast<Message::Transport::StoredMessagePackImpl::Type*>(
                search_result->messages.in());
          
            if(msg_transport == 0)
            {
              throw Exception(
                "::NewsGate::Moderation::Category::ManagerImpl::"
                "relevant_phrases: dynamic_cast<const Message::Transport::"
                "StoredMessagePackImpl::Type*> failed");
            }

            messages.reset(msg_transport->release());

//            std::cerr << "MSG: " << messages->size() << "/ "
//                      << El::Moment::time(dt) << std::endl;
            
            TimePhraseArray tpa;
            tpa.time = tm.sec();
          
            for(Message::Transport::StoredMessageArray::const_iterator
                  i(messages->begin()), e(messages->end()); i != e; ++i)
            {
              message_phrases[i->message.id] = tpa;
            }          
          
            get_phrases(*messages, 1, message_phrases, log_name);
            get_phrases(*messages, 2, message_phrases, log_name);
            get_phrases(*messages, 3, message_phrases, log_name);

            WriteGuard guard(srv_lock_);

            for(Message::Transport::StoredMessageArray::const_iterator
                  i(messages->begin()), e(messages->end()); i != e; ++i)
            {
              const Message::Id& id = i->message.id;
              message_phrases_[id] = message_phrases[id];
            }
          }
          
          for(MessagePhraseMap::const_iterator i(message_phrases.begin()),
                e(message_phrases.end()); i != e; ++i)
          {              
            const PhraseArray& phrases = i->second.phrases;

            for(PhraseArray::const_iterator i(phrases.begin()),
                  e(phrases.end()); i != e; ++i)
            {
              PhraseCounter::iterator c =
                phrase_counter.insert(std::make_pair(*i, 0)).first;

              ++(c->second);
            }
          }
          
          message_count += message_phrases.size();
          
//          std::cerr << messages->size() << std::endl;
        }

        return total_message_count;
      }

      void
      ManagerImpl::get_phrases(
        const Message::Transport::StoredMessageArray& messages,
        size_t phrase_len,
        MessagePhraseMap& message_phrases,
        const char* log_name)
          throw(El::Exception)
      {
        for(Message::Transport::StoredMessageArray::const_iterator
              i(messages.begin()), e(messages.end()); i != e; ++i)
        {
          const Message::Transport::StoredMessageDebug& dmsg = *i;
          const Message::StoredMessage& msg = dmsg.message;

          if(log_name)
          {
            std::cerr << "MSG " << msg.id.string() << std::endl;
          }

          PhraseCollector pfc(&dmsg, phrase_len, log_name != 0);
            
          msg.assemble_title(pfc);
          pfc.reset_words();
            
          msg.assemble_description(pfc);
          pfc.reset_words();

          if(log_name)
          {
            std::cerr << std::endl;
          }
          
/*          
          const Message::StoredImageArray* images = msg.content->images.get();

          if(images)
          {
            for(size_t i = 0; i < images->size(); ++i)
            {
              msg.assemble_image_alt(pfc, i);
              pfc.reset_words();
            }
          }
*/

//          std::cout << std::endl;

          MessagePhraseMap::iterator mi = message_phrases.find(msg.id);
          assert(mi != message_phrases_.end());

          PhraseArray& mp = mi->second.phrases;
          
          for(PhraseSet::const_iterator i(pfc.phrases().begin()),
                e(pfc.phrases().end()); i != e; ++i)
          {
            mp.push_back(*i);
          }
        }        
      }
      
      Search::Expression*
      ManagerImpl::segment_search_expression(const char* exp)
        throw(Exception,
              El::Exception,
              NewsGate::Moderation::Category::ExpressionParseError)
      {
        std::string expression;
      
        try
        {        
          NewsGate::Segmentation::Segmentor_var segmentor =
            Application::instance()->segmentor();

          if(CORBA::is_nil(segmentor.in()))
          {
            expression = exp;
          }
          else
          {
            CORBA::String_var res = segmentor->segment_query(exp);
            expression = res.in();
          }
        }
        catch(const NewsGate::Segmentation::NotReady& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::Category::ManagerImpl::"
            "segment_search_expression: Segmentation::NotReady "
            "caught. Description:\n" << e.reason.in();

          throw Exception(ostr.str());
        }
        catch(const NewsGate::Segmentation::InvalidArgument& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::Category::ManagerImpl::"
            "segment_search_expression: Segmentation::InvalidArgument "
            "caught. Description:\n" << e.description.in();

          throw Exception(ostr.str());
        }
        catch(const NewsGate::Segmentation::ImplementationException& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::Category::ManagerImpl::"
            "segment_search_expression: Segmentation::"
            "ImplementationException caught. Description:\n"
               << e.description.in();

          throw Exception(ostr.str());
        }
        catch(const CORBA::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::Category::ManagerImpl::"
            "segment_search_expression: CORBA::Exception caught. "
            "Description:\n" << e;

          throw Exception(ostr.str());
        }

        try
        {
          std::wstring query;
          El::String::Manip::utf8_to_wchar(expression.c_str(), query);
            
          Search::ExpressionParser parser;
          std::wistringstream istr(query);
            
          parser.parse(istr);
          return parser.expression().retn();
        }
        catch(const Search::ParseError& e)
        {
          std::ostringstream ostr;
          ostr << e << std::endl << "Bad query: " << exp;
          
          NewsGate::Moderation::Category::ExpressionParseError ex;
          ex.position = e.position;
          ex.description = ostr.str().c_str();
          throw ex;
        }        
      }

      Search::Expression*
      ManagerImpl::normalize_search_expression(Search::Expression* expression)
        throw(Exception, El::Exception)
      {
        try
        {
          Dictionary::WordManager_var word_manager =
            Application::instance()->word_manager();        
          
          Search::Transport::ExpressionImpl::Var
            expression_transport =
            Search::Transport::ExpressionImpl::Init::create(
              new Search::Transport::ExpressionHolder(
                El::RefCount::add_ref(expression)));
          
          expression_transport->serialize();

          NewsGate::Search::Transport::Expression_var result;

          word_manager->normalize_search_expression(
            expression_transport.in(),
            result.out());

          if(dynamic_cast<Search::Transport::ExpressionImpl::Type*>(
               result.in()) == 0)
          {
            throw Exception(
              "NewsGate::Moderation::Category::ManagerImpl::"
              "normalize_search_expression: "
              "dynamic_cast<Search::Transport::ExpressionImpl::Type*> failed");
          }
        
          expression_transport =
            dynamic_cast<Search::Transport::ExpressionImpl::Type*>(
              result._retn());
        
          return expression_transport->entity().expression.retn();
        }
        catch(const Dictionary::NotReady& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::Category::ManagerImpl::"
            "normalize_search_expression: Dictionary::NotReady caught. "
            "Reason:\n" << e.reason.in();

          throw Exception(ostr.str());
        }
        catch(const Dictionary::ImplementationException& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::Category::ManagerImpl::"
            "normalize_search_expression: Dictionary::ImplementationException "
            "caught. Description:\n" << e.description.in();

          throw Exception(ostr.str());
        }
        catch(const CORBA::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::Category::ManagerImpl::"
            "normalize_search_expression: CORBA::Exception caught. "
            "Description:\n" << e;

          throw Exception(ostr.str());
        }      
      }
      
      char
      ManagerImpl::category_status(CategoryStatus status)
        throw(Exception, El::Exception)
      {
        switch(status)
        {
        case CS_ENABLED: return 'E';
        case CS_DISABLED: return 'D';
        default:
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Moderation::Category::ManagerImpl::"
              "category_status: uexpected status " << status;
            
            throw Exception(ostr.str());
          }
        }
      }
        
      char
      ManagerImpl::category_searcheable(CategorySearcheable searcheable)
        throw(Exception, El::Exception)
      {
        switch(searcheable)
        {
        case CR_YES: return 'Y';
        case CR_NO: return 'N';
        default:
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Moderation::Category::ManagerImpl::"
              "category_searcheable: uexpected searcheable flag "
                 << searcheable;

            throw Exception(ostr.str());
          }
        }          
      }

      uint64_t
      ManagerImpl::increment_update_number(El::MySQL::Connection* connection)
        throw(Exception, El::Exception)
      {
        El::MySQL::Result_var qresult = connection->query(
          "select update_num from CategoryUpdateNum for update");

        CategoryUpdateNum num(qresult.in());
            
        if(!num.fetch_row())
        {
          throw Exception(
            "NewsGate::Moderation::Category::ManagerImpl::"
            "increment_update_number: failed to get category update number");
        }

        uint64_t update_num = (uint64_t)num.update_num() + 1;
        
        {
          std::ostringstream ostr;
          ostr << "update CategoryUpdateNum set update_num="
               << update_num;
              
          qresult = connection->query(ostr.str().c_str());
        }

        return update_num;
      }

      void
      ManagerImpl::delete_categories(
        const ::NewsGate::Moderation::Category::CategoryIdSeq& ids,
        ::NewsGate::Moderation::Category::ModeratorId moderator_id,
        const char* moderator_name,
        const char* ip)
        throw(NewsGate::Moderation::Category::ForbiddenOperation,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        try
        {
          CategoryDescriptor_var empty = new CategoryDescriptor();
          
          empty->id = 0;
          empty->status = CS_COUNT;
          empty->searcheable = CR_COUNT;
          empty->version = 0;
          
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();            
          
          El::MySQL::Result_var qresult = connection->query("begin");

          try
          {
            increment_update_number(connection);

            for(size_t i = 0; i < ids.length(); i++)
            {
              CategoryId id = ids[i];
              
              if(id == ROOT_CATEGORY_ID)
              {
                throw ForbiddenOperation(
                  "NewsGate::Moderation::Category::ManagerImpl::"
                  "delete_categories: root category can not be deleted");
              }

              CategoryDescriptor_var category;

              try
              {
                category = get_category(id, connection);
              }
              catch(const CategoryNotFound&)
              {
                continue;
              }
              catch(const NoPath& e)
              {
                std::ostringstream ostr;
                ostr << "unexpected NoPath (" << e.id << "/" <<
                  e.name <<") caught for category " << id;

                throw Exception(ostr.str());
              }
              catch(const Cycle& e)
              {
                std::ostringstream ostr;
                ostr << "unexpected Cycle (" << e.id << "/" <<
                  e.name <<") caught for category " << id;

                throw Exception(ostr.str());
              }

              {
                std::ostringstream ostr;
                ostr << "delete from CategoryChild where child_id=" << id;
                qresult = connection->query(ostr.str().c_str());
              }

              unlink_category(id, connection);

              CategoryChange cat_change(*empty,
                                        *category,
                                        moderator_id,
                                        moderator_name,
                                        ip);
              
              cat_change.save(connection);
            }
            
            qresult = connection->query("commit");
          }
          catch(...)
          {
            qresult = connection->query("rollback");
            throw;
          }            
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::Category::ManagerImpl::"
            "delete_categories: El::Exception caught. Description:\n"
               << e;

          ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }        
      }

      ::NewsGate::Moderation::Category::CategoryFindingSeq*
      ManagerImpl::find_text(const char* text)
        throw(NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        try
        {
          typedef std::map<CategoryId, CategoryFinding> CategoryFindingMap;
          CategoryFindingMap cat_findings;

          typedef __gnu_cxx::hash_set<std::wstring, El::Hash::String>
            StringHashSet;
          
          StringHashSet text_items;
          
          {
            std::ostringstream ostr;
            ostr << "select category_id, name, words from "
              "CategoryWordList where";

            El::String::ListParser parser(text, "\n\r");
              
            const char* item = 0;
            bool first = true;

            El::MySQL::Connection_var connection =
              Application::instance()->dbase()->connect();

            while((item = parser.next_item()) != 0)
            {
              std::string word;
              El::String::Manip::compact(item, word);
              
              if(!word.empty())
              {
                std::wstring wword;
                El::String::Manip::utf8_to_wchar(word.c_str(), wword);
                
                std::wstring lowered;
                El::String::Manip::to_lower(wword.c_str(), lowered);
                
                text_items.insert(lowered);
                
                if(first)
                {
                  first = false;
                }
                else
                {
                  ostr << " or";
                }

                ostr << " locate('"
                     << connection->escape(word.c_str()) << "', words)";
              }
            }

            if(first)
            {
              return new CategoryFindingSeq();
            }            
            
            El::MySQL::Result_var qresult = connection->query("begin");
          
            try
            {            
              {
                qresult = connection->query(ostr.str().c_str());
              
                CategoryWordListFindingDesc rec(qresult.in());

                while(rec.fetch_row())
                {
                  CategoryId id = rec.category_id();
                
                  CategoryFindingMap::iterator i(cat_findings.find(id));

                  if(i == cat_findings.end())
                  {
                    i = cat_findings.insert(
                      std::make_pair(id, CategoryFinding())).first;
                
                    i->second.id = id;
                  }

                  CategoryFinding& cf = i->second;
              
                  size_t len = cf.word_lists.length();
                  cf.word_lists.length(len + 1);
              
                  WordListFinding& wf = cf.word_lists[len];
                  wf.name = rec.name().c_str();
                  wf.words.length(1);                  
                  wf.words[0].text = rec.words().c_str();
                }
              }

              for(CategoryFindingMap::iterator i(cat_findings.begin()),
                    e(cat_findings.end()); i != e; ++i)
              {
                CategoryFinding& cf = i->second;
              
                CategoryDescriptor_var cd = new CategoryDescriptor();
                cd->id = cf.id;
              
                {
                  std::ostringstream ostr;
                  ostr << "select id, name from Category where id=" << cf.id;
              
                  qresult = connection->query(ostr.str().c_str());
              
                  CategoryDesc cat(qresult.in(), 2);
                
                  if(cat.fetch_row())
                  {
                    cd->name = cat.name().c_str();                  
                  }
                }

                if(cd->name.in()[0] != '\0')
                {
                  fill_paths(*cd, connection);

                  if(cd->paths.length())
                  {
                    cf.path = cd->paths[0].path;
                  }    
                }
              }
            
              qresult = connection->query("commit");
            }
            catch(...)
            {
              qresult = connection->query("rollback");
              throw;
            }
          }

          CategoryFindingSeq_var res = new CategoryFindingSeq();

          for(CategoryFindingMap::iterator i(cat_findings.begin()),
                e(cat_findings.end()); i != e; ++i)
          {
            CategoryFinding& cf = i->second;

            if(cf.path.in() == '\0')
            {
              continue;
            }

            size_t len = res->length();
            res->length(len + 1);
            
            CategoryFinding& dest_cf = (*res)[len];

            dest_cf.id = cf.id;
            dest_cf.path = cf.path._retn();

            WordListFindingSeq& word_lists = cf.word_lists;
            WordListFindingSeq& dest_word_lists = dest_cf.word_lists;

            dest_word_lists.length(word_lists.length());
            
            for(size_t i = 0; i < word_lists.length(); ++i)
            {
              dest_word_lists[i].name = word_lists[i].name._retn();
              
              const WordFindingSeq& words = word_lists[i].words;
              assert(words.length() == 1);

              WordFindingSeq& dest_words = dest_word_lists[i].words;
              
              std::string cleaned;
              El::String::Manip::suppress(words[0].text.in(), cleaned, "\r");

              std::wstring wwords;
              El::String::Manip::utf8_to_wchar(cleaned.c_str(), wwords);
              El::String::WListParser parser(wwords.c_str(), L"\n");
              
              const wchar_t* item = 0;              
              while((item = parser.next_item()) != 0)
              {
                std::wstring lowered;
                El::String::Manip::to_lower(item, lowered);
                
                for(StringHashSet::const_iterator it(text_items.begin()),
                      ie(text_items.end()); it != ie; ++it)
                {
                  const wchar_t* substr = it->c_str();
                  const wchar_t* shift = wcsstr(lowered.c_str(), substr);
                  
                  if(shift)
                  {
                    size_t len = dest_words.length();
                    dest_words.length(len + 1);

                    WordFinding& wf = dest_words[len];

                    std::string word;
                    El::String::Manip::wchar_to_utf8(item, word);

                    wf.text = word.c_str();
                    wf.from = shift - lowered.c_str();
                    wf.to = wf.from + wcslen(substr);
                    wf.start = parser.item_offset();
                    wf.end = wf.start + wcslen(item);
                  }
                }
              }
            }
          }
          
          return res._retn();
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::Category::ManagerImpl::"
            "find_text: El::Exception caught. Description:\n" << e;

          ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }          
      }
      
      void
      ManagerImpl::unlink_category(CategoryId id,
                                   El::MySQL::Connection* connection)
        throw(Exception, El::Exception)
      {
        El::MySQL::Result_var qresult;
        
        {
          std::ostringstream ostr;
          ostr << "select id from CategoryChild where child_id=" << id;
          qresult = connection->query(ostr.str().c_str());
        }

        if(qresult->num_rows())
        {
          return;
        }

        {
          std::ostringstream ostr;
          ostr << "delete from Category where id=" << id;          
          qresult = connection->query(ostr.str().c_str());
        }

        {
          std::ostringstream ostr;
          ostr << "delete from CategoryLocale where category_id=" << id;
          qresult = connection->query(ostr.str().c_str());
        }
        
        {
          std::ostringstream ostr;
          ostr << "delete from CategoryExpression where category_id=" << id;
          qresult = connection->query(ostr.str().c_str());
        }

        {
          std::ostringstream ostr;
          ostr << "delete from CategoryWordList where category_id=" << id;
          qresult = connection->query(ostr.str().c_str());
        }

        {
          std::ostringstream ostr;
          ostr << "delete from CategoryMessage where category_id=" << id;
          qresult = connection->query(ostr.str().c_str());
        }

        {
          std::ostringstream ostr;
          ostr << "select CategoryChild.child_id as id from CategoryChild "
            "where CategoryChild.id=" << id;
          
          qresult = connection->query(ostr.str().c_str());
        }

        CategoryIdSeq children;
        children.length(qresult->num_rows());

        CategoryDesc cat(qresult.in(), 1);

        for(size_t i = 0; cat.fetch_row(); i++)
        {
          children[i] = cat.id();
        }
        
        {
          std::ostringstream ostr;
          ostr << "delete from CategoryChild where id=" << id;
          qresult = connection->query(ostr.str().c_str());
        }

        for(size_t i = 0; i < children.length(); i++)
        {
          unlink_category(children[i], connection);
        }
      }
      
      ::NewsGate::Moderation::Category::CategoryDescriptor*
      ManagerImpl::create_category(
        const ::NewsGate::Moderation::Category::CategoryDescriptor& category,
        ::NewsGate::Moderation::Category::ModeratorId moderator_id,
        const char* moderator_name,
        const char* ip)
        throw(NewsGate::Moderation::Category::NoPath,
              NewsGate::Moderation::Category::Cycle,
              NewsGate::Moderation::Category::ForbiddenOperation,
              NewsGate::Moderation::Category::WordListNotFound,
              NewsGate::Moderation::Category::ExpressionParseError,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {        
        try
        {
          uint64_t cat_id = 0;
          char status = category_status(category.status);
          char searcheable = category_searcheable(category.searcheable);

          CategoryDescriptor_var empty = new CategoryDescriptor();

          empty->id = 0;
          empty->status = CS_COUNT;
          empty->searcheable = CR_COUNT;
          empty->version = 0;
          
          CategoryDescriptor_var res;
          
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();            
          
          El::MySQL::Result_var qresult = connection->query("begin");

          try
          {
            increment_update_number(connection);

            {
              std::ostringstream ostr;
              ostr << "insert into Category set name='" <<
                connection->escape(category.name.in()) << "', status='"
                   << status << "', searcheable='" << searcheable << "', "
                "created=NOW(), creator=" << category.creator_id
                   << ", version=1, description='" <<
                connection->escape(category.description.in()) << "'";

              qresult = connection->query(ostr.str().c_str());
            }

            cat_id = connection->insert_id();
            
            write_category_parents(connection,
                                   cat_id,
                                   category.parents);

            write_category_children(connection,
                                    cat_id,
                                    category.children);

            write_category_locales(connection, cat_id, category.locales);

            try
            {
              write_category_expressions(connection,
                                         cat_id,
                                         category.expressions,
                                         category.word_lists,
                                         0);
            }
            catch(const NewsGate::Moderation::Category::VersionMismatch& e)
            {
              std::cerr << "Unexpected version mismatch for cat " << cat_id
                        << "; error: " << e;
              
              assert(false);
            }
            
            res = get_category(cat_id, connection);
            check_name_uniqueness(*res, connection);

            CategoryChange cat_change(*res,
                                      *empty,
                                      moderator_id,
                                      moderator_name,
                                      ip);
              
            cat_change.save(connection);

            qresult = connection->query("commit");
          }
          catch(...)
          {
            qresult = connection->query("rollback");
            throw;
          }
          
          return res._retn();
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::Category::ManagerImpl::"
            "create_category: El::Exception caught. Description:\n"
               << e;

          ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
      }

      
      void
      ManagerImpl::check_name_uniqueness(const CategoryDescriptor& category,
                                         El::MySQL::Connection* connection)
        throw(NewsGate::Moderation::Category::ForbiddenOperation,
              Exception,
              El::Exception)
      {
        check_name_uniqueness(category.id,
                              category.paths[0].path.in(),
                              connection);

        const CategoryDescriptorSeq& parents = category.parents;
        
        for(size_t i = 0; i < parents.length(); i++)
        {
          check_name_uniqueness(parents[i].id,
                                parents[i].paths[0].path.in(),
                                connection);
        }
      }
      
      void
      ManagerImpl::check_name_uniqueness(CategoryId id,
                                         const char* path,
                                         El::MySQL::Connection* connection)
        throw(NewsGate::Moderation::Category::ForbiddenOperation,
              Exception,
              El::Exception)
      {
        CategoryPathElemSeq children = get_children(id, connection);

        for(size_t i = 0; i < children.length(); i++)
        {
          std::string name;
          El::String::Manip::utf8_to_lower(children[i].name.in(), name);

          for(size_t j = i + 1; j < children.length(); j++)
          {
            std::string name2;
            El::String::Manip::utf8_to_lower(children[j].name.in(), name2);
            
            if(name == name2)
            {
              std::ostringstream ostr;

              ostr << "NewsGate::Moderation::Category::ManagerImpl::"
                "check_name_uniqueness: duplicated category name "
                   << path << name;
                
              throw ForbiddenOperation(ostr.str().c_str());
            }
          }
        }
        
      }          
      
      void
      ManagerImpl::write_category_parents(
        El::MySQL::Connection* connection,
        unsigned long long cat_id,
        const CategoryDescriptorSeq& parents)
        throw(Exception, El::Exception)
      {        
        if(!parents.length())
        {
          return;
        }
        
        ACE_Time_Value ctime = ACE_OS::gettimeofday();
              
        std::ostringstream ostr;
        ostr << Application::instance()->config().temp_file() << "."
             << El::Moment(ctime).dense_format() << "." << rand();
            
        std::string filename = ostr.str();

        try
        {
          std::fstream file(filename.c_str(), ios::out);

          if(!file.is_open())
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Moderation::Category::ManagerImpl::"
              "write_category_parents: failed to open file '" << filename
                 << "' for write access";
                
            throw Exception(ostr.str());
          }
              
          bool first_line = true;
          
          for(size_t i = 0; i < parents.length(); i++)
          {
            if(first_line)
            {
              first_line = false;
            }
            else
            {
              file << std::endl;
            }
            
            file << parents[i].id << "\t" << cat_id;
          }
                
          file.close();
                
          {
            std::ostringstream ostr; 
            ostr << "LOAD DATA INFILE '"
                 << connection->escape(filename.c_str())
                 << "' INTO TABLE CategoryChild "
              "character set binary (id, child_id)";
                  
            El::MySQL::Result_var qresult =
              connection->query(ostr.str().c_str());
          }
              
          unlink(filename.c_str());
        }
        catch(...)
        {
          unlink(filename.c_str());
          throw;
        }              
      }
      
      void
      ManagerImpl::write_category_children(
        El::MySQL::Connection* connection,
        unsigned long long cat_id,
        const CategoryDescriptorSeq& children)
        throw(NewsGate::Moderation::Category::ForbiddenOperation,
              Exception,
              El::Exception)
      {        
        if(!children.length())
        {
          return;
        }
        
        ACE_Time_Value ctime = ACE_OS::gettimeofday();
              
        std::ostringstream ostr;
        ostr << Application::instance()->config().temp_file() << "."
             << El::Moment(ctime).dense_format() << "." << rand();
            
        std::string filename = ostr.str();

        try
        {
          std::fstream file(filename.c_str(), ios::out);

          if(!file.is_open())
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Moderation::Category::ManagerImpl::"
              "write_category_children: failed to open file '" << filename
                 << "' for write access";
                
            throw Exception(ostr.str());
          }
              
          bool first_line = true;
          
          for(size_t i = 0; i < children.length(); i++)
          {
            CategoryId id = children[i].id;

            if(id == ROOT_CATEGORY_ID)
            {
              throw ForbiddenOperation(
                "NewsGate::Moderation::Category::ManagerImpl::"
                "write_category_children: root category can not be assigned "
                "as a child");
            }
            
            if(first_line)
            {
              first_line = false;
            }
            else
            {
              file << std::endl;
            }

            file << cat_id << "\t" << id;
          }
                
          file.close();
                
          {
            std::ostringstream ostr; 
            ostr << "LOAD DATA INFILE '"
                 << connection->escape(filename.c_str())
                 << "' INTO TABLE CategoryChild "
              "character set binary (id, child_id)";
                  
            El::MySQL::Result_var qresult =
              connection->query(ostr.str().c_str());
          }
              
          unlink(filename.c_str());
        }
        catch(...)
        {
          unlink(filename.c_str());
          throw;
        }              
      }
      
      void
      ManagerImpl::write_category_locales(
        El::MySQL::Connection* connection,
        unsigned long long cat_id,
        const LocaleDescriptorSeq& locales)
        throw(Exception, El::Exception)
      {        
        if(!locales.length())
        {
          return;
        }
        
        ACE_Time_Value ctime = ACE_OS::gettimeofday();
              
        std::ostringstream ostr;
        ostr << Application::instance()->config().temp_file() << "."
             << El::Moment(ctime).dense_format() << "." << rand();
            
        std::string filename = ostr.str();

        try
        {
          std::fstream file(filename.c_str(), ios::out);

          if(!file.is_open())
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Moderation::Category::ManagerImpl::"
              "write_category_locales: failed to open file '" << filename
                 << "' for write access";
                
            throw Exception(ostr.str());
          }
              
          bool first_line = true;
          
          for(size_t i = 0; i < locales.length(); i++)
          {
            const LocaleDescriptor& desc = locales[i];

            if(first_line)
            {
              first_line = false;
            }
            else
            {
              file << std::endl;
            }
            
            file << cat_id << "\t" << desc.lang << "\t" << desc.country << "\t"
                 << connection->escape_for_load(desc.name.in()) << "\t"
                 << connection->escape_for_load(desc.title.in()) << "\t"
                 << connection->escape_for_load(desc.short_title.in()) << "\t"
                 << connection->escape_for_load(desc.description.in()) << "\t"
                 << connection->escape_for_load(desc.keywords.in());
          }
                
          file.close();
                
          {
            std::ostringstream ostr; 
            ostr << "LOAD DATA INFILE '"
                 << connection->escape(filename.c_str())
                 << "' INTO TABLE CategoryLocale "
              "character set binary (category_id, "
              "lang, country, name, title, short_title, description, keywords)";
                  
            El::MySQL::Result_var qresult =
              connection->query(ostr.str().c_str());
          }
              
          unlink(filename.c_str());
        }
        catch(...)
        {
          unlink(filename.c_str());
          throw;
        }
      }
      
      void
      ManagerImpl::validate_category_expressions(
        El::MySQL::Connection* connection,
        unsigned long long cat_id,
        const ExpressionDescriptorSeq& expressions,
        const WordListDescriptorSeq& word_lists)
        throw(NewsGate::Moderation::Category::WordListNotFound,
              NewsGate::Moderation::Category::ExpressionParseError,
              Exception,
              El::Exception)
      {
        for(size_t i = 0; i < expressions.length(); i++)
        {
          std::string expression;
          El::String::Manip::suppress(expressions[i].expression.in(),
                                      expression,
                                      "\r");

          try
          {
            Message::Categorization::WordListResolver::instantiate_expression(
              expression,
              cat_id,
              connection);
          }
          catch(const Message::Categorization::WordListResolver::
                WordListNotFound& e)
          {
            NewsGate::Moderation::Category::WordListNotFound ex;
            
            ex.name = CORBA::string_dup(e.name);
            ex.position = e.position;
            ex.expression_index = i;

            throw ex;
          }          
          catch(const Message::Categorization::WordListResolver::
                ExpressionParseError& e)
          {
            NewsGate::Moderation::Category::ExpressionParseError ex;

            ex.expression_index = i;
            ex.word_list_name = e.word_list_name;
            ex.position = e.position;
            ex.description = e.what();
  
            throw ex;
          }          
        }
      }
      
      void
      ManagerImpl::write_category_expressions(
        El::MySQL::Connection* connection,
        unsigned long long cat_id,
        const ExpressionDescriptorSeq& expressions,
        const WordListDescriptorSeq& word_lists,
        const CategoryDescriptor* former_category)
        throw(NewsGate::Moderation::Category::WordListNotFound,
              NewsGate::Moderation::Category::ExpressionParseError,
              NewsGate::Moderation::Category::VersionMismatch,
              Exception,
              El::Exception)
      {        
        write_category_expressions(connection,
                                   cat_id,
                                   expressions);
        
        write_category_word_lists(connection,
                                  cat_id,
                                  word_lists,
                                  true,
                                  false,
                                  former_category);

        validate_category_expressions(connection,
                                      cat_id,
                                      expressions,
                                      word_lists);
      }
      
      void
      ManagerImpl::write_category_expressions(
        El::MySQL::Connection* connection,
        unsigned long long cat_id,
        const ExpressionDescriptorSeq& expressions)
        throw(Exception, El::Exception)
      {        
        if(!expressions.length())
        {
          return;
        }
        
        ACE_Time_Value ctime = ACE_OS::gettimeofday();
              
        std::ostringstream ostr;
        ostr << Application::instance()->config().temp_file() << "."
             << El::Moment(ctime).dense_format() << "." << rand();
            
        std::string filename = ostr.str();

        try
        {
          std::fstream file(filename.c_str(), ios::out);

          if(!file.is_open())
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Moderation::Category::ManagerImpl::"
              "write_category_expressions: failed to open file '" << filename
                 << "' for write access";
                
            throw Exception(ostr.str());
          }
              
          bool first_line = true;
      
          for(size_t i = 0; i < expressions.length(); i++)
          {
            const ExpressionDescriptor& desc = expressions[i];
                  
            if(first_line)
            {
              first_line = false;
            }
            else
            {
              file << std::endl;
            }
              
            file << cat_id << "\t"
                 << connection->escape_for_load(desc.expression.in())
                 << "\t"
                 << connection->escape_for_load(desc.description.in());
          }
                
          file.close();
                
          {
            std::ostringstream ostr; 
            ostr << "LOAD DATA INFILE '"
                 << connection->escape(filename.c_str())
                 << "' INTO TABLE CategoryExpression "
              "character set binary (category_id, "
              "expression, description)";
                  
            El::MySQL::Result_var qresult =
              connection->query(ostr.str().c_str());
          }
              
          unlink(filename.c_str());
        }
        catch(...)
        {
          unlink(filename.c_str());
          throw;
        }              
      }

      void
      ManagerImpl::write_category_word_lists(
        El::MySQL::Connection* connection,
        unsigned long long cat_id,
        const WordListDescriptorSeq& word_lists,
        bool category_manager,
        bool subset,
        const CategoryDescriptor* former_category)
        throw(NewsGate::Moderation::Category::VersionMismatch,
              NewsGate::Moderation::Category::WordListNotFound,
              Exception,
              El::Exception)
      {
        typedef __gnu_cxx::hash_map<std::string,
                                    const WordListDescriptor*,
                                    El::Hash::String>
          
          WordListDescriptorMap;
        
        WordListDescriptorMap former_word_list_map;
        
        if(former_category)
        {
          El::String::Set word_list_names;

          for(size_t i = 0; i < word_lists.length(); ++i)
          {
            const WordListDescriptor& wd = word_lists[i];
            word_list_names.insert(wd.name.in());
          }
        
          const WordListDescriptorSeq& former_word_lists =
            former_category->word_lists;

          for(size_t i = 0; i < former_word_lists.length(); ++i)
          {
            const WordListDescriptor& fwl = former_word_lists[i];

            if(word_list_names.find(fwl.name.in()) == word_list_names.end())
            {
              if(!subset)
              {
                std::ostringstream ostr;
                ostr << "delete from CategoryWordList where category_id="
                     << cat_id << " and name='"
                     << connection->escape(fwl.name.in()) << "'";

                El::MySQL::Result_var qresult =
                  connection->query(ostr.str().c_str());
              }

              continue;
            }

            former_word_list_map[fwl.name.in()] = &fwl;
          }
        }

        for(size_t i = 0; i < word_lists.length(); i++)
        {
          const WordListDescriptor& desc = word_lists[i];

          if(!desc.updated)
          {
            continue;
          }
          
          WordListVersion version = 1;

          WordListDescriptorMap::const_iterator it =
            former_word_list_map.find(desc.name.in());

          if(it != former_word_list_map.end())
          {
            const WordListDescriptor& fwl = *it->second;
            
            if(strcmp(fwl.words.in(), desc.words.in()) == 0 &&
               strcmp(fwl.description.in(), desc.description.in()) == 0)
            {
              continue;
            }
            else              
            {
              version = fwl.version;
              
              if(version++ != desc.version)
              {
                std::ostringstream ostr;
                ostr << "NewsGate::Moderation::Category::ManagerImpl::"
                  "write_category_word_lists: word list '"
                     << fwl.name.in() << "' have already been updated";
                
                NewsGate::Moderation::Category::VersionMismatch ex;
                ex.description = CORBA::string_dup(ostr.str().c_str());
                throw ex;
              }
            }
          }
          
          std::string set_fields;
          
          {  
            std::ostringstream ostr;
            
            ostr << "words='"
                 << connection->escape(desc.words.in())
                 << "', version=" << version;

            if(category_manager)
            {
              ostr << ", description='"
                   << connection->escape(desc.description.in())
                   << "'";
            }
              
            set_fields = ostr.str();
          }
            
          std::ostringstream ostr;

          if(subset)
          {
            ostr << "update CategoryWordList set " << set_fields
                 << " where category_id=" << cat_id << " and name='"
                 << connection->escape(desc.name.in()) << "'";
          }
          else
          {
            ostr << "insert into CategoryWordList set category_id="
                 << cat_id << ", name='"
                 << connection->escape(desc.name.in()) << "', " << set_fields
                 << " on duplicate key update " << set_fields;
          }
          
          El::MySQL::Result_var qresult =
            connection->query(ostr.str().c_str());

          if(subset && connection->affected_rows() == 0)
          {
            WordListNotFound ex;
            ex.name = desc.name.in();
            throw ex;
          }
        }
      }
      
      ::NewsGate::Moderation::Category::CategoryDescriptor*
      ManagerImpl::update_word_lists(
        ::NewsGate::Moderation::Category::CategoryId category_id,
        const ::NewsGate::Moderation::Category::WordListDescriptorSeq&
        word_lists,
        ::CORBA::Boolean category_manager,
        ::NewsGate::Moderation::Category::ModeratorId moderator_id,
        const char* moderator_name,
        const char* ip)
        throw(NewsGate::Moderation::Category::ForbiddenOperation,
              NewsGate::Moderation::Category::WordListNotFound,
              NewsGate::Moderation::Category::ExpressionParseError,
              NewsGate::Moderation::Category::VersionMismatch,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        try
        {
          if(category_id == ROOT_CATEGORY_ID)
          {
            throw ForbiddenOperation(
              "NewsGate::Moderation::Category::ManagerImpl::"
              "update_word_lists: root category can not be updated");
          }

          CategoryDescriptor_var res;
          
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();            
          
          El::MySQL::Result_var qresult = connection->query("begin");

          try
          {
            increment_update_number(connection);

            CategoryDescriptor_var former_category;

            try
            {
              former_category = get_category(category_id, connection);

              write_category_word_lists(connection,
                                        category_id,
                                        word_lists,
                                        category_manager,
                                        true,
                                        former_category);
              
              res = get_category(category_id, connection);
            }
            catch(const CategoryNotFound&)
            {
              std::ostringstream ostr;
              ostr << "unexpected CategoryNotFound caught for category "
                   << category_id;

              throw Exception(ostr.str());
            }
            catch(const NoPath& e)
            {
              std::ostringstream ostr;
              ostr << "unexpected NoPath (" << e.id << "/" <<
                e.name <<") caught for category " << category_id;

              throw Exception(ostr.str());
            }
            catch(const Cycle& e)
            {
              std::ostringstream ostr;
              ostr << "unexpected Cycle (" << e.id << "/" <<
                e.name <<") caught for category " << category_id;

              throw Exception(ostr.str());
            }            

            validate_category_expressions(connection,
                                          category_id,
                                          res->expressions,
                                          res->word_lists);
            
            CategoryChange cat_change(*res,
                                      *former_category,
                                      moderator_id,
                                      moderator_name,
                                      ip);
              
            cat_change.save(connection);
            
            qresult = connection->query("commit");
          }
          catch(...)
          {
            qresult = connection->query("rollback");
            throw;
          }  

          return res._retn();
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::Category::ManagerImpl::"
            "update_word_lists: El::Exception caught. Description:\n"
               << e;

          ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
      }
      
      ::NewsGate::Moderation::Category::CategoryDescriptor*
      ManagerImpl::update_category(
        const ::NewsGate::Moderation::Category::CategoryDescriptor& category,
        ::NewsGate::Moderation::Category::ModeratorId moderator_id,
        const char* moderator_name,
        const char* ip)
        throw(NewsGate::Moderation::Category::CategoryNotFound,
              NewsGate::Moderation::Category::NoPath,
              NewsGate::Moderation::Category::Cycle,
              NewsGate::Moderation::Category::ForbiddenOperation,
              NewsGate::Moderation::Category::WordListNotFound,
              NewsGate::Moderation::Category::ExpressionParseError,
              NewsGate::Moderation::Category::VersionMismatch,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        try
        {
          if(category.id == ROOT_CATEGORY_ID)
          {
            throw ForbiddenOperation(
              "NewsGate::Moderation::Category::ManagerImpl::"
              "update_category: root category can not be updated");
          }
          
          char status = category_status(category.status);
          char searcheable = category_searcheable(category.searcheable);
          
          CategoryDescriptor_var res;

          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();
          
          El::MySQL::Result_var qresult = connection->query("begin");

          try
          {
            increment_update_number(connection);
            
            CategoryDescriptor_var former_category =
              get_category(category.id, connection);

            if(former_category->version != category.version)
            {
              std::ostringstream ostr;
              ostr << "NewsGate::Moderation::Category::ManagerImpl::"
                "update_category: category have already been updated";

              NewsGate::Moderation::Category::VersionMismatch ex;
              ex.description = CORBA::string_dup(ostr.str().c_str());
              throw ex;
            }
            
            {
              std::ostringstream ostr;
              ostr << "select id from Category where id=" << category.id;
              
              qresult = connection->query(ostr.str().c_str());

              El::MySQL::Row row(qresult);
              
              if(!row.fetch_row())
              {
                throw CategoryNotFound();
              }
            }

            CategoryPathElemSeq former_children =
              get_children(category.id, connection);

            {
              std::ostringstream ostr;
              ostr << "update Category set name='" <<
                connection->escape(category.name.in()) << "', status='"
                   << status << "', searcheable='" << searcheable << "', "
                "creator=" << category.creator_id << ", version="
                   << (category.version + 1) << ", description='" <<
                connection->escape(category.description.in()) << "'"
                   << " where id=" << category.id;

              qresult = connection->query(ostr.str().c_str());
            }

            {
              std::ostringstream ostr;
              ostr << "delete from CategoryChild where child_id="
                   << category.id;

              qresult = connection->query(ostr.str().c_str());
            }

            write_category_parents(connection,
                                   category.id,
                                   category.parents);

            {
              std::ostringstream ostr;
              ostr << "delete from CategoryChild where id="
                   << category.id;

              qresult = connection->query(ostr.str().c_str());
            }

            write_category_children(connection,
                                    category.id,
                                    category.children);

            {
              std::ostringstream ostr;
              ostr << "delete from CategoryLocale where category_id="
                   << category.id;

              qresult = connection->query(ostr.str().c_str());
            }

            write_category_locales(connection, category.id, category.locales);

            {
              std::ostringstream ostr;
              ostr << "delete from CategoryExpression where category_id="
                   << category.id;

              qresult = connection->query(ostr.str().c_str());
            }

            write_category_expressions(connection,
                                       category.id,
                                       category.expressions,
                                       category.word_lists,
                                       &former_category.in());

            res = get_category(category.id, connection);
            check_name_uniqueness(*res, connection);
            
            //
            // Checking that former children didn't loose path to root
            //
            for(size_t i = 0; i < former_children.length(); i++)
            {
              CategoryIdSet context;
              CategoryPathElem& child = former_children[i];          
              
              get_full_paths(child.id, child.name.in(), context, connection);
            }

            CategoryChange cat_change(*res,
                                      *former_category,
                                      moderator_id,
                                      moderator_name,
                                      ip); 
              
            cat_change.save(connection);
           
            qresult = connection->query("commit");
          }
          catch(...)
          {
            qresult = connection->query("rollback");
            throw;
          }  

          return res._retn();
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::Category::ManagerImpl::"
            "update_category: El::Exception caught. Description:\n"
               << e;

          ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
      }

      void
      ManagerImpl::add_category_message(
        ::NewsGate::Moderation::Category::CategoryId category_id,
        ::CORBA::ULongLong message_id,
        ::CORBA::Char relation,
        ::NewsGate::Moderation::Category::ModeratorId moderator_id,
        const char* moderator_name,
        const char* ip)
        throw(NewsGate::Moderation::Category::CategoryNotFound,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();
          
          El::MySQL::Result_var qresult = connection->query("begin");
          
          try
          {
            increment_update_number(connection);

            CategoryDescriptor_var former_category;
            CategoryDescriptor_var res;

            try
            {
              former_category = get_category(category_id, connection);
            
              El::MySQL::Result_var qresult;
            
              {
                std::ostringstream ostr;
                ostr << "select id, name, status, searcheable, updated, "
                  "created, creator, version, description "
                  "from Category where id=" << category_id;
              
                qresult = connection->query(ostr.str().c_str());
              }
            
              CategoryDesc cat(qresult.in());
            
              if(!cat.fetch_row())
              {
                throw CategoryNotFound();
              }

              {
                std::ostringstream ostr;
                ostr << "insert into CategoryMessage set category_id=" <<
                  category_id << ", message_id="
                     << message_id << ", relation='" << relation
                     << "' on duplicate key update relation='" << relation
                     << "'";

                qresult = connection->query(ostr.str().c_str());              
              }

              res = get_category(category_id, connection);
            }
            catch(const NoPath& e)
            {
              std::ostringstream ostr;
              ostr << "unexpected NoPath (" << e.id << "/" <<
                e.name <<") caught for category " << category_id;
              
              throw Exception(ostr.str());
            }
            catch(const Cycle& e)
            {
              std::ostringstream ostr;
              ostr << "unexpected Cycle (" << e.id << "/" <<
                e.name <<") caught for category " << category_id;
              
              throw Exception(ostr.str());
            }
            
            CategoryChange cat_change(*res,
                                      *former_category,
                                      moderator_id,
                                      moderator_name,
                                      ip);
              
            cat_change.save(connection);
            
            qresult = connection->query("commit");
          }
          catch(...)
          {
            qresult = connection->query("rollback");
            throw;
          }          
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::Category::ManagerImpl::"
            "add_category_message: El::Exception caught. Description:\n"
               << e;
          
          ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
      }
      
      ::NewsGate::Moderation::Category::CategoryDescriptor*
      ManagerImpl::find_category(const char* path)
        throw(NewsGate::Moderation::Category::CategoryNotFound,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        try
        {
          CategoryDescriptor_var res;
         
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();
          
          El::MySQL::Result_var qresult = connection->query("begin");
          
          try
          {
            CategoryId id = find_category(path, connection.in());

            try
            {
              res = get_category(id, connection.in());
            }
            catch(const NoPath& e)
            {
              std::ostringstream ostr;
              ostr << "unexpected NoPath (" << e.id << "/" <<
                e.name <<") caught for category " << id;

              throw Exception(ostr.str());
            }
            catch(const Cycle& e)
            {
              std::ostringstream ostr;
              ostr << "unexpected Cycle (" << e.id << "/" <<
                e.name <<") caught for category " << id;

              throw Exception(ostr.str());
            }            
            
            qresult = connection->query("commit");
          }
          catch(...)
          {
            qresult = connection->query("rollback");
            throw;
          }        
        
          return res._retn();
        }
        catch(const NoPath& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::Category::ManagerImpl::"
            "find_category: NoPath caught. id "
               << e.id << ", name " << e.name.in();
          
          ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;          
        }
        catch(const Cycle& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::Category::ManagerImpl::"
            "find_category: Cycle caught. id "
               << e.id << ", name " << e.name.in();
          
          ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;          
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::Category::ManagerImpl::"
            "find_category: El::Exception caught. Description:\n"
               << e;
          
          ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
      }
      
      ::NewsGate::Moderation::Category::CategoryDescriptor*
      ManagerImpl::get_category(
        ::NewsGate::Moderation::Category::CategoryId id)
        throw(NewsGate::Moderation::Category::CategoryNotFound,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        try
        {
          CategoryDescriptor_var res;
         
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();
          
          El::MySQL::Result_var qresult = connection->query("begin");
          
          try
          {
            res = get_category(id, connection.in());
            
            qresult = connection->query("commit");
          }
          catch(...)
          {
            qresult = connection->query("rollback");
            throw;
          }          
        
          return res._retn();
        }
        catch(const NoPath& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::Category::ManagerImpl::"
            "get_category: NoPath caught. id "
               << e.id << ", name " << e.name.in();
          
          ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;          
        }
        catch(const Cycle& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::Category::ManagerImpl::"
            "get_category: Cycle caught. id "
               << e.id << ", name " << e.name.in();
          
          ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;          
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::Category::ManagerImpl::"
            "get_category: El::Exception caught. Description:\n"
               << e;
          
          ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
      }

      ::NewsGate::Moderation::Category::CategoryDescriptor*
      ManagerImpl::get_category_version(
        ::NewsGate::Moderation::Category::CategoryId id)
        throw(NewsGate::Moderation::Category::CategoryNotFound,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException)
      {
        try
        {
          CategoryDescriptor_var res;
         
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();
          
          El::MySQL::Result_var qresult = connection->query("begin");
          
          try
          {
            res = new CategoryDescriptor();

            El::MySQL::Result_var qresult;
        
            {
              std::ostringstream ostr;
              ostr << "select version from Category where id=" << id;
              
              qresult = connection->query(ostr.str().c_str());

              CategoryVer cat(qresult.in());
            
              if(!cat.fetch_row())
              {
                throw CategoryNotFound();
              }

              res->version = cat.version();
            }

            {
              std::ostringstream ostr;
              ostr << "select name, version from CategoryWordList where "
                "category_id=" << id;
              
              qresult = connection->query(ostr.str().c_str());
              
              WordListDescriptorSeq& word_lists = res->word_lists;
              word_lists.length(qresult->num_rows());
              
              CategoryWordListVer wl(qresult.in());
              
              for(size_t i = 0; wl.fetch_row(); i++)
              {
                WordListDescriptor& desc = word_lists[i];                
                desc.name = wl.name().c_str();
                desc.version = wl.version();
              }
            }
            
            qresult = connection->query("commit");
          }
          catch(...)
          {
            qresult = connection->query("rollback");
            throw;
          }          
        
          return res._retn();
        }
        catch(const NoPath& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::Category::ManagerImpl::"
            "get_category: NoPath caught. id "
               << e.id << ", name " << e.name.in();
          
          ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;          
        }
        catch(const Cycle& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::Category::ManagerImpl::"
            "get_category: Cycle caught. id "
               << e.id << ", name " << e.name.in();
          
          ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;          
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::Category::ManagerImpl::"
            "get_category: El::Exception caught. Description:\n"
               << e;
          
          ImplementationException ex;
          ex.description = ostr.str().c_str();
          throw ex;
        }
      }

      CategoryId
      ManagerImpl::find_category(const char* path,
                                 El::MySQL::Connection* connection)
        throw(NewsGate::Moderation::Category::CategoryNotFound,
              Exception,
              El::Exception)
      {
        CategoryId id = 1;
        El::MySQL::Result_var qresult;

        El::String::ListParser parser(path, "/");

        const char* name = 0;
        while((name = parser.next_item()) != 0)
        {
          std::ostringstream ostr;

          ostr << "select CategoryChild.child_id as id, Category.name as name "
            "from CategoryChild left join Category on "
            "CategoryChild.child_id = Category.id where CategoryChild.id="
               << id;

          qresult = connection->query(ostr.str().c_str());

          CategoryId child_id = 0;
          CategoryChild rec(qresult.in());

          while(!child_id && rec.fetch_row())
          {
            std::string child_name = rec.name();
            
            if(child_name == name)
            {
              child_id = rec.id();
            }
          }

          if(child_id == 0)
          {
            throw CategoryNotFound();
          }

          id = child_id;
        }
        
        return id;
      }

      NewsGate::Moderation::Category::CategoryVersion
      ManagerImpl::get_category_version(CategoryId id,
                                        El::MySQL::Connection* connection)
        throw(NewsGate::Moderation::Category::CategoryNotFound,
              Exception,
              El::Exception)
      {        
        std::ostringstream ostr;
        ostr << "select id, name, status, searcheable, updated, "
          "created, creator, version from Category where id="
             << id;
        
        El::MySQL::Result_var qresult =
          connection->query(ostr.str().c_str());
        
        CategoryDesc cat(qresult.in(), 8);
        
        if(!cat.fetch_row())
        {
          throw CategoryNotFound();
        }

        return cat.version();
      }
      
      CategoryDescriptor*
      ManagerImpl::get_category(CategoryId id,
                                El::MySQL::Connection* connection)
        throw(NewsGate::Moderation::Category::CategoryNotFound,
              NewsGate::Moderation::Category::NoPath,
              NewsGate::Moderation::Category::Cycle,
              Exception,
              El::Exception)
      {        
        CategoryDescriptor_var res = new CategoryDescriptor();

        El::MySQL::Result_var qresult;
        
        {
          std::ostringstream ostr;
          ostr << "select id, name, status, searcheable, updated, "
            "created, creator, version, description from Category where id="
               << id;
              
          qresult = connection->query(ostr.str().c_str());

          CategoryDesc cat(qresult.in());
            
          if(!cat.fetch_row())
          {
            throw CategoryNotFound();
          }

          category_from_db(*res, cat);              
        }
            
        {
          std::ostringstream ostr;
          ostr << "select lang, country, name, title, short_title, "
            "description, keywords from CategoryLocale where category_id=" << id;
              
          qresult = connection->query(ostr.str().c_str());
            
          LocaleDescriptorSeq& locales = res->locales;
          locales.length(qresult->num_rows());
              
          CategoryLocaleDesc exp(qresult.in());
              
          for(size_t i = 0; exp.fetch_row(); i++)
          {
            LocaleDescriptor& desc = locales[i];
            
            desc.lang = exp.lang();
            desc.country = exp.country();
            desc.name = exp.name().c_str();
            desc.title = exp.title().c_str();
            desc.short_title = exp.short_title().c_str();
            desc.description = exp.description().c_str();
            desc.keywords = exp.keywords().c_str();
          }
        }

        {
          std::ostringstream ostr;
          ostr << "select expression, description from "
            "CategoryExpression where category_id=" << id;
              
          qresult = connection->query(ostr.str().c_str());
            
          ExpressionDescriptorSeq& expressions = res->expressions;
          expressions.length(qresult->num_rows());
              
          CategoryExpressionDesc exp(qresult.in());
              
          for(size_t i = 0; exp.fetch_row(); i++)
          {
            ExpressionDescriptor& desc = expressions[i];                
            desc.expression = exp.expression().c_str();
            desc.description =
              exp.description().is_null() ? "" : exp.description().c_str();
          }
        }

        {
          std::ostringstream ostr;
          ostr << "select name, words, version, description from "
            "CategoryWordList where category_id=" << id
               << " order by name DESC";
              
          qresult = connection->query(ostr.str().c_str());
            
          WordListDescriptorSeq& word_lists = res->word_lists;
          word_lists.length(qresult->num_rows());
              
          CategoryWordListDesc wl(qresult.in());
              
          for(size_t i = 0; wl.fetch_row(); i++)
          {
            WordListDescriptor& desc = word_lists[i];                
            desc.name = wl.name().c_str();
            desc.words = wl.words().c_str();
            desc.version = wl.version();
            desc.description =
              wl.description().is_null() ? "" : wl.description().c_str();
          }
        }

        {
          std::ostringstream ostr;

          ostr << "select message_id, relation from CategoryMessage "
            "where category_id=" << id;
        
          qresult = connection->query(ostr.str().c_str());
        
          CategoryMessage msg(qresult.in());

          while(msg.fetch_row())
          {
            MessageIdSeq& msg_seq = msg.relation().value()[0] == 'E' ?
              res->excluded_messages : res->included_messages;

            size_t len = msg_seq.length();
            msg_seq.length(len + 1);
            msg_seq[len] = msg.message_id();
          }
        }

        {
          std::ostringstream ostr;
          ostr << "select Category.id as id, Category.name as name, "
            "Category.status as status, "
            "Category.searcheable as searcheable, "
            "Category.updated as updated, "
            "Category.creator as created, Category.creator as creator, "
            "Category.version as version, Category.description as description "
            "from CategoryChild left join Category on "
            "CategoryChild.id = Category.id where CategoryChild.child_id="
               << id;
              
          qresult = connection->query(ostr.str().c_str());

          CategoryDescriptorSeq& parents = res->parents;
          parents.length(qresult->num_rows());
              
          CategoryDesc cat(qresult.in());
              
          for(size_t i = 0; cat.fetch_row(); i++)
          {
            category_from_db(parents[i], cat);
          }
        }

        CategoryDescriptorSeq& children = res->children;
            
        {
          std::ostringstream ostr;
          ostr << "select Category.id as id, Category.name as name, "
            "Category.status as status, "
            "Category.searcheable as searcheable, "
            "Category.updated as updated, "
            "Category.creator as created, Category.creator as creator, "
            "Category.version as version, Category.description as description "
            "from CategoryChild left join Category on "
            "CategoryChild.child_id = Category.id where CategoryChild.id="
               << id;
              
          qresult = connection->query(ostr.str().c_str());

          children.length(qresult->num_rows());
              
          CategoryDesc cat(qresult.in());
              
          for(size_t i = 0; cat.fetch_row(); i++)
          {
            category_from_db(children[i], cat);              
          }
        }

        fill_paths(*res, connection);

        return res._retn();
      }
      
      void
      ManagerImpl::fill_paths(CategoryDescriptor& desc,
                              El::MySQL::Connection* connection)
        throw(NewsGate::Moderation::Category::NoPath,
              NewsGate::Moderation::Category::Cycle,
              Exception,
              El::Exception)
      {
        {
          CategoryIdSet context;
        
          desc.paths =
            get_full_paths(desc.id, desc.name.in(), context, connection);
        }
        
        CategoryDescriptorSeq& children = desc.children;

        for(size_t i = 0; i < children.length(); i++)
        {
          CategoryIdSet context;
          CategoryDescriptor& child = children[i];          
          
          child.paths = get_full_paths(child.id,
                                       child.name.in(),
                                       context,
                                       connection);
        }

        CategoryDescriptorSeq& parents = desc.parents;
        
        for(size_t i = 0; i < parents.length(); i++)
        {
          CategoryIdSet context;
          CategoryDescriptor& parent = parents[i];          
          
          parent.paths = get_full_paths(parent.id,
                                        parent.name.in(),
                                        context,
                                        connection);
        }
      }

      CategoryPathSeq
      ManagerImpl::get_full_paths(const CategoryId& id,
                                  const char* name,
                                  CategoryIdSet& context,
                                  El::MySQL::Connection* connection)
        throw(NewsGate::Moderation::Category::NoPath,
              NewsGate::Moderation::Category::Cycle,
              Exception,
              El::Exception)
      {
        CategoryPathSeq paths;
        
        if(id == ROOT_CATEGORY_ID)
        {
          paths.length(1);
          
          paths[0].elems.length(1);
          paths[0].elems[0].id = id;

          paths[0].path = "/";

          return paths;
        }

        CategoryPathElemSeq parents = get_parents(id, connection);

        context.insert(id);

        for(size_t i = 0; i < parents.length(); i++)
        {
          const CategoryPathElem& parent = parents[i];
          
          if(context.find(parent.id) != context.end())
          {
            throw Cycle(id, name);
          }

          try
          {
            CategoryPathSeq parent_paths =
              get_full_paths(parent.id, parent.name.in(), context, connection);

            unsigned long start = paths.length();
            paths.length(start + parent_paths.length());

            for(size_t j = 0; j < parent_paths.length(); j++)
            {
              CategoryPath& dest = paths[start + j];
              const CategoryPath& src = parent_paths[j];
              
              dest.path = (std::string(src.path.in()) + name + "/").c_str();
              
              dest.elems = src.elems;
              
              size_t len = dest.elems.length();
              dest.elems.length(len + 1);
              
              dest.elems[len].id = id;
              dest.elems[len].name = name;
            }
          }
          catch(const NoPath& )
          {
            continue;
          }
        }
        
        context.erase(id);

        if(paths.length() == 0)
        {
          throw NoPath(id, name);
        }
          
        return paths;
      }

      CategoryPathElemSeq
      ManagerImpl::get_parents(const CategoryId& id,
                               El::MySQL::Connection* connection)
        throw(Exception, El::Exception)
      {
        CategoryPathElemSeq parents;

        std::ostringstream ostr;
        ostr << "select Category.id as id, Category.name as name from "
          "CategoryChild left join Category on CategoryChild.id = Category.id "
          "where CategoryChild.child_id=" << id;
        
        El::MySQL::Result_var qresult = connection->query(ostr.str().c_str());

        parents.length(qresult->num_rows());
        
        CategoryDesc cat(qresult.in(), 2);
        
        for(size_t i = 0; cat.fetch_row(); i++)
        {
          parents[i].id = cat.id();
          parents[i].name = cat.name().c_str();
        }
      
        return parents;
      }
      
      CategoryPathElemSeq
      ManagerImpl::get_children(const CategoryId& id,
                                El::MySQL::Connection* connection)
        throw(Exception, El::Exception)
      {
        CategoryPathElemSeq children;

        std::ostringstream ostr;
        ostr << "select Category.id as id, Category.name as name from "
          "CategoryChild left join Category on CategoryChild.child_id = "
          "Category.id where CategoryChild.id=" << id;
        
        El::MySQL::Result_var qresult = connection->query(ostr.str().c_str());

        children.length(qresult->num_rows());
        
        CategoryDesc cat(qresult.in(), 2);
        
        for(size_t i = 0; cat.fetch_row(); i++)
        {
          children[i].id = cat.id();
          children[i].name = cat.name().c_str();
        }
      
        return children;
      }
      
      bool
      ManagerImpl::notify(El::Service::Event* event)
        throw(El::Exception)
      {
        if(El::Service::CompoundService<>::notify(event))
        {
          return true;
        }
        
        return true;
      }

      CategoryStatus
      ManagerImpl::category_status(const char* status)
        throw(Exception, El::Exception)
      {
        switch(status[0])
        {
        case 'E': return CS_ENABLED;
        case 'D': return CS_DISABLED;
        }
        
        std::ostringstream ostr;
        ostr << "NewsGate::Moderation::Category::ManagerImpl::"
          "category_status: uexpected status " << status;
        
        throw Exception(ostr.str());
      }

      CategorySearcheable
      ManagerImpl::category_searcheable(const char* searcheable)
        throw(Exception, El::Exception)
      {
        switch(searcheable[0])
        {
        case 'Y': return CR_YES;
        case 'N': return CR_NO;
        }
        
        std::ostringstream ostr;
        ostr << "NewsGate::Moderation::Category::ManagerImpl::"
          "category_status: uexpected searcheable flag " << searcheable;
        
        throw Exception(ostr.str());
      }

      void
      ManagerImpl::category_from_db(CategoryDescriptor& desc,
                                    const CategoryDesc& cat)
        throw(El::Exception)
      {
        desc.id = cat.id();
        desc.name = cat.name().c_str();
        desc.status = category_status(cat.status().c_str());
        desc.searcheable = category_searcheable(cat.searcheable().c_str());
        
        desc.description = cat.description().is_null() ? "" :
          cat.description().c_str();
        
        desc.creator_id = cat.creator();
        desc.version = cat.version();
      }

      //
      // PhraseCollector class
      //
      PhraseCollector::PhraseCollector(
        const Message::Transport::StoredMessageDebug* cur_msg,
        size_t phrase_len,
          bool dump)
          throw(El::Exception) 
          : current_msg_(cur_msg),
            phrase_len_(phrase_len),
            dump_(dump)
      {
        const Message::CoreWords& core_words = cur_msg->message.core_words;

//        std::cerr << "MSG " << cur_msg->message.id.string() << " "
//                  << (uint32_t)core_words.size();

        if(!core_words.empty())
        {
          for(Message::CoreWords::const_iterator i(core_words.begin()),
                e(core_words.end()); i != e; ++i)
          {
            core_words_.insert(*i);
//            std::cerr << " " << *i;
          }
        }

//        std::cerr << std::endl;
      }
      
      bool
      PhraseCollector::word(const char* text,
                            Message::WordPosition position)
        throw(El::Exception)
      {
//        std::cout << position << ":" << text << ":";

        const Message::WordsFreqInfo::PositionInfoMap& word_positions =
          current_msg_->debug_info.words_freq.word_positions;

        Message::WordsFreqInfo::PositionInfoMap::const_iterator
          it = word_positions.find(position);

        assert(it != word_positions.end());

        const Message::WordsFreqInfo::PositionInfo& pi = it->second;
        El::Dictionary::Morphology::WordId norm_form = pi.norm_form;

        if(!norm_form)
        {
          std::string uniformed;
          El::String::Manip::utf8_to_uniform(text, uniformed);
          norm_form = El::Dictionary::Morphology::pseudo_id(uniformed.c_str());
        }

        if(words_.size() == phrase_len_)
        {
//          words_.pop_front();
          words_.erase(words_.begin());
        }
        
        words_.push_back(
          Word(text,
               norm_form,
               pi.token_type == Message::WordPositions::TT_STOP_WORD ?
               Word::WF_STOP_WORD : 0));

        if(dump_)
        {
          std::cerr << text << ":" << norm_form << " ";
        }
            
//        std::cout << norm_form << "/" << words_.size() << " ";

        if(words_.size() == phrase_len_)
        {
          bool valid_word = false;
          
          for(WordArray::const_iterator i(words_.begin()), e(words_.end());
              i != e && !valid_word; ++i)
          {
            valid_word = core_words_.find(i->id) != core_words_.end();
          }

          if(valid_word)
          {
            phrases_.insert(Phrase(words_));

            if(dump_)
            {
              std::cerr << "PH: " << Phrase(words_).text() << " :HP ";
            }
          }
        }
        
        return true;
      }
    }
  }
}
