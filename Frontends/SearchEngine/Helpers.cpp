/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Frontends/SearchEngine/Helpers.cpp
 * @author Karen Arutyunov
 * $Id:$
 */
#include <Python.h>
#include <El/CORBA/Corba.hpp>

#include <string>

#include <El/Exception.hpp>

#include <Commons/Message/StoredMessage.hpp>

#include "Helpers.hpp"
#include "SearchEngine.hpp"

namespace NewsGate
{
  void
  fill_position_set(const NewsGate::Search::WordPositionArray* pos_array,
                    WordPositionSet& word_positions)
    throw(El::Exception)
  {
    word_positions.clear();

    if(pos_array)
    {
      word_positions.resize(pos_array->size());
    
      for(NewsGate::Search::WordPositionArray::const_iterator
            i(pos_array->begin()), e(pos_array->end()); i != e; ++i)
      {
        word_positions.insert(std::make_pair(*i, false));
      }
    }
  }
  
  MessageWriter::MessageWriter(
    std::ostream* postr,
    const Message::Transport::StoredMessageDebug* cur_msg,
    WordPositionSet& word_pos,
    unsigned long formatting,
    size_t word_max_len,
    size_t max_len,
    const char* in_msg_link_class,
    const char* in_msg_link_style)
    throw(El::Exception)
      : length(0),
        poutput_(postr),
        current_msg(cur_msg),
        highlighted_(HL_NONE),
        word_positions_(&word_pos),
        max_word_position_(-1),
        last_word_position_(-1),
        formatting_(formatting),
        word_max_len_(word_max_len),
        max_len_(max_len),
        in_msg_link_class_(in_msg_link_class),
        in_msg_link_style_(in_msg_link_style)
  {
    if(formatting_ & SearchContext::FF_HTML)
    {
      if(!word_positions_->empty())
      {
        if(max_len_)
        {
          for(WordPositionSet::iterator i(word_positions_->begin()),
                e(word_positions_->end()); i != e; ++i)
          {
            max_word_position_ = std::max(max_word_position_, (long)i->first);
            i->second = false;
          }
        }
        else
        {
          for(WordPositionSet::iterator i(word_positions_->begin()),
                e(word_positions_->end()); i != e; ++i)
          {
            i->second = false;
          }
        }

        const Message::StoredMessage& msg = cur_msg->message;
        
        const Message::MessageWordPosition& word_positions =
          msg.word_positions;

        for(size_t i = 0; i < word_positions.size(); i++)
        {
          const Message::WordPositions& positions = word_positions[i].second;

          if(positions.flags & Message::WordPositions::FL_CORE_WORD)
          {
            for(Message::WordPositionNumber j = 0;
                j < positions.position_count(); j++)
            {
              Message::WordPosition pos = positions.position(msg.positions, j);
              WordPositionSet::iterator it = word_positions_->find(pos);

              if(it != word_positions_->end())
              {
                it->second = true;
              }
            }
          }
        }

        const Message::NormFormPosition& norm_form_positions =
          msg.norm_form_positions;

        for(size_t i = 0; i < norm_form_positions.size(); i++)
        {
          const Message::WordPositions& positions =
            norm_form_positions[i].second;

          if(positions.flags & Message::WordPositions::FL_CORE_WORD)
          {
            for(Message::WordPositionNumber j = 0;
                j < positions.position_count(); j++)
            {
              Message::WordPosition pos = positions.position(msg.positions, j);
              WordPositionSet::iterator it = word_positions_->find(pos);

              if(it != word_positions_->end())
              {
                it->second = true;
              }
            }
          }
        }
        
      }
    }
    else
    {
      formatting_ &=
        ~(SearchContext::FF_WRAP_LINKS | SearchContext::FF_FANCY_SEGMENTATION);
    }
  }
  
  bool
  MessageWriter::word(const char* text,
                      Message::WordPosition position)
    throw(El::Exception)
  {
    unsigned long flags = 0;
    bool max_len_exceeded = false;

    if(poutput_)
    {
      WordPositionSet::const_iterator it = word_positions_->find(position);
      
      HighLight highlight = it == word_positions_->end() ?
        HL_NONE : it->second ? HL_CORE_WORD : HL_WORD;

      if(highlight)
      {
        if(highlight != highlighted_)
        {
          if(highlighted_)
          {
            *poutput_ << "</span>";
          }

          *poutput_ << "<span class=\""
                    << ( highlight == HL_CORE_WORD ? "found_core_word" :
                         "found_word") << "\">";
          
          highlighted_ = highlight;
        }
      }
      else if(highlighted_)
      {
        *poutput_ << "</span>";
        highlighted_ = HL_NONE;
      }

      flags = El::String::Manip::XE_TEXT_ENCODING |
        El::String::Manip::XE_ATTRIBUTE_ENCODING |
        El::String::Manip::XE_PRESERVE_UTF8 |
        El::String::Manip::XE_FORCE_NUMERIC_ENCODING;
    }

    std::string url;

    if((formatting_ & SearchContext::FF_WRAP_LINKS) && is_url(text, url))
    {        
      if(poutput_ == 0 || max_len_)
      {
        unsigned long len = 3;
        
        try
        {
          std::wstring wurl;
          El::String::Manip::utf8_to_wchar(url.c_str(), wurl);          
          len = wurl.length();
        }
        catch(const El::String::Manip::InvalidArg&)
        {
        }

        if(max_len_)
        {
          last_word_position_ = position;
          
          if(length + len > max_len_ && position > max_word_position_)
          {    
            max_len_exceeded = true;
          }
        }
        
        if(!max_len_exceeded)
        {
          length += len;
        }
      }

      if(poutput_ && !max_len_exceeded)
      {
        try
        {
          std::string link_encoded;
          El::String::Manip::xml_encode(text, link_encoded, flags);
        
          std::string url_encoded;
          El::String::Manip::xml_encode(url.c_str(), url_encoded, flags);
          
          *poutput_ << "<a href=\"" << link_encoded << "\"";

          if(!in_msg_link_class_.empty())
          {
            *poutput_ << " class=\"" << in_msg_link_class_ << "\"";
          }
          
          if(!in_msg_link_style_.empty())
          {
            *poutput_ << " style=\"" << in_msg_link_style_ << "\"";
          }
          
          *poutput_ << " target=\"_blank\" rel=\"nofollow\">"
                    << url_encoded << "</a>";
        }
        catch(const El::String::Manip::InvalidArg&)
        {
          *poutput_ << "???";
        }
      }
    }
    else
    {
      if(poutput_ == 0 || max_len_)
      {
        unsigned long len = 3;

        try
        {
          std::wstring wtext;
          El::String::Manip::utf8_to_wchar(text, wtext);
          len = wtext.length();
        }
        catch(const El::String::Manip::InvalidArg&)
        {
        }
        
        if(max_len_)
        {
          last_word_position_ = position;
          
          if(length + len > max_len_ && position > max_word_position_)
          {    
            max_len_exceeded = true;
          }
        }
        
        if(!max_len_exceeded)
        {
          length += len;
        }        
      }
      
      if(poutput_ && !max_len_exceeded)
      {
        try
        {
          if(formatting_ & SearchContext::FF_HTML)
          {
//            El::String::Manip::xml_encode(text, *poutput_, flags);

            std::wstring val;
        
            El::String::Manip::utf8_to_wchar(
              text,
              val,
              flags & El::String::Manip::XE_LAX_ENCODING);
/*
            if(val == L"173")
            {
              val += L"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
            }
*/
            
            if(word_max_len_ && val.length() > word_max_len_ + 3)
            {
              val = std::wstring(val.c_str(), word_max_len_) + L"...";
            }
            
            El::String::Manip::xml_encode(val.c_str(), *poutput_, flags);
          }
          else
          {
            *poutput_ << text;
          }
        }
        catch(const El::String::Manip::InvalidArg&)
        {
          *poutput_ << "???";
        }  
      }
    }

    if(poutput_ && max_len_exceeded)
    {
      *poutput_ << "...";
    }
    
    if(highlighted_ && (word_positions_->find(position + 1) ==
                        word_positions_->end() || max_len_exceeded))
    {
      *poutput_ << "</span>";
      highlighted_ = HL_NONE;
    }

    if(poutput_ && current_msg->debug_info.on &&
       (formatting_ & SearchContext::FF_HTML))
    {
      const Message::WordsFreqInfo::PositionInfoMap& word_positions =
        current_msg->debug_info.words_freq.word_positions;

      Message::WordsFreqInfo::PositionInfoMap::const_iterator
        it = word_positions.find(position);

      if(it != word_positions.end())
      {
        const Message::WordsFreqInfo::PositionInfo& pi = it->second;

        poutput_->precision(3);
        
        *poutput_ << std::fixed << "<span class=\"dbg-word\"> [" << position
                  << " " << pi.word_position_count << "/"
                  << pi.word_frequency << "; " << pi.norm_form_position_count
                  << "/" << pi.norm_form_frequency << "/"
                  <<  pi.norm_form << "/" << pi.lang.l3_code() << "; "
          /*<< pi.wp_frequency << "; "*/;

        switch(pi.token_type)
        {
        case Message::WordPositions::TT_UNDEFINED: *poutput_ << "U"; break;
        case Message::WordPositions::TT_WORD: *poutput_ << "W"; break;
        case Message::WordPositions::TT_NUMBER: *poutput_ << "N"; break;
        case Message::WordPositions::TT_SURROGATE: *poutput_ << "R"; break;
        case Message::WordPositions::TT_KNOWN_WORD: *poutput_ << "K"; break;
        case Message::WordPositions::TT_STOP_WORD: *poutput_ << "S"; break;
        case Message::WordPositions::TT_FEED_STOP_WORD:
          {
            *poutput_ << "F";
            break;
          }
        }

        *poutput_ << " ";

        if(pi.position_flags)
        {
          if(pi.get_position_flag(
               Message::WordsFreqInfo::PositionInfo::PF_CAPITAL))
          {
            *poutput_ << "C";
          }
        
          if(pi.get_position_flag(
               Message::WordsFreqInfo::PositionInfo::PF_PROPER_NAME))
          {
            *poutput_ << "P";
          }
        
          if(pi.get_position_flag(
               Message::WordsFreqInfo::PositionInfo::PF_TITLE))
          {
            *poutput_ << "T";
          }
        
          if(pi.get_position_flag(
             Message::WordsFreqInfo::PositionInfo::PF_ALTERNATE_ONLY))
          {
            *poutput_ << "A";
          }
        
          if(pi.get_position_flag(
             Message::WordsFreqInfo::PositionInfo::PF_META_INFO))
          {
            *poutput_ << "M";
          }
        }
        else
        {
          *poutput_ << "-";          
        }

        *poutput_ << " ";
        
        if(pi.word_flags & Message::WordPositions::FL_TITLE)
        {
          *poutput_ << "T";
        }
           
        if(pi.word_flags & Message::WordPositions::FL_DESC)
        {
          *poutput_ << "D";
        }

        if(pi.word_flags & Message::WordPositions::FL_ALT)
        {
          *poutput_ << "A";
        }
          
        if(pi.word_flags & Message::WordPositions::FL_CAPITAL_WORD)
        {
          *poutput_ << "C";
        }

        if(pi.word_flags & Message::WordPositions::FL_PROPER_NAME)
        {
          *poutput_ << "P";
        }

        if(pi.word_flags & Message::WordPositions::FL_SENTENCE_PROPER_NAME)
        {
          *poutput_ << "S";
        }

        *poutput_ << ":";

        if(pi.norm_form_flags & Message::WordPositions::FL_TITLE)
        {
          *poutput_ << "T";
        }
           
        if(pi.norm_form_flags & Message::WordPositions::FL_DESC)
        {
          *poutput_ << "D";
        }

        if(pi.norm_form_flags & Message::WordPositions::FL_ALT)
        {
          *poutput_ << "A";
        }
          
        if(pi.norm_form_flags & Message::WordPositions::FL_CAPITAL_WORD)
        {
          *poutput_ << "C";
        }        
        
        if(pi.norm_form_flags & Message::WordPositions::FL_PROPER_NAME)
        {
          *poutput_ << "P";
        }        
        
        if(pi.norm_form_flags &
           Message::WordPositions::FL_SENTENCE_PROPER_NAME)
        {
          *poutput_ << "S";
        }        
        
        *poutput_ << "]</span>";
      }  
    }

    return !max_len_exceeded;
  }
  
  bool
  MessageWriter::interword(const char* text) throw(El::Exception)
  {
    bool max_len_exceeded = false;

    if(poutput_ == 0 || max_len_)
    {
      unsigned long len = 3;
      
      try
      {
        std::wstring wtext;
        El::String::Manip::utf8_to_wchar(text, wtext);
        len = wtext.length();
      }
      catch(const El::String::Manip::InvalidArg&)
      {
      }
      
      if(max_len_ && length + len > max_len_ &&
         last_word_position_ >= max_word_position_)
      {    
        max_len_exceeded = true;
      }
      
      if(!max_len_exceeded)
      {
        length += len;
      }      
    }

    if(poutput_)
    {
      if(max_len_exceeded)
      {
        *poutput_ << "...";
      }
      else
      {
        try
        {
          if(formatting_ & SearchContext::FF_HTML)
          {
            ::El::String::Manip::xml_encode(
              text,
              *poutput_,
              El::String::Manip::XE_TEXT_ENCODING |
              El::String::Manip::XE_ATTRIBUTE_ENCODING |
              El::String::Manip::XE_PRESERVE_UTF8 |
              El::String::Manip::XE_FORCE_NUMERIC_ENCODING);
          }
          else
          {
            *poutput_ << text;
          }
        }
        catch(const El::String::Manip::InvalidArg&)
        {
          *poutput_ << "???";
        }
      }
    }

    return !max_len_exceeded;
  }
  
  bool
  MessageWriter::segmentation() throw(El::Exception)
  {
    if(poutput_)
    {
      if(formatting_ & SearchContext::FF_FANCY_SEGMENTATION)
      {
        *poutput_ << "<span class=\"dbg-seg\">_</span>";
      }
      else if(formatting_ & SearchContext::FF_SEGMENTATION)
      {
        *poutput_ << " ";        
      }
    }

    return true;
  }
  
}
