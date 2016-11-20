/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Statistics/StatProcessor/MsgClickStatProcessor.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_STATISTICS_STATPROCESSOR_MSGCLICKSTATPRROCESSOR_HPP_
#define _NEWSGATE_SERVER_SERVICES_STATISTICS_STATPROCESSOR_MSGCLICKSTATPRROCESSOR_HPP_

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
    class MsgClickStatProcessor : public StatProcessorBase
    {
    public:

      MsgClickStatProcessor(El::Service::Callback* callback)
        throw(Exception, El::Exception);
      
      virtual ~MsgClickStatProcessor() throw() {}      

      typedef std::list<Transport::MessageClickInfoPackImpl::Var>
      MessageClickInfoPackList;
      
      void enqueue(
        ::NewsGate::Statistics::Transport::MessageClickInfoPack* pack)
        throw(Exception, El::Exception);

      bool get_packs(MessageClickInfoPackList& info_packs,
                     size_t max_record_count) throw(El::Exception);
      
      void save(const MessageClickInfoPackList& packs) throw(El::Exception);

    private:
      MessageClickInfoPackList packs_;
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

#endif // _NEWSGATE_SERVER_SERVICES_STATISTICS_STATPROCESSOR_MSGCLICKSTATPRROCESSOR_HPP_
