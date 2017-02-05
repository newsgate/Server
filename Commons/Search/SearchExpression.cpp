/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Commons/Search/SearchExpression.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <ctype.h>
#include <stdlib.h>
#include <limits.h>

#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <vector>
#include <set>

#include <ext/hash_set>

#include <google/dense_hash_map>

#include <El/Exception.hpp>
#include <El/String/Manip.hpp>
#include <El/Net/URL.hpp>
#include <El/Net/HTTP/URL.hpp>
#include <El/Moment.hpp>
#include <El/ArrayPtr.hpp>
#include <El/Hash/Hash.hpp>
#include <El/Locale.hpp>

#include "SearchExpression.hpp"

//#define TRACE_WEIGHT
//#define TRACE_SORT_TIME
//#define TRACE_SEARCH_RES_TIME

namespace
{
  const size_t SORT_COUNT_FACTOR = 10;
  
  const float SORT_BY_RELEVANCE_CORENESS_WEIGHT = 15;
  const float SORT_BY_RELEVANCE_DATE_WEIGHT = 1;
//  const float SORT_BY_RELEVANCE_TOPICALITY_WEIGHT = 7;
  
//  const float SORT_BY_RELEVANCE_RCTR_WEIGHT = 4;
//  const float SORT_BY_RELEVANCE_FEED_RCTR_WEIGHT = 0.4;
//  const float SORT_BY_RELEVANCE_IMAGE_WEIGHT = 0.1;
//  const float SORT_BY_RELEVANCE_CAPACITY_WEIGHT = 2;

  const float SORT_BY_CAPACITY_CAPACITY_WEIGHT = 500;
  const float SORT_BY_CAPACITY_DATE_WEIGHT = 5;
//  const float SORT_BY_CAPACITY_IMAGE_WEIGHT = 0.5;
  const float SORT_BY_CAPACITY_IMAGE_WEIGHT = 0.01;
  const float SORT_BY_CAPACITY_RCTR_WEIGHT = 0.1;
  
  const float SORT_BY_POPULARITY_RCTR_WEIGHT = 500;
  const float SORT_BY_POPULARITY_DATE_WEIGHT = 5;
  const float SORT_BY_POPULARITY_IMAGE_WEIGHT = 0.5;

  const float MIN_FACTOR = 2;
  const float MAX_FACTOR = 5;
  
  const wchar_t* SPECIAL_WORD[] =
  {
    L"ALL",
    L"ANY",
    L"SITE",
    L"URL",
    L"CATEGORY",
    L"MSG",
    L"EVENT",
    L"EVERY",
    L"AND",
    L"EXCEPT",
    L"OR",
    L"(",
    L")",
    L"LANGUAGE",
    L"COUNTRY",
    L"SPACE",
    L"DOMAIN",
    L"CAPACITY",
    L"DATE",
    L"FETCHED",
    L"NOT",
    L"BEFORE",
    L"WITH",
    L"NO",
    L"IMAGE",
    L"SIGNATURE",
    L"NONE",
    L"IMPRESSIONS",
    L"CLICKS",
    L"CTR",
    L"RCTR",
    L"F-IMPRESSIONS",
    L"F-CLICKS",
    L"F-CTR",
    L"F-RCTR",
    L"CORE",
    L"TITLE",
    L"DESCRIPTION",
    L"IMAGE-ALT",
    L"KEYWORDS",
    L"VISITED"
  };
}

namespace NewsGate
{
  namespace Search
  {
    const Counter Counter::null;
    
    const ExpressionParser::TokenType ExpressionParser::SPECIAL_WORD_TYPE[] =
    {
      NewsGate::Search::ExpressionParser::TT_ALL,
      NewsGate::Search::ExpressionParser::TT_ANY,
      NewsGate::Search::ExpressionParser::TT_SITE,
      NewsGate::Search::ExpressionParser::TT_URL,
      NewsGate::Search::ExpressionParser::TT_CATEGORY,
      NewsGate::Search::ExpressionParser::TT_MSG,
      NewsGate::Search::ExpressionParser::TT_EVENT,
      NewsGate::Search::ExpressionParser::TT_EVERY,
      NewsGate::Search::ExpressionParser::TT_AND,
      NewsGate::Search::ExpressionParser::TT_EXCEPT,
      NewsGate::Search::ExpressionParser::TT_OR,
      NewsGate::Search::ExpressionParser::TT_OPEN,
      NewsGate::Search::ExpressionParser::TT_CLOSE,
      NewsGate::Search::ExpressionParser::TT_LANG,
      NewsGate::Search::ExpressionParser::TT_COUNTRY,
      NewsGate::Search::ExpressionParser::TT_SPACE,
      NewsGate::Search::ExpressionParser::TT_DOMAIN,
      NewsGate::Search::ExpressionParser::TT_CAPACITY,
      NewsGate::Search::ExpressionParser::TT_PUB_DATE,
      NewsGate::Search::ExpressionParser::TT_FETCHED,
      NewsGate::Search::ExpressionParser::TT_NOT,
      NewsGate::Search::ExpressionParser::TT_BEFORE,
      NewsGate::Search::ExpressionParser::TT_WITH,
      NewsGate::Search::ExpressionParser::TT_NO,
      NewsGate::Search::ExpressionParser::TT_IMAGE,
      NewsGate::Search::ExpressionParser::TT_SIGNATURE,
      NewsGate::Search::ExpressionParser::TT_NONE,
      NewsGate::Search::ExpressionParser::TT_IMPRESSIONS,
      NewsGate::Search::ExpressionParser::TT_CLICKS,
      NewsGate::Search::ExpressionParser::TT_CTR,
      NewsGate::Search::ExpressionParser::TT_RCTR,
      NewsGate::Search::ExpressionParser::TT_FEED_IMPRESSIONS,
      NewsGate::Search::ExpressionParser::TT_FEED_CLICKS,
      NewsGate::Search::ExpressionParser::TT_FEED_CTR,
      NewsGate::Search::ExpressionParser::TT_FEED_RCTR,
      NewsGate::Search::ExpressionParser::TT_CORE,
      NewsGate::Search::ExpressionParser::TT_TITLE,
      NewsGate::Search::ExpressionParser::TT_DESC,
      NewsGate::Search::ExpressionParser::TT_IMG_ALT,
      NewsGate::Search::ExpressionParser::TT_KEYWORDS,
      NewsGate::Search::ExpressionParser::TT_VISITED     
    };    
  
    WeightedId WeightedId::zero;
    WeightedId WeightedId::nonexistent(Message::Id::nonexistent);
    
    El::Stat::TimeMeter Expression::search_meter("Expression::search",
                                                 false);
    
    El::Stat::TimeMeter Expression::search_simple_meter(
      "Expression::search_simple",
      false);

    El::Stat::TimeMeter Expression::take_top_meter(
      "Expression::take_top",
      false);

    El::Stat::TimeMeter Expression::sort_and_cut_meter(
      "Expression::sort_and_cut",
      false);

    El::Stat::TimeMeter Expression::sort_skip_cut_meter(
      "Expression::sort_skip_cut",
      false);

    El::Stat::TimeMeter Expression::remove_similar_meter(
      "Expression::remove_similar",
      false);

    El::Stat::TimeMeter Expression::remove_similar_p1_meter(
      "Expression::remove_similar[P1]",
      false);

    El::Stat::TimeMeter Expression::remove_similar_p2_meter(
      "Expression::remove_similar[P2]",
      false);

    El::Stat::TimeMeter Expression::collapse_events_meter(
      "Expression::collapse_events",
      false);

    El::Stat::TimeMeter Expression::copy_range_meter(
      "Expression::copy_range",
      false);

    El::Stat::TimeMeter Expression::search_p1_meter("Expression::search[P1]",
                                                    false);
    
    El::Stat::TimeMeter Expression::search_p2_meter("Expression::search[P2]",
                                                    false);    

    //
    // ExpressionParser
    //
    ExpressionParser::ExpressionParser(uint32_t respected_impression_level)
      throw()
        : position_(1),
          last_token_begins_(0),
          last_token_type_(TT_UNDEFINED),
          last_token_quoted_(L'\0'),
          buffered_(false),
          condition_flags_(0),
          respected_impression_level_(respected_impression_level)
    {
    }
    
    void
    ExpressionParser::parse(std::wistream& istr)
      throw(ParseError, Exception, El::Exception)
    {
      position_ = 1;
      last_token_begins_ = 0;
      last_token_type_ = TT_UNDEFINED;
      last_token_.clear();
      last_token_quoted_ = L'\0';
      buffered_ = false;
      expression_ = 0;
      condition_flags_ = 0;
      
      expression_ = new Expression(read_expression(istr, false));
    }

    Condition*
    ExpressionParser::read_condition(std::wistream& istr,
                                     Condition* first_operand)
      throw(ParseError, Exception, El::Exception)
    {
      Condition_var condition;

      if(first_operand == 0)
      {
        condition = read_operand(istr);
      }
      else
      {
        El::RefCount::add_ref(first_operand);
        condition = first_operand;
      }
      
      TokenType type;
      while((type = read_token(istr)) != TT_UNDEFINED)
      {
        if(!is_operation(type))
        {
          put_back();
          break;
        }
      
        switch(type)
        {
        case TT_AND: condition = read_and(istr, condition.in()); break;
        case TT_EXCEPT: condition = read_except(istr, condition.in()); break;
        case TT_OR: condition = read_or(istr, condition.in()); break;
        default:
          {
            put_back();
            break;
          }
        }
      }

      condition_flags_ |= ((uint64_t)1) << condition->type();

      return condition.retn();
    }

    void
    ExpressionParser::read_word_modifiers(std::wistream& istr,
                                          uint32_t& flags,
                                          uint32_t* match_threshold)
      throw(ParseError, Exception, El::Exception)
    {
      flags = 0;
      
      if(match_threshold)
      {
        *match_threshold = 1;
      }

      bool awaiting_match_threshold = match_threshold != 0;

      TokenType type;
      
      while(true)
      {
        type = read_token(istr);

        if(type == TT_REGULAR)
        {
          if(last_token_quoted_ == L'\0' && awaiting_match_threshold &&
             El::String::Manip::numeric(last_token_.c_str(), *match_threshold))
          {
            awaiting_match_threshold = false;
            continue;
          }
        }
        else if(type == TT_CORE)
        {
          if((flags & Words::WF_CORE) == 0)
          {
            flags |= Words::WF_CORE;
            continue;
          }
        }
        else if(type == TT_TITLE)
        {
          if((flags & Words::WF_TITLE) == 0)
          {
            flags |= Words::WF_TITLE;
            continue;
          }
        }
        else if(type == TT_DESC)
        {
          if((flags & Words::WF_DESC) == 0)
          {
            flags |= Words::WF_DESC;
            continue;
          }
        }
        else if(type == TT_IMG_ALT)
        {
          if((flags & Words::WF_IMG_ALT) == 0)
          {
            flags |= Words::WF_IMG_ALT;
            continue;
          }
        }
        else if(type == TT_KEYWORDS)
        {
          if((flags & Words::WF_KEYWORDS) == 0)
          {
            flags |= Words::WF_KEYWORDS;
            continue;
          }
        }

        break;
      }

      if(type != TT_UNDEFINED)
      {
        put_back();
      }

      if((flags & (Words::WF_TITLE | Words::WF_DESC | Words::WF_IMG_ALT |
                   Words::WF_KEYWORDS))== 0)
      {
        flags |= Words::WF_TITLE | Words::WF_DESC | Words::WF_IMG_ALT;
      }
    }
    
    Condition*
    ExpressionParser::read_all(std::wistream& istr)
      throw(ParseError, Exception, El::Exception)
    {
      AllWords* all = new AllWords();
      Condition_var condition = all;

      read_word_modifiers(istr, all->word_flags, 0);

      if(!read_words(istr, all->words))
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_all: "
          "no words specified for ALL condition at position "
             << last_token_begins_;

        throw NoWordsForAll(ostr.str().c_str(), last_token_begins_);
      }

      return condition.retn();
    }

    Condition*
    ExpressionParser::read_any(std::wistream& istr)
      throw(ParseError, Exception, El::Exception)
    {
      AnyWords* any = new AnyWords();
      Condition_var condition = any;
      
      read_word_modifiers(istr, any->word_flags, &any->match_threshold);

      if(!read_words(istr, any->words))
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_any: "
          "no words specified for ANY condition at position "
             << last_token_begins_;

        throw NoWordsForAny(ostr.str().c_str(), last_token_begins_);
      }

      any->remove_duplicates(false);

      return condition.retn();
    }

    Condition*
    ExpressionParser::read_site(std::wistream& istr)
      throw(ParseError, Exception, El::Exception)
    {
      Site* site = new Site();
      Condition_var condition = site;

      TokenType type;
      while((type = read_token(istr)) == TT_REGULAR)
      {
        std::string hostname;
        El::String::Manip::wchar_to_utf8(last_token_.c_str(), hostname);

        std::string idn_host;
        std::string host;

        try
        {
          El::Net::idna_encode(hostname.c_str(), idn_host);        
          El::Net::idna_decode(idn_host.c_str(), host);
        }
        catch(const El::Exception)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Search::ExpressionParser::read_site: "
            "valid host name expected for SITE condition at position "
               << last_token_begins_;

          throw DomainExpected(ostr.str().c_str(), last_token_begins_);
        }
        
        site->hostnames.push_back(host);

        if(host != idn_host)
        {
          site->hostnames.push_back(idn_host);
        }
      }
      
      if(site->hostnames.empty())
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_site: "
          "no host name specified for SITE condition at position "
             << last_token_begins_;

        throw NoHostForSite(ostr.str().c_str(), last_token_begins_);
      }

      if(type != TT_UNDEFINED)
      {
        put_back();
      }

      return condition.retn();
    }

    Condition*
    ExpressionParser::read_url(std::wistream& istr)
      throw(ParseError, Exception, El::Exception)
    {
      Url* url = new Url();
      Condition_var condition = url;
      
      TokenType type;
      while((type = read_token(istr)) == TT_REGULAR)
      {
        std::string urlpath;
        El::String::Manip::wchar_to_utf8(last_token_.c_str(), urlpath);

        try
        {
          El::Net::HTTP::URL http_url(urlpath.c_str());

          url->urls.push_back(http_url.string());

          if(strcmp(http_url.string(), http_url.idn_string()))
          {
            url->urls.push_back(http_url.idn_string());
          }
        }
        catch(const ParseError&)
        {
          throw;

        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Search::ExpressionParser::read_url: "
            "bad URL condition at position " << last_token_begins_;

          throw BadPathForUrl(ostr.str().c_str(), last_token_begins_);
        }
      }
      
      if(url->urls.empty())
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_url: "
          "no url path specified for URL condition at position "
             << last_token_begins_;

        throw NoPathForUrl(ostr.str().c_str(), last_token_begins_);
      }

      if(type != TT_UNDEFINED)
      {
        put_back();
      }
        
      return condition.retn();
    }

    Condition*
    ExpressionParser::read_category(std::wistream& istr)
      throw(ParseError, Exception, El::Exception)
    {
      Category* category = new Category();
      Condition_var condition = category;

      typedef std::vector<std::string> CategoryArray;
      CategoryArray categories;
      
      TokenType type;
      while((type = read_token(istr)) == TT_REGULAR)
      {
        std::wstring lower;
        El::String::Manip::to_lower(last_token_.c_str(), lower);
        
        std::string path;
        El::String::Manip::wchar_to_utf8(lower.c_str(), path);

        if(path[0] != '/')
        {
          path = std::string("/") + path;
        }        

        if(path[path.length() - 1] != '/')
        {
          path += "/";
        }        

        categories.push_back(path);
      }
      
      if(categories.empty())
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_category: "
          "no category specified for CATEGORY condition at position "
             << last_token_begins_;

        throw NoPathForCategory(ostr.str().c_str(), last_token_begins_);
      }

      category->categories.init(categories);

      if(type != TT_UNDEFINED)
      {
        put_back();
      }
        
      return condition.retn();
    }

    Condition*
    ExpressionParser::read_msg(std::wistream& istr)
      throw(ParseError, Exception, El::Exception)
    {
      Msg* msg = new Msg();
      Condition_var condition = msg;
      
      TokenType type;
      while((type = read_token(istr)) == TT_REGULAR)
      {
        std::string message_id;
        El::String::Manip::wchar_to_utf8(last_token_.c_str(), message_id);

        try
        {
          size_t size = message_id.size();
          bool dense = size > 0 && message_id[size - 1] == '=';
            
          Message::Id id(message_id.c_str(), dense);
          msg->ids.push_back(id);
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Search::ExpressionParser::read_msg: "
            "bad MSG condition at position " << last_token_begins_;

          throw BadMessageId(ostr.str().c_str(), last_token_begins_);
        }
      }
      
      if(msg->ids.empty())
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_msg: "
          "no message id specified for MSG condition at position "
             << last_token_begins_;

        throw NoMessageId(ostr.str().c_str(), last_token_begins_);
      }

      if(type != TT_UNDEFINED)
      {
        put_back();
      }
        
      return condition.retn();
    }

    Condition*
    ExpressionParser::read_event(std::wistream& istr)
      throw(ParseError, Exception, El::Exception)
    {
      Event* event = new Event();
      Condition_var condition = event;
      
      TokenType type;
      while((type = read_token(istr)) == TT_REGULAR)
      {
        std::string event_id;
        El::String::Manip::wchar_to_utf8(last_token_.c_str(), event_id);

        try
        {
          size_t size = event_id.size();
          bool dense = size > 0 && event_id[size - 1] == '=';
          
          El::Luid id(event_id.c_str(), dense);
          event->ids.push_back(id);
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Search::ExpressionParser::read_event: "
            "bad EVENT condition at position " << last_token_begins_;

          throw BadEventId(ostr.str().c_str(), last_token_begins_);
        }
      }
      
      if(event->ids.empty())
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_event: "
          "no event id specified for EVENT condition at position "
             << last_token_begins_;

        throw NoEventId(ostr.str().c_str(), last_token_begins_);
      }

      if(type != TT_UNDEFINED)
      {
        put_back();
      }
        
      return condition.retn();
    }

    Condition*
    ExpressionParser::read_every(std::wistream& istr)
      throw(ParseError, Exception, El::Exception)
    {
      return new Every();
    }

    Condition*
    ExpressionParser::read_none(std::wistream& istr)
      throw(ParseError, Exception, El::Exception)
    {
      return new None();
    }

    Condition*
    ExpressionParser::read_operand(std::wistream& istr)
      throw(ParseError, Exception, El::Exception)
    {
      TokenType type = read_token(istr);

      switch(type)
      {
      case TT_REGULAR:
      case TT_CORE:
      case TT_TITLE:
      case TT_DESC:
      case TT_IMG_ALT:
      case TT_KEYWORDS:
        {
          put_back();
          type = TT_ALL;
          break;
        }
      default: break;
      }
      
      if(!is_operand(type))
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_operand: "
          "operand expected at position " << last_token_begins_;
          
        throw OperandExpected(ostr.str().c_str(), last_token_begins_);
      }

      Condition_var condition;
      
      switch(type)
      {
      case TT_ALL: condition = read_all(istr); break;
      case TT_ANY: condition = read_any(istr); break;
      case TT_SITE: condition = read_site(istr); break;
      case TT_URL: condition = read_url(istr); break;
      case TT_CATEGORY: condition = read_category(istr); break;
      case TT_MSG: condition = read_msg(istr); break;
      case TT_EVENT: condition = read_event(istr); break;
      case TT_EVERY: condition = read_every(istr); break;
      case TT_NONE: condition = read_none(istr); break;
      case TT_OPEN: condition = read_expression(istr, true); break;
      default:
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Search::ExpressionParser::read_operand: "
            "shouldn't be here (position " << last_token_begins_ << ")";
          
          throw Exception(ostr.str());  
        }
      }

      condition_flags_ |= ((uint64_t)1) << condition->type();
      
      return condition.retn();
    }
    
    Condition*
    ExpressionParser::read_expression(std::wistream& istr, bool nested)
      throw(ParseError, Exception, El::Exception)
    {
      Condition_var condition;
      
      while(true)
      {
        condition = read_condition(istr, condition.in());

        TokenType type = read_token(istr);

        if(is_filter(type))
        {
          switch(type)
          {
          case TT_PUB_DATE:
            {
              condition = read_pub_date(istr, condition.in());
              break;
            }
          case TT_FETCHED:
            {
              condition = read_fetched(istr, condition.in());
              break;
            }
          case TT_VISITED:
            {
              condition = read_visited(istr, condition.in());
              break;
            }
          case TT_WITH:
            {
              condition = read_with(istr, condition.in());
              break;
            }
          case TT_LANG:
            {
              condition = read_lang(istr, condition.in());
              break;
            }
          case TT_COUNTRY:
            {
              condition = read_country(istr, condition.in());
              break;
            }
          case TT_SPACE:
            {
              condition = read_space(istr, condition.in());
              break;
            }
          case TT_DOMAIN:
            {
              condition = read_domain(istr, condition.in());
              break;
            }
          case TT_CAPACITY:
            {
              condition = read_capacity(istr, condition.in());
              break;
            }
          case TT_IMPRESSIONS:
            {
              condition = read_impressions(istr, condition.in());
              break;
            }
          case TT_FEED_IMPRESSIONS:
            {
              condition = read_feed_impressions(istr, condition.in());
              break;
            }
          case TT_CLICKS:
            {
              condition = read_clicks(istr, condition.in());
              break;
            }
          case TT_FEED_CLICKS:
            {
              condition = read_feed_clicks(istr, condition.in());
              break;
            }
          case TT_CTR:
            {
              condition = read_ctr(istr, condition.in());
              break;
            }
          case TT_FEED_CTR:
            {
              condition = read_feed_ctr(istr, condition.in());
              break;
            }
          case TT_RCTR:
            {
              condition = read_rctr(istr, condition.in());
              break;
            }
          case TT_FEED_RCTR:
            {
              condition = read_feed_rctr(istr, condition.in());
              break;
            }
          case TT_SIGNATURE:
            {
              condition = read_signature(istr, condition.in());
              break;
            }
          default:
            {
              std::ostringstream ostr;
              ostr << "NewsGate::Search::ExpressionParser::read_expression: "
                "shouldn't be here (position " << last_token_begins_ << ")";
          
              throw Exception(ostr.str());  
            }  
          }

          condition_flags_ |= ((uint64_t)1) << condition->type();          
          continue;
        }
        
        if(nested)
        {
          if(type == TT_CLOSE)
          {
            break;
          }
          else
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Search::ExpressionParser::read_expression: "
              "')' is expected at position " << last_token_begins_;
          
            throw
              CloseParenthesisExpected(ostr.str().c_str(), last_token_begins_);
          }
        }
        else
        {
          if(type == TT_UNDEFINED)
          {
            break;
          }
          else
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Search::ExpressionParser::read_expression: "
              "operation expected at position " << last_token_begins_;
          
            throw OperationExpected(ostr.str().c_str(), last_token_begins_);
          }
        }
      }
      
      return condition.retn();
    }
    
    Condition*
    ExpressionParser::read_fetched(std::wistream& istr,
                                   Condition* left_op)
      throw(ParseError, Exception, El::Exception)
    {
      TokenType type = read_token(istr);

      if(type != TT_REGULAR && type != TT_BEFORE)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_fetched: "
          "time or word BEFORE is expected at position "
             << last_token_begins_;

        throw NoTimeForFetched(ostr.str().c_str(), last_token_begins_);
      }

      bool reversed = type == TT_BEFORE;

      if(reversed)
      {
        type = read_token(istr);
      }
      
      if(type != TT_REGULAR)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_fetched: "
          "time is expected at position " << last_token_begins_;

        throw NoTimeForFetched(ostr.str().c_str(), last_token_begins_);
      }

      try
      {
        Condition::Time time(last_token_.c_str());

        Fetched* fetched = new Fetched();
        fetched->time = time;
        fetched->reversed = reversed;
        
        Condition_var condition = fetched;

        El::RefCount::add_ref(left_op);
        fetched->condition = left_op;
        
        return condition.retn();
      }
      catch(const Condition::Time::InvalidArg& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_fetched: "
          "reading time error. Description:\n" << e;

        throw WrongTimeForFetched(ostr.str().c_str(), last_token_begins_);
      }
    }
    
    Condition*
    ExpressionParser::read_visited(std::wistream& istr,
                                   Condition* left_op)
      throw(ParseError, Exception, El::Exception)
    {
      TokenType type = read_token(istr);

      if(type != TT_REGULAR && type != TT_BEFORE)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_visited: "
          "time or word BEFORE is expected at position "
             << last_token_begins_;

        throw NoTimeForVisited(ostr.str().c_str(), last_token_begins_);
      }

      bool reversed = type == TT_BEFORE;

      if(reversed)
      {
        type = read_token(istr);
      }
      
      if(type != TT_REGULAR)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_visited: "
          "time is expected at position " << last_token_begins_;

        throw NoTimeForVisited(ostr.str().c_str(), last_token_begins_);
      }

      try
      {
        Condition::Time time(last_token_.c_str());

        Visited* visited = new Visited();
        visited->time = time;
        visited->reversed = reversed;
        
        Condition_var condition = visited;

        El::RefCount::add_ref(left_op);
        visited->condition = left_op;
        
        return condition.retn();
      }
      catch(const Condition::Time::InvalidArg& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_visited: "
          "reading time error. Description:\n" << e;

        throw WrongTimeForVisited(ostr.str().c_str(), last_token_begins_);
      }
    }
    
    Condition*
    ExpressionParser::read_with(std::wistream& istr,
                                Condition* left_op)
      throw(ParseError, Exception, El::Exception)
    {
      TokenType type = read_token(istr);

      if(type != TT_NO && type != TT_IMAGE)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_with: "
          "words NO, IMAGE expected at position "
             << last_token_begins_;

        throw NoParamsForWith(ostr.str().c_str(), last_token_begins_);
      }

      bool reversed = type == TT_NO;

      if(!reversed)
      {
        put_back();
      }

      With* with = new With();
      Condition_var condition = with;

      while((type = read_token(istr)) == TT_IMAGE)
      {
        with->features.push_back(With::FT_IMAGE);
      }
      
      if(with->features.empty())
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_with: "
          "language specification is expected at position "
             << last_token_begins_;
        
        throw NoParamsForWith(ostr.str().c_str(), last_token_begins_);
      }

      if(type != TT_UNDEFINED)
      {
        put_back();
      }

      with->reversed = reversed;

      El::RefCount::add_ref(left_op);
      with->condition = left_op;
        
      return condition.retn();
    }
    
    Condition*
    ExpressionParser::read_pub_date(std::wistream& istr,
                                    Condition* left_op)
      throw(ParseError, Exception, El::Exception)
    {
      TokenType type = read_token(istr);
      
      if(type != TT_REGULAR && type != TT_BEFORE)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_pub_date: "
          "time  or word BEFORE is expected at position "
             << last_token_begins_;

        throw NoTimeForPubDate(ostr.str().c_str(), last_token_begins_);
      }

      bool reversed = type == TT_BEFORE;

      if(reversed)
      {
        type = read_token(istr);
      }
      
      if(type != TT_REGULAR)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_pub_date: "
          "time is expected at position " << last_token_begins_;

        throw NoTimeForPubDate(ostr.str().c_str(), last_token_begins_);
      }

      try
      {
        Condition::Time time(last_token_.c_str());

        PubDate* pub_date = new PubDate();
        pub_date->time = time;
        pub_date->reversed = reversed;
        
        Condition_var condition = pub_date;

        El::RefCount::add_ref(left_op);
        pub_date->condition = left_op;
        
        return condition.retn();
      }
      catch(const Condition::Time::InvalidArg& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_pub_date: "
          "reading time error. Description:\n" << e;

        throw WrongTimeForPubDate(ostr.str().c_str(), last_token_begins_);
      }
    }
    
    Condition*
    ExpressionParser::read_lang(std::wistream& istr,
                                Condition* left_op)
      throw(ParseError, Exception, El::Exception)
    {
      TokenType type = read_token(istr);

      if(type != TT_REGULAR && type != TT_NOT)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_lang: "
          "language specification or word NOT is expected at position "
             << last_token_begins_;
        
        throw LangOrNotExpected(ostr.str().c_str(), last_token_begins_);
      }

      bool reversed = type == TT_NOT;

      if(!reversed)
      {
        put_back();
      }

      Lang* lang = new Lang();
      Condition_var condition = lang;

      while((type = read_token(istr)) == TT_REGULAR)
      {
        std::string token;
        El::String::Manip::wchar_to_utf8(last_token_.c_str(), token);

        try
        {
          lang->values.push_back(El::Lang(token.c_str()));
        }
        catch(const El::Lang::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Search::ExpressionParser::read_lang: "
            "invalid language specification at position "
               << last_token_begins_;
        
          throw InvalidLang(ostr.str().c_str(), last_token_begins_);        
        }
      }
      
      if(lang->values.empty())
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_lang: "
          "language specification is expected at position "
             << last_token_begins_;
        
        throw LangExpected(ostr.str().c_str(), last_token_begins_);
      }

      if(type != TT_UNDEFINED)
      {
        put_back();
      }

      lang->reversed = reversed;

      El::RefCount::add_ref(left_op);
      lang->condition = left_op;
        
      return condition.retn();
    }

    Condition*
    ExpressionParser::read_country(std::wistream& istr,
                                   Condition* left_op)
      throw(ParseError, Exception, El::Exception)
    {
      TokenType type = read_token(istr);

      if(type != TT_REGULAR && type != TT_NOT)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_country: "
          "country specification or word NOT is expected at position "
             << last_token_begins_;
        
        throw CountryOrNotExpected(ostr.str().c_str(), last_token_begins_);
      }

      bool reversed = type == TT_NOT;

      if(!reversed)
      {
        put_back();
      }

      Country* country = new Country();
      Condition_var condition = country;

      while((type = read_token(istr)) == TT_REGULAR)
      {
        std::string token;
        El::String::Manip::wchar_to_utf8(last_token_.c_str(), token);

        try
        {
          country->values.push_back(El::Country(token.c_str()));
        }
        catch(const El::Country::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Search::ExpressionParser::read_country: "
            "invalid country specification at position "
               << last_token_begins_;
        
          throw InvalidCountry(ostr.str().c_str(), last_token_begins_);        
        }
      }
      
      if(country->values.empty())
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_country: "
          "country specification is expected at position "
             << last_token_begins_;
        
        throw CountryExpected(ostr.str().c_str(), last_token_begins_);
      }

      if(type != TT_UNDEFINED)
      {
        put_back();
      }

      country->reversed = reversed;

      El::RefCount::add_ref(left_op);
      country->condition = left_op;
        
      return condition.retn();
    }

    Condition*
    ExpressionParser::read_space(std::wistream& istr,
                                 Condition* left_op)
      throw(ParseError, Exception, El::Exception)
    {
      TokenType type = read_token(istr);

      if(type != TT_REGULAR && type != TT_NOT)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_space: "
          "space specification or word NOT is expected at position "
             << last_token_begins_;
        
        throw SpaceOrNotExpected(ostr.str().c_str(), last_token_begins_);
      }

      bool reversed = type == TT_NOT;

      if(!reversed)
      {
        put_back();
      }
      
      Space* space = new Space();
      Condition_var condition = space;

      while((type = read_token(istr)) == TT_REGULAR)
      {
        std::string feed_space;
        El::String::Manip::wchar_to_utf8(last_token_.c_str(), feed_space);

        NewsGate::Feed::Space fspace =
          NewsGate::Feed::space(feed_space.c_str());

        if(fspace == NewsGate::Feed::SP_NONEXISTENT)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Search::ExpressionParser::read_space: "
            "unexpected space '" << feed_space << "' at position "
               << last_token_begins_;
        
          throw UnexpectedSpace(ostr.str().c_str(), last_token_begins_);
        }

        space->values.push_back(fspace);
      }

      if(space->values.empty())
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_space: "
          "space specification is expected at position "
             << last_token_begins_;
        
        throw SpaceExpected(ostr.str().c_str(), last_token_begins_);
      }
      
      if(type != TT_UNDEFINED)
      {
        put_back();
      }

      space->reversed = reversed;

      El::RefCount::add_ref(left_op);
      space->condition = left_op;
        
      return condition.retn();
    }

    Condition*
    ExpressionParser::read_domain(std::wistream& istr, Condition* left_op)
      throw(ParseError, Exception, El::Exception)
    {
      TokenType type = read_token(istr);

      if(type != TT_REGULAR && type != TT_NOT)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_domain: "
          "domain specification or word NOT is expected at position "
             << last_token_begins_;
        
        throw DomainOrNotExpected(ostr.str().c_str(), last_token_begins_);
      }

      bool reversed = type == TT_NOT;

      if(!reversed)
      {
        put_back();
      }
      
      Domain* domains = new Domain();
      Condition_var condition = domains;

      Domain::DomainList& domain_list = domains->domains;

      while((type = read_token(istr)) == TT_REGULAR)
      {
        std::string dnsname;
        El::String::Manip::wchar_to_utf8(last_token_.c_str(), dnsname);
        
        std::string idn_domain;
        std::string domain;

        try
        {
          El::Net::idna_encode(dnsname.c_str(), idn_domain);        
          El::Net::idna_decode(idn_domain.c_str(), domain);
        }
        catch(const El::Exception)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Search::ExpressionParser::read_domain: "
            "valid domain specification expected at position "
               << last_token_begins_;

          throw DomainExpected(ostr.str().c_str(), last_token_begins_);
        }
        
        Domain::DomainRec dm;
        dm.name = domain;
        dm.len = dm.name.length();
        
        domain_list.push_back(dm);

        if(domain != idn_domain)
        {
          Domain::DomainRec dm;
          dm.name = idn_domain;
          dm.len = dm.name.length();

          domain_list.push_back(dm);
        }        
      }
      
      if(domain_list.empty())
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_domain: "
          "domain specification is expected at position "
             << last_token_begins_;
        
        throw DomainExpected(ostr.str().c_str(), last_token_begins_);
      }

      if(type != TT_UNDEFINED)
      {
        put_back();
      }

      domains->reversed = reversed;
/*
      if(same_level_domains)
      {
        domains->domain_map_size = domain_map.size();
      }
*/
      El::RefCount::add_ref(left_op);
      domains->condition = left_op;
        
      return condition.retn();
    }

    Condition*
    ExpressionParser::read_capacity(std::wistream& istr,
                                    Condition* left_op)
      throw(ParseError, Exception, El::Exception)
    {
      TokenType type = read_token(istr);

      if(type != TT_REGULAR && type != TT_NOT)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_capacity: "
          "event capacity or word NOT is expected at position "
             << last_token_begins_;
        
        throw CapacityOrNotExpected(ostr.str().c_str(), last_token_begins_);
      }

      bool reversed = type == TT_NOT;

      if(!reversed)
      {
        put_back();
      }

      type = read_token(istr);

      if(type != TT_REGULAR)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_capacity: "
          "event capacity is expected at position " << last_token_begins_;

        throw CapacityExpected(ostr.str().c_str(), last_token_begins_);
      }

      uint32_t capacity_value = 0;
      
      if(!El::String::Manip::numeric(last_token_.c_str(), capacity_value) ||
         capacity_value == 0)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_capacity: "
          "positive integer is expected as event capacity at position "
             << last_token_begins_;
        
        throw WrongCapacity(ostr.str().c_str(), last_token_begins_);
      }
  
      Capacity* capacity = new Capacity();
      Condition_var condition = capacity;
      
      capacity->value = capacity_value;
      capacity->reversed = reversed;

      El::RefCount::add_ref(left_op);
      capacity->condition = left_op;

      return condition.retn();
    }

    Condition*
    ExpressionParser::read_impressions(std::wistream& istr,
                                       Condition* left_op)
      throw(ParseError, Exception, El::Exception)
    {
      TokenType type = read_token(istr);

      if(type != TT_REGULAR && type != TT_NOT)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_impressions: "
          "message impression count or word NOT is expected at position "
             << last_token_begins_;
        
        throw ImpressionsOrNotExpected(ostr.str().c_str(), last_token_begins_);
      }

      bool reversed = type == TT_NOT;

      if(!reversed)
      {
        put_back();
      }

      type = read_token(istr);

      if(type != TT_REGULAR)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_impressions: "
          "message impression count is expected at position "
             << last_token_begins_;

        throw ImpressionsExpected(ostr.str().c_str(), last_token_begins_);
      }

      uint64_t impressions_value = 0;
      
      if(!El::String::Manip::numeric(last_token_.c_str(), impressions_value))
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_impressions: "
          "non-negative integer is expected as message impression count "
          "at position " << last_token_begins_;
        
        throw WrongImpressions(ostr.str().c_str(), last_token_begins_);
      }
  
      Impressions* impressions = new Impressions();
      Condition_var condition = impressions;
      
      impressions->value = impressions_value;
      impressions->reversed = reversed;

      El::RefCount::add_ref(left_op);
      impressions->condition = left_op;

      return condition.retn();
    }

    Condition*
    ExpressionParser::read_feed_impressions(std::wistream& istr,
                                            Condition* left_op)
      throw(ParseError, Exception, El::Exception)
    {
      TokenType type = read_token(istr);

      if(type != TT_REGULAR && type != TT_NOT)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_feed_impressions: "
          "feed message impression count or word NOT is expected at position "
             << last_token_begins_;
        
        throw FeedImpressionsOrNotExpected(ostr.str().c_str(),
                                           last_token_begins_);
      }

      bool reversed = type == TT_NOT;

      if(!reversed)
      {
        put_back();
      }

      type = read_token(istr);

      if(type != TT_REGULAR)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_feed_impressions: "
          "feed message impression count is expected at position "
             << last_token_begins_;

        throw FeedImpressionsExpected(ostr.str().c_str(), last_token_begins_);
      }

      uint64_t impressions_value = 0;
      
      if(!El::String::Manip::numeric(last_token_.c_str(), impressions_value))
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_feed_impressions: "
          "non-negative integer is expected as feed message impression count "
          "at position " << last_token_begins_;
        
        throw WrongFeedImpressions(ostr.str().c_str(), last_token_begins_);
      }
  
      FeedImpressions* impressions = new FeedImpressions();
      Condition_var condition = impressions;
      
      impressions->value = impressions_value;
      impressions->reversed = reversed;

      El::RefCount::add_ref(left_op);
      impressions->condition = left_op;

      return condition.retn();
    }

    Condition*
    ExpressionParser::read_clicks(std::wistream& istr, Condition* left_op)
      throw(ParseError, Exception, El::Exception)
    {
      TokenType type = read_token(istr);

      if(type != TT_REGULAR && type != TT_NOT)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_clicks: "
          "message click count or word NOT is expected at position "
             << last_token_begins_;
        
        throw ClicksOrNotExpected(ostr.str().c_str(), last_token_begins_);
      }

      bool reversed = type == TT_NOT;

      if(!reversed)
      {
        put_back();
      }

      type = read_token(istr);

      if(type != TT_REGULAR)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_clicks: "
          "message click count is expected at position " << last_token_begins_;

        throw ClicksExpected(ostr.str().c_str(), last_token_begins_);
      }

      uint64_t clicks_value = 0;
      
      if(!El::String::Manip::numeric(last_token_.c_str(), clicks_value))
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_clicks: "
          "non-negative integer is expected as message click count "
          "at position " << last_token_begins_;
        
        throw WrongClicks(ostr.str().c_str(), last_token_begins_);
      }
  
      Clicks* clicks = new Clicks();
      Condition_var condition = clicks;
      
      clicks->value = clicks_value;
      clicks->reversed = reversed;

      El::RefCount::add_ref(left_op);
      clicks->condition = left_op;

      return condition.retn();
    }

    Condition*
    ExpressionParser::read_feed_clicks(std::wistream& istr, Condition* left_op)
      throw(ParseError, Exception, El::Exception)
    {
      TokenType type = read_token(istr);

      if(type != TT_REGULAR && type != TT_NOT)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_feed_clicks: "
          "feed message click count or word NOT is expected at position "
             << last_token_begins_;
        
        throw FeedClicksOrNotExpected(ostr.str().c_str(), last_token_begins_);
      }

      bool reversed = type == TT_NOT;

      if(!reversed)
      {
        put_back();
      }

      type = read_token(istr);

      if(type != TT_REGULAR)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_feed_clicks: "
          "feed message click count is expected at position "
             << last_token_begins_;

        throw FeedClicksExpected(ostr.str().c_str(), last_token_begins_);
      }

      uint64_t clicks_value = 0;
      
      if(!El::String::Manip::numeric(last_token_.c_str(), clicks_value))
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_feed_clicks: "
          "non-negative integer is expected as feed message click count "
          "at position " << last_token_begins_;
        
        throw WrongFeedClicks(ostr.str().c_str(), last_token_begins_);
      }
  
      FeedClicks* clicks = new FeedClicks();
      Condition_var condition = clicks;
      
      clicks->value = clicks_value;
      clicks->reversed = reversed;

      El::RefCount::add_ref(left_op);
      clicks->condition = left_op;

      return condition.retn();
    }

    Condition*
    ExpressionParser::read_ctr(std::wistream& istr, Condition* left_op)
      throw(ParseError, Exception, El::Exception)
    {
      TokenType type = read_token(istr);

      if(type != TT_REGULAR && type != TT_NOT)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_ctr: "
          "message CTR or word NOT is expected at position "
             << last_token_begins_;
        
        throw CTR_OrNotExpected(ostr.str().c_str(), last_token_begins_);
      }

      bool reversed = type == TT_NOT;

      if(!reversed)
      {
        put_back();
      }

      type = read_token(istr);

      if(type != TT_REGULAR)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_ctr: "
          "message CTR is expected at position "
             << last_token_begins_;

        throw CTR_Expected(ostr.str().c_str(), last_token_begins_);
      }

      float ctr_value = 0;
      
      if(!El::String::Manip::numeric(last_token_.c_str(), ctr_value))
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_ctr: "
          "non-negative float number is expected as message CTR "
          "at position " << last_token_begins_;
        
        throw WrongCTR(ostr.str().c_str(), last_token_begins_);
      }
  
      CTR* ctr = new CTR();
      Condition_var condition = ctr;
      
      ctr->value = ctr_value;
      ctr->reversed = reversed;

      El::RefCount::add_ref(left_op);
      ctr->condition = left_op;

      return condition.retn();
    }

    Condition*
    ExpressionParser::read_feed_ctr(std::wistream& istr, Condition* left_op)
      throw(ParseError, Exception, El::Exception)
    {
      TokenType type = read_token(istr);

      if(type != TT_REGULAR && type != TT_NOT)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_feed_ctr: "
          "feed message CTR or word NOT is expected at position "
             << last_token_begins_;
        
        throw FeedCTR_OrNotExpected(ostr.str().c_str(), last_token_begins_);
      }

      bool reversed = type == TT_NOT;

      if(!reversed)
      {
        put_back();
      }

      type = read_token(istr);

      if(type != TT_REGULAR)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_feed_ctr: "
          "feed message CTR is expected at position "
             << last_token_begins_;

        throw FeedCTR_Expected(ostr.str().c_str(), last_token_begins_);
      }

      float ctr_value = 0;
      
      if(!El::String::Manip::numeric(last_token_.c_str(), ctr_value))
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_feed_ctr: "
          "non-negative float number is expected as feed message CTR "
          "at position " << last_token_begins_;
        
        throw WrongFeedCTR(ostr.str().c_str(), last_token_begins_);
      }
  
      FeedCTR* ctr = new FeedCTR();
      Condition_var condition = ctr;
      
      ctr->value = ctr_value;
      ctr->reversed = reversed;

      El::RefCount::add_ref(left_op);
      ctr->condition = left_op;

      return condition.retn();
    }

    Condition*
    ExpressionParser::read_rctr(std::wistream& istr, Condition* left_op)
      throw(ParseError, Exception, El::Exception)
    {
      TokenType type = read_token(istr);

      if(type != TT_REGULAR && type != TT_NOT)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_rctr: "
          "message RCTR or word NOT is expected at position "
             << last_token_begins_;
        
        throw RCTR_OrNotExpected(ostr.str().c_str(), last_token_begins_);
      }

      bool reversed = type == TT_NOT;

      if(!reversed)
      {
        put_back();
      }

      type = read_token(istr);

      if(type != TT_REGULAR)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_rctr: "
          "message RCTR is expected at position "
             << last_token_begins_;

        throw RCTR_Expected(ostr.str().c_str(), last_token_begins_);
      }

      float rctr_value = 0;
      
      if(!El::String::Manip::numeric(last_token_.c_str(), rctr_value))
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_rctr: "
          "non-negative float number is expected as message RCTR "
          "at position " << last_token_begins_;
        
        throw WrongRCTR(ostr.str().c_str(), last_token_begins_);
      }

      RCTR* rctr = new RCTR();
      Condition_var condition = rctr;
      
      rctr->value = rctr_value;
      rctr->reversed = reversed;
      
      type = read_token(istr);

      if(type == TT_REGULAR)
      {
        if(!El::String::Manip::numeric(last_token_.c_str(),
                                       rctr->respected_impression_level))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Search::ExpressionParser::read_rctr: "
            "non-negative number is expected as a respected impression level "
            "at position " << last_token_begins_;
        
          throw WrongRIL(ostr.str().c_str(), last_token_begins_);
        }
      }
      else
      {
        rctr->ril_is_default = 1;
        rctr->respected_impression_level = respected_impression_level_;
        
        put_back();
      }  

      El::RefCount::add_ref(left_op);
      rctr->condition = left_op;

      return condition.retn();
    }

    Condition*
    ExpressionParser::read_feed_rctr(std::wistream& istr, Condition* left_op)
      throw(ParseError, Exception, El::Exception)
    {
      TokenType type = read_token(istr);

      if(type != TT_REGULAR && type != TT_NOT)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_feed_rctr: "
          "feed message RCTR or word NOT is expected at position "
             << last_token_begins_;
        
        throw FeedRCTR_OrNotExpected(ostr.str().c_str(), last_token_begins_);
      }

      bool reversed = type == TT_NOT;

      if(!reversed)
      {
        put_back();
      }

      type = read_token(istr);

      if(type != TT_REGULAR)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_feed_rctr: "
          "feed message RCTR is expected at position "
             << last_token_begins_;

        throw FeedRCTR_Expected(ostr.str().c_str(), last_token_begins_);
      }

      float rctr_value = 0;
      
      if(!El::String::Manip::numeric(last_token_.c_str(), rctr_value))
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_feed_rctr: "
          "non-negative float number is expected as feed message RCTR "
          "at position " << last_token_begins_;
        
        throw WrongFeedRCTR(ostr.str().c_str(), last_token_begins_);
      }

      FeedRCTR* rctr = new FeedRCTR();
      Condition_var condition = rctr;
      
      rctr->value = rctr_value;
      rctr->reversed = reversed;
      
      type = read_token(istr);

      if(type == TT_REGULAR)
      {
        if(!El::String::Manip::numeric(last_token_.c_str(),
                                       rctr->respected_impression_level))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Search::ExpressionParser::read_feed_rctr: "
            "non-negative number is expected as a respected impression level "
            "at position " << last_token_begins_;
        
          throw WrongFeedRIL(ostr.str().c_str(), last_token_begins_);
        }
      }
      else
      {
        rctr->ril_is_default = 1;
        rctr->respected_impression_level = respected_impression_level_;
        
        put_back();
      }  

      El::RefCount::add_ref(left_op);
      rctr->condition = left_op;

      return condition.retn();
    }

    Condition*
    ExpressionParser::read_signature(std::wistream& istr,
                                     Condition* left_op)
      throw(ParseError, Exception, El::Exception)
    {
      TokenType type = read_token(istr);

      if(type != TT_REGULAR && type != TT_NOT)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_signature: "
          "message signature or word NOT is expected at position "
             << last_token_begins_;
        
        throw SignatureOrNotExpected(ostr.str().c_str(), last_token_begins_);
      }

      bool reversed = type == TT_NOT;

      if(!reversed)
      {
        put_back();
      }

      typedef std::vector<uint64_t> SignatureVector;
      SignatureVector signatures;

      while((type = read_token(istr)) == TT_REGULAR)
      {
        uint64_t value = 0;
      
        if(!El::String::Manip::numeric(last_token_.c_str(),
                                       value,
                                       El::String::Manip::NF_HEX))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Search::ExpressionParser::read_signature: "
            "unsigned hexadecimal integer is expected as message signature at "
            "position " << last_token_begins_;
          
          throw WrongSignature(ostr.str().c_str(), last_token_begins_);
        }
        
        signatures.push_back(value);
      }

      if(signatures.empty())
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Search::ExpressionParser::read_signature: "
          "message signature is expected at position " << last_token_begins_;

        throw SignatureExpected(ostr.str().c_str(), last_token_begins_);
      }

      if(type != TT_UNDEFINED)
      {
        put_back();
      }
      
      Signature* signature = new Signature();
      Condition_var condition = signature;
      
      signature->values.init(signatures);
      signature->reversed = reversed;

      El::RefCount::add_ref(left_op);
      signature->condition = left_op;

      return condition.retn();
    }

    Condition*
    ExpressionParser::read_and(std::wistream& istr, Condition* left_op)
      throw(ParseError, Exception, El::Exception)
    {
      And* and_ptr = new And();
      Condition_var condition = and_ptr;

      El::RefCount::add_ref(left_op);
      and_ptr->operands.push_back(left_op);
      
      while(true)
      {        
        and_ptr->operands.push_back(read_operand(istr));

        TokenType type = read_token(istr);
        
        if(type == TT_UNDEFINED)
        {
          break;
        }

        if(type != TT_AND)
        {
          put_back();
          break;
        }        
      }
      
      return condition.retn();
    }
    
    Condition*
    ExpressionParser::read_except(std::wistream& istr, Condition* left_op)
      throw(ParseError, Exception, El::Exception)
    {
      Except* except = new Except();
      Condition_var condition = except;

      El::RefCount::add_ref(left_op);
      except->left = left_op;
      except->right = read_operand(istr);
      
      return condition.retn();
    }
    
    Condition*
    ExpressionParser::read_or(std::wistream& istr, Condition* left_op)
      throw(ParseError, Exception, El::Exception)
    {
      Condition_var condition;
      
      Condition_var right_op = read_condition(istr, 0);
      Or* rigth_or = dynamic_cast<Or*>(right_op.in());
        
      if(rigth_or != 0)
      {
        El::RefCount::add_ref(left_op);

        ConditionArray& ops = rigth_or->operands;
        ops.insert(ops.begin(), left_op);

        condition = right_op;
      }
      else
      {
        Or* or_ptr = new Or();
        condition = or_ptr;

        El::RefCount::add_ref(left_op);
        or_ptr->operands.push_back(left_op);        
        or_ptr->operands.push_back(right_op);
      }

      return condition.retn();
    }
/*
    void
    dump(const wchar_t* val)
    {
      El::String::Manip::wchar_to_utf8(val, std::cerr);
      std::cerr << ":";

      for(; *val != L'\0'; ++val)
      {
        std::cerr << " " << std::hex << std::uppercase << (unsigned long)*val;
      }

      std::cerr << std::endl;
    }
*/
    bool
    ExpressionParser::read_words(std::wistream& istr, WordList& words)
      throw(ParseError, Exception, El::Exception)
    {
      words.clear();
      
      TokenType type;
      while((type = read_token(istr)) == TT_REGULAR)
      {
        std::wstring lowered;
        El::String::Manip::to_uniform(last_token_.c_str(), lowered);

//        dump(last_token_.c_str());
//        dump(lowered.c_str());
        
        if(last_token_quoted_ != L'\0')
        {
          unsigned char group = 1;
          bool exact = last_token_quoted_ == L'"';

          if(words.rbegin() != words.rend())
          {
            group = words.rbegin()->group() == 1 ? 2 : 1;
          }
            
          std::wstring word;
          unsigned long word_count = 0;
          
          for(const wchar_t* ptr = lowered.c_str(); *ptr != L'\0'; ++ptr)
          {
            if(El::String::Unicode::CharTable::is_space(*ptr))
            {
              if(push_word(word, words, group, exact))
              {
                ++word_count;
              }              
            }
            else
            {
              word.append(ptr, 1);
            }
          }

          if(push_word(word, words, group, exact))
          {
            ++word_count;
          }

          if(word_count == 1)
          {
            words.rbegin()->group(0);
          }
        }
        else
        {          
          push_word(lowered, words, 0, false);
        }
      }

      if(type != TT_UNDEFINED)
      {
        put_back();
      }

      return !words.empty();
    }
    
    ExpressionParser::TokenType 
    ExpressionParser::read_token(std::wistream& istr)
      throw(ParseError, Exception, El::Exception)
    {
      if(buffered_)
      {
        buffered_ = false;        
        return last_token_type_;
      }
      
      last_token_.clear();
      last_token_type_ = TT_UNDEFINED;
      last_token_begins_ = 0;
      last_token_quoted_ = L'\0';

      wchar_t chr;

      while(!istr.get(chr).fail())
      {
        ++position_;

        if(!El::String::Unicode::CharTable::is_space(chr))
        {
          break;
        }
      }

      bool reading_token = !istr.fail();

      if(reading_token)
      {
        last_token_begins_ = position_ - 1;

        Message::StoredMessage::normalize_char(chr);

        if(chr == L'"' || chr == L'\'')
        {
          last_token_quoted_ = chr;
          
          wchar_t prev_chr = 0;

          for(istr.get(chr); !istr.fail(); istr.get(chr))
          {
            ++position_;
            
            if(prev_chr == last_token_quoted_ &&
               El::String::Unicode::CharTable::is_space(chr))
            {
              break;
            }
                
            Message::StoredMessage::normalize_char(chr);
            
            last_token_.append(&chr, 1);
            prev_chr = chr;
          }

          if(prev_chr != last_token_quoted_)
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Search::ExpressionParser::read_token: "
              "unexpected end of stream at position " << position_;

            if(istr.eof())
            {
              throw UnexpectedEndOfExpression(ostr.str().c_str(), position_);
            }
            else
            {
              throw Exception(ostr.str());
            }
          }

          last_token_.resize(last_token_.length() - 1);
        }
        else
        {
          last_token_.append(&chr, 1);
          
          for(istr.get(chr); !istr.fail(); istr.get(chr))
          {
            ++position_;

            if(El::String::Unicode::CharTable::is_space(chr))
            {
              break;
            }
              
            Message::StoredMessage::normalize_char(chr);
            last_token_.append(&chr, 1);
          }
        }
      }
      
      if(!istr.fail() || (istr.eof() && reading_token))
      {
        if(last_token_quoted_ == L'\0')
        {
          for(unsigned long i = 0;
              i < sizeof(SPECIAL_WORD_TYPE) / sizeof(SPECIAL_WORD_TYPE[0]);
              ++i)
          {
            if(last_token_ == SPECIAL_WORD[i])
            {
              last_token_type_ = SPECIAL_WORD_TYPE[i];
              return last_token_type_;
            }
          }
        }
        
        last_token_type_ = TT_REGULAR;
        return last_token_type_;
      }

      if(!istr.eof())
      {
        throw Exception(
          "NewsGate::Search::ExpressionParser::read_token: reading "
          "stream failed");
      }

      last_token_begins_ = position_;
      return last_token_type_;
    }

    //
    // Expression struct
    //
    void
    Expression::dump(std::wostream& ostr) const throw(El::Exception)
    {
      std::wstring ident;
      condition->dump(ostr, ident);
    }

    void
    Expression::dump(std::ostream& ostr) const throw(El::Exception)
    {
      std::wostringstream wostr;
      dump(wostr);

      std::string val;
      El::String::Manip::wchar_to_utf8(wostr.str().c_str(), val);

      ostr << val;
    }

    Result*
    Expression::search_result(const Condition::Context& context,
                              bool copy_msg_struct,
                              const Condition::Result* cond_res,
                              const Strategy& strategy,
                              const Condition::MessageMatchInfoMap& match_info,
                              MessageWordPositionMap* mwp_map,
                              time_t* current_time)
      const throw(El::Exception)
    {
#     ifdef TRACE_SEARCH_RES_TIME
      ACE_Time_Value stat_tm;
      ACE_Time_Value cat_stat_tm;
      ACE_Time_Value cw_stat_tm;
      ACE_Time_Value wc_stat_tm;
      ACE_Time_Value pi_stat_tm;
        
      ACE_High_Res_Timer timer;
      ACE_High_Res_Timer timer2;
      ACE_Time_Value tm;
      
      timer.start();
#     endif

      uint64_t cur_time = current_time ?
        *current_time : ACE_OS::gettimeofday().sec();

      unsigned long date_watermark = 0;
      unsigned long topicality_date_watermark = 0;

      const Strategy::SortUseTime* sort_use_time =
        dynamic_cast<Strategy::SortUseTime*>(strategy.sorting.get());

      if(sort_use_time)
      {
        date_watermark = cur_time - sort_use_time->message_max_age;
        topicality_date_watermark = cur_time - TOPICALITY_DATE_RANGE;
      }

      float freshness_factor = 0;
      float rctr_factor = 0;
      float capacity_factor = 0;
      float coreness_factor = 0;
      
      Strategy::SortByRelevance* sort_by_relevance = 0;
      Strategy::SortByEventCapacity* sort_by_event_capacity = 0;
      Strategy::SortByPopularity* sort_by_popularity = 0;
      
      El::ArrayPtr<unsigned char> core_word_indexes_holder;
      unsigned char* core_word_indexes = 0;

      uint32_t event_max_size = 0;
      uint64_t impression_respected_level = 0;
      
      if((sort_by_relevance = dynamic_cast<Strategy::SortByRelevance*>(
            strategy.sorting.get())) != 0)
      {
        assert(sort_use_time != 0);

        core_word_indexes_holder.reset(
          new unsigned char[sort_by_relevance->max_core_words]);

        core_word_indexes = core_word_indexes_holder.get();
        
        freshness_factor =
          10000.0F * SORT_BY_RELEVANCE_DATE_WEIGHT /
          sort_by_relevance->message_max_age;

        coreness_factor = 10000.0F * SORT_BY_RELEVANCE_CORENESS_WEIGHT;        
      }
      else if((sort_by_event_capacity =
               dynamic_cast<Strategy::SortByEventCapacity*>(
                 strategy.sorting.get())) != 0)
      {
        assert(sort_use_time != 0);

        freshness_factor =
          10000.0F * SORT_BY_CAPACITY_DATE_WEIGHT /
          sort_use_time->message_max_age;

        event_max_size =
          std::max(sort_by_event_capacity->event_max_size, (uint32_t)1);

        capacity_factor = 10000.0F * SORT_BY_CAPACITY_CAPACITY_WEIGHT /
          event_max_size;

        rctr_factor = 10000.0F * SORT_BY_CAPACITY_RCTR_WEIGHT;

        impression_respected_level =
          sort_by_event_capacity->impression_respected_level;
      }
      else if((sort_by_popularity =
               dynamic_cast<Strategy::SortByPopularity*>(
                 strategy.sorting.get())) != 0)
      {
        assert(sort_use_time != 0);

        freshness_factor =
          10000.0F * SORT_BY_POPULARITY_DATE_WEIGHT /
          sort_use_time->message_max_age;

        rctr_factor = 10000.0F * SORT_BY_POPULARITY_RCTR_WEIGHT;

        impression_respected_level =
          sort_by_popularity->impression_respected_level;
      }

      uint8_t strategy_sorting_type = strategy.sorting->type();

      Strategy::SuppressionType suppression_type =
        strategy.suppression->type();
      
      bool core_words_required = suppression_type == Strategy::ST_SIMILAR ||
        suppression_type == Strategy::ST_COLLAPSE_EVENTS;

      bool fill_message_info = strategy.result_flags & Strategy::RF_MESSAGES;

      const Strategy::Filter& filter = strategy.filter;

      const El::Lang& filter_lang = filter.lang;
      bool filter_by_lang = filter_lang != El::Lang::null;

      const El::Country& filter_country = filter.country;
      bool filter_by_country = filter_country != El::Country::null;

      const Message::StringConstPtr& filter_feed = filter.feed;
      bool filter_by_feed = !filter_feed.empty();
      
      std::auto_ptr<Message::NumberSet> filter_cat;
      const Message::NumberSet* filter_category = 0;
      Message::NumberSet::const_iterator filter_category_end;
      
      if(!filter.category.empty())
      {
        std::string lower;
        El::String::Manip::utf8_to_lower(filter.category.c_str(), lower);

        if(lower[0] != '/')
        {
          lower = std::string("/") + lower;
        }

        if(lower[lower.length() - 1] != '/')
        {
          lower += "/";
        }
        
        const Message::Category* category =
          context.messages.root_category->find(lower.c_str() + 1);

        if(category)
        {
          filter_cat.reset(category->all_messages());
          filter_category = filter_cat.get();
          filter_category_end = filter_category->end();
        }
      }

      const El::Luid& filter_event = filter.event;
      bool filter_by_event = filter_event != El::Luid::null;
      
      bool collect_lang_stat = strategy.result_flags & Strategy::RF_LANG_STAT;
      
      bool collect_country_stat =
        strategy.result_flags & Strategy::RF_COUNTRY_STAT;
        
      bool collect_feed_stat =
        strategy.result_flags & Strategy::RF_FEED_STAT;

      bool collect_category_stat =
        strategy.result_flags & Strategy::RF_CATEGORY_STAT;

//      typedef __gnu_cxx::hash_set<El::String::StringConstPtr,
//        El::Hash::StringConstPtr> StringSet;

      ResultPtr result;

      if(fill_message_info)
      {
        result.reset(new Result(cond_res->size()));
      }
      else
      {
        result.reset(new Result());
      }

      Stat& stat = result->stat;
      
      LangCounterMap& lang_counter = stat.lang_counter;
      CountryCounterMap& country_counter = stat.country_counter;
      StringCounterMap& feed_counter = stat.feed_counter;
      StringCounterMap& category_counter = stat.category_counter;

      MessageInfoArray& message_infos = *(result->message_infos);

      unsigned long mi_index = 0;

      uint32_t& result_max_weight = result->max_weight;
      uint32_t& result_min_weight = result->min_weight;

//      const Message::FeedInfoMap& feeds = context.messages.feeds;
//      Message::FeedInfoMap::const_iterator feeds_end = feeds.end();

      if(collect_category_stat)
      {
        const Message::LocaleCategoryCounter& lcc =
          context.messages.category_counter;
        
        Message::LocaleCategoryCounter::const_iterator i =
          lcc.find(El::Locale(filter_lang, filter_country));

        if(i != lcc.end())
        {
          const Message::CategoryCounter& cc = *i->second;

          for(Message::CategoryCounter::const_iterator i(cc.begin()),
                e(cc.end()); i != e; ++i)
          {
            category_counter.absorb(i->first.c_str(), i->second);
          }
        }
      }

      bool search_hidden = strategy.search_hidden;
      
      Condition::MessageMatchInfoMap::const_iterator match_info_end =
        match_info.end();

#     ifdef TRACE_SEARCH_RES_TIME
      timer.stop();
      ACE_Time_Value prep_tm;
      timer.elapsed_time(prep_tm);

      timer.start();
#     endif
      
      for(Condition::Result::const_iterator it(cond_res->begin()),
            end(cond_res->end()); it != end; ++it)
      {
#     ifdef TRACE_SEARCH_RES_TIME
        timer2.start();
#     endif          
        
        const Message::StoredMessage& msg = *it->second;

        if(msg.hidden() != search_hidden)
        {
          continue;
        }

        Message::Number msg_num = it->first;
        
        bool valid_lang = !filter_by_lang || filter_lang == msg.lang;
        
        bool valid_country = !filter_by_country ||
          filter_country == msg.country;

        bool valid_feed = !filter_by_feed || filter_feed == msg.source_url;

        bool valid_category = !filter_category ||
          filter_category->find(msg_num) != filter_category_end;

        bool valid_event = !filter_by_event || filter_event == msg.event_id;

        bool valid = valid_lang && valid_country && valid_feed &&
          valid_category && valid_event;
        
        if(collect_lang_stat && valid_country && valid_feed &&
           valid_category && valid_event)
        {
          ++(lang_counter.insert(std::make_pair(msg.lang, 0)).first->second);
        }
        
        if(collect_country_stat && valid_lang && valid_feed &&
           valid_category && valid_event)
        {
          ++(country_counter.insert(std::make_pair(msg.country, 0)).first->
             second);
        }
        
        if(collect_feed_stat && valid_lang && valid_country &&
           valid_category && valid_event)
        {
          feed_counter.absorb(msg.source_url, msg.source_title, 0, valid);
        }
        
#     ifdef TRACE_SEARCH_RES_TIME
        timer2.stop();
        timer2.elapsed_time(tm);
        stat_tm += tm;
#     endif
        
        if(!valid)
        {
          continue;
        }

#     ifdef TRACE_SEARCH_RES_TIME
        timer2.start();
#     endif        
        
        if(collect_category_stat)
        {
          for(Message::Categories::CategoryArray::const_iterator
                p(msg.category_paths.begin()), e(msg.category_paths.end());
              p != e; ++p)
          {
            ++(category_counter.find(p->c_str())->second.count2);
          }
        }

#     ifdef TRACE_SEARCH_RES_TIME
        timer2.stop();
        timer2.elapsed_time(tm);
        cat_stat_tm += tm;
#     endif
        
        ++stat.total_messages;
          
        if(!fill_message_info)
        {
          continue;
        }

#     ifdef TRACE_SEARCH_RES_TIME
        timer2.start();
#     endif
        
        MessageInfo& wmi = message_infos[mi_index++];
        
        wmi.wid.id = msg.id;
        wmi.signature = msg.signature;
        wmi.url_signature = msg.url_signature;
        wmi.event_id = msg.event_id;
        
        if(core_words_required)
        {
          wmi.core_words(msg.core_words, copy_msg_struct);
        }

#     ifdef TRACE_SEARCH_RES_TIME
        timer2.stop();
        timer2.elapsed_time(tm);
        cw_stat_tm += tm;

        timer2.start();
#     endif
        
        uint32_t& weight = wmi.wid.weight;
        const Condition::MessageMatchInfo* mm_info = 0;
        
        switch(strategy_sorting_type)
        {
        case Strategy::SM_BY_RELEVANCE_DESC:
        case Strategy::SM_BY_RELEVANCE_ASC:
          {
            //
            // Each component of weight match should be normalized in
            // range [0, 10000] and multiplied by component weight
            //
            
#           ifdef TRACE_WEIGHT
            std::ostringstream ostr;
#           endif
            
#           ifdef TRACE_WEIGHT
            ostr << msg.id.string() << " tm:" << msg.fetched << " sh:"
                 << (msg.fetched < date_watermark ? 0 :
                     (msg.fetched - date_watermark));
#           endif

            //
            // Calculating date-based component. Weak but allows to sort
            // even badly matched results among each other.
            //

            weight = msg.fetched < date_watermark ?
              0 : (uint32_t)(freshness_factor *
                             (msg.fetched - date_watermark) + 0.5F);
            
#           ifdef TRACE_WEIGHT
            ostr << " fr:" << weight;
#           endif

/*            
            std::cerr << msg.id.string() << std::endl;
            std::cerr << "S: " << shift << ", F: " << freshness << std::endl;
*/
            
#           ifdef TRACE_WEIGHT
            ostr << " w1:" << weight;
#           endif
            
            //
            // Calculating topicality component. Stronger than date-based but
            // weaker then match level. Allows to emphasize on fresh news.
            //

            if(msg.fetched > topicality_date_watermark)
            {
#             ifdef TRACE_WEIGHT
              ostr << " sh:" << (msg.fetched - topicality_date_watermark);
#             endif

              weight += msg.fetched < cur_time ?
                topicality_[msg.fetched - topicality_date_watermark] :
                topicality_[TOPICALITY_DATE_RANGE - 1];
              
#             ifdef TRACE_WEIGHT
              ostr << " w2:" << weight;
#             endif
            }

            weight += msg.search_weight + *msg.feed_search_weight;

            //
            // Calculating match level - the strongest metrics.
            //
            Condition::MessageMatchInfoMap::const_iterator mit =
              match_info.find(msg_num);

            if(mit != match_info_end)
            {
              mm_info = &mit->second;

              if(mm_info->core_words.get())
              {
                const Condition::CoreWordsSet& matched_core_words =
                  *(mm_info->core_words);
              
                size_t index_count = 0;
              
                Condition::CoreWordsSet::const_iterator
                  matched_core_words_end = matched_core_words.end();
            
                for(Condition::CoreWordsSet::const_iterator
                      wit = matched_core_words.begin();
                    wit != matched_core_words_end; ++wit)
                { 
                  if(msg.core_words.find(*wit,
                                         core_word_indexes + index_count))
                  {
                    ++index_count;
                  }
                }

                if(index_count)
                {
                  std::sort(core_word_indexes,
                            core_word_indexes + index_count);

                  float core_words_weight = 0;
                  float factor = 0.5;
                
                  for(size_t i = 0; i < index_count; ++i)
                  { 
                    core_words_weight +=
                      factor * (msg.core_words.size() - core_word_indexes[i]) /
                      sort_by_relevance->max_core_words;

                    factor /= 2;
                  }
                
                  weight +=
                    (uint32_t)(coreness_factor * core_words_weight + 0.5);
                }
              }
            }
            
#           ifdef TRACE_WEIGHT
              ostr << " w5:" << weight;
#           endif

            if(strategy_sorting_type == Strategy::SM_BY_RELEVANCE_ASC)
            {
              // Invert
              weight = UINT32_MAX - weight;
              
#           ifdef TRACE_WEIGHT
              ostr << " w7:" << weight;
#           endif
            }

#           ifdef TRACE_WEIGHT
              ostr << std::endl;
              std::cerr << ostr.str();
#           endif
            
            break;
          }          
        case Strategy::SM_BY_PUB_DATE_DESC:
          {
            weight = msg.published < date_watermark ?
              0 : (msg.published - date_watermark);
            
            break;
          }
        case Strategy::SM_BY_FETCH_DATE_DESC:
          {
            weight = msg.fetched < date_watermark ?
              0 : (msg.fetched - date_watermark);
            
            break;
          }
        case Strategy::SM_BY_PUB_DATE_ASC:
          {
            weight = UINT32_MAX - (msg.published < date_watermark ?
                                   0 : (msg.published - date_watermark));
            
            break;
          }
        case Strategy::SM_BY_FETCH_DATE_ASC:
          {
            weight = UINT32_MAX - (msg.fetched < date_watermark ?
                                   0 : (msg.fetched - date_watermark));
            
            break;
          }
        case Strategy::SM_BY_EVENT_CAPACITY_DESC:
        case Strategy::SM_BY_EVENT_CAPACITY_ASC:
          {            
            uint32_t shift = msg.published < date_watermark ?
              0 : (msg.published - date_watermark);
//            uint32_t shift = msg.fetched < date_watermark ?
//              0 : (msg.fetched - date_watermark);
              
            uint32_t freshness =
              (uint32_t)(freshness_factor * shift + 0.5F);

            weight = freshness;

            uint32_t cap_weight =
              (uint32_t)(capacity_factor *
                         std::min(msg.event_capacity, event_max_size) + 0.5F);

            weight += cap_weight;

            uint64_t respected_impressions =
              std::max(msg.impressions, impression_respected_level);

            uint32_t rctr_weight =
              (uint32_t)(rctr_factor *
                         std::min(msg.clicks, msg.impressions) /
                         respected_impressions + 0.5F);
             
            weight += rctr_weight;
             
            if(msg.flags & Message::StoredMessage::MF_HAS_IMAGES)
            {
              weight +=
                (uint32_t)(SORT_BY_CAPACITY_IMAGE_WEIGHT * 10000);
            }

            if(strategy_sorting_type == Strategy::SM_BY_EVENT_CAPACITY_ASC)
            {
              // Invert
              weight = UINT32_MAX - weight;
            }

            break;
          }
        case Strategy::SM_BY_POPULARITY_DESC:
        case Strategy::SM_BY_POPULARITY_ASC:
          {
            uint32_t shift = msg.fetched < date_watermark ?
              0 : (msg.fetched - date_watermark);
              
            uint32_t freshness = (uint32_t)(freshness_factor * shift + 0.5F);

            weight = freshness;

            uint64_t respected_impressions =
              std::max(msg.impressions, impression_respected_level);

            if(respected_impressions)
            {
              uint32_t rctr_weight =
                (uint32_t)(rctr_factor *
                           std::min(msg.clicks, msg.impressions) /
                           respected_impressions + 0.5F);

              weight += rctr_weight;
            }
              
            if(msg.flags & Message::StoredMessage::MF_HAS_IMAGES)
            {
              weight +=
                (uint32_t)(SORT_BY_POPULARITY_IMAGE_WEIGHT * 10000);
            }

            if(strategy_sorting_type == Strategy::SM_BY_POPULARITY_ASC)
            {
              // Invert
              weight = UINT32_MAX - weight;
            }
            
            break;
          }
        case Strategy::SM_NONE:
          { // Set pretty random weigth
            // Zero weight is bad for take_top
            weight = mi_index;
            break;
          }
        }

        if(result_min_weight > weight)
        {
          result_min_weight = weight;
        }
        
        if(result_max_weight < weight)
        {
          result_max_weight = weight;
        }

#     ifdef TRACE_SEARCH_RES_TIME
        timer2.stop();
        timer2.elapsed_time(tm);
        wc_stat_tm += tm;

        timer2.start();
#     endif

        if(mwp_map)
        {
          if(mm_info == 0)
          {
            Condition::MessageMatchInfoMap::const_iterator mit =
              match_info.find(msg_num);
            
            if(mit != match_info_end)
            {
              mm_info = &mit->second;
            }
          }
            
          if(mm_info)
          {
            WordPositionArray& positions = (*mwp_map)[wmi.wid.id];
            const Condition::PositionSet* pset = mm_info->positions.get();

            if(pset != 0)
            {
              for(Condition::PositionSet::const_iterator sit(pset->begin()),
                    end(pset->end()); sit != end; ++sit)
              {
                Message::WordPosition pos = *sit;
                WordPositionArray::iterator pa_it(positions.begin());
                WordPositionArray::iterator pa_end(positions.end());
                
                for(; pa_it != pa_end && *pa_it < pos; ++pa_it);
                positions.insert(pa_it, pos);
              }
            }
          }
        }
        
#     ifdef TRACE_SEARCH_RES_TIME
        timer2.stop();
        timer2.elapsed_time(tm);
        pi_stat_tm += tm;
#     endif
      }

#     ifdef TRACE_SEARCH_RES_TIME
      timer.stop();
      
      ACE_Time_Value calc_tm;
      timer.elapsed_time(calc_tm);
#     endif

      message_infos.resize(mi_index);      

#     ifdef TRACE_SEARCH_RES_TIME
      
      std::cerr << "search_res: " << message_infos.size() << " msg; "
                << "prep " << El::Moment::time(prep_tm) << "; calc "
                << El::Moment::time(calc_tm) << "; flt "
                << El::Moment::time(stat_tm) << "; cat "
                << El::Moment::time(cat_stat_tm) << "; core "
                << El::Moment::time(cw_stat_tm) << "; wgh "
                << El::Moment::time(wc_stat_tm) << "; pos "
                << El::Moment::time(pi_stat_tm)
                << std::endl;
#     endif

      return result.release();
    }

    struct WidComparator
    {
      bool operator()(const WeightedId* a, const WeightedId* b) throw();
    };

    inline
    bool
    WidComparator::operator()(const WeightedId* a, const WeightedId* b) throw()
    {
      return a->weight != b->weight ?
        (a->weight > b->weight) : (a->id < b->id);
    }
    
    void
    Result::set_extras(const Message::SearcheableMessageMap& messages,
                       unsigned long flags,
                       uint32_t crc_init)
      throw(El::Exception)
    {
      if(message_infos.get() && (flags || crc_init))
      {
        for(MessageInfoArray::iterator i(message_infos->begin()),
              e(message_infos->end()); i != e; ++i)
        {
          MessageInfo& mi(*i);          
          const Message::StoredMessage* msg = messages.find(mi.wid.id);

          if(msg)
          {
            mi.extras.reset(new MessageInfo::Extras());
            MessageInfo::Extras& extras = *mi.extras;

            if(flags & SHF_UPDATED)
            {
              extras.published = msg->published;
            }
            
            uint32_t& state_hash = extras.state_hash;
            state_hash = crc_init;

            if(flags & SHF_STAT)
            {
              El::CRC(state_hash,
                      (const unsigned char*)&msg->impressions,
                      sizeof(msg->impressions));
              
              El::CRC(state_hash,
                      (const unsigned char*)&msg->clicks,
                      sizeof(msg->clicks));
            }

            if(flags & SHF_EVENT)
            {
              El::CRC(state_hash,
                      (const unsigned char*)&msg->event_capacity,
                      sizeof(msg->event_capacity));

              El::CRC(state_hash,
                      (const unsigned char*)&msg->event_id,
                      sizeof(msg->event_id));
            }
            
            if(flags & SHF_CATEGORY)
            {
              El::CRC(state_hash,
                      (const unsigned char*)&msg->categories.categorizer_hash,
                      sizeof(msg->categories.categorizer_hash));
            }
          }
          else
          {
            mi.extras.reset(0);
          }
        }        
      }
    }
    
    MessageInfoArray*
    Result::sort_skip_cut(size_t start_from,
                          size_t results_count,
                          const Strategy& strategy,
                          size_t* suppressed)
      throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(Expression::sort_skip_cut_meter);
      
      size_t total_res_count = start_from + results_count;

      if(total_res_count == 0 || message_infos->empty())
      {
        return new MessageInfoArray();
      }
      
      const Strategy::CollapseEvents* collapse_events_strategy =
        dynamic_cast<const Strategy::CollapseEvents*>(
          strategy.suppression.get());
      
      unsigned long msg_per_event = collapse_events_strategy ?
        collapse_events_strategy->msg_per_event : 0;      
        
      bool suppress_dups =
        strategy.suppression->type() != Strategy::ST_NONE;

      Strategy::SuppressSimilar* suppress_similar =
        dynamic_cast<Strategy::SuppressSimilar*>(strategy.suppression.get());

      uint32_t similarity_threshold = suppress_similar ?
        suppress_similar->similarity_threshold : 0;
      
      uint32_t containment_level = suppress_similar ?
        suppress_similar->containment_level : 0;

      uint32_t min_core_words = suppress_similar ?
        suppress_similar->min_core_words : 0;

      size_t suppr = 0;

//      std::sort(message_infos->begin(), message_infos->end());

      SignatureSet signatures;
      SignatureSet url_signatures;
      EventIdCounter msg_event_counter;

      if(suppress_dups)
      {
        signatures.resize(total_res_count);
        url_signatures.resize(total_res_count);
      }

      if(msg_per_event)
      {
        msg_event_counter.resize(total_res_count);
      }
      
      size_t start_skipped = 0;

      MessageInfoArrayPtr result(new MessageInfoArray());
      result->reserve(results_count);

      IdCounterMap id_counters;
      MessageInfoPtrMap message_info_map;
      WordIdMap word_ids;
      WordIdMap::iterator wi_end = word_ids.end();

      Strategy::SortByEventCapacity* sort_by_event_capacity =
        dynamic_cast<Strategy::SortByEventCapacity*>(
          strategy.sorting.get());      
      
      size_t sort_count = total_res_count *
        std::max(sort_by_event_capacity ?
                 sort_by_event_capacity->event_max_size : 10, (uint32_t)1);

      ::NewsGate::Search::MessageInfoArray::iterator i(message_infos->begin());
      ::NewsGate::Search::MessageInfoArray::iterator e(message_infos->end());
      ::NewsGate::Search::MessageInfoArray::iterator sort_bound(i);
      
      for(; i != e && result->size() < results_count; ++i)
      {
        if(i == sort_bound)
        {
          sort_bound = (size_t)(e - i) > sort_count ? i + sort_count : e;
          std::partial_sort(i, sort_bound, e);

          sort_count *= SORT_COUNT_FACTOR;
        }
        
        const MessageInfo& mi = *i;

        Message::Signature signature = mi.signature;
        Message::Signature url_signature = mi.url_signature;
        
        if(suppress_dups)
        {
          if(signature)
          {
            SignatureSet::const_iterator sit = signatures.find(signature);
            
            if(sit != signatures.end())
            {
              ++suppr;
              continue;
            }
          }
        
          if(url_signature)
          {
            SignatureSet::const_iterator sit =
              url_signatures.find(url_signature);
            
            if(sit != url_signatures.end())
            {
              ++suppr;
              continue;
            }
          }
        }

        bool record_event_id = false;
        const El::Luid& event_id = mi.event_id;
        EventIdCounter::iterator mit;
        
        if(msg_per_event && event_id != El::Luid::null)
        {
          mit = msg_event_counter.find(event_id);
            
          if(mit != msg_event_counter.end() && mit->second == msg_per_event)
          {
            ++suppr;
            continue;
          }

          record_event_id = true;          
        }

        if(suppress_similar)
        {
          const uint32_t* core_words = mi.core_words();
          uint8_t core_words_count = mi.core_words_count();

          if(core_words_count > min_core_words)
          {
            id_counters.clear();
            
            for(size_t i = 0; i < core_words_count; ++i)
            {
              WordIdMap::iterator it = word_ids.find(core_words[i]);
          
              if(it == wi_end)
              {
                continue;
              }
          
              const IdSet& id_set = *it->second;

              for(IdSet::const_iterator i(id_set.begin()), e(id_set.end());
                  i != e; ++i)
              {
                const Message::Id& id = *i;
                IdCounterMap::iterator idc = id_counters.find(id);

                if(idc == id_counters.end())
                {
                  id_counters[id] = 1;
                }
                else
                {
                  ++(idc->second);
                }
              }
            }

            IdCounterMap::iterator ic(id_counters.begin());
            IdCounterMap::iterator ie(id_counters.end());
              
            for(; ic != ie; ++ic)
            {
              uint32_t count = ic->second;
              
              MessageInfoPtrMap::const_iterator it =
                message_info_map.find(ic->first);
              
              assert(it != message_info_map.end());

              uint8_t cwc = it->second->core_words_count();
            
              if(// Significant intersection of bigger post with smaller one
                 100.0 * count / std::max(core_words_count, cwc) >=
                 similarity_threshold ||
                 
                 // Significant intersection of smaller post with bigger one
                 // if matching at 100% means containment
                 100.0 * count / std::min(core_words_count, cwc) >=
                 containment_level)
              {
                break;
              }              
            }

            if(ic != ie)
            {
              ++suppr;
              continue;
            }

            message_info_map[mi.wid.id] = &mi;
        
            for(size_t i = 0; i < core_words_count; ++i)
            {
              uint32_t id = core_words[i];
          
              WordIdMap::iterator wit = word_ids.find(id);

              if(wit == wi_end)
              {
                wit = word_ids.insert(std::make_pair(id, new IdSet())).first;
                wi_end = word_ids.end();
              }
          
              wit->second->insert(mi.wid.id);
            }
          }
        }
        
        if(record_event_id)
        {
          if(mit == msg_event_counter.end())
          {
            msg_event_counter.insert(std::make_pair(event_id, 1));
          }
          else
          {
            ++(mit->second);
          }
        }
        
        if(suppress_dups)
        {
          if(signature)
          {
            signatures.insert(signature);
          }
          
          if(url_signature)
          {
            url_signatures.insert(url_signature);
          }
        }
        
        if(start_skipped < start_from)
        {
          ++start_skipped;
          continue;
        }

        result->push_back(mi);
      }

      if(suppressed)
      {
        *suppressed = suppr;
      }
      
      return result.release();
    }
    
    MessageInfoArray*
    Result::sort_and_cut(size_t start_from,
                         size_t results_count,
                         const Strategy& strategy,
                         size_t* suppressed,
                         IdSet* suppressed_messages) const
      throw(El::Exception)
    {
      El::Stat::TimeMeasurement measurement(Expression::sort_and_cut_meter);

      if(suppressed)
      {
        *suppressed = 0;
      }
      
      size_t matched_message_count = message_infos->size();

      if(matched_message_count <= start_from || !results_count)
      {
        return new MessageInfoArray();
      }

      size_t intermediate_results_count = start_from + results_count;      

      size_t range_count =
        std::min((size_t)sqrt(matched_message_count),
                 std::max(matched_message_count / intermediate_results_count,
                          (size_t)1));
      
      unsigned long diapason = max_weight - min_weight + 1;

      SignatureMap signatures;
      SignatureMap url_signatures;

      bool suppress_dups = strategy.suppression->type() != Strategy::ST_NONE;
    
      if(suppress_dups)
      {
        signatures.resize(message_infos->size());
        url_signatures.resize(message_infos->size());
      }

      MessageInfoMapArray ranges(range_count);

      ::NewsGate::Search::MessageInfoArray::const_iterator mi_end =
          message_infos->end();

#     ifdef TRACE_SORT_TIME
        ACE_High_Res_Timer timer;
        ACE_Time_Value rg_tm;
        ACE_Time_Value rz_tm;
        ACE_Time_Value sd_tm;
        ACE_Time_Value in_tm;
        ACE_Time_Value ri1_tm;
        ACE_Time_Value ri2_tm;
        ACE_Time_Value ri3_tm;
        ACE_Time_Value ri4_tm;
        size_t shortcuts = 0;
        size_t range_passes = 0;
        size_t total_range_pass = 0;
#     endif

      for(::NewsGate::Search::MessageInfoArray::const_iterator it =
            message_infos->begin(); it != mi_end; ++it)
      {
        const MessageInfo& mi = *it;
        const WeightedId& wid = mi.wid;
        
        if(suppressed_messages &&
           suppressed_messages->find(wid.id) != suppressed_messages->end())
        {
          if(suppressed)
          {
            ++(*suppressed);
          }
          
          continue;
        }
        
        unsigned long index = get_index(wid, range_count, diapason);

        if(index >= ranges.size())
        {
#       ifdef TRACE_SORT_TIME
          ++shortcuts;
#        endif
          continue;
        }

        MessageInfoConstPtrMap& range = ranges[index];

//        std::cerr << range.size() << " * " << index << " ? "
//                  << intermediate_results_count << " :";

        if(range.size() * index > intermediate_results_count)
        {
#       ifdef TRACE_SORT_TIME
          timer.start();
#        endif
          
          size_t msg_count = 0;
        
          MessageInfoMapArray::const_iterator j = ranges.begin();
          MessageInfoMapArray::const_iterator end = j + index;        
          
          for(; j <= end; ++j)
          {
            msg_count += j->size();

//            std::cerr << " " << j->size();

            if(msg_count >= intermediate_results_count)
            {
              break;
            }
          }

#       ifdef TRACE_SORT_TIME
          timer.stop();
          ACE_Time_Value tm;
          timer.elapsed_time(tm);
          rg_tm += tm;

          size_t range_pass = j - ranges.begin();

          if(range_pass)
          {
            ++range_passes;
            total_range_pass += range_pass;
          }
          
#       endif

//          std::cerr << std::endl;
          
          if(j < end)
          {
//            std::cerr << "CUT " << (j - ranges.begin() + 1) << std::endl;
            
#       ifdef TRACE_SORT_TIME
            timer.start();
#       endif
          
            ranges.resize(j - ranges.begin() + 1);

#       ifdef TRACE_SORT_TIME
            timer.stop();
            ACE_Time_Value tm;
            timer.elapsed_time(tm);
            rz_tm += tm;
#       endif
          
            continue;
          }
/*          
          else
          {
            std::cerr << "NOCUT" << std::endl;
          }
*/
        }
/*        
        else
        {
          std::cerr << "\nNOTRY" << std::endl;
        }
*/
        
        if(suppress_dups)
        {
#       ifdef TRACE_SORT_TIME
          timer.start();
#       endif
          //
          // Handling "full" identity
          //
            
          Message::Signature msg_signature = mi.signature;

          if(!check_uniqueness(wid,
                               msg_signature,
                               signatures,
                               range_count,
                               diapason,
                               ranges,
                               suppressed,
                               suppressed_messages))
          {
            continue;
          }

          Message::Signature url_signature = mi.url_signature;

          if(!check_uniqueness(wid,
                               url_signature,
                               url_signatures,
                               range_count,
                               diapason,
                               ranges,
                               suppressed,
                               suppressed_messages))
          {
            continue;
          }
          
          if(msg_signature)
          {
            signatures[msg_signature] = wid;
          }

          if(url_signature)
          {
            url_signatures[url_signature] = wid;
          }

#       ifdef TRACE_SORT_TIME
          timer.stop();
          ACE_Time_Value tm;
          timer.elapsed_time(tm);
          sd_tm += tm;
#       endif          
        }
        
#       ifdef TRACE_SORT_TIME
        timer.start();
#       endif
          
        range.insert(std::make_pair(wid.id, &mi));
        
#       ifdef TRACE_SORT_TIME
        timer.stop();
        ACE_Time_Value tm;
        timer.elapsed_time(tm);
        in_tm += tm;
#       endif
      }

#     ifdef TRACE_SORT_TIME
      timer.start();
#     endif

      bool single_range = ranges.size() == 1;
      MessageInfoConstPtrMap united_result;
      
      MessageInfoConstPtrMap& result_infos = single_range ? ranges[0]
        : united_result;

      size_t skip_first = 0;

      if(single_range)
      {
        skip_first = start_from;
      }
      else
      {
        size_t elem_to_sort = 0;
        size_t size = 0;
        
        for(MessageInfoMapArray::const_iterator i(ranges.begin()),
              e(ranges.end()); i != e &&
              elem_to_sort < intermediate_results_count;
            ++i, elem_to_sort += size)
        {
          const MessageInfoConstPtrMap& map = *i;
          
          size = map.size();
          
          if(!size || elem_to_sort + size <= start_from)
          {
            continue;
          }
          
          if(elem_to_sort < start_from)
          {
            skip_first = start_from - elem_to_sort;
          }
          
          for(MessageInfoConstPtrMap::const_iterator it(map.begin()),
                end(map.end()); it != end; ++it)
          {
            const MessageInfo* mi = it->second;
            result_infos[mi->wid.id] = mi;
          }
        }
      }

#     ifdef TRACE_SORT_TIME
      timer.stop();
      timer.elapsed_time(ri1_tm);
      timer.start();
#     endif
      
      typedef std::vector<const WeightedId*> WeightedIdArray;
      WeightedIdArray wids;

      wids.reserve(result_infos.size());

      MessageInfoConstPtrMap::const_iterator ri_end = result_infos.end();
      
      for(MessageInfoConstPtrMap::const_iterator it = result_infos.begin();
          it != ri_end; ++it)
      {
        wids.push_back(&it->second->wid);
      }

#     ifdef TRACE_SORT_TIME
      timer.stop();
      timer.elapsed_time(ri2_tm);
      timer.start();
#     endif

      size_t result_size =
        std::min(results_count,
                 (size_t)(wids.size() > skip_first ?
                          wids.size() - skip_first : 0));
      
      WidComparator cmp;
      
      std::partial_sort(wids.begin(),
                        wids.begin() + skip_first + result_size,
                        wids.end(),
                        cmp);

#     ifdef TRACE_SORT_TIME
      timer.stop();
      timer.elapsed_time(ri3_tm);
      timer.start();
#     endif
        
      MessageInfoArrayPtr result(new MessageInfoArray(result_size));
      WeightedIdArray::const_iterator iw(wids.begin() + skip_first);

      for(MessageInfoArray::iterator i(result->begin()), e(result->end());
          i != e; ++i, ++iw)
      {
        *i = *result_infos[(*iw)->id];
      }
      
#     ifdef TRACE_SORT_TIME
      timer.stop();
      timer.elapsed_time(ri4_tm);

      std::cerr << "  rg_tm: " << El::Moment::time(rg_tm)
                << "; rz_tm: " << El::Moment::time(rz_tm)
                << "; sd_tm: " << El::Moment::time(sd_tm)
                << "; in_tm: " << El::Moment::time(in_tm)
                << "; st: " << shortcuts << "/" << range_passes << "/"
                << total_range_pass
                << "; ri_tms: " << El::Moment::time(ri1_tm) << "/"
                << El::Moment::time(ri2_tm) << "/"
                << El::Moment::time(ri3_tm) << "/"
                << El::Moment::time(ri4_tm)
                << std::endl;
/*
      size_t ind = 0;
      
      for(MessageInfoMapArray::const_iterator i(ranges.begin()),
            e(ranges.end()); i != e; ++i)
      {
        std::cerr << ind++ << " " << i->size() << std::endl;
      }
*/

#     endif
      
      return result.release();
    }

    size_t
    Result::remove_similar(MessageInfoArray* message_infos,
                           size_t total_required,
                           const Strategy& strategy,
                           size_t* suppressed,
                           IdSet* suppressed_messages)
      throw(InvalidArg, El::Exception)
    {
      El::Stat::TimeMeasurement measurement(Expression::remove_similar_meter);
      
      Strategy::SuppressSimilar* suppress_similar =
        dynamic_cast<Strategy::SuppressSimilar*>(strategy.suppression.get());
        
      if(suppress_similar == 0)
      {
        std::ostringstream ostr;
        ostr << "Result::remove_similar: unexpected Strategy::suppress_mode "
             << (unsigned long)strategy.suppression->type();
        
        throw InvalidArg(ostr.str());
      }

      WordIdMap word_ids;
      WordIdMap::iterator wi_end = word_ids.end();
      
      MessageInfoPtrMap message_info_map;

      size_t different_messages = 0;
      size_t i = 0;
      
      IdCounterMap id_counters;

      ::NewsGate::Search::MessageInfoArray::iterator mi_end =
          message_infos->end();
      
      for(::NewsGate::Search::MessageInfoArray::iterator miit =
            message_infos->begin(); miit != mi_end &&
            different_messages < total_required; ++miit, ++i)
      {
        MessageInfo& msg_info = *miit;

        const uint32_t* core_words = msg_info.core_words();
        uint8_t core_words_count = msg_info.core_words_count();        

        if(core_words_count < suppress_similar->min_core_words)
        {
          ++different_messages;
          continue;
        }
        
        El::Stat::TimeMeasurement
          measurement1(Expression::remove_similar_p1_meter);
        
        id_counters.clear();
        
        for(size_t i = 0; i < core_words_count; ++i)
        {
          WordIdMap::iterator it = word_ids.find(core_words[i]);
          
          if(it == wi_end)
          {
            continue;
          }
          
          const IdSet& id_set = *it->second;
          IdSet::const_iterator is_end = id_set.end();

          for(IdSet::const_iterator iit = id_set.begin(); iit != is_end; ++iit)
          {
            const Message::Id& id = *iit;
            IdCounterMap::iterator idc = id_counters.find(id);

            if(idc == id_counters.end())
            {
              id_counters[id] = 1;
            }
            else
            {
              ++(idc->second);
            }
          }  
        }

        measurement1.stop();

        El::Stat::TimeMeasurement
          measurement2(Expression::remove_similar_p2_meter);
        
        IdCounterMap::iterator iit = id_counters.begin();
        IdCounterMap::iterator ic_end = id_counters.end();
        MessageInfoPtrMap::const_iterator mit_end = message_info_map.end();
        
        for(; iit != ic_end; ++iit)
        {          
          uint8_t max_core_words = core_words_count;
          uint8_t min_core_words = core_words_count;

          MessageInfoPtrMap::const_iterator mit =
            message_info_map.find(iit->first);
          
          if(mit != mit_end)
          {
            uint8_t cwc = mit->second->core_words_count();
            
            max_core_words = std::max(max_core_words, cwc);
            min_core_words = std::min(min_core_words, cwc);
          }
             
          float match = 100.0 * iit->second / max_core_words;

          if(match >= suppress_similar->similarity_threshold)
          {
            // Significant intersection of bigger post with smaller one
            break;
          }

          match = 100.0 * iit->second / min_core_words;

          if(match >= suppress_similar->containment_level)
          {
            // Significant intersection of smaller post with bigger one
            // if matching at 100% means containment
            break;
          }
        }

        if(iit != ic_end)
        {
          if(suppressed_messages)
          {
            suppressed_messages->insert(msg_info.wid.id);
          }
          
          msg_info.wid = WeightedId::zero;

          if(suppressed)
          {
            ++(*suppressed);
          }
          
          continue;
        }

        ++different_messages;

        message_info_map[msg_info.wid.id] = &msg_info;
        
        for(size_t i = 0; i < core_words_count; ++i)
        {
          uint32_t id = core_words[i];
          
          WordIdMap::iterator wit = word_ids.find(id);

          if(wit == wi_end)
          {
            wit = word_ids.insert(std::make_pair(id, new IdSet())).first;
            wi_end = word_ids.end();
          }
          
          wit->second->insert(msg_info.wid.id);
        }
      }

      message_infos->resize(i);

      return different_messages;
    }

    size_t
    Result::collapse_events(MessageInfoArray* message_infos,
                            size_t total_required,
                            const Strategy& strategy,
                            size_t* suppressed,
                            IdSet* suppressed_messages)
      throw(InvalidArg, El::Exception)
    {
      const Strategy::CollapseEvents* collapse_events_strategy =
        dynamic_cast<const Strategy::CollapseEvents*>(
          strategy.suppression.get());

      assert(collapse_events_strategy != 0);
      unsigned long msg_per_event = collapse_events_strategy->msg_per_event;
        
      El::Stat::TimeMeasurement measurement(Expression::collapse_events_meter);

      EventIdCounter msg_event_counter;
        
      size_t different_messages = 0;
      size_t i = 0;
      
      for(::NewsGate::Search::MessageInfoArray::iterator it =
            message_infos->begin(); it != message_infos->end() &&
            different_messages < total_required; ++it, ++i)
      {
        MessageInfo& msg_info = *it;

        if(msg_info.wid == WeightedId::zero)
        {
          continue;
        }
        
        const El::Luid& event_id = msg_info.event_id;
        
        if(event_id == El::Luid::null)
        {
          ++different_messages;
          continue;          
        }

        EventIdCounter::iterator mit = msg_event_counter.find(event_id);

        if(mit == msg_event_counter.end())
        {
          mit = msg_event_counter.insert(std::make_pair(event_id, 0)).first;
        }

        if(mit->second < msg_per_event)
        {
          ++(mit->second);
          ++different_messages;
          continue;          
        }

        if(suppressed_messages)
        {
          suppressed_messages->insert(msg_info.wid.id);
        }
        
        msg_info.wid = WeightedId::zero;

        if(suppressed)
        {
          ++(*suppressed);
        }
      }

      message_infos->resize(i);
      return different_messages;
    }

    void
    Result::take_top(size_t start_from,
                     size_t results_count,
                     const Strategy& strategy,
                     size_t* suppressed)
      throw(El::Exception)
    {
#     ifdef TRACE_SEARCH_TIME
      ACE_High_Res_Timer timer;
      timer.start();
#     endif
      
      El::Stat::TimeMeasurement measurement(Expression::take_top_meter);

      unsigned long total_matched = message_infos->size();
      unsigned long total_required = start_from + results_count;
      
#     ifdef TRACE_SORT_TIME
        std::cerr << "matched: " << total_matched << ", required: "
                  << total_required << std::endl;
#     endif
      
      Strategy::SuppressionType suppression_type =
        strategy.suppression->type();
      
      if(suppression_type == Strategy::ST_COLLAPSE_EVENTS &&
         dynamic_cast<Strategy::SortByEventCapacity*>(strategy.sorting.get()))
      {
#       ifdef TRACE_SORT_TIME
          ACE_High_Res_Timer timer2;
          timer2.start();
#       endif

        message_infos.reset(
          sort_skip_cut(start_from,
                        results_count,
                        strategy,
                        suppressed));
        
#       ifdef TRACE_SORT_TIME
          timer2.stop();
          ACE_Time_Value tm;
          timer2.elapsed_time(tm);
        
          std::cerr << "  ssc: " << El::Moment::time(tm) << std::endl;
#       endif
      }
      else if(suppression_type == Strategy::ST_SIMILAR ||
              suppression_type == Strategy::ST_COLLAPSE_EVENTS)
      {
        size_t intermediate_results = total_required;
        float factor = MIN_FACTOR;

        IdSet suppressed_messages;
        
        for(size_t iteration = 1; true; ++iteration)
        {
#         ifdef TRACE_SORT_TIME
            ACE_High_Res_Timer timer2;
            timer2.start();
#         endif
            
          intermediate_results =
            (unsigned long)(factor * intermediate_results);

          size_t cut = 0;

          MessageInfoArrayPtr intermediate_result(
            sort_and_cut(0,
                         intermediate_results,
                         strategy,
                         &cut,
                         &suppressed_messages));
          
#         ifdef TRACE_SORT_TIME
            timer2.stop();
            ACE_Time_Value sc_tm;
            timer2.elapsed_time(sc_tm);
            timer2.start();
#         endif

          size_t removed = 0;
          
          size_t different_messages =
            remove_similar(
              intermediate_result.get(),
              suppression_type == Strategy::ST_COLLAPSE_EVENTS ?
              SIZE_MAX : total_required,
              strategy,
              &removed,
              &suppressed_messages);

#         ifdef TRACE_SORT_TIME
            timer2.stop();
            ACE_Time_Value rs_tm;
            timer2.elapsed_time(rs_tm);

            ACE_Time_Value ce_tm;
#         endif
          
          if(suppression_type == Strategy::ST_COLLAPSE_EVENTS)
          {
#           ifdef TRACE_SORT_TIME
              timer2.start();
#           endif

            different_messages =
              collapse_events(intermediate_result.get(),
                              total_required,
                              strategy,
                              &removed,
                              &suppressed_messages);

#           ifdef TRACE_SORT_TIME
              timer2.stop();
              timer2.elapsed_time(ce_tm);
#           endif
          }
          
          El::Stat::TimeMeasurement measurement(Expression::copy_range_meter);
          
#         ifdef TRACE_SORT_TIME
            std::cerr << "  ir: " << intermediate_results << ", dm: "
                      << different_messages << ", sc: "
                      << El::Moment::time(sc_tm) << ", rs: "
                      << El::Moment::time(rs_tm) << ", ce: "
                      << El::Moment::time(ce_tm) << ", f: " << factor
                      << std::endl;
#           endif

          if(different_messages >= total_required ||
             intermediate_results >= total_matched)
          {
            if(suppressed)
            {
              *suppressed += cut + removed;
            }

            if(different_messages > start_from)
            {
              different_messages -= start_from;
            }
            else
            {
              different_messages = 0;
            }
               
            message_infos.reset(
              new MessageInfoArray(std::min(different_messages,
                                            results_count)));

            size_t j = 0;
            size_t offset = 0;
            
            for(unsigned long i = 0; i < intermediate_result->size() &&
                  j < message_infos->size(); ++i)
            {
              const MessageInfo& mi = (*intermediate_result)[i];
                
              if(mi.wid.id != Message::Id::zero)
              {
                if(offset < start_from)
                {
                  ++offset;
                }
                else
                {
                  (*message_infos)[j++] = mi;
                }
              }
            }
            
            break;
          }

          factor =
            std::max(std::min(MAX_FACTOR,
                              (float)intermediate_results /
                              std::max((float)1, (float)different_messages)) /
                     iteration,
                     MIN_FACTOR);          
        }
      }
      else
      {
#       ifdef TRACE_SORT_TIME
          ACE_High_Res_Timer timer2;
          timer2.start();
#       endif
        
        message_infos.reset(
          sort_and_cut(start_from, results_count, strategy, suppressed, 0));

#       ifdef TRACE_SORT_TIME
          timer2.stop();
          ACE_Time_Value tm;
          timer2.elapsed_time(tm);
        
          std::cerr << "  sc: " << El::Moment::time(tm) << std::endl;
#       endif
      }

      MessageInfoArray& mi = *message_infos;
        
      if(mi.empty())
      {
        max_weight = 0;
        min_weight = UINT32_MAX;
      }
      else
      {
        max_weight = mi.begin()->wid.weight;
        min_weight = mi.rbegin()->wid.weight;
      }
      
      for(MessageInfoArray::iterator it(mi.begin()), end(mi.end()); it != end;
          ++it)
      {
        it->own_core_words();
      }

#     ifdef TRACE_SEARCH_TIME
      timer.stop();
      ACE_Time_Value tm;
      timer.elapsed_time(tm);

      std::cerr << "Search::take_top: " << El::Moment::time(tm) << std::endl;
#     endif
      
#     ifdef TRACE_SORT_TIME
        std::cerr << "end\n";
#     endif
    }
    
  }
}
