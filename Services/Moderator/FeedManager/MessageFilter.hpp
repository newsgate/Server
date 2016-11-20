/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/FeedManager/MessageFilter.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MODERATOR_FEEDMANAGER_MESSAGEFILTER_HPP_
#define _NEWSGATE_SERVER_SERVICES_MODERATOR_FEEDMANAGER_MESSAGEFILTER_HPP_

#include <Services/Moderator/Commons/FeedManager.hpp>

namespace NewsGate
{
  namespace Moderation
  {
    class MessageFilter
    {
    public:
      EL_EXCEPTION(Exception, El::ExceptionBase);

    public:
      void set_message_fetch_filter(
        const ::NewsGate::Moderation::MessageFetchFilterRuleSeq& rules)
        throw(NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);
      
      ::NewsGate::Moderation::MessageFetchFilterRuleSeq*
      get_message_fetch_filter()
        throw(NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);
      
      void add_message_filter(
        const ::NewsGate::Moderation::MessageIdSeq& ids)
        throw(NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);
    };
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Moderation
  {
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_MODERATOR_FEEDMANAGER_MESSAGEFILTER_HPP_
