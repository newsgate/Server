/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Commons/Statistics/StatLogger.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <El/CORBA/Corba.hpp>

#include <google/dense_hash_map>

#include <sstream>
#include <ext/hash_map>

#include <El/Exception.hpp>
#include <El/Net/HTTP/URL.hpp>
#include <El/Net/HTTP/Utility.hpp>

#include <Commons/Message/Message.hpp>
#include <Commons/Message/TransportImpl.hpp>

#include <Services/Commons/FraudPrevention/FraudPreventionServices.hpp>
#include <Services/Commons/FraudPrevention/TransportImpl.hpp>
#include <Services/Commons/Statistics/StatisticsServices.hpp>
#include <Services/Commons/Statistics/TransportImpl.hpp>

#include "StatLogger.hpp"

namespace NewsGate
{
  namespace Statistics
  {
    //
    // MessageStatMap struct
    //
    class MessageStatMap :
      public google::dense_hash_map<Message::Id,
                                    Message::Transport::MessageStat,
                                    Message::MessageIdHash>
    {
    public:
      MessageStatMap() throw(El::Exception);
    };    
    
    MessageStatMap::MessageStatMap() throw(El::Exception)
    {
      set_deleted_key(Message::Id::zero);
      set_empty_key(Message::Id::nonexistent);
    }

    //
    // EventStatMap struct
    //
    class EventStatMap :
      public google::dense_hash_map<El::Luid,
                                    Message::Transport::EventStat,
                                    El::Hash::Luid>
    {
    public:
      EventStatMap() throw(El::Exception);
    };    
    
    EventStatMap::EventStatMap() throw(El::Exception)
    {
      set_deleted_key(El::Luid::null);
      set_empty_key(El::Luid::nonexistent);
    }

    //
    // MessageStatInfoMap class
    //
    typedef __gnu_cxx::hash_map<Message::Id,
                                Message::Transport::MessageStatInfo,
                                Message::MessageIdHash>
    MessageStatInfoMap;
    
    //
    // StatLogger class
    //
    StatLogger::StatLogger(
      const char* stat_processor_ref,
      const char* limit_checker_ref,
      const FraudPrevention::EventLimitCheckDescArray& limit_check_descriptors,
      const IpSet& limit_check_ip_whitelist,
      CORBA::ORB_ptr orb,
      time_t flush_period,
      El::Service::Callback* callback)
      throw(Exception, El::Exception)
        : El::Service::CompoundService<>(callback, "StatLogger"),
          limit_check_descriptors_(limit_check_descriptors),
          limit_check_ip_whitelist_(limit_check_ip_whitelist),
          flush_period_(flush_period)
    {
      try
      {
        Message::Transport::register_valuetype_factories(orb);
        stat_processor_ = StatProcessorRef(stat_processor_ref, orb);

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
        ostr << "NewsGate::Statistics::StatLogger::StatLogger: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
      
      El::Service::CompoundServiceMessage_var msg = new Flush(this);
      deliver_at_time(msg.in(), ACE_OS::gettimeofday() + flush_period_);
    }
    
    void
    StatLogger::search_request(const RequestInfo& request)
      throw(Exception, El::Exception)
    {
      Guard guard(lock_);
    
      if(requests_.get() == 0)
      {
        requests_.reset(new RequestInfoArray());
        requests_->reserve(100 * (flush_period_.sec() + 1));
      }

      requests_->push_back(request);
    }

    void
    StatLogger::page_impression(const PageImpressionInfo& impression)
      throw(Exception, El::Exception)
    {
      Guard guard(lock_);

      if(page_impressions_.get() == 0)
      {
        page_impressions_.reset(new PageImpressionInfoArray());
        page_impressions_->reserve(100 * (flush_period_.sec() + 1));
      }

      page_impressions_->push_back(impression);
    }
    
    void
    StatLogger::message_impression(const MessageImpressionInfo& impression)
      throw(Exception, El::Exception)
    {
      if(!impression.messages.empty())
      {
        Guard guard(lock_);
    
        if(message_impressions_.get() == 0)
        {
          message_impressions_.reset(new MessageImpressionInfoArray());
          message_impressions_->reserve(100 * (flush_period_.sec() + 1));
        }

        message_impressions_->push_back(impression);
      }
    }

    void
    StatLogger::message_click(const MessageClickInfo& click)
      throw(Exception, El::Exception)
    {
      if(!click.messages.empty())
      {
        Guard guard(lock_);
    
        if(message_clicks_.get() == 0)
        {
          message_clicks_.reset(new MessageClickInfoArray());
          message_clicks_->reserve(100 * (flush_period_.sec() + 1));
        }

        message_clicks_->push_back(click);
      }
    }
    
    void
    StatLogger::message_visit(const MessageVisitInfo& visit)
      throw(Exception, El::Exception)
    {
      if(!visit.messages.empty() || !visit.events.empty())
      {
        Guard guard(lock_);
    
        if(message_visits_.get() == 0)
        {
          message_visits_.reset(new MessageVisitInfoArray());
          message_visits_->reserve(100 * (flush_period_.sec() + 1));
        }

        message_visits_->push_back(visit);
      }
    }
    
    bool
    StatLogger::notify(El::Service::Event* event) throw(El::Exception)
    {
      if(El::Service::CompoundService<>::notify(event))
      {
        return true;
      }

      if(dynamic_cast<Flush*>(event) != 0)
      {
        flush("StatLogger::notify");
        return true;
      }

      return false;
    }

    void
    StatLogger::flush(const char* caller) throw(Exception, El::Exception)
    {
      RequestInfoArrayPtr requests;
      PageImpressionInfoArrayPtr page_impressions;
      MessageImpressionInfoArrayPtr message_impressions;
      MessageClickInfoArrayPtr message_clicks;
      MessageVisitInfoArrayPtr message_visits;
        
      {
        Guard guard(lock_);
    
        requests.reset(requests_.release());
        page_impressions.reset(page_impressions_.release());
        message_impressions.reset(message_impressions_.release());
        message_clicks.reset(message_clicks_.release());
        message_visits.reset(message_visits_.release());
      }

      try
      {
        if(message_clicks.get() && !limit_checker_.empty())
        {
          MessageClickInfoArrayPtr clicks(message_clicks.release());
          
          ::NewsGate::FraudPrevention::LimitChecker_var limit_checker =
            limit_checker_.object();

          FraudPrevention::Transport::EventLimitCheckPackImpl::Var
            check_pack =
            FraudPrevention::Transport::EventLimitCheckPackImpl::Init::create(
              new FraudPrevention::EventLimitCheckArray());

          FraudPrevention::EventLimitCheckArray& checks =
            check_pack->entities();

          for(MessageClickInfoArray::const_iterator cit(clicks->begin()),
                cie(clicks->end()); cit != cie; ++cit)
          {
            const MessageClickInfo& click_info = *cit;
            
            if(limit_check_ip_whitelist_.find(click_info.ip) !=
               limit_check_ip_whitelist_.end())
            {
              continue;
            }
              
            const Message::Transport::MessageStatInfoArray& messages =
              cit->messages;
            
            for(Message::Transport::MessageStatInfoArray::const_iterator
                  it(messages.begin()), ie(messages.end()); it != ie; ++it)
            {
              const Message::Id& id = it->id;

              for(FraudPrevention::EventLimitCheckDescArray::const_iterator
                    i(limit_check_descriptors_.begin()),
                    e(limit_check_descriptors_.end()); i != e; ++i)
              {
                const FraudPrevention::EventLimitCheckDesc& desc = *i;
                
                if(desc.user && *click_info.client_id == '\0')
                {
                  continue;
                }
                
                FraudPrevention::EventLimitCheck
                  check((uint64_t)0, it->count, desc.times, desc.interval);

                check.update_event((const unsigned char*)&desc.type,
                                   sizeof(desc.type));

                if(desc.user)
                {
                  check.update_event((const unsigned char*)"U:", 2);
                    
                  check.update_event(
                    (const unsigned char*)click_info.client_id,
                    strlen(click_info.client_id));
                }
                  
                if(desc.ip)
                {
                  check.update_event((const unsigned char*)"I:", 2);
                    
                  check.update_event((const unsigned char*)click_info.ip,
                                     strlen(click_info.ip));
                }

                if(desc.item)
                {
                  check.update_event((const unsigned char*)"M:", 2);
                    
                  check.update_event(
                    (const unsigned char*)id.string().c_str(),
                    id.string().length());
                }

                checks.push_back(check);                
              }
            }
          }
          
          size_t check_count = checks.size();

          if(check_count)
          {
            FraudPrevention::Transport::EventLimitCheckResultPack_var res = 
              limit_checker->check(
                FraudPrevention::LimitChecker::INTERFACE_VERSION,
                check_pack.in());
          
            FraudPrevention::Transport::EventLimitCheckResultPackImpl::Type*
              res_impl = dynamic_cast<FraudPrevention::Transport::
              EventLimitCheckResultPackImpl::Type*>(res.in());
          
            if(res_impl == 0)
            {
              std::ostringstream ostr;
              ostr << "NewsGate::Statistics::StatLogger::flush(" << caller
                   << "): dynamic_cast<FraudPrevention::Transport::"
                "EventLimitCheckResultPackImpl::Type*> failed";
              
              El::Service::Error error(ostr.str().c_str(), this);
              callback_->notify(&error);
            }

            message_clicks.reset(new MessageClickInfoArray());
            message_clicks->reserve(clicks->size());

            const FraudPrevention::EventLimitCheckResultArray& check_results =
              res_impl->entities();
            
            assert(check_results.size() == check_count);

            FraudPrevention::EventLimitCheckResultArray::const_iterator
              cri(check_results.begin());

            for(MessageClickInfoArray::const_iterator cit(clicks->begin()),
                  cie(clicks->end()); cit != cie; ++cit)
            {
              MessageClickInfo mci = *cit;

              if(limit_check_ip_whitelist_.find(mci.ip) !=
                 limit_check_ip_whitelist_.end())
              {
                continue;
              }              
            
              Message::Transport::MessageStatInfoArray& dest_messages =
                mci.messages;
            
              size_t size = dest_messages.size();
              dest_messages.clear();
              dest_messages.reserve(size);
            
//            const char* client_id = mci.client_id;
              
              const Message::Transport::MessageStatInfoArray& messages =
                cit->messages;
            
              for(Message::Transport::MessageStatInfoArray::const_iterator
                    it(messages.begin()), ie(messages.end()); it != ie; ++it)
              {
                bool exeeded = false;
              
                for(FraudPrevention::EventLimitCheckDescArray::const_iterator
                      i(limit_check_descriptors_.begin()),
                      e(limit_check_descriptors_.end()); i != e; ++i)
                {
                  const FraudPrevention::EventLimitCheckDesc& desc = *i;
                
                  if(desc.user && *mci.client_id == '\0')
                  {
                    continue;
                  }

                  exeeded |= cri++->limit_exceeded;
                }

                if(!exeeded)
                {
                  dest_messages.push_back(*it);
                }
              }

              if(!dest_messages.empty())
              {
                message_clicks->push_back(mci);
              }
            }
          
            if(!message_clicks->size())
            {
              message_clicks.reset(0);
            }
          }
          else
          {
            message_clicks.reset(clicks.release());
          }
        }
      }
      catch(const ::NewsGate::FraudPrevention::IncompatibleVersion& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Statistics::StatLogger::flush(" << caller
             << "): ::NewsGate::FraudPrevention::IncompartibleVersion "
          "caught (" << e.current_version << ")";
        
        El::Service::Error error(ostr.str().c_str(), this);
        callback_->notify(&error);
      }
      catch(const ::NewsGate::FraudPrevention::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Statistics::StatLogger::flush(" << caller
             << "): ::NewsGate::FraudPrevention::ImplementationException "
          "caught. Description:\n" << e.description.in();
        
        El::Service::Error error(ostr.str().c_str(), this);
        callback_->notify(&error);
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Statistics::StatLogger::flush(" << caller
             << "): for ::NewsGate::FraudPrevention::LimitChecker::check "
          "CORBA::Exception caught. Description:\n" << e;
          
        El::Service::Error error(ostr.str().c_str(), this);
        callback_->notify(&error);
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Statistics::StatLogger::flush(" << caller
             << "): for ::NewsGate::FraudPrevention::LimitChecker::check "
          "El::Exception caught. Description:\n" << e;
          
        El::Service::Error error(ostr.str().c_str(), this);
        callback_->notify(&error);
      }
        
      try
      {
        if(requests.get() || page_impressions.get() ||
           message_impressions.get() || message_clicks.get() ||
           message_visits.get())
        {
          StatProcessor_var stat_processor = stat_processor_.object();

          if(requests.get())
          {
            Transport::RequestInfoPackImpl::Var pack =
              new Transport::RequestInfoPackImpl::Type(requests.release());
            
            stat_processor->search_request(StatProcessor::INTERFACE_VERSION,
                                           pack.in());
          }

          if(page_impressions.get())
          {
            Transport::PageImpressionInfoPackImpl::Var pack =
              new Transport::PageImpressionInfoPackImpl::Type(
                page_impressions.release());
            
            stat_processor->page_impression(StatProcessor::INTERFACE_VERSION,
                                            pack.in());
          }

          if(message_impressions.get() || message_clicks.get() ||
             message_visits.get())
          {
            Message::BankClientSession_var session;
            
            try
            {
              session = bank_client_session();
            }
            catch(const Exception& e)
            {
              std::ostringstream ostr;                
              ostr << "NewsGate::Statistics::StatLogger::flush(" << caller
                   << "): bank_client_session failed. Reason:\n" << e;
                
              El::Service::Error error(ostr.str().c_str(), this);
              callback_->notify(&error);
            }

            if(session.in() != 0)
            {
              MessageStatMap message_stat_map;
              EventStatMap event_stat_map;
                
              if(message_impressions.get())
              {
                const MessageImpressionInfoArray& imp = *message_impressions;
                
                for(MessageImpressionInfoArray::const_iterator iit =
                      imp.begin(); iit != imp.end(); iit++)
                {
                  const Message::Transport::MessageStatInfoArray& messages =
                    iit->messages;
                
                  for(Message::Transport::MessageStatInfoArray::const_iterator
                        it = messages.begin(); it != messages.end(); it++)
                  {
                    const Message::Id& id = it->id;

                    if(id == Message::Id::zero ||
                       id == Message::Id::nonexistent)
                    {
                      continue;
                    }                    
                    
                    MessageStatMap::iterator mit = message_stat_map.find(id);
                    
                    if(mit == message_stat_map.end())
                    {
                      mit = message_stat_map.insert(
                        std::make_pair(id,
                                       Message::Transport::MessageStat(id))).
                        first;
                    }
                    
                    mit->second.impressions += it->count;
                  }
                }
              }

              if(message_clicks.get())
              {
                const MessageClickInfoArray& clk = *message_clicks;

                for(MessageClickInfoArray::const_iterator cit = clk.begin();
                    cit != clk.end(); cit++)
                {
                  const Message::Transport::MessageStatInfoArray& messages =
                    cit->messages;

                  for(Message::Transport::MessageStatInfoArray::const_iterator
                        it = messages.begin(); it != messages.end(); it++)
                  {
                    const Message::Id& id = it->id;

                    if(id == Message::Id::zero ||
                       id == Message::Id::nonexistent)
                    {
                      continue;
                    }                    
                    
                    MessageStatMap::iterator mit = message_stat_map.find(id);
                    
                    if(mit == message_stat_map.end())
                    {
                      mit = message_stat_map.insert(
                        std::make_pair(id,
                                       Message::Transport::MessageStat(id))).
                        first;
                    }
                    
                    mit->second.clicks += it->count;
                  }
                }
              }

              if(message_visits.get())
              {
                const MessageVisitInfoArray& vst = *message_visits;
                
                for(MessageVisitInfoArray::const_iterator vit(vst.begin()),
                      vie(vst.end()); vit != vie; ++vit)
                {
                  uint64_t time = vit->time;
                  const NewsGate::Message::IdArray& messages = vit->messages;

                  for(NewsGate::Message::IdArray::const_iterator
                        it(messages.begin()), ie(messages.end()); it != ie;
                      ++it)
                  {
                    const Message::Id& id = *it;

                    if(id == Message::Id::zero ||
                       id == Message::Id::nonexistent)
                    {
                      continue;
                    }
                    
                    MessageStatMap::iterator mit = message_stat_map.find(id);
                    
                    if(mit == message_stat_map.end())
                    {
                      mit = message_stat_map.insert(
                        std::make_pair(id,
                                       Message::Transport::MessageStat(id))).
                        first;
                    }
                    
                    mit->second.visited = std::max(time, mit->second.visited);
                  }

                  const MessageVisitInfo::EventIdArray& events = vit->events;
                  
                  for(MessageVisitInfo::EventIdArray::const_iterator
                        it(events.begin()), ie(events.end()); it != ie; ++it)
                  {
                    const El::Luid& id = *it;

                    if(id == El::Luid::null || id == El::Luid::nonexistent)
                    {
                      continue;
                    }
                    
                    EventStatMap::iterator eit = event_stat_map.find(id);
                    
                    if(eit == event_stat_map.end())
                    {
                      eit = event_stat_map.insert(
                        std::make_pair(id, Message::Transport::EventStat(id))).
                        first;
                    }
                    
                    eit->second.visited = std::max(time, eit->second.visited);
                  }
                }
              }

              try
              {                
                Message::Transport::MessageStatRequestImpl::Var
                  request = Message::Transport::
                  MessageStatRequestImpl::Init::create(
                    new Message::Transport::MessageStatRequestInfo(
                      message_impressions.get() || message_clicks.get()));
                
                Message::Transport::MessageStatArray& message_stat =
                  request->entity().message_stat;
                
                message_stat.reserve(message_stat_map.size());
                
                for(MessageStatMap::const_iterator i(message_stat_map.begin()),
                      e(message_stat_map.end()); i != e; ++i)
                {
                  message_stat.push_back(i->second);
                }

                Message::Transport::EventStatArray& event_stat =
                  request->entity().event_stat;
                
                event_stat.reserve(event_stat_map.size());
                
                for(EventStatMap::const_iterator i(event_stat_map.begin()),
                      e(event_stat_map.end()); i != e; ++i)
                {
                  event_stat.push_back(i->second);
                }

                request->serialize();

                Message::Transport::Response_var response;
                
                Message::BankClientSession::RequestResult_var result =
                  session->send_request(request.in(), response.out());

                Message::Transport::MessageStatResponseImpl::Type*
                  stat_response = 0;

                if(result->code == Message::BankClientSession::RRC_OK)
                {
                  stat_response = dynamic_cast<
                    Message::Transport::MessageStatResponseImpl::Type*>(
                      response.in());
                  
                  if(stat_response == 0)
                  {
                    std::ostringstream ostr;
                    ostr << "NewsGate::Statistics::StatLogger::flush("
                         << caller << "): dynamic_cast<Message::Transport::"
                      "MessageStatResponseImpl::Type*> failed";
                    
                    El::Service::Error error(ostr.str().c_str(), this);
                    callback_->notify(&error);
                  }
                }
                else
                {
                  std::ostringstream ostr;
                  ostr << "NewsGate::Statistics::StatLogger::flush(" << caller
                       << "): send_request failed; code " << result->code
                       << ". Description:\n" << result->description.in();

                  El::Service::Error error(ostr.str().c_str(), this);
                  callback_->notify(&error);                  
                }

                if(stat_response)
                {
                  MessageStatInfoMap message_stat_info_map;
                  
                  const Message::Transport::MessageStatInfoArray&
                    message_stat_infos = stat_response->entities();
                  
                  message_stat_info_map.resize(message_stat_infos.size());

                  for(Message::Transport::MessageStatInfoArray::const_iterator
                        i(message_stat_infos.begin()),
                        e(message_stat_infos.end()); i != e; ++i)
                  {
                    message_stat_info_map[i->id] = *i;
                  }

                  if(message_impressions.get())
                  {
                    MessageImpressionInfoArray& imp = *message_impressions;
                
                    for(MessageImpressionInfoArray::iterator i(imp.begin()),
                          e(imp.end()); i != e; ++i)
                    {
                      Message::Transport::MessageStatInfoArray&
                        messages = i->messages;
                
                      for(Message::Transport::MessageStatInfoArray::
                            iterator i(messages.begin()), e(messages.end());
                          i != e; ++i)
                      {
                        MessageStatInfoMap::const_iterator mit =
                          message_stat_info_map.find(i->id);
                    
                        if(mit != message_stat_info_map.end())
                        {
                          i->source_id = mit->second.source_id;
                        }
                      }
                    }
                  }

                  if(message_clicks.get())
                  {
                    MessageClickInfoArray& clk = *message_clicks;

                    for(MessageClickInfoArray::iterator cit = clk.begin();
                        cit != clk.end(); cit++)
                    {
                      Message::Transport::MessageStatInfoArray&
                        messages = cit->messages;

                      for(Message::Transport::MessageStatInfoArray::
                            iterator it = messages.begin();
                          it != messages.end(); it++)
                      {
                        const Message::Id& id = it->id;

                        MessageStatInfoMap::const_iterator mit =
                          message_stat_info_map.find(id);
                    
                        if(mit != message_stat_info_map.end())
                        {
                          it->source_id = mit->second.source_id;
                        }
                      }
                    }
                  }
                }
              }
              catch(const Message::ImplementationException& e)
              {
                std::ostringstream ostr;
                
                ostr << "NewsGate::Statistics::StatLogger::flush(" << caller
                       << "): Message::ImplementationException caught. "
                  "Description:\n"
                     << e.description.in();
                
                El::Service::Error error(ostr.str().c_str(), this);
                callback_->notify(&error);
              }
              catch(const CORBA::Exception& e)
              {
                std::ostringstream ostr;
                ostr << "NewsGate::Statistics::StatLogger::flush(" << caller
                     << "): CORBA::Exception caught. Description:\n" << e;
                
                El::Service::Error error(ostr.str().c_str(), this);
                callback_->notify(&error);
              }
            }
          }

          if(message_impressions.get())
          {
            Transport::MessageImpressionInfoPackImpl::Var pack =
              new Transport::MessageImpressionInfoPackImpl::Type(
                message_impressions.release());
            
            stat_processor->message_impression(
              StatProcessor::INTERFACE_VERSION,
              pack.in());
          }

          if(message_clicks.get())
          {
            Transport::MessageClickInfoPackImpl::Var pack =
              new Transport::MessageClickInfoPackImpl::Type(
                message_clicks.release());
            
            stat_processor->message_click(StatProcessor::INTERFACE_VERSION,
                                          pack.in());
          }
        }
      }
      catch(const ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Statistics::StatLogger::flush(" << caller
             << "): ::NewsGate::Statistics::ImplementationException caught. "
          "Description:\n" << e.description.in();
        
        El::Service::Error error(ostr.str().c_str(), this);
        callback_->notify(&error);
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Statistics::StatLogger::flush(" << caller
             << "): CORBA::Exception caught. Description:\n" << e;
          
        El::Service::Error error(ostr.str().c_str(), this);
        callback_->notify(&error);
      }
      
      if(started())
      {
        El::Service::CompoundServiceMessage_var msg = new Flush(this);
        deliver_at_time(msg.in(), ACE_OS::gettimeofday() + flush_period_);
      }
    }
    
    void
    StatLogger::wait() throw(Exception, El::Exception)
    {
      El::Service::CompoundService<>::wait();
      flush("StatLogger::wait");
    }

    Message::BankClientSession*
    StatLogger::bank_client_session() throw(Exception, El::Exception)
    {
      {
        Guard guard(lock_);

        if(bank_client_session_.in() != 0)
        {
          guard.release();
        
          bank_client_session_->_add_ref();
          return bank_client_session_.in();
        }
      }

      try
      {
        StatProcessor_var stat_processor = stat_processor_.object();
        
        CORBA::String_var message_bank_manager_ref =
          stat_processor->message_bank_manager();

        Message::BankManager_var message_bank_manager =
          MessageBankManagerRef(
            message_bank_manager_ref.in(), stat_processor_.orb()).object();

        Message::BankClientSession_var bank_client_session =
          message_bank_manager->bank_client_session();
      
        if(bank_client_session.in() == 0)
        {
          throw Exception(
            "NewsGate::Statistics::StatLogger::bank_client_session: "
            "bank_client_session.in() == 0");
        }
      
        Message::BankClientSessionImpl* bank_client_session_impl =
          dynamic_cast<Message::BankClientSessionImpl*>(
            bank_client_session.in());
      
        if(bank_client_session_impl == 0)
        {
          throw Exception(
            "NewsGate::Statistics::StatLogger::bank_client_session: "
            "dynamic_cast<Message::BankClientSessionImpl*> failed");
        }      
    
        Guard guard(lock_);
      
        if(bank_client_session_.in() != 0)
        {
          guard.release();
        
          bank_client_session_->_add_ref();
          return bank_client_session_.in();
        }
      
        bank_client_session_impl->_add_ref();
        bank_client_session_ = bank_client_session_impl;
      
        bank_client_session_->init_threads(this);
        bank_client_session_->_add_ref();
        
        return bank_client_session_.in();
      }
      catch(const Statistics::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Statistics::StatLogger::bank_client_session: "
          "::NewsGate::Statistics::ImplementationException caught. "
          "Description:\n" << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const Message::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Statistics::StatLogger::bank_client_session: "
             << "Message::ImplementationException caught. "
          "Description:\n" << e.description.in();

        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Statistics::StatLogger::bank_client_session: "
          "CORBA::Exception caught. Description:\n" << e;

        throw Exception(ostr.str());
      }
    }    

    //
    // RefererInfo struct
    //
    void
    RefererInfo::referer(const char* ref,
                         const char* site_host) throw(El::Exception)
    {
      El::Net::HTTP::URL_var u = new El::Net::HTTP::URL(ref);

      strncpy(url, u->string(), sizeof(url) - 1);
      
      std::string p = u->schema_and_endpoint() + u->path();
      strncpy(page, p.c_str(), sizeof(page) - 1);
      
      strncpy(site, u->host(), sizeof(site) - 1);
      
      std::string cd;
      
      if(El::Net::company_domain(u->host(), &cd))
      {
        strncpy(company_domain, cd.c_str(), sizeof(company_domain) - 1);
      }

      if(site_host)
      {
        try
        {
          El::Net::HTTP::URL_var site_url = new El::Net::HTTP::URL(site_host);

          internal = strcmp(site_url->host(), u->host()) == 0 &&
            site_url->port() == u->port();
        }
        catch(const El::Net::URL::Exception&)
        {
        }        
      }
    }

    //
    // ClientInfo struct
    //
    void
    ClientInfo::agent(const char* val) throw(El::Exception)
    {
      strncpy(user_agent, val, sizeof(user_agent) - 1);
          
      const char* nm = El::Net::HTTP::crawler(val);

      if(*nm)
      {
        type = 'C';
      }
      else if(*(nm = El::Net::HTTP::feed_reader(val)))
      {
        type = 'R';
      }
      else if(*(nm = El::Net::HTTP::browser(val)))
      {
        type = 'B';
      }
      else
      {
        type = 'U';
      }

      strcpy(name, nm);
      strcpy(os, El::Net::HTTP::os(val));

      const char* dv = El::Net::HTTP::computer(val);

      if(*dv)
      {
        device_type = 'C';
      }
      else if(*(dv = El::Net::HTTP::phone(val)))
      {
        device_type = 'P';
      }
      else if(*(dv = El::Net::HTTP::tab(val)))
      {
        device_type = 'T';
      }
      else
      {
        device_type = 'U';
      }
        
      strcpy(device, dv);      
    }
    
  }
}
