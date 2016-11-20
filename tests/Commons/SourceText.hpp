/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/tests/Commons/SourceText.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_TESTS_COMMONS_SOURCE_TEXT_HPP_
#define _NEWSGATE_SERVER_TESTS_COMMONS_SOURCE_TEXT_HPP_

#include <string>
#include <iostream>
#include <vector>

#include <El/Exception.hpp>
#include <El/RefCount/All.hpp>
#include <El/SyncPolicy.hpp>

namespace NewsGate
{
  namespace Test
  {
    class SourceText :
      public virtual El::RefCount::DefaultImpl<El::Sync::ThreadPolicy>
    {
    public:
      EL_EXCEPTION(Exception, El::ExceptionBase);
      
      SourceText() throw(Exception, El::Exception);
      SourceText(std::istream& istr) throw(Exception, El::Exception);
      
      ~SourceText() throw();

      void load(std::istream& istr) throw(Exception, El::Exception);
      
      std::string get_random_substr(unsigned long desired_len)
        throw(Exception, El::Exception);

      unsigned long size() const throw();

    private:
      typedef std::vector<std::string::size_type> OffsetArray;
        
      std::string text_;
      OffsetArray sentence_pos_;
    };

    typedef El::RefCount::SmartPtr<SourceText> SourceText_var;      
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Test
  {
    //
    // SourceText class
    //
    inline
    SourceText::~SourceText() throw()
    {
    }

    inline
    unsigned long
    SourceText::size() const throw()
    {
      return text_.size();
    }
  }
}

#endif // _NEWSGATE_SERVER_TESTS_COMMONS_SOURCE_TEXT_HPP_
