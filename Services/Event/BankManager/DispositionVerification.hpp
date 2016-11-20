/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Event/BankManager/DispositionVerification.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_EVENT_BANKMANAGER_DISPOSITIONVERIFICATION_HPP_
#define _NEWSGATE_SERVER_SERVICES_EVENT_BANKMANAGER_DISPOSITIONVERIFICATION_HPP_

#include <string>
#include <list>
#include <set>

#include <ext/hash_map>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/Service/Service.hpp>
#include <El/Hash/Hash.hpp>

namespace NewsGate
{
  namespace Event
  {
    //
    // DispositionVerification class
    //
    class DispositionVerification : public BankManagerState
    {
    public:
        
      EL_EXCEPTION(Exception, BankManagerState::Exception);

      DispositionVerification(
        const BankDisposition& disposition,
        const char* session_guid,
        const ACE_Time_Value& presence_poll_timeout,
        const ACE_Time_Value& presence_check_period,
        BankManagerStateCallback* callback)
        throw(Exception, El::Exception);

      virtual ~DispositionVerification() throw();

      virtual ::NewsGate::Event::BankSession* bank_login(const char* bank_ior)
        throw(NewsGate::Event::NotReady,
              NewsGate::Event::ImplementationException,
              ::CORBA::SystemException);

      virtual void ping(const char* bank_ior,
                        ::NewsGate::Event::BankSessionId* current_session_id)
        throw(NewsGate::Event::Logout,
              NewsGate::Event::ImplementationException,
              ::CORBA::SystemException);

      virtual ::NewsGate::Event::BankClientSession* bank_client_session()
        throw(NewsGate::Event::ImplementationException,
              ::CORBA::SystemException);
      
      virtual bool notify(El::Service::Event* event) throw(El::Exception);

    private:
      
      void check_banks_presence() throw();
      
      struct CheckBanksPresence : public El::Service::CompoundServiceMessage
      {
        CheckBanksPresence(DispositionVerification* state)
          throw(El::Exception);
      };
      
    private:
      BankDisposition disposition_;
      std::string session_guid_;
      
      ACE_Time_Value presence_poll_timeout_;
      ACE_Time_Value presence_check_period_;
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
    // DispositionVerification::CheckBanksPresence class
    //
    inline
    DispositionVerification::CheckBanksPresence::CheckBanksPresence(
      DispositionVerification* state) throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }
    
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_EVENT_BANKMANAGER_DISPOSITIONVERIFICATION_HPP_
