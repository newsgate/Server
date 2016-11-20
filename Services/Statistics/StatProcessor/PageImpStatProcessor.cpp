/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Statistics/StatProcessor/PageImpStatProcessor.cpp
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
#include "PageImpStatProcessor.hpp"

namespace NewsGate
{
  namespace Statistics
  {    
    //
    // PageImpStatProcessor class
    //
    PageImpStatProcessor::PageImpStatProcessor(El::Service::Callback* callback)
      throw(Exception, El::Exception)
        : StatProcessorBase(callback)
    {
    }

    void
    PageImpStatProcessor::enqueue(
      ::NewsGate::Statistics::Transport::PageImpressionInfoPack* pack)
      throw(Exception, El::Exception)
    {
      Transport::PageImpressionInfoPackImpl::Type* pack_impl =
        dynamic_cast<Transport::PageImpressionInfoPackImpl::Type*>(pack);

      if(pack_impl == 0)
      {
        throw Exception("NewsGate::Statistics::PageImpStatProcessor::enqueue: "
                        "dynamic_cast<Transport::"
                        "PageImpressionInfoPackImpl::Type*> failed");
      }

      pack_impl->_add_ref();

      WriteGuard guard(lock_);
        
      packs_.push_back(pack_impl);
      packs_count_++;
    }

    void
    PageImpStatProcessor::save(const PageImpressionInfoPackList& packs)
      throw(El::Exception)
    {
      std::string filename;
      
      try
      {
        ACE_High_Res_Timer timer;
        timer.start();

        size_t save_records = 0;
        size_t info_packs_count = 0;
      
        filename = cache_filename_ + ".pimp." +
          El::Moment(ACE_OS::gettimeofday()).dense_format();
      
        std::fstream file(filename.c_str(), ios::out);
        
        if(!file.is_open())
        {
          std::ostringstream ostr;
          ostr << "PageImpStatProcessor::save: failed to "
            "open file '" << filename << "' for write access";
          
          throw Exception(ostr.str());
        }

        bool first_line = true;
            
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();

        for(PageImpressionInfoPackList::const_iterator pi(packs.begin()),
              pe(packs.end()); pi != pe; ++pi, ++info_packs_count)
        {
          const PageImpressionInfoArray& infos = (*pi)->entities();
          save_records += infos.size();
        
          for(PageImpressionInfoArray::const_iterator
                i(infos.begin()), e(infos.end()); i != e; ++i)
          {
            const PageImpressionInfo& pi = *i;
            const RefererInfo& rf = pi.referer;

            if(first_line)
            {
              first_line = false;
            }
            else
            {
              file << std::endl;
            }
            
            file << pi.id << "\t";
            El::MySQL::Connection::escape_for_load(file, rf.url);

            file  << "\t" << pi.time.datetime << "\t"
                  << pi.time.usec << "\t" << (char)pi.protocol
                  << "\t" << (int)rf.internal << "\t";
            
            El::MySQL::Connection::escape_for_load(file, rf.site);
            file << "\t";
            
            El::MySQL::Connection::escape_for_load(file, rf.company_domain);
            file << "\t";
            
            El::MySQL::Connection::escape_for_load(file, rf.page);
            file << "\t1";
          }
        }
            
        if(file.fail())
        {
          std::ostringstream ostr;
          ostr << "PageImpStatProcessor::save: "
            "failed to write into file '" << filename << "'";

          throw Exception(ostr.str());
        }
      
        file.close();

        ACE_Time_Value write_time;
        
        timer.stop();
        timer.elapsed_time(write_time);

        timer.start();
        
        DB_Guard guard(db_lock_);
        
        El::MySQL::Result_var result =
          connection->query("delete from StatPageImpressionBuff");
        
        {
          std::ostringstream ostr;
        
          ostr << "LOAD DATA INFILE '"
               << connection->escape(filename.c_str())
               << "' INTO TABLE StatPageImpressionBuff character set binary";
          
          result = connection->query(ostr.str().c_str());
        }
        
        unlink(filename.c_str());
        filename.clear();
        
        ACE_Time_Value load_time;
        
        timer.stop();
        timer.elapsed_time(load_time);

        timer.start();

        result = connection->query(
            "INSERT INTO StatPageImpression SELECT * FROM "
            "StatPageImpressionBuff ON DUPLICATE KEY UPDATE "
            "StatPageImpression.count=StatPageImpression.count+1");

        guard.release();
        
        ACE_Time_Value insert_time;
        
        timer.stop();
        timer.elapsed_time(insert_time);

        std::ostringstream ostr;
            
        ostr << "PageImpStatProcessor::save: saving "
             << save_records << " records in "
             << info_packs_count << " packs for "
             << El::Moment::time(write_time) << " + "
             << El::Moment::time(load_time) << " + "
             << El::Moment::time(insert_time);
          
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
        
        ostr << "PageImpStatProcessor::save: "
          "El::Exception caught. Description:\n" << e;

        El::Service::Error error(ostr.str(), 0);
        callback_->notify(&error);        
      }
    }

    bool
    PageImpStatProcessor::get_packs(PageImpressionInfoPackList& info_packs,
                                    size_t max_record_count)
      throw(El::Exception)
    {
      try
      {
        info_packs.clear();
      
        Transport::PageImpressionInfoPackImpl::Var pack;
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
        ostr << "PageImpStatProcessor::get_packs: El::Exception caught. "
          "Description:\n" << e;

        El::Service::Error error(ostr.str(), 0);
        callback_->notify(&error);      
      }

      return !info_packs.empty();
    }
  }
}
