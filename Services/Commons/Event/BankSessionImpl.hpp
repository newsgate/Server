/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Commons/Event/BankSessionImpl.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_SERVICES_COMMONS_EVENT_BANKSESSIONIMPL_HPP_
#define _NEWSGATE_SERVER_SERVICES_COMMONS_EVENT_BANKSESSIONIMPL_HPP_

#include <stdint.h>

#include <string>

#include <El/Exception.hpp>

#include <Services/Commons/Event/EventServices.hpp>

namespace NewsGate
{
  namespace Event
  {
    class BankSessionIdImpl : public virtual BankSessionId,
                              public virtual El::Corba::ValueRefCountBase
      // public virtual CORBA::DefaultValueRefCountBase
    {
    public:
      BankSessionIdImpl(const char* guid_val = 0,
                        uint32_t index_val = 0,
                        uint32_t bank_count_val = 0) throw(El::Exception);
      
      virtual ~BankSessionIdImpl() throw();      

      //
      // IDL:NewsGate/Event/BankSession/to_string:1.0
      //
      virtual char* to_string();

      //
      // IDL:NewsGate/Event/BankSession/from_string:1.0
      //
      virtual void from_string(const char* str);

      //
      // IDL:omg.org/CORBA/CustomMarshal/marshal:1.0
      //
      virtual void marshal(CORBA::DataOutputStream* os);

      //
      // IDL:omg.org/CORBA/CustomMarshal/unmarshal:1.0
      //
      virtual void unmarshal(CORBA::DataInputStream* is);

      bool is_equal(const BankSessionIdImpl* session) const throw();

    private:
      virtual CORBA::ValueBase* _copy_value() throw(CORBA::NO_IMPLEMENT);

    public:
      std::string guid;
      uint32_t index;
      uint32_t banks_count;
    };

    typedef El::Corba::ValueVar<BankSessionIdImpl> BankSessionIdImpl_var;

    class BankSessionImpl : public virtual BankSession,
                            public virtual El::Corba::ValueRefCountBase
    {
    public:
      BankSessionImpl(BankSessionIdImpl* id,
                      Bank_ptr left_neighbour_ptr,
                      Bank_ptr right_neighbour_ptr,
                      uint32_t bank_count)
        throw(El::Exception);
      
      virtual ~BankSessionImpl() throw();

      static void register_valuetype_factories(CORBA::ORB* orb)
        throw(CORBA::Exception, El::Exception);

      //
      // IDL:NewsGate/Event/BankSession/id:1.0
      //
      virtual void id(BankSessionId*) throw(CORBA::NO_IMPLEMENT);
      virtual BankSessionId* id() const;

      //
      //
      // IDL:NewsGate/Event/BankSession/left_neighbour:1.0
      //
      virtual void left_neighbour(Bank_ptr);
      virtual Bank_ptr left_neighbour() const;
      
      //
      //
      // IDL:NewsGate/Event/BankSession/right_neighbour:1.0
      //
      virtual void right_neighbour(Bank_ptr);
      virtual Bank_ptr right_neighbour() const;
      
      //
      // IDL:NewsGate/Event/BankSession/bank_count:1.0
      //
      virtual void bank_count(::CORBA::ULong);
      virtual ::CORBA::ULong bank_count() const;

      //
      // IDL:omg.org/CORBA/CustomMarshal/marshal:1.0
      //
      virtual void marshal(CORBA::DataOutputStream* os);

      //
      // IDL:omg.org/CORBA/CustomMarshal/unmarshal:1.0
      //
      virtual void unmarshal(CORBA::DataInputStream* is);

    private:
      virtual CORBA::ValueBase* _copy_value() throw(CORBA::NO_IMPLEMENT);

    private:
      BankSessionIdImpl_var id_;
      Bank_var left_neighbour_;
      Bank_var right_neighbour_;
      uint32_t bank_count_;
    };

    typedef El::Corba::ValueVar<BankSessionImpl> BankSessionImpl_var;

    class BankSessionIdImpl_init : public virtual CORBA::ValueFactoryBase
    {
    public:
      static BankSessionIdImpl* create() throw(El::Exception);
      
      virtual CORBA::ValueBase* create_for_unmarshal();
    };
    
    class BankSessionImpl_init : public virtual CORBA::ValueFactoryBase
    {
    public:
      virtual CORBA::ValueBase* create_for_unmarshal();
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
    // BankSessionIdImpl class
    //
    inline
    BankSessionIdImpl::BankSessionIdImpl(const char* guid_val,
                                         uint32_t index_val,
                                         uint32_t bank_count_val)
      throw(El::Exception)
        : guid(guid_val ? guid_val : ""),
          index(index_val),
          banks_count(bank_count_val)
    {
    }    

    inline
    BankSessionIdImpl::~BankSessionIdImpl() throw()
    {
    }

    inline
    CORBA::ValueBase*
    BankSessionIdImpl::_copy_value() throw(CORBA::NO_IMPLEMENT)
    {
      return new BankSessionIdImpl(guid.c_str(), index, banks_count);
    }

    inline
    bool
    BankSessionIdImpl::is_equal(const BankSessionIdImpl* session) const throw()
    {
      return guid == session->guid && banks_count == session->banks_count &&
        index == session->index;
    }

    //
    // BankSessionImpl class
    //
    inline
    BankSessionImpl::BankSessionImpl(BankSessionIdImpl* id,
                                     Bank_ptr left_neighbour_ptr,
                                     Bank_ptr right_neighbour_ptr,
                                     uint32_t bank_count)
      throw(El::Exception)
        : id_(id),
          left_neighbour_(Bank::_duplicate(left_neighbour_ptr)),
          right_neighbour_(Bank::_duplicate(right_neighbour_ptr)),
          bank_count_(bank_count)
    {
    }
      
    inline
    BankSessionImpl::~BankSessionImpl() throw()
    {
    }

    inline
    void BankSessionImpl::id(BankSessionId*) throw(CORBA::NO_IMPLEMENT)
    {
      throw CORBA::NO_IMPLEMENT();
    }
    
    inline
    BankSessionId*
    BankSessionImpl::id() const
    {
      return id_.in();
    }
    
    inline
    CORBA::ValueBase*
    BankSessionImpl::_copy_value() throw(CORBA::NO_IMPLEMENT)
    {
      BankSessionIdImpl_var id =
        new BankSessionIdImpl(id_->guid.c_str(), id_->index, id_->banks_count);
      
      return new BankSessionImpl(id._retn(),
                                 left_neighbour_.in(),
                                 right_neighbour_.in(),
                                 bank_count_);
    }

    inline
    void
    BankSessionImpl::left_neighbour(Bank_ptr neighbour_ptr)
    {
      left_neighbour_ = Bank::_duplicate(neighbour_ptr);
    }

    inline
    Bank_ptr
    BankSessionImpl::left_neighbour() const
    {
      return left_neighbour_.in();
    }
    
    inline
    void
    BankSessionImpl::right_neighbour(Bank_ptr neighbour_ptr)
    {
      right_neighbour_ = Bank::_duplicate(neighbour_ptr);
    }

    inline
    Bank_ptr
    BankSessionImpl::right_neighbour() const
    {
      return right_neighbour_.in();
    }
    
    inline
    void
    BankSessionImpl::bank_count(::CORBA::ULong val)
    {
      bank_count_ = val;
    }
    
    inline
    ::CORBA::ULong
    BankSessionImpl::bank_count() const
    {
      return bank_count_;
    }
    
    //
    // BankSessionId_init class
    //
    inline
    BankSessionIdImpl*
    BankSessionIdImpl_init::create() throw(El::Exception)
    {
      return new BankSessionIdImpl();
    }

    inline
    CORBA::ValueBase* 
    BankSessionIdImpl_init::create_for_unmarshal()
    {
      return create();
    }

    //
    // BankSessionImpl_init class
    //
    inline
    CORBA::ValueBase* 
    BankSessionImpl_init::create_for_unmarshal()
    {
      return new BankSessionImpl(0, 0, 0, 0);
    }
    
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_COMMONS_EVENT_BANKSESSIONIMPL_HPP_
