/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/FeedManager/MessageFilter.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <Python.h>
#include <stdint.h>

#include <El/CORBA/Corba.hpp>

#include <string>
#include <sstream>
#include <fstream>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/Moment.hpp>

#include "MessageFilter.hpp"
#include "FeedManagerMain.hpp"
#include "FeedRecord.hpp"

namespace NewsGate
{
  namespace Moderation
  {    
    void
    MessageFilter::set_message_fetch_filter(
      const ::NewsGate::Moderation::MessageFetchFilterRuleSeq& rules)
      throw(NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      std::string filename;
      
      try
      {
        {
          ACE_Time_Value ctime = ACE_OS::gettimeofday();
        
          std::ostringstream ostr;
          ostr << Application::instance()->config().temp_dir()
               << "/FeedManager.cache."
               << El::Moment(ctime).dense_format() << "." << rand();

          filename = ostr.str();
        }

        std::fstream file(filename.c_str(), ios::out);

        if(!file.is_open())
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::MessageFilter::"
            "set_message_fetch_filter: failed to open file '" << filename
               << "' for write access";
        
          throw Exception(ostr.str());
        }

        bool first_line = true;
        
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();

        El::MySQL::Result_var qresult;
        
        for(size_t i = 0; i < rules.length(); i++)
        {
          const MessageFetchFilterRule& rule = rules[i];

          if(first_line)
          {
            first_line = false;
          }
          else
          {
            file << std::endl;
          }
          
          file << connection->escape_for_load(rule.expression.in())
               << "\t" << connection->escape_for_load(rule.description.in());
        }
        
        file.close();

        qresult = connection->query("begin");

        try
        {
          qresult = connection->query(
            "select update_num from MessageFilterUpdateNum for update");

          MessageFilterUpdateNum num(qresult.in());
            
          if(!num.fetch_row())
          {
            throw Exception(
              "NewsGate::Moderation::MessageFilter::"
              "set_message_fetch_filter: failed to get latest message "
              "filter update number");
          }

          uint64_t update_num = (uint64_t)num.update_num() + 1;

          {
            std::ostringstream ostr;
            ostr << "update MessageFilterUpdateNum set update_num="
                 << update_num;
              
            qresult = connection->query(ostr.str().c_str());
          }

          qresult = connection->query("delete from MessageFetchFilter");
          
          {
            std::ostringstream ostr; 
            ostr << "LOAD DATA INFILE '"
                 << connection->escape(filename.c_str())
                 << "' INTO TABLE MessageFetchFilter character set binary";
            
            qresult = connection->query(ostr.str().c_str());
          }
          
          qresult = connection->query("commit");
        }
        catch(...)
        {
          qresult = connection->query("rollback");
          throw;
        }
        
        unlink(filename.c_str());
      }
      catch(const El::Exception& e)
      {
        if(!filename.empty())
        {
          unlink(filename.c_str());
        }
        
        std::ostringstream ostr;
        ostr << "NewsGate::Moderation::MessageFilter::"
          "set_message_fetch_filter: El::Exception caught. Description:\n"
             << e;
        
        NewsGate::Moderation::ImplementationException ex;
        ex.description = ostr.str().c_str();
        
        throw ex;
      }      
    }
        
    ::NewsGate::Moderation::MessageFetchFilterRuleSeq*
    MessageFilter::get_message_fetch_filter()
      throw(NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      try
      {
        MessageFetchFilterRuleSeq_var rules = new MessageFetchFilterRuleSeq();

        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();

        El::MySQL::Result_var qresult = connection->query(
          "select expression, description from MessageFetchFilter");

        rules->length(qresult->num_rows());

        MessageFetchFilter record(qresult.in());

        for(size_t i = 0; record.fetch_row(); i++)
        {
          MessageFetchFilterRule& rule = rules[i];

          rule.expression = record.expression().c_str();
          
          rule.description = record.description().is_null() ? "" :
            record.description().c_str();
        }

        return rules._retn();
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Moderation::MessageFilter::"
          "get_message_fetch_filter: El::Exception caught. Description:\n"
             << e;
        
        NewsGate::Moderation::ImplementationException ex;
        ex.description = ostr.str().c_str();
        
        throw ex;
      }      
    }

    void
    MessageFilter::add_message_filter(
      const ::NewsGate::Moderation::MessageIdSeq& ids)
      throw(NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      std::string filename;

      try
      {
        {
          ACE_Time_Value ctime = ACE_OS::gettimeofday();
        
          std::ostringstream ostr;
          ostr << Application::instance()->config().temp_dir()
               << "/FeedManager.cache."
               << El::Moment(ctime).dense_format() << "." << rand();

          filename = ostr.str();
        }

        std::fstream file(filename.c_str(), ios::out);

        if(!file.is_open())
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Moderation::MessageFilter::"
            "add_message_filter: failed to open file '" << filename
               << "' for write access";
        
          throw Exception(ostr.str());
        }

        bool first_line = true;
        
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();

        El::MySQL::Result_var qresult;
        
        for(size_t i = 0; i < ids.length(); i++)
        {
          if(first_line)
          {
            first_line = false;
          }
          else
          {
            file << std::endl;
          }

          file << ids[i];
        }
        
        file.close();

        qresult = connection->query("begin");

        try
        {
          qresult = connection->query(
            "select update_num from MessageFilterUpdateNum for update");

          MessageFilterUpdateNum num(qresult.in());
            
          if(!num.fetch_row())
          {
            throw Exception(
              "NewsGate::Moderation::MessageFilter::"
              "add_message_filter: failed to get latest message "
              "filter update number");
          }

          uint64_t update_num = (uint64_t)num.update_num() + 1;

          {
            std::ostringstream ostr;
            ostr << "update MessageFilterUpdateNum set update_num="
                 << update_num;
              
            qresult = connection->query(ostr.str().c_str());
          }

          {
            std::ostringstream ostr; 
            ostr << "LOAD DATA INFILE '"
                 << connection->escape(filename.c_str())
                 << "' REPLACE INTO TABLE MessageFilter character set binary";
            
            qresult = connection->query(ostr.str().c_str());
          }
          
          qresult = connection->query("commit");
        }
        catch(...)
        {
          qresult = connection->query("rollback");
          throw;
        }
        
        unlink(filename.c_str());
      }
      catch(const El::Exception& e)
      {
        if(!filename.empty())
        {
          unlink(filename.c_str());
        }
        
        std::ostringstream ostr;
        ostr << "NewsGate::Moderation::MessageFilter::"
          "add_message_filter: El::Exception caught. Description:\n"
             << e;
        
        NewsGate::Moderation::ImplementationException ex;
        ex.description = ostr.str().c_str();
        
        throw ex;
      }
    }
    
  }  
}
