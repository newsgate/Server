/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Event/Bank/ManagingEvents.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <string>
#include <sstream>
#include <fstream>
#include <utility>
#include <vector>

#include <ace/OS.h>

#include <El/Exception.hpp>

#include <Commons/Event/TransportImpl.hpp>

#include "SubService.hpp"
#include "BankMain.hpp"
#include "ManagingEvents.hpp"
#include "EventRecord.hpp"

#include <Services/Dictionary/Commons/DictionaryServices.hpp>
#include <Services/Dictionary/Commons/TransportImpl.hpp>

//#define SEARCH_PROFILING 100

namespace
{
};

namespace NewsGate
{
  namespace Event
  {
    //
    // ManagingEvents class
    //
    ManagingEvents::ManagingEvents(
      Event::BankSession* session,
      const ACE_Time_Value& report_presence_period,
      BankStateCallback* callback)
      throw(Exception, El::Exception)
        : BankState(callback, "ManagingEvents"),
          report_presence_period_(report_presence_period),
//          dict_hash_(UINT32_MAX),
          ready_(false),
          config_(Application::instance()->config())
    {
      session->_add_ref();
      session_ = session;

      create_managers();
      
      El::Service::CompoundServiceMessage_var msg = new ReportPresence(this);
      
      deliver_at_time(msg.in(),
                      ACE_OS::gettimeofday() + report_presence_period_);

      msg = new Monitoring(this);
      deliver_now(msg.in());
    }
    
    ManagingEvents::~ManagingEvents() throw()
    {
    }

    void
    ManagingEvents::add_lang_managers(const EventManager::LangSet& langs)
      throw(Exception, El::Exception)
    {
      size_t backets = config_.backets();

      WriteGuard guard(srv_lock_);

      for(EventManager::LangSet::const_iterator it = langs.begin();
          it != langs.end(); ++it)
      {
        El::Lang lang(*it);
        
        if(event_manager_langs_.find(lang) != event_manager_langs_.end())
        {
          continue;
        }

        if(event_managers_.size() < backets)
        {
          EventManager_var event_manager =
            new EventManager(session_.in(),
                             this,
                             false,
                             false,
                             config_.event_manager());

          event_manager->add_lang(lang);
          event_manager_langs_.insert(lang);
          
          event_managers_.push_back(event_manager);
          event_manager->start();
          
          continue;
        }

        size_t min_count = SIZE_MAX;
        
        EventManagerArray::const_iterator min_count_eit =
          event_managers_.end();
        
        for(EventManagerArray::const_iterator eit = event_managers_.begin();
            eit != event_managers_.end(); ++eit)
        {
          EventManager_var event_manager = *eit;
          
          size_t count = event_manager->event_count();
            
          if(count < min_count)
          {
            min_count = count;
            min_count_eit = eit;
          }          
        }

        assert(min_count_eit != event_managers_.end());
        
        (*min_count_eit)->add_lang(lang);
        event_manager_langs_.insert(lang);
      }
    }
    
    void
    ManagingEvents::create_managers() throw(Exception, El::Exception)
    {
      size_t event_count = 0;
      size_t backets = config_.backets();
      
      EventManager::EventCardinalities changed_events;
      EventManager::MergeDenialMap merge_blacklist;
      
      std::string cache_filename = config_.event_manager().cache_file() +
        ".chn";
      
      std::fstream file(cache_filename.c_str(), ios::in);
      
      WriteGuard guard(srv_lock_);

      typedef std::pair<EventManager::LangSet, size_t> LangEventCounter;
      typedef std::vector<LangEventCounter> LangEventCounters;
      
      LangEventCounters lang_events;
        
      El::MySQL::Connection_var connection =
        Application::instance()->dbase()->connect();
      
      El::MySQL::Result_var result =
        connection->query(
          "select lang, count(*) as events from Event group by lang");

      EventLangRecord record(result.in());
      
      while(record.fetch_row())
      {
        El::Lang lang((El::Lang::ElCode)record.lang().value());
        size_t count = record.events();
        
        LangEventCounters::iterator it = lang_events.begin();
        for(; it != lang_events.end() && it->second > count; ++it);

        EventManager::LangSet lang_set;
        lang_set.insert(lang);
        
        lang_events.insert(it, std::make_pair(lang_set, count));
        event_count += count;
      }      

      while(lang_events.size() > backets)
      {
        LangEventCounters::iterator ait = lang_events.begin() + backets;
        LangEventCounters::iterator min_count_it = lang_events.end();
        size_t min_count = SIZE_MAX;
        
        for(LangEventCounters::iterator it = lang_events.begin(); it != ait;
            ++it)
        {
          if(it->second < min_count)
          {
            min_count = it->second;
            min_count_it = it;
          }
        }

        assert(min_count_it != lang_events.end());

        min_count_it->first.insert(*ait->first.begin());
        min_count_it->second += ait->second;

        lang_events.erase(ait);
      }

      bool all_events_changed = true;

      if(file.is_open())
      {
        El::BinaryInStream bstr(file);
        
        try
        {
          bstr >> changed_events;
          bstr.read_map(merge_blacklist);
          
          all_events_changed = false;
        }
        catch(const El::Exception&)
        {
          // It can be not just El::BinaryInStream::Exception but some
          // STL exception like std::bad_alloc due to file corruption
          
          changed_events.clear();
          merge_blacklist.clear();
        }
          
        file.close();
        unlink(cache_filename.c_str());        
      }      

      std::ostringstream ostr;
      ostr << "ManagingEvents::create_managers:";
      
      for(LangEventCounters::const_iterator it = lang_events.begin();
          it != lang_events.end(); ++it)
      {
        EventManager_var event_manager =
          new EventManager(session_.in(),
                           this,
                           true,
                           all_events_changed,
                           config_.event_manager());

        ostr << std::endl;
        const EventManager::LangSet& langs = it->first;
        
        for(EventManager::LangSet::const_iterator lit = langs.begin();
            lit != langs.end(); ++lit)
        {
          ostr << lit->l3_code() << " ";
          event_manager->add_lang(*lit);
          event_manager_langs_.insert(*lit);
        }

        ostr << ": " << it->second;
        event_managers_.push_back(event_manager);

        event_manager->take(changed_events, merge_blacklist);
        event_manager->start();
      }

      Application::logger()->trace(ostr.str(),
                                   Aspect::STATE,
                                   El::Logging::MIDDLE);

      ready_ = true;

      if(backets > 1)
      {
        const Server::Config::EventBankType::recreate_managers_type& config =
          config_.recreate_managers();
  
        El::Service::CompoundServiceMessage_var msg =
          new RecreateManagers(
            std::max(config.event_count_min(),
                     event_count * config.event_count_rescale()), this);
        
        deliver_now(msg.in());
      }
    }

    bool
    ManagingEvents::stop() throw(Exception, El::Exception)
    {
      bool res = BankState::stop();      
        
      ReadGuard guard(srv_lock_);

      for(EventManagerArray::iterator it = event_managers_.begin();
          it != event_managers_.end(); ++it)
      {
        res |= (*it)->stop();
      }

      return res;
    }
    
    void
    ManagingEvents::destroy_managers() throw(Exception, El::Exception)
    {
      EventManager::EventCardinalities changed_events;
      EventManager::MergeDenialMap merge_blacklist;
      EventManagerArray managers;
      
      {
        WriteGuard guard(srv_lock_);
        
        if(!ready_)
        {
          return;
        }
        
        ready_ = false;
        managers = event_managers_;
      }

      for(EventManagerArray::iterator it = managers.begin();
          it != managers.end(); ++it)
      {
        (*it)->stop();
      }
        
      for(EventManagerArray::iterator it = managers.begin();
            it != managers.end(); ++it)
      {
        EventManager_var manager = *it;
        
        manager->wait();
        manager->give(changed_events, merge_blacklist);
      }

      {
        WriteGuard guard(srv_lock_);
        
        event_managers_.clear();
        event_manager_langs_.clear();
      }
      
      std::string cache_filename = config_.event_manager().cache_file() +
        ".chn";
      
      std::fstream file(cache_filename.c_str(), ios::out);
        
      if(!file.is_open())
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Event::ManagingEvents::wait: "
          "failed to open file '" << cache_filename << "' for write access";
        
        throw Exception(ostr.str());
      }
      
      El::BinaryOutStream bstr(file);

      try
      {
        bstr << changed_events;
        bstr.write_map(merge_blacklist);
      }
      catch(...)
      {
        unlink(cache_filename.c_str());
        throw;
      }
    }
    
    void
    ManagingEvents::wait() throw(Exception, El::Exception)
    {
      BankState::wait();
      destroy_managers();
    }
    
    void
    ManagingEvents::report_presence() throw()
    {
      if(!started())
      {
        return;
      }
      
      try
      {
        try
        {
          Event::BankManager_var bank_manager =
            Application::instance()->bank_manager();

          bank_manager->ping(Application::instance()->bank_ior(),
                             session_->id());
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::ManagingEvents::report_presence: "
            "El::Exception caught. Description:" << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(const NewsGate::Event::Logout& e)
        {  
          if(started())
          {
            if(Application::will_trace(El::Logging::MIDDLE))
            {
              std::ostringstream ostr;
              ostr << "NewsGate::Event::ManagingEvents::report_presence: "
                "loging out. Reason:\n" << e.reason.in();
            
              Application::logger()->trace(ostr.str(),
                                           Aspect::STATE,
                                           El::Logging::MIDDLE);
            }
            
            callback_->logout(session_->id());
          }
          
          return;
        }
        catch(const CORBA::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::ManagingEvents::report_presence: "
            "CORBA::Exception caught. Description:" << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }

        try
        {
          El::Service::CompoundServiceMessage_var msg =
            new ReportPresence(this);
          
          deliver_at_time(
            msg.in(),
            ACE_OS::gettimeofday() + report_presence_period_);
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::ManagingEvents::report_presence: "
            "El::Exception caught while scheduling task. Description:"
               << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        
      }
      catch(...)
      {
        El::Service::Error error(
          "NewsGate::Event::ManagingEvents::report_presence: "
          "unexpected exception caught.",
          this);
        
        callback_->notify(&error);
      }    
      
    }

    ::CORBA::ULong
    ManagingEvents::get_message_events(
      ::NewsGate::Message::Transport::IdPack* messages,
      ::NewsGate::Message::Transport::MessageEventPack_out events)
      throw(NewsGate::Event::NotReady,
            NewsGate::Event::ImplementationException,
            ::CORBA::SystemException)
    {
      try
      {
        if(!ready())
        {
          NewsGate::Event::NotReady ex;
          
          ex.reason = "NewsGate::Event::ManagingEvents::get_message_events: "
            "not ready yet";
          
          throw ex;
        }
        
        ::NewsGate::Message::Transport::IdPackImpl::Type*
          message_transport = dynamic_cast<
          ::NewsGate::Message::Transport::IdPackImpl::Type*>(messages);

        if(message_transport == 0)
        {
          throw Exception(
            "NewsGate::Event::ManagingEvents::get_message_events: "
            "dynamic_cast failed for messages");
        }

        Message::Transport::MessageEventPackImpl::Var result =
          new Message::Transport::MessageEventPackImpl::Type(
            new Message::Transport::MessageEventArray());
        
        bool do_log = Application::will_trace(El::Logging::MIDDLE);
        const Message::IdArray& ids = message_transport->entities();
        
        std::ostringstream log_stream;

        if(do_log)
        {
          log_stream << "ManagingEvents::get_message_events: " <<
            ids.size() << " message ids arrived";
        }

        Message::Transport::MessageEventArray& message_events =
          result->entities();
          
        message_events.resize(ids.size());

        size_t i = 0;
        
        for(Message::IdArray::const_iterator it = ids.begin();
            it != ids.end(); it++, i++)
        {
          message_events[i].id = *it;
        }

        size_t total_message_count = 0;
        size_t found_messages = 0;

        EventManagerArray managers = event_managers();

        for(EventManagerArray::const_iterator it = managers.begin();
            it != managers.end(); it++)
        {
          EventManager_var manager = *it;
          
          if(do_log)
          {
            log_stream << std::endl << manager->lang_string();
          }
          
          if(!manager->get_message_events(
               ids,
               message_events,
               found_messages,
               total_message_count,
               Application::will_trace(El::Logging::HIGH) ? &log_stream : 0))
          {
            NewsGate::Event::NotReady ex;
            ex.reason = "NewsGate::Event::ManagingEvents::get_message_events: "
              "events not loaded yet";
            
            throw ex;
          }
        }
      
        if(do_log)
        {
          log_stream << "\n  * found " << found_messages << " of "
                     << ids.size();
          
          Application::logger()->trace(log_stream.str(),
                                       Aspect::EVENT_MANAGEMENT,
                                       El::Logging::MIDDLE);
        }

        events = result._retn();
        return total_message_count;
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Event::ManagingEvents::post_message_digest: "
          "El::Exception caught. Description:\n" << e.what();
        
        ImplementationException e;
        e.description = CORBA::string_dup(ostr.str().c_str());
        
        throw e;
      }
    }
    
    void
    ManagingEvents::delete_messages(
      ::NewsGate::Message::Transport::IdPack* ids)
      throw(NewsGate::Event::NotReady,
            NewsGate::Event::ImplementationException,
            ::CORBA::SystemException)
    {      
      try
      {
        Message::Transport::IdPackImpl::Type*
          ids_transport = dynamic_cast<
          Message::Transport::IdPackImpl::Type*>(ids);

        if(ids_transport == 0)
        {
          throw Exception(
            "NewsGate::Event::ManagingEvents::delete_messages: "
            "dynamic_cast failed for ids");
        }
        
        bool do_log = Application::logger()->will_log(El::Logging::MIDDLE);
        const Message::IdArray& id_array = ids_transport->entities();
        
        std::ostringstream log_stream;

        if(do_log)
        {
          log_stream << "ManagingEvents::delete_messages: " <<
            id_array.size() << " ids arrived";
        }

        bool loaded = true;
        EventManagerArray managers = event_managers();

        for(EventManagerArray::const_iterator it = managers.begin();
            it != managers.end(); it++)
        {
          EventManager_var manager = *it;
          
          loaded &=
            manager->delete_messages(id_array, do_log ? &log_stream : 0);
        }
      
        if(do_log)
        {
          Application::logger()->trace(log_stream.str(),
                                       Aspect::EVENT_MANAGEMENT,
                                       El::Logging::MIDDLE);
        }

        if(!loaded)
        {
          NewsGate::Event::NotReady ex;
          
          ex.reason = "NewsGate::Event::ManagingEvents::delete_messages: "
            "events not loaded yet or too busy";
          
          throw ex;
        }
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Event::ManagingEvents::delete_messages: "
          "El::Exception caught. Description:\n" << e.what();
        
        ImplementationException e;
        e.description = CORBA::string_dup(ostr.str().c_str());
        
        throw e;
      }
    }

    void
    ManagingEvents::post_message_digest(
      ::NewsGate::Event::Transport::MessageDigestPack* digests,
      ::NewsGate::Message::Transport::MessageEventPack_out events)
      throw(NewsGate::Event::NotReady,
            NewsGate::Event::ImplementationException,
            ::CORBA::SystemException)
    {
      try
      {
        if(!ready())
        {
          NewsGate::Event::NotReady ex;
          
          ex.reason = "NewsGate::Event::ManagingEvents::post_message_digest: "
            "not ready yet";
          
          throw ex;
        }

        Event::Transport::MessageDigestPackImpl::Type*
          digests_transport = dynamic_cast<
          Event::Transport::MessageDigestPackImpl::Type*>(digests);

        if(digests_transport == 0)
        {
          throw Exception(
            "NewsGate::Event::ManagingEvents::post_message_digest: "
            "dynamic_cast failed for digests");
        }

        Message::Transport::MessageEventPackImpl::Var result =
          new Message::Transport::MessageEventPackImpl::Type(
            new Message::Transport::MessageEventArray());
        
        bool do_log = Application::logger()->will_log(El::Logging::MIDDLE);
        
        const Event::Transport::MessageDigestArray& digests =
          digests_transport->entities();
        
        std::ostringstream log_stream;

        if(do_log)
        {
          log_stream << "ManagingEvents::post_message_digest: " <<
            digests.size() << " digests arrived";
        }

        Message::Transport::MessageEventArray& message_events =
          result->entities();
          
        message_events.resize(digests.size());

        EventManager::LangSet langs;
        size_t i = 0;
        
        for(Event::Transport::MessageDigestArray::const_iterator
              it = digests.begin(); it != digests.end(); it++, i++)
        {
          message_events[i].id = it->id;
          langs.insert(it->lang);
        }

        add_lang_managers(langs);

        EventManagerArray managers = event_managers();
        
        for(EventManagerArray::const_iterator it = managers.begin();
            it != managers.end(); it++)
        {
          EventManager_var manager = *it;
          
          if(do_log)
          {
            log_stream << std::endl << manager->lang_string();
          }
          
          if(!manager->insert(digests,
                              message_events,
                              true,
                              do_log ? &log_stream : 0))
          {
            NewsGate::Event::NotReady ex;
            
            ex.reason = "NewsGate::Event::ManagingEvents::"
              "post_message_digest: events not loaded yet";
            
            throw ex;
          }
        }
      
        if(do_log)
        {
          Application::logger()->trace(log_stream.str(),
                                       Aspect::EVENT_MANAGEMENT,
                                       El::Logging::MIDDLE);
        }

        events = result._retn();
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Event::ManagingEvents::post_message_digest: "
          "El::Exception caught. Description:\n" << e.what();
        
        ImplementationException e;
        e.description = CORBA::string_dup(ostr.str().c_str());
        
        throw e;
      }
    }

    void
    ManagingEvents::push_events(
      ::NewsGate::Event::Transport::MessageDigestPack* digests,
      ::NewsGate::Event::Transport::EventPushInfoPack* events)
      throw(NewsGate::Event::NotReady,
            NewsGate::Event::ImplementationException,
            ::CORBA::SystemException)
    {
      try
      {
        if(!ready())
        {
          NewsGate::Event::NotReady ex;
          
          ex.reason = "NewsGate::Event::ManagingEvents::push_events: "
            "not ready yet";
          
          throw ex;
        }

        Event::Transport::MessageDigestPackImpl::Type*
          digests_transport = dynamic_cast<
          Event::Transport::MessageDigestPackImpl::Type*>(digests);

        if(digests_transport == 0)
        {
          throw Exception(
            "NewsGate::Event::ManagingEvents::push_events: "
            "dynamic_cast failed for digests");
        }

        Event::Transport::EventPushInfoPackImpl::Type*
          events_transport = dynamic_cast<
          Event::Transport::EventPushInfoPackImpl::Type*>(events);

        if(events_transport == 0)
        {
          throw Exception(
            "NewsGate::Event::ManagingEvents::push_events: "
            "dynamic_cast failed for events");
        }
        
        bool do_log = Application::logger()->will_log(El::Logging::MIDDLE);
        
        const Event::Transport::MessageDigestArray& digests =
          digests_transport->entities();
        
        const Event::Transport::EventPushInfoArray& events =
          events_transport->entities();
        
        std::ostringstream log_stream;

        if(do_log)
        {
          log_stream << "ManagingEvents::push_events: "
                     << digests.size() << " digests for " << events.size()
                     << " events arrived";
        }

        EventManager::LangSet langs;
        
        for(Event::Transport::MessageDigestArray::const_iterator it =
              digests.begin(); it != digests.end(); ++it)
        {
          langs.insert(it->lang);
        }

        add_lang_managers(langs);        
        
        EventManagerArray managers = event_managers();
        
        for(EventManagerArray::const_iterator it = managers.begin();
            it != managers.end(); it++)
        {
          EventManager_var manager = *it;
          
          if(do_log)
          {
            log_stream << std::endl << manager->lang_string();
          }
          
          if(!manager->accept_events(digests,
                                     events,
                                     do_log ? &log_stream : 0))
          {
            NewsGate::Event::NotReady ex;
            ex.reason = "NewsGate::Event::ManagingEvents::push_events: "
              "events not loaded yet";

            throw ex;
          }
        }
        
        if(do_log)
        {
          Application::logger()->trace(log_stream.str(),
                                       Aspect::EVENT_MANAGEMENT,
                                       El::Logging::MIDDLE);
        }
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Event::ManagingEvents::push_events: "
          "El::Exception caught. Description:\n" << e.what();
        
        ImplementationException e;
        e.description = CORBA::string_dup(ostr.str().c_str());
        
        throw e;
      }
    }
    
    void
    ManagingEvents::get_events(
      ::NewsGate::Event::Transport::EventIdRelPack* ids,
      ::NewsGate::Event::Transport::EventObjectRelPack_out events)
      throw(NewsGate::Event::NotReady,
            NewsGate::Event::ImplementationException,
            ::CORBA::SystemException)
    {
      try
      {
        if(!ready())
        {
          NewsGate::Event::NotReady ex;
          
          ex.reason = "NewsGate::Event::ManagingEvents::get_events: "
            "not ready yet";
          
          throw ex;
        }        

        Event::Transport::EventIdRelPackImpl::Type*
          ids_transport = dynamic_cast<
          Transport::EventIdRelPackImpl::Type*>(ids);

        if(ids_transport == 0)
        {
          throw Exception("NewsGate::Event::ManagingEvents::get_events: "
                          "dynamic_cast failed for ids");
        }

        Transport::EventObjectRelPackImpl::Var result =
          new Transport::EventObjectRelPackImpl::Type(
            new Transport::EventObjectRelArray());

        const Transport::EventIdRelArray& ids = ids_transport->entities();
        
        Transport::EventObjectRelArray& result_events = result->entities();
        result_events.reserve(ids.size());
        
        EventManagerArray managers = event_managers();
        
        for(EventManagerArray::const_iterator it = managers.begin();
            it != managers.end(); it++)
        {
          EventManager_var manager = *it;          
          manager->get_events(ids, result_events);
        }

        events = result._retn();
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Event::ManagingEvents::get_events: "
          "El::Exception caught. Description:\n" << e.what();
        
        ImplementationException e;
        e.description = CORBA::string_dup(ostr.str().c_str());
        
        throw e;
      }
    }

    bool
    ManagingEvents::ready() const throw()
    {
      EventManagerArray event_managers;
      
      {
        ReadGuard guard(srv_lock_);

        if(!ready_)
        {
          return false;
        }

        event_managers = event_managers_;
      }

      for(EventManagerArray::const_iterator it = event_managers.begin();
          it != event_managers.end(); ++it)
      {
        if(!(*it)->loaded())
        {
          return false;
        }
      }

      return true;
    }

    void
    ManagingEvents::monitor_state() throw(El::Exception)
    {
      EventManagerArray managers = event_managers();
      
      std::ostringstream ostr;
      ostr << "ManagingEvents::monitor_state: " << managers.size()
           << " managers:";

      for(EventManagerArray::const_iterator it = managers.begin();
          it != managers.end(); ++it)
      {
        ostr << std::endl;
        (*it)->state().dump(ostr);
      }

      Application::logger()->trace(ostr.str(),
                                   Aspect::STATE,
                                   El::Logging::MIDDLE);

      El::Service::CompoundServiceMessage_var msg = new Monitoring(this);
      
      deliver_at_time(msg.in(),
                      ACE_OS::gettimeofday() +
                      ACE_Time_Value(config_.dump_state_period()));      
    }

    size_t
    ManagingEvents::event_count() const throw(El::Exception)
    {
      size_t count = 0;
      EventManagerArray managers = event_managers();

      for(EventManagerArray::const_iterator it = managers.begin();
          it != managers.end(); ++it)
      {
        count += (*it)->event_count();
      }

      return count;
    }
    
    void
    ManagingEvents::recreate_managers(size_t event_count_threshold)
      throw(El::Exception)
    {
      size_t count = event_count();
      
      std::ostringstream ostr;      
      ostr << "ManagingEvents::recreate_managers: checking for an event "
        "count threshold " << event_count_threshold << " (current count "
           << count << ")";
      
      Application::logger()->trace(ostr.str(),
                                   Aspect::STATE,
                                   El::Logging::MIDDLE);        
      
      if(count < event_count_threshold)
      {
        El::Service::CompoundServiceMessage_var msg =
          new RecreateManagers(event_count_threshold, this);
        
        deliver_at_time(
          msg.in(),
          ACE_OS::gettimeofday() +
          ACE_Time_Value(config_.recreate_managers().check_period()));

        return;
      }

      destroy_managers();
      create_managers();
    }
    
    bool
    ManagingEvents::notify(El::Service::Event* event) throw(El::Exception)
    {
      if(BankState::notify(event))
      {
        return true;
      }

      if(dynamic_cast<const ReportPresence*>(event) != 0)
      {
        report_presence();
        return true;
      }

      const RecreateManagers* rm = dynamic_cast<RecreateManagers*>(event);
      
      if(rm != 0)
      {
        recreate_managers(rm->event_count_threshold);
        return true;
      }

      if(dynamic_cast<const Monitoring*>(event) != 0)
      {
        monitor_state();
        return true;
      }

      std::ostringstream ostr;
      ostr << "NewsGate::Event::ManagingEvents::notify: unknown " << *event;
      
      El::Service::Error error(ostr.str(), this);
      callback_->notify(&error);
      
      return true;
    }
  }
}
