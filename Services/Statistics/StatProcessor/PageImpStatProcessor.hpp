/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Statistics/StatProcessor/PageImpStatProcessor.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_STATISTICS_STATPROCESSOR_PAGEIMPSTATPRROCESSOR_HPP_
#define _NEWSGATE_SERVER_SERVICES_STATISTICS_STATPROCESSOR_PAGEIMPSTATPRROCESSOR_HPP_

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
    class PageImpStatProcessor : public StatProcessorBase
    {
    public:

      PageImpStatProcessor(El::Service::Callback* callback)
        throw(Exception, El::Exception);
      
      virtual ~PageImpStatProcessor() throw() {}      

      typedef std::list<Transport::PageImpressionInfoPackImpl::Var>
      PageImpressionInfoPackList;
      
      void enqueue(
        ::NewsGate::Statistics::Transport::PageImpressionInfoPack* pack)
        throw(Exception, El::Exception);

      bool get_packs(PageImpressionInfoPackList& info_packs,
                     size_t max_record_count) throw(El::Exception);
      
      void save(const PageImpressionInfoPackList& packs) throw(El::Exception);

    private:

      PageImpressionInfoPackList packs_;
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

#endif // _NEWSGATE_SERVER_SERVICES_STATISTICS_STATPROCESSOR_PAGEIMPSTATPRROCESSOR_HPP_
