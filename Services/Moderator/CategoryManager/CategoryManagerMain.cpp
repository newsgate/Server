/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/CategoryManager/CategoryManagerMain.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <Python.h>
#include <unistd.h>

#include <El/CORBA/Corba.hpp>

#include <El/Exception.hpp>
#include <El/XML/Use.hpp>
#include <El/Logging/FileLogger.hpp>
#include <El/Logging/StreamLogger.hpp>

#include <xsd/ConfigParser.hpp>

#include <Commons/Message/TransportImpl.hpp>
#include <Commons/Search/TransportImpl.hpp>

#include <Services/Commons/Message/BankClientSessionImpl.hpp>
#include <Services/Dictionary/Commons/TransportImpl.hpp>
#include <Services/Segmentation/Commons/TransportImpl.hpp>

#include "CategoryManagerMain.hpp"

namespace
{
  const char ASPECT[] = "Application";
  const char USAGE[] = "Usage: CategoryManager <config_file> <orb options>";
}

CategoryManagerApp::CategoryManagerApp() throw (El::Exception)
    : logger_(new El::Logging::StreamLogger(std::cerr))
{
}

void
CategoryManagerApp::terminate_process() throw(El::Exception)
{
  if(category_manager_impl_.in() != 0)
  {
    category_manager_impl_->stop();
    category_manager_impl_->wait();
  }
}

void
CategoryManagerApp::main(int& argc, char** argv) throw()
{  
  try
  {
    srand(time(0));

    if (!::setlocale(LC_CTYPE, "en_US.utf8"))
    {
      throw Exception(
        "CategoryManagerApp::main: cannot set en_US.utf8 locale");
    }

    El::XML::Use use;

    // Getting orb reference
      
    El::Corba::OrbAdapter* orb_adapter =
      El::Corba::Adapter::orb_adapter(argc, argv);

    orb(orb_adapter->orb());

    // Checking params
      
    if (argc < 2)
    {
      std::ostringstream ostr;
      ostr << "CategoryManagerApp::main: config file is not specified\n"
           << USAGE;
        
      throw InvalidArgument(ostr.str());
    }

    config_ = ConfigPtr(Server::Config::config(argv[1]));

    logger_.reset(NewsGate::Config::create_logger(config().logger()));
    dbase_ = NewsGate::Config::create_db(config().data_base(), "utf8");

    // Creating and registering value type factories

    ::NewsGate::Message::Transport::register_valuetype_factories(orb());
    ::NewsGate::Search::Transport::register_valuetype_factories(orb());
    ::NewsGate::Dictionary::Transport::register_valuetype_factories(orb());
    ::NewsGate::Segmentation::Transport::register_valuetype_factories(orb());

    ::NewsGate::Message::BankClientSessionImpl::register_valuetype_factories(
      orb());

    //
    // Resolving WordManager reference
    //

    std::string word_manager_ref = config().word_manager_ref();
      
    CORBA::Object_var obj =
      orb()->string_to_object(word_manager_ref.c_str());
      
    if (CORBA::is_nil(obj.in()))
    {
      std::ostringstream ostr;
      ostr << "CategoryManagerApp::main: string_to_object() "
        "gives nil reference. Ior: " << word_manager_ref;

      throw Exception(ostr.str().c_str());
    }

    word_manager_ = NewsGate::Dictionary::WordManager::_narrow(obj.in());
        
    if(CORBA::is_nil(word_manager_.in()))
    {
      std::ostringstream ostr;
      ostr << "CategoryManagerApp::main: Dictionary::WordManager::_narrow "
        "gives nil reference. Ior: " << word_manager_ref;
    
      throw Exception(ostr.str().c_str());
    }

    //
    // Resolving Segmentor reference
    //

    std::string segmentor_ref = config().segmentor_ref();
      
    if(!segmentor_ref.empty())
    {
      obj = orb()->string_to_object(segmentor_ref.c_str());
      
      if (CORBA::is_nil(obj.in()))
      {
        std::ostringstream ostr;
        ostr << "CategoryManagerApp::main: string_to_object() "
          "gives nil reference. Ior: " << segmentor_ref;
          
        throw Exception(ostr.str().c_str());
      }
        
      segmentor_ = NewsGate::Segmentation::Segmentor::_narrow(obj.in());
        
      if(CORBA::is_nil(segmentor_.in()))
      {
        std::ostringstream ostr;
        ostr << "CategoryManagerApp::main: Segmentation::Segmentor::_narrow "
          "gives nil reference. Ior: " << segmentor_ref;
          
        throw Exception(ostr.str().c_str());
      }
    }
    
    // Getting poa & poa manager references

    CORBA::Object_var poa_object =
      orb()->resolve_initial_references("RootPOA");
        
    PortableServer::POA_var root_poa =
      PortableServer::POA::_narrow (poa_object.in ());
    
    if (CORBA::is_nil (root_poa.in ()))
    {
      throw Exception("CategoryManagerApp::main: unable to resolve RootPOA.");
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
    
    // Creating category manager servant
      
    category_manager_impl_ =
      new NewsGate::Moderation::Category::ManagerImpl(this);
    
    // Creating Object Ids

    PortableServer::ObjectId_var category_manager_obj_id =
      PortableServer::string_to_ObjectId("CategoryManager");
      
    PortableServer::ObjectId_var process_control_id =
      PortableServer::string_to_ObjectId("ProcessControl");
     
    // Activating CORBA objects
      
    service_poa->activate_object_with_id(category_manager_obj_id,
                                         category_manager_impl_.in());
      
    service_poa->activate_object_with_id(process_control_id, this);
      
    // Registering reference in IOR table

    CORBA::Object_var category_manager =
      service_poa->id_to_reference(category_manager_obj_id);

    CORBA::Object_var process_control =
      service_poa->id_to_reference(process_control_id);
    
    orb_adapter->add_binding("ProcessControl",
                            process_control,
                            "PersistentIdPOA");
    
    orb_adapter->add_binding("CategoryManager",
                            category_manager,
                            "PersistentIdPOA");
    
    category_manager_impl_->start();
      
    // Activating POA manager
      
    poa_manager->activate();
  
    // Running orb loop
      
    orb_adapter->orb_run();
  
    // Waiting for application to stop

    wait();

    category_manager_impl_ = 0;

    dbase_ = 0;

    // To stop rotator thread
    logger_.reset(0);

    El::Corba::Adapter::orb_adapter_cleanup();
  }
  catch (const CORBA::Exception& e)
  {
    std::ostringstream ostr;
    ostr << "CategoryManagerApp::main: CORBA::Exception caught. "
      "Description: \n" << e;
        
    Application::logger()->emergency(ostr.str(), ASPECT);
  }
  catch (const xml_schema::exception& e)
  {
    std::ostringstream ostr;
    ostr << "CategoryManagerApp::main: xml_schema::exception caught. "
      "Description: \n" << e;
      
    Application::logger()->emergency(ostr.str(), ASPECT);
  }
  catch (const El::Exception& e)
  {
    std::ostringstream ostr;
    ostr << "CategoryManagerApp::main: El::Exception caught. Description: \n"
         << e;
      
    Application::logger()->emergency(ostr.str(), ASPECT);
  }
}

bool
CategoryManagerApp::notify(El::Service::Event* event) throw(El::Exception)
{
  El::Service::Error* error = dynamic_cast<El::Service::Error*>(event);
  
  if(error != 0)
  {
    El::Service::log(event,
                     "CategoryManagerApp::notify: ",
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
