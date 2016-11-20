/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Commons/Feed/Types.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include "Types.hpp"

namespace NewsGate
{
  namespace Feed
  {
    const char* SPACE_NAMES[] =
    {
      "undefined",
      "news",
      "talk",
      "ad",
      "blog",
      "article",
      "photo",
      "video",
      "audio",
      "printed"
    };

    NewsGate::Feed::Space
    space(const char* value) throw()
    {
      for(size_t i = 0; i < sizeof(SPACE_NAMES) / sizeof(SPACE_NAMES[0]); ++i)
      {
        if(strcasecmp(value, SPACE_NAMES[i]) == 0)
        {
          return (Space)i;
        }
      }
      
      return SP_NONEXISTENT;
    }

    const char*
    space(NewsGate::Feed::Space value) throw()
    {
      return value < SP_SPACES_COUNT ? SPACE_NAMES[value] : "";
    }
  }
}
