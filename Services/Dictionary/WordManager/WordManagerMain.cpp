/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Dictionary/WordManager/WordManagerMain.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <Python.h>
#include <locale.h>

#include <El/CORBA/Corba.hpp>
#include <El/Exception.hpp>
#include <El/Logging/StreamLogger.hpp>

#include <xsd/ConfigParser.hpp>

#include <Commons/Search/TransportImpl.hpp>
#include <Services/Dictionary/Commons/TransportImpl.hpp>

#include "WordManagerMain.hpp"

namespace
{
  const char ASPECT[]  = "WordManager";
  const char USAGE[] = "Usage: WordManager <config_file> <orb options>";
}

WordManagerApp::WordManagerApp() throw (El::Exception)
    : logger_(new El::Logging::StreamLogger(std::cerr))
{
}

void
WordManagerApp::terminate_process() throw(El::Exception)
{
  if(word_manager_impl_.in() != 0)
  {
    word_manager_impl_->stop();
    word_manager_impl_->wait();
  }
}

void
WordManagerApp::main(int& argc, char** argv) throw()
{
  try
  {
    try
    {
//      ACE_OS::sleep(20);
      
      srand(time(0));
      
      if (!::setlocale(LC_CTYPE, "en_US.utf8"))
      {
        throw Exception("WordManagerApp::main: cannot set en_US.utf8 locale");
      }

      // Getting orb reference
      
      El::Corba::OrbAdapter* orb_adapter =
        El::Corba::Adapter::orb_adapter(argc, argv);

      orb(orb_adapter->orb());
      
      // Checking params
      
      if (argc < 2)
      {
        std::ostringstream ostr;
        ostr << "WordManagerApp::main: config file is not specified\n"
             << USAGE;
        
        throw InvalidArgument(ostr.str());
      }

      config_ = ConfigPtr(Server::Config::config(argv[1]));      
      logger_.reset(NewsGate::Config::create_logger(config().logger()));

      // Creating and registering value type factories      

      ::NewsGate::Dictionary::Transport::register_valuetype_factories(orb());
      ::NewsGate::Search::Transport::register_valuetype_factories(orb());

      // Getting poa & poa manager references

      CORBA::Object_var poa_object =
        orb()->resolve_initial_references("RootPOA");
        
      PortableServer::POA_var root_poa =
        PortableServer::POA::_narrow(poa_object.in ());
    
      if (CORBA::is_nil(root_poa.in()))
      {
        throw Exception("WordManagerApp::main: "
                        "Unable to resolve RootPOA.");
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
        root_poa->create_POA("PersistentIdPOA", poa_manager, policies);
      
      policies[0]->destroy();
      policies[1]->destroy();

      // Creating word manager server servant
      
      word_manager_impl_ = new NewsGate::Dictionary::WordManagerImpl(this);
    
      // Creating Object Ids

      PortableServer::ObjectId_var word_manager_obj_id =
        PortableServer::string_to_ObjectId("WordManager");
      
      PortableServer::ObjectId_var process_control_id =
        PortableServer::string_to_ObjectId("ProcessControl");
    
      // Activating CORBA objects
      
      service_poa->activate_object_with_id(word_manager_obj_id,
                                           word_manager_impl_.in());
      
      service_poa->activate_object_with_id(process_control_id, this);
      
      // Registering reference in IOR table

      CORBA::Object_var word_manager =
        service_poa->id_to_reference(word_manager_obj_id);

      CORBA::Object_var process_control =
        service_poa->id_to_reference(process_control_id);
    
      orb_adapter->add_binding("ProcessControl",
                              process_control,
                              "PersistentIdPOA");
      
      orb_adapter->add_binding("WordManager",
                              word_manager,
                              "PersistentIdPOA");

      word_manager_impl_->start();
      
      // Activating POA manager
      
      poa_manager->activate();

      // Running orb loop
      
      orb_adapter->orb_run();
  
      // Waiting for application to stop

      wait();  

      word_manager_impl_ = 0;
      
      // To stop rotator thread
      logger_.reset(0);

      El::Corba::Adapter::orb_adapter_cleanup();
    }
    catch (const CORBA::Exception& e)
    {
      std::ostringstream ostr;
      ostr << "WordManagerApp::main: CORBA::Exception caught. "
        "Description: \n" << e;
        
      Application::logger()->emergency(ostr.str(), ASPECT);
    }
    catch (const xml_schema::exception& e)
    {
      std::ostringstream ostr;
      ostr << "WordManagerApp::main: xml_schema::exception caught. "
        "Description: \n" << e;
      
      Application::logger()->emergency(ostr.str(), ASPECT);
    }
    catch (const El::Exception& e)
    {
      std::ostringstream ostr;
      ostr << "WordManagerApp::main: El::Exception caught. Description: \n"
           << e;
      
      Application::logger()->emergency(ostr.str(), ASPECT);
    }
  }
  catch (...)
  {
    Application::logger()->emergency(
      "WordManagerApp::main: unknown exception caught",
      ASPECT);
  }
}

bool
WordManagerApp::notify(El::Service::Event* event)
  throw(El::Exception)
{

  El::Service::Error* error = dynamic_cast<El::Service::Error*>(event);
  
  if(error != 0)
  {
    El::Service::log(event,
                     "WordManagerApp::notify: ",
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
