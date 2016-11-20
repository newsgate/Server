/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Statistics/StatProcessor/StatProcessorBase.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <string>

#include <El/Exception.hpp>

#include "StatProcessorMain.hpp"
#include "StatProcessorBase.hpp"

namespace NewsGate
{
  namespace Statistics
  {
    const char StatProcessorBase::Aspect::STATE[] = "State";
    
    //
    // StatProcessorBase class
    //
    StatProcessorBase::StatProcessorBase(El::Service::Callback* callback)
      throw(Exception, El::Exception)
        : callback_(callback),
          packs_count_(0),
          cache_filename_(Application::instance()->config().cache_file())
    {
    }
    
    size_t
    StatProcessorBase::truncate_raw_stat(
      El::MySQL::Connection* connection,
      const char* table_name,
      const ACE_Time_Value& time_threshold,
      ACE_Time_Value& duration_time)
      throw(El::Exception)
    {
      size_t records = 0;
  
      try
      {        
        ACE_High_Res_Timer timer;
        timer.start();
      
        std::ostringstream qstr;
        
        qstr << "delete from " << table_name << " where time<'"
             << El::Moment(time_threshold).iso8601(false, true) << "'";
        
        {
          DB_Guard guard(db_lock_);
          El::MySQL::Result_var result = connection->query(qstr.str().c_str());
        }
        
        records = connection->affected_rows();
          
        timer.stop();
        timer.elapsed_time(duration_time);
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        
        ostr << "StatProcessorBase::truncate_raw_stat: for " << table_name
             << " El::Exception caught. Description:\n" << e;

        El::Service::Error error(ostr.str(), 0);
        callback_->notify(&error);        
      }

      return records;          
    }

  }
}
