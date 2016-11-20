/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Event/Commons/BankClientSessionImpl.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <El/CORBA/Corba.hpp>

#include <string>
#include <sstream>

#include <ext/hash_map>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/Hash/Hash.hpp>
#include <El/CRC.hpp>

#include <Commons/Event/TransportImpl.hpp>
#include <Commons/Message/TransportImpl.hpp>

#include <Services/Commons/Event/EventServices.hpp>

#include "BankClientSessionImpl.hpp"

namespace NewsGate
{
  namespace Event
  {
    //
    // BankClientSessionImpl class
    //
    void
    BankClientSessionImpl::register_valuetype_factories(CORBA::ORB* orb)
      throw(CORBA::Exception, El::Exception)
    {
      CORBA::ValueFactoryBase_var factory =
        new NewsGate::Event::BankClientSessionImpl_init(orb);
      
      CORBA::ValueFactoryBase_var old = orb->register_value_factory(
        "IDL:NewsGate/Event/BankClientSession:1.0",
        factory);
    }

    void
    BankClientSessionImpl::marshal(CORBA::DataOutputStream* os)
    {
      os->write_Object(bank_manager_.in());
      os->write_ulong(refresh_period_.sec());
      os->write_ulong(invalidate_timeout_.sec());
      os->write_ulong(threads_);
      os->write_ulong(banks_.size());

      for(BankRecordArray::const_iterator it = banks_.begin();
          it != banks_.end(); it++)
      {
        os->write_string(it->bank.reference().c_str());
      }
    }

    void
    BankClientSessionImpl::unmarshal(CORBA::DataInputStream* is)
    {
      CORBA::Object_var object = is->read_Object();
      bank_manager_ = ::NewsGate::Event::BankManager::_narrow(object.in());

      if(CORBA::is_nil(bank_manager_.in()))
      {
        throw El::Corba::MARSHAL(
          "NewsGate::Event::BankClientSessionImpl::unmarshal: "
          "::NewsGate::Event::BankManager::_narrow failed");
      }

      refresh_period_ = is->read_ulong();
      invalidate_timeout_ = is->read_ulong();
      threads_ = is->read_ulong();

      banks_.resize(is->read_ulong());

      ACE_Time_Value tm = ACE_OS::gettimeofday();

      for(BankRecordArray::iterator it = banks_.begin(); it != banks_.end();
          it++)
      {
        CORBA::String_var ior = is->read_string();
        
        BankRecord& br = *it;
        
        br.bank = BankRef(ior.in(), orb_.in());
        br.invalidated = ACE_Time_Value::zero;
      }
      
      refreshed_ = tm;
    }

    BankManager_ptr
    BankClientSessionImpl::bank_manager() throw(CORBA::SystemException)
    {
      return NewsGate::Event::BankManager::_duplicate(bank_manager_.in());
    }

    void
    BankClientSessionImpl::delete_messages(
      ::NewsGate::Message::Transport::IdPack* ids)
      throw(NewsGate::Event::NotReady,
            NewsGate::Event::ImplementationException,
            ::CORBA::SystemException)
    {
      try
      {   
        {
          WriteGuard guard(lock_);
          refresh_session();
        }

        BankRecordArray banks;
        
        {
          ReadGuard guard(lock_);

          if(banks_.empty())
          {
            NotReady e;
            
            e.reason =
              CORBA::string_dup(
                "Events::BankClientSessionImpl::delete_messages: "
                "no banks available");
            
            throw e;
          }

          banks = banks_;
        }

        for(BankRecordArray::iterator it = banks.begin(); it != banks.end();
            it++)
        {
          NewsGate::Event::Bank_var bank = it->bank.object();
          bank->delete_messages(ids);
        }
        
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "Event::BankClientSessionImpl::delete_messages: "
          "El::Exception caught. Description:\n" << e.what();
        
        ImplementationException ex;
        ex.description = CORBA::string_dup(ostr.str().c_str());
        
        throw ex;
      }      
      
    }
    
    BankClientSession::RequestResult*
    BankClientSessionImpl::post_message_digest(
      ::NewsGate::Event::Transport::MessageDigestPack* digests,
      ::NewsGate::Message::Transport::MessageEventPack_out events)
      throw(NewsGate::Event::NotReady,
            NewsGate::Event::ImplementationException,
            ::CORBA::SystemException)
    {
      try
      {
        Transport::MessageDigestPackImpl::Type* digests_impl =
          dynamic_cast<Transport::MessageDigestPackImpl::Type*>(digests);

        if(digests_impl == 0)
        {
          throw Exception(
            "Event::BankClientSessionImpl::post_message_digest: "
            "dynamic_cast<Transport::MessageDigestPackImpl::Type*>(digests) "
            "failed");
        }

        const Transport::MessageDigestArray& message_digests =
          digests_impl->entities();

        Message::Transport::MessageEventPackImpl::Var result =
          new Message::Transport::MessageEventPackImpl::Type(
            new Message::Transport::MessageEventArray());

        if(message_digests.size() == 0)
        {
          events = result._retn();
          
          RequestResult_var res = new RequestResult();        
          res->code = RRC_OK;

          return res._retn();
        }

        Message::Transport::IdPackImpl::Var message_ids =
          new Message::Transport::IdPackImpl::Type(new Message::IdArray());
        
        Message::IdArray& ids = message_ids->entities();

        //
        // Filling message ids array to request message event info 
        //
        ids.reserve(message_digests.size());

        for(Transport::MessageDigestArray::const_iterator
              it = message_digests.begin(); it != message_digests.end(); it++)
        {
          ids.push_back(it->id);
        }

        message_ids->serialize();

        RequestMessageDigestsTask_var task =
          new RequestMessageDigestsTask(callback_, message_ids.in());

        {
          WriteGuard guard(lock_);
          refresh_session();
        }
        
        bool execute_in_current_thread = true;
        unsigned long banks_count = 0;
        
        {
          ReadGuard guard(lock_);

          execute_in_current_thread = thread_pool_.in() == 0;
          banks_count = banks_.size();
          
          if(banks_count == 0)
          {
            NotReady e;
            
            e.reason =
              CORBA::string_dup(
                "Events::BankClientSessionImpl::post_message_digest: "
                "no banks available");
            
            throw e;
          }

          for(BankRecordArray::const_iterator it = banks_.begin();
              it != banks_.end(); it++)
          {
            task->add_bank(it->bank, thread_pool_.in());
          }
        }

        if(execute_in_current_thread)
        {
          while(banks_count--)
          {
            task->execute();
          }
        }
        
        task->wait();

        if(task->result != RRC_OK)
        {
          events = result._retn();
          
          RequestResult_var res = new RequestResult();
        
          res->code = task->result;
          res->description = task->error_desc.c_str();

          return res._retn();
        }

        //
        // Taking events info for existing messages, preparing posting info
        // for new ones
        //
        
        RequestMessageDigestsTask::MessageInfoMap& message_infos =
          task->message_infos;

        Transport::MessageDigestPackImpl::Var post_digests =
          new Transport::MessageDigestPackImpl::Type(
            new Transport::MessageDigestArray());

        Transport::MessageDigestArray& post_digests_array =
          post_digests->entities();

        post_digests_array.reserve(
          message_digests.size() - message_infos.size());
          
        Message::Transport::MessageEventArray& result_array =
          result->entities();

        result_array.reserve(message_digests.size());

        for(Transport::MessageDigestArray::const_iterator
              it = message_digests.begin(); it != message_digests.end(); it++)
        {
          Message::Transport::MessageEvent me;
          me.id = it->id;
          
          RequestMessageDigestsTask::MessageInfoMap::const_iterator
            mim = message_infos.find(me.id);

          if(mim == message_infos.end())
          {
            post_digests_array.push_back(*it);
          }
          else
          {
            const RequestMessageDigestsTask::MessageInfo& mi = mim->second;
            
            me.event_id = mi.event_id;
            me.event_capacity = mi.event_capacity;
          }

          result_array.push_back(me);          
        }

        if(post_digests_array.size() == 0)
        {
          //
          // All messages found in event banks, no futher processing required
          //
          events = result._retn();
          
          RequestResult_var res = new RequestResult();        
          res->code = RRC_OK;

          return res._retn();
        }

        ::NewsGate::Message::Transport::MessageEventPack_var result_events;
            
        //
        // Posting digests for those messages which do not belong to any
        // event bank
        //
        
        RequestResult_var res =
          post_message_digest(task->requested_banks,
                              post_digests.in(),
                              result_events.out());

        if(res->code != RRC_OK)
        {
          events = result._retn();
          return res._retn();
        }

        Message::Transport::MessageEventPackImpl::Type* mep =
          dynamic_cast<Message::Transport::MessageEventPackImpl::Type*>(
            result_events.in());

        if(mep == 0)
        {
          throw Exception(
            "NewsGate::Event::BankClientSessionImpl::post_message_digest: "
            "dynamic_cast<Message::Transport::MessageEventPackImpl::Type*>"
            "(res.in()) failed");          
        }

        const Message::Transport::MessageEventArray& mea =
          mep->entities();

        //
        // Merging event info for existing and newly added messages
        //
        message_infos.clear();
        
        for(Message::Transport::MessageEventArray::const_iterator
              it = mea.begin(); it != mea.end(); it++)
        {
          RequestMessageDigestsTask::MessageInfo& mi =
            message_infos[it->id];
              
          mi.event_id = it->event_id;
          mi.event_capacity = it->event_capacity;
        }

        for(Message::Transport::MessageEventArray::iterator
              it = result_array.begin(); it != result_array.end(); it++)
        {
          Message::Transport::MessageEvent& me = *it;
          
          if(me.event_id != El::Luid::null)
          {
            continue;
          }

          RequestMessageDigestsTask::MessageInfoMap::const_iterator mit =
            message_infos.find(me.id);

          if(mit != message_infos.end())
          {
            const RequestMessageDigestsTask::MessageInfo& mi = mit->second;
            
            me.event_id = mi.event_id;
            me.event_capacity = mi.event_capacity;
          }
        }
        
        events = result._retn();
        return res._retn();
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "Event::BankClientSessionImpl::post_message_digest: "
          "El::Exception caught. Description:\n" << e.what();
        
        ImplementationException ex;
        ex.description = CORBA::string_dup(ostr.str().c_str());
        
        throw ex;
      }
      
    }

    BankClientSession::RequestResult*
    BankClientSessionImpl::post_message_digest(
      BankInfoList& requested_banks,
      Transport::MessageDigestPack* digests,
      ::NewsGate::Message::Transport::MessageEventPack_out events)
      throw(Exception, El::Exception)
    {
      std::string last_error_desc;
      
      RequestResult_var res = new RequestResult();
      res->code = RRC_OK;
      
      for(BankInfoList::iterator it = requested_banks.begin();
          it != requested_banks.end(); it++)
      {
        BankInfo& bank_info = *it;

        try
        {
          NewsGate::Event::Bank_var bank = bank_info.bank.object();  
          bank->post_message_digest(digests, events);

          res = new RequestResult();
          res->code = RRC_OK;
        
          return res._retn();
        }
        catch(const NewsGate::Event::NotReady& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::BankClientSessionImpl::"
            "post_message_digest: NewsGate::Event::NotReady caught. "
            "Reason:\n" << e.reason;

          last_error_desc = ostr.str();        

          if(res->code < RRC_NOT_READY)
          {
            res->code = RRC_NOT_READY;
            res->description = last_error_desc.c_str();
          }
        }
        catch(const NewsGate::Event::ImplementationException& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::BankClientSessionImpl::"
            "RequestMessageDigestsTask::execute: "
            "NewsGate::Event::ImplementationException caught. Description:\n"
               << e.description;

          last_error_desc = ostr.str();
        
          if(res->code < RRC_EVENT_BANK_ERROR)
          {
            res->code = RRC_EVENT_BANK_ERROR;
            res->description = last_error_desc.c_str();
          }
        }
        catch(const CORBA::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Event::BankClientSessionImpl::"
            "RequestMessageDigestsTask::execute: CORBA::Exception caught. "
            "Description:\n" << e;

          last_error_desc = ostr.str();        
        
          if(res->code < RRC_CORBA_ERROR)
          {
            res->code = RRC_CORBA_ERROR;
            res->description = last_error_desc.c_str();
          }
        }

        if(callback_ && !last_error_desc.empty())
        {
          El::Service::Error error(
            last_error_desc,
            0,
            res->code <= BankClientSession::RRC_NOT_READY ?
            El::Service::Error::NOTICE : El::Service::Error::ALERT);
        
          callback_->notify(&error);
        }
      }
      
      return res._retn();
    }

    BankClientSession::RequestResult*
    BankClientSessionImpl::get_events(
      ::NewsGate::Event::Transport::EventIdRelPack* ids,
      ::NewsGate::Event::Transport::EventObjectRelPack_out events)
      throw(NewsGate::Event::NotReady,
            NewsGate::Event::ImplementationException,
            ::CORBA::SystemException)
    {
      try
      {
        Transport::EventIdRelPackImpl::Type* ids_impl =
          dynamic_cast<Transport::EventIdRelPackImpl::Type*>(ids);

        if(ids_impl == 0)
        {
          throw Exception(
            "Event::BankClientSessionImpl::get_events: "
            "dynamic_cast<Transport::EventIdRelPackImpl::Type*>(ids) failed");
        }

        ids_impl->serialize();

        RequestEventsTask_var task = new RequestEventsTask(callback_, ids);

        {
          WriteGuard guard(lock_);
          refresh_session();
        }
        
        bool execute_in_current_thread = true;
        uint32_t banks_count = 0;
        
        {
          ReadGuard guard(lock_);

          execute_in_current_thread = thread_pool_.in() == 0;
          banks_count = banks_.size();
          
          if(banks_count == 0)
          {
            NotReady e;
            
            e.reason =
              CORBA::string_dup(
                "Event::BankClientSessionImpl::get_events: "
                "no banks available");
            
            throw e;
          }

          for(BankRecordArray::const_iterator it = banks_.begin();
              it != banks_.end(); it++)
          {
            task->add_bank(it->bank, thread_pool_.in());
          }
        }

        if(execute_in_current_thread)
        {
          while(banks_count--)
          {
            task->execute();
          }
        }
        
        task->wait();

        Transport::EventObjectRelPackImpl::Var result =
          new Transport::EventObjectRelPackImpl::Type(
            new Transport::EventObjectRelArray());

        if(task->result != RRC_OK)
        {
          events = result._retn();
          
          RequestResult_var res = new RequestResult();
        
          res->code = task->result;
          res->description = task->error_desc.c_str();

          return res._retn();
        }

        RequestEventsTask::EventInfoMap& event_infos = task->event_infos;

        Transport::EventObjectRelArray& event_array = result->entities();
        event_array.reserve(event_infos.size());
        
        for(RequestEventsTask::EventInfoMap::const_iterator
              i(event_infos.begin()), e(event_infos.end()); i != e; ++i)
        {
          event_array.push_back(i->second.event_rel);
        }

        events = result._retn();
        
        RequestResult_var res = new RequestResult();        
        res->code = RRC_OK;
        
        return res._retn();
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "Event::BankClientSessionImpl::get_events: "
          "El::Exception caught. Description:\n" << e.what();
        
        ImplementationException ex;
        ex.description = CORBA::string_dup(ostr.str().c_str());
        
        throw ex;
      }
    }
    
    void
    BankClientSessionImpl::init_threads(El::Service::Callback* callback,
                                        unsigned long threads)
      throw(El::Exception)
    {
      WriteGuard guard(lock_);
      
      if(thread_pool_.in() == 0)
      {
        callback_ = callback;

        if(threads)
        {
          threads_ = threads;
        }
        
        thread_pool_ =
          new El::Service::ThreadPool(callback, "CallThreadPool", threads_);

        thread_pool_->start();
      }
    }

    void
    BankClientSessionImpl::refresh_session()
      throw(Exception, El::Exception, CORBA::Exception)
    {
      ACE_Time_Value tm = ACE_OS::gettimeofday();

      if(refreshed_ + refresh_period_ > tm)
      {
        return;
      }
        
      if(CORBA::is_nil(bank_manager_.in()))
      {
        std::ostringstream ostr;
        ostr << "BankClientSessionImpl::refresh_session: "
             << "bank manager undefined";
            
        throw Exception(ostr.str());
      }
          
      BankClientSession_var session = bank_manager_->bank_client_session();

      BankClientSessionImpl* session_impl =
        dynamic_cast<BankClientSessionImpl*>(session.in());

      if(session_impl == 0)
      {
        std::ostringstream ostr;
        ostr << "BankClientSessionImpl::refresh_session: "
             << "dynamic_cast<BankClientSessionImpl*> failed";
            
        throw Exception(ostr.str());
      }

      bank_manager_ = session_impl->bank_manager_;

      for(BankRecordArray::iterator it = banks_.begin();
          it != banks_.end(); it++)
      {
        if(it->invalidated != ACE_Time_Value::zero)
        {
          std::string bank_ior = it->bank.reference();
            
          BankRecordArray::iterator it2 = session_impl->banks_.begin();
          for(; it2 != session_impl->banks_.end() &&
                it2->bank.reference() != bank_ior; it2++);
              
          if(it2 != session_impl->banks_.end())
          {
            it2->invalidated = it->invalidated;
          }
        }
      }
            
      banks_ = session_impl->banks_;
      refreshed_ = session_impl->refreshed_;
      refresh_period_ = session_impl->refresh_period_;
      invalidate_timeout_ = session_impl->invalidate_timeout_;

      if(thread_pool_.in() == 0)
      {
        threads_ = session_impl->threads_;
      }
    }  
    
    void
    BankClientSessionImpl::invalidate_bank(const char* bank_ior)
      throw(El::Exception)
    {
      for(BankRecordArray::iterator it = banks_.begin();
          it != banks_.end(); it++)
      {
        BankRecord& bank_rec = *it;
              
        if(bank_rec.bank.reference() == bank_ior)
        {
          bank_rec.invalidated = ACE_OS::gettimeofday();
          return;
        }
      }
    }
    
    bool
    BankClientSessionImpl::validate_bank(BankRecord& bank)
      throw(El::Exception)
    {
      if(bank.invalidated == ACE_Time_Value::zero)
      {
        return true;
      }

      ACE_Time_Value current_time = ACE_OS::gettimeofday();
      
      if(bank.invalidated + invalidate_timeout_ > current_time)
      {
        return false;
      }

      Event::Transport::EventIdRelPackImpl::Var ids =
        new Event::Transport::EventIdRelPackImpl::Type(
          new Event::Transport::EventIdRelArray());
      
      try
      {
        Event::Transport::EventObjectRelPack_var events;
        NewsGate::Event::Bank_var bank_ref = bank.bank.object();
          
        bank_ref->get_events(ids.in(), events.out());
        bank.invalidated = ACE_Time_Value::zero;
      }
      catch(...)
      {
        bank.invalidated = current_time;
        return false;
      }

      return true;
    }    

    //
    // BankClientSessionImpl::RequestMessageDigestsTask class
    //
    void
    BankClientSessionImpl::RequestMessageDigestsTask::execute()
      throw(El::Exception)
    {
      BankInfo bank_info = get_bank();
      std::string last_error_desc;

      try
      {
        Message::Transport::MessageEventPack_var res;

        NewsGate::Event::Bank_var bank = bank_info.bank.object();
          
        bank_info.message_count =
          bank->get_message_events(message_ids_.in(), res.out());

        Message::Transport::MessageEventPackImpl::Type* mep =
          dynamic_cast<Message::Transport::MessageEventPackImpl::Type*>(
            res.in());

        if(mep == 0)
        {          
          last_error_desc = "NewsGate::Event::BankClientSessionImpl::"
            "RequestMessageDigestsTask::execute: dynamic_cast<"
            "Message::Transport::MessageEventPackImpl::Type*>(res.in()) "
            "failed";
          
          WriteGuard guard(lock_);

          if(result < BankClientSession::RRC_EVENT_BANK_ERROR)
          {
            result = BankClientSession::RRC_EVENT_BANK_ERROR;
            error_desc = last_error_desc;
          }
        }
        else
        {
          const Message::Transport::MessageEventArray& mea =
            mep->entities();

          WriteGuard guard(lock_);
          
          for(Message::Transport::MessageEventArray::const_iterator
                it = mea.begin(); it != mea.end(); it++)
          {
            if(it->event_id == El::Luid::null)
            {
              continue;
            }
            
            MessageInfoMap::const_iterator mit =
              message_infos.find(it->id);
              
            if(mit == message_infos.end() ||
               mit->second.bank_hash > bank_info.hash)
            {
              MessageInfo& mi = message_infos[it->id];
              
              mi.event_id = it->event_id;
              mi.event_capacity = it->event_capacity;
              mi.bank_hash = bank_info.hash;
            }
          }

          BankInfoList::iterator it = requested_banks.begin();
          
          for(; it != requested_banks.end() &&
                it->message_count < bank_info.message_count; it++);

          requested_banks.insert(it, bank_info);
        }
      }
      catch(const NewsGate::Event::NotReady& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Event::BankClientSessionImpl::"
          "RequestMessageDigestsTask::execute: "
          "NewsGate::Event::NotReady caught. Reason:\n" << e.reason;

        last_error_desc = ostr.str();        

        WriteGuard guard(lock_);

        if(result < BankClientSession::RRC_NOT_READY)
        {
          result = BankClientSession::RRC_NOT_READY;
          error_desc = last_error_desc;
        }
      }
      catch(const NewsGate::Event::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Event::BankClientSessionImpl::"
          "RequestMessageDigestsTask::execute: "
          "NewsGate::Event::ImplementationException caught. Description:\n"
             << e.description;

        last_error_desc = ostr.str();
        
        WriteGuard guard(lock_);

        if(result < BankClientSession::RRC_EVENT_BANK_ERROR)
        {
          result = BankClientSession::RRC_EVENT_BANK_ERROR;
          error_desc = last_error_desc;
        }
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Event::BankClientSessionImpl::"
          "RequestMessageDigestsTask::execute: CORBA::Exception caught. "
          "Description:\n" << e;

        last_error_desc = ostr.str();        
        
        WriteGuard guard(lock_);

        if(result < BankClientSession::RRC_CORBA_ERROR)
        {
          result = BankClientSession::RRC_CORBA_ERROR;
          error_desc = last_error_desc;
        }
      }

      if(callback_ && !last_error_desc.empty())
      {
        // Notice on BankClientSession::RRC_CORBA_ERROR
        // as Event Bank could be shutdown by that time
        
        El::Service::Error error(
          last_error_desc,
          0,
          result <= BankClientSession::RRC_NOT_READY ||
          result == BankClientSession::RRC_CORBA_ERROR ?
          El::Service::Error::NOTICE : El::Service::Error::ALERT);
        
        callback_->notify(&error);
      }

      signal_if_done();      
    }    
    
    //
    // BankClientSessionImpl::RequestEventsTask class
    //
    void
    BankClientSessionImpl::RequestEventsTask::execute() throw(El::Exception)
    {
      BankInfo bank_info = get_bank();

      std::string last_error_desc;

      try
      {
        Event::Transport::EventObjectRelPack_var res;
        
        NewsGate::Event::Bank_var bank = bank_info.bank.object();
        bank->get_events(event_ids_.in(), res.out());

        Event::Transport::EventObjectRelPackImpl::Type* eop =
          dynamic_cast<Event::Transport::EventObjectRelPackImpl::Type*>(
            res.in());

        if(eop == 0)
        {          
          last_error_desc = "NewsGate::Event::BankClientSessionImpl::"
            "RequestEventsTask::execute: dynamic_cast<"
            "Event::Transport::EventObjectRelPackImpl::Type*>(res.in()) "
            "failed";
          
          WriteGuard guard(lock_);

          if(result < BankClientSession::RRC_EVENT_BANK_ERROR)
          {
            result = BankClientSession::RRC_EVENT_BANK_ERROR;
            error_desc = last_error_desc;
          }
        }
        else
        {
          const Event::Transport::EventObjectRelArray& eoa =
            eop->entities();

          WriteGuard guard(lock_);
          
          for(Event::Transport::EventObjectRelArray::const_iterator
                i(eoa.begin()), e(eoa.end()); i != e; ++i)
          {
            const Event::Transport::EventObjectRel& event_rel = *i;
            
            EventInfoMap::const_iterator eit =
              event_infos.find(event_rel.rel.object1.id);
              
            if(eit == event_infos.end() ||
               eit->second.bank_hash > bank_info.hash)
            {
              EventInfo& ei = event_infos[event_rel.rel.object1.id];
              
              ei.event_rel = event_rel;
              ei.bank_hash = bank_info.hash;
            }
          }
        }
      }
      catch(const NewsGate::Event::NotReady& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Event::BankClientSessionImpl::"
          "RequestEventsTask::execute: "
          "NewsGate::Event::NotReady caught. Reason:\n" << e.reason;

        last_error_desc = ostr.str();        

        WriteGuard guard(lock_);

        if(result < BankClientSession::RRC_NOT_READY)
        {
          result = BankClientSession::RRC_NOT_READY;
          error_desc = last_error_desc;
        }
      }
      catch(const NewsGate::Event::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Event::BankClientSessionImpl::"
          "RequestEventsTask::execute: "
          "NewsGate::Event::ImplementationException caught. Description:\n"
             << e.description;

        last_error_desc = ostr.str();
        
        WriteGuard guard(lock_);

        if(result < BankClientSession::RRC_EVENT_BANK_ERROR)
        {
          result = BankClientSession::RRC_EVENT_BANK_ERROR;
          error_desc = last_error_desc;
        }
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Event::BankClientSessionImpl::"
          "RequestEventsTask::execute: CORBA::Exception caught. "
          "Description:\n" << e;

        last_error_desc = ostr.str();        
        
        WriteGuard guard(lock_);

        if(result < BankClientSession::RRC_CORBA_ERROR)
        {
          result = BankClientSession::RRC_CORBA_ERROR;
          error_desc = last_error_desc;
        }
      }

      if(callback_ && !last_error_desc.empty())
      {
        El::Service::Error error(
          last_error_desc,
          0,
          result <= BankClientSession::RRC_NOT_READY ?
          El::Service::Error::NOTICE : El::Service::Error::ALERT);
        
        callback_->notify(&error);
      }
      
      signal_if_done();      
    }    
  }
}
