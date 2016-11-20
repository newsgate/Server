/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Event/Bank/EventManager.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <stdlib.h>
#include <string.h>

#include <sstream>
#include <fstream>
#include <string>
#include <list>
#include <utility>
#include <vector>
#include <iostream>
#include <limits>
#include <algorithm>

#include <google/dense_hash_set>
#include <google/dense_hash_map>

#include <ace/OS.h>
#include <ace/High_Res_Timer.h>

#include <El/Exception.hpp>
#include <El/MySQL/DB.hpp>
#include <El/Utility.hpp>
#include <El/Moment.hpp>
#include <El/Guid.hpp>
#include <El/Logging/Logger.hpp>
#include <El/Service/CompoundService.hpp>
#include <El/Stat.hpp>

#include <Commons/Event/TransportImpl.hpp>
#include <Services/Commons/Message/BankClientSessionImpl.hpp>

#include "SubService.hpp"

#include "EventManager.hpp"
#include "BankMain.hpp"
#include "EventRecord.hpp"

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
  static const size_t pw_arr_len = 100;
  static const size_t pw2_arr_len = 100;
  static const size_t pw3_arr_len = 100;
  static const size_t penalty_arr_len = 100;
  static const double pw_power = 1.1;
  static const double pw2_power = 2;
  static const double pw3_power = 3;
  static const size_t max_overlap_array_len = 100;
  static const size_t overlap_factor_array_len = 100;
  
  std::cerr << "    const double EventObject::pw_array_"
    "[EventObject::pw_array_len_] =\n    {";
    
  for(size_t i = 0; i < pw_arr_len; ++i)
  {
    if(i % 8 == 0)
    {
      std::cerr << "\n      ";
    }
    
    std::cerr << pow(i, pw_power);

    if(i < pw_arr_len - 1)
    {
      std::cerr << ", ";
    }
  }

  std::cerr << "\n    };\n\n";

  std::cerr << "    const double EventObject::pw2_array_"
    "[EventObject::pw2_array_len_] =\n    {";
    
  for(size_t i = 0; i < pw2_arr_len; ++i)
  {
    if(i % 8 == 0)
    {
      std::cerr << "\n      ";
    }
    
    std::cerr << pow(i, pw2_power);

    if(i < pw2_arr_len - 1)
    {
      std::cerr << ", ";
    }
  }

  std::cerr << "\n    };\n\n";

  std::cerr << "    const double EventObject::pw3_array_"
    "[EventObject::pw3_array_len_] =\n    {";
    
  for(size_t i = 0; i < pw3_arr_len; ++i)
  {
    if(i % 8 == 0)
    {
      std::cerr << "\n      ";
    }
    
    std::cerr << pow(i, pw3_power);

    if(i < pw3_arr_len - 1)
    {
      std::cerr << ", ";
    }
  }

  std::cerr << "\n    };\n\n";

  std::cerr << "    const double EventObject::pw_mult_array_"
    "[EventObject::pw_array_len_]\n"
    "      [EventObject::pw_array_len_] =\n    {";

  for(size_t i = 0; i < pw_arr_len; ++i)
  {
    std::cerr << "\n      {";

    for(size_t j = 0; j < pw_arr_len; ++j)
    {
      if(j % 7 == 0)
      {
        std::cerr << "\n        ";
      }
    
      std::cerr << pow(i, pw_power) * pow(j, pw_power);

      if(j < pw_arr_len - 1)
      {
        std::cerr << ", ";
      }
    }

    std::cerr << "\n      }";

    if(i < pw_arr_len - 1)
    {
      std::cerr << ",";
    }
  }
  
  std::cerr << "\n    };\n\n";  

  std::cerr << "    const double EventObject::pw_s2_array_"
    "[EventObject::pw_array_len_] =\n    {";

  static const double pw_power2 = pw_power * 2.0;
    
  for(size_t i = 0; i < pw_arr_len; ++i)
  {
    double sum = 0;
    
    for(size_t j = 1; j <= i; ++j)
    {
      sum += pow(j, pw_power2);
    }
    
    if(i % 5 == 0)
    {
      std::cerr << "\n      ";
    }
    
    std::cerr << sum;

    if(i < pw_arr_len - 1)
    {
      std::cerr << ", ";
    }
  }
    
  std::cerr << "\n    };\n\n";

  std::cerr << "    const size_t EventObject::penalty_array_"
    "[EventObject::penalty_array_len_] =\n    {";
    
  for(size_t i = 0; i < penalty_arr_len; ++i)
  {    
    if(i % 5 == 0)
    {
      std::cerr << "\n      ";
    }
    
    std::cerr << i * (i + 1) / 2 * 5;

    if(i < penalty_arr_len - 1)
    {
      std::cerr << ", ";
    }
  }
    
  std::cerr << "\n    };\n\n";

  std::cerr << "    const double EventObject::max_overlap_array_\n"
    "      [EventObject::max_overlap_array_len_]\n"
    "      [EventObject::max_overlap_array_len_] =\n    {";

  for(size_t i = 0; i < max_overlap_array_len; ++i)
  {
    std::cerr << "\n      {";

    for(size_t j = 0; j < max_overlap_array_len; ++j)
    {
      if(j % 7 == 0)
      {
        std::cerr << "\n        ";
      }

      double max_overlap = 0;
      size_t len_short = std::min(i, j);
      size_t len_long = std::max(i, j);

      for(; len_short; --len_short, --len_long)
      {
        max_overlap += pow(len_short, pw_power) * pow(len_long, pw_power);
      }
      
      std::cerr << max_overlap;

      if(j < max_overlap_array_len - 1)
      {
        std::cerr << ", ";
      }
    }

    std::cerr << "\n      }";

    if(i < max_overlap_array_len - 1)
    {
      std::cerr << ",";
    }
  }
  
  std::cerr << "\n    };\n\n";  

  std::cerr << "    const double EventObject::rev_max_overlap_array_\n"
    "      [EventObject::max_overlap_array_len_]\n"
    "      [EventObject::max_overlap_array_len_] =\n    {";

  for(size_t i = 0; i < max_overlap_array_len; ++i)
  {
    std::cerr << "\n      {";

    for(size_t j = 0; j < max_overlap_array_len; ++j)
    {
      if(j % 7 == 0)
      {
        std::cerr << "\n        ";
      }

      double max_overlap = 0;
      size_t len_short = std::min(i, j);
      size_t len_long = std::max(i, j);

      for(; len_short; --len_short, --len_long)
      {
        max_overlap += pow(len_short, pw_power) * pow(len_long, pw_power);
      }
      
      std::cerr << ((double)100.0 / std::max(max_overlap, 1.0));

      if(j < max_overlap_array_len - 1)
      {
        std::cerr << ", ";
      }
    }

    std::cerr << "\n      }";

    if(i < max_overlap_array_len - 1)
    {
      std::cerr << ",";
    }
  }
  
  std::cerr << "\n    };\n\n";  

  std::cerr << "    const double EventObject::overlap_factor_array_\n"
    "      [EventObject::overlap_factor_array_len_]\n"
    "      [EventObject::overlap_factor_array_len_] =\n    {";
  
  for(size_t i = 0; i < overlap_factor_array_len; ++i)
  {
    std::cerr << "\n      {";

    for(size_t j = 0; j < overlap_factor_array_len; ++j)
    {
      if(j % 7 == 0)
      {
        std::cerr << "\n        ";
      }
      
      double sum = 0;
      size_t val = i;
      
      for(; val; --val)
      {
        sum += pow(val, pw_power2);
      }
      
      std::cerr << (i && j ? (double)i / j / sum : 0);

      if(j < overlap_factor_array_len - 1)
      {
        std::cerr << ", ";
      }
    }

    std::cerr << "\n      }";

    if(i < overlap_factor_array_len - 1)
    {
      std::cerr << ",";
    }
  }
  
  std::cerr << "\n    };\n\n";  
}

static ABC abc;
*/
//
// EventManager::HashPair struct
//

namespace NewsGate
{
  namespace Event
  {
    const EventManager::HashPair EventManager::HashPair::zero;
    
    const EventManager::HashPair
    EventManager::HashPair::unexistent(UINT32_MAX, 0, El::Lang::null);

    EventManager::DBMutex EventManager::event_buff_lock_;
    EventManager::DBMutex EventManager::message_buff_lock_;
    
    EventManager::EventManager(
      Event::BankSession* session,
      EventManagerCallback* callback,
      bool load,
      bool all_events_changed,
      const Server::Config::BankEventManagerType& config)
      throw(El::Exception)
        : El::Service::CompoundService<
            El::Service::Service, // Threads for merge, traverse, remake
            EventManagerCallback>(callback, "EventManager", 3),
          config_(config),
          max_core_words_(config.event().max_core_words()),
          max_message_core_words_(config.event().max_message_core_words()),
          max_size_(config.event().max_size()),
          max_time_range_(config.event().max_time_range()),          
          merge_level_base_(config.event().merge_level_base()),
          merge_level_min_(config.event().merge_level_min()),
          merge_level_size_based_decrement_step_(
            config.event().merge_level_size_based_decrement_step()),
          merge_max_strain_(config.event().merge_max_strain()),
          merge_max_time_diff_(config.event().merge_max_time_diff()),
          min_rift_time_(config.event().min_rift_time()),
          merge_deny_size_factor_(config.event().merge_deny_size_factor()),
          merge_deny_max_time_(config.event().merge_deny_max_time()),
          merge_level_time_based_increment_step_(
            config.event().merge_level_time_based_increment_step() / 86400),
          merge_level_range_based_increment_step_(
            config.event().merge_level_range_based_increment_step() / 86400),
          merge_level_strain_based_increment_step_(
            config.event().merge_level_strain_based_increment_step()),
          remake_min_improve_(config_.event_remake().min_improve()),
          remake_min_size_(config_.event_remake().min_size_remake()),
          remake_min_size_revise_(config_.event_remake().min_size_revise()),
          remake_min_part_(config_.event_remake().min_part()),
          events_loaded_(!load),
          events_unloaded_(false),
          all_events_changed_(all_events_changed),
          message_time_lower_boundary_(std::numeric_limits<time_t>::max()),
          msg_core_words_next_preemt_(0),
          merge_blacklist_cleanup_time_(0),
          last_event_number_(0),
//          next_revision_time_(ACE_Time_Value::zero),
          traverse_event_it_(traverse_event_.end()),
          remake_traverse_event_it_(remake_traverse_event_.end()),
          changed_events_sum_(0),
          changed_events_avg_(0),
          traverse_period_(config_.event_cache().traverse_period_min()),
          dissenters_(0)
//          cleanup_allowed_(false),
//          traverse_period_increased_(false)
    {
//      ACE_OS::sleep(20);
      
      memset(changed_events_counters_, 0, sizeof(changed_events_counters_));
      
      if(merge_level_size_based_decrement_step_ <= 0 &&
         merge_level_time_based_increment_step_ >= 0 &&
         merge_level_range_based_increment_step_ >= 0)
      {
        merge_level_min_ = merge_level_base_;
      }
        
      session->_add_ref();
      session_ = session;

      try
      {
        std::string message_bank_manager_ref =
          config_.message_bank_manager_ref();

        if(!message_bank_manager_ref.empty())
        {
          CORBA::Object_var obj =
            Application::instance()->orb()->string_to_object(
              message_bank_manager_ref.c_str());
      
          if(CORBA::is_nil(obj.in()))
          {
            std::ostringstream ostr;
            ostr << "::NewsGate::Event::EventManager::EventManager: "
              "string_to_object() gives nil reference for "
                 << message_bank_manager_ref;
        
            throw Exception(ostr.str().c_str());
          }

          Message::BankManager_var manager =
            Message::BankManager::_narrow(obj.in());
        
          if (CORBA::is_nil(manager.in()))
          {
            std::ostringstream ostr;
            ostr << "::NewsGate::Event::EventManager::EventManager: "
              "Message::BankManager::_narrow gives nil reference for "
                 << message_bank_manager_ref;
      
            throw Exception(ostr.str().c_str());
          }

          bank_client_session_ = manager->bank_client_session();

          Message::BankClientSessionImpl* bank_client_session =
            dynamic_cast<Message::BankClientSessionImpl*>(
              bank_client_session_.in());
      
          if(bank_client_session == 0)
          {
            throw Exception(
              "::NewsGate::Event::EventManager::EventManager: "
              "dynamic_cast<Message::BankClientSessionImpl*> failed");
          }

          bank_client_session->init_threads(this);
        }
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "::NewsGate::Event::EventManager::EventManager: "
          "retrieving message bank client session failed. Reason:\n" << e;

        throw Exception(ostr.str());
      }
    }
    
    EventManager::~EventManager() throw()
    {
    }
    
    El::Luid
    EventManager::get_event_id() throw(El::Exception)
    {
      El::Luid event_id;
      
      do
      {
        event_id.generate();
      }
      while(id_to_number_map_.find(event_id) != id_to_number_map_.end());

      return event_id;
    }
    
    uint32_t
    EventManager::get_event_number() throw(El::Exception)
    {
      do
      {
        last_event_number_++;
      }
      while(last_event_number_ == NUMBER_ZERO ||
            last_event_number_ == NUMBER_UNEXISTENT ||
            events_.find(last_event_number_) != events_.end());

      return last_event_number_;
    }

    void EventManager::add_lang(const El::Lang& lang) throw(El::Exception)
    {
      WriteGuard guard(srv_lock_);
      langs_.insert(lang);
    }
    
    void
    EventManager::load_events() throw()
    {
      try
      {
        bool load_finished = false;
        bool chunk_read = false;

        typedef std::vector<El::Luid> EventIdArray;
        EventIdArray broken_events;

        std::string lang_filter;
        
        {
          std::ostringstream ostr;
          ostr << " and lang in ( ";
          
          {  
            ReadGuard guard(srv_lock_);
            
            for(LangSet::const_iterator it = langs_.begin();
                it != langs_.end(); ++it)
            {
              if(it != langs_.begin())
              {
                ostr << ", ";
              }
              
              ostr << it->el_code();
            }
          }
          
          ostr << " )";
          lang_filter = ostr.str();
        }
        
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          El::MySQL::Connection_var connection2 =
            Application::instance()->dbase()->connect();
          
          std::ostringstream ostr;
          ostr << "select event_id, data from Event where event_id>"
               << event_load_last_id_.data << lang_filter
               << " order by event_id limit " << config_.read_chunk_size();

          ACE_High_Res_Timer timer;
          timer.start();
          
          El::MySQL::Result_var result =
            connection->query(ostr.str().c_str());

          size_t loaded_events = 0;
          size_t deleted_messages = 0;

          EventRecord record(result.in());
          std::auto_ptr<std::ostringstream> log_ostr;
            
          if(Application::will_trace(El::Logging::MIDDLE))
          {
            log_ostr.reset(new std::ostringstream());
            
            *log_ostr << "NewsGate::Event::EventManager::load_events("
                      << lang_string() << "):\n";
          }

          {            
            WriteGuard guard(srv_lock_);
          
            for(; record.fetch_row(); ++loaded_events)
            {
              uint64_t id = record.event_id().value();
/*
              if(id == 0x9EE7514132E8FD49ULL)
              {
                std::cerr << "AAA\n";
              }
*/              
              if(id_to_number_map_.find(id) != id_to_number_map_.end())
              {
                continue;
              }

              bool rebuild_event = false;
              std::auto_ptr<EventObject> pevent(new EventObject());
              
              std::string data = record.data();

              std::istringstream istr(data);
              El::BinaryInStream bin_istr(istr);
              
              try
              {
                Message::IdSet msg_ids;

                uint64_t sign = 0;

                bin_istr >> sign;

                if(sign == UINT64_MAX)
                {
                  uint32_t version = 0;
                  bin_istr >> version >> *pevent;
                }
                else
                {
                  rebuild_event = true;
                  
                  std::istringstream istr(data);
                  El::BinaryInStream bin_istr(istr);
                  
                  pevent->read_old(bin_istr);
                }
                
                for(MessageInfoArray::const_iterator
                      i(pevent->messages().begin()),
                      e(pevent->messages().end()); i != e; ++i)
                {
                  const Message::Id& id = i->id;
                  
                  if(!msg_ids.insert(id).second)
                  {
                    std::ostringstream ostr;
                    ostr << "duplicate event message id " << id.string();

                    throw Exception(ostr.str());
                  }

                  if(message_events_.find(id) != message_events_.end())
                  {
                    std::ostringstream ostr;
                    ostr << "globally duplicate event message id "
                         << id.string();

                    throw Exception(ostr.str());
                  }
                }
              }
              catch(const El::Exception& e)
              {
                broken_events.push_back(id);
                
                // It can be not just El::BinaryInStream::Exception but some
                // STL exception like std::bad_alloc due to data corruption
                
                std::ostringstream ostr;
                ostr << "NewsGate::Event::EventManager::load_events("
                      << lang_string() << "): "
                  "failed to read event " << pevent->id.data
                     << " from blob at pos " << bin_istr.read_bytes()
                     << ". rebuild_event=" << rebuild_event
                     << ". Description:" << std::endl << e;
        
                El::Service::Error error(ostr.str(), this);
                callback_->notify(&error);
                
                continue;
              }

              uint32_t event_number = get_event_number();
              
              EventObject& event =
                *events_.insert(
                  std::make_pair(event_number,
                                 pevent.release())).first->second;

//              std::cerr << event.id.string() << std::endl;

              id_to_number_map_[event.id] = event_number;

              for(size_t i = 0; i < event.messages().size(); i++)
              {
                const MessageInfo& mi = event.message(i);

                if(mi.published == 0)
                {
                  ++deleted_messages;
                }
                
                message_events_[mi.id] = event_number;
              }

              if(all_events_changed_ || rebuild_event)
              {
                load_message_core_words(event_number, connection2.in());

                EventWordWeightMap event_words_pw;
        
                get_message_word_weights(event,
                                         event_words_pw,
                                         0,
                                         false);
          
                event.set_words(event_words_pw, max_core_words_, true);
/*
                if(id == 0xF7027E956D38D8B1ULL)
                {
                  std::cerr << event.id.string() << ":";
                  
                  for(EventWordWeightArray::const_iterator
                        i(event.words.begin()), e(event.words.end()); i != e;
                      ++i)
                  {
                    std::cerr << " " << i->word_id << "/" << i->weight << "/"
                              << i->first_count;
                  }

                  std::cerr << std::endl;
                }
*/
                
                event.flags &= ~EventObject::EF_REVISED;

                uint32_t dis_val = 0;

                detach_message_candidate(event,
                                         0, //connection2.in(),
                                         0,
                                         0,
                                         &dis_val);
                
                event.dissenters(dis_val);
                
                if(set_merge(event))
                {
                  changed_events_.insert(EventCardinality(event)); 
                }

//                if(flags != event.flags || dissenters != event.dissenters())
                {
                  event.flags |= EventObject::EF_DIRTY;
                }
              }

              for(size_t i = 0; i < event.words.size(); i++)
              {
                uint32_t word_id = event.words[i].word_id;

                WordToEventNumberMap::iterator it = word_map_.find(word_id);
                
                if(it == word_map_.end())
                {
                  it = word_map_.insert(
                    std::make_pair(word_id, new EventNumberSet())).first;
                }
                
                it->second->insert(event_number);
              }

              dissenters_ += event.dissenters();

//              assert_can_merge(event, "0", false);
              
              if(Application::will_trace(El::Logging::MIDDLE))
              {
                *log_ostr << "  " << event.id.string() << "/" << event_number
                          << std::endl;
              }

              event_load_last_id_ = event.id;
            }
          }
          
          timer.stop();
          ACE_Time_Value load_time;
          timer.elapsed_time(load_time);

          timer.start();
            
          if(!broken_events.empty())
          {
            std::ostringstream ostr;
            ostr << "delete from Event where event_id in (";
              
            for(EventIdArray::const_iterator b(broken_events.begin()), i(b),
                  e(broken_events.end()); i != e; ++i)
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
            *log_ostr << "  * " << loaded_events << " events load time: "
                      << El::Moment::time(load_time)
                      << "\n    " << deleted_messages
                      << " deleted messages\n    " << broken_events.size()
                      << " broken events delete time "
                      << El::Moment::time(delete_time);
            
            Application::logger()->trace(log_ostr->str(),
                                         Aspect::EVENT_MANAGEMENT,
                                         El::Logging::MIDDLE);
          }

          load_finished = loaded_events < config_.read_chunk_size();
          
          if(load_finished && Application::will_trace(El::Logging::MIDDLE))
          {
            std::ostringstream ostr;              
            ostr << "NewsGate::Event::EventManager::load_events("
                 << lang_string() << "): completed;\n  "
                 << events_.size() << " events, "
                 << message_events_.size() << " messages";
            
            Application::logger()->trace(ostr.str(),
                                         Aspect::EVENT_MANAGEMENT,
                                         El::Logging::MIDDLE);
          }

          chunk_read = true;
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::EventManager::load_events("
               << lang_string() << "): "
            "El::Exception caught. Description:" << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }

        try
        {
          El::Service::CompoundServiceMessage_var msg;
          ACE_Time_Value delay;

          if(load_finished)
          {
            msg = new DeleteObsoleteMessages(this);
            delay = ACE_Time_Value::zero;
          }
          else
          {
            msg = new LoadEvents(this);

            delay = chunk_read ?
              ACE_Time_Value(0) :
              ACE_Time_Value(config_.event_cache().event_load_retry_delay());
          }

          deliver_at_time(msg.in(), ACE_OS::gettimeofday() + delay);
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::EventManager::load_events("
               << lang_string() << "): "
            "El::Exception caught while scheduling task. Description:"
               << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }        
        
      }
      catch(...)
      {
        std::ostringstream ostr;
        
        ostr << "NewsGate::Event::EventManager::load_events("
          << lang_string() << "): " "unexpected exception caught.";
        
        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
      }     
    }

    void
    EventManager::delete_obsolete_messages() throw()
    {
      try
      {
        bool load_finished = false;
        bool chunk_read = false;

        Message::IdArray obsolete_messages;

        std::string lang_filter;
        
        {
          std::ostringstream ostr;
          ostr << " and lang in ( ";
          
          {  
            ReadGuard guard(srv_lock_);
            
            for(LangSet::const_iterator it = langs_.begin();
                it != langs_.end(); ++it)
            {
              if(it != langs_.begin())
              {
                ostr << ", ";
              }
              
              ostr << it->el_code();
            }
          }
          
          ostr << " )";
          lang_filter = ostr.str();
        }
        
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          std::ostringstream ostr;
          ostr << "select id from EventMessage where id>"
               << msg_load_last_id_.data << lang_filter
               << " order by id limit " << config_.read_chunk_size();

          ACE_High_Res_Timer timer;
          timer.start();
          
          El::MySQL::Result_var result =
            connection->query(ostr.str().c_str());

          size_t loaded_messages = 0;

          EventMessageRecord record(result.in(), 1);

          {
            WriteGuard guard(srv_lock_);
          
            for(; record.fetch_row(); ++loaded_messages)
            {
              msg_load_last_id_ = record.id().value();

              if(message_events_.find(msg_load_last_id_) ==
                 message_events_.end())
              {
                obsolete_messages.push_back(msg_load_last_id_);
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
            ostr << "delete from EventMessage where id in (";
              
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
            ostr << "NewsGate::Event::EventManager::delete_obsolete_messages("
                 << lang_string() << "):\n  " << loaded_messages
                 << " messages, " << obsolete_messages.size()
                 << " obsolete\n"
                 << "  load time: " << El::Moment::time(load_time)
                 << ", delete time: " << El::Moment::time(delete_time);
            
            Application::logger()->trace(ostr.str(),
                                         Aspect::EVENT_MANAGEMENT,
                                         El::Logging::MIDDLE);
          }
          
          load_finished = loaded_messages < config_.read_chunk_size();
          
          if(load_finished)
          {
            if(Application::will_trace(El::Logging::MIDDLE))
            {              
              std::ostringstream ostr;              
              ostr << "NewsGate::Event::EventManager::"
                "delete_obsolete_messages("
                   << lang_string() << "): completed";
                
              Application::logger()->trace(ostr.str(),
                                           Aspect::EVENT_MANAGEMENT,
                                           El::Logging::MIDDLE);
            }
        
            StatusGuard guard(status_lock_);
            events_loaded_ = true;
          }
          
          chunk_read = true;
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::EventManager::delete_obsolete_messages("
               << lang_string() << "): "
            "El::Exception caught. Description:" << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
          
        try
        {
          El::Service::CompoundServiceMessage_var msg;

          if(load_finished)
          {
            schedule_merge(0);
            
            msg = new TraverseEvents(this);
            deliver_now(msg.in());

            if(config_.event_remake().traverse_records())
            {
              msg = new RemakeTraverseEvents(this);
              deliver_now(msg.in());
            }
          }
          else
          {
            msg = new DeleteObsoleteMessages(this);

            ACE_Time_Value delay = chunk_read ?
              ACE_Time_Value(0) :
              ACE_Time_Value(config_.event_cache().event_load_retry_delay());

            deliver_at_time(msg.in(), ACE_OS::gettimeofday() + delay);
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::EventManager::delete_obsolete_messages("
               << lang_string() << "): "
            "El::Exception caught while scheduling task. Description:"
               << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
      }
      catch(...)
      {
        std::ostringstream ostr;
        
        ostr << "NewsGate::Event::EventManager::delete_obsolete_messages("
          << lang_string() << "): " "unexpected exception caught.";
        
        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
      }     
    }
    
    void
    EventManager::get_events(const Transport::EventIdRelArray& ids,
                             Transport::EventObjectRelArray& events)
      throw(Exception, El::Exception)
    {
      El::MySQL::Connection_var connection =
        Application::instance()->dbase()->connect();
      
      WriteGuard guard(srv_lock_);

      for(Transport::EventIdRelArray::const_iterator i(ids.begin()),
            e(ids.end()); i != e; ++i)
      {
        EventIdToEventNumberMap::const_iterator iit =
          id_to_number_map_.find(i->id);

        if(iit == id_to_number_map_.end())
        {
          continue;
        }

        EventNumber event_number = iit->second;
        const EventObject& event = *events_.find(event_number)->second;
        
        Transport::EventObjectRel event_rel;
        event_rel.rel.object1 = event;
        event_rel.changed = changed_events_.contain(event_rel.rel.object1.id);

        if(i->rel != El::Luid::null)
        {
          event_releations(event_number, i->rel, event_rel.rel, connection);
        }        

        if(i->split != Message::Id::zero)
        {
          split_event(event_number, i->split, event_rel.split, connection);
        }

        if(i->separate)
        {
          separate_event(event_number,
                         i->separate,
                         i->narrow_separation,
                         i->rel,
                         event_rel.separate,
                         connection);
        }
        
/*        
        std::cerr << event_rel.object.id.string() << ":"
                  << ((event_rel.object.flags & EventObject::EF_REVISED) ?
                      "R" : "-") << std::endl;
*/
        events.push_back(event_rel);
      }
    }
    
    void
    EventManager::event_releations(EventNumber event_number,
                                   const El::Luid& related_event,
                                   Transport::EventRel& event_rel,
                                   El::MySQL::Connection* connection)
      throw(El::Exception)
    {
      EventIdToEventNumberMap::const_iterator iit =
        id_to_number_map_.find(related_event);

      if(iit != id_to_number_map_.end())
      {
        event_rel.colocated = 1;

        EventNumber rel_event_number = iit->second;
            
        const EventObject& rel_event =
          *events_.find(rel_event_number)->second;

        event_rel.object2 = rel_event;

        MergeDenialMap::const_iterator it =
          merge_blacklist_.find(HashPair(event_rel.object1.hash(),
                                         rel_event.hash(),
                                         event_rel.object1.lang));

        if(it != merge_blacklist_.end())
        {
          event_rel.merge_blacklist_timeout = it->second.timeout;
        }
            
        EventNumberSet event_set;

        if(event_number)
        {
          event_set.insert(event_number);
        }
        
        event_set.insert(rel_event_number);

        load_message_core_words(event_set, connection);            

        size_t event_size = event_rel.object1.messages().size();
        size_t rel_event_size = rel_event.messages().size();
            
        MessageInfoArray messages(event_size + rel_event_size);
            
        messages.copy(event_rel.object1.messages(), 0, event_size);
        messages.copy(rel_event.messages(), event_size, rel_event_size);
            
        create_event(event_rel.merge_result, messages, event_rel.object1.lang);
      } 
    }
    
    void
    EventManager::split_event(EventNumber event_number,
                              const Message::Id& split_id,
                              Transport::EventParts& parts,
                              El::MySQL::Connection* connection)
      throw(El::Exception)
    {      
      EventNumberSet event_set;
      event_set.insert(event_number);

      load_message_core_words(event_set, connection);

      MessageInfoVector sorted_messages;
      const EventObject& event = *events_.find(event_number)->second;

      for(MessageInfoArray::const_iterator i(event.messages().begin()),
            e(event.messages().end()); i != e; ++i)
      {
        uint64_t published = i->published;
            
        MessageCoreWordsMap::const_iterator it =
          message_core_words_.find(i->id);

        if(!published || it == message_core_words_.end())
        {
          continue;
        }
        
        MessageInfoVector::iterator j(sorted_messages.begin()),
          je(sorted_messages.end());
        
        for(; j != je && j->published > published; ++j);
        sorted_messages.insert(j, *i);
      }

      for(MessageInfoVector::const_iterator i(sorted_messages.begin()),
            e(sorted_messages.end()); i != e; ++i)
      {
        if(i->id == split_id)
        {
          create_event(parts.part1, sorted_messages.begin(), i, event.lang);
          create_event(parts.part2, i, sorted_messages.end(), event.lang);
          break;
        }
      }
    }

    void
    EventManager::separate_event(EventNumber event_number,
                                 uint32_t separate,
                                 bool narrow_separation,
                                 const El::Luid& rel_event_id,
                                 Transport::EventParts& parts,
                                 El::MySQL::Connection* connection)
      throw(El::Exception)
    {
      EventNumberSet event_set;
      event_set.insert(event_number);

      load_message_core_words(event_set, connection);

      MessageInfoVector sorted_messages;
      const EventObject& event = *events_.find(event_number)->second;

      MessageInfoVector messages1;
      MessageInfoVector messages2;

      for(MessageInfoArray::const_iterator i(event.messages().begin()),
            e(event.messages().end()); i != e; ++i)
      {
        MessageCoreWordsMap::const_iterator it =
          message_core_words_.find(i->id);

        if(it == message_core_words_.end())
        {
          continue;
        }

        const Message::CoreWords& words = it->second->words;
//        bool sep = words.size() && words[0] == separate;

        bool sep = false;
          
        for(Message::CoreWords::const_iterator b(words.begin()), j(b),
              e(words.end()); j != e; ++j)
        {
          if(*j == separate && (!narrow_separation || j == b))
          {
            sep = true;
            break;
          }
        }
        
        if(sep)
        {
          messages1.push_back(*i);
        }
        else
        {
          messages2.push_back(*i);
        }
      }

      if(!messages1.empty())
      {
        create_event(parts.part1,
                     messages1.begin(),
                     messages1.end(),
                     event.lang);

        if(rel_event_id != El::Luid::null)
        {
          parts.merge1.object1 = parts.part1;
          event_releations(0, rel_event_id, parts.merge1, connection);
        }
      }
      
      if(!messages2.empty())
      {
        create_event(parts.part2,
                     messages2.begin(),
                     messages2.end(),
                     event.lang);

        if(rel_event_id != El::Luid::null)
        {
          parts.merge2.object1 = parts.part2;
          event_releations(0, rel_event_id, parts.merge2, connection);
        }        
      }
    }
    
    EventManager::State
    EventManager::state() const throw(El::Exception)
    {
      State state;
      
      {
        
        ReadGuard guard(srv_lock_);

        state.languages = langs_;
        state.events = events_.size();
        state.messages = message_events_.size();
        state.changed_events = changed_events_.size();
        state.merge_blacklist_size = merge_blacklist_.size();
        state.task_queue_size = task_queue_size();
      }

      state.loaded = loaded();
      return state;
    }
    
    std::string
    EventManager::lang_string() const throw(El::Exception)
    {
      ReadGuard guard(srv_lock_);
      return langs_.string();
    }
    
    bool
    EventManager::get_message_events(
      const Message::IdArray& message_ids,
      Message::Transport::MessageEventArray& message_events,
      size_t& found_messages,
      size_t& total_message_count,
      std::ostringstream* log_stream)
      throw(Exception, El::Exception)
    {
      size_t i = 0;
      
      {
        ReadGuard guard(srv_lock_);

        total_message_count += message_events_.size();

        if(!loaded())
        {
          return false;
        }

        for(Message::IdArray::const_iterator it = message_ids.begin();
            it != message_ids.end(); ++it, ++i)
        {
          Message::Transport::MessageEvent& message_event = message_events[i];

          if(message_event.event_id != El::Luid::null)
          {
            continue;
          }
          
          const Message::Id& id = *it;
          
//          message_event.message_id = id;
          
          MessageIdToEventNumberMap::const_iterator eit =
            message_events_.find(id);
      
          if(eit != message_events_.end())
          {
            const EventObject& event = *events_.find(eit->second)->second;
            
            message_event.event_id = event.id;
            message_event.event_capacity = event.messages().size();

            if(log_stream)
            {
              *log_stream << "\n    found msg " << id.string() << ":"
                          << event.id.string();
            }

            found_messages++;
          }
        }
      }

      return true;
    }
    
    bool
    EventManager::accept_events(
      const Transport::MessageDigestArray& digests,
      const Transport::EventPushInfoArray& events,
      std::ostringstream* log_stream)
      throw(Exception, El::Exception)
    {
      if(!loaded())
      {
        return false;
      }
    
      if(digests.empty())
      {
        return true;
      }
      
      ACE_High_Res_Timer timer;
      timer.start();

      unsigned long cur_time = ACE_OS::gettimeofday().sec();
          
      unsigned long expire_time =
        cur_time > config_.message_expiration_time() ?
        cur_time - config_.message_expiration_time() : 0;

      std::fstream file;        
      std::string cache_filename;
      
      unsigned long accepted_events = 0;
      unsigned long accepted_messages = 0;
      unsigned long changed_events = 0;
      
      try
      {
        EventMessageDigestMap event_message_digests;

        WriteGuard guard(srv_lock_);

        for(Transport::MessageDigestArray::const_iterator it = digests.begin();
            it != digests.end(); it++)
        {
          if(langs_.find(it->lang) != langs_.end() &&
             it->published > expire_time &&
             message_events_.find(it->id) == message_events_.end())
          {
            event_message_digests[it->event_id].push_back(&(*it));
          }
        }

        changed_events = changed_events_.size();

        for(Transport::EventPushInfoArray::const_iterator it = events.begin();
            it != events.end(); it++)
        {
          const El::Luid& event_id = it->id;
          
          EventMessageDigestMap::const_iterator eit =
            event_message_digests.find(event_id);

          if(eit == event_message_digests.end() ||
             id_to_number_map_.find(event_id) != id_to_number_map_.end())
          {
            continue;
          }
            
          accepted_events++;
          
          unsigned long event_number = get_event_number();      
          
          EventObject& event =
            *events_.insert(
              std::make_pair(event_number, new EventObject())).first->second;

          const MessageDigestPtrArray& digest_ptrs = eit->second;
          assert(!digest_ptrs.empty());

          event.id = event_id;
          event.lang = (*digest_ptrs.begin())->lang;
          event.spin = it->spin;
          event.dissenters(it->dissenters);          
          
          event.flags = (it->flags |
                         EventObject::EF_DIRTY | EventObject::EF_MEM_ONLY) &
            ~EventObject::EF_REVISED;
          
          id_to_number_map_[event_id] = event_number;

          accepted_messages += digest_ptrs.size();

          EventWordWeightMap event_words_pw;
          MessageInfoArray messages(digest_ptrs.size());
          size_t i = 0;
          
          for(MessageDigestPtrArray::const_iterator it = digest_ptrs.begin();
              it != digest_ptrs.end(); it++, i++)
          {
            const Transport::MessageDigest& md = **it;
              
            messages[i] = MessageInfo(md.id, md.published);
            message_events_[md.id] = event_number;

            const Message::CoreWords& msg_core_words = md.core_words;
            
            MessageCoreWordsMap::iterator mit = message_core_words_.find(md.id);

            if(mit == message_core_words_.end())
            {
              mit = message_core_words_.insert(
                std::make_pair(md.id, new MessageCoreWords())).first;
            }
            
            MessageCoreWords& cw = *mit->second;          
            cw.words = msg_core_words;
//            cw.timestamp = current_time;
            
            unsigned long msg_core_words_count = msg_core_words.size();

            for(size_t j = 0; j < msg_core_words_count; j++)
            {
              uint32_t word_id = msg_core_words[j];
          
              unsigned long long weight =
                EventObject::core_word_weight(j,
                                              msg_core_words_count,
                                              max_message_core_words_);
          
              EventWordWeightMap::iterator it = event_words_pw.find(word_id);

              if(it == event_words_pw.end())
              {
                event_words_pw[word_id] = EventWordWC(weight, j ? 0 : 1);
              }
              else
              {
                EventWordWC& wc = it->second;
                wc.weight += weight;

                if(!j)
                {
                  ++wc.first_count;
                }
              }
            }

            bool first_line = !file.is_open();

            if(first_line)
            {
              El::Guid guid;
              guid.generate();
              
              cache_filename = config_.cache_file() + ".msg." +
                guid.string(El::Guid::GF_DENSE);
//                El::Moment(ACE_OS::gettimeofday()).dense_format();

              file.open(cache_filename.c_str(), ios::out);

              if(!file.is_open())
              {
                std::ostringstream ostr;
                ostr << "NewsGate::Event::EventManager::accept_events("
                     << lang_string() << "): "
                  "failed to open file '" << cache_filename
                     << "' for write access";
                
                throw Exception(ostr.str());
              } 
            }

            write_msg_line(md, file, first_line);
          }
          
          set_messages(event, messages, &event_words_pw);
          
          uint32_t dis_val = 0;
          detach_message_candidate(event, 0, 0, 0, &dis_val);
          
          event.dissenters(dis_val);
          dissenters_ += dis_val;

          if(set_merge(event))
          {
            changed_events_.insert(EventCardinality(event));
          }

//          assert_can_merge(event, "acc", false);
        }
        
        if(!cache_filename.empty())
        {
          if(!file.fail())
          {
            file.flush();
          }
            
          if(file.fail())
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Event::EventManager::accept_events("
                 << lang_string() << "): "
              "failed to write into file '" << cache_filename << "'";
            
            throw Exception(ostr.str());
          }

          file.close();
          
          write_messages_to_db(cache_filename.c_str());
          unlink(cache_filename.c_str());
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
      
      timer.stop();
      ACE_Time_Value tm;
      timer.elapsed_time(tm);
      
      {
        ReadGuard guard(srv_lock_);

        if(log_stream)
        {
          *log_stream << "\n  * accepted: events " << accepted_events
                      << ", messages " << accepted_messages
                      << ", time: " << El::Moment::time(tm)
                      << ", changed events: "
                      << changed_events << "->" << changed_events_.size()
                      << std::endl;
        }
      }
      
      optimize_mem_usage(log_stream);

      return true;
    }

    size_t
    EventManager::event_count() const throw()
    {
      ReadGuard guard(srv_lock_);
      return events_.size();
    }

    void
    EventManager::delete_messages(const Message::IdArray& ids)
      throw(El::Exception)
    {
      std::ostringstream ostr;

      ostr << "NewsGate::Event::EventManager::delete_messages("
           << lang_string() << "): deleting " << ids.size() << " messages";

      size_t count = 0;
      
      {
        WriteGuard guard(srv_lock_);

        for(Message::IdArray::const_iterator it = ids.begin(); it != ids.end();
            ++it)
        {
          const Message::Id& id = *it;
        
          MessageIdToEventNumberMap::const_iterator mit =
            message_events_.find(id);

          if(mit != message_events_.end())
          {          
            EventNumberToEventMap::iterator eit = events_.find(mit->second);
            assert(eit != events_.end());

            EventObject& event = *eit->second;
          
            event.flags |= EventObject::EF_DIRTY;
            
            event.flags &=
              ~(EventObject::EF_REVISED | EventObject::EF_CAN_MERGE);

            MessageInfoArray& messages = event.messages();
            MessageInfoArray::iterator mit = messages.begin();
          
            for(; mit != messages.end() && mit->id != id; mit++);
            assert(mit != messages.end());

            mit->published = 0;

            // To recalc published_max & published_min
            event.calc_hash();

            ostr << "\n  " << id.string();
            ++count;
          }
        }
      }

      ostr << "\n  * " << count << " marked deleted";

      Application::logger()->trace(ostr.str(),
                                   Aspect::EVENT_MANAGEMENT,
                                   El::Logging::HIGH);
    }
      
    bool
    EventManager::delete_messages(const Message::IdArray& ids,
                                  std::ostringstream* log_stream)
      throw(Exception, El::Exception)
    {
      if(ids.empty())
      {
        return true;
      }
      
      {
        StatusGuard guard(status_lock_);

        if(!events_loaded_ || events_unloaded_ /*|| !cleanup_allowed_ ||
                                                 traverse_period_increased_*/)
        {
          return false;
        }
      }
      
      El::Service::CompoundServiceMessage_var msg =
        new DeleteMessages(this, ids);
      
      deliver_now(msg.in());
      
      return true;
    }

    void
    EventManager::write_msg_line(const Event::Transport::MessageDigest& msg,
                                 std::ostream& stream,
                                 bool first_line)
      throw(Exception, El::Exception)
    {
      std::ostringstream data_ostr;
      El::BinaryOutStream bin_str(data_ostr);
      
      msg.core_words.write(bin_str);

      std::string core_words = data_ostr.str();
          
      std::string escaped_core_words =
        El::MySQL::Connection::escape_for_load(core_words.c_str(),
                                               core_words.length());

      if(!first_line)
      {
        stream << std::endl;
      }
      
      stream << msg.id.data << "\t"
             << msg.lang.el_code() << "\t";
      
      stream.write(escaped_core_words.c_str(), escaped_core_words.length());
    }
    
    void
    EventManager::write_messages_to_db(const char* cache_filename)
      throw(Exception, El::Exception)
    {
      DBGuard guard(message_buff_lock_);
      
      El::MySQL::Connection_var connection =
        Application::instance()->dbase()->connect();

      El::MySQL::Result_var result =
        connection->query("delete from EventMessageBuff");
      
      std::string query_load_msg = std::string("LOAD DATA INFILE '") +
        connection->escape(cache_filename) +
        "' REPLACE INTO TABLE EventMessageBuff character set binary";
          
      result = connection->query(query_load_msg.c_str());
          
      result = connection->query(
        "INSERT IGNORE INTO EventMessage SELECT * FROM EventMessageBuff"
        " ON DUPLICATE KEY UPDATE lang=lang_, data=data_");
    }

    void
    EventManager::schedule_merge(time_t delay) throw(El::Exception)
    {
      El::Service::CompoundServiceMessage_var msg = new MergeEvents(this);

      if(delay)
      {
        deliver_at_time(
          msg.in(),
          ACE_OS::gettimeofday() + ACE_Time_Value(delay));
      }
      else
      {
        deliver_now(msg.in());
      }
    }
    
    void
    EventManager::write_event_line(const EventObject& event,
                                   std::ostream& stream,
                                   bool first_line)
      throw(Exception, El::Exception)
    {
      std::ostringstream data_ostr;
      El::BinaryOutStream bin_str(data_ostr);
      
      bin_str << (uint64_t)UINT64_MAX << (uint32_t)1 << event;

      std::string event_data = data_ostr.str();
          
      std::string escaped_event_data =
        El::MySQL::Connection::escape_for_load(event_data.c_str(),
                                               event_data.length());

      if(!first_line)
      {
        stream << std::endl;
      }
      
      stream << event.id.data << "\t" << event.lang.el_code() << "\t";      
      stream.write(escaped_event_data.c_str(), escaped_event_data.length());
    }    
      
    void
    EventManager::write_events_to_db(const char* cache_filename)
      throw(Exception, El::Exception)
    {
      DBGuard guard(event_buff_lock_);

      El::MySQL::Connection_var connection =
        Application::instance()->dbase()->connect();

      El::MySQL::Result_var result =
        connection->query("delete from EventBuff");
      
      std::string query_load_events = std::string("LOAD DATA INFILE '") +
        connection->escape(cache_filename) +
        "' REPLACE INTO TABLE EventBuff character set binary";
          
      result = connection->query(query_load_events.c_str());
          
      result = connection->query(
        "INSERT IGNORE INTO Event SELECT * FROM EventBuff"
        " ON DUPLICATE KEY UPDATE lang=lang_, data=data_");
    }
    
    bool
    EventManager::insert(
      const Event::Transport::MessageDigestArray& message_digests,
      Message::Transport::MessageEventArray& message_events,
      bool save_messages,
      std::ostringstream* log_stream)
      throw(Exception, El::Exception)
    {
      if(message_digests.empty())
      {
        return true;
      }
    
      ACE_High_Res_Timer timer;

      std::fstream file;        
      std::string cache_filename;
      
      size_t inserted = 0;
      size_t found = 0;

      if(!loaded())
      {
        return false;
      }
      
      try
      {
        WriteGuard guard(srv_lock_);
        
        timer.start();

        time_t cur_time = ACE_OS::gettimeofday().sec();
          
        time_t expire_time =
          cur_time > (time_t)config_.message_expiration_time() ?
          cur_time - config_.message_expiration_time() : 0;

        size_t i = 0;
        
        for(Event::Transport::MessageDigestArray::const_iterator
              it = message_digests.begin(); it != message_digests.end();
            it++, i++)
        {
          Message::Transport::MessageEvent& message_event = message_events[i];

          if(message_event.event_id != El::Luid::null)
          {
            continue;
          } 
            
          const Event::Transport::MessageDigest& msg = *it;

          if(langs_.find(msg.lang) == langs_.end())
          {
            continue;
          }
          
          if(insert(msg, message_event, expire_time, log_stream))
          {
            inserted++;
            
            if(save_messages)
            {
              bool first_line = !file.is_open();
              
              if(first_line)
              {
                El::Guid guid;
                guid.generate();
                
                cache_filename = config_.cache_file() + ".msg." +
                  guid.string(El::Guid::GF_DENSE);
                
//                  El::Moment(ACE_OS::gettimeofday()).dense_format();

                file.open(cache_filename.c_str(), ios::out);

                if(!file.is_open())
                {
                  std::ostringstream ostr;
                  ostr << "NewsGate::Event::EventManager::insert("
                       << lang_string() << "): "
                    "failed to open file '" << cache_filename
                       << "' for write access";
                
                  throw Exception(ostr.str());
                } 
              }

              write_msg_line(msg, file, first_line);
            }
          }
          else
          {
            found++;
          }
        }

        if(!cache_filename.empty())
        {
          if(!file.fail())
          {
            file.flush();
          }
            
          if(file.fail())
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Event::EventManager::insert("
                 << lang_string() << "): "
              "failed to write into file '" << cache_filename << "'";
            
            throw Exception(ostr.str());
          }

          file.close();
          
          write_messages_to_db(cache_filename.c_str());
          unlink(cache_filename.c_str());
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
      
      timer.stop();
      ACE_Time_Value tm;
      timer.elapsed_time(tm);
      
      {
        ReadGuard guard(srv_lock_);

        if(log_stream)
        {
          *log_stream << "\n  * from " << message_digests.size()
                      << " inserted: " << inserted << ", found: " << found
                      << ", time: " << El::Moment::time(tm)
                      << ", changed events: "
                      << changed_events_.size() << std::endl;
        }
      }
      
      optimize_mem_usage(log_stream);

      return true;
    }

    EventObject* 
    EventManager::insert(const Event::Transport::MessageDigest& message_digest,
                         Message::Transport::MessageEvent& message_event,
                         uint64_t expire_time,
                         std::ostream* log_stream)
      throw(Exception, El::Exception)
    {
      if(message_digest.published <= expire_time)
      {
        return 0;
      }

      MessageIdToEventNumberMap::const_iterator eit =
        message_events_.find(message_digest.id);
      
      if(eit != message_events_.end())
      {
        const EventObject& event = *events_.find(eit->second)->second;
          
        message_event.event_id = event.id;
        message_event.event_capacity = event.messages().size();

        if(log_stream)
        {
          *log_stream << "\n    found " << message_digest.id.string() << ":"
                      << message_digest.event_id.string() << " <- "
                      << event.id.string();
        }
        
        return 0;
      }
      
      const Message::CoreWords& core_words = message_digest.core_words;

      El::Luid event_id = get_event_id();
      unsigned long event_number = get_event_number();      

      message_event.event_id = event_id;
      message_event.event_capacity = 1;

      EventObject& event =
        *events_.insert(
          std::make_pair(event_number, new EventObject())).first->second;

      event.id = event_id;
      event.lang = message_digest.lang;
      
      event.flags |=
        EventObject::EF_DIRTY | EventObject::EF_MEM_ONLY |
        EventObject::EF_REVISED;
        
      id_to_number_map_[event_id] = event_number;
      message_events_[message_digest.id] = event_number;

      if(log_stream)
      {
        *log_stream << "\n    add " << message_digest.id.string() << ":"
                    << message_digest.event_id.string() << " -> "
                    << event.id.string() << "/" << event_number;
      }

      MessageInfoArray new_messages(1);
      
      new_messages[0] = MessageInfo(message_digest.id,
                                    message_digest.published);
      
      EventWordWeightMap event_words_pw;

      for(size_t i = 0; i < core_words.size(); i++)
      {
        event_words_pw[core_words[i]] =
          EventWordWC(
            EventObject::core_word_weight(i,
                                          core_words.size(),
                                          max_message_core_words_),
            i ? 0 : 1);
      }
      
      set_messages(event, new_messages, &event_words_pw);
      
      if(set_merge(event))
      {
        changed_events_.insert(EventCardinality(event));
      }

      return &event;
    }

    bool
    EventManager::notify(El::Service::Event* event) throw(El::Exception)
    {
      if(El::Service::CompoundService<El::Service::Service,
                                      EventManagerCallback>::notify(event))
      {
        return true;
      }

      TraverseEvents* te = dynamic_cast<TraverseEvents*>(event);
        
      if(te != 0)
      {
        traverse_events();
        return true;
      }
      
      RemakeTraverseEvents* rte = dynamic_cast<RemakeTraverseEvents*>(event);
        
      if(rte != 0)
      {
        remake_traverse_events();
        return true;
      }
      
      if(dynamic_cast<MergeEvents*>(event) != 0)
      {
        merge_events();
        return true;
      }

      DeleteMessages* dm = dynamic_cast<DeleteMessages*>(event);
        
      if(dm != 0)
      {
        delete_messages(dm->ids);
        return true;
      }
      
      LoadEvents* le = dynamic_cast<LoadEvents*>(event);
        
      if(le != 0)
      {
        load_events();
        return true;
      }

      DeleteObsoleteMessages* dom =
        dynamic_cast<DeleteObsoleteMessages*>(event);

      if(dom)
      {
        delete_obsolete_messages();
        return true;
      }
      
      std::ostringstream ostr;
      ostr << "NewsGate::Event::EventManager::notify("
           << lang_string() << "): unknown " << *event;
      
      El::Service::Error error(ostr.str(), this);
      callback_->notify(&error);
      
      return true;
    }

    void
    EventManager::merge_events() throw()
    {
      try
      {
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();
        
        MessageIdToEventInfoMapPtr message_event_updates;
        StringList defered_queries;

        {
          WriteGuard guard(srv_lock_);
          
          if(changed_events_.empty())
          {
            schedule_merge(config_.event_cache().merge_check_period());
          }
          else
          {
            try
            {
              do_merge_events(connection.in(), defered_queries);
            }
            catch(...)
            {
              schedule_merge(std::max(
                               config_.event_cache().event_load_retry_delay(),
                               config_.event_cache().merge_check_period()));
              throw;
            }
            
            schedule_merge(0);
          }

          if(changed_events_.empty() ||
             (message_event_updates_.get() != 0 &&
              message_event_updates_->size() >=
              config_.message_update_chunk_size()))
          {
            ACE_Time_Value tm = ACE_OS::gettimeofday();

            if(tm >= next_message_event_update_time_)
            {
              message_event_updates.reset(message_event_updates_.release());
              
              next_message_event_update_time_ =
                tm + ACE_Time_Value(config_.message_update_chunk_min_period());
            }
          }
        }

        execute_queries(connection, defered_queries, "merge_events");
        
        if(message_event_updates.get())
        {
          post_message_event_updates(message_event_updates.release());
        }
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Event::EventManager::merge_events("
             << lang_string() << "): "
          "El::Exception caught. Description:" << std::endl << e;
        
        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);      
      }
      catch(...)
      {
        std::ostringstream ostr;
        
        ostr << "NewsGate::Event::EventManager::merge_events("
             << lang_string() << "): unexpected exception caught.";
          
        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
      }
    }

    void
    EventManager::execute_queries(El::MySQL::Connection* connection,
                                  const StringList& queries,
                                  const char* caller)
      throw(El::Exception)
    {
      if(queries.empty())
      {
        return;
      }
      
      ACE_High_Res_Timer timer;

      bool trace = Application::will_trace(El::Logging::MIDDLE);
      
      if(trace)
      {
        timer.start();
      }

      size_t query_count = 0;
      size_t exec_query_count = 0;
      
      for(StringList::const_iterator i(queries.begin()),
            e(queries.end()); i != e; ++i, ++query_count)
      {
        try
        {
          El::MySQL::Result_var result = connection->query(i->c_str());
          ++exec_query_count;
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::EventManager::execute_queries: failed for "
               << caller << " as El::Exception caught. Description:"
               << std::endl << e;
          
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);      
        }
      }

      if(trace)
      {
        timer.stop();
        ACE_Time_Value tm;
        timer.elapsed_time(tm);
      
        std::ostringstream ostr;
        ostr << "NewsGate::Event::EventManager::execute_queries: for "
             << caller << " " << exec_query_count << " of " << query_count
             << " queries executed for " << El::Moment::time(tm);
        
        Application::logger()->trace(ostr.str(),
                                     Aspect::EVENT_MANAGEMENT,
                                     El::Logging::MIDDLE);
      }
    }

    void
    EventManager::assert_can_merge(const EventObject& e,
                                   const char* point,
                                   bool print) const throw()
    {
      bool broken =
        e.can_merge(merge_max_strain_,
                    max_time_range_,
                    max_size_) !=
        ((e.flags & EventObject::EF_CAN_MERGE) != 0);
      
      if(print || broken)
      {
        std::cerr << "assert_can_merge(" << point << "): " << e.id.string()
                  << ", " << ((e.flags & EventObject::EF_CAN_MERGE) != 0)
                  << "/"
                  << e.can_merge(merge_max_strain_, max_time_range_, max_size_)
                  << std::endl;
        
        e.dump(std::cerr, true);
        std::cerr << std::endl;        
      }

      if(broken)
      {
        assert(false);
      }
    }
    
    void
    EventManager::do_merge_events(El::MySQL::Connection* connection,
                                  StringList& defered_queries)
      throw(El::Exception)
    {
      ACE_Time_Value find_best_overlap_time;
      ACE_Time_Value merge_events_time;

      ACE_High_Res_Timer timer;
      timer.start();

      bool verbose = false;
      std::ostringstream log_stream;
      
      if(Application::will_trace(El::Logging::MIDDLE))
      {
        log_stream << "EventManager::merge_events("
                   << lang_string() << "):";

        verbose = Application::will_trace(El::Logging::HIGH);
      }

      ACE_Time_Value find_best_overlap_timeout(
        config_.event_cache().find_best_overlap_timeout());
      
      std::ostringstream* high_log_stream =
        Application::will_trace(El::Logging::MIDDLE) ? &log_stream : 0;
      
      {
        // Check DB availability
        El::MySQL::Result_var result = connection->query("show tables");
      }
      
      uint64_t now = ACE_OS::gettimeofday().sec();

      if(now >= merge_blacklist_cleanup_time_)
      {
        for(MergeDenialMap::iterator it(merge_blacklist_.begin()),
              ie(merge_blacklist_.end()); it != ie; ++it)
        {
          const MergeDenialInfo& hpi = it->second;
          
          if(hpi.timeout < now)
          {
            if(hpi.event_id != El::Luid::null)
            {
              EventIdToEventNumberMap::const_iterator i =
                id_to_number_map_.find(hpi.event_id);

              if(i != id_to_number_map_.end())
              {
                const EventObject& e = *events_.find(i->second)->second;

//                assert_can_merge(e, "1", false);
                       
                if((e.flags & EventObject::EF_CAN_MERGE) != 0 &&
                   e.dissenters() == 0)
                {
                  uint32_t hash = e.hash();
                  const HashPair& hp = it->first;
                
                  if(hash == hp.first() || hash == hp.second())
                  {
                    changed_events_.insert(EventCardinality(e));
                  }
                }
              }
            }
          
            merge_blacklist_.erase(it);
          }
        }

        merge_blacklist_cleanup_time_ =
          now + config_.event_cache().merge_blacklist_cleanup_period();
      }
      
      while(!changed_events_.empty() &&
            find_best_overlap_time < find_best_overlap_timeout)
      {
        EventCardinality cardinality = changed_events_.pop();
        
        EventIdToEventNumberMap::iterator iit =
          id_to_number_map_.find(cardinality.id);

        if(iit == id_to_number_map_.end())
        {
          continue;
        }
        
        EventNumber event_number = iit->second;

        EventNumber best_overlap_event = 0;
        EventNumber denied_best_overlap_event = 0;
        uint32_t merge_deny_timeout = 0;
        
        {
          ACE_High_Res_Timer timer;
          timer.start();

          best_overlap_event =
            find_best_overlap(event_number,
                              connection,
                              now,
                              1,
                              denied_best_overlap_event,
                              merge_deny_timeout,
                              high_log_stream);

          timer.stop();
          ACE_Time_Value tm;
          timer.elapsed_time(tm);
          
          find_best_overlap_time += tm;
        }
        
        EventNumberToEventMap::iterator eit1 = events_.find(event_number);
        assert(eit1 != events_.end());
        EventObject& event1 = *eit1->second;
        
        if(best_overlap_event)
        {
          ACE_High_Res_Timer timer;
          timer.start();

          EventNumberToEventMap::iterator eit2 =
            events_.find(best_overlap_event);

          assert(eit2 != events_.end());
            
          EventObject& event2 = *eit2->second;

          if(event1.messages().size() < event2.messages().size())
          {
            merge_events(event1,
                         event2,
                         connection,
                         defered_queries,
                         log_stream,
                         verbose);
          }
          else
          {
            merge_events(event2,
                         event1,
                         connection,
                         defered_queries,
                         log_stream,
                         verbose);
          }

          timer.stop();
          ACE_Time_Value tm;
          timer.elapsed_time(tm);
          
          merge_events_time += tm;
          break;
        }
        else if(denied_best_overlap_event)
        {
          EventNumberToEventMap::iterator eit =
            events_.find(denied_best_overlap_event);
          
          assert(eit != events_.end());
          EventObject& event2 = *eit->second;
          
          HashPair hp(event1.hash(), event2.hash(), event1.lang);
          MergeDenialMap::iterator hit = merge_blacklist_.find(hp);
          
          assert(hit != merge_blacklist_.end());
          
          hit->second.event_id = event1.id;

          if(Application::will_trace(El::Logging::MIDDLE))
          {
            log_stream << "\n  merge denied for " << event1.id.string()
                       << " (0x" << std::hex << event1.hash() << std::dec
                       << "/" << event1.messages().size()
                       << event1.flags_string() << " " << event1.strain()
                       << ") and " << event2.id.string()
                       << " (0x" << std::hex << event2.hash() << std::dec
                       << "/" << event2.messages().size()
                       << event1.flags_string() << " " << event2.strain()
                       << "), wait " << merge_deny_timeout
                       << " sec";
          }
        }
      }

      timer.stop();
      ACE_Time_Value tm;
      timer.elapsed_time(tm);

      if(Application::will_trace(El::Logging::MIDDLE))
      {
        log_stream << "\nmerge time: " << El::Moment::time(tm)
                   << " (" << El::Moment::time(find_best_overlap_time)
                   << " + " << El::Moment::time(merge_events_time)
                   << "), changed events: "
                   << changed_events_.size()
                   << ", merge blacklist size: "
                   << merge_blacklist_.size()
                   << "; tasks in queue "
                   << task_queue_size();
        
        Application::logger()->trace(log_stream.str(),
                                     Aspect::EVENT_MANAGEMENT,
                                     El::Logging::MIDDLE);
      }
    }

    EventNumber
    EventManager::find_best_overlap(
      EventNumber event_number,
      El::MySQL::Connection* connection,
      uint64_t now,
      size_t min_size,
      EventNumber& max_denied_overlap_event_number,
      uint32_t& merge_deny_timeout,
      std::ostringstream* log_stream)
      throw(Exception, El::Exception)
    {
      EventNumberToEventMap::const_iterator eit = events_.find(event_number);
      assert(eit != events_.end());

      return find_best_overlap(*eit->second,
                               event_number,
                               connection,
                               now,
                               min_size,
                               max_denied_overlap_event_number,
                               merge_deny_timeout,
                               log_stream);
    }
      
    EventNumber
    EventManager::find_best_overlap(
      const EventObject& event,
      EventNumber event_number,
      El::MySQL::Connection* connection,
      uint64_t now,
      size_t min_size,
      EventNumber& max_denied_overlap_event_number,
      uint32_t& merge_deny_timeout,
      std::ostringstream* log_stream)
      throw(Exception, El::Exception)
    {
      merge_deny_timeout = 0;
      max_denied_overlap_event_number = 0;
      

//      assert_can_merge(event, "2", false);

/*      
      bool trace = event.id.string() == std::string("DF81BEA603BC82BB");

      if(trace)
      {
        std::cerr << "\n* " << event.id.string() << ":\n";
      }
*/

      if((event.flags & EventObject::EF_CAN_MERGE) == 0)
      {
/*        
        if(trace)
        {
          std::cerr << "  can't\n";
        }
*/      
        return 0;
      }

      EventNumberFastSet skip_events;
      skip_events.insert(event_number);
      
      size_t event_size = event.messages().size();
      uint32_t candidate_event_max_size = max_size_ - event_size;

      size_t event_strain = event.strain();
      
      float max_allowed_rel_overlap = 0;
      uint64_t min_allowed_time_diff = UINT64_MAX;
      
      float max_denied_rel_overlap = 0;
      uint64_t min_denied_time_diff = UINT64_MAX;
      
      EventNumber max_overlap_event_number = 0;

      WordToEventNumberMap::const_iterator wit_end = word_map_.end();
      EventNumberToEventMap::const_iterator events_end = events_.end();
      
      MergeDenialMap::const_iterator merge_blacklist_end =
        merge_blacklist_.end();
      
      const El::Lang& event_lang = event.lang;

      bool load_event = true;
      
      for(EventWordWeightArray::const_iterator i(event.words.begin()),
            e(event.words.end()); i != e; ++i)
      {        
        WordToEventNumberMap::const_iterator wit = word_map_.find(i->word_id);
        
        if(wit == wit_end)
        {
          // Temporary event
          continue;
        }

        const EventNumberSet& numbers = *wit->second;

        for(EventNumberSet::const_iterator i(numbers.begin()),
              e(numbers.end()); i != e; ++i)
        {
//          fbo_meter1.start();
          
          EventNumber number = *i;

//          fbo_meter11.start();
          
          if(skip_events.find(number) != skip_events.end())
          {
//            fbo_meter11.stop();
//            fbo_meter1.stop();
            continue;
          }
          
//          fbo_meter11.stop();
          
//          fbo_meter12.start();
          skip_events.insert(number);
//          fbo_meter12.stop();

//          fbo_meter13.start();
          EventNumberToEventMap::const_iterator eit = events_.find(number);
//          fbo_meter13.stop();
          
          assert(eit != events_end);

          const EventObject& candidate = *eit->second;

          if(candidate.size() < min_size)
          {
            continue;
          }

//          assert_can_merge(candidate, "3", false);

//          fbo_meter14.start();
          
          uint64_t time_diff = event.time_diff(candidate);

          if(time_diff > merge_max_time_diff_)
          {
//            fbo_meter14.stop();
//            fbo_meter1.stop();
/*
            if(trace)
            {
              std::cerr << "  can't with " << candidate.id.string()
                        << " due time_diff " << time_diff
                        << "\n";
            }
*/          
            continue;
          }          

//          fbo_meter14.stop();          
//          fbo_meter15.start();

          if(candidate.lang != event_lang ||
             (candidate.flags & EventObject::EF_CAN_MERGE) == 0)
          {
//            fbo_meter15.stop();
//            fbo_meter1.stop();
/*
            if(trace)
            {
              std::cerr << "  can't with " << candidate.id.string() << "\n";
            }
*/            
            continue;
          }
            
//          fbo_meter15.stop();
//          fbo_meter16.start();

            
          uint64_t time_range = event.time_range(candidate);
            
          if(time_range > max_time_range_)
          {
//            fbo_meter16.stop();
//            fbo_meter1.stop();
/*
            if(trace)
            {
              std::cerr << "  can't with " << candidate.id.string()
                        << " due time_range " << time_range
                        << "\n";
            }
*/            
            continue;
          }

//          fbo_meter16.stop();          
//          fbo_meter17.start();
          
//          size_t candidate_size = candidate.messages().size();
//          if(candidate_size > candidate_event_max_size)
          if(candidate.messages().size() > candidate_event_max_size)
          {
//            fbo_meter17.stop();
//            fbo_meter1.stop();
/*
            if(trace)
            {
              std::cerr << "  can't with " << candidate.id.string()
                        << " due candidate size "
                        << candidate.messages().size()
                        << "\n";
            }
*/            
            continue;
          }

//          fbo_meter17.stop();
//          fbo_meter1.stop();
          
//          fbo_meter2.start();
          
          size_t overlap = event.words_overlap(candidate);
          
          if(overlap < merge_level_min_)
          {
//            fbo_meter2.stop();
/*
            if(trace)
            {
              std::cerr << "  can't with " << candidate.id.string()
                        << " due to small overlap "
                        << overlap
                        << "\n";
            }
*/            
            continue;
          }

//          fbo_meter2.stop();

//          fbo_meter3.start();
          
          size_t merge_level =
            event_merge_level(0, //std::max(event_size, candidate_size),
                              std::max(event_strain, candidate.strain()),
                              time_diff,
                              time_range);
          
          if(overlap < merge_level)
          {
//            fbo_meter3.stop();
/*
            if(trace)
            {
              std::cerr << "  can't with " << candidate.id.string()
                        << " due to not enough overlap "
                        << overlap
                        << "\n";
            }
*/            
            continue;
          }

          float rel_overlap = (float)overlap / merge_level;
          HashPair hp(event.hash(), candidate.hash(), event.lang);
          
          MergeDenialMap::const_iterator hit = merge_blacklist_.find(hp);

          bool merge_allowed = hit == merge_blacklist_end ||
            hit->second.timeout < now;

          float& max_rel_overlap = merge_allowed ?
            max_allowed_rel_overlap : max_denied_rel_overlap;
              
          uint64_t& min_time_diff = merge_allowed ?
            min_allowed_time_diff : min_denied_time_diff;
            
          if(rel_overlap > max_rel_overlap ||
             (max_rel_overlap - rel_overlap > 0.001 &&
              time_diff < min_time_diff))
          {
            // Can check here is event resulted from merge will have too high
            // strain; goto next iteration is it does.

            EventNumberSet event_set;

            if(load_event)
            {
              event_set.insert(event_number);
              load_event = false;
            }

            event_set.insert(number);

            load_message_core_words(event_set, connection);

            size_t candidate_size = candidate.messages().size();

            MessageInfoArray messages(event_size + candidate_size);
            
            messages.copy(event.messages(), 0, event_size);
            messages.copy(candidate.messages(), event_size, candidate_size);
            
            EventObject merged_event;
            create_event(merged_event, messages, event.lang);

            if(merged_event.strain() <= merge_max_strain_ ||
               merged_event.dissenters() < 2 ||
               merged_event.dissenters() < std::min(event_size,
                                                    candidate_size))
            {
              if(rel_overlap > max_rel_overlap)
              {
                max_rel_overlap = rel_overlap;
              }
              
              min_time_diff = time_diff;
            }
            else
            {  
/*              
              if(trace)
              {
                std::cerr << "  can't with " << candidate.id.string()
                          << " due to big strain "
                          << merged_event.strain()
                          << "\n";
              }
*/              
              continue;
            }
          }
          else
          {
/*            
            if(trace)
            {
              std::cerr << "  can't with " << candidate.id.string()
                        << " as other is better "
                        << "\n";
            }
*/            
            continue;
          }

          if(merge_allowed)
          {   
            max_overlap_event_number = number;
/*
            if(trace)
            {
              std::cerr << " allowed with " << candidate.id.string()
                        << "\n";
            }
*/         
          }
          else if(!max_overlap_event_number)
          {
/*            
            if(trace)
            {
              std::cerr << " denied with " << candidate.id.string()
                        << "\n";
            }
*/            
            max_denied_overlap_event_number = number;

            uint32_t timeout = hit->second.timeout > now ?
              (hit->second.timeout - now) : 0;
                
            merge_deny_timeout = timeout;
                  
            if(log_stream)
            {
              *log_stream << "\n  denied 0x" << std::hex << hp.first()
                          << " + 0x" << std::hex << hp.second()
                          << std::dec << " (" << event.messages().size()
                          << " + " << candidate.messages().size()
                          << ") for " << timeout << " sec";
            }
          }

//          fbo_meter3.stop();
        }        
      }

      return max_overlap_event_number;
    }

    uint64_t
    EventManager::merge_deny_timeout(const EventObject& e1,
                                     const EventObject& e2)
      throw(El::Exception)
    {
      uint64_t timeout = (e1.messages().size() + e2.messages().size()) *
        merge_deny_size_factor_;

      timeout = std::min(timeout, merge_deny_max_time_ / 2);
      
      timeout += (unsigned long long)rand() *
        timeout / ((unsigned long long)RAND_MAX + 1);
      
      return timeout;
    }

    void
    EventManager::set_messages(EventObject& event,
                               MessageInfoArray& messages,
                               const EventWordWeightMap* event_words_pw,
                               std::ostream* log_stream,
                               bool verbose)
      throw(El::Exception)
    {
      event.messages(messages);

      if(event_words_pw)
      {
        set_words(event, *event_words_pw);
      }
      else
      {
        EventWordWeightMap event_words_pw;
        
        get_message_word_weights(event,
                                 event_words_pw,
                                 log_stream,
                                 verbose);

        set_words(event, event_words_pw);
      }
      
    }
      
    void
    EventManager::set_words(EventObject& event,
                            const EventWordWeightMap& event_words_pw)
      throw(Exception, El::Exception)
    {
      event.set_words(event_words_pw, max_core_words_, true);

      EventNumber number = id_to_number_map_[event.id];
      const EventWordWeightArray& words = event.words;

      for(size_t i = 0; i < words.size(); i++)
      {
        uint32_t word_id = words[i].word_id;
        
        WordToEventNumberMap::iterator it = word_map_.find(word_id);
        
        if(it == word_map_.end())
        {
          it = word_map_.insert(
            std::make_pair(word_id, new EventNumberSet())).first;
        }
                
        it->second->insert(number);
      }
    }
      
    void
    EventManager::get_message_word_weights(const EventObject& event,
                                           EventWordWeightMap& event_words_pw,
                                           std::ostream* log_stream,
                                           bool verbose)
      const throw(El::Exception)
    {
      MessageCoreWordsMap::const_iterator message_core_words_end =
        message_core_words_.end();
      
      for(MessageInfoArray::const_iterator i(event.messages().begin()),
            e(event.messages().end()); i != e; ++i)
      {
        const MessageInfo& msg = *i;

        MessageCoreWordsMap::const_iterator it =
          message_core_words_.find(msg.id);

        if(it == message_core_words_end)
        {
          if(log_stream)
          {
            *log_stream << "\nNO CORE WORDS found for msg " << msg.id.string()
                        << " belonging to ";
            
            event.dump(*log_stream, verbose);
          }
          
          continue;
        }

        const Message::CoreWords& msg_core_words = it->second->words;
        unsigned long msg_core_words_count = msg_core_words.size();

        for(size_t i = 0; i < msg_core_words_count; i++)
        {
          uint32_t word_id = msg_core_words[i];
          
          uint64_t weight =
            EventObject::core_word_weight(i,
                                          msg_core_words_count,
                                          max_message_core_words_);
          
          EventWordWeightMap::iterator it = event_words_pw.find(word_id);

          if(it == event_words_pw.end())
          {
            event_words_pw[word_id] = EventWordWC(weight, i ? 0 : 1);
          }
          else
          {
            EventWordWC& wc = it->second;
            wc.weight += weight;

            if(!i)
            {
              ++wc.first_count;
            }            
          }
        }
      }
    }
      
    void
    EventManager::unreference_words(const EventObject& event)
      throw(Exception, El::Exception)
    {
      const EventWordWeightArray& words = event.words;
      EventNumber number = id_to_number_map_[event.id];

      for(size_t i = 0; i < words.size(); i++)
      {
        uint32_t word_id = words[i].word_id;
        
        WordToEventNumberMap::iterator it = word_map_.find(word_id);
        assert(it != word_map_.end());
        
        EventNumberSet* numbers = it->second;
        numbers->erase(number);

        if(numbers->empty())
        {
          delete numbers;
          word_map_.erase(word_id);
        }
      }
    }

    size_t
    EventManager::words_overlap(const EventObject& event,
                                Message::Id msg_id,
                                const EventWordWeightMap* ewwm,
                                size_t* common_word_count)
      const throw(El::Exception)
    {
      MessageCoreWordsMap::const_iterator it =
        message_core_words_.find(msg_id);

      if(it == message_core_words_.end())
      {
        return 0;
      }

      const MessageInfoArray& messages = event.messages();

      if(messages.size() == 2)
      {
        size_t i = messages[0].id == msg_id ? 1 : 0;
        assert(messages[i ? 0 : 1].id  == msg_id);

        MessageCoreWordsMap::const_iterator it2 =
          message_core_words_.find(messages[i].id);
        
        if(it2 == message_core_words_.end())
        {
//        std::cerr << "AAAA\n";
          return 0;
        }

        return EventObject::words_overlap(it->second->words,
                                          it2->second->words,
                                          max_message_core_words_,
                                          common_word_count);
      }

      return event.words_overlap(it->second->words,
                                 max_message_core_words_,
                                 ewwm,
                                 common_word_count);
    }
      
    void
    EventManager::merge_events(const EventObject& src,
                               EventObject& dest,
                               El::MySQL::Connection* connection,
                               StringList& defered_queries,
                               std::ostream& log_stream,
                               bool verbose)
      throw(Exception, El::Exception)
    {
      uint64_t src_id = src.id.data;
      uint64_t dest_id = dest.id.data;

      std::string stage = "0";
        
      try
      {
        size_t overlap_common_words = 0;
      
        WORD_OVERLAP_DEBUG_STREAM_DEF

        size_t overlap = src.words_overlap(dest,
                                           &overlap_common_words
                                           WORD_OVERLAP_DEBUG_STREAM_PASS);

        uint64_t time_diff = src.time_diff(dest);
      
//      md_insert_meter.start();
      
        merge_blacklist_[HashPair(src.hash(), dest.hash(), src.lang)] =
          MergeDenialInfo(ACE_OS::gettimeofday().sec() +
                          merge_deny_timeout(src, dest));

//      md_insert_meter.stop();

        EventNumber src_number = id_to_number_map_[src.id];
        EventNumber dest_number = id_to_number_map_[dest.id];

        EventNumberSet merged_events;
        merged_events.insert(src_number);
        merged_events.insert(dest_number);

        stage = "1";
        
        load_message_core_words(merged_events, connection);

        stage = "2";        

        unreference_words(src);
        unreference_words(dest);
      
        for(MessageInfoArray::const_iterator i(src.messages().begin()),
              e(src.messages().end()); i != e; ++i)
        {
          message_events_[i->id] = dest_number;
        }

        uint32_t src_message_count = src.messages().size();
        uint32_t dest_message_count = dest.messages().size();
        uint32_t event_capacity = src_message_count + dest_message_count;      

        MessageInfoArray new_messages(event_capacity);

        new_messages.copy(src.messages(), 0, src_message_count);
      
        new_messages.copy(dest.messages(),
                          src_message_count,
                          dest_message_count);

        set_messages(dest, new_messages, 0, &log_stream, verbose);
        
        uint32_t dis_val = 0;
        detach_message_candidate(dest, connection, 0, 0, &dis_val);

        dissenters_ += dis_val;
        dissenters_ -= src.dissenters() + dest.dissenters();
        
        dest.dissenters(dis_val);
        
        dest.flags |= EventObject::EF_DIRTY;
        dest.flags &= ~EventObject::EF_REVISED;

        stage = "3";
        
        if(message_event_updates_.get() == 0)
        {
          message_event_updates_.reset(new MessageIdToEventInfoMap());
        }

        for(MessageInfoArray::const_iterator i(dest.messages().begin()),
              e(dest.messages().end()); i != e; ++i)
        {
          (*message_event_updates_)[i->id] =
            EventInfo(dest.id, event_capacity);
        }      

        if(Application::will_trace(El::Logging::MIDDLE))
        {
          log_stream << "\nabsorb " << src.id.string() << "/" << src_number
                     << " spin " << src.spin << " overlap " << overlap
                     << "/" << overlap_common_words
                     << " time diff " << time_diff
                     << " " << WORD_OVERLAP_DEBUG_STREAM_VAL
                     << " with #" << dest_number
                     << " ";
        
          dest.dump(log_stream, verbose);
        }
      
        El::Luid src_id = src.id;
        bool delete_src = (src.flags & EventObject::EF_MEM_ONLY) == 0;
      
        id_to_number_map_.erase(src_id);

        EventNumberToEventMap::iterator eit = events_.find(src_number);
        assert(eit != events_.end());

        delete eit->second;
        events_.erase(eit);

        if(set_merge(dest))
        {
          changed_events_.insert(EventCardinality(dest));
        }
      
        if(delete_src)
        {
          std::ostringstream ostr;
          ostr << "delete from Event where event_id=" << src_id.data;
          defered_queries.push_back(ostr.str());
        }
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Event::EventManager::merge_events("
             << src_id << "+" << dest_id << "/" << stage << "): "
          "El::Exception caught. Description:" << std::endl << e;

        throw Exception(ostr.str());
      }
    }

    void
    EventManager::traverse_events() throw()
    {
      try
      {
        ACE_Time_Value current_time = ACE_OS::gettimeofday();
        time_t cur_time = current_time.sec();
          
        uint64_t expire_time =
          cur_time > (time_t)config_.message_expiration_time() ?
          cur_time - config_.message_expiration_time() : 0;

        El::Logging::Logger* trace_logger =
          Application::will_trace(El::Logging::MIDDLE) ?
          Application::logger() : (El::Logging::Logger*)0;

        bool verbose = Application::will_trace(El::Logging::HIGH);
        size_t traverse_period = 0;

        bool end_reached = false;
        ACE_High_Res_Timer timer;
        
        if(trace_logger)
        {
          timer.start();
        }
        
        try
        {
          EventNumberSet load_msg_events;
          EventNumberSet obsolete_events;
          EventNumberSet events_to_revise;
          EventNumberSet flush_events;
          EventNumberSet push_out_events;

          bool to_revise_events =
            next_revision_time_ == ACE_Time_Value::zero;

          bool push_events_out =
            config_.event_cache().push_out_prc_per_hour() > 0 &&
            !CORBA::is_nil(session_->left_neighbour());

          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();          
          
          std::ostringstream revise_log_stream;
          std::ostringstream cleanup_log_stream;
          StringList defered_queries;
          
          size_t changed_events_avg_new = 0;
          size_t changed_events_avg_old = 0;
          
          size_t changed_events_counters_len =
            sizeof(changed_events_counters_) /
            sizeof(changed_events_counters_[0]);

          bool cleanup_allowed = false;
          size_t revision_completed = 0;
          
          {
            size_t push_out_events_count = 0;
            size_t traverse_chunk_size = 0;
              
            WriteGuard guard(srv_lock_);

            changed_events_avg_old = changed_events_sum_ /
              changed_events_counters_len;
            
            changed_events_sum_ -=
              changed_events_counters_[changed_events_counters_len - 1];

            memmove(changed_events_counters_ + 1,
                    changed_events_counters_,
                    (changed_events_counters_len - 1) *
                    sizeof(changed_events_counters_[0]));
            
            *changed_events_counters_ = changed_events_.size();
            changed_events_sum_ += *changed_events_counters_;

            cleanup_allowed = *changed_events_counters_ <=
              config_.event_cache().cleanup_allowed_change_events_count();
            
            changed_events_avg_new = changed_events_sum_ /
              changed_events_counters_len;

            bool maximize_period =
              changed_events_avg_new > changed_events_avg_old &&
              changed_events_avg_old;

            for(size_t *i(changed_events_counters_),
                  *e(changed_events_counters_ + changed_events_counters_len);
                i != e && maximize_period; ++i)
            {
              maximize_period = *i != 0;
            }
              
            if(maximize_period)
            {
              std::string method =
                config_.event_cache().traverse_period_maximization();

              if(method == "inc")
              {
                traverse_period_ =
                  std::min(traverse_period_ + 1,
                           (size_t)config_.event_cache().
                           traverse_period_max());
              }
              else if(method == "dbl")
              {
                traverse_period_ =
                  std::max((size_t)1,
                           std::min(
                             traverse_period_ * 2,
                             (size_t)config_.event_cache().
                             traverse_period_max()));
              }
              else
              {
                traverse_period_ = config_.event_cache().traverse_period_max();
              }
            }
            else
            {
              if(traverse_period_ >
                 config_.event_cache().traverse_period_min())
              {
//                std::cerr << lang_string() << ":" << *changed_events_counters_
//                          << std::endl;
                  
                if(*changed_events_counters_)
                {
                  --traverse_period_;
                }
                else
                {
                  traverse_period_ =
                    config_.event_cache().traverse_period_min();
                }
              }
            }

            traverse_period = traverse_period_;
            
            if(push_events_out)
            {
              size_t i = 0;
              
              for(EventNumberArray::const_iterator it = traverse_event_it_;
                  i < config_.event_cache().traverse_records() &&
                    it != traverse_event_.end(); ++i)
              {
                if(events_.find(*it++) != events_.end())
                {
                  traverse_chunk_size++;
                }
              }

              push_out_events_count =
                std::max(
                  (unsigned long long)1,
                  (unsigned long long)events_.size() *
                  config_.event_cache().push_out_prc_per_hour() *
                  traverse_period_ / (3600 * 100));
            }

            for(size_t i = 0; i < config_.event_cache().traverse_records() &&
                  traverse_event_it_ != traverse_event_.end(); i++)
            {
              EventNumber event_number = *traverse_event_it_++;

              EventNumberToEventMap::const_iterator it =
                events_.find(event_number);

              if(it == events_.end())
              {
                continue;
              }

              const EventObject& event = *it->second;
              
              if(event.flags & EventObject::EF_PUSH_IN_PROGRESS)
              {
                continue;
              }

              if(push_events_out &&
                 (unsigned long long)rand() * traverse_chunk_size /
                 ((unsigned long long)RAND_MAX + 1) < push_out_events_count)
              {
                push_out_events.insert(event_number);
                load_msg_events.insert(event_number);
              }
              else
              {
                if(event.published_min < expire_time)
                {
                  if(cleanup_allowed)
                  {
                    obsolete_events.insert(event_number);
                    load_msg_events.insert(event_number);
                  }
                }
                else if(to_revise_events &&
                        (event.flags & EventObject::EF_REVISED) == 0)
                {
                  events_to_revise.insert(event_number);
                  load_msg_events.insert(event_number);
                }
                
                if(event.flags & EventObject::EF_DIRTY)
                {
                  flush_events.insert(event_number);
                }
              }
            }
            
            load_message_core_words(load_msg_events, connection);
            
            revision_completed =
              revise_events(events_to_revise,
                            cur_time,
                            expire_time,
                            connection,
                            trace_logger ? &revise_log_stream : 0,
                            verbose);
            
            cleanup_obsolete_events(obsolete_events,
                                    expire_time,
                                    true,
                                    connection,
                                    defered_queries,
                                    trace_logger ? &cleanup_log_stream : 0,
                                    verbose);
          }
 
          execute_queries(connection, defered_queries, "traverse_events");
          
          std::ostringstream flush_log_stream;
          
          flush_dirty_events(flush_events,
                             true,
                             trace_logger ? &flush_log_stream : 0);
          
          std::ostringstream push_out_log_stream;

          if(!push_out_events.empty())
          {
            push_events_to_neigbours(push_out_events,
                                     connection,
                                     trace_logger ? &push_out_log_stream : 0);
          }

          {
            MessageIdToEventInfoMapPtr message_event_updates;
            
            {
              WriteGuard guard(srv_lock_);
              
              ACE_Time_Value tm = ACE_OS::gettimeofday();
              
              if(tm >= next_message_event_update_time_)
              {
                message_event_updates.reset(message_event_updates_.release());
                
                next_message_event_update_time_ = tm +
                  ACE_Time_Value(config_.message_update_chunk_min_period());
              }
            }
            
            if(message_event_updates.get())
            {
              post_message_event_updates(message_event_updates.release());
            }
          }
          
/*
          bool traverse_period_increased = false;
          
          {
            StatusGuard guard(status_lock_);
            
            cleanup_allowed_ = cleanup_allowed;
            
            traverse_period_increased_ =
              traverse_period > config_.event_cache().traverse_period_min();

            traverse_period_increased = traverse_period_increased_;
          }
*/

          if(traverse_event_it_ == traverse_event_.end())
          {
            end_reached = true;
            traverse_period = config_.event_cache().traverse_period_max();

            std::ostringstream ostr;
            optimize_mem_usage(&ostr);

            Application::logger()->trace(ostr.str(),
                                         Aspect::EVENT_MANAGEMENT,
                                         El::Logging::MIDDLE);

            if(to_revise_events)
            {              
              size_t revise_events_period =
                config_.event_cache().revise_events_period();
              
              next_revision_time_ = revise_events_period ?
                ACE_Time_Value(ACE_OS::gettimeofday().sec() +
                               revise_events_period) :
                ACE_Time_Value::zero;
            }
            else if(current_time >= next_revision_time_)
            {
              next_revision_time_ = ACE_Time_Value::zero;
            }
            
/*
            std::cerr << El::Moment(ACE_OS::gettimeofday()).iso8601()
                      << " REV " << lang_string() << " "
                      << next_revision_time_.sec() << std::endl;
*/
            
            traverse_event_.clear();

            ReadGuard guard(srv_lock_);

            traverse_event_.reserve(events_.size());
                
            for(EventNumberToEventMap::const_iterator it =
                  events_.begin(); it != events_.end(); it++)
            {
              traverse_event_.push_back(it->first);
            }

            traverse_event_it_ = traverse_event_.begin();
          }

          if(trace_logger)
          {            
            std::string cleanup_log_str = cleanup_log_stream.str();
            std::string revise_log_str = revise_log_stream.str();
            std::string flush_log_str = flush_log_stream.str();
            std::string push_out_log_str = push_out_log_stream.str();

            bool log = false;
            std::ostringstream log_stream;
            
            if(end_reached || !cleanup_log_str.empty() ||
               !revise_log_str.empty() || !flush_log_str.empty() ||
               !push_out_log_str.empty())
            {
              log_stream << "EventManager::traverse_events("
                         << lang_string() << "):";
              
              log = true;
            }

            if(!revise_log_str.empty())
            {
              log_stream << "\n  revising events:" << revise_log_str;
            }

            if(!cleanup_log_str.empty())
            {
              log_stream << "\n  remove events:" << cleanup_log_str;
            }
            
            if(!flush_log_str.empty())
            {
              log_stream << "\n  flush events:" << flush_log_str;
            }

            if(!push_out_log_str.empty())
            {
              log_stream << "\n  pushed out events:" << push_out_log_str;
            }
            
            if(log)
            {
              timer.stop();
              ACE_Time_Value tm;
              timer.elapsed_time(tm);

              log_stream << "\n  changed events avg " << changed_events_avg_new
                         << " <- " << changed_events_avg_old
                         << "\n  traverse period " << traverse_period
                         << "\n  traverse period increased "
                         << (traverse_period >
                             config_.event_cache().traverse_period_min())
                         << "\n  cleanup allowed " << cleanup_allowed
                         << "\n  revision completed " << revision_completed
                         << "\n  * revision time: " << El::Moment::time(tm)
                         << "; end" << (end_reached ? "" : " not")
                         << " reached";

              if(!end_reached)
              {
                log_stream
                  << " " << 100 * (traverse_event_it_ -
                                   traverse_event_.begin()) /
                  traverse_event_.size() << "%";
              }
              
              log_stream << " (" << lang_string() << ")";
              
              trace_logger->trace(log_stream.str(),
                                  Aspect::EVENT_MANAGEMENT,
                                  El::Logging::MIDDLE);
            }
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::EventManager::traverse_events("
               << lang_string() << "): "
            "El::Exception caught. Description:" << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
      
        try
        {
          El::Service::CompoundServiceMessage_var msg =
            new TraverseEvents(this);          

          deliver_at_time(msg.in(),
                          ACE_OS::gettimeofday() +
                            ACE_Time_Value(traverse_period));
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::EventManager::traverse_events("
               << lang_string() << "): "
            "El::Exception caught while scheduling task. Description:"
               << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
      }
      catch(...)
      {
        std::ostringstream ostr;
        
        ostr << "NewsGate::Event::EventManager::traverse_events("
             << lang_string() << "): unexpected exception caught.";
        
        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
      }
    }

    void
    EventManager::remake_traverse_events() throw()
    {
      try
      {
        ACE_Time_Value current_time = ACE_OS::gettimeofday();
        time_t cur_time = current_time.sec();
          
        uint64_t expire_time =
          cur_time > (time_t)config_.message_expiration_time() ?
          cur_time - config_.message_expiration_time() : 0;

        uint64_t recompose_time =
          cur_time > (time_t)config_.event_cache().recompose_timeout() ?
          cur_time - config_.event_cache().recompose_timeout() : 0;
  
        El::Logging::Logger* trace_logger =
          Application::will_trace(El::Logging::MIDDLE) ?
          Application::logger() : (El::Logging::Logger*)0;

//        bool verbose = Application::will_trace(El::Logging::HIGH);

        bool end_reached = false;
        ACE_High_Res_Timer timer;
        
        if(trace_logger)
        {
          timer.start();
        }
        
        try
        {
          EventNumberSet events_to_remake;

          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();          
          
          std::auto_ptr<std::ostringstream> remake_log_stream(
            trace_logger ? new std::ostringstream() : 0);

          size_t traversed_events = 0;

          {
            WriteGuard guard(srv_lock_);

            if(changed_events_.size() <=
               config_.event_cache().cleanup_allowed_change_events_count() &&
               traverse_period_ <= config_.event_cache().traverse_period_min())
            {
              for(; traversed_events <
                    config_.event_remake().traverse_records() &&
                    remake_traverse_event_it_ != remake_traverse_event_.end();
                  ++traversed_events)
              {
                EventNumber event_number = *remake_traverse_event_it_++;

                EventNumberToEventMap::const_iterator it =
                  events_.find(event_number);

                if(it == events_.end())
                {
                  continue;
                }

                const EventObject& event = *it->second;
/*                
                bool selected = false;
                
                for(EventWordWeightArray::const_iterator i(event.words.begin()),
                      e(event.words.end()); i != e; ++i)
                {
                  if(i->word_id == 2198700)
                  {
                    selected = true;
                    break;
                  }
                }

                if(!selected)
                {
                  continue;
                }

                std::cerr << event.id.string();
                */
/*
                if(event.id.data != 0x8DC286B6C91A0514ULL)
                {
                  continue;
//                  std::cerr << "BBBBBB " << lang_string() << "\n";
                }

                std::cerr << "BBBBBB\n";
*/
                if(event.published_max > recompose_time &&
                   (event.flags & EventObject::EF_REVISED) != 0 &&
                   event.published_min > expire_time &&
                   (event.flags & EventObject::EF_PUSH_IN_PROGRESS) == 0)
                {
                  if(remake_event(event_number,
                                  cur_time,
                                  remake_min_size_,
                                  connection,
                                  remake_log_stream.get(),
                                  ""))
                  {
//                    std::cerr << "  remade\n";
                    ++traversed_events;
                    break;
                  }
                }

//                std::cerr << "\n";
              }
            }
          }
          
          if(remake_traverse_event_it_ == remake_traverse_event_.end())
          {
            end_reached = true;            
            remake_traverse_event_.clear();

            ReadGuard guard(srv_lock_);

            remake_traverse_event_.reserve(events_.size());
                
            for(EventNumberToEventMap::const_iterator it =
                  events_.begin(); it != events_.end(); it++)
            {
              remake_traverse_event_.push_back(it->first);
            }

            remake_traverse_event_it_ = remake_traverse_event_.begin();
          }

          if(trace_logger)
          {            
            std::string remake_log_str = remake_log_stream->str();

            bool log = false;
            std::ostringstream log_stream;
            
//            if(end_reached || !remake_log_str.empty())
            {
              log_stream << "EventManager::remake_traverse_events("
                         << lang_string() << "):\n  traversed "
                         << traversed_events;
              
              log = true;
            }

            if(!remake_log_str.empty())
            {
              log_stream << "\n  remaking events:" << remake_log_str;
            }

            if(log)
            {
              timer.stop();
              ACE_Time_Value tm;
              timer.elapsed_time(tm);

              log_stream << "\n  * remake time: " << El::Moment::time(tm)
                         << "; end" << (end_reached ? "" : " not")
                         << " reached";
              
              if(!end_reached)
              {
                log_stream
                  << " " << 100 * (remake_traverse_event_it_ -
                                   remake_traverse_event_.begin()) /
                  remake_traverse_event_.size() << "%";
              }
              
              log_stream << " (" << lang_string() << ")";
              
              trace_logger->trace(log_stream.str(),
                                  Aspect::EVENT_MANAGEMENT,
                                  El::Logging::MIDDLE);
            }
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::EventManager::remake_traverse_events("
               << lang_string() << "): "
            "El::Exception caught. Description:" << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
      
        try
        {
          El::Service::CompoundServiceMessage_var msg =
            new RemakeTraverseEvents(this);          

          deliver_at_time(
            msg.in(),
            ACE_OS::gettimeofday() +
            ACE_Time_Value(end_reached ?
                           config_.event_remake().traverse_pause() :
                           config_.event_remake().traverse_period()));
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::EventManager::remake_traverse_events("
               << lang_string() << "): "
            "El::Exception caught while scheduling task. Description:"
               << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
      }
      catch(...)
      {
        std::ostringstream ostr;
        
        ostr << "NewsGate::Event::EventManager::remake_traverse_events("
             << lang_string() << "): unexpected exception caught.";
        
        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
      }
    }
    
    void
    EventManager::optimize_mem_usage(std::ostringstream* log_stream)
      throw(El::Exception)
    {
      ACE_High_Res_Timer timer;
      ACE_Time_Value optimize_mem_usage_tm;
      
      {
        WriteGuard guard(srv_lock_); 

        timer.start();
        
        for(WordToEventNumberMap::iterator it = word_map_.begin();
            it != word_map_.end(); it++)
        {
          it->second->resize(0);
        }
        
        word_map_.resize(0);
      
        events_.resize(0);
        id_to_number_map_.resize(0);
        message_events_.resize(0);
        changed_events_.optimize_mem_usage();
        merge_blacklist_.resize(0);
        message_core_words_.resize(0);
      }
      
      timer.stop();
      timer.elapsed_time(optimize_mem_usage_tm);

      if(Application::will_trace(El::Logging::MIDDLE) && log_stream)
      {
        ReadGuard guard(srv_lock_);

        *log_stream << "EventManager::optimize_mem_usage("
                    << lang_string() << "): time "
                    << El::Moment::time(optimize_mem_usage_tm) << std::endl
                    << events_.size() << " events, "
                    << message_events_.size()
                    << " messages\nMessages in CoreWords Cache: "
                    << message_core_words_.size() << "\nChanged events: "
                    << changed_events_.size() << "\nMerge blacklist size: "
                    << merge_blacklist_.size() << "\nDissenters: "
                    << dissenters_ << " "
                    << (message_events_.size() ?
                        dissenters_ * 100 / message_events_.size() : 0)
                    << "%\nEvents: " << events_.size() << " "
                    << (message_events_.size() ?
                        events_.size() * 100 / message_events_.size() : 0)
                    << "%";
      }
    }
      
    void
    EventManager::cleanup_obsolete_events(
      const EventNumberSet& obsolete_events,
      time_t expire_time,
      bool delete_messages,
      El::MySQL::Connection* connection,
      StringList& defered_queries,
      std::ostringstream* log_stream,
      bool verbose) throw(El::Exception)
    {
      SStreamPtr remove_events_query;
          
      for(EventNumberSet::const_iterator it = obsolete_events.begin();
          it != obsolete_events.end(); it++)
      {
        EventNumber number = *it;
        SStreamPtr delete_msg_query;
        
        EventNumberToEventMap::iterator eit = events_.find(number);
            
        if(eit != events_.end())
        {
          cleanup_event(*eit->second,
                        number,
                        expire_time,
                        connection,
                        remove_events_query,
                        delete_messages ? &delete_msg_query : 0,
                        log_stream,
                        verbose);
        }

        if(delete_msg_query.get())
        {
          defered_queries.push_back(delete_msg_query->str());
        }
      }

      if(remove_events_query.get() != 0)
      {
        *remove_events_query << " )";
        defered_queries.push_back(remove_events_query->str());        
      }
    }
    
    void
    EventManager::cleanup_event(EventObject& event,
                                EventNumber number,
                                time_t expire_time,
                                El::MySQL::Connection* connection,
                                SStreamPtr& remove_events_query,
                                SStreamPtr* delete_msg_query,
                                std::ostringstream* log_stream,
                                bool verbose)
      throw(El::Exception)
    {
      const MessageInfoArray& messages = event.messages();
      
//      EventObject::assert_msg_consistency(messages);
      
      size_t new_size = 0;
      bool prn_head = true;

      Message::IdSet removed_ids;
      
      for(size_t i = 0; i < messages.size(); i++)
      {
        const MessageInfo& msg = messages[i];
          
        if((time_t)msg.published > expire_time)
        {
          new_size++;
        }
        else
        {
          size_t overlap_common_words = 0;
          
          size_t overlap = words_overlap(event,
                                         msg.id,
                                         0,
                                         &overlap_common_words);
            
          message_events_.erase(msg.id);
          removed_ids.insert(msg.id);

          if(log_stream)
          {
            if(prn_head)
            {
              prn_head = false;
              
              *log_stream << std::endl << "    M-DEL ("
                          << event.id.string() << "/" << number << "):";
            }
              
            *log_stream << " " << msg.id.string() << "/" << msg.published
                        << " overlap " << overlap << "/"
                        << overlap_common_words;
          }

          if(delete_msg_query)
          {
            if(delete_msg_query->get())
            {
              **delete_msg_query << ", ";
            }
            else
            {
              delete_msg_query->reset(new std::ostringstream());
              **delete_msg_query << "delete from EventMessage where id in ( ";
            }
        
            **delete_msg_query << msg.id.data;
          }
          
        }
      }

      if(delete_msg_query && delete_msg_query->get())
      {
        **delete_msg_query << " )";
      }

      if(message_event_updates_.get() == 0)
      {
        message_event_updates_.reset(new MessageIdToEventInfoMap());
      }
            
      for(size_t i = 0; i < messages.size(); i++)
      {
        bool valid = new_size && (time_t)messages[i].published > expire_time;
              
        (*message_event_updates_)[messages[i].id] =
          EventInfo(valid ? event.id : El::Luid::null,
                    valid ? new_size : 0);
      }
            
      unreference_words(event);
      
      dissenters_ -= event.dissenters();
      
      if(new_size)
      {
        MessageInfoArray new_messages(new_size);
        size_t j = 0;

        for(size_t i = 0; i < messages.size(); i++)
        {
          const MessageInfo& mi = messages[i];
              
          if((time_t)mi.published > expire_time)
          {
            new_messages[j++] = mi;
            assert(removed_ids.find(mi.id) == removed_ids.end());            
          }
        }

        size_t old_size = event.messages().size();
        size_t old_words_count = event.words.size();        

        set_messages(event, new_messages, 0, log_stream, verbose);

        uint32_t dis_val = 0;
        detach_message_candidate(event, connection, 0, 0, &dis_val);

        dissenters_ += dis_val;
        event.dissenters(dis_val);
        
        event.flags |= EventObject::EF_DIRTY;
        event.flags &= ~EventObject::EF_REVISED;

        if(set_merge(event))
        {
          changed_events_.insert(EventCardinality(event));
        }

        if(log_stream)
        {
          const MessageInfoArray& messages = event.messages();

          *log_stream << std::endl << "    UPD (" << old_size << "/"
                      << old_words_count << "->"
                      << messages.size() << "/" << event.words.size()
                      << "): " << event.id.string() << "/" << number
                      << event.flags_string() << " " << event.strain();
/*          
          for(size_t i = 0; i < messages.size(); i++)
          {
            const Message::Id id = messages[i].id;
            assert(removed_ids.find(id) == removed_ids.end());
          }
*/
        }        
      }
      else
      {
        if((event.flags & EventObject::EF_MEM_ONLY) == 0)
        {
          if(remove_events_query.get() == 0)
          {
            remove_events_query.reset(new std::ostringstream());
            *remove_events_query << "delete from Event where event_id in ( ";
          }
          else
          {
            *remove_events_query << ", ";
          }
            
          *remove_events_query << event.id.data;
        }
 
        if(log_stream)
        {
          *log_stream << std::endl << "    REM: "
                      << event.id.string() << "/" << number;
        }

        id_to_number_map_.erase(event.id);

        EventNumberToEventMap::iterator eit = events_.find(number);
        assert(eit != events_.end());
        
        delete eit->second;
        events_.erase(eit);
      }

      for(Message::IdSet::const_iterator i(removed_ids.begin()),
            e(removed_ids.end()); i != e; ++i)
      {
        erase_message_core_words(*i);
      }
    }
    
    void
    EventManager::post_message_event_updates(
      MessageIdToEventInfoMap* message_event_updates)
      throw(Exception, El::Exception)
    {
      MessageIdToEventInfoMapPtr holder(message_event_updates);
        
      size_t updates_count = message_event_updates->size();

      std::auto_ptr<Message::Transport::MessageEventArray>
        message_event_updates_array(
          new Message::Transport::MessageEventArray);

      message_event_updates_array->reserve(updates_count);

      for(MessageIdToEventInfoMap::const_iterator
            it = message_event_updates->begin();
          it != message_event_updates->end(); it++)
      {
        message_event_updates_array->push_back(
          Message::Transport::MessageEvent(it->first,
                                           it->second.id,
                                           it->second.capacity));
      }
      
      Message::Transport::MessagePack_var message_events =
        new Message::Transport::MessageEventPackImpl::Type(
          message_event_updates_array.release());

      try
      {
        ACE_High_Res_Timer timer;
        timer.start();

        if(bank_client_session_.in())
        {
          bank_client_session_->post_messages(
            message_events.in(),
            Message::PMR_EVENT_UPDATE,
            Message::BankClientSession::PS_DISTRIBUTE_BY_MSG_ID,
            "");
        }

        timer.stop();
        ACE_Time_Value tm;
        timer.elapsed_time(tm);
        
        if(Application::will_trace(El::Logging::MIDDLE))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::EventManager::post_message_event_updates("
               << lang_string() << "): "
               << updates_count << " updates; time: " << El::Moment::time(tm);
            
          Application::logger()->trace(ostr.str(),
                                       Aspect::EVENT_MANAGEMENT,
                                       El::Logging::MIDDLE);
        }        
      }
      catch(const Message::BankClientSession::FailedToPostMessages& e)
      {
        if(Application::will_trace(El::Logging::MIDDLE))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::EventManager::post_message_event_updates("
               << lang_string() << "): "
            "FailedToPostMessages caught. Description:\n"
               << e.description.in();
            
          Application::logger()->trace(ostr.str(),
                                       Aspect::EVENT_MANAGEMENT,
                                       El::Logging::MIDDLE);
        }        
      }
      catch(const Message::NotReady& e)
      {
        if(Application::will_trace(El::Logging::MIDDLE))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::EventManager::post_message_event_updates: "
            "NotReady caught. Description:\n"
               << e.reason.in();
            
          Application::logger()->trace(ostr.str(),
                                       Aspect::EVENT_MANAGEMENT,
                                       El::Logging::MIDDLE);
        }        
      }
      catch(const Message::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Event::EventManager::post_message_event_updates("
             << lang_string() << "): "
          "ImplementationException caught. Description:\n"
             << e.description.in();

        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Event::EventManager::post_message_event_updates("
             << lang_string() << "): "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    }

    void
    EventManager::load_message_core_words(EventNumber event_number,
                                          El::MySQL::Connection* connection)
      throw(El::Exception)
    {
      EventNumberSet events;
      events.insert(event_number);

      load_message_core_words(events, connection);
    }

    void
    EventManager::erase_message_core_words(const Message::Id& id)
      throw(El::Exception)
    {
      MessageCoreWordsMap::iterator i = message_core_words_.find(id);

      if(i != message_core_words_.end())
      {
        delete i->second;
        message_core_words_.erase(i);
      }
    }
    
    void
    EventManager::load_message_core_words(const EventNumberSet& events,
                                          El::MySQL::Connection* connection)
      throw(El::Exception)
    {
      if(events.empty())
      {
        return;
      }

      MessageCoreWordsMap::iterator core_words_end = message_core_words_.end();
      EventNumberToEventMap::const_iterator events_end = events_.end();
      
      Message::IdArray ids;

      time_t current_time = ACE_OS::gettimeofday().sec();
      
      for(EventNumberSet::const_iterator it(events.begin()),
            it_end(events.end()); it != it_end; ++it)
      {
        EventNumberToEventMap::const_iterator eit = events_.find(*it);
            
        if(eit != events_end)
        {
          const MessageInfoArray& messages = eit->second->messages();
            
          for(MessageInfoArray::const_iterator i(messages.begin()),
                e(messages.end()); i < e; ++i)
          {
            const Message::Id& id = i->id;
            MessageCoreWordsMap::iterator mit = message_core_words_.find(id);
            
            if(mit == core_words_end)
            {
              ids.push_back(id);
            }
            else
            {
              mit->second->timestamp = current_time;
            }
          }
        }
      }

      if(current_time >= msg_core_words_next_preemt_)
      {
        time_t threshold = current_time -
          config_.event_cache().msg_core_words_timeout();
      
        for(MessageCoreWordsMap::iterator i(message_core_words_.begin()),
              e(message_core_words_.end()); i != e; ++i)
        {
          if((time_t)i->second->timestamp <= threshold)
          {
            delete i->second;
            message_core_words_.erase(i);
          }
        }

        msg_core_words_next_preemt_ =
          current_time + config_.event_cache().msg_core_words_preemt_period();
      }

      SStreamPtr ostr;
      size_t i = 0;
      size_t load_message_count = config_.event_cache().load_messages();

      for(Message::IdArray::const_iterator it(ids.begin()), it_end(ids.end());
          it != it_end; ++it, ++i)
      {
        if(i == load_message_count)
        {
          load_message_core_words(ostr, current_time, connection);
          i = 0;
        }

        if(ostr.get())
        {
          *ostr << ", ";
        }
        else
        {
          ostr.reset(new std::ostringstream());
          *ostr << "select id, data from EventMessage where id in ( ";
        }
        
        *ostr << it->data;
      }

      if(i)
      {
        load_message_core_words(ostr, current_time, connection);
      }
    }
    
    void
    EventManager::load_message_core_words(SStreamPtr& ostr,
                                          time_t current_time,
                                          El::MySQL::Connection* connection)
      throw(El::Exception)
    {      
      *ostr << " )";

    
      El::MySQL::Result_var result =
        connection->query(ostr->str().c_str());
      
      EventMessageRecord record(result.in());

      while(record.fetch_row())
      {
        Message::Id id;
        id.data = record.id();
        
        std::string data = record.data();

        std::istringstream istr(data);
        El::BinaryInStream bin_istr(istr);

        MessageCoreWordsMap::iterator it = message_core_words_.find(id);

        if(it == message_core_words_.end())
        {
          it = message_core_words_.insert(
            std::make_pair(id, new MessageCoreWords())).first;
        }
                            
        MessageCoreWords& cw = *it->second;

        try
        {
          cw.words.read(bin_istr);
        }
        catch(const El::Exception& e)
        {
          std::fstream file;        
          std::string cache_filename;
          
          El::Guid guid;
          guid.generate();
          
          cache_filename = config_.cache_file() + ".err." +
            id.string() + "." + guid.string(El::Guid::GF_DENSE);

          file.open(cache_filename.c_str(), ios::out);

          if(file.is_open())
          {
            file.write(data.c_str(), data.length());
          }
            
          std::ostringstream ostr;
          ostr << "NewsGate::Event::EventManager::load_message_core_words: "
            "for msg " << id.data
               << " El::Exception caught. Description:" << std::endl << e;

          throw Exception(ostr.str());
        }
        
        cw.timestamp = current_time;
      }

      ostr.reset(0);
    }
    
    void
    EventManager::push_events_to_neigbours(
      const EventNumberSet& events_to_push,
      El::MySQL::Connection* connection,
      std::ostringstream* log_stream)
      throw(El::Exception)
    {
      size_t pushed_events = 0;
      size_t size = events_to_push.size() / 2 + 1;
      
      Transport::EventPushInfoPackImpl::Var event_push_left_pack =
        new Transport::EventPushInfoPackImpl::Type(
          new Transport::EventPushInfoArray());

      Transport::EventPushInfoPackImpl::Var event_push_right_pack =
        new Transport::EventPushInfoPackImpl::Type(
          new Transport::EventPushInfoArray());

      Transport::EventPushInfoArray& event_push_left =
        event_push_left_pack->entities();

      Transport::EventPushInfoArray& event_push_right =
        event_push_right_pack->entities();

      event_push_left.reserve(size);
      event_push_right.reserve(size);

      Transport::MessageDigestPackImpl::Var message_digest_left_pack =
        new Transport::MessageDigestPackImpl::Type(
          new Transport::MessageDigestArray());

      Transport::MessageDigestPackImpl::Var message_digest_right_pack =
        new Transport::MessageDigestPackImpl::Type(
          new Transport::MessageDigestArray());

      Transport::MessageDigestArray& message_digest_left =
        message_digest_left_pack->entities();

      Transport::MessageDigestArray& message_digest_right =
        message_digest_right_pack->entities();

      message_digest_left.reserve(size);
      message_digest_right.reserve(size);

      {
        WriteGuard guard(srv_lock_);

        for(EventNumberSet::const_iterator it = events_to_push.begin();
            it != events_to_push.end(); it++)
        {
          EventNumberToEventMap::iterator eit = events_.find(*it);
          
          if(eit != events_.end())
          {
            EventObject& event = *eit->second;
            
            event.flags |= EventObject::EF_PUSH_IN_PROGRESS;
            event.flags &= ~EventObject::EF_CAN_MERGE;

            int bank_count = session_->bank_count();

            int r = (unsigned long long)rand() * 2 /
              ((unsigned long long)RAND_MAX + 1);
            
            long spin = event.spin ? event.spin :
              (r ? bank_count : -bank_count);

            if(spin < 0)
            {
              event_push_left.push_back(
                Transport::EventPushInfo(event.id,
                                         spin + 1,
                                         event.persistent_flags(),
                                         event.dissenters()));
            }
            else
            {
              event_push_right.push_back(
                Transport::EventPushInfo(event.id,
                                         spin - 1,
                                         event.persistent_flags(),
                                         event.dissenters()));
            }

            const MessageInfoArray& messages = event.messages();

            for(unsigned long i = 0; i < messages.size(); i++)
            {
              const MessageInfo& mi = messages[i];

              if(spin < 0)
              {
                message_digest_left.push_back(Transport::MessageDigest());
              }
              else
              {
                message_digest_right.push_back(Transport::MessageDigest());
              }

              Transport::MessageDigest& md = spin < 0 ?
                *message_digest_left.rbegin() : *message_digest_right.rbegin();

              md.id = mi.id;
              md.published = mi.published;
              md.lang = event.lang;
              md.event_id = event.id;

              MessageCoreWordsMap::const_iterator it =
                message_core_words_.find(mi.id);

              if(it != message_core_words_.end())
              {
                md.core_words = it->second->words;
              }
            }
            
            if(log_stream)
            {
              *log_stream << std::endl << "    " << event.id.string()
                          << "/" << *it;
            }

            pushed_events++;
          }
        }
      }

      std::string push_left_error;
      
      if(!event_push_left.empty())
      {
        push_events(message_digest_left_pack.in(),
                    event_push_left_pack.in(),
                    session_->left_neighbour(),
                    connection,
                    push_left_error);
      }
      
      std::string push_right_error;
      
      if(!event_push_right.empty())
      {
        push_events(message_digest_right_pack.in(),
                    event_push_right_pack.in(),
                    session_->right_neighbour(),
                    connection,
                    push_right_error);
      }
      
      if(log_stream && pushed_events)
      {
        *log_stream << std::endl << "    pushing " << pushed_events
                    << " events";

        if(!push_left_error.empty())
        {
          *log_stream << std::endl << "    left push failed; description\n"
                      << push_left_error;
        }
        
        if(!push_right_error.empty())
        {
          *log_stream << std::endl << "    right push failed; description\n"
                      << push_right_error;
        }
        
      }
    }

    bool
    EventManager::push_events(
      Transport::MessageDigestPackImpl::Type* message_digest,
      Transport::EventPushInfoPackImpl::Type* event_pack,
      Event::Bank_ptr bank,
      El::MySQL::Connection* connection,
      std::string& error_desc)
      throw(Exception, El::Exception)
    {
      // Important to copy as package embedded array is destructed as
      // a part of serialization procedure
      
      const Event::Transport::EventPushInfoArray events =
        event_pack->entities();

      try
      {
        bank->push_events(message_digest, event_pack);

        //
        // TODO: Need to invent some timeout during which events can not be
        // pushed out being inserted into bank. This will decrease a
        // possibility that it will not be pushed back immediatelly
        // right before deletion below. Also this approach will
        // give some additional time for merge in current bank.
        //

        StringList defered_queries;
        SStreamPtr remove_events_query;

        WriteGuard guard(srv_lock_);

        for(Event::Transport::EventPushInfoArray::const_iterator
              it = events.begin(); it != events.end(); it++)
        {
          EventIdToEventNumberMap::iterator
            iit = id_to_number_map_.find(it->id);
          
          assert(iit != id_to_number_map_.end());

          unsigned long event_number = iit->second;

          EventNumberToEventMap::iterator eit = events_.find(event_number);
          assert(eit != events_.end());
          
          EventObject* event = eit->second;

          std::ostringstream delete_msg_query;
          const MessageInfoArray& messages = event->messages();

          for(unsigned long i = 0; i < messages.size(); i++)
          {
            const MessageInfo& msg = messages[i];
          
            message_events_.erase(msg.id);
            erase_message_core_words(msg.id);

            if(i)
            {
              delete_msg_query << ", ";
            }
            else
            {
              delete_msg_query << "delete from EventMessage where id in ( ";
            }
        
            delete_msg_query << msg.id.data;
          }

          delete_msg_query << " )";
          
          unreference_words(*event);

          if((event->flags & EventObject::EF_MEM_ONLY) == 0)
          {
            if(remove_events_query.get() == 0)
            {
              remove_events_query.reset(new std::ostringstream());
              *remove_events_query << "delete from Event where event_id in ( ";
            }
            else
            {
              *remove_events_query << ", ";
            }
            
            *remove_events_query << event->id.data;
          }

          dissenters_ -= event->dissenters();

          id_to_number_map_.erase(event->id);

          delete event;
          events_.erase(eit);

          defered_queries.push_back(delete_msg_query.str());          
        }

// Need to remove pushed out data under the lock
// otherwise it is possible that will remove it after
// event have been pushed back        
//        guard.release();
        
        if(remove_events_query.get() != 0)
        {
          *remove_events_query << " )";
          defered_queries.push_back(remove_events_query->str());          
        }

        execute_queries(connection, defered_queries, "push_events");
        return true;
      }
      catch(const NewsGate::Event::NotReady& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Event::EventManager::push_events("
             << lang_string() << "): "
          "NewsGate::Event::NotReady caught. Reason:\n" << e.reason;

        error_desc = ostr.str();        
      }
      catch(const NewsGate::Event::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Event::EventManager::push_events("
             << lang_string() << "): "
          "NewsGate::Event::ImplementationException caught. Description:\n"
             << e.description;

        error_desc = ostr.str();        
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Event::EventManager::push_events("
             << lang_string() << "): "
          "CORBA::Exception caught. Description:\n" << e;

        error_desc = ostr.str();        
      }

      WriteGuard guard(srv_lock_);

      for(Event::Transport::EventPushInfoArray::const_iterator
            it = events.begin(); it != events.end(); it++)
      {
        EventIdToEventNumberMap::iterator
          iit = id_to_number_map_.find(it->id);
        
        assert(iit != id_to_number_map_.end());
        
        unsigned long event_number = iit->second;
        
        EventNumberToEventMap::iterator eit = events_.find(event_number);
        assert(eit != events_.end());

        EventObject& event = *eit->second;
        
        event.flags &= ~EventObject::EF_PUSH_IN_PROGRESS;

        if(set_merge(event))
        {
          changed_events_.insert(EventCardinality(event));
        }
      }

      return false;
    }
    
    void
    EventManager::flush_dirty_events(const EventNumberSet& events_to_flush,
                                     bool lock,
                                     std::ostringstream* log_stream)
      throw(El::Exception)
    {
      std::string cache_filename;
      std::fstream file;

      try
      {
        std::auto_ptr<WriteGuard> guard(
          lock ? new WriteGuard(srv_lock_) : 0);
      
        for(EventNumberSet::const_iterator it = events_to_flush.begin();
            it != events_to_flush.end(); it++)
        {
          EventNumberToEventMap::iterator eit = events_.find(*it);
            
          if(eit == events_.end())
          {
            continue;
          }

          EventObject& event = *eit->second;

          bool first_line = cache_filename.empty();
          
          if(first_line)
          {
            El::Guid guid;
            guid.generate();
                
            cache_filename = config_.cache_file() + ".drt." +
              guid.string(El::Guid::GF_DENSE);
            
            file.open(cache_filename.c_str(), ios::out);

            if(!file.is_open())
            {
              std::ostringstream ostr;
              ostr << "NewsGate::Event::EventManager::flush_events("
                   << lang_string() << "): "
                "failed to open file '" << cache_filename
                   << "' for write access";

              throw Exception(ostr.str());
            }
          }

          write_event_line(event, file, first_line);
        }

        if(!file.fail())
        {
          file.flush();
        }
            
        if(file.fail())
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::EventManager::flush_events("
               << lang_string() << "): "
            "failed to write into file '" << cache_filename << "'";

          throw Exception(ostr.str());
        }
      
        if(!cache_filename.empty())
        {
          file.close();
          write_events_to_db(cache_filename.c_str());
          unlink(cache_filename.c_str());

          for(EventNumberSet::const_iterator it = events_to_flush.begin();
              it != events_to_flush.end(); it++)
          {
            EventNumberToEventMap::iterator eit = events_.find(*it);
            
            if(eit != events_.end())
            {
              EventObject& event = *eit->second;
              
              event.flags &= ~(EventObject::EF_MEM_ONLY |
                               EventObject::EF_DIRTY);

              if(log_stream)
              {
                *log_stream << std::endl << "    " << event.id.string()
                            << "/" << *it << " " << event.flags_string()
                            << " " << event.strain();
              }
            }
          }
          
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

    size_t
    EventManager::revise_events(const EventNumberSet& events_to_revise,
                                time_t now,
                                time_t expire_time,
                                El::MySQL::Connection* connection,
                                std::ostringstream* log_stream,
                                bool verbose)
      throw(El::Exception)
    {
      EventNumberSet changed_events;
      EventNumberSet revised_events;
      Transport::MessageDigestArray detached_message_digests;      
      std::ostringstream split_log_stream;
      std::ostringstream separate_log_stream;
      std::ostringstream remake_log_stream;

      size_t revision_completed = 0;

        uint64_t recompose_time =
          now > (time_t)config_.event_cache().recompose_timeout() ?
          now - config_.event_cache().recompose_timeout() : 0;      
      
      for(EventNumberSet::const_iterator it = events_to_revise.begin();
          it != events_to_revise.end(); it++)
      {
        EventNumber event_number = *it;

        EventNumberToEventMap::const_iterator eit = events_.find(event_number);
            
        if(eit == events_.end())
        {
          continue;
        }
        
        const EventObject& event = *eit->second;

        bool can_recompose = event.published_max > recompose_time;
        
/*
        EventNumberToEventMap::iterator eit = events_.find(event_number);
            
        if(eit != events_.end())
        {
          EventObject& event = *eit->second;

          if(event.id.data != 0x60F4E1D900B6A901ULL)
          {
            continue;
          }
        }
*/
        if(!can_recompose ||
           (!split_event(event_number,
                         expire_time,
                         changed_events,
                         log_stream ? &split_log_stream : 0) &&
            !separate_event(event_number,
                            expire_time,
                            changed_events,
                            log_stream ? &separate_log_stream : 0)))
        {
          if(revise_event(event_number,
                          expire_time,
                          revised_events,
                          detached_message_digests))
          {
            if(can_recompose)
            {
              remake_event(event_number,
                           now,
                           remake_min_size_revise_,
                           connection,
                           log_stream ? &remake_log_stream : 0,
                           "  ");
            }

            ++revision_completed;
          }
        }
      }

      if(log_stream)
      {
        std::string log_str = split_log_stream.str();
          
        if(!log_str.empty())
        {
          *log_stream << "\n    * splitted events:" << log_str;
        }

        log_str = separate_log_stream.str();
          
        if(!log_str.empty())
        {
          *log_stream << "\n    * separated events:" << log_str;
        }        

        log_str = remake_log_stream.str();
          
        if(!log_str.empty())
        {
          *log_stream << "\n    * remade events:" << log_str;
        }        
      }      

      if(!changed_events.empty())
      {
        std::ostringstream flush_log_stream;
        
        flush_dirty_events(changed_events,
                           false,
                           log_stream ? &flush_log_stream : 0);

        if(log_stream)
        {
          std::string flush_log_str = flush_log_stream.str();
          
          if(!flush_log_str.empty())
          {
            *log_stream << "\n    * flushing splitted & separated events:"
                        << flush_log_str;
          }
        }        
      }

      if(!revised_events.empty())
      {
        StringList defered_queries;
        std::ostringstream detach_log_stream;
        std::ostringstream flush_log_stream;
        
        cleanup_obsolete_events(revised_events,
                                0,
                                false,
                                connection,
                                defered_queries,
                                log_stream ? &detach_log_stream : 0,
                                verbose);

        flush_dirty_events(revised_events,
                           false,
                           log_stream ? &flush_log_stream : 0);

        if(!defered_queries.empty())
        {
          execute_queries(connection, defered_queries, "revise_events");
        }

        if(log_stream)
        {
          std::string detach_log_str = detach_log_stream.str();
          
          if(!detach_log_str.empty())
          {
            *log_stream << "\n    * detaching messages:" << detach_log_str;
          }

          std::string flush_log_str = flush_log_stream.str();
          
          if(!flush_log_str.empty())
          {
            *log_stream << "\n    * flushing revised events:" << flush_log_str;
          }
        }
      }

      if(!detached_message_digests.empty())
      {
        std::ostringstream insert_log_stream;
        
        Message::Transport::MessageEventArray message_events;
        message_events.resize(detached_message_digests.size());

        size_t i = 0;
        
        for(Event::Transport::MessageDigestArray::const_iterator
              it = detached_message_digests.begin();
            it != detached_message_digests.end(); it++, i++)
        {
          message_events[i].id = it->id;
        }        
      
        insert(detached_message_digests,
               message_events,
               false,
               log_stream ? &insert_log_stream : 0);
      
        if(message_event_updates_.get() == 0)
        {
          message_event_updates_.reset(new MessageIdToEventInfoMap());
        }
          
        for(unsigned long i = 0; i < detached_message_digests.size(); i++)
        {
          const Message::Transport::MessageEvent& me = message_events[i];
            
          (*message_event_updates_)[me.id] =
            EventInfo(me.event_id, me.event_capacity);
        }
      
        std::string insert_log_str = insert_log_stream.str();

        if(log_stream && !insert_log_str.empty())
        {
          *log_stream << "\n    * inserting events:" << insert_log_str;
        }
      }

      return revision_completed;
    }

    bool
    EventManager::detach_message_candidate(const EventObject& event,
                                           El::MySQL::Connection* connection,
                                           time_t expire_time,
                                           size_t* detach_message_index,
                                           uint32_t* dissenters)
      throw(El::Exception)
    {
      if(detach_message_index)
      {
        *detach_message_index = 0;
      }

      if(dissenters)
      {
        *dissenters = 0;
      }

      const MessageInfoArray& messages = event.messages();
      size_t event_size = messages.size();

      if(event_size <= 1)
      {
        return false;
      }
      
      EventWordWeightMap event_words_pw;
      event.get_word_weights(event_words_pw);

      size_t min_overlap = SIZE_MAX;
      time_t candidate_message_time = 0;
      
      size_t merge_level =
        event_merge_level(event_size, 0, 0, event.time_range());

      for(size_t i = 0; i < event_size; ++i)
      {          
        time_t message_published = messages[i].published;

        if(message_published <= expire_time)
        {
          if(dissenters)
          {
            ++(*dissenters);
          }
          
          continue;
        }

        if(connection)
        {
          EventIdToEventNumberMap::const_iterator it =
            id_to_number_map_.find(event.id);

          assert(it != id_to_number_map_.end());

          load_message_core_words(it->second, connection);
        }
        
        const Message::Id& message_id = messages[i].id;

        size_t overlap = words_overlap(event, message_id, &event_words_pw);

        if(overlap < merge_level)
        {
          if(dissenters)
          {
            ++(*dissenters);
          }

          if(overlap < min_overlap ||
             (overlap == min_overlap &&
              message_published < candidate_message_time))
          {
            if(detach_message_index)
            {
              *detach_message_index = i;
            }
            else if(dissenters == 0)
            {
              return true;
            }

            min_overlap = overlap;
            candidate_message_time = message_published;            
          }
        }
      }

      return min_overlap < SIZE_MAX;
    }

    bool
    EventManager::split_event(EventNumber event_number,
                              time_t expire_time,
                              EventNumberSet& changed_events,
                              std::ostringstream* log_stream)
      throw(El::Exception)
    {
      EventNumberToEventMap::iterator eit = events_.find(event_number);
            
      if(eit == events_.end())
      {
        return false;
      }

      EventObject& event = *eit->second;
      const MessageInfoArray& messages = event.messages();
      size_t event_size = messages.size();

      if(event_size < 4)
      {
        return false;
      }

      MessageInfoVector sorted_messages;

      for(MessageInfoArray::const_iterator i(messages.begin()),
            e(messages.end()); i != e; ++i)
      {
        uint64_t published = i->published;
        
        MessageCoreWordsMap::const_iterator it =
          message_core_words_.find(i->id);

        if(it == message_core_words_.end() ||
           published <= (uint64_t)expire_time)
        {
          return false;
        }
        
        MessageInfoVector::iterator j(sorted_messages.begin()),
          je(sorted_messages.end());
        
        for(; j != je && j->published < published; ++j);
        sorted_messages.insert(j, *i);
      }

      float min_rel_overlap = 101;
      uint64_t max_time_diff = 0;
      size_t split_overlap = 0;
      size_t split_merge_level = 0;
      
      MessageInfoVector::const_iterator rift(sorted_messages.end());
      MessageInfoVector::const_iterator prev(sorted_messages.begin() + 1);
        
      for(MessageInfoVector::const_iterator i(prev + 1),
            e(sorted_messages.end()); i + 1 != e; prev = i++)
      {
        uint64_t time_diff = i->published - prev->published;
        
        if(time_diff >= min_rift_time_)
        {          
          EventObject event1;
          create_event(event1, sorted_messages.begin(), i, event.lang);
          
          EventObject event2;
          create_event(event2, i, sorted_messages.end(), event.lang);
          
          size_t overlap = time_diff > merge_max_time_diff_ ? 0 :
            event1.words_overlap(event2);

          if(event.dissenters() <= event1.dissenters() + event2.dissenters())
          {
            continue;
          }
          
          float rel_overlap = 0;
          size_t merge_level = 0;
          
          if(overlap)
          {
            merge_level =
              event_merge_level(0,
                                std::max(event1.strain(),
                                         event2.strain()),
                                time_diff,
                                event1.time_range(event2));

            if(overlap >= merge_level)
            {
              continue;
            }

            rel_overlap = (float)overlap / merge_level;
          }
          
          if(rel_overlap < min_rel_overlap ||
             (rel_overlap - min_rel_overlap > 0.001 &&
              time_diff > max_time_diff))
          {
            rift = i;

            if(rel_overlap < min_rel_overlap)
            {
              min_rel_overlap = rel_overlap;
            }
                  
            max_time_diff = time_diff;

            split_overlap = overlap;
            split_merge_level = merge_level;
          }
        }
      }

      if(rift == sorted_messages.end())
      {
        return false;
      }

      if(log_stream)
      {
        *log_stream << "\n    " << event.id.string() << " ("
                    << event.messages().size() << "/" << event.dissenters()
                    << "/" << event.strain() << "): ";
      }

      size_t part1_size = rift - sorted_messages.begin();
      size_t part2_size = sorted_messages.size() - part1_size;
      bool part1_smaller = part1_size < part2_size;

      MessageInfoVector::const_iterator new_event_msg_begin(
        part1_smaller ? sorted_messages.begin() : rift);
      
      MessageInfoVector::const_iterator new_event_msg_end(
        part1_smaller ? rift : sorted_messages.end());
      
      MessageInfoVector::const_iterator old_event_msg_begin(
        part1_smaller ? rift : sorted_messages.begin());
      
      MessageInfoVector::const_iterator old_event_msg_end(
        part1_smaller ? sorted_messages.end() : rift);

      EventObject* new_event =
        detach_event(event,
                     new_event_msg_begin,
                     new_event_msg_end,
                     old_event_msg_begin,
                     old_event_msg_end,
                     changed_events);
        
      if(log_stream)
      {
        *log_stream << event.id.string() << " (" << event.messages().size()
                    << "/" << event.dissenters() << "/" << event.strain()
                    << ") + " << new_event->id.string() << " ("
                    << new_event->messages().size() << "/"
                    << new_event->dissenters() << "/" << new_event->strain()
                    << "); " << split_overlap << "/" << split_merge_level
                    << " "
                    << El::Moment::time(ACE_Time_Value(max_time_diff), true);
      }
      
      return true;
    }

    EventObject*
    EventManager::detach_event(
      EventObject& event,
      MessageInfoVector::const_iterator new_event_msg_begin,
      MessageInfoVector::const_iterator new_event_msg_end,
      MessageInfoVector::const_iterator old_event_msg_begin,
      MessageInfoVector::const_iterator old_event_msg_end,
      EventNumberSet& changed_events)
      throw(El::Exception)
    {
      El::Luid new_event_id = get_event_id();
      unsigned long new_event_number = get_event_number();

      EventObject& new_event =
        *events_.insert(
          std::make_pair(new_event_number, new EventObject())).first->second;

      new_event.id = new_event_id;
      new_event.lang = event.lang;
      new_event.flags |= EventObject::EF_DIRTY | EventObject::EF_MEM_ONLY |
        (event.flags & EventObject::EF_DISSENTERS_CLEANUP);

      id_to_number_map_[new_event_id] = new_event_number;

      for(MessageInfoVector::const_iterator i(new_event_msg_begin);
          i != new_event_msg_end; ++i)
      {
        message_events_[i->id] = new_event_number;
      }
      
      unreference_words(event);

      MessageInfoArray new_event_messages;
      
      new_event_messages.init<MessageInfoVector::const_iterator>(
        new_event_msg_begin, new_event_msg_end);

      size_t new_event_capacity = new_event_msg_end - new_event_msg_begin;

      set_messages(new_event, new_event_messages);
      
      uint32_t dis_val = 0;
      detach_message_candidate(new_event, 0, 0, 0, &dis_val);
      new_event.dissenters(dis_val);

      if(set_merge(new_event))
      {
        changed_events_.insert(EventCardinality(new_event));
      }

      event.flags |= EventObject::EF_DIRTY;
      
      MessageInfoArray old_event_messages;
      
      old_event_messages.init<MessageInfoVector::const_iterator>(
        old_event_msg_begin, old_event_msg_end);

      size_t event_capacity = old_event_msg_end - old_event_msg_begin;
      set_messages(event, old_event_messages);

      dis_val = 0;
      detach_message_candidate(event, 0, 0, 0, &dis_val);

      dissenters_ += new_event.dissenters() + dis_val;
      dissenters_ -= event.dissenters();
      
      event.dissenters(dis_val);
      
      if(set_merge(event))
      {
        changed_events_.insert(EventCardinality(event));
      }

      if(message_event_updates_.get() == 0)
      {
        message_event_updates_.reset(new MessageIdToEventInfoMap());
      }

      for(MessageInfoVector::const_iterator i(new_event_msg_begin);
          i != new_event_msg_end; ++i)
      {
        (*message_event_updates_)[i->id] =
          EventInfo(new_event.id, new_event_capacity);
      }      
        
      for(MessageInfoVector::const_iterator i(old_event_msg_begin);
          i != old_event_msg_end; ++i)
      {
        (*message_event_updates_)[i->id] =
          EventInfo(event.id, event_capacity);
      }

      EventIdToEventNumberMap::const_iterator i =
        id_to_number_map_.find(event.id);

      assert(i != id_to_number_map_.end());
      changed_events.insert(i->second);

// New event is not added to changed_events intentionally. Allows to keep
// new event mem only and save on DB operation if absorb other event or
// get absorbed soon. Some code relies on the fact new event do not get into DB
// immediatelly.
      
      return &new_event;
    }
    
    bool
    EventManager::separate_event(EventNumber event_number,
                                 time_t expire_time,
                                 EventNumberSet& changed_events,
                                 std::ostringstream* log_stream)
      throw(El::Exception)
    {
      EventNumberToEventMap::iterator eit = events_.find(event_number);
            
      if(eit == events_.end())
      {
        return false;
      }

      EventObject& event = *eit->second;
/*
      if(event.id.data == 0x60F4E1D900B6A901ULL)
      {
        std::cerr << "AAAAA\n";
      }
*/    
      const MessageInfoArray& messages = event.messages();
      size_t event_size = messages.size();

      if(event_size < 4)
      {
        return false;
      }

      size_t max_event_size = 0;
      uint32_t max_word_id = 0;
      MessageInfoVector new_event_messages;
      MessageInfoVector old_event_messages;
      
      for(EventWordWeightArray::const_iterator b(event.words.begin()), i(b),
            e(event.words.end()); i != e; ++i)
      {
        if(i->first_count == 0)
        {
          continue;
        }

        uint32_t word_id = i->word_id;
        MessageInfoVector msgs1;
        MessageInfoVector msgs2;

        for(MessageInfoArray::const_iterator i(messages.begin()),
              e(messages.end()); i != e; ++i)
        {
          uint64_t published = i->published;
          
          MessageCoreWordsMap::const_iterator it =
            message_core_words_.find(i->id);
          
          if(it == message_core_words_.end() ||
             published <= (uint64_t)expire_time)
          {
            return false;
          }

          const Message::CoreWords& words = it->second->words;

          bool separate = false;
          
          for(Message::CoreWords::const_iterator j(words.begin()),
                e(words.end()); j != e; ++j)
          {
            if(*j == word_id)
            {
              separate = true;
              break;
            }
          }

          if(separate)
          {
            msgs1.push_back(*i);
          }
          else
          {
            msgs2.push_back(*i);
          }
        }

        size_t msgs_size = std::min(msgs1.size(), msgs2.size());
        
        if(msgs_size < 2)
        {
          continue;
        }
        
        EventObject event1;
        EventObject event2;
          
        create_event(event1, msgs1.begin(), msgs1.end(), event.lang);
        create_event(event2, msgs2.begin(), msgs2.end(), event.lang);

        bool proper_strain =
          (event1.strain() <= merge_max_strain_ &&
           event2.strain() <= merge_max_strain_) ||
          (event1.dissenters() + event2.dissenters() < event.dissenters());

        if(!proper_strain)
        {
          continue;
        }
        
        size_t overlap = event1.words_overlap(event2);

        size_t merge_level =
          event_merge_level(0,
                            0,
                            event1.time_diff(event2),
                            event1.time_range(event2));

        if(overlap >= merge_level)
        {
          continue;
        }
          
        if(max_event_size < msgs_size)
        {
          max_event_size = msgs_size;
          max_word_id = word_id;

          if(msgs1.size() < msgs2.size())
          {
            new_event_messages = msgs1;
            old_event_messages = msgs2;
          }
          else
          {
            new_event_messages = msgs2;
            old_event_messages = msgs1;
          }
        }
      }

      if(max_word_id)
      {
        if(log_stream)
        {
          *log_stream << "\n    decompose: " << event.id.string() << " ("
                      << event_size << "/" << event.dissenters()
                      << "/" << event.strain() << ") / " << max_word_id
                      << ": ";
        }

        EventObject* new_event =
          detach_event(event,
                       new_event_messages.begin(),
                       new_event_messages.end(),
                       old_event_messages.begin(),
                       old_event_messages.end(),
                       changed_events);        
          
        size_t overlap = new_event->words_overlap(event);

        size_t merge_level =
          event_merge_level(0,
                            0,
                            new_event->time_diff(event),
                            new_event->time_range(event));
        
        if(log_stream)
        {
          *log_stream << new_event->id.string() << ":"
                      << new_event_messages.size() << "/"
                      << new_event->dissenters() << "/" << new_event->strain()
                      << " + "
                      << event.id.string() << ":" << old_event_messages.size()
                      << "/" << event.dissenters() << "/" << event.strain()
                      << " = " << overlap << "/" << merge_level;
/*
          << "   MSG";
            
          for(MessageInfoVector::const_iterator i(new_event_messages.begin()),
                e(new_event_messages.end()); i != e; ++i)
          {
            *log_stream << " " << i->id.string();
          }
*/
        }

        return true;
      }

      size_t top_first_count = 0;

      for(EventWordWeightArray::const_iterator i(event.words.begin()),
            e(event.words.end()); i != e; ++i)
      {
        size_t fc = i->first_count;
        
        if(fc)
        {
          top_first_count += fc;
        }
        else
        {
          break;
        }
      }

      if(top_first_count < event_size / 2)
      {
        if(log_stream)
        {
          *log_stream << "\n    sloppy: " << event.id.string() << " ("
                      << top_first_count << "/" << event_size << ")";
        }
      }
        
      return false;
    }

    class EW_Set :
      public google::dense_hash_set<uint32_t,
                                    El::Hash::Numeric<uint32_t> >
    {
    public:
      EW_Set() throw(El::Exception);
    };

    inline
    EW_Set::EW_Set() throw(El::Exception)
    {
      set_empty_key(0);
      set_deleted_key(UINT32_MAX);
    }
    
    class EW_Map :
      public google::dense_hash_map<uint32_t,
                                    bool,
                                    El::Hash::Numeric<uint32_t> >
    {
    public:
      EW_Map() throw(El::Exception);
    };

    inline
    EW_Map::EW_Map() throw(El::Exception)
    {
      set_empty_key(0);
      set_deleted_key(UINT32_MAX);
    }
    
    struct MCW_Map :
      public google::dense_hash_map<Message::Id,
                                    EW_Set*,
                                    Message::MessageIdHash>
    {
      MCW_Map() throw(El::Exception);
      ~MCW_Map() throw();
    };    

    inline
    MCW_Map::MCW_Map() throw(El::Exception)
    {
      set_empty_key(Message::Id::zero);
      set_deleted_key(Message::Id::nonexistent);
    }
    
    inline
    MCW_Map::~MCW_Map() throw()
    {
      for(iterator it = begin(); it != end(); it++)
      {
        delete it->second;
      }
    }
    
    bool
    EventManager::remake_event(EventNumber event_number,
                               time_t now,
                               size_t remake_min_size,
                               El::MySQL::Connection* connection,
                               std::ostringstream* log_stream,
                               const char* ident)
      throw(El::Exception)
    {
      EventNumberToEventMap::iterator eit = events_.find(event_number);
            
      if(eit == events_.end())
      {
        return false;
      }

      EventObject& event = *eit->second;

      const MessageInfoArray& messages = event.messages();
      size_t event_size = messages.size();

      if(event_size < remake_min_size)
      {
        return false;
      }

//      unsigned long max_improve = config_.event_remake().min_improve();
      float max_improve_ln = 0;
      unsigned long candidate_improve = 0;
      unsigned long candidate_size = 0;
      uint32_t remake_word_id = 0;
      MessageInfoVector new_event_messages;
      MessageInfoVector old_event_messages;
      EventObject* improve_candidate = 0;
      float remake_rel_candidate_overlap = 0;
      float remake_rel_overlap = 0;
      bool reversed = false;

      float min_rel_overlap = 1.1;

      EventNumberFastSet skip_set;

      load_message_core_words(event_number, connection);

      EW_Map event_words;
      event_words.resize(event.words.size());

      for(EventWordWeightArray::const_iterator i(event.words.begin()),
            e(event.words.end()); i != e; ++i)
      {
        event_words.insert(std::make_pair(i->word_id, true));
      }

      const uint32_t max_mcw_check = 3;

      for(MessageInfoArray::const_iterator i(messages.begin()),
            e(messages.end()); i != e; ++i)
      {
        Message::Id id = i->id;
        MessageCoreWordsMap::const_iterator it = message_core_words_.find(id);
        
        if(it == message_core_words_.end())
        {
          return false;
        }

        const Message::CoreWords& words = it->second->words;

        for(Message::CoreWords::const_iterator j(words.begin()),
              e(words.size() > max_mcw_check ?
                words.begin() + max_mcw_check : words.end()); j != e; ++j)
        {
          event_words.insert(std::make_pair(*j, false));
        }
      }

      MCW_Map message_words;
      message_words.resize(messages.size());
        
      for(MessageInfoArray::const_iterator i(messages.begin()),
            e(messages.end()); i != e; ++i)
      {
        Message::Id id = i->id;
        MessageCoreWordsMap::const_iterator it = message_core_words_.find(id);
        
        if(it == message_core_words_.end())
        {
          return false;
        }

        const Message::CoreWords& words = it->second->words;
        
        EW_Set* ew = new EW_Set();
        message_words[id] = ew;
        ew->resize(event.words.size());        

        for(Message::CoreWords::const_iterator j(words.begin()),
              e(words.end()); j != e; ++j)
        {
          uint32_t word_id = *j;

          if(event_words.find(word_id) != event_words.end())
          {
            ew->insert(word_id);
          }
        }
      }

      for(EW_Map::const_iterator i(event_words.begin()), e(event_words.end());
          i != e; ++i)
      {
        uint32_t word_id = i->first;
        bool is_event_word = i->second;
        
        MessageInfoVector msgs1;
        MessageInfoVector msgs2;

        msgs1.reserve(messages.size());
        msgs2.reserve(messages.size());

        for(MessageInfoArray::const_iterator i(messages.begin()),
              e(messages.end()); i != e; ++i)
        {
          const MessageInfo& mi = *i;
          const EW_Set& ew = *(message_words[mi.id]);

          if(ew.find(word_id) != ew.end())
          {
            msgs1.push_back(mi);
          }
          else
          {
            msgs2.push_back(mi);
          }
        }

        size_t msgs_size = std::min(msgs1.size(), msgs2.size());
        
        if(msgs_size < remake_min_part_)
        {
          continue;
        }

        EventObject event1;
        create_event(event1, msgs1.begin(), msgs1.end(), event.lang);

        if(skip_set.find(event1.hash()) != skip_set.end())
        {
          continue;
        }

        skip_set.insert(event1.hash());

        EventObject event2;
        create_event(event2, msgs2.begin(), msgs2.end(), event.lang);

        if(skip_set.find(event2.hash()) != skip_set.end())
        {
          continue;
        }

        skip_set.insert(event2.hash());
        
        bool proper_strain =
          event1.strain() <= merge_max_strain_ &&
          event2.strain() <= merge_max_strain_;

        if(!proper_strain)
        {
          continue;
        }

        MergeDenialMap::const_iterator it =
          merge_blacklist_.find(HashPair(event1.hash(),
                                         event2.hash(),
                                         event.lang));
        
        if(it != merge_blacklist_.end())
        {
//          std::cerr << "AAA: " << event.id.string() << " " << word_id
//                    << std::endl;
          
          continue;
        }

        event1.flags |= EventObject::EF_CAN_MERGE;
        event2.flags |= EventObject::EF_CAN_MERGE;

        size_t overlap = event1.words_overlap(event2);

        size_t merge_level =
          event_merge_level(0,
                            0,
                            event1.time_diff(event2),
                            event1.time_range(event2));
        
        float rel_overlap = (float)overlap / merge_level;

        EventObject* candidate =
          find_remake_candidate(word_id,
                                is_event_word,
                                event_number,
                                event1,
                                msgs1,
                                event2,
                                msgs2,
                                rel_overlap,
                                now,
                                max_improve_ln,
                                candidate_improve,
                                candidate_size,
                                remake_word_id,
                                remake_rel_overlap,
                                remake_rel_candidate_overlap,
                                new_event_messages,
                                old_event_messages,
                                connection,
                                log_stream);

        if(candidate)
        {
          improve_candidate = candidate;
          reversed = false;
        }

        candidate = find_remake_candidate(word_id,
                                          is_event_word,
                                          event_number,
                                          event2,
                                          msgs2,
                                          event1,
                                          msgs1,
                                          rel_overlap,
                                          now,
                                          max_improve_ln,
                                          candidate_improve,
                                          candidate_size,
                                          remake_word_id,
                                          remake_rel_overlap,
                                          remake_rel_candidate_overlap,
                                          new_event_messages,
                                          old_event_messages,
                                          connection,
                                          log_stream);

        if(candidate)
        {
          improve_candidate = candidate;
          reversed = true;
        }

        if(!improve_candidate && overlap < merge_level &&
           min_rel_overlap > rel_overlap)
        {
          min_rel_overlap = rel_overlap;
          new_event_messages = msgs1;
          old_event_messages = msgs2;
          remake_word_id = word_id;
        }
      }

      if(!new_event_messages.empty())
      {
        if(log_stream)
        {
          *log_stream << std::endl << ident << "    remake: event "
                      << event.id.string()
                      << " (" << event.size()
                      << "); word " << (reversed ? "-" : "")
                      << remake_word_id << "; candidate ";

          if(improve_candidate)
          {
            *log_stream << improve_candidate->id.string()
                        << " (" << improve_candidate->size()
                        << "); rel_overlap " << remake_rel_overlap
                        << "/" << remake_rel_candidate_overlap
                        << "; improve " << candidate_improve
                        << "%; improve_ln " << max_improve_ln
                        << "; url1 v=E0+" << event.id.string()
                        << "&ep=" << remake_word_id << "&eo="
                        << improve_candidate->id.string()
                        << "; url2 v=E0+" << improve_candidate->id.string();
          }
          else
          {
            *log_stream << "NONE; rel_overlap " << min_rel_overlap
                        << "; url v=E0+" << event.id.string()
                        << "&ep=" << remake_word_id;
          }
          
          *log_stream << " " << event.lang.l3_code();
        }

        if(!config_.event_remake().dry_run())
        {
          EventNumberSet changed_events;
          
          EventObject* new_event =
            detach_event(event,
                         new_event_messages.begin(),
                         new_event_messages.end(),
                         old_event_messages.begin(),
                         old_event_messages.end(),
                         changed_events);

          time_t current_time = ACE_OS::gettimeofday().sec();
          uint64_t timeout = merge_deny_timeout(event, *new_event);
/*
          if(!improve_candidate)
          {
// TODO: productionize ...
            timeout *= 10;
          }
*/        
          merge_blacklist_[HashPair(event.hash(),
                                    new_event->hash(),
                                    event.lang)] =
            MergeDenialInfo(current_time + timeout);
          
          if(log_stream)
          {
            const MessageInfoArray& messages = new_event->messages();
              
            *log_stream << std::endl << ident << "      "
                        << new_event->id.string()
                        << " (" << messages.size() << "): MSG";

            for(MessageInfoArray::const_iterator i(messages.begin()),
                  e(messages.end()); i != e; ++i)
            {
              *log_stream << " " << i->id.string();
            }
          }

          flush_dirty_events(changed_events,
                             false,
                             /*log_stream ? &flush_log_stream :*/ 0);

          if(improve_candidate)
          {
            StringList defered_queries;
            std::ostringstream ostr;
            
            merge_events(*new_event,
                         *improve_candidate,
                         connection,
                         defered_queries,
                         log_stream ? *log_stream : ostr,
                         log_stream &&
                         Application::will_trace(El::Logging::HIGH));

            merge_blacklist_[HashPair(event.hash(),
                                      improve_candidate->hash(),
                                      event.lang)] =
              MergeDenialInfo(current_time +
                              merge_deny_timeout(event, *improve_candidate));

// No need to call as the only query is deletion of *new_event which is
// redundunt as it is on memory only
//            execute_queries(connection, defered_queries, "remake_events");
          }
        }
        
        return true;
      }

      return false;
    }

    EventObject*
    EventManager::find_remake_candidate(uint32_t word_id,
                                        bool is_event_word,
                                        EventNumber event_number,
                                        const EventObject& event1,
                                        const MessageInfoVector& msgs1,
                                        const EventObject& event2,
                                        const MessageInfoVector& msgs2,
                                        float rel_overlap,
                                        time_t now,
                                        float& max_improve_ln,
                                        unsigned long& candidate_improve,
                                        unsigned long& candidate_size,
                                        uint32_t& remake_word_id,
                                        float& remake_rel_overlap,
                                        float& remake_rel_candidate_overlap,
                                        MessageInfoVector& new_event_messages,
                                        MessageInfoVector& old_event_messages,
                                        El::MySQL::Connection* connection,
                                        std::ostringstream* log_stream)
      throw(El::Exception)
    {
      EventNumber denied_best_overlap_event = 0;
      uint32_t merge_deny_timeout = 0;

      EventNumber best_overlap_event =
        find_best_overlap(event1,
                          event_number,
                          connection,
                          now,
                          remake_min_part_,
                          denied_best_overlap_event,
                          merge_deny_timeout,
                          log_stream);
      
      if(best_overlap_event == 0)
      {
        return 0;
      }

      EventNumberToEventMap::iterator eit = events_.find(best_overlap_event);
      assert(eit != events_.end());
        
      EventObject& candidate = *eit->second;
      size_t candidate_overlap = event1.words_overlap(candidate);

      size_t candidate_merge_level =
        event_merge_level(0,
                          std::max(event1.strain(), candidate.strain()),
                          event1.time_diff(candidate),
                          event1.time_range(candidate));        

      float rel_candidate_overlap =
        (float)candidate_overlap / candidate_merge_level;

      rel_overlap = std::max(rel_overlap, (float)0.001);

      if(rel_candidate_overlap > rel_overlap)
      {
        float imp = (rel_candidate_overlap - rel_overlap) / rel_overlap;
        unsigned long improve = (unsigned long)(imp * 100.0 + 0.5);

        if(improve >= remake_min_improve_)
        {
/*          
          float improve_ln = imp *
            log((float)std::min(event1.size() + candidate.size(),
                                event2.size()) +
                0.01);
*/

          float improve_ln = imp *
            log((float)std::min(std::min(event1.size(), event2.size()),
                                candidate.size()) +
                0.01);

          if(is_event_word)
          {
            improve_ln *= 10;
          }
        
//        if(improve > max_improve ||
//           (improve == max_improve &&
//            candidate.messages().size() > candidate_size))
        
          if(improve_ln > max_improve_ln)
          {
            max_improve_ln = improve_ln;
            candidate_improve = improve;
            candidate_size = candidate.messages().size();          
            remake_word_id = word_id;
            
            remake_rel_candidate_overlap = rel_candidate_overlap;
            remake_rel_overlap = rel_overlap;
            
            new_event_messages = msgs1;
            old_event_messages = msgs2;
            return &candidate;
          }
        }
      }

      return 0;
    }
    
    bool
    EventManager::revise_event(
      EventNumber event_number,
      time_t expire_time,
      EventNumberSet& revised_events,
      Transport::MessageDigestArray& detached_message_digests)
      throw(El::Exception)
    {
      EventNumberToEventMap::iterator eit = events_.find(event_number);
            
      if(eit == events_.end())
      {
        return false;
      }

      EventObject& event = *eit->second;
      MessageInfoArray& messages = event.messages();

      size_t detach_message_index = 0;

      uint32_t dis_val = 0;

//      bool debug_event = event.id.data == 0x25DCB3EEDC151B1ELL;

      bool res = detach_message_candidate(event,
                                          0,
                                          expire_time,
                                          &detach_message_index,
                                          &dis_val);
      
      dissenters_ += dis_val;
      dissenters_ -= event.dissenters();

      event.dissenters(dis_val);
/*
      if(debug_event && !res)
      {
        std::cerr << "Revised, dis " << event.dissenters() << std::endl;
      }
*/
/*
      if(event.id.data == 16354552122676279857ULL)
      {
        std::cerr << El::Moment(ACE_OS::gettimeofday()).iso8601()
                  << " " << res << "/" << detach_message_index
                  << "/" << dis_val << std::endl;
      }
*/
      
      if(!res)
      {
        set_merge(event);
        event.flags |= EventObject::EF_REVISED;
        return true;
      }

      uint64_t& message_published =
        messages[detach_message_index].published;
        
      const Message::Id& message_id = messages[detach_message_index].id;

      MessageCoreWordsMap::const_iterator it =
        message_core_words_.find(message_id);

      if(it != message_core_words_.end())
      {
        const Message::CoreWords& core_words = it->second->words;        
        
        detached_message_digests.push_back(
          Transport::MessageDigest(message_id,
                                   message_published,
                                   event.lang,
                                   core_words,
                                   event.id));
      }
      
      message_published = 0;
      event.flags |= EventObject::EF_DIRTY;
      event.flags &= ~EventObject::EF_CAN_MERGE;
      
      revised_events.insert(event_number);

      if(messages.size() == 2)
      {
        for(size_t i = 0; i < messages.size(); i++)
        {
          const Message::Id& message_id = messages[i].id;
          uint64_t& message_published = messages[i].published;
          
          if(!message_published)
          {
            continue;
          }
          
          MessageCoreWordsMap::const_iterator it =
            message_core_words_.find(message_id);

          if(it != message_core_words_.end())
          {
            detached_message_digests.push_back(
              Transport::MessageDigest(message_id,
                                       message_published,
                                       event.lang,
                                       it->second->words,
                                       event.id));
          }

          message_published = 0;
        }
      }

      // To recalc published_max & published_min
      event.calc_hash();
      return false;
    }

    bool
    EventManager::start() throw(Exception, El::Exception)
    {      
      if(!El::Service::CompoundService<
         El::Service::Service, EventManagerCallback>::start())
      {
        return false;
      }

      El::Service::CompoundServiceMessage_var msg;
      
      if(loaded())
      {
        schedule_merge(0);
            
        msg = new TraverseEvents(this);
        deliver_now(msg.in());
            
        if(config_.event_remake().traverse_records())
        {
          msg = new RemakeTraverseEvents(this);
          deliver_now(msg.in());
        }
      }
      else
      {
        msg = new LoadEvents(this);
        deliver_now(msg.in());
      }

      return true;
    }

    void
    EventManager::give(EventCardinalities& changed_events,
                       MergeDenialMap& merge_blacklist)
      throw(El::Exception)
    {
      WriteGuard guard(srv_lock_);

      while(!changed_events_.empty())
      {
        EventCardinality cardinality = changed_events_.pop();
        changed_events.insert(cardinality);
      }
      
      merge_blacklist.insert(merge_blacklist_.begin(), merge_blacklist_.end());
      merge_blacklist_.clear();
    }
    
    void
    EventManager::take(EventCardinalities& changed_events,
                       MergeDenialMap& merge_blacklist)
      throw(El::Exception)
    {
      EventCardinalities new_changed_events;
      
      {
        WriteGuard guard(srv_lock_);

        while(!changed_events.empty())
        {
          EventCardinality cardinality = changed_events.pop();
          
          if(langs_.find(cardinality.lang) == langs_.end())
          {
            new_changed_events.insert(cardinality);
          }
          else
          {
            changed_events_.insert(cardinality);
          }
        }

//        std::cerr << "PRE: " << merge_blacklist.size() << std::endl;
        
        for(MergeDenialMap::iterator i(merge_blacklist.begin()),
              e(merge_blacklist.end()); i != e; ++i)
        {
          const HashPair& hp = i->first;
          const MergeDenialInfo& hpi = i->second;

          if(langs_.find(hp.lang) != langs_.end())
          {
//            md_insert_meter.start();
            merge_blacklist_.insert(std::make_pair(hp, hpi));
//            md_insert_meter.stop();
            merge_blacklist.erase(i);
          }
        }

//        std::cerr << "POST: " << merge_blacklist.size() << "/"
//                  << merge_blacklist_.size() << std::endl;
      }
      
      new_changed_events.swap(changed_events);
    }
    
    void
    EventManager::wait() throw(Exception, El::Exception)
    {      
      El::Service::CompoundService<
      El::Service::Service, EventManagerCallback>::wait();

      {
        StatusGuard guard(status_lock_);
        
        if(events_unloaded_)
        {
          return;
        }
        else
        {
          events_unloaded_ = true;
        }
      } 
      
      EventNumberSet flush_events;
      
      {
        ReadGuard guard(srv_lock_);
      
        for(EventNumberToEventMap::const_iterator it = events_.begin();
            it != events_.end(); it++)
        {
          if(it->second->flags & EventObject::EF_DIRTY)
          {
            flush_events.insert(it->first);
          }
        }
/*
        fbo_meter1.dump_header(
          (std::string("FBO 1 ") + lang_string()).c_str());

        fbo_meter11.dump_header(
          (std::string("FBO 11 ") + lang_string()).c_str());
        fbo_meter12.dump_header(
          (std::string("FBO 12 ") + lang_string()).c_str());
        fbo_meter13.dump_header(
          (std::string("FBO 13 ") + lang_string()).c_str());
        fbo_meter14.dump_header(
          (std::string("FBO 14 ") + lang_string()).c_str());
        fbo_meter15.dump_header(
          (std::string("FBO 15 ") + lang_string()).c_str());
        fbo_meter16.dump_header(
          (std::string("FBO 16 ") + lang_string()).c_str());
        fbo_meter17.dump_header(
          (std::string("FBO 17 ") + lang_string()).c_str());
        
        fbo_meter2.dump_header(
          (std::string("FBO 2 ") + lang_string()).c_str());
          
        fbo_meter3.dump_header(
          (std::string("FBO 3 ") + lang_string()).c_str());
          
        fbo_meter1.dump(std::cerr);
        fbo_meter11.dump(std::cerr);
        fbo_meter12.dump(std::cerr);
        fbo_meter13.dump(std::cerr);
        fbo_meter14.dump(std::cerr);
        fbo_meter15.dump(std::cerr);
        fbo_meter16.dump(std::cerr);
        fbo_meter17.dump(std::cerr);
        fbo_meter2.dump(std::cerr);
        fbo_meter3.dump(std::cerr);
*/
      }

      std::ostringstream flush_log_stream;

      El::Logging::Logger* trace_logger =
        Application::will_trace(El::Logging::MIDDLE) ?
        Application::logger() : (El::Logging::Logger*)0;
      
      if(trace_logger)
      {
        flush_log_stream
          << "NewsGate::Event::EventManager::wait("
          << lang_string() << "): dumping dirty events:\n";
      }

      try
      {
        flush_dirty_events(flush_events,
                           true,
                           trace_logger ? &flush_log_stream : 0);
        
        if(trace_logger)
        {
          trace_logger->trace(flush_log_stream.str(),
                              Aspect::EVENT_MANAGEMENT,
                              El::Logging::MIDDLE);
        }
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Event::EventManager::wait("
             << lang_string() << "): "
          "El::Exception caught. Description:" << std::endl << e;
        
        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);      
      }
      
    }

  }
}
