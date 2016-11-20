/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/tests/Commons/SourceText.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <string>
#include <iostream>

#include <El/Exception.hpp>

#include "SourceText.hpp"

namespace
{
  const char WHITESPACES[]=" \t\n\r";
}

namespace NewsGate
{
  namespace Test
  {
    //
    // SourceText class
    //
    SourceText::SourceText(std::istream& istr)
      throw(Exception, El::Exception)
    {
      load(istr);
    }

    SourceText::SourceText() throw(SourceText::Exception, El::Exception)
    {
    }

    void
    SourceText::load(std::istream& istr) throw(Exception, El::Exception)
    {
      std::getline(istr, text_, '\0');
      sentence_pos_.push_back(0);

      std::string::size_type pos = 0;
      while((pos = text_.find('.', pos + 1)) != std::string::npos)
      {
        sentence_pos_.push_back(pos + 1);
      }
    }

    std::string
    SourceText::get_random_substr(unsigned long desired_len)
      throw(Exception, El::Exception)
    {
      unsigned long size = sentence_pos_.size();
      
      unsigned long index = (unsigned long long)rand() * size /
        ((unsigned long long)RAND_MAX + 1);

      const char* ptr = text_.c_str() + sentence_pos_[index];

      ptr += strspn(ptr, WHITESPACES);
      size = strlen(ptr);

      if(size > desired_len)
      {
        desired_len += strcspn(ptr + desired_len, WHITESPACES);
        size = desired_len;
      }

      std::string res;
      res.assign(ptr, size);
      
      return res;
    }
    
  }  
}
