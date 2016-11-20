/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Commons/Message/BankClientSessionImpl.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MESSAGE_COMMONS_BANKCLIENTSESSIONIMPL_HPP_
#define _NEWSGATE_SERVER_SERVICES_MESSAGE_COMMONS_BANKCLIENTSESSIONIMPL_HPP_

#include <vector>
#include <string>
#include <list>

#include <ext/hash_set>
#include <google/dense_hash_map>

#include <El/Exception.hpp>
#include <El/Hash/Hash.hpp>
#include <El/Stat.hpp>

#include <El/Service/Service.hpp>
#include <El/Service/ThreadPool.hpp>

#include <ace/OS.h>
#include <ace/Synch.h>
#include <ace/Guard_T.h>
#include <ace/High_Res_Timer.h>

#include <Commons/Message/TransportImpl.hpp>
#include <Commons/Search/TransportImpl.hpp>

#include <Services/Commons/Message/MessageServices.hpp>

namespace NewsGate
{
  namespace Message
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
                            NewsGate::Message::BankManager_ptr bank_manager,
                            const char* process_id,
                            const ACE_Time_Value& refresh_period,
                            const ACE_Time_Value& invalidate_timeout,
                            unsigned long message_post_retries,
                            unsigned long threads)
        throw(InvalidArg, El::Exception);
      
      virtual ~BankClientSessionImpl() throw();      

      static void register_valuetype_factories(CORBA::ORB* orb)
        throw(CORBA::Exception, El::Exception);

      //
      // Requred to be invoked before search and send_request method calls
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
      // IDL:NewsGate/Message/BankClientSession/post_messages:1.0
      //
      virtual void post_messages(
        ::NewsGate::Message::Transport::MessagePack* messages,
        ::NewsGate::Message::PostMessageReason reason,
        PostStrategy post_strategy,
        const char* validation_id)
        throw(FailedToPostMessages,
              NotReady,
              ImplementationException,
              CORBA::SystemException);

      //
      // IDL:NewsGate/Message/BankClientSession/message_sharing_offer:1.0
      //
      virtual void message_sharing_offer(
        ::NewsGate::Message::Transport::MessageSharingInfoPack*
          offered_messages,
        const char* validation_id,
        ::NewsGate::Message::Transport::IdPack_out requested_messages)
        throw(NotReady,
              ImplementationException,
              CORBA::SystemException);
      
      //
      // IDL:NewsGate/Message/BankClientSession/search:1.0
      //
      virtual void search(const SearchRequest& request,
                          SearchResult_out result)
        throw(ImplementationException,
              CORBA::SystemException);
      
      //
      // IDL:NewsGate/Message/BankClientSession/owner_bank_state:1.0
      //
      virtual BankState owner_bank_state(CORBA::ULongLong feed_id);

      //
      // IDL:NewsGate/Message/BankClientSession/send_request:1.0
      //
      virtual RequestResult* send_request(
        ::NewsGate::Message::Transport::Request* req,
        ::NewsGate::Message::Transport::Response_out resp)
        throw(ImplementationException,
              CORBA::SystemException);

      //
      // IDL:NewsGate/Message/BankClientSession/get_message:1.0
      //
      virtual ::NewsGate::Message::Transport::StoredMessage*
      get_message(::NewsGate::Message::Transport::Id* message_id,
                  ::CORBA::ULongLong gm_flags,
                  ::CORBA::Long img_index,
                  ::CORBA::Long thumb_index,
                  ::CORBA::ULongLong message_signature)
        throw(NotFound,
              NotReady,
              ImplementationException,
              CORBA::SystemException);

      //
      // IDL:NewsGate/Message/BankClientSession/bank_manager:1.0
      //
      virtual BankManager_ptr bank_manager() throw(CORBA::SystemException);

      //
      // IDL:NewsGate/Message/BankClientSession/process_id:1.0
      //
      virtual char* process_id() throw(CORBA::SystemException);      
      
      typedef El::Corba::SmartRef<NewsGate::Message::Bank> BankRef;

      struct BankRecord
      {
        BankRef bank;
        ACE_Time_Value invalidated;
      };

      typedef std::vector<BankRecord> BankRecordArray;

/*      
      typedef std::vector<std::string> EndpointArray;
      
      struct ColoFrontend
      {
        enum Relation
        {
          RL_OWN,
          RL_SOURCE,
          RL_SINK
        };

        Relation relation;
        std::string process_id;
        EndpointArray search_endpoints;
        EndpointArray limited_endpoints;

        ColoFrontend() throw() : relation(RL_OWN) {}

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);
        
        void marshal(CORBA::DataOutputStream* os) const;
        void unmarshal(CORBA::DataInputStream* is);
      };

      typedef std::vector<ColoFrontend> ColoFrontendArray;
*/    
// Not thread safe      
      BankRecordArray& banks() throw();
      const BankRecordArray& banks() const throw();
      Transport::ColoFrontendArray& colo_frontends() throw();

      unsigned long threads() const throw();
      
      Transport::ColoFrontendArray
      colo_frontends(Transport::ColoFrontend::Relation relation) const
        throw(El::Exception);

    private:      
      virtual CORBA::ValueBase* _copy_value() throw(CORBA::NO_IMPLEMENT);
      
      NewsGate::Message::Bank_ptr bank_for_message_post(
        std::string& bank_ior,
        const char* validation_id)
        throw(Exception, El::Exception, CORBA::Exception);

      NewsGate::Message::Bank_ptr get_next_bank(
        size_t from,
        size_t till,
        std::string& bank_ior,
        const char* validation_id)
        throw(Exception, El::Exception, CORBA::Exception);

      template<typename MESSAGE_PACK>
      void post_messages_to_random_bank(
        typename MESSAGE_PACK::Type* message_pack,
        ::NewsGate::Message::PostMessageReason reason,
        const char* validation_id)
        throw(FailedToPostMessages,
              NotReady,
              ImplementationException,
              CORBA::SystemException);

      template<typename MESSAGE_PACK>
      void post_messages_by_id(
        typename MESSAGE_PACK::Type* message_pack,
        ::NewsGate::Message::PostMessageReason reason,
        const char* validation_id)
        throw(FailedToPostMessages,
              NotReady,
              ImplementationException,
              CORBA::SystemException);

      template<typename MESSAGE_PACK>
      void push_best_effort(
        typename MESSAGE_PACK::Type* message_pack,
        ::NewsGate::Message::PostMessageReason reason,
        const char* validation_id)
        throw(FailedToPostMessages,
              NotReady,
              ImplementationException,
              CORBA::SystemException);
        
      void invalidate_bank(const char* bank_ior) throw(El::Exception);

      bool validate_bank(BankRecord& bank, const char* validation_id)
        throw(El::Exception);
      
      void refresh_session() throw(Exception, El::Exception, CORBA::Exception);

    ::NewsGate::Message::Transport::StoredMessage*
    search_message(const Message::Id& id,
                   uint64_t gm_flags,
                   int32_t img_index,
                   int32_t thumb_index,
                   Message::Signature signature)
      throw(NotFound,
            ImplementationException,
            CORBA::SystemException);
      
    private:

      static const ACE_Time_Value MESSAGE_POST_SLEEP_ON_FAILURE;

      typedef ACE_RW_Thread_Mutex     Mutex_;
      typedef ACE_Read_Guard<Mutex_>  ReadGuard;
      typedef ACE_Write_Guard<Mutex_> WriteGuard;
     
      mutable Mutex_ lock_;

      struct PostMessagesInfo
      {
        NewsGate::Message::Transport::MessagePack_var msg_pack;
        BankRecord bank_record;
      };

      typedef std::vector<PostMessagesInfo> PostMessagesInfoArray;

      struct MessageOfferInfo
      {
        NewsGate::Message::Transport::MessageSharingInfoPack_var pack;
        BankRecord bank_record;
      };

      typedef std::vector<MessageOfferInfo> MessageOfferInfoArray;

      class RequestTask : public virtual El::Service::ThreadPool::TaskBase
      {
      public:
        
        RequestTask(El::Service::Callback* callback,
                    ::NewsGate::Message::Transport::Request* req)
          throw(El::Exception);

        virtual ~RequestTask() throw() {}
        
        void add_bank(const BankRef& bank,
                      El::Service::ThreadPool* thread_pool)
          throw(El::Exception);

        void wait() throw(El::Exception);

      public:
        ::NewsGate::Message::Transport::Response_var response;
        RequestResultCode result;
        std::string error_desc;

        virtual void execute() throw(El::Exception);
        
      private:
        typedef ACE_Thread_Mutex       Mutex;
        typedef ACE_Read_Guard<Mutex>  ReadGuard;
        typedef ACE_Write_Guard<Mutex> WriteGuard;

        typedef ACE_Condition<ACE_Thread_Mutex> Condition;
        typedef std::list<BankRef> BankList;

        mutable Mutex lock_;
        Condition request_completed_;
        
        El::Service::Callback* callback_;
        
        BankList banks_;
        unsigned long banks_count_;
        unsigned long completed_requests_;
        
        ::NewsGate::Message::Transport::Request_var request_;
      };

      typedef El::RefCount::SmartPtr<RequestTask> RequestTask_var;

      class MessageSearch : public virtual El::Service::ThreadPool::TaskBase
      {
      public:
        MessageSearch(El::Service::Callback* callback,
                      const SearchRequest& request,
                      unsigned long reserve_banks) throw(El::Exception);
        
        virtual ~MessageSearch() throw();

        struct SearchResult
        {
          BankRef bank;
          NewsGate::Message::MatchedMessages_var match;
        };
        
        typedef std::vector<SearchResult> SearchResultArray;
        typedef std::vector<BankRef> BankArray;

        BankArray banks;
        unsigned long banks_count;
        SearchResultArray results;
        bool com_failure;
        
        // Is not thread safe; assumed ot be called before banks requesting.
        void add_bank(const BankRef& bank) throw(El::Exception);
        
        void wait() throw(El::Exception);

        virtual void execute() throw(El::Exception);

      private:
        typedef ACE_Thread_Mutex       Mutex;
        typedef ACE_Read_Guard<Mutex>  ReadGuard;
        typedef ACE_Write_Guard<Mutex> WriteGuard;

        mutable Mutex lock_;

        El::Service::Callback* callback_;
        const SearchRequest& request_;
     
        typedef ACE_Condition<ACE_Thread_Mutex> Condition;
        Condition search_completed_;
        
        unsigned long completed_requests_;
      };

      typedef El::RefCount::SmartPtr<MessageSearch> MessageSearch_var;

      class MessageFetch : public virtual El::Service::ThreadPool::TaskBase
      {
      public:
        MessageFetch(El::Service::Callback* callback,
                     unsigned long reserve_requests,
                     unsigned long long gm_flags,
                     long img_index,
                     long thumb_index) throw(El::Exception);
        
        virtual ~MessageFetch() throw();

        struct FetchRequest
        {
          BankRef bank;
          Transport::IdPack_var message_ids;
        };
        
        typedef std::vector<FetchRequest> FetchRequestArray;
        typedef std::vector<Transport::MessagePack_var> ResultArray;

        FetchRequestArray requests;
        unsigned long requests_count;
        ResultArray results;
        
        // Is not thread safe; assumed ot be called before banks requesting.
        void add_request(const BankRef& bank,
                         Transport::IdPack* message_ids) throw(El::Exception);
        
        void wait() throw(El::Exception);

        virtual void execute() throw(El::Exception);

      private:
        typedef ACE_Thread_Mutex       Mutex;
        typedef ACE_Read_Guard<Mutex>  ReadGuard;
        typedef ACE_Write_Guard<Mutex> WriteGuard;

        mutable Mutex lock_;

        El::Service::Callback* callback_;
     
        typedef ACE_Condition<ACE_Thread_Mutex> Condition;
        Condition fetch_completed_;
        
        unsigned long completed_requests_;
        unsigned long long gm_flags_;
        long img_index_;
        long thumb_index_;
      };

      typedef El::RefCount::SmartPtr<MessageFetch> MessageFetch_var;

      class MessageIdMap :
        public google::dense_hash_map<Message::Id,
                                      unsigned long,
                                      Message::MessageIdHash>
      {
      public:
        MessageIdMap() throw(El::Exception);
      };

      Search::Result* search_messages(
        const SearchRequest& request,
        size_t duplicate_factor,
        Categorizer::Category::Locale& category_locale,
        MessageSearch_var& search,
        MessageIdMap& msg_id_map,
        size_t& total_results_count,
        size_t& results_left,
        size_t& suppressed_messages,
        bool& messages_loaded)
        throw(ImplementationException, CORBA::SystemException, El::Exception);

      typedef std::auto_ptr<Categorizer::Category::Locale> CategoryLocalePtr;
      
      SearchResult* create_result(uint64_t etag,
                                  uint64_t gm_flags,
                                  Search::Result* search_result,
                                  CategoryLocalePtr& category_locale,
                                  size_t total_results_count,
                                  size_t suppressed_messages,
                                  bool messages_loaded)
        throw(El::Exception, CORBA::SystemException);
      
      Transport::StoredMessagePack*
      fetch_messages(MessageSearch* search,
                     const MessageIdMap& msg_id_map,
                     uint64_t gm_flags,
                     int32_t img_index,
                     int32_t thumb_index,
                     Search::MessageInfoArray& message_infos)
        throw(El::Exception, CORBA::SystemException);
      
    private:
      
      CORBA::ORB_var orb_;
      El::Service::Callback* callback_;
      NewsGate::Message::BankManager_var bank_manager_;
      std::string process_id_;
      BankRecordArray banks_;
      std::auto_ptr<Transport::ColoFrontendArray> colo_frontends_;
      ACE_Time_Value refreshed_;
      ACE_Time_Value refresh_period_;
      ACE_Time_Value invalidate_timeout_;
      unsigned long message_post_bank_;
      unsigned long message_post_retries_;
      
      unsigned long threads_;
      El::Service::ThreadPool_var thread_pool_;

    public:
      static El::Stat::TimeMeter search_meter;

    private:
      BankClientSessionImpl(BankClientSessionImpl&);
      void operator=(BankClientSessionImpl&);
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
        NewsGate::Message::BankManager_ptr bank_manager,
        const char* process_id,
        const ACE_Time_Value& refresh_period,
        const ACE_Time_Value& invalidate_timeout,
        unsigned long message_post_retries,
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
  namespace Message
  {
/*    
    //
    // BankClientSessionImpl::ColoFrontend class
    //
    inline
    void
    BankClientSessionImpl::ColoFrontend::write(El::BinaryOutStream& bstr)
      const throw(El::Exception)
    {
      bstr << (uint32_t)relation << process_id;
      bstr.write_array(search_endpoints);
      bstr.write_array(limited_endpoints);
    }
    
    inline
    void
    BankClientSessionImpl::ColoFrontend::read(El::BinaryInStream& bstr)
      throw(El::Exception)
    {
      uint32_t rel = 0;
      bstr >> rel >> process_id;
      bstr.read_array(search_endpoints);
      bstr.read_array(limited_endpoints);

      relation = (Relation)rel;
    }
    
    inline
    void
    BankClientSessionImpl::ColoFrontend::marshal(CORBA::DataOutputStream* os)
      const
    {
      os->write_ulong(relation);
      os->write_string(process_id.c_str());
      
      os->write_ulong(search_endpoints.size());

      for(EndpointArray::const_iterator i(search_endpoints.begin()),
            e(search_endpoints.end()); i != e; ++i)
      {
        os->write_string(i->c_str());
      }

      os->write_ulong(limited_endpoints.size());

      for(EndpointArray::const_iterator i(limited_endpoints.begin()),
            e(limited_endpoints.end()); i != e; ++i)
      {
        os->write_string(i->c_str());
      }
    }
      
    inline
    void
    BankClientSessionImpl::ColoFrontend::unmarshal(CORBA::DataInputStream* is)
    {
      search_endpoints.clear();
      limited_endpoints.clear();

      relation = (Relation)is->read_ulong();

      CORBA::String_var pid = is->read_string();
      process_id = pid.in();
      
      unsigned long count = is->read_ulong();      
      search_endpoints.reserve(count);

      while(count--)
      {
        CORBA::String_var ep = is->read_string();
        search_endpoints.push_back(ep.in());
      }

      count = is->read_ulong();      
      limited_endpoints.reserve(count);

      while(count--)
      {
        CORBA::String_var ep = is->read_string();
        limited_endpoints.push_back(ep.in());
      }
    }
*/
    //
    // BankClientSessionImpl::RequestTask class
    //
    inline
    BankClientSessionImpl::RequestTask::RequestTask(
      El::Service::Callback* callback,
      ::NewsGate::Message::Transport::Request* req) throw(El::Exception)
        : TaskBase(false),
          result(BankClientSession::RRC_OK),
          request_completed_(lock_),
          callback_(callback),
          banks_count_(0),
          completed_requests_(0),
          request_(req)
    {
      req->_add_ref();
    }
      
    inline
    void
    BankClientSessionImpl::RequestTask::add_bank(
      const BankRef& bank,
      El::Service::ThreadPool* thread_pool) throw(El::Exception)
    { 
      {
        WriteGuard guard(lock_);
      
        banks_.push_back(bank);
        banks_count_++;
      }

      if(thread_pool)
      {
        thread_pool->execute(this);
      }
    }

    inline
    void
    BankClientSessionImpl::RequestTask::wait() throw(El::Exception)
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
          ostr << "BankClientSessionImpl::RequestTask::wait: "
            "request_completed_.wait() failed. "
            "Errno " << error << ". Description:" << std::endl
               << ACE_OS::strerror(error);
          
          throw Exception(ostr.str());
        }
      }
    }

    //
    // BankClientSessionImpl::MessageSearch class
    //
    inline
    BankClientSessionImpl::MessageSearch::MessageSearch(
      El::Service::Callback* callback,
      const SearchRequest& request,
      unsigned long reserve_banks) throw(El::Exception)
        : TaskBase(false),
          banks_count(0),
          com_failure(false),
          callback_(callback),
          request_(request),
          search_completed_(lock_),
          completed_requests_(0)
    {
      banks.reserve(reserve_banks);
      results.reserve(reserve_banks);
    }

    inline
    BankClientSessionImpl::MessageSearch::~MessageSearch() throw()
    {
    }

    inline
    void
    BankClientSessionImpl::MessageSearch::add_bank(const BankRef& bank)
      throw(El::Exception)
    {
      banks.push_back(bank);
      banks_count++;
    }

    inline
    void
    BankClientSessionImpl::MessageSearch::wait() throw(El::Exception)
    {
      while(true)
      {
        ReadGuard guard(lock_);
        
        if(completed_requests_ == banks_count)
        {
          break;
        }
        
        if(search_completed_.wait(0))
        {
          int error = ACE_OS::last_error();
          
          std::ostringstream ostr;
          ostr << "BankClientSessionImpl::MessageSearch::wait: "
            "search_completed_.wait() failed. "
            "Errno " << error << ". Description:" << std::endl
               << ACE_OS::strerror(error);
          
          throw Exception(ostr.str());
        }
      }
    }

    //
    // BankClientSessionImpl::MessageIdMap class
    //
    inline
    BankClientSessionImpl::MessageIdMap::MessageIdMap() throw(El::Exception)
    {
      set_empty_key(Message::Id::zero);
      set_deleted_key(Message::Id::nonexistent);
    }
    
    //
    // BankClientSessionImpl::MessageFetch class
    //
    inline
    BankClientSessionImpl::MessageFetch::MessageFetch(
      El::Service::Callback* callback,
      unsigned long reserve_requests,
      unsigned long long gm_flags,
      long img_index,
      long thumb_index) throw(El::Exception)
        : TaskBase(false),
          requests_count(0),
          callback_(callback),
          fetch_completed_(lock_),
          completed_requests_(0),
          gm_flags_(gm_flags),
          img_index_(img_index),
          thumb_index_(thumb_index)
    {
      requests.reserve(reserve_requests);
      results.reserve(reserve_requests);
    }

    inline
    BankClientSessionImpl::MessageFetch::~MessageFetch() throw()
    {
    }

    inline
    void
    BankClientSessionImpl::MessageFetch::add_request(
      const BankRef& bank,
      Transport::IdPack* message_ids) throw(El::Exception)
    {
      FetchRequest request;

      request.bank = bank;
      request.message_ids = message_ids;
      message_ids->_add_ref();
      
      requests.push_back(request);
      requests_count++;
    }

    inline
    void
    BankClientSessionImpl::MessageFetch::wait() throw(El::Exception)
    {
      while(true)
      {
        ReadGuard guard(lock_);
        
        if(completed_requests_ == requests_count)
        {
          break;
        }
        
        if(fetch_completed_.wait(0))
        {
          int error = ACE_OS::last_error();
          
          std::ostringstream ostr;
          ostr << "BankClientSessionImpl::MessageFetch::wait: "
            "fetch_completed_.wait() failed. "
            "Errno " << error << ". Description:" << std::endl
               << ACE_OS::strerror(error);
          
          throw Exception(ostr.str());
        }
      }
    }
    
    //
    // BankClientSessionImpl class
    //
    inline
    BankClientSessionImpl::BankClientSessionImpl(
      CORBA::ORB_ptr orb,
      NewsGate::Message::BankManager_ptr bank_manager,
      const char* process_id,
      const ACE_Time_Value& refresh_period,
      const ACE_Time_Value& invalidate_timeout,
      unsigned long message_post_retries,
      unsigned long threads)
      throw(InvalidArg, El::Exception)
        : orb_(CORBA::ORB::_duplicate(orb)),
          callback_(0),
          bank_manager_(NewsGate::Message::BankManager::_duplicate(
                          bank_manager)),
          process_id_(process_id ? process_id : ""),
          colo_frontends_(new Transport::ColoFrontendArray()),
          refresh_period_(refresh_period),
          invalidate_timeout_(invalidate_timeout),
          message_post_bank_(0),
          message_post_retries_(message_post_retries),
          threads_(threads)
    {
      if(CORBA::is_nil(orb))
      {
        throw InvalidArg("NewsGate::Message::BankClientSessionImpl::"
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
    unsigned long
    BankClientSessionImpl::threads() const throw()
    {
      ReadGuard guard(lock_);
      return threads_;
    }
    
    inline
    CORBA::ValueBase*
    BankClientSessionImpl::_copy_value() throw(CORBA::NO_IMPLEMENT)
    {
      BankClientSessionImpl_var res =
        new BankClientSessionImpl(orb_.in(),
                                  bank_manager_.in(),
                                  process_id_.c_str(),
                                  refresh_period_,
                                  invalidate_timeout_,
                                  message_post_retries_,
                                  threads_);

      res->callback_ = callback_;
      res->banks_ = banks_;
      res->refreshed_ = refreshed_;
      res->message_post_bank_ = message_post_bank_;
      res->colo_frontends_ = colo_frontends_;
  
      return res._retn();
    }

    inline
    Transport::ColoFrontendArray&
    BankClientSessionImpl::colo_frontends() throw()
    {
      return *colo_frontends_;
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

    inline
    BankClientSession::BankState
    BankClientSessionImpl::owner_bank_state(CORBA::ULongLong feed_id)
    {
      ReadGuard guard(lock_);

      if(banks_.size() == 0)
      {
        return BS_PROBABLY_VALID;
      }

      unsigned long index = feed_id % banks_.size();
      const BankRecord& bank_record = banks_[index];
      
      return bank_record.invalidated == ACE_Time_Value::zero ? BS_VALID :
        (bank_record.invalidated + invalidate_timeout_ >
         ACE_OS::gettimeofday() ? BS_INVALIDATED : BS_PROBABLY_VALID);
    }
    
    template<typename MESSAGE_PACK>
    void
    BankClientSessionImpl::push_best_effort(
      typename MESSAGE_PACK::Type* message_pack,
      ::NewsGate::Message::PostMessageReason reason,
      const char* validation_id)
      throw(FailedToPostMessages,
            NotReady,
            ImplementationException,
            CORBA::SystemException)
    {
      try
      {
        post_messages_by_id<MESSAGE_PACK>(message_pack, reason, validation_id);
      }
      catch(FailedToPostMessages& e)
      {
        typename MESSAGE_PACK::Var message_pack =
          dynamic_cast<typename MESSAGE_PACK::Type*>(e.messages.in());

        if(message_pack.in() == 0)
        {
          throw Exception(
            "NewsGate::Message::BankClientSessionImpl::push_best_effort: "
            "dynamic_cast<MESSAGE_PACK::Type*> failed");
        }

        message_pack->_add_ref();
        
        post_messages_to_random_bank<MESSAGE_PACK>(message_pack.in(),
                                                   reason,
                                                   validation_id);
      }
    }

    template<typename MESSAGE_PACK>
    void
    BankClientSessionImpl::post_messages_by_id(
      typename MESSAGE_PACK::Type* message_pack,
      ::NewsGate::Message::PostMessageReason reason,
      const char* validation_id)
      throw(FailedToPostMessages,
            NotReady,
            ImplementationException,
            CORBA::SystemException)
    {
//      std::cerr << "*** post_messages_by_message_id\n";
      
      try
      {
        refresh_session();
        
        typename MESSAGE_PACK::MessageArray& msg_entities =
          message_pack->entities();
        
        typename MESSAGE_PACK::Var failed_to_post_msgs;
        std::ostringstream failed_to_post_msgs_desc;
        
        PostMessagesInfoArray post_msg_infos;

        {
          WriteGuard guard(lock_);
      
          if(banks_.size() == 0)
          {
            NotReady e;
            e.reason =
              CORBA::string_dup("BankClientSessionImpl::"
                                "post_messages_by_id_hash: "
                                "no banks available");
            throw e;
          }
      
          post_msg_infos.resize(banks_.size());

          typedef __gnu_cxx::hash_set<std::string, El::Hash::String> StringSet;
          StringSet not_validated_banks;

          for(typename MESSAGE_PACK::MessageArray::iterator it =
                msg_entities.begin(); it != msg_entities.end(); it++)
          {
            unsigned long index = it->get_id().src_id() % banks_.size();
/*
            std::cerr << "PM: " << it->id().string() << " " << index << "/"
                      << banks_.size() << std::endl;
*/
            BankRecord& bank_record = banks_[index];

            if(validate_bank(bank_record, validation_id))
            {
              NewsGate::Message::Transport::MessagePack_var& msg_pack =
                post_msg_infos[index].msg_pack;

              if(msg_pack.in() == 0)
              {
                msg_pack = MESSAGE_PACK::Init::create(
                  new typename MESSAGE_PACK::MessageArray());

                post_msg_infos[index].bank_record = bank_record;
              }
            
              typename MESSAGE_PACK::Type* message_pack =
                dynamic_cast<typename MESSAGE_PACK::Type*>(msg_pack.in());

              if(message_pack == 0)
              {
                ImplementationException e;
              
                e.description = CORBA::string_dup(
                  "NewsGate::Message::BankClientSessionImpl::"
                  "post_messages_by_id_hash: "
                  "dynamic_cast<MESSAGE_PACK::Type*> failed (2)");

                throw e;
              }

              message_pack->entities().push_back(*it);
            }
            else
            {
              if(failed_to_post_msgs.in() == 0)
              {
                failed_to_post_msgs =
                  MESSAGE_PACK::Init::create(
                    new typename MESSAGE_PACK::MessageArray());

                failed_to_post_msgs_desc << "BankClientSessionImpl::"
                  "post_messages_by_id_hash: failed to post to banks:";
              }

              std::string bank_ior = bank_record.bank.reference();
              
              if(not_validated_banks.find(bank_ior) ==
                 not_validated_banks.end())
              {
                not_validated_banks.insert(bank_ior);
              
                failed_to_post_msgs_desc << std::endl << bank_ior
                                         << " not validated";
              }
            
              failed_to_post_msgs->entities().push_back(*it);
            }
          }
        }

        for(PostMessagesInfoArray::iterator it = post_msg_infos.begin();
            it != post_msg_infos.end(); it++)
        {
          NewsGate::Message::Transport::MessagePack_var& msg_pack =
            it->msg_pack;

          if(msg_pack.in() == 0)
          {
            continue;
          }
        
          typename MESSAGE_PACK::Type* message_pack =
            dynamic_cast<typename MESSAGE_PACK::Type*>(msg_pack.in());

          if(message_pack == 0)
          {
            ImplementationException e;
              
            e.description = CORBA::string_dup(
              "NewsGate::Message::BankClientSessionImpl::"
              "post_messages_by_id_hash: "
              "dynamic_cast<MESSAGE_PACK::Type*> failed (3)");

            throw e;
          }
        
          BankRecord& bank_record = it->bank_record;
          std::string error;
          
          try
          {            
/*
            std::cerr << "Posting messages ("
                      << message_pack->entities().size() << ") ...\n";
            
            ACE_High_Res_Timer timer;
            timer.start();
*/
            NewsGate::Message::Bank_var bank = bank_record.bank.object();
            bank->post_messages(msg_pack.in(), reason, validation_id);
/*
            timer.stop();
            ACE_Time_Value tm;
            timer.elapsed_time(tm);

            std::cerr << "posted, time " << El::Moment::time(tm) << std::endl;
*/
          }
          catch(const NotReady& e)
          {
            std::ostringstream ostr;
            ostr << "CORBA::NotReady: " << e.reason.in();
            error = ostr.str();
          }
          catch(const ImplementationException& e)
          {
            std::ostringstream ostr;
            ostr << "CORBA::ImplementationException: " << e.description.in();
            error = ostr.str();
          }
          catch(const CORBA::Exception& e)
          {
            std::ostringstream ostr;
            ostr << "CORBA::Exception: " << e;
            error = ostr.str();
          }

          if(!error.empty())
          {
            //
            // Putting unsent messages into failed_to_post_msgs list
            //
            if(failed_to_post_msgs.in() == 0)
            {
              failed_to_post_msgs = MESSAGE_PACK::Init::create(
                new typename MESSAGE_PACK::MessageArray());

              failed_to_post_msgs_desc << "BankClientSessionImpl::"
                "post_messages_by_id_hash: failed to post to banks:";
            }

            failed_to_post_msgs_desc << std::endl
                                     << bank_record.bank.reference()
                                     << " failed (" << error << ")";

            const typename MESSAGE_PACK::MessageArray& entities =
              message_pack->entities();
            
            for(typename MESSAGE_PACK::MessageArray::const_iterator
                  it = entities.begin(); it != entities.end(); it++)
            {
              failed_to_post_msgs->entities().push_back(*it);
            }

            //
            // Invalidating bank
            //

            WriteGuard guard(lock_);
            invalidate_bank(bank_record.bank.reference().c_str());
          }
        
          msg_pack = 0;
        }

        if(failed_to_post_msgs.in() != 0)
        {
          FailedToPostMessages e;
        
          e.messages = failed_to_post_msgs._retn();
          e.description =
            CORBA::string_dup(failed_to_post_msgs_desc.str().c_str());

          throw e;
        }
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "BankClientSessionImpl::post_messages_by_id_hash: "
          "El::Exception caught. Description:\n" << e.what();
        
        ImplementationException ex;
        ex.description = CORBA::string_dup(ostr.str().c_str());
        
        throw ex;
      }
      
    }
    
    template<typename MESSAGE_PACK>
    void
    BankClientSessionImpl::post_messages_to_random_bank(
      typename MESSAGE_PACK::Type* message_pack,
      ::NewsGate::Message::PostMessageReason reason,
      const char* validation_id)
      throw(FailedToPostMessages,
            NotReady,
            ImplementationException,
            CORBA::SystemException)
    {
//      std::cerr << "*** post_messages_to_random_bank\n";

      try
      {
        unsigned long tries = 0;
        
        {
          ReadGuard guard(lock_);
          tries = message_post_retries_ + 1;
        }

        while(tries--)
        {
          std::string bank_ior;
          std::string error;

          try
          {
            NewsGate::Message::Bank_var bank =
              bank_for_message_post(bank_ior, validation_id);
            
            bank->post_messages(message_pack, reason, validation_id);
            break;
          }
          catch(const NotReady& e)
          {
            std::ostringstream ostr;
            ostr << "CORBA::NotReady: " << e.reason.in();
            error = ostr.str();
          }
          catch(const ImplementationException& e)
          {
            std::ostringstream ostr;
            ostr << "CORBA::ImplementationException: " << e.description.in();
            error = ostr.str();
          }
          catch(const CORBA::Exception& e)
          {
            std::ostringstream ostr;
            ostr << "CORBA::Exception: " << e;
            error = ostr.str();
          }

          if(!error.empty())
          {
            WriteGuard guard(lock_);

            if(!bank_ior.empty())
            {
              invalidate_bank(bank_ior.c_str());

              if(tries)
              {
                guard.release();
              
                ACE_OS::sleep(MESSAGE_POST_SLEEP_ON_FAILURE);
                continue;
              }
            }

            FailedToPostMessages ex;
              
            message_pack->_add_ref();
            ex.messages = message_pack;

            std::ostringstream ostr;
            ostr << "NewsGate::Message::BankClientSessionImpl::"
              "post_messages_to_random_bank: "
                 << "failed to post messages. Reason:\n" << error;
            
            ex.description = CORBA::string_dup(ostr.str().c_str());            
            throw ex;
          }
        }
        
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::BankClientSessionImpl::"
          "post_messages_to_random_bank: "
             << "El::Exception caught. Description:\n" << e;

        ImplementationException ex;
        ex.description = CORBA::string_dup(ostr.str().c_str());

        throw ex;
      }
      
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
        throw InvalidArg("NewsGate::Message::BankClientSessionImpl_init::"
                         "BankClientSessionImpl_init: orb not defined");
      }
    }
    
    inline
    BankClientSessionImpl*
    BankClientSessionImpl_init::create(
      CORBA::ORB_ptr orb,
      NewsGate::Message::BankManager_ptr bank_manager,
      const char* process_id,
      const ACE_Time_Value& refresh_period,
      const ACE_Time_Value& invalidate_timeout,
      unsigned long message_post_retries,
      unsigned long threads) throw(El::Exception)
    {
      return new BankClientSessionImpl(orb,
                                       bank_manager,
                                       process_id,
                                       refresh_period,
                                       invalidate_timeout,
                                       message_post_retries,
                                       threads);
    }

    inline
    CORBA::ValueBase* 
    BankClientSessionImpl_init::create_for_unmarshal()
    {
      return create(orb_.in(),
                    0,
                    0,
                    ACE_Time_Value::zero,
                    ACE_Time_Value::zero,
                    0,
                    0);
    }

  }
}

#endif // _NEWSGATE_SERVER_SERVICES_MESSAGE_COMMONS_BANKCLIENTSESSIONIMPL_HPP_
