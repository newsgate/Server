/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/ModeratorManager/ModeratorManagerMain.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <Python.h>

#include <El/CORBA/Corba.hpp>
#include <El/Exception.hpp>
#include <El/Logging/FileLogger.hpp>
#include <El/Logging/StreamLogger.hpp>

#include <xsd/ConfigParser.hpp>

#include "ModeratorManagerMain.hpp"

namespace
{
  const char ASPECT[] = "Application";
  const char USAGE[] = "Usage: ModeratorManager <config_file> <orb options>";
}

ModeratorManagerApp::ModeratorManagerApp() throw (El::Exception)
    : logger_(new El::Logging::StreamLogger(std::cerr))
{
}

void
ModeratorManagerApp::terminate_process() throw(El::Exception)
{
  if(moderator_manager_impl_.in() != 0)
  {
    moderator_manager_impl_->stop();
    moderator_manager_impl_->wait();
  }
}

void
ModeratorManagerApp::main(int& argc, char** argv) throw()
{
  try
  {
    try
    {
      srand(time(0));
      
      // Getting orb reference
      
      El::Corba::OrbAdapter* orb_adapter =
        El::Corba::Adapter::orb_adapter(argc, argv);
      
      orb(orb_adapter->orb());

      // Checking params
      
      if (argc < 2)
      {
        std::ostringstream ostr;
        ostr << "ModeratorManagerApp::main: config file is not specified\n"
             << USAGE;
        
        throw InvalidArgument(ostr.str());
      }

      config_ = ConfigPtr(Server::Config::config(argv[1]));

      logger_.reset(NewsGate::Config::create_logger(config().logger()));
      dbase_ = NewsGate::Config::create_db(config().data_base(), "utf8");
      
      client_management_dbase_ = NewsGate::Config::create_db(
        config().client_management().data_base(), "utf8");

      // Getting poa & poa manager references

      CORBA::Object_var poa_object =
        orb()->resolve_initial_references("RootPOA");
        
      PortableServer::POA_var root_poa =
        PortableServer::POA::_narrow (poa_object.in ());
    
      if (CORBA::is_nil (root_poa.in ()))
      {
        throw Exception("ModeratorManagerApp::main: "
                        "unable to resolve RootPOA.");
      }
    
      PortableServer::POAManager_var poa_manager = 
        root_poa->the_POAManager ();

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
    
      // Creating message moderator manager servant
      
      moderator_manager_impl_ =
        new NewsGate::Moderation::ModeratorManagerImpl(this);
    
      // Creating Object Ids

      PortableServer::ObjectId_var moderator_manager_obj_id =
        PortableServer::string_to_ObjectId("ModeratorManager");
      
      PortableServer::ObjectId_var process_control_id =
        PortableServer::string_to_ObjectId("ProcessControl");
    
      // Activating CORBA objects
      
      service_poa->activate_object_with_id(moderator_manager_obj_id,
                                           moderator_manager_impl_.in());
      
      service_poa->activate_object_with_id(process_control_id, this);
      
      // Registering reference in IOR table

      CORBA::Object_var moderator_manager =
        service_poa->id_to_reference(moderator_manager_obj_id);

      CORBA::Object_var process_control =
        service_poa->id_to_reference(process_control_id);
      
      orb_adapter->add_binding("ProcessControl",
                               process_control,
                               "PersistentIdPOA");
      
      orb_adapter->add_binding("ModeratorManager",
                               moderator_manager,
                               "PersistentIdPOA");

      moderator_manager_impl_->start();
      
      // Activating POA manager
      
      poa_manager->activate();
  
      // Running orb loop
      
      orb_adapter->orb_run();
  
      // Waiting for application to stop

      wait();

      moderator_manager_impl_ = 0;
      
      dbase_ = 0;
      client_management_dbase_ = 0;

      // To stop rotator thread
      logger_.reset(0);

      El::Corba::Adapter::orb_adapter_cleanup();
    }
    catch (const CORBA::Exception& e)
    {
      std::ostringstream ostr;
      ostr << "ModeratorManagerApp::main: CORBA::Exception caught. "
        "Description: \n" << e;
        
      Application::logger()->emergency(ostr.str(), ASPECT);
    }
    catch (const xml_schema::exception& e)
    {
      std::ostringstream ostr;
      ostr << "ModeratorManagerApp::main: xml_schema::exception caught. "
        "Description: \n" << e;
      
      Application::logger()->emergency(ostr.str(), ASPECT);
    }
    catch (const El::Exception& e)
    {
      std::ostringstream ostr;
      ostr << "ModeratorManagerApp::main: El::Exception caught. Description: \n"
           << e;
      
      Application::logger()->emergency(ostr.str(), ASPECT);
    }
  }
  catch (...)
  {
    Application::logger()->emergency(
      "ModeratorManagerApp::main: unknown exception caught",
      ASPECT);
  }
}

bool
ModeratorManagerApp::notify(El::Service::Event* event) throw(El::Exception)
{
  El::Service::Error* error = dynamic_cast<El::Service::Error*>(event);
  
  if(error != 0)
  {
    El::Service::log(event,
                     "ModeratorManagerApp::notify: ",
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
