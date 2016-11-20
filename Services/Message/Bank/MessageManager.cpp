/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Message/Bank/MessageManager.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <memory>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <iomanip>
#include <map>

#include <ext/hash_set>

#include <ace/OS.h>
#include <ace/High_Res_Timer.h>

#include <El/Exception.hpp>
#include <El/Moment.hpp>
#include <El/MySQL/DB.hpp>
#include <El/String/ListParser.hpp>
#include <El/Utility.hpp>
#include <El/CRC.hpp>
#include <El/Guid.hpp>
#include <El/FileSystem.hpp>

#include <Commons/Message/StoredMessage.hpp>
#include <Commons/Message/TransportImpl.hpp>
#include <Commons/Event/TransportImpl.hpp>

#include <Commons/Search/SearchExpression.hpp>
#include <Commons/Search/TransportImpl.hpp>

#include <Services/Dictionary/Commons/DictionaryServices.hpp>
#include <Services/Dictionary/Commons/TransportImpl.hpp>

#include "MessageManager.hpp"
#include "BankMain.hpp"
#include "MessageLoader.hpp"

namespace
{
}
/*
struct ABC
{
  ABC();
};

ABC::ABC()
{
  std::cerr <<
    "#include \"SearchExpression.hpp\"\n\nnamespace NewsGate\n{\n"
    "  namespace Search\n  {\n    const uint32_t Expression::topicality_"
    "[Expression::TOPICALITY_DATE_RANGE] =\n    {";
    
  for(size_t i = 0; i < ::NewsGate::Search::Expression::TOPICALITY_DATE_RANGE;
      ++i)
  {
    if(i % 10 == 0)
    {
      std::cerr << "\n      ";
    }
    
    uint32_t freshness =
      (uint32_t)((double)i /
                 ::NewsGate::Search::Expression::TOPICALITY_DATE_RANGE *
                 10000 + 0.5);
    
    std::cerr << (uint32_t)
      (((unsigned long long)freshness * freshness * freshness /
        100000000) *
       ::NewsGate::Search::Expression::SORT_BY_RELEVANCE_TOPICALITY_WEIGHT);
    
    if(i < ::NewsGate::Search::Expression::TOPICALITY_DATE_RANGE - 1)
    {
      std::cerr << ", ";
    }
  }
  
  std::cerr << "\n    };\n  }\n}\n";
}

static ABC abc;
*/

namespace NewsGate
{
  namespace Message
  {
    //
    // MessageManager class
    //
    MessageManager::MessageManager(
      MessageManagerCallback* callback,
      const Server::Config::BankMessageManagerType& config,
      Message::BankSession* session,
      BankClientSession* bank_client_session)
      throw(El::Exception)
        : BaseClass(callback, "MessageManager"),
          mgr_lock_(config.adapting_mutex().check_period(),
                    config.adapting_mutex().tries(),
                    ACE_Time_Value(0, config.adapting_mutex().wait_time())),
          config_(config),
          session_(session),
          bank_client_session_(bank_client_session),
          wp_counter_type_(config.word_pair_counter().type()),
          wp_intervals_(config.word_pair_counter().interval_groups()),
          messages_(!config.store_duplicate_messages(),
                    false,
                    config.impression_respected_level(),
                    wp_counter_type_.empty() ? 0 : this),
          next_sharing_time_(ACE_Time_Value::zero),
          dict_hash_(0),
          preload_mem_usage_(0),
          capacity_threshold_(config_.message_cache().capacity() *
                              (1.0 - config_.message_cache().room())),
          traverse_message_it_(traverse_message_.end()),
          traverse_prio_message_it_(traverse_prio_message_.end()),
          loaded_(false),
          flushed_(false)
    {
/*
      pthread_rwlockattr_t attr; 
      pthread_rwlockattr_init(&attr); 
      
      pthread_rwlockattr_setkind_np(
        &attr,
        PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
      
      pthread_rwlock_init(const_cast<ACE_rwlock_t*>(&srv_lock_.lock()), &attr);
*/
        
//      ACE_OS::sleep(20);
      
      session->_add_ref();
      bank_client_session->_add_ref();

      if(capacity_threshold_ < config_.message_cache().capacity())
      {
        Search::ExpressionParser parser;
        std::wistringstream istr(L"EVERY");
    
        parser.parse(istr);
        capacity_filter_ = parser.expression();
      }

      if(wp_counter_type_ == "hash" || wp_counter_type_ == "btree" ||
         wp_counter_type_ == "db")
      {
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();

        El::MySQL::Result_var result =
          connection->query("select count(1) as count from MessageDict");

        MessageCount record(result.in());

        if(!record.fetch_row())
        {
          throw Exception(
            "NewsGate::Message::MessageManager::MessageManager:"
            "failed to fetch number of messages");
        }

        int64_t bucket_count =
          (int64_t)((float)record.count() * 175 *
                    config_.word_pair_counter().bucket_factor());

        El::String::ListParser parser(
          config_.word_pair_counter().main_languages().c_str());

        typedef __gnu_cxx::hash_set<El::Lang, El::Hash::Lang> LangSet;
        LangSet languages;
        
        const char* lang = 0;

        while((lang = parser.next_item()) != 0)
        {
          languages.insert(El::Lang(lang));
        }
        
        std::string word_pair_counter_type = config.word_pair_counter().type();
        
        size_t language_count = word_pair_counter_type == "db" ?
          0 : languages.size();
        
        size_t wp_managers = language_count * wp_intervals_ + 1;

        bucket_count = std::max((int64_t)1000000,
                                (int64_t)(bucket_count / wp_managers));

        size_t cache_size = config.word_pair_counter().cache_size();        
        
        word_pair_managers_.reset(new TCWordPairManagerMap());
        
        for(LangSet::const_iterator l(languages.begin()),
              e(languages.end()); l != e; ++l)
        {
          for(size_t j = 0; j < wp_intervals_; ++j)
          {
            LT key(*l, j);
            
            (*word_pair_managers_)[key] = new
              TCWordPairManager(
                word_pair_counter_type.c_str(),
                key.string().c_str(),
                config_.word_pair_counter().filename().c_str(),
                true,
                bucket_count,
                cache_size);            
          }
        }

        LT key(El::Lang::null, 0);
        
        (*word_pair_managers_)[key] = new
          TCWordPairManager(word_pair_counter_type.c_str(),
                            key.string().c_str(),
                            config_.word_pair_counter().filename().c_str(),
                            false,
                            bucket_count,
                            cache_size);
      }
      
      memset(message_wp_freq_distribution_,
             0,
             sizeof(message_wp_freq_distribution_));

      memset(message_word_count_distribution_,
             0,
             sizeof(message_word_count_distribution_));

      memset(message_word_freq_distribution_,
             0,
             sizeof(message_word_freq_distribution_));

      memset(norm_forms_freq_distribution_,
             0,
             sizeof(norm_forms_freq_distribution_));
/*
      message_word_len_stat_.reset(new WordLenStatMap());
      message_unique_word_len_stat_.reset(new WordLenStatMap());
      message_unique_words_.reset(new WordSet());
*/
      
      cache_filename_ = std::string(config.cache_file_dir().c_str()) +
        "/MessageManager.cache";
      
      MessageLoader_var message_loader =
        new MessageLoader(this,
                          config_.message_loader(),
                          config_.message_expiration_time());
      
      state(message_loader.in());
    }
    
    MessageManager::~MessageManager() throw()
    {
    }

    void
    MessageManager::session(Message::BankSession* value)
      throw(Exception, El::Exception)
    {
      value->_add_ref();

      {
        MgrWriteGuard guard(mgr_lock_);
        session_ = value;
      }
    }

    struct MsgPubId
    {
      uint64_t published;
      Id id;

      MsgPubId() throw() : published(0) {}
      MsgPubId(uint64_t p, const Id& i) throw() : published(p), id(i) {}
      
      bool operator<(const MsgPubId& val) const throw();
    };

    inline
    bool
    MsgPubId::operator<(const MsgPubId& val) const throw()
    {
      return published < val.published ||
        (published == val.published && id < val.id);
    }

    typedef std::map<MsgPubId, const StoredMessage*> MsgPubIdMap;

    void
    MessageManager::flush_messages() throw(Exception, El::Exception)
    {
      time_t cur_time = ACE_OS::gettimeofday().sec();

      std::string stat_cache_filename;
      std::string msg_cache_filename;
      bool write_msg = false;
      ACE_Time_Value write_tm;
      MsgPubIdMap ordered_messages;
      
      MgrWriteGuard guard(mgr_lock_);
      
      if(flushed_)
      {
        Application::logger()->trace(
          "NewsGate::Message::MessageManager::flush_messages: already flushed",
          Aspect::MSG_MANAGEMENT,
          El::Logging::HIGH);
        
        return;
      }
      
      Application::logger()->trace(
        "NewsGate::Message::MessageManager::flush_messages: flushing ...",
        Aspect::MSG_MANAGEMENT,
        El::Logging::HIGH);

      try
      {        
        size_t written_messages = 0;
        size_t dirty_messages = 0;
        StoredMessageMap& messages = messages_.messages;
        size_t message_count = messages.size();
        
        std::fstream stat_file;
        
        {
          std::fstream msg_file;
          El::BinaryOutStream bstr(msg_file);

          ACE_High_Res_Timer timer;
          timer.start();

          std::string filename = cache_filename_ + ".msg.tmp";
          
          if(loaded_ && config_.message_cache().use_cache_message_file())
          {
            msg_file.open(filename.c_str(), ios::out);

            write_msg = msg_file.is_open();
        
            if(write_msg)
            {
              msg_cache_filename = filename;
            }
            else
            {              
              std::ostringstream ostr;
              ostr << "NewsGate::Message::MessageManager::flush_messages: "
                "failed to open file '" << filename << "' for write access";
            
              Application::logger()->alert(ostr.str(),
                                           Aspect::MSG_MANAGEMENT);
            }
          }
          else
          {
            unlink(filename.c_str());
          }

          if(write_msg)
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Message::MessageManager::flush_messages: "
              "writing msg "
              "cache " << filename;
            
            Application::logger()->trace(ostr.str(),
                                         Aspect::MSG_MANAGEMENT,
                                         El::Logging::HIGH);        
          }
          else
          {
            Application::logger()->trace(
              "NewsGate::Message::MessageManager::flush_messages: no "
              "msg cache being written",
              Aspect::MSG_MANAGEMENT,
              El::Logging::HIGH);        
          }
            
          for(StoredMessageMap::iterator i(messages.begin()),
                e(messages.end()); i != e; ++i)
          {
            StoredMessage& msg = const_cast<StoredMessage&>(*i->second);

            if(msg.hidden())
            {
              std::ostringstream ostr;
              ostr << "NewsGate::Message::MessageManager::flush_messages: "
                "message " << msg.id.string() << " is unexpectedly hidden";
            
              Application::logger()->emergency(ostr.str(),
                                               Aspect::MSG_MANAGEMENT);

              if(write_msg)
              {
                write_msg = false;

                msg_file.close();
                unlink(msg_cache_filename.c_str());  
                msg_cache_filename.clear();
              }
            }

            if(msg.flags & StoredMessage::MF_DIRTY)
            {
              if(stat_cache_filename.empty())
              {
                stat_cache_filename = cache_filename_ + ".upd." +
                  El::Moment(ACE_Time_Value(cur_time)).dense_format();
            
                stat_file.open(stat_cache_filename.c_str(), ios::out);
            
                if(!stat_file.is_open())
                {
                  std::ostringstream ostr;
                  ostr << "NewsGate::Message::MessageManager::flush_messages: "
                    "failed to open file '" << stat_cache_filename
                       << "' for write access";
              
                  throw Exception(ostr.str());
                }
              }
          
              flush_msg_stat(stat_file, msg, dirty_messages++);
            }

            if(write_msg)
            {
              ordered_messages.insert(
                std::make_pair(MsgPubId(msg.published, msg.id), &msg));
            }
          }

          if(write_msg)
          {
            Application::logger()->trace(
              "NewsGate::Message::MessageManager::flush_messages: writing "
              "msg cache ...",
              Aspect::MSG_MANAGEMENT,
              El::Logging::HIGH);        
            
//            bstr << (uint32_t)0;
            bstr << dict_hash_;
            
            for(MsgPubIdMap::const_iterator i(ordered_messages.begin()),
                  e(ordered_messages.end()); i != e; ++i)
            {
              const StoredMessage& msg = *i->second;
              
              bstr << msg.id
                   << (uint8_t)(msg.flags & StoredMessage::MF_PERSISTENT_FLAGS)
                   << msg.signature << msg.url_signature
                   << msg.source_id << msg.published 
                   << msg.fetched << msg.space << msg.lang << msg.country
                   << msg.event_id << msg.event_capacity << msg.impressions
                   << msg.clicks << msg.visited << msg.categories;

              bstr.write_string_buff(msg.source_title.c_str());
              msg.write_broken_down(bstr);

              ++written_messages;

//              std::cerr << msg.published << std::endl;
            }
            
            if(!msg_file.fail())
            {
              msg_file.flush();
            }
            
            if(msg_file.fail())
            {
              std::ostringstream ostr;
              ostr << "NewsGate::Message::MessageManager::flush_messages: "
                "failed to write into file '" << msg_cache_filename << "'";
              
              Application::logger()->alert(ostr.str(),
                                           Aspect::MSG_MANAGEMENT);
              
              msg_file.close();
              unlink(msg_cache_filename.c_str());  
              msg_cache_filename.clear();
            }
          }

          timer.stop();
          timer.elapsed_time(write_tm);          
        }

        flushed_ = true;
        guard.release();

        Application::logger()->trace(
          "NewsGate::Message::MessageManager::flush_messages: finalizing ...",
          Aspect::MSG_MANAGEMENT,
          El::Logging::HIGH);
        
        if(!msg_cache_filename.empty())
        {            
          std::string filename = cache_filename_ + ".msg";

          {
            std::ostringstream ostr;
            ostr << "NewsGate::Message::MessageManager::flush_messages: "
              "renaming '" << msg_cache_filename << "' to '" << filename << "'";
            
            Application::logger()->trace(ostr.str(),
                                         Aspect::MSG_MANAGEMENT,
                                         El::Logging::HIGH);
          }
          
          if(rename(msg_cache_filename.c_str(), filename.c_str()) < 0)
          {
            int error = ACE_OS::last_error();

            std::ostringstream ostr;
            
            ostr <<" NewsGate::Message::MessageManager::flush_messages: "
              "rename '" << msg_cache_filename << "' to '" << filename
                 << "' failed. Errno " << error << ". Description:\n"
                 << ACE_OS::strerror(error);
            
            Application::logger()->alert(ostr.str(),
                                         Aspect::MSG_MANAGEMENT);
          }
          else
          {
            msg_cache_filename.clear();
          }
        }

        ACE_Time_Value update_tm;
          
        if(stat_file.is_open())
        {
          Application::logger()->trace(
            "NewsGate::Message::MessageManager::flush_messages: "
            "uploading stat ...",
            Aspect::MSG_MANAGEMENT,
            El::Logging::HIGH);
          
          if(!stat_file.fail())
          {
            stat_file.flush();
          }
            
          if(stat_file.fail())
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Message::MessageManager::flush_messages: "
              "failed to write into file '" << stat_cache_filename << "'";
            
            throw Exception(ostr.str());
          }
          
          stat_file.close();

          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          upload_flushed_msg_stat(connection.in(),
                                  stat_cache_filename.c_str(),
                                  update_tm);          
        }

        {
          MgrReadGuard guard(mgr_lock_);

          if(message_count != messages.size())
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Message::MessageManager::flush_messages: "
              "message number " << messages.size()
                 << "is different from expected (" << message_count << ")";

            if(write_msg)
            {
              std::string filename = cache_filename_ + ".msg";
              ostr << "; removing " << filename;

              unlink(filename.c_str());
            }
            
            Application::logger()->emergency(ostr.str(),
                                             Aspect::MSG_MANAGEMENT);
          }
        }        
        
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::flush_messages: of "
             << message_count << " messages\n"
             << written_messages << " written to cache, "
             << dirty_messages << " updates flushed;\ntime: "
             << "save " << El::Moment::time(write_tm)
             << ", update " << El::Moment::time(update_tm)
             << std::endl;

        Application::logger()->trace(ostr.str(),
                                     Aspect::MSG_MANAGEMENT,
                                     El::Logging::HIGH);        
      }
      catch(const El::Exception& e)
      {
        if(!stat_cache_filename.empty())
        {
          unlink(stat_cache_filename.c_str());
        }

        if(!msg_cache_filename.empty())
        {
          unlink(msg_cache_filename.c_str());
        }        
        
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::flush_messages: "
          "El::Exception caught. Description:\n" << e;
        
        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
      }
    }
    
    void
    MessageManager::wait() throw(Exception, El::Exception)
    {
      BaseClass::wait();
      
      MgrWriteGuard guard(mgr_lock_);

      Application::logger()->trace(
        "NewsGate::Message::MessageManager::wait: saving info ...",
        Aspect::MSG_MANAGEMENT,
        El::Logging::HIGH);
      
      save_msg_time_map(del_msg_notifications_, ".del");
      save_msg_time_map(recent_msg_deletions_, ".rdel");

      if(message_categorizer_.in() != 0)
      {
        std::string cache_filename = cache_filename_ + ".czr";

        save_message_categorizer(*message_categorizer_,
                                 cache_filename.c_str());
        
        message_categorizer_ = 0;
      }

      if(message_filters_.in() != 0)
      {
        std::string cache_filename = cache_filename_ + ".ffr";
        save_message_filters(*message_filters_, cache_filename.c_str());
        message_filters_ = 0;
      }

      Application::logger()->trace(
        "NewsGate::Message::MessageManager::wait: done",
        Aspect::MSG_MANAGEMENT,
        El::Logging::HIGH);
    }
    
    void
    MessageManager::load_msg_time_map(IdTimeMap& msg_map, const char* ext)
      throw(Exception, El::Exception)
    {
      std::string cache_filename = cache_filename_ + ext;
      std::fstream file(cache_filename.c_str(), ios::in);
        
      if(file.is_open())
      {
        El::BinaryInStream bstr(file);
        
        try
        {
          bstr.read_map(msg_map);
        }
        catch(const El::Exception&)
        {
          msg_map.clear();
        }

        file.close();
        unlink(cache_filename.c_str());  
      }
    }
    
    void
    MessageManager::save_msg_time_map(IdTimeMap& msg_map, const char* ext)
      throw(Exception, El::Exception)
    {
      if(!msg_map.empty())
      {
        std::string cache_filename = cache_filename_ + ext;
        std::fstream file(cache_filename.c_str(), ios::out);
        
        if(file.is_open())
        {
          El::BinaryOutStream bstr(file);
          bstr.write_map(msg_map);
        }
        else
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::save_msg_time_map: "
            "failed to open file '" << cache_filename << "' for write access";

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        
        msg_map.clear();
      }
    }
    
    void
    MessageManager::set_mirrored_manager(const char* mirrored_manager,
                                         const char* sharing_id)
      throw(El::Exception)
    {
      El::Service::CompoundServiceMessage_var msg =
        new SetMirroredManager(this, mirrored_manager, sharing_id);
        
      deliver_now(msg.in());
    }

    void
    MessageManager::set_mirrored_banks(const char* mirrored_manager,
                                       const char* sharing_id)
      throw(El::Exception)
    {
      {
        MgrReadGuard guard(mgr_lock_);
        
        if(mirrored_manager_ == mirrored_manager &&
           mirrored_sharing_id_ == sharing_id)
        {
          return;
        }
      }

      if(*mirrored_manager == '\0' || *sharing_id == '\0')
      {
        MgrWriteGuard guard(mgr_lock_);
        
        mirrored_banks_ = 0;
        mirrored_manager_.clear();
        mirrored_sharing_id_.clear();
        
        return;
      }

      try
      {
        CORBA::Object_var obj =
          Application::instance()->orb()->string_to_object(mirrored_manager);
      
        if(CORBA::is_nil(obj.in()))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::set_mirrored_banks: "
            "string_to_object() gives nil reference. Ior: "
               << mirrored_manager;

          throw Exception(ostr.str().c_str());
        }

        Message::BankManager_var bank_manager =
          NewsGate::Message::BankManager::_narrow(obj.in ());
        
        if(CORBA::is_nil(bank_manager.in()))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::set_mirrored_banks: "
            "Message::BankManager::_narrow gives nil reference. Ior: "
               << mirrored_manager;
    
          throw Exception(ostr.str().c_str());
        }
      
        BankClientSession_var mirrored_banks =
          bank_manager->bank_client_session();

        {
          MgrWriteGuard guard(mgr_lock_);
        
          if(mirrored_manager_ != mirrored_manager ||
             mirrored_sharing_id_ != sharing_id)
          {
            mirrored_banks_ = mirrored_banks._retn();
            mirrored_manager_ = mirrored_manager;
            mirrored_sharing_id_ = sharing_id;
          }
        }
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::set_mirrored_banks: "
          "CORBA::Exception caught. Description: \n" << e;

        El::Service::Error error(ostr.str().c_str(), this);
        callback_->notify(&error);
      }
      catch (const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::set_mirrored_banks: "
          "El::Exception caught. Description: \n" << e;
        
        El::Service::Error error(ostr.str().c_str(), this);
        callback_->notify(&error);
      }
    }

    MessageManager::MessageFetchFilterMap*
    MessageManager::load_message_filters(const char* filename)
      throw(Exception, El::Exception)
    {
      std::fstream file(filename, ios::in);
        
      if(!file.is_open())
      {
        return 0;
      }
      
      El::BinaryInStream bstr(file);
      
      MessageFetchFilterMap_var filters = new MessageFetchFilterMap();
          
      try
      {
        bstr >> *filters;
      }
      catch(const El::Exception& e)
      {
        filters = 0;

        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::load_message_filters: "
          "El::Exception caught. Description:\n" << e;

        El::Service::Error error(ostr.str().c_str(), this);
        callback_->notify(&error);
      }

      file.close();
      unlink(filename);

      return filters.retn();      
    }
    
    MessageCategorizer*
    MessageManager::load_message_categorizer(const char* filename)
      throw(Exception, El::Exception)
    {
      std::fstream file(filename, ios::in);
        
      if(!file.is_open())
      {
        return 0;
      }
      
      El::BinaryInStream bstr(file);
      
      MessageCategorizer_var categorizer =
        new MessageCategorizer(config_.message_categorizer());
          
      try
      {
        bstr >> *categorizer;
        
        // To make it replaceable by any categorizer being assigned
        categorizer->stamp = 0;
      }
      catch(const El::Exception& e)
      {
        categorizer = 0;

        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::load_message_categorizer: "
          "El::Exception caught. Description:\n" << e;

        El::Service::Error error(ostr.str().c_str(), this);
        callback_->notify(&error);
      }

      file.close();
      unlink(filename);

      return categorizer.retn();
    }
    
    bool
    MessageManager::save_message_filters(
      const MessageFetchFilterMap& message_filters,
      const char* filename)
      throw(Exception, El::Exception)
    {
      std::fstream file(filename, ios::out);
        
      if(file.is_open())
      {
        El::BinaryOutStream bstr(file);
        bstr << message_filters;

        if(!file.fail())
        {
          file.flush();
        }
            
        if(file.fail())
        {
          file.close();
          unlink(filename);
          
          std::ostringstream ostr;
          ostr
            << "NewsGate::Message::MessageManager::save_message_filters: "
            "failed to write to file '" << filename << "'";
          
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        else
        {
          return true;
        }
      }
      else
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::save_message_filters: "
          "failed to open file '" << filename << "' for write access";

        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
      }

      return false;
    }
    
    bool
    MessageManager::save_message_categorizer(
      const MessageCategorizer& categorizer,
      const char* filename)
      throw(Exception, El::Exception)
    {
      std::fstream file(filename, ios::out);
        
      if(file.is_open())
      {
        El::BinaryOutStream bstr(file);
        bstr << categorizer;

        if(!file.fail())
        {
          file.flush();
        }
            
        if(file.fail())
        {
          file.close();
          unlink(filename);
          
          std::ostringstream ostr;
          ostr
            << "NewsGate::Message::MessageManager::save_message_categorizer: "
            "failed to write to file '" << filename << "'";
          
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        else
        {
          return true;
        }
      }
      else
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::save_message_categorizer: "
          "failed to open file '" << filename << "' for write access";

        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
      }

      return false;
    }
    
    bool
    MessageManager::start() throw(Exception, El::Exception)
    {      
      if(!BaseClass::start())
      {
        return false;
      }

      bool loaded = false;
      
      {
        MgrReadGuard guard(mgr_lock_);
        loaded = loaded_;
      }
      
      {
        MgrWriteGuard guard(mgr_lock_);     
        load_msg_time_map(del_msg_notifications_, ".del");
        load_msg_time_map(recent_msg_deletions_, ".rdel");
      }
      
      El::Service::CompoundServiceMessage_var msg =
        new MsgDeleteNotification(this);
      
      deliver_now(msg.in());
      
      {
        std::string cache_filename = cache_filename_ + ".czr";

        MgrWriteGuard guard(mgr_lock_);
            
        message_categorizer_ =
          load_message_categorizer(cache_filename.c_str());        
      }

      {
        std::string cache_filename = cache_filename_ + ".ffr";

        MgrWriteGuard guard(mgr_lock_);
            
        message_filters_ =
          load_message_filters(cache_filename.c_str());        
      }

      if(loaded)
      {
        schedule_traverse();
      }
      else
      {
        schedule_load();
      }      

      return true;
    }

    void
    MessageManager::update_events(
      const Transport::MessageEventArray& message_events)
      throw(Exception, El::Exception)
    {
      if(message_events.empty())
      {
        return;
      }

      size_t change_count = 0;
      size_t event_id_changes = 0;
      ACE_Time_Value tm;
      
      {
        MgrWriteGuard guard(mgr_lock_);
        
        if(flushed_)
        {
          return;
        }      

        ACE_High_Res_Timer timer;
        timer.start();

        for(Transport::MessageEventArray::const_iterator
              i(message_events.begin()), e(message_events.end()); i != e; ++i)
        {
          const Transport::MessageEvent& me = *i;
          const El::Luid& event_id = me.event_id;

          StoredMessage* msg = messages_.find(me.id);

          if(msg && msg->visible() &&
             (msg->event_id != event_id ||
              msg->event_capacity != me.event_capacity))
          {
            if(msg->event_id != event_id)
            {
              messages_.set_event_id(*msg, event_id);
              event_id_changes++;
            }
              
            msg->event_capacity = me.event_capacity;
            messages_.calc_search_weight(*msg);
            
            msg->flags |= StoredMessage::MF_DIRTY;
            
            change_count++;
          }
        }

        timer.stop();
        timer.elapsed_time(tm);
      }
      
      if(Application::will_trace(El::Logging::HIGH))
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::update_events: "
             << message_events.size() << "/" << change_count
             << "/" << event_id_changes
             << " updates; time: " << El::Moment::time(tm);
            
        Application::logger()->trace(ostr.str(),
                                     Aspect::MSG_MANAGEMENT,
                                     El::Logging::HIGH);
      }        
    }
    
    void
    MessageManager::insert(const StoredMessageList& messages,
                           ::NewsGate::Message::PostMessageReason reason)
      throw(WordManagerNotReady, Exception, El::Exception)
    {
      if(messages.empty())
      {
        return;
      }

      unsigned long sharing_flags = 0;
      StoredMessageList share_msg;
      
      Event::BankClientSession_var event_bank_client_session =
        Application::instance()->event_bank_client_session();
      
      bool push_to_event_bank = event_bank_client_session.in() != 0;
      
      ::NewsGate::Message::PostMessageReason sharing_reason =
          PMR_SHARED_MESSAGE_SHARING;

      bool colo_local_msg = false;
      bool mirror = false;
      bool replace = false;
      
      {
        MgrReadGuard guard(mgr_lock_);
        mirror = session_->mirror();
        replace = mirror && reason == PMR_OLD_MESSAGE_SHARING;        
      }

      switch(reason)
      {
      case PMR_NEW_MESSAGES:
        {
          if(mirror)
          {
            El::Service::Error notice(
              "NewsGate::Message::MessageManager::insert: "
              "can't accept raw messages in mirror mode",
              this,
              El::Service::Error::NOTICE);
            
            callback_->notify(&notice);
            return;
          }
          
          sharing_flags = MSRT_NEW_MESSAGES;
          sharing_reason = PMR_NEW_MESSAGE_SHARING;
          
          break;
        }
      case PMR_PUSH_OUT_FOREIGN_MESSAGES:
        {
          // Do not share "pushed out foreign messages" - this is
          // just redistributing messages inside a cluster

          push_to_event_bank = false;
          colo_local_msg = true;
          
          break;
        }
      case PMR_NEW_MESSAGE_SHARING:
      case PMR_SHARED_MESSAGE_SHARING:
      case PMR_OLD_MESSAGE_SHARING:
        {
          sharing_flags = MSRT_SHARED_MESSAGES;
          sharing_reason = PMR_SHARED_MESSAGE_SHARING;
            
          break;
        }
      case PMR_LOST_MESSAGE_SHARING:
        {
          // Do not share "lost messages" - this is just a restoration
          // from a mirror cluster
          break;
        }
      default:
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::insert: "
            "unexpected reason " << reason;
            
          El::Service::Error error(ostr.str().c_str(), this);
          callback_->notify(&error);
          
          break;
        }
      }

      MessageSinkMap message_sink;

      if(sharing_flags)
      {
        message_sink = callback_->message_sink_map(sharing_flags);
      }

      Event::Transport::MessageDigestPackImpl::Var message_digest_pack =
        new Event::Transport::MessageDigestPackImpl::Type(
          new Event::Transport::MessageDigestArray());
      
      Event::Transport::MessageDigestArray& message_digests =
        message_digest_pack->entities();

      bool dict_not_changed =
        insert(messages,
               replace,
               colo_local_msg,
               mirror,
               push_to_event_bank ? &message_digests :
                 (Event::Transport::MessageDigestArray*)0,
               message_sink.empty() ? (StoredMessageList*)0 : &share_msg);

      if(!message_digests.empty())
      {
        post_digests(event_bank_client_session.in(), message_digest_pack.in());
      }
      
      if(!share_msg.empty())
      {
        share_all_messages(share_msg, message_sink, sharing_reason);
      }

      if(!dict_not_changed)
      {
        callback_->dictionary_hash_changed();
      }
    }

    void
    MessageManager::process_message_events(
      Transport::MessageEventPackImpl::Type* pack,
      ::NewsGate::Message::PostMessageReason reason)
      throw(Exception, El::Exception)
    {
      Transport::MessageEventArray& entities = pack->entities();
      update_events(entities);
      
      unsigned long flags = 0;

      ::NewsGate::Message::PostMessageReason sharing_reason =
          PMR_OWN_EVENT_SHARING;
      
      switch(reason)
      {
      case PMR_EVENT_UPDATE:
        {
          flags = MSRT_OWN_EVENTS;
          sharing_reason = PMR_OWN_EVENT_SHARING;
          break;
        }
      case PMR_OWN_EVENT_SHARING:
      case PMR_SHARED_EVENT_SHARING:
        {
          flags = MSRT_SHARED_EVENTS;
          sharing_reason = PMR_SHARED_EVENT_SHARING;
          break;
        }
      default:
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::process_message_events: "
            "unexpected reason " << reason;
          
          El::Service::Error error(ostr.str().c_str(), this);
          callback_->notify(&error);
          break;
        }
      }

      MessageSinkMap message_sink;
      std::string sharing_id;

      if(flags)
      {
        message_sink = callback_->message_sink_map(flags);

        MgrReadGuard guard(mgr_lock_);
        sharing_id = session_->sharing_id();
      }

      for(MessageSinkMap::iterator it = message_sink.begin();
          it != message_sink.end(); it++)
      {
        MessageSink& sink = it->second;
            
        try
        {
          sink.message_sink->post_messages(
            pack,
            sharing_reason,
            BankClientSession::PS_PUSH_BEST_EFFORT,
            sharing_id.c_str());
        }
        catch(const BankClientSession::FailedToPostMessages& e)
        {
          if(Application::will_trace(El::Logging::HIGH))
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Message::MessageManager::"
              "process_message_events: FailedToPostMessages caught. Reason:\n"
                 << e.description.in();

            Application::logger()->trace(ostr.str(),
                                         Aspect::MSG_SHARING,
                                         El::Logging::HIGH);
          }
        }
        catch(const NotReady& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::process_message_events: "
            "NotReady caught. Description:" << std::endl
               << e.reason.in();
        
          Application::logger()->trace(ostr.str(),
                                       Aspect::MSG_SHARING,
                                       El::Logging::HIGH);
        }
        catch(const ImplementationException& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::process_message_events: "
            "ImplementationException caught. Description:" << std::endl
               << e.description.in();
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(const CORBA::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::process_message_events: "
            "CORBA::Exception caught. Description:" << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
      }
    }

    void
    MessageManager::share_all_messages(const StoredMessageList& share_msg,
                                       MessageSinkMap& message_sink,
                                       PostMessageReason sharing_reason)
      throw(Exception, El::Exception)
    {
      if(message_sink.empty())
      {
        return;
      }
      
      Transport::StoredMessagePackImpl::Var message_pack =
        Transport::StoredMessagePackImpl::Init::create(
          new Transport::StoredMessagePackImpl::MessageArray());

      Transport::StoredMessagePackImpl::MessageArray& messages =
        message_pack->entities();

      messages.reserve(share_msg.size());

      std::string sharing_id;
      
      {
        MgrReadGuard guard(mgr_lock_);

        sharing_id = session_->sharing_id();        
      
        for(StoredMessageList::const_iterator mit = share_msg.begin();
            mit != share_msg.end(); mit++)
        {
          Message::Transport::StoredMessageDebug msg;

          msg.message = *mit;

          const StoredMessage* cur_msg = messages_.find(msg.message.id);

          if(cur_msg && cur_msg->visible())
          {
            msg.message.event_id = cur_msg->event_id;
            msg.message.event_capacity = cur_msg->event_capacity;
            msg.message.impressions = cur_msg->impressions;
            msg.message.clicks = cur_msg->clicks;
            msg.message.visited = cur_msg->visited;
            msg.message.categories = cur_msg->categories;
          }
        
          messages.push_back(msg);
        }
      }
      
      load_img_thumb("share_all_messages", messages);
      
      for(MessageSinkMap::iterator it = message_sink.begin();
          it != message_sink.end(); it++)
      {
        MessageSink& sink = it->second;
        try
        {
          sink.message_sink->post_messages(
            message_pack.in(),
            sharing_reason,
            BankClientSession::PS_PUSH_BEST_EFFORT,
            sharing_id.c_str());
        }
        catch(const BankClientSession::FailedToPostMessages& e)
        {
          if(Application::will_trace(El::Logging::HIGH))
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Message::MessageManager::share_all_messages: "
              "FailedToPostMessages caught. Reason:\n"
                 << e.description.in();

            Application::logger()->trace(ostr.str(),
                                         Aspect::MSG_SHARING,
                                         El::Logging::HIGH);
          }
        }
        catch(const NotReady& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::share_all_messages: "
            "NotReady caught. Description:" << std::endl
               << e.reason.in();
        
          Application::logger()->trace(ostr.str(),
                                       Aspect::MSG_SHARING,
                                       El::Logging::HIGH);
        }
        catch(const ImplementationException& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::share_all_messages: "
            "ImplementationException caught. Description:" << std::endl
               << e.description.in();
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(const CORBA::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::share_all_messages: "
            "CORBA::Exception caught. Description:" << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
      }
    }

    void
    MessageManager::post_digests(
      Event::BankClientSession* event_bank_client_session,
      Event::Transport::MessageDigestPackImpl::Type* message_digest_pack)
      throw(Exception, El::Exception)
    {
      try
      {
        Transport::MessageEventPack_var message_events;
            
        Event::BankClientSession::RequestResult_var res =
          event_bank_client_session->post_message_digest(
            message_digest_pack,
            message_events.out());

        if(res->code == Event::BankClientSession::RRC_OK)
        {
          Transport::MessageEventPackImpl::Type* msg_events =
            dynamic_cast<Transport::MessageEventPackImpl::Type*>(
              message_events.in());

          if(msg_events == 0)
          {
            throw Exception(
              "NewsGate::Message::MessageManager::post_digests: "
              "dynamic_cast<Transport::MessageEventPackImpl::"
              "Type*> failed");            
          }

          update_events(msg_events->entities());
        }
        else
        {
          std::ostringstream ostr;
          ostr << "MessageManager::post_digests: "
            "Event::BankClientSession::post_message_digest failed. "
            "Code: " << res->code << ". Description:\n"
               << res->description.in();
          
          El::Service::Error
            error(ostr.str(), this, El::Service::Error::NOTICE);
          
          callback_->notify(&error);        
        }
      }
      catch(const Event::NotReady& e)
      {
        std::ostringstream ostr;
        ostr << "MessageManager::post_digests: event bank not ready. "
          "Reason:\n" << e.reason.in();
          
        El::Service::Error
          error(ostr.str(), this, El::Service::Error::NOTICE);
          
        callback_->notify(&error);        
      }
      catch(const Event::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "MessageManager::post_digests: "
          "Event::ImplementationException "
          "thrown by Event::Bank::post_message_digest. Description:\n"
             << e.description.in();
          
        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "MessageManager::post_digests: CORBA::Exception "
          "thrown by Event::Bank::post_message_digest. Description:\n" << e;
          
        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::post_digests: "
          "El::Exception caught. Description:" << std::endl << e;
        
        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
      }
    }
    
    bool
    MessageManager::normalize_words(StoredMessageList& messages,
                                    uint32_t dict_hash,
                                    IdSet* changed_norm_forms)
      throw(WordManagerNotReady, Exception, El::Exception)
    {
      bool dict_not_changed = true;
      
      try
      {
        Dictionary::Transport::MessageWordsPackImpl::Var words_pack =
          new Dictionary::Transport::MessageWordsPackImpl::Type(
            new Dictionary::Transport::MessageWordsArray());
        
        Dictionary::Transport::MessageWordsArray& words =
          words_pack->entities();

        words.reserve(messages.size());

        for(StoredMessageList::const_iterator it = messages.begin();
            it != messages.end(); it++)
        {
          const StoredMessage& msg = *it;

          if((msg.content.in() != 0 && msg.content->dict_hash == dict_hash) ||
             msg.hidden())
          {
            continue;
          }
          
          words.push_back(Dictionary::Transport::MessageWords());
            
          Dictionary::Transport::MessageWords& msg_words = *words.rbegin();
          
          msg_words.word_positions = msg.word_positions;
          msg_words.positions = msg.positions;
          msg_words.language = msg.lang;
        }

        if(!words.empty())
        {
          Dictionary::WordManager_var word_manager =
            Application::instance()->word_manager();
          
          Dictionary::Transport::NormalizedMessageWordsPack_var result;
          
          if(word_manager->normalize_message_words(words_pack, result.out()) !=
             dict_hash)
          {
            dict_not_changed = false;
          }
        
          Dictionary::Transport::NormalizedMessageWordsPackImpl::Type* impl =
            dynamic_cast<
            Dictionary::Transport::NormalizedMessageWordsPackImpl::Type*>(
              result.in());
        
          if(impl == 0)
          {
            throw Exception(
              "MessageManager::normalize_words: "
              "dynamic_cast<Dictionary::Transport::"
              "NormalizedMessageWordsPackImpl::Type*> failed");
          }

          Dictionary::Transport::NormalizedMessageWordsArray& norm_words =
            impl->entities();
          
          size_t i = 0;
          
          for(StoredMessageList::iterator it = messages.begin();
              it != messages.end(); it++)
          {
            StoredMessage& msg = *it;

            if((msg.content.in() != 0 && msg.content->dict_hash == dict_hash) ||
               msg.hidden())
            {
              continue;
            }

//            uint64_t word_hash = changed_norm_forms ? msg.word_hash() : 0;

            StoredMessage new_msg = msg;
              
            const Dictionary::Transport::NormalizedMessageWords& nw =
              norm_words[i];
            
            new_msg.norm_form_positions = nw.norm_form_positions;
            new_msg.positions = nw.resulted_positions;
            new_msg.lang = nw.lang;

            MessageWordPosition& word_positions = new_msg.word_positions;
            assert(word_positions.size() == nw.word_infos.size());

//            bool dump = msg.id.data == 0x0B8C97D35AB4D58ALL;
//            std::cerr << "Got Pack ********************\n";
            
            for(size_t j = 0; j < word_positions.size(); j++)
            {
              word_positions[j].second.lang = nw.word_infos[j].lang;
/*
              if(dump && strcmp(word_positions[j].first.c_str(), "of") == 0)
              {
                std::cerr << "BBB: " << word_positions[j].first.c_str() << " "
                          << word_positions[j].second.lang.l3_code()
                          << "\n";
              }
*/            
            }

            if((msg.content.in() != 0 && msg.content->dict_hash == 0) ||
               msg.word_hash(false) != new_msg.word_hash(false))
            {
              msg.steal(new_msg);
              
              if(changed_norm_forms)
              {
                changed_norm_forms->insert(msg.id);
              }
            }
/*
            std::cerr << "ID " << msg.id.string() << "\npositions:\n";
            
            for(unsigned long j = 0; j < word_positions.size(); j++)
            {
              const MessageWordPosition::KeyValue& kv = word_positions[j];
              const WordPositions& wp = kv.second;
              
              std::cerr << "  " << kv.first << "/" << wp.lang.l3_code()
                        << " :";


              for(unsigned long k = 0; k < wp.position_count(); k++)
              {
                std::cerr << " " << wp.position(msg.positions, k);
              }
              
              std::cerr << std::endl;
            }

            const NormFormPosition& norm_form_positions =
              msg.norm_form_positions;

            std::cerr << "norm positions:\n";
            
            for(unsigned long j = 0; j < norm_form_positions.size(); j++)
            {
              const NormFormPosition::KeyValue& kv = norm_form_positions[j];
              const WordPositions& wp = kv.second;
              
              std::cerr << "  " << kv.first << "/" << wp.lang.l3_code()
                        << " :";

              for(unsigned long k = 0; k < wp.position_count(); k++)
              {
                std::cerr << " " << wp.position(msg.positions, k);
              }
              
              std::cerr << std::endl;
            }
*/          
            if(msg.content.in() != 0)
            {
              msg.content->dict_hash = dict_hash;
            }

            i++;
          }
          
        }
      }
      catch(const Dictionary::NotReady& e)
      {
        std::ostringstream ostr;
        ostr << "MessageManager::normalize_words: word manager "
          "not ready. Reason:\n" << e.reason.in();

        throw WordManagerNotReady(ostr.str());
      }
      catch(const Dictionary::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "MessageManager::normalize_words: "
          "Dictionary::WordManager::ImplementationException caught. "
          "Description:\n" << e.description.in();
          
        throw Exception(ostr.str());
      }      
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "MessageManager::normalize_words: word manager "
          "not ready. Reason: CORBA::Exception caught. "
          "Description:\n" << e;
          
        throw WordManagerNotReady(ostr.str());
      }

      return dict_not_changed;
    }

    bool
    MessageManager::insert(
      const StoredMessageList& message_list,
      bool replace,
      bool colo_local_msg,
      bool mirror,
      Event::Transport::MessageDigestArray* message_digests,
      StoredMessageList* share_messages)
      throw(WordManagerNotReady, Exception, El::Exception)
    {
      bool insert_as_is = mirror || colo_local_msg;
      
      StoredMessageList messages = message_list;
      size_t message_count = messages.size();

      bool dict_not_changed = insert_as_is ||
        normalize_words(messages, dictionary_hash());

      IdTimeMap inserted_messages;
      inserted_messages.resize(message_count);

//      StoredMessagePtrArray inserted_message_ptr;
//      inserted_message_ptr.reserve(message_count);

      IdTimeMap removed_msg;

      MessageFetchFilterMap_var filters;
      MessageCategorizer_var categorizer;
      
      if(!insert_as_is)
      {
        filters = get_message_fetch_filters();
        categorizer = get_message_categorizer();
      }
      
      std::auto_ptr<std::ostringstream> log_ostr;

      std::string suffix = El::Moment(ACE_OS::gettimeofday()).dense_format();
      std::string ins_filename = cache_filename_ + ".ins." + suffix;
      std::string cat_filename = cache_filename_ + ".cat." + suffix;
      std::string dic_filename = cache_filename_ + ".dic." + suffix;
        
      try
      {
        std::fstream ins_file(ins_filename.c_str(), ios::out);
        
        if(!ins_file.is_open())
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::insert: "
            "failed to open file '" << ins_filename << "' for write access";
        
          throw Exception(ostr.str());
        }
        
        std::fstream cat_file(cat_filename.c_str(), ios::out);
        
        if(!cat_file.is_open())
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::insert: "
            "failed to open file '" << cat_filename << "' for write access";
        
          throw Exception(ostr.str());
        }
        
        std::fstream dic_file(dic_filename.c_str(), ios::out);
        
        if(!dic_file.is_open())
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::insert: "
            "failed to open file '" << dic_filename << "' for write access";
        
          throw Exception(ostr.str());
        }
        
        ACE_High_Res_Timer timer;
        
        if(Application::will_trace(El::Logging::HIGH))
        {
          log_ostr.reset(new std::ostringstream());
          
          *log_ostr << "NewsGate::Message::MessageManager::insert: file '"
                    << ins_filename << "', messages:";

          timer.start();
        }
        
        ACE_High_Res_Timer timer2;

        ACE_Time_Value insertion_time;
        ACE_Time_Value temp_insertion_time;
        ACE_Time_Value categorization_time;
        ACE_Time_Value filtering_time;
        ACE_Time_Value saving_time;
        ACE_Time_Value db_load_time;
        ACE_Time_Value showing_time;
        
        if(Application::will_trace(El::Logging::HIGH))
        {        
          timer2.start();
        }

        SearcheableMessageMap temp_messages(
          false,
          true,
          config_.impression_respected_level(),
          0);
        
        {
          MgrWriteGuard guard(mgr_lock_);

          for(StoredMessageList::const_iterator it(messages.begin()),
                end(messages.end()); it != end; ++it)
          {
            const Message::StoredMessage& new_msg = *it;
            const Message::Id& id = new_msg.id;

            if(replace)
            {
              // As existing message were requested need to
              // free space for them removing current ones
              
              StoredMessage* msg = messages_.find(id);

              if(msg)
              {
                if(msg->visible())
                {
                  removed_msg.insert(std::make_pair(id, msg->published));
                  messages_.remove(id);
                }
                else
                {
                  continue;
                }
              }
            }            
            
            NewsGate::Message::StoredMessage* msg =
              insert_as_is ? messages_.insert(new_msg, 0, 0/*, false*/) :
              messages_.insert(new_msg,
                               config_.core_words().prc(),
                               config_.core_words().max()
                               /*false*/);
            
            if(msg == 0)
            {
              if(log_ostr.get() != 0)
              {
                *log_ostr << std::endl << id.string() << " skip duplicate";
              }
              
              continue;
            }

            msg->hide();
            
//            inserted_message_ptr.push_back(msg);
            inserted_messages.insert(std::make_pair(msg->id, msg->published));
          }
/*
          for(StoredMessagePtrArray::iterator i(inserted_message_ptr.begin()),
                e(inserted_message_ptr.end()); i != e; ++i)
          {
            messages_.sort_core_words(**i, 0);
          }
*/
        }

        if(!removed_msg.empty())
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();
          
          delete_messages(removed_msg, connection.in());
          removed_msg.clear();
        }

        if(Application::will_trace(El::Logging::HIGH))
        {        
          timer2.stop();  
          timer2.elapsed_time(insertion_time);

          timer2.start();
        }

        {
          MgrReadGuard guard(mgr_lock_);
          
          for(IdTimeMap::iterator i(inserted_messages.begin()),
                e(inserted_messages.end()); i != e; ++i)
          {
            const StoredMessage* msg = messages_.find(i->first);
            assert(msg != 0);

            StoredMessage* new_msg = temp_messages.insert(*msg, 0, 0 /*,false*/);
            assert(new_msg != 0);
            new_msg->show();
          }
        } 

        if(message_digests)
        {
          message_digests->reserve(inserted_messages.size());
        }

        if(Application::will_trace(El::Logging::HIGH))
        {        
          timer2.stop();  
          timer2.elapsed_time(temp_insertion_time);
        }        
        
        if(!insert_as_is)
        {
          if(Application::will_trace(El::Logging::HIGH))
          {        
            timer2.start();
          }

          MessageCategorizer::MessageCategoryMap old_categories;

          categorize(temp_messages,
                     categorizer.in(),
                     inserted_messages,
                     false,
                     old_categories);

          if(Application::will_trace(El::Logging::HIGH))
          {        
            timer2.stop();
            timer2.elapsed_time(categorization_time);
            
            timer2.start();
          }
        
          apply_message_fetch_filters(temp_messages,
                                      filters.in(),
                                      0,
                                      removed_msg,
                                      SIZE_MAX);

          if(Application::will_trace(El::Logging::HIGH))
          {        
            timer2.stop();  
            timer2.elapsed_time(filtering_time);
          }
        }
          
        if(Application::will_trace(El::Logging::HIGH))
        {
          timer2.start();
        }

        size_t inserted_count = 0;
        
        for(IdTimeMap::iterator i(inserted_messages.begin()),
              e(inserted_messages.end()); i != e; ++i)
        {
          const StoredMessage* msg = temp_messages.find(i->first);

          if(msg == 0)
          {
            if(log_ostr.get() != 0)
            {
              *log_ostr << std::endl << i->first.string() << " filtered out";
            }
            
            continue;
          }

          if(share_messages)
          {
            share_messages->push_back(*msg);
          }

          if(message_digests)
          { 
            message_digests->push_back(
              Event::Transport::MessageDigest(msg->id,
                                              msg->published,
                                              msg->lang,
                                              msg->core_words,
                                              msg->event_id));
          }

          msg->write_image_thumbs(config_.thumbnail_dir().c_str(), true);

          std::string broken_down;
          
          {
            std::ostringstream ostr;
            msg->write_broken_down(ostr);

            broken_down = ostr.str();

            broken_down = El::MySQL::Connection::escape_for_load(
              broken_down.c_str(),
              broken_down.length());
          }

          std::string complements;
          
          {
            std::ostringstream ostr;
            msg->content->write_complements(ostr);

            complements = ostr.str();
          
            complements = El::MySQL::Connection::escape_for_load(
              complements.c_str(),
              complements.length());
          }
          
          std::string categories;
          
          {
            std::ostringstream ostr;
            msg->categories.write(ostr);

            categories = ostr.str();
          
            categories = El::MySQL::Connection::escape_for_load(
              categories.c_str(),
              categories.length());
          }

          if(inserted_count++)
          {
            ins_file << std::endl;
            cat_file << std::endl;
            dic_file << std::endl;
          }
          
          ins_file << msg->id.data << "\t"
                   << (unsigned long)(msg->flags &
                                      StoredMessage::MF_PERSISTENT_FLAGS)
                   << "\t" << msg->signature<< "\t" << msg->url_signature
                   << "\t" << msg->source_id << "\t"
                   << msg->published << "\t" << msg->fetched << "\t"
                   << (unsigned long)msg->space << "\t";

          ins_file.write(complements.c_str(), complements.length());
          ins_file << "\t";
          
          ins_file << El::MySQL::Connection::escape_for_load(
            msg->content->url.c_str()) << "\t"
                   << msg->lang.el_code() << "\t" << msg->country.el_code()
                   << "\t" << El::MySQL::Connection::escape_for_load(
                     msg->source_title.c_str()) << "\t"
                   << El::MySQL::Connection::escape_for_load(
                     msg->content->source_html_link.c_str()) << "\t";
          
          ins_file.write(broken_down.c_str(), broken_down.length());

          cat_file << msg->id.data << "\t" << msg->categories.categorizer_hash
                   << "\t";

          cat_file.write(categories.c_str(), categories.length());

          dic_file << msg->id.data << "\t" << msg->content->dict_hash;

          if(log_ostr.get() != 0)
          {
            *log_ostr << std::endl << msg->id.string() << " saved to DB";
          }
        }
          
        if(!ins_file.fail())
        {
          ins_file.flush();
        }
            
        if(ins_file.fail())
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::insert: "
            "failed to write into file '" << ins_filename << "'";

          throw Exception(ostr.str());
        }

        if(!cat_file.fail())
        {
          cat_file.flush();
        }
            
        if(cat_file.fail())
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::insert: "
            "failed to write into file '" << cat_filename << "'";

          throw Exception(ostr.str());
        }

        if(!dic_file.fail())
        {
          dic_file.flush();
        }
            
        if(dic_file.fail())
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::insert: "
            "failed to write into file '" << dic_filename << "'";

          throw Exception(ostr.str());
        }

        ins_file.close();
        cat_file.close();
        dic_file.close();
        
        if(Application::will_trace(El::Logging::HIGH))
        {        
          timer2.stop();
          timer2.elapsed_time(saving_time);
        }

        size_t loaded_rows = 0;
        size_t inserted_rows = 0;
        
        if(inserted_count)
        {
          save_messages(ins_filename.c_str(),
                        cat_filename.c_str(),
                        dic_filename.c_str(),
                        loaded_rows,
                        inserted_rows,
                        db_load_time);
        }
        
        unlink(ins_filename.c_str());
        unlink(cat_filename.c_str());
        unlink(dic_filename.c_str());

        if(Application::will_trace(El::Logging::HIGH))
        {
          timer2.start();          
        }

        {
          MgrWriteGuard guard(mgr_lock_);

          for(IdTimeMap::iterator i(inserted_messages.begin()),
                e(inserted_messages.end()); i != e; ++i)
          {
            const Id& id = i->first;
            StoredMessage* temp_msg = temp_messages.find(id);

            if(temp_msg == 0)
            {
              messages_.remove(id);
            }
            else
            {
              StoredMessage* msg = messages_.find(id);
              assert(msg != 0);

              if(!insert_as_is)
              {
                messages_.set_categories(*msg, temp_msg->categories);
              }

              msg->content = 0;
              msg->flags |= StoredMessage::MF_DIRTY;
              msg->show();
            }
          }

          recent_msg_deletions(removed_msg);
        }
        
        if(log_ostr.get() != 0)
        {
          timer2.stop();
          timer2.elapsed_time(showing_time);
          
          timer.stop();
          ACE_Time_Value tm;
          timer.elapsed_time(tm);
          
          *log_ostr << std::endl << inserted_count << "/"
                    << removed_msg.size()
                    << " messages inserted/filtered out, "
                    << loaded_rows << "/" << inserted_rows
                    << " rows loaded/inserted\ntime "
                    << El::Moment::time(tm) << " ( insertion "
                    << El::Moment::time(insertion_time) << "/"
                    << El::Moment::time(temp_insertion_time)
                    << ", categorization "
                    << El::Moment::time(categorization_time)
                    << ", filtering " << El::Moment::time(filtering_time)
                    << ", saving " << El::Moment::time(saving_time)
                    << ", DB inserting " << El::Moment::time(db_load_time)
                    << ", showing " << El::Moment::time(showing_time)
                    << " )";
          
          Application::logger()->trace(log_ostr->str(),
                                       Aspect::MSG_MANAGEMENT,
                                       El::Logging::HIGH);
        }
      }
      catch(...)
      {
        try
        {
          if(!inserted_messages.empty())
          {
            MgrWriteGuard guard(mgr_lock_);
            
            for(IdTimeMap::iterator it = inserted_messages.begin();
                it != inserted_messages.end(); it++)
            {
              messages_.remove(it->first);
            } 
          }
        
          unlink(ins_filename.c_str());
          unlink(cat_filename.c_str());
          unlink(dic_filename.c_str());

          if(!inserted_messages.empty())
          {
            El::MySQL::Connection_var connection =
              Application::instance()->dbase()->connect();
            
            delete_messages(inserted_messages, connection.in());
          }

          if(!removed_msg.empty())
          {
            El::MySQL::Connection_var connection =
              Application::instance()->dbase()->connect();
        
            delete_messages(removed_msg, connection.in());
          }
        }
        catch(...)
        {
        }
        
        throw;
      }

      return dict_not_changed;
    }

    void
    MessageManager::save_messages(const char* msg_filename,
                                  const char* cat_filename,
                                  const char* dic_filename,
                                  size_t& loaded_rows,
                                  size_t& inserted_rows,
                                  ACE_Time_Value& time) const
      throw(El::Exception)
    {   
      ACE_High_Res_Timer timer;
          
      if(Application::will_trace(El::Logging::HIGH))
      {
        timer.start();
      }

      Guard guard(db_lock_);      
          
      El::MySQL::Connection_var connection =
        Application::instance()->dbase()->connect();

      El::MySQL::Result_var result =
        connection->query("delete from MessageBuff");
      
      std::string query_load_messages = std::string("LOAD DATA INFILE '") +
        connection->escape(msg_filename) +
        "' REPLACE INTO TABLE MessageBuff character set binary";
          
      result = connection->query(query_load_messages.c_str());
      loaded_rows = connection->affected_rows();

      std::string query_load_categories = std::string("LOAD DATA INFILE '") +
        connection->escape(cat_filename) +
        "' REPLACE INTO TABLE MessageCat character set binary";
          
      std::string query_load_dict_info = std::string("LOAD DATA INFILE '") +
        connection->escape(dic_filename) +
        "' REPLACE INTO TABLE MessageDict character set binary";
          
      result = connection->query("begin");
      
      try
      {
        result =
          connection->query("REPLACE INTO Message SELECT * FROM MessageBuff");
      
        inserted_rows = connection->affected_rows();
        result = connection->query(query_load_categories.c_str());
        result = connection->query(query_load_dict_info.c_str());
        result = connection->query("commit");
      }
      catch(...)
      {
        result = connection->query("rollback");
        throw;
      }      
      
      guard.release();

      if(Application::will_trace(El::Logging::HIGH))
      {
        timer.stop();        
        timer.elapsed_time(time);
      }
    }
    
    uint32_t
    MessageManager::dictionary_hash()
      throw(WordManagerNotReady, Exception, El::Exception)
    {
      {
        MgrReadGuard guard(mgr_lock_);
        
        if(dict_hash_)
        {
          return dict_hash_;
        }
      }
        
      try
      {
        Dictionary::WordManager_var word_manager =
          Application::instance()->word_manager();
        
        uint32_t dict_hash = word_manager->hash();
        
        MgrWriteGuard guard(mgr_lock_);
        
        dict_hash_ = dict_hash;
        return dict_hash;
      }
      catch(const Dictionary::NotReady& e)
      {
        std::ostringstream ostr;
        ostr << "MessageManager::dictionary_hash: word manager "
          "not ready. Reason:\n" << e.reason.in();

        throw WordManagerNotReady(ostr.str());
      }
      catch(const Dictionary::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "MessageManager::dictionary_hash: "
          "Dictionary::WordManager::ImplementationException caught. "
          "Description:\n" << e.description.in();
          
        throw Exception(ostr.str());
      }      
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "MessageManager::dictionary_hash: CORBA::Exception  caught. "
          "Description:\n" << e;
          
        throw Exception(ostr.str());
      }
    }

    bool
    MessageManager::renormalize_words(StoredMessageList& messages,
                                      uint32_t dict_hash)
      throw(WordManagerNotReady, Exception, El::Exception)
    {
      if(messages.empty())
      {
        return true;
      }

      IdSet changed_norm_forms;
      bool dict_not_changed = false;
      ACE_Time_Value normalization_time;
      ACE_Time_Value insertion_time;
      ACE_Time_Value dict_hash_update_time;
      
      ACE_High_Res_Timer timer;
        
      if(Application::will_trace(El::Logging::HIGH))
      {
        timer.start();
      }
          
      dict_not_changed =
        normalize_words(messages, dict_hash, &changed_norm_forms);
      
      if(Application::will_trace(El::Logging::HIGH))
      {
        timer.stop();
        timer.elapsed_time(normalization_time);

        timer.start();
      }

      size_t dict_hash_updated = 0;
      
      std::ostringstream dict_hash_update_str;
      dict_hash_update_str << "UPDATE MessageDict SET hash="
                           << dict_hash << " where id in (";
      
      IdTimeMap to_remove;
      
      {
        MgrWriteGuard guard(mgr_lock_);

        for(StoredMessageList::iterator i(messages.begin()),
              e(messages.end()); i != e; ++i)
        {
          StoredMessage& m = *i;
          
          m.content = 0;
          
          if(changed_norm_forms.find(m.id) != changed_norm_forms.end())
          {
            changed_messages_.insert(m);
          }
          else
          {
            const NewsGate::Message::StoredMessage* msg =
              messages_.insert(m, 0, 0/*, false*/);

            if(msg)
            {
              dict_hash_update_str << (dict_hash_updated++ ? ", " : " ")
                                   << msg->id.data;
            }
            else
            {
              to_remove.insert(std::make_pair(m.id, m.published));
            }
          }
        }

        dict_hash_update_str << " )";
      }
    
      if(Application::will_trace(El::Logging::HIGH))
      {
        timer.stop();
        timer.elapsed_time(insertion_time);
      }

      if(dict_hash_updated)
      {
        if(Application::will_trace(El::Logging::HIGH))
        {
          timer.start();
        }

        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();
        
        El::MySQL::Result_var result =
          connection->query(dict_hash_update_str.str().c_str());
        
        if(Application::will_trace(El::Logging::HIGH))
        {
          timer.stop();
          timer.elapsed_time(dict_hash_update_time);
        }
      }

      ACE_Time_Value delete_time;
      
      if(!to_remove.empty())
      {
        if(Application::will_trace(El::Logging::HIGH))
        {
          timer.start();
        }
        
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();
        
        delete_messages(to_remove, connection);

        if(Application::will_trace(El::Logging::HIGH))
        {
          timer.stop();
          timer.elapsed_time(delete_time);
        }
      }
      
      if(Application::will_trace(El::Logging::HIGH))
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::renormalize_words: "
          " from " << messages.size() << " messages: changed "
             << changed_norm_forms.size()
             << ", updated " << dict_hash_updated
             << ", deleted " << to_remove.size()
             << ", times:\nnormalization "
             << El::Moment::time(normalization_time)
             << ", insertion " << El::Moment::time(insertion_time)
             << ", dict hash update "
             << El::Moment::time(dict_hash_update_time)
             << ", delete " << El::Moment::time(delete_time);
          
        Application::logger()->trace(ostr.str(),
                                     Aspect::DB_PERFORMANCE,
                                     El::Logging::HIGH);
      }      

      return dict_not_changed;
    }

    void
    MessageManager::insert_changed_messages(InsertChangedMessages* icm)
      throw(Exception, El::Exception)
    {
      ACE_Time_Value insertion_time;
      ACE_Time_Value dict_hash_update_time;
      ACE_Time_Value categorization_time;
      ACE_Time_Value delete_time;

      ACE_High_Res_Timer timer;

      if(Application::will_trace(El::Logging::HIGH))
      {
        timer.start();
      }

      std::string dic_filename = cache_filename_ + ".dic." +
        El::Moment(ACE_OS::gettimeofday()).dense_format();

      std::fstream dic_file(dic_filename.c_str(), ios::out);
        
      if(!dic_file.is_open())
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::insert_changed_messages: "
          "failed to open file '" << dic_filename << "' for write access";
        
        throw Exception(ostr.str());
      }
      
      size_t dict_hash_updated = 0;
      uint32_t dict_hash = dictionary_hash();
      
//      std::ostringstream dict_hash_update_str;
//      dict_hash_update_str << "UPDATE MessageDict SET hash="
//                           << dictionary_hash() << " where id in (";
      
      IdTimeMap to_categorize;
      IdTimeMap to_remove;
      MessageCategorizer_var categorizer = get_message_categorizer();
      
      StoredMessageSet& message_set = icm->message_set;
      size_t count = config_.message_cache().changed_msg_insert_chunk_size();
      size_t message_count = 0;

      {
//        StoredMessagePtrArray inserted_message_ptr;
//        inserted_message_ptr.reserve(count);
        
        MgrWriteGuard guard(mgr_lock_);

        for(; !message_set.empty() && count--; ++message_count)
        {
          StoredMessageSet::iterator i(message_set.begin());
          const NewsGate::Message::StoredMessage& m = *i;

          NewsGate::Message::StoredMessage* msg = 
            messages_.insert(m,
                             config_.core_words().prc(),
                             config_.core_words().max()
                             /*false*/);

          if(msg)
          {
            if(dict_hash_updated++)
            {
              dic_file << std::endl;
            }

            dic_file << msg->id.data << "\t" << dict_hash;
            
//            dict_hash_update_str << (dict_hash_updated++ ? ", " : " ")
//                                 << msg->id.data;            

            to_categorize.insert(std::make_pair(msg->id, msg->published));
          }
          else
          {
            to_remove.insert(std::make_pair(m.id, m.published));
          }

          message_set.erase(i);
        }

        if(!dic_file.fail())
        {
          dic_file.flush();
        }

        if(dic_file.fail())
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::"
            "insert_changed_messages: failed to write into file '"
               << dic_filename << "'";

          throw Exception(ostr.str());
        }

        dic_file.close();

//        dict_hash_update_str << " )";
      }

      if(Application::will_trace(El::Logging::HIGH))
      {
        timer.stop();
        timer.elapsed_time(insertion_time);

        timer.start();
      }

      try
      {
        categorize_and_save(categorizer.in(), to_categorize, true, 0);
        msg_delete_event_bank_notify(to_categorize);

        if(Application::will_trace(El::Logging::HIGH))
        {
          timer.stop();
          timer.elapsed_time(categorization_time);
        }

        if(dict_hash_updated)
        {
          if(Application::will_trace(El::Logging::HIGH))
          {
            timer.start();
          }
          
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();
        
//          El::MySQL::Result_var result =
//            connection->query(dict_hash_update_str.str().c_str());


          std::string query_load_dict_info =
            std::string("LOAD DATA INFILE '") +
            connection->escape(dic_filename.c_str()) +
            "' REPLACE INTO TABLE MessageDict character set binary";

          El::MySQL::Result_var result =
            connection->query(query_load_dict_info.c_str());
        
          if(Application::will_trace(El::Logging::HIGH))
          {
            timer.stop();
            timer.elapsed_time(dict_hash_update_time);
          }
        }
      
        unlink(dic_filename.c_str());
        
        if(!to_remove.empty())
        {
          if(Application::will_trace(El::Logging::HIGH))
          {
            timer.start();
          }
        
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();
        
          delete_messages(to_remove, connection);

          if(Application::will_trace(El::Logging::HIGH))
          {
            timer.stop();
            timer.elapsed_time(delete_time);
          }
        }
      }
      catch(const El::Exception& e)
      {
        unlink(dic_filename.c_str());
        
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::insert_changed_messages: "
          "El::Exception caught. Description:" << std::endl << e;
        
        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
        
        deliver_at_time(
          icm,
          ACE_OS::gettimeofday() +
          ACE_Time_Value(config_.message_cache().
                         loaded_msg_insert_retry_delay()));
        
        return;
      }
      
      if(Application::will_trace(El::Logging::HIGH))
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::insert_changed_messages: "
          " from " << message_count << " messages: categorized "
             << to_categorize.size()
             << ", deleted " << to_remove.size()
             << ", times:\ninsertion " << El::Moment::time(insertion_time)
             << ", categorization " << El::Moment::time(categorization_time)
             << ", dict hash update "
             << El::Moment::time(dict_hash_update_time)
             << ", delete " << El::Moment::time(delete_time);
          
        Application::logger()->trace(ostr.str(),
                                     Aspect::DB_PERFORMANCE,
                                     El::Logging::HIGH);
      }
      
      if(message_set.empty())
      {
        dump_mem_usage();
            
        dump_message_size_distribution();
//        dump_word_mem_distribution();

        dump_word_freq_distribution(messages_.words,
                                    "Words");

        dump_word_freq_distribution(messages_.norm_forms,
                                    "Normal forms");

        dump_word_pair_freq_distribution();

        if(word_pair_managers_.get())
        {
          for(TCWordPairManagerMap::iterator
                i(word_pair_managers_->begin()),
                e(word_pair_managers_->end()); i != e; ++i)
          {
            TCWordPairManager_var wp_manager = i->second;
            wp_manager->stat(false);
//            wp_manager->sync();
          }
        }        
        
        El::Service::CompoundServiceMessage_var msg =
          new DeleteObsoleteMessages(this, "MessageCat");
        
        deliver_now(msg.in());
      }
      else
      {
        deliver_at_time(icm, ACE_OS::gettimeofday() + ACE_Time_Value(1));
      }
    }
    
    void
    MessageManager::insert_loaded_messages(
      StoredMessageArray& messages,
      uint32_t dict_hash,
      StoredMessageList& messages_to_renormalize)
      throw(Exception, El::Exception)
    {      
      messages_to_renormalize.clear();

      ACE_Time_Value insert_time;
      ACE_Time_Value filter_time;
      ACE_Time_Value delete_time;
      
      ACE_High_Res_Timer timer;
      size_t inserted_count = 0;
      size_t present_count = 0;
      size_t broken_count = 0;
      size_t to_renormalize_count = 0;
      size_t skipped_count = 0;
      
      IdTimeMap removed_msg;
      
      {
        uint64_t expired = ACE_OS::gettimeofday().sec() -
          config_.message_expiration_time();
      
        MgrWriteGuard guard(mgr_lock_);

        bool mirror = session_->mirror();

        if(Application::will_trace(El::Logging::HIGH))
        {
          timer.start();
        }
          
        for(StoredMessageArray::iterator it = messages.begin();
            it != messages.end(); ++it)
        {
          StoredMessage& msg = *it;

          if(messages_.find(msg.id))
          {
            ++present_count;
            continue;
          }

          if(msg.published <= expired)
          {
            removed_msg.insert(std::make_pair(msg.id, msg.published));
            continue;
          }

          if(msg.flags & StoredMessage::MF_DIRTY)
          {
            ++broken_count;
            continue;
          }

          bool to_renormalize = !mirror &&
            (msg.content == 0 || msg.content->dict_hash != dict_hash ||
             msg.content->dict_hash == 0);
          
          if(to_renormalize)
          {
            // Do not reset msg.content to allow renormalize_words to
            // enforse message core words recalculation
            // if msg.content->dict_hash == 0

            // msg.content = 0;
            
            messages_to_renormalize.push_back(msg);
            ++to_renormalize_count;
          }   
          else
          {
            msg.content = 0;
            
            if(messages_.insert(msg, 0, 0/*, false*/))
            {
              ++inserted_count;
            }
            else
            {
              ++skipped_count;
              removed_msg.insert(std::make_pair(msg.id, msg.published));
            }
          }
        }

        if(Application::will_trace(El::Logging::HIGH))
        {
          timer.stop();
          timer.elapsed_time(insert_time);
            
          timer.start();
        }
          
        apply_message_fetch_filters(messages_,
                                    message_filters_.in(),
                                    0,
                                    removed_msg,
                                    SIZE_MAX);

        recent_msg_deletions(removed_msg);
        
        if(Application::will_trace(El::Logging::HIGH))
        {
          timer.stop();
          timer.elapsed_time(filter_time);
        }
      }

      if(!removed_msg.empty())
      {
        if(Application::will_trace(El::Logging::HIGH))
        {
          timer.start();
        }
        
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();
        
        delete_messages(removed_msg, connection);

        if(Application::will_trace(El::Logging::HIGH))
        {
          timer.stop();
          timer.elapsed_time(delete_time);
        }
      }
      
      if(Application::will_trace(El::Logging::HIGH))
      {
        std::ostringstream ostr;    
        ostr << "NewsGate::Message::MessageManager::insert_loaded_messages: "
             << inserted_count << " msg insert "
             << El::Moment::time(insert_time) << ", filter " 
             << El::Moment::time(filter_time) << "; "
             << removed_msg.size() << " msg deleted "
             << El::Moment::time(delete_time)
             << ", " << present_count << " present, "
             << to_renormalize_count << " to renormalize, "
             << skipped_count << " skipped, "
             << broken_count << " broken";
        
        Application::logger()->trace(ostr.str(),
                                     Aspect::MSG_MANAGEMENT,
                                     El::Logging::HIGH);
      }
    }
    
    struct TimeSet :
      public google::sparse_hash_set<uint64_t, El::Hash::Numeric<uint64_t> >
    {
      TimeSet() throw(El::Exception) { set_deleted_key(0); }
    };
    
    class MessageImageFinder : public El::FileSystem::DirectoryReader
    {  
    public:
      MessageImageFinder(const char* dir_path,
                         const MessageManager::IdTimeMap& ids)
        throw(El::Exception);

      virtual bool select(const struct dirent* dir) throw(El::Exception);
      
    private:
      const MessageManager::IdTimeMap& ids_;
    };

    MessageImageFinder::MessageImageFinder(
      const char* dir_path,
      const MessageManager::IdTimeMap& ids)
      throw(El::Exception) : ids_(ids)
    {
      read(dir_path);
    }
    
    bool
    MessageImageFinder::select(const struct dirent* dir) throw(El::Exception)
    {
      const char* name = dir->d_name;
      const char* ptr = strchr(name, '-');

      try
      {
        std::string id_str(ptr ? std::string(name, ptr - name) : name);
        Id id(id_str.c_str());
          
        return ids_.find(id) != ids_.end();
      }
      catch(const InvalidArg&)
      {
      }
      
      return false;
    }

    void
    MessageManager::delete_messages(const IdTimeMap& ids,
                                    El::MySQL::Connection* connection,
                                    bool event_bank_notify)
      throw(Exception, El::Exception)
    {
      if(ids.empty())
      {
        return;
      }

      TimeSet times;
      
      std::ostringstream ostr_msg;
      ostr_msg << " where id in ( ";
      
      for(IdTimeMap::const_iterator b(ids.begin()), i(b), e(ids.end());
          i != e; ++i)
      {
        if(i != b)
        {
          ostr_msg << ", ";
        }

        ostr_msg << i->first.data;        
        times.insert(i->second / 86400 * 86400);
      }

      ostr_msg << " )";

      try
      {
        El::MySQL::Result_var result = connection->query("begin");

        try
        {
          result = connection->query(
            (std::string("delete from Message") + ostr_msg.str()).c_str());
          
          result = connection->query(
            (std::string("delete from MessageCat") + ostr_msg.str()).c_str());
          
          result = connection->query(
            (std::string("delete from MessageStat") + ostr_msg.str()).c_str());
          
          result = connection->query(
            (std::string("delete from MessageDict") + ostr_msg.str()).c_str());
          
          result = connection->query("commit");
        }
        catch(...)
        {
          result = connection->query("rollback");
          throw;
        }
      }
      catch(const El::MySQL::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::"
          "delete_messages: El::MySQL::Exception caught. Description:\n" << e;
        
        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
      }

      for(TimeSet::const_iterator it = times.begin(); it != times.end(); it++)
      {
        std::string dir = std::string(config_.thumbnail_dir()) + "/" +
           El::Moment(ACE_Time_Value(*it)).dense_format(El::Moment::DF_DATE);

        try
        {
          MessageImageFinder finder(dir.c_str(), ids);
          std::string prefix = dir + "/";
      
          for(unsigned long i = 0; i < finder.count(); i++)
          {
            std::string full_path = prefix + finder[i].d_name;
            unlink(full_path.c_str());
          }
        }
        catch(const El::FileSystem::Exception&)
        {
        }

        rmdir(dir.c_str());
      }

      if(event_bank_notify)
      {
        msg_delete_event_bank_notify(ids);
      }
    }

    void
    MessageManager::recent_msg_deletions(const IdTimeMap& ids)
      throw(El::Exception)
    {
      uint64_t cur_time = ACE_OS::gettimeofday().sec();
      
      for(IdTimeMap::const_iterator i(ids.begin()), e(ids.end()); i != e; ++i)
      {
        recent_msg_deletions_[i->first] = cur_time;
      }
    }

    void
    MessageManager::msg_delete_event_bank_notify(const IdTimeMap& ids)
      throw(El::Exception)
    {
      if(ids.empty())
      {
        return;
      }
      
      Event::BankClientSession_var event_bank_client_session =
        Application::instance()->event_bank_client_session();

      if(event_bank_client_session.in() != 0)
      {
        MgrWriteGuard guard(mgr_lock_);

        bool send = del_msg_notifications_.empty();
        
        for(IdTimeMap::const_iterator i(ids.begin()), e(ids.end()); i != e;
            ++i)
        {
          del_msg_notifications_[i->first] = i->second;
        }

        if(send)
        {
          El::Service::CompoundServiceMessage_var msg =
            new MsgDeleteNotification(this);
          
          deliver_now(msg.in());
        }
      }
    }

    void
    MessageManager::msg_delete_notification(MsgDeleteNotification* mdn)
      throw(Exception, El::Exception)
    {
      {
        MgrReadGuard guard(mgr_lock_);

        if(del_msg_notifications_.empty())
        {
          return;
        }
      }

      IdTimeMap deleted_messages;
      
      time_t cur_time = ACE_OS::gettimeofday().sec();
          
      uint64_t expire_time =
        cur_time > (time_t)config_.message_expiration_time() ?
        cur_time - config_.message_expiration_time() : 0;
      
      const Server::Config::MessageBankType::event_management_type&
        config = Application::instance()->config().event_management();
        
      Event::BankClientSession_var event_bank_client_session =
          Application::instance()->event_bank_client_session();
        
      { 
        MgrWriteGuard guard(mgr_lock_);
        
        if(event_bank_client_session.in() == 0)
        {
          del_msg_notifications_.clear();
        }

        if(del_msg_notifications_.empty())
        {
          return;
        }

        const size_t max_notification_pack_size =
          config.message_delete_pack_size();

        size_t added = 0;

//        while(!del_msg_notifications_.empty() &&
//              deleted_messages.size() < max_notification_pack_size)
        
        for(IdTimeMap::iterator i(del_msg_notifications_.begin()),
              e(del_msg_notifications_.end()); i != e &&
              added < max_notification_pack_size; ++i)
        {
//          IdTimeMap::iterator it = del_msg_notifications_.begin();

          if(i->second > expire_time)
          {
            deleted_messages.insert(std::make_pair(i->first, i->second));
            ++added;
          }

          del_msg_notifications_.erase(i);
        }
      }

      del_msg_notifications_.resize(0);

      ACE_Time_Value delay;
      
      if(!deleted_messages.empty())
      {
        ACE_High_Res_Timer timer;
        timer.start();
        
        Transport::IdPackImpl::Var id_pack =
          new Transport::IdPackImpl::Type(new IdArray());
          
        IdArray& id_array = id_pack->entities();
        id_array.reserve(deleted_messages.size());
        
        for(IdTimeMap::const_iterator it = deleted_messages.begin();
            it != deleted_messages.end(); it++)
        {
          id_array.push_back(it->first);
        }
        
        delay = ACE_Time_Value(config.message_delete_retry_period());
        
        try
        {
          event_bank_client_session->delete_messages(id_pack);
          delay = ACE_Time_Value::zero;
        }
        catch(const Event::NotReady& e)
        {
          if(Application::will_trace(El::Logging::HIGH))
          {
            std::ostringstream ostr;
        
            ostr << "NewsGate::Message::MessageManager::"
              "msg_delete_notification: NotReady caught. "
              "Description:\n" << e.reason.in();
            
            Application::logger()->trace(ostr.str(),
                                         Aspect::MSG_SHARING,
                                         El::Logging::HIGH);
          }
        }
        catch(const Event::ImplementationException& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::"
            "msg_delete_notification: ImplementationException "
            "caught. Description:\n" << e.description.in();
          
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(const CORBA::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::"
            "msg_delete_notification: CORBA::Exception caught. "
            "Description:\n" << e;
          
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }

        timer.stop();
        ACE_Time_Value tm;
        timer.elapsed_time(tm);
        
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::"
          "msg_delete_notification: time "
             << El::Moment::time(tm);
        
        Application::logger()->trace(ostr.str(),
                                     Aspect::MSG_MANAGEMENT,
                                     El::Logging::HIGH);
      }
      
      if(delay != ACE_Time_Value::zero)
      {
        MgrWriteGuard guard(mgr_lock_);

        for(IdTimeMap::const_iterator it = deleted_messages.begin();
            it != deleted_messages.end(); it++)
        {
          del_msg_notifications_.insert(std::make_pair(it->first, it->second));
        }
      }
      else
      {
        delay = ACE_Time_Value(1);
      }
        
      deliver_at_time(mdn, ACE_OS::gettimeofday() + delay);
    }
    
    void
    MessageManager::insert_loaded_messages() throw(El::Exception)
    {
      bool finished = false;
      
      time_t retry_delay =
        config_.message_cache().loaded_msg_insert_retry_delay();
      
      MessageLoader_var loader;
      MessageLoader::StoredMessageArrayPtr msg_array;
      
      try
      { 
        uint32_t dict_hash = dictionary_hash();

        if(!normalize_categorizer(dict_hash) || !normalize_filters(dict_hash))
        {
          return;
        }

        loader = state();
        msg_array.reset(loader->pop_messages(finished));
        
        if(!finished)
        {
          if(msg_array.get() == 0)
          {
            El::Service::CompoundServiceMessage_var msg =
              new InsertLoadedMsg(this);
            
            deliver_at_time(msg.in(),
                            ACE_OS::gettimeofday() + ACE_Time_Value(1));
            
            return;
          }
          
          StoredMessageList messages_to_renormalize;

          insert_loaded_messages(*msg_array,
                                 dict_hash,
                                 messages_to_renormalize);

          if(!messages_to_renormalize.empty())
          {
            if(!renormalize_words(messages_to_renormalize, dict_hash))
            {  
              callback_->dictionary_hash_changed();
            }
          }
        }
      }
      catch(const WordManagerNotReady&)
      {
        if(msg_array.get())
        {
          loader->push_messages(msg_array.release());
        }
          
        El::Service::CompoundServiceMessage_var msg =
          new InsertLoadedMsg(this);
        
        deliver_at_time(msg.in(),
                        ACE_OS::gettimeofday() + ACE_Time_Value(retry_delay));
        
        return;
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::insert_loaded_messages: "
          "El::Exception caught. Description:" << std::endl << e;
        
        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
          
        El::Service::CompoundServiceMessage_var msg =
          new InsertLoadedMsg(this);
        
        deliver_at_time(msg.in(),
                        ACE_OS::gettimeofday() + ACE_Time_Value(retry_delay));
        
        return;
      }

      El::Service::CompoundServiceMessage_var msg;
      ACE_Time_Value delay;

      if(finished)
      {   
        state(0);

        if(word_pair_managers_.get())
        {
          for(TCWordPairManagerMap::iterator
                i(word_pair_managers_->begin()),
                e(word_pair_managers_->end()); i != e; ++i)
          {
            i->second->cache_size(0);
          }
        }
        
        El::Service::CompoundServiceMessage_var msg;
        
        {
          MgrWriteGuard guard(mgr_lock_);
          msg = new InsertChangedMessages(this, changed_messages_);
        }
        
        deliver_now(msg.in());
      }
      else
      {
        schedule_load();
      }
    }

    void
    MessageManager::delete_obsolete_messages(const char* table_name)
      throw(El::Exception)
    {
      time_t retry_delay =
        config_.message_cache().loaded_msg_insert_retry_delay();

      bool load_finished = false;
      
      try
      {
        Message::IdArray obsolete_messages;
        
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();

        std::ostringstream ostr;
        ostr << "select id from " << table_name << " where id>"
             << msg_dom_last_id_.data << " order by id limit "
             << config_.message_loader().read_chunk_size();

        ACE_High_Res_Timer timer;
        timer.start();
          
        El::MySQL::Result_var result =
          connection->query(ostr.str().c_str());

        size_t loaded_messages = 0;

        MessageContentRecord record(result.in(), 1);
        
        {
          MgrReadGuard guard(mgr_lock_);
          
          for(; record.fetch_row(); ++loaded_messages)
          {
            // Do not aquire write lock as used in single thread
            msg_dom_last_id_ = record.id().value();

            if(messages_.find(msg_dom_last_id_) == 0)
            {
              obsolete_messages.push_back(msg_dom_last_id_);
            }
          }
        }

        timer.stop();
        
        ACE_Time_Value load_time;
        timer.elapsed_time(load_time);
          
        timer.start();

        if(!obsolete_messages.empty())
        {
          std::ostringstream ostr;
          ostr << "delete from " << table_name << " where id in (";
              
          for(Message::IdArray::const_iterator
                b(obsolete_messages.begin()), i(b),
                e(obsolete_messages.end()); i != e; ++i)
          {
            if(i != b)
            {
              ostr << ", ";
            }
                
            ostr << i->data;
          }

          ostr << ")";
            
          result = connection->query(ostr.str().c_str());
        }
            
        timer.stop();
        ACE_Time_Value delete_time;
        timer.elapsed_time(delete_time);
          
        if(Application::will_trace(El::Logging::MIDDLE))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::"
            "delete_obsolete_messages("
               << table_name << "):\n  " << loaded_messages
               << " messages, " << obsolete_messages.size()
               << " obsolete\n"
               << "  load time: " << El::Moment::time(load_time)
               << ", delete time: " << El::Moment::time(delete_time);
            
          Application::logger()->trace(ostr.str(),
                                       Aspect::MSG_MANAGEMENT,
                                       El::Logging::MIDDLE);
        }
          
        load_finished =
          loaded_messages < config_.message_loader().read_chunk_size();
          
        if(load_finished)
        {
          if(Application::will_trace(El::Logging::MIDDLE))
          {              
            std::ostringstream ostr;              
            ostr << "NewsGate::Message::MessageManager::"
              "delete_obsolete_messages("
                 << table_name << "): completed";
                
            Application::logger()->trace(ostr.str(),
                                         Aspect::MSG_MANAGEMENT,
                                         El::Logging::MIDDLE);
          }        
        }          
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::delete_obsolete_messages: "
          "El::Exception caught. Description:" << std::endl << e;
        
        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
          
        El::Service::CompoundServiceMessage_var msg =
          new DeleteObsoleteMessages(this, table_name);
        
        deliver_at_time(msg.in(),
                        ACE_OS::gettimeofday() + ACE_Time_Value(retry_delay));
        
        return;
      }

      if(load_finished)
      {        
        if(strcmp(table_name, "MessageCat") == 0)
        {
          {
            MgrWriteGuard guard(mgr_lock_);
            msg_dom_last_id_ = Id::zero;
          }
          
          El::Service::CompoundServiceMessage_var msg =
            new DeleteObsoleteMessages(this, "MessageStat");
          
          deliver_now(msg.in());
        }
        else if(strcmp(table_name, "MessageStat") == 0)
        {
          {
            MgrWriteGuard guard(mgr_lock_);
            msg_dom_last_id_ = Id::zero;
          }
          
          El::Service::CompoundServiceMessage_var msg =
            new DeleteObsoleteMessages(this, "MessageDict");
          
          deliver_now(msg.in());
        }
        else
        {          
          {
            MgrWriteGuard guard(mgr_lock_);
            loaded_ = true;
          }          
          
          schedule_traverse();
        }
      }
      else
      {
        El::Service::CompoundServiceMessage_var msg =
          new DeleteObsoleteMessages(this, table_name);
        
        deliver_now(msg.in());
      }
    }

    Search::Condition*
    MessageManager::segment_search_expression(const char* exp)
      throw(SegmentorNotReady, Exception, El::Exception)
    {
      std::string expression;
      
      try
      {        
        NewsGate::Segmentation::Segmentor_var segmentor =
          Application::instance()->segmentor();

        if(CORBA::is_nil(segmentor.in()))
        {
          expression = exp;
        }
        else
        {
          CORBA::String_var res = segmentor->segment_query(exp);
          expression = res.in();
        }

        try
        {
          std::wstring query;
          El::String::Manip::utf8_to_wchar(expression.c_str(), query);
            
          Search::ExpressionParser parser;
          std::wistringstream istr(query);
            
          parser.parse(istr);
          return parser.expression()->condition.retn();
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::"
            "segment_search_expression: El::Exception caught. "
            "Description:\n" << e << "\nExpression:\n" << expression;
            
          throw Exception(ostr.str());
        }
      }
      catch(const NewsGate::Segmentation::NotReady& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::"
          "segment_search_expression: Segmentation::NotReady "
          "caught. Description:\n" << e.reason.in();

        throw SegmentorNotReady(ostr.str());
      }
      catch(const NewsGate::Segmentation::InvalidArgument& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::"
          "segment_search_expression: Segmentation::InvalidArgument "
          "caught. Description:\n" << e.description.in();

        throw Exception(ostr.str());
      }
      catch(const NewsGate::Segmentation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::"
          "segment_search_expression: Segmentation::"
          "ImplementationException caught. Description:\n"
             << e.description.in();

        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::"
          "segment_search_expression: CORBA::Exception caught. "
          "Description:\n" << e;

        throw Exception(ostr.str());
      }
    }
    
    bool
    MessageManager::normalize_search_condition(
      Search::Condition_var& condition)
      throw(WordManagerNotReady, Exception, El::Exception)
    {
      try
      {
        Dictionary::WordManager_var word_manager =
          Application::instance()->word_manager();
        
        Search::Expression_var expression =
          new Search::Expression(El::RefCount::add_ref(condition.in()));
        
        expression->add_ref();
          
        Search::Transport::ExpressionImpl::Var
          expression_transport =
          Search::Transport::ExpressionImpl::Init::create(
            new Search::Transport::ExpressionHolder(expression));
          
        expression_transport->serialize();

        NewsGate::Search::Transport::Expression_var result;

//        uint32_t dict_hash =
          word_manager->normalize_search_expression(
            expression_transport.in(),
            result.out());

        // Remove the code below to avoid redundant locking
/*
        {
          MgrReadGuard guard(mgr_lock_);
        
          if(dict_hash_)
          {
            if(dict_hash_ != dict_hash)
            {
              std::ostringstream ostr;
              ostr << "NewsGate::Message::MessageManager::"
                "normalize_search_condition: dict_hash changed from "
                   << dict_hash_ << " to " << dict_hash;

              guard.release();
              
              El::Service::Error
                error(ostr.str(), this, El::Service::Error::NOTICE);
              
              callback_->notify(&error);
              callback_->dictionary_hash_changed();
              return false;
            }
          }
          else
          {
            guard.release();
            
            MgrWriteGuard guard(mgr_lock_);
            dict_hash_ = dict_hash;
          }
        }
*/

        if(dynamic_cast<Search::Transport::ExpressionImpl::Type*>(
             result.in()) == 0)
        {
          throw Exception(
            "NewsGate::Message::MessageManager::normalize_search_condition: "
            "dynamic_cast<Search::Transport::ExpressionImpl::Type*> failed");
        }
        
        expression_transport =
          dynamic_cast<Search::Transport::ExpressionImpl::Type*>(
            result._retn());
        
        expression = expression_transport->entity().expression;
        condition = expression->condition.retn();

        return true;
      }
      catch(const Dictionary::NotReady& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::"
          "normalize_search_condition: Dictionary::NotReady caught. "
          "Reason:\n" << e.reason.in();

        throw WordManagerNotReady(ostr.str());
      }
      catch(const Dictionary::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::"
          "normalize_search_condition: Dictionary::ImplementationException "
          "caught. Description:\n" << e.description.in();

        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::"
          "normalize_search_condition: CORBA::Exception caught. "
          "Description:\n" << e;

        throw Exception(ostr.str());
      }      
    }

    bool
    MessageManager::normalize_categorizer(uint32_t dict_hash)
      throw(Exception, El::Exception)
    {
      std::string cache_filename;
      ACE_High_Res_Timer timer;
      
      {
        MgrReadGuard guard(mgr_lock_);

        if(message_categorizer_.in() == 0 ||
           message_categorizer_->dict_hash == dict_hash)
        {
          return true;
        }

        if(Application::will_trace(El::Logging::HIGH))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::"
            "normalize_categorizer: dict_hash changed from "
               << message_categorizer_->dict_hash << " to " << dict_hash
               << "; renormalizing ...";
          
          Application::logger()->trace(ostr.str(),
                                       Aspect::MSG_CATEGORIZATION,
                                       El::Logging::HIGH);

          timer.start();
        }        

        cache_filename = cache_filename_ + ".czr." +
          El::Moment(ACE_OS::gettimeofday()).dense_format();

        save_message_categorizer(*message_categorizer_,
                                 cache_filename.c_str());
      }

      MessageCategorizer_var categorizer =
        load_message_categorizer(cache_filename.c_str());

      if(categorizer == 0)
      {
        MgrWriteGuard guard(mgr_lock_);

        if(message_categorizer_.in() != 0 &&
           message_categorizer_->dict_hash != dict_hash)
        {
          message_categorizer_ = 0;
          
          El::Service::Error error(
            "NewsGate::Message::MessageManager::normalize_categorizer: "
            "can't normalize categorizer so deleting.",
            this);
          
          callback_->notify(&error);
        }

        return true;
      }
      
      try
      {
        for(MessageCategorizer::CategoryMap::iterator
              i(categorizer->categories.begin()),
              e(categorizer->categories.end()); i != e; ++i)
        {
          if(!normalize_search_condition(i->second.condition))
          {
            return false;
          }
        }
      }
      catch(const WordManagerNotReady& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::"
          "normalize_categorizer: WordManagerNotReady caught. "
          "Description:\n" << e;

        El::Service::Error
          error(ostr.str(), this, El::Service::Error::NOTICE);
        
        callback_->notify(&error);
        callback_->dictionary_hash_changed();
        return false;
      }

      categorizer->dict_hash = dict_hash;
      bool categorizer_changed = false;
      
      MgrWriteGuard guard(mgr_lock_);

      if(message_categorizer_.in() != 0 &&
         message_categorizer_->dict_hash != dict_hash)
      {
        message_categorizer_ = categorizer.retn();
        categorizer_changed = true;
      }

      if(Application::will_trace(El::Logging::HIGH))
      {
        timer.stop();
        ACE_Time_Value tm;
        timer.elapsed_time(tm);
        
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::"
          "normalize_categorizer: renormalization completed in "
             << El::Moment::time(tm) << "; categorizer "
             << (categorizer_changed ? "" : "not ") << "updated";
          
        Application::logger()->trace(ostr.str(),
                                     Aspect::MSG_CATEGORIZATION,
                                     El::Logging::HIGH);

      }
      
      return true;
    }

    bool
    MessageManager::normalize_filters(uint32_t dict_hash)
      throw(Exception, El::Exception)
    {
      std::string cache_filename;
      ACE_High_Res_Timer timer;
      
      {
        MgrReadGuard guard(mgr_lock_);

        if(message_filters_.in() == 0 ||
           message_filters_->dict_hash == dict_hash)
        {
          return true;
        }

        if(Application::will_trace(El::Logging::HIGH))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::"
            "normalize_filters: dict_hash changed from "
               << message_filters_->dict_hash << " to " << dict_hash
               << "; renormalizing ...";
          
          Application::logger()->trace(ostr.str(),
                                       Aspect::MSG_CATEGORIZATION,
                                       El::Logging::HIGH);

          timer.start();
        }        

        cache_filename = cache_filename_ + ".ffr." +
          El::Moment(ACE_OS::gettimeofday()).dense_format();
        
        save_message_filters(*message_filters_,
                             cache_filename.c_str());
      }

      MessageFetchFilterMap_var filters =
        load_message_filters(cache_filename.c_str());

      if(filters == 0)
      {
        MgrWriteGuard guard(mgr_lock_);

        if(message_filters_.in() != 0 &&
           message_filters_->dict_hash != dict_hash)
        {
          message_filters_ = 0;
          
          El::Service::Error error(
            "NewsGate::Message::MessageManager::normalize_filters: "
            "can't normalize filters so deleting.",
            this);
          
          callback_->notify(&error);
        }

        return true;
      }
      
      try
      {
        for(MessageFetchFilterMap::iterator i(filters->begin()),
              e(filters->end()); i != e; ++i)
        {
          if(!normalize_search_condition(i->second.condition))
          {
            return false;
          }
        }
      }
      catch(const WordManagerNotReady& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::"
          "normalize_filters: WordManagerNotReady caught. "
          "Description:\n" << e;

        El::Service::Error
          error(ostr.str(), this, El::Service::Error::NOTICE);
        
        callback_->notify(&error);
        callback_->dictionary_hash_changed();
        return false;
      }

      filters->dict_hash = dict_hash;
      bool filters_changed = false;
      
      {
        MgrWriteGuard guard(mgr_lock_);
        
        if(message_filters_.in() != 0 &&
           message_filters_->dict_hash != dict_hash)
        {
          message_filters_ = filters.retn();
          filters_changed = true;
        }
      }

      if(Application::will_trace(El::Logging::HIGH))
      {
        timer.stop();
        ACE_Time_Value tm;
        timer.elapsed_time(tm);
        
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::"
          "normalize_filters: renormalization completed in "
             << El::Moment::time(tm) << "; filters "
             << (filters_changed ? "" : "not ") << "updated";
          
        Application::logger()->trace(ostr.str(),
                                     Aspect::MSG_CATEGORIZATION,
                                     El::Logging::HIGH);
      }
      
      return true;
    }

    void
    MessageManager::schedule_traverse() throw(El::Exception)
    {
      optimize_mem_time_ =
        ACE_Time_Value(ACE_OS::gettimeofday().sec() +
                       config_.message_cache().optimize_mem_period());
      
      El::Service::CompoundServiceMessage_var msg;
        
      msg = new ImportMsg(this);
      deliver_now(msg.in());

      msg = new TraverseCache(this);
      deliver_now(msg.in());

      if(config_.message_filter().reapply_period())
      {
        msg = new ReapplyMsgFetchFilters(this);
        deliver_now(msg.in());
      }
    }
    
    void
    MessageManager::schedule_load() throw(El::Exception)
    {
      El::Service::CompoundServiceMessage_var msg =
        new InsertLoadedMsg(this);
        
      deliver_now(msg.in());
    }
    /*
    void
    MessageManager::record_word_mem_distribution(const StoredMessage& msg)
      throw(Exception, El::Exception)
    {
      const MessageWordPosition& word_positions = msg.word_positions;

      for(size_t i = 0; i < word_positions.size(); i++)
      {
        const char* word = word_positions[i].first.c_str();
        size_t len = strlen(word) + 1;

        WordLenStatMap::iterator it = message_word_len_stat_->find(len);

        if(it == message_word_len_stat_->end())
        {
          it = message_word_len_stat_->insert(
            std::make_pair(len, WordLenStat())).first;
        }

        it->second.count++;

        if(message_unique_words_->find(word) == message_unique_words_->end())
        {
          message_unique_words_->insert(word);
          
          WordLenStatMap::iterator it =
            message_unique_word_len_stat_->find(len);

          if(it == message_unique_word_len_stat_->end())
          {
            it = message_unique_word_len_stat_->insert(
              std::make_pair(len, WordLenStat())).first;
          }

          it->second.count++;
        }
        
      }
    }
    */
    void
    MessageManager::record_word_freq_distribution(const StoredMessage& msg)
      throw(Exception, El::Exception)
    { 
      size_t positions = msg.positions.size();
      
      if(positions < 16)
      {
        message_word_count_distribution_[0]++;
      }
      else if(positions < 256)
      {
        message_word_count_distribution_[1]++;
      }
      else
      {
        message_word_count_distribution_[2]++;
      }

      for(size_t i = 0; i < msg.word_positions.size(); i++)
      {
        size_t wpos_count = msg.word_positions[i].second.position_count();

        if(wpos_count <= 2)
        {
          message_word_freq_distribution_[0]++;
        }
        else if(wpos_count <= 4 && positions < 256)
        {
          message_word_freq_distribution_[1]++;
        }
        else
        {
          message_word_freq_distribution_[2]++;
        }
      }
      
      for(size_t i = 0; i < msg.norm_form_positions.size(); i++)
      {
        size_t wpos_count = msg.norm_form_positions[i].second.position_count();

        if(wpos_count <= 2)
        {
          norm_forms_freq_distribution_[0]++;
        }
        else if(wpos_count <= 4 && positions < 256)
        {
          norm_forms_freq_distribution_[1]++;
        }
        else
        {
          norm_forms_freq_distribution_[2]++;
        }
      }
    }

    template<typename TYPE>
    void
    MessageManager::dump_word_freq_distribution(
      const TYPE& word_messages,
      const char* prefix)
      const throw(Exception, El::Exception)
    {
      std::ostringstream ostr;
      
      ostr << "NewsGate::Message::MessageManager::"
        "dump_word_freq_distribution: ";

      {
        MgrReadGuard guard(mgr_lock_);

        size_t total_messages = messages_.messages.size();

        ostr << "words " << word_messages.size()
             << ", total messages " << total_messages
             << "\n" << prefix << " for predefined occurance-in-message "
          "frequencies (%):\n";
        
        unsigned long frequency_distr[1001];
        memset(frequency_distr, 0, sizeof(frequency_distr));
        
        for(typename TYPE::const_iterator it = word_messages.begin();
            it != word_messages.end(); it++)
        {
          frequency_distr[total_messages ?
                          it->second->messages.size() * 1000 / total_messages :
                          0]++;
        }

        for(size_t i = 0; i < sizeof(frequency_distr) /
              sizeof(frequency_distr[0]); i++)
        {
          if(!frequency_distr[i])
          {
            continue;
          }
          
          ostr << i / 10 << "." << i % 10 << " - " << frequency_distr[i]
               << " words (" << std::setprecision(2) << (word_messages.size() ?
            frequency_distr[i] * 100 / word_messages.size() : 0) << "%)\n";
        }
      }
      
      Application::logger()->info(ostr.str(), Aspect::MSG_MANAGEMENT);
    }
    
    void
    MessageManager::dump_message_size_distribution() const
      throw(Exception, El::Exception)
    {
      size_t total_messages = 0;

      for(size_t i = 0; i < sizeof(message_word_count_distribution_) /
            sizeof(message_word_count_distribution_[0]); i++)
      {
        total_messages += message_word_count_distribution_[i];
      }
              
      std::ostringstream ostr;

      if(total_messages)
      {
        ostr << "NewsGate::Message::MessageManager::"
          "dump_message_size_distribution: "
          "message sizes distribution:\n  Small (<16 pos) - "
             << message_word_count_distribution_[0] << " ("
             << message_word_count_distribution_[0] * 100 /
          total_messages
             << "%)\n  Medium (<256 pos) - "
             << message_word_count_distribution_[1] << " ("
             << message_word_count_distribution_[1] * 100 / total_messages
             << "%)\n  Large (>=256 pos) - "
             << message_word_count_distribution_[2] << " ("
             << message_word_count_distribution_[2] * 100 / total_messages
             << "%)\n";
      }
            
      unsigned long long total_words = 0;

      for(size_t i = 0;
          i < sizeof(message_word_freq_distribution_) /
            sizeof(message_word_freq_distribution_[0]); i++)
      {
        total_words += message_word_freq_distribution_[i];
      }

      if(total_words)
      {
        ostr << "Message word position info compression "
          "effectiveness:\n  Hight (2 byte per pos) - "
             << message_word_freq_distribution_[0] << " ("
             << message_word_freq_distribution_[0] * 100 / total_words
             << "%)\n  Medium (1 byte per pos) - "
             << message_word_freq_distribution_[1] << " ("
             << message_word_freq_distribution_[1] * 100 / total_words
             << "%)\n  None (all other) - "
             << message_word_freq_distribution_[2] << " ("
             << message_word_freq_distribution_[2] * 100 / total_words
             << "%)\n";
      }
            
      unsigned long long total_norm_forms = 0;

      for(size_t i = 0; i < sizeof(norm_forms_freq_distribution_) /
            sizeof(norm_forms_freq_distribution_[0]); i++)
      {
        total_norm_forms += norm_forms_freq_distribution_[i];
      }

      if(total_norm_forms)
      {
        ostr << "Message norm forms position info compression "
          "effectiveness:\n  Hight (2 byte per pos) - "
             << norm_forms_freq_distribution_[0] << " ("
             << norm_forms_freq_distribution_[0] * 100 / total_norm_forms
             << "%)\n  Medium (1 byte per pos) - "
             << norm_forms_freq_distribution_[1] << " ("
             << norm_forms_freq_distribution_[1] * 100 / total_norm_forms
             << "%)\n  None (all other) - "
             << norm_forms_freq_distribution_[2] << " ("
             << norm_forms_freq_distribution_[2] * 100 / total_norm_forms
             << "%)";
      }
      
      Application::logger()->info(ostr.str(), Aspect::MSG_MANAGEMENT);
    }

    void
    MessageManager::dump_word_pair_freq_distribution()
      throw(Exception, El::Exception)
    {
      if(wp_counter_type_.empty())
      {
        return;
      }

      std::ostringstream ostr;
      
      if(word_pair_managers_.get())
      {
        ostr << "NewsGate::Message::MessageManager::"
          "dump_word_pair_freq_distribution:";

        for(TCWordPairManagerMap::iterator
              i(word_pair_managers_->begin()), e(word_pair_managers_->end());
            i != e; ++i)
        {
          i->second->dump_word_pair_freq_distribution(ostr);
        }
      }
      else
      {
        ostr << "NewsGate::Message::MessageManager::"
          "dump_word_pair_freq_distribution: refs -> wp count; ops";
      
        unsigned long total_count = 0;
        unsigned long total_ops = 0;
      
        {
          Guard guard(ltwp_counter_lock_);

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
                 << (total_count ? count * 100 / total_count : 0)
                 << "%; " << ops << " "
                 << (total_ops ? ops * 100 / total_ops : 0) << "%";
          }
        }
      
        ostr << std::endl << "total " << total_count << "; ops " << total_ops;
      }
      
      Application::logger()->info(ostr.str(), Aspect::MSG_MANAGEMENT);
    }

/*    
    void
    MessageManager::dump_word_mem_distribution()
      throw(Exception, El::Exception)
    {
      message_unique_words_.reset(0);
      
      dump_word_mem_distribution(message_word_len_stat_,
                                 "message word");
      
      dump_word_mem_distribution(message_unique_word_len_stat_,
                                 "message unique word");
    }
    
    void
    MessageManager::dump_word_mem_distribution(
      WordLenStatMapPtr& message_word_len_stat,
      const char* stat_name)
      throw(Exception, El::Exception)
    {
      size_t total_words = 0;
      size_t sizes = 0;
      unsigned long long total_size = 0;
        
      for(WordLenStatMap::const_iterator it = message_word_len_stat->begin();
          it != message_word_len_stat->end(); it++, sizes++)
      {
        total_words += it->second.count;
        total_size += ((unsigned long long)it->first) * it->second.count;
      }

      std::ostringstream ostr;
      ostr << "NewsGate::Message::MessageManager::dump_word_mem_distribution:"
           << " " << stat_name
           << " memory usage (word len - words, total size):\n";
        
      for(WordLenStatMap::const_iterator it = message_word_len_stat->begin();
          it != message_word_len_stat->end(); it++)
      {
        size_t len = it->first;
        size_t words = it->second.count;
        unsigned long long size = len * words;
        
        ostr << len << " - " << words << " (" << std::setprecision(2)
             << words * 100 / total_words << "%), " << size << " ("
             << std::setprecision(2) << size * 100 / total_size << "%)\n";
      }

      ostr << "Total sizes " << sizes << ", words " << total_words << ", len "
           << total_size / 1024 << "K";

      Application::logger()->info(ostr.str(), Aspect::MSG_MANAGEMENT);
      message_word_len_stat.reset(0);
    }
*/
    void
    MessageManager::save_message_sharing_info(
      Transport::MessageSharingInfoArray& info) throw(El::Exception)
    {
      size_t present = 0;
      size_t foreign = 0;

      IdTimeMap recategorize_ids;
      MessageCategorizer::MessageCategoryMap old_categories;
      MessageCategorizer_var categorizer = get_message_categorizer();

      ACE_High_Res_Timer timer;
      timer.start();

      {
        MgrWriteGuard guard(mgr_lock_);

        bool mirror = session_->mirror();
        
        for(Transport::MessageSharingInfoArray::iterator
              it(info.begin()), ie(info.end()); it != ie; ++it)
        {
          Transport::MessageSharingInfo& sharing_info = *it;
          StoredMessage* msg = messages_.find(sharing_info.message_id);
          
          if(msg && msg->visible())
          {
            if((mirror || categorizer.in() == 0) &&
               msg->categories.categorizer_hash !=
               sharing_info.categories.categorizer_hash)
            {
              old_categories[msg->id] = msg->categories;
              messages_.set_categories(*msg, sharing_info.categories);
              
              recategorize_ids.insert(
                std::make_pair(sharing_info.message_id, msg->published));
            }

            if(msg->impressions != sharing_info.impressions ||
               msg->clicks != sharing_info.clicks)
            {
              messages_.set_impressions(*msg, sharing_info.impressions);
              messages_.set_clicks(*msg, sharing_info.clicks);
              messages_.calc_search_weight(*msg);
              
              msg->flags |= StoredMessage::MF_DIRTY;
            }

            uint64_t visited = std::max(sharing_info.visited, msg->visited);
            
            if(msg->visited != visited)
            {
              msg->visited = visited;
              msg->flags |= StoredMessage::MF_DIRTY;
            }
            
            present++;
          }
          else if(!own_message(sharing_info.message_id))
          {
            foreign++;
          }
        }
      }

      timer.stop();

      ACE_Time_Value applying_change_time;      
      timer.elapsed_time(applying_change_time);      

      Event::BankClientSession_var event_bank_client_session =
        Application::instance()->event_bank_client_session();

      if(event_bank_client_session.in() == 0)
      {
        Transport::MessageEventArray event_info;
        event_info.reserve(info.size());
        
        for(Transport::MessageSharingInfoArray::const_iterator
              it(info.begin()), ie(info.end()); it != ie; ++it)
        {
          event_info.push_back(
            Transport::MessageEvent(it->message_id,
                                    it->event_id,
                                    it->event_capacity));
        }
          
        update_events(event_info);
      }

      ACE_Time_Value filtering_time;
      ACE_Time_Value deletion_time;
      ACE_Time_Value saving_time;
      ACE_Time_Value db_insertion_time;

      post_categorize_and_save(0,
                               recategorize_ids,
                               old_categories,
                               false,
                               filtering_time,
                               applying_change_time,
                               deletion_time,
                               saving_time,
                               db_insertion_time);
      
      if(Application::will_trace(El::Logging::HIGH))
      {
        std::ostringstream ostr;
        
        ostr << "NewsGate::Message::MessageManager::"
          "save_message_sharing_info: " << info.size()
             << " message ids received.\nPresent " << present << ", foreign "
             << foreign << ", recategorized " << recategorize_ids.size()
             << ", times:\napplying change "
             << El::Moment::time(applying_change_time)
             << ", filtering "
             << El::Moment::time(filtering_time)
             << ", deletion "
             << El::Moment::time(deletion_time)
             << ", saving "
             << El::Moment::time(saving_time)
             << ", db insertion "
             << El::Moment::time(db_insertion_time);
        
        Application::logger()->trace(ostr.str(),
                                     Aspect::DB_PERFORMANCE,
                                     El::Logging::HIGH);
      }
    }
      
    void
    MessageManager::message_sharing_offer(
      Transport::MessageSharingInfoArray* offered_msg,
      IdArray& requested_msg)
      throw(Exception, El::Exception)
    {
      std::auto_ptr<Transport::MessageSharingInfoArray>
        offered_messages(offered_msg);
        
      size_t present = 0;
      size_t foreign = 0;
      size_t changed = 0;
      size_t recently_deleted = 0;

      {
        MgrReadGuard guard(mgr_lock_);

        bool mirror = session_->mirror();
        
        for(Transport::MessageSharingInfoArray::iterator
              it(offered_msg->begin()), ie(offered_msg->end()); it != ie; ++it)
        {
          const Id& id = it->message_id;
          const StoredMessage* msg = messages_.find(id);
          bool request_msg = false;
          
          if(msg)
          {
            ++present;

            if(msg->visible() && mirror &&
               it->word_hash != msg->word_hash(true))
            {
              request_msg = true;
              ++changed;
            }
          }
          else if(!own_message(id))
          {
            ++foreign;
          }
          else if(recent_msg_deletions_.find(id) !=
                  recent_msg_deletions_.end())
          {
            ++recently_deleted;
          }
          else
          {
            request_msg = true;
          }

          if(request_msg)
          {
            requested_msg.push_back(id);
          }
        }
      }

      if(Application::will_trace(El::Logging::HIGH))
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::message_sharing_offer: "
             << offered_msg->size() << " message ids received. Present "
             << present << ", changed " << changed << ", foreign " << foreign
             << ", recently deleted " << recently_deleted
             << ", requested " << requested_msg.size();
        
        Application::logger()->trace(ostr.str().c_str(),
                                     Aspect::MSG_SHARING,
                                     El::Logging::HIGH);
      }

      El::Service::CompoundServiceMessage_var msg =
        new SaveMsgSharingInfo(this, offered_messages.release());
      
      deliver_now(msg.in());
    }
        
    ::NewsGate::Search::Result*
    MessageManager::search(const ::NewsGate::Search::Expression* expression,
                           size_t start_from,
                           size_t results_count,
                           const ::NewsGate::Search::Strategy& strategy,
                           const El::Locale& locale,
                           unsigned long long gm_flags,
                           const char* request_category_locale,
                           Categorizer::Category::Locale& category_locale,
                           size_t& total_matched_messages,
                           size_t& suppressed_messages) const
      throw(Exception, El::Exception)
    {
      suppressed_messages = 0;
      
      NewsGate::Search::ResultPtr search_result;

      if(start_from + results_count)
      {
        MgrReadGuard guard(mgr_lock_);

        search_result.reset(expression->search(messages_, false, strategy));
        
        total_matched_messages = search_result->message_infos->size();

        search_result->take_top(start_from,
                                results_count,
                                strategy,
                                &suppressed_messages);

        unsigned long flags = 0;
        uint32_t crc_init = gm_flags & Bank::GM_CORE_WORDS ? dict_hash_ : 0;
        
        if(gm_flags & Bank::GM_STAT)
        {
          flags |= Search::Result::SHF_STAT;
        }
        
        if(gm_flags & Bank::GM_EVENT)
        {
          flags |= Search::Result::SHF_EVENT;
        }
        
        if(gm_flags & Bank::GM_CATEGORIES)
        {
          flags |= Search::Result::SHF_CATEGORY;
        }
        
        if(gm_flags & Bank::GM_PUB_DATE)
        {
          flags |= Search::Result::SHF_UPDATED;
        }
        
        search_result->set_extras(messages_,
                                  flags,
                                  crc_init);

        total_matched_messages -= suppressed_messages;
      }
      else
      {
        Search::Strategy optimized_strategy = strategy;
        optimized_strategy.result_flags &= ~Search::Strategy::RF_MESSAGES;
          
        MgrReadGuard guard(mgr_lock_);
        
        search_result.reset(
          expression->search(messages_, false, optimized_strategy));
        
        total_matched_messages = search_result->stat.total_messages;        
      }

      if((strategy.result_flags & Search::Strategy::RF_CATEGORY_STAT) ||
         *request_category_locale != '\0')
      {
        MessageCategorizer_var categorizer = get_message_categorizer();

        if(categorizer.in())
        {
          if(strategy.result_flags & Search::Strategy::RF_CATEGORY_STAT)
          {
            categorizer->translate(
              search_result->stat.category_counter, locale);
          }

          if(*request_category_locale != '\0')
          {
            categorizer->locale(request_category_locale,
                                locale,
                                category_locale);
          }  
        }
        else
        {
          search_result->stat.category_counter.clear();
        } 
      }

      return search_result.release();
    }

    struct MessagePerWordDistrCounter
    {
      size_t count;
      size_t total_size;

      MessagePerWordDistrCounter() throw();

      void increment(unsigned long size) throw();
    };
    
    MessagePerWordDistrCounter::MessagePerWordDistrCounter() throw()
        : count(0),
          total_size(0)
    {
    }
    
    void
    MessagePerWordDistrCounter::increment(unsigned long size) throw()
    {
      ++count;
      total_size += size;
    }
    
    void
    MessageManager::dump_mem_usage() const throw(El::Exception)
    {
      size_t mem_used = El::Utility::mem_used() -
        Application::instance()->preload_mem_usage();
      
      size_t messages_count = 0;
      
      size_t word_positions_size = 0;
      size_t norm_form_positions_size = 0;
      size_t positions_size = 0;
      size_t positions_stat[101];
      size_t word_position_stat[11];
      
      memset(positions_stat, 0, sizeof(positions_stat));
      memset(word_position_stat, 0, sizeof(word_position_stat));
      
      size_t positions_stat_size =
        sizeof(positions_stat) / sizeof(positions_stat[0]) - 1;
      
      size_t word_position_stat_size =
        sizeof(word_position_stat) / sizeof(word_position_stat[0]) - 1;

      size_t total_word_positions = 0;
      size_t long_messages = 0;

      std::ostringstream ostr;
        
      {
        MgrReadGuard guard(mgr_lock_);
        
        messages_count = messages_.messages.size();
        const StoredMessageMap& messages = messages_.messages;
        
        for(StoredMessageMap::const_iterator it = messages.begin();
            it != messages.end(); it++)
        {
          const StoredMessage& msg = *it->second;

          word_positions_size +=
            msg.word_positions.size() * sizeof(msg.word_positions[0]);
          
          norm_form_positions_size += msg.norm_form_positions.size() *
            sizeof(msg.norm_form_positions[0]);

          positions_size += msg.positions.size() * sizeof(msg.positions[0]);

          positions_stat[msg.positions.size() < positions_stat_size ?
                         msg.positions.size() : positions_stat_size]++;

          bool long_message = false;

          for(size_t i = 0; i < msg.word_positions.size(); i++)
          {
            const WordPositions& wp = msg.word_positions[i].second;  
            size_t wpos_count = wp.position_count();
            
            word_position_stat[wpos_count < word_position_stat_size ?
                               wpos_count : word_position_stat_size]++;

            for(size_t j = 0; j < wpos_count; j++)
            {
              WordPosition p = wp.position(msg.positions, j);
              
              if(p >= 256)
              {
                long_message = true;
              }
            }
          }

          total_word_positions += msg.word_positions.size();

          if(long_message)
          {
            long_messages++;
          }
        }

        float msg_usage = messages_count ?
          (float)mem_used / messages_count : (float)0;

        ostr << "NewsGate::Message::MessageManager::dump_mem_usage:\n"
             << mem_used << " KB used by " << messages_count << " messages ("
             << msg_usage << " per message)\nSearcheableMessageMap stat:";

        messages_.dump(ostr);
      }

      ostr << "\nSharedStringManager info:";
        
      El::String::SharedStringManager::Info info =
        StringConstPtr::string_manager.info();

      info.dump(ostr);      
      El::Utility::dump_mallinfo(ostr);
      
      ostr << "\nLong Messages (positions >= 256): " << long_messages
           << " (" << (messages_count ? (float)long_messages * 100 /
                       messages_count : 0.0)
           << "%)\nStoredMessage::word_positions use "
           << word_positions_size / 1024
           << " KB\nStoredMessage::norm_form_positions use "
           << norm_form_positions_size / 1024
           << " KB\nStoredMessage::positions use "
           << positions_size / 1024<< " KB\nPositions sizes:";

      for(size_t i = 0; i <= positions_stat_size; i++)
      {
        ostr << "\n  ";

        if(i < positions_stat_size)
        {
          ostr << i;
        }
        else
        {
          ostr << "other";
        }
        
        ostr  << " : " << positions_stat[i] << " (" << std::setprecision(2)
              << (messages_count ?
                  (float)positions_stat[i] * 100 / messages_count : 0.0)
              << "%)";
      }

      ostr << "\nWord position sizes:";

      for(size_t i = 0; i <= word_position_stat_size; i++)
      {
        ostr << "\n  ";

        if(i < word_position_stat_size)
        {
          ostr << i;
        }
        else
        {
          ostr << "other";
        }
        
        ostr  << " : " << word_position_stat[i] << " ("
              << std::setprecision(2)
              << (total_word_positions ?
                  (float)word_position_stat[i] * 100 / total_word_positions :
                  0.0) << "%)";
      }

      MessagePerWordDistrCounter msg_per_word_stat[101];
      MessagePerWordDistrCounter msg_per_norm_stat[101];
      
      size_t msg_per_word_stat_size =
        sizeof(msg_per_word_stat) / sizeof(msg_per_word_stat[0]) - 1;
      
      size_t msg_per_norm_stat_size =
        sizeof(msg_per_norm_stat) / sizeof(msg_per_norm_stat[0]) - 1;

      size_t word_msg_refs = 0;
      size_t norm_msg_refs = 0;

      const WordToMessageNumberMap& words = messages_.words;

      for(WordToMessageNumberMap::const_iterator it = words.begin();
          it != words.end(); it++)
      {
        size_t count = it->second->messages.size();
        word_msg_refs += count;

        msg_per_word_stat[count < msg_per_word_stat_size ?
                          count : msg_per_word_stat_size].increment(count);
      }
      
      const WordIdToMessageNumberMap& norm_forms = messages_.norm_forms;

      for(WordIdToMessageNumberMap::const_iterator it = norm_forms.begin();
          it != norm_forms.end(); it++)
      {
        size_t count = it->second->messages.size();
        norm_msg_refs += count;

        msg_per_norm_stat[count < msg_per_norm_stat_size ?
                          count : msg_per_norm_stat_size].increment(count);
      }
      
      ostr << "\nMsg per word distribution (" << word_msg_refs << "):";
      size_t words_count = words.size();

      for(size_t i = 0; i <= msg_per_word_stat_size; i++)
      {
        ostr << "\n  ";

        if(i < msg_per_word_stat_size)
        {
          ostr << i;
        }
        else
        {
          ostr << "other";
        }

        const MessagePerWordDistrCounter& stat = msg_per_word_stat[i];
        
        ostr  << " : " << stat.count << " (" << std::setprecision(2)
              << (words_count ? (float)stat.count * 100 / words_count : 0.0)
              << "%); refs " << stat.total_size << " (" << std::setprecision(2)
              << (word_msg_refs ? (float)stat.total_size * 100 / word_msg_refs
                  : 0.0) << "%)";
      }

      ostr << "\nMsg per norm form distribution (" << norm_msg_refs << "):";
      size_t norm_count = norm_forms.size();

      for(size_t i = 0; i <= msg_per_norm_stat_size; i++)
      {
        ostr << "\n  ";

        if(i < msg_per_norm_stat_size)
        {
          ostr << i;
        }
        else
        {
          ostr << "other";
        }
        
        const MessagePerWordDistrCounter& stat = msg_per_norm_stat[i];

        ostr  << " : " << stat.count << " (" << std::setprecision(2)
              << (norm_count ? (float)stat.count * 100 / norm_count : 0.0)
              << "%); refs " << stat.total_size << " (" << std::setprecision(2)
              << (norm_msg_refs ? (float)stat.total_size * 100 / word_msg_refs
                  : 0.0) << "%)";
      }

      Application::logger()->info(ostr.str(), Aspect::MSG_MANAGEMENT);      
    }
            
    Transport::Response*
    MessageManager::message_stat(Transport::MessageStatRequestInfo* stat)
      throw(El::Exception)
    {
      std::auto_ptr<Transport::MessageStatRequestInfo> msg_stat(stat);
        
      Transport::MessageStatResponseImpl::Var response =
        new Transport::MessageStatResponseImpl::Type(
          new Transport::MessageStatInfoArray());

      if(msg_stat->require_feed_ids)
      {
        Transport::MessageStatInfoArray& stat_infos = response->entities();
        stat_infos.reserve(stat->message_stat.size());

        MgrReadGuard guard(mgr_lock_);
      
        for(Transport::MessageStatArray::const_iterator
              i(stat->message_stat.begin()), e(stat->message_stat.end());
            i != e; ++i)
        {
          const StoredMessage* msg = messages_.find(i->id);

          if(msg && msg->visible())
          {
            stat_infos.push_back(Transport::MessageStatInfo());
        
            Transport::MessageStatInfo& stat_info = *stat_infos.rbegin();
            
            stat_info.id = msg->id;
            stat_info.source_id = msg->source_id;
          }
        }
      }
      
      El::Service::CompoundServiceMessage_var msg =
        new SaveMsgStat(this, msg_stat.release());
      
      deliver_now(msg.in());
      
      return response._retn();
    }

    void
    MessageManager::save_message_stat(
      Transport::MessageStatRequestInfo* stat_val)
      throw(El::Exception)
    {
      std::auto_ptr<Transport::MessageStatRequestInfo> stat(stat_val);
        
      MessageSinkMap message_sink;
      
      message_sink = callback_->message_sink_map(
        stat_val->require_feed_ids ? MSRT_OWN_MESSAGE_STAT :
        MSRT_SHARED_MESSAGE_STAT);

      bool share = !message_sink.empty();

      const Transport::EventStatArray& event_stat = stat->event_stat;
      
      {
        MgrWriteGuard guard(mgr_lock_);

        if(flushed_)
        {
          return;
        }
        
        for(Transport::MessageStatArray::iterator
              i(stat->message_stat.begin()), e(stat->message_stat.end());
            i != e; )
        {
          const Transport::MessageStat& st = *i;
          StoredMessage* msg = messages_.find(st.id);

          if(msg)
          {
            if(msg->visible())
            {
              messages_.set_impressions(*msg,
                                        msg->impressions + st.impressions);
              
              messages_.set_clicks(*msg, msg->clicks + st.clicks);
              messages_.calc_search_weight(*msg);

              msg->visited = std::max(st.visited, msg->visited);              
              msg->flags |= StoredMessage::MF_DIRTY;
            }
            else
            {
              msg = 0;
            }
          }

          if(msg == 0 && share)
          {
            i = stat->message_stat.erase(i);
            e = stat->message_stat.end();
          }
          else
          {
            ++i;
          }
        }

        if(!event_stat.empty())
        {
          const StoredMessageMap& messages = messages_.messages;
        
          const EventToNumberMap& events = messages_.event_to_number;
          EventToNumberMap::const_iterator eit_end = events.end();

          for(Transport::EventStatArray::const_iterator
                i(event_stat.begin()), e(event_stat.end()); i != e; ++i)
          {
            const Transport::EventStat& st = *i;
            EventToNumberMap::const_iterator eit = events.find(st.id);

            if(eit != eit_end)
            {
              uint64_t visited = st.visited;
              const NumberSet& numbers = *eit->second;

              for(NumberSet::const_iterator nit(numbers.begin()),
                    nit_end(numbers.end()); nit != nit_end; ++nit)
              {
                StoredMessage& msg =
                  const_cast<StoredMessage&>(*messages.find(*nit)->second);
              
                if(msg.visible())
                {
                  msg.visited = std::max(visited, msg.visited);
                  msg.flags |= StoredMessage::MF_DIRTY;
                }
              }
            } 
          }
        }
      }

      if(share)
      {
        stat->require_feed_ids = false;
        
        Message::Transport::MessageStatRequestImpl::Var
          request = Message::Transport::
          MessageStatRequestImpl::Init::create(stat.release());

        request->serialize();
        
        for(MessageSinkMap::iterator it = message_sink.begin();
            it != message_sink.end(); it++)
        {
          MessageSink& sink = it->second;
            
          try
          {
            Message::Transport::Response_var response;
            
            Message::BankClientSession::RequestResult_var result =
              sink.message_sink->send_request(request.in(), response.out());
          }
          catch(const ImplementationException& e)
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Message::MessageManager::save_message_stat: "
              "ImplementationException caught. Description:" << std::endl
                 << e.description.in();
        
            El::Service::Error error(ostr.str(), this);
            callback_->notify(&error);
          }
          catch(const CORBA::Exception& e)
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Message::MessageManager::save_message_stat: "
              "CORBA::Exception caught. Description:" << std::endl << e;
        
            El::Service::Error error(ostr.str(), this);
            callback_->notify(&error);
          }
        }
      }
    }
    
    Transport::Response*
    MessageManager::get_message_digests(Transport::EventIdArray& event_ids)
      const throw(El::Exception)
    {
      Transport::MessageDigestResponseImpl::Var response =
        new Transport::MessageDigestResponseImpl::Type(
          new Transport::MessageDigestArray());

      Transport::MessageDigestArray& digests = response->entities();
      digests.reserve(event_ids.size() * 10);
      
      MgrReadGuard guard(mgr_lock_);

      const EventToNumberMap& events = messages_.event_to_number;
      const StoredMessageMap& messages = messages_.messages;
      
      for(Transport::EventIdArray::const_iterator it = event_ids.begin();
          it != event_ids.end(); it++)
      {
        const El::Luid& event_id = *it;
        EventToNumberMap::const_iterator eit = events.find(event_id);

        if(eit != events.end())
        {
          const NumberSet& numbers = *eit->second;

          for(NumberSet::const_iterator nit = numbers.begin();
              nit != numbers.end(); nit++)
          {
            const StoredMessage& msg = *messages.find(*nit)->second;

            if(msg.visible())
            {
              digests.push_back(Transport::MessageDigest());            
              Transport::MessageDigest& digest = *digests.rbegin();
              
              digest.event_id = event_id;
              digest.message_id = msg.id;
              digest.core_words = msg.core_words;
            }
          }
        }
        
      }
      
      return response._retn();
    }

    Transport::Response*
    MessageManager::check_mirrored_messages(IdArray& message_ids,
                                            bool ready) const
      throw(El::Exception)
    {
      Transport::CheckMirroredMessagesResponseImpl::Var response =
          Transport::CheckMirroredMessagesResponseImpl::Init::create(
            new IdArray());

      IdArray& found_ids = response->entities();
      found_ids.reserve(message_ids.size());

      MgrReadGuard guard(mgr_lock_);

      if(!loaded_)
      {
        ready = false;
      }

      for(IdArray::const_iterator i(message_ids.begin()), e(message_ids.end());
          i != e; ++i)
      {
        const Id& id = *i;
        
        if((ready && messages_.find(id)) || (!ready && own_message(id)) ||
           recent_msg_deletions_.find(id) != recent_msg_deletions_.end())
        {
          found_ids.push_back(id);
        }
      }

      return response._retn();
    }

    Transport::StoredMessageArray*
    MessageManager::get_messages(const IdArray& ids,
                                 uint64_t gm_flags,
                                 int32_t img_index,
                                 int32_t thumb_index,
                                 IdArray& notfound_msg_ids)
      throw(El::Exception)
    {
      Transport::StoredMessageArrayPtr result(
        new Transport::StoredMessageArray());
      
      result->reserve(ids.size());

      IdArray load_ids;
      time_t timestamp = 0;

      bool get_content =
        gm_flags & (Bank::GM_TITLE | Bank::GM_DESC | Bank::GM_IMG |
                    Bank::GM_KEYWORDS | Bank::GM_SOURCE | Bank::GM_LINK);

      bool get_positions =
        gm_flags & (Bank::GM_TITLE | Bank::GM_DESC | Bank::GM_IMG |
                    Bank::GM_KEYWORDS | Bank::GM_CORE_WORDS |
                    Bank::GM_DEBUG_INFO);

      bool get_word_positions =
        gm_flags & (Bank::GM_TITLE | Bank::GM_DESC | Bank::GM_IMG |
                    Bank::GM_KEYWORDS | Bank::GM_CORE_WORDS |
                    Bank::GM_DEBUG_INFO);

      bool get_norm_form_positions =
        gm_flags & (Bank::GM_TITLE | Bank::GM_DESC | Bank::GM_IMG |
                    Bank::GM_KEYWORDS | Bank::GM_CORE_WORDS |
                    Bank::GM_DEBUG_INFO);
        
      bool get_core_words =
        gm_flags & (Bank::GM_CORE_WORDS | Bank::GM_DEBUG_INFO);

      bool get_source_url =
        gm_flags & (Bank::GM_SOURCE | Bank::GM_DEBUG_INFO |
                    Bank::GM_EXTRA_MSG_INFO);
        
      bool get_source_title =
        gm_flags & (Bank::GM_SOURCE | Bank::GM_DEBUG_INFO);
        
      bool get_thumb = gm_flags & Bank::GM_IMG_THUMB;

      bool get_categories = gm_flags & Bank::GM_CATEGORIES;

      {
        MgrReadGuard guard(mgr_lock_);

        for(IdArray::const_iterator it = ids.begin(); it != ids.end(); it++)
        {
          const Id& id = *it;
          const StoredMessage* stored_msg = messages_.find(id);
        
          if(stored_msg == 0 || stored_msg->hidden())
          {
            notfound_msg_ids.push_back(id);
            continue;
          }

          result->push_back(Transport::StoredMessageDebug());
          StoredMessage& msg = result->rbegin()->message;

          msg.id = stored_msg->id;
          msg.description_pos = stored_msg->description_pos;
          msg.img_alt_pos = stored_msg->img_alt_pos;
          msg.keywords_pos = stored_msg->keywords_pos;
          msg.event_id = stored_msg->event_id;
          msg.event_capacity = stored_msg->event_capacity;
          msg.flags = stored_msg->flags;
          msg.impressions = stored_msg->impressions;
          msg.clicks = stored_msg->clicks;
          msg.published = stored_msg->published;
          msg.fetched = stored_msg->fetched;
          msg.visited = stored_msg->visited;
          msg.signature = stored_msg->signature;
          msg.url_signature = stored_msg->url_signature;
          msg.source_id = stored_msg->source_id;
          msg.space = stored_msg->space;
          msg.country = stored_msg->country;
          msg.lang = stored_msg->lang;

          if(get_source_title)
          {
            msg.source_title = stored_msg->source_title;
          }

          if(get_categories)
          {
            msg.categories = stored_msg->categories;
          }

          if(get_word_positions)
          {
            msg.word_positions = stored_msg->word_positions;
          }
          
          if(get_norm_form_positions)
          {
            msg.norm_form_positions = stored_msg->norm_form_positions;
          }
          
          if(get_positions)
          {
            msg.positions = stored_msg->positions;
          }

          if(get_core_words)
          {
            msg.core_words = stored_msg->core_words;
          }

          if(get_source_url)
          {
            msg.set_source_url(stored_msg->source_url.c_str());
          }

          if(get_content || get_thumb)
          {
            msg.content = stored_msg->content;
            
            if(stored_msg->content.in() == 0)
            {
              load_ids.push_back(id);              
            }
            else if(get_content)
            {
              if(timestamp == 0)
              {
                timestamp = ACE_OS::gettimeofday().sec();
              }

              stored_msg->content->timestamp(timestamp);
            }
          }
          else
          {
            msg.content = new StoredContent();
          }
          
        }
      }

      if(!load_ids.empty())
      {
        load_msg_content(load_ids, get_content, *result, notfound_msg_ids);
        flush_content_cache(true);
      }

      if(gm_flags & (Bank::GM_DEBUG_INFO | Bank::GM_EXTRA_MSG_INFO))
      {
        fill_debug_info(*result, gm_flags);
      }

      if(get_thumb)
      {
        load_img_thumb("get_messages", *result, img_index, thumb_index);
      }
      
      return result.release();
    }

    void
    MessageManager::load_img_thumb(const char* context,
                                   Transport::StoredMessageArray& result,
                                   int32_t img_index,
                                   int32_t thumb_index)
      throw(El::Exception)
    {
      if(result.empty())
      {
        return;
      }
      
      ACE_High_Res_Timer timer;

      if(Application::will_trace(El::Logging::HIGH))
      {
        timer.start();
      }

      size_t loaded_img = 0;
      size_t loaded_msg = 0;

      for(Transport::StoredMessageArray::iterator it =
            result.begin(); it != result.end(); it++)
      {
        if((it->message.flags & StoredMessage::MF_HAS_THUMBS) == 0)
        {
          continue;
        }

        ++loaded_msg;
        
        StoredContent_var content = new StoredContent();

        // Make a copy not to hold image thumbnail as a part of stored messages
        content->copy(*it->message.content);
        it->message.content = content;

        assert(content->images.get() != 0);

        try
        {
          it->message.read_image_thumbs(config_.thumbnail_dir().c_str(),
                                        img_index,
                                        thumb_index,
                                        &loaded_img);
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "MessageManager::load_img_thumb: read_image_thumbs failed ("
               << context << "). Reason:\n" << e;
          
          El::Service::Error
            error(ostr.str(), this, El::Service::Error::NOTICE);
          
          callback_->notify(&error);
        }        
      }

      if(Application::will_trace(El::Logging::HIGH))
      {
        timer.stop();
        ACE_Time_Value tm;
        timer.elapsed_time(tm);
            
        std::ostringstream ostr;
        ostr << "MessageManager::load_img_thumb: " << loaded_img
             << " images for " << loaded_msg << " messages of "
             << result.size() << " loaded (" << context
             << "); time " << El::Moment::time(tm);

        Application::logger()->trace(ostr.str(),
                                     Aspect::DB_PERFORMANCE,
                                     El::Logging::HIGH);
      }
    }
    
    void
    MessageManager::flush_content_cache(bool skip_if_busy) throw(El::Exception)
    {
      int r = skip_if_busy ?
        mgr_lock_.tryacquire_write() : mgr_lock_.acquire_write();

      if(r < 0)
      {
        int error = errno;
        
        if(!skip_if_busy || error != EBUSY)
        {
          std::ostringstream ostr;
          
          ostr << "NewsGate::Message::MessageManager::flush_content_cache: "
               << (skip_if_busy ? "tryacquire_write" : "acquire_write")
               << " failed. Errno " << error;
          
          El::Service::Error
            error(ostr.str(), this, El::Service::Error::ALERT);
          
          callback_->notify(&error);
        }
        
        return;
      }

      try
      {
        ContentCache::StoredContentMapPtr contents(content_cache_.flush());

        if(contents.get() == 0)
        {
          mgr_lock_.release();
          return;
        }
      
        const IdToNumberMap& id_to_number = messages_.id_to_number;
        StoredMessageMap& messages = messages_.messages;
        
        for(ContentCache::StoredContentMap::iterator it = contents->begin();
            it != contents->end(); it++)
        {
          IdToNumberMap::const_iterator iit = id_to_number.find(it->first);
          
          if(iit != id_to_number.end())
          {
            StoredMessageMap::iterator mit = messages.find(iit->second);
            
            if(mit != messages.end())
            {
              StoredMessage* msg = const_cast<StoredMessage*>(mit->second);

              if(msg->visible())
              {
                msg->content = it->second.retn();
              }
            }
          }
        }
      }
      catch(...)
      {
        mgr_lock_.release();
        throw;
      }

      mgr_lock_.release();
    }
    
    void
    MessageManager::load_msg_content(const IdArray& ids,
                                     bool update_storage,
                                     Transport::StoredMessageArray& result,
                                     IdArray& notfound_msg_ids)
      throw(El::Exception)
    {
      ContentCache::StoredContentMapPtr contents(
        content_cache_.get(ids, update_storage));        

      for(Transport::StoredMessageArray::iterator it =
            result.begin(); it != result.end(); )
      {
        if(it->message.content.in() != 0)
        {
          it++;
          continue;
        }
        
        ContentCache::StoredContentMap::iterator cit =
          contents->find(it->get_id());

        if(cit == contents->end())
        { 
          std::ostringstream ostr;
            
          ostr << "NewsGate::Message::MessageManager::load_msg_content: "
            "message with id=" << it->get_id().string()
               << " can't be found in DB (signature="<< it->message.signature
               << ", url_signature="<< it->message.url_signature
               << ", published=" << it->message.published << ", source="
               << it->message.source_url.c_str() << ")";
        
          El::Service::Error
            error(ostr.str(), this, El::Service::Error::ALERT);
            
          callback_->notify(&error);
            
          notfound_msg_ids.push_back(it->get_id());
          it = result.erase(it);
        }
        else
        {
          it->message.content = cit->second;
          it++;
        }
      }
    }
    
    void
    MessageManager::fill_debug_info(
      Transport::StoredMessageArray& message,
      uint64_t gm_flags) const
      throw(El::Exception)
    {
      MgrReadGuard guard(mgr_lock_);

      const Message::FeedInfoMap& feeds = messages_.feeds;
      
      for(Transport::StoredMessageArray::iterator
            i(message.begin()), e(message.end()); i != e; ++i)
      {
        Transport::DebugInfo& di(i->debug_info);
        const StoredMessage& msg(i->message);
          
        di.on = 1;

        if(gm_flags & Bank::GM_DEBUG_INFO)
        {
          messages_.calc_words_freq(msg, false, di.words_freq);
          messages_.calc_word_pairs_freq(msg, di.words_freq);
        }
      
        Message::FeedInfoMap::const_iterator fi =
          feeds.find(msg.source_url.c_str());

        if(fi != feeds.end())
        {
          di.feed_impressions = fi->second->impressions;
          di.feed_clicks = fi->second->clicks;
        }
      } 
    }
        
    void
    MessageManager::traverse_messages(bool prio_msg) throw(El::Exception)
    {
      if(prio_msg)
      { 
        MgrReadGuard guard(mgr_lock_);
        
        if(traverse_prio_message_it_ == traverse_prio_message_.end())
        {
          return;
        }
      }

      const char* context = prio_msg ? "prio traverse" : "regular traverse";
      
      MessageCategorizer_var categorizer = get_message_categorizer();
        
      MessageOperationList operations;

      ACE_Time_Value current_time = ACE_OS::gettimeofday();
      time_t cur_time = current_time.sec();
          
      time_t expire_time =
        cur_time > (time_t)config_.message_expiration_time() ?
        cur_time - config_.message_expiration_time() : 0;
          
      time_t preempt_time =
        cur_time > (time_t)config_.message_cache().timeout() ?
        cur_time - config_.message_cache().timeout() : 0;

      //
      // next_sharing_time_ is not locked as read/written from
      // single thread
      //
      bool to_share_messages =
        !prio_msg && next_sharing_time_ == ACE_Time_Value::zero;

      Event::BankClientSession_var event_bank_client_session;
        
      std::auto_ptr<std::ostringstream> log_ostr;

      Transport::StoredMessageArrayPtr move_messages;
      std::auto_ptr<IdToSizeMap> move_messages_mapping;
          
      std::auto_ptr<std::ostringstream> load_msg_content_request_ostr;

      Transport::MessageSharingInfoPackImpl::Var messages_to_share;
      Event::Transport::MessageDigestPackImpl::Var message_digests;

      IdTimeMap to_categorize;

      MessageSinkMap message_sink =
        callback_->message_sink_map(MSRT_OLD_MESSAGES);

      bool can_share = !message_sink.empty();

      if(to_share_messages)
      {
        if(can_share)
        {
          messages_to_share =
            Transport::MessageSharingInfoPackImpl::Init::create(
              new Transport::MessageSharingInfoArray());
        }

        event_bank_client_session =
          Application::instance()->event_bank_client_session();

        if(event_bank_client_session.in() != 0)
        {
          message_digests =
            Event::Transport::MessageDigestPackImpl::Init::create(
              new Event::Transport::MessageDigestArray());
        }
      }

      size_t foreign_msg_db_query_max_count =
        config_.message_cache().foreign_msg_db_query_max_count();
          
      size_t foreign_msg_db_query_count = 0;

      BankClientSession_var mirrored_banks;
      std::string mirrored_sharing_id;
          
      Transport::CheckMirroredMessagesRequestImpl::Var
        mirrored_messages_to_check;
            
      { 
        MgrReadGuard guard(mgr_lock_);

        StoredMessageMap& messages = messages_.messages;

        size_t traverse_records =
          std::min((size_t)config_.message_cache().traverse_records(),
                   messages.size());

        if(to_share_messages && session_->mirror() &&
           mirrored_banks_.in() != 0)
        {
          mirrored_banks = mirrored_banks_;
          mirrored_sharing_id = mirrored_sharing_id_;
              
          mirrored_messages_to_check =
            Transport::CheckMirroredMessagesRequestImpl::Init::create(
              new IdArray());

          mirrored_messages_to_check->entities().reserve(traverse_records);
        }

        double shared_msg_threshold = traverse_records ?
          (double)config_.message_cache().shared_msg_max_count() /
          traverse_records : 1.0;
        
        NumberSet::const_iterator& msg_it =
          prio_msg ? traverse_prio_message_it_ : traverse_message_it_;        
        
        NumberSet::const_iterator msg_end = prio_msg ?
          traverse_prio_message_.end() : traverse_message_.end();
        
        for(size_t i = 0; i < traverse_records && msg_it != msg_end; ++i)
        {
          Number number = *msg_it++;
                
          StoredMessageMap::const_iterator it = messages.find(number);
              
          if(it == messages.end())
          {
            // Can happen due to message filtering out by fetch filter
            continue;
          }
              
          const StoredMessage& msg = *it->second;

          if(msg.hidden())
          {
            continue;
          }

          if((time_t)msg.published <= expire_time)
          {
            operations.push_back(std::make_pair(msg.id, MO_DELETE));
          }
          else if(!own_message(msg.id) &&
                  (msg.content.in() != 0 || foreign_msg_db_query_count <
                   foreign_msg_db_query_max_count))
          {
            if(move_messages.get() == 0)
            {
              size_t reserve_count =
                config_.message_cache().traverse_records() - i;
                  
              move_messages.reset(
                new Transport::StoredMessageArray());
                  
              move_messages->reserve(reserve_count);
            }

            if(msg.content.in() == 0)
            {
              ContentCache::query_stored_content(
                msg.id,
                load_msg_content_request_ostr);

              ++foreign_msg_db_query_count;

              if(move_messages_mapping.get() == 0)
              {
                size_t reserve_count =
                  config_.message_cache().traverse_records() - i;
                    
                move_messages_mapping.reset(new IdToSizeMap());
                move_messages_mapping->resize(reserve_count);
              }
            }
                
            move_messages->push_back(Transport::StoredMessageDebug(msg));
          }
          else
          {  
            if(msg.content.in() != 0 &&
               (time_t)msg.content->timestamp() < preempt_time)
            {
              operations.push_back(std::make_pair(msg.id, MO_PREEMPT));
            }

            if(msg.flags & StoredMessage::MF_DIRTY)
            {
              operations.push_back(std::make_pair(msg.id, MO_FLUSH_STATE));
            }

            if(categorizer.in() &&
               categorizer->hash != msg.categories.categorizer_hash)
            {
              to_categorize.insert(std::make_pair(msg.id, msg.published));
            }
                
            if(messages_to_share.in())
            {
              bool share = true;

              if(shared_msg_threshold < 1)
              {
                double val = (double)rand() / ((double)RAND_MAX + 1);
                share = val < shared_msg_threshold;
              }

              if(share)
              {
                messages_to_share->entities().push_back(
                  Transport::MessageSharingInfo(msg.id,
                                                msg.event_id,
                                                msg.event_capacity,
                                                msg.impressions,
                                                msg.clicks,
                                                msg.visited,
                                                msg.categories,
                                                msg.word_hash(true)));
              }
            }

            if(mirrored_messages_to_check.in())
            {
              mirrored_messages_to_check->entities().push_back(msg.id);
            }
                
            if(message_digests.in() != 0)
            {
              message_digests->entities().push_back(
                Event::Transport::MessageDigest(msg.id,
                                                msg.published,
                                                msg.lang,
                                                msg.core_words,
                                                msg.event_id));
            }
          }
        }

        if(Application::will_trace(El::Logging::HIGH))
        {
          log_ostr.reset(new std::ostringstream());
          
          *log_ostr <<
            "NewsGate::Message::MessageManager::traverse_cache: "
                    << context << " end " << (msg_it == msg_end ?
                                              "" : "not ") << "reached\n";
        }
      }

      categorize_and_save(categorizer.in(),
                          to_categorize,
                          false,
                          can_share ? &messages_to_share : 0);
          
      if(message_digests.in() != 0)
      {
        post_digests(event_bank_client_session.in(), message_digests.in());
      }

      if(messages_to_share.in() != 0)
      {
        share_requested_messages(messages_to_share.in());
      }

      if(mirrored_messages_to_check.in() != 0)
      {
        check_mirrored_messages(mirrored_banks.in(),
                                mirrored_sharing_id.c_str(),
                                mirrored_messages_to_check.in());
      }

      exec_msg_operations(operations,
                          cur_time,
                          preempt_time,
                          expire_time,
                          *log_ostr);

      if(Application::will_trace(El::Logging::HIGH))
      {
        Application::logger()->trace(log_ostr->str(),
                                     Aspect::MSG_MANAGEMENT,
                                     El::Logging::HIGH);
      }

      if(load_msg_content_request_ostr.get())
      {
        *load_msg_content_request_ostr << " )";
      }

      if(move_messages.get())
      {
        move_messages_to_owners(
          move_messages,
          move_messages_mapping.get(),
          load_msg_content_request_ostr.get() != 0 ?
          load_msg_content_request_ostr->str().c_str() : 0);
      }
    }

    void
    MessageManager::traverse_cache() throw()
    {
      try
      {
        flush_content_cache(false);

        try
        {
          traverse_messages(false);
          traverse_messages(true);
          
          if(traverse_message_it_ == traverse_message_.end())
          {
            //
            // End of message cache reached
            //

            if(!traverse_message_.empty())
            {
              bool to_share_messages =
                next_sharing_time_ == ACE_Time_Value::zero;
              
              if(to_share_messages)
              {
                next_sharing_time_ = ACE_OS::gettimeofday().sec() +
                  config_.message_cache().share_messages_period();
              }
              else if(ACE_OS::gettimeofday() >= next_sharing_time_)
              {
                next_sharing_time_ = ACE_Time_Value::zero;
              }
            }

            StoredMessage::cleanup_thumb_files(
              config_.thumbnail_dir().c_str(),
              config_.message_expiration_time());

            size_t preoptimize_mem = El::Utility::mem_used();        
            ACE_Time_Value optimize_mem_usage_tm;

            ACE_Time_Value current_time = ACE_OS::gettimeofday();
                
            uint64_t optimize_mem_period =
              config_.message_cache().optimize_mem_period();
            
            bool optimize_mem =
              optimize_mem_period && optimize_mem_time_ <= current_time;

            size_t word_pair_count = 0;
            size_t rare_word_pair_count = 0;
            
            {
              ACE_High_Res_Timer timer;

              uint64_t threshold_time = current_time.sec() -
                config_.message_cache().keep_recently_deleted();
              
              MgrWriteGuard guard(mgr_lock_); 

              timer.start();

              if(optimize_mem)
              {
                messages_.optimize_mem_usage();

                if(!word_pair_managers_.get())
                {                  
                  for(LTWPCounter::iterator i(ltwp_counter_.begin()),
                        e(ltwp_counter_.end()); i != e; ++i)
                  {
                    WordPairCounter& wpc = *i->second;
                    wpc.resize(0);
                    word_pair_count += wpc.size();
                  }

                  ltwp_counter_.resize(0);

                  for(LTWPSet::iterator i(ltwp_set_.begin()),
                        e(ltwp_set_.end()); i != e; ++i)
                  {
                    WordPairSet& wps = *i->second;                    
                    wps.resize(0);
                    rare_word_pair_count += wps.size();
                  }

                  ltwp_set_.resize(0);
                }
                
                optimize_mem_time_ =
                  ACE_Time_Value(current_time.sec() + optimize_mem_period);
              }
              
              timer.stop();
              timer.elapsed_time(optimize_mem_usage_tm);

              for(IdTimeMap::iterator i(recent_msg_deletions_.begin()),
                  e(recent_msg_deletions_.end()); i != e; ++i)
              {
                if(i->second <= threshold_time)
                {
                  recent_msg_deletions_.erase(i);
                }
              }
            }
              
            size_t postoptimize_mem = El::Utility::mem_used();
            size_t cached_messages = 0;

            {
              // Can modify traverse_message_ without lock because
              // traverse_message_ is acessed from a single thread

              traverse_message_.clear();
              
              MgrReadGuard guard(mgr_lock_);

              StoredMessageMap& messages = messages_.messages;
              traverse_message_.resize(messages.size());
                
              for(StoredMessageMap::const_iterator it =
                    messages.begin(); it != messages.end(); it++)
              {
                const StoredMessage* msg = it->second;

                if(msg->visible())
                {
                  traverse_message_.insert(it->first);

                  if(msg->content.in() != 0)
                  {
                    cached_messages++;
                  }
                }
              }

              traverse_message_it_ = traverse_message_.begin();
            }

            if(Application::will_trace(El::Logging::HIGH))
            {
              std::ostringstream ostr;

              ostr << "NewsGate::Message::MessageManager::traverse_cache: "
                   << "\nRecently deleted messages: "
                   << recent_msg_deletions_.size()
                   << "\nPending message delete notifications: "
                   << del_msg_notifications_.size();

              if(optimize_mem)
              {
                ostr << "\nOptimize Mem Usage: " << preoptimize_mem
                     << " - " << postoptimize_mem << " KB (";
              
                MgrReadGuard guard(mgr_lock_);

                unsigned long messages = messages_.messages.size();
                
                ostr << (messages ?
                         (float)(preoptimize_mem - preload_mem_usage_) /
                         messages : 0.0) << " - "
                     << (messages ?
                         (float)(postoptimize_mem - preload_mem_usage_) /
                         messages : 0.0) << " per message); time: "
                     << El::Moment::time(optimize_mem_usage_tm)
                     << "\nSearcheableMessageMap stat:";
                
                messages_.dump(ostr);

                ostr << "\nWord Pairs: " << word_pair_count << " in "
                     << ltwp_counter_.size() << " lang/time buckets; "
                     << rare_word_pair_count << " rare wp in "
                     << ltwp_set_.size() << " lang/time buckets";
              }

              ostr << "\nCached messages: " << cached_messages;

              ostr << std::endl;
              StoredMessage::object_counter.dump(ostr);

              ostr << std::endl;
              NumberSet::object_counter.dump(ostr);

              ostr  << "\nSharedStringManager info:";
/*
              El::String::SharedStringManager::Info info =
                StringConstPtr::string_manager.info();
              
              info.dump(ostr);
*/
              StringConstPtr::string_manager.dump(ostr); //, 20
              El::Utility::dump_mallinfo(ostr);
              
              Application::logger()->trace(ostr.str(),
                                           Aspect::MSG_MANAGEMENT,
                                           El::Logging::HIGH);
            }
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::traverse_cache: "
            "El::Exception caught. Description:" << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }

        try
        {
          El::Service::CompoundServiceMessage_var msg =
            new TraverseCache(this);
          
          ACE_Time_Value delay(config_.message_cache().traverse_period());

          deliver_at_time(msg.in(), ACE_OS::gettimeofday() + delay);
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::traverse_cache: "
            "El::Exception caught while scheduling task. Description:"
               << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
      }
      catch(...)
      {
        El::Service::Error error(
          "NewsGate::Message::MessageManager::load_messages: "
          "unexpected exception caught.",
          this);
          
        callback_->notify(&error);
      }      
    }
    
    MessageManager::MessageFetchFilterMap*
    MessageManager::get_message_fetch_filters() const throw()
    {
      MessageFetchFilterMap_var result = new MessageFetchFilterMap();
      
      MgrReadGuard guard(mgr_lock_);
      
      if(message_filters_.in() == 0)
      {
        return 0;
      }

      *result = *message_filters_.in();
      return result.retn();
    }

    MessageCategorizer*
    MessageManager::get_message_categorizer() const throw()
    {
      MgrReadGuard guard(mgr_lock_);
      return El::RefCount::add_ref(message_categorizer_.in());      
    }
    
    MessageManager::AssignResult
    MessageManager::set_message_fetch_filter(const FetchFilter& filter)
      throw(Exception, El::Exception)
    {
      AssignResult res = assign_message_fetch_filter(filter);
        
      if(res == AR_SUCCESS)
      {
        El::Service::CompoundServiceMessage_var msg =
          new ApplyMsgFetchFilters(this);
        
        deliver_now(msg.in());
      }

      return res;
    }

    void
    MessageManager::reapply_message_fetch_filters()
      throw(Exception, El::Exception)
    {
      apply_message_fetch_filters();

      El::Service::CompoundServiceMessage_var msg =
        new ReapplyMsgFetchFilters(this);
      
      deliver_at_time(msg.in(),
                      ACE_OS::gettimeofday() +
                      ACE_Time_Value(
                        config_.message_filter().reapply_period()));
      
    }
    
    void
    MessageManager::apply_message_fetch_filters()
      throw(Exception, El::Exception)
    {
      IdTimeMap removed_msg;
      MessageFetchFilterMap_var filters = get_message_fetch_filters();

      {
        MgrWriteGuard guard(mgr_lock_);
        
        apply_message_fetch_filters(
          messages_,
          filters.in(),
          capacity_filter_.in(),
          removed_msg,
          config_.message_cache().delete_message_pack());

        recent_msg_deletions(removed_msg);        
      }

      if(!removed_msg.empty())
      {
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();
        
        delete_messages(removed_msg, connection.in());

        if(removed_msg.size() >= config_.message_cache().delete_message_pack())
        {
          El::Service::CompoundServiceMessage_var msg =
            new ApplyMsgFetchFilters(this);
          
          deliver_now(msg.in());
        }
      }
    }

    bool
    MessageManager::reset_categorizer(const std::string& new_cat_source,
                                      uint64_t new_cat_stamp,
                                      bool enforce) throw()
    {
      if(message_categorizer_.in() == 0 || enforce)
      {
        return true;
      }
      
      uint64_t cur_cat_stamp = message_categorizer_->stamp;

      if(!cur_cat_stamp)
      {
        // Current categorizer is loaded from cache need to replace it anyway.
        // Otherwise it can stick forever in case something have been
        // reconfigured, resored from backup ....
        return true;
      }
      
      const std::string& cur_cat_source = message_categorizer_->source;
          
      if(!new_cat_source.empty() && cur_cat_source.empty())
      {
        // Prefer own caterorizer (distinguished by empty source)
        return false;
      }
      else if(!new_cat_source.empty() && !cur_cat_source.empty())
      {
        if(new_cat_source != cur_cat_source ||
           cur_cat_stamp >= new_cat_stamp)
        {
          // Maintain fresh categorizer from single external source
          return false;
        }
      }
      else if(new_cat_source.empty() && cur_cat_source.empty())
      {
        if(cur_cat_stamp >= new_cat_stamp)
        {
          // Maintain fresh own categorizer
          return false;
        }
      }
      else
      {
        // When new is own while current is external -
        // always prefer new own
      }

      return true;
    }
    
    MessageManager::AssignResult
    MessageManager::assign_message_categorizer(const Categorizer& categorizer)
      throw(Exception, El::Exception)
    {
      {
        MgrReadGuard guard(mgr_lock_);

        if(!reset_categorizer(categorizer.source,
                              categorizer.stamp,
                              categorizer.enforced))
        {
          return AR_NOT_CHANGED;
        }
      }

      NewsGate::Segmentation::Segmentor_var segmentor =
        Application::instance()->segmentor();
      
      Dictionary::WordManager_var word_manager =
        Application::instance()->word_manager();
        
      MessageCategorizer_var message_categorizer =
        new MessageCategorizer(config_.message_categorizer());

      const Categorizer::CategoryMap& src_categories = categorizer.categories;

      IdSet categorize_msg_ids;
        
      try
      {
        for(Categorizer::CategoryMap::const_iterator
              i(src_categories.begin()), e(src_categories.end()); i != e; ++i)
        {
          const Categorizer::Category& src_category = i->second;
        
          MessageCategorizer::Category& dest_category =
            message_categorizer->categories.insert(
              std::make_pair(i->first, MessageCategorizer::Category())).
            first->second;

          dest_category.name = src_category.name.c_str();
          dest_category.searcheable = src_category.searcheable;
          dest_category.children = src_category.children;
          dest_category.locales = src_category.locales;
          
          Search::Or* or_cond = new Search::Or();
          dest_category.condition = or_cond;
        
          const Categorizer::Category::ExpressionArray& expressions =
            src_category.expressions;
        
          for(Categorizer::Category::ExpressionArray::const_iterator
                i(expressions.begin()), e(expressions.end()); i != e; ++i)
          {
            or_cond->operands.push_back(
              segment_search_expression(i->c_str()));            
          }

          if(!normalize_search_condition(dest_category.condition))
          {
            return AR_NOT_READY;
          }

          Search::Condition_var msg_cond =
            category_message_condition(src_category.excluded_messages,
                                       categorize_msg_ids);

          if(msg_cond.in())
          {
            Search::Except* ex_cond = new Search::Except();
            ex_cond->left = dest_category.condition.retn();
            ex_cond->right = msg_cond.retn();
            dest_category.condition = ex_cond;
          }
          
          msg_cond =
            category_message_condition(src_category.included_messages,
                                       categorize_msg_ids);

          if(msg_cond.in())
          {
            Search::Or* or_cond = new Search::Or();
            Search::Condition_var cond = or_cond;
            or_cond->operands.push_back(dest_category.condition.retn());
            or_cond->operands.push_back(msg_cond.retn());
            dest_category.condition = cond.retn();
          }          
        }
        
        message_categorizer->dict_hash = dictionary_hash();
      }
      catch(const WordManagerNotReady& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::"
          "assign_message_categorizer: WordManagerNotReady caught. "
          "Description:\n" << e;
        
        El::Service::Error
          error(ostr.str(), this, El::Service::Error::NOTICE);
        
        callback_->notify(&error);
        return AR_NOT_READY;
      }
      catch(const SegmentorNotReady& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::"
          "assign_message_categorizer: SegmentorNotReady caught. "
          "Description:\n" << e;
        
        El::Service::Error
          error(ostr.str(), this, El::Service::Error::NOTICE);
        
        callback_->notify(&error);
        return AR_NOT_READY;
      }
      
      message_categorizer->stamp = categorizer.stamp;
      message_categorizer->source = categorizer.source;
      message_categorizer->hash = 0; //El::CRC32_init();
      
      {
        std::ostringstream ostr;
        El::BinaryOutStream bstr(ostr);

        bstr.write_map(src_categories);

        std::string plain_cat = ostr.str();

        // El::CRC32
        El::CRC(message_categorizer->hash,
                (unsigned char*)plain_cat.c_str(),
                plain_cat.length());
      }
      
      message_categorizer->init();

      NumberSet traverse_prio_message;

      if(!categorize_msg_ids.empty())
      {          
        MgrReadGuard guard(mgr_lock_);

        IdToNumberMap::const_iterator num_end = messages_.id_to_number.end();
        StoredMessageMap::const_iterator msg_end = messages_.messages.end();

        for(IdSet::const_iterator i(categorize_msg_ids.begin()),
              e(categorize_msg_ids.end()); i != e; ++i)
        {
          IdToNumberMap::const_iterator num_it =
            messages_.id_to_number.find(*i);

          if(num_it != num_end)
          {
            Number num = num_it->second;
            
            StoredMessageMap::const_iterator msg_it =
              messages_.messages.find(num);
            
            if(msg_it != msg_end)
            {
              if(msg_it->second->visible())
              {
                traverse_prio_message.insert(num);
              }
            }
          }
        }
      }
      
      {
        MgrWriteGuard guard(mgr_lock_);

        if(reset_categorizer(message_categorizer->source,
                             message_categorizer->stamp,
                             categorizer.enforced))
        {
          message_categorizer_ = message_categorizer.retn();
          
          traverse_prio_message_.swap(traverse_prio_message);
          traverse_prio_message_it_ = traverse_prio_message_.begin();
        }
        else
        {
          return AR_NOT_CHANGED;
        } 
      }

      return AR_SUCCESS;
    }

    Search::Condition*
    MessageManager::category_message_condition(
      const Categorizer::Category::RelMsgArray& messages,
      IdSet& recent_msg_ids) const
      throw(Exception, El::Exception)
    {
      if(messages.empty())
      {
        return 0;
      }

      Search::Msg* msg_cond = new Search::Msg();
      Search::Condition_var cond = msg_cond;
      
      Message::IdArray& ids = msg_cond->ids;
      ids.reserve(messages.size());

      uint64_t threshold = ACE_OS::gettimeofday().sec() - 86400;

      for(Categorizer::Category::RelMsgArray::const_iterator
            i(messages.begin()), e(messages.end()); i != e; ++i)
      {
        const Categorizer::Category::RelMsg& rmsg = *i;
        
        ids.push_back(rmsg.id);

        if(rmsg.updated >= threshold)
        {
          recent_msg_ids.insert(rmsg.id);
        }
      }     
        
      return cond.retn();
    }
    
    void
    MessageManager::post_categorize_and_save(
      SearcheableMessageMap* temp_messages,
      const IdTimeMap& ids,
      const MessageCategorizer::MessageCategoryMap& old_categories,
      bool extended_save,
      ACE_Time_Value& filtering_time,
      ACE_Time_Value& applying_change_time,
      ACE_Time_Value& deletion_time,
      ACE_Time_Value& saving_time,
      ACE_Time_Value& db_insertion_time)
      throw(Exception, El::Exception)
    {
      if(ids.empty())
      {
        return;
      }

      IdTimeMap removed_msg;
      MessageFetchFilterMap_var filters = get_message_fetch_filters();
      
      ACE_High_Res_Timer timer;
      
      if(temp_messages)
      {
        timer.start();

        apply_message_fetch_filters(*temp_messages,
                                    filters.in(),
                                    0,
                                    removed_msg,
                                    SIZE_MAX);

        timer.stop();
        timer.elapsed_time(filtering_time);
      
        timer.start();
        
        {
          MgrWriteGuard guard(mgr_lock_);
          
          for(IdTimeMap::const_iterator i(ids.begin()), e(ids.end());
              i != e; ++i)
          {
            const Id& id = i->first;
            StoredMessage* temp_msg = temp_messages->find(id);
            
            if(temp_msg == 0)
            {
              assert(removed_msg.find(id) != removed_msg.end());
              messages_.remove(id);
            }
            else
            {
              StoredMessage* msg = messages_.find(id);
              
              if(msg != 0 && msg->visible())
              {
                messages_.set_categories(*msg, temp_msg->categories);
              }
            }
          }

          recent_msg_deletions(removed_msg);
        }
      
        timer.stop();
        timer.elapsed_time(applying_change_time);
      }
      else
      {
        timer.start();
        
        {
          MgrWriteGuard guard(mgr_lock_);
          
          apply_message_fetch_filters(messages_,
                                      filters.in(),
                                      capacity_filter_.in(),
                                      removed_msg,
                                      SIZE_MAX);

          recent_msg_deletions(removed_msg);
        }
        
        timer.stop();
        timer.elapsed_time(filtering_time);
      }
      
      timer.start();

      El::MySQL::Connection_var connection =
        Application::instance()->dbase()->connect();
      
      delete_messages(removed_msg, connection);
      
      timer.stop();
      timer.elapsed_time(deletion_time);

      timer.start();

      std::string suffix = El::Moment(ACE_OS::gettimeofday()).dense_format();
      std::string cat_filename = cache_filename_ + ".cat." + suffix;
      std::string ucat_filename = cache_filename_ + ".ucat." + suffix;
      std::string msg_filename;

      try
      {
        std::fstream cat_file(cat_filename.c_str(), ios::out);

        if(!cat_file.is_open())
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::"
            "post_categorize_and_save: failed to open file '"
               << cat_filename << "' for write access";
          
          throw Exception(ostr.str());
        }  
          
        std::fstream ucat_file(ucat_filename.c_str(), ios::out);

        if(!ucat_file.is_open())
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::"
            "post_categorize_and_save: failed to open file '"
               << ucat_filename << "' for write access";
          
          throw Exception(ostr.str());
        }  
          
        std::auto_ptr<std::fstream> msg_file;
          
        if(extended_save)
        {
          msg_filename = cache_filename_ + ".msg." + suffix;
          msg_file.reset(new std::fstream(msg_filename.c_str(), ios::out));

          if(!msg_file->is_open())
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Message::MessageManager::"
              "post_categorize_and_save: failed to open file '"
                 << msg_filename << "' for write access";
            
            throw Exception(ostr.str());
          }
        }        

        size_t unchanged_categories = 0;
        size_t recategorized = 0;
        size_t messages = 0;

        timer.start();
        
        {
          MgrReadGuard guard(mgr_lock_);        

          for(IdTimeMap::const_iterator i(ids.begin()), e(ids.end()); i != e;
              ++i)
          {
            const Id& id = i->first;
            const StoredMessage* msg = messages_.find(id);
            
            if(msg == 0 || msg->hidden())
            {
              continue;
            }

            MessageCategorizer::MessageCategoryMap::const_iterator cit =
              old_categories.find(id);

            if(cit != old_categories.end() &&
               cit->second.array == msg->categories.array)
            {
              if(unchanged_categories++)
              {
                ucat_file << std::endl;
              }
              
              ucat_file << msg->id.data << "\t"
                        << msg->categories.categorizer_hash;
            }
            else
            {
              std::string categories;
            
              {
                std::ostringstream ostr;
                msg->categories.write(ostr);
                
                categories = ostr.str();
                
                categories = El::MySQL::Connection::escape_for_load(
                  categories.c_str(),
                  categories.length());
              }

              if(recategorized++)
              {
                cat_file << std::endl;
              }
              
              cat_file << msg->id.data << "\t"
                       << msg->categories.categorizer_hash << "\t";
              
              cat_file.write(categories.c_str(), categories.length());
            }

            if(msg_file.get())
            {
              if(messages++)
              {  
                *msg_file << std::endl;
              }
              
              *msg_file << msg->id.data << "\t0\t0\t0\t0\t0\t0\t0\t\t\t"
                        << msg->lang.el_code() << "\t0\t\t\t";

              std::ostringstream ostr;
              msg->write_broken_down(ostr);
              
              std::string broken_down = ostr.str();
              
              broken_down = El::MySQL::Connection::escape_for_load(
                broken_down.c_str(),
                broken_down.length());
              
              msg_file->write(broken_down.c_str(), broken_down.length());
            }
          }
        }
        
        if(!cat_file.fail())
        {
          cat_file.flush();
        }
            
        if(cat_file.fail())
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::"
            "post_categorize_and_save: failed to write into file '"
               << cat_filename << "'";
            
          throw Exception(ostr.str());
        }
          
        cat_file.close();

        if(!ucat_file.fail())
        {
          ucat_file.flush();
        }
            
        if(ucat_file.fail())
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::"
            "post_categorize_and_save: failed to write into file '"
               << ucat_filename << "'";
            
          throw Exception(ostr.str());
        }
          
        ucat_file.close();

        if(msg_file.get())
        {
          if(!msg_file->fail())
          {
            msg_file->flush();
          }
            
          if(msg_file->fail())
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Message::MessageManager::"
              "post_categorize_and_save: failed to write into file '"
                 << msg_filename << "'";
            
            throw Exception(ostr.str());
          }

          msg_file.reset(0);
        }

        timer.stop();
        timer.elapsed_time(saving_time);

        timer.start();

        std::string query_load_messages;
  
        if(messages)
        {          
          query_load_messages = std::string("LOAD DATA INFILE '") +
            connection->escape(msg_filename.c_str()) +
            "' REPLACE INTO TABLE MessageBuff character set binary";
        }

        std::string query_load_categories;
  
        if(recategorized)
        {          
          query_load_categories = std::string("LOAD DATA INFILE '") +
            connection->escape(cat_filename.c_str()) +
            "' REPLACE INTO TABLE MessageCat character set binary";
        }

        std::string query_load_unchanged_categories;
  
        if(unchanged_categories)
        {          
          query_load_unchanged_categories = std::string("LOAD DATA INFILE '") +
            connection->escape(ucat_filename.c_str()) +
            "' REPLACE INTO TABLE MessageCatBuff character set binary";
        }

        El::MySQL::Result_var result;
        
        Guard db_guard(db_lock_);
        
        if(messages)
        {        
          result = connection->query("delete from MessageBuff");
          result = connection->query(query_load_messages.c_str());
          
          result = connection->query(
            "INSERT INTO Message SELECT * FROM MessageBuff ON "
            "DUPLICATE KEY UPDATE Message.lang=MessageBuff.lang, "
            "Message.broken_down=MessageBuff.broken_down");
        }

        if(!msg_filename.empty())
        {
          unlink(msg_filename.c_str());
        }
        
        if(recategorized)
        {        
          result = connection->query(query_load_categories.c_str());
        }

        if(!cat_filename.empty())
        {
          unlink(cat_filename.c_str());
        }

        if(unchanged_categories)
        {        
          result = connection->query("delete from MessageCatBuff");
          result = connection->query(query_load_unchanged_categories.c_str());
          
          result = connection->query(
            "INSERT INTO MessageCat SELECT *, '' FROM MessageCatBuff ON "
            "DUPLICATE KEY UPDATE MessageCat.categorizer_hash="
            "MessageCatBuff.categorizer_hash");
        }

        if(!ucat_filename.empty())
        {
          unlink(ucat_filename.c_str());
        }

        timer.stop();
        timer.elapsed_time(db_insertion_time);
      }
      catch(...)
      {
        if(!msg_filename.empty())
        {
          unlink(msg_filename.c_str());
        }
        
        if(!cat_filename.empty())
        {
          unlink(cat_filename.c_str());
        }
        
        if(!ucat_filename.empty())
        {
          unlink(ucat_filename.c_str());
        }
        
        throw;
      }
    }

    void
    MessageManager::categorize_and_save(
      MessageCategorizer* categorizer,
      const IdTimeMap& ids,
      bool extended_save,
      Transport::MessageSharingInfoPackImpl::Var* share_messages)
      throw(Exception, El::Exception)
    {
      if(categorizer == 0 || ids.empty())
      {
        return;
      }

      ACE_Time_Value insertion_time;
      ACE_Time_Value categorization_time;      

      ACE_High_Res_Timer timer;
      timer.start();
      
      SearcheableMessageMap temp_messages(
        false,
        true,
        config_.impression_respected_level(),
        0);

      {
        MgrReadGuard guard(mgr_lock_);

        for(IdTimeMap::const_iterator it(ids.begin()), end(ids.end());
            it != end; ++it)
        {
          StoredMessage* msg = messages_.find(it->first);

          if(msg && msg->visible())
          {
            temp_messages.insert(*msg, 0, 0/*, false*/);
          }
        }        
      }

      timer.stop();
      timer.elapsed_time(insertion_time);
      timer.start();

      MessageCategorizer::MessageCategoryMap old_categories;
      
      categorize(temp_messages,
                 categorizer,
                 ids,
                 extended_save,
                 old_categories);
      
      timer.stop();
      timer.elapsed_time(categorization_time);

      ACE_Time_Value filtering_time;
      ACE_Time_Value applying_change_time;
      ACE_Time_Value deletion_time;
      ACE_Time_Value saving_time;
      ACE_Time_Value db_insertion_time;

      post_categorize_and_save(&temp_messages,
                               ids,
                               old_categories,
                               extended_save,
                               filtering_time,
                               applying_change_time,
                               deletion_time,
                               saving_time,
                               db_insertion_time);
      
      if(Application::will_trace(El::Logging::HIGH))
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::categorize_and_save: "
             << ids.size() << " message, "
          "times:\ninsertion " << El::Moment::time(insertion_time)
             << ", categorization "
             << El::Moment::time(categorization_time)
             << ", filtering "
             << El::Moment::time(filtering_time)
             << ", applying change "
             << El::Moment::time(applying_change_time)
             << ", deletion "
             << El::Moment::time(deletion_time)
             << ", saving "
             << El::Moment::time(saving_time)
             << ", db insertion "
             << El::Moment::time(db_insertion_time);
        
          Application::logger()->trace(ostr.str(),
                                       Aspect::DB_PERFORMANCE,
                                       El::Logging::HIGH);
      }

      if(share_messages)
      {
        if(share_messages->in())
        {
          Transport::MessageSharingInfoArray& entities =
            (*share_messages)->entities();

          MgrReadGuard guard(mgr_lock_);

          for(Transport::MessageSharingInfoArray::iterator
                i(entities.begin()), e(entities.end()); i != e; ++i)
          {
            if(ids.find(i->id()) != ids.end())
            {
              StoredMessage* msg = messages_.find(i->id());

              if(msg && msg->visible())
              {
                i->categories = msg->categories;
              }
            }
          }
        }
        else
        {
          *share_messages =
            Transport::MessageSharingInfoPackImpl::Init::create(
              new Transport::MessageSharingInfoArray());

          Transport::MessageSharingInfoArray& entities =
            (*share_messages)->entities();

          entities.reserve(ids.size());

          MgrReadGuard guard(mgr_lock_);
        
          for(IdTimeMap::const_iterator i(ids.begin()), e(ids.end()); i != e;
              ++i)
          {           
            StoredMessage* msg = messages_.find(i->first);
          
            if(msg && msg->visible())
            {
              entities.push_back(
                Transport::MessageSharingInfo(msg->id,
                                              msg->event_id,
                                              msg->event_capacity,
                                              msg->impressions,
                                              msg->clicks,
                                              msg->visited,
                                              msg->categories,
                                              msg->word_hash(true)));
            }
          }
        }
      }
    }
    
    void
    MessageManager::categorize(
      SearcheableMessageMap& messages,
      MessageCategorizer* categorizer,
      const IdTimeMap& ids,
      bool no_hash_check,
      MessageCategorizer::MessageCategoryMap& old_categories)
      throw(Exception, El::Exception)
    {
      if(categorizer)
      {
        IdSet id_set;
        id_set.resize(ids.size());

        for(IdTimeMap::const_iterator it = ids.begin(); it != ids.end(); it++)
        {
          id_set.insert(it->first);
        }
        
        categorizer->categorize(messages,
                                id_set,
                                no_hash_check,
                                old_categories);
      }
    }

    void
    MessageManager::adjust_capacity_threshold(size_t pack_message_count) throw()
    {
      size_t capacity_threshold = 0;
      
      {
        MgrReadGuard guard(mgr_lock_);
      
        size_t room = config_.message_cache().capacity() - capacity_threshold_;
        size_t min_room = 2 * pack_message_count;

        if(room >= min_room)
        {
          return;
        }

        capacity_threshold = config_.message_cache().capacity() - min_room;
      }

      MgrWriteGuard guard(mgr_lock_);
      capacity_threshold_ = capacity_threshold;
    }
    
    size_t
    MessageManager::apply_message_fetch_filters(
      SearcheableMessageMap& messages,
      MessageFetchFilterMap* filters,
      Search::Expression* capacity_filter,
      IdTimeMap& removed_msg,
      size_t max_remove_count)
      throw(Exception, El::Exception)
    {
      size_t removed_count = 0;
      
      ACE_High_Res_Timer timer;

      if(Application::will_trace(El::Logging::HIGH))
      {
        timer.start();
      }
      
      std::ostringstream removed_messages_ostr;

      if(filters)
      {
        for(MessageFetchFilterMap::const_iterator it = filters->begin();
            it != filters->end() && removed_count < max_remove_count; it++)
        {
          const MessageFetchFilter& filter = it->second;
          
          Search::Condition::Context context(messages);
          Search::Condition::MessageMatchInfoMap match_info;
          
          Search::Condition::ResultPtr result(
            filter.condition->evaluate(context, match_info, 0));
          
          size_t count =
            std::min(max_remove_count - removed_count, result->size());
          
          removed_msg.resize(removed_msg.size() + count);

          for(Search::Condition::Result::const_iterator it = result->begin();
              it != result->end() && removed_count < max_remove_count;
              it++, removed_count++)
          {
            const StoredMessage* msg = it->second;
            
            const Id& id = msg->id;

            removed_messages_ostr << std::endl << id.string() << " "
                                  << msg->published;
          
            removed_msg.insert(std::make_pair(id, msg->published));
            messages.remove(id);
          }
        }
      }
      
      if(capacity_filter && removed_count < max_remove_count &&
         capacity_threshold_ < messages.messages.size())
      {
        size_t count =
          std::min(messages.messages.size() - capacity_threshold_,
                   max_remove_count - removed_count);


        Search::Strategy strategy(new Search::Strategy::SortByPubDateAcs(),
                                  new Search::Strategy::SuppressNone(),
                                  false,
                                  Search::Strategy::Filter(),
                                  Search::Strategy::RF_MESSAGES);

        Search::ResultPtr res(
          capacity_filter->search(messages, false, strategy));

        res->take_top(0, count, strategy);

        removed_messages_ostr << "\n*******";

        for(Search::MessageInfoArray::const_iterator
              i(res->message_infos->begin()), e(res->message_infos->end());
            i != e; ++i)
        {
          const Search::MessageInfo& mi(*i);
          
          const Id& id = mi.wid.id;
          const StoredMessage* msg = messages.find(id);

          assert(msg);
          
          removed_messages_ostr << std::endl << id.string() << " "
                                << msg->published;
          
          removed_msg.insert(std::make_pair(id, msg->published));
          messages.remove(id);        
        }

        removed_count += res->message_infos->size();
      }
         
      if(removed_count && Application::will_trace(El::Logging::HIGH))
      {
        timer.stop();
        ACE_Time_Value tm;
        timer.elapsed_time(tm);
            
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::"
          "apply_message_fetch_filters: " << removed_count
             << " messages removed; time " << El::Moment::time(tm) << ":"
             << removed_messages_ostr.str();

        Application::logger()->trace(ostr.str(),
                                     Aspect::DB_PERFORMANCE,
                                     El::Logging::HIGH);
      }
      
      return removed_count;
    }

    MessageManager::AssignResult
    MessageManager::assign_message_fetch_filter(const FetchFilter& filter)
      throw(Exception, El::Exception)
    {
      {
        MgrReadGuard guard(mgr_lock_);

        if(message_filters_.in() != 0)
        {
          MessageFetchFilterMap::const_iterator it =
            message_filters_->find(filter.source);

          if(it != message_filters_->end() &&
             it->second.stamp >= filter.stamp && !filter.enforced)
          {  
            return AR_NOT_CHANGED;
          }
        }
      }
      
      Search::Or* or_cond = new Search::Or();
      Search::Condition_var cond = or_cond;

      const FetchFilter::ExpressionArray& expressions = filter.expressions;

      uint32_t dict_hash = 0;

      try
      {
        for(FetchFilter::ExpressionArray::const_iterator
              i(expressions.begin()), e(expressions.end()); i != e; ++i)
        {
          or_cond->operands.push_back(segment_search_expression(i->c_str()));
        }
        
        if(!normalize_search_condition(cond))
        {
          return AR_NOT_READY;
        }

        dict_hash = dictionary_hash();
      }
      catch(const WordManagerNotReady& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::"
          "assign_message_fetch_filter: WordManagerNotReady caught. "
          "Description:\n" << e;
        
        El::Service::Error
          error(ostr.str(), this, El::Service::Error::NOTICE);
        
        callback_->notify(&error);
        return AR_NOT_READY;
      }
      catch(const SegmentorNotReady& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::"
          "assign_message_fetch_filter: SegmentorNotReady caught. "
          "Description:\n" << e;
        
        El::Service::Error
          error(ostr.str(), this, El::Service::Error::NOTICE);
        
        callback_->notify(&error);
        return AR_NOT_READY;
      }

      MgrWriteGuard guard(mgr_lock_);

      if(message_filters_.in() == 0)
      {
        message_filters_ = new MessageFetchFilterMap();
      }

      message_filters_->dict_hash = dict_hash;
      
      MessageFetchFilterMap::const_iterator it =
        message_filters_->find(filter.source);

      if(it != message_filters_->end() && it->second.stamp >= filter.stamp &&
         !filter.enforced)
      {  
        return AR_NOT_CHANGED;
      }
      
      (*message_filters_)[filter.source] =
        MessageFetchFilter(filter.stamp, cond.retn());
      
      return AR_SUCCESS;
    }

    void
    MessageManager::check_mirrored_messages(
      BankClientSession* mirrored_banks,
      const char* mirrored_sharing_id,
      Transport::CheckMirroredMessagesRequestImpl::Type* messages_to_check)
      throw(Exception, El::Exception)
    {
      if(mirrored_banks == 0 || *mirrored_sharing_id == '\0' ||
         messages_to_check == 0 || messages_to_check->entities().empty())
      {
        return;
      }
      
      try
      {
        messages_to_check->serialize();
        
        Message::Transport::Response_var response;
        
        Message::BankClientSession::RequestResult_var result =
          mirrored_banks->send_request(messages_to_check, response.out());

        if(result->code != BankClientSession::RRC_OK)
        {
          return;
        }

        Transport::CheckMirroredMessagesResponseImpl::Type* found_msg =
          dynamic_cast<Transport::CheckMirroredMessagesResponseImpl::Type*>(
            response.in());

        if(found_msg == 0)
        {
          throw Exception(
            "NewsGate::Message::MessageManager::check_mirrored_messages: "
            "dynamic_cast<Transport::CheckMirroredMessagesResponseImpl::"
            "Type*> failed");
        }

        IdSet found_messages;
        
        {
          const IdArray& ids = found_msg->entities();
          found_messages.resize(ids.size());
          
          for(IdArray::const_iterator i(ids.begin()), e(ids.end());
              i != e; ++i)
          {
            found_messages.insert(*i);
          }
        }

        const IdArray& ids = messages_to_check->entities();
        IdArray lost_messages;
        
        for(IdArray::const_iterator i(ids.begin()), e(ids.end());
            i != e; ++i)
        {
          const Id& id = *i;

          if(found_messages.find(id) == found_messages.end())
          {
            lost_messages.push_back(id);
          }
        }

        if(Application::will_trace(El::Logging::HIGH))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::"
            "check_mirrored_messages: " << lost_messages.size()
               << " lost messages requested by master cluster";

          Application::logger()->trace(ostr.str(),
                                       Aspect::MSG_SHARING,
                                       El::Logging::HIGH);
        }
        
        share_messages(lost_messages,
                       mirrored_banks,
                       PMR_LOST_MESSAGE_SHARING,
                       mirrored_sharing_id);        
      }
      catch(const BankClientSession::FailedToPostMessages& e)
      {
        if(Application::will_trace(El::Logging::HIGH))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::"
            "check_mirrored_messages: FailedToPostMessages caught. "
            "Reason:\n" << e.description.in();
          
          Application::logger()->trace(ostr.str(),
                                       Aspect::MSG_SHARING,
                                       El::Logging::HIGH);
        }
      }
      catch(const NotReady& e)
      {
        if(Application::will_trace(El::Logging::HIGH))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::"
            "check_mirrored_messages: NotReady caught. Description:"
               << std::endl << e.reason.in();
          
          Application::logger()->trace(ostr.str(),
                                       Aspect::MSG_SHARING,
                                       El::Logging::HIGH);
        }
      }
      catch(const ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::check_mirrored_messages: "
          "ImplementationException caught. Description:" << std::endl
             << e.description.in();
        
        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::check_mirrored_messages: "
          "CORBA::Exception caught. Description:" << std::endl << e;
        
        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
      }
    }
    
    void
    MessageManager::share_requested_messages(
      Transport::MessageSharingInfoPackImpl::Type* messages_to_share)
      throw(Exception, El::Exception)
    {
      if(messages_to_share == 0)
      {
        return;
      }

      std::string sharing_id;
      
      {
        Transport::MessageSharingInfoArray& sharing_infos =
          messages_to_share->entities();

        MgrReadGuard guard(mgr_lock_);

        sharing_id = session_->sharing_id();

        for(Transport::MessageSharingInfoArray::iterator
              i(sharing_infos.begin()), e(sharing_infos.end()); i != e; ++i)
        {
          Transport::MessageSharingInfo& mi(*i);

          const StoredMessage* msg = messages_.find(mi.id());

          if(msg && msg->visible())
          {
            mi.event_id = msg->event_id;
            mi.event_capacity = msg->event_capacity;
            mi.impressions = msg->impressions;
            mi.clicks = msg->clicks;
            mi.visited = msg->visited;
            mi.categories = msg->categories;
          }
        }
      }
      
      MessageSinkMap message_sink =
        callback_->message_sink_map(MSRT_OLD_MESSAGES);

      for(MessageSinkMap::iterator it = message_sink.begin();
          it != message_sink.end(); it++)
      {
        MessageSink& sink = it->second;
        
        try
        {
          Transport::IdPack_var requested_msg_pack;

          if(Application::will_trace(El::Logging::HIGH))
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Message::MessageManager::"
              "share_requested_messages: sending message_sharing_offer "
              "request with " << messages_to_share->entities().size()
                 << " ids to cluster " << it->first;

            Application::logger()->trace(ostr.str(),
                                         Aspect::MSG_SHARING,
                                         El::Logging::HIGH);
          }
          
          sink.message_sink->message_sharing_offer(messages_to_share,
                                                   sharing_id.c_str(),
                                                   requested_msg_pack.out());

          Transport::IdPackImpl::Type* requested_msg =
            dynamic_cast<Transport::IdPackImpl::Type*>(
              requested_msg_pack.in());

          if(requested_msg == 0)
          {
            throw Exception(
              "NewsGate::Message::MessageManager::share_requested_messages: "
              "dynamic_cast<Transport::IdPackImpl*> failed");
          }

          const IdArray& requested_messages = requested_msg->entities();

          if(Application::will_trace(El::Logging::HIGH))
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Message::MessageManager::"
              "share_requested_messages: " << requested_messages.size()
                 << " messages requested by cluster " << it->first;

            Application::logger()->trace(ostr.str(),
                                         Aspect::MSG_SHARING,
                                         El::Logging::HIGH);
          }

          share_messages(requested_messages,
                         sink.message_sink.in(),
                         PMR_OLD_MESSAGE_SHARING,
                         sharing_id.c_str());
        }
        catch(const BankClientSession::FailedToPostMessages& e)
        {
          if(Application::will_trace(El::Logging::HIGH))
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Message::MessageManager::"
              "share_requested_messages: FailedToPostMessages caught. "
              "Reason:\n" << e.description.in();

            Application::logger()->trace(ostr.str(),
                                         Aspect::MSG_SHARING,
                                         El::Logging::HIGH);
          }
        }
        catch(const NotReady& e)
        {
          if(Application::will_trace(El::Logging::HIGH))
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Message::MessageManager::"
              "share_requested_messages: NotReady caught. Description:"
                 << std::endl << e.reason.in();
        
            Application::logger()->trace(ostr.str(),
                                         Aspect::MSG_SHARING,
                                         El::Logging::HIGH);
          }
        }
        catch(const ImplementationException& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::"
            "share_requested_messages: ImplementationException caught. "
            "Description:" << std::endl << e.description.in();
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(const CORBA::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::"
            "share_requested_messages: CORBA::Exception caught. Description:"
               << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::"
            "share_requested_messages: El::Exception caught. Description:"
               << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
      }
    }

    void
    MessageManager::share_messages(
      const IdArray& ids,
      ::NewsGate::Message::BankClientSession* session,
      ::NewsGate::Message::PostMessageReason reason,
      const char* sharing_id)
      throw(CORBA::Exception, El::Exception)
    {
      if(ids.empty())
      {
        return;
      }
          
      std::auto_ptr<std::ostringstream> load_msg_content_request_ostr;

      Transport::StoredMessagePackImpl::Var message_pack =
        Message::Transport::StoredMessagePackImpl::Init::create(
          new Transport::StoredMessageArray());

      Transport::StoredMessageArray& messages_to_share =
        message_pack->entities();

      messages_to_share.reserve(ids.size());

      IdToSizeMap id_to_index;
      id_to_index.resize(ids.size());
          
      {
        MgrReadGuard guard(mgr_lock_);
            
        for(IdArray::const_iterator mit = ids.begin(); mit != ids.end(); ++mit)
        {
          const Id& id = *mit;
          const StoredMessage* msg = messages_.find(id);
              
          if(msg == 0 || msg->hidden())
          {
            continue;
          }

          messages_to_share.push_back(Transport::StoredMessageDebug(*msg));

          if(msg->content.in() == 0)
          {
            ContentCache::query_stored_content(
              id,
              load_msg_content_request_ostr);

            id_to_index.insert(
              std::make_pair(id, messages_to_share.size() - 1));
          }
        }
      }

      if(load_msg_content_request_ostr.get() != 0)
      {
        *load_msg_content_request_ostr << " )";
            
        std::string query = load_msg_content_request_ostr->str();

        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();

        ACE_High_Res_Timer timer;

        if(Application::will_trace(El::Logging::HIGH))
        {
          timer.start();
        }
            
        El::MySQL::Result_var result = connection->query(query.c_str());

        MessageContentRecord record(result.in());

        while(record.fetch_row())
        {
          Id id(record.id());

          IdToSizeMap::const_iterator it = id_to_index.find(id);
          assert(it != id_to_index.end());

          messages_to_share[it->second].message.content =
            ContentCache::read_stored_content(record);
        }

        for(Transport::StoredMessageArray::iterator it =
              messages_to_share.begin(); it != messages_to_share.end(); )
        {
          if(it->message.content.in() == 0)
          {
            it = messages_to_share.erase(it);
          }
          else
          {
            ++it;
          }
        }

        if(Application::will_trace(El::Logging::HIGH))
        {
          timer.stop();
          ACE_Time_Value tm;
          timer.elapsed_time(tm);
            
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::"
            "share_messages: " << messages_to_share.size()
               << " messages content query time " << El::Moment::time(tm);

          Application::logger()->trace(ostr.str(),
                                       Aspect::DB_PERFORMANCE,
                                       El::Logging::HIGH);
        }
      }

      if(!messages_to_share.empty())
      {
        load_img_thumb("share_messages", messages_to_share);
            
        session->post_messages(message_pack.in(),
                               reason,
                               BankClientSession::PS_PUSH_BEST_EFFORT,
                               sharing_id);
      }
    }                              

    void
    MessageManager::move_messages_to_owners(
      Transport::StoredMessageArrayPtr& messages,
      const IdToSizeMap* it_to_index_map,
      const char* query)
      throw(Exception, El::Exception)
    {
      if(query != 0)
      {
        assert(it_to_index_map != 0);
        
        if(Application::will_trace(El::Logging::HIGH))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::"
            "move_messages_to_owners: loading " << messages->size()
               << " messages to be moved to owner-banks:\n" << query;
                
          Application::logger()->trace(ostr.str(),
                                       Aspect::MSG_MANAGEMENT,
                                       El::Logging::HIGH);
        }
              
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();

        ACE_High_Res_Timer timer;

        if(Application::will_trace(El::Logging::HIGH))
        {
          timer.start();
        }
        
        El::MySQL::Result_var result = connection->query(query);

        MessageContentRecord record(result.in());

        while(record.fetch_row())
        {
          Id id(record.id());
            
          for(Transport::StoredMessageArray::iterator it =
                messages->begin(); it != messages->end(); it++)
          {
            if(it->get_id() == id)
            {
              it->message.content = ContentCache::read_stored_content(record);
              break;
            }
          }  
        }

        connection = 0;

        if(Application::will_trace(El::Logging::HIGH))
        {
          timer.stop();
          ACE_Time_Value tm;
          timer.elapsed_time(tm);
            
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::"
            "move_messages_to_owners: " << messages->size()
               << " message content query time " << El::Moment::time(tm);

          Application::logger()->trace(ostr.str(),
                                       Aspect::DB_PERFORMANCE,
                                       El::Logging::HIGH);
        }
        
        for(Transport::StoredMessageArray::iterator it =
              messages->begin(); it != messages->end(); )
        {
          if(it->message.content.in() == 0)
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Message::MessageManager::"
              "move_messages_to_owners: message with id="
                 << it->get_id().string() << " can't be found in DB";
        
            El::Service::Error
              error(ostr.str(), this, El::Service::Error::ALERT);
            
            callback_->notify(&error);
            
            it = messages->erase(it);
          }
          else
          {
            it++;
          }
        }
      }
            
      IdTimeMap posted;
      post_to_owners(messages.release(), posted);

      if(posted.size())
      {
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();

        ACE_High_Res_Timer timer;

        if(Application::will_trace(El::Logging::HIGH))
        {
          timer.start();
        }
          
        delete_messages(posted, connection, false);      
        
        if(Application::will_trace(El::Logging::HIGH))
        {
          timer.stop();
          ACE_Time_Value tm;
          timer.elapsed_time(tm);
            
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::"
            "move_messages_to_owners: messages delete time "
               << El::Moment::time(tm);

          Application::logger()->trace(ostr.str(),
                                       Aspect::DB_PERFORMANCE,
                                       El::Logging::HIGH);
        }
        
        MgrWriteGuard guard(mgr_lock_);

        for(IdTimeMap::const_iterator it = posted.begin();
            it != posted.end(); it++)
        {
          messages_.remove(it->first);
        }
      }
    }

    void
    MessageManager::exec_msg_operations(const MessageOperationList& operations,
                                        time_t cur_time,
                                        time_t preempt_time,
                                        time_t expire_time,
                                        std::ostream& log_ostr)
      throw(Exception, El::Exception)
    {
      if(operations.empty())
      {
        return;
      }
      
      if(Application::will_trace(El::Logging::HIGH))
      {
        log_ostr << "Operations:\n";
      }

      std::string cache_filename;

      try
      {        
        std::fstream file;
        
        size_t dirty_messages = 0;
        size_t flushed_messages = 0;

        unsigned long max_flush_pack_size =
          config_.message_cache().max_flush_pack_size();
        
        IdTimeMap deleted_messages;

        MgrWriteGuard guard(mgr_lock_);

        for(MessageOperationList::const_iterator it = operations.begin();
            it != operations.end(); it++)
        {
          if(it->second == MO_FLUSH_STATE)
          {
            StoredMessage* msg = messages_.find(it->first);

            if(msg && msg->visible() && (msg->flags & StoredMessage::MF_DIRTY))
            {   
              dirty_messages++;
            }
          }
        }

        for(MessageOperationList::const_iterator it = operations.begin();
            it != operations.end(); it++)
        {
          StoredMessage* msg = messages_.find(it->first);

          if(msg == 0 || msg->hidden())
          {
            continue;
          }
          
          const Id& id = it->first;
                
          switch(it->second)
          {
          case MO_PREEMPT:
            {
              if(msg->content.in() != 0 &&
                 (time_t)msg->content->timestamp() < preempt_time)
              {
                if(Application::will_trace(El::Logging::HIGH))
                {
                  El::Moment timestamp(
                    ACE_Time_Value(msg->content->timestamp()));
                        
                  log_ostr << "  preemting " << id.string()
                           << " (used at " << timestamp.iso8601()
                           << ")\n";
                }

                msg->content = 0;
              }
                    
              break;
            }
          case MO_DELETE:
            {
              if((time_t)msg->published < expire_time)
              {
                if(Application::will_trace(El::Logging::HIGH))
                {
                  El::Moment published(ACE_Time_Value(msg->published));
                        
                  log_ostr << "  deleting " << id.string()
                           << " (published at " << published.iso8601()
                           << ")\n";
                }

                deleted_messages[id] = msg->published;
                messages_.remove(id);
                recent_msg_deletions_[id] = (uint64_t)cur_time;
              }
                    
              break;
            }
          case MO_FLUSH_STATE:
            {
              if(msg->flags & StoredMessage::MF_DIRTY)
              {
                if(dirty_messages > max_flush_pack_size)
                {
                  unsigned long long rval = (unsigned long long)rand() *
                    dirty_messages / ((unsigned long long)RAND_MAX + 1);

                  if(rval >= max_flush_pack_size)
                  {
                    continue;
                  }
                }
                
                if(cache_filename.empty())
                {
                  cache_filename = cache_filename_ + ".upd." +
                    El::Moment(ACE_OS::gettimeofday()).dense_format();

                  file.open(cache_filename.c_str(), ios::out);

                  if(!file.is_open())
                  {
                    std::ostringstream ostr;
                    ostr << "NewsGate::Message::MessageManager::"
                      "exec_msg_operations: failed to open file '"
                         << cache_filename << "' for write access";

                    throw Exception(ostr.str());
                  }
                }

                flush_msg_stat(file, *msg, flushed_messages++);
              }
              
              break;
            }
            
          }
        }

        guard.release();

        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();

        if(!deleted_messages.empty())
        {
          ACE_High_Res_Timer timer;
          timer.start();

          delete_messages(deleted_messages, connection, false);
            
          timer.stop();
          ACE_Time_Value tm;
          timer.elapsed_time(tm);
          
          if(Application::will_trace(El::Logging::HIGH))
          {
            log_ostr << " * deleted " << deleted_messages.size() << "; time: "
                     << El::Moment::time(tm) << std::endl;
          }
        }        
        
        if(cache_filename.empty())
        {
          return;
        }
        
        if(!file.fail())
        {
          file.flush();
        }
            
        if(file.fail())
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::exec_msg_operations: "
            "failed to write into file '" << cache_filename << "'";

          throw Exception(ostr.str());
        }
      
        file.close();

        ACE_Time_Value tm;
        upload_flushed_msg_stat(connection.in(), cache_filename.c_str(), tm);

        if(Application::will_trace(El::Logging::HIGH))
        {
          log_ostr << " * flushed " << flushed_messages << "/"
                   << dirty_messages << "; time: "
                   << El::Moment::time(tm) << std::endl;
        }
      }
      catch(...)
      {
        if(!cache_filename.empty())
        {
          unlink(cache_filename.c_str());
        }

        throw;
      }
      
    }

    void
    MessageManager::flush_msg_stat(std::fstream& file,
                                   StoredMessage& msg,
                                   bool newline)
      throw(El::Exception)
    {
      if(newline)
      {
        file << std::endl;
      }
      
      file << msg.id.data << "\t" << msg.event_id.data
           << "\t" << msg.event_capacity
           << "\t" << msg.impressions << "\t" << msg.clicks
           << "\t" << msg.visited;
      
      msg.flags &= ~StoredMessage::MF_DIRTY;
    }

    void
    MessageManager::upload_flushed_msg_stat(El::MySQL::Connection* connection,
                                            const char* filename,
                                            ACE_Time_Value& tm)
      throw(El::Exception)
    {
      ACE_High_Res_Timer timer;
      timer.start();

      std::string query_load_messages =
        std::string("LOAD DATA INFILE '") + connection->escape(filename) +
        "' REPLACE INTO TABLE MessageStat character set binary";
      
      {
        Guard db_guard(db_lock_);

        El::MySQL::Result_var result =
          connection->query(query_load_messages.c_str());
      }
        
      unlink(filename);
          
      timer.stop();
      timer.elapsed_time(tm);
    }

    void
    MessageManager::import_messages(ImportMsg* im) throw(El::Exception)
    {
      const Server::Config::BankMessageManagerType::message_import_type& cfg =
        config_.message_import();

      ImportMessagesResult result = import_messages(
        cfg.import_local_directory().c_str(), PMR_PUSH_OUT_FOREIGN_MESSAGES);

      if(result == IMR_NONE)
      {
        result = import_messages(cfg.import_directory().c_str(),
                                 PMR_NEW_MESSAGE_SHARING);
      }
      
      ACE_Time_Value delay(result == IMR_ERROR ? cfg.on_error_retry_period() :
                           (result == IMR_SUCCESS ? 0 :
                            cfg.check_period()));
      
      deliver_at_time(im, ACE_OS::gettimeofday() + delay);
    }

    MessageManager::ImportMessagesResult
    MessageManager::import_messages(const char* directory,
                                    PostMessageReason pack_reason)
      throw(El::Exception)
    {
      std::string filename(directory);
      MessageManager::ImportMessagesResult result = IMR_NONE;
      
      try
      {
        El::FileSystem::DirectoryReader finder(filename.c_str());
        filename += "/";
        
        for(size_t i = 0; i < finder.count(); ++i)
        {
          const dirent& entry = finder[i];

          if(entry.d_type == DT_DIR)
          {
            continue;
          }
              
          std::string full_path = filename + entry.d_name;
          std::fstream file(full_path.c_str(), std::ios::in);

          if(!file.is_open())
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Message::MessageManager::import_messages: "
              "failed to open file " << full_path << " for read access";
            
            El::Service::Error err(ostr.str(),
                                   this,
                                   El::Service::Error::CRITICAL);
            
            callback_->notify(&err);
            result = IMR_ERROR;
            break;
          }
            
          El::BinaryInStream bstr(file);
            
          uint32_t type = 0;
          bstr >> type;

          if(type != 0)
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Message::MessageManager::import_messages: "
              "unexpected type " << type << " for file " << full_path
                 << std::endl;
        
            El::Service::Error err(ostr.str(),
                                   this,
                                   El::Service::Error::CRITICAL);
              
            callback_->notify(&err);
            result = IMR_ERROR;
            break;
          }

          size_t messages = 0;
          size_t thumbnails = 0;
          StringArray pack_files;

          try
          {
            import_messages(bstr,
                            pack_reason,
                            messages,
                            thumbnails,
                            pack_files);
          }
          catch(const El::Exception& e)
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Message::MessageManager::import_messages: "
              "El::Exception caught. Description:\n" << e;
            
            El::Service::Error err(ostr.str(),
                                   this,
                                   El::Service::Error::CRITICAL);
            
            callback_->notify(&err);
            result = IMR_ERROR;
          }

          file.close();
          
          if(result != IMR_ERROR)
          {
            if(Application::will_trace(El::Logging::HIGH))
            {
              std::ostringstream ostr;  
              ostr << "NewsGate::Message::MessageManager::import_messages: "
                "read " << messages << ", " << thumbnails << " thumbnails";
              
              Application::logger()->trace(ostr.str(),
                                           Aspect::MSG_MANAGEMENT,
                                           El::Logging::HIGH);
            }

            unlink(full_path.c_str());
          
            struct stat64 stat;
            memset(&stat, 0, sizeof(stat));
          
            if(stat64(full_path.c_str(), &stat) == 0)
            {
              std::ostringstream ostr;
              ostr << "NewsGate::Message::MessageManager::import_messages: "
                "failed to delete file " << full_path
                   << "; messages will not be imported";
        
              El::Service::Error err(ostr.str(),
                                     this,
                                     El::Service::Error::CRITICAL);
              
              callback_->notify(&err);
              result = IMR_ERROR;
            }
          }
          
          if(result == IMR_ERROR)
          {
            for(StringArray::const_iterator it = pack_files.begin();
                it != pack_files.end(); ++it)
            {
              unlink(it->c_str());
            }
          }
          else
          {
            for(StringArray::const_iterator it = pack_files.begin();
                it != pack_files.end(); ++it)
            {
              const char* name = it->c_str();
              const char* ext = strrchr(name, '.');
              assert(ext != 0);
              
              std::string new_name(name, ext - name);
              
              if(rename(name, new_name.c_str()) < 0)
              {
                int error = ACE_OS::last_error();

                std::ostringstream ostr;
                
                ostr <<" NewsGate::Message::MessageManager::import_messages: "
                  "rename '" << name << "' to '" << new_name << "' failed. "
                  "Errno " << error << ". Description:\n"
                     << ACE_OS::strerror(error);

                El::Service::Error err(ostr.str(),
                                       this,
                                       El::Service::Error::CRITICAL);
        
                callback_->notify(&err);
                result = IMR_ERROR;
              }
            }

            if(result != IMR_ERROR)
            {
              result = IMR_SUCCESS;
            }
          }
          
          break;
        }
      }
      catch(const El::FileSystem::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::import_messages: "
          "El::FileSystem::Exception caught. Description:\n" << e;
        
        El::Service::Error err(ostr.str(),
                               this,
                               El::Service::Error::CRITICAL);
        
        callback_->notify(&err);
        result = IMR_ERROR;
      }

      return result;
    }
    
    void
    MessageManager::import_messages(El::BinaryInStream& bstr,
                                    PostMessageReason pack_reason,
                                    size_t& messages,
                                    size_t& thumbnails,
                                    StringArray& pack_files)
      throw(Exception, El::Exception)
    {          
      uint64_t id = 0;
      uint64_t event_id = 0;
      uint32_t event_capacity = 0;
      uint8_t flags = 0;
      uint64_t signature = 0;
      uint64_t url_signature = 0;
      uint64_t source_id = 0;
      uint32_t dict_hash = 0;
      uint64_t impressions = 0;
      uint64_t clicks = 0;
      uint64_t published = 0;
      uint64_t fetched = 0;
      uint64_t visited = 0;
      uint16_t space = 0;

      uint64_t complements_len = 0;
      unsigned char complements[65535];
    
      char url[2048 * 6 + 1];
      uint16_t lang = 0;
      uint16_t country = 0;

      char source_title[1024 * 6 + 1];
      char source_html_link[2048 * 6 + 1];
      
      uint64_t broken_down_len = 0;
      unsigned char broken_down[65535];

//      uint32_t categorizer_hash = 0;
      uint64_t categories_len = 0;
      unsigned char categories[65535];

      const Server::Config::BankMessageManagerType::message_import_type& cfg =
        config_.message_import();

      size_t pack_size = cfg.pack_size();
      
      Transport::StoredMessagePackImpl::Var pack =
        Transport::StoredMessagePackImpl::Init::create(
          new Transport::StoredMessageArray());
      
      uint32_t version = 0;
      bstr >> version;

      if(version < 6)
      {
        std::ostringstream ostr;
        ostr << "::NewsGate::MessageManager::import_messages: version "
             << version << " is not supported anymore";
        
        throw Exception(ostr.str());
      }
      
      while(true)
      {        
        try
        {
          bstr >> id;
        }
        catch(const El::BinaryInStream::Exception&)
        {
          break;
        }

        if(pack.in() != 0 && pack->entities().size() == pack_size)
        {
          PendingMessagePack pmp;
          
          pmp.reason = pack_reason;
          pmp.pack = pack._retn();
          
          pack_files.push_back(save_pack(pmp, true));
        }

        if(pack.in() == 0)
        {
          pack = Transport::StoredMessagePackImpl::Init::create(
            new Transport::StoredMessageArray());

          pack->entities().reserve(pack_size);
        }

        pack->entities().push_back(StoredMessage());
        StoredMessage& msg = pack->entities().rbegin()->message;
        
        msg.id = Id(id);

        try
        {
          bstr >> event_id >> event_capacity >> flags >> signature
               >> url_signature >> source_id >> dict_hash >> impressions
               >> clicks >> published >> fetched >> visited >> space
               >> complements_len;

          if(sizeof(complements) < complements_len)
          {
            std::ostringstream ostr;

            ostr << "::NewsGate::MessageManager::import_messages: unexpected "
              "message " << msg.id.string() << " complements length "
                 << complements_len;

            throw Exception(ostr.str());
          }
        
          bstr.read_raw_bytes(complements, complements_len);

          try
          {
            bstr.read_string_buff(url, sizeof(url));
          }
          catch(const El::BinaryInStream::Exception& e)
          {
            std::ostringstream ostr;

            ostr << "::NewsGate::MessageManager::import_messages: "
              "error reading message " << msg.id.string() << " url: " << e;

            throw Exception(ostr.str());
          }

          bstr >> lang >> country;
        
          try
          {
            bstr.read_string_buff(source_title, sizeof(source_title));
          }
          catch(const El::BinaryInStream::Exception& e)
          {
            std::ostringstream ostr;

            ostr << "::NewsGate::MessageManager::import_messages: "
              "error reading message " << msg.id.string() << " source title: "
                 << e;

            throw Exception(ostr.str());
          }

          try
          {
            bstr.read_string_buff(source_html_link,
                                  sizeof(source_html_link));
          }
          catch(const El::BinaryInStream::Exception& e)
          {
            std::ostringstream ostr;
            
            ostr << "::NewsGate::MessageManager::import_messages: "
              "error reading message " << msg.id.string()
                 << " source html link: " << e;
            
            throw Exception(ostr.str());
          }
          
          bstr >> broken_down_len;

          if(sizeof(broken_down) < broken_down_len)
          {
            std::ostringstream ostr;

            ostr << "::NewsGate::MessageManager::import_messages: unexpected "
              "message " << msg.id.string() << " broken down length "
                 << broken_down_len;

            throw Exception(ostr.str());
          }
        
          bstr.read_raw_bytes(broken_down, broken_down_len);

          // TODO: uncomment together with exporting code
//          bstr >> categorizer_hash;
          
          bstr >> categories_len;

          if(sizeof(categories) < categories_len)
          {
            std::ostringstream ostr;

            ostr << "::NewsGate::MessageManager::import_messages: unexpected "
              "message " << msg.id.string() << " categories length "
                 << categories_len;

            throw Exception(ostr.str());
          }
        
          bstr.read_raw_bytes(categories, categories_len);
        
          msg.lang = El::Lang((El::Lang::ElCode)lang);
          msg.event_id.data = event_id;
          msg.event_capacity = event_capacity;
          msg.flags = flags;
          msg.signature = signature;
          msg.url_signature = url_signature;
          msg.source_id = source_id;
          msg.impressions = impressions;
          msg.clicks = clicks;
          msg.published = published;
          msg.fetched = fetched;
          msg.visited = visited;
          msg.space = space;

          msg.source_title = source_title;

          if(msg.fetched == 0)
          {
            msg.fetched = msg.published;
          }
                
          msg.country = El::Country((El::Country::ElCode)country);

          uint16_t msg_version = 0;
        
          {
            std::string broken_down_msg((const char*)broken_down,
                                        broken_down_len);
          
            std::istringstream istr(broken_down_msg);
            msg_version = msg.read_broken_down(istr);
          }
        
          {
            std::string msg_categories((const char*)categories,
                                       categories_len);
            
            std::istringstream istr(msg_categories);
            msg.categories.read(istr);
          }
        
          Message::StoredContent_var content = new Message::StoredContent();
        
          content->dict_hash = dict_hash;
          content->url = url;
          content->source_html_link = source_html_link;

          WordPosition description_base = 0;
          
          {
            std::string compl_str((const char*)complements, complements_len);
            std::istringstream istr(compl_str);
            
            content->read_complements(istr, description_base);
          }

          if(flags & Message::StoredMessage::MF_HAS_THUMBS)
          {
            Message::StoredImageArray& images = *content->images.get();
        
            for(size_t i = 0; i < images.size(); i++)
            {
              Message::ImageThumbArray& thumbs = images[i].thumbs;

              assert(!thumbs.empty());

              for(size_t j = 0; j < thumbs.size(); j++)
              {
                Message::ImageThumb& thumb = thumbs[j];

                bstr >> thumb.length;

                if(thumb.length)
                {
                  thumb.image.reset(new unsigned char[thumb.length]);
                  bstr.read_raw_bytes(thumb.image.get(), thumb.length);
                  ++thumbnails;
                }
              }
            }
          }

          msg.content = content.retn();
          msg.adjust_positions(msg_version, description_base);
        }
        catch(const El::BinaryInStream::Exception& e)
        {
          std::ostringstream ostr;
          
          ostr << "::NewsGate::MessageManager::import_messages: error reading "
            "message " << msg.id.string() << " (" << id << ") data: " << e;
          
          throw Exception(ostr.str());
        }
        
        messages++;
      }

      if(pack.in() != 0)
      {
        PendingMessagePack pmp;
          
        pmp.reason = pack_reason;
        pmp.pack = pack._retn();
          
        pack_files.push_back(save_pack(pmp, true));
      }
    }

    std::string
    MessageManager::save_pack(const PendingMessagePack& pack, bool temporary)
      const throw(Exception, El::Exception)
    {
      std::string filename =
        std::string(config_.cache_file_dir().c_str()) + "/ImportedMsg." +
        El::Moment(ACE_OS::gettimeofday()).dense_format() + ".mpk";

      if(temporary)
      {
        filename += ".tmp";
      }

      std::fstream file(filename.c_str(), std::ios::out);

      if(!file.is_open())
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageManager::save_pack: "
          "failed to open file '" << filename << "' for write access";
        
        throw Exception(ostr.str());        
      }
      
      El::BinaryOutStream bstr(file);
      bstr << pack;
      
      return filename;
    }
      
    bool
    MessageManager::post_to_owners(Transport::StoredMessageArray* messages,
                                   IdTimeMap& posted) throw()
    {
      try
      {
        Transport::StoredMessagePackImpl::Var message_pack =
          Message::Transport::StoredMessagePackImpl::Init::create(
            messages);

        try
        {
          posted.clear();

          for(Transport::StoredMessageArray::const_iterator it =
                messages->begin(); it != messages->end(); it++)
          {
            posted.insert(std::make_pair(it->get_id(), it->message.published));
          }
          
          load_img_thumb("post_to_owners", message_pack->entities());
          
          bank_client_session_->post_messages(
            message_pack.in(),
            PMR_PUSH_OUT_FOREIGN_MESSAGES,
            NewsGate::Message::BankClientSession::
            PS_DISTRIBUTE_BY_MSG_ID,
            "");

          return true;
        }
        catch(const NewsGate::Message::BankClientSession::
              FailedToPostMessages& e)
        {
          if(Application::will_trace(El::Logging::HIGH))
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Message::MessageManager::post_to_owners: "
              "BankClientSession::FailedToPostMessages caught. "
              "Description:" << std::endl << e.description.in();

            Application::logger()->trace(ostr.str(),
                                         Aspect::MSG_MANAGEMENT,
                                         El::Logging::HIGH);
          }

          Message::Transport::StoredMessagePackImpl::Type* not_posted =
            dynamic_cast<Message::Transport::StoredMessagePackImpl::Type*>(
              e.messages.in());

          if(not_posted == 0)
          {
            throw Exception(
              "NewsGate::Message::MessageManager::post_to_owners: "
              "dynamic_cast<Message::Transport::StoredMessagePackImpl::"
              "Type*> failed");
          }

          Transport::StoredMessageArray& messages =
            not_posted->entities();

          for(Transport::StoredMessageArray::const_iterator
                i(messages.begin()), e(messages.end()); i != e; ++i)
          {
            IdTimeMap::iterator pit = posted.find(i->get_id());

            if(pit != posted.end())
            {
              posted.erase(pit);
            }
          }
          
        }
        catch(const ::NewsGate::Message::NotReady& e)
        {
          posted.clear();
          
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::post_to_owners: "
            "NotReady caught. Description:" << std::endl << e.reason.in();
        
          El::Service::Error error(ostr.str(),
                                   this,
                                   El::Service::Error::WARNING);
          
          callback_->notify(&error);
        }
        catch(const ::NewsGate::Message::ImplementationException& e)
        {
          posted.clear();

          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::post_to_owners: "
            "ImplementationException caught. "
            "Description:" << std::endl << e.description.in();
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(const CORBA::Exception& e)
        {
          posted.clear();

          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::post_to_owners: "
            "CORBA::Exception caught. Description:" << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        } 
      }
      catch(const El::Exception& e)
      {
        posted.clear();
        
        try
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::post_to_owners: "
            "El::Exception caught. Description:" << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(...)
        {
          El::Service::Error error(
            "NewsGate::Message::MessageManager::post_to_owners: "
            "unknown exception caught",
            this);
          
          callback_->notify(&error);
        }
      }
      
      return false;
    }

    bool
    MessageManager::notify(El::Service::Event* event) throw(El::Exception)
    {
      ACE_High_Res_Timer timer;
            
      if(Application::will_trace(El::Logging::HIGH))
      {        
        timer.start();
      }
      
      std::string type = process_event(event);

      if(Application::will_trace(El::Logging::HIGH))
      {        
        timer.stop();
        
        ACE_Time_Value time;
        timer.elapsed_time(time);
        
        std::ostringstream ostr;
        
        ostr << "NewsGate::Message::MessageManager::notify: event "
             << type << "; time " << El::Moment::time(time)
             << "; " << task_queue_size() << " in queue";
        
        Application::logger()->trace(ostr.str(),
                                     Aspect::MSG_MANAGEMENT,
                                     El::Logging::HIGH);
      }
      
      return true;
    }

    std::string
    MessageManager::process_event(El::Service::Event* event)
      throw(El::Exception)
    {    
      if(BaseClass::notify(event))
      {
        return "base_notify";
      }

      TraverseCache* tc = dynamic_cast<TraverseCache*>(event);
        
      if(tc != 0)
      {
        traverse_cache();
        return "traverse_cache";
      }
      
      SaveMsgStat* sms = dynamic_cast<SaveMsgStat*>(event);
      
      if(sms != 0)
      {
        save_message_stat(sms->stat.release());
        return "save_message_stat";
      }

      SaveMsgSharingInfo* smsi = dynamic_cast<SaveMsgSharingInfo*>(event);
      
      if(smsi != 0)
      {
        save_message_sharing_info(*smsi->info);
        return "save_message_sharing_info";
      }

      ImportMsg* im = dynamic_cast<ImportMsg*>(event);
        
      if(im != 0)
      {
        import_messages(im);
        return "import_messages";
      }      

      ApplyMsgFetchFilters* af = dynamic_cast<ApplyMsgFetchFilters*>(event);
        
      if(af != 0)
      {
        apply_message_fetch_filters();
        return "apply_message_fetch_filters";
      }

      ReapplyMsgFetchFilters* rf =
        dynamic_cast<ReapplyMsgFetchFilters*>(event);
        
      if(rf != 0)
      {
        reapply_message_fetch_filters();
        return "reapply_message_fetch_filters";
      }

      MsgDeleteNotification* mdn = dynamic_cast<MsgDeleteNotification*>(event);
        
      if(mdn != 0)
      {
        msg_delete_notification(mdn);
        return "msg_delete_notification";
      }

      SetMirroredManager* smm = dynamic_cast<SetMirroredManager*>(event);
      
      if(smm != 0)
      {
        set_mirrored_banks(smm->mirrored_manager_ref.c_str(),
                           smm->sharing_id.c_str());
        
        return "set_mirrored_banks";
      }

      if(dynamic_cast<InsertLoadedMsg*>(event) != 0)
      {
        insert_loaded_messages();
        return "insert_loaded_messages";
      }

      DeleteObsoleteMessages* dom =
        dynamic_cast<DeleteObsoleteMessages*>(event);

      if(dom)
      {
        delete_obsolete_messages(dom->table_name.c_str());
        return std::string("delete_obsolete_messages ") + dom->table_name;
      }

      InsertChangedMessages* icm = dynamic_cast<InsertChangedMessages*>(event);

      if(icm)
      {
        insert_changed_messages(icm);
        return "insert_changed_messages";
      }      
      
      std::ostringstream ostr;
      ostr << "NewsGate::Message::MessageManager::notify: unknown " << *event;
      
      El::Service::Error error(ostr.str(), this);
      callback_->notify(&error);
      
      return "unknown";
    }

    void
    MessageManager::wp_increment_counter(const El::Lang& lang,
                                         uint32_t time_index,
                                         const WordPair& wp)
      throw(El::Exception)
    {
      if(word_pair_managers_.get())
      {
        word_pair_manager(LT(lang, time_index % wp_intervals_)).
          wp_increment_counter(lang, time_index, wp);        
      }
      else
      {
//        std::cerr << " I: " << time_index << " " << wp.word1 << "/"
//                  << wp.word2 << " ";

        int count = 0;
        
        LT lt(lang, time_index);
        LTWPCounter::iterator i = ltwp_counter_.find(lt);

        if(i != ltwp_counter_.end())
        {
//          std::cerr << "A,";
          
          WordPairCounter::iterator j = i->second->find(wp);

          if(j != i->second->end())
          {
            count = ++(j->second);          
//            std::cerr << "B" << count << ",";
          }
        }

        if(!count)
        {
//          std::cerr << "C,";
          
          LTWPSet::iterator j = ltwp_set_.find(lt);

          if(j == ltwp_set_.end())
          {
//            std::cerr << "D,";
            j = ltwp_set_.insert(
              std::make_pair(lt, new WordPairSet())).first;
          }
          
          WordPairSet& wps = *j->second;
          WordPairSet::iterator k = wps.find(wp);
          
          if(k == wps.end())
          {
//            std::cerr << "E,";
            wps.insert(wp);
            count = 1;
          }
          else
          {
//            std::cerr << "F,";
            wps.erase(k);

            if(wps.empty())
            {
//              std::cerr << "I,";
              delete j->second;
              ltwp_set_.erase(j);
            }
            
            if(i == ltwp_counter_.end())
            {
//              std::cerr << "J,";
              i = ltwp_counter_.insert(
                std::make_pair(lt, new WordPairCounter())).first;
              
              i->second->resize(100000);
            }
            
            i->second->insert(std::make_pair(wp, 2));
            count = 2;
          }          
        }        
        
//        std::cerr << "|" << count << std::endl;
        
        Guard guard(ltwp_counter_lock_);
        
        inc_wp_freq_distr(count - 1, -1);
        inc_wp_freq_distr(count, 1);
      }      
    }
    
    void
    MessageManager::wp_decrement_counter(const El::Lang& lang,
                                         uint32_t time_index,
                                         const WordPair& wp)
      throw(El::Exception)
    {
      if(word_pair_managers_.get())
      {
        word_pair_manager(LT(lang, time_index % wp_intervals_)).
          wp_decrement_counter(lang, time_index, wp);        
      }      
      else
      {
        int count = 0;
        LT lt(lang, time_index);
        
//        std::cerr << " D: " << time_index << " " << wp.word1 << "/"
//                  << wp.word2 << " ";
        
        LTWPCounter::iterator i = ltwp_counter_.find(lt);

        if(i != ltwp_counter_.end())
        {
//          std::cerr << "A,";
          WordPairCounter& wpc = *i->second;
        
          WordPairCounter::iterator wit = wpc.find(wp);

          if(wit != wpc.end())
          {
            count = --(wit->second);

//            std::cerr << "B" << count << ",";
            
            assert(count);

            if(count == 1)
            {
//              std::cerr << "C,";
              wpc.erase(wit);

              if(wpc.empty())
              {
//                std::cerr << "D,";
                delete i->second;
                ltwp_counter_.erase(i);
              }

              LTWPSet::iterator j = ltwp_set_.find(lt);

              if(j == ltwp_set_.end())
              {
//                std::cerr << "E,";
                
                j = ltwp_set_.insert(
                  std::make_pair(lt, new WordPairSet())).first;
              }
          
              WordPairSet& wps = *j->second;                
              bool res = wps.insert(wp).second;
              assert(res);
            }
          }
        }
        
        if(!count)
        {
//          std::cerr << "F,";
          LTWPSet::iterator i = ltwp_set_.find(lt);
          assert(i != ltwp_set_.end());

          WordPairSet& wps = *i->second;
          WordPairSet::iterator j = wps.find(wp);
          assert(j != wps.end());
          
          wps.erase(j);
          
          if(wps.empty())
          {
//            std::cerr << "I,";
            delete i->second;
            ltwp_set_.erase(i);
          }
        }
        
//        std::cerr << "|" << count << std::endl;
        
        Guard guard(ltwp_counter_lock_);

        inc_wp_freq_distr(count, 1);
        inc_wp_freq_distr(count + 1, -1);
      }        
    }

    uint32_t
    MessageManager::wp_get(const El::Lang& lang,
                           uint32_t time_index,
                           const WordPair& wp)
      throw(El::Exception)
    {  
      int count = 0;      

      if(word_pair_managers_.get())
      {
        count = word_pair_manager(LT(lang, time_index % wp_intervals_)).
          wp_get(lang, time_index, wp);
      }
      else
      {
        LT lt(lang, time_index);
        LTWPCounter::const_iterator i = ltwp_counter_.find(lt);
        
        if(i != ltwp_counter_.end())
        {
          const WordPairCounter& wpc = *i->second;
          WordPairCounter::const_iterator wit = wpc.find(wp);
            
          if(wit != wpc.end())
          {
            count = wit->second;
          }
        }

        if(!count)
        {
          LTWPSet::const_iterator i = ltwp_set_.find(lt);
          
          if(i != ltwp_set_.end())
          {
            WordPairSet& wps = *i->second;

            if(wps.find(wp) != wps.end())
            {
              count = 1;
            }
          }          
        }        
      }
        
      return count;
    }
    
    void
    MessageManager::dump_buff(ostream& ostr,
                              const unsigned char* buff,
                              size_t size)
      throw(El::Exception)
    {
      for(const unsigned char *i(buff), *e(buff + size); i != e; ++i)
      {
        ostr.width(2);
        ostr.fill('0');
        
        ostr << std::hex << (uint32_t)*i;
      }
    }
  }
}
