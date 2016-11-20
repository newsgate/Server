/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Event/Bank/ManagingEvents.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_EVENT_BANK_MANAGINGEVENTS_HPP_
#define _NEWSGATE_SERVER_SERVICES_EVENT_BANK_MANAGINGEVENTS_HPP_

#include <string>
#include <ext/hash_map>
#include <deque>
#include <memory>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/Hash/Hash.hpp>
#include <El/Stat.hpp>

#include <Services/Commons/Event/EventServices.hpp>
#include <Services/Dictionary/Commons/DictionaryServices.hpp>

#include "SubService.hpp"
#include "EventManager.hpp"

namespace NewsGate
{
  namespace Event
  {
    //
    // ManagingEvents class
    //
    class ManagingEvents : public virtual BankState,
                           public virtual EventManagerCallback
    {
    public:        
      EL_EXCEPTION(Exception, BankState::Exception);

      ManagingEvents(Event::BankSession* session,
                     const ACE_Time_Value& report_presence_period,
                     BankStateCallback* callback)
        throw(Exception, El::Exception);

      virtual ~ManagingEvents() throw();
      
      virtual ::CORBA::ULong get_message_events(
        ::NewsGate::Message::Transport::IdPack* messages,
        ::NewsGate::Message::Transport::MessageEventPack_out events)
        throw(NewsGate::Event::NotReady,
              NewsGate::Event::ImplementationException,
              ::CORBA::SystemException);
      
      virtual void post_message_digest(
        ::NewsGate::Event::Transport::MessageDigestPack* digests,
        ::NewsGate::Message::Transport::MessageEventPack_out events)
        throw(NewsGate::Event::NotReady,
              NewsGate::Event::ImplementationException,
              ::CORBA::SystemException);      

      virtual void get_events(
        ::NewsGate::Event::Transport::EventIdRelPack* ids,
        ::NewsGate::Event::Transport::EventObjectRelPack_out events)
        throw(NewsGate::Event::NotReady,
              NewsGate::Event::ImplementationException,
              ::CORBA::SystemException);
      
      virtual void push_events(
        ::NewsGate::Event::Transport::MessageDigestPack* digests,
        ::NewsGate::Event::Transport::EventPushInfoPack* events)
        throw(NewsGate::Event::NotReady,
              NewsGate::Event::ImplementationException,
              ::CORBA::SystemException);

      virtual void delete_messages(::NewsGate::Message::Transport::IdPack* ids)
        throw(NewsGate::Event::NotReady,
              NewsGate::Event::ImplementationException,
              ::CORBA::SystemException);      
      
    private:

      void create_managers() throw(Exception, El::Exception);
      void destroy_managers() throw(Exception, El::Exception);
      
      bool ready() const throw();
      size_t event_count() const throw(El::Exception);

      void add_lang_managers(const EventManager::LangSet& langs)
        throw(Exception, El::Exception);

      void report_presence() throw();
//      void get_dict_hash() throw(El::Exception);
      void monitor_state() throw(El::Exception);
      
      void recreate_managers(size_t event_count_threshold)
        throw(El::Exception);
      
      typedef std::vector<EventManager_var> EventManagerArray;
      
      EventManagerArray event_managers() const throw(Exception, El::Exception);
      
      virtual bool stop() throw(Exception, El::Exception);
      virtual void wait() throw(Exception, El::Exception);
      virtual bool notify(El::Service::Event* event) throw(El::Exception);
      
    private:

      struct ReportPresence : public El::Service::CompoundServiceMessage
      {
        ReportPresence(ManagingEvents* state) throw(El::Exception);
      };

/*      
      struct GetDictHash : public El::Service::CompoundServiceMessage
      {
        GetDictHash(ManagingEvents* state) throw(El::Exception);
      };
*/
      struct Monitoring : public El::Service::CompoundServiceMessage
      {
        Monitoring(ManagingEvents* state) throw(El::Exception);
      };

      struct RecreateManagers : public El::Service::CompoundServiceMessage
      {
        size_t event_count_threshold;
        
        RecreateManagers(size_t event_count_threshold_val,
                         ManagingEvents* state) throw(El::Exception);
      };

      EventManagerArray event_managers_;
      EventManager::LangSet event_manager_langs_;
      
      Event::BankSession_var session_;
//      NewsGate::Dictionary::WordManager_var word_manager_;
      
      ACE_Time_Value report_presence_period_;
//      uint32_t dict_hash_;
      
      bool ready_;
      const Server::Config::EventBankType& config_;
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
    // ManagingEvents class
    //

    inline
    ManagingEvents::EventManagerArray
    ManagingEvents::event_managers() const throw(Exception, El::Exception)
    {
      ReadGuard guard(srv_lock_);
      return event_managers_;
    }
 
    //
    // ManagingEvents::ReportPresence class
    //
    inline
    ManagingEvents::ReportPresence::ReportPresence(
      ManagingEvents* state) throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }
/*    
    //
    // ManagingEvents::GetDictHash class
    //
    inline
    ManagingEvents::GetDictHash::GetDictHash(
      ManagingEvents* state) throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }
*/

    //
    // ManagingEvents::Monitoring class
    //
    inline
    ManagingEvents::Monitoring::Monitoring(
      ManagingEvents* state) throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }

    //
    // ManagingEvents::RecreateManagers class
    //
    inline
    ManagingEvents::RecreateManagers::RecreateManagers(
      size_t event_count_threshold_val,
      ManagingEvents* state)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state),
          event_count_threshold(event_count_threshold_val)
    {
    }
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_EVENT_BANK_MANAGINGEVENTS_HPP_
