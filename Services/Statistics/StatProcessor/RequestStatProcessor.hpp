/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Statistics/StatProcessor/RequestStatProcessor.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_STATISTICS_STATPROCESSOR_REQUESTSTATPRROCESSOR_HPP_
#define _NEWSGATE_SERVER_SERVICES_STATISTICS_STATPROCESSOR_REQUESTSTATPRROCESSOR_HPP_

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
    class RequestStatProcessor : public StatProcessorBase
    {
    public:

      RequestStatProcessor(El::Service::Callback* callback)
        throw(Exception, El::Exception);
      
      virtual ~RequestStatProcessor() throw() {}      

      typedef std::list<Transport::RequestInfoPackImpl::Var>
      RequestInfoPackList;
      
      void enqueue(::NewsGate::Statistics::Transport::RequestInfoPack* pack)
        throw(Exception, El::Exception);

      bool get_packs(RequestInfoPackList& info_packs,
                     size_t max_record_count) throw(El::Exception);
      
      void save(const RequestInfoPackList& packs) throw(El::Exception);

    private:
      RequestInfoPackList packs_;
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

#endif // _NEWSGATE_SERVER_SERVICES_STATISTICS_STATPROCESSOR_REQUESTSTATPRROCESSOR_HPP_
