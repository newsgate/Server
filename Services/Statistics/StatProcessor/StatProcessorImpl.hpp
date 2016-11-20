/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Statistics/StatProcessor/StatProcessorImpl.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_STATISTICS_STATPROCESSOR_STATPROCESSORIMPL_HPP_
#define _NEWSGATE_SERVER_SERVICES_STATISTICS_STATPROCESSOR_STATPROCESSORIMPL_HPP_

#include <string>
#include <vector>
#include <list>
#include <memory>

#include <ext/hash_map>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/Hash/Hash.hpp>
#include <El/Service/CompoundService.hpp>
#include <El/TokyoCabinet/HashDBM.hpp>

#include <Services/Commons/Statistics/StatLogger.hpp>
#include <Services/Commons/Statistics/TransportImpl.hpp>
#include <Services/Commons/Statistics/StatisticsServices_s.hpp>

#include "RequestStatProcessor.hpp"
#include "PageImpStatProcessor.hpp"
#include "MsgImpStatProcessor.hpp"
#include "MsgClickStatProcessor.hpp"
#include "FeedStatProcessor.hpp"
#include "UserStatProcessor.hpp"

namespace NewsGate
{
  namespace Statistics
  {
    class StatProcessorImpl :
      public virtual POA_NewsGate::Statistics::StatProcessor,
      public virtual El::Service::CompoundService<> 
    {
    public:
      EL_EXCEPTION(Exception, El::ExceptionBase);

    public:

      StatProcessorImpl(El::Service::Callback* callback)
        throw(Exception, El::Exception);

      virtual ~StatProcessorImpl() throw();

      virtual void wait() throw(Exception, El::Exception);

    private:

      //
      // IDL:NewsGate/Statistics/StatProcessor/message_bank_manager:1.0
      //
      virtual char* message_bank_manager()
        throw(NewsGate::Statistics::ImplementationException,
              CORBA::SystemException);
      
      //
      // IDL:NewsGate/Statistics/StatProcessor/search_request:1.0
      //
      virtual void search_request(
        ::CORBA::ULong interface_version,
        ::NewsGate::Statistics::Transport::RequestInfoPack* pack)
        throw(NewsGate::Statistics::ImplementationException,
              CORBA::SystemException);      

      //
      // IDL:NewsGate/Statistics/StatProcessor/page_impression:1.0
      //
      virtual void page_impression(
        ::CORBA::ULong interface_version,
        ::NewsGate::Statistics::Transport::PageImpressionInfoPack* pack)
        throw(NewsGate::Statistics::ImplementationException,
              CORBA::SystemException);

      //
      // IDL:NewsGate/Statistics/StatProcessor/message_impression:1.0
      //
      virtual void message_impression(
        ::CORBA::ULong interface_version,
        ::NewsGate::Statistics::Transport::MessageImpressionInfoPack* pack)
        throw(NewsGate::Statistics::ImplementationException,
              CORBA::SystemException);

      //
      // IDL:NewsGate/Statistics/StatProcessor/message_click:1.0
      //
      virtual void message_click(
        ::CORBA::ULong interface_version,
        ::NewsGate::Statistics::Transport::MessageClickInfoPack* pack)
        throw(NewsGate::Statistics::ImplementationException,
              CORBA::SystemException);

      virtual bool notify(El::Service::Event* event) throw(El::Exception);

      void truncate_raw_stat() throw(El::Exception);
      void monitor() throw(El::Exception);
      
      void dispatch_stat_processing() throw(El::Exception);
      void dispatch_request_processing() throw(El::Exception);
      void dispatch_page_impression_processing() throw(El::Exception);
      void dispatch_message_impression_processing() throw(El::Exception);
      void dispatch_message_click_processing() throw(El::Exception);

      void process_request_stat(
        const RequestStatProcessor::RequestInfoPackList& packs)
        throw(El::Exception);

      void process_page_impression_stat(
        const PageImpStatProcessor::PageImpressionInfoPackList& packs)
        throw(El::Exception);

      void process_message_impression_stat(
        const MsgImpStatProcessor::MessageImpressionInfoPackList& packs)
        throw(El::Exception);

      void process_message_click_stat(
        const MsgClickStatProcessor::MessageClickInfoPackList& packs)
        throw(El::Exception);

      void push_stat() throw(El::Exception);
      
    private:

      struct ProcessRequestStat : public El::Service::CompoundServiceMessage
      {
        RequestStatProcessor::RequestInfoPackList info_packs;
        
        ProcessRequestStat(StatProcessorImpl* processor,
                           RequestStatProcessor::RequestInfoPackList& packs)
          throw(El::Exception);
        
        ~ProcessRequestStat() throw() {}
      };      

      struct ProcessPageImpressionStat :
        public El::Service::CompoundServiceMessage
      {
        PageImpStatProcessor::PageImpressionInfoPackList info_packs;
        
        ProcessPageImpressionStat(
          StatProcessorImpl* processor,
          PageImpStatProcessor::PageImpressionInfoPackList& packs)
          throw(El::Exception);
        
        ~ProcessPageImpressionStat() throw() {}
      };

      struct ProcessMessageImpressionStat :
        public El::Service::CompoundServiceMessage
      {
        MsgImpStatProcessor::MessageImpressionInfoPackList info_packs;
        
        ProcessMessageImpressionStat(
          StatProcessorImpl* processor,
          MsgImpStatProcessor::MessageImpressionInfoPackList& packs)
          throw(El::Exception);
        
        ~ProcessMessageImpressionStat() throw() {}
      };      

      struct ProcessMessageClickStat :
        public El::Service::CompoundServiceMessage
      {
        MsgClickStatProcessor::MessageClickInfoPackList info_packs;
        
        ProcessMessageClickStat(
          StatProcessorImpl* processor,
          MsgClickStatProcessor::MessageClickInfoPackList& packs)
          throw(El::Exception);
        
        ~ProcessMessageClickStat() throw() {}
      };

      struct DispatchStatProcessing :
        public El::Service::CompoundServiceMessage
      {
        DispatchStatProcessing(StatProcessorImpl* processor)
          throw(El::Exception);
        
        ~DispatchStatProcessing() throw() {}
      };      

      struct TruncateRawStat : public El::Service::CompoundServiceMessage
      {
        TruncateRawStat(StatProcessorImpl* processor) throw(El::Exception);
        ~TruncateRawStat() throw() {}
      };

      struct Monitoring : public El::Service::CompoundServiceMessage
      {
        Monitoring(StatProcessorImpl* processor) throw(El::Exception);
        ~Monitoring() throw() {}
      };

      struct PushStat : public El::Service::CompoundServiceMessage
      {
        PushStat(StatProcessorImpl* processor) throw(El::Exception);
        ~PushStat() throw() {}
      };      

      unsigned long raw_stat_keep_days_;
      size_t save_records_at_once_;

      RequestStatProcessor request_processor_;
      PageImpStatProcessor page_imp_processor_;
      MsgImpStatProcessor msg_imp_processor_;
      MsgClickStatProcessor msg_click_processor_;
      FeedStatProcessor feed_stat_processor_;
      UserStatProcessor user_stat_processor_;
    };

    typedef El::RefCount::SmartPtr<StatProcessorImpl> StatProcessorImpl_var;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Statistics
  {
    //
    // StatProcessorImpl::ProcessRequestStat class
    //
    inline
    StatProcessorImpl::ProcessRequestStat::ProcessRequestStat(
      StatProcessorImpl* processor,
      RequestStatProcessor::RequestInfoPackList& packs) throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(processor, processor, true),
          El::Service::CompoundServiceMessage(processor, processor)
    {
      info_packs.swap(packs);
    }
    
    //
    // StatProcessorImpl::ProcessPageImpressionStat class
    //
    inline
    StatProcessorImpl::ProcessPageImpressionStat::ProcessPageImpressionStat(
      StatProcessorImpl* processor,
      PageImpStatProcessor::PageImpressionInfoPackList& packs)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(processor, processor, true),
          El::Service::CompoundServiceMessage(processor, processor)
    {
      info_packs.swap(packs);
    }
    
    //
    // StatProcessorImpl::ProcessMessageImpressionStat class
    //
    inline
    StatProcessorImpl::ProcessMessageImpressionStat::
    ProcessMessageImpressionStat(
      StatProcessorImpl* processor,
      MsgImpStatProcessor::MessageImpressionInfoPackList& packs)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(processor, processor, true),
          El::Service::CompoundServiceMessage(processor, processor)
    {
      info_packs.swap(packs);
    }

    //
    // StatProcessorImpl::ProcessMessageClickStat class
    //
    inline
    StatProcessorImpl::ProcessMessageClickStat::ProcessMessageClickStat(
      StatProcessorImpl* processor,
      MsgClickStatProcessor::MessageClickInfoPackList& packs)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(processor, processor, true),
          El::Service::CompoundServiceMessage(processor, processor)
    {
      info_packs.swap(packs);
    }

    //
    // StatProcessorImpl::DispatchStatProcessing class
    //
    inline
    StatProcessorImpl::DispatchStatProcessing::DispatchStatProcessing(
      StatProcessorImpl* processor) throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(processor, processor, true),
          El::Service::CompoundServiceMessage(processor, processor)
    {
    }
    
    //
    // StatProcessorImpl::TruncateRawStat class
    //
    inline
    StatProcessorImpl::TruncateRawStat::TruncateRawStat(
      StatProcessorImpl* processor) throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(processor, processor, true),
          El::Service::CompoundServiceMessage(processor, processor)
    {
    }

    //
    // StatProcessorImpl::Monitoring class
    //
    inline
    StatProcessorImpl::Monitoring::Monitoring(
      StatProcessorImpl* processor) throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(processor, processor, true),
          El::Service::CompoundServiceMessage(processor, processor)
    {
    }

    //
    // StatProcessorImpl::PushStat class
    //
    inline
    StatProcessorImpl::PushStat::PushStat(
      StatProcessorImpl* processor) throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(processor, processor, true),
          El::Service::CompoundServiceMessage(processor, processor)
    {
    }
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_STATISTICS_STATPROCESSOR_STATPROCESSORIMPL_HPP_
