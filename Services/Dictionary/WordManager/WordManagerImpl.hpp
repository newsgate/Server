/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Dictionary/WordManager/WordManagerImpl.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_DICTIONARY_WORDMANAGER_PULLERMANAGERIMPL_HPP_
#define _NEWSGATE_SERVER_SERVICES_DICTIONARY_WORDMANAGER_PULLERMANAGERIMPL_HPP_

#include <memory>

#include <ext/hash_map>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/RefCount/All.hpp>
#include <El/Dictionary/Morphology.hpp>

#include <El/Service/Service.hpp>
#include <El/Service/CompoundService.hpp>

#include <Services/Dictionary/Commons/DictionaryServices_s.hpp>

namespace NewsGate
{
  namespace Dictionary
  {
    class WordManagerImpl :
      public virtual POA_NewsGate::Dictionary::WordManager,
      public virtual El::Service::CompoundService<>
    {
    public:
      EL_EXCEPTION(Exception, El::ExceptionBase);
      EL_EXCEPTION(InvalidArgument, Exception);

    public:

      WordManagerImpl(El::Service::Callback* callback)
        throw(InvalidArgument, Exception, El::Exception);

      virtual ~WordManagerImpl() throw();

    protected:

      //
      // IDL:NewsGate/Dictionary/WordManager/normalize_words:1.0
      //
      virtual ::CORBA::ULong normalize_words (
        const ::NewsGate::Dictionary::WordManager::WordSeq& words,
        const char* language,
        ::NewsGate::Dictionary::Transport::NormalizedWordsPack_out result)
        throw(NewsGate::Dictionary::NotReady,
              NewsGate::Dictionary::ImplementationException,
              ::CORBA::SystemException);
      
      //
      // IDL:NewsGate/Dictionary/WordManager/normalize_message_words:1.0
      //
      virtual ::CORBA::ULong normalize_message_words(
        ::NewsGate::Dictionary::Transport::MessageWordsPack* words,
        ::NewsGate::Dictionary::Transport::NormalizedMessageWordsPack_out result)
        throw(NewsGate::Dictionary::NotReady,
              NewsGate::Dictionary::ImplementationException,
              ::CORBA::SystemException);

      //
      // IDL:NewsGate/Dictionary/WordManager/is_ready:1.0
      //
      virtual ::CORBA::Boolean is_ready()
        throw(NewsGate::Dictionary::ImplementationException,
              ::CORBA::SystemException);

      //
      // IDL:NewsGate/Dictionary/WordManager/normalize_search_expression:1.0
      //
      virtual ::CORBA::ULong normalize_search_expression(
        ::NewsGate::Search::Transport::Expression* expression,
        ::NewsGate::Search::Transport::Expression_out result)
        throw(NewsGate::Dictionary::NotReady,
              NewsGate::Dictionary::ImplementationException,
              ::CORBA::SystemException);

      //
      // IDL:NewsGate/Dictionary/WordManager/hash:1.0
      //
      virtual ::CORBA::ULong hash()
        throw(NewsGate::Dictionary::NotReady,
              NewsGate::Dictionary::ImplementationException,
              ::CORBA::SystemException);
      
      //
      // IDL:NewsGate/Dictionary/WordManager/get_lemmas:1.0
      //
      virtual void get_lemmas(
        ::NewsGate::Dictionary::Transport::GetLemmasParams* params,
        ::NewsGate::Dictionary::Transport::LemmaPack_out result)
        throw(NewsGate::Dictionary::NotReady,
              NewsGate::Dictionary::ImplementationException,
              ::CORBA::SystemException);

      virtual bool notify(El::Service::Event* event) throw(El::Exception);
      
    private:

      struct LoadDicts : public El::Service::CompoundServiceMessage
      {
        LoadDicts(WordManagerImpl* service) throw(El::Exception);
      };
      
      void load_dicts() throw(El::Exception);

    private:
      
      typedef std::auto_ptr<El::Dictionary::Morphology::WordInfoManager>
      WordInfoManagerPtr;
      
      WordInfoManagerPtr word_info_manager_;
    };

    typedef El::RefCount::SmartPtr<WordManagerImpl> WordManagerImpl_var;

  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Dictionary
  {
    //
    // WordManagerImpl::LoadDicts class
    //
    inline
    WordManagerImpl::LoadDicts::LoadDicts(WordManagerImpl* state)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }
    
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_DICTIONARY_WORDMANAGER_PULLERMANAGERIMPL_HPP_
