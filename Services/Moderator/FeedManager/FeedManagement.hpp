/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/FeedManager/FeedManagement.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MODERATOR_FEEDMANAGER_FEEDMANAGEMENT_HPP_
#define _NEWSGATE_SERVER_SERVICES_MODERATOR_FEEDMANAGER_FEEDMANAGEMENT_HPP_

#include <string>
#include <vector>
#include <list>
#include <iostream>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/String/Manip.hpp>
#include <El/MySQL/DB.hpp>

#include <Services/Moderator/Commons/FeedManager.hpp>

namespace NewsGate
{
  namespace Moderation
  {
    class FeedManagement
    {
    public:
      EL_EXCEPTION(Exception, El::ExceptionBase);

    public:
      
      ::NewsGate::Moderation::FeedInfoResult*
      feed_info_range(::CORBA::ULong start_from,
                      ::CORBA::ULong results,
                      ::CORBA::Boolean get_stat,
                      ::CORBA::ULong stat_from_date,
                      ::CORBA::ULong stat_to_date,
                      const ::NewsGate::Moderation::SortInfo& sort,
                      const ::NewsGate::Moderation::FilterInfo& filter)
        throw(NewsGate::Moderation::FilterRuleError,
              NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);
      
      ::NewsGate::Moderation::FeedInfoSeq* feed_info_seq(
        const ::NewsGate::Moderation::FeedIdSeq& ids,
        ::CORBA::Boolean get_stat,
        ::CORBA::ULong stat_from_date,
        ::CORBA::ULong stat_to_date)
        throw(NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);

      void feed_update_info(
        const ::NewsGate::Moderation::FeedUpdateInfoSeq& feed_infos)
        throw(NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);
      
      struct FeedColumn
      {
        FieldSelector id;
        const char* column_name;
        const char* column_full_name;
        unsigned long flags;

        FieldSelector spec(std::ostream& ostr,
                           bool& first_col,
                           const char* table_name = 0,
                           bool use_full_name = true) const
          throw(El::Exception);

        const char* table_name() const throw(El::Exception);
      };      
      
    private:
      
      std::string order_specification(
        const ::NewsGate::Moderation::SortInfo& sort,
        size_t& sort_column_index)
        throw(Exception, El::Exception);

      enum FieldTable
      {
        FT_FEED = 0x1,
        FT_STATE = 0x2,
        FT_STAT = 0x4,
        FT_PERF_STAT = 0x8,
        FT_HIDDEN = 0x10,
        FT_HAVING = 0x20
      };

      struct FilterSpec
      {
        std::string where;
        std::string having;

        FilterSpec(const char* w = 0, const char* h = 0) throw(El::Exception);
      };
      
      FilterSpec filter_specification(
        const ::NewsGate::Moderation::FilterInfo& filter,
        El::MySQL::Connection* connection,
        unsigned long table_mask = FT_FEED | FT_STATE | FT_STAT | FT_PERF_STAT)
        throw(Exception, NewsGate::Moderation::FilterRuleError, El::Exception);

      typedef std::vector<std::string> FeedIdArray;

      size_t get_feed_range(
        El::MySQL::Connection* connection,
        size_t start_from,
        size_t results,
        const char* stat_from,
        const char* stat_to,
        const ::NewsGate::Moderation::SortInfo& sort,
        size_t sort_column_index,
        const char* order_spec,
        const ::NewsGate::Moderation::FilterInfo& filter,
        const FilterSpec& filter_spec,
        FeedIdArray& feed_ids,
        ACE_Time_Value& time)
        throw(Exception, El::Exception);
      
      void feeds_from_range(El::MySQL::Connection* connection,
                            const FeedIdArray& feed_ids,
                            bool get_stat,
                            const char* stat_from,
                            const char* stat_to,
                            const char* order_spec,
                            FeedInfoSeq& feed_infos,
                            ACE_Time_Value& time)
        throw(Exception, El::Exception);

      static const FeedColumn& column(FieldSelector id)
        throw(Exception, El::Exception);
      
      static const FeedColumn feed_columns_[];
    };
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Moderation
  {
    //
    // NewsGate::Moderation::FeedManagement::FilterSpec
    //
    inline
    FeedManagement::FilterSpec::FilterSpec(const char* w, const char* h)
      throw(El::Exception)
        : where(w ? w : ""),
          having(h ? h : "")
    {
    }
    
    //
    // NewsGate::Moderation::FeedManagement::FeedColumn
    //
    inline
    FieldSelector
    FeedManagement::FeedColumn::spec(std::ostream& ostr,
                                     bool& first_col,
                                     const char* new_table_name,
                                     bool use_full_name) const
      throw(El::Exception)
    {
      ostr << (first_col ? "" : ",") << " ";

      if(use_full_name)
      {
        if(new_table_name && *new_table_name != '\0')
        {
          El::String::Manip::replace(column_full_name,
                                     table_name(),
                                     new_table_name,
                                     ostr);
          
          ostr << " as " << column_name;
        }
        else
        {
          ostr << column_full_name << " as " << column_name;
        }
      }
      else
      {
        if(new_table_name && *new_table_name != '\0')
        {
          ostr << new_table_name << ".";
        }

        ostr << column_name;
      }
      
      first_col = false;
      return id;
    }

    inline
    const char*
    FeedManagement::FeedColumn::table_name() const throw(El::Exception)
    {
      return (flags & FT_FEED) ? "Feed" :
        ((flags & FT_STATE) ? "RSSFeedState" : ((flags & FT_STAT) ?
                                                "RSSFeedStat" : "StatFeed"));
    }    
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_MODERATOR_FEEDMANAGER_FEEDMANAGEMENT_HPP_
