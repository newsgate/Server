/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Commons/SearchMailing/SearchMailing.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_SERVICES_COMMONS_SEARCHMAILING_SEARCHMAILING_HPP_
#define _NEWSGATE_SERVER_SERVICES_COMMONS_SEARCHMAILING_SEARCHMAILING_HPP_

#include <stdint.h>

#include <string>
#include <vector>
#include <set>
#include <deque>

#include <ace/Synch.h>
#include <ace/Guard_T.h>

#include <El/Exception.hpp>
#include <El/Moment.hpp>
#include <El/BinaryStream.hpp>
#include <El/Guid.hpp>
#include <El/Locale.hpp>

#include <Commons/Search/SearchExpression.hpp>

#include <Services/Commons/SearchMailing/SearchMailingServices.hpp>
#include <Services/Commons/FraudPrevention/LimitCheck.hpp>
#include <Services/Commons/FraudPrevention/FraudPreventionServices.hpp>

namespace NewsGate
{
  namespace SearchMailing
  {
    struct Time
    {
      uint8_t day;
      uint16_t time;

      Time() throw() : day(0), time(0) {}
      Time(uint8_t d, uint16_t t) throw() : day(d), time(t) {}

      bool operator<(const Time& val) const throw();
      
      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };
    
    typedef std::set<Time> TimeSet;
    
    struct Subscription
    {
      enum MailFormat
      {
        MF_HTML,
        MF_TEXT,
        MF_ALL,
        MF_COUNT
      };

      enum SubsStatus
      {
        SS_ENABLED,
        SS_DISABLED,
        SS_DELETED,
        SS_COUNT
      };
      
      El::Guid    id;
      SubsStatus  status;
      El::Moment  reg_time;
      El::Moment  update_time;
      uint64_t    search_time;
      std::string email;
      MailFormat  format;
      uint16_t    length;
      int16_t     time_offset;
      std::string title;
      std::string query;
      std::string modifier;
      std::string filter;
      std::string resulted_query;
      Search::Strategy::Filter resulted_filter;
      El::Locale locale;
      El::Lang lang;
      std::string user_id;
      std::string user_ip;
      std::string user_agent;
      TimeSet times;
      std::string user_session;

      static const uint16_t TIMES_MAX = 24;

      Subscription() throw();
      
      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };

    struct Confirmation
    {
      std::string token;
      std::string user;
      std::string ip;
      std::string agent;
      
      Confirmation() throw() {}
      
      Confirmation(const char* t,
                   const char* u,
                   const char* i,
                   const char* a)
        throw(El::Exception);

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };
    
    typedef std::deque<Subscription> SubscriptionArray;
    typedef std::auto_ptr<SubscriptionArray> SubscriptionArrayPtr;

    typedef std::deque<Confirmation> ConfirmationArray;
    typedef std::auto_ptr<ConfirmationArray> ConfirmationArrayPtr;
    
    class MailManager
    {
    public:
      EL_EXCEPTION(Exception, El::ExceptionBase);
      EL_EXCEPTION(LimitExceeded, Exception);

      enum ConfirmingOperation
      {
        CO_NOT_FOUND,
        CO_YES,
        CO_EMAIL_CHANGE
      };
        
      enum UpdatingSubscription
      {
        US_YES,
        US_EMAIL_CHANGE,
        US_CHECK_HUMAN,
        US_MAILED,
        US_NOT_FOUND,
        US_LIMIT_EXCEEDED
      };
      
      enum EnablingSubscription
      {
        ES_YES,
        ES_CHECK_HUMAN,
        ES_MAILED,
        ES_ALREADY,
        ES_NOT_FOUND,
        ES_LIMIT_EXCEEDED
      };

    public:

      MailManager() throw(Exception, El::Exception);
      
      MailManager(const char* mailer_ref,
                  const char* limit_checker_ref,
                  const FraudPrevention::EventLimitCheckDescArray&
                  limit_check_descriptors,
                  CORBA::ORB_ptr orb)
        throw(Exception, El::Exception);

      virtual ~MailManager() throw() {}

      Subscription* get_subscription(const El::Guid& id)
        throw(Exception, El::Exception);
      
      SubscriptionArray* get_subscriptions(const char* email,
                                           const El::Lang& lang,
                                           const char* token,
                                           const char* user,
                                           const char* ip,
                                           const char* agent,
                                           std::string& session,
                                           bool is_human)
        throw(SearchMailing::CheckHuman,
              LimitExceeded,
              Exception,
              El::Exception);
      
      UpdatingSubscription update_subscription(const Subscription& subs,
                                               bool is_human,
                                               std::string& new_session)
        throw(Exception, El::Exception);
      
      ConfirmingOperation confirm_operation(const Confirmation& conf,
                                            std::string& email,
                                            std::string& session)
        throw(Exception, El::Exception);
      
      EnablingSubscription enable_subscription(const char* email,
                                               const El::Guid& id,
                                               Subscription::SubsStatus status,
                                               const char* user,
                                               const char* ip,
                                               const char* agent,
                                               std::string& session,
                                               bool is_human)
        throw(Exception, El::Exception);

    private:
      
      bool check_limits(const char* email,
                        const char* user,
                        const char* ip,
                        FraudPrevention::EventType type)
        throw(Exception, El::Exception);

    private:
      
      typedef El::Corba::SmartRef<Mailer> MailerRef;
      
      typedef El::Corba::SmartRef< ::NewsGate::FraudPrevention::LimitChecker >
      LimitCheckerRef;

      MailerRef mailer_;
      LimitCheckerRef limit_checker_;
      FraudPrevention::EventLimitCheckDescArray limit_check_descriptors_;
    };

    typedef El::RefCount::SmartPtr<MailManager> MailManager_var;
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
    // Time struct
    //
    inline
    bool
    Time::operator<(const Time& val) const throw()
    {
      return day < val.day || (day == val.day && time < val.time);
    }
    
    inline
    void
    Time::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << day << time;
    }
    
    inline
    void
    Time::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> day >> time;
    }
    
    //
    // Subscription struct
    //
    inline
    Subscription::Subscription() throw() :
        status(SS_ENABLED),
        search_time(0),
        format(MF_HTML),
        length(0),
        time_offset(0)
    {
    }
    
    inline
    void
    Subscription::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << id << (uint8_t)status << reg_time << update_time << search_time
           << email << (uint8_t)format << length << time_offset << title
           << query << modifier << filter << resulted_query << resulted_filter
           << locale << lang << user_id << user_ip << user_agent
           << user_session;

      bstr.write_set(times);
    }
    
    inline
    void
    Subscription::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      uint8_t frm = 0;
      uint8_t st = 0;
      
      bstr >> id >> st >> reg_time >> update_time >> search_time >> email
           >> frm >> length >> time_offset >> title >> query >> modifier
           >> filter >> resulted_query >> resulted_filter >> locale >> lang
           >> user_id >> user_ip >> user_agent >> user_session;
      
      bstr.read_set(times);

      format = (MailFormat)frm;
      status = (SubsStatus)st;
    }

    //
    // Confirmation class
    //
    inline
    Confirmation::Confirmation(const char* t,
                               const char* u,
                               const char* i,
                               const char* a)
        throw(El::Exception)
        : token(t),
          user(u),
          ip(i),
          agent(a)
    {
    }
    
    inline
    void
    Confirmation::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << token << user << ip << agent;
    }
    
    inline
    void
    Confirmation::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> token >> user >> ip >> agent;
    }
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_COMMONS_SEARCHMAILING_SEARCHMAILING_HPP_
