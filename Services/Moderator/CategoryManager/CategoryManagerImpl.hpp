/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/CategoryManager/CategoryManagerImpl.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MODERATOR_CATEGORYMANAGER_CATEGORYMANAGERIMPL_HPP_
#define _NEWSGATE_SERVER_SERVICES_MODERATOR_CATEGORYMANAGER_CATEGORYMANAGERIMPL_HPP_

#include <string>
#include <set>
#include <vector>
#include <deque>
#include <list>
#include <iostream>
#include <ext/hash_map>
#include <ext/hash_set>

#include <google/dense_hash_set>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/MySQL/DB.hpp>
#include <El/Service/CompoundService.hpp>
#include <El/CRC.hpp>

#include <Commons/Message/StoredMessage.hpp>
#include <Commons/Message/TransportImpl.hpp>
#include <Commons/Message/Categorizer.hpp>
#include <Commons/Search/SearchExpression.hpp>

#include <Services/Commons/Message/MessageServices.hpp>

#include <Services/Moderator/Commons/CategoryManager.hpp>
#include <Services/Moderator/Commons/CategoryManager_s.hpp>

#include "DB_Record.hpp"

namespace NewsGate
{
  namespace Moderation
  {
    namespace Category
    {
      struct Word
      {
        enum Flags
        {
          WF_STOP_WORD = 0x1
        };
        
        uint8_t flags;
        std::string text;
        El::Dictionary::Morphology::WordId id;

        Word() throw();        
        Word(const char* t,
             El::Dictionary::Morphology::WordId i = 0,
             uint8_t f = 0) throw();
      };

      typedef std::vector<Word> WordArray;

      struct Phrase;
      typedef std::vector<Phrase> PhraseArray;
      
      struct Phrase
      {
        enum Flag
        {
          PF_TRIVIAL_COMPLEMENT = 0x1
        };
        
        uint8_t flags;
        uint64_t id;
        WordArray words;
        unsigned long occurances;
        unsigned long occurances_irrelevant;
        double occurances_freq_excess;

        Phrase() throw();
        Phrase(const WordArray& words) throw(El::Exception);
        
        Phrase(WordArray::const_iterator from,
               WordArray::const_iterator to,
               uint8_t fl)
          throw(El::Exception);

        std::string text() const throw(El::Exception);

        bool operator==(const Phrase& p) const throw();
        bool contains(const Phrase& p) const throw();

        void subphrases(PhraseArray& subs, bool complement = false) const
          throw(El::Exception);

        void dump(std::ostream& ostr) const throw(El::Exception);

      private:
        
        void init(WordArray::const_iterator from, WordArray::const_iterator to)
          throw(El::Exception);
      };

      struct PhraseHash
      {
        size_t operator()(const Phrase& p) const throw() { return p.id; }
      };

      typedef __gnu_cxx::hash_set<Phrase, PhraseHash> PhraseSet;
      typedef __gnu_cxx::hash_map<Phrase, size_t, PhraseHash> PhraseCounter;

      typedef __gnu_cxx::hash_map<std::string,
                                  ::El::Dictionary::Morphology::WordInfo,
                                  El::Hash::String>
      WordInfoMap;

      struct TimePhraseArray
      {
        uint64_t time;
        PhraseArray phrases;

        TimePhraseArray() throw(El::Exception) {}
        TimePhraseArray(uint64_t t, const PhraseArray& p) throw(El::Exception);
      };
      
      typedef __gnu_cxx::hash_map<Message::Id,
                                  TimePhraseArray,
                                  Message::MessageIdHash>
      MessagePhraseMap;

      class PhraseIdSet:
        public google::dense_hash_set<uint64_t, El::Hash::Numeric<uint64_t> >
      {
      public:
        PhraseIdSet() throw() { set_empty_key(UINT64_MAX); set_deleted_key(UINT64_MAX - 1); }
      };  
      
      class PhraseCollector :
        public Message::StoredMessage::MessageBuilder
      {
      public:
        PhraseCollector(
          const Message::Transport::StoredMessageDebug* cur_msg,
          size_t phrase_len,
          bool dump)
          throw(El::Exception);

        virtual ~PhraseCollector() throw() {}

        const PhraseSet& phrases() const throw() { return phrases_; }
        void reset_words() throw() { words_.clear(); }

      private:

        virtual bool word(const char* text, Message::WordPosition position)
          throw(El::Exception);

        virtual bool interword(const char* text) throw(El::Exception);
        virtual bool segmentation() throw(El::Exception) { return true; }

      private:

        class WordIdSet:
          public google::dense_hash_set<El::Dictionary::Morphology::WordId,
                                        El::Hash::Numeric<uint32_t> >
        {
        public:
          WordIdSet() throw() { set_empty_key(UINT32_MAX); set_deleted_key(UINT32_MAX - 1); }
        };
        
        const Message::Transport::StoredMessageDebug* current_msg_;
        WordArray words_;
        size_t phrase_len_;
        PhraseSet phrases_;
        bool dump_;
        WordIdSet core_words_;
      };  
      
      class ManagerImpl :
        public virtual POA_NewsGate::Moderation::Category::Manager,
        public virtual El::Service::CompoundService<>
      {
      public:
        EL_EXCEPTION(Exception, El::ExceptionBase);
        EL_EXCEPTION(InvalidArgument, Exception);

      public:
        ManagerImpl(El::Service::Callback* callback)
          throw(InvalidArgument, Exception, El::Exception);

        virtual ~ManagerImpl() throw() {}

      protected:

        //
        // IDL:NewsGate/Moderation/Category/Manager/category_relevant_phrases:1.0
        //
        virtual ::NewsGate::Moderation::Category::RelevantPhrases*
        category_relevant_phrases(
          ::NewsGate::Moderation::Category::CategoryId id,
          const char* lang,
          const ::NewsGate::Moderation::Category::PhraseIdSeq& skip_phrases)
        throw(NewsGate::Moderation::Category::CategoryNotFound,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);
        
        //
        // IDL:NewsGate/Moderation/Category/Manager/create_category:1.0
        //
        virtual ::NewsGate::Moderation::Category::CategoryDescriptor*
        create_category(
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
                ::CORBA::SystemException);
        
        //
        // IDL:NewsGate/Moderation/Category/Manager/get_category:1.0
        //
        virtual ::NewsGate::Moderation::Category::CategoryDescriptor*
        get_category(::NewsGate::Moderation::Category::CategoryId id)
          throw(NewsGate::Moderation::Category::CategoryNotFound,
                NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);

        //
        // IDL:NewsGate/Moderation/Category/Manager/get_category_version:1.0
        //
        virtual ::NewsGate::Moderation::Category::CategoryDescriptor*
        get_category_version(::NewsGate::Moderation::Category::CategoryId id)
          throw(NewsGate::Moderation::Category::CategoryNotFound,
                NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);

        //
        // IDL:NewsGate/Moderation/Category/Manager/find_category:1.0
        //
        virtual ::NewsGate::Moderation::Category::CategoryDescriptor*
        find_category(const char* path)
          throw(NewsGate::Moderation::Category::CategoryNotFound,
                NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);

        //
        // IDL:NewsGate/Moderation/Category/Manager/remove_category:1.0
        //
        virtual void add_category_message (
          ::NewsGate::Moderation::Category::CategoryId category_id,
          ::CORBA::ULongLong message_id,
          ::CORBA::Char relation,
          ::NewsGate::Moderation::Category::ModeratorId moderator_id,
          const char* moderator_name,
          const char* ip)
          throw(NewsGate::Moderation::Category::CategoryNotFound,
                NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);
        
        //
        // IDL:NewsGate/Moderation/Category/Manager/update_category:1.0
        //
        virtual ::NewsGate::Moderation::Category::CategoryDescriptor*
        update_category(
          const ::NewsGate::Moderation::Category::CategoryDescriptor& category,
          ::NewsGate::Moderation::Category::ModeratorId moderator_id,
          const char* moderator_name,
          const char* ip)
          throw(NewsGate::Moderation::Category::NoPath,
                NewsGate::Moderation::Category::Cycle,
                NewsGate::Moderation::Category::CategoryNotFound,
                NewsGate::Moderation::Category::ForbiddenOperation,
                NewsGate::Moderation::Category::WordListNotFound,
                NewsGate::Moderation::Category::ExpressionParseError,
                NewsGate::Moderation::Category::VersionMismatch,
                NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);

        //
        // IDL:NewsGate/Moderation/Category/Manager/update_word_lists:1.0
        //
        virtual ::NewsGate::Moderation::Category::CategoryDescriptor*
        update_word_lists(
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
                ::CORBA::SystemException);
        
        //
        // IDL:NewsGate/Moderation/Category/Manager/delete_categories:1.0
        //
        virtual void delete_categories(
          const ::NewsGate::Moderation::Category::CategoryIdSeq& ids,
          ::NewsGate::Moderation::Category::ModeratorId moderator_id,
          const char* moderator_name,
          const char* ip)
          throw(NewsGate::Moderation::Category::ForbiddenOperation,
                NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);

        //
        // IDL:NewsGate/Moderation/Category/Manager/find:1.0
        //
        virtual ::NewsGate::Moderation::Category::CategoryFindingSeq*
        find_text(const char* text)
          throw(NewsGate::Moderation::ImplementationException,
                ::CORBA::SystemException);
        
        virtual bool notify(El::Service::Event* event) throw(El::Exception);

        static CategoryStatus category_status(const char* status)
          throw(Exception, El::Exception);
        
        static CategorySearcheable category_searcheable(
          const char* searcheable)
          throw(Exception, El::Exception);

        static char category_status(CategoryStatus status)
          throw(Exception, El::Exception);
        
        static char category_searcheable(CategorySearcheable searcheable)
          throw(Exception, El::Exception);

        static void category_from_db(CategoryDescriptor& desc,
                                     const CategoryDesc& cat)
          throw(El::Exception);
        
        static void write_category_locales(
          El::MySQL::Connection* connection,
          unsigned long long cat_id,
          const LocaleDescriptorSeq& locales)
          throw(Exception, El::Exception);

        static void write_category_expressions(
          El::MySQL::Connection* connection,
          unsigned long long cat_id,
          const ExpressionDescriptorSeq& expressions,
          const WordListDescriptorSeq& word_lists,
          const CategoryDescriptor* former_category)
          throw(NewsGate::Moderation::Category::WordListNotFound,
                NewsGate::Moderation::Category::ExpressionParseError,
                NewsGate::Moderation::Category::VersionMismatch,
                Exception,
                El::Exception);

        static void validate_category_expressions(
          El::MySQL::Connection* connection,
          unsigned long long cat_id,
          const ExpressionDescriptorSeq& expressions,
          const WordListDescriptorSeq& word_lists)
          throw(NewsGate::Moderation::Category::WordListNotFound,
                NewsGate::Moderation::Category::ExpressionParseError,
                Exception,
                El::Exception);

        static void remove_general_phrases(const PhraseSet& phrases,
                                           PhraseCounter& relevant_phrases)
          throw(El::Exception);
        
        static void write_category_expressions(
          El::MySQL::Connection* connection,
          unsigned long long cat_id,
          const ExpressionDescriptorSeq& expressions)
          throw(Exception, El::Exception);

        static void write_category_word_lists(
          El::MySQL::Connection* connection,
          unsigned long long cat_id,
          const WordListDescriptorSeq& word_lists,
          bool category_manager,
          bool subset,
          const CategoryDescriptor* former_category)
          throw(NewsGate::Moderation::Category::WordListNotFound,
                NewsGate::Moderation::Category::VersionMismatch,
                Exception,
                El::Exception);

        void write_category_parents(
          El::MySQL::Connection* connection,
          unsigned long long cat_id,
          const CategoryDescriptorSeq& parents)
          throw(Exception,
                El::Exception);
        
        void write_category_children(
          El::MySQL::Connection* connection,
          unsigned long long cat_id,
          const CategoryDescriptorSeq& children)
          throw(NewsGate::Moderation::Category::ForbiddenOperation,
                Exception,
                El::Exception);
        
        typedef std::set<CategoryId> CategoryIdSet;

        static CategoryPathSeq get_full_paths(
          const CategoryId& id,
          const char* name,
          CategoryIdSet& context,
          El::MySQL::Connection* connection)
          throw(NewsGate::Moderation::Category::NoPath,
                NewsGate::Moderation::Category::Cycle,
                Exception,
                El::Exception);
        
        static CategoryPathElemSeq get_parents(
          const CategoryId& id,
          El::MySQL::Connection* connection)
          throw(Exception, El::Exception);

        static CategoryPathElemSeq get_children(
          const CategoryId& id,
          El::MySQL::Connection* connection)
          throw(Exception, El::Exception);

        static void fill_paths(CategoryDescriptor& desc,
                               El::MySQL::Connection* connection)
          throw(NewsGate::Moderation::Category::NoPath,
                NewsGate::Moderation::Category::Cycle,
                Exception,
                El::Exception);

        static CategoryDescriptor*
        get_category(CategoryId id, El::MySQL::Connection* connection)
          throw(NewsGate::Moderation::Category::CategoryNotFound,
                NewsGate::Moderation::Category::NoPath,
                NewsGate::Moderation::Category::Cycle,
                Exception,
                El::Exception);

        static NewsGate::Moderation::Category::CategoryVersion
        get_category_version(CategoryId id,
                             El::MySQL::Connection* connection)
          throw(NewsGate::Moderation::Category::CategoryNotFound,
                Exception,
                El::Exception);
        
        static CategoryId find_category(const char* path,
                                        El::MySQL::Connection* connection)
          throw(NewsGate::Moderation::Category::CategoryNotFound,
                Exception,
                El::Exception);
        
        static uint64_t increment_update_number(
          El::MySQL::Connection* connection)
          throw(Exception, El::Exception);

        static void unlink_category(CategoryId id,
                                    El::MySQL::Connection* connection)
          throw(Exception, El::Exception);
        
        static void check_name_uniqueness(const CategoryDescriptor& category,
                                          El::MySQL::Connection* connection)
          throw(NewsGate::Moderation::Category::ForbiddenOperation,
                Exception,
                El::Exception);

        static void check_name_uniqueness(CategoryId id,
                                          const char* path,
                                          El::MySQL::Connection* connection)
          throw(NewsGate::Moderation::Category::ForbiddenOperation,
                Exception,
                El::Exception);
        
        Search::Expression* segment_search_expression(const char* exp)
          throw(Exception,
                El::Exception,
                NewsGate::Moderation::Category::ExpressionParseError);

        Search::Expression*
        normalize_search_expression(Search::Expression* expression)
          throw(Exception, El::Exception);

        Message::BankClientSession* bank_client_session() throw(El::Exception);

        size_t count_phrases(const char* query,
                             const char* lang,
                             PhraseCounter& phrase_counter,
                             size_t& message_count,
                             ACE_Time_Value& search_time,
                             bool collapse_events,
                             const char* log_name = 0)
          throw(NewsGate::Moderation::Category::ExpressionParseError,
                El::Exception);

        void set_phrase_usefulness_flag(const char* query,
                                        const char* lang,
                                        PhraseOccuranceSeq& phrases)
          throw(NewsGate::Moderation::Category::ExpressionParseError,
                El::Exception);
        
        void get_phrases(
          const Message::Transport::StoredMessageArray& messages,
          size_t phrase_len,
          MessagePhraseMap& message_phrases,
          const char* log_name)
          throw(El::Exception);
        
        static std::string category_expression(
          Message::Categorizer::Category::Id id,
          El::MySQL::Connection* connection,
          const CategoryDescriptorSeq& children)
          throw(Exception, El::Exception);
        
        static void category_phrases(Message::Categorizer::Category::Id id,
                                     const char* lang,
                                     PhraseSet* any_phrases,
                                     PhraseSet* any2_phrases,
                                     PhraseSet* skip_phrases,
                                     PhraseSet* skip2_phrases,
                                     El::MySQL::Connection* connection,
                                     CategoryIdSet& processed_categories)
          throw(Exception, El::Exception);

        static void possible_phrases(WordArray::const_iterator begin,
                                     WordArray::const_iterator end,
                                     const WordInfoMap& words,
                                     PhraseArray& phrases)
          throw(El::Exception);
        
      private:

        Message::BankClientSession_var bank_client_session_;
        MessagePhraseMap message_phrases_;
      }; 
        
      typedef El::RefCount::SmartPtr<ManagerImpl> ManagerImpl_var;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Moderation
  {
    namespace Category
    {
      //
      // Word struct
      //
      inline
      Word::Word() throw()
          : flags(0),
            id(0)
      {
      }
      
      inline
      Word::Word(const char* t,
                 El::Dictionary::Morphology::WordId i,
                 uint8_t f) throw()
          : flags(f),
            text(t ? t : ""),
            id(i)
      {
      }

      //
      // Phrase struct
      //

      inline
      Phrase::Phrase() throw()
          : flags(0),
            id(0),
            occurances(0),
            occurances_irrelevant(0),
            occurances_freq_excess(0)
      {
      }
      
      inline
      Phrase::Phrase(const WordArray& wrd) throw(El::Exception)
          : flags(0),
            id(0),
            occurances(0),
            occurances_irrelevant(0),
            occurances_freq_excess(0)
      {
        init(wrd.begin(), wrd.end());
      }

      inline
      Phrase::Phrase(WordArray::const_iterator from,
                     WordArray::const_iterator to,
                     uint8_t fl)
          throw(El::Exception)
          : flags(fl),
            id(0),
            occurances(0),
            occurances_irrelevant(0),
            occurances_freq_excess(0)
      {
        init(from, to);
      }
            
      inline
      void
      Phrase::init(WordArray::const_iterator from, WordArray::const_iterator to)
        throw(El::Exception)
      {
        words.reserve(to - from);
        
        for(WordArray::const_iterator i(from); i != to; ++i)
        {
          const Word& w = *i;
          words.push_back(w);          
          El::CRC(id, (unsigned char*)&w.id, sizeof(w.id));
        }
      }

      inline
      std::string
      Phrase::text() const throw(El::Exception)
      {
        std::string res;
        
        for(WordArray::const_iterator b(words.begin()), i(b), e(words.end());
            i != e; ++i)
        {
          if(i != b)
          {
            res += " ";
          }

          res += i->text;          
        }

        return res;
      }

      inline
      void
      Phrase::dump(std::ostream& ostr) const throw(El::Exception)
      {
        ostr << text() << "/" << id;

        for(WordArray::const_iterator i(words.begin()), e(words.end());
            i != e; ++i)
        {
          ostr << " " << i->text << ":" << i->id;
        }
      }
      
      inline
      bool
      Phrase::operator==(const Phrase& p) const throw()
      {
        return id == p.id;
      }

      inline
      bool
      Phrase::contains(const Phrase& p) const throw()
      {
        if(words.size() < p.words.size())
        {
          return false;
        }

        size_t shifts = words.size() - p.words.size() + 1;

        for(size_t shift = 0; shift < shifts; ++shift)
        {
          bool match = true;
          WordArray::const_iterator j(words.begin() + shift);
        
          for(WordArray::const_iterator i(p.words.begin()), e(p.words.end());
              i != e && match; ++i, ++j)
          {
            if(i->id != j->id)
            {
              match = false;
            }
          }

          if(match)
          {
            return true;
          }
        }

        return false;
      }

      inline
      void
      Phrase::subphrases(PhraseArray& subs, bool complement) const
        throw(El::Exception)
      {
        for(WordArray::const_iterator b(words.begin()), i(b), e(words.end());
            i != e; ++i)
        {
          for(WordArray::const_iterator j(i); j != e; ++j)
          {
            WordArray::const_iterator next(j + 1);

            if(complement)
            {
              bool all_stops = true;
              
              for(WordArray::const_iterator k(b); k != i && all_stops; ++k)
              {
                all_stops = k->flags & Word::WF_STOP_WORD;
              }

              if(all_stops)
              {
                for(WordArray::const_iterator k(next); k != e && all_stops;
                    ++k)
                {
                  all_stops = k->flags & Word::WF_STOP_WORD;
                }
              }

              subs.push_back(
                Phrase(i,
                       next,
                       all_stops ? Phrase::PF_TRIVIAL_COMPLEMENT : 0));
            }
            else
            { 
              subs.push_back(Phrase(i, next, 0));
            }
          }
        }
      }

      //
      // TimePhraseArray struct
      //
      inline
      TimePhraseArray::TimePhraseArray(uint64_t t, const PhraseArray& p)
        throw(El::Exception)
          : time(t),
            phrases(p)
      {
      }

      //
      // PhraseCollector struct
      //
      inline
      bool
      PhraseCollector::interword(const char* text) throw(El::Exception)
      {
        for(const char* p = text; *p != '\0'; ++p)
        {
          if(!El::String::Unicode::CharTable::is_space(*p))
          {
            words_.clear();
            return true;
          }
        }
        
        return true;
      }
    }
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_MODERATOR_CATEGORYMANAGER_CATEGORYMANAGERIMPL_HPP_
