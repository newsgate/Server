/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Commons/Category/WordListResolver.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <iostream>
#include <sstream>

#include <Commons/Search/SearchExpression.hpp>

#include "WordListResolver.hpp"

namespace
{
  const char TEMPLATE_LEFT_MARKER[] = "{{LIST:";
  const char TEMPLATE_RIGHT_MARKER[] = "}}";
  const char AT_POSITION_SIGN[] = " at position ";
}

namespace NewsGate
{
  namespace Message
  {
    namespace Categorization
    {
      //
      // WordListResolver class
      //
      WordListResolver::WordListResolver(Categorizer::Category::Id category_id,
                                         El::MySQL::Connection* connection,
                                         Context& context)
        throw(El::Exception)
          : category_id_(category_id),
            connection_(connection),
            context_(context)
      {
      }

      void
      WordListResolver::chunk(const El::String::Template::Chunk& value) const
        throw(El::Exception)
      {
        std::wstring wtext;
        El::String::Manip::utf8_to_wchar(value.text.c_str(), wtext);
        
        size_t len = wtext.length();
        context_.source_offset += len;
        context_.dest_offset += len;
      }
      
      bool
      WordListResolver::write(const El::String::Template::Chunk& chunk,
                              std::ostream& output) const
        throw(El::Exception)
      {
        context_.source_offset += sizeof(TEMPLATE_LEFT_MARKER) - 1;
        
        std::ostringstream ostr;
        ostr << "select words from CategoryWordList where category_id="
             << category_id_ << " and name='"
             << connection_->escape(chunk.text.c_str()) << "'";

        El::MySQL::Result_var result = connection_->query(ostr.str().c_str());
        El::MySQL::Row row(result.in());
      
        if(!row.fetch_row())
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::Category::WordListResolver::write: "
            "word list '" << chunk.text << "' not found for category "
               << category_id_;
        
          throw WordListNotFound(chunk.text.c_str(),
                                 context_.source_offset + 1,
                                 ostr.str().c_str());
        }

        size_t name_len = 0;
        
        {
          std::wstring wtext;
          El::String::Manip::utf8_to_wchar(chunk.text.c_str(), wtext);
          name_len = wtext.length();
        }

        std::string suppressed;
        El::String::Manip::suppress(row.string(0).value(), suppressed, "\r");

        if(context_.error_pos != SIZE_MAX)
        {
          std::wstring wtext;
          El::String::Manip::utf8_to_wchar(suppressed.c_str(), wtext);

          size_t text_len = wtext.length();
          
          if(context_.error_pos < context_.dest_offset)
          {
            size_t error_offset = context_.error_pos +
              context_.prev_list_key_end - context_.prev_list_end;
            
            throw ExpressionParseError("",
                                       error_offset + 1,
                                       context_.error_desc.c_str());
          }
          else if(context_.error_pos >= context_.dest_offset &&
                  context_.error_pos < context_.dest_offset + text_len)
          {
            throw ExpressionParseError(
              chunk.text.c_str(),
              context_.error_pos - context_.dest_offset + 1,
              context_.error_desc.c_str());
          }

          context_.prev_list_key_end = context_.source_offset + name_len +
            (sizeof(TEMPLATE_RIGHT_MARKER) - 1);
          
          context_.prev_list_end = context_.dest_offset + text_len;
          context_.dest_offset += text_len;
        }
        
        output << suppressed;
        context_.list_count++;

        context_.source_offset +=
          name_len + sizeof(TEMPLATE_RIGHT_MARKER) - 1;
        
        return true;
      }

      void
      WordListResolver::instantiate_expression(
        std::string& expression,
        Categorizer::Category::Id category_id,
        El::MySQL::Connection* connection)
        throw(WordListNotFound, ExpressionParseError, Exception, El::Exception)
      {
        std::string expr;
        
        {
          WordListResolver::Context ctx;
          WordListResolver resolver(category_id, connection, ctx);
        
          El::String::Template::Parser templ_parser(expression.c_str(),
                                                    TEMPLATE_LEFT_MARKER,
                                                    TEMPLATE_RIGHT_MARKER);
            
          std::ostringstream ostr;
          templ_parser.instantiate(resolver, ostr);
        
          if(ctx.list_count)
          {
            expr = ostr.str();
          }
        }

        std::wstring query;
        
        El::String::Manip::utf8_to_wchar(
          expr.empty() ? expression.c_str() : expr.c_str(), query);
      
        Search::ExpressionParser parser;
        std::wistringstream istr(query);

        try
        {
          parser.parse(istr);

          if(!expr.empty())
          {
            expression = expr;
          }
        }
        catch(const Search::ParseError& e)
        {
          const char* desc = strstr(e.what(), ": ");
          
          WordListResolver::Context ctx(e.position - 1,
                                        desc ? desc + 2 : e.what());
          
          WordListResolver resolver(category_id, connection, ctx);
        
          El::String::Template::Parser templ_parser(expression.c_str(),
                                                    TEMPLATE_LEFT_MARKER,
                                                    TEMPLATE_RIGHT_MARKER);
            
          std::ostringstream ostr;
          templ_parser.instantiate(resolver, ostr);

          size_t error_offset = ctx.error_pos +
            ctx.prev_list_key_end - ctx.prev_list_end;
            
          throw ExpressionParseError("",
                                     error_offset + 1,
                                     ctx.error_desc.c_str());
        }

      }

      //
      // ExpressionParseError class
      //
      WordListResolver::ExpressionParseError::ExpressionParseError(
        const char* nm,
        size_t pos,
        const char* desc)
        throw()
          : position(pos)
      {
        strncpy(word_list_name, nm, sizeof(word_list_name) - 1);
        word_list_name[sizeof(word_list_name) - 1] = '\0';

        const char* at_pos = desc ? strstr(desc, AT_POSITION_SIGN) : 0;

        if(at_pos)
        {
          const char* at_pos_end = at_pos + sizeof(AT_POSITION_SIGN) - 1;
          
          std::ostringstream ostr;
          ostr.write(desc, at_pos_end - desc);
          ostr << pos << (at_pos_end + strcspn(at_pos_end, " \t\r\n"));

          init(ostr.str().c_str());
        }
        else
        {
          init(desc);
        }
      }

    }
  }
}
