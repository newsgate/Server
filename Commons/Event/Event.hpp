/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Commons/Event/Event.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_COMMONS_EVENT_EVENT_HPP_
#define _NEWSGATE_SERVER_COMMONS_EVENT_EVENT_HPP_

#include <stdint.h>

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#include <ext/hash_map>

#include <google/sparse_hash_set>
#include <google/sparse_hash_map>
#include <google/dense_hash_map>
#include <google/dense_hash_set>

#include <El/Exception.hpp>
#include <El/Hash/Hash.hpp>
#include <El/Hash/Map.hpp>
#include <El/BinaryStream.hpp>
#include <El/LightArray.hpp>
#include <El/Luid.hpp>
#include <El/Lang.hpp>
#include <El/ArrayPtr.hpp>

#include <Commons/Message/StoredMessage.hpp>

#define WORD_OVERLAP_DEBUG 1

#ifdef WORD_OVERLAP_DEBUG
#  define WORD_OVERLAP_DEBUG_PARAMS_DECL , std::ostream* debug_stream = 0
#  define WORD_OVERLAP_DEBUG_PARAMS_DEF , std::ostream* debug_stream
#  define WORD_OVERLAP_DEBUG_PARAMS_PASS , debug_stream
#  define WORD_OVERLAP_DEBUG_STREAM_DEF  std::ostringstream dstr;
#  define WORD_OVERLAP_DEBUG_STREAM_PASS , &dstr
#  define WORD_OVERLAP_DEBUG_STREAM_VAL dstr.str()
#else
#  define WORD_OVERLAP_DEBUG_PARAMS_DECL
#  define WORD_OVERLAP_DEBUG_PARAMS_DEF
#  define WORD_OVERLAP_DEBUG_PARAMS_PASS
#  define WORD_OVERLAP_DEBUG_STREAM_DEF
#  define WORD_OVERLAP_DEBUG_STREAM_PASS
#  define WORD_OVERLAP_DEBUG_STREAM_VAL ""
#endif

namespace NewsGate
{
  namespace Event
  {
    EL_EXCEPTION(Exception, El::ExceptionBase);
    
    typedef uint32_t EventNumber;
    
    const EventNumber NUMBER_ZERO = 0;
    const EventNumber NUMBER_UNEXISTENT = 1;

    class EventNumberSet :
      public google::sparse_hash_set<EventNumber,
                                     El::Hash::Numeric<EventNumber> >
    {
    public:
      EventNumberSet() throw(El::Exception) { set_deleted_key(NUMBER_ZERO); }
    };
    
    class EventNumberFastSet :
      public google::dense_hash_set<EventNumber,
                                    El::Hash::Numeric<EventNumber> >
    {
    public:
      EventNumberFastSet() throw(El::Exception);      
    };
    
    struct MessageInfo
    {
      uint64_t published;
      Message::Id id;
        
      MessageInfo(const Message::Id& id_val = Message::Id::zero,
                  uint64_t published_val = 0) throw();

      bool operator<(const MessageInfo& val) const throw();

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };      

    typedef El::LightArray<MessageInfo, uint32_t> MessageInfoArray;
      
    struct EventWordWeight
    {
      uint32_t word_id;
      uint32_t first_count;
      uint64_t weight;

      EventWordWeight(uint32_t word_id_val = 0,
                      uint64_t weight_val = 0,
                      uint32_t first_count_val = 0)
        throw();

      bool operator==(const EventWordWeight& val) const throw();

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };

    typedef El::LightArray<EventWordWeight, uint32_t> EventWordWeightArray;
    typedef std::vector<EventWordWeight> EventWordWeightVector;

    struct EventWordWeightOld
    {
      uint32_t word_id;
      uint64_t weight;

      EventWordWeightOld(uint32_t word_id_val = 0, uint64_t weight_val = 0)
        throw();

      bool operator==(const EventWordWeightOld& val) const throw();

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };

    typedef El::LightArray<EventWordWeightOld, uint32_t>
    EventWordWeightOldArray;

    class EventWordPosMap :
      public google::dense_hash_map<uint32_t,
                                    uint32_t,
                                    El::Hash::Numeric<uint32_t> >
    {
    public:
      EventWordPosMap() throw(El::Exception);
    };

    struct EventWordWC
    {
      uint32_t first_count;
      uint64_t weight;

      EventWordWC() throw() : first_count(0), weight(0) {}
      EventWordWC(uint64_t w, uint32_t c) throw() : first_count(c),weight(w) {}
    };
    
    class EventWordWeightMap :
      public google::dense_hash_map<uint32_t,
//                                    uint64_t,
                                    EventWordWC,
                                    El::Hash::Numeric<uint32_t> >
    {
    public:
      EventWordWeightMap() throw(El::Exception);
    };

    class WordToEventNumberMap :
      public google::sparse_hash_map<uint32_t,
                                     EventNumberSet*,
                                     El::Hash::Numeric<uint32_t> >
    {
    public:
      WordToEventNumberMap() throw(El::Exception) { set_deleted_key(0); }
      ~WordToEventNumberMap() throw();
    };

    struct EventObject
    {
      enum EventFlags
      {
        EF_REVISED = 0x1,
        EF_CAN_MERGE = 0x2,
        EF_DISSENTERS_CLEANUP = 0x4,
        EF_PUSH_IN_PROGRESS = 0x20,
        EF_MEM_ONLY = 0x40,
        EF_DIRTY = 0x80
      };

    private:
      uint8_t strain_;

    public:
      uint8_t flags;
      El::Lang lang;
      int32_t spin;

    private: // Keep (private) members here to compactize the structure
      uint32_t dissenters_;
      uint32_t hash_;
      MessageInfoArray messages_;
      
    public:
      EventWordWeightArray words;
      EventWordPosMap word_positions;
      El::Luid id;
      uint64_t published_min;
      uint64_t published_max;      

      EventObject() throw(El::Exception);

      uint32_t hash() const throw() { return hash_; }

      uint32_t dissenters() const throw() { return dissenters_; }
      void dissenters(uint32_t val) throw();

      uint64_t time_diff(const EventObject& event) const throw();
      
      uint64_t time_range() const throw();
      uint64_t time_range(const EventObject& event) const throw();      
      
      const MessageInfoArray& messages() const throw() { return messages_; }
      MessageInfoArray& messages() throw() { return messages_; }

      const MessageInfo& message(uint32_t index) const throw();
      MessageInfo& message(uint32_t index) throw();

      size_t size() const throw() { return messages_.size(); }      

      void messages(MessageInfoArray& src) throw();

//      static void assert_msg_consistency(const MessageInfoArray& messages)
//        throw();
      
      void get_word_weights(EventWordWeightMap& event_words_pw) const
        throw(El::Exception);

      void set_words(const EventWordWeightMap& event_words_pw,
                     uint32_t max_words,
                     bool respect_most_frequent)
        throw(El::Exception);

      uint64_t cardinality() const throw();
      size_t strain() const throw();
      
      uint8_t persistent_flags() const throw();
      std::string flags_string() const throw(El::Exception);

      bool can_merge(size_t max_strain,
                     size_t max_time_range,
                     size_t max_size) const throw();

      size_t words_overlap(const EventObject& event,
                           size_t* common_word_count = 0
                           WORD_OVERLAP_DEBUG_PARAMS_DECL) const
        throw(El::Exception);

      static size_t words_overlap(const Message::CoreWords& msg_core_words1,
                                  const Message::CoreWords& msg_core_words2,
                                  uint32_t max_core_words_count,
                                  size_t* common_word_count = 0
                                  WORD_OVERLAP_DEBUG_PARAMS_DECL)
        throw(El::Exception);

      size_t words_overlap(const Message::CoreWords& msg_core_words,
                           uint32_t max_core_words_count,
                           const EventWordWeightMap* ewwm = 0,
                           size_t* common_word_count = 0
                           WORD_OVERLAP_DEBUG_PARAMS_DECL) const
        throw(El::Exception);

      static size_t event_merge_level(
        size_t event_size,
        size_t strain,
        uint64_t time_diff,
        uint64_t time_range,
        size_t merge_level_base,
        size_t merge_level_min,
        uint64_t min_rift_time,
        float merge_level_size_based_decrement_step,
        float merge_level_time_based_increment_step,
        float merge_level_range_based_increment_step,
        float merge_level_strain_based_increment_step) throw();

      static uint32_t core_word_weight(uint32_t index,
                                       uint32_t count,
                                       uint32_t max_count) throw();

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);

      void read_old(El::BinaryInStream& bstr) throw(El::Exception);

      void dump(std::ostream& ostr, bool verbose) const
        throw(El::Exception);

      void calc_hash() throw();
      
    private:
      void calc_strain() throw();
      void calc_word_pos() throw();

      static size_t offset_penalty(size_t val) throw();
      static double pw(size_t val) throw();
      static double pw3(size_t val) throw();
      static double pw2(size_t val) throw();
      static double max_overlap(size_t len1, size_t len2) throw();
      static double rev_max_overlap(size_t len1, size_t len2) throw();
      static double overlap_factor(size_t len1, size_t len2) throw();
//      static double comm_words_factor(size_t count) throw();

    public:
      static double pw_mult(size_t val1, size_t val2) throw();
      static double pw_s2(size_t val) throw();
      
    private:
/*
      typedef ACE_Thread_Mutex ThreadMutex;
      typedef ACE_Guard<ThreadMutex> Guard;

      static ThreadMutex lock_;
*/
      static const size_t pw_array_len_ = 100;
      static const size_t pw2_array_len_ = 100;
      static const size_t pw3_array_len_ = 100;
      static const size_t penalty_array_len_ = 100;
      static const double pw_array_[pw_array_len_];
      static const double pw2_array_[pw2_array_len_];
      static const double pw3_array_[pw3_array_len_];
      static const double pw_mult_array_[pw_array_len_][pw_array_len_];
      static const double pw_s2_array_[pw_array_len_];
      static const size_t penalty_array_[penalty_array_len_];
      CONSTEXPR static const double pw_power_ = 1.1;
      CONSTEXPR static const double pw2_power_ = 2;
      CONSTEXPR static const double pw3_power_ = 3;
//      constexpr static const double pw_power_ = 1.0;

      static const size_t max_overlap_array_len_ = 100;
      
      static const double max_overlap_array_
      [max_overlap_array_len_][max_overlap_array_len_];

      static const double rev_max_overlap_array_
      [max_overlap_array_len_][max_overlap_array_len_];

      static const size_t overlap_factor_array_len_ = 100;
      
      static const double overlap_factor_array_
      [overlap_factor_array_len_][overlap_factor_array_len_];
    };
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Event
  {
    //
    // EventWordWeight struct
    //
    inline
    EventWordWeight::EventWordWeight(uint32_t word_id_val,
                                     uint64_t weight_val,
                                     uint32_t first_count_val) throw()
        : word_id(word_id_val),
          first_count(first_count_val),
          weight(weight_val)
    {
    }

    inline
    bool
    EventWordWeight::operator==(const EventWordWeight& val) const throw()
    {
      return word_id == val.word_id;
    }
    
    inline
    void
    EventWordWeight::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr << word_id << weight << first_count;
    }
    
    inline
    void
    EventWordWeight::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> word_id >> weight >> first_count;
    }

    //
    // EventWordWeightOld struct
    //
    inline
    EventWordWeightOld::EventWordWeightOld(uint32_t word_id_val,
                                           uint64_t weight_val) throw()
        : word_id(word_id_val),
          weight(weight_val)
    {
    }

    inline
    bool
    EventWordWeightOld::operator==(const EventWordWeightOld& val) const throw()
    {
      return word_id == val.word_id;
    }
    
    inline
    void
    EventWordWeightOld::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr << word_id << weight;
    }
    
    inline
    void
    EventWordWeightOld::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> word_id >> weight;
    }

    //
    // EventObject struct
    //
    inline
    EventObject::EventObject() throw(El::Exception)
        : strain_(0),
          flags(0),
          spin(0),
          dissenters_(0),
          hash_(0),
          published_min(UINT64_MAX),
          published_max(0)
    {
    }
/*
    inline
    uint64_t
    EventObject::published() const throw()
    {
      uint64_t published = 0;
      MessageInfoArray::const_iterator end = messages_.end();
      
      for(MessageInfoArray::const_iterator it = messages_.begin();
          it != end; ++it)
      {        
        if(published < it->published)
        {
          published = it->published;
        }
      }

      return published;
    }
*/
    
    inline
    uint32_t
    EventObject::core_word_weight(uint32_t index,
                                  uint32_t count,
                                  uint32_t max_count) throw()
    {
      //
      // + 1 is make more frequent among event messages words
      // get higher weight that less frequent but bit more valuable.
      // So for example 6 entrance of word with rating=(count - index) 3 will
      // be better than 3 entrance of word with range 6
      //
//      return (uint32_t)(pow(count - index, 1.6) + 0.5); // + 1;
//      return count - index; // + 1;

//      return (uint32_t)(pw(count - index) + 0.5);

//      uint32_t res = (uint32_t)pw3(count - index);
//      assert(res == pow(count - index, 3));
//      return res;

      double res = count < max_count ?
        pw((size_t)((float)max_count * (count - index) / count + 0.5)) :
        pw(count - index);
      
//      return (uint32_t)((index ? res : (res * 1.5)) + 0.5);
//      return (uint32_t)((index ? res : (res * 2.0)) + 0.5);
      return (uint32_t)((index ? res : (res * 1.75)) + 0.5);
      
//      return (uint32_t)(res + 0.5);
      
//      return (uint32_t)pow(count - index, 3);
    }
    
    inline
    uint64_t
    EventObject::cardinality() const throw()
    {
      uint64_t cardinality = 0;
      EventWordWeightArray::const_iterator end = words.end();
      
      for(EventWordWeightArray::const_iterator it = words.begin();
          it != end; ++it)
      {
        cardinality += it->weight;
      }

      return cardinality;
    }

    inline
    void
    EventObject::calc_hash() throw()
    {
//      EventObject::assert_msg_consistency(messages_);
      
      hash_ = 0;
      published_min = UINT64_MAX;
      published_max = 0;
      
      for(MessageInfoArray::const_iterator i(messages_.begin()),
            e(messages_.end()); i != e; ++i)
      {
        const MessageInfo& mi = *i;
        
        El::CRC(hash_, (const unsigned char*)&mi.id, sizeof(mi.id));

        uint64_t pub = mi.published;

        if(published_min > pub)
        {
          published_min = pub;
        }

        if(published_max < pub)
        {
          published_max = pub;
        }
      }
    }

    inline
    void
    EventObject::calc_word_pos() throw()
    {
      word_positions.clear();
      uint32_t pos = words.size();
      word_positions.resize(pos);
      
      for(EventWordWeightArray::const_iterator i(words.begin()); pos; ++i)
      {
        word_positions[i->word_id] = pos--;
      }
    }
    
    inline
    void
    EventObject::dissenters(uint32_t val) throw()
    {
      dissenters_ = val;

      if(!val)
      {
        flags &= ~EF_DISSENTERS_CLEANUP;
      }
      
      calc_strain();
    }    
      
    inline
    void
    EventObject::calc_strain() throw()
    {
      strain_ = messages_.empty() ? 0 :
        (size_t)(100.0 * dissenters_ / messages_.size() + 0.5);
    }

    inline
    size_t
    EventObject::strain() const throw()
    {
//      assert(!strain_ || strain_ && dissenters_);
      return strain_;
    }
    
    inline
    uint64_t
    EventObject::time_diff(const EventObject& event) const throw()
    {
      if(published_min > event.published_max)
      {
        return published_min - event.published_max;
      }
      
      if(event.published_min > published_max)
      {
        return event.published_min - published_max;
      }

      return 0;
    }

    inline
    const MessageInfo&
    EventObject::message(uint32_t index) const throw()
    {
      return messages_[index];
    }

    inline
    MessageInfo&
    EventObject::message(uint32_t index) throw()
    {
      return messages_[index];
    }
/*
    inline
    void
    EventObject::assert_msg_consistency(const MessageInfoArray& messages)
      throw()
    {
      Message::IdSet ids;

      for(size_t i = 0; i < messages.size(); i++)
      {
        const MessageInfo& msg = messages[i];
        assert(ids.find(msg.id) == ids.end());
        ids.insert(msg.id);
      }
    }
*/  
    inline
    void
    EventObject::messages(MessageInfoArray& src) throw()
    {
//      EventObject::assert_msg_consistency(src);      
      src.release(messages_);
      
      messages_.sort();
//      EventObject::assert_msg_consistency(messages_);

      calc_strain();
      calc_hash();
    }

    inline
    uint64_t
    EventObject::time_range() const throw()
    {
      return messages_.empty() ? 0 : (published_max - published_min);
    }
    
    inline
    uint64_t
    EventObject::time_range(const EventObject& event) const throw()
    {
      return std::max(event.published_max, published_max) -
        std::min(event.published_min, published_min);
    }

    inline
    size_t
    EventObject::offset_penalty(size_t val) throw()
    {
      return val < penalty_array_len_ ? penalty_array_[val] :
        (val * (val + 1) / 2 * 5);
    }
    
    inline
    double
    EventObject::pw(size_t val) throw()
    {
      return val < pw_array_len_ ? pw_array_[val] : pow(val, pw_power_);
//      return pow(val, pw_power_);
    }

    inline
    double
    EventObject::pw2(size_t val) throw()
    {
      return val < pw2_array_len_ ? pw2_array_[val] : pow(val, pw2_power_);
    }

    inline
    double
    EventObject::pw3(size_t val) throw()
    {
      return val < pw3_array_len_ ? pw3_array_[val] : pow(val, pw3_power_);
    }

    inline
    double
    EventObject::pw_mult(size_t val1, size_t val2) throw()
    {
      return val1 < pw_array_len_ && val2 < pw_array_len_ ?
        pw_mult_array_[val1][val2] : (pw(val1) * pw(val2));

//      return pw(val1) * pw(val2);
    }

    inline
    double
    EventObject::max_overlap(size_t len1, size_t len2) throw()
    {
      if(len1 < max_overlap_array_len_ && len2 < max_overlap_array_len_)
      {
        return max_overlap_array_[len1][len2];
//        * comm_words_factor(std::min(len1, len2));
      }

      double max_overlap = 0;
      size_t len_short = std::min(len1, len2);
      size_t len_long = std::max(len1, len2);

      for(; len_short; --len_short, --len_long)
      {
        max_overlap += pw_mult(len_short, len_long);
      }
      
      return max_overlap /* * comm_words_factor(len_short)*/;
    }

    inline
    double
    EventObject::rev_max_overlap(size_t len1, size_t len2) throw()
    {
      if(len1 < max_overlap_array_len_ && len2 < max_overlap_array_len_)
      {
        return rev_max_overlap_array_[len1][len2];
      }

      return (double)100.0 / max_overlap(len1, len2);
    }
    
/*
    inline
    double
    EventObject::comm_words_factor(size_t count) throw()
    {
//      return log(std::max(count, (size_t)2));
//      return log(count + 80);
      return 1;
    }
*/  
    inline
    double
    EventObject::overlap_factor(size_t len1, size_t len2) throw()
    {
      return
        (
        len1 < overlap_factor_array_len_ &&
        len2 < overlap_factor_array_len_ ?
        overlap_factor_array_[len1][len2] :
        ((double)len1 / len2 / pw_s2(len1))
        ); // / comm_words_factor(len1);

//      return (double)len1 / len2 / pw_s2(len1);
    }
    
    inline
    double
    EventObject::pw_s2(size_t val) throw()
    {
      if(val < pw_array_len_)
      {
        return pw_s2_array_[val];
      }

      const double pw_power2 = pw_power_ * 2.0;
      
      double sum = 0;
      
      for(; val; --val)
      {
        sum += pow(val, pw_power2);
      }
      
      return sum;
    }
/*
    inline
    size_t
    EventObject::words_overlap(const EventObject& event,
                               size_t* common_word_count
                               WORD_OVERLAP_DEBUG_PARAMS_DEF)
      const throw(El::Exception)
    {
      size_t len_short = words.size();
      size_t len_long = event.words.size();

      if(len_short > len_long)
      {
        return event.words_overlap(*this,
                                   common_word_count
                                   WORD_OVERLAP_DEBUG_PARAMS_PASS);
      }
      
      typedef std::pair<size_t, size_t> WordPosPair;
      WordPosPair common_words[len_short];

      size_t short_pos = len_short;
      size_t common_words_cnt = 0;

      const EventWordPosMap& word_positions_long = event.word_positions;      
      EventWordPosMap::const_iterator wp_end = word_positions_long.end();
      
      for(EventWordWeightArray::const_iterator i(words.begin());
          short_pos; ++i)
      {
        EventWordPosMap::const_iterator wit =
          word_positions_long.find(i->word_id);

        if(wit != wp_end)
        {
          common_words[common_words_cnt++] =
            WordPosPair(wit->second, short_pos);
        }

        --short_pos;
      }
      
      if(common_word_count)
      {
        *common_word_count = common_words_cnt;
      }

#     ifdef WORD_OVERLAP_DEBUG
      
      if(debug_stream)
      {
        debug_stream->precision(5);
        *debug_stream << " " << common_words_cnt << ":" << len_short << ":"
                      << len_long;
      }
      
#     endif
    
      if(common_words_cnt < 3 &&
         (len_long > 2 || (common_words_cnt == 1 && len_short > 1)))
      {
        return 0;
      }

      double overlap = 0;
      size_t min_long_offset = len_long;

      for(WordPosPair *i(common_words), *e(common_words + common_words_cnt);
          i != e; ++i)
      {
        size_t pos = i->first;
        overlap += pw_mult(pos, i->second);
        min_long_offset = std::min(min_long_offset, len_long - pos);
      }
      
      size_t min_short_offset = len_short - common_words->second;      
      
#     ifdef WORD_OVERLAP_DEBUG
    
      if(debug_stream)
      {
        *debug_stream << " " << overlap << ":" << min_short_offset << ":"
                      << min_long_offset;
      }
    
#     endif

      if(len_short < len_long)
      {
        double sl = (double)len_short / len_long;
        
        min_long_offset = (size_t)((double)min_long_offset * sl + 0.5);
        min_short_offset = (size_t)((double)min_short_offset / sl + 0.5);
        
#       ifdef WORD_OVERLAP_DEBUG
        
        if(debug_stream)
        {
          *debug_stream << " >" << min_short_offset << ":" << min_long_offset;
        }
        
#       endif
      }

      size_t result =
        (size_t)(overlap * rev_max_overlap(len_short, len_long) + 0.5);
      
      size_t penalty = std::max(
        std::min(min_short_offset, min_long_offset) * 22,
        std::max(min_short_offset, min_long_offset) * 6);

#    ifdef WORD_OVERLAP_DEBUG
      
      if(debug_stream)
      {
        *debug_stream << " " << result << ":" << penalty;
      }
        
#    endif
    
      return result < penalty ? 0 : (result - penalty);
    }

*/
    inline
    size_t
    EventObject::words_overlap(const EventObject& event,
                               size_t* common_word_count
                               WORD_OVERLAP_DEBUG_PARAMS_DEF)
      const throw(El::Exception)
    {
      size_t len_short = words.size();
      size_t len_long = event.words.size();

      if(len_short > len_long)
      {
        return event.words_overlap(*this,
                                   common_word_count
                                   WORD_OVERLAP_DEBUG_PARAMS_PASS);
      }
      
      size_t short_common_words[len_short];
      size_t long_common_words[len_short];

      size_t short_pos = len_short;
      size_t common_words_cnt = 0;

      const EventWordPosMap& word_positions_long = event.word_positions;      
      EventWordPosMap::const_iterator wp_end = word_positions_long.end();
      
      for(EventWordWeightArray::const_iterator i(words.begin());
          short_pos; ++i)
      {
        EventWordPosMap::const_iterator wit =
          word_positions_long.find(i->word_id);

        if(wit != wp_end)
        {
          short_common_words[common_words_cnt] = short_pos;
          long_common_words[common_words_cnt++] = wit->second;
        }

        --short_pos;
      }
      
      if(common_word_count)
      {
        *common_word_count = common_words_cnt;
      }

#     ifdef WORD_OVERLAP_DEBUG
      
      if(debug_stream)
      {
        debug_stream->precision(5);
        *debug_stream << " " << common_words_cnt << ":" << len_short << ":"
                      << len_long;
      }
      
#     endif
    
      if(common_words_cnt < 3 &&
         (len_long > 2 || (common_words_cnt == 1 && len_short > 1)))
      {
        return 0;
      }

      std::sort(long_common_words,
                long_common_words + common_words_cnt,
                std::greater<size_t>());

      double overlap = 0;
/*      
      size_t bs = 0;
      size_t ps = 0;

      size_t bl = 0;
      size_t pl = 0;
*/      
      for(size_t *is(short_common_words),
            *ise(short_common_words + common_words_cnt), *il(long_common_words);
          is != ise; ++is, ++il)
      {
/*        
        if(*is + 1 < ps)
        {
          bs = 0;
        }

        if(*il + 1 < pl)
        {
          bl = 0;
        }

        overlap += pw_mult(std::max(*is, bs), std::max(*il, bl));

        ps = *is;
        pl = *il;
        
        if(!bs)
        {
          bs = ps;
        }
        
        if(!bl)
        {
          bl = pl;
        }
*/
        overlap += pw_mult(*is, *il);
      }
      
      size_t short_offset = len_short - *short_common_words;
      size_t long_offset = len_long - *long_common_words;
      
#     ifdef WORD_OVERLAP_DEBUG
    
      if(debug_stream)
      {
        *debug_stream << " " << overlap << ":" << short_offset << ":"
                      << long_offset;
      }
    
#     endif

      if(len_short < len_long)
      {
        double sl = (double)len_short / len_long;
        
        short_offset = (size_t)((double)short_offset / sl + 0.5);
        long_offset = (size_t)((double)long_offset * sl + 0.5);
        
#       ifdef WORD_OVERLAP_DEBUG
        
        if(debug_stream)
        {
          *debug_stream << " >" << short_offset << ":" << long_offset;
        }
        
#       endif
      }

      size_t result =
        (size_t)(overlap * rev_max_overlap(len_short, len_long) + 0.5);

      size_t offset = short_offset + long_offset;

/*      
      size_t penalty =
        ((short_offset ? short_offset * (short_offset + 1) / 2 : 0) +
         (long_offset ? long_offset * (long_offset + 1) / 2 : 0)) * 5;
*/

      if(offset)
      {
//        size_t penalty = offset * (offset + 1) / 2 * 5;
        size_t penalty = offset_penalty(offset);

#    ifdef WORD_OVERLAP_DEBUG
      
        if(debug_stream)
        {
          *debug_stream << " " << result << ":" << penalty;
        }
        
#    endif
        
        return result < penalty ? 0 : (result - penalty);
      }
      else
      {
#    ifdef WORD_OVERLAP_DEBUG
      
        if(debug_stream)
        {
          *debug_stream << " " << result << ":" << 0;
        }
        
#    endif
        return result;
      }
    
//      return result < penalty ? 0 : (result - penalty);
//      return std::min(result < penalty ? 0 : (result - penalty), (size_t)100);
    }

    inline
    size_t
    EventObject::event_merge_level(
      size_t event_size,
      size_t strain,
      uint64_t time_diff,
      uint64_t time_range,
      size_t merge_level_base,
      size_t merge_level_min,
      uint64_t min_rift_time,
      float merge_level_size_based_decrement_step,
      float merge_level_time_based_increment_step,
      float merge_level_range_based_increment_step,
      float merge_level_strain_based_increment_step)
      throw()
    {
//      std::cerr << merge_level_base;
        
      size_t level = merge_level_base +
        (size_t)(merge_level_range_based_increment_step * time_range);

//      std::cerr << " " << level;
      
      if(time_diff > min_rift_time)
      {
        level += (size_t)(merge_level_time_based_increment_step *
                          (time_diff - min_rift_time));
      }      
/*
      std::cerr << " " << level << "("
                << merge_level_time_based_increment_step << " " << time_diff
                << " " << min_rift_time << ")";
*/
      
      level += (size_t)(merge_level_strain_based_increment_step * strain);
      
//      std::cerr << " " << level;
      
      size_t decr =
        (size_t)(merge_level_size_based_decrement_step * event_size);

//      std::cerr << " (" << merge_level_size_based_decrement_step
//                << "*" << event_size << "=" << decr << ")";
        
      level = level > decr ? level - decr : 0;

//      std::cerr << " " << level;
      
      return std::min(std::max(merge_level_min, level), (size_t)100);

//      level = std::min(std::max(merge_level_min, level), (size_t)100);
//      std::cerr << " " << level << std::endl;
//      return level;
    }

    inline
    size_t
    EventObject::words_overlap(const Message::CoreWords& msg_core_words,
                               uint32_t max_core_words_count,
                               const EventWordWeightMap* ewwm,
                               size_t* common_word_count
                               WORD_OVERLAP_DEBUG_PARAMS_DEF) const
      throw(El::Exception)
    {
/*      
      if(messages().size() <= 1)
      {
        return 100;
      }
*/
      size_t core_words_count = msg_core_words.size();

      EventWordWeightMap event_words_pw;

      if(ewwm)
      {
        event_words_pw = *ewwm;
      }
      else
      {
        get_word_weights(event_words_pw);
      }

      EventWordWeightMap msg_event_words_pw;
      EventWordWeightMap::iterator event_words_pw_end = event_words_pw.end();

      for(size_t i = 0; i < core_words_count; ++i)
      {
        uint32_t word_id = msg_core_words[i];
        
        uint64_t mcww =
          core_word_weight(i, core_words_count, max_core_words_count);

        msg_event_words_pw[word_id] = EventWordWC(mcww, 0);

        EventWordWeightMap::iterator it = event_words_pw.find(word_id);

        if(it != event_words_pw_end)
        {
//          uint64_t& ecww = it->second;          
          EventWordWC& ecww = it->second;
          
          if(ecww.weight < mcww)
          {
            ecww.weight = 0;
          }
          else
          {
            ecww.weight -= mcww;
          }

          if(!i && ecww.first_count)
          {
            --ecww.first_count;
          }
        }
      }
      
      EventObject no_msg_event;
      no_msg_event.set_words(event_words_pw, UINT32_MAX, true);

      EventObject msg_event;
      msg_event.set_words(msg_event_words_pw, UINT32_MAX, false);
      
      return msg_event.words_overlap(no_msg_event,
                                     common_word_count
                                     WORD_OVERLAP_DEBUG_PARAMS_PASS);
    }
      
    inline
    size_t
    EventObject::words_overlap(const Message::CoreWords& msg_core_words1,
                               const Message::CoreWords& msg_core_words2,
                               uint32_t max_core_words_count,
                               size_t* common_word_count
                               WORD_OVERLAP_DEBUG_PARAMS_DEF)
      throw(El::Exception)
    {
      EventWordWeightMap msg_event_words_pw1;
      size_t core_words_count1 = msg_core_words1.size();

      for(size_t i = 0; i < core_words_count1; ++i)
      {
        msg_event_words_pw1[msg_core_words1[i]] =
          EventWordWC(
            core_word_weight(i, core_words_count1, max_core_words_count),
            0);
      }

      EventWordWeightMap msg_event_words_pw2;
      size_t core_words_count2 = msg_core_words2.size();

      for(size_t i = 0; i < core_words_count2; ++i)
      {
        msg_event_words_pw2[msg_core_words2[i]] =
          EventWordWC(
            core_word_weight(i, core_words_count2, max_core_words_count),
            0);
      }
      
      EventObject msg_event1;
      msg_event1.set_words(msg_event_words_pw1, UINT32_MAX, false);
      
      EventObject msg_event2;
      msg_event2.set_words(msg_event_words_pw2, UINT32_MAX, false);
      
      return msg_event1.words_overlap(msg_event2,
                                      common_word_count
                                      WORD_OVERLAP_DEBUG_PARAMS_PASS);
    }
      
    inline
    void
    EventObject::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      uint8_t pf = persistent_flags();
        
      bstr << id << pf << spin << dissenters_ << lang;
/*
      std::cerr << "write:" << id.string() << " " << std::hex
                << (uint64_t)flags << "/" << std::hex
                << (uint64_t)pf << std::endl;
*/

      words.write(bstr);
      messages_.write(bstr);
    }

    inline
    void
    EventObject::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> id >> flags >> spin >> dissenters_ >> lang;
      
      words.read(bstr);
      messages_.read(bstr);
      
      calc_strain();
      calc_hash();
      calc_word_pos();
//      std::cerr << "read:" << id.string() << " " << std::hex << (uint64_t)flags
//                << std::endl;
    }
    
    inline
    void
    EventObject::read_old(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> id >> flags >> spin >> dissenters_ >> lang;
      
      EventWordWeightOldArray words_old;
      words_old.read(bstr);

      words.resize(words_old.size());

      for(uint32_t i = 0; i < words_old.size(); ++i)
      {
        words[i] = EventWordWeight(words_old[i].word_id, words_old[i].weight);
      }
      
      messages_.read(bstr);
      
      calc_strain();
      calc_hash();
      calc_word_pos();
    }
    
    inline
    void
    EventObject::get_word_weights(EventWordWeightMap& event_words_pw) const
      throw(El::Exception)
    {
      for(EventWordWeightArray::const_iterator wit(words.begin()),
            end(words.end()); wit != end; ++wit)
      {
        const EventWordWeight& eww = *wit;
        EventWordWeightMap::iterator it = event_words_pw.find(eww.word_id);

        if(it == event_words_pw.end())
        {
          event_words_pw.insert(
            std::make_pair(eww.word_id,
                           EventWordWC(eww.weight, eww.first_count)));
        }
        else
        {
          EventWordWC& wc = it->second;
          wc.weight += eww.weight;
          wc.first_count += eww.first_count;
          
//          it->second += wit->weight;
        }
      }
    }
    
    inline
    void
    EventObject::set_words(const EventWordWeightMap& event_words_pw,
                           uint32_t max_words,
                           bool respect_most_frequent)
      throw(El::Exception)
    {
      EventWordWeightVector event_word_weights;
      event_word_weights.reserve(event_words_pw.size());

      uint64_t max_weight = 0;
      uint32_t max_count = 0;
      uint32_t max_id = 0;
      
      for(EventWordWeightMap::const_iterator it(event_words_pw.begin()),
            end(event_words_pw.end()); it != end; ++it)
      {
        uint32_t id = it->first;
        const EventWordWC& wc = it->second;
        
        uint64_t weight = wc.weight;

        if(respect_most_frequent)
        {
          uint32_t count = wc.first_count;

          if(count && (max_count < count ||
                       (max_count == count && max_weight < weight)))
          {
            max_count = count;
            max_weight = weight;
            max_id = id;
          }
        }

        EventWordWeightVector::iterator eit = event_word_weights.begin();
        EventWordWeightVector::iterator eend = event_word_weights.end();
        
        for(; eit != eend && eit->weight > weight; ++eit);
        
        event_word_weights.insert(eit,
                                  EventWordWeight(id, weight, wc.first_count));
      }

      if(max_count > 1)
      {
        assert(!event_word_weights.empty());
        
        for(EventWordWeightVector::iterator b(event_word_weights.begin()),
              i(b), e(event_word_weights.end()); i != e; ++i)
        {
          if(i->word_id == max_id)
          {
            if(i != b)
            {
              EventWordWeight tmp = *i;              
              std::copy_backward(b, i, i + 1);
              *b = tmp;
              
//              std::cerr << "N: " << max_id << std::endl;
            }
/*            
            else
            {
              std::cerr << "F: " << max_id << std::endl;
            }
*/

            break;
          }
        }
      }

      size_t size = event_word_weights.size();
      
      for(size_t i = 0; i < size; ++i)
      {
        unsigned long weight = event_word_weights[i].weight;
        
        if((weight == 0 || (i >= max_words &&
                            weight != event_word_weights[i - 1].weight)))
        {
          event_word_weights.resize(i);
          break;
        }
      }

      words.init(event_word_weights);
      calc_word_pos();
    }
    
    inline
    uint8_t
    EventObject::persistent_flags() const throw()
    {
      return flags & ~(EF_MEM_ONLY | EF_DIRTY | EF_PUSH_IN_PROGRESS);
    }

    inline
    bool
    EventObject::can_merge(size_t max_strain,
                           size_t max_time_range,
                           size_t max_size) const throw()
    {
      return (flags & EF_DISSENTERS_CLEANUP) == 0 &&
        time_range() <= max_time_range &&
        strain() <= max_strain && published_min &&
        messages_.size() <= max_size &&
        (flags & EF_PUSH_IN_PROGRESS) == 0;
    }
    
    inline
    std::string
    EventObject::flags_string() const throw(El::Exception)
    {
      std::string res(flags ? " " : "");
      
      if(flags & EF_REVISED)
      {
        res += "R";
      }
        
      if(flags & EF_DIRTY)
      {
        res += "D";
      }
        
      if(flags & EF_MEM_ONLY)
      {
        res += "M";
      }
      
      if(flags & EF_PUSH_IN_PROGRESS)
      {
        res += "P";
      }

      if(flags & EF_CAN_MERGE)
      {
        res += "G";
      }

      if(flags & EF_DISSENTERS_CLEANUP)
      {
        res += "C";
      }

      return res;
    }
    
    inline
    void
    EventObject::dump(std::ostream& ostr, bool verbose) const
      throw(El::Exception)
    {
      ostr << "event " << id.string() << " lang " << lang.l3_code()
           << " hash 0x"<< std::hex << hash_ << " flags 0x"
           << (unsigned long)flags << flags_string()
           << std::dec << " spin " << spin << " dissenters " << dissenters()
           << " strain " << strain() << " time "
           << published_max << "-" << published_min << "=" << time_range()
           << "\nwords (" << words.size() << "):";

      for(size_t i = 0; i < words.size(); i++)
      {
        unsigned long wid = words[i].word_id;
        
        ostr << ((wid & 0x80000000) ? " $" : " ") << wid << " /"
             << words[i].weight;
      }

      ostr << "\n" << messages_.size() << " MSG";

      if(verbose)
      {
        for(size_t i = 0; i < messages_.size(); i++)
        {
          ostr << " " << messages_[i].id.string() << "/"
               << messages_[i].published;
        }
      }
    }

    //
    // MessageInfo struct
    //
    inline
    MessageInfo::MessageInfo(const Message::Id& id_val,
                             uint64_t published_val) throw()
        : published(published_val), id(id_val)
    {
    }

    inline
    bool
    MessageInfo::operator<(const MessageInfo& val) const throw()
    {
      return id < val.id;
    }
      
    inline
    void
    MessageInfo::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << id << published;
    }
      
    inline
    void
    MessageInfo::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> id >> published;
    }

    //
    // WordToEventNumberMap class
    //
    inline
    WordToEventNumberMap::~WordToEventNumberMap() throw()
    {
      for(iterator it = begin(); it != end(); it++)
      {
        delete it->second;
      }
    }

    //
    // EventWordPosMap class
    //
    inline
    EventWordPosMap::EventWordPosMap() throw(El::Exception)
    {
      set_empty_key(0);
      set_deleted_key(UINT32_MAX);
//      set_deleted_key(0);
    }

    //
    // EventWordWeightMap class
    //
    inline
    EventWordWeightMap::EventWordWeightMap() throw(El::Exception)
    {
      set_empty_key(0);
      set_deleted_key(UINT32_MAX);
    }

    //
    // EventNumberFastSet class
    //
    inline
    EventNumberFastSet::EventNumberFastSet() throw(El::Exception)
    {
      set_deleted_key(NUMBER_ZERO);
      set_empty_key(NUMBER_UNEXISTENT);
    }
    
  }
}

#endif // _NEWSGATE_SERVER_COMMONS_EVENT_EVENT_HPP_
