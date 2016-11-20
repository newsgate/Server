/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Message/Bank/MessageLoader.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_MESSAGELOADER_HPP_
#define _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_MESSAGELOADER_HPP_

#include <iostream>
#include <memory>
#include <deque>
#include <fstream>

#include <ext/hash_set>
#include <google/sparse_hash_map>

#include <El/Exception.hpp>
#include <El/RefCount/All.hpp>
#include <El/Service/CompoundService.hpp>
#include <El/MySQL/DB.hpp>
#include <El/Luid.hpp>

#include <xsd/Config.hpp>

#include <Commons/Message/StoredMessage.hpp>
#include <Commons/Message/TransportImpl.hpp>

namespace NewsGate
{
  namespace Message
  {
    class MessageLoaderCallback : public virtual El::Service::Callback
    {
    };

    class MessageLoader :
      public El::Service::CompoundService<El::Service::Service,
                                          MessageLoaderCallback>
    {
    public:
      EL_EXCEPTION(Exception, El::ExceptionBase);

      typedef std::auto_ptr<StoredMessageArray> StoredMessageArrayPtr;

      typedef Server::Config::MessageBankType::message_manager_type::
      message_loader_type Config;
      
    public:
      MessageLoader(MessageLoaderCallback* callback,
                    const Config& config,
                    uint64_t message_expiration_time)
        throw(Exception, El::Exception);
      
      virtual ~MessageLoader() throw();
      virtual bool start() throw(Exception, El::Exception);

      StoredMessageArray* pop_messages(bool& finished) throw(El::Exception);
      
      void push_messages(Message::StoredMessageArray* messages)
        throw(El::Exception);

      void sleep(bool val) throw();
      
    private:

      virtual bool notify(El::Service::Event* event) throw(El::Exception);

      struct LoadMessages : public El::Service::CompoundServiceMessage
      {
        LoadMessages(MessageLoader* state) throw(El::Exception);
      };

      void load_messages(LoadMessages* lm) throw(El::Exception);
      
      bool load_messages(time_t& delay)
        throw(El::MySQL::Exception, El::Exception);

      void stmt_create() throw(El::MySQL::Exception, El::Exception);
      void stmt_clear() throw();

      size_t load_from_db(StoredMessageArray* messages,
                          std::ostream* log_ostr)
        throw(El::MySQL::Exception, El::Exception);
      
      size_t load_from_file(StoredMessageArray* messages,
                            std::ostream* log_ostr)
        throw(Exception, El::Exception);

      bool renorm_msg(const StoredMessage& msg) const throw();
      
    private:

      typedef El::Service::CompoundService<El::Service::Service,
                                           MessageLoaderCallback>
      BaseClass;      
      
      struct StoredMessageArrayQueue : public std::deque<StoredMessageArray*>
      {
        void clear() throw(El::Exception);
      };

      const Config& config_;
      uint64_t message_expiration_time_;
      StoredMessageArrayQueue msg_queue_;
      bool loaded_;
      size_t sleep_;
      
      El::MySQL::Connection_var connection_;
      MYSQL_STMT* statement_;
      MYSQL_BIND query_param_;
      MYSQL_BIND query_result_[20];

      unsigned long long prev_msg_id_;
      unsigned long long msg_id_;
      unsigned char msg_flags_;
      unsigned long long msg_signature_;
      unsigned long long msg_url_signature_;
      unsigned long long msg_source_id_;
      unsigned int msg_dict_hash_;
      unsigned long long msg_published_;
      unsigned long long msg_fetched_;
      unsigned short msg_space_;
      unsigned short msg_lang_;
      unsigned short msg_country_;

      // zero terminated utf8 encoded 1024 chars
      char msg_source_title_[1024 * 6 + 1]; 
      unsigned long msg_source_title_len_;

      char msg_broken_down_[65535];
      unsigned long msg_broken_down_len_;
    
      char msg_categories_[65535];
      unsigned long msg_categories_len_;

      unsigned long long event_id_;
      unsigned int event_capacity_;
      unsigned long long msg_impressions_;
      unsigned long long msg_clicks_;
      unsigned long long msg_visited_;
      unsigned int msg_categorizer_hash_;

      std::string msg_file_name_;
      std::fstream msg_file_;
      std::auto_ptr<El::BinaryInStream> msg_bstr_;
      uint32_t dict_hash_;

      struct LuidSet : public google::sparse_hash_set<El::Luid, El::Hash::Luid>
      {
        LuidSet() throw(El::Exception){set_deleted_key(El::Luid::nonexistent);}
      };

      struct WordIdSet :
        public google::sparse_hash_set<
        El::Dictionary::Morphology::WordId,
        El::Hash::Numeric<El::Dictionary::Morphology::WordId> >
      {
        WordIdSet() throw(El::Exception) { set_deleted_key(0); }
      };

      typedef __gnu_cxx::hash_set<std::string, El::Hash::String> StringSet;
      
      bool renorm_all_messages_;
      IdSet renorm_messages_;
      LuidSet renorm_events_;
      StringSet renorm_words_;
      WordIdSet renorm_norm_forms_;
    };
    
    typedef El::RefCount::SmartPtr<MessageLoader> MessageLoader_var;
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
    // NewsGate::Message::MessageLoader::LoadMessages class
    //
    inline
    MessageLoader::LoadMessages::LoadMessages(MessageLoader* state)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }
    
    //
    // NewsGate::Message::MessageLoader::StoredMessageArrayQueue class
    //
    inline
    void
    MessageLoader::StoredMessageArrayQueue::clear() throw(El::Exception)
    {
      for(iterator it = begin(); it != end(); ++it)
      {
        delete *it;
      }

      std::deque<StoredMessageArray*>::clear();
    }
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_MESSAGELOADER_HPP_
