/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Segmentation.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SEGMENTATION_HPP_
#define _NEWSGATE_SEGMENTATION_HPP_

#include <string>
#include <stdexcept>

namespace NewsGate
{
  namespace Segmentation
  {
    struct Interface
    {
      typedef std::exception Exception;
      
      virtual ~Interface() throw() {}

      // If return 0 or "" object considered to be fully functional.
      // Otherwise segmentor will not be used by NewsGate.
      virtual const char* creation_error() throw() = 0;

      // Should destroy an object
      virtual void release() throw(Exception) = 0;

      // User to segment new posts.
      // Should return string produced by inserting space characters into
      // original text. No other text modifications allowed.
      virtual std::string segment_text(const char* src) const
        throw(Exception) = 0;

      // Used to segment search query expression.
      // Allows arbitrary text modifications like removing pronouns or smth.
      virtual std::string segment_query(const char* src) const
        throw(Exception) = 0;
    };
    
  }
}

typedef ::NewsGate::Segmentation::Interface*
(* CreateSegmentor)(const char* args);

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////


#endif // _NEWSGATE_SEGMENTATION_HPP_
