/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/FeedManager/FeedManagement.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <Python.h>
#include <stdint.h>

#include <El/CORBA/Corba.hpp>

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <set>

#include <ace/OS.h>
#include <mysql/mysqld_error.h>

#include <El/Exception.hpp>
#include <El/Moment.hpp>
#include <El/MySQL/DB.hpp>
#include <El/String/Manip.hpp>

#include <xsd/ConfigParser.hpp>

#include "FeedManagement.hpp"
#include "FeedManagerMain.hpp"
#include "FeedRecord.hpp"

namespace Aspect
{
  const char FEED_STAT[] = "FeedStat";
}

namespace NewsGate
{
  namespace Moderation
  {
    const FeedManagement::FeedColumn
    FeedManagement::feed_columns_[] =
    {
      { FS_ID, "id", "Feed.id", FT_FEED },
      { FS_TYPE, "type", "Feed.type", FT_FEED },
      { FS_URL, "url", "Feed.url", FT_FEED },
      { FS_ENCODING, "encoding", "Feed.encoding", FT_FEED },
      { FS_SPACE, "space", "Feed.space", FT_FEED },
      { FS_LANG, "lang", "Feed.lang", FT_FEED },
      { FS_COUNTRY, "country", "Feed.country", FT_FEED },      
      { FS_STATUS, "status", "Feed.status", FT_FEED },
      { FS_CREATOR, "creator", "Feed.creator", FT_FEED },
      { FS_CREATOR_TYPE, "creator_type", "Feed.creator_type", FT_FEED },
      { FS_KEYWORDS, "keywords", "ifnull(Feed.keywords, '')", FT_FEED },
      { FS_ADJUSTMENT_SCRIPT, "adjustment_script",
        "ifnull(Feed.adjustment_script, '')", FT_FEED
      },
      { FS_COMMENT, "comment", "ifnull(Feed.comment, '')", FT_FEED },
      { FS_CREATED, "created", "Feed.created", FT_FEED },
      { FS_UPDATED, "updated", "Feed.updated", FT_FEED },

      // For filtering purposes only
      { FS_STATE_ID,
        "state_feed_id",
        "RSSFeedState.feed_id",
        FT_STATE | FT_HIDDEN
      },
      
      { FS_CHANNEL_TITLE,
        "channel_title",
        "ifnull(RSSFeedState.channel_title, '')",
        FT_STATE },
      
      { FS_CHANNEL_DESCRIPTION,
        "channel_description",
        "ifnull(RSSFeedState.channel_description, '')",
        FT_STATE
      },
      
      { FS_CHANNEL_HTML_LINK,
        "channel_html_link",
        "ifnull(RSSFeedState.channel_html_link, '')",
        FT_STATE
      },
      
      { FS_CHANNEL_LANG,
        "channel_lang",
        "ifnull(RSSFeedState.channel_lang, 0)",
        FT_STATE },
      
      { FS_CHANNEL_COUNTRY,
        "channel_country",
        "ifnull(RSSFeedState.channel_country, 0)",
        FT_STATE
      },
      
      { FS_CHANNEL_TTL,
        "channel_ttl",
        "ifnull(RSSFeedState.channel_ttl, 0)",
        FT_STATE
      },
      
      { FS_CHANNEL_LAST_BUILD_DATE,
        "channel_last_build_date",
        "ifnull(RSSFeedState.channel_last_build_date, '1970-01-01 00:00:00')",
        FT_STATE
      },
      
      { FS_LAST_REQUEST_DATE,
        "last_request_date",
        "ifnull(RSSFeedState.last_request_date, '1970-01-01 00:00:00')",
        FT_STATE
      },
      
      { FS_LAST_MODIFIED_HDR,
        "last_modified_hdr",
        "ifnull(RSSFeedState.last_modified_hdr, '')",
        FT_STATE
      },
      
      { FS_ETAG_HDR,
        "etag_hdr",
        "ifnull(RSSFeedState.etag_hdr, '')",
        FT_STATE
      },
      
      { FS_CONTENT_LENGTH_HDR,
        "content_length_hdr",
        "ifnull(RSSFeedState.content_length_hdr, 0)",
        FT_STATE
      },
      
      { FS_ENTROPY, "entropy", "ifnull(RSSFeedState.entropy, 0)", FT_STATE },
      
      { FS_ENTROPY_UPDATED_DATE,
        "entropy_updated_date",
        "ifnull(RSSFeedState.entropy_updated_date, '1970-01-01 00:00:00')",
        FT_STATE
      },
      
      { FS_SIZE, "size", "ifnull(RSSFeedState.size, 0)", FT_STATE },
      
      { FS_SINGLE_CHUNKED,
        "single_chunked",
        "ifnull(RSSFeedState.single_chunked, 0)",
        FT_STATE
      },
      
      { FS_FIRST_CHUNK_SIZE,
        "first_chunk_size",
        "ifnull(RSSFeedState.first_chunk_size, -1)",
        FT_STATE
      },
      
      { FS_HEURISTICS_COUNTER,
        "heuristics_counter",
        "ifnull(RSSFeedState.heuristics_counter, -1000000000)",
        FT_STATE
      },
      
      // For filtering purposes only
      { FS_STAT_ID,
        "stat_feed_id",
        "RSSFeedStat.feed_id",
        FT_STAT | FT_HIDDEN
      },
      
      { FS_REQUESTS,
        "requests",
        "ifnull(sum(RSSFeedStat.requests), 0)",
        FT_STAT | FT_HAVING
      },
      
      { FS_FAILED,
        "failed",
        "ifnull(sum(RSSFeedStat.failed), 0)",
        FT_STAT | FT_HAVING
      },
      
      { FS_UNCHANGED,
        "unchanged",
        "ifnull(sum(RSSFeedStat.unchanged), 0)",
        FT_STAT | FT_HAVING
      },
      
      { FS_NOT_MODIFIED,
        "not_modified",
        "ifnull(sum(RSSFeedStat.not_modified), 0)",
        FT_STAT | FT_HAVING
      },
      
      { FS_PRESUMABLY_UNCHANGED,
        "presumably_unchanged",
        "ifnull(sum(RSSFeedStat.presumably_unchanged), 0)",
        FT_STAT | FT_HAVING
      },
      
      { FS_HAS_CHANGES,
        "has_changes",
        "ifnull(sum(RSSFeedStat.has_changes), 0)",
        FT_STAT | FT_HAVING
      },
      
      { FS_WASTED,
        "wasted",
        "ifnull(sum(RSSFeedStat.wasted), 0)",
        FT_STAT | FT_HAVING
      },
      
      { FS_OUTBOUND,
        "outbound",
        "ifnull(sum(RSSFeedStat.outbound), 0)",
        FT_STAT | FT_HAVING
      },
      
      { FS_INBOUND,
        "inbound",
        "ifnull(sum(RSSFeedStat.inbound), 0)",
        FT_STAT | FT_HAVING
      },
      
      { FS_REQUESTS_DURATION,
        "requests_duration",
        "ifnull(sum(RSSFeedStat.requests_duration), 0)",
        FT_STAT | FT_HAVING
      },
      
      { FS_MESSAGES,
        "messages",
        "ifnull(sum(RSSFeedStat.messages), 0)",
        FT_STAT | FT_HAVING
      },
      
      { FS_MESSAGES_SIZE,
        "messages_size",
        "ifnull(sum(RSSFeedStat.messages_size), 0)",
        FT_STAT | FT_HAVING
      },
      
      { FS_MESSAGES_DELAY,
        "messages_delay",
        "ifnull(sum(RSSFeedStat.messages_delay), 0)",
        FT_STAT | FT_HAVING
      },

      { FS_MAX_MESSAGE_DELAY,
        "max_message_delay",
        "ifnull(max(RSSFeedStat.max_message_delay), 0)",
        FT_STAT | FT_HAVING
      },

      // For filtering purposes only
      { FS_PERF_STAT_ID,
        "perf_stat_feed_id",
        "StatFeed.feed_id",
        FT_PERF_STAT | FT_HIDDEN
      },
      
      { FS_MSG_IMPRESSIONS,
        "msg_impressions",
        "ifnull(sum(StatFeed.msg_impressions), 0)",
        FT_PERF_STAT | FT_HAVING
      },
      
      { FS_MSG_CLICKS,
        "msg_clicks",
        "ifnull(sum(StatFeed.msg_clicks), 0)",
        FT_PERF_STAT | FT_HAVING
      },
      
      { FS_MSG_CTR,
        "msg_ctr",
        "if(sum(StatFeed.msg_impressions) > 0, "
        "ifnull(sum(StatFeed.msg_clicks) * 10000 / "
        "sum(StatFeed.msg_impressions), 0), 0)",
        FT_PERF_STAT | FT_HAVING
      }
    };

    const FeedManagement::FeedColumn&
    FeedManagement::column(FieldSelector id)
      throw(Exception, El::Exception)
    {
      const size_t len = sizeof(feed_columns_) /
        sizeof(feed_columns_[0]);
      
      size_t i = 0;
      
      for(; i < len && feed_columns_[i].id != id; i++);
      
      if(i == len)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Moderation::FeedManagement::find_feed_column: "
          "unexpected filter rule field " << id;
        
        throw Exception(ostr.str());
      }

      return feed_columns_[i];
    }
    
    std::string
    FeedManagement::order_specification(
      const ::NewsGate::Moderation::SortInfo& sort,
      size_t& sort_column_index)
      throw(Exception, El::Exception)
    {
      FieldSelector sort_field =
        sort.field == FS_NONE ? FS_ID : sort.field;

      const FeedManagement::FeedColumn& col = column(sort_field);
      sort_column_index = &col - feed_columns_;
      
      std::ostringstream ostr;
      
      ostr << " order by " << col.column_name << " " <<
        (sort.descending ? "DESC" : "ASC") << " ";
      
      return ostr.str();
    }

    FeedManagement::FilterSpec
    FeedManagement::filter_specification(
      const ::NewsGate::Moderation::FilterInfo& filter,
      El::MySQL::Connection* connection,
      unsigned long table_mask)
      throw(Exception, NewsGate::Moderation::FilterRuleError, El::Exception)
    {
      const FilterRuleSeq& filter_rules = filter.rules;
            
      if(filter_rules.length() == 0)
      {
        return FilterSpec();
      }

      bool where_first = true;
      bool having_first = true;
      
      std::ostringstream where_ostr;
      std::ostringstream having_ostr;
            
      for(size_t i = 0; i < filter_rules.length(); ++i)
      {
        const FilterRule& rule = filter_rules[i];
        const FeedColumn& feed_column = column(rule.field);

        if((table_mask & feed_column.flags) == 0)
        {
          continue;
        }

        std::ostringstream& ostr = feed_column.flags & FT_HAVING ?
          having_ostr : where_ostr;

        bool& first = feed_column.flags & FT_HAVING ?
          having_first : where_first;
        
        if(first)
        {
          first = false;
        }
        else
        {
          ostr << " and";
        }
        
        ostr << " ( ";

        const char* col = feed_column.flags & FT_HAVING ?
          feed_column.column_name : feed_column.column_full_name;

        switch(rule.operation)
        {
        case FO_NOT_LIKE:
        case FO_LIKE:
          {
            if(rule.args.length() < 1)
            {              
              NewsGate::Moderation::FilterRuleError e;
              
              e.id = rule.id;
              e.description = "no regexps provided";
              throw e;
            }

            ostr << "( ";
            
            for(size_t i = 0; i < rule.args.length(); ++i)
            {
              if(i)
              {
                ostr << (rule.operation == FO_NOT_LIKE ? "AND " : "OR ");
              }
              
              ostr << col << (rule.operation == FO_NOT_LIKE ? " NOT" : "")
                   << " LIKE '" << connection->escape(rule.args[i]) << "' ";
            }
            
            ostr << " )";
                    
            break;
          }
        case FO_NOT_REGEXP:
        case FO_REGEXP:
          {
            if(rule.args.length() < 1)
            {              
              NewsGate::Moderation::FilterRuleError e;
              
              e.id = rule.id;
              e.description = "no regexps provided";
              throw e;
            }

            ostr << "( ";
            
            for(size_t i = 0; i < rule.args.length(); ++i)
            {
              std::string regexp;
              
              {
                std::ostringstream ostr;
                ostr << " REGEXP '" << connection->escape(rule.args[i])
                     << "' ";
                
                regexp = ostr.str();
              }
            
              std::string column_name = column(rule.field).column_name;
            
              std::ostringstream test_expr;
              
              test_expr << "select " << column_name
                        << " from Feed left join RSSFeedState on "
                "id=RSSFeedState.feed_id where " << column_name << " REGEXP '"
                        << connection->escape(rule.args[i]) << "'  LIMIT 1";
              
              try
              {
                El::MySQL::Result_var qresult =
                  connection->query(test_expr.str().c_str());
              }
              catch(const El::MySQL::Exception&)
              {
                unsigned int error = mysql_errno(connection->mysql());
                
                switch(error)
                {
                case ER_REGEXP_ERROR:
                  {
                    NewsGate::Moderation::FilterRuleError e;
                    
                    e.id = rule.id;
                    e.description = mysql_error(connection->mysql());
                    
                    throw e;
                  }
                case ER_CANT_AGGREGATE_2COLLATIONS:
                  {
                    std::ostringstream ostr;
                    
                    ostr << mysql_error(connection->mysql())
                         << ".\nEnsure that only ASCII characters present.";
                    
                    NewsGate::Moderation::FilterRuleError e;
                    
                    e.id = rule.id;
                    e.description = ostr.str().c_str();
                    
                    throw e;
                  }                
                }
                
                throw;
              }

              if(i)
              {
                ostr << (rule.operation == FO_NOT_REGEXP ? "AND " : "OR ");
              }
              
              ostr << col << (rule.operation == FO_NOT_REGEXP ? " NOT" : "")
                   << regexp;
            }

            ostr << " )";
            break;
          }
        case FO_EQ:
        case FO_NE:
        case FO_LT:
        case FO_LE:
        case FO_GT:
        case FO_GE:
          {
            if(rule.args.length() != 1)
            {
              NewsGate::Moderation::FilterRuleError e;
              
              e.id = rule.id;
              e.description = "single value expected";
              throw e;
            }

            bool is_date = false;
            
            switch(rule.field)
            {
            case FS_CREATED:
            case FS_UPDATED:
            case FS_CHANNEL_LAST_BUILD_DATE:
            case FS_LAST_REQUEST_DATE:
            case FS_ENTROPY_UPDATED_DATE: is_date = true; break;
            default: is_date = false; break;
            }

            std::string value;
            
            if(is_date)
            {
              size_t today =
                ACE_OS::gettimeofday().sec() / 86400 * 86400;

              const char* rule_arg = rule.args[0];
              
              switch(rule_arg[0])
              {
              case 't':
                {
                  value =
                    El::Moment(ACE_Time_Value(today)).iso8601(false, false);
                  
                  break;
                }
              case 'y':
                {
                  value =
                    El::Moment(ACE_Time_Value(today - 86400)).
                    iso8601(false, false);
                  
                  break;
                }
              case 'w':
                {
                  value =
                    El::Moment(ACE_Time_Value(today - 86400 * 7)).
                    iso8601(false, false);
                  
                  break;
                }
              case 'm':
                {
                  value =
                    El::Moment(ACE_Time_Value(today - 86400 * 30)).
                    iso8601(false, false);
                  
                  break;
                }
              case 'r':
                {
                  value =
                    El::Moment(ACE_Time_Value(today - 86400 * 365)).
                    iso8601(false, false);
                  
                  break;
                }
              default:
                {
                  value = rule.args[0];
                  break;
                }
              }
                
              ostr << "to_days(" << col << ")";
            }
            else
            {            
              if(rule.field == FS_WASTED)
              {
                double num_value = 0;
                if(!El::String::Manip::numeric(rule.args[0], num_value))
                {
                  NewsGate::Moderation::FilterRuleError e;
                  
                  e.id = rule.id;
                  e.description = "value of type double expected";
                  
                  throw e;
                }

                value  = El::String::Manip::string(num_value);
              }
              else
              {
                double num_value = 0;
                
                if(!El::String::Manip::numeric(rule.args[0], num_value))
                {
                  NewsGate::Moderation::FilterRuleError e;
                  
                  e.id = rule.id;
                  e.description = "numeric value expected";
                  
                  throw e;
                }

                value  = El::String::Manip::string(num_value);
              }
                  
              ostr << col;
            }

            switch(rule.operation)
            {
            case FO_EQ: ostr << "="; break;
            case FO_NE: ostr << "!="; break;
            case FO_LT: ostr << "<"; break;
            case FO_LE: ostr << "<="; break;
            case FO_GT: ostr << ">"; break;
            case FO_GE: ostr << ">="; break;
            default: break;
            }
        
            if(is_date)
            {
              ostr << "to_days('" << value << "') ";
            }
            else
            {
              ostr << value << " ";
            }
            
            break;
          }
        case FO_NONE_OF:
        case FO_ANY_OF:
          {
            if(rule.args.length() == 0)
            {
              ostr << (rule.operation == FO_ANY_OF ? "false" : "true")
                   << " ";
              
              break;
            }

            size_t have_null = 0;
            
            for(size_t i = 0; i < rule.args.length(); i++)
            {
              if(strcasecmp(rule.args[i], "null") == 0)
              {
                have_null++;
              }
            }

            if(have_null)
            {
              ostr << col << " IS" <<
                (rule.operation == FO_NONE_OF ? " NOT" : "") << " null ";
            }

            if(have_null < rule.args.length())
            {
              if(have_null)
              {
                ostr << (rule.operation == FO_ANY_OF ? "or " : "and ");
              }
            
              ostr << col << (rule.operation == FO_NONE_OF ? " NOT" : "")
                   << " IN ( ''";

              bool first = true;
              
              for(size_t i = 0; i < rule.args.length(); i++)
              {
                if(strcasecmp(rule.args[i], "null") == 0)
                {
                  continue;
                }
                
                if(first)
                {
                  first = false;
                }
                else
                {
                  ostr << ",";
                }
              
                ostr << " '" << connection->escape(rule.args[i]) << "'";
              }

              ostr << " ) ";
            }
            
            break;
          }
        default:
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Moderation::FeedManagement::"
              "filter_specification: unexpected operation " << rule.operation;
                    
            throw Exception(ostr.str());
          }
        }

        ostr << ") ";
      }

      return FilterSpec(where_ostr.str().c_str(), having_ostr.str().c_str());
    }

    size_t
    FeedManagement::get_feed_range(
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
      throw(Exception, El::Exception)
    {
      typedef std::set<FieldSelector> FieldSelectorSet;
      
      ACE_High_Res_Timer timer;
      timer.start();
 
      const FeedColumn& sort_feed_column = feed_columns_[sort_column_index];
      unsigned long table_mask = sort_feed_column.flags;
      
      const FilterRuleSeq& filter_rules = filter.rules;

      for(size_t i = 0; i < filter_rules.length(); i++)
      {
        const FeedColumn& feed_column =
          column(filter_rules[i].field);

        table_mask |= feed_column.flags;
      }

      if(table_mask == 0)
      {
        throw Exception(
          "NewsGate::Moderation::FeedManagement::get_feed_range: "
          "table mask unexpectedly empty");
      }

      bool first_col = true;
      size_t table_num = 0;
      std::string master_id_col_name;
      
      std::ostringstream query_ostr;
      std::ostringstream table_ref_ostr;

      query_ostr << "select";

//      if(table_mask & FT_FEED)
      {
        FieldSelectorSet selected_fields;
        const FeedColumn& id_column = column(FS_ID);

        selected_fields.insert(id_column.spec(query_ostr, first_col));
        
        if(table_mask & FT_FEED)
        {
          if((sort_feed_column.flags & FT_FEED) &&
             selected_fields.find(sort_feed_column.id) ==
             selected_fields.end())
          {
            selected_fields.insert(
              sort_feed_column.spec(query_ostr, first_col));
          }

          for(size_t i = 0; i < filter_rules.length(); i++)
          {
            const FeedColumn& feed_column = column(
              filter_rules[i].field);
            
            if((FT_FEED & feed_column.flags) &&
               selected_fields.find(feed_column.id) == selected_fields.end())
            {
              selected_fields.insert(
                feed_column.spec(query_ostr, first_col));
            }
          }
        }
        
        if(table_num++)
        {
          table_ref_ostr << " join " << id_column.table_name() << " on "
                         << id_column.column_full_name << "="
                         << master_id_col_name;
        }
        else
        {
          table_ref_ostr << " from " << id_column.table_name();
          master_id_col_name = id_column.column_full_name;
        }
      }

      if(table_mask & FT_STATE)
      {
        FieldSelectorSet selected_fields;
        const FeedColumn& id_column = column(FS_STATE_ID);
        
        if(!table_num)
        {
          selected_fields.insert(id_column.spec(query_ostr, first_col));
        }

        if(sort_feed_column.flags & FT_STATE)
        {          
          selected_fields.insert(
            sort_feed_column.spec(query_ostr, first_col));
        }

        for(size_t i = 0; i < filter_rules.length(); i++)
        {
          const FeedColumn& feed_column = column(
            filter_rules[i].field);

          if((FT_STATE & feed_column.flags) &&
             selected_fields.find(feed_column.id) == selected_fields.end())
          {
            selected_fields.insert(
              feed_column.spec(query_ostr, first_col));
          }
        }

        if(table_num++)
        {
          table_ref_ostr << " left join " << id_column.table_name() << " on "
                         << id_column.column_full_name << "="
                         << master_id_col_name;
        }
        else
        {
          table_ref_ostr << " from " << id_column.table_name();
          master_id_col_name = id_column.column_full_name;
        }
      }

      if(table_mask & FT_STAT)
      {
        FieldSelectorSet selected_fields;
        const FeedColumn& id_column = column(FS_STAT_ID);
        
        if(!table_num)
        {
          selected_fields.insert(id_column.spec(query_ostr, first_col));
        }

        if(sort_feed_column.flags & FT_STAT)
        {          
          selected_fields.insert(
            sort_feed_column.spec(query_ostr, first_col));
        }

        for(size_t i = 0; i < filter_rules.length(); i++)
        {
          const FeedColumn& feed_column = column(
            filter_rules[i].field);

          if((FT_STAT & feed_column.flags) &&
             selected_fields.find(feed_column.id) == selected_fields.end())
          {
            selected_fields.insert(
              feed_column.spec(query_ostr, first_col));
          }
        }

        std::string table_name = id_column.table_name();
        
        if(table_num++)
        {
          table_ref_ostr << " left join " << table_name << " on "
                         << id_column.column_full_name << "="
                         << master_id_col_name << " and " << table_name
                         << ".date>='" << stat_from << "' and " << table_name
                         << ".date<='" << stat_to << "'";
        }
        else
        {
          table_ref_ostr << " from " << table_name;
          master_id_col_name = id_column.column_full_name;
        }
      }

      if(table_mask & FT_PERF_STAT)
      {
        FieldSelectorSet selected_fields;
        const FeedColumn& id_column = column(FS_PERF_STAT_ID);
        
        if(!table_num)
        {
          selected_fields.insert(id_column.spec(query_ostr, first_col));
        }

        if(sort_feed_column.flags & FT_PERF_STAT)
        {          
          selected_fields.insert(
            sort_feed_column.spec(query_ostr, first_col));
        }

        for(size_t i = 0; i < filter_rules.length(); i++)
        {
          const FeedColumn& feed_column = column(
            filter_rules[i].field);

          if((FT_PERF_STAT & feed_column.flags) &&
             selected_fields.find(feed_column.id) == selected_fields.end())
          {
            selected_fields.insert(
              feed_column.spec(query_ostr, first_col));
          }
        }

        std::string table_name = id_column.table_name();
        
        if(table_num++)
        {
          table_ref_ostr << " left join " << table_name << " on "
                         << id_column.column_full_name << "="
                         << master_id_col_name << " and " << table_name
                         << ".date>='" << stat_from << "' and " << table_name
                         << ".date<='" << stat_to << "'";
        }
        else
        {
          table_ref_ostr << " from " << table_name;
          master_id_col_name = id_column.column_full_name;
        }
      }

      std::string tref = table_ref_ostr.str();
      query_ostr << tref;

      if(!filter_spec.where.empty())
      {
        query_ostr << " where" << filter_spec.where;
      }

      if(table_mask & (FT_STAT | FT_PERF_STAT))
      {
        query_ostr << " group by " << master_id_col_name;
      }

      if(!filter_spec.having.empty())
      {
        query_ostr << " having" << filter_spec.having;
      }
      
      query_ostr << order_spec;
      
      std::string query = query_ostr.str();

      std::cerr << "Filtering:\n" << query << std::endl << std::endl;

      El::MySQL::Result_var qres  = connection->query(query.c_str());
      size_t rows = qres->num_rows();

      El::MySQL::Row record(qres.in());
      feed_ids.reserve(results);
      
      record.data_seek(start_from);
            
      for(size_t i = 0; i < results && record.fetch_row(); i++)
      {
        feed_ids.push_back(record.string(0));
      }      
      
      timer.stop();
      timer.elapsed_time(time);
      
      return rows;
    }

    void
    FeedManagement::feed_update_info(
      const ::NewsGate::Moderation::FeedUpdateInfoSeq& feed_infos)
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
          ostr << "NewsGate::Moderation::FeedManagement::feed_update_info: "
            "failed to open file '" << filename << "' for write access";
        
          throw Exception(ostr.str());
        }

        bool first_line = true;
        FeedIdArray delete_feeds;
        
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();
        
        for(size_t i = 0; i < feed_infos.length(); i++)
        {
          const FeedUpdateInfo& fi = feed_infos[i];

          if(fi.status == 'L')
          {
            delete_feeds.push_back(El::String::Manip::string(fi.id));
          }

          if(first_line)
          {
            first_line = false;
          }
          else
          {
            file << std::endl;
          }
          
          file << fi.id << "\t\t\t"
               << connection->escape_for_load(fi.encoding.in()) << "\t"
               << fi.space << "\t" << fi.lang << "\t" << fi.country
               << "\t\t0\t\t\t\t" << fi.status << "\t"
               << connection->escape_for_load(fi.keywords.in())
               << "\t"
               << connection->escape_for_load(fi.adjustment_script.in())
               << "\t" << connection->escape_for_load(fi.comment.in());
        }

        file.close();
        
        El::MySQL::Result_var result = connection->query("begin");
        
        try
        {
          result = connection->query(
            "select update_num from FeedUpdateNum for update");
          
          FeedUpdateNum feeds_stamp(result.in());
            
          if(!feeds_stamp.fetch_row())
          {
            throw Exception(
              "NewsGate::Moderation::FeedManagement::feed_update_info: "
              "failed to get latest feed update number");
          }

          uint64_t update_num = (uint64_t)feeds_stamp.update_num() + 1;

          {
            std::ostringstream ostr;
            ostr << "update FeedUpdateNum set update_num=" << update_num;
              
            result = connection->query(ostr.str().c_str());
          }
          
          result = connection->query("delete from FeedUpdateBuff");

          std::string query =
            std::string("LOAD DATA INFILE '") +
            connection->escape(filename.c_str()) +
            "' REPLACE INTO TABLE FeedUpdateBuff character set binary";
      
          result = connection->query(query.c_str());

          unlink(filename.c_str());

          std::ostringstream ostr;
          ostr << "INSERT INTO Feed SELECT * FROM FeedUpdateBuff "
            "ON DUPLICATE KEY UPDATE encoding = encoding_, space = space_, "
            "lang = lang_, country = country_, status = status_, "
            "keywords = keywords_, adjustment_script = adjustment_script_, "
            "comment = comment_, update_num=" << update_num;

          result = connection->query(ostr.str().c_str());

          if(!delete_feeds.empty())
          {
            std::ostringstream ostr;
            ostr << " where feed_id in (";

            for(FeedIdArray::const_iterator it = delete_feeds.begin();
                it != delete_feeds.end(); it++)
            {
              ostr << (it == delete_feeds.begin() ? " " : ", ") << *it;
            }

            ostr << " )";

            std::string qpart = ostr.str();
            
            result = connection->query((std::string("delete from RSSFeedStat")
                                        + qpart.c_str()).c_str());

            result = connection->query((std::string("delete from RSSFeedState")
                                        + qpart.c_str()).c_str());
          }
          
          result = connection->query("commit");
        }
        catch(...)
        {
          result = connection->query("rollback");
          throw;
        }

        result = connection->query("begin");
        
        try
        {
          result = connection->query(
            "select update_num from MessageFilterUpdateNum for update");

          MessageFilterUpdateNum num(result.in());
            
          if(!num.fetch_row())
          {
            throw Exception(
              "NewsGate::Moderation::FeedManagement::feed_update_info: "
              "failed to get latest message filter update number");
          }

          uint64_t update_num = (uint64_t)num.update_num() + 1;

          {
            std::ostringstream ostr;
            ostr << "update MessageFilterUpdateNum set update_num="
                 << update_num;
              
            result = connection->query(ostr.str().c_str());
          }
          
          result = connection->query("commit");
        }
        catch(...)
        {
          result = connection->query("rollback");
          throw;
        }
      }
      catch(const El::Exception& e)
      {
        if(!filename.empty())
        {
          unlink(filename.c_str());
        }
        
        std::ostringstream ostr;
        ostr << "NewsGate::Moderation::FeedManagement::feed_update_info: "
          "El::Exception caught. Description: " << e;
        
        NewsGate::Moderation::ImplementationException ex;
        ex.description = ostr.str().c_str();
        
        throw ex;
      }
    }
    
    ::NewsGate::Moderation::FeedInfoSeq*
    FeedManagement::feed_info_seq(
      const ::NewsGate::Moderation::FeedIdSeq& ids,
      ::CORBA::Boolean get_stat,
      ::CORBA::ULong stat_from_date,
      ::CORBA::ULong stat_to_date)
      throw(NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      try
      {
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();

        El::MySQL::Result_var qresult;
          
        std::string stat_from =
          El::Moment(ACE_Time_Value(std::min(stat_from_date,
                                             stat_to_date))).iso8601(false);
          
        std::string stat_to =
          El::Moment(ACE_Time_Value(std::max(stat_from_date,
                                             stat_to_date))).iso8601(false);
          
        FeedIdArray feed_ids;
        feed_ids.reserve(ids.length());

        for(size_t i = 0; i < ids.length(); i++)
        {
          feed_ids.push_back(El::String::Manip::string(ids[i]));
        }
          
        FeedInfoSeq_var feed_infos = new FeedInfoSeq();
          
        ACE_Time_Value time;
          
        feeds_from_range(connection,
                         feed_ids,
                         get_stat,
                         stat_from.c_str(),
                         stat_to.c_str(),
                         0,
                         feed_infos,
                         time);          

        std::ostringstream ostr;
          
        ostr << "FeedManagement::feed_info_seq: feeds_from_range for "
             << feed_ids.size() << " feeds, get stat " << (int)get_stat
             << ", dates [" << stat_from << ", " << stat_to
             << "] for " << El::Moment::time(time);
          
        Application::logger()->trace(ostr.str().c_str(),
                                     Aspect::FEED_STAT,
                                     El::Logging::HIGH);
          
        return feed_infos._retn();
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Moderation::FeedManagement::feed_info: "
          "El::Exception caught. Description: " << e;
        
        NewsGate::Moderation::ImplementationException ex;
        ex.description = ostr.str().c_str();
        
        throw ex;
      }     
    }

    ::NewsGate::Moderation::FeedInfoResult*
    FeedManagement::feed_info_range(
      ::CORBA::ULong start_from,
      ::CORBA::ULong results,
      ::CORBA::Boolean get_stat,
      ::CORBA::ULong stat_from_date,
      ::CORBA::ULong stat_to_date,
      const ::NewsGate::Moderation::SortInfo& sort,
      const ::NewsGate::Moderation::FilterInfo& filter)
      throw(NewsGate::Moderation::FilterRuleError,
            NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      try
      {
        El::MySQL::Connection_var connection;
        
//        bool lock_stat = get_stat;
          
        size_t sort_column_index = 0;
          
        std::string order_spec =
          order_specification(sort, sort_column_index);

        if(feed_columns_[sort_column_index].flags &
           (FT_STAT | FT_PERF_STAT))
        {
//          lock_stat = true;
          get_stat = true;
        }
/*
        const FilterRuleSeq& filter_rules = filter.rules;

        for(size_t i = 0; i < filter_rules.length(); i++)
        {
          if(column(filter_rules[i].field).flags & (FT_STAT | FT_PERF_STAT))
          {
            lock_stat = true;
          }
        }
*/
          
        connection = Application::instance()->dbase()->connect();
        FilterSpec filter_spec = filter_specification(filter, connection);
          
        El::MySQL::Result_var qresult;
          
        std::string stat_from =
          El::Moment(ACE_Time_Value(std::min(stat_from_date,
                                             stat_to_date))).iso8601(false);
          
        std::string stat_to =
          El::Moment(ACE_Time_Value(std::max(stat_from_date,
                                             stat_to_date))).iso8601(false);
          
        ::NewsGate::Moderation::FeedInfoResult_var result =
            new ::NewsGate::Moderation::FeedInfoResult();

        FeedIdArray feed_ids;
          
        ACE_Time_Value gfr_time;
          
        result->feed_count = get_feed_range(connection,
                                            start_from,
                                            results,
                                            stat_from.c_str(),
                                            stat_to.c_str(),
                                            sort,
                                            sort_column_index,
                                            order_spec.c_str(),
                                            filter,
                                            filter_spec,
                                            feed_ids,
                                            gfr_time);


        ACE_Time_Value ffr_time;
          
        feeds_from_range(connection,
                         feed_ids,
                         get_stat,
                         stat_from.c_str(),
                         stat_to.c_str(),
                         order_spec.c_str(),
                         result->feed_infos,
                         ffr_time);          

        std::ostringstream ostr;
          
        ostr << "FeedManagement::feed_info_range:"
          "\n  get_feed_range for start/count "
             << start_from << "/" << results << ", dates [" << stat_from
             << ", " << stat_to << "], " << order_spec << " for "
             << El::Moment::time(gfr_time) << "\n  feeds_from_range for "
             << feed_ids.size() << " feeds, get stat " << (int)get_stat
             << " for " << El::Moment::time(ffr_time);
          
        Application::logger()->trace(ostr.str().c_str(),
                                     Aspect::FEED_STAT,
                                     El::Logging::HIGH);          

        return result._retn();
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Moderation::FeedManagement::feed_info_range: "
          "El::Exception caught. Description: " << e;
        
        NewsGate::Moderation::ImplementationException ex;
        ex.description = ostr.str().c_str();
        
        throw ex;
      }  
    }
    
    void
    FeedManagement::feeds_from_range(El::MySQL::Connection* connection,
                                     const FeedIdArray& feed_ids,
                                     bool get_stat,
                                     const char* stat_from,
                                     const char* stat_to,
                                     const char* order_spec,
                                     FeedInfoSeq& feed_infos,
                                     ACE_Time_Value& time)
      throw(Exception, El::Exception)
    {
      if(feed_ids.empty())
      {
        feed_infos.length(0);
        time = ACE_Time_Value::zero;
        return;
      }
      
      ACE_High_Res_Timer timer;
      timer.start();

      std::string state_query_select;
      std::string state_query_from;
      std::string state_query_where;
      
      {
        bool first_col = true;
        std::ostringstream ostr;
        ostr << "select";
        
        for(size_t i = FS_ID; i < FS_STAT_ID; ++i)
        {
          const FeedColumn& fc = column((FieldSelector)i);

          if((fc.flags & FT_HIDDEN) == 0)
          {
            fc.spec(ostr, first_col);            
          }
        }
        
        state_query_select = ostr.str();
      }
      
      {
        std::ostringstream ostr;
        ostr << " from Feed left join RSSFeedState on id=RSSFeedState.feed_id";
        state_query_from = ostr.str();        
      }
      
      {
        std::ostringstream ostr;
        ostr << " where id in (";
            
        for(FeedIdArray::const_iterator b(feed_ids.begin()), i(b),
              e(feed_ids.end()); i != e; ++i)
        {
          ostr << (i == b ? " " : ", ") << *i;
        }
      
        ostr << " )";
        state_query_where = ostr.str();
      }

      std::string query;
        
      if(get_stat)
      {
        bool first_col = true;
        
        std::ostringstream ostr;
        ostr << "select";

        for(size_t i = FS_ID; i < FS_COUNT; ++i)
        {
          const FeedColumn& fc = column((FieldSelector)i);

          if((fc.flags & FT_HIDDEN) == 0)
          {
            fc.spec(ostr,
                    first_col,
                    i < FS_PERF_STAT_ID ? "FS" : 0,
                    i < FS_STAT_ID || i >= FS_PERF_STAT_ID);
          }
        }        

        ostr << " from ( " << state_query_select;

        first_col = false;
        
        for(size_t i = FS_STAT_ID; i < FS_PERF_STAT_ID; ++i)
        {
          const FeedColumn& fc = column((FieldSelector)i);

          if((fc.flags & FT_HIDDEN) == 0)
          {
            fc.spec(ostr, first_col);
          }
        }      
        
        ostr << state_query_from << " left join RSSFeedStat on "
          "(id=RSSFeedStat.feed_id and RSSFeedStat.date>='"
             << stat_from << "' and RSSFeedStat.date<='"
             << stat_to << "')" << state_query_where << " group by id ) as FS "
          "left join StatFeed on (id=StatFeed.feed_id and StatFeed.date>='"
             << stat_from << "' and StatFeed.date<='"
             << stat_to << "') group by id";

        query = ostr.str();
      }
      else
      {
        query = state_query_select + state_query_from + state_query_where;
      }

      if(order_spec)
      {
        query += order_spec;
      }
      
      std::cerr << "View:\n" << query << std::endl << std::endl;
      
      El::MySQL::Result_var qresult = connection->query(query.c_str());
      
      feed_infos.length(qresult->num_rows());

      size_t use_columns = 0;

      if(get_stat)
      {
        use_columns = SIZE_MAX;
      }
      else
      {
        for(size_t i = 0; i < sizeof(feed_columns_) /
              sizeof(feed_columns_[0]); i++)
        {
          const FeedColumn& fcol = feed_columns_[i];
            
          if((fcol.flags & (FT_STAT | FT_PERF_STAT | FT_HIDDEN)) == 0)
          {
            use_columns++;
          }
        }
      }

      FeedRecord record(qresult.in(), use_columns);
      
      for(size_t i = 0; record.fetch_row(); i++)
      {
        FeedInfo& fi = feed_infos[i];
        
        fi.id = record.id();
        fi.type = record.type();
        fi.encoding = record.encoding().c_str();
        fi.space = record.space();
        fi.lang = record.lang();
        fi.country = record.country();
        fi.status = record.status().value()[0];
        fi.url = record.url().c_str();
        
        fi.creator = record.creator();
        fi.creator_type = record.creator_type().value()[0];
        
        fi.created = ACE_Time_Value(record.created().moment()).sec();
        fi.updated = ACE_Time_Value(record.updated().moment()).sec();
        
        fi.keywords = record.keywords().is_null() ?
          "" : record.keywords().c_str();

        fi.adjustment_script = record.adjustment_script().is_null() ?
          "" : record.adjustment_script().c_str();

        fi.comment = record.comment().is_null() ?
          "" : record.comment().c_str();
        
        fi.channel_title = record.channel_title().is_null() ?
          "" : record.channel_title().c_str();
        
        fi.channel_description = record.channel_description().is_null() ?
          "" : record.channel_description().c_str();
        
        fi.channel_html_link = record.channel_html_link().is_null() ?
          "" : record.channel_html_link().c_str();
        
        fi.channel_lang = record.channel_lang().is_null() ?
          (unsigned short)El::Lang::EC_NUL : record.channel_lang();
        
        fi.channel_country = record.channel_country().is_null() ?
          (unsigned short)El::Country::EC_NUL : record.channel_country();
        
        fi.channel_ttl = record.channel_ttl().is_null() ?
          -1 : record.channel_ttl();
        
        fi.channel_last_build_date =
          record.channel_last_build_date().is_null() ? 0 :
          ACE_Time_Value(record.channel_last_build_date().moment()).sec();
        
        fi.last_request_date = record.last_request_date().is_null() ?
          0 : ACE_Time_Value(record.last_request_date().moment()).sec();
        
        fi.last_modified_hdr = record.last_modified_hdr().is_null() ?
          "" : record.last_modified_hdr().c_str();
        
        fi.etag_hdr = record.etag_hdr().is_null() ?
          "" : record.etag_hdr().c_str();
        
        fi.content_length_hdr = record.content_length_hdr().is_null() ?
          -1 : record.content_length_hdr();
        
        fi.entropy = record.entropy().is_null() ? 0 : record.entropy();
        
        fi.entropy_updated_date = record.entropy_updated_date().is_null() ?
          0 : ACE_Time_Value(record.entropy_updated_date().moment()).sec();
        
        fi.size = record.size().is_null() ? 0 : record.size();
        
        fi.single_chunked = record.single_chunked().is_null() ?
          -1 : record.single_chunked();
        
        fi.first_chunk_size = record.first_chunk_size().is_null() ?
          -1 : record.first_chunk_size();
        
        fi.heuristics_counter = record.heuristics_counter().is_null() ?
          0 : record.heuristics_counter();

        if(get_stat)
        {
          fi.requests = record.requests().is_null() ? 0 : record.requests();
          fi.failed = record.failed().is_null() ? 0 : record.failed();
        
          fi.unchanged = record.unchanged().is_null() ?
            0 : record.unchanged();
        
          fi.not_modified = record.not_modified().is_null() ?
            0 : record.not_modified();
        
          fi.presumably_unchanged = record.presumably_unchanged().is_null() ?
            0 : record.presumably_unchanged();
        
          fi.has_changes = record.has_changes().is_null() ?
            0 : record.has_changes();
        
          fi.wasted = record.wasted().is_null() ? 0.0 : record.wasted();
        
          fi.outbound = record.outbound().is_null() ?
            0 : record.outbound();
        
          fi.inbound = record.inbound().is_null() ?
            0 : record.inbound();
        
          fi.requests_duration = record.requests_duration().is_null() ?
            0 : record.requests_duration();
        
          fi.messages = record.messages().is_null() ?
            0 : record.messages();
        
          fi.messages_size = record.messages_size().is_null() ?
            0 : record.messages_size();
        
          fi.messages_delay = record.messages_delay().is_null() ?
            0 : record.messages_delay();
        
          fi.max_message_delay = record.max_message_delay().is_null() ?
            0 : record.max_message_delay();

          fi.msg_impressions = record.msg_impressions().is_null() ?
            0 : record.msg_impressions();
          
          fi.msg_clicks = record.msg_clicks().is_null() ?
            0 : record.msg_clicks();
          
          fi.msg_ctr = record.msg_ctr().is_null() ? (float)0 :
            (float)((double)record.msg_ctr() / 100);
        }
        else
        {
          fi.requests = 0;
          fi.failed = 0;
          fi.unchanged = 0;
          fi.not_modified = 0;
          fi.presumably_unchanged = 0;
          fi.has_changes = 0;
          fi.wasted = 0.0;
          fi.outbound = 0;
          fi.inbound = 0;
          fi.requests_duration = 0;
          fi.messages = 0;
          fi.messages_size = 0;
          fi.messages_delay = 0;        
          fi.max_message_delay = 0;
          fi.msg_impressions = 0;
          fi.msg_clicks = 0;
          fi.msg_ctr = 0;
        }
      }
      
      timer.stop();
      timer.elapsed_time(time);
    }
  }  
}
