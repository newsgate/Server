/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Statistics/StatProcessor/RequestStatProcessor.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <string>
#include <sstream>
#include <fstream>

#include <El/Exception.hpp>
#include <El/Moment.hpp>
//#include <El/Stat.hpp>
#include <El/MySQL/DB.hpp>

#include <ace/OS.h>
#include <ace/High_Res_Timer.h>

#include "StatProcessorMain.hpp"
#include "RequestStatProcessor.hpp"

namespace NewsGate
{
  namespace Statistics
  {    
    //
    // RequestStatProcessor class
    //
    RequestStatProcessor::RequestStatProcessor(El::Service::Callback* callback)
      throw(Exception, El::Exception)
        : StatProcessorBase(callback)
    {
    }

    void
    RequestStatProcessor::enqueue(
      ::NewsGate::Statistics::Transport::RequestInfoPack* pack)
      throw(Exception, El::Exception)
    {
      Transport::RequestInfoPackImpl::Type* pack_impl =
        dynamic_cast<Transport::RequestInfoPackImpl::Type*>(pack);

      if(pack_impl == 0)
      {
        throw Exception("NewsGate::Statistics::RequestStatProcessor::enqueue: "
                        "dynamic_cast<Transport::"
                        "RequestInfoPackImpl::Type*> failed");
      }

      pack_impl->_add_ref();

      WriteGuard guard(lock_);
        
      packs_.push_back(pack_impl);
      packs_count_++;
    }

    void
    RequestStatProcessor::save(const RequestInfoPackList& packs)
      throw(El::Exception)
    {
      std::string filename;
      
      try
      {
        ACE_High_Res_Timer timer;
        timer.start();

        size_t save_records = 0;
        size_t info_packs_count = 0;
      
        filename = cache_filename_ + ".req." +
          El::Moment(ACE_OS::gettimeofday()).dense_format();
      
        std::fstream file(filename.c_str(), ios::out);
        
        if(!file.is_open())
        {
          std::ostringstream ostr;
          ostr << "RequestStatProcessor::save: failed to "
            "open file '" << filename << "' for write access";
          
          throw Exception(ostr.str());
        }

        bool first_line = true;
            
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();

        for(RequestInfoPackList::const_iterator pi(packs.begin()),
              pe(packs.end()); pi != pe; ++pi, ++info_packs_count)
        {
          const RequestInfoArray& request_infos = (*pi)->entities();
          save_records += request_infos.size();
        
          for(RequestInfoArray::const_iterator i(request_infos.begin()),
                e(request_infos.end()); i != e; ++i)
          {
            const RequestInfo& ri = *i;
            const ClientInfo& ci = ri.client;
            const RefererInfo& rf = ri.referer;
            const RequestParams& rp = ri.params;
            const Filter& fl = rp.filter;
            const ResponseInfo& rs = ri.response;

            if(first_line)
            {
              first_line = false;
            }
            else
            {
              file << std::endl;
            }

            file << ri.id << "\t" << ri.time.datetime << "\t"
                 << ri.time.usec << "\t"<< ri.request_duration << "\t"
                 << ri.search_duration << "\t" << ri.host << "\t"
                 << ci.id << "\t" << (char)ci.type << "\t" << ci.name
                 << "\t" << ci.os << "\t" << (char)ci.device_type << "\t"
                 << ci.device << "\t";
          
            El::MySQL::Connection::escape_for_load(file, ci.user_agent);

            file << "\t" << ci.lang << "\t" << ci.country << "\t" << ci.ip
                 << "\t" << (int)rf.internal << "\t";

            El::MySQL::Connection::escape_for_load(file, rf.site);
            file << "\t";

            El::MySQL::Connection::escape_for_load(file, rf.company_domain);
            file << "\t";
            
            El::MySQL::Connection::escape_for_load(file, rf.page);
            file << "\t";
            
            El::MySQL::Connection::escape_for_load(file, rf.url);
                 
            file << "\t" << (char)rp.protocol
                 << "\t" << (int)rp.create_informer << "\t" << (int)rp.columns
                 << "\t" << (int)rp.sorting_type
                 << "\t" << (int)rp.suppression_type
                 << "\t" << rp.start_item
                 << "\t" << rp.item_count
                 << "\t" << rp.annotation_len
                 << "\t" << rp.sr_flags
                 << "\t" << rp.gm_flags
                 << "\t" << rp.locale.lang
                 << "\t" << rp.locale.country << "\t";

            El::MySQL::Connection::escape_for_load(file, rp.query);
            
            file << "\t";
            El::MySQL::Connection::escape_for_load(file, rp.informer_params);
            
            file << "\t" << (char)rp.modifier.type << "\t";
                
            El::MySQL::Connection::escape_for_load(file,
                                                   rp.modifier.value);

            file << "\t" << fl.lang
                 << "\t" << fl.country
                 << "\t" << fl.event
                 << "\t" << fl.feed << "\t";

            El::MySQL::Connection::escape_for_load(file, fl.category);
          
            file << "\t" << (int)rs.messages_loaded
                 << "\t" << rs.total_matched_messages
                 << "\t" << rs.suppressed_messages << "\t";

            El::MySQL::Connection::escape_for_load(file, rs.optimized_query);

            file << "\t" << rp.translate_def_lang << "\t" << rp.translate_lang
                 << "\t" << (char)rp.message_view << "\t"
                 << (int)rp.print_left_bar;
          }
        }
            
        if(file.fail())
        {
          std::ostringstream ostr;
          ostr << "RequestStatProcessor::save: "
            "failed to write into file '" << filename << "'";

          throw Exception(ostr.str());
        }
      
        file.close();

        ACE_Time_Value write_time;
        
        timer.stop();
        timer.elapsed_time(write_time);

        timer.start();
        
        {
          std::ostringstream ostr;
        
          ostr << "LOAD DATA INFILE '"
               << connection->escape(filename.c_str())
               << "' REPLACE INTO TABLE StatSearchRequest "
            "character set binary";
          
          El::MySQL::Result_var result =
            connection->query(ostr.str().c_str());
        }

        unlink(filename.c_str());
        filename.clear();
        
        ACE_Time_Value load_time;
        
        timer.stop();
        timer.elapsed_time(load_time);

        std::ostringstream ostr;
            
        ostr << "RequestStatProcessor::save: saving "
             << save_records << " records in "
             << info_packs_count << " packs for "
             << El::Moment::time(write_time) << " + "
             << El::Moment::time(load_time);
          
        Application::logger()->trace(ostr.str().c_str(),
                                     Aspect::STATE,
                                     El::Logging::HIGH);
      }
      catch(const El::Exception& e)
      {
        if(!filename.empty())
        {
          unlink(filename.c_str());
        }
        
        std::ostringstream ostr;
        
        ostr << "RequestStatProcessor::save: "
          "El::Exception caught. Description:\n" << e;

        El::Service::Error error(ostr.str(), 0);
        callback_->notify(&error);        
      }
    }

    bool
    RequestStatProcessor::get_packs(RequestInfoPackList& info_packs,
                                    size_t max_record_count)
      throw(El::Exception)
    {
      try
      {
        info_packs.clear();
      
        Transport::RequestInfoPackImpl::Var pack;
        size_t save_records = 0;
        
        for(; save_records < max_record_count;
            save_records += pack->entities().size())
        {
          {
            WriteGuard guard(lock_);
            
            if(packs_.empty())
            {
              break;
            }

            pack = *packs_.begin();
          
            packs_.pop_front();
            packs_count_--;
          }
          
          info_packs.push_back(pack);
        }
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "RequestStatProcessor::get_packs: El::Exception caught. "
          "Description:\n" << e;

        El::Service::Error error(ostr.str(), 0);
        callback_->notify(&error);      
      }

      return !info_packs.empty();
    }
  }
}
