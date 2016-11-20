/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Commons/Message/StoredMessage.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <El/CORBA/Corba.hpp>

#include <stdint.h>
#include <string.h> 

#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <ext/hash_map>

#include <list>
#include <string>
#include <utility>
#include <vector>
#include <fstream>

#include <El/Exception.hpp>
#include <El/Hash/Hash.hpp>
#include <El/ArrayPtr.hpp>
#include <El/CRC.hpp>
#include <El/Net/HTTP/URL.hpp>
#include <El/BinaryStream.hpp>
#include <El/String/Manip.hpp>
#include <El/String/SharedString.hpp>
#include <El/String/Unicode.hpp>
#include <El/FileSystem.hpp>

#include <Commons/Message/StoredMessage.hpp>

namespace
{
  const float CAPITAL_WORD_FACTOR = 1.5;
  const float PROPER_NAME_WORD_FACTOR = 1.5;
  const float ALTERNATE_ONLY_WORD_FACTOR = 0.7;
  const float META_INFO_WORD_FACTOR = 0;

  const float TOKEN_FACTOR[16] =
  { 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 };
//  { 0, 1, 0.5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 };
  
  const float TITLE_TOKEN_FACTOR[16] =
  { 0, 3.5, 4.5, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3.5, 0 };
//  { 0, 3.5, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3.5, 0 };

  const size_t MESSAGES_IN_FEED_THRESHOLD = 500;
  const size_t CORE_WORD_MIN_NUMBER = 2;
  const float CORE_WORD_MIN_WEIGHT = 0.001;
  const ::NewsGate::Message::WordPosition CORE_POS_THRESHOLD = 100;

  const float SORT_BY_RELEVANCE_RCTR_WEIGHT = 4;
  const float SORT_BY_RELEVANCE_IMAGE_WEIGHT = 0.1;
  const float SORT_BY_RELEVANCE_CAPACITY_WEIGHT = 2;
  const uint32_t SORT_BY_RELEVANCE_MAX_EVENT_SIZE = 10000;
  const float SORT_BY_RELEVANCE_FEED_RCTR_WEIGHT = 0.4;
  
  const float SORT_BY_RELEVANCE_RCTR_FACTOR =
    10000.0F * SORT_BY_RELEVANCE_RCTR_WEIGHT;

  const float SORT_BY_RELEVANCE_FEED_RCTR_FACTOR =
    10000.0F * SORT_BY_RELEVANCE_FEED_RCTR_WEIGHT;

  const float SORT_BY_RELEVANCE_CAPACITY_FACTOR =
    10000.0F * SORT_BY_RELEVANCE_CAPACITY_WEIGHT /
    SORT_BY_RELEVANCE_MAX_EVENT_SIZE;

  const uint32_t word_pair_short_period = 86400 * 5;
  const uint32_t word_pair_long_period = 86400 * 9 * 30;

//  const uint32_t word_pair_short_period = 86400;
//  const uint32_t word_pair_long_period = 86400 * 2;
  const float MAX_WP_FACTOR = 1.5;
  const double WP_LOG_SHIFT = 0.5;
}

namespace NewsGate
{ 
  namespace Message
  {
    size_t
    feed_stop_word_threshold(size_t messages) throw()
    {
      return 91;
    }
/*
    //
    // Approximates points: (10, 100), (500, 81), (1000, 76), (10000, 67)
    // Online tool: http://www.soft4structures.com/Regr/JSPageRegr_1.jsp
    // Params: correlation coefficient 0.5, approximation err 0.01,
    //         coefficients 3
    //
    size_t
    feed_stop_word_threshold(size_t messages) throw()
    {
      if(messages <= 10)
      {
        return 100;
      }
      
      double X = pow(messages, -0.5);
      return (size_t)(62.494984 + 461.34 * X - 1083.835 * pow(X, 2));
    }    
*/  
/*  RECENT
    //
    // Approximates points: (10, 51), (350, 35), (1000, 23), (10000, 10)
    // Online tool: http://www.soft4structures.com/Regr/JSPageRegr_1.jsp
    // Params: correlation coefficient 0.5, approximation err 0.01,
    //         coefficients 3
    //
    size_t
    feed_stop_word_threshold(size_t messages) throw()
    {
      if(messages <= 10)
      {
        return 51;
      }
      
      double X = pow(messages, -0.5);
      return (size_t)(3.4611027 + 670.3337 * X - 1644.3922 * pow(X, 2));
    }
*/

/*  OLD
    //
    // Approximates points: (10, 51), (100, 35), (1000, 23), (10000, 10)
    // Online tool: http://www.soft4structures.com/Regr/JSPageRegr_1.jsp
    // Params: correlation coefficient 0.5, approximation err 0.01,
    //         coefficients 3
    //
    size_t
    feed_stop_word_threshold(size_t messages) throw()
    {
      if(messages <= 10)
      {
        return 51;
      }
      
      double X = log(messages);      
      return (size_t)((67.703705 - 6.023825 * X)/(1.0 + 0.02412747 * X) + 0.5);
    }
*/
    El::Stat::TimeMeter
    StoredMessage::break_down_meter("StoredMessage::break_down", false);

    El::Stat::TimeMeter
    SearcheableMessageMap::insert_meter("SearcheableMessageMap::insert",
                                        false);
    
    El::Stat::TimeMeter
    SearcheableMessageMap::remove_meter("SearcheableMessageMap::remove",
                                        false);

    El::Stat::Counter
    StoredMessage::object_counter("StoredMessage::object_counter", true);

    El::Stat::Counter
    NumberSet::object_counter("NumberSet::object_counter", true);

    uint32_t StoredMessage::zero_feed_search_weight = 0;
    
    //
    // FeedInfo struct
    //
    void
    FeedInfo::calc_search_weight(uint32_t impression_respected_level) throw()
    {
      uint64_t respected_impressions =
        std::max(impressions, (uint64_t)impression_respected_level);
      
      search_weight = respected_impressions ?
        (uint32_t)(SORT_BY_RELEVANCE_FEED_RCTR_FACTOR *
                   std::min(clicks, impressions) /
                   respected_impressions + 0.5F) : 0;
    }

    //
    // StoredMessage::WordComplementElem struct
    //
    inline
    StoredMessage::WordComplementElem::WordComplementElem(WordPosition pos,
                                                          uint8_t tp,
                                                          const char* txt)
      throw(El::Exception)
        : position(pos),
          type(tp),
          text(txt)
    {
    }
    
    inline
    void
    StoredMessage::WordComplementElem::set_text(const wchar_t* txt)
      throw(El::Exception)
    {
      std::string txt_s;
      El::String::Manip::wchar_to_utf8(txt, txt_s);
      text = txt_s.c_str();
    }    

    //
    // StoredMessage::WordComplementElemArray struct
    //
    
    inline
    void
    StoredMessage::WordComplementElemArray::add(WordComplementElem& wc)
      throw(El::Exception)
    {
      if(wc.type != WordComplement::TP_UNDEFINED)
      {
        push_back(wc);
        
        wc.type = WordComplement::TP_UNDEFINED;
        wc.text = "";
      }
    }

    inline
    void
    StoredMessage::MsWdPs::push_word(std::wstring& word,
                                     uint16_t flags,
                                     WordPosition& position,
                                     Signature* psignature,
                                     WordComplementElemArray& word_complements,
                                     bool& end_of_sentence)
      throw(El::Exception)
    {      
      StoredMessage::normalize_word(word);
      
      if(word[0] == L'\0')
      {
        return;
      }
      
      unsigned long cat_flags = El::String::Unicode::EC_QUOTE |
        El::String::Unicode::EC_BRACKET | El::String::Unicode::EC_STOP;
      
      long start = 0;
      bool has_stops = false;
      bool eos = false;
      unsigned long cat = 0;
      
      for(; (cat = El::String::Unicode::CharTable::el_categories(word[start]))
            & cat_flags; start++)
      {
        if(cat & El::String::Unicode::EC_STOP)
        {
          has_stops = true;
        }
        
        if(cat & El::String::Unicode::EC_END_OF_SENTENCE)
        {
          eos = true;
        }
      }

      if(has_stops)
      {
        position++;
        has_stops = false;
      }
      
      WordComplementElem word_complement(position);

      if(start)
      {
        word_complement.type = WordComplement::TP_STANDALONE;
        word_complement.set_text(word.substr(0, start).c_str());
        
        word = word.substr(start);
        
        if(word.empty())
        {
          word_complements.add(word_complement);
          
          if(eos)
          {
            end_of_sentence = true;
          }
          
          return;
        }

        word_complement.type = WordComplement::TP_PREFIX;
      }

      long last = word.length() - 1;
      eos = false;
      
      long suffix = 0;
      for(suffix = last; suffix >= 0 &&
            ((cat = El::String::Unicode::CharTable::el_categories(
                word[suffix])) & cat_flags); suffix--)
      {
        if(cat & El::String::Unicode::EC_STOP)
        {
          has_stops = true;
        }

        if(cat & El::String::Unicode::EC_END_OF_SENTENCE)
        {
          eos = true;
        }        
      }

      if(suffix++ != last)
      {
        if(suffix == 0)
        {
          std::string word_s;
          El::String::Manip::wchar_to_utf8(word.c_str(), word_s);
          
          word_complement.text =
            std::string(word_complement.text.c_str()) + word_s;

          word_complement.type = WordComplement::TP_STANDALONE;
          word_complements.add(word_complement);

          word.clear();
          
          if(has_stops)
          {
            position++;
          }

          if(eos)
          {
            end_of_sentence = true;
          }

          return;
        }

        if(suffix <= last)
        {
          word_complements.add(word_complement);
          
          word_complement.type = WordComplement::TP_SUFFIX;
          word_complement.set_text(word.substr(suffix).c_str());
          word = word.substr(0, suffix);
        }
      }

      std::wstring lower_word;
      El::String::Manip::to_uniform(word.c_str(), lower_word);

      if(word[0] != lower_word[0])
      {
        flags |= WordPositions::FL_CAPITAL_WORD;

        if(!end_of_sentence)
        {
          flags |= WordPositions::FL_SENTENCE_PROPER_NAME;
        }
      }

      end_of_sentence = eos;

      if(lower_word != word)
      {
        std::string word_s;
        El::String::Manip::wchar_to_utf8(word.c_str(), word_s);

        switch(word_complement.type)
        {
        case WordComplement::TP_SUFFIX:
          {
            WordComplementElem replacement(word_complement.position,
                                           WordComplement::TP_REPLACEMENT,
                                           word_s.c_str());
            
            word_complements.add(replacement);            
            word_complements.add(word_complement);
            
            break;
          }
        default:
          {
            word_complements.add(word_complement);

            word_complement.text = word_s;
            word_complement.type = WordComplement::TP_REPLACEMENT;
            word_complements.add(word_complement);
            
            break;
          }
        }
      }

      word_complements.add(word_complement);
      
      std::string word_s;
      El::String::Manip::wchar_to_utf8(lower_word.c_str(), word_s);

      if(psignature)
      {
        El::CRC(*psignature,
                (const unsigned char*)word_s.c_str(),
                word_s.length());
        
        El::CRC(*psignature, (const unsigned char*)" ", 1);
      }
      
      MsWdPs::iterator it = find(word_s);
      bool new_item = it == end();

      if(new_item)
      {
        it = insert(std::make_pair(word_s, MsPs())).first;
      }

      MsPs& msg_pos = it->second;
        
      msg_pos.push_back(position++);

      bool reset_caspital_flag = !new_item &&
        ((flags & WordPositions::FL_CAPITAL_WORD) == 0 ||
         (msg_pos.flags & WordPositions::FL_CAPITAL_WORD) == 0);
         
      msg_pos.flags |= flags;

      if(reset_caspital_flag)
      {
        // If at least one occurance of the word is not in upper case,
        // then word considered not capitalized
        msg_pos.flags &= ~WordPositions::FL_CAPITAL_WORD;
      }

      if((msg_pos.flags & WordPositions::FL_CAPITAL_WORD) == 0)
      {
        msg_pos.flags &= ~WordPositions::FL_SENTENCE_PROPER_NAME;
      }
      
      word.clear();

      if(has_stops)
      {
        position++;
      }
    }
    
    //
    // StoredMessage class
    //

    uint64_t
    StoredMessage::word_hash(bool core_word_flag) const throw()
    {
      uint64_t crc = 0;
      El::CRC(crc, (unsigned char*)&lang, sizeof(lang));

      {  
        NormFormPosition::Size size = norm_form_positions.size();      
        El::CRC(crc, (unsigned char*)&size, sizeof(size));

        for(NormFormPosition::Size i = 0; i < size; ++i)
        {
          const NormFormPosition::KeyValue& kw = norm_form_positions[i];
          El::CRC(crc, (unsigned char*)&kw.first, sizeof(kw.first));

          const WordPositions& nf_pos = kw.second;
          WordPositionNumber count = nf_pos.position_count();
        
          El::CRC(crc, (unsigned char*)&count, sizeof(count));

          uint16_t flags = core_word_flag ? nf_pos.flags :
            (nf_pos.flags & ~WordPositions::FL_CORE_WORD);

          // After 1.6.89.0 running long enough after deployment to
          // prod colos, can remove that line. Added fo now
          // to avoid bulk message change after flag invention.
          flags &= ~(WordPositions::FL_PROPER_NAME |
                     WordPositions::FL_SENTENCE_PROPER_NAME);
          
          El::CRC(crc, (unsigned char*)&flags, sizeof(flags));
          El::CRC(crc, (unsigned char*)&nf_pos.lang, sizeof(nf_pos.lang));

          for(WordPositionNumber j = 0; j < count; ++j)
          {
            WordPosition pos = nf_pos.position(positions, j);
            El::CRC(crc, (unsigned char*)&pos, sizeof(pos));
          }
        }   
      }
      
      {  
        MessageWordPosition::Size size = word_positions.size();      
        El::CRC(crc, (unsigned char*)&size, sizeof(size));

        for(MessageWordPosition::Size i = 0; i < size; ++i)
        {
          const MessageWordPosition::KeyValue& kw = word_positions[i];
          El::CRC(crc, (unsigned char*)kw.first.c_str(), kw.first.length());

          const WordPositions& w_pos = kw.second;
          WordPositionNumber count = w_pos.position_count();
        
          El::CRC(crc, (unsigned char*)&count, sizeof(count));

          uint16_t flags = core_word_flag ? w_pos.flags :
            (w_pos.flags & ~WordPositions::FL_CORE_WORD);
          
          El::CRC(crc, (unsigned char*)&flags, sizeof(flags));
          El::CRC(crc, (unsigned char*)&w_pos.lang, sizeof(w_pos.lang));

          for(WordPositionNumber j = 0; j < count; ++j)
          {
            WordPosition pos = w_pos.position(positions, j);
            El::CRC(crc, (unsigned char*)&pos, sizeof(pos));
          }
        }
      }
      
      return crc;
    }
    
    void
    StoredMessage::clear() throw()
    {
      try
      {
        id = Id::zero;
        event_id = El::Luid::null;
        description_pos = 0;
        img_alt_pos = 0;
        keywords_pos = 0;
        event_capacity = 0;
        flags = 0;
        impressions = 0;
        clicks = 0;
        published = 0;
        fetched = 0;
        visited = 0;
        signature = 0;
        url_signature = 0;
        content = 0;
        country = El::Country::null;
        lang = El::Lang::null;
        source_title.clear();
        
        word_positions.clear();
        norm_form_positions.clear();
        positions.resize(0);
        core_words.clear();

        categories.clear();
        category_paths.clear();

        search_weight = 0;
        feed_search_weight = &zero_feed_search_weight;

        set_source_url(0);
      }      
      catch(const El::Exception& e)
      {
        std::cerr << "StoredMessage::clear: exception caught. Desc: " << e
                  << std::endl;
      }
    }
    
    struct WordPositionsArray : public std::vector<WordPositionNumber>
    {
      uint16_t flags;
      El::Lang lang;
      bool is_stop_word;

      WordPositionsArray(bool is_stop_word_val) throw();
    };

    WordPositionsArray::WordPositionsArray(bool is_stop_word_val) throw()
        : flags(0),
          is_stop_word(is_stop_word_val)
    {
    }

    //
    // StoredMessage class
    //
    
    const wchar_t StoredMessage::FUNKY_QUOTES[] =
      L"`\x91\x92\x2018\x2019"
      "\x93\x94\x00AB\x00BB\x201C\x201D\x201E\x301D\x301E\x301F";

    const wchar_t StoredMessage::REPLACEMENT_QUOTES[] =
      L"'''''\"\"\"\"\"\"\"\"\"\"";
    
    void
    StoredMessage::normalize_word(std::wstring& word) throw(El::Exception)
    {
      El::String::Manip::replace(word, FUNKY_QUOTES, REPLACEMENT_QUOTES);
    }
     
    void
    StoredMessage::set_normal_forms(
      const El::Dictionary::Morphology::WordInfoArray& word_infos,
      const MessageWordPosition& word_positions,
      const WordPositionArray& positions,
      NormFormPosition& norm_form_positions,
      WordPositionArray& resulted_positions)
      throw(El::Exception)
    {
      norm_form_positions.clear();
      resulted_positions.clear();

      WordPositionNumber norm_form_offset = 0;

      for(unsigned long i = 0; i < word_positions.size(); i++)
      {
        const WordPositions& wpos = word_positions[i].second;

        WordPositionNumber offset = 0;
        if(wpos.position_offset(offset))
        {
          norm_form_offset =
            std::max(norm_form_offset,
                     (WordPositionNumber)(offset + wpos.position_count()));
        }
      }

      typedef __gnu_cxx::hash_map<
        El::Dictionary::Morphology::WordId,
        WordPositionsArray,
        El::Hash::Numeric<El::Dictionary::Morphology::WordId> >
      NormFormPosMap;

      NormFormPosMap norm_form_pos_map;
      size_t norm_form_pos_count = 0;

      for(size_t i = 0; i < word_positions.size(); i++)
      {
        const WordPositions& wpos = word_positions[i].second;
        unsigned long wpos_count = wpos.position_count();

        uint16_t flags = wpos.flags &
          (WordPositions::FL_LOCATION_MASK | WordPositions::FL_CAPITAL_WORD |
           WordPositions::FL_SENTENCE_PROPER_NAME |
           WordPositions::FL_PROPER_NAME);

        const El::Dictionary::Morphology::WordFormArray& wf =
          word_infos[i].forms;

        for(El::Dictionary::Morphology::WordFormArray::const_iterator it =
              wf.begin(); it != wf.end(); it++)
        {
          El::Dictionary::Morphology::WordId norm_form_id = it->id;
          const El::Lang& norm_form_lang = it->lang;
          bool is_stop_word = it->is_stop_word;

          for(size_t k = 0; k < wpos_count; k++)
          {
            NormFormPosMap::iterator nit =
              norm_form_pos_map.find(norm_form_id);
            
            bool new_item = nit == norm_form_pos_map.end();

            if(new_item)
            {
              nit = norm_form_pos_map.insert(
                std::make_pair(norm_form_id,
                               WordPositionsArray(is_stop_word))).first;
            }
            
            WordPositionsArray& wpa = nit->second;
            wpa.lang = norm_form_lang;
              
            wpa.push_back(wpos.position(positions, k));

            bool reset_caspital_flag = !new_item &&
              ((flags & WordPositions::FL_CAPITAL_WORD) == 0 ||
               (wpa.flags & WordPositions::FL_CAPITAL_WORD) == 0);
            
            wpa.flags |= flags;

            if(reset_caspital_flag)
            {
              // If at least one occurance of the word is not in upper case,
              // then word considered not capitalized
              wpa.flags &= ~(WordPositions::FL_CAPITAL_WORD |
                             WordPositions::FL_PROPER_NAME |
                             WordPositions::FL_SENTENCE_PROPER_NAME);
            }
          }

          norm_form_pos_count += wpos_count;
        }
      }

      norm_form_positions.resize(norm_form_pos_map.size());
      
      resulted_positions.resize(norm_form_offset + norm_form_pos_count);
      resulted_positions.copy(positions, 0, norm_form_offset);

      for(NormFormPosMap::const_iterator it = norm_form_pos_map.begin();
          it != norm_form_pos_map.end(); it++)
      {
        WordPositions& wp = norm_form_positions.insert(it->first,
                                                       WordPositions()).second;

        const WordPositionsArray& pos_array = it->second;

        wp.lang = pos_array.lang;
        wp.flags = pos_array.flags;

        wp.token_type(pos_array.is_stop_word ?
                      WordPositions::TT_STOP_WORD :
                      WordPositions::TT_KNOWN_WORD);
        
        wp.set_positions(norm_form_offset, pos_array.size());

        for(size_t i = 0; i < pos_array.size(); i++)
        {
          wp.insert_position(resulted_positions,
                             norm_form_offset,
                             pos_array[i]);
        }
      }

      resulted_positions.resize(norm_form_offset);
    }

    typedef std::vector<const char*> PositionedWordArray;

    bool
    StoredMessage::assemble(MessageBuilder& builder,
                            WordPosition min_pos,
                            WordPosition max_pos) const
      throw(Exception, El::Exception)
    {
      long positions_count = -1;
      bool interrupted = false;
      
      for(size_t i = 0; i < word_positions.size(); i++)
      {
        const WordPositions& word_pos = word_positions[i].second;
        size_t word_pos_count = word_pos.position_count();

        for(size_t j = 0; j < word_pos_count; j++)
        {
          WordPosition pos = word_pos.position(positions, j);

          if(pos < min_pos || pos >= max_pos)
          {
            continue;
          }

          pos -= min_pos;
          positions_count = std::max(positions_count, (long)pos);
        }
      }

      positions_count++;

      PositionedWordArray words(positions_count);

      for(PositionedWordArray::iterator it = words.begin();
          it != words.end(); it++)
      {
        *it = 0;
      }
      
      for(size_t i = 0; i < word_positions.size(); i++)
      {
        const WordPositions& word_pos = word_positions[i].second;
        size_t word_pos_count = word_pos.position_count();
        
        const char* word = word_positions[i].first.c_str();

        for(size_t j = 0; j < word_pos_count; j++)
        {
          WordPosition pos = word_pos.position(positions, j);

          if(pos < min_pos || pos >= max_pos)
          {
            continue;
          }

          pos -= min_pos;
          words[pos] = word;
        }          
      }

      const WordComplementLighArray& word_complements =
        content->word_complements;

      size_t j = 0;

      for(; j < word_complements.size() &&
            word_complements[j].position < min_pos; j++);
      
      bool write_space = false;

      WordPosition real_pos = min_pos;

      for(WordPosition i = 0; i < words.size(); i++, real_pos++)
      {
        if(interrupted)
        {
          return false;
        }
        
        bool replaced = false;
        bool suffix = false;

        for(; j < word_complements.size() &&
              word_complements[j].position <= real_pos; j++)
        {
          if(interrupted)
          {
            return false;
          }
          
          const Message::WordComplement& wc = word_complements[j];
          
          switch(wc.type)
          {
          case Message::WordComplement::TP_SEGMENTATION:
            {
              if(!builder.segmentation())
              {
                interrupted = true;
                continue;
              }
              
              write_space = false;
              break;
            }
          case Message::WordComplement::TP_STANDALONE:
            {
              if(write_space)
              {
                if(!builder.interword(" "))
                {
                  interrupted = true;
                  continue;
                }
              }
              
              if(!builder.interword(wc.text.c_str()))
              {
                interrupted = true;
                continue;                
              }
              
              write_space = true;
              break;
            }
          case Message::WordComplement::TP_REPLACEMENT:
            {
              if((write_space && !builder.interword(" ")) ||
                 !builder.word(wc.text.c_str(), wc.position))
              {
                interrupted = true;
                continue;
              }
              
              write_space = true;              
              replaced = true;
              
              break;
            }
          case Message::WordComplement::TP_PREFIX:
            {
              if((write_space && !builder.interword(" ")) ||
                 !builder.interword(wc.text.c_str()))
              {
                interrupted = true;
                continue;
              }
              
              write_space = false;
              break;
            }
          case Message::WordComplement::TP_SUFFIX:
            {
              suffix = true;
              break;
            }
          }

          if(suffix)
          {
            break;
          }
        }

        if(interrupted)
        {
          continue;
        }
        
        if(!replaced && words[i])
        {
          if((write_space && !builder.interword(" ")) ||
             !builder.word(words[i], min_pos + i))
          {
            interrupted = true;
            continue;
          }
          
          write_space = true;
        }

        if(suffix)
        {
          if(!builder.interword(word_complements[j++].text.c_str()))
          {
            interrupted = true;
            continue;           
          }
          
          write_space = true;
        }
      }

      for(; j < word_complements.size() &&
            word_complements[j].position < max_pos; j++)
      {
        if(interrupted)
        {
          return false;
        }

        const Message::WordComplement& wc = word_complements[j];
          
        switch(wc.type)
        {
        case Message::WordComplement::TP_SEGMENTATION:
          {
            if(!builder.segmentation())
            {
              interrupted = true;
              continue;
            }
            
            write_space = false;
            break;
          }
        case Message::WordComplement::TP_STANDALONE:
          {
            if((write_space && !builder.interword(" ")) ||
               !builder.interword(wc.text.c_str()))
            {
              interrupted = true;
              continue;            
            }
            
            write_space = true;
            break;
          }
        case Message::WordComplement::TP_SUFFIX:
          {
            if(!builder.interword(wc.text.c_str()))
            {
              interrupted = true;
              continue;
            }
            
            break;
          }
        }
      }

      return true;
    }

    void
    StoredMessage::set_source_url(const char* val) throw(El::Exception)
    {
      source_url.clear();
      hostname.clear();
      hostname_len = 0;

      if(val && *val != '\0')
      {
        source_url = val;

        typedef std::list<std::string> DomainList;
        DomainList domain_list;

        El::Net::HTTP::URL url(source_url.c_str());

        hostname = url.host();
        hostname_len = hostname.length();

        if(El::Net::ip(hostname.c_str()))
        {
          hostname_len = 0;
        }
        
      }
    }
    
    void
    StoredMessage::break_down(const char* title,
                              const char* description,
                              const RawImageArray* images,
                              const char* keywords,
                              const SegmentationInfo* segmentation_info)
      throw(Exception, El::Exception)
    {
      El::Stat::TimeMeasurement measurement(break_down_meter);

      if(content.in() == 0)
      {
        throw Exception("NewsGate::Message::StoredMessage::break_down: "
                        "no content");
      }

      word_positions.clear();
      positions.resize(0);

      MsWdPs word_pos;
      WordPosition position = 0;
      WordComplementElemArray word_complements;

      word_complements.reserve(1000);

      signature = 0;

//      std::cerr << id.string() << std::endl;
      
      parse_text(title,
                 WordPositions::FL_TITLE,
                 position,
                 word_pos,
                 word_complements,
                 &signature,
                 segmentation_info ? &segmentation_info->title : 0);

//      std::cerr << std::endl;

      // position is incremented to make quoted sentence not match on
      // word sequences on title/description boundry

      description_pos = ++position;

      parse_text(description,
                 WordPositions::FL_DESC,
                 position,
                 word_pos,
                 word_complements,
                 &signature,
                 segmentation_info ? &segmentation_info->description : 0);

      img_alt_pos = ++position;

      if(signature && word_pos.size() > 1)
      {
        El::CRC(signature, (const unsigned char*)"A", 1);
      }
      else
      {
        signature = 0;

        const El::String::LightString& url = content->url;
        
        if(!url.empty())
        {
          El::CRC(signature, (const unsigned char*)url.c_str(), url.length());
          El::CRC(signature, (const unsigned char*)"L", 1);
        }
      }

      content->images.reset(0);
      flags &= ~(MF_HAS_THUMBS | MF_HAS_IMAGES);
      
      if(images && images->size())
      {
        StoredImageArrayPtr stored_images(new StoredImageArray(
                                            images->size()));

        for(size_t i = 0; i < images->size(); i++)
        {
          const RawImage& raw_img = (*images)[i];
          StoredImage& stored_img = (*stored_images)[i];
          
          ((Image&)stored_img) = (const Image&)raw_img;
          stored_img.alt_base = position;
          
          parse_text(raw_img.alt.c_str(),
                     WordPositions::FL_ALT,
                     position,
                     word_pos,
                     word_complements,
                     0,
                     segmentation_info ? &segmentation_info->images[i] : 0);

          ++position;

          stored_img.thumbs = raw_img.thumbs;
          
          if(!stored_img.thumbs.empty())
          {
            flags |= MF_HAS_THUMBS;
          }   
        }

        content->images.reset(stored_images.release());
        flags |= MF_HAS_IMAGES;
      }

      keywords_pos = position;
      
      parse_text(keywords,
                 0,
                 position,
                 word_pos,
                 word_complements,
                 &signature,
                 0);
      
//                 segmentation_info ? &segmentation_info->keywords : 0);      

      content->word_complements.resize(word_complements.size());

      unsigned long complements_len = 1;      
      for(size_t i = 0; i < word_complements.size(); i++)
      {
        complements_len += word_complements[i].text.length() + 1;
      }

      for(size_t i = 0; i < word_complements.size(); i++)
      {
        const WordComplementElem& elem = word_complements[i];

        WordComplement& complement = content->word_complements[i];

        complement.type = elem.type;
        complement.position = elem.position;
        complement.text = elem.text.c_str();
      }
      
      WordPositionNumber offset = 0;      
      positions.resize(position);
      word_positions.resize(word_pos.size());

      for(MsWdPs::iterator it = word_pos.begin(); it != word_pos.end(); it++)
      {
        const char* word = it->first.c_str();
        
        WordPositions& wpos =
          word_positions.insert(word, WordPositions()).second;
        
        MsPs& word_pos = it->second;

//        wpos.flags |= word_pos.flags;
        wpos.flags = word_pos.flags;
        wpos.set_positions(offset, word_pos.size());
        
        for(MsPs::const_iterator pit = word_pos.begin(); pit != word_pos.end();
            pit++)
        {
          wpos.insert_position(positions, offset, *pit);
        }              
      }

      positions.resize(offset);

/*
      for(const char* ptr = strings; *ptr != '\0'; ptr += strlen(ptr) + 1)
      {
        std::cerr << ptr << std::endl;
      }
      
      std::cerr << std::endl;
*/
    }

    std::string
    StoredMessage::image_thumb_name(const char* dir_path,
                                    size_t img_index,
                                    size_t thumb_index,
                                    bool create_dir)
      const throw(El::Exception)
    {
      std::string date_dir = std::string(dir_path) + "/" +
        El::Moment(ACE_Time_Value(published)).dense_format(El::Moment::DF_DATE);

      if(create_dir)
      {
        mkdir(date_dir.c_str(), 0755);
      }
      
      std::ostringstream ostr;
      ostr << date_dir << "/" << id.string() << "-" << img_index << "-"
           << thumb_index;
      
      return ostr.str();
    }

    std::string
    StoredMessage::image_thumbs_name(const char* dir_path,
                                     bool create_dir)
      const throw(El::Exception)
    {
      std::string date_dir = std::string(dir_path) + "/" +
        El::Moment(ACE_Time_Value(published)).dense_format(El::Moment::DF_DATE);

      if(create_dir)
      {
        mkdir(date_dir.c_str(), 0755);
      }
      
      std::ostringstream ostr;
      ostr << date_dir << "/" << id.string();
      
      return ostr.str();
    }

    void
    StoredMessage::write_image_thumbs(const char* dir_path,
                                      bool drop) const
      throw(Exception, El::Exception)
    {
      if(content.in() == 0)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::StoredMessage::write_image_thumbs: content is "
          "null for " << id.string();

        throw Exception(ostr.str());
      }

      if(content->images.get() == 0)
      {
        return;
      }

      StoredImageArray& images = *content->images.get();

      if(images.empty())
      {
        return;
      }
      
      std::string path = image_thumbs_name(dir_path, true);
 
      std::fstream file(path.c_str(), ios::out);
        
      if(!file.is_open())
      {
        std::ostringstream ostr;
        ostr << "NewsGate::StoredMessage::write_image_thumbs: "
          "failed to open '" << path << "' for write access";
        
        throw Exception(ostr.str());
      }
     
      try
      {
        El::BinaryOutStream bstr(file);

        uint64_t offset = 0;
        
        for(StoredImageArray::const_iterator i(images.begin()),
              e(images.end()); i != e; ++i)
        {
          const ImageThumbArray& thumbs = i->thumbs;
          
          for(ImageThumbArray::const_iterator i(thumbs.begin()),
                e(thumbs.end()); i != e; ++i)
          {
            offset += sizeof(uint64_t);
          }
        }
        
        for(StoredImageArray::const_iterator i(images.begin()),
              e(images.end()); i != e; ++i)
        {
          const ImageThumbArray& thumbs = i->thumbs;
          
          for(ImageThumbArray::const_iterator i(thumbs.begin()),
                e(thumbs.end()); i != e; ++i)
          {
            bstr << offset;
            offset += sizeof(uint64_t) + i->length;
          }
        }
        
        for(StoredImageArray::const_iterator i(images.begin()),
              e(images.end()); i != e; ++i)
        {
          const ImageThumbArray& thumbs = i->thumbs;
          
          for(ImageThumbArray::const_iterator i(thumbs.begin()),
                e(thumbs.end()); i != e; ++i)
          {
            i->write_image(bstr);
          }
        }      
        
        if(drop)
        {
          for(StoredImageArray::iterator i(images.begin()),
                e(images.end()); i != e; ++i)
          {
            ImageThumbArray& thumbs = i->thumbs;
            
            for(ImageThumbArray::iterator i(thumbs.begin()),
                  e(thumbs.end()); i != e; ++i)
            {
              i->drop_image();
            }
          }      
        }
      }
      catch(...)
      {
        file.close();
        unlink(path.c_str());
      }
    }
    
    void
    StoredMessage::read_image_thumbs(const char* dir_path,
                                     int32_t img_index,
                                     int32_t thumb_index,
                                     size_t* loaded_thumbs)
      throw(Exception, El::Exception)
    {
      if(content.in() == 0)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::StoredMessage::read_image_thumbs: content is "
          "null for " << id.string();

        throw Exception(ostr.str());
      }

      if(content->images.get() == 0)
      {
        return;
      }

      StoredImageArray& images = *content->images.get();

      int32_t m = 0;
      bool read_required = false;
      
      for(StoredImageArray::const_iterator i(images.begin()),
            e(images.end()); i != e && !read_required; ++i, ++m)
      {
        if(img_index >= 0 && m != img_index)
        {
          continue;
        }
        
        const ImageThumbArray& thumbs = i->thumbs;

        int32_t t = 0;
        
        for(ImageThumbArray::const_iterator i(thumbs.begin()),
              e(thumbs.end()); i != e && !read_required; ++i, ++t)
        {
          if(thumb_index >= 0 && t != thumb_index)
          {
            continue;
          }
          
          if(i->empty())
          {
            read_required = true;
            break;
          }
        }
      }
      
      if(!read_required)
      {
        return;
      }
      
      std::string path = image_thumbs_name(dir_path, false);
 
      std::fstream file(path.c_str(), ios::in);
        
      if(!file.is_open())
      {
        std::ostringstream ostr;
        ostr << "NewsGate::StoredMessage::read_image_thumbs: "
          "failed to open '" << path << "' for read access";
        
        throw Exception(ostr.str());
      }
     
      El::BinaryInStream bstr(file);

      typedef std::vector<uint64_t> OffsetVector;
      typedef std::vector<OffsetVector> Offsets;

      uint64_t offset = 0;
      Offsets offsets;

      offsets.reserve(images.size());
        
      for(StoredImageArray::const_iterator i(images.begin()),
            e(images.end()); i != e; ++i)
      {
        const ImageThumbArray& thumbs = i->thumbs;

        offsets.push_back(OffsetVector());
        OffsetVector& offset_vector = *offsets.rbegin();
        
        offset_vector.reserve(thumbs.size());
          
        for(ImageThumbArray::const_iterator i(thumbs.begin()),
              e(thumbs.end()); i != e; ++i)
        {
          bstr >> offset;
          offset_vector.push_back(offset);
        }
      }

      m = 0;
          
      for(StoredImageArray::iterator i(images.begin()),
            e(images.end()); i != e; ++i, ++m)
      {
        if(img_index >= 0 && m != img_index)
        {
          continue;
        }
          
        ImageThumbArray& thumbs = i->thumbs;
          
        int32_t t = 0;
          
        for(ImageThumbArray::iterator i(thumbs.begin()),
              e(thumbs.end()); i != e; ++i, ++t)
        {
          if(thumb_index >= 0 && t != thumb_index)
          {
            continue;
          }

          bstr.seek(offsets[m][t], std::ios::beg);
          i->read_image(bstr, false);

          if(loaded_thumbs)
          {
            ++(*loaded_thumbs);
          }
        }
      }
    }
    
    void
    StoredMessage::parse_text(const char* text,
                              uint16_t flags,
                              WordPosition& position,
                              MsWdPs& word_pos,
                              WordComplementElemArray& word_complements,
                              Signature* psignature,
                              const SegMarkerPositionSet* seg_markers)
      throw(El::Exception)
    {
      std::wstring wtext;
      El::String::Manip::utf8_to_wchar(text, wtext);
      
      std::wstring word;
      word.reserve(100);
      word = L"";

//      MsWdPs::QuoteStack quotes;
      const wchar_t* text_str = wtext.c_str();

/*      
      if(flags & WordPositions::FL_TITLE)
      {
        std::cerr << text << std::endl;

        if(seg_markers)
        {
          std::cerr << "SEG:";
          
          for(SegMarkerPositionSet::const_iterator it =
                seg_markers->begin(); it != seg_markers->end(); it++)
          {
            std::cerr << " " << *it;
          }
        }
      }
*/
      bool end_of_sentence = true;
      
      for(const wchar_t* ptr = text_str; *ptr != L'\0'; ptr++)
      {  
        if(El::String::Unicode::CharTable::is_space(*ptr)) 
        {
          word_pos.push_word(word,
                             flags,
                             position,
                             psignature,
                             word_complements,
                             end_of_sentence);
          
          if(seg_markers &&
             seg_markers->find(ptr - text_str) != seg_markers->end())
          {
            if(!word_complements.empty())
            {
              const WordComplementElem& last = *word_complements.rbegin();

              if(last.position == position &&
                 last.type == WordComplement::TP_SEGMENTATION)
              {
                // Segmentation marker for this position already set
                continue;
              }
            }

            if(ptr == text_str || ptr[1] == L'\0' ||
               El::String::Unicode::CharTable::is_space(*(ptr - 1)) ||
               El::String::Unicode::CharTable::is_space(*(ptr + 1)))
            {
              // No need in segmentation marker at the beginning, end or
              // next to whitespace character
              continue;
            }
               
            WordComplementElem wc(position,
                                  WordComplement::TP_SEGMENTATION,
                                  0);
            
            word_complements.add(wc);
          }
          
          continue;
        }

        word.append(ptr, 1);
      }
      
      word_pos.push_word(word,
                         flags,
                         position,
                         psignature,
                         word_complements,
                         end_of_sentence);
    }
    
    void
    StoredMessage::write_broken_down(std::ostream& ostr) const
      throw(Exception, El::Exception)
    {
      El::BinaryOutStream bin_str(ostr);
      write_broken_down(bin_str);
    }
    
    void
    StoredMessage::write_broken_down(El::BinaryOutStream& bin_str) const
      throw(Exception, El::Exception)
    {
      // Version
      bin_str << (uint16_t)5; // 3 - 1.4.43.0, 4 - 1.5.26.0, 5 - 1.5.95.0

      bin_str.write_string(source_url.c_str());
      
      bin_str << description_pos << img_alt_pos << keywords_pos
              << (uint16_t)word_positions.size();
      
      for(unsigned long i = 0; i < word_positions.size(); i++)
      {
        const MessageWordPosition::KeyValue& key_val = word_positions[i];

        bin_str.write_string(key_val.first.c_str());
        key_val.second.write(bin_str);        
      }

      bin_str << (uint16_t)norm_form_positions.size();
      
      for(unsigned long i = 0; i < norm_form_positions.size(); i++)
      {
        const NormFormPosition::KeyValue& key_val = norm_form_positions[i];

        bin_str << (uint32_t)key_val.first;
        key_val.second.write(bin_str);        
      }

      bin_str << (uint16_t)positions.size();
      
      for(unsigned long i = 0; i < positions.size(); i++)
      {
        bin_str << positions[i];
      }

      core_words.write(bin_str);
    }

    uint16_t
    StoredMessage::read_broken_down(std::istream& istr)
      throw(Exception, El::Exception)
    {
      El::BinaryInStream bin_str(istr);
      return read_broken_down(bin_str);
    }
    
    uint16_t
    StoredMessage::read_broken_down(El::BinaryInStream& bin_str)
      throw(Exception, El::Exception)
    {
      uint16_t version;
      bin_str >> version;

      El::ArrayPtr<char> src_url;
      bin_str.read_string(src_url.out());

      set_source_url(src_url.get());

      if(version < 5)
      {
        description_pos = 0;
        img_alt_pos = 0;
        keywords_pos = 0;
        assert(false);
      }
      else
      {
        bin_str >> description_pos >> img_alt_pos >> keywords_pos;
      }
      
      uint16_t size = 0;
      
      bin_str >> size;
      word_positions.resize(size);

      for(unsigned long i = 0; i < size; i++)
      {
        StringConstPtr word;
        word.read(bin_str);

        MessageWordPosition::KeyValue& key_value =
          word_positions.insert(i, word, WordPositions());

        if(version < 2)
        {
          assert(false);
          key_value.second.read_1(bin_str);
        }
        else if(version < 3)
        {
          assert(false);
          key_value.second.read_2(bin_str);
        }
        else
        {
          key_value.second.read(bin_str);
        }
      }

      bin_str >> size;
      norm_form_positions.resize(size);

      for(size_t i = 0; i < size; i++)
      {
        uint32_t word_id;
        bin_str >> word_id;

        NormFormPosition::KeyValue& key_value =
          norm_form_positions.insert(i, word_id, WordPositions());
        
        if(version < 2)
        {
          assert(false);
          key_value.second.read_1(bin_str);
        }
        else if(version < 3)
        {
          assert(false);
          key_value.second.read_2(bin_str);
        }
        else
        {
          key_value.second.read(bin_str);
        }
      }

      bin_str >> size;
      positions.resize(size);
      
      for(unsigned long i = 0; i < size; i++)
      {
        bin_str >> positions[i];
      }

      core_words.read(bin_str);

/*
      std::cerr << "Read message '"
                << (content.in() == 0 ? "?" : content->title.c_str())
                << "'. broken down bytes " << bin_str.read_bytes()
                << std::endl;
*/
      return version;
    }

    class ExpiredDirList : public El::FileSystem::DirectoryReader
    {
    public:
      EL_EXCEPTION(Exception, El::ExceptionBase);
      
    public:
      ExpiredDirList(const char* dir_path, unsigned long max_age)
        throw(Exception, El::Exception);

    private:
      virtual bool select(const struct dirent* dir) throw(El::Exception);

    private:
      unsigned long threshold_;
    };
    
    ExpiredDirList::ExpiredDirList(const char* dir_path, unsigned long max_age)
      throw(Exception, El::Exception)
        : threshold_(0)
    {
      ACE_Time_Value time =
        ACE_Time_Value(ACE_OS::gettimeofday().sec() - max_age);
        
      std::string date =
        El::Moment(time).dense_format(El::Moment::DF_DATE);

      if(!El::String::Manip::numeric(date.c_str(), threshold_))
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::ExpiredDirList::ExpiredDirList: "
          "El::String::Manip::numeric failed for '" << date << "'";
        
        throw Exception(ostr.str());
      }

      read(dir_path);
    }
    
    bool
    ExpiredDirList::select(const struct dirent* dir) throw(El::Exception)
    {
      unsigned long date = 0;
      
      return dir->d_type == DT_DIR &&
        El::String::Manip::numeric(dir->d_name, date) && date < threshold_;
    }

    void
    StoredMessage::cleanup_thumb_files(const char* dir_path,
                                       time_t max_age)
      throw(Exception, El::Exception)
    {
      ExpiredDirList list(dir_path, max_age);
      std::string prefix = std::string(dir_path) + "/";
      
      for(unsigned long i = 0; i < list.count(); i++)
      {
        std::string full_path = prefix + list[i].d_name;        
        El::FileSystem::remove_directory(full_path.c_str());
      }
    }

    //
    // SearcheableMessageMap class
    //

    void
    SearcheableMessageMap::dump(std::ostream& ostr) const throw(El::Exception)
    {
      ostr << "\n  messages: " << messages.size() << "\n  words: "
           << words.size() << "\n  norm. forms: " << norm_forms.size()
           << "\n  feeds: " << feeds.size() << "\n  sites: " << sites.size()
           << "\n  events: " << event_to_number.size()
           << "\n  word refs: " << total_words_
           << "\n  norm. form refs: " << total_norm_forms_
           << "\n  word positions: " << total_word_positions_
           << "\n  norm. form positions: " << total_norm_form_positions_
           << "\n  recycled numbers: " << recycled_numbers_.size();
        
      assert(id_to_number.size() == messages.size());
      assert(event_to_number.size() <= messages.size());
      
      assert(signatures_.size() == messages.size() &&
             (suppress_duplicates_ == false ||
              url_signatures_.size() == messages.size()));

      size_t msg_count = 0;
      
      for(SiteToMessageNumberMap::const_iterator it = sites.begin();
          it != sites.end(); it++)
      {
        msg_count += it->second->size();
      }
      
      assert(msg_count == messages.size());

      msg_count = 0;
      
      for(FeedInfoMap::const_iterator it = feeds.begin(); it != feeds.end();
          it++)
      {
        msg_count += it->second->messages.size();
      }
      
      assert(msg_count == messages.size());

      msg_count = 0;

      for(WordToMessageNumberMap::const_iterator it = words.begin();
          it != words.end(); it++)
      {
        msg_count += it->second->messages.size();
      }
      
      assert(msg_count == total_words_);
      
      msg_count = 0;

      for(WordIdToMessageNumberMap::const_iterator it = norm_forms.begin();
          it != norm_forms.end(); it++)
      {
        msg_count += it->second->messages.size();
      }
      
      assert(msg_count == total_norm_forms_);
    }

    void
    SearcheableMessageMap::optimize_mem_usage() throw(El::Exception)
    {
      norm_forms.resize(0);

      for(WordIdToMessageNumberMap::iterator it = norm_forms.begin();
          it != norm_forms.end(); it++)
      {
        it->second->messages.resize(0);
      }
      
      words.resize(0);

      for(WordToMessageNumberMap::iterator it = words.begin();
          it != words.end(); it++)
      {
        it->second->messages.resize(0);
      }

      sites.resize(0);
      
      for(SiteToMessageNumberMap::iterator it = sites.begin();
          it != sites.end(); it++)
      {
        it->second->resize(0);
      }
      
      feeds.resize(0);
      
      for(FeedInfoMap::iterator it = feeds.begin(); it != feeds.end(); it++)
      {
        it->second->optimize_mem_usage();
      }

      event_to_number.resize(0);

      for(EventToNumberMap::iterator it = event_to_number.begin();
          it != event_to_number.end(); it++)
      {
        it->second->resize(0);
      }

      signatures_.resize(0);
      url_signatures_.resize(0);

      root_category->optimize_mem_usage();

      messages.resize(0);
      id_to_number.resize(0);

//      categories2.resize(0);

      StringConstPtr::string_manager.optimize_mem_usage();
    }
    
    StoredMessage*
    SearcheableMessageMap::insert(const StoredMessage& msg,
                                  unsigned long core_words_prc,
                                  unsigned long max_core_words
                                  /* bool sort_cwords*/)
      throw(Exception, El::Exception)
    {        
      El::Stat::TimeMeasurement measurement(insert_meter);

      if(!msg.id.valid())
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::SearcheableMessageMap::insert: "
          "invalid message id=" << msg.id.string();

        throw Exception(ostr.str());
      }

      if(find(msg.id) != 0)
      {
        return 0;
      }

      const Categories::CategoryArray& categories = msg.categories.array;
      
      for(size_t i = 0; i < categories.size(); i++)
      {
        if(categories[i].c_str()[0] != '/')
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::SearcheableMessageMap::insert: "
            "invalid category name '" << categories[i].c_str()
               << "' for message; id=" << msg.id.string();
          
          throw Exception(ostr.str());
        }
      }

      Signature msg_signature = 0;

      if(!viewer_)
      {
        msg_signature = message_signature(msg);

        if(signatures_.find(msg_signature) != signatures_.end() ||
           (suppress_duplicates_ &&
            url_signatures_.find(msg.url_signature) != url_signatures_.end()))
        {
          return 0;
        }
      }

      Number number = assign_number();

      id_to_number[msg.id] = number;
      
      if(!viewer_ && msg.lang != El::Lang::null)
      {        
        LangInfoMap::iterator i = lang_info.find(msg.lang);

        if(i == lang_info.end())
        {
          i = lang_info.insert(std::make_pair(msg.lang, new LangInfo())).first;
        }

        i->second->message_count++;
      }
      
      if(msg.event_id != El::Luid::null)
      {
        EventToNumberMap::iterator it = event_to_number.find(msg.event_id);

        if(it == event_to_number.end())
        {
          it = event_to_number.insert(
            std::make_pair(msg.event_id, new NumberSet())).first;
        }

        it->second->insert(number);
      }

      const MessageWordPosition& word_positions = msg.word_positions;

      for(WordPositionNumber i = 0; i < word_positions.size(); i++)
      {
        const MessageWordPosition::KeyValue& wpos = word_positions[i];
        
        unsigned long positions_count = wpos.second.position_count();
        total_word_positions_ += positions_count;

        const StringConstPtr& word = wpos.first;

        WordToMessageNumberMap::iterator it = words.find(word.c_str());

        if(it == words.end())
        {
          it = words.insert(
            std::make_pair(word.add_ref(), new WordMessages())).first;
        }
          
        WordMessages& word_messages = *it->second;
/*
        if(wpos.second.flags & WordPositions::FL_CAPITAL_WORD)
        {
          std::cerr << word.c_str() << std::endl;          
        }
*/
        
        word_messages.add_message(
          msg.lang,
          wpos.second.flags & WordPositions::FL_CAPITAL_WORD,
          number,
          messages,
          viewer_);
      }

      const NormFormPosition& norm_form_positions = msg.norm_form_positions;

      for(WordPositionNumber i = 0; i < norm_form_positions.size(); i++)
      {
        const NormFormPosition::KeyValue& wpos = norm_form_positions[i];
        unsigned long positions_count = wpos.second.position_count();
        
        total_norm_form_positions_ += positions_count;

        El::Dictionary::Morphology::WordId wid = wpos.first;
        
        WordIdToMessageNumberMap::iterator it = norm_forms.find(wid);

        if(it == norm_forms.end())
        {
          it =
            norm_forms.insert(std::make_pair(wid, new WordMessages())).first;
        }
          
        WordMessages& word_messages = *it->second;
        
        word_messages.add_message(
          msg.lang,
          wpos.second.flags & WordPositions::FL_CAPITAL_WORD,
          number,
          messages,
          viewer_);
      }

      if(!viewer_)
      {
        total_words_ += word_positions.size();      
        total_norm_forms_ += norm_form_positions.size();
      }
      
      if(!msg.hostname.empty())
      {
        SiteToMessageNumberMap::iterator it =
          sites.find(msg.hostname.c_str());

        if(it == sites.end())
        {
          it = sites.insert(
            std::make_pair(msg.hostname.add_ref(), new NumberSet())).first;
        }

        it->second->insert(number);
      }
      
      StoredMessage* message = new StoredMessage(msg);
      messages.insert(std::make_pair(number, message));

      add_categories(*message, number);

      WordsFreqInfo word_freq;      
      
      if(!viewer_)
      {
        if(core_words_prc)
        {
          MessageWordPosition& word_positions = message->word_positions;
        
          for(WordPositionNumber i = 0; i < word_positions.size(); i++)
          {
            MessageWordPosition::KeyValue& wpos = word_positions[i];
            WordPositions& wp = wpos.second;
            
            wp.flags &=
              ~(WordPositions::FL_CORE_WORD |
                WordPositions::FL_TOKEN_TYPE_MASK);

            const StringConstPtr& word = wpos.first;

            bool pname = wp.flags & WordPositions::FL_SENTENCE_PROPER_NAME;

            if(!pname && (wp.flags & WordPositions::FL_CAPITAL_WORD))
            {
              WordToMessageNumberMap::iterator it = words.find(word.c_str());
              assert(it != words.end());
              pname = it->second->proper_name(message->lang);
            }

            if(pname)
            {
              wp.flags |= WordPositions::FL_PROPER_NAME;
              
//              std::cerr << "WPN: " << message->id.string() << " "
//                        << word.c_str() << std::endl;
            }
            else
            {
              wp.flags &= ~WordPositions::FL_PROPER_NAME;
            }

            El::Dictionary::Morphology::TokenType type =
              El::Dictionary::Morphology::token_type(word.c_str());

            wp.token_type((WordPositions::TokenType)type);
          }

          NormFormPosition& norm_form_positions = message->norm_form_positions;
        
          for(WordPositionNumber i = 0; i < norm_form_positions.size(); i++)
          {
            NormFormPosition::KeyValue& wpos = norm_form_positions[i];
            WordPositions& wp = wpos.second;
            
            wp.flags &= ~WordPositions::FL_CORE_WORD;

            bool pname = wp.flags & WordPositions::FL_SENTENCE_PROPER_NAME;

            if(!pname && (wp.flags & WordPositions::FL_CAPITAL_WORD))
            {            
              WordIdToMessageNumberMap::iterator it =
                norm_forms.find(wpos.first);

              assert(it != norm_forms.end());
              pname = it->second->proper_name(message->lang);
            }
              
            if(pname)
            {
              wp.flags |= WordPositions::FL_PROPER_NAME;
            }
            else
            {
              wp.flags &= ~WordPositions::FL_PROPER_NAME;
            }
          }  
        }
        
        calc_words_freq(*message, true, word_freq);
        
        if(core_words_prc)
        {
          const WordsFreqInfo::WordInfoArray& word_infos =
            word_freq.word_infos;

          unsigned long watermark =
            std::min((unsigned long)((float)core_words_prc *
                                     word_infos.size() / 100 + 0.5),
                     max_core_words);

          CoreWords& core_words = message->core_words;
          core_words.resize(watermark);

          MessageWordPosition& word_positions = message->word_positions;
          NormFormPosition& norm_form_positions = message->norm_form_positions;

          size_t i = 0;
        
          for(WordsFreqInfo::WordInfoArray::const_iterator
                it(word_infos.begin()), eit(word_infos.end()); it != eit &&
                (i < CORE_WORD_MIN_NUMBER ||
                 (i < watermark && it->cw_weight >= CORE_WORD_MIN_WEIGHT));
              ++it)
          {
            core_words[i++] = it->id();
          
            const StringConstPtr& text = it->text;
            MessageWordPosition::KeyValue* kw = word_positions.find(text);
            
            kw->second.flags |= WordPositions::FL_CORE_WORD;
          
            if(it->norm_form)
            {
              NormFormPosition::KeyValue* kw =
                norm_form_positions.find(it->norm_form);
            
              kw->second.flags |= WordPositions::FL_CORE_WORD;
            }
          }

          core_words.resize(i < CORE_WORD_MIN_NUMBER ? 0 : i);
        }
        
        if(word_pair_manager_ && message->lang != El::Lang::null &&
           message->core_words.size() > 1)
        {
          // Long and short time indexes can share common space as
          // word_pair_short_period and word_pair_long_period differs so much
          // that resulted indexes occupy non overlapping numeric ranges

//          std::cerr << "insert " << message->id.string() << " "
//                    << message->lang.l3_code() << std::endl;
          
          uint32_t short_time_index =
            message->published / word_pair_short_period;
          
          uint32_t long_time_index =
            ((uint32_t)(message->published / word_pair_long_period));

          for(CoreWords::const_iterator i(message->core_words.begin()),
                e(message->core_words.end()); i != e; ++i)
          {
            // TODO: research
            WordPair wo(*i, 0);
            
            word_pair_manager_->wp_increment_counter(message->lang,
                                                     long_time_index,
                                                     wo);
            
            word_pair_manager_->wp_increment_counter(message->lang,
                                                     short_time_index,
                                                     wo);
            
            for(CoreWords::const_iterator j(i + 1); j != e; ++j)
            {
              WordPair wp(*i, *j);
              
              word_pair_manager_->wp_increment_counter(message->lang,
                                                       long_time_index,
                                                       wp);
              
              word_pair_manager_->wp_increment_counter(message->lang,
                                                       short_time_index,
                                                       wp);              
            }
          }

          if(core_words_prc/* && sort_cwords*/)
          {
            sort_cw(*message, word_freq);
          }
        }
      }
      
      if(!msg.source_url.empty())
      {
        FeedInfoMap::iterator it = feeds.find(msg.source_url.c_str());

        if(it == feeds.end())
        {
          it = feeds.insert(
            std::make_pair(msg.source_url.add_ref(), new FeedInfo())).first;
        }

        FeedInfo* feed_info = it->second;

        message->feed_search_weight = &feed_info->search_weight;
          
        feed_info->impressions += msg.impressions;
        feed_info->clicks += msg.clicks;
        
        El::ArrayPtr<El::Dictionary::Morphology::WordId> msg_words;
        
        if(!viewer_)
        {          
          size_t feed_word_count = 0;
          
          const WordsFreqInfo::WordInfoArray& word_infos =
            word_freq.word_infos;
          
          for(WordsFreqInfo::WordInfoArray::const_iterator
                it(word_infos.begin()), eit(word_infos.end()); it != eit; ++it)
          {
            if(it->feed_countable)
            {
              ++feed_word_count;
            }
          }

          if(feed_word_count)
          {
            msg_words.reset(
              new El::Dictionary::Morphology::WordId[feed_word_count + 1]);

            size_t i = 0;
            WordIdToCountMap& word_counters = feed_info->words;
            
            for(WordsFreqInfo::WordInfoArray::const_iterator
                  it(word_infos.begin()), eit(word_infos.end()); it != eit;
                ++it)
            {
/*              
              if((it->pseudo_id == 4014164171 || it->norm_form == 1506009) &&
                 strcmp(msg.source_url.c_str(),
                        "http://www.aif.ru/rss/news.php") == 0)
              {
                std::cerr << "FC: "
                          << it->pseudo_id << "/" << it->norm_form << "/"
                          << it->text.c_str()
                          << " "
                          << it->feed_countable << std::endl;
              }
*/
              
              if(it->feed_countable)
              {
                
          // Usually feed stop words uses same form, so use pseudo_id
          // here instead of id(). However it can happen that being present
          // in each message of a feed a word is not considered feed stop.
          // For example if word Car present in each message but in half
          // of them it get overriden in word_infos by Cars word having a
          // different pseudo id.
                
//                El::Dictionary::Morphology::WordId word_id = it->id();
                El::Dictionary::Morphology::WordId word_id = it->pseudo_id;
                assert(word_id);
                
                WordIdToCountMap::iterator wit = word_counters.find(word_id);
            
                if(wit == word_counters.end())
                {
                  word_counters[word_id] = 1;
                }
                else
                {
                  ++(wit->second);
                }
/*
                if(word_id == 4014164171 &&
                   strcmp(msg.source_url.c_str(),
                          "http://www.aif.ru/rss/news.php") == 0)
                {
                  std::cerr << "INS: " << word_counters[word_id]
                            << " " << (feed_info->messages.size() + 1)
                            << std::endl;
                }
*/
              
                msg_words[i++] = word_id;
              }
            }

            msg_words[i] = 0;
          }
        }

        feed_info->messages.insert(
          std::make_pair(number, msg_words.release()));
      }

      if(!viewer_)
      {
        signatures_.insert(msg_signature);
      
        if(suppress_duplicates_)
        {
          url_signatures_.insert(msg.url_signature);
        }

        if(message->content.in() != 0)
        {
          message->content->timestamp(ACE_OS::gettimeofday().sec());
        }
      }

      calc_search_weight(*message);

      return message;
    }
/*
    void
    SearcheableMessageMap::sort_core_words(StoredMessage& msg,
                                           WordsFreqInfo* word_freq) const
      throw(Exception, El::Exception)
    {
      if(word_pair_manager_ && msg.lang != El::Lang::null &&
         msg.core_words.size() > 1)
      {
        std::auto_ptr<WordsFreqInfo> wf;
      
        if(word_freq == 0)
        {
          wf.reset(new WordsFreqInfo());
          word_freq = wf.get();
          calc_words_freq(msg, false, *word_freq);        
        }

        sort_cw(msg, *word_freq);
      }
    }    
*/    
    void
    SearcheableMessageMap::sort_cw(StoredMessage& msg,
                                   WordsFreqInfo& word_freq) const
      throw(Exception, El::Exception)
    {
      calc_word_pairs_freq(msg, word_freq);

      MessageWordPosition& word_positions = msg.word_positions;

      for(size_t i = 0; i < word_positions.size(); ++i)
      {
        word_positions[i].second.flags &= ~WordPositions::FL_CORE_WORD;
      }
            
      NormFormPosition& norm_form_positions = msg.norm_form_positions;
      
      for(size_t i = 0; i < norm_form_positions.size(); ++i)
      {
        norm_form_positions[i].second.flags &= ~WordPositions::FL_CORE_WORD;
      }
            
      const WordsFreqInfo::WordInfoArray& word_infos = word_freq.word_infos;
      size_t j = 0;
      size_t core_words_size = msg.core_words.size();

      for(WordsFreqInfo::WordInfoArray::const_iterator i(word_infos.begin()),
            e(word_infos.end()); i != e && j < core_words_size; ++i, ++j)
      {
        msg.core_words[j] = i->id();

        const StringConstPtr& text = i->text;
        MessageWordPosition::KeyValue* kw = word_positions.find(text);
            
        kw->second.flags |= WordPositions::FL_CORE_WORD;
          
        if(i->norm_form)
        {
          NormFormPosition::KeyValue* kw =
            norm_form_positions.find(i->norm_form);
            
          kw->second.flags |= WordPositions::FL_CORE_WORD;
        }
      }

      assert(j == core_words_size);
    }
    
    void
    SearcheableMessageMap::remove(const Id& id) throw(Exception, El::Exception)
    {    
      El::Stat::TimeMeasurement measurement(remove_meter);

      IdToNumberMap::iterator itn = id_to_number.find(id);

      if(itn == id_to_number.end())
      {
        return;
      }

      Number number = itn->second;

      id_to_number.erase(itn);
      recycle_number(number);

      StoredMessageMap::iterator it = messages.find(number);

      if(it == messages.end())
      {
        return;
      }

      StoredMessage* msg = const_cast<StoredMessage*>(it->second);

      if(!viewer_ && msg->lang != El::Lang::null)
      {
        LangInfoMap::iterator i = lang_info.find(msg->lang);
        assert(i != lang_info.end());

        if(word_pair_manager_ && msg->core_words.size() > 1)
        {
          // Long and short time indexes can share common space as
          // word_pair_short_period and word_pair_long_period differs so much
          // that resulted indexes occupy non overlapping numeric ranges
          
//          std::cerr << "remove " << msg->id.string() << " "
//                    << msg->lang.l3_code() << std::endl;

          uint32_t short_time_index = msg->published / word_pair_short_period;

          uint32_t long_time_index =
            ((uint32_t)(msg->published / word_pair_long_period));

          for(CoreWords::const_iterator i(msg->core_words.begin()),
                e(msg->core_words.end()); i != e; ++i)
          {
            // TODO: research
            WordPair wo(*i, 0);
            
            word_pair_manager_->wp_decrement_counter(msg->lang,
                                                     short_time_index,
                                                     wo);
            
            word_pair_manager_->wp_decrement_counter(msg->lang,
                                                     long_time_index,
                                                     wo);
            
            for(CoreWords::const_iterator j(i + 1); j != e; ++j)
            {
              WordPair wp(*i, *j);

              word_pair_manager_->wp_decrement_counter(msg->lang,
                                                       short_time_index,
                                                       wp);

              word_pair_manager_->wp_decrement_counter(msg->lang,
                                                       long_time_index,
                                                       wp);
            }
          }
        }
        
        if(--(i->second->message_count) == 0)
        {
          delete i->second;
          lang_info.erase(i);
        }
      }

      if(!msg->source_url.empty())
      {
        FeedInfo* feed_info = feeds[msg->source_url.c_str()];

        assert(feed_info != 0);
        assert(feed_info->impressions >= msg->impressions);
        assert(feed_info->clicks >= msg->clicks);
        
        feed_info->impressions -= msg->impressions;
        feed_info->clicks -= msg->clicks;

        MessageWordMap& feed_messages = feed_info->messages;
        
        MessageWordMap::iterator it = feed_messages.find(number);
        assert(it != feed_messages.end());

        El::Dictionary::Morphology::WordId* msg_words = it->second;
          
        if(msg_words)
        {
          WordIdToCountMap& word_counters = feed_info->words;
          El::Dictionary::Morphology::WordId word_id = 0;
          
          while((word_id = *msg_words++) != 0)
          {
            WordIdToCountMap::iterator wit = word_counters.find(word_id);
            assert(wit != word_counters.end());

            if(--(wit->second) == 0)
            {
              word_counters.erase(word_id);
            }
          }          

          delete [] it->second;
        }

        feed_messages.erase(number);

        if(feed_messages.empty())
        {
          msg->source_url.remove();
          delete feed_info;

          feeds.erase(msg->source_url.c_str());
        }
        else
        {
          feed_info->calc_search_weight(impression_respected_level_);
        }
      }

      if(msg->event_id != El::Luid::null)
      {
        NumberSet* number_set = event_to_number[msg->event_id];
        number_set->erase(number);

        if(number_set->empty())
        {
          delete number_set;
          event_to_number.erase(msg->event_id);
        }
      }

      if(!viewer_)
      {
        signatures_.erase(message_signature(*msg));
        
        if(suppress_duplicates_)
        {
          url_signatures_.erase(msg->url_signature);
        }
      }

      const MessageWordPosition& word_positions = msg->word_positions;
        
      for(WordPositionNumber i = 0; i < word_positions.size(); i++)
      {
        const MessageWordPosition::KeyValue& wpos = word_positions[i];
        
        unsigned long positions_count = wpos.second.position_count();
        total_word_positions_ -= positions_count;

        const StringConstPtr& word = wpos.first;

        WordMessages* word_messages = words[word.c_str()];
        
        word_messages->remove_message(
          msg->lang,
          wpos.second.flags & WordPositions::FL_CAPITAL_WORD,
          number,
          viewer_);

        if(word_messages->messages.empty())
        {
          word.remove();
          delete word_messages;
          
          words.erase(word.c_str());
        }
      }

      const NormFormPosition& norm_form_positions = msg->norm_form_positions;
        
      for(WordPositionNumber i = 0; i < norm_form_positions.size(); i++)
      {
        const NormFormPosition::KeyValue& wpos = norm_form_positions[i];
        unsigned long positions_count = wpos.second.position_count();
        
        total_norm_form_positions_ -= positions_count;

        El::Dictionary::Morphology::WordId word_id = wpos.first;
          
        WordMessages* word_messages = norm_forms[word_id];
        
        word_messages->remove_message(
          msg->lang,
          wpos.second.flags & WordPositions::FL_CAPITAL_WORD,
          number,
          viewer_);
        
        if(word_messages->messages.empty())
        {
          delete word_messages;
          norm_forms.erase(word_id);
        }
      }

      if(!viewer_)
      {
        total_words_ -= word_positions.size();
        total_norm_forms_ -= norm_form_positions.size();
      }

      if(!msg->hostname.empty())
      {
        NumberSet* number_set = sites[msg->hostname.c_str()];
        number_set->erase(number);

        if(number_set->empty())
        {
          msg->hostname.remove();
          delete number_set;
          
          sites.erase(msg->hostname.c_str());
        }
      }
      
      remove_categories(*msg, number);
      
      delete msg;
      messages.erase(it); 
    }

    void
    SearcheableMessageMap::remove_categories(StoredMessage& msg,
                                             Number number)
      throw(El::Exception)
    {
      const Categories::CategoryArray& array = msg.categories.array;
      size_t len = array.size();

      if(len)
      {
        for(size_t i = 0; i < len; i++)
        {
          std::string lower;
          El::String::Manip::utf8_to_lower(array[i].c_str() + 1, lower);
          root_category->remove(lower.c_str(), number);        
        }

        for(Categories::CategoryArray::const_iterator
              p(msg.category_paths.begin()), e(msg.category_paths.end());
            p != e; ++p)
        {              
          category_counter.decrement(msg.lang, msg.country, p->c_str());
        }

        msg.category_paths.clear();        
      }
    }
    
    void
    SearcheableMessageMap::add_categories(StoredMessage& msg,
                                          Number number)
      throw(El::Exception)
    {
      const Categories::CategoryArray& array = msg.categories.array;      
      size_t len = array.size();

      if(len)
      {
        El::ArrayPtr<char> category_buff;
        size_t category_buff_len = 0;
        
        StringConstPtrSet path_set;
        
        for(size_t i = 0; i < len; ++i)
        {
          const char* cat = array[i].c_str();
          
          std::string lower;
          El::String::Manip::utf8_to_lower(cat + 1, lower);
          root_category->insert(lower.c_str(), number, &msg);

          size_t len = strlen(cat);

          if(len > category_buff_len)
          {
            category_buff.reset(new char[len + 1]);
            category_buff_len = len;
          }

          save_category_paths(cat, len, category_buff.get(), path_set);
        }

        set_category_paths(msg, path_set, number);        
      }
    }

    void
    SearcheableMessageMap::save_category_paths(const char* category,
                                               size_t category_len,
                                               char* buffer,
                                               StringConstPtrSet& path_set)
      throw(El::Exception)
    {
      char* begin = buffer;
      strcpy(begin, category);
                
      for(char* ptr = begin + category_len - 1; ptr >= begin; --ptr)
      {
        if(*ptr == '/')
        {
          ptr[1] = '\0';
              
          StringConstPtr path(begin);
          path_set.insert(path);
        }
      }
    }

    void
    SearcheableMessageMap::set_category_paths(StoredMessage& msg,
                                              StringConstPtrSet& path_set,
                                              Number number)
      throw(El::Exception)
    {
      for(Categories::CategoryArray::const_iterator
            p(msg.category_paths.begin()), e(msg.category_paths.end());
          p != e; ++p)
      {
        path_set.insert(p->c_str());
        category_counter.decrement(msg.lang, msg.country, p->c_str());
      }
      
      msg.category_paths.resize(path_set.size());        

      Categories::CategoryArray::iterator p = msg.category_paths.begin();
        
      for(StringConstPtrSet::iterator i(path_set.begin()), e(path_set.end());
          i != e; ++i, ++p)
      {          
        *p = *i;
        category_counter.increment(msg.lang, msg.country, p->c_str());
      }      
    }

    void
    SearcheableMessageMap::add_category(StoredMessage& msg,
                                        const StringConstPtr& category,
                                        const char* lower_category)
      throw(El::Exception)
    {
      Categories::CategoryArray& cats = msg.categories.array;
      
      size_t cat_len = category.length();
      size_t cat_count = cats.size();
      size_t i = 0;

      for(; i < cat_count &&
            strncmp(category.c_str(), cats[i].c_str(), cat_len); i++);

      if(i == cat_count)
      {
        cats.resize(cat_count + 1);
        cats[cat_count] = category;

        IdToNumberMap::const_iterator it = id_to_number.find(msg.id);
        assert(it != id_to_number.end());
        
        Number number = it->second;

        if(lower_category)
        {
          root_category->insert(lower_category + 1, number, &msg);
        }
        else
        {
          std::string lower;
          El::String::Manip::utf8_to_lower(category.c_str() + 1, lower);
          root_category->insert(lower.c_str(), number, &msg);
        }

        size_t len = strlen(category.c_str());
        El::ArrayPtr<char> category_buff(new char[len + 1]);

        StringConstPtrSet path_set;
        
        save_category_paths(category.c_str(),
                            len,
                            category_buff.get(),
                            path_set);

        set_category_paths(msg, path_set, number);        
      }
    }

    void
    SearcheableMessageMap::set_impressions(StoredMessage& msg,
                                           uint64_t impressions)
      throw(El::Exception)
    {
      if(!msg.source_url.empty())
      {
        FeedInfo* feed_info = feeds[msg.source_url.c_str()];

        assert(feed_info != 0);
        assert(feed_info->impressions >= msg.impressions);
        
        feed_info->impressions -= msg.impressions;
        feed_info->impressions += impressions;
      }

      msg.impressions = impressions;
    }
      
    void
    SearcheableMessageMap::set_clicks(StoredMessage& msg, uint64_t clicks)
      throw(El::Exception)
    {
      if(!msg.source_url.empty())
      {
        FeedInfo* feed_info = feeds[msg.source_url.c_str()];

        assert(feed_info != 0);
        assert(feed_info->clicks >= msg.clicks);
        
        feed_info->clicks -= msg.clicks;
        feed_info->clicks += clicks;
      }

      msg.clicks = clicks;
    }
      
    void
    SearcheableMessageMap::set_categories(StoredMessage& msg,
                                          Categories& categories)
      throw(El::Exception)
    {
      if(msg.categories.array == categories.array)
      {
        msg.categories.swap(categories);
      }
      else
      {
        IdToNumberMap::const_iterator it = id_to_number.find(msg.id);
        assert(it != id_to_number.end());
        
        Number number = it->second;
      
        remove_categories(msg, number);
        msg.categories.swap(categories);
        add_categories(msg, number);
      }
    }

    void
    SearcheableMessageMap::calc_search_weight(StoredMessage& msg) throw()
    {      
      msg.search_weight =
        (uint32_t)(SORT_BY_RELEVANCE_CAPACITY_FACTOR *
                   std::min(msg.event_capacity,
                            SORT_BY_RELEVANCE_MAX_EVENT_SIZE) + 0.5F);

      uint64_t respected_impressions =
        std::max(msg.impressions, (uint64_t)impression_respected_level_);

      if(respected_impressions)
      {
        msg.search_weight +=
          (uint32_t)(SORT_BY_RELEVANCE_RCTR_FACTOR *
                     std::min(msg.clicks, msg.impressions) /
                     respected_impressions + 0.5F);
      }
      
      if(msg.flags & StoredMessage::MF_HAS_IMAGES)
      {
        msg.search_weight +=
          (uint32_t)(SORT_BY_RELEVANCE_IMAGE_WEIGHT * 10000);  
      }

      if(!msg.source_url.empty())
      {
        FeedInfo* feed_info = feeds[msg.source_url.c_str()];
        assert(feed_info != 0);

        feed_info->calc_search_weight(impression_respected_level_);
      }
    }

    void
    SearcheableMessageMap::calc_word_pairs_freq(const StoredMessage& msg,
                                                WordsFreqInfo& wfi)
      const throw(El::Exception)
    {      
      if(word_pair_manager_ && msg.lang != El::Lang::null &&
         msg.core_words.size() > 1)
      {
        uint32_t short_time_index = msg.published / word_pair_short_period;
        uint32_t short_time_prev_index = short_time_index - 1;

        // Long and short time indexes can share common space as
        // word_pair_short_period and word_pair_long_period differs so much
        // that resulted indexes occupy non overlapping numeric ranges  
        
        uint32_t long_time_index =
          ((uint32_t)(msg.published / word_pair_long_period));

        uint32_t long_time_prev_index =
          ((uint32_t)(msg.published / word_pair_long_period - 1));

        assert(long_time_prev_index == long_time_index - 1);
        
        double max_wwp_rel = 0;
        double min_wwp_rel = ULONG_MAX;

        float factor_min = ULONG_MAX;
        float factor_max = 0;
        double factor_sum = 0;
        size_t factor_count = 0;

//        std::cerr << std::endl << msg.id.string() << " pairs:";
        
        WordFreqWeight word_weight;
        
        for(CoreWords::const_iterator b(msg.core_words.begin()), i(b),
              e(msg.core_words.end()); i != e; ++i)
        {          
          for(CoreWords::const_iterator j(i + 1); j != e; ++j)
          {
            WordPair wp(*i, *j);

            uint32_t long_time_pairs_count =
              word_pair_manager_->wp_get(msg.lang, long_time_index, wp) +
              word_pair_manager_->wp_get(msg.lang, long_time_prev_index, wp);

            uint32_t short_time_pairs_count =
              word_pair_manager_->wp_get(msg.lang, short_time_index, wp) +
              word_pair_manager_->wp_get(msg.lang, short_time_prev_index, wp);

            if(short_time_pairs_count > 1)
            {
              float wp_weight =
                //  + 0.72 is to make log = 1 for short_time_pairs_count = 2
                (double)log(short_time_pairs_count + 0.72) *
                short_time_pairs_count /
                long_time_pairs_count;

              word_weight.absorb(*i, wp_weight);
              word_weight.absorb(*j, wp_weight);
            }
          }

          // TODO: research

          El::Dictionary::Morphology::WordId wid = *i;
          
          WordPair wo(wid, 0);
              
          uint32_t long_time_word_count =
            word_pair_manager_->wp_get(msg.lang, long_time_index, wo) +
            word_pair_manager_->wp_get(msg.lang, long_time_prev_index, wo);

          uint32_t short_time_word_count =
            word_pair_manager_->wp_get(msg.lang, short_time_index, wo) +
            word_pair_manager_->wp_get(msg.lang, short_time_prev_index, wo);

          assert(long_time_word_count >= short_time_word_count);

          uint32_t word_count = long_time_word_count - short_time_word_count;
          uint32_t max_pairs_count = 0;
          uint32_t most_often_pair = 0;          

          for(CoreWords::const_iterator j(b); j != e; ++j)
          {
            if(j != i)
            {
              WordPair wp(wid, *j);

              uint32_t long_time_pairs_count =
                word_pair_manager_->wp_get(msg.lang, long_time_index, wp) +
                word_pair_manager_->wp_get(msg.lang, long_time_prev_index, wp);

              uint32_t short_time_pairs_count =
                word_pair_manager_->wp_get(msg.lang, short_time_index, wp) +
                word_pair_manager_->wp_get(msg.lang, short_time_prev_index, wp);

              assert(long_time_pairs_count >= short_time_pairs_count);

              uint32_t pair_count =
                long_time_pairs_count - short_time_pairs_count;

              if(max_pairs_count < pair_count)
              {
                max_pairs_count = pair_count;
                most_often_pair = *j;
              }
            }
          }

          assert(word_count >= max_pairs_count);
/*          
          std::cerr << " " << wid << ":" << word_count << "|"
                    << long_time_word_count
                    << "/" << max_pairs_count << "(" << most_often_pair
                    << ")";
*/
          WordFreqWeight::iterator wit = word_weight.find(wid);

          if(wit != word_weight.end())
          {
            float factor = ULONG_MAX;
            
            if(max_pairs_count)
            {
              factor = log((double)word_count / max_pairs_count + WP_LOG_SHIFT);

              if(factor_max < factor)
              {
                factor_max = factor;
              }

              double wwp_rel = (double)word_count / max_pairs_count;
              
//              std::cerr << "=" << (float)wwp_rel << ";log="
//                        << log(wwp_rel + WP_LOG_SHIFT) << "/" << log(wwp_rel);

              if(max_wwp_rel < wwp_rel)
              {
                max_wwp_rel = wwp_rel;
              }
              
              if(min_wwp_rel > wwp_rel)
              {
                min_wwp_rel = wwp_rel;
              }

              factor_sum += factor;
              ++factor_count;
            }
/*            
            else
            {
              std::cerr << "!";
            }
*/

            wit->second.factor = factor;

            if(factor_min > factor)
            {
              factor_min = factor;
            }
          }
/*          
          else
          {
            std::cerr << "?";
          }
*/
          
          // TODO: research end
        }
/*
        float factor_shift = factor_min < ULONG_MAX - 1 && factor_min > 1 ?
          factor_min - 1 : 0;

        std::cerr << " summary:";
            
        if(max_wwp_rel > 0)
        {
          std::cerr << " wwp " << (float)min_wwp_rel << " ("
                    << log(min_wwp_rel+ WP_LOG_SHIFT) << "/" << log(min_wwp_rel)
                    << ") " << (float)max_wwp_rel << " ("
                    << log(max_wwp_rel+ WP_LOG_SHIFT) << "/" << log(max_wwp_rel)
                    << "); fa " << (float)(factor_sum / factor_count)
                    << " fs " << factor_shift;
        }
*/

//        float np_factor = factor_count ? (float)(factor_sum / factor_count) :
//          (float)1.0;
        
        float np_factor = factor_count ?
          (float)(factor_sum / factor_count + factor_max) / 2 :
          (float)1.0;

        WordsFreqInfo::WordInfoArray& word_infos = wfi.word_infos;
        float max_wp_weight = 0;
        float min_wp_weight = FLT_MAX;
        float max_cw_weight = 0;        
          
        for(WordsFreqInfo::WordInfoArray::iterator i(word_infos.begin()),
              e(word_infos.end()); i != e; ++i)
        {
          if(max_cw_weight < i->cw_weight)
          {
            max_cw_weight = i->cw_weight;
          }
          
          WordFreqWeight::const_iterator j = word_weight.find(i->id());

          if(j != word_weight.end())
          {
            float& wp_weight = i->wp_weight;
//            i->wp_weight = j->second.weight_max;
            
            const WFW& wfw = j->second;

            // TODO: research
            wp_weight = (wfw.weight_average() + wfw.weight_max) / 2 *
//              std::min(wfw.factor - factor_shift, MAX_WP_FACTOR);
              (wfw.factor < ULONG_MAX - 1 ? wfw.factor : np_factor);
            
//            wp_weight = wfw.weight_average();

            if(max_wp_weight < wp_weight)
            {
              max_wp_weight = wp_weight;
            }

            if(min_wp_weight > wp_weight)
            {
              min_wp_weight = wp_weight;
            }
          }
        }

        float avg_wp_weight = max_wp_weight > 0 ?
          (max_wp_weight + min_wp_weight) / 2: 0;        
        
        WordsFreqInfo::WordInfoArray new_word_infos;
        new_word_infos.reserve(word_infos.size());
        
        size_t pos = 0;
        size_t core_words_size = msg.core_words.size();

        float max_weight = 0;
        float min_weight = FLT_MAX;
        
        WordsFreqInfo::WordInfoArray::iterator i(word_infos.begin());
        WordsFreqInfo::WordInfoArray::iterator e(word_infos.end());
        
        for(; i != e && pos < core_words_size; ++i, ++pos)
        {
          float& weight = i->weight;
//          weight += (float)(core_words_size - pos - 1) / core_words_size;

          float cw = (float)(core_words_size - pos) / core_words_size;

//          if(max_wp_weight > 1.0)
          
          if(max_wp_weight > 0.1)
          {
            weight = i->wp_weight + cw * max_wp_weight;
          }
          else
          {
            weight = cw;
          }

          if(max_weight < weight)
          {
            max_weight = weight;
          }

          if(min_weight > weight)
          {
            min_weight = weight;
          }

          WordsFreqInfo::WordInfoArray::iterator j(new_word_infos.begin());

          for(WordsFreqInfo::WordInfoArray::iterator je(new_word_infos.end());
              j != je && j->weight >= weight; ++j);

          new_word_infos.insert(j, *i);
        }

        float avg_weight = max_weight > 0 ?
          (max_weight + min_weight) / 2: 0;

        bool short_msg = core_words_size < 11;

        WordsFreqInfo::WordInfoArray::iterator begin = new_word_infos.begin();
        WordsFreqInfo::WordInfoArray::iterator end = new_word_infos.end();

        if(begin != end &&
           (begin->position_flags &
            WordsFreqInfo::PositionInfo::PF_PROPER_NAME) == 0)
        {
          bool swapped = false;
          
          for(WordsFreqInfo::WordInfoArray::iterator it = begin + 1; it != end;
              ++it)
          {
            const WordsFreqInfo::WordInfo& cwi = *it;
            
            if((cwi.position_flags &
                WordsFreqInfo::PositionInfo::PF_PROPER_NAME) &&
               (cwi.position_flags & WordsFreqInfo::PositionInfo::PF_TITLE))
            {
              WordsFreqInfo::WordInfo wi = *it;
              wi.weight = (long)(begin->weight + 1.0);
              std::copy_backward(begin, it, it + 1);
              *begin = wi;
              swapped = true;
              break;
            }
          }

          if(!swapped)
          {
            float bcww = begin->cw_weight;
            float bwpw = begin->wp_weight;
          
            for(WordsFreqInfo::WordInfoArray::iterator it = begin + 1;
                it != end; ++it)
            {
              const WordsFreqInfo::WordInfo& cwi = *it;
            
              if((cwi.position_flags &
                  WordsFreqInfo::PositionInfo::PF_PROPER_NAME) &&
                 (short_msg || bcww <= cwi.cw_weight || bwpw <= cwi.wp_weight ||
                  cwi.wp_weight >= avg_wp_weight || cwi.weight >= avg_weight))
              {
                WordsFreqInfo::WordInfo wi = *it;
                wi.weight = (long)(begin->weight + 1.0);
                std::copy_backward(begin, it, it + 1);
                *begin = wi;
                break;
              }
            }
          }
        }

        for(; i != e; ++i)
        {
          new_word_infos.push_back(*i);          
        }

        word_infos.swap(new_word_infos);
      }
    }
    
    void
    SearcheableMessageMap::calc_words_freq(const StoredMessage& msg,
                                           bool new_msg,
                                           WordsFreqInfo& result)
      const throw(El::Exception)
    {      
      const FeedInfo* feed_info = 0;
      
      if(!msg.source_url.empty())
      {
        FeedInfoMap::const_iterator it = feeds.find(msg.source_url.c_str());
        
        if(it != feeds.end())
        {
          feed_info = it->second;
        }
      }
/*
      if(strcmp(msg.source_url.c_str(),
                "http://www.aif.ru/rss/news.php") == 0)
      {
        std::cerr << "CALC!\n";
      }
*/

      size_t message_count = lang_message_count(msg.lang);
      assert(message_count > 0);
      
      size_t messages_in_feed = new_msg +
        (feed_info ? feed_info->messages.size() : 0);
      
      const size_t threshold = feed_stop_word_threshold(messages_in_feed);
      
      WordsFreqInfo::WordInfoHashSet word_infos;
      
      WordPositionNumber position_count = 0;
      const MessageWordPosition& word_positions = msg.word_positions;

      for(size_t i = 0; i < word_positions.size(); i++)
      {
        const MessageWordPosition::KeyValue& kw = word_positions[i];
        
        const WordPositions& positions = kw.second;
        const StringConstPtr& word = kw.first;        

        WordToMessageNumberMap::const_iterator wit = words.find(word.c_str());
        assert(wit != words.end());
        
        const WordMessages& word_messages = *(wit->second);
        
        float word_frequency =
          (float)word_messages.lang_message_count(msg.lang, messages) * 100 /
          message_count;

        for(size_t i = 0; i < positions.position_count(); i++)
        {
          WordPosition pos = positions.position(msg.positions, i);
            
          WordsFreqInfo::PositionInfo& pi =
            result.word_positions[pos];

          if(pos < msg.keywords_pos && position_count < pos)
          {
            position_count = pos;
          }
            
          pi.word_position_count = positions.position_count();
          pi.word_frequency = word_frequency;
          pi.word = word;
          pi.lang = positions.lang;
          pi.token_type = positions.token_type();
          
          pi.word_flags = positions.flags &
            (WordPositions::FL_LOCATION_MASK |
             WordPositions::FL_CAPITAL_WORD |
             WordPositions::FL_PROPER_NAME |
             WordPositions::FL_SENTENCE_PROPER_NAME);

          pi.set_position_flag(
            WordsFreqInfo::PositionInfo::PF_CAPITAL,
            positions.flags & WordPositions::FL_CAPITAL_WORD);
          
          pi.set_position_flag(
            WordsFreqInfo::PositionInfo::PF_PROPER_NAME,
            (pi.token_type == WordPositions::TT_WORD ||
             pi.token_type == WordPositions::TT_KNOWN_WORD) &&
            (positions.flags & WordPositions::FL_PROPER_NAME));
          
          pi.set_position_flag(WordsFreqInfo::PositionInfo::PF_TITLE,
                               positions.flags & WordPositions::FL_TITLE);

          pi.set_position_flag(
            WordsFreqInfo::PositionInfo::PF_ALTERNATE_ONLY,
            (positions.flags & (WordPositions::FL_TITLE |
                                WordPositions::FL_DESC)) == 0 &&
            (positions.flags & WordPositions::FL_ALT) != 0);

          pi.set_position_flag(
            WordsFreqInfo::PositionInfo::PF_META_INFO,
            (positions.flags & (WordPositions::FL_TITLE |
                                WordPositions::FL_DESC |
                                WordPositions::FL_ALT)) == 0);
        }
      }

      position_count++;

      WordPosition core_pos_threshold =
        std::min(position_count, CORE_POS_THRESHOLD);
      
      const NormFormPosition& norm_form_positions =
        msg.norm_form_positions;
      
      for(size_t i = 0; i < norm_form_positions.size(); i++)
      {
        const NormFormPosition::KeyValue& kw = norm_form_positions[i];
        const WordPositions& positions = kw.second;

        WordsFreqInfo::PositionInfo pos_info;
          
        pos_info.norm_form_position_count = positions.position_count();

        WordIdToMessageNumberMap::const_iterator nit =
          norm_forms.find(kw.first);
        
        assert(nit != norm_forms.end());
        
        const WordMessages& word_messages = *(nit->second);
        
        pos_info.norm_form_frequency =
          (float)word_messages.lang_message_count(msg.lang, messages) * 100 /
          message_count;

        pos_info.set_position_flag(
          WordsFreqInfo::PositionInfo::PF_CAPITAL,
          positions.flags & WordPositions::FL_CAPITAL_WORD);

        pos_info.set_position_flag(
          WordsFreqInfo::PositionInfo::PF_PROPER_NAME,
          (positions.token_type() == WordPositions::TT_WORD ||
           positions.token_type() == WordPositions::TT_KNOWN_WORD) &&
          (positions.flags & WordPositions::FL_PROPER_NAME));

        pos_info.set_position_flag(
          WordsFreqInfo::PositionInfo::PF_TITLE,
          positions.flags & WordPositions::FL_TITLE);

        pos_info.set_position_flag(
          WordsFreqInfo::PositionInfo::PF_ALTERNATE_ONLY,
          (positions.flags &
           (WordPositions::FL_TITLE | WordPositions::FL_DESC)) == 0 &&
          (positions.flags & WordPositions::FL_ALT) != 0);

        pos_info.set_position_flag(
          WordsFreqInfo::PositionInfo::PF_META_INFO,
          (positions.flags & (WordPositions::FL_TITLE |
                              WordPositions::FL_DESC |
                              WordPositions::FL_ALT)) == 0);

        for(size_t i = 0; i < positions.position_count(); i++)
        {
          WordPosition p = positions.position(msg.positions, i);
          WordsFreqInfo::PositionInfo& pi = result.word_positions[p];

          //
          // When word in position have several norm forms, then
          // most probable (the one with higher frequency and language same
          // as for message as a whole) considered
          //
          if((msg.lang != El::Lang::null && positions.lang == msg.lang &&
              pi.lang != msg.lang) ||
             ((pos_info.norm_form_frequency > pi.norm_form_frequency ||
               (pos_info.norm_form_frequency == pi.norm_form_frequency &&
                pos_info.norm_form > pi.norm_form)) &&
              (msg.lang == El::Lang::null || positions.lang == msg.lang ||
              /*pi.lang != msg.lang*/ pi.lang == El::Lang::null ||
              positions.lang == pi.lang)))
          {
            pi.norm_form_position_count = pos_info.norm_form_position_count;
            pi.norm_form_frequency = pos_info.norm_form_frequency;
            pi.norm_form = kw.first;
            pi.lang = positions.lang;
            pi.token_type = positions.token_type();
            
            pi.norm_form_flags = positions.flags &
              (WordPositions::FL_LOCATION_MASK |
               WordPositions::FL_CAPITAL_WORD |
               WordPositions::FL_PROPER_NAME |
               WordPositions::FL_SENTENCE_PROPER_NAME);
            
            pi.set_position_flag(
              WordsFreqInfo::PositionInfo::PF_CAPITAL,
              std::min(pi.get_position_flag(
                         WordsFreqInfo::PositionInfo::PF_CAPITAL),
                       pos_info.get_position_flag(
                         WordsFreqInfo::PositionInfo::PF_CAPITAL)));
                                 
            pi.set_position_flag(
              WordsFreqInfo::PositionInfo::PF_PROPER_NAME,
              pi.get_position_flag(
                WordsFreqInfo::PositionInfo::PF_CAPITAL) &&
              pos_info.get_position_flag(
                WordsFreqInfo::PositionInfo::PF_PROPER_NAME));
                                 
            pi.set_position_flag(
              WordsFreqInfo::PositionInfo::PF_TITLE,
              std::max(pi.get_position_flag(
                         WordsFreqInfo::PositionInfo::PF_TITLE),
                       pos_info.get_position_flag(
                         WordsFreqInfo::PositionInfo::PF_TITLE)));
                                 
            pi.set_position_flag(
              WordsFreqInfo::PositionInfo::PF_ALTERNATE_ONLY,
              std::min(pi.get_position_flag(
                         WordsFreqInfo::PositionInfo::PF_ALTERNATE_ONLY),
                       pos_info.get_position_flag(
                         WordsFreqInfo::PositionInfo::PF_ALTERNATE_ONLY)));

            pi.set_position_flag(
              WordsFreqInfo::PositionInfo::PF_META_INFO,
              std::min(pi.get_position_flag(
                         WordsFreqInfo::PositionInfo::PF_META_INFO),
                       pos_info.get_position_flag(
                         WordsFreqInfo::PositionInfo::PF_META_INFO)));
          }
        }
      }
/*
      if(strcmp(msg.source_url.c_str(),
                "http://www.aif.ru/rss/news.php") == 0)
      {
        std::cerr << "CALC*\n";
      }
*/
      
      for(WordsFreqInfo::PositionInfoMap::const_iterator it = 
            result.word_positions.begin();
          it != result.word_positions.end(); it++)
      {
        WordsFreqInfo::WordInfo wi;
        const WordsFreqInfo::PositionInfo& pi = it->second;

        wi.position_flags = pi.position_flags;
        wi.text = pi.word;
        wi.norm_form = pi.norm_form;

        wi.pseudo_id =
          El::Dictionary::Morphology::pseudo_id(wi.text.c_str());
        
        wi.lang = pi.lang;
        wi.token_type = pi.token_type;
        
        wi.feed_countable = pi.token_type != WordPositions::TT_STOP_WORD &&
          !pi.get_position_flag(WordsFreqInfo::PositionInfo::PF_META_INFO);

        float in_feed_word_freq = 0;
/*
        if(wi.pseudo_id == 4014164171 &&
           strcmp(msg.source_url.c_str(),
                  "http://www.aif.ru/rss/news.php") == 0)
        {
          std::cerr << "CALC-" << wi.pseudo_id <<
            "/" << wi.norm_form << " " << wi.feed_countable << std::endl;
        }
*/
        
        if(feed_info && messages_in_feed > 1)
        {
          const WordIdToCountMap& word_counters = feed_info->words;
          
          // Usually feed stop words uses same form, so use pseudo_id
          // here instead of id(). However it can happen that being present
          // in each message of a feed a word is not considered feed stop.
          // For example if word Car present in each message but in half
          // of them it get overriden in word_infos by Cars word having a
          // different pseudo id.
          
//          WordIdToCountMap::const_iterator wit = word_counters.find(wi.id());
          
          WordIdToCountMap::const_iterator wit =
            word_counters.find(wi.pseudo_id);

          in_feed_word_freq =
            ((float)(wit == word_counters.end() ? 0 : wit->second) +
             new_msg) * 100 / messages_in_feed;

          if(wi.feed_countable && in_feed_word_freq >= threshold)
          {
            wi.token_type = WordPositions::TT_FEED_STOP_WORD;
          }

/*          
          if(wi.pseudo_id == 4014164171 &&
             strcmp(msg.source_url.c_str(),
                    "http://www.aif.ru/rss/news.php") == 0)
          {
            size_t cnt = ((wit == word_counters.end() ? 0 : wit->second) +
                          new_msg);
            
            std::cerr << "CALC: "
                      << cnt
                      << " " << messages_in_feed
                      << " " << wi.token_type << std::endl;

            if(cnt == 7 && messages_in_feed == 7)
            {
              std::cerr << "X: " << msg.id.string() << "\n";
            }
          }
*/
        }        
        
        float frequency = pi.norm_form_position_count ?
          pi.norm_form_frequency : pi.word_frequency;

        if(messages_in_feed >= MESSAGES_IN_FEED_THRESHOLD &&
           wi.feed_countable)
        {
          frequency = (2 * frequency + in_feed_word_freq) / 3;
        }

        // [0.1-100]        
        float rounded_frequency =
          std::max((float)((unsigned long)(frequency * 10 + 0.5)) / 10,
                   (float)0.1);
        
        size_t word_occurance = pi.norm_form_position_count ?
          pi.norm_form_position_count : pi.word_position_count;

        float rel_position_weight =
          (1.0 - ((float)std::min(it->first, position_count) /
                  position_count));

        float abs_position_weight =
          (1.0 - ((float)std::min(it->first, core_pos_threshold) /
                  core_pos_threshold));

        float position_weight =
          (rel_position_weight + abs_position_weight) / 2;

        wi.cw_weight = word_occurance * pow(position_weight + 1, 3.5) /
          rounded_frequency;

        bool in_title =
          pi.get_position_flag(WordsFreqInfo::PositionInfo::PF_TITLE);
        
        wi.cw_weight = (float)wi.cw_weight *
          (pi.get_position_flag(WordsFreqInfo::PositionInfo::PF_CAPITAL) ?
           CAPITAL_WORD_FACTOR : 1.0) *
          (pi.get_position_flag(WordsFreqInfo::PositionInfo::PF_PROPER_NAME) ?
           PROPER_NAME_WORD_FACTOR : 1.0) *
          (pi.get_position_flag(
             WordsFreqInfo::PositionInfo::PF_ALTERNATE_ONLY) ?
           ALTERNATE_ONLY_WORD_FACTOR :
           (pi.get_position_flag(WordsFreqInfo::PositionInfo::PF_META_INFO) ?
            META_INFO_WORD_FACTOR : 1.0));

        wi.cw_weight *=
          (in_title ? TITLE_TOKEN_FACTOR : TOKEN_FACTOR)[wi.token_type];

        WordsFreqInfo::WordInfoHashSet::iterator wit =
          word_infos.find(wi);

        bool is_new = wit == word_infos.end();

        if(is_new || wit->cw_weight < wi.cw_weight)
        {
          if(!is_new)
          {
            word_infos.erase(wit);
          }
          
          word_infos.insert(wi);
        }
      }

      result.word_infos.reserve(word_infos.size());

      for(WordsFreqInfo::WordInfoHashSet::const_iterator
            it(word_infos.begin()), end(word_infos.end()); it != end; ++it)
      {
        float weight = it->cw_weight;
        
        WordsFreqInfo::WordInfoArray::iterator wit =
          result.word_infos.begin();

        for(WordsFreqInfo::WordInfoArray::iterator
              wend(result.word_infos.end());
            wit != wend && wit->cw_weight > weight; ++wit);

        result.word_infos.insert(wit, *it);
      }
/*
      WordsFreqInfo::WordInfoArray::iterator begin = result.word_infos.begin();
      WordsFreqInfo::WordInfoArray::iterator end = result.word_infos.end();

      if(begin != end && (begin->position_flags &
                          WordsFreqInfo::PositionInfo::PF_PROPER_NAME) == 0)
      {
        for(WordsFreqInfo::WordInfoArray::iterator it = begin + 1; it != end;
            ++it)
        {
          if(it->position_flags & WordsFreqInfo::PositionInfo::PF_PROPER_NAME)
          {
            WordsFreqInfo::WordInfo wi = *it;
            wi.cw_weight = (long)(begin->cw_weight + 1.0);
              
            std::copy_backward(begin, it, it + 1);
            *begin = wi;
            break;
          }
        }
      }
*/
      
/*      
      if(strcmp(msg.source_url.c_str(),
                "http://www.aif.ru/rss/news.php") == 0)
      {
        std::cerr << "CALC?\n";
      }
*/
    }
    
    //
    // StoredMessage::DefaultMessageBuilder class
    //
    bool
    StoredMessage::DefaultMessageBuilder::word(const char* text,
                                               Message::WordPosition position)
      throw(El::Exception)
    {
      std::wstring val;
      El::String::Manip::utf8_to_wchar(text, val);
      
      El::String::Manip::xml_encode(
        val.c_str(),
        output_,
        El::String::Manip::XE_TEXT_ENCODING |
        El::String::Manip::XE_ATTRIBUTE_ENCODING |
        El::String::Manip::XE_PRESERVE_UTF8 |
        El::String::Manip::XE_FORCE_NUMERIC_ENCODING);

      if(max_length_ != SIZE_MAX)
      {
        length_ += val.length();

        if(length_ >= max_length_)
        {
          return false;
        }
      }

      return true;
    }

    inline
    bool
    StoredMessage::DefaultMessageBuilder::interword(const char* text)
      throw(El::Exception)
    {
      std::wstring val;
      El::String::Manip::utf8_to_wchar(text, val);

      El::String::Manip::xml_encode(
        val.c_str(),
        output_,
        El::String::Manip::XE_TEXT_ENCODING |
        El::String::Manip::XE_ATTRIBUTE_ENCODING |
        El::String::Manip::XE_PRESERVE_UTF8 |
        El::String::Manip::XE_FORCE_NUMERIC_ENCODING);

      if(max_length_ != SIZE_MAX)
      {
        length_ += val.length();

        if(length_ >= max_length_)
        {
          return false;
        }
      }

      return true;
    }
    
    inline
    bool
    StoredMessage::DefaultMessageBuilder::segmentation() throw(El::Exception)
    {
      return true;
    }

    void
    Category::insert(const char* path,
                     Number number,
                     const StoredMessage* msg)
      throw(InvalidArg, El::Exception)
    {
      messages.insert(std::make_pair(number, msg));
      
      if(*path == '\0')
      {
//        messages.insert(number);
        return;
      }
      
      const char* name_end = strchr(path, '/');

      if(name_end)
      {
        std::string name(path, name_end - path);
        
        Category_var& child = children[name];
        
        if(child.in() == 0)
        {
          child = new Category();
        }

        try
        {
          child->insert(name_end + 1, number, msg);
          return;
        }
        catch(const InvalidArg&)
        {
        }
      }

      std::ostringstream ostr;
      ostr << "::NewsGate::Message::Category::insert: invalid category name "
        "'" << path << "'";
          
      throw InvalidArg(ostr.str());
    }

    void
    Category::remove(const char* path, Number number) throw(El::Exception)
    {
      messages.erase(number);
      
      if(*path == '\0')
      {
//        messages.erase(number);
        return;
      }
      
      const char* name_end = strchr(path, '/');
      assert(name_end != 0);

      std::string name(path, name_end - path);

      CategoryMap::iterator it = children.find(name);
      assert(it != children.end());

      Category& category = *it->second;

      category.remove(name_end + 1, number);

      if(category.empty())
      {
        children.erase(name);
      }
    }

    const Category*
    Category::find(const char* path) const throw(El::Exception)
    {
      if(*path == '\0')
      {
        return this;
      }
      
      const char* name_end = strchr(path, '/');
      assert(name_end != 0);

      std::string name(path, name_end - path);

      CategoryMap::const_iterator it = children.find(name);

      if(it == children.end())
      {
        return 0;
      }

      return it->second->find(name_end + 1);
    }

    void
    Category::optimize_mem_usage() throw(El::Exception)
    {
      messages.resize(0);
      children.resize(0);

      for(CategoryMap::iterator it = children.begin();
          it != children.end(); it++)
      {
        it->second->optimize_mem_usage();
      }
    }

    //
    // Categories class
    //
    void
    Categories::write(std::ostream& ostr) const
      throw(Exception, El::Exception)
    {
      El::BinaryOutStream bin_str(ostr);
      bin_str << *this;
    }
    
    void
    Categories::write(El::BinaryOutStream& bin_str) const
      throw(Exception, El::Exception)
    {
      // Version
      bin_str << (uint16_t)1 << categorizer_hash << array;
    }

    void
    Categories::read(std::istream& istr)
      throw(Exception, El::Exception)
    {
      El::BinaryInStream bin_str(istr);
      bin_str >> *this;
    }
    
    void
    Categories::read(El::BinaryInStream& bin_str)
      throw(Exception, El::Exception)
    {
      uint16_t version;
      bin_str >> version >> categorizer_hash >> array;
    }
    
  }
}
