/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Mailing/SearchMailer/Template.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MAILING_SEARCHMAILER_TEMPLATE_HPP_
#define _NEWSGATE_SERVER_SERVICES_MAILING_SEARCHMAILER_TEMPLATE_HPP_

#include <string>
#include <iostream>

#include <ext/hash_map>

#include <El/Exception.hpp>
#include <El/Lang.hpp>
#include <El/String/Manip.hpp>

#include <El/Cache/LocalizedTemplateFileCache.hpp>
#include <El/Cache/EncodingAwareLocalizedTemplate.hpp>

#include <Commons/Message/TransportImpl.hpp>
#include <Services/Commons/SearchMailing/SearchMailing.hpp>

namespace NewsGate
{
  namespace SearchMailing
  {
    class TemplateCache
    {
    public:
      TemplateCache(ACE_Time_Value review_filetime_period =
                    ACE_Time_Value::zero) throw(El::Exception);
      
      virtual ~TemplateCache() throw() {}

      void run(const char* template_path,
               const Subscription& subscription,
               const char* endpoint,
               const Message::Transport::StoredMessageArray& messages,
               bool prn_src,
               bool prn_country,
               std::ostream& ostr)
        throw(El::Cache::Exception, El::Exception);

    protected:

      struct TemplateParseInterceptor :
        public El::Cache::EncodingAwareTemplateParseInterceptor
      {
        virtual ~TemplateParseInterceptor() throw() {}

        virtual void update(const std::string& tag,
                            const std::string& value,
                            El::String::Template::Chunk& chunk) const
          throw(El::String::Template::ParsingFailed,
                El::String::Template::Exception,
                El::Exception);
      };

      TemplateParseInterceptor parse_interceptor_;
      El::Cache::LocalizedTemplateFileCache template_cache_;
    };    
    
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace SearchMailing
  {    
    //
    // TemplateCache class
    //
    inline
    TemplateCache::TemplateCache(ACE_Time_Value review_filetime_period)
      throw(El::Exception)
        : template_cache_("<?",
                          "?>",
                          review_filetime_period,
                          &parse_interceptor_,
                          "")
    {
    }

  }
}

#endif // _NEWSGATE_SERVER_SERVICES_MAILING_SEARCHMAILER_TEMPLATE_HPP_
