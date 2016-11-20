/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Statistics/StatProcessor/StatProcessorImpl.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <string>
#include <sstream>
#include <fstream>

#include <El/Exception.hpp>
#include <El/Moment.hpp>
#include <El/Stat.hpp>
#include <El/MySQL/DB.hpp>

#include <ace/OS.h>
#include <ace/High_Res_Timer.h>

#include "StatProcessorImpl.hpp"
#include "StatProcessorMain.hpp"

namespace NewsGate
{
  namespace Statistics
  {
    namespace Aspect
    {
      const char STATE[] = "State";
    }
    
    //
    // StatProcessorImpl class
    //
    StatProcessorImpl::StatProcessorImpl(El::Service::Callback* callback)
      throw(Exception, El::Exception)
        : El::Service::CompoundService<>(
            callback,
            "StatProcessorImpl",
            Application::instance()->config().threads()),
          raw_stat_keep_days_(
            Application::instance()->config().raw_stat_keep_days()),
          save_records_at_once_(
            Application::instance()->config().save_records_at_once()),
          request_processor_(callback),
          page_imp_processor_(callback),
          msg_imp_processor_(callback),
          msg_click_processor_(callback),
          feed_stat_processor_(callback),
          user_stat_processor_(callback)
    {
      if(Application::will_trace(El::Logging::LOW))
      {
        Application::logger()->info(
          "NewsGate::Statistics::StatProcessorImpl::StatProcessorImpl: "
          "starting",
          Aspect::STATE);
      }

      ACE_Time_Value now = ACE_OS::gettimeofday();

      El::Service::CompoundServiceMessage_var msg =
        new DispatchStatProcessing(this);
      
      deliver_now(msg.in());
      
      msg = new Monitoring(this);
      deliver_now(msg.in());

      if(Application::instance()->config().push_stat().present())
      {
        msg = new PushStat(this);
        deliver_now(msg.in());
      }
      
      msg = new TruncateRawStat(this);
        
      deliver_at_time(msg.in(),
                      ACE_Time_Value((now.sec() / 86400 + 1) * 86400 + 1));
    }

    StatProcessorImpl::~StatProcessorImpl() throw()
    {
      // Check if state is active, then deactivate and log error
    }

    char*
    StatProcessorImpl::message_bank_manager()
      throw(NewsGate::Statistics::ImplementationException,
            CORBA::SystemException)
    {
      try
      {
        return CORBA::string_dup(
          Application::instance()->config().bank_manager().c_str());
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Statistics::StatProcessorImpl::"
          "message_bank_manager: El::Exception caught. Description:\n"
             << e.what();
        
        ImplementationException e;
        e.description = CORBA::string_dup(ostr.str().c_str());
        
        throw e;
      }
    }
    
    void
    StatProcessorImpl::search_request(
      ::CORBA::ULong interface_version,
      ::NewsGate::Statistics::Transport::RequestInfoPack* pack)
      throw(NewsGate::Statistics::ImplementationException,
            CORBA::SystemException)
    {
      if(interface_version !=
         ::NewsGate::Statistics::StatProcessor::INTERFACE_VERSION)
      {
        return;
      }
      
      try
      {
        request_processor_.enqueue(pack);
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Statistics::StatProcessorImpl::search_request: "
          "El::Exception caught. Description:\n" << e.what();
        
        ImplementationException e;
        e.description = CORBA::string_dup(ostr.str().c_str());
        
        throw e;
      }
    }

    void
    StatProcessorImpl::page_impression(
      ::CORBA::ULong interface_version,
      ::NewsGate::Statistics::Transport::PageImpressionInfoPack* pack)
      throw(NewsGate::Statistics::ImplementationException,
            CORBA::SystemException)
    {
      if(interface_version !=
         ::NewsGate::Statistics::StatProcessor::INTERFACE_VERSION)
      {
        return;
      }

      try
      {
        page_imp_processor_.enqueue(pack);
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Statistics::StatProcessorImpl::page_impression: "
          "El::Exception caught. Description:\n" << e.what();
        
        ImplementationException e;
        e.description = CORBA::string_dup(ostr.str().c_str());
        
        throw e;
      }
    }

    void
    StatProcessorImpl::message_impression(
      ::CORBA::ULong interface_version,
      ::NewsGate::Statistics::Transport::MessageImpressionInfoPack* pack)
      throw(NewsGate::Statistics::ImplementationException,
            CORBA::SystemException)
    {
      if(interface_version !=
         ::NewsGate::Statistics::StatProcessor::INTERFACE_VERSION)
      {
        return;
      }

      try
      {
        msg_imp_processor_.enqueue(pack);
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Statistics::StatProcessorImpl::message_impression: "
          "El::Exception caught. Description:\n" << e.what();
        
        ImplementationException e;
        e.description = CORBA::string_dup(ostr.str().c_str());
        
        throw e;
      }
    }

    void
    StatProcessorImpl::message_click(
      ::CORBA::ULong interface_version,
      ::NewsGate::Statistics::Transport::MessageClickInfoPack* pack)
      throw(NewsGate::Statistics::ImplementationException,
            CORBA::SystemException)
    {
      if(interface_version !=
         ::NewsGate::Statistics::StatProcessor::INTERFACE_VERSION)
      {
        return;
      }

      try
      {
        msg_click_processor_.enqueue(pack);
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Statistics::StatProcessorImpl::message_click: "
          "El::Exception caught. Description:\n" << e.what();
        
        ImplementationException e;
        e.description = CORBA::string_dup(ostr.str().c_str());
        
        throw e;
      }
    }
    
    void
    StatProcessorImpl::process_request_stat(
      const RequestStatProcessor::RequestInfoPackList& packs)
      throw(El::Exception)
    {
      if(raw_stat_keep_days_)
      {
        request_processor_.save(packs);
      }

      user_stat_processor_.save(packs);
    }

    void
    StatProcessorImpl::process_page_impression_stat(
      const PageImpStatProcessor::PageImpressionInfoPackList& packs)
      throw(El::Exception)
    {
      if(raw_stat_keep_days_)
      {
        page_imp_processor_.save(packs);
      }
    }

    void
    StatProcessorImpl::process_message_impression_stat(
      const MsgImpStatProcessor::MessageImpressionInfoPackList& packs)
      throw(El::Exception)
    {
      if(raw_stat_keep_days_)
      {
        msg_imp_processor_.save(packs);
      }

      feed_stat_processor_.save_impressions(packs);
    }

    void
    StatProcessorImpl::process_message_click_stat(
      const MsgClickStatProcessor::MessageClickInfoPackList& packs)
      throw(El::Exception)
    {
      if(raw_stat_keep_days_)
      {
        msg_click_processor_.save(packs);
      }
      
      feed_stat_processor_.save_clicks(packs);
    }

    void
    StatProcessorImpl::truncate_raw_stat() throw(El::Exception)
    {
      try
      {
        ACE_Time_Value tm((ACE_OS::gettimeofday().sec() / 86400 -
                           raw_stat_keep_days_) * 86400);        
        
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();

        ACE_Time_Value trunc_req_time;
        
        unsigned long trunc_req_records =
          request_processor_.truncate_raw_stat(connection.in(),
                                               "StatSearchRequest",
                                               tm,
                                               trunc_req_time);
        
        
        ACE_Time_Value trunc_page_imp_time;
        unsigned long trunc_page_imp_records =
          page_imp_processor_.truncate_raw_stat(connection.in(),
                                                "StatPageImpression",
                                                tm,
                                                trunc_page_imp_time);
        
        ACE_Time_Value trunc_msg_imp_time;
        unsigned long trunc_msg_imp_records =
          msg_imp_processor_.truncate_raw_stat(connection.in(),
                                               "StatMessageImpression",
                                               tm,
                                               trunc_msg_imp_time);
        
        ACE_Time_Value trunc_msg_click_time;
        unsigned long trunc_msg_click_records =
          msg_click_processor_.truncate_raw_stat(connection.in(),
                                                 "StatMessageClick",
                                                 tm,
                                                 trunc_msg_click_time);
        
        std::ostringstream ostr;
        ostr << "StatProcessorImpl::truncate_raw_stat:\n"
             << trunc_req_records << " request records removed for "
             << El::Moment::time(trunc_req_time) << std::endl
             << trunc_page_imp_records
             << " page impression records removed for "
             << El::Moment::time(trunc_page_imp_time) << std::endl
             << trunc_msg_imp_records
             << " message impression records removed for "
             << El::Moment::time(trunc_msg_imp_time) << std::endl
             << trunc_msg_click_records
             << " message click records removed for "
             << El::Moment::time(trunc_msg_click_time);
        
        Application::logger()->trace(ostr.str().c_str(),
                                     Aspect::STATE,
                                     El::Logging::HIGH);
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        
        ostr << "StatProcessorImpl::truncate_raw_stat: "
          "El::Exception caught. Description:\n" << e;

        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
      }
        
      El::Service::CompoundServiceMessage_var msg =
        new TruncateRawStat(this);

      ACE_Time_Value tm(
        (ACE_OS::gettimeofday().sec() / 86400 + 1) * 86400 + 1);

      deliver_at_time(msg.in(), tm);
    }

    void
    StatProcessorImpl::monitor() throw(El::Exception)
    {
      std::ostringstream ostr;
        
      ostr << "StatProcessorImpl::monitor: queue sizes: "
           << request_processor_.packs_count() << "/"
           << page_imp_processor_.packs_count() << "/"
           << msg_imp_processor_.packs_count() << "/"
           << msg_click_processor_.packs_count();
        
      Application::logger()->trace(ostr.str().c_str(),
                                   Aspect::STATE,
                                   El::Logging::HIGH);

      El::Service::CompoundServiceMessage_var msg = new Monitoring(this);
      deliver_at_time(msg.in(), ACE_OS::gettimeofday() + ACE_Time_Value(60));
    }

    void
    StatProcessorImpl::dispatch_request_processing() throw(El::Exception)
    {
      RequestStatProcessor::RequestInfoPackList info_packs;      

      if(request_processor_.get_packs(info_packs, save_records_at_once_))
      {        
        El::Service::CompoundServiceMessage_var msg =
          new ProcessRequestStat(this, info_packs);
          
        deliver_now(msg.in());
      }
    }
    
    void
    StatProcessorImpl::dispatch_page_impression_processing()
      throw(El::Exception)
    {
      PageImpStatProcessor::PageImpressionInfoPackList info_packs;

      if(page_imp_processor_.get_packs(info_packs, save_records_at_once_))
      {        
        El::Service::CompoundServiceMessage_var msg =
          new ProcessPageImpressionStat(this, info_packs);
          
        deliver_now(msg.in());
      }
    }
    
    void
    StatProcessorImpl::dispatch_message_impression_processing()
      throw(El::Exception)
    {
      MsgImpStatProcessor::MessageImpressionInfoPackList info_packs;

      if(msg_imp_processor_.get_packs(info_packs, save_records_at_once_))
      {
        El::Service::CompoundServiceMessage_var msg =
          new ProcessMessageImpressionStat(this, info_packs);
          
        deliver_now(msg.in());
      }
    }

    void
    StatProcessorImpl::dispatch_message_click_processing() throw(El::Exception)
    {
      MsgClickStatProcessor::MessageClickInfoPackList info_packs;

      if(msg_click_processor_.get_packs(info_packs, save_records_at_once_))
      {
        El::Service::CompoundServiceMessage_var msg =
          new ProcessMessageClickStat(this, info_packs);
          
        deliver_now(msg.in());
      }
    }
    void
    StatProcessorImpl::dispatch_stat_processing() throw(El::Exception)
    {
      dispatch_request_processing();
      dispatch_page_impression_processing();
      dispatch_message_impression_processing();
      dispatch_message_click_processing();
      
      El::Service::CompoundServiceMessage_var msg =
        new DispatchStatProcessing(this);

      if(request_processor_.empty() &&
         page_imp_processor_.empty() &&
         msg_imp_processor_.empty() &&
         msg_click_processor_.empty())
      {        
        deliver_at_time(msg.in(),
                        ACE_Time_Value(ACE_OS::gettimeofday().sec() + 1));
      }
      else
      {
        deliver_now(msg.in());
      }
    }

    void
    StatProcessorImpl::push_stat() throw(El::Exception)
    {
      feed_stat_processor_.push();
      
      if(started())
      {
        El::Service::CompoundServiceMessage_var msg = new PushStat(this);

        deliver_at_time(msg.in(),
                        ACE_Time_Value(ACE_OS::gettimeofday().sec() +
                                       Application::instance()->config().
                                       push_stat()->period()));
      }
    }
    
    bool
    StatProcessorImpl::notify(El::Service::Event* event) throw(El::Exception)
    {
      if(El::Service::CompoundService<>::notify(event))
      {
        return true;
      }

      ProcessRequestStat* prs = dynamic_cast<ProcessRequestStat*>(event);
      
      if(prs != 0)
      {
        process_request_stat(prs->info_packs);
        return true;
      }
      
      ProcessMessageImpressionStat* pis =
        dynamic_cast<ProcessMessageImpressionStat*>(event);
      
      if(pis != 0)
      {
        process_message_impression_stat(pis->info_packs);
        return true;
      }

      ProcessPageImpressionStat* pps =
        dynamic_cast<ProcessPageImpressionStat*>(event);
      
      if(pps != 0)
      {
        process_page_impression_stat(pps->info_packs);
        return true;
      }
      
      ProcessMessageClickStat* pcs = dynamic_cast<ProcessMessageClickStat*>(
        event);
      
      if(pcs != 0)
      {
        process_message_click_stat(pcs->info_packs);
        return true;
      }

      DispatchStatProcessing* dsp =
        dynamic_cast<DispatchStatProcessing*>(event);
      
      if(dsp != 0)
      {
        dispatch_stat_processing();
        return true;
      }
            
      PushStat* ps = dynamic_cast<PushStat*>(event);
      
      if(ps != 0)
      {
        push_stat();
        return true;
      }

      Monitoring* mtr = dynamic_cast<Monitoring*>(event);
      
      if(mtr != 0)
      {
        monitor();
        return true;
      }

      TruncateRawStat* trs = dynamic_cast<TruncateRawStat*>(event);
      
      if(trs != 0)
      {
        truncate_raw_stat();
        return true;
      }
      
      return false;
    }
    
    void
    StatProcessorImpl::wait() throw(Exception, El::Exception)
    {
      El::Service::CompoundService<>::wait();
      
      {
        MsgClickStatProcessor::MessageClickInfoPackList info_packs;      

        while(msg_click_processor_.get_packs(info_packs,
                                             save_records_at_once_))
        {
          process_message_click_stat(info_packs);
        }

        push_stat();
      }      

      {
        MsgImpStatProcessor::MessageImpressionInfoPackList info_packs;      

        while(msg_imp_processor_.get_packs(info_packs, save_records_at_once_))
        {
          process_message_impression_stat(info_packs);
        }
      }

      {
        RequestStatProcessor::RequestInfoPackList info_packs;      

        while(request_processor_.get_packs(info_packs, save_records_at_once_))
        {
          process_request_stat(info_packs);
        }
      }

      {
        PageImpStatProcessor::PageImpressionInfoPackList info_packs;      

        while(page_imp_processor_.get_packs(info_packs, save_records_at_once_))
        {
          process_page_impression_stat(info_packs);
        }
      }

    }
  }  
}
