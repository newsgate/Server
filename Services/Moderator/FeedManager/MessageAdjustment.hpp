/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/FeedManager/MessageAdjustment.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MODERATOR_FEEDMANAGER_MESSAGEADJUSTMENT_HPP_
#define _NEWSGATE_SERVER_SERVICES_MODERATOR_FEEDMANAGER_MESSAGEADJUSTMENT_HPP_

#include <string>

#include <xercesc/sax2/SAX2XMLReader.hpp>

#include <El/Exception.hpp>
#include <El/Country.hpp>
#include <El/Lang.hpp>
#include <El/Geography/AddressInfo.hpp>
#include <El/Net/HTTP/Session.hpp>
#include <El/Python/SandboxService.hpp>

#include <Commons/Message/Automation/Automation.hpp>

#include <xsd/DataFeed/RSS/Data.hpp>
#include <xsd/DataFeed/RSS/ParserFactory.hpp>

#include <Services/Moderator/Commons/FeedManager.hpp>

namespace NewsGate
{
  namespace Moderation
  {
    class MessageAdjustment : public El::Python::SandboxService::Callback
    {
    public:
      EL_EXCEPTION(Exception, El::ExceptionBase);

    public:
      MessageAdjustment() throw(El::Exception);
      
      virtual ~MessageAdjustment() throw();

      void stop() throw();
      
      char* xpath_url(const char * xpath,
                      const char * url)
        throw(NewsGate::Moderation::OperationFailed,
              NewsGate::Moderation::ImplementationException,
              CORBA::SystemException);
      
      void adjust_message(
          const char * adjustment_script,
          ::NewsGate::Moderation::Transport::MsgAdjustmentContext * ctx,
          ::NewsGate::Moderation::Transport::MsgAdjustmentResult_out result)
        throw(NewsGate::Moderation::ImplementationException,
              ::CORBA::SystemException);

      ::NewsGate::Moderation::Transport::MsgAdjustmentContextPack*
      get_feed_items(const char* url,
                     ::CORBA::ULong type,
                     ::CORBA::ULong space,
                     ::CORBA::ULong country,
                     ::CORBA::ULong lang,
                     const char* encoding)
        throw(NewsGate::Moderation::OperationFailed,
              NewsGate::Moderation::ImplementationException,
              CORBA::SystemException);

      ::NewsGate::Moderation::Transport::GetHTMLItemsResult*
      get_html_items(const char* url,
                     const char* script,
                     ::CORBA::ULong type,
                     ::CORBA::ULong space,
                     ::CORBA::ULong country,
                     ::CORBA::ULong lang,
                     const ::NewsGate::Moderation::KeywordsSeq& keywords,
                     const char* cache,
                     const char* encoding)
        throw(NewsGate::Moderation::OperationFailed,
              NewsGate::Moderation::ImplementationException,
              CORBA::SystemException);

      virtual bool notify(El::Service::Event* event) throw(El::Exception);
      
        virtual bool interrupt_execution(std::string& reason)
          throw(El::Exception);
      
    private:

      El::Net::HTTP::Session* start_session(const char* url,
                                            std::string* permanent_url = 0)
        throw(El::Exception);

      ::NewsGate::Moderation::Transport::MsgAdjustmentContextPack*
      get_feed_items(const char* url,
                     NewsGate::Feed::Type type,
                     NewsGate::Feed::Space space,
                     const El::Country& country,
                     const El::Lang& lang,
                     bool use_http_charset,
                     const char* encoding)
        throw(Exception,
              RSS::Parser::EncodingError,
              El::Exception,
              NewsGate::Moderation::OperationFailed,
              NewsGate::Moderation::ImplementationException,
              CORBA::SystemException);

    private:
      El::Geography::AddressInfo address_info_;      
      ::NewsGate::Message::Automation::StringSet image_extension_whitelist_;
      ::NewsGate::Message::Automation::StringArray image_prefix_blacklist_;
      El::Python::SandboxService_var sandbox_service_;
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

#endif // _NEWSGATE_SERVER_SERVICES_MODERATOR_FEEDMANAGER_MESSAGEADJUSTMENT_HPP_
