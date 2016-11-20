/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   DummySegmentor.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <sstream>
#include <iostream>

#include <El/Exception.hpp>

#include <NewsGate/Segmentation.hpp>

EL_EXCEPTION(Exception, El::ExceptionBase);

class DummySegmentor : public ::NewsGate::Segmentation::Interface
{
public:
  DummySegmentor() throw() {}
  virtual ~DummySegmentor() throw() {}

  virtual const char* creation_error() throw() { return 0; }
  virtual void release() throw(Exception);
  virtual std::string segment_text(const char* src) const throw(Exception);
  virtual std::string segment_query(const char* src) const throw(Exception);
};

void
DummySegmentor::release() throw(Exception)
{
  delete this;
}

std::string
DummySegmentor::segment_text(const char* src) const throw(Exception)
{
  std::ostringstream ostr;
  bool word_start = true;
  bool segment_word = false;
  unsigned long in_word_offset = 0;
  
  unsigned long len = strlen(src);
  for(unsigned long i = 0; i < len; i++)
  {
    char chr = src[i];
    
    if(isspace(chr))
    {
      word_start = true;
      segment_word = false;
      in_word_offset = 0;
      
      ostr << chr;
    }
    else
    {
      if(word_start)
      {
        segment_word = chr == 'a' || chr == 'A';
        word_start = false;
      }

      if(segment_word && in_word_offset && (in_word_offset % 3) == 0 &&
         ((chr >= 'a' && chr <= 'z') || (chr >= 'A' && chr <= 'Z')))
      {
        ostr << " ";
      } 

      ostr << chr;
      in_word_offset++;
    }
  }

  return ostr.str();
}
    
std::string
DummySegmentor::segment_query(const char* src) const throw(Exception)
{
  return segment_text(src);
}

// the class factories

extern "C"
::NewsGate::Segmentation::Interface*
create_segmentor(const char* args)
{
  return new DummySegmentor();
}
