/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */


/**
 * @file   NewsGate/Server/Frontends/SearchEngine/Helpers.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_FRONTENDS_SEARCHENGINE_HELPERS_HPP_
#define _NEWSGATE_SERVER_FRONTENDS_SEARCHENGINE_HELPERS_HPP_

#include <iostream>
#include <string>

#include <google/dense_hash_map>
#include <google/sparse_hash_set>

#include <El/Exception.hpp>

#include <Commons/Message/StoredMessage.hpp>
#include <Commons/Search/TransportImpl.hpp>
#include <Commons/Message/TransportImpl.hpp>

#include "Helpers.hpp"

namespace NewsGate
{
  struct WordPositionSet :
    public google::dense_hash_map<Message::WordPosition,
                                  bool,
                                  El::Hash::Numeric<Message::WordPosition> >
  {
    WordPositionSet() throw(El::Exception);
  };
  
  class MessageWriter : public Message::StoredMessage::MessageBuilder
  {
  public:
    unsigned long length;

  public:
    MessageWriter(size_t word_max_len) throw(El::Exception);
        
    MessageWriter(std::ostream* postr,
                  const Message::Transport::StoredMessageDebug* cur_msg,
                  WordPositionSet& word_pos,
                  unsigned long formatting,
                  size_t word_max_len,
                  size_t max_len,
                  const char* in_msg_link_class,
                  const char* in_msg_link_style)
      throw(El::Exception);

    virtual ~MessageWriter() throw() {}

  private:

    virtual bool word(const char* text, Message::WordPosition position)
      throw(El::Exception);

    virtual bool interword(const char* text) throw(El::Exception);
    virtual bool segmentation() throw(El::Exception);
    
    bool is_url(const char* text, std::string& url) throw(El::Exception);

  private:

    enum HighLight
    {
      HL_NONE,
      HL_CORE_WORD,
      HL_WORD
    };
    
    std::ostream* poutput_;
    const Message::Transport::StoredMessageDebug* current_msg;
    HighLight highlighted_;
    WordPositionSet* word_positions_;
    long max_word_position_;
    long last_word_position_;
    unsigned long formatting_;
    size_t word_max_len_;
    size_t max_len_;
    std::string in_msg_link_class_;
    std::string in_msg_link_style_;
  };
  
  class TextStatusChecker : public Message::StoredMessage::MessageBuilder
  {
  public:
    
    enum AltStatus
    {
      AS_EMPTY,
      AS_NOT_HIGHLIGHTED,
      AS_HIGHLIGHTED
    };

    AltStatus status;
      
    TextStatusChecker(const WordPositionSet& word_pos) throw(El::Exception);
    virtual ~TextStatusChecker() throw() {}
    
    virtual bool word(const char* text, Message::WordPosition position)
      throw(El::Exception);

    virtual bool interword(const char* text) throw(El::Exception);
    virtual bool segmentation() throw(El::Exception);

  private:
    const WordPositionSet& word_positions_;
  };      

  class EmptyChecker : public Message::StoredMessage::MessageBuilder
  {
  public:
    
    EmptyChecker() throw(El::Exception) : empty(true) {}
    virtual ~EmptyChecker() throw() {}
    
    virtual bool word(const char* text, Message::WordPosition position)
      throw(El::Exception);

    virtual bool interword(const char* text) throw(El::Exception);
    virtual bool segmentation() throw(El::Exception);
    
    bool empty;
  };      

  void fill_position_set(const NewsGate::Search::WordPositionArray* pos_array,
                         WordPositionSet& word_positions)
    throw(El::Exception);  
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  //
  // NewsGate::MessageWriter class
  //
  inline
  MessageWriter::MessageWriter(size_t word_max_len)
    throw(El::Exception)
      : length(0),
        poutput_(0),
        highlighted_(HL_NONE),
        word_positions_(0),
        formatting_(0),
        word_max_len_(word_max_len),
        max_len_(0)
  {
  }
    
  inline
  bool
  MessageWriter::is_url(const char* text, std::string& url)
    throw(El::Exception)
  {
    unsigned long prefix_len = 0;
      
    if(strncmp(text, "http://", prefix_len = 7) == 0) {}
    else if(strncmp(text, "https://", prefix_len = 8) == 0) {}
    else if(strncmp(text, "ftp://", prefix_len = 6) == 0) {}
    else
    {
      return false;
    }
    
    unsigned long len = strcspn(text + prefix_len, "/?");
      
    url.assign(text, len + prefix_len);
    
    const char* ptr = text + len + prefix_len;
      
    if(*ptr != '\0')
    {
      url.append(ptr, 1);
      
      if(ptr[1] != '\0')
      {
        url += "...";
      }
    }

    return true;
  }

  //
  // NewsGate::MessageWriter::WordPositionSet class
  //
  inline
  WordPositionSet::WordPositionSet() throw(El::Exception)
  {
    set_empty_key(Message::WORD_POSITION_MAX);
  }

  //
  // EmptyChecker class
  //  
  inline
  TextStatusChecker::TextStatusChecker(const WordPositionSet& word_pos)
    throw(El::Exception)
      : status(AS_EMPTY),
        word_positions_(word_pos)
  {
  }

  inline
  bool
  TextStatusChecker::word(const char* text, Message::WordPosition position)
    throw(El::Exception)
  {
    if(word_positions_.find(position) != word_positions_.end())
    {
      status = AS_HIGHLIGHTED;  
    }
    else if(status == AS_EMPTY)
    {
      status = AS_NOT_HIGHLIGHTED;
    }
      
    return status != AS_HIGHLIGHTED;
  }
  
  inline
  bool
  TextStatusChecker::interword(const char* text) throw(El::Exception)
  {
    return true;
  }

  inline
  bool
  TextStatusChecker::segmentation() throw(El::Exception)
  {
    return true;    
  }  
  
  //
  // EmptyChecker class
  //  
  inline
  bool
  EmptyChecker::word(const char* text, Message::WordPosition position)
    throw(El::Exception)
  {
    empty = false;
    return false;
  }
  
  inline
  bool
  EmptyChecker::interword(const char* text) throw(El::Exception)
  {
    return true;
  }

  inline
  bool
  EmptyChecker::segmentation() throw(El::Exception)
  {
    return true;    
  }

}

#endif // _NEWSGATE_SERVER_FRONTENDS_SEARCHENGINE_HELPERS_HPP_


