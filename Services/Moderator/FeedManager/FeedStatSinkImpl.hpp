/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/FeedManager/FeedStatSinkImpl.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MODERATOR_FEEDMANAGER_FEEDSTATSINKIMPL_HPP_
#define _NEWSGATE_SERVER_SERVICES_MODERATOR_FEEDMANAGER_FEEDSTATSINKIMPL_HPP_

#include <memory>

#include <El/Exception.hpp>
#include <El/Service/CompoundService.hpp>
#include <El/MySQL/DB.hpp>

#include <Services/Commons/Statistics/StatLogger.hpp>
#include <Services/Commons/Statistics/TransportImpl.hpp>
#include <Services/Commons/Statistics/StatisticsServices_s.hpp>

namespace NewsGate
{
  namespace Statistics
  {
    class FeedSinkImpl :
      public virtual POA_NewsGate::Statistics::FeedSink,
      public virtual El::Service::CompoundService<>
    {
    public:
      EL_EXCEPTION(Exception, El::ExceptionBase);
      EL_EXCEPTION(InvalidArgument, Exception);

    public:
      FeedSinkImpl(El::Service::Callback* callback)
        throw(InvalidArgument, Exception, El::Exception);

      virtual ~FeedSinkImpl() throw();

      virtual void wait() throw(Exception, El::Exception);

    private:

      //
      // IDL:NewsGate/Statistics/FeedSink/feed_impression_click:1.0
      //
      virtual void feed_impression_click(
        ::NewsGate::Statistics::Transport::MessageImpressionClickCounter*
        counter)
        throw(NewsGate::Statistics::ImplementationException,
              CORBA::SystemException);      
      
      virtual bool notify(El::Service::Event* event) throw(El::Exception);

      struct ProcessMessageImpressionClickStat :
        public El::Service::CompoundServiceMessage
      {
        ::NewsGate::Statistics::Transport::MessageImpressionClickCounterImpl::
        Var pack;
        
        ProcessMessageImpressionClickStat(
          FeedSinkImpl* service,
          ::NewsGate::Statistics::Transport::
          MessageImpressionClickCounterImpl::Type* counter)
          throw(El::Exception);
        
        ~ProcessMessageImpressionClickStat() throw() {}
      };      

      struct SaveMessageImpressionClickStat :
        public El::Service::CompoundServiceMessage
      {
        SaveMessageImpressionClickStat(FeedSinkImpl* service)
          throw(El::Exception);
        
        ~SaveMessageImpressionClickStat() throw() {}
      };

      void save_message_stat(SaveMessageImpressionClickStat* smics)
        throw(El::Exception);

      void save_message_stat(
        ::NewsGate::Statistics::Transport::MessageImpressionClickMap& stat,
        El::MySQL::Connection* connection)
        throw(El::Exception);
        
      void cleanup_stat(time_t cur_time,
                        El::MySQL::Connection* connection)
        throw(El::Exception);
      
      void process_message_imp_click_stat(
        ::NewsGate::Statistics::Transport::MessageImpressionClickMap* counter)
        throw(El::Exception);
      
    private:

      typedef std::auto_ptr< ::NewsGate::Statistics::Transport::
      MessageImpressionClickMap > MessageImpressionClickMapPtr;

      MessageImpressionClickMapPtr imp_click_stat_;
      ACE_Time_Value next_stat_cleanup_;
    };

    typedef El::RefCount::SmartPtr<FeedSinkImpl> FeedSinkImpl_var;
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
    // FeedSinkImpl::ProcessMessageImpressionClickStat class
    //
    inline
    FeedSinkImpl::ProcessMessageImpressionClickStat::
    ProcessMessageImpressionClickStat(
      FeedSinkImpl* service,
      ::NewsGate::Statistics::Transport::
      MessageImpressionClickCounterImpl::Type* counter) throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(service, service, true),
          El::Service::CompoundServiceMessage(service, service),
          pack(counter)
    {
      pack->_add_ref();
    }

    //
    // FeedSinkImpl::SaveMessageImpressionClickStat class
    //
    inline
    FeedSinkImpl::SaveMessageImpressionClickStat::
    SaveMessageImpressionClickStat(FeedSinkImpl* service) throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(service, service, true),
          El::Service::CompoundServiceMessage(service, service)
    {
    }

  }
}

#endif // _NEWSGATE_SERVER_SERVICES_MODERATOR_FEEDMANAGER_FEEDSTATSINKIMPL_HPP_
