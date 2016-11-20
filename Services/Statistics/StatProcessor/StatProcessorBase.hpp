/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Statistics/StatProcessor/StatProcessorBase.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_STATISTICS_STATPROCESSOR_STATPRROCESSORBASE_HPP_
#define _NEWSGATE_SERVER_SERVICES_STATISTICS_STATPROCESSOR_STATPRROCESSORBASE_HPP_

#include <string>

#include <ace/OS.h>
#include <ace/Synch.h>
#include <ace/Guard_T.h>

#include <El/Exception.hpp>
#include <El/Service/Service.hpp>

namespace NewsGate
{
  namespace Statistics
  {
    class StatProcessorBase
    {
    public:
      
      EL_EXCEPTION(Exception, El::ExceptionBase);

    public:

      StatProcessorBase(El::Service::Callback* callback)
        throw(Exception, El::Exception);
      
      virtual ~StatProcessorBase() throw() {}      

      size_t packs_count() const throw();
      bool empty() const throw();

      size_t truncate_raw_stat(El::MySQL::Connection* connection,
                               const char* table_name,
                               const ACE_Time_Value& time_threshold,
                               ACE_Time_Value& duration_time)
        throw(El::Exception);
      
    protected:

      struct Aspect
      {
        static const char STATE[];
      };
      
    protected:
      
      typedef ACE_RW_Thread_Mutex Mutex;
      typedef ACE_Write_Guard<Mutex> WriteGuard;
      typedef ACE_Read_Guard<Mutex> ReadGuard;

      typedef ACE_Thread_Mutex DB_Mutex;
      typedef ACE_Guard<DB_Mutex> DB_Guard;      

      mutable Mutex lock_;
      mutable DB_Mutex db_lock_;
      
      El::Service::Callback* callback_;
      size_t packs_count_;
      std::string cache_filename_;
    };

  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Statistics
  {
    inline
    size_t
    StatProcessorBase::packs_count() const throw()
    {
      ReadGuard guard(lock_);
      return packs_count_;
    }

    inline
    bool
    StatProcessorBase::empty() const throw()
    {
      ReadGuard guard(lock_);
      return packs_count_ == 0;
    }
    
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_STATISTICS_STATPROCESSOR_STATPRROCESSORBASE_HPP_
