/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/FraudPrevention/LimitChecker/LimitCheckerImpl.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <string>
#include <sstream>
#include <fstream>

#include <El/Exception.hpp>
#include <El/Moment.hpp>

#include <ace/OS.h>
#include <ace/High_Res_Timer.h>

#include "LimitCheckerImpl.hpp"
#include "LimitCheckerMain.hpp"

namespace NewsGate
{
  namespace FraudPrevention
  {
    namespace Aspect
    {
      const char STATE[] = "State";
    }
    
    //
    // LimitCheckerImpl class
    //
    LimitCheckerImpl::LimitCheckerImpl(El::Service::Callback* callback)
      throw(Exception, El::Exception)
        : El::Service::CompoundService<>(callback, "LimitCheckerImpl", 1),
          start_time_(ACE_OS::gettimeofday().sec()),
          checks_count_(0),
          exceeds_count_(0),
          check_reqs_count_(0),
          dumped_(false)
    {
      if(Application::will_trace(El::Logging::LOW))
      {
        Application::logger()->info(
          "NewsGate::FraudPrevention::LimitCheckerImpl::LimitCheckerImpl: "
          "starting",
          Aspect::STATE);
      }

      std::string filename =
        Application::instance()->config().cache_file();

      std::fstream file(filename.c_str(), ios::in);
        
      if(file.is_open())
      {
        try
        {
          EventIntervalCountMap event_intervals;
          
          El::BinaryInStream bstr(file);

          uint32_t min_from = UINT32_MAX;
          uint64_t start_time;
          bstr >> start_time;

          while(true)
          {
            EventFreq ef;
            EventIntervalCounter ei;

            try
            {
              bstr >> ef;
            }
            catch(...)
            {
              break;
            }

            bstr >> ei;

            if(start_time + ei.from + ef.interval > start_time_)
            {
              min_from = std::min(min_from, ei.from);
              event_intervals.insert(std::make_pair(ef, ei));
            }
          }

          for(EventIntervalCountMap::iterator i(event_intervals.begin()),
                e(event_intervals.end()); i != e; ++i)
          {
            i->second.from -= min_from;
          }

          start_time_ = start_time + min_from;

          event_intervals.swap(event_intervals_);
        }
        catch(...)
        {
        }

        file.close();
        unlink(filename.c_str());
      }
      

//      ACE_OS::sleep(20);
      
      El::Service::CompoundServiceMessage_var msg = new TraverseEvents(this);
  
      deliver_now(msg.in());
    }

    LimitCheckerImpl::~LimitCheckerImpl() throw()
    {
      // Check if state is active, then deactivate and log error
    }

    ::NewsGate::FraudPrevention::Transport::EventLimitCheckResultPack*
    LimitCheckerImpl::check(
      ::CORBA::ULong interface_version,
      ::NewsGate::FraudPrevention::Transport::EventLimitCheckPack* pack)
      throw(::NewsGate::FraudPrevention::IncompatibleVersion,
            ::NewsGate::FraudPrevention::ImplementationException,
            CORBA::SystemException)
    {
      if(interface_version !=
         ::NewsGate::FraudPrevention::LimitChecker::INTERFACE_VERSION)
      {
        IncompatibleVersion ex;
        
        ex.current_version =
          ::NewsGate::FraudPrevention::LimitChecker::INTERFACE_VERSION;
        
        throw ex;
      }

      FraudPrevention::Transport::EventLimitCheckResultPackImpl::Var
        result_pack =
        FraudPrevention::Transport::EventLimitCheckResultPackImpl::Init::
        create(new FraudPrevention::EventLimitCheckResultArray());
      
      try
      {
        FraudPrevention::Transport::EventLimitCheckPackImpl::Type* pack_impl =
          dynamic_cast<
          FraudPrevention::Transport::EventLimitCheckPackImpl::Type*>(pack);
        
        if(pack_impl == 0)
        {
          throw Exception("NewsGate::FraudPrevention::LimitCheckerImpl::check:"
                          " dynamic_cast<FraudPrevention::Transport::"
                          "EventLimitCheckPackImpl::Type*> failed");
        }

        const FraudPrevention::EventLimitCheckArray& checks =
          pack_impl->entities();

        FraudPrevention::EventLimitCheckResultArray& results =
          result_pack->entities();

        results.reserve(checks.size());

        ACE_High_Res_Timer timer;
        
        uint32_t now = ACE_OS::gettimeofday().sec() - start_time_;
        
        {
          Guard guard(lock_);

          timer.start();
          
          for(FraudPrevention::EventLimitCheckArray::const_iterator
                i(checks.begin()), e(checks.end()); i != e; ++i)
          {
            const EventLimitCheck& ch = *i;
            const EventFreq& ef = ch.event_freq;
            
            EventIntervalCountMap::iterator eit =
              event_intervals_.insert(
                std::make_pair(ef, EventIntervalCounter(now, 0))).first;

            EventIntervalCounter& eic = eit->second;

            if(now - eic.from < ef.interval)
            {
              eic.count += ch.count;
            }
            else
            {
              eic.from = now;
              eic.count = ch.count;
            }

            bool exceed = eic.count > ef.times;
/*              
            std::cerr << ef.event << " " << ef.times << "/" << ef.interval
                      << " -> " << eic.from << " " << eic.count
                      << " : " << exceed << std::endl;
*/
            if(exceed)
            {
              ++exceeds_count_;
            }
            
            results.push_back(FraudPrevention::EventLimitCheckResult(exceed));
          }
        }
        
        ACE_Time_Value time;
        
        timer.stop();
        timer.elapsed_time(time);

        check_time_ += time;
        checks_count_ += checks.size();
        ++check_reqs_count_;

//        std::cerr << std::endl;
        
        return result_pack._retn();
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FraudPrevention::LimitCheckerImpl::"
          "check: El::Exception caught. Description:\n"
             << e.what();
        
        ImplementationException e;
        e.description = CORBA::string_dup(ostr.str().c_str());
        
        throw e;
      }
    }
    
    void
    LimitCheckerImpl::traverse_events(TraverseEvents* te) throw(El::Exception)
    {
      const Server::Config::LimitCheckerType& config =
        Application::instance()->config();

      size_t traverse_records = config.traverse_records();
      EventFreq& last_key = te->last_key;

      uint32_t now = ACE_OS::gettimeofday().sec() - start_time_;
      size_t deleted_records = 0;
      size_t total_records = 0;
      size_t traversed_records = 0;
      
      ACE_High_Res_Timer timer;
      
      {
        Guard guard(lock_);

        timer.start();
        
        EventIntervalCountMap::iterator i;

        if(last_key == EventFreq::null)
        {
          i = event_intervals_.begin();
        }
        else
        {
          i = event_intervals_.find(last_key);
          assert(i != event_intervals_.end());
        }

        EventIntervalCountMap::iterator e(event_intervals_.end());

        if(i == e)
        {
          i = event_intervals_.begin();
        }
        
        for(; i != e && traverse_records--; ++i, ++traversed_records)
        {
          if(now - i->second.from > i->first.interval)
          {
            event_intervals_.erase(i);
            e = event_intervals_.end();
            ++deleted_records;
          }
        }

        last_key = i == e ? EventFreq::null : i->first;
        total_records = event_intervals_.size();
      }

      ACE_Time_Value time;
        
      timer.stop();
      timer.elapsed_time(time);

      bool end_reached = last_key == EventFreq::null;
      
      std::ostringstream ostr;
      
      ostr << "LimitCheckerImpl::traverse_events: "
           << total_records << " events, "
           << traversed_records << " traversed, "
           << deleted_records << " deleted, end"
           << (end_reached ? "" : " not")
           << " reached, tm " << El::Moment::time(time);

      if(end_reached)
      {
        ostr << "\n  " << checks_count_ << " checks, " << exceeds_count_
             << " exceeds";

        if(checks_count_)
        {
          uint64_t usec = ((uint64_t)check_time_.sec() * 1000000 +
                           check_time_.usec()) / checks_count_;

          ostr << "; avg tm "
               << El::Moment::time(ACE_Time_Value(usec / 1000000,
                                                  usec % 1000000));
        }
        
        ostr << "\n  " << check_reqs_count_ << " check reqs made";

        if(check_reqs_count_)
        {
          uint64_t usec = ((uint64_t)check_time_.sec() * 1000000 +
                           check_time_.usec()) / check_reqs_count_;

          ostr << "; avg tm "
               << El::Moment::time(ACE_Time_Value(usec / 1000000,
                                                  usec % 1000000));
        }
        
        check_time_ = ACE_Time_Value::zero;
        checks_count_ = 0;
        exceeds_count_ = 0;
        check_reqs_count_ = 0;
      }

      Application::logger()->trace(ostr.str().c_str(),
                                   Aspect::STATE,
                                   El::Logging::HIGH);
      
      deliver_at_time(te,
                      ACE_Time_Value(ACE_OS::gettimeofday().sec() +
                                     config.traverse_period()));
    }
    
    bool
    LimitCheckerImpl::notify(El::Service::Event* event) throw(El::Exception)
    {
      if(El::Service::CompoundService<>::notify(event))
      {
        return true;
      }
      
      TraverseEvents* te = dynamic_cast<TraverseEvents*>(event);
      
      if(te != 0)
      {
        traverse_events(te);
        return true;
      }
  
      return false;
    }

    void
    LimitCheckerImpl::wait() throw(Exception, El::Exception)
    {
      El::Service::CompoundService<>::wait();

      Guard guard(lock_);

      if(dumped_)
      {
        return;
      }
      
      dumped_ = true;

      std::string filename =
        Application::instance()->config().cache_file();
      
      std::fstream file(filename.c_str(), ios::out);
        
      if(!file.is_open())
      {
        std::ostringstream ostr;
        ostr << "LimitCheckerImpl::wait: "
          "failed to open file '" << filename << "' for write access";
        
        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
        
        return;
      }
      
      El::BinaryOutStream bstr(file);
      bstr << start_time_;
      
      for(EventIntervalCountMap::const_iterator i(event_intervals_.begin()),
            e(event_intervals_.end()); i != e; ++i)
      {
        bstr << i->first << i->second;
      }
    }
  }  
}
