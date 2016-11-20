/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Message/Bank/WordPairManager.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <sstream>
#include <string>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/TokyoCabinet/HashDBM.hpp>
#include <El/TokyoCabinet/BTreeDBM.hpp>

#include "BankMain.hpp"
#include "WordPairManager.hpp"

namespace NewsGate
{
  namespace Message
  {
    const size_t TCWordPairManager::LTWPBuff::SIZE = 12;
    const size_t TCWordPairManager::LTWPBuff::SIZE_NOLANG = 10;
    
    //
    // TCWordPairManager class
    //
    TCWordPairManager::TCWordPairManager(const char* wp_counter_type,
                                         const char* id,
                                         const char* file_name,
                                         bool monolang,
                                         size_t bucket_count,
                                         size_t cache_size)
      throw(El::Exception)
        : wp_counter_type_(CT_HASH),
          id_(id),
          monolang_(monolang),
          cache_size_(cache_size),
          stat_(true),
          param_count_(0),
          result_count_(0),
          update_statement_(0),
          select_statement_(0),
          delete_statement_(0)
    {
      memset(param_key_, 0, sizeof(param_key_));
      memset(update_query_param_, 0, sizeof(update_query_param_));
      memset(&select_query_param_, 0, sizeof(select_query_param_));
      memset(&select_query_result_, 0, sizeof(select_query_result_));
      memset(&delete_query_param_, 0, sizeof(delete_query_param_));
      
      std::string filename = std::string(file_name) + "." + id;
        
      if(!strcmp(wp_counter_type, "hash"))
      {
        wp_counter_type_ = CT_HASH;
        filename += ".tch";
      
        ltwp_base_ =
          new El::TokyoCabinet::HashDBM(
            filename.c_str(),
            HDBOWRITER | HDBOCREAT | HDBONOLCK | HDBOTRUNC,
            true,
            bucket_count,
            0,
            -1,
            HDBTLARGE,
            0,
            bucket_count * 8);
      }
      else if(!strcmp(wp_counter_type, "btree"))
      {        
        wp_counter_type_ = CT_BTREE;
        filename += ".tcb";
        
        ltwp_base_ =
          new El::TokyoCabinet::BTreeDBM(
            filename.c_str(),
            HDBOWRITER | HDBOCREAT | HDBONOLCK | HDBOTRUNC,
            true,
            bucket_count,
            -1,
            -1,
            HDBTLARGE,
            0,
            0,
            bucket_count * 8);
      }
      else if(!strcmp(wp_counter_type, "db"))
      {        
        wp_counter_type_ = CT_DB;

        El::MySQL::ConnectionFactory* connection_factory =
          new El::MySQL::NewConnectionsFactory(0, "utf8");

        El::MySQL::DB& app_db = *Application::instance()->dbase();

        El::MySQL::DB_var db = *app_db.unix_socket() == '\0' ?
          new El::MySQL::DB(app_db.user(),
                            app_db.passwd(),
                            app_db.db(),
                            app_db.port(),
                            app_db.host(),
                            app_db.client_flag(),
                            connection_factory) :
          new El::MySQL::DB(app_db.user(),
                            app_db.passwd(),
                            app_db.db(),
                            app_db.unix_socket(),
                            app_db.client_flag(),
                            connection_factory);
        
        connection_ = db->connect();

/*        
        El::MySQL::Result_var result =
          connection_->query(
            "create table if not exists WordPair ( "
            "ltwp BINARY(12) NOT NULL PRIMARY KEY, "
            "count INT NOT NULL ) engine=MyISAM");
*/

         El::MySQL::Result_var result =
           connection_->query("delete from WordPair");

         MYSQL* mysql = connection_->mysql();

         try
         {
           update_statement_ = mysql_stmt_init(mysql);

           if(update_statement_ == 0)
           {
             std::ostringstream ostr;
           
             ostr << "NewsGate::Message::TCWordPairManager::"
               "TCWordPairManager: mysql_stmt_init failed for update "
               "statement. Error code "
                  << std::dec
                  << mysql_errno(mysql) << ", description:\n"
                  << mysql_error(mysql) << std::endl;
           
             throw El::MySQL::Exception(ostr.str());
           }
         
           {
             std::ostringstream query_str;

             query_str << "insert into WordPair set ltwp=?, count=? "
               "on duplicate key update count=count+?";           

             std::string query = query_str.str();
        
             if(mysql_stmt_prepare(update_statement_,
                                   query.c_str(),
                                   query.length()))
             {
               std::ostringstream ostr;
             
               ostr << "NewsGate::Message::TCWordPairManager::"
                 "TCWordPairManager: mysql_stmt_prepare failed for insert "
                 "statement. Error code "
                    << std::dec
                    << mysql_stmt_errno(update_statement_)
                    << ", description:\n"
                    << mysql_stmt_error(update_statement_) << std::endl;
             
               throw El::MySQL::Exception(ostr.str());
             }
           }
         
           size_t i = 0;
         
           {
             MYSQL_BIND& p = update_query_param_[i++];

             p.buffer_type = MYSQL_TYPE_BLOB;
             p.buffer = param_key_;
             p.buffer_length = sizeof(param_key_);
           }

           {
             MYSQL_BIND& p = update_query_param_[i++];

             p.buffer_type = MYSQL_TYPE_LONG;
             p.buffer = &param_count_;
             p.is_unsigned = false;
           }
             
           {
             MYSQL_BIND& p = update_query_param_[i++];

             p.buffer_type = MYSQL_TYPE_LONG;
             p.buffer = &param_count_;
             p.is_unsigned = false;
           }

           assert(i==sizeof(update_query_param_)/
                  sizeof(update_query_param_[0]));

           if(mysql_stmt_bind_param(update_statement_, update_query_param_))
           {      
             std::ostringstream ostr;
           
             ostr << "NewsGate::Message::TCWordPairManager::TCWordPairManager:"
               " mysql_stmt_bind_param failed for insert statement. "
               "Error code "  << std::dec
                  << mysql_stmt_errno(update_statement_) << ", description:\n"
                  << mysql_stmt_error(update_statement_) << std::endl;
           
             throw El::MySQL::Exception(ostr.str());
           }
         
           select_statement_ = mysql_stmt_init(mysql);

           if(select_statement_ == 0)
           {
             std::ostringstream ostr;
           
             ostr << "NewsGate::Message::TCWordPairManager::"
               "TCWordPairManager: mysql_stmt_init failed for select "
               "statement. Error code "
                  << std::dec
                  << mysql_errno(mysql) << ", description:\n"
                  << mysql_error(mysql) << std::endl;
           
             throw El::MySQL::Exception(ostr.str());
           }

           {
             std::ostringstream query_str;

             query_str << "select count from WordPair where ltwp=?";           

             std::string query = query_str.str();
        
             if(mysql_stmt_prepare(select_statement_,
                                   query.c_str(),
                                   query.length()))
             {
               std::ostringstream ostr;
             
               ostr << "NewsGate::Message::TCWordPairManager::"
                 "TCWordPairManager: mysql_stmt_prepare failed for select "
                 "statement. Error code "
                    << std::dec << mysql_stmt_errno(update_statement_)
                    << ", description:\n"
                    << mysql_stmt_error(update_statement_) << std::endl;
             
               throw El::MySQL::Exception(ostr.str());
             }
           }
         
           select_query_param_.buffer_type = MYSQL_TYPE_BLOB;
           select_query_param_.buffer = param_key_;
           select_query_param_.buffer_length = sizeof(param_key_);
         
           if(mysql_stmt_bind_param(select_statement_, &select_query_param_))
           {      
             std::ostringstream ostr;
           
             ostr << "NewsGate::Message::TCWordPairManager::TCWordPairManager:"
               " mysql_stmt_bind_param failed for select statement. "
               "Error code "  << std::dec
                  << mysql_stmt_errno(select_statement_) << ", description:\n"
                  << mysql_stmt_error(select_statement_) << std::endl;
           
             throw El::MySQL::Exception(ostr.str());
           }

           select_query_result_.buffer_type = MYSQL_TYPE_LONG;
           select_query_result_.buffer = &result_count_;
           select_query_result_.is_unsigned = false;
         
           if(mysql_stmt_bind_result(select_statement_, &select_query_result_))
           {
             std::ostringstream ostr;
             
             ostr << "NewsGate::Message::TCWordPairManager::"
               "TCWordPairManager: mysql_stmt_bind_result failed for select "
               "statement. Error code "
                  << std::dec
                  << mysql_stmt_errno(select_statement_) << ", description:\n"
                  << mysql_stmt_error(select_statement_) << std::endl;        
          
             throw El::MySQL::Exception(ostr.str());
           }
           
           delete_statement_ = mysql_stmt_init(mysql);

           if(delete_statement_ == 0)
           {
             std::ostringstream ostr;
           
             ostr << "NewsGate::Message::TCWordPairManager::"
               "TCWordPairManager: mysql_stmt_init failed for delete "
               "statement. Error code "
                  << std::dec
                  << mysql_errno(mysql) << ", description:\n"
                  << mysql_error(mysql) << std::endl;
           
             throw El::MySQL::Exception(ostr.str());
           }

           {
             std::ostringstream query_str;

             query_str << "delete from WordPair where ltwp=?";           

             std::string query = query_str.str();
        
             if(mysql_stmt_prepare(delete_statement_,
                                   query.c_str(),
                                   query.length()))
             {
               std::ostringstream ostr;
             
               ostr << "NewsGate::Message::TCWordPairManager::"
                 "TCWordPairManager: mysql_stmt_prepare failed for delete "
                 "statement. Error code "
                    << std::dec << mysql_stmt_errno(delete_statement_)
                    << ", description:\n"
                    << mysql_stmt_error(delete_statement_) << std::endl;
             
               throw El::MySQL::Exception(ostr.str());
             }
           }
         
           delete_query_param_.buffer_type = MYSQL_TYPE_BLOB;
           delete_query_param_.buffer = param_key_;
           delete_query_param_.buffer_length = sizeof(param_key_);
         
           if(mysql_stmt_bind_param(delete_statement_, &delete_query_param_))
           {      
             std::ostringstream ostr;
           
             ostr << "NewsGate::Message::TCWordPairManager::TCWordPairManager:"
               " mysql_stmt_bind_param failed for select statement. "
               "Error code "  << std::dec
                  << mysql_stmt_errno(delete_statement_) << ", description:\n"
                  << mysql_stmt_error(delete_statement_) << std::endl;
           
             throw El::MySQL::Exception(ostr.str());
           }
         }
         catch(...)
         {
           stmt_clear();
           throw;
         }
      }
      else
      {
        std::ostringstream ostr;
        ostr << "TCWordPairManager::TCWordPairManager: unexpected counter type"
          " " << wp_counter_type;

        throw Exception(ostr.str());
      }

      memset(message_wp_freq_distribution_,
             0,
             sizeof(message_wp_freq_distribution_));
    }
    
    TCWordPairManager::~TCWordPairManager() throw()
    {
      stmt_clear();
    }

    void
    TCWordPairManager::stmt_clear() throw()
    {
      if(update_statement_)
      {
        mysql_stmt_close(update_statement_);
        update_statement_ = 0;
      }
      
      if(select_statement_)
      {
        mysql_stmt_close(select_statement_);
        select_statement_ = 0;
      }

      if(delete_statement_)
      {
        mysql_stmt_close(delete_statement_);
        delete_statement_ = 0;
      }
    }    

    void
    TCWordPairManager::cache_size(size_t val) throw(El::Exception)
    {
      if(cache_size_)
      {
        switch(wp_counter_type_)
        {
        case CT_HASH:
        case CT_DB:
        case CT_BTREE:
          {
            Guard guard(lock_);

            if(!val)
            {
              sync_ltwp_cache(true);
            }

            cache_size_ = val;
            break;
          }
        default: break;
        }
      } 
    }
    
    void
    TCWordPairManager::sync() throw(El::Exception)
    {
      if(cache_size_)
      {
        switch(wp_counter_type_)
        {
        case CT_HASH:
        case CT_DB:
        case CT_BTREE:
          {
            Guard guard(lock_);
            sync_ltwp_cache(true);
            break;
          }
        default: break;
        }
      } 
    }
    
    void
    TCWordPairManager::sync_ltwp_cache(bool full) throw(El::Exception)
    {
      if(ltwp_counter_hashmap_cache_.empty())
      {
        return;
      }
      
      switch(wp_counter_type_)
      {
      case CT_HASH:
        {
          if(full)
          {
/*            
            std::cerr << "SYNC " << id_ << " "
                      << ltwp_counter_hashmap_cache_.size()
                      << " full" << std::endl;
*/
          
            for(LTWPBuffCounterHashMap::const_iterator
                  i(ltwp_counter_hashmap_cache_.begin()),
                  e(ltwp_counter_hashmap_cache_.end()); i != e; ++i)
            {            
              const LTWPBuff& key = i->first;            
              int increment = i->second;
            
              int count =
                ltwp_base_->add_int(
                  key.buff,
                  monolang_ ? LTWPBuff::SIZE_NOLANG : LTWPBuff::SIZE,
                  increment);
            
              if(!count)
              {
                ltwp_base_->erase(
                  key.buff,
                  monolang_ ? LTWPBuff::SIZE_NOLANG : LTWPBuff::SIZE);
              }

              if(stat_)
              {
                inc_wp_freq_distr(count - increment, -1);
                inc_wp_freq_distr(count, 1);
              }
            }

            ltwp_counter_hashmap_cache_.clear();
          }
          else
          {
//            std::cerr << "SYNC " << id_ << " "
//                      << ltwp_counter_hashmap_cache_.size() << " part ";
          
            for(LTWPBuffCounterHashMap::iterator
                  i(ltwp_counter_hashmap_cache_.begin()),
                  e(ltwp_counter_hashmap_cache_.end()); i != e; ++i)
            {            
              int increment = i->second;

              if(increment < 2)
              {
                const LTWPBuff& key = i->first;
            
                int count =
                  ltwp_base_->add_int(
                    key.buff,
                    monolang_ ? LTWPBuff::SIZE_NOLANG : LTWPBuff::SIZE,
                    increment);
              
                if(!count)
                {
                  ltwp_base_->erase(
                    key.buff,
                    monolang_ ? LTWPBuff::SIZE_NOLANG : LTWPBuff::SIZE);
                }

                ltwp_counter_hashmap_cache_.erase(i);

                if(stat_)
                {
                  inc_wp_freq_distr(count - increment, -1);
                  inc_wp_freq_distr(count, 1);
                }
              }
            }

//            std::cerr << ltwp_counter_hashmap_cache_.size() << std::endl;
          
            if(ltwp_counter_hashmap_cache_.size() > cache_size_ / 2)
            {
              sync_ltwp_cache(true);
            }
          }
          
          break;
        }
      case CT_DB:
        {
          if(full)
          {
/*            
            std::cerr << "SYNC " << id_ << " "
                      << ltwp_counter_hashmap_cache_.size()
                      << " full" << std::endl;
*/
          
            for(LTWPBuffCounterHashMap::const_iterator
                  i(ltwp_counter_hashmap_cache_.begin()),
                  e(ltwp_counter_hashmap_cache_.end()); i != e; ++i)
            {            
              const LTWPBuff& key = i->first;            
              int increment = i->second;
            
              int count = db_increment(key, increment);
              
              if(!count)
              {
                db_delete(key);                
              }

              if(stat_)
              {
                inc_wp_freq_distr(count - increment, -1);
                inc_wp_freq_distr(count, 1);
              }
            }

            ltwp_counter_hashmap_cache_.clear();
          }
          else
          {
//            std::cerr << "SYNC " << id_ << " "
//                      << ltwp_counter_hashmap_cache_.size() << " part ";
          
            for(LTWPBuffCounterHashMap::iterator
                  i(ltwp_counter_hashmap_cache_.begin()),
                  e(ltwp_counter_hashmap_cache_.end()); i != e; ++i)
            {            
              int increment = i->second;

              if(increment < 2)
              {
                const LTWPBuff& key = i->first;

                int count = db_increment(key, increment);
              
                if(!count)
                {
                  db_delete(key);                
                }

                ltwp_counter_hashmap_cache_.erase(i);

                if(stat_)
                {
                  inc_wp_freq_distr(count - increment, -1);
                  inc_wp_freq_distr(count, 1);
                }
              }
            }

//            std::cerr << ltwp_counter_hashmap_cache_.size() << std::endl;
          
            if(ltwp_counter_hashmap_cache_.size() > cache_size_ / 2)
            {
              sync_ltwp_cache(true);
            }
          }
          
          break;
        }
        
      case CT_BTREE:
        {        
          for(LTWPBuffCounterMap::const_iterator
                i(ltwp_counter_map_cache_.begin()),
                e(ltwp_counter_map_cache_.end()); i != e; ++i)
          {
            const LTWPBuff& key = i->first;
        
            int increment = i->second;
          
            int count =
              ltwp_base_->add_int(
                key.buff,
                monolang_ ? LTWPBuff::SIZE_NOLANG : LTWPBuff::SIZE,
                increment);

            if(!count)
            {
              ltwp_base_->erase(
                key.buff,
                monolang_ ? LTWPBuff::SIZE_NOLANG : LTWPBuff::SIZE);
            }

            if(stat_)
            {
              inc_wp_freq_distr(count - increment, -1);
              inc_wp_freq_distr(count, 1);
            }
          }

          ltwp_counter_map_cache_.clear();
          break;
        }
      }    
    }

    int
    TCWordPairManager::db_increment(const LTWPBuff& key, int increment)
      throw(El::Exception)
    {
      memcpy(param_key_, key.buff, LTWPBuff::SIZE);
      param_count_ = increment;
      
      if(mysql_stmt_execute(update_statement_))
      {
        std::ostringstream ostr;
        
        ostr << "NewsGate::Message::TCWordPairManager::db_increment: "
          "mysql_stmt_execute failed for update statement. Error code "
             << std::dec
             << mysql_stmt_errno(update_statement_) << ", description:\n"
             << mysql_stmt_error(update_statement_) << std::endl;        
      
        throw El::MySQL::Exception(ostr.str());
      }

      return db_get(key);
    }

    void
    TCWordPairManager::db_delete(const LTWPBuff& key)
      throw(El::Exception)
    {
      memcpy(param_key_, key.buff, LTWPBuff::SIZE);
      
      if(mysql_stmt_execute(delete_statement_))
      {
        std::ostringstream ostr;
        
        ostr << "NewsGate::Message::TCWordPairManager::db_increment: "
          "mysql_stmt_execute failed for update statement. Error code "
             << std::dec
             << mysql_stmt_errno(delete_statement_) << ", description:\n"
             << mysql_stmt_error(delete_statement_) << std::endl;        
      
        throw El::MySQL::Exception(ostr.str());
      }
    }

    int
    TCWordPairManager::db_get(const LTWPBuff& key) throw(El::Exception)
    {
      memcpy(param_key_, key.buff, LTWPBuff::SIZE);
      
      try
      {
        if(mysql_stmt_execute(select_statement_))
        {
          std::ostringstream ostr;
        
          ostr << "NewsGate::Message::TCWordPairManager::db_get: "
            "mysql_stmt_execute failed for select statement. Error code "
               << std::dec
               << mysql_stmt_errno(select_statement_) << ", description:\n"
               << mysql_stmt_error(select_statement_) << std::endl;        
      
          throw El::MySQL::Exception(ostr.str());
        }
/*
        if(mysql_stmt_store_result(select_statement_))
        {
          std::ostringstream ostr;
        
          ostr << "NewsGate::Message::TCWordPairManager::db_get: "
            "mysql_stmt_store_result failed for select statement. Error code "
               << std::dec
               << mysql_stmt_errno(select_statement_) << ", description:\n"
               << mysql_stmt_error(select_statement_) << std::endl;        
      
          throw El::MySQL::Exception(ostr.str());
        }
*/

        int r = mysql_stmt_fetch(select_statement_);
        
        if(r)
        {
          if(r != MYSQL_NO_DATA)
          {
            std::ostringstream ostr;
            
            ostr << "NewsGate::Message::TCWordPairManager::db_get: "
              "mysql_stmt_fetch failed. Error code "  << std::dec
                 << mysql_stmt_errno(select_statement_) << ", description:\n"
                 << mysql_stmt_error(select_statement_) << std::endl;        
            
            throw El::MySQL::Exception(ostr.str());      
          }

          result_count_ = 0;
        }

        mysql_stmt_free_result(select_statement_);
      }
      catch(...)
      {
        mysql_stmt_free_result(select_statement_);
        throw;
      }

      return result_count_;
    }

    void
    TCWordPairManager::wp_increment_counter(const El::Lang& lang,
                                            uint32_t time_index,
                                            const WordPair& wp)
      throw(El::Exception)
    {
      if(cache_size_)
      {
        LTWPBuff key(lang, time_index, wp);
        
        Guard guard(lock_);
        
        switch(wp_counter_type_)
        {
        case CT_HASH:
        case CT_DB:
          {
            ++(ltwp_counter_hashmap_cache_.insert(std::make_pair(key, 0)).
               first->second);
            
            if(ltwp_counter_hashmap_cache_.size() > cache_size_)
            {
              sync_ltwp_cache(false);
            }
            
            break;
          }
        case CT_BTREE:
          {
            ++(ltwp_counter_map_cache_.insert(std::make_pair(key, 0)).first->
               second);
            
            if(ltwp_counter_map_cache_.size() > cache_size_)
            {
              sync_ltwp_cache(false);
            }
            
            break;
          }
        }
      }
      else
      {
        LTWPBuff key(lang, time_index, wp);

        switch(wp_counter_type_)
        {
        case CT_DB:
          {
            Guard guard(lock_);
            
            int count = db_increment(key, 1);
            
            if(stat_)
            {
              inc_wp_freq_distr(count - 1, -1);
              inc_wp_freq_distr(count, 1);
            }

            break;
          }
        default:
          {
            int count =
              ltwp_base_->add_int(
                key.buff,
                monolang_ ? LTWPBuff::SIZE_NOLANG : LTWPBuff::SIZE,
                1);
            
            Guard guard(lock_);
            
            if(stat_)
            {
              inc_wp_freq_distr(count - 1, -1);
              inc_wp_freq_distr(count, 1);
            }
          }
        }
      }
      
    }
    
    void
    TCWordPairManager::wp_decrement_counter(const El::Lang& lang,
                                            uint32_t time_index,
                                            const WordPair& wp)
      throw(El::Exception)
    {
      if(cache_size_)
      {
        LTWPBuff key(lang, time_index, wp);
        
        Guard guard(lock_);
          
        switch(wp_counter_type_)
        {
        case CT_HASH:
        case CT_DB:
          {    
            --(ltwp_counter_hashmap_cache_.insert(std::make_pair(key, 0)).
               first->second);
            
            if(ltwp_counter_hashmap_cache_.size() > cache_size_)
            {
              sync_ltwp_cache(false);
            }
            
            break;
          }
        case CT_BTREE:
          {
            --(ltwp_counter_map_cache_.insert(std::make_pair(key, 0)).first->
               second);
            
            if(ltwp_counter_map_cache_.size() > cache_size_)
            {
              sync_ltwp_cache(false);
            }
            
            break;
          }         
        }
      }
      else
      {
        LTWPBuff key(lang, time_index, wp);
      
        switch(wp_counter_type_)
        {
        case CT_DB:
          {
            Guard guard(lock_);
            
            int count = db_increment(key, -1);

            if(!count)
            {
              db_delete(key);
            }
            
            if(stat_)
            {
              inc_wp_freq_distr(count, 1);
              inc_wp_freq_distr(count + 1, -1);
            }

            break;
          }
        default:
          {
            Guard guard(lock_);
            
            int count =
              ltwp_base_->add_int(
                key.buff,
                monolang_ ? LTWPBuff::SIZE_NOLANG : LTWPBuff::SIZE,
                -1);

            if(!count)
            {
              ltwp_base_->erase(
                key.buff,
                monolang_ ? LTWPBuff::SIZE_NOLANG : LTWPBuff::SIZE);
            }            

            if(stat_)
            {
              inc_wp_freq_distr(count, 1);
              inc_wp_freq_distr(count + 1, -1);
            }
            
            break;
          }
        }
      }
    }

    uint32_t
    TCWordPairManager::wp_get(const El::Lang& lang,
                              uint32_t time_index,
                              const WordPair& wp)
      throw(El::Exception)
    {
      if(cache_size_)
      {
        Guard guard(lock_);
        sync_ltwp_cache(true);
      }

      LTWPBuff key(lang, time_index, wp);      
      
      switch(wp_counter_type_)
      {
      case CT_DB:
        {
          return db_get(key);
        }
      default:
        {
          void* val =
            ltwp_base_->find(
              key.buff,
              monolang_ ? LTWPBuff::SIZE_NOLANG : LTWPBuff::SIZE);
          
          int res = val ? *((int*)val) : 0;
          
          if(val)
          {
            free(val);
          }
      
          return res;
        }
      }
    }
      
    void
    TCWordPairManager::dump_word_pair_freq_distribution(std::ostream& ostr)
      throw(Exception, El::Exception)
    {
      ostr << "\n" << id_ << ": refs -> wp count; ops";
      
      unsigned long total_count = 0;
      unsigned long total_ops = 0;
      
      Guard guard(lock_);

      if(ltwp_base_.in())
      {
        sync_ltwp_cache(true);
      }

      for(size_t i = 0; i < sizeof(message_wp_freq_distribution_) /
            sizeof(message_wp_freq_distribution_[0]); i++)
      {
        unsigned long count = message_wp_freq_distribution_[i];
        total_count += count;
        total_ops += (i + 1) * count;
      }        
        
      for(size_t i = 0; i < sizeof(message_wp_freq_distribution_) /
            sizeof(message_wp_freq_distribution_[0]); i++)
      {
        unsigned long count = message_wp_freq_distribution_[i];
        unsigned long ops = (i + 1) * count;
          
        ostr << std::endl << i + 1 << " -> " << count << " "
             << (total_count ? count * 100 / total_count : 0) << "%; "
             << ops << " " << (total_ops ? ops * 100 / total_ops : 0) << "%";
      }
      
      ostr << std::endl << "total " << total_count << "; ops " << total_ops;
    }    
  }
}
