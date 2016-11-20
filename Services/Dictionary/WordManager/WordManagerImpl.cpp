/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Dictionary/WordManager/WordManagerImpl.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <unistd.h>
#include <limits.h>

#include <string>
#include <sstream>
#include <fstream>

#include <ace/OS.h>
#include <ace/High_Res_Timer.h>

#include <El/Moment.hpp>
#include <El/Utility.hpp>

#include <Commons/Message/StoredMessage.hpp>
#include <Commons/Search/TransportImpl.hpp>
#include <Services/Dictionary/Commons/TransportImpl.hpp>

#include "WordManagerImpl.hpp"
#include "WordManagerMain.hpp"

namespace
{
  const char ASPECT[] = "State";
}

namespace NewsGate
{
  namespace Dictionary
  {
    //
    // WordManagerImpl class
    //
    WordManagerImpl::WordManagerImpl(El::Service::Callback* callback)
      throw(InvalidArgument, Exception, El::Exception)
        : El::Service::CompoundService<>(callback, "WordManagerImpl")
    {
      El::Service::CompoundServiceMessage_var msg = new LoadDicts(this);
      deliver_now(msg.in());
    }

    WordManagerImpl::~WordManagerImpl() throw()
    {
      // Check if state is active, then deactivate and log error
    }
    
    ::CORBA::ULong
    WordManagerImpl::normalize_search_expression(
      ::NewsGate::Search::Transport::Expression* expression,
      ::NewsGate::Search::Transport::Expression_out result)
      throw(NewsGate::Dictionary::NotReady,
            NewsGate::Dictionary::ImplementationException,
            ::CORBA::SystemException)
    {
      try
      {
        ReadGuard guard(srv_lock_); 

        if(word_info_manager_.get() == 0)
        {
          NewsGate::Dictionary::NotReady ex;
        
          ex.reason = "WordManagerImpl::normalize_search_expression: "
            "dictionaries are not loaded yet";

          throw ex;
        }

        guard.release();

        Search::Transport::ExpressionImpl::Type* pack =
          dynamic_cast<Search::Transport::ExpressionImpl::Type*>(expression);

        if(pack == 0)
        {
          NewsGate::Dictionary::ImplementationException ex;
        
          ex.description =
            "WordManagerImpl::normalize_search_expression: dynamic_cast<"
            "Search::Transport::ExpressionImpl::Type*>(words) failed";

          throw ex;
        }

        NewsGate::Search::Expression_var expression =
          pack->entity().expression;

        expression->normalize(*word_info_manager_);

        result = Search::Transport::ExpressionImpl::Init::create(
          new Search::Transport::ExpressionHolder(
            expression.retn()));
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "WordManagerImpl::normalize_search_expression: El::Exception "
          "caught. Description: " << e;
        
        NewsGate::Dictionary::ImplementationException ex;        
        ex.description = ostr.str().c_str();

        throw ex;
      }

      return word_info_manager_->hash();
    }

    ::CORBA::ULong
    WordManagerImpl::normalize_words(
      const ::NewsGate::Dictionary::WordManager::WordSeq& words,
      const char* language,
      ::NewsGate::Dictionary::Transport::NormalizedWordsPack_out result)
      throw(NewsGate::Dictionary::NotReady,
            NewsGate::Dictionary::ImplementationException,
            ::CORBA::SystemException)
    {
      Transport::NormalizedWordsPackImpl::Var result_pack;
      
      try
      {
        ReadGuard guard(srv_lock_); 

        if(word_info_manager_.get() == 0)
        {
          NewsGate::Dictionary::NotReady ex;
        
          ex.reason = "WordManagerImpl::normalize_words: "
            "dictionaries are not loaded yet";

          throw ex;
        }

        guard.release();

        El::Lang lang;

        if(*language != '\0')
        {
          lang = El::Lang(language);
        }

//        std::cerr << lang.l3_code() << std::endl;
        
        ::El::Dictionary::Morphology::WordArray wrd(words.length());

        for(size_t i = 0; i < words.length(); ++i)
        {
          wrd[i] = words[i].in();
        }
      
        result_pack =
          new Transport::NormalizedWordsPackImpl::Type(
            new El::Dictionary::Morphology::WordInfoArray());        
      
        word_info_manager_->normal_form_ids(wrd,
                                            result_pack->entities(),
                                            lang == El::Lang::null ? 0 : &lang,
                                            false);
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "WordManagerImpl::normalize_words: El::Exception "
          "caught. Description: " << e;
        
        NewsGate::Dictionary::ImplementationException ex;        
        ex.description = ostr.str().c_str();

        throw ex;
      }

      result = result_pack._retn();
      return word_info_manager_->hash();      

      return 0;
    }
    
    ::CORBA::ULong
    WordManagerImpl::normalize_message_words(
      ::NewsGate::Dictionary::Transport::MessageWordsPack* words,
      ::NewsGate::Dictionary::Transport::NormalizedMessageWordsPack_out result)
      throw(NewsGate::Dictionary::NotReady,
            NewsGate::Dictionary::ImplementationException,
            ::CORBA::SystemException)
    {
      Transport::NormalizedMessageWordsPackImpl::Var result_pack;
      
      try
      {
        ReadGuard guard(srv_lock_); 

        if(word_info_manager_.get() == 0)
        {
          NewsGate::Dictionary::NotReady ex;
        
          ex.reason = "WordManagerImpl::normalize_message_words: "
            "dictionaries are not loaded yet";

          throw ex;
        }

        guard.release();

        Transport::MessageWordsPackImpl::Type* word_pack =
          dynamic_cast<Transport::MessageWordsPackImpl::Type*>(words);

        if(word_pack == 0)
        {
          NewsGate::Dictionary::ImplementationException ex;
        
          ex.description =
            "WordManagerImpl::normalize_message_words: dynamic_cast<"
            "Transport::MessageWordsPackImpl::Type*>(words) failed";

          throw ex;
        }
      
        Transport::MessageWordsArray& words_array = word_pack->entities();

        result_pack =
          new Transport::NormalizedMessageWordsPackImpl::Type(
            new Transport::NormalizedMessageWordsArray());

        Transport::NormalizedMessageWordsArray& res = result_pack->entities();
        res.resize(words_array.size());

        for(size_t i = 0; i < words_array.size(); i++)
        {
          const Transport::MessageWords& words_info = words_array[i];
          Transport::NormalizedMessageWords& res_info = res[i];

          El::Lang lang = words_info.language;

          const Message::MessageWordPosition& word_positions =
            words_info.word_positions;

          if(word_positions.size())
          {
            El::Dictionary::Morphology::WordArray words(word_positions.size());
            
            for(size_t j = 0; j < word_positions.size(); j++)
            {
              words[j] = word_positions[j].first.c_str();
            }

            El::Dictionary::Morphology::WordInfoArray word_infos;
            
            word_info_manager_->normal_form_ids(words,
                                                word_infos,
                                                &lang,
//                                                false);
// TODO: make "true" in final version
                                                true);
            
            Message::StoredMessage::set_normal_forms(
              word_infos,
              words_info.word_positions,
              words_info.positions,
              res_info.norm_form_positions,
              res_info.resulted_positions);

            res_info.word_infos.resize(word_positions.size());

            for(size_t j = 0; j < word_positions.size(); j++)
            {
              res_info.word_infos[j].lang = word_infos[j].lang;
              
/* TOREMOVE
//              res_info.word_infos[j].lang = El::Lang("EGY");

              if(strcmp(words[j].c_str(), "of") == 0 && lang.el_code() ==
                 El::Lang::EC_GER)
              {
                std::cerr << "XXX: " << words[j].c_str() << " "
                          << res_info.word_infos[j].lang.l3_code() << "\n";
              }
*/
            }

          }

          res_info.lang = lang;
        }
        
/*            
        for(unsigned long i = 0; i < words_array.size(); i++)
        { 
          Transport::NormalizedMessageWords& res_info = res[i];
          
          const Message::MessageWordPosition& word_positions =
            words_array[i].word_positions;

          std::cerr << "Pack ********************\n";

          for(unsigned long j = 0; j < word_positions.size(); j++)
          {
//            if(strcmp(word_positions[j].first.c_str(), "of") == 0 &&
//               res_info.language.el_code() == El::Lang::EC_GER)
            {
              std::cerr << "YYY: " << word_positions[j].first.c_str() << " "
                        << res_info.word_infos[j].lang.l3_code() << "\n";
            }
          }
        }
*/

      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "WordManagerImpl::normalize_message_words: El::Exception "
          "caught. Description: " << e;
        
        NewsGate::Dictionary::ImplementationException ex;        
        ex.description = ostr.str().c_str();

        throw ex;
      }

      result = result_pack._retn();
      return word_info_manager_->hash();
    }

    void
    WordManagerImpl::get_lemmas(
      ::NewsGate::Dictionary::Transport::GetLemmasParams* params,
      ::NewsGate::Dictionary::Transport::LemmaPack_out result)
      throw(NewsGate::Dictionary::NotReady,
            NewsGate::Dictionary::ImplementationException,
            ::CORBA::SystemException)
    {
      Transport::LemmaPackImpl::Var result_pack;
      
      try
      {
        ReadGuard guard(srv_lock_); 

        if(word_info_manager_.get() == 0)
        {
          NewsGate::Dictionary::NotReady ex;
        
          ex.reason = "WordManagerImpl::get_lemmas: "
            "dictionaries are not loaded yet";

          throw ex;
        }

        guard.release();

        Transport::GetLemmasParamsImpl::Type* params_impl =
          dynamic_cast<Transport::GetLemmasParamsImpl::Type*>(params);

        if(params == 0)
        {
          NewsGate::Dictionary::ImplementationException ex;
        
          ex.description =
            "WordManagerImpl::get_lemmas: dynamic_cast<"
            "Transport::Transport::GetLemmasParamsImpl::Type*>(params) failed";

          throw ex;
        }
      
        Transport::GetLemmasParamsStruct& prm = params_impl->entity();
        const Transport::WordsArray& prm_words = prm.words;
        
        El::Dictionary::Morphology::WordArray words(prm_words.size());

        for(size_t i = 0; i < prm_words.size(); i++)
        {
          words[i] = prm_words[i].c_str();
        }

        result_pack =
          new Transport::LemmaPackImpl::Type(
            new El::Dictionary::Morphology::LemmaInfoArrayArray());

        El::Dictionary::Morphology::LemmaInfoArrayArray& res =
          result_pack->entities();

        word_info_manager_->get_lemmas(
          words,
          prm.lang == El::Lang::null ? 0 : &prm.lang,
          prm.guess_strategy,
          res);
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "WordManagerImpl::get_lemmas: El::Exception "
          "caught. Description: " << e;
        
        NewsGate::Dictionary::ImplementationException ex;        
        ex.description = ostr.str().c_str();

        throw ex;
      }

      result = result_pack._retn();
    }

    ::CORBA::ULong
    WordManagerImpl::hash()
      throw(NewsGate::Dictionary::NotReady,
            NewsGate::Dictionary::ImplementationException,
            ::CORBA::SystemException)
    {
      ReadGuard guard(srv_lock_); 

      if(word_info_manager_.get() == 0)
      {
        NewsGate::Dictionary::NotReady ex;
        ex.reason = "WordManagerImpl::hash: dictionaries are not loaded yet";
        throw ex;
      }
      
      return word_info_manager_->hash();
    }
    
    ::CORBA::Boolean
    WordManagerImpl::is_ready()
      throw(NewsGate::Dictionary::ImplementationException,
            ::CORBA::SystemException)
    {
      ReadGuard guard(srv_lock_); 
      return word_info_manager_.get() != 0;
    }
    
    bool
    WordManagerImpl::notify(El::Service::Event* event) throw(El::Exception)
    {
      if(El::Service::CompoundService<>::notify(event))
      {
        return true;
      }

      if(dynamic_cast<LoadDicts*>(event) != 0)
      {
        load_dicts();
        return true;
      }

      return false;
    }

    void
    WordManagerImpl::load_dicts() throw(El::Exception)
    {
      const Server::Config::WordManagerType& config =
        Application::instance()->config();
      
      typedef Server::Config::WordManagerDictionariesType::dict_sequence
        DictsConf;

      const DictsConf& dicts = config.dictionaries().dict();

      El::Logging::Logger* logger = Application::logger();

      WordInfoManagerPtr word_info_manager(
        new El::Dictionary::Morphology::WordInfoManager(
          config.default_lang_validation_level(),
          config.guessing_default_lang_validation_level(),
          config.lang_validation_level()));

      size_t prev_mem = El::Utility::mem_used();
      
      for(DictsConf::const_iterator it = dicts.begin();
          
          it != dicts.end(); it++)
      {
        std::string filename = it->filename();
        
        {
          std::ostringstream ostr;
          ostr << "WordManagerImpl::load_dicts: loading " << filename
               << " ...";
          
          logger->info(ostr.str().c_str(), ASPECT);
        }

        std::ostringstream warnings_stream;
        
        ACE_High_Res_Timer timer;
        timer.start();

        word_info_manager->load(filename.c_str(), &warnings_stream);

        timer.stop();
        ACE_Time_Value tm;
        timer.elapsed_time(tm);

        size_t mem = El::Utility::mem_used();
        
        {
          std::string warnings = warnings_stream.str();

          if(!warnings.empty())
          {
            std::ostringstream ostr;
            ostr << "WordManagerImpl::load_dicts: " << warnings;
            
            logger->warning(ostr.str().c_str(), ASPECT);
          }
          
          std::ostringstream ostr;
          ostr << "WordManagerImpl::load_dicts: completed; time " <<
            El::Moment::time(tm) << "; mem "
               << (mem - prev_mem) / 1024 << " Mb";
          
          logger->info(ostr.str().c_str(), ASPECT);
        }

        prev_mem = mem;
      }
      
      {
        WriteGuard guard(srv_lock_); 
        word_info_manager_ = word_info_manager;
      }
      
      logger->info("WordManagerImpl::load_dicts: loading completed", ASPECT);
    }
  }
  
}
