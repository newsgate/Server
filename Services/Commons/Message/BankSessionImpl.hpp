/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Message/Commons/BankSessionImpl.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MESSAGE_COMMONS_BANKSESSIONIMPL_HPP_
#define _NEWSGATE_SERVER_SERVICES_MESSAGE_COMMONS_BANKSESSIONIMPL_HPP_

#include <stdint.h>

#include <El/Exception.hpp>

#include <Services/Commons/Message/MessageServices.hpp>

namespace NewsGate
{
  namespace Message
  {
    class BankSessionIdImpl : public virtual BankSessionId,
                              public virtual El::Corba::ValueRefCountBase
    {
    public:
      BankSessionIdImpl(uint32_t index_val = 0, uint32_t bank_count_val = 0)
        throw(El::Exception);
      
      virtual ~BankSessionIdImpl() throw();      

      //
      // IDL:NewsGate/Message/BankSession/to_string:1.0
      //
      virtual char* to_string();

      //
      // IDL:NewsGate/Message/BankSession/from_string:1.0
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
      uint32_t index;
      uint32_t banks_count;
    };

    typedef El::Corba::ValueVar<BankSessionIdImpl> BankSessionIdImpl_var;

    class BankSessionImpl : public virtual BankSession,
                            public virtual El::Corba::ValueRefCountBase
    {
    public:
      BankSessionImpl(BankSessionIdImpl* id,
                      const char* sharing_id,
                      bool mirror) throw(El::Exception);
      
      virtual ~BankSessionImpl() throw();

      static void register_valuetype_factories(CORBA::ORB* orb)
        throw(CORBA::Exception, El::Exception);

      //
      // IDL:NewsGate/Message/BankSession/id:1.0
      //
      virtual void id(BankSessionId*) throw(CORBA::NO_IMPLEMENT);
      virtual BankSessionId* id() const;

      virtual void sharing_id(char *val);
      virtual void sharing_id(const char *val);
      virtual void sharing_id(const ::CORBA::String_var &val);
      virtual const char *sharing_id() const;

      virtual void mirror(const ::CORBA::Boolean);
      virtual ::CORBA::Boolean mirror(void) const;      

      //
      // IDL:NewsGate/Message/BankSession/location:1.0
      //
      virtual void location(CORBA::ULong_out index,
                            CORBA::ULong_out banks_count);

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
      CORBA::String_var sharing_id_;
      bool mirror_;
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
  namespace Message
  {
    //
    // BankSessionIdImpl class
    //
    inline
    BankSessionIdImpl::BankSessionIdImpl(uint32_t index_val,
                                         uint32_t bank_count_val)
      throw(El::Exception)
        : index(index_val),
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
      return new BankSessionIdImpl(index, banks_count);
    }  

    inline
    bool
    BankSessionIdImpl::is_equal(const BankSessionIdImpl* session) const throw()
    {
      return banks_count == session->banks_count &&
        index == session->index;
    }

    //
    // BankSessionImpl class
    //
    inline
    BankSessionImpl::BankSessionImpl(BankSessionIdImpl* id,
                                     const char* sharing_id,
                                     bool mirror)
      throw(El::Exception)
        : id_(id),
          sharing_id_(sharing_id),
          mirror_(mirror)
    {
    }
      
    inline
    BankSessionImpl::~BankSessionImpl() throw()
    {
    }

    inline
    void
    BankSessionImpl::sharing_id(char *val)
    {
      sharing_id_ = val;
    }

    inline
    void
    BankSessionImpl::sharing_id(const char *val)
    {
      sharing_id_ = val;
    }

    inline
    void
    BankSessionImpl::sharing_id(const ::CORBA::String_var &val)
    {
      sharing_id_ = val;
    }

    inline
    const char *
    BankSessionImpl::sharing_id() const
    {
      return sharing_id_.in();
    }

    inline
    void
    BankSessionImpl::mirror(const ::CORBA::Boolean val)
    {
      mirror_ = val;
    }
    
    inline
    ::CORBA::Boolean
    BankSessionImpl::mirror(void) const
    {
      return mirror_;
    }
    
    inline
    void
    BankSessionImpl::id(BankSessionId*) throw(CORBA::NO_IMPLEMENT)
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
        new BankSessionIdImpl(id_->index, id_->banks_count);

      return new BankSessionImpl(id._retn(), sharing_id_.in(), mirror_);
    }

    inline
    void
    BankSessionImpl::location(CORBA::ULong_out index,
                              CORBA::ULong_out banks_count)
    {
      index = id_->index;
      banks_count = id_->banks_count;
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
      return new BankSessionImpl(0, "", false);
    }
    
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_MESSAGE_COMMONS_BANKSESSIONIMPL_HPP_
