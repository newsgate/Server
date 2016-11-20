/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Event/BankManager/SessionSupport.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_EVENT_BANKMANAGER_SESSIONSUPPORT_HPP_
#define _NEWSGATE_SERVER_SERVICES_EVENT_BANKMANAGER_SESSIONSUPPORT_HPP_

#include <string>
#include <list>

#include <ext/hash_set>
#include <ext/hash_map>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/Hash/Hash.hpp>

#include <Services/Commons/Event/BankSessionImpl.hpp>
#include <Services/Commons/Event/EventServices.hpp>

#include "SubService.hpp"

namespace NewsGate
{
  namespace Event
  {
    //
    // RegisteringBanks class
    //
    class RegisteringBanks : public BankManagerState
    {
    public:
        
      EL_EXCEPTION(Exception, BankManagerState::Exception);

      RegisteringBanks(const ACE_Time_Value& presence_poll_timeout,
                       const ACE_Time_Value& registration_timeout,
                       BankManagerStateCallback* callback)
        throw(Exception, El::Exception);

      virtual ~RegisteringBanks() throw();

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

      void start_gathering_info() throw();
      void finalize_registration() throw();
      void create_disposition() throw(Exception, El::Exception);

      ::NewsGate::Event::BankSession* login(const char* bank_ior)
        throw(NewsGate::Event::NotReady,
              NewsGate::Event::ImplementationException,
              CORBA::SystemException,
              El::Exception);      

    private:

      void save_bank_info(const char* bank_ior)
        throw(NewsGate::Event::NotReady,
              NewsGate::Event::ImplementationException,
              CORBA::SystemException,
              El::Exception);

      struct FinalizeRegistration :
        public El::Service::CompoundServiceMessage
      {
        FinalizeRegistration(RegisteringBanks* state) throw(El::Exception);
      };

      typedef __gnu_cxx::hash_set<std::string, El::Hash::String> BankSet;

/*      
      struct BankRecord
      {
        ACE_Time_Value timestamp;
        size_t banks_count;
        std::string bank_ior;
      };

      class BankRecordMap :
        public __gnu_cxx::hash_map<unsigned long,
                                   BankRecord,
                                   El::Hash::Numeric<unsigned long> >
      {
      public:
        BankRecordMap() throw(El::Exception) {}
      };
*/

    private:
      ACE_Time_Value presence_poll_timeout_;
      ACE_Time_Value registration_timeout_;
      ACE_Time_Value end_registration_time_;
      bool gathering_info_;
      std::string session_guid_;
      BankSet banks_;
      BankDisposition disposition_;
    };

    //
    // DispositionDisbandment class
    //
    class DispositionDisbandment : public BankManagerState
    {
    public:
        
      EL_EXCEPTION(Exception, BankManagerState::Exception);

      DispositionDisbandment(const ACE_Time_Value& reset_timeout,
                             BankManagerStateCallback* callback)
        throw(Exception, El::Exception);

      virtual ~DispositionDisbandment() throw();

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

      void finalize_disbandment() throw();

      struct FinalizeDisbandment : public El::Service::CompoundServiceMessage
      {
        FinalizeDisbandment(DispositionDisbandment* state)
          throw(El::Exception);
      };
      
    private:
      ACE_Time_Value reset_timeout_;
      ACE_Time_Value end_disbandment_time_;
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
    // RegisteringBanks class
    //
    
    inline
    RegisteringBanks::~RegisteringBanks() throw()
    {
    }

    //
    // RegisteringBanks::FinalizePullersRegistration class
    //
    
    inline
    RegisteringBanks::FinalizeRegistration::
    FinalizeRegistration(RegisteringBanks* state) throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }
    
    //
    // DispositionDisbandment::FinalizeDisbandment class
    //

    inline
    DispositionDisbandment::FinalizeDisbandment::FinalizeDisbandment(
      DispositionDisbandment* state)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }    
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_EVENT_BANKMANAGER_SESSIONSUPPORT_HPP_
