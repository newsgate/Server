/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Statistics/StatProcessor/UserStatProcessor.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_STATISTICS_STATPROCESSOR_USERSTATPRROCESSOR_HPP_
#define _NEWSGATE_SERVER_SERVICES_STATISTICS_STATPROCESSOR_USERSTATPRROCESSOR_HPP_

#include <vector>
#include <string>

#include <El/Exception.hpp>
#include <El/Service/Service.hpp>

#include <Services/Commons/Statistics/StatLogger.hpp>
#include <Services/Commons/Statistics/TransportImpl.hpp>

#include "StatProcessorBase.hpp"
#include "RequestStatProcessor.hpp"

namespace NewsGate
{
  namespace Statistics
  {
    class UserStatProcessor : public StatProcessorBase
    {
    public:

      UserStatProcessor(El::Service::Callback* callback)
        throw(Exception, El::Exception);
      
      virtual ~UserStatProcessor() throw() {}

      void save(const RequestStatProcessor::RequestInfoPackList& packs)
        throw(El::Exception);
      
    private:

      struct UserInfo
      {
        uint8_t devoted;
        uint32_t views;
        uint64_t first_seen;
        uint64_t last_seen;

        UserInfo() throw() : devoted(0),views(0),first_seen(0),last_seen(0) {}
      };
      
      typedef __gnu_cxx::hash_map<std::string,
                                  UserInfo,
                                  El::Hash::String> UserInfoMap;

      std::auto_ptr<UserInfoMap> user_infos_;      
      El::TokyoCabinet::HashDBM_var user_base_;
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

#endif // _NEWSGATE_SERVER_SERVICES_STATISTICS_STATPROCESSOR_USERSTATPRROCESSOR_HPP_
