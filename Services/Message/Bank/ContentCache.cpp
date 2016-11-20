/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Message/Bank/ContentCache.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <utility>

#include <ace/OS.h>
#include <ace/Synch.h>
#include <ace/Guard_T.h>
#include <ace/High_Res_Timer.h>

#include <El/MySQL/DB.hpp>

#include "BankMain.hpp"
#include "ContentCache.hpp"

namespace NewsGate
{
  namespace Message
  {
    ContentCache::ContentCache() throw(El::Exception)
        : content_map_(new StoredContentMap())
    {
    }

    ContentCache::StoredContentMap*
    ContentCache::flush() throw(El::Exception)
    {
      StoredContentMapPtr result;
      
      {
        WriteGuard guard(lock_);

        if(content_map_->empty())
        {
          return 0;
        }

        result.reset(content_map_.release());
        content_map_.reset(new StoredContentMap());
      }

      return result.release();
    }
    
    ContentCache::StoredContentMap*
    ContentCache::get(const IdArray& ids, bool keep_loaded)
      throw(El::Exception)
    {
      StoredContentMapPtr result(new StoredContentMap(ids.size()));
      std::auto_ptr<std::ostringstream> load_msg_content_request_ostr;
      
      time_t timestamp = ACE_OS::gettimeofday().sec();
      
      {
        ReadGuard guard(lock_);
      
        for(IdArray::const_iterator it = ids.begin(); it != ids.end(); it++)
        {
          const Id& id = *it;
          StoredContentMap::const_iterator cit = content_map_->find(id);

          if(cit == content_map_->end())
          {
            query_stored_content(id, load_msg_content_request_ostr);
          }
          else
          {
            StoredContent_var content = cit->second;
            
            content->timestamp(timestamp);
            result->insert(std::make_pair(id, content));
          }
        }
      }

      if(load_msg_content_request_ostr.get() == 0)
      {
        return result.release();
      }
        
      *load_msg_content_request_ostr << " )";
      
      El::MySQL::Connection_var connection =
        Application::instance()->dbase()->connect();

      ACE_High_Res_Timer timer;
      
      if(Application::will_trace(El::Logging::HIGH))
      {
        timer.start();
      }
        
      El::MySQL::Result_var query_result =
        connection->query(load_msg_content_request_ostr->str().c_str());
      
      MessageContentRecord record(query_result.in());

      unsigned long loaded_msg_count = query_result->num_rows();

      typedef std::vector< std::pair<Id, StoredContent_var> >
        StoredContentArray;
      
      std::auto_ptr<StoredContentArray> loaded_messages;

      if(keep_loaded)
      {
        loaded_messages.reset(new StoredContentArray());
        loaded_messages->reserve(loaded_msg_count);
      }

      while(record.fetch_row())
      {
        Id id(record.id());

        try
        {
          StoredContent_var content = read_stored_content(record);
        
          if(keep_loaded)
          {
            content->timestamp(timestamp);
            loaded_messages->push_back(std::make_pair(id, content));
          }
        
          (*result)[id] = content.retn();
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::ContentCache::get: El::Exception caught "
            "while retrieving content of message " << id.string()
               << ". Description:\n" << e;
          
          Application::logger()->emergency(ostr.str(), Aspect::MSG_MANAGEMENT);
        }
      }

      if(Application::will_trace(El::Logging::HIGH))
      {
        timer.stop();
        ACE_Time_Value tm;
        timer.elapsed_time(tm);
            
        std::ostringstream ostr;
        ostr << "NewsGate::Message::ContentCache::get: "
             << loaded_msg_count << " messages content query time "
             << El::Moment::time(tm);

        Application::logger()->trace(ostr.str(),
                                     Aspect::DB_PERFORMANCE,
                                     El::Logging::HIGH);
      }
      
      if(keep_loaded)
      {
        WriteGuard guard(lock_);

        for(StoredContentArray::const_iterator it = loaded_messages->begin();
            it != loaded_messages->end(); it++)
        {
          (*content_map_)[it->first] = it->second;
        }
      }
      
      return result.release();
    }
    
    StoredContent*
    ContentCache::read_stored_content(const MessageContentRecord& record)
      throw(El::Exception)
    {
      StoredContent_var content = new StoredContent();

      content->dict_hash = record.dict_hash().is_null() ?
        0 : record.dict_hash();
      
      content->url = record.url();
      content->source_html_link = record.source_html_link();
      
      {
        std::string complements = record.complements();
        std::istringstream istr(complements);

        WordPosition description_base = 0;
        content->read_complements(istr, description_base);
      }
      
      return content.retn();
    }

    void
    ContentCache::query_stored_content(
      const Message::Id& id,
      std::auto_ptr<std::ostringstream>& load_msg_content_request_ostr)
      throw(El::Exception)
    {
      if(load_msg_content_request_ostr.get() == 0)
      {
        load_msg_content_request_ostr.reset(
          new std::ostringstream());
        
        *load_msg_content_request_ostr
          << "select Message.id as id, MessageDict.hash as dict_hash, "
          "complements, url, source_html_link from Message "
          "left join MessageDict on Message.id=MessageDict.id "
          "where Message.id in ( ";
      }
      else
      {
        *load_msg_content_request_ostr << ", ";
      }
      
      *load_msg_content_request_ostr << id.data;
    }
    
  }
}
