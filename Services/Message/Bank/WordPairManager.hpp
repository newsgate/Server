/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Message/Bank/WordPairManager.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_WORDPAIRMANAGER_HPP_
#define _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_WORDPAIRMANAGER_HPP_

#include <stdint.h>

#include <string>
#include <map>
#include <sstream>

#include <google/dense_hash_map>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/Lang.hpp>
#include <El/CRC.hpp>
#include <El/RefCount/All.hpp>
#include <El/TokyoCabinet/DBM.hpp>
#include <El/MySQL/DB.hpp>

#include <Commons/Message/StoredMessage.hpp>

namespace NewsGate
{
  namespace Message
  {
    class TCWordPairManager :
      public virtual ::El::RefCount::DefaultImpl< ::El::Sync::ThreadPolicy >
    {
    public:
      EL_EXCEPTION(Exception, El::ExceptionBase);

      struct LTWPBuff
      {        
        unsigned char buff[2 * sizeof(uint16_t) + 2 * sizeof(uint32_t)];
        
        LTWPBuff() throw() { memset(buff, 0, sizeof(buff)); }
        
        LTWPBuff(const El::Lang& lang,
                 uint32_t ti,
                 const WordPair& wp) throw();

        bool operator==(const LTWPBuff& val) const throw();
        bool operator<(const LTWPBuff& val) const throw();
        
        static const size_t SIZE;
        static const size_t SIZE_NOLANG;
      };

      struct LTWPBuffHash
      {
        size_t operator()(const LTWPBuff& ltwp) const throw();
      };
      
      struct LTWPBuffCounterHashMap :
        public google::dense_hash_map<LTWPBuff, int, LTWPBuffHash>
      {
        LTWPBuffCounterHashMap() throw(El::Exception);
      };
      
    public:
      
      TCWordPairManager(const char* wp_counter_type,
                        const char* id,
                        const char* file_name,
                        bool monolang,
                        size_t bucket_count,
                        size_t cache_size)
        throw(El::Exception);
      
      virtual ~TCWordPairManager() throw();

      void sync() throw(El::Exception);
      void cache_size(size_t val) throw(El::Exception);      
      void stat(bool val) throw(El::Exception);      

      void wp_increment_counter(const El::Lang& lang,
                                uint32_t time_index,
                                const WordPair& wp)
        throw(El::Exception);
      
      void wp_decrement_counter(const El::Lang& lang,
                                uint32_t time_index,
                                const WordPair& wp)
        throw(El::Exception);
      
      uint32_t wp_get(const El::Lang& lang,
                      uint32_t time_index,
                      const WordPair& wp)
        throw(El::Exception);

      void dump_word_pair_freq_distribution(std::ostream& ostr)
        throw(Exception, El::Exception);
      
    private:

      void inc_wp_freq_distr(int count, int incr) throw();
      void sync_ltwp_cache(bool full) throw(El::Exception);
      void stmt_clear() throw();

      int db_increment(const LTWPBuff& key, int increment)
        throw(El::Exception);
      
      void db_delete(const LTWPBuff& key) throw(El::Exception);      
      int db_get(const LTWPBuff& key) throw(El::Exception);

    private:
      
      typedef ACE_Thread_Mutex ThreadMutex;
      typedef ACE_Guard<ThreadMutex> Guard;

      mutable ThreadMutex lock_;

      typedef std::map<LTWPBuff, int> LTWPBuffCounterMap;

      enum COUNTER_TYPE
      {
        CT_HASH,
        CT_BTREE,
        CT_DB
      };

      COUNTER_TYPE wp_counter_type_;
      std::string id_;
      bool monolang_;
      LTWPBuffCounterMap ltwp_counter_map_cache_;
      LTWPBuffCounterHashMap ltwp_counter_hashmap_cache_;
      
      El::TokyoCabinet::DBM_var ltwp_base_;
      El::MySQL::Connection_var connection_;
      size_t cache_size_;
      bool stat_;

      MYSQL_BIND update_query_param_[3];
      MYSQL_BIND select_query_param_;
      MYSQL_BIND select_query_result_;
      MYSQL_BIND delete_query_param_;

      char param_key_[12/*LTWPBuff::SIZE*/];
      int param_count_;
      int result_count_;
      
      MYSQL_STMT* update_statement_;
      MYSQL_STMT* select_statement_;
      MYSQL_STMT* delete_statement_;
      
      unsigned long message_wp_freq_distribution_[100];
    };
    
    typedef El::RefCount::SmartPtr<TCWordPairManager> TCWordPairManager_var;
  }
}


///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Message
  {
    //
    // TCWordPairManager class
    //
    inline
    void
    TCWordPairManager::inc_wp_freq_distr(int count, int incr) throw()
    {
      if(count--)
      {
        int len = sizeof(message_wp_freq_distribution_) /
          sizeof(message_wp_freq_distribution_[0]);
          
        message_wp_freq_distribution_[count < len ? count : (len - 1)] += incr;
      }    
    }

    inline
    void
    TCWordPairManager::stat(bool val) throw(El::Exception)
    {
      Guard guard(lock_);
      stat_ = val;
    }
    
    //
    // LTWPBuff struct
    //
    inline
    TCWordPairManager::LTWPBuff::LTWPBuff(const El::Lang& lang,
                                          uint32_t ti,
                                          const WordPair& wp)
      throw()
    {
      unsigned char* p = buff;

      assert(ti < UINT16_MAX);
      uint16_t t = ti;

      memcpy(p, &t, sizeof(t));
      p += sizeof(t);

      memcpy(p, &wp.word1, sizeof(wp.word1));
      p += sizeof(wp.word1);
      
      memcpy(p, &wp.word2, sizeof(wp.word2));
      p += sizeof(wp.word2);

      uint16_t l = lang.el_code();
      memcpy(p, &l, sizeof(l));
    }

    inline
    bool
    TCWordPairManager::LTWPBuff::operator==(const LTWPBuff& val) const throw()
    {
      return memcmp(buff, val.buff, sizeof(buff)) == 0;
    }

    inline
    bool
    TCWordPairManager::LTWPBuff::operator<(const LTWPBuff& val) const throw()
    {
      return memcmp(buff, val.buff, sizeof(buff)) < 0;
    }

    inline
    size_t
    TCWordPairManager::LTWPBuffHash::operator()(const LTWPBuff& ltwp) const
      throw()
    {
      unsigned long crc = El::CRC32_init();
      El::CRC32(crc, (const unsigned char*)ltwp.buff, sizeof(ltwp.buff));
      return crc;
    }

    //
    // TCWordPairManager::LTWPBuffCounterHashMap struct
    //
    inline
    TCWordPairManager::LTWPBuffCounterHashMap::LTWPBuffCounterHashMap()
      throw(El::Exception)
    {
      set_deleted_key(LTWPBuff(El::Lang::nonexistent, 0, WordPair()));
      set_empty_key(LTWPBuff(El::Lang::nonexistent2, 0, WordPair()));
    }
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_WORDPAIRMANAGER_HPP_
