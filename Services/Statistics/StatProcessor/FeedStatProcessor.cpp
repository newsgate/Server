/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Statistics/StatProcessor/FeedStatProcessor.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <string>
#include <sstream>
#include <fstream>

#include <El/Exception.hpp>
#include <El/Moment.hpp>
#include <El/MySQL/DB.hpp>

#include <ace/OS.h>
#include <ace/High_Res_Timer.h>

#include "StatProcessorMain.hpp"
#include "FeedStatProcessor.hpp"

namespace NewsGate
{
  namespace Statistics
  {    
    //
    // FeedStatProcessor class
    //
    FeedStatProcessor::FeedStatProcessor(El::Service::Callback* callback)
      throw(Exception, El::Exception)
        : StatProcessorBase(callback)
    {
      if(Application::instance()->config().push_stat().present())
      {
        const Server::Config::StatProcessorType::push_stat_type&
          push_stat_conf = *Application::instance()->config().push_stat();

        const std::string& feed_stat_sink_ref =
          push_stat_conf.feed_stat_sink_ref();

        if(!feed_stat_sink_ref.empty())
        {
          feed_stat_sink_ = FeedStatSinkRef(feed_stat_sink_ref.c_str(),
                                            Application::instance()->orb());
        }
      }
    }

    void
    FeedStatProcessor::save_impressions(
      const MsgImpStatProcessor::MessageImpressionInfoPackList& packs)
      throw(El::Exception)
    {
      try
      {
        if(feed_stat_sink_.empty())
        {
          return;
        }
      
        ACE_High_Res_Timer timer;
        timer.start();

        size_t save_records = 0;
        size_t info_packs_count = 0;

        {
          WriteGuard guard(lock_);

          if(feed_stat_.get() == 0)
          {
            feed_stat_.reset(new Transport::MessageImpressionClickMap());
          }
        
          for(MsgImpStatProcessor::MessageImpressionInfoPackList::const_iterator
                pi(packs.begin()), pe(packs.end()); pi != pe;
              ++pi, ++info_packs_count)
          {
            const MessageImpressionInfoArray& impression_infos =
              (*pi)->entities();
          
            for(MessageImpressionInfoArray::const_iterator
                  i(impression_infos.begin()), e(impression_infos.end());
                i != e; ++i)
            {
              const MessageImpressionInfo& ii = *i;
          
              Transport::DailyNumber stat_key;
              stat_key.date = ii.time.sec / 86400;
          
              const Message::Transport::MessageStatInfoArray& mi = ii.messages;
              save_records += mi.size();

              Message::Transport::MessageStatInfoArray::const_iterator mit_end =
                mi.end();
            
              for(Message::Transport::MessageStatInfoArray::const_iterator
                    mit = mi.begin(); mit != mit_end; ++mit)
              {
                const Message::Transport::MessageStatInfo& stat = *mit;
                stat_key.value = stat.source_id;
            
                Transport::MessageImpressionClickMap::iterator fit =
                  feed_stat_->find(stat_key);

                if(fit == feed_stat_->end())
                {
                  fit = feed_stat_->insert(
                    std::make_pair(stat_key,
                                   Transport::MessageImpressionClick())).first;
                }

                fit->second.impressions += stat.count;
              }
            }
          }
        }
        
        ACE_Time_Value time;
        
        timer.stop();
        timer.elapsed_time(time);

        std::ostringstream ostr;
        
        ostr << "FeedStatProcessor::save_impressions: saving "
             << save_records << " records in "
             << info_packs_count << " packs for "
             << El::Moment::time(time);
        
        Application::logger()->trace(ostr.str().c_str(),
                                     Aspect::STATE,
                                     El::Logging::HIGH);
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "FeedStatProcessor::save: El::Exception caught. Description:\n"
             << e;

        El::Service::Error error(ostr.str(), 0);
        callback_->notify(&error);      
      }      
    }
    
    void
    FeedStatProcessor::save_clicks(
      const MsgClickStatProcessor::MessageClickInfoPackList& packs)
      throw(El::Exception)
    {
      if(feed_stat_sink_.empty())
      {
        return;
      }
      
      ACE_High_Res_Timer timer;
      timer.start();

      size_t save_records = 0;
      size_t info_packs_count = 0;
      
      {
        WriteGuard guard(lock_);

        if(feed_stat_.get() == 0)
        {
          feed_stat_.reset(new Transport::MessageImpressionClickMap());
        }

        for(MsgClickStatProcessor::MessageClickInfoPackList::const_iterator
              pi(packs.begin()), pe(packs.end()); pi != pe;
            ++pi, ++info_packs_count)
        {
          const MessageClickInfoArray& click_infos = (*pi)->entities();
          
          for(MessageClickInfoArray::const_iterator i(click_infos.begin()),
                e(click_infos.end()); i != e; ++i)
          {
            const MessageClickInfo& ci = *i;

            Transport::DailyNumber stat_key;
            stat_key.date = ci.time.sec / 86400;

            const Message::Transport::MessageStatInfoArray& mi = ci.messages;
            save_records += mi.size();

            Message::Transport::MessageStatInfoArray::const_iterator mit_end =
              mi.end();
            
            for(Message::Transport::MessageStatInfoArray::const_iterator
                  mit = mi.begin(); mit != mit_end; ++mit)
            {
              const Message::Transport::MessageStatInfo& stat = *mit;
              stat_key.value = stat.source_id;

              Transport::MessageImpressionClickMap::iterator fit =
                feed_stat_->find(stat_key);

              if(fit == feed_stat_->end())
              {
                fit = feed_stat_->insert(
                  std::make_pair(stat_key,
                                 Transport::MessageImpressionClick())).first;
              }

              fit->second.clicks += stat.count;
            }
          }
        }
      }
      
      ACE_Time_Value time;
        
      timer.stop();
      timer.elapsed_time(time);

      std::ostringstream ostr;
        
      ostr << "FeedStatProcessor::save_clicks: saving "
           << save_records << " records in "
           << info_packs_count << " packs for "
           << El::Moment::time(time);
      
      Application::logger()->trace(ostr.str().c_str(),
                                   Aspect::STATE,
                                   El::Logging::HIGH);
    }

    void
    FeedStatProcessor::push() throw(El::Exception)
    {
      if(feed_stat_sink_.empty())
      {
        return;
      }

      std::auto_ptr<Transport::MessageImpressionClickMap> feed_stat;

      {  
        WriteGuard guard(lock_);
        feed_stat.reset(feed_stat_.release());
      }

      if(feed_stat.get() == 0)
      {
        return;
      }

      try
      {
        ACE_High_Res_Timer timer;
        timer.start();

/*
        std::cerr << "ImpClick:\n";

        for(Transport::ImpressionClickMap::const_iterator it =
              feed_stat->begin(); it != feed_stat->end(); it++)
        {
          std::cerr << it->first.date << "/" << it->first.value << " : "
                    << it->second.impressions << "/" << it->second.clicks
                    << std::endl;
        }
*/        
        size_t records = feed_stat->size();
        FeedSink_var feed_stat_sink = feed_stat_sink_.object();

        Transport::MessageImpressionClickCounterImpl::Var counter =
          new Transport::MessageImpressionClickCounterImpl::Type(
            feed_stat.release());
            
        feed_stat_sink->feed_impression_click(counter.in());

        ACE_Time_Value time;
        
        timer.stop();
        timer.elapsed_time(time);
        
        std::ostringstream ostr;
        
        ostr << "FeedStatProcessor::push: pushing "
             << records << " records for "
             << El::Moment::time(time);
        
        Application::logger()->trace(ostr.str().c_str(),
                                     Aspect::STATE,
                                     El::Logging::HIGH);
      }
      catch(const ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FeedStatProcessor::push: "
          "::NewsGate::Statistics::ImplementationException caught. "
          "Description:\n" << e.description.in();
        
        El::Service::Error error(ostr.str().c_str(), 0);
        callback_->notify(&error);
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::FeedStatProcessor::push: "
          "CORBA::Exception caught. Description:\n" << e;
          
        El::Service::Error error(ostr.str().c_str(), 0);
        callback_->notify(&error);
      }
      
    }

  }
}
