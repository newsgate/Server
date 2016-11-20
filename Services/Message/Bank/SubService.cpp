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
  namespace Message
  {
    namespace Aspect
    {
      const char STATE[] = "State";
      const char MSG_SHARING[] = "MessageSharing";
      const char MSG_MANAGEMENT[] = "MessageManagement";
      const char DB_PERFORMANCE[] = "DBPerformance";
      const char PERFORMANCE[] = "Performance";
      const char MSG_CATEGORIZATION[] = "MessageCategorization";
    }    
  }
}

