/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Mailing/SearchMailer/SearchMailerImpl.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MAILING_SEARCHMAILER_SEARCHMAILERIMPL_HPP_
#define _NEWSGATE_SERVER_SERVICES_MAILING_SEARCHMAILER_SEARCHMAILERIMPL_HPP_

#include <stdint.h>

#include <string>
#include <deque>
#include <iostream>
#include <set>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/Service/CompoundService.hpp>
#include <El/MySQL/DB.hpp>
#include <El/BinaryStream.hpp>
#include <El/Cache/TextTemplateFileCache.hpp>
#include <El/Guid.hpp>
#include <El/Net/SMTP/Session.hpp>

#include <xsd/Config.hpp>

#include <Services/Commons/SearchMailing/SearchMailing.hpp>
#include <Services/Commons/SearchMailing/TransportImpl.hpp>
#include <Services/Commons/SearchMailing/SearchMailingServices_s.hpp>

#include <Services/Commons/Message/MessageServices.hpp>

#include "DB_Record.hpp"
#include "Template.hpp"

namespace NewsGate
{
  namespace SearchMailing
  {
    class SearchMailerImpl :
      public virtual POA_NewsGate::SearchMailing::Mailer,
      public virtual El::Service::CompoundService<> 
    {
    public:
      EL_EXCEPTION(Exception, El::ExceptionBase);

    public:

      SearchMailerImpl(El::Service::Callback* callback)
        throw(Exception, El::Exception);

      virtual ~SearchMailerImpl() throw();

      virtual void wait() throw(Exception, El::Exception);

    private:

      //
      // IDL:NewsGate/SearchMailing/Mailer/get_subscription:1.0
      //
      virtual ::NewsGate::SearchMailing::Transport::Subscription*
      get_subscription(::CORBA::ULong interface_version, const char* id)
        throw(NewsGate::SearchMailing::IncompatibleVersion,
              NewsGate::SearchMailing::NotFound,
              NewsGate::SearchMailing::ImplementationException,
              CORBA::SystemException);

      //
      // IDL:NewsGate/SearchMailing/Mailer/get_subscriptions:1.0
      //
      virtual ::NewsGate::SearchMailing::Transport::SubscriptionPack*
      get_subscriptions(::CORBA::ULong interface_version,
                        const char* email,
                        const char* lang,
                        const char* token,
                        const char* user,
                        const char* ip,
                        const char* agent,
                        char*& session,
                        ::CORBA::Boolean is_human)
      throw(NewsGate::SearchMailing::CheckHuman,
            NewsGate::SearchMailing::NeedConfirmation,
            NewsGate::SearchMailing::NotFound,
            NewsGate::SearchMailing::IncompatibleVersion,
            NewsGate::SearchMailing::ImplementationException,
            CORBA::SystemException);
      
      //
      // IDL:NewsGate/SearchMailing/Mailer/update_subscription:1.0
      //
      virtual char* update_subscription(
        ::CORBA::ULong interface_version,
        ::NewsGate::SearchMailing::Transport::Subscription* subs,
        ::CORBA::Boolean is_human)
        throw(NewsGate::SearchMailing::EmailChange,
              NewsGate::SearchMailing::CheckHuman,
              NewsGate::SearchMailing::NotFound,
              NewsGate::SearchMailing::IncompatibleVersion,
              NewsGate::SearchMailing::ImplementationException,
              CORBA::SystemException);
      
      //
      // IDL:NewsGate/SearchMailing/Mailer/enable_subscription:1.0
      //
      virtual CORBA::Boolean enable_subscription(const char* email,
                                                 const char* id,
                                                 ::CORBA::ULong status,
                                                 const char* user,
                                                 const char* ip,
                                                 const char* agent,
                                                 char*& session,
                                                 ::CORBA::Boolean is_human)
        throw(NewsGate::SearchMailing::CheckHuman,
              NewsGate::SearchMailing::NoOp,
              NewsGate::SearchMailing::NotFound,
              NewsGate::SearchMailing::ImplementationException,
              CORBA::SystemException);

      //
      // IDL:NewsGate/SearchMailing/Mailer/confirm_operation:1.0
      //
      virtual char* confirm_operation(const char* token,
                                      const char* user,
                                      const char* ip,
                                      const char* agent,
                                      CORBA::String_out email)
        throw(NewsGate::SearchMailing::EmailChange,
              NewsGate::SearchMailing::ImplementationException,
              CORBA::SystemException);
        
      virtual bool notify(El::Service::Event* event) throw(El::Exception);

      void update_subscriptions() throw(Exception, El::Exception);
      void confirm_operations() throw(Exception, El::Exception);
      void dispatch_search_mail() throw(Exception, El::Exception);
      void search_mail_task() throw(Exception, El::Exception);
      void send_smtp_message() throw(Exception, El::Exception);
      void cleanup_task() throw(Exception, El::Exception);

      struct SearchMailTimes
      {
        El::Guid id;
        TimeSet times;
      };      

      typedef std::vector<SearchMailTimes> SearchMailTimesArray;

      struct SubscriptionUpdate
      {
        enum State
        {
          US_NEW,
          US_TRUSTED,
          US_CONFIRMED,
          US_EXPIRED
        };

        enum Type
        {
          UT_UPDATE,
          UT_ENABLE,
          UT_MANAGE
        };
          
        El::Guid token;
        Type type;
        State state;
        std::string conf_email;
        uint64_t conf_time;
        Subscription subscription;

        SubscriptionUpdate() throw(El::Exception);

        void init(const DB::SubscriptionUpdate& record) throw(El::Exception);

        void save(El::MySQL::Connection* connection) const throw(El::Exception);
        bool load(El::MySQL::Connection* connection) throw(El::Exception);

        void confirm(El::MySQL::Connection* connection,
                     bool update_user_info) const
          throw(El::Exception);

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);

        char db_state() const throw();
      };      

      bool confirm_op(const Confirmation& conf,
                      bool& email_changed,
                      std::string& email,
                      std::string& session,
                      SearchMailTimes& mail_times,
                      SearchMailTimesArray& mail_old_times)
        throw(Exception, El::Exception);

      void remove_search_mail_times(const SearchMailTimes& mail_times,
                                    std::ostream* log)
        throw(Exception, El::Exception);

      void remove_search_mail_times(const SearchMailTimesArray& mail_times,
                                    std::ostream* log)
        throw(Exception, El::Exception);
        
      void add_search_mail_times(const SearchMailTimes& mail_times,
                                 std::ostream* log)
        throw(Exception, El::Exception);

      void create_confirmation_mail(const SubscriptionUpdate& update,
                                    const char* title,
                                    El::Net::SMTP::Message& msg,
                                    const char* templ)
        throw(El::Exception);

      void create_subscription_mail(
        const Subscription& subs,
        const Message::Transport::StoredMessageArray& messages,
        bool prn_src,
        bool prn_country,
        El::Net::SMTP::Message& msg)
        throw(Exception, El::Exception);

      bool trust_update(const Subscription& subs,
                        El::MySQL::Connection* connection)
        const throw(El::Exception);

      Message::BankClientSession* bank_client_session(bool reserve = false)
        throw(El::Exception);

      bool adjust_query(Subscription& subs) throw(Exception, El::Exception);
 
      bool get_event_id(const El::Guid& subs_id,
                        const char* query,
                        El::Luid& event_id)
        throw(Exception, El::Exception);
     
      Message::Transport::StoredMessageArray* search(Subscription& subs,
                                                     bool& prn_src,
                                                     bool& prn_country)
        throw(Exception, El::Exception);
      
      Search::Expression* segment_search_expression(const char* exp)
        throw(Exception, Search::ParseError, El::Exception);
      
      Search::Expression* normalize_search_expression(
        Search::Expression* expression) throw(Exception, El::Exception);

      static void dump_mail(const El::Net::SMTP::Message& msg,
                            std::ostream& ostr)
        throw(El::Exception);
      
      static El::Guid find_by_search(const Subscription& subscription,
                                     El::MySQL::Connection* connection)
        throw(Exception, El::Exception);
      
      static void load_subscriptions(const char* email,
                                     SubscriptionArray& subscriptions,
                                     El::MySQL::Connection* connection)
        throw(Exception, El::Exception);
      
      static void load_mail_schedule(El::MySQL::Connection* connection,
                                     SearchMailTimesArray& mail_times)
        throw(Exception, El::Exception);
      
      static void save_subscription(const Subscription& subs,
                                    El::MySQL::Connection* connection)
        throw(El::Exception);
      
      static bool load_subscription(Subscription& subs,
                                    El::MySQL::Connection* connection)
        throw(El::Exception);

      static void subs_from_row(Subscription& subs, DB::Subscription& record)
        throw(El::Exception);

      static void save_subscription_search_time(
        const Subscription& subs,
        El::MySQL::Connection* connection)
        throw(El::Exception);

      static void save_subscription_status(
        const Subscription& subs,
        El::MySQL::Connection* connection)
        throw(El::Exception);

      static char subs_db_status(const Subscription& subs) throw();
        
    private:

      struct SearchMailTime
      {
        El::Guid id;
        uint64_t time; // minutes

        SearchMailTime() throw() : time(0) {}
        SearchMailTime(El::Guid id_val, uint64_t time_val) throw();
        
        bool operator<(const SearchMailTime& val) const throw();
      };

      const Server::Config::SearchMailerType& config_;

      typedef std::set<SearchMailTime> SearchMailTimeSet;
      typedef std::set<std::string> RecipientSet;

      std::string email_sender_;
      std::string mailer_id_;
      uint64_t day_start_;
      uint8_t  day_week_;
      uint64_t last_dispatch_time_;
      uint64_t cached_dispatch_time_;
      SearchMailTimeSet search_mail_times_;
      RecipientSet recipient_blacklist_;

      bool cache_saved_;

      typedef std::deque<SubscriptionUpdate> SubscriptionUpdateQueue;
      SubscriptionUpdateQueue subscription_update_reqs_;
      
      ConfirmationArray confirmations_;

      typedef std::set<El::Guid> GuidSet;

      GuidSet dispatched_search_mails_;
      GuidSet generating_search_mails_;

      typedef std::deque<El::Net::SMTP::Message> SmtpMessageQueue;
      SmtpMessageQueue smtp_message_queue_;
      
      El::Cache::TextTemplateFileCache subscription_confirmation_template_;
      TemplateCache subscription_mail_template_;
      
      Message::BankClientSession_var bank_client_session_;
      Message::BankClientSession_var reserve_bank_client_session_;
      
    private:
      
      struct UpdateSubscription : public El::Service::CompoundServiceMessage
      {
        UpdateSubscription(SearchMailerImpl* mailer) throw(El::Exception);
        ~UpdateSubscription() throw() {}
      };

      struct SendSmtpMessage : public El::Service::CompoundServiceMessage
      {
        SendSmtpMessage(SearchMailerImpl* mailer) throw(El::Exception);
        ~SendSmtpMessage() throw() {}
      };

      struct ConfirmOperations : public El::Service::CompoundServiceMessage
      {
        ConfirmOperations(SearchMailerImpl* mailer) throw(El::Exception);
        ~ConfirmOperations() throw() {}
      };
      
      struct DispatchSearchMail : public El::Service::CompoundServiceMessage
      {
        DispatchSearchMail(SearchMailerImpl* mailer) throw(El::Exception);
        ~DispatchSearchMail() throw() {}
      };
      
      struct SearchMailTask : public El::Service::CompoundServiceMessage
      {
        SearchMailTask(SearchMailerImpl* mailer) throw(El::Exception);
        ~SearchMailTask() throw() {}
      };
      
      struct CleanupTask : public El::Service::CompoundServiceMessage
      {
        CleanupTask(SearchMailerImpl* mailer) throw(El::Exception);
        ~CleanupTask() throw() {}
      };
    };

    typedef El::RefCount::SmartPtr<SearchMailerImpl> SearchMailerImpl_var;
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
    // SearchMailerImpl::UpdateSubscription class
    //
    inline
    SearchMailerImpl::UpdateSubscription::UpdateSubscription(
      SearchMailerImpl* mailer)
        throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(mailer, mailer, false),
          El::Service::CompoundServiceMessage(mailer, mailer)
    {
    }
    
    //
    // SearchMailerImpl::SendSmtpMessage class
    //
    inline
    SearchMailerImpl::SendSmtpMessage::SendSmtpMessage(
      SearchMailerImpl* mailer)
        throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(mailer, mailer, false),
          El::Service::CompoundServiceMessage(mailer, mailer)
    {
    }

    //
    // SearchMailerImpl::ConfirmOperations class
    //
    inline
    SearchMailerImpl::ConfirmOperations::ConfirmOperations(
      SearchMailerImpl* mailer)
        throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(mailer, mailer, false),
          El::Service::CompoundServiceMessage(mailer, mailer)
    {
    }
    
    //
    // SearchMailerImpl::DispatchSearchMail class
    //
    inline
    SearchMailerImpl::DispatchSearchMail::DispatchSearchMail(
      SearchMailerImpl* mailer)
        throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(mailer, mailer, false),
          El::Service::CompoundServiceMessage(mailer, mailer)
    {
    }
    
    //
    // SearchMailerImpl::SearchMailTask class
    //
    inline
    SearchMailerImpl::SearchMailTask::SearchMailTask(
      SearchMailerImpl* mailer)
        throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(mailer, mailer, false),
          El::Service::CompoundServiceMessage(mailer, mailer)
    {
    }

    //
    // SearchMailerImpl::CleanupTask class
    //
    inline
    SearchMailerImpl::CleanupTask::CleanupTask(
      SearchMailerImpl* mailer)
        throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(mailer, mailer, false),
          El::Service::CompoundServiceMessage(mailer, mailer)
    {
    }

    //
    // SearchMailerImpl::SubscriptionUpdate struct
    //

    inline
    SearchMailerImpl::SubscriptionUpdate::SubscriptionUpdate()
        throw(El::Exception)
        : type(UT_UPDATE),
          state(US_NEW),
          conf_time(0)
    {
    }
    
    inline
    void
    SearchMailerImpl::SubscriptionUpdate::write(El::BinaryOutStream& bstr)
      const throw(El::Exception)
    {
      bstr << subscription << token << (uint8_t)type << (uint8_t)state
           << conf_email << conf_time;
    }
    
    inline
    void
    SearchMailerImpl::SubscriptionUpdate::read(El::BinaryInStream& bstr)
      throw(El::Exception)
    {
      uint8_t tp = 0;
      uint8_t st = 0;

      bstr >> subscription >> token >> tp >> st >> conf_email >> conf_time;

      type = (Type)tp;
      state = (State)st;
    }

    inline
    char
    SearchMailerImpl::SubscriptionUpdate::db_state() const throw()
    {
      switch(state)
      {
      case US_NEW: return 'N';
      case US_TRUSTED: return 'T';
      case US_CONFIRMED: return 'C';
      case US_EXPIRED: return 'E';
      }
      
      assert(false);
      return 0;
    }
    
    //
    // SearchMailerImpl::SearchMailTime struct
    //

    inline
    SearchMailerImpl::SearchMailTime::SearchMailTime(El::Guid id_val,
                                                     uint64_t time_val) throw()
        : id(id_val), time(time_val)
    {
    }
    
    inline
    bool
    SearchMailerImpl::SearchMailTime::operator<(const SearchMailTime& val) const
      throw()
    {
      return time < val.time ? true : (time > val.time ? false : (id < val.id));
    }

    //
    // SearchMailerImpl struct
    //

    inline
    char
    SearchMailerImpl::subs_db_status(const Subscription& subs)  throw()
    {
      switch(subs.status)
      {
      case Subscription::SS_ENABLED: return 'E';
      case Subscription::SS_DISABLED: return 'D';
      case Subscription::SS_DELETED: return 'L';
      default: break;
      }
      
      assert(false);
      return 0;
    }
    
    
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_MAILING_SEARCHMAILER_SEARCHMAILERIMPL_HPP_
