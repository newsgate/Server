/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Dictionary/Commons/TransportImpl.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_SERVICES_DICTIONARY_COMMONS_TRANSPORTIMPL_IDL_
#define _NEWSGATE_SERVER_SERVICES_DICTIONARY_COMMONS_TRANSPORTIMPL_IDL_

#include <vector>
#include <iostream>
#include <list>

#include <El/Exception.hpp>
#include <El/Lang.hpp>
#include <El/Dictionary/Morphology.hpp>

#include <El/CORBA/Transport/EntityPack.hpp>

#include <Commons/Message/StoredMessage.hpp>

#include <Services/Dictionary/Commons/DictionaryServices.hpp>

namespace NewsGate
{
  namespace Dictionary
  {
    namespace Transport
    {
      //
      // MessageWordsPack implementation
      //

      struct MessageWords
      {
        El::Lang language;
        
        ::NewsGate::Message::MessageWordPosition word_positions;
        ::NewsGate::Message::WordPositionArray positions;

        MessageWords() throw(El::Exception);

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);        
      };

      typedef std::vector<MessageWords> MessageWordsArray;
      
      struct MessageWordsPackImpl
      {
        typedef El::Corba::Transport::EntityPack<
          MessageWordsPack,
          MessageWords,
          MessageWordsArray,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::EntityPack_init<MessageWordsPack,
                                                      MessageWordsArray,
                                                      Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };
      
      //
      // NormalizedWordsPackImpl implementation
      //

      struct NormalizedWordsPackImpl
      {
        typedef El::Corba::Transport::EntityPack<
          NormalizedWordsPack,
          ::El::Dictionary::Morphology::WordInfo,
          ::El::Dictionary::Morphology::WordInfoArray,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::EntityPack_init<
          NormalizedWordsPack,
          ::El::Dictionary::Morphology::WordInfoArray,
          Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };
      
      //
      // NormalizedMessageWordsPack implementation
      //

      struct WordInfo
      {
        El::Lang lang;

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);        
      };

      typedef std::vector<WordInfo> WordInfoArray;

      struct NormalizedMessageWords
      {
        El::Lang lang;

        ::NewsGate::Message::NormFormPosition norm_form_positions;
        ::NewsGate::Message::WordPositionArray resulted_positions;
        ::NewsGate::Dictionary::Transport::WordInfoArray word_infos;

        NormalizedMessageWords() throw(El::Exception);

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);        
      };

      typedef std::vector<NormalizedMessageWords> NormalizedMessageWordsArray;
      
      struct NormalizedMessageWordsPackImpl
      {
        typedef El::Corba::Transport::EntityPack<
          NormalizedMessageWordsPack,
          NormalizedMessageWords,
          NormalizedMessageWordsArray,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::EntityPack_init<
          NormalizedMessageWordsPack,
          NormalizedMessageWordsArray,
          Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };
      
      typedef std::vector<std::string> WordsArray;

      struct GetLemmasParamsStruct
      {
        El::Lang lang;
        WordsArray words;
        ::El::Dictionary::Morphology::Lemma::GuessStrategy guess_strategy;

        GetLemmasParamsStruct() throw(El::Exception);

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);        
      };
      
      struct GetLemmasParamsImpl
      {
        typedef El::Corba::Transport::Entity<
          NewsGate::Dictionary::Transport::GetLemmasParams,
          NewsGate::Dictionary::Transport::GetLemmasParamsStruct,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::Entity_init<
          NewsGate::Dictionary::Transport::GetLemmasParamsStruct,
          Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };
      
      struct LemmaPackImpl
      {
        typedef El::Corba::Transport::EntityPack<
          LemmaPack,
          El::Dictionary::Morphology::LemmaInfoArray,
          El::Dictionary::Morphology::LemmaInfoArrayArray,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::EntityPack_init<
          LemmaPack,
          El::Dictionary::Morphology::LemmaInfoArrayArray,
          Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };

      void register_valuetype_factories(CORBA::ORB* orb)
        throw(CORBA::Exception, El::Exception);
    }
  }
}


///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Dictionary
  {
    namespace Transport
    {
      //
      // GetLemmasParamsStruct struct
      //
      inline
      GetLemmasParamsStruct::GetLemmasParamsStruct() throw(El::Exception)
          : guess_strategy(::El::Dictionary::Morphology::Lemma::GS_NONE)
      {
      }
      
      inline
      void
      GetLemmasParamsStruct::write(El::BinaryOutStream& bstr) const
        throw(El::Exception)
      {
        bstr << lang << (uint32_t)guess_strategy;
        bstr.write_array(words);
      }
      
      inline
      void
      GetLemmasParamsStruct::read(El::BinaryInStream& bstr)
        throw(El::Exception)
      {
        uint32_t gs = 0;
        
        bstr >> lang >> gs;
        bstr.read_array(words);
        
        guess_strategy =
          (::El::Dictionary::Morphology::Lemma::GuessStrategy)gs;
      }
      
      //
      // WordInfo struct
      //
      inline
      void
      WordInfo::write(El::BinaryOutStream& bstr) const throw(El::Exception)
      {
        bstr << lang;
      }
      
      inline
      void
      WordInfo::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> lang;
      }

      //
      // MessageWords struct
      //
      
      inline
      MessageWords::MessageWords() throw(El::Exception)
          : word_positions(NewsGate::Message::StringConstPtr::null)
      {
      }
      
      inline
      void
      MessageWords::write(El::BinaryOutStream& bstr) const throw(El::Exception)
      {
        bstr << language << word_positions << positions;
      }
      
      inline
      void
      MessageWords::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> language >> word_positions >> positions;
      }

      //
      // NormalizedMessageWords struct
      //
      
      inline
      NormalizedMessageWords::NormalizedMessageWords() throw(El::Exception)
          : norm_form_positions(0)
      {
      }
      
      inline
      void
      NormalizedMessageWords::write(El::BinaryOutStream& bstr) const
        throw(El::Exception)
      {
        bstr << lang << norm_form_positions << resulted_positions;        
        bstr.write_array(word_infos);        
      }
      
      inline
      void
      NormalizedMessageWords::read(El::BinaryInStream& bstr)
        throw(El::Exception)
      {
        bstr >> lang >> norm_form_positions >> resulted_positions;
        bstr.read_array(word_infos);
      }
    }
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_DICTIONARY_COMMONS_TRANSPORTIMPL_IDL_
