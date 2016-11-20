/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/tests/Commons/SourceUrls.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_TESTS_COMMONS_SOURCE_URLS_HPP_
#define _NEWSGATE_SERVER_TESTS_COMMONS_SOURCE_URLS_HPP_

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
    class SourceUrls :
      public virtual El::RefCount::DefaultImpl<El::Sync::ThreadPolicy>
    {
    public:
      EL_EXCEPTION(Exception, El::ExceptionBase);
      
      SourceUrls() throw(Exception, El::Exception);
      SourceUrls(std::istream& istr) throw(Exception, El::Exception);
      
      ~SourceUrls() throw();

      void load(std::istream& istr) throw(Exception, El::Exception);
      
      std::string get_random_url() throw(Exception, El::Exception);

      unsigned long size() const throw();

    private:
      typedef std::vector<std::string> StringArray;
        
      StringArray urls_;
    };

    typedef El::RefCount::SmartPtr<SourceUrls> SourceUrls_var;      
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
    // SourceUrls class
    //
    inline
    SourceUrls::~SourceUrls() throw()
    {
    }

    inline
    unsigned long
    SourceUrls::size() const throw()
    {
      return urls_.size();
    }
  }
}

#endif // _NEWSGATE_SERVER_TESTS_COMMONS_SOURCE_URLS_HPP_
