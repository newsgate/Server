/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Mailing/SearchMailer/SearchMailerImpl.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <string>
#include <sstream>
#include <fstream>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/Moment.hpp>
#include <El/MySQL/DB.hpp>
#include <El/Net/SMTP/Session.hpp>
#include <El/String/Template.hpp>

#include <Services/Commons/Message/BankClientSessionImpl.hpp>

#include "SearchMailerImpl.hpp"
#include "SearchMailerMain.hpp"
#include "DB_Record.hpp"

namespace NewsGate
{
  namespace SearchMailing
  {
    namespace Aspect
    {
      const char STATE[] = "State";
    }
    
    //
    // SearchMailerImpl class
    //
    SearchMailerImpl::SearchMailerImpl(El::Service::Callback* callback)
      throw(Exception, El::Exception)
        : El::Service::CompoundService<>(
            callback,
            "SearchMailerImpl",
            Application::instance()->config().threads()),
          config_(Application::instance()->config()),
          day_start_(0),
          day_week_(0),
          last_dispatch_time_(0),
          cached_dispatch_time_(0),
          cache_saved_(false),
          subscription_confirmation_template_("{{", "}}")
    {
      if(Application::will_trace(El::Logging::LOW))
      {
        Application::logger()->info(
          "NewsGate::SearchMailing::SearchMailerImpl::SearchMailerImpl: "
          "starting",
          Aspect::STATE);
      }

      mailer_id_ = config_.mailer_id();
      
      {
        std::ostringstream ostr;
        ostr << config_.sender_name() << " <" << config_.email() << ">";

        email_sender_ = ostr.str();
      }
          
      std::fstream file;
      El::BinaryInStream bstr(file);

      file.open(config_.cache_file().c_str(), ios::in);

      if(file.is_open())
      {
        try
        {
          uint32_t version = 0;
          bstr >> version >> cached_dispatch_time_;

          bstr.read_array(subscription_update_reqs_);
          bstr.read_array(confirmations_);
          bstr.read_set(dispatched_search_mails_);
          bstr.read_array(smtp_message_queue_);
          
          unlink(config_.cache_file().c_str());
        }
        catch(const El::Exception& e)
        {
          cached_dispatch_time_ = 0;
          
          subscription_update_reqs_.clear();
          confirmations_.clear();
          dispatched_search_mails_.clear();
          smtp_message_queue_.clear();
          
          std::ostringstream ostr;
          ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
            "SearchMailerImpl: failed to read " << config_.cache_file();
            
          El::Service::Error error(ostr.str().c_str(), this);
          callback_->notify(&error);
        }
      }

      std::string recipients = config_.recipient_blacklist();

      El::String::ListParser parser(recipients.c_str());
      
      const char* item;
      while((item = parser.next_item()) != 0)
      {
        recipient_blacklist_.insert(item);
      }
      
      El::Service::CompoundServiceMessage_var msg =
        new UpdateSubscription(this);
      
      deliver_now(msg.in());

      msg = new SendSmtpMessage(this);
      deliver_now(msg.in());

      msg = new ConfirmOperations(this);
      deliver_now(msg.in());

      msg = new DispatchSearchMail(this);
      deliver_now(msg.in());

      for(size_t i = 0; i < config_.search_workers(); ++i)
      {
        msg = new SearchMailTask(this);
        deliver_now(msg.in());
      }

      msg = new CleanupTask(this);        
      deliver_now(msg.in());
    }

    SearchMailerImpl::~SearchMailerImpl() throw()
    {
      // Check if state is active, then deactivate and log error
    }

    CORBA::Boolean
    SearchMailerImpl::enable_subscription(const char* email,
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
            CORBA::SystemException)
    {
      CORBA::String_var sess = session;
      session = 0;
      
      try
      {
        bool found = false;

        SubscriptionUpdate update;
        Subscription& subscription = update.subscription;

        update.type = SubscriptionUpdate::UT_ENABLE;
        subscription.id = id;

        bool status_change = false;
        Subscription::SubsStatus st = (Subscription::SubsStatus)status;
        
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();
        
        found = load_subscription(subscription, connection) &&
          subscription.email == email;

        if(found)
        {
          status_change = subscription.status != st;

          if(status_change)
          {
            subscription.user_id = user;
            subscription.user_ip = ip;
            subscription.user_agent = agent;
            subscription.user_session = sess.in();
            subscription.status = st;
            
            update.state = trust_update(subscription, connection) ?
              SubscriptionUpdate::US_TRUSTED : SubscriptionUpdate::US_NEW;

            if(update.state == SubscriptionUpdate::US_NEW && !is_human)
            {
              throw CheckHuman();
            }

            if(update.state != SubscriptionUpdate::US_TRUSTED)
            {
              El::Guid session;
              session.generate();

              subscription.user_session = session.string(El::Guid::GF_DENSE);
              sess = subscription.user_session.c_str();
            }            

            if(st == Subscription::SS_DELETED)
            {
              // Send email to ensure this is not a wrong decision
              update.state = SubscriptionUpdate::US_NEW;
            }
          }
          else
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Statistics::SearchMailerImpl::"
              "enable_subscription: status not changed for subscription "
                 << id;
            
            NoOp e;
            e.description = CORBA::string_dup(ostr.str().c_str());
            
            throw e;
          }
        }
        else
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Statistics::SearchMailerImpl::"
            "enable_subscription: subscription " << id << " not found";
        
          NotFound e;
          e.description = CORBA::string_dup(ostr.str().c_str());
        
          throw e;
        }
        
        if(Application::will_trace(El::Logging::HIGH))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
            "enable_subscription: id " << id
               << ", status " << subscription.status << ", trusted "
               << (update.state == SubscriptionUpdate::US_TRUSTED);
          
          Application::logger()->trace(ostr.str(),
                                       Aspect::STATE,
                                       El::Logging::HIGH);          
        }

        session = sess._retn();

        WriteGuard guard(srv_lock_);
        subscription_update_reqs_.push_back(update);
        
        return update.state == SubscriptionUpdate::US_TRUSTED;
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Statistics::SearchMailerImpl::"
          "enable_subscription: El::Exception caught. Description:\n"
             << e.what();
        
        ImplementationException e;
        e.description = CORBA::string_dup(ostr.str().c_str());
        
        throw e;
      }
    }

    ::NewsGate::SearchMailing::Transport::Subscription*
    SearchMailerImpl::get_subscription(::CORBA::ULong interface_version,
                                       const char* id)
      throw(NewsGate::SearchMailing::IncompatibleVersion,
            NewsGate::SearchMailing::NotFound,
            NewsGate::SearchMailing::ImplementationException,
            CORBA::SystemException)
    {
      if(interface_version != SearchMailing::Mailer::INTERFACE_VERSION)
      {
        IncompatibleVersion ex;
        ex.current_version = SearchMailing::Mailer::INTERFACE_VERSION;
        
        throw ex;
      }

      bool exist = false;

      Transport::SubscriptionImpl::Var subs_pack =
        new Transport::SubscriptionImpl::Type(new Subscription());

      Subscription& subs = subs_pack->entity();
      
      try
      {
        subs.id = id;
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
          "get_subscription: El::Exception while parsing id. Description:"
             << e;

        NotFound ex;
        ex.description = ostr.str().c_str();        
        throw ex;        
      }
      
      try
      {
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();
        
        exist = load_subscription(subs, connection);
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
          "get_subscription: El::Exception caught. Description:\n" << e;
          
        El::Service::Error error(ostr.str().c_str(), this);
        callback_->notify(&error);

        ImplementationException ex;
        ex.description = ostr.str().c_str();        
        throw ex;
      }

      if(!exist)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
          "get_subscription: no subscription found for " << id;

        NotFound ex;
        ex.description = ostr.str().c_str();        
        throw ex;        
      }

      return subs_pack._retn();
    }

    ::NewsGate::SearchMailing::Transport::SubscriptionPack*
    SearchMailerImpl::get_subscriptions(::CORBA::ULong interface_version,
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
          CORBA::SystemException)
    {
      CORBA::String_var sess = session;
      session = 0;
      
      if(interface_version != SearchMailing::Mailer::INTERFACE_VERSION)
      {
        IncompatibleVersion ex;
        ex.current_version = SearchMailing::Mailer::INTERFACE_VERSION;
        
        throw ex;
      }

      try
      {
        SubscriptionUpdate update;
        Subscription& subscription = update.subscription;
          
        subscription.user_id = user;
        subscription.user_ip = ip;
        subscription.user_agent = agent;
        subscription.user_session = sess.in();

        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();

        if(*token == '\0')
        {
          subscription.email = email;
          
          if(!trust_update(subscription, connection))
          {
            if(!is_human)
            {
              throw CheckHuman();
            }
            
            update.type = SubscriptionUpdate::UT_MANAGE;
            subscription.lang = El::Lang(lang);

            El::Guid session;
            session.generate();
            
            subscription.user_session = session.string(El::Guid::GF_DENSE);

            NeedConfirmation ex;
            ex.session = subscription.user_session.c_str();
            
            WriteGuard guard(srv_lock_);
            subscription_update_reqs_.push_back(update);

            throw ex;
          }
        }
        else
        {
          update.token = token;

          if(!update.load(connection) ||
             update.state != SubscriptionUpdate::US_NEW)
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Statistics::SearchMailerImpl::"
              "get_subscriptions: token " << token
                 << " not found or in wrong state";
            
            NotFound ex;
            ex.description = ostr.str().c_str();
            
            throw ex;
          }

          update.conf_time = ACE_OS::gettimeofday().sec();
          update.confirm(connection, true);
        }
        
        SubscriptionArrayPtr subscriptions(new SubscriptionArray());

        load_subscriptions(subscription.email.c_str(),
                           *subscriptions,
                           connection);
        
        session = CORBA::string_dup(subscription.user_session.c_str());

        return Transport::SubscriptionPackImpl::Init::create(
          subscriptions.release());
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Statistics::SearchMailerImpl::"
          "get_subscriptions: El::Exception caught. Description:\n"
             << e.what();
        
        ImplementationException e;
        e.description = CORBA::string_dup(ostr.str().c_str());
        
        throw e;
      }
    }
    
    char*
    SearchMailerImpl::update_subscription(
      ::CORBA::ULong interface_version,
      ::NewsGate::SearchMailing::Transport::Subscription* subs,
      ::CORBA::Boolean is_human)
      throw(NewsGate::SearchMailing::EmailChange,
            NewsGate::SearchMailing::CheckHuman,
            NewsGate::SearchMailing::NotFound,
            NewsGate::SearchMailing::IncompatibleVersion,
            NewsGate::SearchMailing::ImplementationException,
            CORBA::SystemException)
    {
      if(interface_version != SearchMailing::Mailer::INTERFACE_VERSION)
      {
        IncompatibleVersion ex;
        ex.current_version = SearchMailing::Mailer::INTERFACE_VERSION;
        
        throw ex;
      }

      std::string new_session;
      bool email_changed = false;
      
      try
      {
        Transport::SubscriptionImpl::Type* impl =
          dynamic_cast<Transport::SubscriptionImpl::Type*>(subs);
        
        if(impl == 0)
        {
          throw Exception("NewsGate::SearchMailing::SearchMailerImpl::"
                          "update_subscription: dynamic_cast<"
                          "Transport::SubscriptionImpl::Type*> failed");
        }

        const Subscription& s = impl->entity();

        SubscriptionUpdate update;
        Subscription& subscription = update.subscription;
        
        subscription = s;

        bool found = false;
        Subscription subs;
        
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();

        if(subscription.id != El::Guid::null)
        {
          subs.id = subscription.id;
          found = load_subscription(subs, connection);

          if(!found)
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Statistics::SearchMailerImpl::"
              "update_subscription: subscription "
                 << subs.id.string(El::Guid::GF_DENSE) << " not found";
            
            NotFound ex;
            ex.description = ostr.str().c_str();
              
            throw ex;
          }
        }        
          
        update.state = trust_update(s, connection) ?
          SubscriptionUpdate::US_TRUSTED : SubscriptionUpdate::US_NEW;

        if(update.state == SubscriptionUpdate::US_NEW && !is_human)
        {
          throw CheckHuman();
        }
       
        bool trusted = update.state == SubscriptionUpdate::US_TRUSTED;

        if(trusted)
        {
          if(found)
          {
            email_changed = subs.email != subscription.email;
          }
        }
        else
        {
          El::Guid session;
          session.generate();
          
          new_session = session.string(El::Guid::GF_DENSE);
          subscription.user_session = new_session;
        }
        
        const Search::Strategy::Filter& filter = s.resulted_filter;
        
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
          "update_subscription: id: " << s.id.string(El::Guid::GF_DENSE)
             << ", email: " << s.email
             << ", trusted: " << trusted
             << ", email_changed: " << email_changed
             << ", session: " << subscription.user_session
             << ", format: " << s.format
             << ", length: " << s.length
             << ", time_offset: " << s.time_offset
             << ", title: " << s.title
             << ", query: " << s.resulted_query
             << ", filter: l=" << filter.lang.l3_code() << " c="
             << filter.country.l3_code() << " f=" << filter.feed << " g="
             << filter.category << " e=" << filter.event.string()
             << ", loc: " << s.locale.lang.l3_code(true) << "/"
             << s.locale.country.l3_code(true)
             << ", lang: " << s.lang.l3_code(true)
             << ", uid: " << s.user_id
             << ", ip: " << s.user_ip
             << ", ua: " << s.user_agent
             << ", times (" << s.times.size() << "):";
        
        for(SearchMailing::TimeSet::const_iterator
              i(s.times.begin()), e(s.times.end()); i != e; ++i)
        {
          uint32_t m = i->time;
          ostr << " " << m / 60 << ":" << m % 60 << " " << (uint32_t)i->day;
        }
        
        Application::logger()->trace(ostr.str(),
                                     Aspect::STATE,
                                     El::Logging::HIGH);

        WriteGuard guard(srv_lock_);
        subscription_update_reqs_.push_back(update);
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Statistics::SearchMailerImpl::"
          "update_subscription: El::Exception caught. Description:\n"
             << e.what();
        
        ImplementationException e;
        e.description = CORBA::string_dup(ostr.str().c_str());
        
        throw e;
      }

      if(email_changed)
      {
        throw EmailChange();
      }
      
      return CORBA::string_dup(new_session.c_str());
    }
    
    char*
    SearchMailerImpl::confirm_operation(const char* token,
                                        const char* user,
                                        const char* ip,
                                        const char* agent,
                                        CORBA::String_out email)
      throw(NewsGate::SearchMailing::EmailChange,
            NewsGate::SearchMailing::ImplementationException,
            CORBA::SystemException)
    {        
      try
      {
        ACE_Time_Value total_tm = ACE_OS::gettimeofday();
        
        Confirmation conf(token, user, ip, agent);

        std::string eml;
        std::string session;
        SearchMailTimes mail_times;
        SearchMailTimesArray mail_old_times;

        bool email_changed = false;
        
        bool confirmed = confirm_op(conf,
                                    email_changed,
                                    eml,
                                    session,
                                    mail_times,
                                    mail_old_times);
              
        if(Application::will_trace(El::Logging::HIGH))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
            "confirm_operation: token " << token << (confirmed ? "" : " not")
               << " found, session " << session;

          Application::logger()->trace(ostr.str(),
                                       Aspect::STATE,
                                       El::Logging::HIGH);          
        }

        if(confirmed)
        {
          std::ostringstream ostr;

          if(Application::will_trace(El::Logging::HIGH))
          {
            ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
              "confirm_operation: schedule update:";
          }

          {
            WriteGuard guard(srv_lock_);
            
            remove_search_mail_times(mail_old_times, &ostr);
            add_search_mail_times(mail_times, &ostr);
          }

          if(Application::will_trace(El::Logging::HIGH))
          {
            total_tm = ACE_OS::gettimeofday() - total_tm;
            ostr << "\n  time: " << El::Moment::time(total_tm);
            
            Application::logger()->trace(ostr.str(),
                                         Aspect::STATE,
                                         El::Logging::HIGH);          
          }

          if(email_changed)
          {
            EmailChange e; 
            e.email = CORBA::string_dup(eml.c_str());
            e.session = CORBA::string_dup(session.c_str());

            throw e;
          }
        }

        email = CORBA::string_dup(eml.c_str());
        return CORBA::string_dup(confirmed ? session.c_str() : "");
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
          "confirm_operation: El::Exception caught. Description:\n"
             << e.what();
        
        ImplementationException e;
        e.description = CORBA::string_dup(ostr.str().c_str());
        
        throw e;
      }
    }

    bool
    SearchMailerImpl::adjust_query(Subscription& subs)
      throw(Exception, El::Exception)
    {
      if(strncasecmp(subs.modifier.c_str(), "v=E", 3) == 0)
      {
        std::string event_str;
        El::String::Manip::mime_url_decode(subs.modifier.c_str() + 3,
                                           event_str);

        std::string::size_type pos = event_str.find(' ');

        std::string mid;
        std::string eid;
      
        if(pos == std::string::npos)
        {
           mid = event_str;
        }
        else
        {
          mid.assign(event_str.c_str(), pos);
          eid = event_str.c_str() + pos + 1;
        }

        El::Luid event_id;
        std::string query = std::string("MSG ") + mid;
      
        if(!get_event_id(subs.id, query.c_str(), event_id))
        {
          return 0;
        }
      
        if(event_id == El::Luid::null &&
           !get_event_id(subs.id, subs.resulted_query.c_str(), event_id))
        {
          return 0;
        }

        if(event_id == El::Luid::null)
        {
          query = std::string("EVENT ") + eid;
          
          if(!get_event_id(subs.id, query.c_str(), event_id))
          {
            return 0;
          }
        }
        
        if(event_id == El::Luid::null)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::SearchMailing::SearchMailerImpl::"
            "get_event_id: event modifier seems to expire for "
               << subs.id.string(El::Guid::GF_DENSE);
          
          Application::logger()->trace(ostr.str(),
                                       Aspect::STATE,
                                       El::Logging::HIGH);

          return false;
        }
        else
        {
          subs.resulted_query = std::string("EVENT ") + event_id.string();
        }
      }
      
      if(subs.resulted_filter.event != El::Luid::null)
      {
        std::string event_str;
        El::String::ListParser parser(subs.filter.c_str(), "&");
      
        const char* item;
        while((item = parser.next_item()) != 0 && event_str.empty())
        {
          if(strncasecmp(item, "h=", 2) == 0)
          {
            El::String::Manip::mime_url_decode(item + 2, event_str);
          }
        }

        if(!event_str.empty())
        {
          std::string::size_type pos = event_str.find(' ');

          std::string mid;
          std::string eid;
      
          if(pos == std::string::npos)
          {
            mid = event_str;
          }
          else
          {
            mid.assign(event_str.c_str(), pos);
            eid = event_str.c_str() + pos + 1;
          }

          El::Luid event_id;
          std::string query = std::string("MSG ") + mid;
      
          if(!get_event_id(subs.id, query.c_str(), event_id))
          {
            return 0;
          }
      
          if(event_id == El::Luid::null)
          {            
            query = std::string("EVENT ") + subs.resulted_filter.event.string();
            
            if(!get_event_id(subs.id, query.c_str(), event_id))
            {
              return 0;
            }
          }
          
          if(event_id == El::Luid::null)
          {
            query = std::string("EVENT ") + eid;
          
            if(!get_event_id(subs.id, query.c_str(), event_id))
            {
              return 0;
            }
          }
        
          if(event_id == El::Luid::null)
          {
            std::ostringstream ostr;
            ostr << "::NewsGate::SearchMailing::SearchMailerImpl::"
              "get_event_id: event filter seems to expire for "
                 << subs.id.string(El::Guid::GF_DENSE);
            
            Application::logger()->trace(ostr.str(),
                                         Aspect::STATE,
                                         El::Logging::HIGH);
            
            return false;
          }
          else
          {
            subs.resulted_filter.event = event_id;
          }          
        }
      }

      return true;
    }

    bool
    SearchMailerImpl::get_event_id(const El::Guid& subs_id,
                                   const char* query,
                                   El::Luid& event_id)
      throw(Exception, El::Exception)
    {
      event_id = El::Luid::null;

      Search::Expression_var expression;

      try
      {
        std::wstring wquery;
        El::String::Manip::utf8_to_wchar(query, wquery);
            
        Search::ExpressionParser parser;
        std::wistringstream istr(wquery);
            
        parser.parse(istr);
        expression = parser.expression().retn();
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
          "get_event_id: El::Exception caught while parsing '" << query
             << "' for subs "<< subs_id.string(El::Guid::GF_DENSE)
             << ". Description:\n" << e;
          
        El::Service::Error error(ostr.str().c_str(), this);
        callback_->notify(&error);
        
        return false;
      }

      Search::Transport::ExpressionImpl::Var
        expression_transport =
        Search::Transport::ExpressionImpl::Init::create(
          new Search::Transport::ExpressionHolder(expression.retn()));
      
      Search::Strategy::SortingPtr sorting(new Search::Strategy::SortNone());
      
      Search::Strategy::SuppressionPtr suppression(
        new Search::Strategy::SuppressNone());

      Search::Strategy::Filter filter;

      Search::Transport::StrategyImpl::Var strategy_transport = 
        Search::Transport::StrategyImpl::Init::create(
          new Search::Strategy(sorting.release(),
                               suppression.release(),
                               false,
                               filter,
                               Search::Strategy::RF_MESSAGES));

      Message::SearchRequest_var search_request =
        new Message::SearchRequest();

      search_request->gm_flags = Message::Bank::GM_EVENT;

      search_request->expression = expression_transport._retn();
      search_request->strategy = strategy_transport._retn();
      search_request->results_count = 1;
      
      Message::SearchResult_var search_result;
      ACE_Time_Value tm = ACE_OS::gettimeofday();

      try
      {
        Message::BankClientSession_var session;
        
        try
        {
          session = bank_client_session();
          session->search(search_request.in(), search_result.out());
          session = 0;
        }
        catch(const CORBA::Exception& e)
        {          
          session = bank_client_session(true);

          if(session.in() == 0)
          {
            throw;
          }
          
          std::ostringstream ostr;
          ostr << "::NewsGate::SearchMailing::SearchMailerImpl::"
            "get_event_id: bank manager unavailable; using reserve one.\n"
            "CORBA::Exception caught. Description:\n" << e;
          
          Application::logger()->trace(ostr.str(),
                                       Aspect::STATE,
                                       El::Logging::HIGH);          
        }
        catch(const El::Exception& e)
        {          
          session = bank_client_session(true);

          if(session.in() == 0)
          {
            throw;
          }
          
          std::ostringstream ostr;
          ostr << "::NewsGate::SearchMailing::SearchMailerImpl::"
            "get_event_id: bank manager unavailable; using reserve one.\n"
            "El::Exception caught. Description:\n" << e;
          
          Application::logger()->trace(ostr.str(),
                                       Aspect::STATE,
                                       El::Logging::HIGH);          
        }

        if(session.in() != 0)
        {
          session->search(search_request.in(), search_result.out());
        }
      }
      catch(const NewsGate::Message::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "::NewsGate::SearchMailing::SearchMailerImpl::"
          "get_event_id: ImplementationException caught. "
          "Description:\n" << e.description.in();
            
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "::NewsGate::SearchMailing::SearchMailerImpl::"
          "get_event_id: CORBA::Exception caught. "
          "Description:\n" << e;
            
        throw Exception(ostr.str());
      }      

      if(!search_result->messages_loaded)
      {
        Message::BankClientSession_var session = bank_client_session(true);

        if(session.in() != 0)
        {
          try
          {
            search_result = 0;
            
            Message::BankClientSession_var session = bank_client_session(true);
            session->search(search_request.in(), search_result.out());
          }
          catch(const NewsGate::Message::ImplementationException& e)
          {
            std::ostringstream ostr;
            ostr << "::NewsGate::SearchMailing::SearchMailerImpl::"
              "get_event_id(reserve): ImplementationException caught. "
              "Description:\n" << e.description.in();
            
            throw Exception(ostr.str());
          }
          catch(const CORBA::Exception& e)
          {
            std::ostringstream ostr;
            ostr << "::NewsGate::SearchMailing::SearchMailerImpl::"
              "get_event_id(reserve): CORBA::Exception caught. "
              "Description:\n" << e;
            
            throw Exception(ostr.str());
          } 
        }

        if(!search_result->messages_loaded)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::SearchMailing::SearchMailerImpl::"
            "get_event_id: delaying search for subs " <<
            subs_id.string(El::Guid::GF_DENSE)
               << " as messages not fully loaded";
          
          throw Exception(ostr.str());
        }
      }
      
      Message::Transport::StoredMessagePackImpl::Type* msg_transport =
        dynamic_cast<Message::Transport::StoredMessagePackImpl::Type*>(
          search_result->messages.in());
      
      if(msg_transport == 0)
      {
        throw Exception(
          "::NewsGate::SearchMailing::SearchMailerImpl::"
          "get_event_id: dynamic_cast<const Message::Transport::"
          "StoredMessagePackImpl::Type*> failed");
      }

      tm = ACE_OS::gettimeofday() - tm;
      
      Message::Transport::StoredMessageArrayPtr messages(
        msg_transport->release());

      if(!messages->empty())
      {
        event_id = (*messages)[0].message.event_id;
      }      
      
      std::ostringstream ostr;
      ostr << "SearchMailerImpl::get_event_id("
           << subs_id.string(El::Guid::GF_DENSE)
           << "): for '" << query << "' event_id obtained "
           << event_id.string() << " (" << event_id.string(true)
           << "); time " << El::Moment::time(tm);
      
      Application::logger()->trace(ostr.str(),
                                   Aspect::STATE,
                                   El::Logging::HIGH);

      
      return true;
    }
      
    Message::Transport::StoredMessageArray*
    SearchMailerImpl::search(Subscription& subs,
                             bool& prn_src,
                             bool& prn_country)
      throw(Exception, El::Exception)
    {
      if(!adjust_query(subs))
      {
        return 0;
      }
      
      Search::Expression_var expression;

      try
      {
        expression = segment_search_expression(subs.resulted_query.c_str());
      }
      catch(const Search::ParseError& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::SearchMailerImpl::search: query "
          "parsing failed for subscription "
             << subs.id.string(El::Guid::GF_DENSE) << ". Description:\n"
             << e << "\nQuery: " << subs.resulted_query;

        El::Service::Error error(ostr.str().c_str(), this);
        callback_->notify(&error);
        
        return 0;
      }

      expression = normalize_search_expression(expression.in());

      bool collapse_events = dynamic_cast<Search::Event*>(
        expression->condition.in()) == 0 &&
        subs.resulted_filter.event == El::Luid::null;

      prn_src = dynamic_cast<Search::Url*>(expression->condition.in()) == 0 &&
        subs.resulted_filter.feed.empty();
      
      prn_country = prn_src &&
        subs.resulted_filter.country == El::Country::null;

      Search::Fetched_var cond = new Search::Fetched();
      cond->condition = expression->condition.retn();
      
      Search::Condition::Time& t = cond->time;
      t.offset = subs.search_time;
      t.type = Search::Condition::Time::OT_SINCE_EPOCH;
      
      expression->condition = cond.retn();

      const Server::Config::SearchMailerType::search_type& config =
        config_.search();
      
      Search::Transport::ExpressionImpl::Var
        expression_transport =
        Search::Transport::ExpressionImpl::Init::create(
          new Search::Transport::ExpressionHolder(expression.retn()));
      
      Search::Strategy::SortingPtr sorting(collapse_events ?
        (Search::Strategy::Sorting*)
        new Search::Strategy::SortByRelevanceDesc(
          config.sorting().message_max_age(),
          config.sorting().max_core_words(),
          config.sorting().impression_respected_level()) :
        (Search::Strategy::Sorting*)
        new Search::Strategy::SortByPubDateDesc(
          config.sorting().message_max_age()));
          
      Search::Strategy::SuppressionPtr suppression(
        collapse_events ?
        new Search::Strategy::CollapseEvents(
          config.supress_cw().intersection(),
          config.supress_cw().containment_level(),
          config.supress_cw().min_count(),
          1) :
        new Search::Strategy::SuppressSimilar(
          config.supress_cw().intersection(),
          config.supress_cw().containment_level(),
          config.supress_cw().min_count()));

      Search::Transport::StrategyImpl::Var strategy_transport = 
        Search::Transport::StrategyImpl::Init::create(
          new Search::Strategy(sorting.release(),
                               suppression.release(),
                               false,
                               subs.resulted_filter,
                               Search::Strategy::RF_MESSAGES));

      Message::SearchRequest_var search_request =
        new Message::SearchRequest();

      search_request->gm_flags =
        Message::Bank::GM_ID | Message::Bank::GM_EVENT |
        Message::Bank::GM_TITLE | Message::Bank::GM_LINK |
        Message::Bank::GM_PUB_DATE | Message::Bank::GM_SOURCE;
            
      search_request->expression = expression_transport._retn();
      search_request->strategy = strategy_transport._retn();
      
      search_request->results_count =
        std::min(subs.length, (uint16_t)config.search_results());

      Message::SearchResult_var search_result;
      ACE_Time_Value tm = ACE_OS::gettimeofday();

      try
      {
        Message::BankClientSession_var session;
        
        try
        {
          session = bank_client_session();
          session->search(search_request.in(), search_result.out());
          session = 0;
        }
        catch(const CORBA::Exception& e)
        {          
          session = bank_client_session(true);

          if(session.in() == 0)
          {
            throw;
          }
          
          std::ostringstream ostr;
          ostr << "::NewsGate::SearchMailing::SearchMailerImpl::"
            "search: bank manager unavailable; using reserve one.\n"
            "CORBA::Exception caught. Description:\n" << e;
          
          Application::logger()->trace(ostr.str(),
                                       Aspect::STATE,
                                       El::Logging::HIGH);          
        }
        catch(const El::Exception& e)
        {          
          session = bank_client_session(true);

          if(session.in() == 0)
          {
            throw;
          }
          
          std::ostringstream ostr;
          ostr << "::NewsGate::SearchMailing::SearchMailerImpl::"
            "search: bank manager unavailable; using reserve one.\n"
            "El::Exception caught. Description:\n" << e;
          
          Application::logger()->trace(ostr.str(),
                                       Aspect::STATE,
                                       El::Logging::HIGH);          
        }

        if(session.in() != 0)
        {
          session->search(search_request.in(), search_result.out());
        }
      }
      catch(const NewsGate::Message::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "::NewsGate::SearchMailing::SearchMailerImpl::"
          "search: ImplementationException caught. "
          "Description:\n" << e.description.in();
            
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "::NewsGate::SearchMailing::SearchMailerImpl::"
          "search: CORBA::Exception caught. "
          "Description:\n" << e;
            
        throw Exception(ostr.str());
      }      

      if(!search_result->messages_loaded)
      {
        Message::BankClientSession_var session = bank_client_session(true);

        if(session.in() != 0)
        {
          try
          {
            search_result = 0;
            
            Message::BankClientSession_var session = bank_client_session(true);
            session->search(search_request.in(), search_result.out());
          }
          catch(const NewsGate::Message::ImplementationException& e)
          {
            std::ostringstream ostr;
            ostr << "::NewsGate::SearchMailing::SearchMailerImpl::"
              "search(reserve): ImplementationException caught. "
              "Description:\n" << e.description.in();
            
            throw Exception(ostr.str());
          }
          catch(const CORBA::Exception& e)
          {
            std::ostringstream ostr;
            ostr << "::NewsGate::SearchMailing::SearchMailerImpl::"
              "search(reserve): CORBA::Exception caught. "
              "Description:\n" << e;
            
            throw Exception(ostr.str());
          } 
        }

        if(!search_result->messages_loaded)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::SearchMailing::SearchMailerImpl::"
            "search: delaying search for subs " <<
            subs.id.string(El::Guid::GF_DENSE)
               << " as messages not fully loaded";
          
          throw Exception(ostr.str());
        }
      }

      Message::Transport::StoredMessagePackImpl::Type* msg_transport =
        dynamic_cast<Message::Transport::StoredMessagePackImpl::Type*>(
          search_result->messages.in());
      
      if(msg_transport == 0)
      {
        throw Exception(
          "::NewsGate::SearchMailing::SearchMailerImpl::"
          "search: dynamic_cast<const Message::Transport::"
          "StoredMessagePackImpl::Type*> failed");
      }

      subs.search_time = tm.sec();

      tm = ACE_OS::gettimeofday() - tm;
      
      Message::Transport::StoredMessageArrayPtr messages(
        msg_transport->release());
      
      std::ostringstream ostr;
      ostr << "SearchMailerImpl::search(" << subs.id.string(El::Guid::GF_DENSE)
           << "): " << search_result->total_matched_messages << " (+"
           << search_result->suppressed_messages << ") messages for "
           << El::Moment::time(tm);
      
      Application::logger()->trace(ostr.str(),
                                   Aspect::STATE,
                                   El::Logging::HIGH);
      
      return messages->empty() ? 0 : messages.release();
    }

    Search::Expression*
    SearchMailerImpl::normalize_search_expression(
      Search::Expression* expression) throw(Exception, El::Exception)
    {
      try
      {
        Dictionary::WordManager_var word_manager =
          Application::instance()->word_manager();        
          
        Search::Transport::ExpressionImpl::Var
          expression_transport =
          Search::Transport::ExpressionImpl::Init::create(
            new Search::Transport::ExpressionHolder(
              El::RefCount::add_ref(expression)));
          
        expression_transport->serialize();

        NewsGate::Search::Transport::Expression_var result;

        word_manager->normalize_search_expression(
          expression_transport.in(),
          result.out());

        if(dynamic_cast<Search::Transport::ExpressionImpl::Type*>(
             result.in()) == 0)
        {
          throw Exception(
            "NewsGate::SearchMailing::SearchMailerImpl::"
            "normalize_search_expression: "
            "dynamic_cast<Search::Transport::ExpressionImpl::Type*> failed");
        }
        
        expression_transport =
          dynamic_cast<Search::Transport::ExpressionImpl::Type*>(
            result._retn());
        
        return expression_transport->entity().expression.retn();
      }
      catch(const Dictionary::NotReady& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
          "normalize_search_expression: Dictionary::NotReady caught. "
          "Reason:\n" << e.reason.in();

        throw Exception(ostr.str());
      }
      catch(const Dictionary::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
          "normalize_search_expression: Dictionary::ImplementationException "
          "caught. Description:\n" << e.description.in();

        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
          "normalize_search_expression: CORBA::Exception caught. "
          "Description:\n" << e;

        throw Exception(ostr.str());
      }      
    }
      
    Search::Expression*
    SearchMailerImpl::segment_search_expression(const char* exp)
      throw(Exception, Search::ParseError, El::Exception)
    {
      std::string expression;
      
      try
      {        
        NewsGate::Segmentation::Segmentor_var segmentor =
          Application::instance()->segmentor();

        if(CORBA::is_nil(segmentor.in()))
        {
          expression = exp;
        }
        else
        {
          CORBA::String_var res = segmentor->segment_query(exp);
          expression = res.in();
        }
      }
      catch(const NewsGate::Segmentation::NotReady& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
          "segment_search_expression: Segmentation::NotReady "
          "caught. Description:\n" << e.reason.in();

        throw Exception(ostr.str());
      }
      catch(const NewsGate::Segmentation::InvalidArgument& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
          "segment_search_expression: Segmentation::InvalidArgument "
          "caught. Description:\n" << e.description.in();

        throw Exception(ostr.str());
      }
      catch(const NewsGate::Segmentation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
          "segment_search_expression: Segmentation::"
          "ImplementationException caught. Description:\n"
             << e.description.in();

        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
          "segment_search_expression: CORBA::Exception caught. "
          "Description:\n" << e;

        throw Exception(ostr.str());
      }

      std::wstring query;
      El::String::Manip::utf8_to_wchar(expression.c_str(), query);
            
      Search::ExpressionParser parser;
      std::wistringstream istr(query);
            
      parser.parse(istr);
      return parser.expression().retn();
    }

    void
    SearchMailerImpl::create_subscription_mail(
      const Subscription& subs,
      const Message::Transport::StoredMessageArray& messages,
      bool prn_src,
      bool prn_country,
      El::Net::SMTP::Message& msg)
      throw(Exception, El::Exception)
    {
      msg.from = email_sender_;
      msg.recipients.push_back(subs.email);
      msg.subject = subs.title;

      std::string id = subs.id.string(El::Guid::GF_DENSE);

      if(subs.format == Subscription::MF_TEXT ||
         subs.format == Subscription::MF_ALL)
      {
        std::string filename =
          config_.config_dir() + "/SubscriptionMail.text";

        std::ostringstream ostr;

        subscription_mail_template_.run(filename.c_str(),
                                        subs,
                                        config_.frontend_hostname().c_str(),
                                        messages,
                                        prn_src,
                                        prn_country,
                                        ostr);
        
        msg.text = ostr.str();
      }            

      if(subs.format == Subscription::MF_HTML ||
         subs.format == Subscription::MF_ALL)
      {
        std::string filename =
          config_.config_dir() + "/SubscriptionMail.html";

        std::ostringstream ostr;

        subscription_mail_template_.run(filename.c_str(),
                                        subs,
                                        config_.frontend_hostname().c_str(),
                                        messages,
                                        prn_src,
                                        prn_country,
                                        ostr);
        
        msg.html = ostr.str();
      }            
    }

    void
    SearchMailerImpl::search_mail_task() throw(Exception, El::Exception)
    {
      ACE_Time_Value period;
      Subscription subs;
      
      try
      {
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();
        
        WriteGuard guard(srv_lock_);

        if(dispatched_search_mails_.empty())
        {
          period = config_.task_period();          
        }
        else
        {
          subs.id = *dispatched_search_mails_.begin();
          
          bool send = load_subscription(subs, connection) &&
            subs.status == Subscription::SS_ENABLED;
        
          dispatched_search_mails_.erase(subs.id);

          if(send)
          {
            generating_search_mails_.insert(subs.id);
            guard.release();

            El::Net::SMTP::Message msg;

            if(recipient_blacklist_.find(subs.email) ==
               recipient_blacklist_.end())
            {
              bool prn_src = false;
              bool prn_country = false;
              
              Message::Transport::StoredMessageArrayPtr messages(
                search(subs, prn_src, prn_country));

              if(messages.get())
              {
                create_subscription_mail(subs,
                                         *messages,
                                         prn_src,
                                         prn_country,
                                         msg);
                
                save_subscription_search_time(subs, connection);
              }
            }
            else
            {
              std::ostringstream ostr;
            
              ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
                "search_mail_task: email to '" << subs.email
                   << "' blacklisted; subs "
                   << subs.id.string(El::Guid::GF_DENSE) << " skipped";
              
              Application::logger()->trace(ostr.str(),
                                           Aspect::STATE,
                                           El::Logging::HIGH);
            }          

            {
              std::ostringstream ostr;
              ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
                "search_mail_task: enqueue mail for "
                   << subs.id.string(El::Guid::GF_DENSE) << std::endl;
              
              dump_mail(msg, ostr);
              
              guard.acquire_write();
        
              if(!msg.recipients.empty())
              {
                Application::logger()->trace(ostr.str(),
                                             Aspect::STATE,
                                             El::Logging::HIGH);

                smtp_message_queue_.push_back(msg);
              }
            
              generating_search_mails_.erase(subs.id);
            }
          }
        }
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
          "search_mail_task: El::Exception caught. Description:\n" << e;
          
        El::Service::Error error(ostr.str().c_str(), this);
        callback_->notify(&error);

        period = config_.task_increased_period();

        if(subs.id != El::Guid::null)
        {
          WriteGuard guard(srv_lock_);
          
          generating_search_mails_.erase(subs.id);
          dispatched_search_mails_.insert(subs.id);
        }
      }
      
      if(started())
      {
        El::Service::CompoundServiceMessage_var msg =
          new SearchMailTask(this);

        if(period == ACE_Time_Value::zero)
        {
          deliver_now(msg.in());
        }
        else
        {
          deliver_at_time(msg.in(), ACE_OS::gettimeofday() + period);
        }         
      }
    }
    
    void
    SearchMailerImpl::dispatch_search_mail() throw(Exception, El::Exception)
    {
      ACE_Time_Value save_period(config_.task_period());
      
      size_t dispatched_mails = 0;
      size_t dispatch_mail_left = 0;
      uint64_t last_dispatch_time = 0;
      uint64_t day_start = 0;

      GuidSet dispatched_ids;
      
      std::ostringstream log_str;
      
      try
      {
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();
        
        WriteGuard guard(srv_lock_);

        uint64_t cur_sec = ACE_OS::gettimeofday().sec();
        uint64_t last_dispatch_day_start = last_dispatch_time_ / 86400 * 86400;

        if(last_dispatch_time_ == 0)
        {
          El::MySQL::Result_var result =
            connection->query(
              "select last_dispatch_time, mailer_id from SearchMailState");
      
          DB::MailState record(result.in());

          if(!record.fetch_row())
          {
            El::Service::Error error(
              "NewsGate::SearchMailing::SearchMailerImpl::"
              "dispatch_search_mail: failed to read from "
              "SearchMailState; dispatching disabled ... ", this);
            
            callback_->notify(&error);
            return;
          }

          // record.last_dispatch_time() can be not too accurate due to
          // restoration from backup.
          last_dispatch_time_ = cached_dispatch_time_ ? cached_dispatch_time_ :
            record.last_dispatch_time();
          
          std::string mailer_id = record.mailer_id();
      
          if(last_dispatch_time_ && mailer_id != mailer_id_)
          {
            std::ostringstream ostr;
            ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
              "dispatch_search_mail: mailer id '" << mailer_id_
                 << "' do not match the one from DB '" << mailer_id
                 << "'; dispatching disabled ... ";
          
            El::Service::Error error(ostr.str().c_str(), this);
            callback_->notify(&error);            
            return;
          }
            
          std::ostringstream ostr;
          ostr << "SearchMailerImpl::dispatch_search_mail: "
            "start dispatching from "
               << El::Moment(ACE_Time_Value(last_dispatch_time_)).iso8601();
          
          Application::logger()->trace(ostr.str(),
                                       Aspect::STATE,
                                       El::Logging::HIGH);
        }

        bool day_change = day_start_ && day_start_ < last_dispatch_day_start;
        
        if(day_start_ == 0 || day_change)
        {
          SearchMailTimesArray mail_times;
          load_mail_schedule(connection, mail_times);
      
          day_start_ = cur_sec / 86400 * 86400;
          day_week_ = El::Moment(ACE_Time_Value(day_start_)).tm_wday;

          if(day_change)
          {
            last_dispatch_time_ = day_start_;
          }          
          
          log_str << "\nschedule reload:";
          search_mail_times_.clear();
          
          for(SearchMailTimesArray::const_iterator i(mail_times.begin()),
                e(mail_times.end()); i != e; ++i)
          {
            add_search_mail_times(*i, &log_str);
          }
        }

        while(!search_mail_times_.empty())
        {
          SearchMailTimeSet::iterator i(search_mail_times_.begin());
          const SearchMailTime& smt = *i;

          if(smt.time < last_dispatch_time_)
          {
            log_str << "\ntoo late for "
                    << El::Moment(ACE_Time_Value(smt.time)).iso8601()
                    << " for " << smt.id.string(El::Guid::GF_DENSE);
              
            search_mail_times_.erase(i);
            continue;
          }

          if(smt.time > cur_sec)
          {
            break;
          }

          if(dispatched_ids.find(smt.id) != dispatched_ids.end())
          {
            log_str << "\nskipped duplicate "
                    << El::Moment(ACE_Time_Value(smt.time)).iso8601()
                    << " for " << smt.id.string(El::Guid::GF_DENSE);
              
            search_mail_times_.erase(i);
            continue;            
          }
        
          if(dispatched_search_mails_.find(smt.id) !=
             dispatched_search_mails_.end())
          {
            log_str << "\nskipped dispatched "
                    << El::Moment(ACE_Time_Value(smt.time)).iso8601()
                    << " for " << smt.id.string(El::Guid::GF_DENSE);
              
            search_mail_times_.erase(i);
            continue;            
          }

          if(generating_search_mails_.find(smt.id) !=
             generating_search_mails_.end())
          {
            log_str << "\nskipped processed "
                    << El::Moment(ACE_Time_Value(smt.time)).iso8601()
                    << " for " << smt.id.string(El::Guid::GF_DENSE);
              
            search_mail_times_.erase(i);
            continue;            
          }        

          log_str << "\ndispatch "
                  << El::Moment(ACE_Time_Value(smt.time)).iso8601()
                  << " for " << smt.id.string(El::Guid::GF_DENSE);          
          
          dispatched_ids.insert(smt.id);
          dispatched_search_mails_.insert(smt.id);

          search_mail_times_.erase(i);
          ++dispatched_mails;
        }

        dispatch_mail_left = search_mail_times_.size();
        last_dispatch_time_ = cur_sec;        

        last_dispatch_time = last_dispatch_time_;
        day_start = day_start_;
        
        {
          std::ostringstream ostr;
          ostr << "update SearchMailState set last_dispatch_time="
               << last_dispatch_time_ << ", mailer_id='"
               << connection->escape(mailer_id_.c_str()) << "'";
          
          El::MySQL::Result_var result = connection->query(ostr.str().c_str());
        }
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
          "dispatch_search_mail: El::Exception caught. Description:\n" << e;
          
        El::Service::Error error(ostr.str().c_str(), this);
        callback_->notify(&error);

        save_period = config_.task_increased_period();
      }

      std::string ls = log_str.str();
      
      if(!ls.empty())
      {
        std::ostringstream ostr;
        ostr << "SearchMailerImpl::dispatch_search_mail: "
             << dispatched_mails << " dispatched, "
             << dispatch_mail_left << " left, last_dispatch_time "
             << El::Moment(ACE_Time_Value(last_dispatch_time)).iso8601()
             << ", day_start "
             << El::Moment(ACE_Time_Value(day_start)).iso8601()
             << ls;
        
        Application::logger()->trace(ostr.str(),
                                     Aspect::STATE,
                                     El::Logging::HIGH);
      }

      if(started())
      {
        El::Service::CompoundServiceMessage_var msg =
          new DispatchSearchMail(this);
        
        deliver_at_time(msg.in(), ACE_OS::gettimeofday() + save_period);
      }      
    }
    
    void
    SearchMailerImpl::confirm_operations() throw(Exception, El::Exception)
    {
      ACE_Time_Value task_period(config_.task_period());
      
      try
      {
        bool confirmed = false;
        std::ostringstream ostr;

        {
          WriteGuard guard(srv_lock_);

          if(!confirmations_.empty())
          {
            ostr << "SearchMailerImpl::confirm_operations: schedule update:";

            ACE_Time_Value total_tm = ACE_OS::gettimeofday();

            while(!confirmations_.empty())
            {
              Confirmation conf = *confirmations_.begin();
              guard.release();

              if(!started())
              {
                break;
              }
          
              std::string eml;
              std::string sess;          
              SearchMailTimes mail_times;
              SearchMailTimesArray mail_old_times;

              bool email_changed = false;
          
              bool result = confirm_op(conf,
                                       email_changed,
                                       eml,
                                       sess,
                                       mail_times,
                                       mail_old_times);
          
              guard.acquire_write();

              confirmations_.pop_front();

              if(result)
              {
                remove_search_mail_times(mail_old_times, &ostr);
                add_search_mail_times(mail_times, &ostr);
              }
            }

            total_tm = ACE_OS::gettimeofday() - total_tm;
            ostr << "\n  time: " << El::Moment::time(total_tm);

            confirmed = true;
          }
        }

        if(confirmed)
        {
          Application::logger()->trace(ostr.str(),
                                       Aspect::STATE,
                                       El::Logging::HIGH);
        }
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
          "confirm_operations: El::Exception caught. Description:\n" << e;
          
        El::Service::Error error(ostr.str().c_str(), this);
        callback_->notify(&error);

        task_period = config_.task_increased_period();
      }
      
      if(started())
      {
        El::Service::CompoundServiceMessage_var msg =
          new ConfirmOperations(this);
        
        deliver_at_time(msg.in(), ACE_OS::gettimeofday() + task_period);
      }
    }

    bool
    SearchMailerImpl::confirm_op(const Confirmation& conf,
                                 bool& email_changed,
                                 std::string& email,
                                 std::string& session,
                                 SearchMailTimes& mail_times,
                                 SearchMailTimesArray& mail_old_times)
      throw(Exception, El::Exception)
    {
      email_changed = false;
      
      bool result = false;
      El::Net::SMTP::Message msg;
      
      SubscriptionUpdate update;
      Subscription& subscription = update.subscription;
      
      update.token = conf.token;

      El::MySQL::Connection_var connection =
        Application::instance()->dbase()->connect();

      El::MySQL::Result_var res = connection->query("begin");

      try
      {
        result = update.load(connection) &&
          update.state == SubscriptionUpdate::US_NEW;

        if(result && update.type == SubscriptionUpdate::UT_ENABLE)
        {
          Subscription::SubsStatus status = subscription.status;
          std::string session = subscription.user_session;

          result = load_subscription(subscription, connection);
          
          if(result)
          {
            subscription.status = status;
            subscription.user_session = session;
          }
        }

        if(result)
        {
          email = subscription.email;
          session = subscription.user_session;
          
          bool subscription_updated = false;
          ACE_Time_Value cur_time = ACE_OS::gettimeofday();
          
          update.conf_time = cur_time.sec();
          
          subscription.user_id = conf.user;
          subscription.user_ip = conf.ip;
          subscription.user_agent = conf.agent;

          if(subscription.id != El::Guid::null)
          {
            update.state = SubscriptionUpdate::US_CONFIRMED;

            if(update.conf_email == subscription.email)
            {
              Subscription old_subs;
              old_subs.id = subscription.id;
            
              if(load_subscription(old_subs, connection))
              {
                mail_old_times.push_back(SearchMailTimes());
                SearchMailTimes& smt = *mail_old_times.rbegin();

                smt.id = old_subs.id;
                smt.times.swap(old_subs.times);
              }

              El::Guid existing_id = find_by_search(subscription, connection);
              
              subscription.update_time = cur_time;
              save_subscription(subscription, connection);

              subscription_updated = true;

              if(existing_id != El::Guid::null &&
                 existing_id != subscription.id)
              {
                old_subs.id = existing_id;
                
                if(load_subscription(old_subs, connection))
                {
                  mail_old_times.push_back(SearchMailTimes());
                  SearchMailTimes& smt = *mail_old_times.rbegin();

                  smt.id = old_subs.id;
                  smt.times.swap(old_subs.times);

                  old_subs.update_time = cur_time;
                  old_subs.status = Subscription::SS_DELETED;
                  
                  save_subscription_status(old_subs, connection);
                }
              }
              
              mail_times.id = subscription.id;
        
              if(subscription.status == Subscription::SS_ENABLED)
              {
                mail_times.times.swap(subscription.times);
              }
            }
            else
            {
              SubscriptionUpdate email_update;
            
              email_update.token.generate();
              email_update.subscription = subscription;
              email_update.conf_email = subscription.email;

              create_confirmation_mail(email_update,
                                       email_update.subscription.title.c_str(),
                                       msg,
                                       "SubscriptionEmailUpdateMail");
            
              email_update.save(connection);
              email_changed = true;
            }
          }
          
          update.confirm(connection, !subscription_updated);
        }
            
        res = connection->query("commit");
      }
      catch(...)
      {
        res = connection->query("rollback");
        throw;
      }

      if(!msg.recipients.empty())
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::SearchMailerImpl::confirm_op: "
          "enqueue mail for update " << update.token.string(El::Guid::GF_DENSE)
             << std::endl;
        
        dump_mail(msg, ostr);
        
        WriteGuard guard(srv_lock_);        
        
        Application::logger()->trace(ostr.str(),
                                     Aspect::STATE,
                                     El::Logging::HIGH);
        
        smtp_message_queue_.push_back(msg);
      }
      
      return result;
    }

    void
    SearchMailerImpl::dump_mail(const El::Net::SMTP::Message& msg,
                                std::ostream& ostr)
      throw(El::Exception)
    {
      ostr << "From: '" << msg.from << "'\nTo:";

      for(El::Net::SMTP::Message::RecipientArray::const_iterator
            i(msg.recipients.begin()), e(msg.recipients.end()); i != e; ++i)
      {
        ostr << " '" << *i << "'";
      }

      ostr << "\nSubject: '" << msg.subject
           << "'\nText:\n"
           << std::string(msg.text,
                          0,
                          std::min((size_t)3 * 1024, msg.text.length()))
           << "\nHTML:\n"
           << std::string(msg.html,
                          0,
                          std::min((size_t)3 * 1024, msg.html.length()));
    }
    
    void
    SearchMailerImpl::create_confirmation_mail(const SubscriptionUpdate& update,
                                               const char* title,
                                               El::Net::SMTP::Message& msg,
                                               const char* templ_file)
      throw(El::Exception)
    {
      msg.from = email_sender_;
      msg.recipients.push_back(update.conf_email);

      El::String::Template::VariablesMap vars;          
      vars["TOKEN"] = update.token.string(El::Guid::GF_DENSE);          
      vars["TITLE"] = title;
      vars["SENDER"] = config_.sender_name();

      std::string filename = config_.config_dir() + "/" + templ_file +
        "." + update.subscription.lang.l3_code();
          
      El::Cache::TextTemplateFile* templ =
        subscription_confirmation_template_.get(filename.c_str());
            
      std::ostringstream ostr;
      templ->instantiate(vars, ostr);
      std::string text = ostr.str();

      std::string::size_type pos = text.find('\n');
      assert(pos != std::string::npos);

      msg.subject.assign(text.c_str(), pos);
      msg.text = text.c_str() + pos + 1;
/*
      filename = config_.config_dir() + "/" + templ_file +
        ".html." + update.subscription.lang.l3_code();
          
      templ = subscription_confirmation_template_.get(filename.c_str());

      {
        std::ostringstream ostr;
        templ->instantiate(vars, ostr);
        msg.html = ostr.str();
      }
*/      
    }

    void
    SearchMailerImpl::update_subscriptions() throw(Exception, El::Exception)
    {
      ACE_Time_Value task_period(config_.task_period());

      try
      {
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();
        
        WriteGuard guard(srv_lock_);

        while(!subscription_update_reqs_.empty())
        {
          SubscriptionUpdate update = *subscription_update_reqs_.begin();
          guard.release();

          if(!started())
          {
            break;
          }

          ACE_Time_Value total_tm = ACE_OS::gettimeofday();

          Subscription& subscription = update.subscription;
          
          update.token.generate();
          update.conf_email = subscription.email;

          const char* templ = 0;
          std::string title = subscription.title;
          
          switch(update.type)
          {
          case SubscriptionUpdate::UT_UPDATE:
            {
              if(subscription.id == El::Guid::null)
              {
                subscription.id = find_by_search(subscription, connection);
              }
              
              if(subscription.id == El::Guid::null)
              {
                subscription.id.generate();
                templ = "SubscriptionCreationMail";
              }
              else
              {
                Subscription subs;
                subs.id = subscription.id;
                
                if(load_subscription(subs, connection))
                {
                  title = subs.title;
                  update.conf_email = subs.email;
                  
                  templ = "SubscriptionUpdateMail";
                }
                else
                {
                  std::ostringstream ostr;
                  ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
                    "update_subscriptions: no subscription with id "
                       << subs.id.string(El::Guid::GF_DENSE) << " found";
                  
                  Application::logger()->trace(ostr.str(),
                                               Aspect::STATE,
                                               El::Logging::HIGH);        
                }
              }

              break;
            }
          case SubscriptionUpdate::UT_ENABLE:            
            {
              Subscription subs;
              subs.id = subscription.id;
              
              if(load_subscription(subs, connection) &&
                 subscription.status != subs.status)
              {
                switch(subscription.status)
                {
                case Subscription::SS_ENABLED:
                  {
                    templ = "SubscriptionEnablingMail";
                    break;
                  }
                case Subscription::SS_DISABLED:
                  {
                    templ = "SubscriptionDisablingMail";
                    break;
                  }
                case Subscription::SS_DELETED:
                  {
                    templ = "SubscriptionDeletingMail";
                    break;
                  }
                default:
                  {
                    assert(false);
                  }
                }
              }
              
              break;
            }
          case SubscriptionUpdate::UT_MANAGE:
            {
              templ = "SubscriptionManagementMail";
              break;
            }
          }

          ACE_Time_Value p1 = ACE_OS::gettimeofday() - total_tm;

          El::Net::SMTP::Message msg;
          
          if(templ)
          {
            if(update.state != SubscriptionUpdate::US_TRUSTED)
            {
              create_confirmation_mail(update, title.c_str(), msg, templ);
            }
            
            update.save(connection);
          } 

          {
            std::ostringstream ostr;
            ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
              "update_subscriptions: enqueue mail for update "
                 << update.token.string(El::Guid::GF_DENSE) << std::endl;
            
            dump_mail(msg, ostr);        

            guard.acquire_write();
          
            subscription_update_reqs_.pop_front();

            if(templ)
            {
              if(update.state == SubscriptionUpdate::US_TRUSTED)
              {
                Confirmation conf(
                  update.token.string(El::Guid::GF_DENSE).c_str(),
                  subscription.user_id.c_str(),
                  subscription.user_ip.c_str(),
                  subscription.user_agent.c_str());
                
                confirmations_.push_back(conf);
              }
              else
              {
                Application::logger()->trace(ostr.str(),
                                             Aspect::STATE,
                                             El::Logging::HIGH);
                
                smtp_message_queue_.push_back(msg);
              }
            }
          }
          
          total_tm = ACE_OS::gettimeofday() - total_tm;

          std::ostringstream ostr;
          ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
            "update_subscriptions: update "
               << update.token.string(El::Guid::GF_DENSE)
               << " preparation time " << El::Moment::time(total_tm)
               << " / " << El::Moment::time(p1);

          Application::logger()->trace(ostr.str(),
                                       Aspect::STATE,
                                       El::Logging::HIGH);
        }
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
          "update_subscriptions: El::Exception caught. Description:\n" << e;
          
        El::Service::Error error(ostr.str().c_str(), this);
        callback_->notify(&error);

        task_period = config_.task_increased_period();
      }
      
      if(started())
      {
        El::Service::CompoundServiceMessage_var msg =
          new UpdateSubscription(this);
        
        deliver_at_time(msg.in(), ACE_OS::gettimeofday() + task_period);
      }
    }

    void
    SearchMailerImpl::send_smtp_message() throw(Exception, El::Exception)
    {
      ACE_Time_Value task_period(config_.task_period());
      SmtpMessageQueue failed_smtp_messages;

      try
      {
        std::auto_ptr<El::Net::SMTP::Session> session;
        
        WriteGuard guard(srv_lock_);

        while(!smtp_message_queue_.empty())
        {
          El::Net::SMTP::Message msg = *smtp_message_queue_.begin();
          smtp_message_queue_.pop_front();

          guard.release();

          if(recipient_blacklist_.find(msg.recipients[0]) !=
             recipient_blacklist_.end())
          {
            std::ostringstream ostr;
            
            ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
              "send_smtp_message: email to '" << msg.recipients[0]
                 << "' blacklisted; subject: '" << msg.subject
                 << "', text:\n" << msg.text << "\nHTML:\n" << msg.html;

            Application::logger()->trace(ostr.str(),
                                         Aspect::STATE,
                                         El::Logging::HIGH);        
            
            continue;
          }
          
          try
          {
            if(session.get() == 0)
            {
              session.reset(
                new El::Net::SMTP::Session(config_.smtp().host().c_str(),
                                           config_.smtp().port()));
            }

            session->send(msg);

            std::ostringstream ostr;
            ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
              "send_smtp_message: sent to '" << msg.recipients[0]
                 << "'";
            
            std::string debug_email = config_.debug_email();
            
            if(!debug_email.empty())
            {
              msg.recipients.clear();
              msg.recipients.push_back(debug_email);
              session->send(msg);

              ostr << " and to '" << debug_email << "'";
            }

            Application::logger()->trace(ostr.str(),
                                         Aspect::STATE,
                                         El::Logging::HIGH);
          }
          catch(const El::Exception& e)
          {
            failed_smtp_messages.push_back(msg);
            task_period = config_.task_increased_period();
            
            std::ostringstream ostr;
            
            ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
              "send_smtp_message: El::Net::SMTP::Session::send('"
                 << msg.recipients[0]
                 << "', '" << msg.subject << "') failed. Reason:\n" << e
                 << "\nText:\n" << msg.text << "\nHTML:\n" << msg.html;
            
            El::Service::Error error(ostr.str().c_str(), this);
            callback_->notify(&error);            
          }

          if(!started())
          {
            break;
          }          
        }
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
          "send_smtp_message: El::Exception caught. Description:\n" << e;
            
        El::Service::Error error(ostr.str().c_str(), this);
        callback_->notify(&error);
        
        task_period = config_.task_increased_period();
      }

      if(!failed_smtp_messages.empty())
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
          "send_smtp_message: " << failed_smtp_messages.size()
             << " emails failed to send";
          
        Application::logger()->trace(ostr.str(),
                                     Aspect::STATE,
                                     El::Logging::HIGH);        
        
        WriteGuard guard(srv_lock_);

        while(!failed_smtp_messages.empty())
        {
          smtp_message_queue_.push_front(*failed_smtp_messages.rbegin());
          failed_smtp_messages.pop_back();
        }
      }
      
      if(started())
      {
        El::Service::CompoundServiceMessage_var msg =
          new SendSmtpMessage(this);
        
        deliver_at_time(msg.in(), ACE_OS::gettimeofday() + task_period);
      }
    }

    void
    SearchMailerImpl::cleanup_task() throw(Exception, El::Exception)
    {
      uint64_t now = ACE_OS::gettimeofday().sec();
      
      try
      {
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();
        
        El::MySQL::Result_var result;
        
        {
          std::ostringstream ostr;
          ostr << "update SearchMailSubscriptionUpdate set conf_time="
               << now << ", state='E' where state='N' and "
            "UNIX_TIMESTAMP(CONVERT_TZ(req_time, '+00:00', 'SYSTEM')) < "
               << now - config_.update_timeout();
      
          result = connection->query(ostr.str().c_str());
        }
  
        {
          std::ostringstream ostr;
          ostr << "delete from SearchMailSubscriptionUpdate where "
            "state!='N' and conf_time < "
               << now - config_.subscription_timeout();
      
          result = connection->query(ostr.str().c_str());
        }
  
        {
          std::ostringstream ostr;
          ostr << "delete from SearchMailSubscription where status='L' and "
            "UNIX_TIMESTAMP(CONVERT_TZ(update_time, '+00:00', 'SYSTEM')) < "
               << now - config_.subscription_timeout();
      
          result = connection->query(ostr.str().c_str());
        }
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::SearchMailerImpl::"
          "cleanup_task: El::Exception caught. Description:\n" << e;
            
        El::Service::Error error(ostr.str().c_str(), this);
        callback_->notify(&error);
      }

      if(started())
      {
        El::Service::CompoundServiceMessage_var msg =
          new CleanupTask(this);
        
        deliver_at_time(
          msg.in(),
          ACE_OS::gettimeofday() + ACE_Time_Value(config_.cleanup_period()));
      }
    }

    void
    SearchMailerImpl::remove_search_mail_times(
      const SearchMailTimesArray& mail_times,
      std::ostream* log) throw(Exception, El::Exception)
    {
      for(SearchMailTimesArray::const_iterator i(mail_times.begin()),
            e(mail_times.end()); i != e; ++i)
      {
        remove_search_mail_times(*i, log);
      }
    }
    
    void
    SearchMailerImpl::remove_search_mail_times(
      const SearchMailTimes& mail_times,
      std::ostream* log) throw(Exception, El::Exception)
    {
      if(day_start_)
      {
        for(TimeSet::const_iterator i(mail_times.times.begin()),
              e(mail_times.times.end()); i != e; ++i)
        {
          ACE_Time_Value tm(day_start_ + i->time * 60);          
          search_mail_times_.erase(SearchMailTime(mail_times.id, tm.sec()));

          if(log)
          {
            *log << "\n  removed " << El::Moment(tm).iso8601() << " for "
                 << mail_times.id.string(El::Guid::GF_DENSE);
          }
        }
      }
    }
        
     
    bool
    SearchMailerImpl::notify(El::Service::Event* event) throw(El::Exception)
    {
      if(El::Service::CompoundService<>::notify(event))
      {
        return true;
      }
      
      if(dynamic_cast<SendSmtpMessage*>(event) != 0)
      {
        send_smtp_message();
        return true;
      }

      if(dynamic_cast<SearchMailTask*>(event) != 0)
      {
        search_mail_task();
        return true;
      }

      if(dynamic_cast<UpdateSubscription*>(event) != 0)
      {
        update_subscriptions();
        return true;
      }

      if(dynamic_cast<ConfirmOperations*>(event) != 0)
      {
        confirm_operations();
        return true;
      }

      if(dynamic_cast<DispatchSearchMail*>(event) != 0)
      {
        dispatch_search_mail();
        return true;
      }

      if(dynamic_cast<CleanupTask*>(event) != 0)
      {
        cleanup_task();
        return true;
      }

      return false;
    }
    
    void
    SearchMailerImpl::wait() throw(Exception, El::Exception)
    {
      El::Service::CompoundService<>::wait();      

      WriteGuard guard(srv_lock_);

      if(!cache_saved_)
      {
        std::fstream file;

        {
          El::BinaryOutStream bstr(file);
          file.open(config_.cache_file().c_str(), ios::out);

          if(file.is_open())
          {
            bstr << (uint32_t)1 << last_dispatch_time_;
            bstr.write_array(subscription_update_reqs_);
            bstr.write_array(confirmations_);
            bstr.write_set(dispatched_search_mails_);
            bstr.write_array(smtp_message_queue_);
          
            cache_saved_ = true;
          }
          else
          {
            std::ostringstream ostr;
            ostr << "NewsGate::SearchMailing::SearchMailerImpl::wait: "
              "failed to open " << config_.cache_file() << " for write access";
            
            El::Service::Error error(ostr.str().c_str(), this);
            callback_->notify(&error);
          }
        }

        if(cache_saved_)
        {
          file.flush();
          
          if(file.fail())
          {
            file.close();
            unlink(config_.cache_file().c_str());

            std::ostringstream ostr;
            ostr << "NewsGate::SearchMailing::MailManager::wait: "
              "failed to save to " << config_.cache_file();
            
            El::Service::Error error(ostr.str().c_str(), this);
            callback_->notify(&error);
          }
        }

        bank_client_session_ = 0;
      }
    }

    void
    SearchMailerImpl::add_search_mail_times(const SearchMailTimes& mail_times,
                                            std::ostream* log)
      throw(Exception, El::Exception)
    {      
      if(day_start_)
      {
        for(TimeSet::const_iterator i(mail_times.times.begin()),
              e(mail_times.times.end()); i != e; ++i)
        {
          const Time& t = *i;

          if(t.day == 0 || t.day - 1 == day_week_)
          {
            ACE_Time_Value tm(day_start_ + t.time * 60);
            search_mail_times_.insert(SearchMailTime(mail_times.id, tm.sec()));
            
            if(log)
            {
              *log << "\n  added " << El::Moment(tm).iso8601() << " for "
                   << mail_times.id.string(El::Guid::GF_DENSE);
            }
          }
        }
      }
    }
    
    void
    SearchMailerImpl::load_mail_schedule(
      El::MySQL::Connection* connection,
      SearchMailTimesArray& search_mail_times)
      throw(Exception, El::Exception)
    {
      El::MySQL::Result_var result =
        connection->query(
          "select id, status, reg_time, reg_time_usec, update_time, "
          "update_time_usec, search_time, email, format, length, time_offset, "
          "title, query, modifier, filter, res_query, res_filter_lang, "
          "res_filter_country, res_filter_feed, res_filter_category, "
          "res_filter_event, locale_lang, locale_country, lang, user_id, "
          "user_ip, user_agent, user_session, times from "
          "SearchMailSubscription where status='E'");
      
      DB::Subscription record(result.in());
      
      while(record.fetch_row())
      {
        Subscription subs;
        subs_from_row(subs, record);

        SearchMailTimes mail_times;
        mail_times.id = subs.id;
        mail_times.times.swap(subs.times);

        search_mail_times.push_back(mail_times);
      }
    }

    void
    SearchMailerImpl::load_subscriptions(const char* email,
                                         SubscriptionArray& subscriptions,
                                         El::MySQL::Connection* connection)
      throw(Exception, El::Exception)
    {
      std::ostringstream ostr;
      ostr << "select id, status, reg_time, reg_time_usec, update_time, "
        "update_time_usec, search_time, email, format, length, time_offset, "
        "title, query, modifier, filter, res_query, res_filter_lang, "
        "res_filter_country, res_filter_feed, res_filter_category, "
        "res_filter_event, locale_lang, locale_country, lang, user_id, "
        "user_ip, user_agent, user_session, times from "
        "SearchMailSubscription where email='"
           << connection->escape(email) << "' and status!='L' order by title;";
      
      El::MySQL::Result_var result = connection->query(ostr.str().c_str());
      DB::Subscription record(result.in());
      
      while(record.fetch_row())
      {
        subscriptions.push_back(Subscription());        
        subs_from_row(*subscriptions.rbegin(), record);
      }
    }

    El::Guid
    SearchMailerImpl::find_by_search(const Subscription& subscription,
                                     El::MySQL::Connection* connection)
      throw(Exception, El::Exception)
    {
      const Search::Strategy::Filter& rf = subscription.resulted_filter;
      
      std::ostringstream ostr;
      ostr << "select id from SearchMailSubscription where email='"
           << connection->escape(subscription.email.c_str())
           << "' and res_query='"
           << connection->escape(subscription.resulted_query.c_str())
           << "' and res_filter_category='"
           << connection->escape(rf.category.c_str())
           << "' and res_filter_feed='" << connection->escape(rf.feed.c_str())
           << "' and res_filter_event=" << rf.event.data
           << " and res_filter_lang='" << rf.lang.l3_code(true)
           << "' and res_filter_country='" << rf.country.l3_code(true)
           << "' and status!='L';";
      
      El::MySQL::Result_var result = connection->query(ostr.str().c_str());
      DB::Subscription record(result.in(), 1);

      El::Guid res;
      
      if(record.fetch_row())
      {
        res = record.id().c_str();
      }

      return res;
    }

    void
    SearchMailerImpl::subs_from_row(Subscription& subs,
                                    DB::Subscription& record)
      throw(El::Exception)
    {
      subs.id = record.id().c_str();

      switch(record.status().c_str()[0])
      {
      case 'E': subs.status = Subscription::SS_ENABLED; break;
      case 'D': subs.status = Subscription::SS_DISABLED; break;
      case 'L': subs.status = Subscription::SS_DELETED; break;
      }
      
      subs.reg_time.set_iso8601(record.reg_time().c_str());
      subs.reg_time.tm_usec = record.reg_time_usec();
      subs.update_time.set_iso8601(record.update_time().c_str());
      subs.update_time.tm_usec = record.update_time_usec();
      subs.search_time = record.search_time();
      subs.email = record.email();
      subs.format = (Subscription::MailFormat)record.format().value();
      subs.length = record.length();
      subs.time_offset = record.time_offset();
      subs.title = record.title();
      subs.query = record.query();

      subs.modifier = record.modifier();
      subs.filter = record.filter();
      subs.resulted_query = record.res_query();
        
      Search::Strategy::Filter& rf = subs.resulted_filter;
      rf.lang = El::Lang(record.res_filter_lang().c_str());
      rf.country = El::Country(record.res_filter_country().c_str());
      rf.feed = record.res_filter_feed();
      rf.category = record.res_filter_category();
      rf.event.data = record.res_filter_event();

      El::Locale& loc = subs.locale;
      loc.lang = El::Lang(record.locale_lang().c_str());
      loc.country = El::Country(record.locale_country().c_str());

      subs.lang = El::Lang(record.lang().c_str());
      subs.user_id = record.user_id();
      subs.user_ip = record.user_ip();
      subs.user_agent = record.user_agent();
      subs.user_session = record.user_session();

      std::istringstream istr(std::string(record.times()));
      El::BinaryInStream bstr(istr);
      bstr.read_set(subs.times);
    }

    bool
    SearchMailerImpl::trust_update(const Subscription& subs,
                                   El::MySQL::Connection* connection) const
      throw(El::Exception)
    {
      size_t trust_timeout = config_.trust_timeout();

      if(!trust_timeout || subs.user_id.empty())
      {
        return false;
      }

      uint64_t time = ACE_OS::gettimeofday().sec() - trust_timeout;

      El::MySQL::Result_var result;

      {
        std::ostringstream ostr;
        ostr << "select id from SearchMailSubscriptionUpdate where email='"
             << connection->escape(subs.email.c_str())
             << "' and user_id='" << connection->escape(subs.user_id.c_str())
             << "' and user_ip='" << connection->escape(subs.user_ip.c_str())
             << "' and user_agent='"
             << connection->escape(subs.user_agent.c_str())
             << "' and user_session='"
             << connection->escape(subs.user_session.c_str())
             << "' and conf_time > " << time;

        result = connection->query(ostr.str().c_str());

        DB::Subscription record(result.in(), 1);
        
        if(record.fetch_row())
        {
          return true;
        }
      }
        
      std::ostringstream ostr;
      ostr << "select id from SearchMailSubscription where email='"
           << connection->escape(subs.email.c_str())
           << "' and user_id='" << connection->escape(subs.user_id.c_str())
           << "' and user_ip='" << connection->escape(subs.user_ip.c_str())
           << "' and user_agent='"
           << connection->escape(subs.user_agent.c_str())
             << "' and user_session='"
             << connection->escape(subs.user_session.c_str())
           << "' and UNIX_TIMESTAMP("
        "CONVERT_TZ(update_time, '+00:00', 'SYSTEM')) > " << time;

      result = connection->query(ostr.str().c_str());

      DB::Subscription record(result.in(), 1);
      return record.fetch_row();
    }    
    
    bool
    SearchMailerImpl::load_subscription(Subscription& subs,
                                        El::MySQL::Connection* connection)
      throw(El::Exception)
    {
      std::ostringstream ostr;
      
      ostr << "select id, status, reg_time, reg_time_usec, update_time, "
        "update_time_usec, search_time, email, format, length, time_offset, "
        "title, query, modifier, filter, res_query, res_filter_lang, "
        "res_filter_country, res_filter_feed, res_filter_category, "
        "res_filter_event, locale_lang, locale_country, lang, user_id, "
        "user_ip, user_agent, user_session, times from "
        "SearchMailSubscription where id='"
           << connection->escape(subs.id.string(El::Guid::GF_DENSE).c_str())
           << "' and status!='L'";

      El::MySQL::Result_var result = connection->query(ostr.str().c_str());
      DB::Subscription record(result.in());
      
      if(record.fetch_row())
      {
        subs_from_row(subs, record);
        return true;
      }
      
      return false;
    }
    
    void
    SearchMailerImpl::save_subscription_search_time(
      const Subscription& subs,
      El::MySQL::Connection* connection)
      throw(El::Exception)
    {
      std::ostringstream ostr;
      ostr << "update SearchMailSubscription set search_time="
           << subs.search_time << " where id='"
           << connection->escape(subs.id.string(El::Guid::GF_DENSE).c_str())
           << "'";

      El::MySQL::Result_var result = connection->query(ostr.str().c_str());
    }

    void
    SearchMailerImpl::save_subscription_status(
      const Subscription& subs,
      El::MySQL::Connection* connection)
      throw(El::Exception)
    {
      std::ostringstream ostr;
      
      ostr << "update SearchMailSubscription set status='"
           << subs_db_status(subs) << "', update_time='"
           << connection->escape(
             subs.update_time.iso8601(false, true).c_str())
           << "', update_time_usec=" << subs.update_time.tm_usec
           << " where id='"
           << connection->escape(subs.id.string(El::Guid::GF_DENSE).c_str())
           << "'";

      El::MySQL::Result_var result = connection->query(ostr.str().c_str());
    }
    
    void
    SearchMailerImpl::save_subscription(const Subscription& subs,
                                        El::MySQL::Connection* connection)
      throw(El::Exception)
    {
      std::ostringstream otm;
      
      {
        El::BinaryOutStream bstr(otm);
        bstr.write_set(subs.times);
      }

      std::string tm = otm.str();
      tm = connection->escape(tm.c_str(), tm.length());
      
      const Search::Strategy::Filter& rf = subs.resulted_filter;

      std::string update_time =
        connection->escape(subs.update_time.iso8601(false, true).c_str());

      std::string email = connection->escape(subs.email.c_str());
      std::string title = connection->escape(subs.title.c_str());
      std::string user_id = connection->escape(subs.user_id.c_str());
      std::string user_agent = connection->escape(subs.user_agent.c_str());
      std::string user_session = connection->escape(subs.user_session.c_str());
      
      std::ostringstream ostr;

      ostr << "insert into SearchMailSubscription set id='"
           << connection->escape(subs.id.string(El::Guid::GF_DENSE).c_str())
           << "', status='" << subs_db_status(subs) << "', reg_time='"
           << connection->escape(subs.reg_time.iso8601(false, true).c_str())
           << "', reg_time_usec=" << subs.reg_time.tm_usec
           <<  ", update_time='" << update_time
           << "', update_time_usec=" << subs.update_time.tm_usec
           << ", email='" << email << "', format=" << subs.format
           << ", length=" << subs.length
           << ", time_offset=" << subs.time_offset
           << ", title='"
           << title << "', query='"
           << connection->escape(subs.query.c_str()) << "', modifier='"
           << connection->escape(subs.modifier.c_str(),
                                 subs.modifier.length()) << "', filter='"
           << connection->escape(subs.filter.c_str(), subs.filter.length())
           << "', res_query='"
           << connection->escape(subs.resulted_query.c_str())
           << "', res_filter_lang='" << rf.lang.l3_code(true)
           << "', res_filter_country='" << rf.country.l3_code(true)
           << "', res_filter_feed='" << connection->escape(rf.feed.c_str())
           << "', res_filter_category='"
           << connection->escape(rf.category.c_str())
           << "', res_filter_event=" << rf.event.data
           << ", locale_lang='" << subs.locale.lang.l3_code(true)
           << "', locale_country='"
           << subs.locale.country.l3_code(true)
           << "', lang='" << subs.lang.l3_code(true)
           << "', user_id='" << user_id
           << "', user_ip='" << subs.user_ip
           << "', user_agent='" << user_agent
           << "', user_session='" << user_session
           << "', times='" << tm
           << "' on duplicate key update status='"
           << subs_db_status(subs) << "', update_time='"
           << update_time << "', update_time_usec="
           << subs.update_time.tm_usec
           << ", email='" << email
           << "', format=" << subs.format
           << ", length=" << subs.length
           << ", time_offset=" << subs.time_offset
           << ", title='" << title
           << "', user_id='" << user_id
           << "', user_ip='" << subs.user_ip
           << "', user_agent='" << user_agent
           << "', user_session='" << user_session
           << "', times='" << tm
           << "'";

      El::MySQL::Result_var result =
        connection->query(ostr.str().c_str());
    }

    Message::BankClientSession*
    SearchMailerImpl::bank_client_session(bool reserve) throw(El::Exception)
    {
      Message::BankClientSession_var& bank_client_session = reserve ?
        reserve_bank_client_session_ : bank_client_session_;
        
      {
        ReadGuard guard(srv_lock_);

        if(bank_client_session.in() != 0)
        {
          bank_client_session->_add_ref();
          return bank_client_session.in();
        }
      }

      WriteGuard guard(srv_lock_);

      if(bank_client_session.in() != 0)
      {
        bank_client_session->_add_ref();
        return bank_client_session.in();
      }
        
      try
      {
        std::string message_bank_manager_ref = reserve ?
          config_.reserve_message_bank_manager_ref() :
          config_.message_bank_manager_ref();

        if(reserve && message_bank_manager_ref.empty())
        {
          return 0;
        }

        CORBA::Object_var obj =
          Application::instance()->orb()->string_to_object(
            message_bank_manager_ref.c_str());
      
        if(CORBA::is_nil(obj.in()))
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::SearchMailing::SearchMailerImpl::"
            "bank_client_session: string_to_object() "
            "gives nil reference for " << message_bank_manager_ref;
        
          throw Exception(ostr.str().c_str());
        }

        Message::BankManager_var manager =
          Message::BankManager::_narrow(obj.in());
        
        if (CORBA::is_nil(manager.in()))
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::SearchMailing::SearchMailerImpl::"
            "bank_client_session: _narrow gives nil reference for "
               << message_bank_manager_ref;
      
          throw Exception(ostr.str().c_str());
        }

        bank_client_session = manager->bank_client_session();

        Message::BankClientSessionImpl* bank_client_ses =
          dynamic_cast<Message::BankClientSessionImpl*>(
            bank_client_session.in());
      
        if(bank_client_ses == 0)
        {
          throw Exception(
            "::NewsGate::SearchMailing::SearchMailerImpl::"
            "bank_client_session: "
            "dynamic_cast<Message::BankClientSessionImpl*> failed");
        }

        bank_client_ses->init_threads(this);
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "::NewsGate::SearchMailing::SearchMailerImpl::"
          "bank_client_session: retrieving message bank client "
          "session failed. Reason:\n" << e;

        throw Exception(ostr.str());
      }

      bank_client_session->_add_ref();
      return bank_client_session.in();
    }

    //
    // SearchMailerImpl::SearchMailUpdate struct
    //

    void
    SearchMailerImpl::SubscriptionUpdate::init(
      const DB::SubscriptionUpdate& record)
      throw(El::Exception)
    {
      token = El::Guid(record.token().c_str());

      switch(record.state().c_str()[0])
      {
      case 'N': state = US_NEW; break;
      case 'C': state = US_CONFIRMED; break;
      case 'E': state = US_EXPIRED; break;
      }

      type = (Type)record.type().value();
      conf_email = record.conf_email();
      conf_time = record.conf_time();
      
      subscription.id = El::Guid(record.id().c_str());

      switch(record.status().c_str()[0])
      {
      case 'E': subscription.status = Subscription::SS_ENABLED; break;
      case 'D': subscription.status = Subscription::SS_DISABLED; break;
      case 'L': subscription.status = Subscription::SS_DELETED; break;
      }
      
      subscription.reg_time.set_iso8601(record.req_time().c_str());
      subscription.reg_time.tm_usec = record.req_time_usec();
      subscription.email = record.email();
      subscription.format = (Subscription::MailFormat)record.format().value();
      subscription.length = record.length();
      subscription.time_offset = record.time_offset();
      subscription.title = record.title();
      subscription.query = record.query();
      
      subscription.modifier = record.modifier();
      subscription.filter = record.filter();
      subscription.resulted_query = record.res_query();
        
      Search::Strategy::Filter& rf = subscription.resulted_filter;
      rf.lang = El::Lang(record.res_filter_lang().c_str());
      rf.country = El::Country(record.res_filter_country().c_str());
      rf.feed = record.res_filter_feed();
      rf.category = record.res_filter_category();
      rf.event.data = record.res_filter_event();

      El::Locale& loc = subscription.locale;
      loc.lang = El::Lang(record.locale_lang().c_str());
      loc.country = El::Country(record.locale_country().c_str());

      subscription.lang = El::Lang(record.lang().c_str());
      subscription.user_id = record.user_id();
      subscription.user_ip = record.user_ip();
      subscription.user_agent = record.user_agent();
      subscription.user_session = record.user_session();

      std::istringstream istr(std::string(record.times()));
      El::BinaryInStream bstr(istr);
      bstr.read_set(subscription.times);      
    }
    
    void
    SearchMailerImpl::SubscriptionUpdate::save(
      El::MySQL::Connection* connection)
      const throw(El::Exception)
    {
      std::ostringstream otm;
      
      {
        El::BinaryOutStream bstr(otm);
        bstr.write_set(subscription.times);
      }

      std::string tm = otm.str();
      const Search::Strategy::Filter& rf = subscription.resulted_filter;

      std::ostringstream ostr;
      ostr << "insert into SearchMailSubscriptionUpdate set token='"
           << connection->escape(
             token.string(El::Guid::GF_DENSE).c_str())
           << "', state='" << db_state()
           << "', type=" << type
           << ", conf_email='" << connection->escape(conf_email.c_str())
           << "', conf_time=" << conf_time << ", id='" << connection->escape(
             subscription.id.string(El::Guid::GF_DENSE).c_str())
           << "', status='" << subs_db_status(subscription) << "', req_time='"
           << connection->escape(
             subscription.reg_time.iso8601(false, true).c_str())
           << "', req_time_usec=" << subscription.reg_time.tm_usec
           << ", email='" << connection->escape(subscription.email.c_str())
           << "', format=" << subscription.format
           << ", length=" << subscription.length
           << ", time_offset=" << subscription.time_offset
           << ", title='" << connection->escape(subscription.title.c_str())
           << "', query='" << connection->escape(subscription.query.c_str())
           << "', modifier='"
           << connection->escape(subscription.modifier.c_str(),
                                 subscription.modifier.length())
           << "', filter='" << connection->escape(subscription.filter.c_str(),
                                                  subscription.filter.length())
           << "', res_query='"
           << connection->escape(subscription.resulted_query.c_str())
           << "', res_filter_lang='" << rf.lang.l3_code(true)
           << "', res_filter_country='" << rf.country.l3_code(true)
           << "', res_filter_feed='" << connection->escape(rf.feed.c_str())
           << "', res_filter_category='"
           << connection->escape(rf.category.c_str())
           << "', res_filter_event=" << rf.event.data
           << ", locale_lang='" << subscription.locale.lang.l3_code(true)
           << "', locale_country='"
           << subscription.locale.country.l3_code(true)
           << "', lang='" << subscription.lang.l3_code(true)
           << "', user_id='"
           << connection->escape(subscription.user_id.c_str())
           << "', user_ip='" << subscription.user_ip
           << "', user_agent='"
           << connection->escape(subscription.user_agent.c_str())
           << "', user_session='"
           << connection->escape(subscription.user_session.c_str())
           << "', times='"
           << connection->escape(tm.c_str(), tm.length()) << "'";

      El::MySQL::Result_var result =
        connection->query(ostr.str().c_str());
    }

    void
    SearchMailerImpl::SubscriptionUpdate::confirm(
      El::MySQL::Connection* connection,
      bool update_user_info) const throw(El::Exception)
    {
      std::ostringstream ostr;
      ostr << "update SearchMailSubscriptionUpdate set state='" << db_state()
           << "', conf_time=" << conf_time;
      
      if(update_user_info)
      {
        ostr << ", user_id='"
             << connection->escape(subscription.user_id.c_str())
             << "', user_ip='" << subscription.user_ip << "', user_agent='"
             << connection->escape(subscription.user_agent.c_str())
             << "'";
      }

      ostr << " where token='" << token.string(El::Guid::GF_DENSE) << "'";
      
      El::MySQL::Result_var result = connection->query(ostr.str().c_str());
    }
    
    bool
    SearchMailerImpl::SubscriptionUpdate::load(
      El::MySQL::Connection* connection)
      throw(El::Exception)
    {
      std::ostringstream ostr;
      
      ostr << "select token, state, type, conf_email, conf_time, id, status, "
        "req_time, req_time_usec, email, format, length, time_offset, title, "
        "query, modifier, filter, res_query, res_filter_lang, "
        "res_filter_country, res_filter_feed, res_filter_category, "
        "res_filter_event, locale_lang, locale_country, lang, user_id, "
        "user_ip, user_agent, user_session, times from "
        "SearchMailSubscriptionUpdate where token='"
           << connection->escape(token.string(El::Guid::GF_DENSE).c_str())
           << "'";

      El::MySQL::Result_var result =
        connection->query(ostr.str().c_str());

      DB::SubscriptionUpdate record(result.in());
      
      if(record.fetch_row())
      {
        init(record);
        return true;
      }
      
      return false;
    }

  }
}
