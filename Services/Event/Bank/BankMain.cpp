
/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Event/Bank/BankMain.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <Python.h>
#include <locale.h>

#include <El/CORBA/Corba.hpp>

#include <iostream>
#include <sstream>
#include <string>

#include <El/Exception.hpp>

#include <El/Logging/FileLogger.hpp>
#include <El/Logging/StreamLogger.hpp>

#include <xsd/ConfigParser.hpp>

#include <Commons/Message/TransportImpl.hpp>
#include <Commons/Event/TransportImpl.hpp>

#include <Services/Commons/Event/BankSessionImpl.hpp>
#include <Services/Commons/Message/BankClientSessionImpl.hpp>

#include "BankMain.hpp"

namespace
{
  const char ASPECT[] = "Application";
  const char USAGE[] = "Usage: EventBank <config_file> <orb options>";
}

BankApp::BankApp() throw (El::Exception)
    : logger_(new El::Logging::StreamLogger(std::cerr))
{
}

void
BankApp::terminate_process() throw(El::Exception)
{
  if(bank_impl_.in() != 0)
  {
    bank_impl_->stop();
    bank_impl_->wait();
    
    Application::logger()->trace(
      "BankApp::terminate_process: servant is stopped",
      ASPECT,
      El::Logging::MIDDLE);
  }
}

void
BankApp::main(int& argc, char** argv) throw()
{
  try
  {
    try
    {
      srand(time(0));

      if (!::setlocale(LC_CTYPE, "en_US.utf8"))
      {
        throw Exception("BankApp::main: cannot set en_US.utf8 locale");
      }
      
      // Getting orb reference
      
      El::Corba::OrbAdapter* orb_adapter =
        El::Corba::Adapter::orb_adapter(argc, argv);

      orb(orb_adapter->orb());

      // Checking params
      
      if (argc < 2)
      {
        std::ostringstream ostr;
        ostr << "BankApp::main: config file is not specified\n"
             << USAGE;
        
        throw InvalidArgument(ostr.str());
      }

      config_ = ConfigPtr(Server::Config::config(argv[1]));

      bank_manager_ref_ =
        config().bank_management().bank_manager_ref().c_str();
      
      logger_.reset(NewsGate::Config::create_logger(config().logger()));
      
      dbase_ = NewsGate::Config::create_db(config().data_base(), "utf8");

      // Creating and registering value type factories

      ::NewsGate::Event::Transport::register_valuetype_factories(orb());
      
      ::NewsGate::Event::BankSessionImpl::register_valuetype_factories(
        orb());

      ::NewsGate::Message::Transport::register_valuetype_factories(orb());

      ::NewsGate::Message::BankClientSessionImpl::register_valuetype_factories(
        orb());
      
      // Getting poa & poa manager references

      CORBA::Object_var poa_object =
        orb()->resolve_initial_references("RootPOA");
        
      PortableServer::POA_var root_poa =
        PortableServer::POA::_narrow (poa_object.in ());
    
      if(CORBA::is_nil(root_poa.in ()))
      {
        throw Exception("BankApp::main: unable to resolve RootPOA");
      }
    
      PortableServer::POAManager_var poa_manager = 
        root_poa->the_POAManager();

      // Creating POA with necessary policies
      
      CORBA::PolicyList policies;
      policies.length(2);

      policies[0] =
        root_poa->create_lifespan_policy(PortableServer::PERSISTENT);
      
      policies[1] =
        root_poa->create_id_assignment_policy(PortableServer::USER_ID);

      PortableServer::POA_var service_poa =
        root_poa->create_POA("PersistentIdPOA",
                             poa_manager,
                             policies);
      
      policies[0]->destroy();
      policies[1]->destroy();
    
      // Creating message bank server servant
      
      bank_impl_ = new NewsGate::Event::BankImpl(this);
    
      // Creating Object Ids
      
      PortableServer::ObjectId_var bank_obj_id =
        PortableServer::string_to_ObjectId("Bank");
      
      PortableServer::ObjectId_var process_control_id =
        PortableServer::string_to_ObjectId("ProcessControl");
    
      // Activating CORBA objects
      
      service_poa->activate_object_with_id(bank_obj_id,
                                           bank_impl_.in());
      
      service_poa->activate_object_with_id(process_control_id, this);
      
      // Registering reference in IOR table

      CORBA::Object_var bank = service_poa->id_to_reference(bank_obj_id);

      CORBA::Object_var process_control =
        service_poa->id_to_reference(process_control_id);
    
      orb_adapter->add_binding("ProcessControl",
                              process_control,
                              "PersistentIdPOA");
      
      bank_ior_ = orb_adapter->add_binding("Bank",
                                          bank,
                                          "PersistentIdPOA");

      bank_impl_->start();

      // Activating POA manager
      
      poa_manager->activate();
  
      // Running orb loop
      
      orb_adapter->orb_run();
  
      // Waiting for application to stop

      wait();
  
      bank_impl_ = 0;
      dbase_ = 0;

      // To stop rotator thread
      logger_.reset(0);

      El::Corba::Adapter::orb_adapter_cleanup();
    }
    catch (const CORBA::Exception& e)
    {
      std::ostringstream ostr;
      ostr << "BankApp::main: CORBA::Exception caught. "
        "Description: \n" << e;
        
      Application::logger()->emergency(ostr.str(), ASPECT);
    }
    catch (const xml_schema::exception& e)
    {
      std::ostringstream ostr;
      ostr << "BankApp::main: xml_schema::exception caught. "
        "Description: \n" << e;
      
      Application::logger()->emergency(ostr.str(), ASPECT);
    }
    catch (const El::Exception& e)
    {
      std::ostringstream ostr;
      ostr << "BankApp::main: El::Exception caught. Description: \n"
           << e;
      
      Application::logger()->emergency(ostr.str(), ASPECT);
    }
  }
  catch (...)
  {
    Application::logger()->emergency(
      "BankApp::main: unknown exception caught",
      ASPECT);
  }
}

NewsGate::Event::BankManager_ptr
BankApp::bank_manager()
  throw(CORBA::SystemException, Exception, El::Exception)
{
  {
    ReadGuard guard(lock_);
  
    if(!CORBA::is_nil(bank_manager_.in()))
    {
      return NewsGate::Event::BankManager::_duplicate(bank_manager_.in());
    }
  }
  
  WriteGuard guard(lock_);
    
  if(!CORBA::is_nil(bank_manager_.in()))
  {
    return NewsGate::Event::BankManager::_duplicate(bank_manager_.in());
  }

  CORBA::Object_var obj = orb()->string_to_object(bank_manager_ref_.c_str());
      
  if (CORBA::is_nil(obj.in()))
  {
    std::ostringstream ostr;
    ostr << "PullerApp::bank_manager: string_to_object() "
      "gives nil reference. Ior: " << bank_manager_ref_;

    throw Exception(ostr.str().c_str());
  }

  bank_manager_ = NewsGate::Event::BankManager::_narrow (obj.in ());
        
  if (CORBA::is_nil (bank_manager_.in ()))
  {
    std::ostringstream ostr;
    ostr << "PullerApp::bank_manager: Message::BankManager::_narrow "
      "gives nil reference. Ior: " << bank_manager_ref_;
    
    throw Exception(ostr.str().c_str());
  }
  
  return NewsGate::Event::BankManager::_duplicate(bank_manager_.in());
}

bool
BankApp::notify(El::Service::Event* event) throw(El::Exception)
{
  El::Service::Error* error = dynamic_cast<El::Service::Error*>(event);
  
  if(error != 0)
  {
    El::Service::log(event,
                     "BankApp::notify: ",
                     Application::logger(),
                     ASPECT);
    
    return true;
  }

  return El::Corba::ProcessCtlImpl::notify(event);
}
  
int
main(int argc, char** argv)
{
  try
  {
    Application::instance()->main(argc, argv);
    return 0;
  }
  catch (...)
  {
    std::cerr << "main: unknown exception caught.\n";
    return -1;
  }
}
