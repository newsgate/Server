/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Message/Commons/BankClientSessionImpl.cpp
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

#include <Commons/Message/TransportImpl.hpp>
#include <Commons/Search/TransportImpl.hpp>

#include <Services/Commons/Message/MessageServices.hpp>

#include "BankClientSessionImpl.hpp"

//#define TRACE_SEARCH_TIME

namespace NewsGate
{
  namespace Message
  {
    const ACE_Time_Value
    BankClientSessionImpl::MESSAGE_POST_SLEEP_ON_FAILURE(0, 10000);
    
    El::Stat::TimeMeter BankClientSessionImpl::search_meter(
      "BankClientSessionImpl::search", false);
      
    //
    // MessageInfoArrayIterator struct
    //
    struct MessageInfoArrayIterator
    {
      const Search::MessageInfoArray* weigted_ids;
      size_t index;
      size_t search_result_num;

      MessageInfoArrayIterator(
        const Search::MessageInfoArray* weigted_ids_val,
        size_t index_val,
        size_t search_result_num_val)
        throw();
    };

    typedef std::vector<MessageInfoArrayIterator>
    MessageInfoArrayIteratorArray;

    inline
    MessageInfoArrayIterator::MessageInfoArrayIterator(
      const Search::MessageInfoArray* weigted_ids_val,
      size_t index_val,
      size_t search_result_num_val)
      throw()
        : weigted_ids(weigted_ids_val),
          index(index_val),
          search_result_num(search_result_num_val)
    {
    }

    //
    // BankClientSessionImpl class
    //
    void
    BankClientSessionImpl::register_valuetype_factories(CORBA::ORB* orb)
      throw(CORBA::Exception, El::Exception)
    {
      CORBA::ValueFactoryBase_var factory =
        new NewsGate::Message::BankClientSessionImpl_init(orb);
      
      CORBA::ValueFactoryBase_var old = orb->register_value_factory(
        "IDL:NewsGate/Message/BankClientSession:1.0",
        factory);
    }

    void
    BankClientSessionImpl::marshal(CORBA::DataOutputStream* os)
    {
      os->write_Object(bank_manager_.in());
      os->write_string(process_id_.c_str());
      os->write_ulong(refresh_period_.sec());
      os->write_ulong(invalidate_timeout_.sec());
      os->write_ulong(message_post_retries_);
      os->write_ulong(threads_);
      
      os->write_ulong(banks_.size());

      for(BankRecordArray::const_iterator i(banks_.begin()), e(banks_.end());
          i != e; ++i)
      {
        os->write_string(i->bank.reference().c_str());
      }

      Transport::ColoFrontendPackImpl::Var colo_frontends_pack =
        new Transport::ColoFrontendPackImpl::Type(
          new Transport::ColoFrontendArray(*colo_frontends_));

      colo_frontends_pack->marshal(os);
    }

    void
    BankClientSessionImpl::unmarshal(CORBA::DataInputStream* is)
    {
      CORBA::Object_var object = is->read_Object();
      bank_manager_ = ::NewsGate::Message::BankManager::_narrow(object.in());

      if(CORBA::is_nil(bank_manager_.in()))
      {
        throw El::Corba::MARSHAL(
          "NewsGate::Message::BankClientSessionImpl::unmarshal: "
          "::NewsGate::Message::BankManager::_narrow failed");
      }

      CORBA::String_var pid = is->read_string();
      process_id_ = pid.in();

      refresh_period_ = is->read_ulong();
      invalidate_timeout_ = is->read_ulong();
      message_post_retries_ = is->read_ulong();
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

      Transport::ColoFrontendPackImpl::Var colo_frontends_pack =
        new Transport::ColoFrontendPackImpl::Type();

      colo_frontends_pack->unmarshal(is);
      colo_frontends_.reset(colo_frontends_pack->release());
      
      refreshed_ = tm;
      message_post_bank_ = 0;
    }

    BankManager_ptr
    BankClientSessionImpl::bank_manager() throw(CORBA::SystemException)
    {
      return NewsGate::Message::BankManager::_duplicate(bank_manager_.in());
    }

    char*
    BankClientSessionImpl::process_id() throw(CORBA::SystemException)
    {
      ReadGuard guard(lock_);
      return CORBA::string_dup(process_id_.c_str());
    }

    Transport::ColoFrontendArray
    BankClientSessionImpl::colo_frontends(
      Transport::ColoFrontend::Relation relation) const
      throw(El::Exception)
    {
      Transport::ColoFrontendArray frontends;
      
      ReadGuard guard(lock_);
      frontends.reserve(colo_frontends_->size());
      
      for(Transport::ColoFrontendArray::const_iterator
            i(colo_frontends_->begin()), e(colo_frontends_->end());
          i != e; ++i)
      {
        const Transport::ColoFrontend& fr = *i;
        
        if(fr.relation == relation)
        {
          frontends.push_back(fr);
        }
      }

      return frontends;
    }
    
    void
    BankClientSessionImpl::post_messages(
      ::NewsGate::Message::Transport::MessagePack* messages,
      ::NewsGate::Message::PostMessageReason reason,
      PostStrategy post_strategy,
      const char* validation_id)
      throw(FailedToPostMessages,
            NotReady,
            ImplementationException,
            CORBA::SystemException)
    {
      Transport::RawMessagePackImpl::Type* global_message_pack =
        dynamic_cast<Transport::RawMessagePackImpl::Type*>(messages);

      if(global_message_pack != 0)
      {
        switch(post_strategy)
        {
        case PS_TO_RANDOM_BANK:
          {
            post_messages_to_random_bank<
            Transport::RawMessagePackImpl>(global_message_pack,
                                           reason,
                                           validation_id);
            
            break;
          }
        case PS_DISTRIBUTE_BY_MSG_ID:
          {
            post_messages_by_id<
            Transport::RawMessagePackImpl>(global_message_pack,
                                           reason,
                                           validation_id);
          
            break;
          }
        case PS_PUSH_BEST_EFFORT:
          {
            push_best_effort<
            Transport::RawMessagePackImpl>(global_message_pack,
                                           reason,
                                           validation_id);
          
            break;
          }          
        }

        return;
      }

      Transport::StoredMessagePackImpl::Type* stored_message_pack =
        dynamic_cast<Transport::StoredMessagePackImpl::Type*>(messages);

      if(stored_message_pack != 0)
      {
        switch(post_strategy)
        {
        case PS_TO_RANDOM_BANK:
          {
            post_messages_to_random_bank<
            Transport::StoredMessagePackImpl>(stored_message_pack,
                                              reason,
                                              validation_id);
          
            break;
          }
        case PS_DISTRIBUTE_BY_MSG_ID:
          {
            post_messages_by_id<
            Transport::StoredMessagePackImpl>(stored_message_pack,
                                              reason,
                                              validation_id);
          
            break;
          }
        case PS_PUSH_BEST_EFFORT:
          {
            push_best_effort<
            Transport::StoredMessagePackImpl>(stored_message_pack,
                                              reason,
                                              validation_id);
          
            break;
          }          
        }

        return;
      }

      Transport::MessageEventPackImpl::Type* msg_event_pack =
        dynamic_cast<Transport::MessageEventPackImpl::Type*>(messages);

      if(msg_event_pack != 0)
      {
        switch(post_strategy)
        {
        case PS_TO_RANDOM_BANK:
          {
            post_messages_to_random_bank<
            Transport::MessageEventPackImpl>(msg_event_pack,
                                             reason,
                                             validation_id);
          
            break;
          }
        case PS_DISTRIBUTE_BY_MSG_ID:
          {
            post_messages_by_id<
            Transport::MessageEventPackImpl>(msg_event_pack,
                                             reason,
                                             validation_id);
          
            break;
          }
        case PS_PUSH_BEST_EFFORT:
          {
            push_best_effort<
            Transport::MessageEventPackImpl>(msg_event_pack,
                                             reason,
                                             validation_id);
          
            break;
          }          
        }

        return;
      }

      ImplementationException e;
        
      e.description = CORBA::string_dup(
        "NewsGate::Message::BankClientSessionImpl::post_messages: "
        "can't cast to any known message pack implementation type");
      
      throw e;
    }
    
    void
    BankClientSessionImpl::message_sharing_offer(
      ::NewsGate::Message::Transport::MessageSharingInfoPack* offered_messages,
      const char* validation_id,
      ::NewsGate::Message::Transport::IdPack_out requested_messages)
      throw(NotReady, ImplementationException, CORBA::SystemException)
    {
      try
      {
        Transport::IdPackImpl::Var requested_msg_pack =
          Transport::IdPackImpl::Init::create(new IdArray());          

        IdArray& requested_ids = requested_msg_pack->entities();

        MessageOfferInfoArray msg_offer_infos;

        refresh_session();

        BankRecordArray banks;
        
        {
          ReadGuard guard(lock_);
          banks = banks_;
        }
        
        if(banks.empty())
        {
          NotReady e;
            
          e.reason =
            CORBA::string_dup(
              "BankClientSessionImpl::message_sharing_offer: "
              "no banks available");
            
          throw e;
        }
      
        msg_offer_infos.resize(banks.size());

        typedef __gnu_cxx::hash_set<std::string, El::Hash::String> StringSet;
        StringSet not_validated_banks;

        Transport::MessageSharingInfoPackImpl::Type* offered_msg_pack =
          dynamic_cast<Transport::MessageSharingInfoPackImpl::Type*>(
            offered_messages);

        if(offered_msg_pack == 0)
        {
          throw Exception(
            "BankClientSessionImpl::message_sharing_offer: "
            "dynamic_cast<Transport::MessageSharingInfoPackImpl*> failed");
        }

        const Transport::MessageSharingInfoArray& offered_msg =
          offered_msg_pack->entities();

        for(Transport::MessageSharingInfoArray::const_iterator
              it = offered_msg.begin(); it != offered_msg.end(); it++)
        {
          const Transport::MessageSharingInfo& sharing_info = *it;
            
          const Id& id = sharing_info.message_id;
          size_t index = id.src_id() % banks.size();

          BankRecord& bank_record = banks[index];

          Transport::MessageSharingInfoPack_var& pack =
            msg_offer_infos[index].pack;

          if(pack.in() == 0)
          {
            pack = Transport::MessageSharingInfoPackImpl::Init::create(
              new Transport::MessageSharingInfoArray());
                
            msg_offer_infos[index].bank_record = bank_record;
          }
            
          Transport::MessageSharingInfoPackImpl::Type* pack_impl =
            dynamic_cast<Transport::MessageSharingInfoPackImpl::Type*>(
              pack.in());

          if(pack_impl == 0)
          {
            ImplementationException e;
              
            e.description = CORBA::string_dup(
              "BankClientSessionImpl::message_sharing_offer: "
              "dynamic_cast<Transport::MessageSharingInfoPackImpl*> "
              "failed (2)");
                
            throw e;
          }

          pack_impl->entities().push_back(sharing_info);
        }

        for(MessageOfferInfoArray::iterator it = msg_offer_infos.begin();
            it != msg_offer_infos.end(); it++)
        {
          Transport::MessageSharingInfoPack_var& pack = it->pack;

          if(pack.in() == 0)
          {
            continue;
          }
        
          BankRecord& bank_record = it->bank_record;
          ::NewsGate::Message::Transport::IdPack_var requested_msg;

          try
          {                  
            NewsGate::Message::Bank_var bank = bank_record.bank.object();
            
            bank->message_sharing_offer(pack.in(),
                                        validation_id,
                                        requested_msg.out());

          }
          catch(const CORBA::Exception& e)
          {
            // probably down this time,
            // will try later
//            std::cerr << "BankClientSessionImpl::message_sharing_offer: "
//                      << e << std::endl;
          }

          if(requested_msg.in() != 0)
          {
            Transport::IdPackImpl::Type* id_pack_impl =
              dynamic_cast<Transport::IdPackImpl::Type*>(requested_msg.in());

            if(id_pack_impl == 0)
            {
              ImplementationException e;
              
              e.description = CORBA::string_dup(
                "BankClientSessionImpl::message_sharing_offer: "
                "dynamic_cast<Transport::IdPackImpl*> failed (3)");
                
              throw e;
            }

            const IdArray& ids = id_pack_impl->entities();

            if(ids.size())
            {
              unsigned long size = requested_ids.size();
              requested_ids.resize(size + ids.size());
              
              for(IdArray::const_iterator it = ids.begin();
                  it != ids.end(); it++)
              {
                requested_ids[size++] = *it;
              }
            }
          }
        }

        requested_messages = requested_msg_pack._retn();
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "BankClientSessionImpl::message_sharing_offer: "
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
          new El::Service::ThreadPool(callback,
                                      "SearchThreadPool",
                                      threads_);

        thread_pool_->start();
      }
    }

    Search::Result*
    BankClientSessionImpl::search_messages(
      const SearchRequest& request,
      size_t duplicate_factor,
      Categorizer::Category::Locale& category_locale,
      MessageSearch_var& search,
      MessageIdMap& msg_id_map,
      size_t& total_results_count,
      size_t& results_left,
      size_t& suppressed_messages,
      bool& messages_loaded)
      throw(ImplementationException, CORBA::SystemException, El::Exception)
    {
      msg_id_map.clear();
      total_results_count = 0;
      results_left = 0;
      suppressed_messages = 0;

      ::NewsGate::Search::Transport::StrategyImpl::Type*
          strategy_transport = dynamic_cast<
          ::NewsGate::Search::Transport::StrategyImpl::Type*>(
            request.strategy.in());
      
      if(strategy_transport == 0)
      {
        throw Exception("BankClientSessionImpl::search_messages: "
                        "dynamic_cast failed for request.strategy");
      }

    // Need to serialize valuetype in advance as same structures will
    // be sent in concurrent requests, so can't delay serialization
    // till marshal phase
      strategy_transport->serialize();      
      
      SearchRequest full_request = request;
        
      full_request.start_from = 0;
      
      full_request.results_count =
        (request.start_from + request.results_count) * duplicate_factor;

      MessageSearch_var prev_search;
      
      if(search.in() != 0)
      {
        prev_search = search.retn();
      }
        
      {
        ReadGuard guard(lock_);

        if(thread_pool_.in() == 0)
        {
          throw Exception("BankClientSessionImpl::search_messages: call "
                          "BankClientSessionImpl::init_threads first");  
        }

        search = new MessageSearch(callback_, full_request, banks_.size());

        if(prev_search.in() != 0)
        {
          search->com_failure = prev_search->com_failure;
          
          MessageSearch::SearchResultArray& search_results =
            prev_search->results;
          
          for(MessageSearch::SearchResultArray::iterator
                it = search_results.begin(); it != search_results.end(); )
          {
            MatchedMessages_var match = it->match;
          
            Search::Transport::ResultImpl::Type* search_res =
              dynamic_cast<Search::Transport::ResultImpl::Type*>(
                match->search_result.in());

            if(search_res == 0)
            {
              throw Exception(
                "BankClientSessionImpl::search_messages: dynamic_cast<"
                "Search::Transport::ResultImpl::Type*> failed");
            }

            if(search_res->entity().message_infos->size() <
               match->total_matched_messages)
            {
              search->add_bank(it->bank);
              it = search_results.erase(it);
            }
            else
            {
              it++;
            }
          }
        }
        else
        {
          for(BankRecordArray::const_iterator it = banks_.begin();
              it != banks_.end(); it++)
          {
            if(it->invalidated == ACE_Time_Value::zero)
            {
              search->add_bank(it->bank);
            }
          }
        }

        guard.release();
        
        unsigned long banks_count = search->banks_count;
        
        {
          //Need lock to make CORBA-requests executed most close to each other
          WriteGuard guard(lock_);
          
          while(banks_count--)
          {
            thread_pool_->execute(search.in());
          }
        }
      }
        
      search->wait();

      //
      // Taking top messages
      //
        
      if(prev_search.in() != 0)
      {
        search->results.insert(search->results.end(),
                               prev_search->results.begin(),
                               prev_search->results.end());

        prev_search = 0;
      }

      const MessageSearch::SearchResultArray& search_results =
        search->results;

      size_t top_results_count = 0;
      messages_loaded = !search_results.empty() && !search->com_failure;

//      bool ml = messages_loaded;
      
      for(MessageSearch::SearchResultArray::const_iterator
            ib(search_results.begin()), it(ib), ie(search_results.end());
          it != ie; ++it)
      {
        MatchedMessages_var match = it->match;

        if(it == ib)
        {
          Transport::CategoryLocaleImpl::Type* cl =
            dynamic_cast<Transport::CategoryLocaleImpl::Type*>(
              match->category_locale.in());
          
          if(cl == 0)
          {
            throw Exception(
              "BankClientSessionImpl::search_messages: dynamic_cast<"
              "Transport::CategoryLocaleImpl::Type*> failed");
          }
          
          category_locale = cl->entity();
        }
          
        Search::Transport::ResultImpl::Type* search_result =
          dynamic_cast<Search::Transport::ResultImpl::Type*>(
            match->search_result.in());

        if(search_result == 0)
        {
          throw Exception(
            "BankClientSessionImpl::search_messages: dynamic_cast<"
            "Search::Transport::ResultImpl::Type*> failed");
        }

        unsigned long ids_count =
          search_result->entity().message_infos->size();

        top_results_count += ids_count;
        total_results_count += match->total_matched_messages;
        results_left += match->total_matched_messages - ids_count;
        suppressed_messages += match->suppressed_messages;
        messages_loaded &= match->messages_loaded;
      }

//      std::cerr << "AAAAAAAAAAAAAA: " << messages_loaded << "/" << ml
//                << std::endl;

      Search::ResultPtr joined_result(new Search::Result());

      Search::MessageInfoArray& joined_message_infos =
        *joined_result->message_infos;

      joined_message_infos.reserve(top_results_count);

      unsigned long index = 0;
        
      MessageInfoArrayIteratorArray weighted_msg_ids_arrays;
      weighted_msg_ids_arrays.reserve(search_results.size());
        
      for(MessageSearch::SearchResultArray::const_iterator
            it(search_results.begin()), ie(search_results.end()); it != ie;
          ++it, ++index)
      {
        MatchedMessages_var match = it->match;
          
        Search::Transport::ResultImpl::Type* search_result =
          dynamic_cast<Search::Transport::ResultImpl::Type*>(
            match->search_result.in());

        if(search_result == 0)
        {
          throw Exception(
            "BankClientSessionImpl::search_messages: dynamic_cast<"
            "Search::Transport::ResultImpl::Type*> failed (2)");
        }

        const Search::Result& sr = search_result->entity();

        if(joined_result->min_weight > sr.min_weight)
        {
          joined_result->min_weight = sr.min_weight;
        }
        
        if(joined_result->max_weight < sr.max_weight)
        {
          joined_result->max_weight = sr.max_weight;
        }

        if(sr.message_infos->size())
        {
          weighted_msg_ids_arrays.push_back(
            MessageInfoArrayIterator(sr.message_infos.get(), 0, index));
        }

        joined_result->stat.absorb(sr.stat);        
      }

      while(!weighted_msg_ids_arrays.empty())
      {  
        for(MessageInfoArrayIteratorArray::iterator it =
              weighted_msg_ids_arrays.begin();
            it != weighted_msg_ids_arrays.end(); )
        {
          Search::MessageInfoArray::const_iterator wit =
            it->weigted_ids->begin() + it->index++;

          joined_message_infos.push_back(Search::MessageInfo());

// It's important not to steal but copy as the original data
// will be reused on probable send_message call with
// duplicate_factor > 1
//          joined_message_infos.rbegin()->steal(*wit);
          
          *joined_message_infos.rbegin() = *wit;
          
          msg_id_map[wit->wid.id] = it->search_result_num;
              
          if(it->index == it->weigted_ids->size())
          {
            it = weighted_msg_ids_arrays.erase(it);
          }
          else
          {
            ++it;
          }

        }
      }

      const ::NewsGate::Search::Strategy& strategy = 
        strategy_transport->entity();
      
      size_t suppressed = 0;

      joined_result->take_top(request.start_from,
                              request.results_count,
                              strategy,
                              &suppressed);
/*
      std::cerr << "total_results_count " << total_results_count
                << ", duplicates " << duplicates << std::endl;
*/   

      total_results_count -= suppressed;
      suppressed_messages += suppressed;

      return joined_result.release();        
    }
    
    SearchResult*
    BankClientSessionImpl::create_result(uint64_t etag,
                                         uint64_t gm_flags,
                                         Search::Result* search_result,
                                         CategoryLocalePtr& category_locale,
                                         size_t total_results_count,
                                         size_t suppressed_messages,
                                         bool messages_loaded)
      throw(El::Exception, CORBA::SystemException)
    {
      SearchResult_var res = new SearchResult();
      
      res->category_locale =
        Transport::CategoryLocaleImpl::Init::create(category_locale.release());
      
      res->total_matched_messages = total_results_count;
      res->suppressed_messages = suppressed_messages;
      res->messages_loaded = messages_loaded;

      Search::Stat* search_stat = new Search::Stat();
      res->stat = Search::Transport::StatImpl::Init::create(search_stat);

      Search::MessageInfoArray& message_infos =
        *search_result->message_infos;
        
      unsigned long long search_etag = 0;
      uint32_t state_hash = 0;
      
      for(Search::MessageInfoArray::const_iterator it =
            message_infos.begin(); it != message_infos.end(); it++)
      {
        const Search::MessageInfo& mi(*it);
        
        El::CRC(search_etag,
                (const unsigned char*)&(mi.wid.id),
                sizeof(it->wid.id));

        if(mi.extras.get() && (state_hash = mi.extras->state_hash))
        {
          El::CRC(search_etag,
                  (const unsigned char*)&state_hash,
                  sizeof(state_hash));
        }
      }

      res->etag = search_etag;

      if(etag && search_etag == etag)
      {
        res->nochanges = 1;

        Transport::StoredMessagePackImpl::Var msg_pack =
          new Transport::StoredMessagePackImpl::Type(
            new Transport::StoredMessageArray());
        
        res->messages = msg_pack._retn();
        return res._retn();
      }

      *search_stat = search_result->stat;
      res->nochanges = 0;

      if((gm_flags & ~(Bank::GM_ID | Bank::GM_PUB_DATE)) == 0)
      {
        Transport::StoredMessageArrayPtr res_messages(
          new Transport::StoredMessageArray());

        if(gm_flags & (Bank::GM_ID | Bank::GM_PUB_DATE))
        {
          res_messages->reserve(message_infos.size());
          
          for(Search::MessageInfoArray::const_iterator
                it(message_infos.begin()), ie(message_infos.end());
              it != ie; ++it)
          {
            res_messages->push_back(Transport::StoredMessageDebug());
            Transport::StoredMessageDebug& msg = *res_messages->rbegin();

            const Search::MessageInfo& mi(*it);
            
            msg.message.id = mi.wid.id;

            if(mi.extras.get())
            {
              msg.message.published = mi.extras->published;
            }
          }
        }
        
        res->messages = new Transport::StoredMessagePackImpl::Type(
          res_messages.release());
        
        return res._retn();
      }
      
      return res._retn();
    }
    
    Transport::StoredMessagePack*
    BankClientSessionImpl::fetch_messages(
      MessageSearch* search,
      const MessageIdMap& msg_id_map,
      uint64_t gm_flags,
      int32_t img_index,
      int32_t thumb_index,
      Search::MessageInfoArray& message_infos)
      throw(El::Exception, CORBA::SystemException)
    {
      typedef __gnu_cxx::hash_map<unsigned long,
        Transport::IdPackImpl::Var,
        El::Hash::Numeric<unsigned long> >
        IdPackMap;

      IdPackMap id_packs;
      size_t message_infos_count = message_infos.size();
        
      const MessageSearch::SearchResultArray& search_results =
        search->results;

      size_t reserve =
        (search_results.size() ? 
         message_infos_count / search_results.size() * 2 : 0) + 1;
        
      MessageIdMap msg_id_order_map;
        
      for(unsigned long i = 0; i < message_infos_count; i++)
      {
        const Message::Id& id = message_infos[i].wid.id;

        msg_id_order_map[id] = i;
        MessageIdMap::const_iterator mit = msg_id_map.find(id);

        if(mit == msg_id_map.end())
        {
          std::ostringstream ostr;
          ostr << "BankClientSessionImpl::search: can't find id "
               << id.string() << " in msg_id_map";
            
          throw Exception(ostr.str());
        }

        size_t index = mit->second;
        IdPackMap::iterator pit = id_packs.find(index);

        if(pit == id_packs.end())
        {
          Transport::IdPackImpl::Var id_pack =
            new Transport::IdPackImpl::Type(new IdArray());

          id_pack->entities().reserve(reserve);
          pit = id_packs.insert(std::make_pair(index, id_pack)).first;
        }

        pit->second->entities().push_back(id);
      }

      MessageFetch_var fetch = new MessageFetch(callback_,
                                                id_packs.size(),
                                                gm_flags,
                                                img_index,
                                                thumb_index);

//        std::cerr << id_packs.size() << " id packs\n";
        
      for(IdPackMap::const_iterator it = id_packs.begin();
          it != id_packs.end(); it++)
      {
//          std::cerr << "Adding fetch request\n";
          
        fetch->add_request(search_results[it->first].bank,
                           it->second.in());
      }
        
      size_t requests_count = fetch->requests_count;

      {
        //Need lock to make CORBA-requests executed most close to each other
        WriteGuard guard(lock_);
        
        while(requests_count--)
        {
//          std::cerr << "Scheduling fetch\n";
          thread_pool_->execute(fetch.in());
        }
      }
      
//        std::cerr << "Waiting fetch\n";
        
      fetch->wait();

//        std::cerr << "Producing response\n";

      Transport::StoredMessageArrayPtr res_messages(
        new Transport::StoredMessageArray(message_infos_count));
        
      MessageFetch::ResultArray& fetch_results = fetch->results;

      for(MessageFetch::ResultArray::iterator
            it = fetch_results.begin(); it != fetch_results.end(); it++)
      {
        Transport::StoredMessagePackImpl::Type* msg_pack =
          dynamic_cast<Transport::StoredMessagePackImpl::Type*>(it->in());

        if(msg_pack == 0)
        {
          throw Exception(
            "BankClientSessionImpl::search: dynamic_cast<"
            "Transport::StoredMessagePackImpl::Type*> failed");
        }
          
        Transport::StoredMessageArray& pack_messages =
          msg_pack->entities();

        for(Transport::StoredMessageArray::iterator
              mit = pack_messages.begin(); mit != pack_messages.end(); mit++)
        {
          MessageIdMap::const_iterator id_it =
            msg_id_order_map.find(mit->get_id());

          if(id_it == msg_id_order_map.end())
          {
            std::ostringstream ostr;
            ostr << "BankClientSessionImpl::search: can't find id "
                 << mit->get_id().string() << " in msg_id_order_map";
            
            throw Exception(ostr.str());
          }

          Transport::StoredMessageDebug& msg =
            (*res_messages)[id_it->second];

          msg.steal(*mit);

          if(gm_flags & Bank::GM_DEBUG_INFO)
          { 
            msg.debug_info.match_weight =
              message_infos[id_it->second].wid.weight;  
          }
        }
      }

//        El::Stat::TimeMeasurement measurement(search_meter);        
//        measurement.stop();
        
      size_t messages_in_response = 0;
        
      for(Transport::StoredMessageArray::iterator
            it = res_messages->begin(); it != res_messages->end(); it++)
      {
        if(it->get_id() != Id::zero)
        {
          messages_in_response++;
        }
      }

      if(messages_in_response != message_infos_count)
      {
        Transport::StoredMessageArrayPtr response_messages(
          new Transport::StoredMessageArray(messages_in_response));

        size_t i = 0;

        for(Transport::StoredMessageArray::iterator
              it = res_messages->begin(); it != res_messages->end(); it++)
        {
          if(it->get_id() != Id::zero)
          {
            (*response_messages)[i++] = *it;
          }
        }

        res_messages.reset(response_messages.release());
      }

      Transport::StoredMessagePackImpl::Var msg_pack =
        new Transport::StoredMessagePackImpl::Type(res_messages.release());
        
      return msg_pack._retn();
    }
    
    void
    BankClientSessionImpl::search(const SearchRequest& request,
                                  SearchResult_out result)
      throw(ImplementationException, CORBA::SystemException)
    {
#     ifdef TRACE_SEARCH_TIME      
      ACE_High_Res_Timer timer;
      ACE_High_Res_Timer timer2;

      ACE_Time_Value refresh_tm;
      ACE_Time_Value search_tm;
      ACE_Time_Value fetch_tm;
      
      timer.start();
#     endif
      
      try
      {
#     ifdef TRACE_SEARCH_TIME      
        timer2.start();
#     endif
        
        refresh_session();

#     ifdef TRACE_SEARCH_TIME      
        timer2.stop();
        timer2.elapsed_time(refresh_tm);

        timer2.start();
#     endif
        
        ::NewsGate::Search::Transport::ExpressionImpl::Type*
          expression_transport = dynamic_cast<
          ::NewsGate::Search::Transport::ExpressionImpl::Type*>(
            request.expression.in());
        
        if(expression_transport == 0) 
        {
          throw Exception("BankClientSessionImpl::search: "
                          "dynamic_cast failed for request.expression");
        }
        
        // Need to serialize valuetype in advance as same structures will
        // be sent in concurrent requests, so can't delay serialization
        // till marshal phase
        expression_transport->serialize();
        
        MessageSearch_var search;
        MessageIdMap msg_id_map;
        size_t total_results_count = 0;
        size_t results_left = 0;
        size_t suppressed_messages = 0;
        size_t duplicate_factor = 1;
        bool messages_loaded = false;
        CategoryLocalePtr category_locale(new Categorizer::Category::Locale());

        Search::ResultPtr joined_result;

        while(true)
        {
          joined_result.reset(search_messages(request,
                                              duplicate_factor,
                                              *category_locale,
                                              search,
                                              msg_id_map,
                                              total_results_count,
                                              results_left,
                                              suppressed_messages,
                                              messages_loaded));
        
/*
          std::cerr << "request.results_count " << request.results_count
                    << ", joined_result->message_infos.size() "
                    << joined_result->message_infos.size()
                    << ", total_results_count " << total_results_count
                    << ", request.start_from " << request.start_from
                    << ", request.results_count " << request.results_count
                    << ", duplicate_factor " << duplicate_factor
                    << std::endl;
*/        
          if(request.results_count == joined_result->message_infos->size() ||
             results_left == 0)
          {
            break;
          }

          duplicate_factor *= 2;
 
          if(callback_)
          {
            std::ostringstream ostr;
            ostr << "BankClientSessionImpl::search: have to repeat search "
              "request (to recover from duplicates) with factor " <<
              duplicate_factor << std::endl;
            
            El::Service::Error error(ostr.str(),
                                     0,
                                     El::Service::Error::NOTICE);
            
            callback_->notify(&error);
          }
        }

        SearchResult_var res = create_result(request.etag,
                                             request.gm_flags,
                                             joined_result.get(),
                                             category_locale,
                                             total_results_count,
                                             suppressed_messages,
                                             messages_loaded);

        if(res->messages.in())
        {
          result = res._retn();
          return;
        }

#     ifdef TRACE_SEARCH_TIME
        timer2.stop();
        timer2.elapsed_time(search_tm);

        timer2.start();
#     endif
        
        res->messages = fetch_messages(search.in(),
                                       msg_id_map,
                                       request.gm_flags,
                                       request.img_index,
                                       request.thumb_index,
                                       *joined_result->message_infos);
        
        result = res._retn();

#     ifdef TRACE_SEARCH_TIME
        timer2.stop();
        timer2.elapsed_time(fetch_tm);

        timer2.start();
#     endif
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "BankClientSessionImpl::search: "
          "El::Exception caught. Description:\n" << e.what();
        
        ImplementationException ex;
        ex.description = CORBA::string_dup(ostr.str().c_str());
        
        throw ex;
      }
      
#     ifdef TRACE_SEARCH_TIME

      timer.stop();
      ACE_Time_Value total_tm;
      timer.elapsed_time(total_tm);

      std::cerr << "BankClientSessionImpl::search: "
                << "total " << El::Moment::time(total_tm)
                << "; refresh " << El::Moment::time(refresh_tm)
                << "; search " << El::Moment::time(search_tm)
                << "; fetch " << El::Moment::time(fetch_tm)
                << std::endl;      
#     endif
    }
    
    void
    BankClientSessionImpl::refresh_session()
      throw(Exception, El::Exception, CORBA::Exception)
    {
      {
        ReadGuard guard(lock_);
      
        ACE_Time_Value tm = ACE_OS::gettimeofday();

        if(refreshed_ + refresh_period_ > tm)
        {
          return;
        }
      }
      
      NewsGate::Message::BankManager_var bank_manager;
      
      {
        WriteGuard guard(lock_);
      
        ACE_Time_Value tm = ACE_OS::gettimeofday();

        if(refreshed_ + refresh_period_ > tm)
        {
          return;
        }

        bank_manager = bank_manager_;
        refreshed_ = tm;
      }      
        
      if(CORBA::is_nil(bank_manager.in()))
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::BankClientSessionImpl::refresh_session: "
             << "bank manager undefined";
            
        throw Exception(ostr.str());
      }

      BankClientSession_var session;

      try
      {
        session = bank_manager->bank_client_session();
      }
      catch(const ImplementationException& e)
      {
        if(callback_)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::BankClientSessionImpl::refresh_session: "
            "ImplementationException caught. Description:\n"
               << e.description.in();
        
          El::Service::Error
            error(ostr.str(), 0, El::Service::Error::CRITICAL);
          
          callback_->notify(&error);
        }
      }
      catch(const CORBA::Exception& e)
      {
        if(callback_)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::BankClientSessionImpl::refresh_session: "
            "CORBA::Exception caught. Description:\n" << e;
        
          El::Service::Error
            error(ostr.str(), 0, El::Service::Error::CRITICAL);
          
          callback_->notify(&error);
        }
      }

      if(session.in() == 0)
      {
        return;
      }

      BankClientSessionImpl* session_impl =
        dynamic_cast<BankClientSessionImpl*>(session.in());

      if(session_impl == 0)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::BankClientSessionImpl::refresh_session: "
             << "dynamic_cast<BankClientSessionImpl*> failed";
            
        throw Exception(ostr.str());
      }

      WriteGuard guard(lock_);

      bank_manager_ = session_impl->bank_manager_;
      process_id_ = session_impl->process_id_;

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
      message_post_retries_ = session_impl->message_post_retries_;
      colo_frontends_ = session_impl->colo_frontends_;

      if(thread_pool_.in() == 0)
      {
        threads_ = session_impl->threads_;
      }
    }  
    
    NewsGate::Message::Bank_ptr
    BankClientSessionImpl::bank_for_message_post(std::string& bank_ior,
                                                 const char* validation_id)
      throw(Exception, El::Exception, CORBA::Exception)
    {
      try
      {
        refresh_session();
        
        WriteGuard guard(lock_);

        if(banks_.size() == 0)
        {
          NotReady e;
          e.reason =
            CORBA::string_dup("BankClientSessionImpl::bank_for_message_post: "
                              "no banks available");
          throw e;
        }

        if(message_post_bank_ >= banks_.size())
        {
          message_post_bank_ = 0;
        }
        
        NewsGate::Message::Bank_var bank =
          get_next_bank(message_post_bank_,
                        banks_.size(),
                        bank_ior,
                        validation_id);

        if(!CORBA::is_nil(bank.in()))
        {
          return bank._retn();
        }

        bank = get_next_bank(0, message_post_bank_, bank_ior, validation_id);

        if(!CORBA::is_nil(bank.in()))
        {
          return bank._retn();
        }
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "BankClientSessionImpl::bank_for_message_post: El::Exception "
          "caught. Description:\n" << e;
          
        ImplementationException ie;
        ie.description = ostr.str().c_str();

        throw ie;
      }

      NotReady e;
      e.reason =
        CORBA::string_dup("BankClientSessionImpl::bank_for_message_post: "
                          "no valid banks available");
      throw e;
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
    BankClientSessionImpl::validate_bank(BankRecord& bank,
                                         const char* validation_id)
      throw(El::Exception)
    {
      if(bank.invalidated == ACE_Time_Value::zero)
      {
        return true;
      }

      if(bank.invalidated + invalidate_timeout_ > ACE_OS::gettimeofday())
      {
        return false;
      }

      NewsGate::Message::Transport::RawMessagePackImpl::Var probe =
        NewsGate::Message::Transport::RawMessagePackImpl::Init::create(
          new NewsGate::Message::Transport::RawMessagePackImpl::
          MessageArray());

      try
      {
        NewsGate::Message::Bank_var bn = bank.bank.object();
        
        if(validation_id)
        {
          bn->post_messages(probe.in(), PMR_NEW_MESSAGES, validation_id);
        }
        else
        {
          Transport::Request_var req =
            Transport::EmptyRequestImpl::Init::create(
              new Transport::EmptyStruct());
          
          ::NewsGate::Message::Transport::Response_var res;
          bn->send_request(req.in(), res.out());
        }
        
        bank.invalidated = ACE_Time_Value::zero;
      }
      catch(...)
      {
        bank.invalidated = ACE_OS::gettimeofday();
        return false;
      }

      return true;
    }
    
    NewsGate::Message::Bank_ptr
    BankClientSessionImpl::get_next_bank(size_t from,
                                         size_t till,
                                         std::string& bank_ior,
                                         const char* validation_id)
      throw(Exception, El::Exception, CORBA::Exception)
    {      
      for(size_t i = from; i < till; i++)
      {
        BankRecord& bank = banks_[i];

        if(!validate_bank(bank, validation_id))
        {
          continue;
        }
        
        message_post_bank_ = i + 1;
        
        bank_ior = bank.bank.reference();
        return bank.bank.object();
        
//        return NewsGate::Message::Bank::_duplicate(bank.bank.in());
      }

      return NewsGate::Message::Bank::_nil();
    }

    BankClientSession::RequestResult*
    BankClientSessionImpl::send_request(
      ::NewsGate::Message::Transport::Request* req,
      ::NewsGate::Message::Transport::Response_out resp)
      throw(ImplementationException,
            CORBA::SystemException)
    {
      RequestTask_var task = new RequestTask(callback_, req);
      bool has_banks = false;
      
      try
      {
        refresh_session();

        bool execute_in_current_thread = true;
        unsigned long banks_count = 0;
        
        {
          WriteGuard guard(lock_);

          execute_in_current_thread = thread_pool_.in() == 0;
          banks_count = 0;

          for(BankRecordArray::iterator it = banks_.begin();
              it != banks_.end(); ++it)
          {
            if(validate_bank(*it, 0))
            {
              task->add_bank(it->bank, thread_pool_.in());
              ++banks_count;
            }
          }
        }

        has_banks = banks_count > 0;

        if(execute_in_current_thread)
        {
          while(banks_count--)
          {
            task->execute();
          }
        }
        
        task->wait();
        resp = task->response._retn();
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::BankClientSessionImpl::send_request: "
          "El::Exception caught. Description:\n" << e.what();
        
        ImplementationException ex;
        ex.description = CORBA::string_dup(ostr.str().c_str());
        
        throw ex;
      }

      RequestResult_var result = new RequestResult();

      if(has_banks)
      {
        result->code = task->result;
        result->description = task->error_desc.c_str();
      }
      else
      {
        result->code = BankClientSession::RRC_NOT_READY;
        
        result->description = CORBA::string_dup(
          "NewsGate::Message::BankClientSessionImpl::send_request: "
          "no banks available");
      }

      return result._retn();
    }
    
    ::NewsGate::Message::Transport::StoredMessage*
    BankClientSessionImpl::get_message(
      ::NewsGate::Message::Transport::Id* message_id,
      ::CORBA::ULongLong gm_flags,
      ::CORBA::Long img_index,
      ::CORBA::Long thumb_index,
      ::CORBA::ULongLong message_signature)
      throw(NotFound,
            NotReady,
            ImplementationException,
            CORBA::SystemException)
    {
      try
      {
        NewsGate::Message::Transport::IdImpl::Type* msg_id =
          dynamic_cast<NewsGate::Message::Transport::IdImpl::Type*>(
            message_id);

        if(msg_id == 0)
        {
          ImplementationException ex;
          
          ex.description = CORBA::string_dup(
            "NewsGate::Message::BankClientSessionImpl::get_message: "
            "dynamic_cast<NewsGate::Message::Transport::IdImpl::Type*> "
            "failed");
          
          throw ex;
        }

        const Message::Id& id = msg_id->entity();

        NewsGate::Message::Bank_var bank;
        
        refresh_session();
        
        {          
          ReadGuard guard(lock_);

          if(banks_.size() == 0)
          {
            NotReady e;
            
            e.reason =
              CORBA::string_dup(
                "NewsGate::Message::BankClientSessionImpl::get_message: "
                "no banks available");
            
            throw e;
          }
      
          bank = banks_[id.src_id() % banks_.size()].bank.object();
        }

        NewsGate::Message::Transport::IdPackImpl::Var ids =
          new NewsGate::Message::Transport::IdPackImpl::Type(new IdArray());

        ids->entities().push_back(id);
        
        Transport::MessagePack_var result;
        Transport::IdPack_var notfound_message_ids;
        
        try
        {
          result = bank->get_messages(ids.in(),
                                      gm_flags,
                                      img_index,
                                      thumb_index,
                                      notfound_message_ids.out());
/*
        result =
          new Transport::StoredMessagePackImpl::Type(
            new Transport::StoredMessageArray());
*/        
        }
        catch(const NewsGate::Message::NotReady& e)
        {
          std::ostringstream ostr; 
          ostr << "NewsGate::Message::BankClientSessionImpl::get_message: "
            "NewsGate::Message::NotReady caught. Reason:\n" << e.reason;

          NotReady e;
          e.reason = CORBA::string_dup(ostr.str().c_str());

          throw e;
        }
        catch(const NewsGate::Message::ImplementationException& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::BankClientSessionImpl::get_message: "
            "NewsGate::Message::ImplementationException caught. "
            "Description:\n" << e.description;

          throw Exception(ostr.str());
        }
        catch(const CORBA::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::BankClientSessionImpl::get_message: "
            "CORBA::Exception caught. Description:\n" << e;

          throw Exception(ostr.str());
        }
 
        Transport::StoredMessagePackImpl::Type* msg_pack =
          dynamic_cast<Transport::StoredMessagePackImpl::Type*>(result.in());

        if(msg_pack == 0)
        {
          ImplementationException ex;
            
          ex.description = CORBA::string_dup(
            "NewsGate::Message::BankClientSessionImpl::get_message: "
            "dynamic_cast<Transport::MessagePackImpl::Type*> "
            "failed");
            
          throw ex;
        }

        const Transport::StoredMessageArray& entities = msg_pack->entities();

        if(entities.size() == 0 ||
           (message_signature && entities[0].message.signature !=
            message_signature))

        {
          return search_message(id,
                                gm_flags,
                                img_index,
                                thumb_index,
                                message_signature);
        }

        Transport::StoredMessageImpl::Var msg =
          new Transport::StoredMessageImpl::Type(
            new Transport::StoredMessageDebug(entities[0]));

        return msg._retn();       
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::BankClientSessionImpl::get_message: "
          "El::Exception caught. Description:\n" << e.what();
        
        ImplementationException ex;
        ex.description = CORBA::string_dup(ostr.str().c_str());
        
        throw ex;
      }

      return 0;
    }

    ::NewsGate::Message::Transport::StoredMessage*
    BankClientSessionImpl::search_message(const Message::Id& id,
                                          uint64_t gm_flags,
                                          int32_t img_index,
                                          int32_t thumb_index,
                                          Message::Signature signature)
      throw(NotFound,
            ImplementationException,
            CORBA::SystemException)
    {
      try
      {
        std::wstring wid;
        El::String::Manip::utf8_to_wchar(id.string().c_str(), wid);
        
        std::wostringstream query;
        query << L"MSG " << wid;

        if(signature)
        {
          query << L" SIGNATURE " << std::uppercase << std::hex << signature;  
        }
        
        std::wstring wquery = query.str();
//        El::String::Manip::utf8_to_wchar(query.c_str(), wquery);
    
        Search::ExpressionParser parser;
        std::wistringstream istr(wquery);
    
        parser.parse(istr);

        Search::Expression_var search_expression = parser.expression();

        search_expression->add_ref();
      
        Search::Transport::ExpressionImpl::Var
          expression_transport =
          Search::Transport::ExpressionImpl::Init::create(
            new Search::Transport::ExpressionHolder(
              search_expression));

        Search::Strategy::SuppressionPtr
          suppression(new Search::Strategy::SuppressNone());
      
        Search::Strategy::SortingPtr sorting(new Search::Strategy::SortNone());
        Search::Strategy::Filter filter;
      
        Search::Transport::StrategyImpl::Var strategy_transport = 
          Search::Transport::StrategyImpl::Init::create(
            new Search::Strategy(sorting.release(),
                                 suppression.release(),
                                 false,
                                 filter,
                                 Search::Strategy::RF_MESSAGES));

        // Need to serialize valuetypes in advance as same structures will
        // be sent in concurrent requests, so can't delay serialization
        // till marshal phase
      
        expression_transport->serialize();
        strategy_transport->serialize();
      
        Message::SearchRequest_var search_request =
          new Message::SearchRequest();
      
        search_request->gm_flags = gm_flags;
        search_request->img_index = img_index;
        search_request->thumb_index = thumb_index;
        search_request->expression = expression_transport._retn();
        search_request->strategy = strategy_transport._retn();
        search_request->start_from = 0;
        search_request->results_count = 1;
        search_request->etag = 0;
      
        Message::SearchResult_var search_result;
        search(search_request.in(), search_result.out());

        Message::Transport::StoredMessagePackImpl::Type* msg_transport =
          dynamic_cast<Message::Transport::StoredMessagePackImpl::Type*>(
            search_result->messages.in());
        
        if(msg_transport == 0)
        {
          throw Exception(
            "NewsGate::Message::BankClientSessionImpl::search_message: "
            "dynamic_cast<const Message::Transport::"
            "StoredMessagePackImpl::Type*> failed");
        }
      
        const Message::Transport::StoredMessageArray& messages =
          msg_transport->entities();

        if(messages.size() == 0)
        {
          throw NotFound();
        }
        
        Transport::StoredMessageImpl::Var msg =
          new Transport::StoredMessageImpl::Type(
            new Transport::StoredMessageDebug(messages[0]));
        
        return msg._retn();       
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::BankClientSessionImpl::search_message: "
          "El::Exception caught. Description:\n" << e.what();
        
        ImplementationException ex;
        ex.description = CORBA::string_dup(ostr.str().c_str());
        
        throw ex;
      }

      return 0;
    }
              
    //
    // BankClientSessionImpl::RequestTask class
    //

    void
    BankClientSessionImpl::RequestTask::execute() throw(El::Exception)
    {
      try
      {
        NewsGate::Message::Bank_var bank;
      
        {
          WriteGuard guard(lock_);
        
          BankList::iterator it = banks_.begin();

          if(it == banks_.end())
          {
            throw Exception("BankClientSessionImpl::RequestTask::execute: "
                            "unexpected end of bank ref set");
          }

          bank = it->object();
          banks_.erase(it);
        }

        ::NewsGate::Message::Transport::Response_var res;
        bank->send_request(request_.in(), res.out());

        WriteGuard guard(lock_);
        
        if(response.in() == 0)
        {
          response = res._retn();
        }
        else
        {
          response->absorb(res.in());
        }
      }
      catch(const NewsGate::Message::NotReady& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::BankClientSessionImpl::RequestTask::"
          "execute: NewsGate::Message::NotReady caught. Reason:\n" << e.reason;

        error_desc = ostr.str();

        WriteGuard guard(lock_);

        if(result < BankClientSession::RRC_NOT_READY)
        {
          result = BankClientSession::RRC_NOT_READY;
        }
      }
      catch(const NewsGate::Message::InvalidData& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::BankClientSessionImpl::RequestTask::"
          "execute: NewsGate::Message::NotReady caught. Reason:\n"
             << e.description;

        error_desc = ostr.str();
        
        WriteGuard guard(lock_);

        if(result < BankClientSession::RRC_INVALID_DATA)
        {
          result = BankClientSession::RRC_INVALID_DATA;
        }
      }
      catch(const NewsGate::Message::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::BankClientSessionImpl::RequestTask::"
          "execute: NewsGate::Message::ImplementationException caught. "
          "Description:\n" << e.description;

        error_desc = ostr.str();
        
        WriteGuard guard(lock_);

        if(result < BankClientSession::RRC_MESSAGE_BANK_ERROR)
        {
          result = BankClientSession::RRC_MESSAGE_BANK_ERROR;
        }
      }
      catch(const NewsGate::Message::Transport::Response::
            ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::BankClientSessionImpl::RequestTask::"
          "execute: NewsGate::Message::Transport::Response::"
          "ImplementationException caught. "
          "Description:\n" << e.description;
        
        error_desc = ostr.str();

        WriteGuard guard(lock_);

        if(result < BankClientSession::RRC_RESPONSE_ERROR)
        {
          result = BankClientSession::RRC_RESPONSE_ERROR;
        }
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::BankClientSessionImpl::RequestTask::"
          "execute: CORBA::Exception caught. Description:\n" << e;

        error_desc = ostr.str();        
        
        WriteGuard guard(lock_);

        if(result < BankClientSession::RRC_CORBA_ERROR)
        {
          result = BankClientSession::RRC_CORBA_ERROR;
        }
      }

      if(callback_ && !error_desc.empty())
      {
        El::Service::Error error(error_desc, 0, El::Service::Error::ALERT);
        callback_->notify(&error);
      }
      
      {
        WriteGuard guard(lock_);

        if(++completed_requests_ == banks_count_)
        {
          request_completed_.signal();
        }
      }      
    }    
    
    //
    // BankClientSessionImpl::MessageSearch class
    //
    void
    BankClientSessionImpl::MessageSearch::execute() throw(El::Exception)
    {
//      std::cerr << "Execute search\n";
      
      BankRef bank;
      
      {
        WriteGuard guard(lock_);
        
        BankArray::reverse_iterator it = banks.rbegin();

        if(it == banks.rend())
        {
          throw Exception("BankClientSessionImpl::MessageSearch::execute: "
                          "unexpected end of bank ref set");
        }

        bank = *it;
        banks.pop_back();
      }

      bool success = false;

      SearchResult search_res;
      std::string error_desc;
      
      try
      {
        MatchedMessages_var res;
        
        NewsGate::Message::Bank_var bn = bank.object();
        bn->search(request_, res.out());
        
        search_res.bank = bank;
        search_res.match = res;
          
        success = true;
      }
      catch(const NewsGate::Message::NotReady& e)
      {
        com_failure = true;
        
        std::ostringstream ostr;
        ostr << "NewsGate::Message::BankClientSessionImpl::MessageSearch::"
          "execute: NewsGate::Message::NotReady caught. Reason:\n" << e.reason;

        error_desc = ostr.str();        
      }
      catch(const NewsGate::Message::ImplementationException& e)
      {
        com_failure = true;
        
        std::ostringstream ostr;
        ostr << "NewsGate::Message::BankClientSessionImpl::MessageSearch::"
          "execute: NewsGate::Message::ImplementationException caught. "
          "Description:\n" << e.description;

        error_desc = ostr.str();        
      }
      catch(const CORBA::Exception& e)
      {
        com_failure = true;
        
        std::ostringstream ostr;
        ostr << "NewsGate::Message::BankClientSessionImpl::MessageSearch::"
          "execute: CORBA::Exception caught. Description:\n" << e;

        error_desc = ostr.str();        
      }

      if(callback_ && !error_desc.empty())
      {

        El::Service::Error error(error_desc, 0, El::Service::Error::ALERT);
        callback_->notify(&error);
      }  
      
      {
        WriteGuard guard(lock_);

        if(success)
        {
          results.push_back(search_res);
        }

        if(++completed_requests_ == banks_count)
        {
          search_completed_.signal();
        }
      }      
    }
    
    //
    // BankClientSessionImpl::MessageFetch class
    //
    void
    BankClientSessionImpl::MessageFetch::execute() throw(El::Exception)
    {
//      std::cerr << "Execute fetch\n";

      FetchRequest request;
      
      {
        WriteGuard guard(lock_);
        
        FetchRequestArray::reverse_iterator it = requests.rbegin();

        if(it == requests.rend())
        {
          throw Exception("BankClientSessionImpl::MessageFetch::execute: "
                          "unexpected end of requests set");
        }

        request = *it;
        requests.pop_back();
      }

      bool success = false;

      std::string error_desc;
      Transport::MessagePack_var result;
      Transport::IdPack_var notfound_message_ids;
      
      try
      {
        NewsGate::Message::Bank_var bank = request.bank.object();
        
        result = bank->get_messages(request.message_ids.in(),
                                    gm_flags_,
                                    img_index_,
                                    thumb_index_,
                                    notfound_message_ids.out());
          
        success = true;
      }
      catch(const NewsGate::Message::NotReady& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::BankClientSessionImpl::MessageFetch::"
          "execute: NewsGate::Message::NotReady caught. Reason:\n" << e.reason;

        error_desc = ostr.str();        
      }
      catch(const NewsGate::Message::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::BankClientSessionImpl::MessageFetch::"
          "execute: NewsGate::Message::ImplementationException caught. "
          "Description:\n" << e.description;

        error_desc = ostr.str();        
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::BankClientSessionImpl::MessageFetch::"
          "execute: CORBA::Exception caught. Description:\n" << e;

        error_desc = ostr.str();        
      }

      if(callback_ && !error_desc.empty())
      {

        El::Service::Error error(error_desc, 0, El::Service::Error::ALERT);
        callback_->notify(&error);
      }  
      
      {
        WriteGuard guard(lock_);

        if(success)
        {
          results.push_back(result);
        }

        if(++completed_requests_ == requests_count)
        {
          fetch_completed_.signal();
        }
      }      
    }
  }
}
