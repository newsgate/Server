/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/tests/Commons/SourceUrls.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <string>
#include <iostream>

#include <El/Exception.hpp>
#include <El/String/Manip.hpp>

#include "SourceUrls.hpp"

namespace NewsGate
{
  namespace Test
  {
    //
    // SourceUrls class
    //
    SourceUrls::SourceUrls(std::istream& istr)
      throw(Exception, El::Exception)
    {
      load(istr);
    }

    SourceUrls::SourceUrls() throw(SourceUrls::Exception, El::Exception)
    {
    }

    void
    SourceUrls::load(std::istream& istr) throw(Exception, El::Exception)
    {
      std::string url;
      
      while(std::getline(istr, url))
      {
        std::string trimmed_url;
        El::String::Manip::trim(url.c_str(), trimmed_url);

        if(!trimmed_url.empty())
        {
          urls_.push_back(trimmed_url);
        }
      }      
    }

    std::string
    SourceUrls::get_random_url() throw(Exception, El::Exception)
    {
      unsigned long size = urls_.size();

      if(size == 0)
      {
        return "";
      }
      
      unsigned long index = (unsigned long long)rand() * urls_.size() /
        ((unsigned long long)RAND_MAX + 1);

      return urls_[index];
    }
    
  }  
}
