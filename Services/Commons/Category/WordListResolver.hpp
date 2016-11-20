/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Commons/Category/WordListResolver.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_SERVICES_COMMONS_CATEGORY_WORDLISTRESOLVER_HPP_
#define _NEWSGATE_SERVER_SERVICES_COMMONS_CATEGORY_WORDLISTRESOLVER_HPP_

#include <iostream>

#include <El/Exception.hpp>
#include <El/String/Template.hpp>
#include <El/MySQL/DB.hpp>

#include <Commons/Message/Categorizer.hpp>

namespace NewsGate
{
  namespace Message
  {
    namespace Categorization
    {
      class WordListResolver : public El::String::Template::Variables
      {
      public:
        EL_EXCEPTION(Exception, El::ExceptionBase);

        struct WordListNotFound : public Exception
        {
          WordListNotFound(const char* nm,
                           size_t pos,
                           const char* desc) throw();

          char name[1024];
          size_t position;        
        };

        struct ExpressionParseError : public Exception
        {
          ExpressionParseError(const char* nm,
                               size_t pos,
                               const char* desc) throw();

          char word_list_name[1024];
          size_t position;        
        };

        struct Context
        {
          size_t list_count;
          size_t error_pos;
          std::string error_desc;
          size_t source_offset;
          size_t dest_offset;
          size_t prev_list_key_end;
          size_t prev_list_end;

          Context(size_t ep = SIZE_MAX, const char* ed = 0) throw();
        };
      
      public:
      
        WordListResolver(Categorizer::Category::Id category_id,
                         El::MySQL::Connection* connection,
                         Context& context)
          throw(El::Exception);

        virtual bool write(const El::String::Template::Chunk& chunk,
                           std::ostream& output) const
          throw(El::Exception);

        virtual void chunk(const El::String::Template::Chunk& value) const
          throw(El::Exception);
        
        static void instantiate_expression(
          std::string& expression,
          Categorizer::Category::Id category_id,
          El::MySQL::Connection* connection)
          throw(WordListNotFound,
                ExpressionParseError,
                Exception,
                El::Exception);
        
      private:
        Categorizer::Category::Id category_id_;
        El::MySQL::Connection* connection_;
        Context& context_;
      };
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Message
  {
    namespace Categorization
    {
      //
      // WordListNotFound class
      //
      inline
      WordListResolver::WordListNotFound::WordListNotFound(
        const char* nm,
        size_t pos,
        const char* desc)
        throw()
          : Exception(desc),
            position(pos)
      {
        strncpy(name, nm, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
      }

      //
      // Context class
      //
      inline
      WordListResolver::Context::Context(size_t ep, const char* ed) throw()
          : list_count(0),
            error_pos(ep),
            error_desc(ed ? ed : ""),
            source_offset(0),
            dest_offset(0),
            prev_list_key_end(0),
            prev_list_end(0)
      {
      }
    }
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_COMMONS_CATEGORY_WORDLISTRESOLVER_HPP_
