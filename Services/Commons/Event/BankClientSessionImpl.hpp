/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Commons/Event/BankClientSessionImpl.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_SERVICES_EVENT_COMMONS_BANKCLIENTSESSIONIMPL_HPP_
#define _NEWSGATE_SERVER_SERVICES_EVENT_COMMONS_BANKCLIENTSESSIONIMPL_HPP_

#include <stdint.h>

#include <vector>

#include <ext/hash_set>
#include <google/sparse_hash_map>

#include <El/Exception.hpp>
#include <El/Hash/Hash.hpp>
#include <El/Stat.hpp>
#include <El/Luid.hpp>

#include <Commons/Message/Message.hpp>
#include <Commons/Event/Event.hpp>

#include <El/Service/Service.hpp>
#include <El/Service/ThreadPool.hpp>

#include <ace/OS.h>
#include <ace/Synch.h>
#include <ace/Guard_T.h>
#include <ace/High_Res_Timer.h>

#include <Commons/Event/TransportImpl.hpp>
#include <Services/Commons/Event/EventServices.hpp>

namespace NewsGate
{
  namespace Event
  {
    class BankClientSessionImpl :
      public virtual BankClientSession,
      public virtual El::Corba::ValueRefCountBase
    {
    public:
      EL_EXCEPTION(Exception, El::ExceptionBase);
      EL_EXCEPTION(InvalidArg, Exception);
      
    public:
      BankClientSessionImpl(CORBA::ORB_ptr orb,
                            NewsGate::Event::BankManager_ptr bank_manager,
                            const ACE_Time_Value& refresh_period,
                            const ACE_Time_Value& invalidate_timeout,
                            unsigned long threads)
        throw(InvalidArg, El::Exception);
      
      virtual ~BankClientSessionImpl() throw();      

      static void register_valuetype_factories(CORBA::ORB* orb)
        throw(CORBA::Exception, El::Exception);

      //
      // Requred to be invoked before ... method calls
      //
      void init_threads(El::Service::Callback* callback,
                        unsigned long threads = 0) throw(El::Exception);
      
      //
      // IDL:omg.org/CORBA/CustomMarshal/marshal:1.0
      //
      virtual void marshal(CORBA::DataOutputStream* os);

      //
      // IDL:omg.org/CORBA/CustomMarshal/unmarshal:1.0
      //
      virtual void unmarshal(CORBA::DataInputStream* is);

      //
      // IDL:NewsGate/Event/BankClientSession/bank_manager:1.0
      //
      virtual BankManager_ptr bank_manager() throw(CORBA::SystemException);

      //
      // IDL:NewsGate/Event/BankClientSession/post_message_digest:1.0
      //
      virtual RequestResult* post_message_digest(
        ::NewsGate::Event::Transport::MessageDigestPack* digests,
        ::NewsGate::Message::Transport::MessageEventPack_out events)
        throw(NewsGate::Event::NotReady,
              NewsGate::Event::ImplementationException,
              ::CORBA::SystemException);      

      //
      // IDL:NewsGate/Event/BankClientSession/get_events:1.0
      //
      virtual RequestResult* get_events(
        ::NewsGate::Event::Transport::EventIdRelPack* ids,
        ::NewsGate::Event::Transport::EventObjectRelPack_out events)
        throw(NewsGate::Event::NotReady,
              NewsGate::Event::ImplementationException,
              ::CORBA::SystemException);
      
      //
      // IDL:NewsGate/Event/BankClientSession/delete_messages:1.0
      //
      virtual void delete_messages(::NewsGate::Message::Transport::IdPack* ids)
        throw(NewsGate::Event::NotReady,
              NewsGate::Event::ImplementationException,
              ::CORBA::SystemException);
      
      typedef El::Corba::SmartRef<NewsGate::Event::Bank> BankRef;
      
      struct BankRecord
      {
        BankRef bank;
        ACE_Time_Value invalidated;
      };

      typedef std::vector<BankRecord> BankRecordArray;

// Not thread safe      
      BankRecordArray& banks() throw();
      const BankRecordArray& banks() const throw();

    private:      
      virtual CORBA::ValueBase* _copy_value() throw(CORBA::NO_IMPLEMENT);      
        
      void invalidate_bank(const char* bank_ior) throw(El::Exception);
      bool validate_bank(BankRecord& bank) throw(El::Exception);
      void refresh_session() throw(Exception, El::Exception, CORBA::Exception);

    private:
      
      struct BankInfo
      {
        BankRef bank;
        uint32_t message_count;
        uint32_t hash;
        
        BankInfo() throw() : message_count(0), hash(0) {}
      };
        
      typedef std::list<BankInfo> BankInfoList;

      class BankClientTask : public virtual El::Service::ThreadPool::TaskBase
      {
      public:
        
        BankClientTask(El::Service::Callback* callback) throw(El::Exception);
        virtual ~BankClientTask() throw() {}
        
        void add_bank(const BankRef& bank,
                      El::Service::ThreadPool* thread_pool)
          throw(El::Exception);

        void wait() throw(El::Exception);

      public:
        RequestResultCode result;
        std::string error_desc;

      protected:
        
        BankInfo get_bank() throw(Exception, El::Exception);
        void signal_if_done() throw();
        
      protected:
        
        typedef ACE_Thread_Mutex       Mutex;
        typedef ACE_Read_Guard<Mutex>  ReadGuard;
        typedef ACE_Write_Guard<Mutex> WriteGuard;

        typedef ACE_Condition<ACE_Thread_Mutex> Condition;

        mutable Mutex lock_;
        Condition request_completed_;
        
        El::Service::Callback* callback_;
        
        BankInfoList banks_;
        uint32_t banks_count_;
        uint32_t completed_requests_;        
      };

      class RequestMessageDigestsTask : public virtual BankClientTask
      {
      public:
        
        RequestMessageDigestsTask(
          El::Service::Callback* callback,
          Message::Transport::IdPack* message_ids)
          throw(El::Exception);

        virtual ~RequestMessageDigestsTask() throw() {}
        
      public:

        struct MessageInfo
        {
          El::Luid event_id;
          uint32_t event_capacity;
          uint32_t bank_hash;

          MessageInfo(El::Luid event_id_val = El::Luid::null,
                      uint32_t event_capacity_val = 0,
                      uint32_t bank_hash_val = 0) throw();
        };

        class MessageInfoMap :
          public google::sparse_hash_map<Message::Id,
                                         MessageInfo,
                                         Message::MessageIdHash>
        {
        public:
          MessageInfoMap() throw(El::Exception);
        };

        MessageInfoMap message_infos;
        BankInfoList requested_banks;

        virtual void execute() throw(El::Exception);
        
      private:
        Message::Transport::IdPack_var message_ids_;
      };

      typedef El::RefCount::SmartPtr<RequestMessageDigestsTask>
      RequestMessageDigestsTask_var;

      class RequestEventsTask : public virtual BankClientTask
      {
      public:
        
        RequestEventsTask(El::Service::Callback* callback,
                          Transport::EventIdRelPack* ids)
          throw(El::Exception);

        virtual ~RequestEventsTask() throw() {}
        
      public:

        struct EventInfo
        {
          Event::Transport::EventObjectRel event_rel;
          uint32_t bank_hash;

          EventInfo() throw(El::Exception) : bank_hash(0) {}
        };

        class EventInfoMap :
          public google::sparse_hash_map<El::Luid, EventInfo, El::Hash::Luid>
        {
        public:
          EventInfoMap() throw(El::Exception);
        };

        EventInfoMap event_infos;

        virtual void execute() throw(El::Exception);
        
      private:        
        Transport::EventIdRelPack_var event_ids_;
      };

      typedef El::RefCount::SmartPtr<RequestEventsTask>
      RequestEventsTask_var;

      RequestResult* post_message_digest(
        BankInfoList& requested_banks,
        Transport::MessageDigestPack* digests,
        ::NewsGate::Message::Transport::MessageEventPack_out events)
        throw(Exception, El::Exception);
      
    private:
      
      typedef ACE_RW_Thread_Mutex     Mutex_;
      typedef ACE_Read_Guard<Mutex_>  ReadGuard;
      typedef ACE_Write_Guard<Mutex_> WriteGuard;
     
      mutable Mutex_ lock_;
      
      CORBA::ORB_var orb_;
      El::Service::Callback* callback_;
      NewsGate::Event::BankManager_var bank_manager_;
      BankRecordArray banks_;
      ACE_Time_Value refreshed_;
      ACE_Time_Value refresh_period_;
      ACE_Time_Value invalidate_timeout_;
      unsigned long threads_;

      El::Service::ThreadPool_var thread_pool_;
    };

    typedef El::Corba::ValueVar<BankClientSessionImpl>
    BankClientSessionImpl_var;

    class BankClientSessionImpl_init : public virtual CORBA::ValueFactoryBase
    {
    public:
      EL_EXCEPTION(Exception, El::ExceptionBase);
      EL_EXCEPTION(InvalidArg, Exception);
      
    public:
      BankClientSessionImpl_init(CORBA::ORB_ptr orb)
        throw(InvalidArg, El::Exception);
      
      static BankClientSessionImpl* create(
        CORBA::ORB_ptr orb,
        NewsGate::Event::BankManager_ptr bank_manager,
        const ACE_Time_Value& refresh_period,
        const ACE_Time_Value& invalidate_timeout,
        unsigned long threads) throw(El::Exception);
      
      virtual CORBA::ValueBase* create_for_unmarshal();

    private:
      CORBA::ORB_var orb_;
      
    };
    
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Event
  {
    //
    // BankClientSessionImpl class
    //
    inline
    BankClientSessionImpl::BankClientSessionImpl(
      CORBA::ORB_ptr orb,
      NewsGate::Event::BankManager_ptr bank_manager,
      const ACE_Time_Value& refresh_period,
      const ACE_Time_Value& invalidate_timeout,
      unsigned long threads)
      throw(InvalidArg, El::Exception)
        : orb_(CORBA::ORB::_duplicate(orb)),
          callback_(0),
          bank_manager_(NewsGate::Event::BankManager::_duplicate(
                          bank_manager)),
          refresh_period_(refresh_period),
          invalidate_timeout_(invalidate_timeout),
          threads_(threads)
    {
      if(CORBA::is_nil(orb))
      {
        throw InvalidArg("NewsGate::Event::BankClientSessionImpl::"
                         "BankClientSessionImpl: orb is undefined");
      }
      
    }    

    inline
    BankClientSessionImpl::~BankClientSessionImpl() throw()
    {
      if(thread_pool_.in() != 0)
      {
        thread_pool_->stop();
        thread_pool_->wait();
      }
    }

    inline
    CORBA::ValueBase*
    BankClientSessionImpl::_copy_value() throw(CORBA::NO_IMPLEMENT)
    {
      BankClientSessionImpl_var res =
        new BankClientSessionImpl(orb_.in(),
                                  bank_manager_.in(),
                                  refresh_period_,
                                  invalidate_timeout_,
                                  threads_);

      res->refreshed_ = refreshed_;
      res->callback_ = callback_;
      res->banks_ = banks_;

      return res._retn();
    }  

    inline
    BankClientSessionImpl::BankRecordArray&
    BankClientSessionImpl::banks() throw()
    {
      return banks_;
    }
    
    inline
    const BankClientSessionImpl::BankRecordArray&
    BankClientSessionImpl::banks() const throw()
    {
      return banks_;
    }

    //
    // BankSessionId_init class
    //
    inline
    BankClientSessionImpl_init::BankClientSessionImpl_init(CORBA::ORB_ptr orb)
      throw(InvalidArg, El::Exception)
        : orb_(CORBA::ORB::_duplicate(orb))
    {
      if(CORBA::is_nil(orb))
      {
        throw InvalidArg("NewsGate::Event::BankClientSessionImpl_init::"
                         "BankClientSessionImpl_init: orb not defined");
      }
    }
    
    inline
    BankClientSessionImpl*
    BankClientSessionImpl_init::create(
      CORBA::ORB_ptr orb,
      NewsGate::Event::BankManager_ptr bank_manager,
      const ACE_Time_Value& refresh_period,
      const ACE_Time_Value& invalidate_timeout,
      unsigned long threads) throw(El::Exception)
    {
      return new BankClientSessionImpl(orb,
                                       bank_manager,
                                       refresh_period,
                                       invalidate_timeout,
                                       threads);
    }

    inline
    CORBA::ValueBase* 
    BankClientSessionImpl_init::create_for_unmarshal()
    {
      return create(orb_.in(),
                    0,
                    ACE_Time_Value::zero,
                    ACE_Time_Value::zero,
                    0);
    }

    
    //
    // BankClientSessionImpl::BankClientTask class
    //
    inline
    BankClientSessionImpl::BankClientTask::BankClientTask(
      El::Service::Callback* callback)
      throw(El::Exception)
        : TaskBase(false),
          result(BankClientSession::RRC_OK),
          request_completed_(lock_),
          callback_(callback),
          banks_count_(0),
          completed_requests_(0)
    {
    }
    
    inline
    void
    BankClientSessionImpl::BankClientTask::add_bank(const BankRef& bank,
      El::Service::ThreadPool* thread_pool) throw(El::Exception)
    {
      BankInfo bank_info;
      bank_info.bank = bank;

      std::string bank_ior = bank.reference();
        
      El::CRC(bank_info.hash,
              (const unsigned char*)bank_ior.c_str(),
              bank_ior.length());
      
      {
        WriteGuard guard(lock_);
      
        banks_.push_back(bank_info);
        banks_count_++;
      }

      if(thread_pool)
      {
        thread_pool->execute(this);
      }
    }

    inline
    void
    BankClientSessionImpl::BankClientTask::wait() throw(El::Exception)
    {
      while(true)
      {
        ReadGuard guard(lock_);
        
        if(completed_requests_ == banks_count_)
        {
          break;
        }
        
        if(request_completed_.wait(0))
        {
          int error = ACE_OS::last_error();
          
          std::ostringstream ostr;
          ostr << "BankClientSessionImpl::BankClientTask::wait: "
            "request_completed_.wait() failed. "
            "Errno " << error << ". Description:" << std::endl
               << ACE_OS::strerror(error);
          
          throw Exception(ostr.str());
        }
      }
    }

    inline
    BankClientSessionImpl::BankInfo
    BankClientSessionImpl::BankClientTask::get_bank()
      throw(Exception, El::Exception)
    {
      WriteGuard guard(lock_);
        
      BankInfoList::iterator it = banks_.begin();
      
      if(it == banks_.end())
      {
        throw Exception("BankClientSessionImpl::BankClientTask::"
                        "execute: unexpected end of bank ref set");
      }
      
      BankInfo bank_info = *it;
      banks_.erase(it);
      return bank_info;
    }

    inline
    void
    BankClientSessionImpl::BankClientTask::signal_if_done() throw()
    {
      WriteGuard guard(lock_);
      
      if(++completed_requests_ == banks_count_)
      {
        request_completed_.signal();
      }
    }
    
    //
    // BankClientSessionImpl::RequestMessageDigestsTask class
    //
    inline
    BankClientSessionImpl::RequestMessageDigestsTask::
    RequestMessageDigestsTask(
      El::Service::Callback* callback,
      Message::Transport::IdPack* message_ids)
      throw(El::Exception)
        : TaskBase(false),
          BankClientTask(callback),
          message_ids_(message_ids)
    {
      message_ids->_add_ref();
    }
      
    //
    // BankClientSessionImpl::RequestEventsTask class
    //
    inline
    BankClientSessionImpl::RequestEventsTask::RequestEventsTask(
      El::Service::Callback* callback,
      Transport::EventIdRelPack* ids)
      throw(El::Exception)
        : TaskBase(false),
          BankClientTask(callback),
          event_ids_(ids)
    {
      ids->_add_ref();
    }
    
    //
    // BankClientSessionImpl::RequestMessageDigestsTask::MessageInfo class
    //
    inline
    BankClientSessionImpl::RequestMessageDigestsTask::MessageInfo::MessageInfo(
      El::Luid event_id_val,
      uint32_t event_capacity_val,
      uint32_t bank_hash_val) throw()
        : event_id(event_id_val),
          event_capacity(event_capacity_val),
          bank_hash(bank_hash_val)
    {
    }

    //
    // BankClientSessionImpl::RequestMessageDigestsTask::MessageInfoMap class
    //
    inline
    BankClientSessionImpl::RequestMessageDigestsTask::MessageInfoMap::
    MessageInfoMap() throw(El::Exception)
    {
      set_deleted_key(Message::Id::zero);
    }
    
    //
    // BankClientSessionImpl::RequestEventsTask::EventInfoMap class
    //
    inline
    BankClientSessionImpl::RequestEventsTask::EventInfoMap::EventInfoMap()
      throw(El::Exception)
    {
      set_deleted_key(El::Luid::null);
    }
    
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_EVENT_COMMONS_BANKCLIENTSESSIONIMPL_HPP_
