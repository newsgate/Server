
/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Message/BankManager/DispositionVerification.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/Logging/Logger.hpp>
#include <El/Service/CompoundService.hpp>
#include <El/Service/Service.hpp>
#include <El/MySQL/DB.hpp>

#include <Commons/Message/TransportImpl.hpp>
#include <Commons/Message/FetchFilter.hpp>
#include <Commons/Message/Categorizer.hpp>

#include <Services/Commons/Message/BankSessionImpl.hpp>
#include <Services/Commons/Message/BankClientSessionImpl.hpp>
#include <Services/Commons/Category/WordListResolver.hpp>

#include "BankManagerMain.hpp"
#include "SessionSupport.hpp"
#include "DispositionVerification.hpp"
#include "DB_Record.hpp"

namespace NewsGate
{
  namespace Message
  {
    const Categorizer::Category::Id ROOT_CATEGORY_ID = 1;
    
    //
    // DispositionVerification class
    //
    DispositionVerification::DispositionVerification(
      const BankDisposition& disposition,
      const ACE_Time_Value& presence_poll_timeout,
      const ACE_Time_Value& presence_check_period,
      const Server::Config::MessageBankManagerType::message_sharing_type&
        message_sharing_conf,
      const Server::Config::MessageManagementType& message_management_conf,
      BankManagerStateCallback* callback)
      throw(Exception, El::Exception)
        : BankManagerState(callback, "DispositionVerification"),
          disposition_(disposition),
          presence_poll_timeout_(presence_poll_timeout),
          presence_check_period_(presence_check_period),
          message_sharing_conf_(message_sharing_conf),
          message_management_conf_(message_management_conf),
          message_sharing_managers_discovered_(false),
          message_fetch_filter_stamp_(0),
          message_categorizer_stamp_(0)
    {
      own_colo_frontend_.process_id = callback_->get_process_id();

      typedef Server::Config::MessageBankManagerType::
        bank_client_management_type::frontend_sequence FrontendSeq;
      
      const FrontendSeq& conf_frontends =
        Application::instance()->config().bank_client_management().frontend();

      for(FrontendSeq::const_iterator i(conf_frontends.begin()),
            e(conf_frontends.end()); i != e; ++i)
      {
        Transport::EndpointArray& endpoints =
          std::string(i->type()) == "limited" ?
          own_colo_frontend_.limited_endpoints :
          own_colo_frontend_.search_endpoints;
        
        endpoints.push_back(i->endpoint());
      }
      
      El::Service::CompoundServiceMessage_var msg;

      if(Application::instance()->message_managing_dbase())
      {
        if(message_management_conf_.filter().present())
        {
          msg = new CheckMessageFetchFilter(this);
          deliver_now(msg.in());
        }
        
        if(message_management_conf_.categories().present())
        {
          msg = new CheckMessageCategorizer(this);
          deliver_now(msg.in());
        }
      }
      
      msg = new MessageSharingRegisterSelf(this);
      deliver_now(msg.in());

      msg = new CheckBanksPresence(this);
      
      deliver_at_time(msg.in(),
                      ACE_OS::gettimeofday() + presence_check_period_);
    }
    
    DispositionVerification::~DispositionVerification() throw()
    {
//      std::cerr << "Deleted\n";
    }

    void
    DispositionVerification::disposition_breakdown() throw()
    {
/*      
      {
        WriteGuard guard(srv_lock_);

        disposition_.clear();
        sharing_sink_map_.clear();
        message_sharing_managers_.clear();
      }
*/
      
      callback_->disposition_breakdown();      
    }
      
    ::NewsGate::Message::BankSession*
    DispositionVerification::bank_login(
      const char* bank_ior,
      ::NewsGate::Message::BankSessionId* current_session_id)
      throw(NewsGate::Message::NotReady,
            NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      if(started())
      {
        disposition_breakdown();
      }

      NewsGate::Message::NotReady e;

      e.reason = CORBA::string_dup(
        "NewsGate::Message::DispositionVerification::bank_login: "
        "quite unexpected");

      if(Application::will_trace(El::Logging::MIDDLE))
      {
        std::ostringstream ostr;
        ostr << e.reason.in() << "; bank " << bank_ior;
        
        Application::logger()->trace(ostr.str(),
                                     Aspect::STATE,
                                     El::Logging::MIDDLE);
      }
              
      throw e;
    }

    void
    DispositionVerification::ping(
      const char* bank_ior,
      ::NewsGate::Message::BankSessionId* current_session_id)
      throw(NewsGate::Message::Logout,
            NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      if(!started())
      {
        return;
      }

      BankSessionIdImpl* session_id =
        dynamic_cast<BankSessionIdImpl*>(current_session_id);

      if(session_id == 0)
      {
        NewsGate::Message::ImplementationException e;
        
        e.description = CORBA::string_dup(
          "NewsGate::Message::DispositionVerification::ping: "
          "dynamic_cast failed");

        throw e;
      }
      
      {
        WriteGuard guard(srv_lock_);

        if(session_id->index >= disposition_.size() ||
           session_id->banks_count != disposition_.size() ||
           disposition_[session_id->index].bank.reference() != bank_ior)
        {
          guard.release();
          
          NewsGate::Message::Logout e;
      
          e.reason = CORBA::string_dup("NewsGate::Message::"
                                       "DispositionVerification::ping: "
                                       "wrong session id");
              
          if(Application::will_trace(El::Logging::MIDDLE))
          {
            std::ostringstream ostr;
            ostr << e.reason.in() << "; bank " << bank_ior;
        
            Application::logger()->trace(ostr.str(),
                                         Aspect::STATE,
                                         El::Logging::MIDDLE);
          }
          
          disposition_breakdown();
          throw e;
        }

        BankInfo& bi = disposition_[session_id->index];
        
        if(Application::will_trace(El::Logging::HIGH))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::DispositionVerification::ping: "
               << (bi.active ? "active" : "inactive")
               << " bank " << bank_ior << " - timestamp updated";
          
          Application::logger()->trace(ostr.str(),
                                       Aspect::STATE,
                                       El::Logging::HIGH);
        }
        
        bi.active = true;
        bi.timestamp = ACE_OS::gettimeofday();
      }
      
    }
    
    void
    DispositionVerification::terminate(
      const char* bank_ior,
      ::NewsGate::Message::BankSessionId* current_session_id)
      throw(NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      if(!started())
      {
        return;
      }

      BankSessionIdImpl* session_id =
        dynamic_cast<BankSessionIdImpl*>(current_session_id);

      if(session_id == 0)
      {
        NewsGate::Message::ImplementationException e;
        
        e.description = CORBA::string_dup(
          "NewsGate::Message::DispositionVerification::terminate: "
          "dynamic_cast failed");

        throw e;
      }
      
      WriteGuard guard(srv_lock_);

      if(session_id->index >= disposition_.size() ||
         session_id->banks_count != disposition_.size() ||
         disposition_[session_id->index].bank.reference() != bank_ior)
      {
        return;
      }

      BankInfo& bi = disposition_[session_id->index];
        
      if(Application::will_trace(El::Logging::HIGH))
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::DispositionVerification::terminate: "
             << (bi.active ? "active" : "inactive")
             << " bank " << bank_ior << " reported termination";
        
        Application::logger()->trace(ostr.str(),
                                     Aspect::STATE,
                                     El::Logging::HIGH);
      }
        
      bi.active = false;
    }
      
    ::NewsGate::Message::BankClientSession*
    DispositionVerification::bank_client_session()
      throw(NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      try
      {
        return create_client_session(false);
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::DispositionVerification::"
          "bank_client_session: El::Exception caught. Description:\n"
             << e.what();

        NewsGate::Message::ImplementationException ex;
        ex.description = CORBA::string_dup(ostr.str().c_str());
        throw ex;
      }
      
    }

    ::NewsGate::Message::BankClientSession*
    DispositionVerification::create_client_session(bool invalidate_inactive)
      throw(CORBA::SystemException, El::Exception)
    {
      BankManagerApp* app = Application::instance();
      
      const Server::Config::MessageBankManagerType::
        bank_client_management_type&
        conf = app->config().bank_client_management();

      BankClientSessionImpl_var session = BankClientSessionImpl_init::create(
        app->orb(),
        app->bank_manager_corba_ref(),
        callback_->get_process_id(),
        ACE_Time_Value(conf.session_refresh_period()),
        ACE_Time_Value(conf.bank_invalidate_timeout()),
        conf.message_post_retries(),
        conf.request_threads());

      BankClientSessionImpl::BankRecordArray& banks = session->banks();
      
      banks.resize(disposition_.size());

      size_t i = 0;
      ACE_Time_Value cur_time = ACE_OS::gettimeofday();
      
      for(BankDisposition::const_iterator it = disposition_.begin();
          it != disposition_.end(); it++, i++)
      {
        banks[i].bank = it->bank;

        if(invalidate_inactive && !it->active)
        {
          banks[i].invalidated = cur_time;
        }
      }

      Transport::ColoFrontendArray& colo_frontends = session->colo_frontends();
      Transport::ColoFrontend::append(colo_frontends, own_colo_frontend_);

      ReadGuard guard(srv_lock_);

      for(SharingSinkMap::const_iterator i(sharing_sink_map_.begin()),
            e(sharing_sink_map_.end()); i != e; ++i)
      {
        const Transport::ColoFrontendArray& mirrors =
          i->second.colo_frontends;

        for(Transport::ColoFrontendArray::const_iterator i(mirrors.begin()),
              e(mirrors.end()); i != e; ++i)
        {
          Transport::ColoFrontend::append(colo_frontends, *i);
        }        
      }

      Transport::ColoFrontend::append(colo_frontends, master_colo_frontend_);
      
      return session._retn();
    }

    bool
    DispositionVerification::discover_message_sharing_managers()
      throw(El::Exception)
    {
      if(message_sharing_managers_discovered_)
      {
        return true;
      }
      
      BankManagerApp* app = Application::instance();
      
      typedef Server::Config::MessageBankManagerType::message_sharing_type::
        bank_manager_sequence MessageSharingManagerConf;
      
      const MessageSharingManagerConf& config =
        message_sharing_conf_.bank_manager();

      bool self_found = false;
      bool categorizer = false;

      MessageSharingManagerList message_sharing_managers;
      
      for(MessageSharingManagerConf::const_iterator it = config.begin();
          it != config.end(); it++)
      {
        El::Corba::SmartRef<NewsGate::Message::BankManager>
          bank_manager_ref(it->ref().c_str(), app->orb());

        try
        {
          BankManager_var reference;
          CORBA::String_var process_id;

          try
          {
            reference = bank_manager_ref.object();
            process_id = reference->process_id();
          }
          catch(const CORBA::Exception& e)
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Message::DispositionVerification::"
              "discover_message_sharing_managers: getting process_id failed "
              "for " << bank_manager_ref.reference()
                 << ", CORBA::Exception caught. Description:\n" << e;
            
            El::Service::Error error(ostr.str(), this);
            callback_->notify(&error);
            
            return false;
          }

          if(strcmp(process_id.in(), callback_->get_process_id()) == 0)
          {
            if(self_found)
            {
              std::ostringstream ostr;
              ostr << "NewsGate::Message::DispositionVerification::"
                "discover_message_sharing_managers: found self TWICE "
                "in message sharing manager list. Bad process_id "
                "generation algorithm.";

              El::Service::Error error(ostr.str(), this);
              callback_->notify(&error);

              return false;
            }
            
            self_found = true;
                
            if(Application::will_trace(El::Logging::MIDDLE))
            {
              std::ostringstream ostr;
              ostr << "NewsGate::Message::DispositionVerification::"
                "discover_message_sharing_managers: found self in message "
                "sharing manager list. Skipping ...";
                  
              Application::logger()->trace(ostr.str(),
                                           Aspect::MSG_SHARING,
                                           El::Logging::MIDDLE);
            }

            continue;
          }

          MessageSharingManager manager;
          manager.reference = reference;
          manager.ior = bank_manager_ref.reference();

          if(it->new_messages())
          {
            manager.flags |= MSRT_NEW_MESSAGES;
          }
        
          if(it->shared_messages())
          {
            manager.flags |= MSRT_SHARED_MESSAGES;
          }
        
          if(it->old_messages())
          {
            manager.flags |= MSRT_OLD_MESSAGES;
          }
        
          if(it->own_events())
          {
            manager.flags |= MSRT_OWN_EVENTS;
          }
        
          if(it->shared_events())
          {
            manager.flags |= MSRT_SHARED_EVENTS;
          }
        
          if(it->own_message_filter())
          {
            manager.flags |= MSRT_OWN_MESSAGE_FILTER;
          }
        
          if(it->shared_message_filter())
          {
            manager.flags |= MSRT_SHARED_MESSAGE_FILTER;
          }
        
          if(it->own_message_stat())
          {
            manager.flags |= MSRT_OWN_MESSAGE_STAT;
          }
        
          if(it->shared_message_stat())
          {
            manager.flags |= MSRT_SHARED_MESSAGE_STAT;
          }

          if(it->message_categorizer())
          {
            if(categorizer)
            {
              std::ostringstream ostr;
              ostr << "NewsGate::Message::DispositionVerification::"
                "discover_message_sharing_managers: categorizer can be "
                "received from single source only; ignored for "
                   << it->ref();

              El::Service::Error error(ostr.str(), this);
              callback_->notify(&error);
            }
            else
            {
              categorizer = true;
              manager.flags |= MSRT_MESSAGE_CATEGORIZER;
            }
          }
        
          message_sharing_managers.push_back(manager);
        }
        catch(const CORBA::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::DispositionVerification::"
            "discover_message_sharing_managers: while dealing with manager "
               << bank_manager_ref.reference()
               << " CORBA::Exception caught. Description:\n" << e;
        
          throw Exception(ostr.str());
        }
      }

      message_sharing_managers_ = message_sharing_managers;
      message_sharing_managers_discovered_ = true;
      
      return true;
    }

    void
    DispositionVerification::message_sharing_register_self() throw()
    {
      if(!started())
      {
        return;
      }

      try
      {
        if(!discover_message_sharing_managers())
        {
          El::Service::CompoundServiceMessage_var msg =
            new MessageSharingRegisterSelf(this);
        
          deliver_at_time(
            msg.in(),
            ACE_OS::gettimeofday() +
            ACE_Time_Value(message_sharing_conf_.registration_period()));

          return;
        }
        
        ::NewsGate::Message::BankClientSession_var client_session;
        
        try
        {
          client_session = create_client_session(true);
        }
        catch(const CORBA::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "DispositionVerification::"
            "message_sharing_register_self: "
               << "CORBA::Exception thrown by create_client_session. "
            "Description:\n" << e;

          throw Exception(ostr.str());
        }

        time_t expiration_time =
          ACE_OS::gettimeofday().sec() +
          message_sharing_conf_.registration_timeout();

        const char* bank_manager_id =
          Application::instance()->bank_manager_id();

        SharingIdSetPtr sharing_ids(new Message::Transport::SharingIdSet());
        size_t registrations = 0;
        size_t weak_registrations = 0;

        Transport::ColoFrontendPackImpl::Var colo_frontends_pack_impl =
          new Transport::ColoFrontendPackImpl::Type(
            new Transport::ColoFrontendArray());

        Transport::ColoFrontendPack_var colo_frontends_pack;
        bool proxy = message_sharing_conf_.proxy();
        
        if(proxy)
        {
          colo_frontends_pack_impl->entities().push_back(own_colo_frontend_);
        }

        colo_frontends_pack_impl->_add_ref();
        colo_frontends_pack = colo_frontends_pack_impl.in();

        std::string mirror = message_sharing_conf_.mirror();
        
        for(MessageSharingManagerList::iterator
              it = message_sharing_managers_.begin();
            it != message_sharing_managers_.end(); it++)
        {
          MessageSharingManager& manager = *it;
          El::Service::Error::Severity severity = El::Service::Error::CRITICAL;

          std::string error;

          try
          {
            CORBA::String_var sharing_id =
              manager.reference->message_sharing_register(
                NewsGate::Message::BankManager::SHARING_INTERFACE_VERSION,
                bank_manager_id,
                client_session.in(),
                mirror == "none" ? 0 : manager.flags,
                expiration_time,
                colo_frontends_pack.inout());

            if(proxy)
            {
              Transport::ColoFrontendPackImpl::Type* colo_frontends =
                dynamic_cast<Transport::ColoFrontendPackImpl::Type*>(
                  colo_frontends_pack.in());
              
              if(colo_frontends == 0)
              {
                throw Exception("dynamic_cast<Transport::"
                                "ColoFrontendPackImpl::Type*> failed");
              }

              const Transport::ColoFrontendArray& cf =
                colo_frontends->entities();

              if(cf.empty())
              {
                master_colo_frontend_.clear();
              }
              else
              {
                master_colo_frontend_ = cf[0];
                
                master_colo_frontend_.relation =
                  Transport::ColoFrontend::RL_MASTER;
              }
            }

            if(*sharing_id.in() == '\0')
            {
              // Happens when message protocol differes
              // (so no message exchange will happen),
              // but enlisted colo frontends are registered for usage at
              // HTTP level delegations
              ++weak_registrations;
            }
            else
            {
              sharing_ids->insert(sharing_id.in());
              ++registrations;
            }
            
            continue;
          }
          catch(const NewsGate::Message::IncompartibleInterface& e)
          {
            std::ostringstream ostr;
            ostr << "DispositionVerification::"
              "message_sharing_register_self: "
              "NewsGate::Message::IncompartibleInterface "
              "thrown by message_sharing_register. Reason:\n" << e.reason.in();

            error = ostr.str();
            severity = El::Service::Error::WARNING;
          }
          catch(const NewsGate::Message::NotReady& e)
          {
            std::ostringstream ostr;
            ostr << "DispositionVerification::"
              "message_sharing_register_self: NewsGate::Message::NotReady "
              "thrown by message_sharing_register. Reason:\n" << e.reason.in();

            error = ostr.str();
            severity = El::Service::Error::WARNING;
          }
          catch(const NewsGate::Message::ImplementationException& e)
          {
            std::ostringstream ostr;
            ostr << "DispositionVerification::message_sharing_register_self: "
                 << "NewsGate::Message::ImplementationException thrown by "
              "message_sharing_register. Description:\n" << e.description.in();

            error = ostr.str();
          }
          catch(const CORBA::Exception& e)
          {
            std::ostringstream ostr;
            ostr << "DispositionVerification::message_sharing_register_self: "
                 << "CORBA::Exception thrown by message_sharing_register. "
              "Description:\n" << e;

            error = ostr.str();
            severity = El::Service::Error::WARNING;
          }

          El::Service::Error err(error, this, severity);
          callback_->notify(&err);
        }

        if(Application::will_trace(El::Logging::MIDDLE))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::DispositionVerification::"
            "message_sharing_register_self: mirror type " <<
            mirror << ", proxying " << proxy << std::endl
               << message_sharing_managers_.size()
               << " bank managers\n" << registrations
               << " registrations\n" << weak_registrations
               << " weak registrations";
            
          Application::logger()->trace(ostr.str(),
                                       Aspect::MSG_SHARING,
                                       El::Logging::MIDDLE);
        }
        
        try
        {
          std::auto_ptr<Transport::MessageSharingSources>
            sharing_sources(new Transport::MessageSharingSources());

          sharing_sources->ids = *sharing_ids;

          if(mirror == "absolute" && !sharing_sources->ids.empty())
          {
            assert(message_sharing_managers_.size() == 1);
            assert(sharing_sources->ids.size() == 1);

            sharing_sources->mirrored_manager =
              message_sharing_managers_.begin()->ior;
          }
          
          Transport::SetMessageSharingSourcesRequestImpl::Var
            request = Transport::
            SetMessageSharingSourcesRequestImpl::Init::create(
              sharing_sources.release());

          Message::Transport::Response_var response;
          
          Message::BankClientSession::RequestResult_var result =
            client_session->send_request(request, response.out());

          if(result->code != Message::BankClientSession::RRC_OK)
          { 
            std::ostringstream ostr;
            ostr << "NewsGate::Message::DispositionVerification::"
              "message_sharing_register_self: send_request result code "
                 << result->code << ", description: "
                 << result->description.in();
            
            if(result->code == Message::BankClientSession::RRC_NOT_READY)
            {
              Application::logger()->trace(ostr.str(),
                                           Aspect::MSG_SHARING,
                                           El::Logging::LOW);
            }
            else
            {
              Application::logger()->emergency(ostr.str(),
                                               Aspect::MSG_SHARING);
            }
          }
        }
        catch(const Message::ImplementationException& e)
        {   
          std::ostringstream ostr;
          
          ostr << "NewsGate::Message::DispositionVerification::"
            "message_sharing_register_self: Message::ImplementationException "
            "caught. Description:\n" << e.description.in();
          
          Application::logger()->emergency(ostr.str(),
                                           Aspect::MSG_CATEGORIZER_DIST);
        }
        catch(const CORBA::Exception& e)
        {   
          std::ostringstream ostr;
          
          ostr << "NewsGate::Message::DispositionVerification::"
            "message_sharing_register_self: CORBA::Exception caught. "
            "Description:\n" << e;
          
          Application::logger()->emergency(ostr.str(),
                                           Aspect::MSG_CATEGORIZER_DIST);
        }

        {
          WriteGuard guard(srv_lock_);
          
          sharing_ids_.reset(sharing_ids.release());
        }

        El::Service::CompoundServiceMessage_var msg =
          new MessageSharingRegisterSelf(this);
        
        deliver_at_time(
          msg.in(),
          ACE_OS::gettimeofday() +
          ACE_Time_Value(message_sharing_conf_.registration_period()));        
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "DispositionVerification::message_sharing_register_self: "
            "El::Exception caught. Description:" << std::endl << e;

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(...)
        {
          El::Service::Error error(
            "DispositionVerification::message_sharing_register_self: "
            "unknown caught.",
            this);
          
          callback_->notify(&error);
        }
      } 
      
    }
    
    void
    DispositionVerification::message_sharing_register_foreign_client(
      CORBA::ULong interface_version,
      const char* manager_persistent_id,
      ::NewsGate::Message::BankClientSession* message_sink,
      CORBA::ULong flags,
      CORBA::ULong expiration_time,
      Transport::ColoFrontendArray* colo_frontends)
      throw()
    {
      if(!started())
      {
        return;
      }

      bool interface_matches = interface_version ==
        NewsGate::Message::BankManager::SHARING_INTERFACE_VERSION;
      
      try
      {
        std::string action(
          message_sink == 0 ? "unregistering" : "registering");
        
        if(Application::will_trace(El::Logging::MIDDLE))
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::DispositionVerification::"
            "message_sharing_register_foreign_client: " << action
               << " manager " << manager_persistent_id << "; interface "
               << (interface_matches ? "" : "not ") << " matches";
            
          Application::logger()->trace(ostr.str(),
                                       Aspect::MSG_SHARING,
                                       El::Logging::MIDDLE);
        }

        for(BankDisposition::const_iterator it = disposition_.begin();
            it != disposition_.end(); it++)
        {
          const BankInfo& bi = *it;

          if(!bi.active)
          {
            continue;
          }

          std::string error;
          El::Service::Error::Severity severity = El::Service::Error::CRITICAL;
          
          try
          {
            NewsGate::Message::Bank_var bank = bi.bank.object();
            
            if(message_sink == 0)
            {
              bank->message_sharing_unregister(manager_persistent_id);
            }
            else if(interface_matches)
            {
              bank->message_sharing_register(manager_persistent_id,
                                             message_sink,
                                             flags,
                                             expiration_time);
            }
          }
          catch(const NewsGate::Message::NotReady& e)
          {
            std::ostringstream ostr;
            ostr << "DispositionVerification::"
              "message_sharing_register_foreign_client: "
                 << "NewsGate::Message::NotReady caught while " << action
                 << ". Reason:\n" << e.reason.in();

            error = ostr.str();
            severity = El::Service::Error::WARNING;
          }
          catch(const NewsGate::Message::ImplementationException& e)
          {
            std::ostringstream ostr;
            ostr << "DispositionVerification::"
              "message_sharing_register_foreign_client: "
                 << "NewsGate::Message::ImplementationException caught while "
                 << action << ". Description:\n" << e.description.in();

            error = ostr.str();
          }
          catch(const CORBA::Exception& e)
          {
            std::ostringstream ostr;
            ostr << "DispositionVerification::"
              "message_sharing_register_foreign_client: "
              "CORBA::Exception caught while " << action
                 << ". Description:\n" << e;

            error = ostr.str();
          }

          if(!error.empty())
          {
            El::Service::Error err(error, this, severity);
            callback_->notify(&err);
          }
        }

        clean_sinks();
        
        if(message_sink)
        {
          assert(colo_frontends != 0);
          
          SharingSink ms;
        
          ms.manager = message_sink->bank_manager();
          ms.expiration_time = ACE_Time_Value(expiration_time);
          ms.flags = interface_matches ? flags : 0;
          ms.colo_frontends = *colo_frontends;

          for(Transport::ColoFrontendArray::iterator
                i(ms.colo_frontends.begin()), e(ms.colo_frontends.end());
              i != e; ++i)
          {
            i->relation = Transport::ColoFrontend::RL_MIRROR;
          }
          
          WriteGuard guard(srv_lock_);
          
          if(sharing_sink_map_.find(manager_persistent_id) ==
             sharing_sink_map_.end())
          {
            if(ms.flags & MSRT_MESSAGE_CATEGORIZER)
            {
              message_categorizer_resend_time_ = ACE_Time_Value::zero;
            }

            if(ms.flags & MSRT_OWN_MESSAGE_FILTER)
            {
              message_fetch_filter_resend_time_ = ACE_Time_Value::zero;
            }
          }
            
          sharing_sink_map_[manager_persistent_id] = ms;
        }
        else
        {
          WriteGuard guard(srv_lock_);
          sharing_sink_map_.erase(manager_persistent_id);
        }
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "DispositionVerification::"
            "message_sharing_register_foreign_client: "
            "El::Exception caught. Description:" << std::endl << e;

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(...)
        {
          El::Service::Error error("DispositionVerification::"
                                   "message_sharing_register_foreign_client: "
                                   "unknown caught.",
                                   this);
          
          callback_->notify(&error);
        }
      } 
    }
    
    void
    DispositionVerification::clean_sinks() throw(Exception, El::Exception)
    {
      ACE_Time_Value cur_time = ACE_OS::gettimeofday();
      
      WriteGuard guard(srv_lock_);

      for(SharingSinkMap::iterator it = sharing_sink_map_.begin();
          it != sharing_sink_map_.end(); )
      {
        if(it->second.expiration_time > cur_time)
        {
          it++;
        }
        else
        {
          SharingSinkMap::iterator cur = it++;
          sharing_sink_map_.erase(cur);
        }
      }
    }

    bool
    DispositionVerification::check_validation_id(const char* validation_id)
      const throw()
    {
      ReadGuard guard(srv_lock_);
          
      return sharing_ids_.get() != 0 &&
        sharing_ids_->find(validation_id) != sharing_ids_->end();
    }
    
    void
    DispositionVerification::set_message_fetch_filter(
      ::NewsGate::Message::Transport::SetMessageFetchFilterRequest* filter,
      ::CORBA::Boolean shared,
        const char* validation_id)
      throw(NewsGate::Message::ImplementationException,
            ::CORBA::SystemException)
    {
      if(check_validation_id(validation_id))
      {
        El::Service::CompoundServiceMessage_var msg =
          new AcceptMessageFetchFilter(this, filter, shared);
      
        deliver_now(msg.in());
      }
    }
    
    void
    DispositionVerification::set_message_categorizer(
      ::NewsGate::Message::Transport::SetMessageCategorizerRequest*
      categorizer,
      const char* validation_id)
      throw(NewsGate::Message::ImplementationException,
            ::CORBA::SystemException)
    {
      if(check_validation_id(validation_id))
      {
        El::Service::CompoundServiceMessage_var msg =
          new AcceptMessageCategorizer(this, categorizer);
        
        deliver_now(msg.in());
      }
    }

    void
    DispositionVerification::accept_message_fetch_filter(
      ::NewsGate::Message::Transport::SetMessageFetchFilterRequest* filter,
      bool shared) throw()
    {
      if(!started())
      {
        return;
      }

      try
      {
        Transport::SetMessageFetchFilterRequestImpl::Type*
          request =
          dynamic_cast<Transport::SetMessageFetchFilterRequestImpl::Type*>(
            filter);

        if(request == 0)
        {
          throw Exception(
            "DispositionVerification::accept_message_fetch_filter: "
            "dynamic_cast<Transport::SetMessageFetchFilterRequestImpl::Type*>"
            " failed");
        }

        propagate_message_fetch_filter(request, true);
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        
        ostr << "DispositionVerification::accept_message_fetch_filter: "
          "El::Exception caught. Description:" << std::endl << e;

        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
      }
    }

    void
    DispositionVerification::accept_message_categorizer(
      ::NewsGate::Message::Transport::SetMessageCategorizerRequest*
      categorier) throw()
    {
      if(!started())
      {
        return;
      }

      try
      {
        Transport::SetMessageCategorizerRequestImpl::Type*
          request =
          dynamic_cast<Transport::SetMessageCategorizerRequestImpl::Type*>(
            categorier);

        if(request == 0)
        {
          throw Exception(
            "DispositionVerification::accept_message_categorizer: "
            "dynamic_cast<Transport::SetMessageCategorizerRequestImpl::Type*>"
            " failed");
        }

        propagate_message_categorizer(request, true);
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        
        ostr << "DispositionVerification::accept_message_categorizer: "
          "El::Exception caught. Description:" << std::endl << e;

        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
      }
    }
    
    void
    DispositionVerification::check_message_fetch_filter() throw()
    {
      if(!started())
      {
        return;
      }

      try
      {
        bool need_propagate_filter = false;
        bool enforce = false;
        ACE_Time_Value current_time = ACE_OS::gettimeofday();
        
        {
          ReadGuard guard(srv_lock_);
          
          need_propagate_filter =
            message_fetch_filter_resend_time_ <= current_time;

          enforce = message_fetch_filter_resend_time_ == ACE_Time_Value::zero;
        }

        std::auto_ptr<FetchFilter> filter;
        bool filter_changed = false;
          
        El::MySQL::Connection_var connection =
          Application::instance()->message_managing_dbase()->connect();

        El::MySQL::Result_var qresult = connection->query("begin");

        try
        {
          //
          // "for update" clause is used as a lock to preserve
          // MessageFilterUpdateNum & MessageFetchFilter consistency
          //
          qresult = connection->query(
            "select update_num from MessageFilterUpdateNum for update");

          MessageFilterUpdateNum num(qresult.in());
            
          if(!num.fetch_row())
          {
            throw Exception(
              "NewsGate::Message::DispositionVerification::"
              "check_message_fetch_filter: failed to get latest message "
              "filter update number");
          }

          uint64_t filter_stamp = num.update_num();
          
          {
            WriteGuard guard(srv_lock_);

            filter_changed = filter_stamp > message_fetch_filter_stamp_;
            need_propagate_filter |= filter_changed;

            message_fetch_filter_stamp_ = num.update_num();
          }
          
          if(need_propagate_filter)
          {
            filter.reset(new FetchFilter());
            filter->stamp = filter_stamp;
            filter->enforced = enforce;            

            FetchFilter::ExpressionArray& exp = filter->expressions;
            
            qresult = connection->query(
              "select expression from MessageFetchFilter");
            
            {
              MessageFetchFilter record(qresult.in());
              exp.reserve(qresult->num_rows());
              
              while(record.fetch_row())
              {
                exp.push_back(record.expression().c_str());
              }
            }
            
            ACE_Time_Value threshold =
              ACE_Time_Value(
                current_time.sec() -
                message_management_conf_.filter()->disabled_feeds_timeout());

            std::auto_ptr<std::ostringstream> former_urls_query;
            
            {
              std::ostringstream ostr;
              
              ostr << "select id, url from Feed where status in ('D', 'L') "
                "and updated>='" << El::Moment(threshold).iso8601(false, true)
                   << "'";
              
              qresult = connection->query(ostr.str().c_str());
            
              Feed record(qresult.in());
              
              if(qresult->num_rows())
              {
                std::ostringstream ostr;
                ostr << "URL";                
                
                while(record.fetch_row())
                {
                  ostr << " " << record.url().c_str();

                  unsigned long id = record.id();

                  if(former_urls_query.get() == 0)
                  {
                    former_urls_query.reset(new std::ostringstream());
                    
                    *former_urls_query << "select FeedFormerUrl.url as url "
                      "from FeedFormerUrl left join Feed on "
                      "FeedFormerUrl.url=Feed.url "
                      "where FeedFormerUrl.feed_id in ( " << id;
                  }
                  else
                  {
                    *former_urls_query << ", " << id;
                  }
                }

                exp.push_back(ostr.str());
              }

              if(former_urls_query.get() != 0)
              {
                *former_urls_query << " ) and Feed.status is NULL";
          
                qresult = connection->query(former_urls_query->str().c_str());

                FeedFormerUrl record(qresult.in());

                if(qresult->num_rows())
                {
                  std::ostringstream ostr;
                  ostr << "URL";
                  
                  while(record.fetch_row())
                  {
                    ostr << " " << record.url().c_str();
                  }

                  exp.push_back(ostr.str());
                }
              }              
            }

            {
              threshold = ACE_Time_Value(
                current_time.sec() -
                message_management_conf_.filter()->deleted_messages_timeout());

              std::ostringstream ostr;
            
              ostr << "delete from MessageFilter where  "
                "updated < '" << El::Moment(threshold).iso8601(false, true)
                   << "'";              
              
              qresult = connection->query(ostr.str().c_str());
              qresult = connection->query("select id from MessageFilter");

              MessageRec record(qresult.in());
              
              if(qresult->num_rows())
              {
                std::ostringstream ostr;
                ostr << "MSG";
                
                while(record.fetch_row())
                {
                  ostr << " " << std::hex << record.id().value();
                }
                
                exp.push_back(ostr.str());
              }
            }
          }
          
          qresult = connection->query("commit");
        }
        catch(...)
        {
          qresult = connection->query("rollback");
          throw;
        }        

        if(filter.get() != 0)
        {
          if(Application::will_trace(El::Logging::HIGH))
          {
            FetchFilter::ExpressionArray& exp = filter->expressions;
            
            std::ostringstream ostr;
            
            ostr << "NewsGate::Message::DispositionVerification::"
              "check_message_fetch_filter: propagating expressions ("
                 << exp.size() << ")";

            if(filter_changed)
            {
              for(FetchFilter::ExpressionArray::const_iterator i(exp.begin()),
                    e(exp.end()); i != e; ++i)
              {
                ostr << std::endl << *i;
              }
            }

            Application::logger()->trace(ostr.str(),
                                         Aspect::MSG_FILTER_DIST,
                                         El::Logging::HIGH);
          }
          
          Message::Transport::SetMessageFetchFilterRequestImpl::Var
            request = Message::Transport::
            SetMessageFetchFilterRequestImpl::Init::create(
              filter.release());

          request->serialize();
          propagate_message_fetch_filter(request.in(), false);          

          WriteGuard guard(srv_lock_);
            
          message_fetch_filter_resend_time_ = ACE_OS::gettimeofday() +
            ACE_Time_Value(
              message_management_conf_.filter()->resend_period());
        }
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "DispositionVerification::check_message_fetch_filter: "
            "El::Exception caught. Description:" << std::endl << e;

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(...)
        {
        }
      }
      
      try
      {
        El::Service::CompoundServiceMessage_var msg =
          new CheckMessageFetchFilter(this);
        
        deliver_at_time(
          msg.in(),
          ACE_OS::gettimeofday() +
            ACE_Time_Value(message_management_conf_.filter()->check_period()));
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          
          ostr << "DispositionVerification::check_message_fetch_filter: "
            "El::Exception caught while scheduling the task. "
            "Description:" << std::endl << e;
          
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(...)
        {
        }
      }
    }
    
    void
    DispositionVerification::propagate_message_categorizer(
      ::NewsGate::Message::Transport::SetMessageCategorizerRequestImpl::Type*
        request,
      bool shared)
      throw(Exception, El::Exception)
    {
      try
      {
        ::NewsGate::Message::BankClientSession_var bcs =
          create_client_session(true);
            
        Message::Transport::Response_var response;

        Message::BankClientSession::RequestResult_var result =
          bcs->send_request(request, response.out());

        if(result->code != Message::BankClientSession::RRC_OK)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::DispositionVerification::"
            "propagate_message_categorizer: send_request result code "
               << result->code << ", description: "
               << result->description.in();
            
          if(result->code == Message::BankClientSession::RRC_NOT_READY)
          {
            Application::logger()->trace(ostr.str(),
                                         Aspect::MSG_CATEGORIZER_DIST,
                                         El::Logging::LOW);
          }
          else
          {
            Application::logger()->emergency(
              ostr.str(),
              Aspect::MSG_CATEGORIZER_DIST);
          }
        }
      }
      catch(const Message::ImplementationException& e)
      {
        std::ostringstream ostr;
            
        ostr << "NewsGate::Message::DispositionVerification::"
          "propagate_message_categorizer: Message::ImplementationException "
          "caught. Description:\n" << e.description.in();
            
        Application::logger()->emergency(ostr.str(),
                                         Aspect::MSG_CATEGORIZER_DIST);
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
            
        ostr << "NewsGate::Message::DispositionVerification::"
          "propagate_message_categorizer: CORBA::Exception caught. "
          "Description:\n" << e;
            
        Application::logger()->emergency(ostr.str(),
                                         Aspect::MSG_CATEGORIZER_DIST);
      }

      clean_sinks();

      SharingSinkMap sharing_sink_map;
      
      {
        ReadGuard guard(srv_lock_);
        sharing_sink_map = sharing_sink_map_;
      }

      if(sharing_sink_map.empty())
      {
        return;
      }
      
      if(!shared)
      {
        request->entity().source = Application::instance()->bank_manager_id();
      }

      for(SharingSinkMap::const_iterator it = sharing_sink_map.begin();
          it != sharing_sink_map.end(); it++)
      {
        const SharingSink& record = it->second;
        
        if(record.flags & MSRT_MESSAGE_CATEGORIZER)
        {
          try
          {
            record.manager->
              set_message_categorizer(request, callback_->get_process_id());
          }
          catch(const Message::ImplementationException& e)
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Message::DispositionVerification::"
              "propagate_message_categorizer: "
              "Message::ImplementationException thrown by manager "
                 << it->first << ". Description:\n" << e.description.in();
            
            Application::logger()->notice(ostr.str(),
                                          Aspect::MSG_FILTER_DIST);
          }
          catch(const CORBA::Exception& e)
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Message::DispositionVerification::"
              "propagate_message_categorizer: CORBA::Exception thrown by "
              "manager " << it->first << ".Description:\n" << e;
            
            Application::logger()->notice(ostr.str(),
                                          Aspect::MSG_FILTER_DIST);
          }
        }
      }      
    }
    
    void
    DispositionVerification::propagate_message_fetch_filter(
      ::NewsGate::Message::Transport::SetMessageFetchFilterRequestImpl::Type*
        request,
      bool shared)
      throw(Exception, El::Exception)
    {
      try
      {
        ::NewsGate::Message::BankClientSession_var bcs =
          create_client_session(true);

        Message::Transport::Response_var response;

        Message::BankClientSession::RequestResult_var result =
          bcs->send_request(request, response.out());

        if(result->code != Message::BankClientSession::RRC_OK)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::DispositionVerification::"
            "propagate_message_fetch_filter: send_request result code "
               << result->code << ", description: "
               << result->description.in();
            
          if(result->code == Message::BankClientSession::RRC_NOT_READY)
          {
            Application::logger()->trace(ostr.str(),
                                         Aspect::MSG_FILTER_DIST,
                                         El::Logging::LOW);
          }
          else
          {
            Application::logger()->emergency(
              ostr.str(),
              Aspect::MSG_FILTER_DIST);
          }
        }
      }
      catch(const Message::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::DispositionVerification::"
          "propagate_message_fetch_filter: Message::ImplementationException "
          "caught. Description:\n" << e.description.in();
            
        Application::logger()->emergency(ostr.str(),
                                         Aspect::MSG_FILTER_DIST);
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::DispositionVerification::"
          "propagate_message_fetch_filter: CORBA::Exception caught. "
          "Description:\n" << e;
            
        Application::logger()->emergency(ostr.str(),
                                         Aspect::MSG_FILTER_DIST);
      }

      if(!shared)
      {
        request->entity().source = Application::instance()->bank_manager_id();
      }

      clean_sinks();

      SharingSinkMap sharing_sink_map;
      
      {
        ReadGuard guard(srv_lock_);
        sharing_sink_map = sharing_sink_map_;
      }
      
      for(SharingSinkMap::const_iterator it = sharing_sink_map.begin();
          it != sharing_sink_map.end(); it++)
      {
        const SharingSink& record = it->second;
        
        if(record.flags & (shared ? MSRT_SHARED_MESSAGE_FILTER :
                           MSRT_OWN_MESSAGE_FILTER))
        {
          try
          {
            record.manager->set_message_fetch_filter(
              request,
              shared,
              callback_->get_process_id());
          }
          catch(const Message::ImplementationException& e)
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Message::DispositionVerification::"
              "propagate_message_fetch_filter: "
              "Message::ImplementationException thrown by manager "
                 << it->first << ". Description:\n" << e.description.in();
            
            Application::logger()->notice(ostr.str(),
                                          Aspect::MSG_FILTER_DIST);
          }
          catch(const CORBA::Exception& e)
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Message::DispositionVerification::"
              "propagate_message_fetch_filter: CORBA::Exception thrown by "
              "manager " << it->first << ".Description:\n" << e;
            
            Application::logger()->notice(ostr.str(),
                                          Aspect::MSG_FILTER_DIST);
          }
        }
      }
      
    }
    
    void
    DispositionVerification::check_message_categorizer() throw()
    {
      if(!started())
      {
        return;
      }

      try
      {
        bool need_propagate_categorizer = false;
        bool enforce = false;
        ACE_Time_Value current_time = ACE_OS::gettimeofday();
        
        {
          ReadGuard guard(srv_lock_);
          
          need_propagate_categorizer =
            message_categorizer_resend_time_ <= current_time;

          enforce = message_categorizer_resend_time_ == ACE_Time_Value::zero;
        }

        std::auto_ptr<Categorizer> categorizer;
          
        El::MySQL::Connection_var connection =
          Application::instance()->message_managing_dbase()->connect();

        El::MySQL::Result_var qresult = connection->query("begin");

        try
        {
          //
          // "for update" clause is used as a lock to preserve
          // CategoryUpdateNum & Category consistency
          //
          qresult = connection->query(
            "select update_num from CategoryUpdateNum for update");

          CategoryUpdateNum num(qresult.in());
            
          if(!num.fetch_row())
          {
            throw Exception(
              "NewsGate::Message::DispositionVerification::"
              "check_message_categorizer: failed to get latest category "
              "update number");
          }

          uint64_t categorizer_stamp = num.update_num();
          
          {
            WriteGuard guard(srv_lock_);
            
            need_propagate_categorizer |=
              categorizer_stamp > message_categorizer_stamp_;

            message_categorizer_stamp_ = num.update_num();
          }
          
          if(need_propagate_categorizer)
          {
            ACE_Time_Value threshold = ACE_Time_Value(
              current_time.sec() -
              message_management_conf_.filter()->deleted_messages_timeout());
            
            std::ostringstream ostr;
            
            ostr << "delete from CategoryMessage where "
              "updated < '" << El::Moment(threshold).iso8601(false, true)
                 << "'";
            
            qresult = connection->query(ostr.str().c_str());
            
            categorizer.reset(new Categorizer());
            categorizer->stamp = categorizer_stamp;
            categorizer->enforced = enforce;            

            IdSet context;

            load_category(ROOT_CATEGORY_ID,
                          categorizer->categories,
                          connection,
                          context);
          }
          
          qresult = connection->query("commit");
        }
        catch(...)
        {
          qresult = connection->query("rollback");
          throw;
        }        

        if(categorizer.get() != 0)
        {
          Message::Transport::SetMessageCategorizerRequestImpl::Var
            request = Message::Transport::
            SetMessageCategorizerRequestImpl::Init::create(
              categorizer.release());

          request->serialize();
          propagate_message_categorizer(request.in(), false);

          WriteGuard guard(srv_lock_);
            
          message_categorizer_resend_time_ = ACE_OS::gettimeofday() +
            ACE_Time_Value(
              message_management_conf_.categories()->resend_period());
        }
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "DispositionVerification::check_message_categorizer: "
            "El::Exception caught. Description:" << std::endl << e;

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(...)
        {
        }
      }
      
      try
      {
        El::Service::CompoundServiceMessage_var msg =
          new CheckMessageCategorizer(this);
        
        deliver_at_time(
          msg.in(),
          ACE_OS::gettimeofday() +
          ACE_Time_Value(
            message_management_conf_.categories()->check_period()));
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          
          ostr << "DispositionVerification::check_message_categorizer: "
            "El::Exception caught while scheduling the task. "
            "Description:" << std::endl << e;
          
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(...)
        {
        }
      }
    }

    void
    DispositionVerification::load_category(
      Categorizer::Category::Id id,
      Categorizer::CategoryMap& categories,
      El::MySQL::Connection* connection,
      IdSet& context)
      throw(Exception, El::Exception)
    {
      if(categories.find(id) != categories.end() ||
         context.find(id) != context.end())
      {
        return;
      }

      Categorizer::Category category;
      El::MySQL::Result_var qresult;
      
      {
        std::ostringstream ostr;

        ostr << "select id, name, searcheable from Category "
          "where status='E' and id=" << id;
        
        qresult = connection->query(ostr.str().c_str());

        CategoryDesc desc(qresult.in());

        if(!desc.fetch_row())
        {
          return;
        }

        category.name = desc.name();
        category.searcheable = desc.searcheable().c_str()[0] == 'Y';
      }
      
      {
        std::ostringstream ostr;

        ostr << "select lang, country, name, title, short_title, "
          "description, keywords from CategoryLocale where category_id=" << id;
        
        qresult = connection->query(ostr.str().c_str());

        Categorizer::Category::LocaleMap& locales =
          category.locales;

        CategoryLocaleDesc desc(qresult.in());

        while(desc.fetch_row())
        {
          El::Locale locale(
            El::Lang((El::Lang::ElCode)desc.lang().value()),
            El::Country((El::Country::ElCode)desc.country().value()));
          
          locales.insert(
            std::make_pair(locale,
                           Categorizer::Category::Locale(
                             desc.name().c_str(),
                             desc.title().c_str(),
                             desc.short_title().c_str(),
                             desc.description().c_str(),
                             desc.keywords().c_str())));
        }
      }

      {
        std::ostringstream ostr;

        ostr << "select expression from CategoryExpression "
          "where category_id=" << id;
        
        qresult = connection->query(ostr.str().c_str());

        Categorizer::Category::ExpressionArray& expressions =
          category.expressions;

        CategoryExpressionDesc exp_desc(qresult.in());

        while(exp_desc.fetch_row())
        {
          expressions.push_back(exp_desc.expression());
        }
      }
      
      {
        Categorizer::Category::ExpressionArray& expressions =
          category.expressions;

        for(Categorizer::Category::ExpressionArray::iterator
              i(expressions.begin()), e(expressions.end()); i != e; )
        {            
          try
          {
            Categorization::WordListResolver::instantiate_expression(
              *i,
              id,
              connection);
            
            ++i;
          }
          catch(const Categorization::WordListResolver::Exception& ex)
          {
            i = expressions.erase(i);
            e = expressions.end();
            
            std::ostringstream ostr;
            ostr << "DispositionVerification::load_category: "
              "WordListResolver::Exception caught. Description:"
                 << std::endl << ex;

            El::Service::Error error(ostr.str(), this);
            callback_->notify(&error);
          } 
        }
      }

      {
        std::ostringstream ostr;

        ostr << "select message_id, updated, relation from CategoryMessage "
          "where category_id=" << id;
        
        qresult = connection->query(ostr.str().c_str());
        
        CategoryMessage msg(qresult.in());

        while(msg.fetch_row())
        {
          Categorizer::Category::RelMsg m;
          
          m.id = Message::Id(msg.message_id());
          
          m.updated = ACE_Time_Value(
            El::Moment(msg.updated().value(), El::Moment::TF_ISO_8601)).sec();

          if(msg.relation().value()[0] == 'E')
          {
            category.excluded_messages.push_back(m);
          }
          else
          {
            category.included_messages.push_back(m);
          }
        }
      }

      {
        std::ostringstream ostr;

        ostr << "select Category.id as id from CategoryChild "
          "left join Category on CategoryChild.child_id = Category.id "
          "where CategoryChild.id=" << id << " and Category.status='E'";
        
        qresult = connection->query(ostr.str().c_str());

        Categorizer::Category::IdArray& children = category.children;
      
        CategoryDesc child_desc(qresult.in(), 1);
        
        while(child_desc.fetch_row())
        {
          children.push_back(child_desc.id());
        }

        context.insert(id);
      
        for(Categorizer::Category::IdArray::const_iterator
              it = children.begin(); it != children.end(); it++)
        {
          load_category(*it, categories, connection, context);
        }

        context.erase(id);
        categories[id] = category;
      }
    }
    
    void
    DispositionVerification::check_banks_presence() throw()
    {
      if(!started())
      {
        return;
      }

      try
      {
        if(Application::will_trace(El::Logging::MIDDLE))
        {
          Application::logger()->trace(
            "NewsGate::Message::DispositionVerification::"
            "check_banks_presence: checking presence",
            Aspect::STATE,
            El::Logging::MIDDLE);
        }
        
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();
          
        ACE_Time_Value tm = ACE_OS::gettimeofday();

        std::string last_check_banks_presence_time =
          El::Moment(tm).iso8601(false);

        std::ostringstream ostr;
        ostr << "insert into MessageBankManagerState set bank_manager_id='"
             << connection->escape(
               Application::instance()->bank_manager_id())
             << "', last_check_banks_presence_time='"
             << last_check_banks_presence_time
             << "' on duplicate key update last_check_banks_presence_time='"
             << last_check_banks_presence_time << "'";

        El::MySQL::Result_var result =
          connection->query(ostr.str().c_str());

        bool some_bank_dissapeared = false;
        
        {
          WriteGuard guard(srv_lock_);

          for(BankDisposition::const_iterator it = disposition_.begin();
              it != disposition_.end(); it++)
          {
            const BankInfo& bi = *it;
            
            std::string last_ping_time =
              El::Moment(bi.timestamp).iso8601(false);

            std::ostringstream ostr;
            ostr << "insert into MessageBankSession set bank_ior='"
                 << connection->escape(it->bank.reference().c_str())
                 << "', last_ping_time='" << last_ping_time
                 << "' on duplicate key update last_ping_time='"
                 << last_ping_time << "'";

            El::MySQL::Result_var result =
              connection->query(ostr.str().c_str());

            if(bi.timestamp + presence_poll_timeout_ < tm)
            {
              if(Application::will_trace(El::Logging::MIDDLE))
              {
                std::ostringstream ostr;
                ostr << "NewsGate::Message::DispositionVerification::"
                  "check_banks_presence: "
                     << (bi.active ? "active" : "inactive")
                     << " bank " << it->bank.reference()
                     << " seems to dissapear";
                
                Application::logger()->trace(ostr.str(),
                                             Aspect::STATE,
                                             El::Logging::MIDDLE);
              }

              some_bank_dissapeared = true;
            }
          }
        }
        
        if(some_bank_dissapeared)
        {
          disposition_breakdown();
        }
        else
        {
          El::Service::CompoundServiceMessage_var msg =
            new CheckBanksPresence(this);
          
          deliver_at_time(msg.in(), tm + presence_check_period_);
        }
        
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "DispositionVerification::check_banks_presence: "
            "El::Exception caught. Description:" << std::endl << e;

          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(...)
        {
          El::Service::Error error(
            "DispositionVerification::check_banks_presence: unknown caught.",
            this);
          
          callback_->notify(&error);
        }
      } 
    }
    
    char*
    DispositionVerification::message_sharing_register(
      CORBA::ULong interface_version,
      const char* manager_persistent_id,
      ::NewsGate::Message::BankClientSession* message_sink,
      CORBA::ULong flags,
      CORBA::ULong expiration_time,
      ::NewsGate::Message::Transport::ColoFrontendPack*& colo_frontends)
      throw(NewsGate::Message::NotReady,
            NewsGate::Message::IncompartibleInterface,
            NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      bool interface_matches = interface_version ==
        NewsGate::Message::BankManager::SHARING_INTERFACE_VERSION;
      
      try
      {
        MessageSharingRegisterForeignClient_var msg =
          new MessageSharingRegisterForeignClient(interface_version,
                                                  manager_persistent_id,
                                                  message_sink,
                                                  flags,
                                                  expiration_time,
                                                  colo_frontends,
                                                  this);

        Transport::ColoFrontendPackImpl::Var colo_frontends_pack_impl =
          new Transport::ColoFrontendPackImpl::Type(
            new Transport::ColoFrontendArray());        

        if(msg->colo_frontends->empty())
        {
          if(!interface_matches)
          {
            NewsGate::Message::IncompartibleInterface e;
            
            e.reason = CORBA::string_dup(
              "DispositionVerification::message_sharing_register: "
              "interface mismatch, no colo frontends for delegations");
            
            throw e;
          }
        }
        else
        {
          colo_frontends_pack_impl->entities().push_back(own_colo_frontend_);
        }

        colo_frontends->_remove_ref();
        colo_frontends = colo_frontends_pack_impl._retn();
        
        deliver_now(msg.in());
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "DispositionVerification::message_sharing_register: "
            "El::Exception caught. Description:" << std::endl << e;

          NewsGate::Message::ImplementationException ex;
          ex.description = CORBA::string_dup(ostr.str().c_str());
          throw ex;
        }
        catch(...)
        {
          throw NewsGate::Message::ImplementationException();
        }
      }

      return CORBA::string_dup(interface_matches && flags ?
                               callback_->get_process_id() : "");
    }

    void
    DispositionVerification::message_sharing_unregister(
      const char* manager_persistent_id)
      throw(NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      try
      {
        El::Service::CompoundServiceMessage_var msg =
          new MessageSharingRegisterForeignClient(0,
                                                  manager_persistent_id,
                                                  0,
                                                  0,
                                                  0,
                                                  0,
                                                  this);
      
        deliver_now(msg.in());
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "DispositionVerification::message_sharing_unregister: "
            "El::Exception caught. Description:" << std::endl << e;

          NewsGate::Message::ImplementationException ex;
          ex.description = CORBA::string_dup(ostr.str().c_str());
          throw ex;
        }
        catch(...)
        {
          throw NewsGate::Message::ImplementationException();
        }
      } 
    }

    bool
    DispositionVerification::notify(El::Service::Event* event)
      throw(El::Exception)
    {
      if(BankManagerState::notify(event))
      {
        return true;
      }

      if(dynamic_cast<CheckBanksPresence*>(event) != 0)
      {
        check_banks_presence();
        return true;
      }

      if(dynamic_cast<CheckMessageFetchFilter*>(event) != 0)
      {
        check_message_fetch_filter();
        return true;
      }

      if(dynamic_cast<CheckMessageCategorizer*>(event) != 0)
      {
        check_message_categorizer();
        return true;
      }

      AcceptMessageFetchFilter* amff =
        dynamic_cast<AcceptMessageFetchFilter*>(event);
      
      if(amff != 0)
      {
        accept_message_fetch_filter(amff->filter, amff->shared);
        return true;
      }

      AcceptMessageCategorizer* amc =
        dynamic_cast<AcceptMessageCategorizer*>(event);
      
      if(amc != 0)
      {
        accept_message_categorizer(amc->categorizer);
        return true;
      }

      MessageSharingRegisterForeignClient* rfc =
        dynamic_cast<MessageSharingRegisterForeignClient*>(event);
      
      if(rfc != 0)
      {
        message_sharing_register_foreign_client(
          rfc->interface_version,
          rfc->manager_persistent_id.c_str(),
          rfc->message_sink.in(),
          rfc->flags,
          rfc->expiration_time,
          rfc->colo_frontends.get());
        
        return true;
      }

      if(dynamic_cast<MessageSharingRegisterSelf*>(event) != 0)
      {
        message_sharing_register_self();
        return true;
      }

      std::ostringstream ostr;
      ostr << "NewsGate::RSS::DispositionVerification::notify: unknown "
           << *event;
      
      El::Service::Error error(ostr.str(), this);
      callback_->notify(&error);
      
      return true;
    }
    
  }
}
