/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Statistics/StatProcessor/MsgImpStatProcessor.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_STATISTICS_STATPROCESSOR_MSGIMPSTATPRROCESSOR_HPP_
#define _NEWSGATE_SERVER_SERVICES_STATISTICS_STATPROCESSOR_MSGIMPSTATPRROCESSOR_HPP_

#include <vector>
#include <string>

#include <El/Exception.hpp>
#include <El/Service/Service.hpp>

#include <Services/Commons/Statistics/StatLogger.hpp>
#include <Services/Commons/Statistics/TransportImpl.hpp>

#include "StatProcessorBase.hpp"

namespace NewsGate
{
  namespace Statistics
  {
    class MsgImpStatProcessor : public StatProcessorBase
    {
    public:

      MsgImpStatProcessor(El::Service::Callback* callback)
        throw(Exception, El::Exception);
      
      virtual ~MsgImpStatProcessor() throw() {}      

      typedef std::list<Transport::MessageImpressionInfoPackImpl::Var>
      MessageImpressionInfoPackList;
      
      void enqueue(
        ::NewsGate::Statistics::Transport::MessageImpressionInfoPack* pack)
        throw(Exception, El::Exception);

      bool get_packs(MessageImpressionInfoPackList& info_packs,
                     size_t max_record_count) throw(El::Exception);
      
      void save(const MessageImpressionInfoPackList& packs)
        throw(El::Exception);

    private:
      MessageImpressionInfoPackList packs_;
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

#endif // _NEWSGATE_SERVER_SERVICES_STATISTICS_STATPROCESSOR_MSGIMPSTATPRROCESSOR_HPP_
