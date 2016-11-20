/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/FeedManager/FeedStatSinkImpl.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <Python.h>

#include <El/CORBA/Corba.hpp>

#include <iostream>
#include <sstream>
#include <fstream>

#include <El/Exception.hpp>
#include <El/Moment.hpp>

#include "FeedStatSinkImpl.hpp"
#include "FeedManagerMain.hpp"

namespace Aspect
{
  const char FEED_STAT[] = "FeedStat";
}

namespace NewsGate
{
  namespace Statistics
  {
    namespace Aspect
    {
      const char FEED_STAT[] = "FeedStat";
    }
      
    //
    // FeedManagerImpl class
    //
    FeedSinkImpl::FeedSinkImpl(El::Service::Callback* callback)
      throw(InvalidArgument, Exception, El::Exception)
        : El::Service::CompoundService<>(
          callback,
          "FeedSinkImpl")
    {
      El::Service::CompoundServiceMessage_var msg =
        new SaveMessageImpressionClickStat(this);
      
      deliver_now(msg.in());
    }

    FeedSinkImpl::~FeedSinkImpl() throw()
    {
      // Check if state is active, then deactivate and log error
    }

    void
    FeedSinkImpl::save_message_stat(SaveMessageImpressionClickStat* smics)
      throw(El::Exception)
    {
      bool do_cleanup = false;
      MessageImpressionClickMapPtr stat;
      ACE_Time_Value cur_time = ACE_OS::gettimeofday();
      
      {
        WriteGuard guard(srv_lock_);
        
        stat.reset(imp_click_stat_.release());
        do_cleanup = smics && next_stat_cleanup_ <= cur_time;

        if(do_cleanup)
        {
          next_stat_cleanup_ =
            (next_stat_cleanup_ == ACE_Time_Value::zero ? cur_time :
             next_stat_cleanup_) + ACE_Time_Value(86400);
        }
      }

      if(stat.get() || do_cleanup)
      {
        try
        {
          El::MySQL::Connection_var connection =
            Application::instance()->dbase()->connect();

          if(do_cleanup)
          {
            cleanup_stat(cur_time.sec(), connection.in());
          }

          if(stat.get())
          {
            while(!stat->empty())
            {
              save_message_stat(*stat, connection.in());
            }
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
        
          ostr << "FeedSinkImpl::save_message_stat: "
            "El::Exception caught. Description:\n" << e;

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
      }
      
      if(smics)
      {
        deliver_at_time(
          smics,
          ACE_Time_Value(ACE_OS::gettimeofday().sec() +
                         Application::instance()->config().
                         save_stat().period()));
      }
    }   

    void
    FeedSinkImpl::cleanup_stat(time_t cur_time,
                               El::MySQL::Connection* connection)
      throw(El::Exception)
    {
      time_t threshold = (cur_time / 86400 - Application::instance()->config().
                          save_stat().keep_days()) * 86400;

      try
      {
        ACE_High_Res_Timer timer;
        timer.start();

        std::ostringstream ostr;
        ostr << "delete from StatFeed where date < '"
             << El::Moment(ACE_Time_Value(threshold)).iso8601(false) << "'";
        
        El::MySQL::Result_var result =
          connection->query(ostr.str().c_str());
        
        ACE_Time_Value time;
        
        timer.stop();
        timer.elapsed_time(time);
        
        {
          std::ostringstream ostr;
          
          ostr << "FeedSinkImpl::cleanup_stat: deleting "
               << connection->affected_rows() << " rows for "
               << El::Moment::time(time);
          
          Application::logger()->trace(ostr.str().c_str(),
                                       Aspect::FEED_STAT,
                                       El::Logging::HIGH);
        }
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        
        ostr << "FeedSinkImpl::cleanup_stat: "
          "El::Exception caught. Description:\n" << e;

        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
      }
    }
      
    void
    FeedSinkImpl::save_message_stat(
      ::NewsGate::Statistics::Transport::MessageImpressionClickMap& stat,
      El::MySQL::Connection* connection)
      throw(El::Exception)
    {
      std::string filename;

      try
      {
        ACE_High_Res_Timer timer;
        timer.start();

        filename = Application::instance()->config().temp_dir() +
          "/FeedManager.cache.fst." +
          El::Moment(ACE_OS::gettimeofday()).dense_format();
      
        std::fstream file(filename.c_str(), ios::out);
        
        if(!file.is_open())
        {
          std::ostringstream ostr;
          ostr << "FeedSinkImpl::save_message_stat: failed to "
            "open file '" << filename << "' for write access";
          
          throw Exception(ostr.str());
        }      

        size_t max_save_records =
          Application::instance()->config().save_stat().chunk_size();

        size_t save_records = 0;
        bool first_line = true;
        
        for(; !stat.empty() && save_records < max_save_records; ++save_records)
        {
          ::NewsGate::Statistics::Transport::MessageImpressionClickMap::
            iterator it(stat.begin());
          
          const ::NewsGate::Statistics::Transport::DailyNumber& key(it->first);
          
          const ::NewsGate::Statistics::Transport::MessageImpressionClick&
            value(it->second);

          if(first_line)
          {
            first_line = false;
          }
          else
          {
            file << std::endl;
          }
              
          file << El::Moment(ACE_Time_Value(key.date * 86400)).iso8601(false)
               << "\t" << key.value << "\t" << value.impressions << "\t"
               << value.clicks;

          stat.erase(it);
        }
            
        if(file.fail())
        {
          std::ostringstream ostr;
          ostr << "FeedSinkImpl::save_message_stat: "
            "failed to write into file '" << filename << "'";

          throw Exception(ostr.str());
        }
      
        file.close();

        ACE_Time_Value write_time;
        
        timer.stop();
        timer.elapsed_time(write_time);

        timer.start();
        
        El::MySQL::Result_var result =
          connection->query("delete from StatFeedBuff");
        
        {
          std::ostringstream ostr;
        
          ostr << "LOAD DATA INFILE '"
               << connection->escape(filename.c_str())
               << "' INTO TABLE StatFeedBuff character set binary";
          
          result = connection->query(ostr.str().c_str());
        }
        
        unlink(filename.c_str());
        filename.clear();
        
        ACE_Time_Value load_time;
        
        timer.stop();
        timer.elapsed_time(load_time);

        timer.start();

        result = connection->query(
            "INSERT INTO StatFeed SELECT * FROM "
            "StatFeedBuff ON DUPLICATE KEY UPDATE "
            "StatFeed.msg_impressions="
              "StatFeed.msg_impressions+StatFeedBuff.msg_impressions, "
            "StatFeed.msg_clicks=StatFeed.msg_clicks+StatFeedBuff.msg_clicks");
        
        ACE_Time_Value insert_time;
        
        timer.stop();
        timer.elapsed_time(insert_time);

        std::ostringstream ostr;
            
        ostr << "FeedSinkImpl::save_message_stat: saving "
             << save_records << " records for "
             << El::Moment::time(write_time) << " + "
             << El::Moment::time(load_time) << " + "
             << El::Moment::time(insert_time);
          
        Application::logger()->trace(ostr.str().c_str(),
                                     Aspect::FEED_STAT,
                                     El::Logging::HIGH);
      }
      catch(const El::Exception& e)
      {
        if(!filename.empty())
        {
          unlink(filename.c_str());
        }
        
        std::ostringstream ostr;
        
        ostr << "FeedSinkImpl::save_message_stat: "
          "El::Exception caught. Description:\n" << e;

        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
      }
    }

    void
    FeedSinkImpl::process_message_imp_click_stat(
      ::NewsGate::Statistics::Transport::MessageImpressionClickMap* counter)
      throw(El::Exception)
    {
      MessageImpressionClickMapPtr stat(counter);
      
      WriteGuard guard(srv_lock_);

      if(imp_click_stat_.get() == 0)
      {
        imp_click_stat_.reset(stat.release());
        return;
      }

      for(::NewsGate::Statistics::Transport::MessageImpressionClickMap::
            const_iterator i(stat->begin()), e(stat->end()); i != e; ++i)
      {
        const ::NewsGate::Statistics::Transport::DailyNumber&
          key = i->first;

        const ::NewsGate::Statistics::Transport::MessageImpressionClick&
          value = i->second;
        
        ::NewsGate::Statistics::Transport::MessageImpressionClickMap::iterator
          it = imp_click_stat_->find(key);

        if(it == imp_click_stat_->end())
        {
          imp_click_stat_->insert(std::make_pair(key, value));
        }
        else
        {
          ::NewsGate::Statistics::Transport::MessageImpressionClick& val =
            it->second;
          
          val.impressions += value.impressions;
          val.clicks += value.clicks;          
        }
      }
    }
    
    bool
    FeedSinkImpl::notify(El::Service::Event* event)
      throw(El::Exception)
    {
      if(El::Service::CompoundService<>::notify(event))
      {
        return true;
      }
      
      ProcessMessageImpressionClickStat* pmics =
        dynamic_cast<ProcessMessageImpressionClickStat*>(event);
      
      if(pmics != 0)
      {
        process_message_imp_click_stat(pmics->pack->release());
        return true;
      }

      SaveMessageImpressionClickStat* smics = 
        dynamic_cast<SaveMessageImpressionClickStat*>(event);
      
      if(smics != 0)
      {
        save_message_stat(smics);
      }
      
      return false;
    }

    void
    FeedSinkImpl::wait() throw(Exception, El::Exception)
    {
      El::Service::CompoundService<>::wait();
      save_message_stat(0);
    }

    void
    FeedSinkImpl::feed_impression_click(
      ::NewsGate::Statistics::Transport::MessageImpressionClickCounter*
      counter)
      throw(NewsGate::Statistics::ImplementationException,
            CORBA::SystemException)
    {
      try
      {
        ::NewsGate::Statistics::Transport::MessageImpressionClickCounterImpl::
          Type* pack = dynamic_cast< ::NewsGate::Statistics::Transport::
          MessageImpressionClickCounterImpl::Type* >(counter);
        
        if(pack == 0)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Statistics::FeedSinkImpl::"
            "feed_impression_click:dynamic_cast<"
            "::NewsGate::Statistics::Transport::"
            "MessageImpressionClickCounterImpl::Type*> failed";

          NewsGate::Statistics::ImplementationException e;
          e.description = ostr.str().c_str();
          throw e;
        }
        
        El::Service::CompoundServiceMessage_var msg =
          new ProcessMessageImpressionClickStat(this, pack);
          
        deliver_now(msg.in());
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "::NewsGate::Statistics::FeedSinkImpl::feed_impression_click:"
          "El::Exception caught. Description:\n" << e;
        
        NewsGate::Statistics::ImplementationException ex;
        ex.description = ostr.str().c_str();
        throw ex;
      }
    }
  }  
}
