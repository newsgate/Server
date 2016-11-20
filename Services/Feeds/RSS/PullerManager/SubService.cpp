/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

#include <El/CORBA/Corba.hpp>

#include "SubService.hpp"

namespace NewsGate
{
  namespace RSS
  {
    namespace Aspect
    {
      const char STATE[] = "State";
      const char PULLING_FEEDS[] = "PullingFeeds";
      const char FEED_MANAGEMENT[] = "FeedManagement";
    }    
  }
}
