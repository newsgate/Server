/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Message/Bank/ContentCache.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_CONTENTCACHE_HPP_
#define _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_CONTENTCACHE_HPP_

#include <memory>
#include <sstream>

#include <ext/hash_map>

#include <ace/OS.h>
#include <ace/Synch.h>
#include <ace/Guard_T.h>

#include <El/Exception.hpp>

#include <Commons/Message/Message.hpp>
#include <Commons/Message/StoredMessage.hpp>

#include "MessageRecord.hpp"

namespace NewsGate
{
  namespace Message
  {
    class ContentCache
    {
    public:

      EL_EXCEPTION(Exception, El::ExceptionBase);
      
      typedef __gnu_cxx::hash_map<Id, StoredContent_var, MessageIdHash>
      StoredContentMap;

      typedef std::auto_ptr<StoredContentMap> StoredContentMapPtr;

      ContentCache() throw(El::Exception);

      StoredContentMap* get(const IdArray& ids, bool keep_loaded)
        throw(El::Exception);
      
      ContentCache::StoredContentMap* flush() throw(El::Exception);
      
      static void query_stored_content(
        const Message::Id& id,
        std::auto_ptr<std::ostringstream>& load_msg_content_request_ostr)
        throw(El::Exception);
      
      static StoredContent* read_stored_content(
        const MessageContentRecord& record)
        throw(El::Exception);
      
    private:

      typedef ACE_RW_Thread_Mutex Mutex;
      typedef ACE_Read_Guard<Mutex> ReadGuard;
      typedef ACE_Write_Guard<Mutex> WriteGuard;

      mutable Mutex lock_;
      
      StoredContentMapPtr content_map_;
    };
    
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_MESSAGE_BANK_CONTENTCACHE_HPP_
