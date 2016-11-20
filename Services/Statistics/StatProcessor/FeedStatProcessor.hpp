/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Statistics/StatProcessor/FeedStatProcessor.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_STATISTICS_STATPROCESSOR_FEEDSTATPRROCESSOR_HPP_
#define _NEWSGATE_SERVER_SERVICES_STATISTICS_STATPROCESSOR_FEEDSTATPRROCESSOR_HPP_

#include <vector>
#include <string>

#include <El/Exception.hpp>
#include <El/Service/Service.hpp>

#include <Services/Commons/Statistics/StatLogger.hpp>
#include <Services/Commons/Statistics/TransportImpl.hpp>

#include "StatProcessorBase.hpp"
#include "MsgImpStatProcessor.hpp"
#include "MsgClickStatProcessor.hpp"

namespace NewsGate
{
  namespace Statistics
  {
    class FeedStatProcessor : public StatProcessorBase
    {
    public:

      FeedStatProcessor(El::Service::Callback* callback)
        throw(Exception, El::Exception);
      
      virtual ~FeedStatProcessor() throw() {}

      void save_impressions(
        const MsgImpStatProcessor::MessageImpressionInfoPackList& packs)
        throw(El::Exception);

      void save_clicks(
        const MsgClickStatProcessor::MessageClickInfoPackList& packs)
        throw(El::Exception);
      
      void push() throw(El::Exception);
      
    private:

      typedef El::Corba::SmartRef<FeedSink> FeedStatSinkRef;
      
      std::auto_ptr<Transport::MessageImpressionClickMap> feed_stat_;
      FeedStatSinkRef feed_stat_sink_;
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
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_STATISTICS_STATPROCESSOR_FEEDSTATPRROCESSOR_HPP_
