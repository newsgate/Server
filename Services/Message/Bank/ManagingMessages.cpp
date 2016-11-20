/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Message/Bank/ManagingMessages.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <stdint.h>

#include <utility>
#include <sstream>
#include <fstream>

#include <El/Exception.hpp>
#include <El/Lang.hpp>
#include <El/Country.hpp>
#include <El/FileSystem.hpp>
#include <El/BinaryStream.hpp>
#include <El/Stat.hpp>

#include <ace/OS.h>

#include <Commons/Message/StoredMessage.hpp>
#include <Commons/Message/TransportImpl.hpp>

#include <Commons/Search/SearchExpression.hpp>
#include <Commons/Search/TransportImpl.hpp>

#include <Services/Commons/Message/MessageServices.hpp>
#include <Services/Commons/Message/BankClientSessionImpl.hpp>
#include <Services/Segmentation/Commons/TransportImpl.hpp>

#include "BankMain.hpp"
#include "ManagingMessages.hpp"

//#define SEARCH_PROFILING 1000

namespace NewsGate
{
  namespace Message
  {
    //
    // ManagingMessages class
    //
    ManagingMessages::ManagingMessages(
      Message::BankSession* session,
      const ACE_Time_Value& report_presence_period,
      BankStateCallback* callback)
      throw(Exception, El::Exception)
        : BankState(callback, "ManagingMessages"),
          report_presence_period_(report_presence_period),
          word_manager_failed_(false),
          segmentor_failed_(false),
          search_counter_(0),
          has_event_bank_(false),
          trace_search_duration_(Application::instance()->config().
                                 message_manager().trace_search_duration()),
          search_meter_("ManagingMessages::search", false),
          get_messages_meter_("ManagingMessages::get_messages", false)
    {
/*
      {
        El::Service::CompoundServiceMessage_var msg =
          new ExitApplication(this);
        
        deliver_at_time(msg.in(),
                        ACE_OS::gettimeofday() + ACE_Time_Value(7200));
      }
*/
      try
      {
        Message::BankManager_var bank_manager =
          Application::instance()->bank_manager();

        bank_client_session_ = bank_manager->bank_client_session();
        
        Event::BankClientSession_var event_bank_client_session =
          Application::instance()->event_bank_client_session();
        
        has_event_bank_ = event_bank_client_session.in() != 0;
      }
      catch(const ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Messages::ManagingMessages::ManagingMessages: "
          "ImplementationException caught. Description:\n"
             << e.description.in();

        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Messages::ManagingMessages::ManagingMessages: "
          "CORBA::Exception caught. Description:\n" << e;
          
        throw Exception(ostr.str());
      }      
      
      session->_add_ref();
      session_ = session;

      const Server::Config::MessageBankType& config =
        Application::instance()->config();

      El::Service::Service_var message_manager =
        new MessageManager(this,
                           config.message_manager(),
                           session_.in(),
                           bank_client_session_.in());
      
      state(message_manager.in());

#   ifdef SEARCH_PROFILING
      search_meter_.active(true);
      get_messages_meter_.active(true);
      Search::Expression::search_meter.active(true);
      Search::Expression::search_p1_meter.active(true);
      Search::Expression::search_p2_meter.active(true);
      Search::AllWords::evaluate_meter.active(true);
      Search::AnyWords::evaluate_meter.active(true);
      Search::Every::evaluate_meter.active(true);
      Search::Event::evaluate_meter.active(true);
      Search::Url::evaluate_meter.active(true);
      Search::Expression::take_top_meter.active(true);

      Search::Expression::search_meter.reset();
      Search::Expression::search_p1_meter.reset();
      Search::Expression::search_p2_meter.reset();
      Search::AllWords::evaluate_meter.reset();
      Search::AnyWords::evaluate_meter.reset();
      Search::Every::evaluate_meter.reset();
      Search::Event::evaluate_meter.reset();
      Search::Url::evaluate_meter.reset();
      Search::Expression::take_top_meter.reset();
#   endif      
    }
    
    ManagingMessages::~ManagingMessages() throw()
    {
    }

    void
    ManagingMessages::session(Message::BankSession* value)
      throw(Exception, El::Exception)
    {
      value->_add_ref();

      {
        WriteGuard guard(srv_lock_);
        session_ = value;
      }

      MessageManager_var manager = message_manager();
      manager->session(value);
    }
    
    bool
    ManagingMessages::start() throw(Exception, El::Exception)
    {      
      if(!BankState::start())
      {
        return false;
      }

      El::Service::CompoundServiceMessage_var msg = new ReportPresence(this);
      
      deliver_at_time(msg.in(),
                      ACE_OS::gettimeofday() + report_presence_period_);

      msg = new AcceptCachedMessages(this);
      deliver_now(msg.in());

      return true;
    }

    void
    ManagingMessages::flush_messages() throw(Exception, El::Exception)
    {
      MessageManager_var manager = message_manager();
      manager->flush_messages();
    }    
    
    void
    ManagingMessages::report_presence() throw()
    {
      if(!started())
      {
        return;
      }
      
      try
      {
        try
        {
          Message::BankManager_var bank_manager =
            Application::instance()->bank_manager();

          std::ostringstream ostr;
          ostr << "ManagingMessages::report_presence: "
               << task_queue_size() << " tasks in queue";
          
          Application::logger()->trace(ostr.str(),
                                       Aspect::MSG_MANAGEMENT,
                                       El::Logging::HIGH);

          bank_manager->ping(Application::instance()->bank_ior(),
                             session_->id());
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::ManagingMessages::report_presence: "
            "El::Exception caught. Description:" << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(const NewsGate::Message::Logout& e)
        {  
          if(started())
          {
            if(Application::will_trace(El::Logging::MIDDLE))
            {
              std::ostringstream ostr;
              ostr << "NewsGate::Message::ManagingMessages::report_presence: "
                "loging out. Reason:\n" << e.reason.in();
            
              Application::logger()->trace(ostr.str(),
                                           Aspect::STATE,
                                           El::Logging::MIDDLE);
            }
            
            callback_->logout(session_->id());
          }
          
          return;
        }
        catch(const CORBA::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::ManagingMessages::report_presence: "
            "CORBA::Exception caught. Description:" << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }

        try
        {
          El::Service::CompoundServiceMessage_var msg =
            new ReportPresence(this);
          
          deliver_at_time(
            msg.in(),
            ACE_OS::gettimeofday() + report_presence_period_);
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::ManagingMessages::report_presence: "
            "El::Exception caught while scheduling task. Description:"
               << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        
      }
      catch(...)
      {
        El::Service::Error error(
          "NewsGate::Message::ManagingMessages::report_presence: "
          "unexpected exception caught.",
          this);
        
        callback_->notify(&error);
      }    
      
    }    

    bool
    ManagingMessages::check_validation_id(bool exception,
                                          const char* validation_id) const
      throw(El::Exception, NewsGate::Message::NotReady)
    {
      bool valid_id = true;
      std::string sharing_id;
      
      {
        ReadGuard guard(srv_lock_);

        sharing_id = session_->sharing_id();
        
        valid_id = (sharing_ids_.get() != 0 &&
                    sharing_ids_->find(validation_id) != sharing_ids_->end()) ||
          sharing_id == validation_id;
      }

      if(!valid_id)
      {
        std::ostringstream ostr;
        ostr << "ManagingMessages::check_validation_id: "
          "validation id '" << validation_id << "' is not acceptable; own "
          "sharing_id is " << sharing_id;
        
        Application::logger()->notice(ostr.str(), Aspect::MSG_MANAGEMENT);
        
        if(exception)
        {
          NewsGate::Message::NotReady ex;
          ex.reason = "Unacceptable validation id";
          throw ex;
        }
      }

      return valid_id;
    }
    
    bool
    ManagingMessages::check_message_manager_readiness(
      bool exception,
      Transport::MessagePack* messages)
      throw(El::Exception, NewsGate::Message::NotReady)
    {
      MessageManager_var manager = message_manager();
      
      if(!manager->loaded())
      {
        if(exception)
        {
          NewsGate::Message::NotReady ex;
          ex.reason = "Message Manager not loaded yet";
          throw ex;
        }

        return false;
      }

      unsigned long capacity = Application::instance()->config().
        message_manager().message_cache().capacity();

      if(capacity)
      {
        size_t message_count = manager->message_count();
        
        size_t pack_message_count =
          PendingMessagePack::message_count(messages);
        
        ReadGuard guard(srv_lock_);

        if(message_count + pending_message_packs_.pending_messages() +
           pack_message_count > capacity)
        {
          manager->adjust_capacity_threshold(pack_message_count);
          
          if(exception)
          {
            std::ostringstream ostr;
            ostr << "ManagingMessages::check_message_manager_readiness: "
              "package rejected as message capacity " << capacity
                 << " will be exceeded:\n" << message_count << " in cache, "
                 << pending_message_packs_.pending_new_messages()
                 << " new msg in processing queue, "
                 << pending_message_packs_.pending_shared_messages()
                 << " shared msg in processing queue, "
                 << pending_message_packs_.pending_pushed_messages()
                 << " pushed msg in processing queue, "
                 << pack_message_count << " in incoming pack";
          
            Application::logger()->notice(ostr.str(),Aspect::MSG_MANAGEMENT);
            
            NewsGate::Message::NotReady ex;
            ex.reason = "Message Manager capacity exceeded";
            throw ex;
          }
          
          return false;        
        }
      }

      return true;
    }

    bool
    ManagingMessages::check_message_pack_queue_readiness(
      bool exception,
      Transport::MessagePack* messages,
      PostMessageReason reason)
      throw(El::Exception, NewsGate::Message::NotReady)
    {
      const Server::Config::MessageBankType::message_manager_type&
        message_manager_conf =
        Application::instance()->config().message_manager();

      if(PendingMessagePack::is_message_event_pack(messages))
      { 
        ReadGuard guard(srv_lock_);

        if(pending_message_packs_.message_event_packs() >=
           message_manager_conf.message_event_pack_processing_queue_size())
        {
          std::ostringstream ostr;
          ostr << "ManagingMessages::check_message_pack_queue_readiness: "
            "package rejected as there is "
               << pending_message_packs_.message_event_packs()
               << " pending message event packs in queue";
        
          Application::logger()->notice(ostr.str(),Aspect::MSG_MANAGEMENT);

          if(exception)
          {
            NewsGate::Message::NotReady ex;
            ex.reason = "Too many message events in processing queue";
            throw ex;
          }

          return false;
        }

        return true;
      }
      
      if(PendingMessagePack::is_new_message_pack(messages, reason))
      { 
        ReadGuard guard(srv_lock_);
        
        if(pending_message_packs_.new_message_packs() >=
           message_manager_conf.new_message_pack_processing_queue_size())
        {
          std::ostringstream ostr;
          ostr << "ManagingMessages::check_message_pack_queue_readiness: "
            "package rejected as there is "
               << pending_message_packs_.new_message_packs()
               << " pending new message packs in queue";
        
          Application::logger()->notice(ostr.str(),Aspect::MSG_MANAGEMENT);

          if(exception)
          {
            NewsGate::Message::NotReady ex;
            ex.reason = "Too many raw messages in processing queue";
            throw ex;
          }

          return false;
        }

        return true;
      }

      // messages == 0 when checking for sharing message offer processing
      if(messages == 0 ||
         PendingMessagePack::is_shared_message_pack(messages, reason))
      { 
        ReadGuard guard(srv_lock_);
        
        if(pending_message_packs_.shared_message_packs() >=
           message_manager_conf.shared_message_pack_processing_queue_size())
        {
          std::ostringstream ostr;
          ostr << "ManagingMessages::check_message_pack_queue_readiness: "
            "package rejected as there is "
               << pending_message_packs_.shared_message_packs()
               << " pending shared message packs in queue";
          
          Application::logger()->notice(ostr.str(),Aspect::MSG_MANAGEMENT);
          
          if(exception)
          {
            NewsGate::Message::NotReady ex;
            ex.reason = "Too many stored messages in processing queue";
            throw ex;
          }
          
          return false;
        }

        return true;
      }

      if(PendingMessagePack::is_pushed_message_pack(messages, reason))
      { 
        ReadGuard guard(srv_lock_);
        
        if(pending_message_packs_.pushed_message_packs() >=
           message_manager_conf.pushed_message_pack_processing_queue_size())
        {
          std::ostringstream ostr;
          ostr << "ManagingMessages::check_message_pack_queue_readiness: "
            "package rejected as there is "
               << pending_message_packs_.pushed_message_packs()
               << " pending pushed message packs in queue";
        
          Application::logger()->notice(ostr.str(),Aspect::MSG_MANAGEMENT);

          if(exception)
          {
            NewsGate::Message::NotReady ex;
            ex.reason = "Too many stored messages in processing queue";
            throw ex;
          }

          return false;
        }

        return true;
      }

      return true;
    }
    
    bool
    ManagingMessages::check_accept_package_readiness(
      bool exception,
      Transport::MessagePack* messages,
      PostMessageReason reason)
      throw(El::Exception, NewsGate::Message::NotReady)
    { 
      return check_message_manager_readiness(exception, messages) && 
        check_message_pack_queue_readiness(exception, messages, reason) &&
        check_word_manager_readiness(exception, messages, reason) &&
        check_segmentor_readiness(exception, messages, reason);
    }
    
    bool
    ManagingMessages::check_word_manager_readiness(
      bool exception,
      Transport::MessagePack* messages,
      PostMessageReason reason)
      throw(El::Exception, NewsGate::Message::NotReady)
    {
      if(PendingMessagePack::new_message_count(messages, reason) +
         PendingMessagePack::shared_message_count(messages, reason) == 0)
      {
        // Such messages do not require to be normalized
        return true;
      }
        
      bool ready = false;
      std::string not_ready_reason;
      
      try
      {
        bool failed = false;
        
        {
          ReadGuard guard(srv_lock_);
          failed = word_manager_failed_;
        }

        if(failed)
        {
          not_ready_reason = "WordManager failed recently";
        }
        else
        {
          NewsGate::Dictionary::WordManager_var word_manager =
            Application::instance()->word_manager();

          ready = word_manager->is_ready() != 0;
        }
        
      }
      catch(const Dictionary::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "ManagingMessages::check_word_manager_readiness: "
          "Dictionary::WordManager::ImplementationException "
          "thrown by WordManager::is_ready. Description:\n"
             << e.description.in();
          
        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
      }      
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "ManagingMessages::check_word_manager_readiness: "
          "CORBA::Exception thrown by WordManager::is_ready. Description:\n"
             << e;
          
        not_ready_reason = ostr.str();

        El::Service::Error error(not_ready_reason, this);
        callback_->notify(&error);
      }

      if(!ready)
      {
        if(exception)
        {
          NewsGate::Message::NotReady ex;
        
          ex.reason = not_ready_reason.empty() ?
            "Word Manager not ready" : not_ready_reason.c_str();
          
          throw ex;
        }

        return false;
      }

      return true;
    }
    
    bool
    ManagingMessages::check_segmentor_readiness(
      bool exception,
      Transport::MessagePack* messages,
      PostMessageReason reason)
      throw(El::Exception, NewsGate::Message::NotReady)
    {
      if(PendingMessagePack::new_message_count(messages, reason) == 0)
      {
        // Such messages do not require to be segmented
        return true;
      }
        
      NewsGate::Segmentation::Segmentor_var segmentor =
        Application::instance()->segmentor();

      if(CORBA::is_nil(segmentor.in()))
      {
        return true;
      }

      bool ready = false;
      std::string not_ready_reason;
      
      try
      {
        bool failed = false;
        
        {
          ReadGuard guard(srv_lock_);
          failed = segmentor_failed_;
        }

        if(failed)
        {
          not_ready_reason = "Segmentor failed recently";
        }
        else
        {
          ready = segmentor->is_ready() != 0;
        }
        
      }
      catch(const Segmentation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "ManagingMessages::check_segmentor_readiness: "
          "Segmentation::ImplementationException "
          "thrown by Segmentor::is_ready. Description:\n"
             << e.description.in();
          
        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
      }      
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "ManagingMessages::check_segmentor_readiness: "
          "CORBA::Exception thrown by Segmentor::is_ready. Description:\n"
             << e;
          
        not_ready_reason = ostr.str();

        El::Service::Error error(not_ready_reason, this);
        callback_->notify(&error);
      }

      if(!ready)
      {
        if(exception)
        {
          NewsGate::Message::NotReady ex;
        
          ex.reason = not_ready_reason.empty() ?
            "Word Manager not ready" : not_ready_reason.c_str();
          
          throw ex;
        }

        return false;
      }

      return true;
    }
    
    void
    ManagingMessages::message_sharing_offer(
      ::NewsGate::Message::Transport::MessageSharingInfoPack* offered_messages,
        const char* validation_id,
      ::NewsGate::Message::Transport::IdPack_out requested_messages)
      throw(NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      if(!check_validation_id(false, validation_id))
      {
        requested_messages =
          Transport::IdPackImpl::Init::create(new IdArray());
        
        return;
      }
      
      Transport::IdPackImpl::Var requested_msg_pack;
      
      try
      {
        Transport::MessageSharingInfoPackImpl::Type* offered_msg_pack =
          dynamic_cast<Transport::MessageSharingInfoPackImpl::Type*>(
            offered_messages);

        if(offered_msg_pack == 0)
        {
          throw Exception("NewsGate::Message::ManagingMessages::"
                          "message_sharing_offer: dynamic_cast<Transport::"
                          "MessageSharingInfoPackImpl*> failed");
        }
          
        requested_msg_pack = Transport::IdPackImpl::Init::create(
          new IdArray());

        if(check_message_manager_readiness(false, 0) && 
           check_message_pack_queue_readiness(false,
                                              0,
                                              PMR_OLD_MESSAGE_SHARING))
        {
          MessageManager_var manager = message_manager();
          
          manager->message_sharing_offer(
            offered_msg_pack->release(),
            requested_msg_pack->entities());
        }
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::ManagingMessages::message_sharing_offer: "
          "El::Exception caught. Description:\n" << e.what();

        ImplementationException e;
        e.description = CORBA::string_dup(ostr.str().c_str());

        throw e;
      }

      requested_messages = requested_msg_pack._retn();
    }

    void
    ManagingMessages::post_messages(
      ::NewsGate::Message::Transport::MessagePack* messages,
      ::NewsGate::Message::PostMessageReason reason,
      const char* validation_id)
      throw(NewsGate::Message::NotReady,
            NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      try
      {
        bool msg_sharing =
          reason >= PMR_NEW_MESSAGE_SHARING && reason < PMR_EVENT_UPDATE;
        
        Application::logger()->trace(
          "ManagingMessages::post_messages: arrived",
          msg_sharing ? Aspect::MSG_SHARING : Aspect::MSG_MANAGEMENT,
          El::Logging::HIGH);

        if(msg_sharing)
        {
          check_validation_id(true, validation_id);
        }

        check_accept_package_readiness(true, messages, reason);        

        {
          WriteGuard guard(srv_lock_);

          pending_message_packs_.push_back(
            PendingMessagePack(messages, reason));
        }
        
        El::Service::CompoundServiceMessage_var msg =
          new AcceptMessages(this);
        
        deliver_now(msg.in());
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::ManagingMessages::post_messages: "
          "El::Exception caught. Description:\n" << e.what();
        
        ImplementationException e;
        e.description = CORBA::string_dup(ostr.str().c_str());
        
        throw e;
      }
    }

    void
    ManagingMessages::search(const ::NewsGate::Message::SearchRequest& request,
           ::NewsGate::Message::MatchedMessages_out result)
      throw(NewsGate::Message::NotReady,
            NewsGate::Message::ImplementationException,
            ::CORBA::SystemException)
    {
//      std::cerr << "ManagingMessages::search: start\n";
      
#ifdef USE_HIRES
      ACE_High_Res_Timer timer;
      timer.start();
#else
      ACE_Time_Value start_time = ACE_OS::gettimeofday();
#endif
      
#   ifdef SEARCH_PROFILING
      El::Stat::TimeMeasurement measurement(search_meter_);
#   endif
      
      try
      {
        ::NewsGate::Search::Transport::ExpressionImpl::Type*
            expression_transport = dynamic_cast<
            ::NewsGate::Search::Transport::ExpressionImpl::Type*>(
              request.expression.in());

        if(expression_transport == 0)
        {
          throw Exception("NewsGate::Message::ManagingMessages::search: "
                          "dynamic_cast failed for request.expression");
        }

        const ::NewsGate::Search::Transport::ExpressionHolder& holder = 
          expression_transport->entity();

        const NewsGate::Search::Expression_var& expression =
          holder.expression;

        ::NewsGate::Search::Transport::StrategyImpl::Type*
            strategy_transport = dynamic_cast<
            ::NewsGate::Search::Transport::StrategyImpl::Type*>(
              request.strategy.in());

        if(strategy_transport == 0)
        {
          throw Exception("NewsGate::Message::ManagingMessages::search: "
                          "dynamic_cast failed for request.strategy");
        }

        const ::NewsGate::Search::Strategy& strategy = 
          strategy_transport->entity();
        
        size_t total_matched_messages = 0;
        size_t suppressed_messages = 0;  
        
        ::NewsGate::Message::MatchedMessages_var res =
            new ::NewsGate::Message::MatchedMessages();

        Transport::CategoryLocaleImpl::Type* res_category_locale =
          Transport::CategoryLocaleImpl::Init::create(
            new Categorizer::Category::Locale());

        res->category_locale = res_category_locale;
        
        MessageManager_var manager = message_manager();
        res->messages_loaded = manager->loaded();

        const SearchLocale& locale = request.locale;
        
        NewsGate::Search::ResultPtr search_result(
          manager->search(
            expression.in(),
            request.start_from,
            request.results_count,
            strategy,
            El::Locale(El::Lang((El::Lang::ElCode)locale.lang),
                       El::Country((El::Country::ElCode)locale.country)),
            request.gm_flags,
            request.category_locale.in(),
            res_category_locale->entity(),
            total_matched_messages,
            suppressed_messages));

        res->total_matched_messages = total_matched_messages;
        res->suppressed_messages = suppressed_messages;

        ::NewsGate::Search::Transport::ResultImpl::Var result_transport =
            new ::NewsGate::Search::Transport::ResultImpl::Type(
              search_result.release());

        // Serialize here to get serialization time counted in time measurement
        result_transport->serialize();

        res->search_result = result_transport._retn();
        result = res._retn();
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::ManagingMessages::search: "
          "El::Exception caught. Description:\n" << e.what();
        
        ImplementationException e;
        e.description = CORBA::string_dup(ostr.str().c_str());
        
        throw e;
      }

//      std::cerr << "ManagingMessages::search: end\n";
      
#   ifdef SEARCH_PROFILING

      measurement.stop();
      
      WriteGuard guard(srv_lock_);

      if(++search_counter_ % SEARCH_PROFILING == 0)
      {
        search_meter_.dump(std::cerr);
        Search::Expression::search_meter.dump(std::cerr);
        Search::Expression::search_p1_meter.dump(std::cerr);
        Search::Expression::search_p2_meter.dump(std::cerr);
        Search::AllWords::evaluate_meter.dump(std::cerr);
        Search::AnyWords::evaluate_meter.dump(std::cerr);
        Search::Every::evaluate_meter.dump(std::cerr);
        Search::Event::evaluate_meter.dump(std::cerr);
        Search::Url::evaluate_meter.dump(std::cerr);
        Search::Expression::take_top_meter.dump(std::cerr);
        get_messages_meter_.dump(std::cerr);

        search_meter_.reset();
        get_messages_meter_.reset();
        Search::Expression::search_meter.reset();
        Search::Expression::search_p1_meter.reset();
        Search::Expression::search_p2_meter.reset();
        Search::AllWords::evaluate_meter.reset();
        Search::AnyWords::evaluate_meter.reset();
        Search::Every::evaluate_meter.reset();
        Search::Event::evaluate_meter.reset();
        Search::Url::evaluate_meter.reset();
        Search::Expression::take_top_meter.reset();
      }      
#   endif

#ifdef USE_HIRES
      timer.stop();
      ACE_Time_Value tm;
      timer.elapsed_time(tm);
#else
      ACE_Time_Value tm = ACE_OS::gettimeofday() - start_time;
#endif
      
      if(tm > trace_search_duration_)
      {
        std::ostringstream ostr;
        ostr << "ManagingMessages::search: long request "
             << El::Moment::time(tm);
        
        Application::logger()->trace(ostr.str(),
                                     Aspect::PERFORMANCE,
                                     El::Logging::HIGH);
      }
      
#     ifdef TRACE_SEARCH_TIME

      std::cerr << "ManagingMessages::search: " << El::Moment::time(tm)
                << std::endl;
#     endif
    }
    
    ::NewsGate::Message::Transport::StoredMessagePack*
    ManagingMessages::get_messages(
      ::NewsGate::Message::Transport::IdPack* message_ids,
      ::CORBA::ULongLong gm_flags,
      ::CORBA::Long img_index,
      ::CORBA::Long thumb_index,
      ::NewsGate::Message::Transport::IdPack_out notfound_message_ids)
      throw(NewsGate::Message::NotReady,
            NewsGate::Message::ImplementationException,
            ::CORBA::SystemException)
    {
#     ifdef TRACE_SEARCH_TIME
      ACE_High_Res_Timer timer;
      timer.start();
#     endif      
      
#   ifdef SEARCH_PROFILING
      El::Stat::TimeMeasurement measurement(get_messages_meter_);
#   endif

      try
      {
        ::NewsGate::Message::Transport::IdPackImpl::Type* msg_ids =
            dynamic_cast< ::NewsGate::Message::Transport::IdPackImpl::Type* >(
              message_ids);
        
        if(msg_ids == 0)
        {
          throw Exception("NewsGate::Message::ManagingMessages::get_messages: "
                          "dynamic_cast failed for message_ids");
        }

        IdArrayPtr notfound_msg_ids(new IdArray());
        MessageManager_var manager = message_manager();
        
        Transport::StoredMessageArrayPtr result(
          manager->get_messages(msg_ids->entities(),
                                gm_flags,
                                img_index,
                                thumb_index,
                                *notfound_msg_ids.get()));
        
        Transport::StoredMessagePackImpl::Var res =
          new Transport::StoredMessagePackImpl::Type(
            result.release());

        Transport::IdPackImpl::Var notfound =
          new Transport::IdPackImpl::Type(
              notfound_msg_ids.release());

        // Serialize here to get serialization time counted in time measurement
        res->serialize();
        notfound->serialize();
        
        notfound_message_ids = notfound._retn();

#     ifdef TRACE_SEARCH_TIME
        timer.stop();
        ACE_Time_Value tm;
        timer.elapsed_time(tm);
        
        std::cerr << "ManagingMessages::get_messages: " << El::Moment::time(tm)
                  << std::endl;
#     endif
        
        return res._retn();
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::ManagingMessages::get_messages: "
          "El::Exception caught. Description:\n" << e.what();
        
        ImplementationException e;
        e.description = CORBA::string_dup(ostr.str().c_str());
        
        throw e;
      }
        
      return 0;
    }

    void
    ManagingMessages::send_request(
      ::NewsGate::Message::Transport::Request* req,
      ::NewsGate::Message::Transport::Response_out resp)
      throw(NewsGate::Message::InvalidData,
            NewsGate::Message::NotReady,
            NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      std::string type = "<unknown>";
      
      try
      {
        Transport::MessageStatRequestImpl::Type* stat_request =
          dynamic_cast<Transport::MessageStatRequestImpl::Type*>(req);

        if(stat_request != 0)
        {
          type = "message_stat";
            
          MessageManager_var manager = message_manager();
          resp = manager->message_stat(stat_request->release());
        
          return;
        }
          
        Transport::MessageDigestRequestImpl::Type* digest_request =
          dynamic_cast<Transport::MessageDigestRequestImpl::Type*>(req);

        if(digest_request != 0)
        {
          type = "get_message_digests";

          MessageManager_var manager = message_manager();
          resp = manager->get_message_digests(digest_request->entities());
        
          return;
        }

        Transport::CheckMirroredMessagesRequestImpl::Type* cmm_request =
          dynamic_cast<Transport::CheckMirroredMessagesRequestImpl::Type*>(
            req);

        if(cmm_request != 0)
        {
          type = "check_mirrored_messages";

          bool ready = check_message_manager_readiness(false, 0) && 
            check_message_pack_queue_readiness(false,
                                               0,
                                               PMR_LOST_MESSAGE_SHARING);
          
          MessageManager_var manager = message_manager();
          
          resp = manager->check_mirrored_messages(cmm_request->entities(),
                                                  ready);
        
          return;
        }
      
        Transport::SetMessageFetchFilterRequestImpl::Type*
          set_message_fetch_filter_request =
          dynamic_cast<Transport::SetMessageFetchFilterRequestImpl::Type*>(
            req);

        if(set_message_fetch_filter_request != 0)
        {
          type = "set_message_fetch_filter";

          El::Service::CompoundServiceMessage_var msg =
            new SetMessageFetchFilter(
              this,
              set_message_fetch_filter_request->release(),
              0);
        
          deliver_now(msg.in());
        
          resp = new Transport::EmptyResponseImpl::Type();          
          return;
        }
      
        Transport::SetMessageCategorizerRequestImpl::Type*
          set_message_categorizer_request =
          dynamic_cast<Transport::SetMessageCategorizerRequestImpl::Type*>(
            req);

        if(set_message_categorizer_request != 0)
        {
          type = "set_message_categorizer_request";

          El::Service::CompoundServiceMessage_var msg =
            new SetMessageCategorizer(
              this,
              set_message_categorizer_request->release(),
              0);
        
          deliver_now(msg.in());
        
          resp = new Transport::EmptyResponseImpl::Type();          
          return;
        }

        Transport::SetMessageSharingSourcesRequestImpl::Type*
          sharing_sources_request = dynamic_cast<
          Transport::SetMessageSharingSourcesRequestImpl::Type*>(req);

        if(sharing_sources_request != 0)
        {
          type = "sharing_sources";

          MessageManager_var manager = message_manager();
            
          const Transport::MessageSharingSources& sharing_sources =
            sharing_sources_request->entity();
          
          if(!sharing_sources.mirrored_manager.empty() &&
             !sharing_sources.ids.empty())
          {
            manager->set_mirrored_manager(
              sharing_sources.mirrored_manager.c_str(),
              sharing_sources.ids.begin()->c_str());
          }
          else
          {
            manager->set_mirrored_manager(0, 0);
          }
          
          resp = new Transport::EmptyResponseImpl::Type();

          WriteGuard guard(srv_lock_);
          sharing_ids_.reset(new Transport::SharingIdSet(sharing_sources.ids));
          
          return;
        }

        Transport::EmptyRequestImpl::Type* empty_request =
          dynamic_cast<Transport::EmptyRequestImpl::Type*>(req);

        if(empty_request != 0)
        {
          return;
        }       
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "::NewsGate::Message::ManagingMessages::send_request: type "
             << type << ": El::Exception caught. Description:\n" << e;

        ImplementationException e;
        e.description = ostr.str().c_str();

        throw e;
      }
      
      NewsGate::Message::InvalidData ex;
      ex.description = "::NewsGate::Message::ManagingMessages::send_request: "
        "unrecognized request";
      
      throw ex;
    }
    
    void
    ManagingMessages::message_sharing_register(
      const char* manager_persistent_id,
      ::NewsGate::Message::BankClientSession* message_sink,
      CORBA::ULong flags,
      CORBA::ULong expiration_time)
      throw(NewsGate::Message::NotReady,
            NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      if(message_sink != 0)
      {
        try
        {
          /*
          // Using just 1 thread not to exaust resources when multiple
          // sinks registered
          bank_client_session_impl->init_threads(this, 1);
          */
          
          WriteGuard guard(srv_lock_);
          
          MessageSink ms;
          
          message_sink->_add_ref();
          ms.message_sink = message_sink;
          ms.expiration_time = ACE_Time_Value(expiration_time);
          ms.flags = flags;
          
          message_sink_map_[manager_persistent_id] = ms;
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::ManagingMessages::"
            "message_sharing_register: El::Exception caught. Description:\n"
               << e;

          NewsGate::Message::ImplementationException ex;
          ex.description = ostr.str().c_str();
            
          throw ex;
        }
        
      }
    }

    void
    ManagingMessages::dictionary_hash_changed() throw(El::Exception)
    {
      const Server::Config::MessageBankType& config =
        Application::instance()->config();

      El::Service::Service_var message_manager =
        new MessageManager(this,
                           config.message_manager(),
                           session_.in(),
                           bank_client_session_.in());
      
      state(message_manager.in());
    }
    
    MessageSinkMap
    ManagingMessages::message_sink_map(unsigned long flags)
      throw(El::Exception)
    {
      typedef std::list<std::string> SinkList;
      SinkList expired_sinks;

      MessageSinkMap sinks;        
      ACE_Time_Value cur_time = ACE_OS::gettimeofday();

      {
        ReadGuard guard(srv_lock_);

        for(MessageSinkMap::const_iterator it = message_sink_map_.begin();
            it != message_sink_map_.end(); it++)
        {
          const MessageSink& sink = it->second;
            
          if(sink.expiration_time >= cur_time)
          {
            if((sink.flags & flags) == flags)
            {
              sinks[it->first] = sink;
            }
          }
          else
          {
            expired_sinks.push_back(it->first);
          }
        }
      }
      
      if(!expired_sinks.empty())
      {
        WriteGuard guard(srv_lock_);

        for(SinkList::const_iterator it = expired_sinks.begin();
            it != expired_sinks.end(); it++)
        {
          MessageSinkMap::iterator msit = message_sink_map_.find(*it);

          if(msit != message_sink_map_.end() &&
             msit->second.expiration_time < cur_time)
          {
            message_sink_map_.erase(msit);
          }
        }  
      }
      
      return sinks;
    }

    void
    ManagingMessages::message_sharing_unregister(
      const char* manager_persistent_id)
      throw(NewsGate::Message::ImplementationException,
            CORBA::SystemException)
    {
      WriteGuard guard(srv_lock_);
      message_sink_map_.erase(manager_persistent_id);
    }

    void
    ManagingMessages::set_message_fetch_filter(FetchFilterPtr& filter,
                                               size_t retry) throw()
    {
      try
      {
        MessageManager_var manager = message_manager();

        if(manager->set_message_fetch_filter(*filter) ==
           MessageManager::AR_NOT_READY)
        {
          const ::Server::Config::BankMessageManagerType::
            message_filter_type config =
            Application::instance()->config().message_manager().
            message_filter();

          if(retry < config.set_retry_max_count())
          {
            El::Service::CompoundServiceMessage_var msg =
              new SetMessageFetchFilter(this, filter.release(), retry + 1);
            
            deliver_at_time(msg.in(),
                            ACE_OS::gettimeofday() +
                            ACE_Time_Value(config.set_retry_max_count()));
          }
        }
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::ManagingMessages::"
          "set_message_fetch_filter: El::Exception caught. Description:\n"
             << e;
        
        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
      }
    }

    void
    ManagingMessages::set_message_categorizer(CategorizerPtr& categorizer,
                                              size_t retry)
      throw()
    {
      try
      {
        MessageManager_var manager = message_manager();

        if(manager->assign_message_categorizer(*categorizer) ==
           MessageManager::AR_NOT_READY)
        {
          const ::Server::Config::BankMessageManagerType::
            message_categorizer_type config =
            Application::instance()->config().message_manager().
            message_categorizer();
          
          if(retry < config.set_retry_max_count())
          {
            El::Service::CompoundServiceMessage_var msg =
              new SetMessageCategorizer(this,
                                        categorizer.release(),
                                        retry + 1);
            
            deliver_at_time(msg.in(),
                            ACE_OS::gettimeofday() +
                            ACE_Time_Value(config.set_retry_period()));
          }
        }
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::ManagingMessages::"
          "set_message_categorizer: El::Exception caught. Description:\n"
             << e;
        
        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
      }
    }

    void
    ManagingMessages::accept_messages(AcceptMessages* am) throw()
    {
      try
      {      
        try
        {
          if(!started())
          {
            return;
          }

          MessageManager_var manager = message_manager();
          bool loaded = manager->loaded();
          
          PendingMessagePack pending_pack;
          
          {
            WriteGuard guard(srv_lock_);

            if(pending_message_packs_.empty())
            {
              return;
            }

            if(!loaded)
            {
              deliver_at_time(
                am,
                ACE_OS::gettimeofday() +
                ACE_Time_Value(
                  Application::instance()->config().word_management().
                  retry_period()));

              return;
            }
            
            pending_pack = pending_message_packs_.pop_front();
          }

          Transport::RawMessagePackImpl::Type* raw_msg_pack =
            dynamic_cast<Transport::RawMessagePackImpl::Type*>(
              pending_pack.pack.in());

          if(raw_msg_pack != 0)
          {
            process_messages(raw_msg_pack, pending_pack.reason);
            return;
          }
          
          Transport::StoredMessagePackImpl::Type* stored_msg_pack =
            dynamic_cast<Transport::StoredMessagePackImpl::Type*>(
              pending_pack.pack.in());

          if(stored_msg_pack != 0)
          {
            process_messages(stored_msg_pack, pending_pack.reason);
            return;
          }

          Transport::MessageEventPackImpl::Type* msg_event_pack =
            dynamic_cast<Transport::MessageEventPackImpl::Type*>(
              pending_pack.pack.in());

          if(msg_event_pack != 0)
          {
            process_messages(msg_event_pack, pending_pack.reason);
            return;
          }

          throw Exception(
            "NewsGate::Message::ManagingMessages::accept_messages: "
            "Transport::MessagePack* couldn't be casted to expected type");
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::ManagingMessages::accept_messages: "
            "El::Exception caught. Description:" << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
        catch(const CORBA::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::ManagingMessages::accept_messages: "
            "CORBA::Exception caught. Description:" << std::endl << e;
        
          El::Service::Error error(ostr.str(), this);
          callback_->notify(&error);
        }
      }
      catch(...)
      {
        El::Service::Error error(
          "NewsGate::Message::ManagingMessages::accept_messages: "
          "unexpected exception caught.",
          this);
          
        callback_->notify(&error);
      }
    }

    Segmentation::Transport::SegmentedMessagePackImpl::Type*
    ManagingMessages::segment_message_components(
      const Transport::RawMessagePackImpl::MessageArray& entities,
      bool& segmentation_enabled)
      throw(Exception, El::Exception)
    {
      NewsGate::Segmentation::Segmentor_var segmentor =
        Application::instance()->segmentor();

      segmentation_enabled = !CORBA::is_nil(segmentor.in());

      if(!segmentation_enabled)
      {
        return 0;
      }
      
      Segmentation::Transport::MessagePackImpl::Var msg_pack =
        new Segmentation::Transport::MessagePackImpl::Type(
          new Segmentation::Transport::MessageArray());
        
      Segmentation::Transport::MessageArray& messages = msg_pack->entities();
      messages.resize(entities.size());
        
      for(size_t i = 0; i < entities.size(); i++)
      {
        const Message::RawContent* src = entities[i].content.in();
        Segmentation::Transport::Message& dest = messages[i];

        dest.title = src->title;
        dest.description = src->description;
        dest.keywords = src->keywords;

        size_t images_count = src->images.get() == 0 ?
          0 : src->images->size();
        
        dest.images.resize(images_count);

        for(size_t i = 0; i < images_count; i++)
        {
          dest.images[i].alt = (*src->images)[i].alt;
        }
      }

      try
      {
        Segmentation::Transport::SegmentedMessagePack_var result =
          segmentor->segment_message_components(msg_pack.in());

        Segmentation::Transport::SegmentedMessagePackImpl::Type* impl =
          dynamic_cast<Segmentation::Transport::SegmentedMessagePackImpl::
          Type*>(result.in());
          
        if(impl == 0)
        {
          El::Service::Error error(
            "ManagingMessages::segment_message_components: "
            "dynamic_cast<Segmentation::Transport::SegmentedMessagePackImpl::"
            "Type*> failed",
            this);
            
          callback_->notify(&error);
        }

        impl->_add_ref();
        return impl;
      }
      catch(const Segmentation::NotReady& e)
      {
        std::ostringstream ostr;
        ostr << "ManagingMessages::segment_message_components: segmentor not "
          "ready. Reason:\n" << e.reason.in();
          
        El::Service::Error
          error(ostr.str(), this, El::Service::Error::NOTICE);
          
        callback_->notify(&error);
      }
      catch(const Segmentation::InvalidArgument& e)
      {
        std::ostringstream ostr;
        ostr << "ManagingMessages::segment_message_components: "
          "Segmentation::InvalidArgument thrown by "
          "Segmentor::segment_message_components. Description:\n"
             << e.description.in();
          
        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
      }
      catch(const Segmentation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "ManagingMessages::segment_message_components: "
          "Segmentation::ImplementationException "
          "thrown by Segmentor::segment_message_components. Description:\n"
             << e.description.in();
          
        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "ManagingMessages::segment_message_components: "
          "CORBA::Exception thrown by Segmentor::segment_message_components. "
          "Description:\n" << e;
          
        El::Service::Error error(ostr.str(), this);
        callback_->notify(&error);
      }

      return 0;
    }

    void
    ManagingMessages::process_messages(
      Transport::RawMessagePackImpl::Type* pack,
      ::NewsGate::Message::PostMessageReason reason)
      throw(Exception, El::Exception, CORBA::Exception)
    {
      {
        Transport::RawMessagePackImpl::MessageArray& entities =
          pack->entities();

        for(Transport::RawMessagePackImpl::MessageArray::iterator it =
              entities.begin(); it != entities.end(); it++)
        {
          RawMessage& message = *it;
          
          if(!El::String::Manip::utf8_valid(
               message.content->source.title.c_str(),
               El::String::Manip::UAC_XML_1_0))
          {
            message.content->source.title = "";
          }
          
          if(!El::String::Manip::utf8_valid(
               message.content->source.html_link.c_str(),
               El::String::Manip::UAC_XML_1_0))
          {
            message.content->source.html_link = "";
          }

          if(!El::String::Manip::utf8_valid(
               message.content->title.c_str(),
               El::String::Manip::UAC_XML_1_0))
          {
            message.content->title = "";
          }
          
          if(!El::String::Manip::utf8_valid(
               message.content->description.c_str(),
               El::String::Manip::UAC_XML_1_0))
          {
            message.content->description = "";
          }

          if(!El::String::Manip::utf8_valid(
               message.content->keywords.c_str(),
               El::String::Manip::UAC_XML_1_0))
          {
            message.content->keywords = "";
          }

          Message::RawImageArray* images = message.content->images.get();

          if(images)
          {
            for(size_t i = 0; i < images->size(); i++)
            {
              Message::RawImage& img = (*images)[i];
              
              if(!El::String::Manip::utf8_valid(
                   img.alt.c_str(),
                   El::String::Manip::UAC_XML_1_0))
              {
                img.alt = "";
              }
            }
          }
        }
      }
      
      const Transport::RawMessagePackImpl::MessageArray& entities =
        pack->entities();

      if(Application::will_trace(El::Logging::HIGH))
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::ManagingMessages::process_messages: "
          "global messages:";
        
        for(Transport::RawMessagePackImpl::MessageArray::const_iterator it =
              entities.begin(); it != entities.end(); it++)
        {
          const RawMessage& message = *it;

          ostr << "\n  id:" << message.id.string()
               << "\n  published: " << message.published
               << "\n  title: " << message.content->title
               << "\n  url: " << message.content->url 
               << "\n  description: " << message.content->description
               << "\n  keywords: " << message.content->keywords
               << "\n  language: " << message.lang
               << "\n  country: " << message.country
               << "\n  source_url: " << message.source_url 
               << "\n  source_id: " << message.source_id 
               << "\n  source.title: " << message.content->source.title
               << "\n  source.html_link: "
               << message.content->source.html_link;

          const Message::RawImageArray* images =
            message.content->images.get();

          if(images)
          {
            for(size_t i = 0; i < images->size(); i++)
            {
              const Message::RawImage& img = (*images)[i];
              
              ostr << "\n  image: " << img.width << "x" << img.height << " "
                   << img.src << " : " << img.alt;
            }
          }
        }
        
        Application::logger()->trace(
          ostr.str(),
          reason >= PMR_NEW_MESSAGE_SHARING ?
          Aspect::MSG_SHARING : Aspect::MSG_MANAGEMENT,
          El::Logging::HIGH);
      }

      if(entities.empty())
      {
        return;
      }

      ACE_Time_Value segmentation_time;
      ACE_Time_Value breakdown_time;
      ACE_High_Res_Timer timer;
      
      bool segmentation_enabled = false;
      
      if(Application::will_trace(El::Logging::HIGH))
      {        
        timer.start();
      }
      
      Segmentation::Transport::SegmentedMessagePackImpl::Var
        segmented_message_components =
        segment_message_components(entities, segmentation_enabled);

      if(Application::will_trace(El::Logging::HIGH))
      {        
        timer.stop();  
        timer.elapsed_time(segmentation_time);
        
        timer.start();
      }

      bool processing_failed = false;

      {
        WriteGuard guard(srv_lock_);
        
        segmentor_failed_ =
          segmentation_enabled && segmented_message_components.in() == 0;

        processing_failed = segmentor_failed_;
      }

      if(!processing_failed)
      { 
        StoredMessageList messages;
        Segmentation::Transport::SegmentedMessageArray* segmented_messages = 0;
          
        if(segmented_message_components.in())
        {
          segmented_messages = &segmented_message_components->entities();
        }

        size_t ent_count = entities.size();
        
        for(size_t i = 0; i < ent_count; i++)
        {
          const RawMessage& message = entities[i];

          StoredContent_var content = new StoredContent();
          
          content->url = message.content->url;
          content->source_html_link = message.content->source.html_link;

          messages.push_back(StoredMessage());
          StoredMessageList::reverse_iterator mit = messages.rbegin();

          StoredMessage& stored_msg = *mit;

          stored_msg.id = message.id;
          stored_msg.space = message.space;
          stored_msg.set_source_url(message.source_url.c_str());
          stored_msg.source_id = message.source_id;
          stored_msg.published = message.published;
          stored_msg.content = content.retn();
          stored_msg.country = message.country;
          stored_msg.lang = message.lang;
          stored_msg.source_title = message.content->source.title;
          stored_msg.set_url_signature(message.content->url.c_str());

          if(segmented_messages)
          {
            Segmentation::Transport::SegmentedMessage& seg_msg =
              (*segmented_messages)[i];

            SegmentationInfo seg_info;
              
            const char* seg_title = seg_msg.title.value.c_str();
            seg_info.title = seg_msg.title.inserted_spaces;
              
            const char* seg_desc = seg_msg.description.value.c_str();
            seg_info.description = seg_msg.description.inserted_spaces;

            const char* seg_keywords = seg_msg.keywords.value.c_str();
            seg_info.keywords = seg_msg.keywords.inserted_spaces;

            RawImageArrayPtr segmented_images;
            unsigned long images_count = seg_msg.images.size();

            if(images_count)
            {
              segmented_images.reset(new RawImageArray(images_count));
              seg_info.images.resize(images_count);
                
              for(size_t i = 0; i < images_count; i++)
              {
                RawImage& image = (*segmented_images)[i];
                image = (*message.content->images)[i];

                El::String::LightString& alt = seg_msg.images[i].alt.value;
                seg_info.images[i] = seg_msg.images[i].alt.inserted_spaces;

                if(alt.c_str() != 0)
                {
                  image.alt.reset(alt.release());
                }
              }
            }
            
            stored_msg.break_down(
              seg_title ? seg_title : message.content->title.c_str(),
              seg_desc ? seg_desc : message.content->description.c_str(),
              segmented_images.get() ?
              segmented_images.get() : message.content->images.get(),
              seg_keywords ? seg_keywords : message.content->keywords.c_str(),
              &seg_info);
          }
          else
          {
            stored_msg.break_down(message.content->title.c_str(),
                                  message.content->description.c_str(),
                                  message.content->images.get(),
                                  message.content->keywords.c_str());
          }
        }

        if(Application::will_trace(El::Logging::HIGH))
        {        
          timer.stop();
          timer.elapsed_time(breakdown_time);

          std::ostringstream ostr;
          
          ostr << "NewsGate::Message::ManagingMessages::process_messages: "
               << ent_count << " messages segmentation time "
               << El::Moment::time(segmentation_time)
               << ", breakdown time " << El::Moment::time(breakdown_time);
          
          Application::logger()->trace(ostr.str(),
                                       Aspect::DB_PERFORMANCE,
                                       El::Logging::HIGH);          
        }
        
        try
        {
          MessageManager_var manager = message_manager();

          set_fetched_time(messages);
          manager->insert(messages, reason);

          WriteGuard guard(srv_lock_);
          word_manager_failed_ = false;
        }
        catch(const MessageManager::WordManagerNotReady& e)
        {
          {
            WriteGuard guard(srv_lock_);
            word_manager_failed_ = true;
          }
          
          processing_failed = true;
          
          std::ostringstream ostr;
          ostr << "NewsGate::Message::ManagingMessages::process_messages: "
            "MessageManager::WordManagerNotReady caught. Description:\n"
               << e;
          
          El::Service::Error
            error(ostr.str(), this, El::Service::Error::NOTICE);
          
          callback_->notify(&error);
        }
        catch(const El::Exception& e)
        {
          processing_failed = true;
          
          std::ostringstream ostr;
          ostr << "NewsGate::Message::ManagingMessages::process_messages: "
            "El::Exception caught. Description:\n" << e;
          
          El::Service::Error
            error(ostr.str(), this, El::Service::Error::NOTICE);
          
          callback_->notify(&error);
        }
      }      
      
      if(processing_failed)
      {
        El::Service::CompoundServiceMessage_var msg = new AcceptMessages(this);

        WriteGuard guard(srv_lock_);
        pending_message_packs_.push_back(PendingMessagePack(pack, reason));
        
        deliver_at_time(
          msg.in(),
          ACE_OS::gettimeofday() +
          ACE_Time_Value(Application::instance()->config().word_management().
                         retry_period()));
      }
    }

    void
    ManagingMessages::process_messages(
      Transport::StoredMessagePackImpl::Type* pack,
      ::NewsGate::Message::PostMessageReason reason)
      throw(Exception, El::Exception, CORBA::Exception)
    {
      const Transport::StoredMessagePackImpl::MessageArray& entities =
        pack->entities();

      if(Application::will_trace(El::Logging::HIGH))
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::ManagingMessages::process_messages: "
          "stored messages:\n";
        
        for(Transport::StoredMessagePackImpl::MessageArray::const_iterator it =
              entities.begin(); it != entities.end(); it++)
        {
          const StoredMessage& message = it->message;

          std::ostringstream tstr;
          Message::StoredMessage::DefaultMessageBuilder title_builder(tstr);
          message.assemble_title(title_builder);

          std::ostringstream dstr;
          Message::StoredMessage::DefaultMessageBuilder desc_builder(dstr);
          message.assemble_description(desc_builder);
          
          ostr << "  title: " << tstr.str()
               << "\n  url: " << message.content->url 
               << "\n  description: " << dstr.str()
               << "\n  language: " << message.lang
               << "\n  country: " << message.country
               << "\n  id: " << message.id.string()
               << "\n  published: " << message.published
               << "\n  source_url: " << message.source_url
               << "\n  source_id: " << message.source_id
               << "\n  source.title: " << message.source_title
               << "\n  source.html_link: "
               << message.content->source_html_link
               << std::endl;

          const Message::StoredImageArray* images =
            message.content->images.get();

          if(images)
          {
            for(size_t i = 0; i < images->size(); i++)
            {
              const Message::StoredImage& img = (*images)[i];
              
              ostr << "  image: " << img.width << "x" << img.height << " "
                   << img.src << " : ";

              Message::StoredMessage::DefaultMessageBuilder builder(ostr);
              message.assemble_image_alt(builder, i);

              ostr << std::endl;
            }
          }

          ostr << std::endl;
        }
        
        Application::logger()->trace(
          ostr.str(),
          reason >= PMR_NEW_MESSAGE_SHARING ?
          Aspect::MSG_SHARING : Aspect::MSG_MANAGEMENT,
          El::Logging::HIGH);
      }      

      if(entities.empty())
      {
        return;
      }

      StoredMessageList messages;
      
      for(Transport::StoredMessagePackImpl::MessageArray::const_iterator it =
            entities.begin(); it != entities.end(); it++)
      {
        messages.push_back(it->message);

        if(has_event_bank_)
        {
          StoredMessage& msg = *messages.rbegin();
          
          msg.event_id = El::Luid::null;
          msg.event_capacity = 0;
        }
      }

      bool processing_failed = false;
      
      try
      {
        MessageManager_var manager = message_manager();

// Keep original fetch time        
//      set_fetched_time(messages);
        
        manager->insert(messages, reason);

        WriteGuard guard(srv_lock_);
        word_manager_failed_ = false;
      }
      catch(const MessageManager::WordManagerNotReady& e)
      {
        {
          WriteGuard guard(srv_lock_);
          word_manager_failed_ = true;
        }
        
        processing_failed = true;
        
        std::ostringstream ostr;
        ostr << "NewsGate::Message::ManagingMessages::process_messages: "
          "MessageManager::WordManagerNotReady caught. Description:\n"
             << e;
        
        El::Service::Error error(ostr.str(),
                                 this,
                                 El::Service::Error::NOTICE);
        
        callback_->notify(&error);
      }
      catch(const El::Exception& e)
      {
        processing_failed = true;
        
        std::ostringstream ostr;
        ostr << "NewsGate::Message::ManagingMessages::process_messages: "
          "El::Exception caught. Description:\n" << e;
        
        El::Service::Error
          error(ostr.str(), this, El::Service::Error::NOTICE);
        
        callback_->notify(&error);
      }
      
      if(processing_failed)
      {
        El::Service::CompoundServiceMessage_var msg = new AcceptMessages(this);
        
        WriteGuard guard(srv_lock_);
        pending_message_packs_.push_back(PendingMessagePack(pack, reason));

        deliver_at_time(
          msg.in(),
          ACE_OS::gettimeofday() +
          ACE_Time_Value(Application::instance()->config().word_management().
                         retry_period()));
      }
    }

    void
    ManagingMessages::process_messages(
      Transport::MessageEventPackImpl::Type* pack,
      ::NewsGate::Message::PostMessageReason reason)
      throw(Exception, El::Exception, CORBA::Exception)
    {
      if(reason == PMR_EVENT_UPDATE || !has_event_bank_)
      {
        MessageManager_var manager = message_manager();
        manager->process_message_events(pack, reason);
      }
    }
        
    void
    ManagingMessages::accept_cached_messages(AcceptCachedMessages* acm)
      throw(Exception, El::Exception)
    {
      const Server::Config::MessageBankType& config =
        Application::instance()->config();

      const Server::Config::BankMessageManagerType::message_import_type&
        imp_cfg = config.message_manager().message_import();      

      if(!acm->pending_filename.empty())
      {
        ACE_Time_Value delay;
        PendingMessagePack& pack = acm->pending_pack;
        
        if(check_accept_package_readiness(false, pack.pack.in(), pack.reason))
        {
          {
            WriteGuard guard(srv_lock_);
            pending_message_packs_.push_back(pack);
          }

          unlink(acm->pending_filename.c_str());
          acm->pending_filename.clear();

          if(Application::will_trace(El::Logging::MIDDLE))
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Message::ManagingMessages::"
              "accept_cached_messages: " << pack.message_count()
                 << " cached messages accepted with delay";
            
            Application::logger()->trace(ostr.str(),
                                         Aspect::STATE,
                                         El::Logging::MIDDLE);
          }
          
          pack = PendingMessagePack();
        }
        else
        {
          delay = ACE_Time_Value(imp_cfg.on_notready_retry_period());
                
          if(Application::will_trace(El::Logging::MIDDLE))
          {
            std::ostringstream ostr;
            ostr << "NewsGate::Message::ManagingMessages::"
              "accept_cached_messages: repeatedly not ready to accept "
                 << pack.message_count() << " cached messages; will retry in "
                 << El::Moment::time(delay);
            
            Application::logger()->trace(ostr.str(),
                                         Aspect::STATE,
                                         El::Logging::MIDDLE);
          }
        }

        deliver_at_time(acm, ACE_OS::gettimeofday() + delay);
        return;
      }
      
      bool error = false;
      ACE_Time_Value delay(imp_cfg.check_period());
      
      MessageManager_var manager = message_manager();

      if(manager->loaded())
      {      
        std::string cache_dir_name = config.message_manager().cache_file_dir();

        try
        {
          El::FileSystem::DirectoryReader finder(cache_dir_name.c_str());
          cache_dir_name += "/";
        
          for(size_t i = 0; i < finder.count(); ++i)
          {
            const dirent& entry = finder[i];
          
            if(entry.d_type == DT_DIR)
            {
              continue;
            }

            const char* ext = strrchr(entry.d_name, '.');

            if(ext == 0 || strcmp(ext + 1, "mpk"))
            {
              continue;
            }

            ACE_High_Res_Timer timer;
            
            if(Application::will_trace(El::Logging::HIGH))
            {        
              timer.start();
            }
            
            std::string filename = cache_dir_name + entry.d_name;
            std::fstream file(filename.c_str(), std::ios::in);

            if(!file.is_open())
            {
              std::ostringstream ostr;
              ostr << "NewsGate::Message::MessageManager::"
                "accept_cached_messages: failed to open file '" << filename
                   << "' for read access";
            
              El::Service::Error err(ostr.str(),
                                     this,
                                     El::Service::Error::CRITICAL);
            
              callback_->notify(&err);
              error = true;
            
              continue;
            }
      
            PendingMessagePack pack;

            try
            {
              El::BinaryInStream bstr(file);
              bstr >> pack;
              file.close(); 

              timer.stop();
              
              ACE_Time_Value time;
              timer.elapsed_time(time);

              if(!check_accept_package_readiness(false,
                                                 pack.pack.in(),
                                                 pack.reason))
              {
                acm->pending_pack = pack;
                acm->pending_filename = filename;
                
                delay = ACE_Time_Value(imp_cfg.on_notready_retry_period());
                
                if(Application::will_trace(El::Logging::MIDDLE))
                {
                  std::ostringstream ostr;
                  ostr << "NewsGate::Message::ManagingMessages::"
                    "accept_cached_messages: not ready to accept "
                       << pack.message_count()
                       << " cached messages (read time "
                       << El::Moment::time(time) << "); will retry in "
                       << El::Moment::time(delay);
            
                  Application::logger()->trace(ostr.str(),
                                               Aspect::STATE,
                                               El::Logging::MIDDLE);
                }
                
                break;
              }
              
              {
                WriteGuard guard(srv_lock_);
                pending_message_packs_.push_back(pack);
              }

              if(Application::will_trace(El::Logging::MIDDLE))
              {
                std::ostringstream ostr;
                ostr << "NewsGate::Message::ManagingMessages::"
                  "accept_cached_messages: " << pack.message_count()
                     << " cached messages accepted (read time "
                       << El::Moment::time(time) << ")";
                
                Application::logger()->trace(ostr.str(),
                                             Aspect::STATE,
                                             El::Logging::MIDDLE);
              }
              
              El::Service::CompoundServiceMessage_var msg =
                new AcceptMessages(this);
          
              deliver_now(msg.in());
            }
            catch(const El::BinaryInStream::Exception& e)
            {
              std::ostringstream ostr;
              ostr << "NewsGate::Message::MessageManager::"
                "accept_cached_messages: El::BinaryInStream::Exception "
                "caught. Description:\n" << e;
            
              El::Service::Error err(ostr.str(),
                                     this,
                                     El::Service::Error::CRITICAL);
            
              callback_->notify(&err);
              error = true;
            }
            catch(const PendingMessagePack::Exception& e)
            {
              std::ostringstream ostr;
              ostr << "NewsGate::Message::MessageManager::"
                "accept_cached_messages: PendingMessagePack::Exception "
                "caught. Description:\n" << e;
            
              El::Service::Error err(ostr.str(),
                                     this,
                                     El::Service::Error::CRITICAL);
            
              callback_->notify(&err);
              error = true;
            }

            file.close(); 
            unlink(filename.c_str());
         
            delay = ACE_Time_Value::zero;
            break;
          }
          
        }
        catch(const El::FileSystem::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::MessageManager::accept_cached_messages: "
            "El::FileSystem::Exception caught. Description:\n" << e;
        
          El::Service::Error err(ostr.str(),
                                 this,
                                 El::Service::Error::CRITICAL);
        
          callback_->notify(&err);
          error = true;
        }
      }
      
      if(error)
      {
        delay = ACE_Time_Value(imp_cfg.on_error_retry_period());
      }
      
      deliver_at_time(acm, ACE_OS::gettimeofday() + delay);
    }
      
    void
    ManagingMessages::wait() throw(Exception, El::Exception)
    {
      BankState::wait();

      try
      {
        Message::BankManager_var bank_manager =
          Application::instance()->bank_manager();

        bank_manager->terminate(Application::instance()->bank_ior(),
                                session_->id());
      }
      catch(...)
      {
      }      

      size_t pending_pack_count = 0;
      size_t pending_message_count = 0;
      
      MessageManager_var manager = message_manager();      
      
      {
        WriteGuard guard(srv_lock_);
      
        while(!pending_message_packs_.empty())
        {
          PendingMessagePack pending_pack = pending_message_packs_.pop_front();
          size_t count = pending_pack.message_count();
          
          if(count)
          {
            manager->save_pack(pending_pack, false);
            
            pending_message_count += count;
            ++pending_pack_count;
          }
        }
      }

      std::ostringstream ostr;
      ostr << "NewsGate::Message::ManagingMessages::wait: "
           << pending_message_count << " pending messages in "
           << pending_pack_count << " packs saved for the better times.";

      size_t tasks = thread_pool_->queue_size();
      
      if(tasks)
      {
        ostr << "\nWorker thread queue contain " << tasks << " task(s)";
        
        Application::logger()->notice(ostr.str(), Aspect::STATE);
      }
      else
      {
        Application::logger()->trace(ostr.str(),
                                     Aspect::STATE,
                                     El::Logging::LOW);
      }
    }
    
    void
    ManagingMessages::set_fetched_time(StoredMessageList& messages)
      throw(El::Exception)
    {
      time_t time = ACE_OS::gettimeofday().sec();

      for(StoredMessageList::iterator it = messages.begin();
          it != messages.end(); it++)
      {
        it->fetched = time;
      }
    }
    
    bool
    ManagingMessages::notify(El::Service::Event* event)
      throw(El::Exception)
    {
      ACE_High_Res_Timer timer;
            
      if(Application::will_trace(El::Logging::HIGH))
      {        
        timer.start();
      }
      
      std::string type = process_event(event);

      if(Application::will_trace(El::Logging::HIGH))
      {        
        timer.stop();
        
        ACE_Time_Value time;
        timer.elapsed_time(time);
        
        std::ostringstream ostr;
        
        ostr << "NewsGate::Message::ManagingMessages::notify: event "
             << type << "; time " << El::Moment::time(time)
             << "; " << task_queue_size() << " in queue";
        
        Application::logger()->trace(ostr.str(),
                                     Aspect::MSG_MANAGEMENT,
                                     El::Logging::HIGH);
      }
      
      return true;
    }

    std::string
    ManagingMessages::process_event(El::Service::Event* event)
      throw(El::Exception)
    {
      if(BankState::notify(event))
      {
        return "base_notify";
      }
      
      if(dynamic_cast<ReportPresence*>(event) != 0)
      {
        report_presence();
        return "report_presense";
      }
      
      AcceptMessages* am = dynamic_cast<AcceptMessages*>(event);
      
      if(am != 0)
      {
        accept_messages(am);
        return "accept_messages";
      }

      SetMessageFetchFilter* smf = dynamic_cast<SetMessageFetchFilter*>(event);
      
      if(smf != 0)
      {
        set_message_fetch_filter(smf->message_filter, smf->retry);
        return "set_message_fetch_filter";
      }

      SetMessageCategorizer* smc = dynamic_cast<SetMessageCategorizer*>(event);
      
      if(smc != 0)
      {
        set_message_categorizer(smc->message_categorizer, smc->retry);
        return "set_message_categorizer";
      }
      
      AcceptCachedMessages* acm = dynamic_cast<AcceptCachedMessages*>(event);
      
      if(acm != 0)
      {
        accept_cached_messages(acm);
        return "accept_cached_messages";
      }

      ExitApplication* ea = dynamic_cast<ExitApplication*>(event);
      
      if(ea != 0)
      {
        std::ostringstream ostr;
        ostr  << "SharedStringManager info:";
        
        StringConstPtr::string_manager.dump(ostr, 100);
        std::cerr << ostr.str();

        exit(1);
        return "";
      }

      std::ostringstream ostr;
      ostr << "NewsGate::Message::ManagingMessages::notify: unknown "
           << *event;
      
      El::Service::Error error(ostr.str(), this);
      callback_->notify(&error);
      
      return "unknown";
    }
  }
}
