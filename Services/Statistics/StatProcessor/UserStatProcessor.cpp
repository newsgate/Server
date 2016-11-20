/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Statistics/StatProcessor/UserStatProcessor.cpp
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
#include "UserStatProcessor.hpp"

namespace NewsGate
{
  namespace Statistics
  {    
    //
    // UserStatProcessor class
    //
    UserStatProcessor::UserStatProcessor(El::Service::Callback* callback)
      throw(Exception, El::Exception)
        : StatProcessorBase(callback)
    {
/*
      if(Application::instance()->config().dbm().present())
      {
        const Server::Config::StatProcessorType::dbm_type&
          dbm_conf = *Application::instance()->config().dbm();

        std::string user_base_name = dbm_conf.dir() + "/users.tch";
        
        user_base_ =
          new El::TokyoCabinet::HashDBM(user_base_name.c_str(),
                                        HDBOWRITER | HDBOCREAT | HDBONOLCK);
      }
*/
    }

    void
    UserStatProcessor::save(
      const RequestStatProcessor::RequestInfoPackList& packs)
      throw(El::Exception)
    {
      try
      {
        if(user_base_.in() == 0)
        {
          return;
        }
      
        ACE_High_Res_Timer timer;
        timer.start();

        size_t save_records = 0;
        size_t info_packs_count = 0;
      
        {
          WriteGuard guard(lock_);

          if(user_infos_.get() == 0)
          {
            user_infos_.reset(new UserInfoMap());
          }
          
          for(RequestStatProcessor::RequestInfoPackList::const_iterator
                pi(packs.begin()), pe(packs.end()); pi != pe;
              ++pi, ++info_packs_count)
          {
            const RequestInfoArray& request_infos = (*pi)->entities();
            save_records += request_infos.size();
            
            for(RequestInfoArray::const_iterator it = request_infos.begin();
                it != request_infos.end(); it++)
            {
              /*
                const RequestInfo& ri = *it;
                const ClientInfo& ci = ri.client;
                const RefererInfo& rf = ri.referer;
                const RequestParams& rp = ri.params;
                const Filter& fl = rp.filter;
                const ResponseInfo& rs = ri.response;
              */
            }
          }
        }
        
        ACE_Time_Value time;
        
        timer.stop();
        timer.elapsed_time(time);

        std::ostringstream ostr;
        
        ostr << "UserStatProcessor::save: saving "
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
        ostr << "UserStatProcessor::save: El::Exception caught. Description:\n"
             << e;

        El::Service::Error error(ostr.str(), 0);
        callback_->notify(&error);      
      }
    }
  }
}
