/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Feeds/RSS/PullerManager/PullerManagerImpl.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <unistd.h>
#include <limits.h>

#include <string>
#include <sstream>
#include <fstream>

#include <El/Moment.hpp>

#include <ace/OS.h>

#include <Services/Feeds/RSS/Commons/TransportImpl.hpp>

#include "PullerManagerImpl.hpp"
#include "PullerManagerMain.hpp"
#include "SessionSupport.hpp"
#include "FeedingPullers.hpp"

namespace NewsGate
{
  namespace RSS
  {
    //
    // PullerManagerImpl class
    //
    PullerManagerImpl::PullerManagerImpl(El::Service::Callback* callback)
      throw(InvalidArgument, Exception, El::Exception)
        : El::Service::CompoundService<PullerManagerState>(
            callback,
            "PullerManagerImpl"),
          session_(0),
          feed_stat_num_(0)
    {
      PullerManagerState_var st = new RegisteringPullers(
        ACE_Time_Value(Application::instance()->
                       config().puller_management().registration_timeout()),
        this);

      state(st.in());
    }

    PullerManagerImpl::~PullerManagerImpl() throw()
    {
      // Check if state is active, then deactivate and log error
    }
    
    ::NewsGate::RSS::SessionId
    PullerManagerImpl::puller_login(::NewsGate::RSS::Puller_ptr puller_object)
      throw(NewsGate::RSS::PullerManager::ImplementationException,
            CORBA::SystemException)
    {
      PullerManagerState_var st = state();
      return st->puller_login(puller_object);
    }

    void
    PullerManagerImpl::puller_logout(::NewsGate::RSS::SessionId session)
      throw(NewsGate::RSS::PullerManager::ImplementationException,
            CORBA::SystemException)
    {
      PullerManagerState_var st = state();
      return st->puller_logout(session);
    }

    CORBA::Boolean
    PullerManagerImpl::feed_state(
      ::NewsGate::RSS::SessionId session,
      ::NewsGate::RSS::FeedStateUpdatePack* state_update)
      throw(NewsGate::RSS::PullerManager::Logout,
            NewsGate::RSS::PullerManager::ImplementationException,
            CORBA::SystemException)
    {
      PullerManagerState_var st = state();
      return st->feed_state(session, state_update);
    }

    void
    PullerManagerImpl::ping(::NewsGate::RSS::SessionId session)
      throw(NewsGate::RSS::PullerManager::Logout,
            NewsGate::RSS::PullerManager::ImplementationException,
            CORBA::SystemException)
    {
      PullerManagerState_var st = state();
      return st->ping(session);
    }

    void
    PullerManagerImpl::feed_stat(::NewsGate::RSS::SessionId session,
                                 ::NewsGate::RSS::FeedsStatistics* stat)
      throw(NewsGate::RSS::PullerManager::Logout,
            NewsGate::RSS::PullerManager::ImplementationException,
            CORBA::SystemException)
    {
      std::string fname;

      try
      {
        ::NewsGate::RSS::Transport::FeedsStatisticsImpl::Type* stat_impl =
            dynamic_cast<
            ::NewsGate::RSS::Transport::FeedsStatisticsImpl::Type*>(stat);

        if(stat_impl == 0)
        {
          throw Exception(
            "::NewsGate::RSS::PullerManagerImpl::feed_stat:"
            "dynamic_cast<::NewsGate::RSS::Transport::FeedsStatisticsImpl::"
            "Type*>(stat) failed");
        }

        const ::NewsGate::RSS::Transport::FeedsStatistics& feed_stat =
          stat_impl->entity();
          
        {
          WriteGuard guard(srv_lock_);
          
          std::ostringstream ostr;
          ostr << Application::instance()->config().saving_feed_stat().
            cache_file().c_str() << "." << feed_stat_num_++ << "." << session;

          fname = ostr.str();
        }
        
        std::fstream file(fname.c_str(), ios::out);

        if(!file.is_open())
        {
          std::ostringstream ostr;
          ostr << "PullerManagerImpl::feed_stat: failed to open file '"
               << fname << "' for write access";

          throw Exception(ostr.str());
        }

        std::string date =
          El::Moment(ACE_Time_Value(feed_stat.date)).iso8601(false);
        
        const ::NewsGate::RSS::Transport::FeedStatArray& stat_seq =
          feed_stat.feeds_stat;

        for(::NewsGate::RSS::Transport::FeedStatArray::const_iterator it =
              stat_seq.begin(); it != stat_seq.end(); it++)
        {
          const ::NewsGate::RSS::Transport::FeedStat& stat = *it;
          
          file << date << "\t" << stat.id << "\t" << stat.requests << "\t"
               << stat.failed << "\t" << stat.unchanged << "\t"
               << stat.not_modified << "\t" << stat.presumably_unchanged
               << "\t" << stat.has_changes << "\t" << stat.wasted
               << "\t" << stat.outbound << "\t" << stat.inbound << "\t"
               << stat.requests_duration << "\t" << stat.messages << "\t"
               << stat.messages_size << "\t" << stat.messages_delay << "\t"
               << stat.max_message_delay << "\t" << stat.mistiming
               << std::endl;

          if(file.bad() || file.fail())
          {
            std::ostringstream ostr;
            ostr << "PullerManagerImpl::feed_stat: failed to write to file '"
                 << fname << "'";

            throw Exception(ostr.str());
          }
        }

        file.close();
      }
      catch(const El::Exception& e)
      {
        try
        {
          unlink(fname.c_str());
            
          std::ostringstream ostr;
          ostr << "FeedingPullers::write_feed_stat: El::Exception caught. "
            "Description:" << std::endl << e;

          const char* descr = ostr.str().c_str();

          El::Service::Error error(descr, this);
          callback_->notify(&error);

          NewsGate::RSS::PullerManager::ImplementationException ex;
          ex.description = CORBA::string_dup(descr);
          throw ex;
        }
        catch(const NewsGate::RSS::PullerManager::ImplementationException&)
        {
          throw;
        }
        catch(...)
        {
          El::Service::Error error(
            "PullerManagerImpl::feed_stat: unknown error", this); 

          callback_->notify(&error);
        }
      }
    }
      
    void
    PullerManagerImpl::register_puller(
      ::NewsGate::RSS::SessionId session_id,
      ::NewsGate::RSS::Puller_ptr puller_object)
      throw()
    {
      std::ostringstream ostr;
      ostr << "PullerManagerImpl::register_puller(" << session_id << ")";

      Application::logger()->trace(ostr.str(),
                                   Aspect::STATE,
                                   El::Logging::LOW);

      PullerRecord puller_record;
      
      puller_record.reference =
        ::NewsGate::RSS::Puller::_duplicate(puller_object);

      puller_record.timestamp = ACE_OS::gettimeofday();
      
      WriteGuard guard(srv_lock_);        

      pullers_[session_id] = puller_record;
    }
    
    void
    PullerManagerImpl::unregister_puller(::NewsGate::RSS::SessionId session_id)
      throw()
    {
      std::ostringstream ostr;
      ostr << "PullerManagerImpl::unregister_puller(" << session_id << ")";

      Application::logger()->trace(ostr.str(),
                                   Aspect::STATE,
                                   El::Logging::LOW);

      WriteGuard guard(srv_lock_);
      pullers_.erase(session_id);
    }
    
    void
    PullerManagerImpl::finalize_puller_registration() throw()
    {
      try
      {
        std::ostringstream ostr;
        ostr << "PullerManagerImpl::finalize_puller_registration()";

        Application::logger()->trace(ostr.str(),
                                     Aspect::STATE,
                                     El::Logging::LOW);

        const Server::Config::RSSFeedPullerManagerType& config =
          Application::instance()->config();
        
        {
          WriteGuard guard(srv_lock_);

          ACE_Time_Value cur_time = ACE_OS::gettimeofday();

          for(PullerHashTable::iterator it = pullers_.begin();
              it != pullers_.end(); it++)
          {
            it->second.timestamp = cur_time;
          }          
        }
        
        PullerManagerState_var st = new FeedingPullers(
          ACE_Time_Value(config.puller_management().presence_poll_timeout()),
          Application::instance()->config().fetching_feeds(),
          Application::instance()->config().saving_feed_state(),
          Application::instance()->config().saving_feed_stat(),
          this);
        
        state(st.in());
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "PullerManagerImpl::finalize_puller_registration: "
            "El::Exception caught. Description:" << std::endl << e;

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(...)
        {
        }
      }      
      
    }

    bool
    PullerManagerImpl::check_pullers_presence(const ACE_Time_Value& timeout)
      throw()
    {
      std::ostringstream ostr;
      ostr << "PullerManagerImpl::check_pullers_presence()";

      Application::logger()->trace(ostr.str(),
                                   Aspect::STATE,
                                   El::Logging::LOW);

      ACE_Time_Value cur_time = ACE_OS::gettimeofday();

      ReadGuard guard(srv_lock_);
      
      for(PullerHashTable::iterator it = pullers_.begin();
          it != pullers_.end(); it++)
      {
        if(it->second.timestamp + timeout < cur_time)
        {
          return false;
        }  
      }
      
      return true;
    }

    bool
    PullerManagerImpl::process_feed_state_update(
      ::NewsGate::RSS::SessionId session,
      ::NewsGate::RSS::Transport::FeedStateUpdatePackImpl::Type* state_update)
      throw(El::Exception)
    {
      {
        WriteGuard guard(srv_lock_);
        
        PullerHashTable::iterator it = pullers_.find(session);

        if(it == pullers_.end())
        {
          std::ostringstream ostr;
          ostr << "PullerManagerImpl::process_feed_state_update: can't find "
            "puller for session " << session;

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);

          pullers_mistiming();
          return false;
        }
        
        it->second.timestamp = ACE_OS::gettimeofday();
      }

      if(Application::will_trace(El::Logging::MIDDLE))
      {
        std::ostringstream ostr;
        ostr << "PullerManagerImpl::process_feed_state_update: "
          "feed updates:\n";

        const Transport::FeedStateUpdateArray& state_updates =
          state_update->entities();

        for(Transport::FeedStateUpdateArray::const_iterator it =
              state_updates.begin(); it != state_updates.end(); it++)
        {
          it->dump(ostr);
        }
        
        Application::logger()->trace(ostr.str(),
                                     Aspect::PULLING_FEEDS,
                                     El::Logging::MIDDLE);
      }
      
      return true;
    }

    bool
    PullerManagerImpl::process_feed_stat(
      ::NewsGate::RSS::SessionId session,
      ::NewsGate::RSS::FeedsStatistics* stat)
      throw(El::Exception)
    {
      ::NewsGate::RSS::Transport::FeedsStatisticsImpl::Type* stat_impl =
        dynamic_cast<
        ::NewsGate::RSS::Transport::FeedsStatisticsImpl::Type*>(stat);
      
      if(stat_impl == 0)
      {
        throw Exception(
          "::NewsGate::RSS::PullerManagerImpl::process_feed_stat:"
          "dynamic_cast<::NewsGate::RSS::Transport::FeedsStatisticsImpl::"
          "Type*>(stat) failed");
      }

      const ::NewsGate::RSS::Transport::FeedsStatistics& feed_stat =
        stat_impl->entity();
        
      {
        WriteGuard guard(srv_lock_);
        
        PullerHashTable::iterator it = pullers_.find(session);

        if(it == pullers_.end())
        {
          std::ostringstream ostr;
          ostr << "PullerManagerImpl::process_feed_stat: can't find puller "
            "for session " << session;

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);

          pullers_mistiming();
          return false;
        }
        
        it->second.timestamp = ACE_OS::gettimeofday();
      }

      const ::NewsGate::RSS::Transport::FeedStatArray& stat_seq =
        feed_stat.feeds_stat;
        
      if(Application::will_trace(El::Logging::MIDDLE))
      {
        std::ostringstream ostr;
        ostr << "PullerManagerImpl::process_feed_stat: " <<
          El::Moment(ACE_Time_Value(feed_stat.date)).iso8601() << ", feeds:\n";

        for(::NewsGate::RSS::Transport::FeedStatArray::const_iterator it =
              stat_seq.begin(); it != stat_seq.end(); it++)
        {
          const ::NewsGate::RSS::Transport::FeedStat& stat = *it;
          
          ostr << "  id=" << stat.id << ", requests=" << stat.requests
               << ", failed=" << stat.failed
               << ", unchanged=" << stat.unchanged
               << ", outbound=" << stat.outbound
               << ", inbound=" << stat.inbound
               << ", requests_duration=" << stat.requests_duration
               << ", messages=" << stat.messages
               << ", messages_size=" << stat.messages_size
               << ", messages_delay=" << stat.messages_delay
               << ", max_message_delay=" << stat.max_message_delay
               << ", mistiming=" << stat.mistiming << std::endl;
        }
        
        Application::logger()->trace(ostr.str(),
                                     Aspect::PULLING_FEEDS,
                                     El::Logging::MIDDLE);
      }
      
      return true;
    }

    void
    PullerManagerImpl::process_ping(::NewsGate::RSS::SessionId session)
      throw(El::Exception)
    {
      {
        WriteGuard guard(srv_lock_);
        
        PullerHashTable::iterator it = pullers_.find(session);

        if(it == pullers_.end())
        {
          std::ostringstream ostr;
          ostr << "PullerManagerImpl::process_ping: can't find puller "
            "for session " << session;

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);

          pullers_mistiming();
          return;
        }
        
        it->second.timestamp = ACE_OS::gettimeofday();
      }

      std::ostringstream ostr;
      ostr << "PullerManagerImpl::process_ping()";

      Application::logger()->trace(ostr.str(),
                                   Aspect::STATE,
                                   El::Logging::LOW);
    }
    
    void
    PullerManagerImpl::pullers_mistiming() throw()
    {
      std::ostringstream ostr;
      ostr << "PullerManagerImpl::pullers_mistiming()";

      Application::logger()->trace(ostr.str(),
                                   Aspect::STATE,
                                   El::Logging::LOW);
       
      try
      {
        PullerManagerState_var st = new EndingFeedingSession(
          ACE_Time_Value(Application::instance()->
                         config().puller_management().reset_timeout()),
          this);
        
        state(st.in());
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "PullerManagerImpl::pullers_mistiming: "
            "El::Exception caught. Description:" << std::endl << e;

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(...)
        {
        }
      }      
    }

    void
    PullerManagerImpl::finalize_pullers_session() throw()
    {
      std::ostringstream ostr;
      ostr << "PullerManagerImpl::finalize_pullers_session()";

      Application::logger()->trace(ostr.str(),
                                   Aspect::STATE,
                                   El::Logging::LOW);

      try
      {
        {
          WriteGuard guard(srv_lock_);
          pullers_.clear();
        }
        
        PullerManagerState_var st = new RegisteringPullers(
          ACE_Time_Value(Application::instance()->
                         config().puller_management().registration_timeout()),
          this);
        
        state(st.in());
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "PullerManagerImpl::finalize_pullers_session: "
            "El::Exception caught. Description:" << std::endl << e;

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(...)
        {
        }
      }      
    }
    
    PullerSessionVector
    PullerManagerImpl::pullers() throw()
    {
      PullerSessionVector pullers(0);

      try
      {
        try
        {
          ReadGuard guard(srv_lock_);
          
          for(PullerHashTable::const_iterator it = pullers_.begin();
              it != pullers_.end(); ++it)
          {
            PullerSession ps;
            ps.puller = it->second.reference;
            ps.session = it->first;
            
            pullers.push_back(ps);
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "PullerManagerImpl::pullers: "
            "El::Exception caught. Description:" << std::endl << e;

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }      
        catch(const CORBA::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "PullerManagerImpl::pullers: "
            "CORBA::Exception caught. Description:" << std::endl << e;

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
      }
      catch(...)
      {
        // nothing we can do here
      }

      return pullers;
    }
    
  }
  
}
