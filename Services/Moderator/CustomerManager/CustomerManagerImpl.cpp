/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/CustomerManager/CustomerManagerImpl.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <string>
#include <sstream>

#include <md5.h>
#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/Guid.hpp>
#include <El/String/Manip.hpp>

#include "CustomerManagerImpl.hpp"
#include "CustomerManagerMain.hpp"
#include "DB_Record.hpp"

namespace Aspect
{
  const char STATE[] = "State";
}

namespace NewsGate
{
  namespace Moderation
  {
    //
    // CustomerManagerImpl class
    //
    CustomerManagerImpl::CustomerManagerImpl(El::Service::Callback* callback)
      throw(InvalidArgument, Exception, El::Exception)
        : El::Service::CompoundService<>(callback, "CustomerManagerImpl")
    {
    }

    CustomerManagerImpl::~CustomerManagerImpl() throw()
    {
      // Check if state is active, then deactivate and log error
    }

    bool
    CustomerManagerImpl::notify(El::Service::Event* event)
      throw(El::Exception)
    {
      if(El::Service::CompoundService<>::notify(event))
      {
        return true;
      }

      return true;
    }

    ::NewsGate::Moderation::CustomerInfo*
    CustomerManagerImpl::get_customer(::NewsGate::Moderation::CustomerId id)
      throw(NewsGate::Moderation::PermissionDenied,
            NewsGate::Moderation::AccountNotExist,
            NewsGate::Moderation::NotReady,
            NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      try
      {
        ::NewsGate::Moderation::CustomerInfo_var result_info =
          new ::NewsGate::Moderation::CustomerInfo();

        result_info->id = id;

        std::ostringstream ostr;
        ostr << "select id, status, balance from Customer where id=" << id;
        
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();        
        
        El::MySQL::Result_var qresult = connection->query(ostr.str().c_str());
        
        Customer::Customer rec(qresult.in());
        
        if(!rec.fetch_row())
        {
          throw NewsGate::Moderation::AccountNotExist();
        }

        result_info->status = rec.status()[0] == 'E' ? Moderation::CS_ENABLED :
          Moderation::CS_DISABLED;

        double balance = rec.balance();
        result_info->balance = balance;

        return result_info._retn();
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "CustomerManagerImpl::get_customer: El::Exception caught. "
          "Description:\n"
             << e;
        
        ImplementationException ex;
        ex.description = ostr.str().c_str();
        throw ex;
      }
    }

    ::NewsGate::Moderation::CustomerInfo*
    CustomerManagerImpl::update_customer(
      const ::NewsGate::Moderation::CustomerUpdateInfo& customer_info)
      throw(NewsGate::Moderation::PermissionDenied,
            NewsGate::Moderation::AccountNotExist,
            NewsGate::Moderation::NotReady,
            NewsGate::Moderation::ImplementationException,
            ::CORBA::SystemException)
    {
      try
      {
        ::NewsGate::Moderation::CustomerInfo_var result_info =
          new ::NewsGate::Moderation::CustomerInfo();

        result_info->id = customer_info.id;
        result_info->status = customer_info.status;
        
        El::MySQL::Connection_var connection =
          Application::instance()->dbase()->connect();            
          
        El::MySQL::Result_var qresult = connection->query("begin");

        try
        {
          El::MySQL::Result_var qresult;
          
          {
            std::ostringstream ostr;
            ostr << "select balance from Customer where id=" <<
              customer_info.id << " for update";
            
            qresult = connection->query(ostr.str().c_str());
          }

          Customer::CustomerBalance rec(qresult.in());
          
          if(!rec.fetch_row())
          {
            throw NewsGate::Moderation::AccountNotExist();
          }
          
          double balance = rec.balance() + customer_info.balance_change;
          
          {
            std::ostringstream ostr;
            ostr.flags(std::ios::scientific);
            ostr.precision(std::numeric_limits<double>::digits10 + 1);
            
            ostr << "update Customer set balance="
                 << balance << ", status='" <<
              (customer_info.status == Moderation::CS_ENABLED ? 'E' : 'D')
                 << "' where id=" << customer_info.id;
            
            qresult = connection->query(ostr.str().c_str());
          }

          result_info->balance = balance;
          
          qresult = connection->query("commit");
        }
        catch(...)
        {
          qresult = connection->query("rollback");
          throw;
        }
          
        return result_info._retn();
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "CustomerManagerImpl::update_customer: El::Exception caught. "
          "Description:\n"
             << e;
        
        ImplementationException ex;
        ex.description = ostr.str().c_str();
        throw ex;
      }
    }
  }  
}
