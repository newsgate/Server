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
      const char MSG_FILTER_DIST[] = "MessageFilterDistribution";
      const char MSG_CATEGORIZER_DIST[] = "MessageCategorizerDistribution";
    }
  }
}
