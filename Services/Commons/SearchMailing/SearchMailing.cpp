/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Commons/SearchMailing/SearchMailing.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <stdio.h>

#include <sstream>
#include <fstream>
#include <string>

#include <El/CORBA/Corba.hpp>

#include <El/Exception.hpp>
#include <El/FileSystem.hpp>
#include <El/Guid.hpp>
#include <El/BinaryStream.hpp>
#include <El/Service/CompoundService.hpp>

#include <Commons/Message/Message.hpp>
#include <Commons/Message/TransportImpl.hpp>

#include <Services/Commons/FraudPrevention/FraudPreventionServices.hpp>
#include <Services/Commons/FraudPrevention/TransportImpl.hpp>
#include <Services/Commons/SearchMailing/SearchMailingServices.hpp>
#include <Services/Commons/SearchMailing/TransportImpl.hpp>

#include "SearchMailing.hpp"

namespace NewsGate
{
  namespace SearchMailing
  {
    //
    // MailManager class
    //
    MailManager::MailManager() throw(Exception, El::Exception)
    {
    }

    MailManager::MailManager(
      const char* mailer_ref,
      const char* limit_checker_ref,
      const FraudPrevention::EventLimitCheckDescArray&
      limit_check_descriptors,
      CORBA::ORB_ptr orb)
        throw(Exception, El::Exception)
        : limit_check_descriptors_(limit_check_descriptors)
    {
      try
      {
        Transport::register_valuetype_factories(orb);
        
        mailer_ = MailerRef(mailer_ref, orb);

        if(limit_checker_ref && *limit_checker_ref != '\0')
        {
          ::NewsGate::FraudPrevention::Transport::register_valuetype_factories(
            orb);
          
          limit_checker_ = LimitCheckerRef(limit_checker_ref, orb);
        }
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::MailManager::MailManager: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    }

    bool
    MailManager::check_limits(const char* email,
                              const char* user,
                              const char* ip,
                              FraudPrevention::EventType type)
      throw(Exception, El::Exception)
    {
      try
      {
        ::NewsGate::FraudPrevention::LimitChecker_var limit_checker =
          limit_checker_.object();

        FraudPrevention::Transport::EventLimitCheckPackImpl::Var
          check_pack =
          FraudPrevention::Transport::EventLimitCheckPackImpl::Init::create(
            new FraudPrevention::EventLimitCheckArray());
        
        FraudPrevention::EventLimitCheckArray& checks =
          check_pack->entities();
        
        for(FraudPrevention::EventLimitCheckDescArray::const_iterator
              i(limit_check_descriptors_.begin()),
              e(limit_check_descriptors_.end()); i != e; ++i)
        {
          const FraudPrevention::EventLimitCheckDesc& desc = *i;

          if(desc.type != type)
          {
            continue;
          }

          FraudPrevention::EventLimitCheck
            check((uint64_t)0, 1, desc.times, desc.interval);          
                
          check.update_event((const unsigned char*)&desc.type,
                             sizeof(desc.type));

          if(desc.user)
          {
            if(*user != '\0')
            {
              check.update_event((const unsigned char*)"U:", 2);
              check.update_event((const unsigned char*)user, strlen(user));
            }
            else
            {
            continue;
            }
          }
                  
          if(desc.ip)
          {
            check.update_event((const unsigned char*)"I:", 2);
            check.update_event((const unsigned char*)ip, strlen(ip));
          }

          if(desc.item)
          {
            if(*email != '\0')
            {
              check.update_event((const unsigned char*)"E:", 2);
              check.update_event((const unsigned char*)email, strlen(email));
            }
            else
            {
              continue;
            }
          }

          checks.push_back(check);
        }

        size_t check_count = checks.size();

        if(check_count)
        {
          FraudPrevention::Transport::EventLimitCheckResultPack_var res;

          try
          {
            res = limit_checker->check(
              FraudPrevention::LimitChecker::INTERFACE_VERSION,
              check_pack.in());
          }
          catch(const ::NewsGate::FraudPrevention::IncompatibleVersion&)
          {
            throw;
          }
          catch(const ::NewsGate::FraudPrevention::ImplementationException&)
          {
            throw;
          }
          catch(const CORBA::Exception& e)
          {
            // To catch-up if server restarted
            res = limit_checker->check(
              FraudPrevention::LimitChecker::INTERFACE_VERSION,
              check_pack.in());
          }
          
          FraudPrevention::Transport::EventLimitCheckResultPackImpl::Type*
            res_impl = dynamic_cast<FraudPrevention::Transport::
            EventLimitCheckResultPackImpl::Type*>(res.in());
          
          if(res_impl == 0)
          {
            std::ostringstream ostr;
            ostr << "NewsGate::SearchMailing::MailManager::"
              "check_limits: dynamic_cast<FraudPrevention::Transport::"
              "EventLimitCheckResultPackImpl::Type*> failed";

            throw Exception(ostr.str());
          }

          const FraudPrevention::EventLimitCheckResultArray& check_results =
            res_impl->entities();
            
          assert(check_results.size() == check_count);

          for(FraudPrevention::EventLimitCheckResultArray::const_iterator
                i(check_results.begin()), e(check_results.end()); i != e; ++i)
          {
            if(i->limit_exceeded)
            {
              return false;
            }
          }
        }

        return true;
      }
      catch(const ::NewsGate::FraudPrevention::IncompatibleVersion& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::MailManager::check_limits: "
          "::NewsGate::FraudPrevention::IncompartibleVersion "
          "caught (" << e.current_version << ")";
        
        throw Exception(ostr.str());
      }
      catch(const ::NewsGate::FraudPrevention::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::MailManager::check_limits: "
          "::NewsGate::FraudPrevention::ImplementationException "
          "caught. Description:\n" << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::MailManager::check_limits: "
          "CORBA::Exception. Description:\n" << e;
          
        throw Exception(ostr.str());
      }
    
    }
    
    MailManager::UpdatingSubscription
    MailManager::update_subscription(const Subscription& subs,
                                     bool is_human,
                                     std::string& new_session)
      throw(Exception, El::Exception)
    {
      try
      {
        if(!check_limits(subs.email.c_str(),
                         subs.user_id.c_str(),
                         subs.user_ip.c_str(),
                         subs.id == El::Guid::null ?
                         FraudPrevention::ET_ADD_SEARCH_MAIL :
                         FraudPrevention::ET_UPDATE_SEARCH_MAIL))
        {
          return US_LIMIT_EXCEEDED;
        }

        Transport::SubscriptionImpl::Var s =
          Transport::SubscriptionImpl::Init::create(new Subscription());

        s->entity() = subs;
        
        Mailer_var mailer = mailer_.object();
        
        try
        {
          CORBA::String_var new_sess = mailer->update_subscription(
            Mailer::INTERFACE_VERSION, s.in(), is_human);

          if(*new_sess.in() == '\0')
          {
            return US_YES;
          }
          else
          {
            new_session = new_sess.in();
            return US_MAILED;
          }
        }
        catch(const EmailChange&)
        {
          throw;
        }
        catch(const CheckHuman&)
        {
          throw;
        }
        catch(const NotFound&)
        {
          throw;
        }
        catch(const IncompatibleVersion& e)
        {
          throw;
        }
        catch(const ImplementationException& e)
        {
          throw;
        }
        catch(const CORBA::Exception& e)
        {
          // To catch-up if server restarted
          CORBA::String_var new_sess = mailer->update_subscription(
            Mailer::INTERFACE_VERSION, s.in(), is_human);
          
          if(*new_sess.in() == '\0')
          {
            return US_YES;
          }
          else
          {
            new_session = new_sess.in();
            return US_MAILED;
          }
        }
      }
      catch(const EmailChange&)
      {
        return US_EMAIL_CHANGE;
      }
      catch(const CheckHuman&)
      {
        return US_CHECK_HUMAN;
      }
      catch(const NotFound&)
      {
        return US_NOT_FOUND;
      }
      catch(const IncompatibleVersion& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::MailManager::update_subscription: "
          "SearchMailing::IncompartibleVersion caught ("
             << e.current_version << ")";
        
        throw Exception(ostr.str());
      }
      catch(const ImplementationException& e)
      {
        std::ostringstream ostr;        
        ostr << "NewsGate::SearchMailing::MailManager::update_subscription: "
          "SearchMailing::ImplementationException caught. "
          "Description:\n" << e.description.in();
                
        throw Exception(ostr.str());
      }       
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::MailManager::update_subscription: "
          "CORBA::Exception caught. Description:\n" << e;
          
        throw Exception(ostr.str());
      }
    }

    MailManager::ConfirmingOperation
    MailManager::confirm_operation(const Confirmation& conf,
                                   std::string& email,
                                   std::string& session)
      throw(Exception, El::Exception)
    {
      try
      {
        Mailer_var mailer = mailer_.object();

        try
        {
          CORBA::String_var e;
          CORBA::String_var s = mailer->confirm_operation(conf.token.c_str(),
                                                          conf.user.c_str(),
                                                          conf.ip.c_str(),
                                                          conf.agent.c_str(),
                                                          e.out());
          email = e.in();
          session = s.in();
          return session.empty() ? CO_NOT_FOUND : CO_YES;
        }
        catch(const EmailChange&)
        {
          throw;
        }
        catch(const ImplementationException& e)
        {
          throw;
        }
        catch(const CORBA::Exception& e)
        {
          // To catch-up if server restarted
          
          CORBA::String_var e;
          CORBA::String_var s = mailer->confirm_operation(conf.token.c_str(),
                                                          conf.user.c_str(),
                                                          conf.ip.c_str(),
                                                          conf.agent.c_str(),
                                                          e.out());
          email = e.in();
          session = s.in();
          return session.empty() ? CO_NOT_FOUND : CO_YES;
        }
      }
      catch(const EmailChange& e)
      {
        email = e.email.in();
        session = e.session.in();
        return CO_EMAIL_CHANGE;
      }
      catch(const ImplementationException& e)
      {
        std::ostringstream ostr;        
        ostr << "NewsGate::SearchMailing::MailManager::"
          "confirm_operation(" << conf.token << ", " << conf.user
             << "): SearchMailing::ImplementationException caught. "
          "Description:\n" << e.description.in();

        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::MailManager::"
          "confirm_operation(" << conf.token << ", " << conf.user
             << "): CORBA::Exception caught. Description:\n" << e;
                
        throw Exception(ostr.str());
      }
    }

    Subscription*
    MailManager::get_subscription(const El::Guid& id)
      throw(Exception, El::Exception)
    {
      std::string subs_id = id.string(El::Guid::GF_DENSE);
      
      try
      {        
        Mailer_var mailer = mailer_.object();
        
        Transport::Subscription* subs = 0;

        try
        {
          subs = mailer->get_subscription(Mailer::INTERFACE_VERSION,
                                          subs_id.c_str());
        }
        catch(const SearchMailing::NotFound&)
        {
          throw;
        }
        catch(const IncompatibleVersion& e)
        {
          throw;
        }
        catch(const ImplementationException& e)
        {
          throw;
        }
        catch(const CORBA::Exception& e)
        {
          // To catch-up if server restarted
          subs = mailer->get_subscription(Mailer::INTERFACE_VERSION,
                                          subs_id.c_str());
        }

        Transport::SubscriptionImpl::Type* s =
          dynamic_cast<Transport::SubscriptionImpl::Type*>(subs);

        if(s == 0)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::SearchMailing::MailManager::get_subscription("
             << subs_id << "): dynamic_cast<Transport::SubscriptionImpl::"
            "Type*> failed";

          throw Exception(ostr.str());
        }

        return s->release();
      }
      catch(const SearchMailing::NotFound&)
      {
        return 0;
      }
      catch(const IncompatibleVersion& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::MailManager::get_subscription("
             << subs_id << "): SearchMailing::IncompatibleVersion caught. "
          "Version: " << e.current_version;
          
        throw Exception(ostr.str());
      }
      catch(const ImplementationException& e)
      {
        std::ostringstream ostr;        
        ostr << "NewsGate::SearchMailing::MailManager::get_subscription("
             << subs_id << "): SearchMailing::ImplementationException caught. "
          "Description:\n" << e.description.in();

        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::MailManager::get_subscription("
             << subs_id << "): CORBA::Exception caught. Description:\n" << e;
                
        throw Exception(ostr.str());
      }
    }

    SubscriptionArray*
    MailManager::get_subscriptions(const char* email,
                                   const El::Lang& lang,
                                   const char* token,
                                   const char* user,
                                   const char* ip,
                                   const char* agent,
                                   std::string& session,
                                   bool is_human)
      throw(SearchMailing::CheckHuman, LimitExceeded, Exception, El::Exception)
    {
      if(!check_limits(email, user, ip, FraudPrevention::ET_UPDATE_SEARCH_MAIL))
      {
        throw LimitExceeded("");
      }
      
      try
      {
        Transport::SubscriptionPack_var pack;
        Mailer_var mailer = mailer_.object();

        CORBA::String_var sess = session.c_str();

        try
        {
          pack = mailer->get_subscriptions(Mailer::INTERFACE_VERSION,
                                           email,
                                           lang.l3_code(true),
                                           token,
                                           user,
                                           ip,
                                           agent,
                                           sess.inout(),
                                           is_human);

          session = sess.in();
        }
        catch(const CheckHuman&)
        {
          throw;
        }
        catch(const NeedConfirmation&)
        {
          throw;
        }
        catch(const NotFound&)
        {
          throw;
        }
        catch(const IncompatibleVersion& e)
        {
          throw;
        }
        catch(const ImplementationException& e)
        {
          throw;
        }
        catch(const CORBA::Exception& e)
        {
          CORBA::String_var sess = session.c_str();
          
          // To catch-up if server restarted
          pack = mailer->get_subscriptions(Mailer::INTERFACE_VERSION,
                                           email,
                                           lang.l3_code(true),
                                           token,
                                           user,
                                           ip,
                                           agent,
                                           sess.inout(),
                                           is_human);
          
          session = sess.in();
        }

        Transport::SubscriptionPackImpl::Type*
          pack_impl = dynamic_cast<Transport::SubscriptionPackImpl::Type*>(
            pack.in());

        if(pack_impl == 0)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::SearchMailing::MailManager::get_subscriptions: "
            "dynamic_cast<Transport::SubscriptionPackImpl::Type*> failed";
          
          throw Exception(ostr.str());
        }
        
        return pack_impl->release();
      }
      catch(const CheckHuman&)
      {
        throw;
      }
      catch(const NeedConfirmation& e)
      {
        session = e.session.in();
        return 0;
      }
      catch(const NotFound&)
      {
        return 0;
      }
      catch(const IncompatibleVersion& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::MailManager::get_subscriptions: "
          "SearchMailing::IncompartibleVersion caught ("
             << e.current_version << ")";
        
        throw Exception(ostr.str());
      }
      catch(const ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::MailManager::get_subscriptions: "
          "SearchMailing::ImplementationException caught. Description:"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::MailManager::get_subscriptions: "
          "CORBA::Exception caught. Description:\n" << e;
                
        throw Exception(ostr.str());
      }
    }
    
    MailManager::EnablingSubscription
    MailManager::enable_subscription(const char* email,
                                     const El::Guid& id,
                                     Subscription::SubsStatus status,
                                     const char* user,
                                     const char* ip,
                                     const char* agent,
                                     std::string& session,
                                     bool is_human)
      throw(Exception, El::Exception)
    {
      if(!check_limits("", user, ip, FraudPrevention::ET_UPDATE_SEARCH_MAIL))
      {
        return ES_LIMIT_EXCEEDED;
      }
      
      std::string subs_id = id.string(El::Guid::GF_DENSE);
      
      try
      {
        Mailer_var mailer = mailer_.object();

        try
        {
          CORBA::String_var sess = session.c_str();
          
          CORBA::Boolean res = mailer->enable_subscription(email,
                                                           subs_id.c_str(),
                                                           status,
                                                           user,
                                                           ip,
                                                           agent,
                                                           sess.inout(),
                                                           is_human);
          session = sess.in();
          
          if(res)
          {
            return ES_YES;
          }
          else
          {
            return ES_MAILED;
          }          
        }
        catch(const SearchMailing::CheckHuman&)
        {
          throw;
        }
        catch(const SearchMailing::NoOp&)
        {
          throw;
        }
        catch(const SearchMailing::NotFound&)
        {
          throw;
        }
        catch(const ImplementationException& e)
        {
          throw;
        }
        catch(const CORBA::Exception& e)
        {
          // To catch-up if server restarted
          
          CORBA::String_var sess = session.c_str();
          
          CORBA::Boolean res = mailer->enable_subscription(email,
                                                           subs_id.c_str(),
                                                           status,
                                                           user,
                                                           ip,
                                                           agent,
                                                           sess.inout(),
                                                           is_human);
          session = sess.in();
          
          if(res)
          {
            return ES_YES;
          }
          else
          {
            return ES_MAILED;
          }          
        }
      }
      catch(const CheckHuman&)
      {
        return ES_CHECK_HUMAN;
      }
      catch(const NoOp&)
      {
        return ES_ALREADY;
      }
      catch(const NotFound&)
      {
        return ES_NOT_FOUND;
      }
      catch(const ImplementationException& e)
      {
        std::ostringstream ostr;        
        ostr << "NewsGate::SearchMailing::MailManager::enable_subscription("
             << subs_id << ", " << status
             << "): SearchMailing::ImplementationException caught. "
          "Description:\n" << e.description.in();

        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::MailManager::enable_subscription("
             << subs_id << ", " << status
             << "): CORBA::Exception caught. Description:\n" << e;
                
        throw Exception(ostr.str());
      }
    }
  }
}
