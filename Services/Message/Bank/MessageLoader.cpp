/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Message/Bank/MessageLoader.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <sstream>
#include <string>

#include <El/Exception.hpp>
#include <El/MySQL/DB.hpp>

#include "MessageLoader.hpp"
#include "BankMain.hpp"

namespace
{
}

namespace NewsGate
{
  namespace Message
  {
    //
    // MessageLoader class
    //
    MessageLoader::MessageLoader(
        MessageLoaderCallback* callback,
        const Config& config,
        uint64_t message_expiration_time)
      throw(Exception, El::Exception)
        : BaseClass(callback, "MessageLoader"),
          config_(config),
          message_expiration_time_(message_expiration_time),
          loaded_(false),
          sleep_(0),
          statement_(0),
          prev_msg_id_(0),
          msg_id_(0),
          msg_flags_(0),
          msg_signature_(0),
          msg_url_signature_(0),
          msg_source_id_(0),
          msg_dict_hash_(0),
          msg_published_(0),
          msg_fetched_(0),
          msg_space_(0),
          msg_lang_(0),
          msg_country_(0),
          msg_source_title_len_(0),
          msg_broken_down_len_(0),    
          msg_categories_len_(0),
          event_id_(0),
          event_capacity_(0),
          msg_impressions_(0),
          msg_clicks_(0),
          msg_visited_(0),
          msg_categorizer_hash_(0),
          dict_hash_(0),
          renorm_all_messages_(false)
    {
      memset(&query_param_, 0, sizeof(query_param_));
      memset(&query_result_, 0, sizeof(query_result_));
      
      memset(msg_source_title_, 0, sizeof(msg_source_title_));
      memset(msg_broken_down_, 0, sizeof(msg_broken_down_));
      memset(msg_categories_, 0, sizeof(msg_categories_));

//      ACE_OS::sleep(20);
      
      const Server::Config::MessageBankType& cfg =
        Application::instance()->config();

      const Server::Config::BankMessageManagerType& man_cfg =
        cfg.message_manager();
      
      std::string filename =
        std::string(man_cfg.cache_file_dir().c_str()) +
        "/MessageManager.cache.renorm";
      
      {
        std::fstream renorm_file;
        renorm_file.open(filename.c_str(), ios::in);

        if(renorm_file.is_open())
        {
          std::string line;
        
          while(std::getline(renorm_file, line))
          {
            std::string tmp;
            El::String::Manip::trim(line.c_str(), tmp);

            if(!tmp.empty())
            {
              if(tmp[0] == '#')
              {
                continue;
              }
              else if(tmp == "*")
              {
                renorm_all_messages_ = true;
              }
              else
              {
                if(strncasecmp(tmp.c_str(), "E:", 2) == 0)
                {
                  renorm_events_.insert(El::Luid(tmp.c_str() + 2));
                }
                else if(strncasecmp(tmp.c_str(), "W:", 2) == 0)
                {                  
                  std::string lower_word;
                  
                  El::String::Manip::utf8_to_uniform(tmp.c_str() + 2,
                                                     lower_word);
                  
                  renorm_words_.insert(lower_word);
                }
                else if(strncasecmp(tmp.c_str(), "N:", 2) == 0)
                {
                  El::Dictionary::Morphology::WordId word_id = 0;

                  if(El::String::Manip::numeric(tmp.c_str() + 2, word_id) &&
                     word_id)
                  {
                    renorm_norm_forms_.insert(word_id);
                  }
                }                  
                else
                {
                  renorm_messages_.insert(Id(tmp.c_str()));
                }
              }
            }
          }
        }
      }

      std::ostringstream ostr;
      ostr << "NewsGate::Message::MessageLoader::MessageLoader: loading from ";

      filename = std::string(man_cfg.cache_file_dir().c_str()) +
        "/MessageManager.cache.msg";
      
      if(man_cfg.message_cache().use_cache_message_file())
      {
        std::string new_name =
          filename + "." + El::Moment(ACE_OS::gettimeofday()).dense_format();

        if(rename(filename.c_str(), new_name.c_str()) == 0)
        {      
          msg_file_.open(new_name.c_str(), ios::in);

          if(msg_file_.is_open())
          {
            msg_bstr_.reset(new El::BinaryInStream(msg_file_));
            *msg_bstr_ >> dict_hash_;
            msg_file_name_ = new_name;
            
            ostr << msg_file_name_;
          }
        }
      }
      else
      {
        unlink(filename.c_str());        
      }

      if(msg_bstr_.get() == 0)
      {
        const Server::Config::DataBaseType& db_cfg = cfg.data_base();

        El::MySQL::ConnectionFactory* connection_factory =
          new El::MySQL::NewConnectionsFactory(0, "utf8");
      
        El::MySQL::DB_var db = db_cfg.unix_socket().empty() ?
          new El::MySQL::DB(db_cfg.user().c_str(),
                            db_cfg.passwd().c_str(),
                            db_cfg.dbname().c_str(),
                            db_cfg.port(),
                            db_cfg.host().c_str(),
                            db_cfg.client_flags(),
                            connection_factory) :
          new El::MySQL::DB(db_cfg.user().c_str(),
                            db_cfg.passwd().c_str(),
                            db_cfg.dbname().c_str(),
                            db_cfg.unix_socket().c_str(),
                            db_cfg.client_flags(),
                            connection_factory);

        connection_ = db->connect();

        ostr << "data base";
      }      
      
      Application::logger()->trace(ostr.str(),
                                   Aspect::MSG_MANAGEMENT,
                                   El::Logging::HIGH);
    }

    MessageLoader::~MessageLoader() throw()
    {
      stmt_clear();

      while(!msg_queue_.empty())
      {
        delete msg_queue_.front();
        msg_queue_.pop_front();
      }

      if(!msg_file_name_.empty())
      {
        unlink(msg_file_name_.c_str());
      }
    }

    bool
    MessageLoader::renorm_msg(const StoredMessage& msg) const throw()
    {
      if(renorm_all_messages_ ||
         renorm_messages_.find(msg.id) != renorm_messages_.end() ||
         renorm_events_.find(msg.event_id) != renorm_events_.end())
      {
        return true;
      }

      if(!renorm_words_.empty() || !renorm_norm_forms_.empty())
      {
        const MessageWordPosition& word_positions = msg.word_positions;
        
        for(size_t i = 0; i < word_positions.size(); ++i)
        {
          const char* word = word_positions[i].first.c_str();
          
          if(renorm_words_.find(word) != renorm_words_.end())
          {
            return true;
          }

          El::Dictionary::Morphology::WordId id =
            El::Dictionary::Morphology::pseudo_id(word);

          if(renorm_norm_forms_.find(id) != renorm_norm_forms_.end())
          {
            return true;
          }          
        }
      }

      if(!renorm_norm_forms_.empty())
      {
        const NormFormPosition& word_positions = msg.norm_form_positions;
        
        for(size_t i = 0; i < word_positions.size(); ++i)
        {
          if(renorm_norm_forms_.find(word_positions[i].first) !=
             renorm_norm_forms_.end())
          {
            return true;
          }
        }
      }

      return false;
    }

    void
    MessageLoader::stmt_create() throw(El::MySQL::Exception, El::Exception)
    {
      if(statement_)
      {
        return;
      }

      statement_ = 0;
      prev_msg_id_ = 0;
      msg_id_ = 0;
      msg_flags_ = 0;
      msg_signature_ = 0;
      msg_url_signature_ = 0;
      msg_source_id_ = 0;
      msg_dict_hash_ = 0;
      msg_published_ = 0;
      msg_fetched_ = 0;
      msg_space_ = 0;
      msg_lang_ = 0;
      msg_country_ = 0;
      msg_source_title_len_ = 0;
      msg_broken_down_len_ = 0;
      msg_categories_len_ = 0;
      
      memset(&query_param_, 0, sizeof(query_param_));
      memset(&query_result_, 0, sizeof(query_result_));
      
      memset(msg_source_title_, 0, sizeof(msg_source_title_));
      memset(msg_broken_down_, 0, sizeof(msg_broken_down_));

      event_id_ = 0;
      event_capacity_ = 0;
      msg_impressions_ = 0;
      msg_clicks_ = 0;
      msg_visited_ = 0;
      
      msg_categorizer_hash_ = 0;
      memset(msg_categories_, 0, sizeof(msg_categories_));

      MYSQL* mysql = connection_->mysql();

      statement_ = mysql_stmt_init(mysql);

      if(statement_ == 0)
      {
        std::ostringstream ostr;
        
        ostr << "NewsGate::Message::MessageLoader::stmt_create: "
          "mysql_stmt_init failed. Error code "  << std::dec
             << mysql_errno(mysql) << ", description:\n"
             << mysql_error(mysql) << std::endl;
        
        throw El::MySQL::Exception(ostr.str());
      }

      try
      {
        std::ostringstream query_str;
        
        query_str << "select Message.id, flags, signature, "
          "url_signature, source_id, MessageDict.hash, published, "
          "fetched, space, lang, country, source_title, "
          "broken_down, event_id, event_capacity, "
          "impressions, clicks, visited, categorizer_hash, categories "
          "from Message left join MessageStat on Message.id=MessageStat.id "
          "left join MessageCat on Message.id=MessageCat.id "
          "left join MessageDict on Message.id=MessageDict.id "
          "where Message.id > ? ORDER BY Message.id LIMIT "
                  << config_.read_chunk_size();

        std::string query = query_str.str();
        
        if(mysql_stmt_prepare(statement_, query.c_str(), query.length()))
        {
          std::ostringstream ostr;
          
          ostr << "NewsGate::Message::MessageLoader::stmt_create: "
            "mysql_stmt_prepare failed. Error code "  << std::dec
               << mysql_stmt_errno(statement_) << ", description:\n"
               << mysql_stmt_error(statement_) << std::endl;
    
          throw El::MySQL::Exception(ostr.str());
        }
  
        query_param_.buffer_type = MYSQL_TYPE_LONGLONG;
        query_param_.buffer = &prev_msg_id_;
        query_param_.is_unsigned = true;
  
        if(mysql_stmt_bind_param(statement_, &query_param_))
        {      
          std::ostringstream ostr;
          
          ostr << "NewsGate::Message::MessageLoader::stmt_create: "
            "mysql_stmt_bind_param failed. Error code "  << std::dec
               << mysql_stmt_errno(statement_) << ", description:\n"
               << mysql_stmt_error(statement_) << std::endl;
          
          throw El::MySQL::Exception(ostr.str());
        }

        size_t i = 0;
        
        query_result_[i].buffer_type = MYSQL_TYPE_LONGLONG;
        query_result_[i].buffer = &msg_id_;
        query_result_[i++].is_unsigned = true;

        query_result_[i].buffer_type = MYSQL_TYPE_TINY;
        query_result_[i].buffer = &msg_flags_;
        query_result_[i++].is_unsigned = true;

        query_result_[i].buffer_type = MYSQL_TYPE_LONGLONG;
        query_result_[i].buffer = &msg_signature_;
        query_result_[i++].is_unsigned = true;

        query_result_[i].buffer_type = MYSQL_TYPE_LONGLONG;
        query_result_[i].buffer = &msg_url_signature_;
        query_result_[i++].is_unsigned = true;

        query_result_[i].buffer_type = MYSQL_TYPE_LONGLONG;
        query_result_[i].buffer = &msg_source_id_;
        query_result_[i++].is_unsigned = true;

        query_result_[i].buffer_type = MYSQL_TYPE_LONG;
        query_result_[i].buffer = &msg_dict_hash_;
        query_result_[i++].is_unsigned = true;

        query_result_[i].buffer_type = MYSQL_TYPE_LONGLONG;
        query_result_[i].buffer = &msg_published_;
        query_result_[i++].is_unsigned = true;

        query_result_[i].buffer_type = MYSQL_TYPE_LONGLONG;
        query_result_[i].buffer = &msg_fetched_;
        query_result_[i++].is_unsigned = true;
  
        query_result_[i].buffer_type = MYSQL_TYPE_SHORT;
        query_result_[i].buffer = &msg_space_;
        query_result_[i++].is_unsigned = true;

        query_result_[i].buffer_type = MYSQL_TYPE_SHORT;
        query_result_[i].buffer = &msg_lang_;
        query_result_[i++].is_unsigned = true;

        query_result_[i].buffer_type = MYSQL_TYPE_SHORT;
        query_result_[i].buffer = &msg_country_;
        query_result_[i++].is_unsigned = true;
  
        query_result_[i].buffer_type = MYSQL_TYPE_VAR_STRING;
        query_result_[i].buffer = msg_source_title_;
        query_result_[i].buffer_length = sizeof(msg_source_title_);
        query_result_[i++].length = &msg_source_title_len_;

        query_result_[i].buffer_type = MYSQL_TYPE_BLOB;
        query_result_[i].buffer = msg_broken_down_;
        query_result_[i].buffer_length = sizeof(msg_broken_down_);
        query_result_[i++].length = &msg_broken_down_len_;

        query_result_[i].buffer_type = MYSQL_TYPE_LONGLONG;
        query_result_[i].buffer = &event_id_;
        query_result_[i++].is_unsigned = true;
  
        query_result_[i].buffer_type = MYSQL_TYPE_LONG;
        query_result_[i].buffer = &event_capacity_;
        query_result_[i++].is_unsigned = true;

        query_result_[i].buffer_type = MYSQL_TYPE_LONGLONG;
        query_result_[i].buffer = &msg_impressions_;
        query_result_[i++].is_unsigned = true;

        query_result_[i].buffer_type = MYSQL_TYPE_LONGLONG;
        query_result_[i].buffer = &msg_clicks_;
        query_result_[i++].is_unsigned = true;

        query_result_[i].buffer_type = MYSQL_TYPE_LONGLONG;
        query_result_[i].buffer = &msg_visited_;
        query_result_[i++].is_unsigned = true;

        query_result_[i].buffer_type = MYSQL_TYPE_LONG;
        query_result_[i].buffer = &msg_categorizer_hash_;
        query_result_[i++].is_unsigned = true;

        query_result_[i].buffer_type = MYSQL_TYPE_BLOB;
        query_result_[i].buffer = msg_categories_;
        query_result_[i].buffer_length = sizeof(msg_categories_);
        query_result_[i++].length = &msg_categories_len_;
        
        
        assert(i == sizeof(query_result_) / sizeof(query_result_[0]));
        
        if(mysql_stmt_bind_result(statement_, query_result_))
        {
          std::ostringstream ostr;
        
          ostr << "NewsGate::Message::MessageLoader::stmt_create: "
            "mysql_stmt_bind_result failed. Error code "  << std::dec
               << mysql_stmt_errno(statement_) << ", description:\n"
               << mysql_stmt_error(statement_) << std::endl;        
          
          throw El::MySQL::Exception(ostr.str());
        }
      }
      catch(...)
      {
        stmt_clear();
        throw;
      }      
      
    }
    
    void
    MessageLoader::stmt_clear() throw()
    {
      if(statement_)
      {
        mysql_stmt_close(statement_);
        statement_ = 0;
      }
    }
    
    bool
    MessageLoader::start() throw(Exception, El::Exception)
    {      
      if(!BaseClass::start())
      {
        return false;
      }

      {
        ReadGuard guard(srv_lock_);

        if(!loaded_)
        {
          El::Service::CompoundServiceMessage_var msg = new LoadMessages(this);
          deliver_now(msg.in());
        }
      }

      return true;
    }

    void
    MessageLoader::load_messages(LoadMessages* lm) throw(El::Exception)
    {
      bool load_finished = false;
      time_t delay = 0;
        
      try
      {
        if(msg_bstr_.get() == 0)
        {
          stmt_create();
        }
        
        load_finished = load_messages(delay);
      }
      catch(const El::MySQL::Exception& e)
      {
        stmt_clear();
        
        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageLoader::load_messages: "
          "El::MySQL::Exception caught. Description:" << std::endl << e;
        
        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
        
        delay = config_.retry_period();
      }

      if(!load_finished)
      {        
        deliver_at_time(lm, ACE_OS::gettimeofday() + ACE_Time_Value(delay));
      }
    }

    void
    MessageLoader::sleep(bool val) throw()
    {
      WriteGuard guard(srv_lock_);

      if(val)
      {
        ++sleep_;
      }
      else
      {
        assert(sleep_);
        --sleep_;
      }
    }
    
    bool
    MessageLoader::load_messages(time_t& delay)
      throw(El::MySQL::Exception, El::Exception)
    {
      bool sleep = false;
      bool queue_if_full = false;
      
      {
        ReadGuard guard(srv_lock_);
        queue_if_full = msg_queue_.size() > config_.max_queue_size();
        sleep = sleep_ > 0;
      }

      if(sleep)
      {
        delay = 1;

        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageLoader::load_messages: "
          "sleeping; will retry in " << delay << " sec";

        Application::logger()->trace(ostr.str(),
                                     Aspect::MSG_MANAGEMENT,
                                     El::Logging::HIGH);
        
        return false;
      }        
      
      if(queue_if_full)
      {
        delay = config_.max_queue_size();

        std::ostringstream ostr;
        ostr << "NewsGate::Message::MessageLoader::load_messages: "
          "queue is full; will retry in " << delay << " sec";

        Application::logger()->trace(ostr.str(),
                                     Aspect::MSG_MANAGEMENT,
                                     El::Logging::HIGH);
        
        return false;
      }
        
      ACE_Time_Value read_time;      
      ACE_High_Res_Timer timer;
          
      if(Application::will_trace(El::Logging::HIGH))
      {
        timer.start();
      }
      
      StoredMessageArrayPtr messages(new StoredMessageArray());
      messages->reserve(config_.read_chunk_size());

      std::auto_ptr<std::ostringstream> log_ostr;
            
      if(Application::will_trace(El::Logging::HIGH))
      {
        log_ostr.reset(new std::ostringstream());
            
        *log_ostr << "NewsGate::Message::MessageLoader::load_messages: "
          " messages:\n";
      }

      size_t records = msg_bstr_.get() ?
        load_from_file(messages.get(), log_ostr.get()) :
        load_from_db(messages.get(), log_ostr.get());
      
      bool loaded = records < config_.read_chunk_size();

      if(loaded && msg_bstr_.get())
      {
        msg_bstr_.reset(0);
        msg_file_.close();
        unlink(msg_file_name_.c_str());
      }

      {
        WriteGuard guard(srv_lock_);

        loaded_ = loaded;

        size_t size = msg_queue_.size();
        
        delay = size > 1 ?
          std::min(size - 1, (size_t)config_.max_queue_size()) : 0;
        
        if(records)
        {
          msg_queue_.push_back(messages.release());
        }
      }

      if(Application::will_trace(El::Logging::HIGH))
      {
        timer.stop();
        timer.elapsed_time(read_time);
        
        *log_ostr << records << " msg read " << El::Moment::time(read_time);
        
        if(loaded)
        {
          *log_ostr << "\nLoad completed";
        }
        else
        {
          *log_ostr << "\nNext read in " << delay << " sec";
        }
            
        Application::logger()->trace(log_ostr->str(),
                                     Aspect::MSG_MANAGEMENT,
                                     El::Logging::HIGH);
      }      

      return loaded;
    }

    size_t
    MessageLoader::load_from_file(StoredMessageArray* messages,
                                  std::ostream* log_ostr)
      throw(Exception, El::Exception)
    {
      size_t count = config_.read_chunk_size();
      size_t records = 0;

      try
      {
        for(; records < count; ++records)
        {
          Id id;
        
          try
          {
            *msg_bstr_ >> id;
          }
          catch(const El::BinaryInStream::Exception&)
          {
            break;
          }
        
          messages->push_back(StoredMessage());
          StoredMessage& msg = *messages->rbegin();

          msg.id = id;

          *msg_bstr_
            >> msg.flags >> msg.signature >> msg.url_signature
            >> msg.source_id >> msg.published 
            >> msg.fetched >> msg.space >> msg.lang >> msg.country
            >> msg.event_id >> msg.event_capacity >> msg.impressions
            >> msg.clicks >> msg.visited >> msg.categories;
        
          msg_bstr_->read_string_buff(msg_source_title_,
                                      sizeof(msg_source_title_));
        
          msg.source_title = msg_source_title_;
          msg.read_broken_down(*msg_bstr_);
        
          msg.content = new StoredContent();
          
          msg.content->dict_hash = renorm_msg(msg) ? 0 : dict_hash_;
                
          if(Application::will_trace(El::Logging::HIGH))
          {          
            *log_ostr << msg.id.string() << "/" << msg.event_id.string()
                      << "/" << msg.content->dict_hash << std::endl;
          }
        }
      }
      catch(const El::Exception& e)
      {
        std::string new_name = msg_file_name_ + ".err";
        
        std::ostringstream ostr;
        ostr << "MessageLoader::load_from_file: failed to read a message. "
          "Error Description: " << e << "\nRenaming " << msg_file_name_
             << " to " << new_name;

        if(rename(msg_file_name_.c_str(), new_name.c_str()) < 0)
        {
          int error = ACE_OS::last_error();

          ostr << "\nRename failed. Errno " << error << ". Description:\n"
               << ACE_OS::strerror(error);    
        }

        throw Exception(ostr.str());
      }
      
      return records;
    }

    size_t
    MessageLoader::load_from_db(StoredMessageArray* messages,
                                std::ostream* log_ostr)
      throw(El::MySQL::Exception, El::Exception)
    {
      size_t records = 0;
      
      if(mysql_stmt_execute(statement_))
      {
        std::ostringstream ostr;
        
        ostr << "NewsGate::Message::MessageLoader::load_from_db: "
          "mysql_stmt_execute failed. Error code "  << std::dec
             << mysql_stmt_errno(statement_) << ", description:\n"
             << mysql_stmt_error(statement_) << std::endl;        
      
        throw El::MySQL::Exception(ostr.str());
      }

      try
      {
        if(mysql_stmt_store_result(statement_))
        {
          std::ostringstream ostr;
        
          ostr << "NewsGate::Message::MessageLoader::load_from_db: "
            "mysql_stmt_store_result failed. Error code "  << std::dec
               << mysql_stmt_errno(statement_) << ", description:\n"
               << mysql_stmt_error(statement_) << std::endl;        
      
          throw El::MySQL::Exception(ostr.str());
        }

        uint64_t cur_time = ACE_OS::gettimeofday().sec();
        uint64_t expired = cur_time - message_expiration_time_;
        
        int r = 0;
      
        for(msg_id_ = 0; (r = mysql_stmt_fetch(statement_)) == 0; ++records)
        {
          messages->push_back(StoredMessage());
          StoredMessage& msg = *messages->rbegin();

          msg.id = Id(msg_id_);
          msg.published = msg_published_;

          if(!msg.published)
          {
            // Can be message categories or smth. saved to DB after message
            // deletion as there is no write lock protection for DB
            // operations by performance reasons. If there were
            // UPDATE .. SELECT MySQL statement this
            // situation could be avoided. Meanwhile there is nothing critical
            // in update-dead-message situation so they could be deleted
            // automatically on next server reload event.

            event_id_ = 0;
            event_capacity_ = 0;
            msg_impressions_ = 0;
            msg_clicks_ = 0;
            msg_visited_ = 0;
            
            msg_categories_len_ = 0;
            msg_categorizer_hash_ = 0;
            msg_dict_hash_ = 0;
            
            continue;
          }
          
          msg.lang = El::Lang((El::Lang::ElCode)msg_lang_);
          msg.flags = msg_flags_;
          msg.signature = msg_signature_;
          msg.url_signature = msg_url_signature_;
          msg.source_id = msg_source_id_;
          msg.fetched = msg_fetched_;
          msg.space = msg_space_;

          std::string title(msg_source_title_, msg_source_title_len_);
          msg.source_title = title.c_str();

          if(msg.fetched == 0)
          {
            msg.fetched = msg.published;
          }
                
          msg.country = El::Country((El::Country::ElCode)msg_country_);

          msg.event_id.data = event_id_;
          msg.event_capacity = event_capacity_;
          msg.impressions = msg_impressions_;
          msg.clicks = msg_clicks_;
          msg.visited = msg_visited_;

          // Values from MessageStat reset as can be absent for next message
          event_id_ = 0;
          event_capacity_ = 0;
          msg_impressions_ = 0;
          msg_clicks_ = 0;
          msg_visited_ = 0;

          try
          {
            std::string broken_down_msg(msg_broken_down_,
                                        msg_broken_down_len_);
            
            std::istringstream istr(broken_down_msg);
            msg.read_broken_down(istr);
          }
          catch(const El::BinaryInStream::Exception& e)
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Message::MessageLoader::load_from_db: "
              "El::BinaryInStream::Exception caught while reading words "
              "of message " << msg.id.string() << " (age "
                 << El::Moment::time(ACE_Time_Value(cur_time - msg.published),
                                     true)
                 << ")";

            if(msg.published <= expired)
            {
              ostr << "; will be deleted";
            }
            
            ostr << ". Error description:\n" << e;
            
            El::Service::Error error(ostr.str(), this);
            callback_->notify(&error);
            
            msg.flags |= StoredMessage::MF_DIRTY;
            continue;
          }

          if(msg_categories_len_)
          {
            try
            {
              std::string categories(msg_categories_, msg_categories_len_);
              std::istringstream istr(categories);
              
              msg.categories.read(istr);
              msg.categories.categorizer_hash = msg_categorizer_hash_;
            }
            catch(const El::BinaryInStream::Exception& e)
            {
              std::ostringstream ostr;
              ostr << "NewsGate::Message::MessageLoader::load_from_db: "
                "El::BinaryInStream::Exception caught while reading "
                "categories of message " << msg.id.string() << " (age "
                   << El::Moment::time(
                     ACE_Time_Value(cur_time - msg.published),
                     true)
                   << ")";
            
              if(msg.published <= expired)
              {
                ostr << "; will be deleted";
              }
            
              ostr << ". Error description:\n" << e;
            
              El::Service::Error error(ostr.str(), this);
              callback_->notify(&error);
            
              msg.flags |= StoredMessage::MF_DIRTY;

              msg_categories_len_ = 0;
              msg_categorizer_hash_ = 0;
              msg_dict_hash_ = 0;
              
              continue;
            }
          }
          
          msg.content = new StoredContent();
          
          msg.content->dict_hash = renorm_msg(msg) ? 0 : msg_dict_hash_;
          
          msg_categories_len_ = 0;
          msg_categorizer_hash_ = 0;
          msg_dict_hash_ = 0;
                
          if(Application::will_trace(El::Logging::HIGH))
          {          
            *log_ostr << msg.id.string() << "/" << msg.event_id.string()
                      << "/" << msg.content->dict_hash << std::endl;
          }
        }
    
        if(r && r != MYSQL_NO_DATA)
        {
          std::ostringstream ostr;
        
          ostr << "NewsGate::Message::MessageLoader::load_from_db: "
            "mysql_stmt_fetch failed. Error code "  << std::dec
               << mysql_stmt_errno(statement_) << ", description:\n"
               << mysql_stmt_error(statement_) << std::endl;        
      
          throw El::MySQL::Exception(ostr.str());      
        }
      }
      catch(...)
      {
        mysql_stmt_free_result(statement_);
        throw;
      }

      mysql_stmt_free_result(statement_);
      
      if(msg_id_)
      {
        prev_msg_id_ = msg_id_;
      }

      return records;
    }

    Message::StoredMessageArray*
    MessageLoader::pop_messages(bool& finished) throw(El::Exception)
    {
      WriteGuard guard(srv_lock_);
      
      if(!msg_queue_.empty())
      {
        finished = false;
        
        StoredMessageArrayPtr msg_array(msg_queue_.front());
        msg_queue_.pop_front();
        return msg_array.release();
      }

      finished = loaded_;
      return 0;
    }

    void
    MessageLoader::push_messages(Message::StoredMessageArray* messages)
      throw(El::Exception)
    {
      WriteGuard guard(srv_lock_);
      msg_queue_.push_front(messages);
    }
    
    bool
    MessageLoader::notify(El::Service::Event* event) throw(El::Exception)
    {
      if(BaseClass::notify(event))
      {
        return true;
      }

      LoadMessages* lm = dynamic_cast<LoadMessages*>(event);
        
      if(lm != 0)
      {
        load_messages(lm);
        return true;
      }
      
      std::ostringstream ostr;
      ostr << "NewsGate::Message::MessageLoader::notify: unknown " << *event;
      
      El::Service::Error error(ostr.str(), this);
      callback_->notify(&error);
      
      return true;
    }
  }
}
