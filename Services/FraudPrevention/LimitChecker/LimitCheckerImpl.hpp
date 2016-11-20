/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/FraudPrevention/LimitChecker/LimitCheckerImpl.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_FRAUDPREVENTION_LIMITCHECKER_LIMITCHECKERIMPL_HPP_
#define _NEWSGATE_SERVER_SERVICES_FRAUDPREVENTION_LIMITCHECKER_LIMITCHECKERIMPL_HPP_

#include <google/sparse_hash_map>

#include <ace/OS.h>
#include <ace/Synch.h>
#include <ace/Guard_T.h>

#include <El/Exception.hpp>
#include <El/Hash/Hash.hpp>
#include <El/Service/CompoundService.hpp>

#include <Services/Commons/FraudPrevention/TransportImpl.hpp>
#include <Services/Commons/FraudPrevention/FraudPreventionServices_s.hpp>

namespace NewsGate
{
  namespace FraudPrevention
  {
    class LimitCheckerImpl :
      public virtual POA_NewsGate::FraudPrevention::LimitChecker,
      public virtual El::Service::CompoundService<> 
    {
    public:
      EL_EXCEPTION(Exception, El::ExceptionBase);

    public:

      LimitCheckerImpl(El::Service::Callback* callback)
        throw(Exception, El::Exception);

      virtual ~LimitCheckerImpl() throw();

      virtual void wait() throw(Exception, El::Exception);
      virtual bool notify(El::Service::Event* event) throw(El::Exception);
      
    private:

      //
      // IDL:NewsGate/FraudPrevention/LimitChecker/check:1.0
      //
      virtual
      ::NewsGate::FraudPrevention::Transport::EventLimitCheckResultPack* check(
        ::CORBA::ULong interface_version,
        ::NewsGate::FraudPrevention::Transport::EventLimitCheckPack* pack)
        throw(::NewsGate::FraudPrevention::IncompatibleVersion,
              ::NewsGate::FraudPrevention::ImplementationException,
              CORBA::SystemException);

    private:

      struct TraverseEvents : public El::Service::CompoundServiceMessage
      {
        EventFreq last_key;
        
        TraverseEvents(LimitCheckerImpl* service) throw(El::Exception);
        ~TraverseEvents() throw() {}
      };

      void traverse_events(TraverseEvents* te) throw(El::Exception);
      
    private:

      typedef ACE_Thread_Mutex Mutex;
      typedef ACE_Guard<Mutex> Guard;
      
      Mutex lock_;
      
      struct EventIntervalCounter
      {
        uint32_t from;
        uint64_t count;

        EventIntervalCounter() : from(0), count(0) {}
        EventIntervalCounter(uint32_t f, uint64_t c) throw();
        
        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);
      };
      
      class EventIntervalCountMap :
        public google::sparse_hash_map<EventFreq,
                                       EventIntervalCounter,
                                       EventFreqHash>
      {
      public:
        EventIntervalCountMap() throw(El::Exception);
      };      

      uint64_t start_time_;
      EventIntervalCountMap event_intervals_;
      ACE_Time_Value check_time_;
      size_t checks_count_;
      size_t exceeds_count_;
      size_t check_reqs_count_;
      bool dumped_;
    };

    typedef El::RefCount::SmartPtr<LimitCheckerImpl> LimitCheckerImpl_var;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace FraudPrevention
  {    
    //
    // StatProcessorImpl::ProcessMessageImpressionStat class
    //
    inline
    LimitCheckerImpl::TraverseEvents::TraverseEvents(LimitCheckerImpl* service)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(service, service, false),
          El::Service::CompoundServiceMessage(service, service)
    {
    }
    
    //
    // EventIntervalCounter struct
    //
    inline
    LimitCheckerImpl::EventIntervalCounter::EventIntervalCounter(uint32_t f,
                                                                 uint64_t c)
      throw()
        : from(f),
          count(c)
    {
    }

    inline
    void
    LimitCheckerImpl::EventIntervalCounter::write(El::BinaryOutStream& bstr)
      const throw(El::Exception)
    {
      bstr << from << count;
    }
    
    inline
    void
    LimitCheckerImpl::EventIntervalCounter::read(El::BinaryInStream& bstr)
      throw(El::Exception)
    {
      bstr >> from >> count;
    }
    
    //
    // EventIntervalCountMap class
    //
    inline
    LimitCheckerImpl::EventIntervalCountMap::EventIntervalCountMap()
      throw(El::Exception)
    {
      set_deleted_key(EventFreq::null);
    }
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_FRAUDPREVENTION_LIMITCHECKER_LIMITCHECKERIMPL_HPP_
