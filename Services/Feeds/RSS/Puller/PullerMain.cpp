/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Feeds/RSS/Puller/PullerMain.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <Python.h>
//#include <Magick++.h>

#include <locale.h>
#include <stdlib.h>

#include <El/CORBA/Corba.hpp>

#include <xercesc/util/XMLString.hpp>
#include <xercesc/sax2/SAX2XMLReader.hpp>
XERCES_CPP_NAMESPACE_USE

#include <El/Exception.hpp>
#include <El/String/Manip.hpp>
#include <El/Logging/FileLogger.hpp>
#include <El/Logging/StreamLogger.hpp>
#include <El/XML/Use.hpp>
#include <El/Python/Utility.hpp>
#include <El/Python/Sandbox.hpp>
#include <El/Python/InterceptorImpl.hpp>

#include <xsd/ConfigParser.hpp>

#include <Commons/Message/TransportImpl.hpp>
#include <Services/Commons/Message/BankClientSessionImpl.hpp>
#include <Services/Feeds/RSS/Commons/TransportImpl.hpp>

#include "PullingFeeds.hpp"
#include "PullerMain.hpp"

namespace
{
  const char ASPECT[] = "Application";
  const char USAGE[] = "Usage: RSSPuller <config_file> <orb options>";
}

PullerApp::PullerApp() throw (El::Exception)
    : logger_(new El::Logging::StreamLogger(std::cerr))
{
}

void
PullerApp::terminate_process() throw(El::Exception)
{
  if(puller_impl_.in() != 0)
  {
//    std::cerr << "Stopping...\n";
    
    puller_impl_->stop();
    puller_impl_->wait();

//    std::cerr << "Stopped\n";
  }
}

void
PullerApp::main(int& argc, char** argv) throw()
{
/*
  std::cerr << "sizeof(PullingFeeds::FeedInfo) "
            << sizeof(NewsGate::RSS::PullingFeeds::FeedInfo)
            << std::endl;

  std::cerr << "sizeof(::NewsGate::Feed::Space) "
            << sizeof(::NewsGate::Feed::Space) << std::endl;
*/ 
  try
  {
    try
    {
      if (!::setlocale(LC_CTYPE, "en_US.utf8"))
      {
        throw Exception("PullerApp::main: cannot set en_US.utf8 locale");
      }
/*
      try 
      {
        Magick::InitializeMagick(0);
      }
      catch(const Magick::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "PullerApp::main: InitializeMagick failed. Reason: " << e;
        throw Exception(ostr.str());
      }
*/

      El::Python::Use python_use;
      
//      El::Python::InterceptorImpl::Installer installer(
//        El::Python::Sandbox::INTERCEPT_FLAGS);
      
      El::Python::AllowOtherThreads python_allow_threads;
      
      El::XML::Use xml_use;
      
/*
      setenv("LANG", "en_US.utf8", 1);

      XMLPlatformUtils::Initialize("en_US.utf8");

      XMLCh src[2];
      src[0] = 0xA0;
      src[1] = 0;

      unsigned char* dest = (unsigned char*)XMLString::transcode(src);

      if(strcmp((char*)dest, "\xC2\xA0") != 0)
      {
        std::cerr << "Shit !\n";
        return;
      }
*/
      // Getting orb reference
      
      El::Corba::OrbAdapter* orb_adapter =
        El::Corba::Adapter::orb_adapter(argc, argv);

      orb(orb_adapter->orb());

      // Checking params

      if (argc < 2)
      {
        std::ostringstream ostr;
        ostr << "PullerApp::main: config file is not specified\n" << USAGE;
        throw InvalidArgument(ostr.str());
      }

      config_ = Server::Config::config(argv[1]);
      logger_.reset(NewsGate::Config::create_logger(config().logger()));

      robots_checker_.reset(
        NewsGate::Config::create_robots_checker(
          config().feed_request().robots_checker()));

      mime_types_.reset(
        new El::Net::HTTP::MimeTypeMap(
          config().feed_request().image().thumbnail().
          mime_types_path().c_str()));
      
      // Creating and registering value type factories
      
      ::NewsGate::RSS::Transport::register_valuetype_factories(orb());

      ::NewsGate::Message::Transport::register_valuetype_factories(orb());

      ::NewsGate::Message::BankClientSessionImpl::register_valuetype_factories(
        orb());

      // Getting poa & poa manager references

      CORBA::Object_var poa_object =
        orb()->resolve_initial_references("RootPOA");
        
      PortableServer::POA_var root_poa =
        PortableServer::POA::_narrow(poa_object.in ());
    
      if (CORBA::is_nil(root_poa.in()))
      {
        throw Exception("PullerApp::main: unable to resolve RootPOA.");
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

      // Creating servant
      
      puller_impl_ = new NewsGate::RSS::PullerImpl(this);

      // Creating Object Ids

      PortableServer::ObjectId_var puller_obj_id =
        PortableServer::string_to_ObjectId("Puller");
      
      PortableServer::ObjectId_var process_control_id =
        PortableServer::string_to_ObjectId("ProcessControl");
    
      // activating CORBA objects
      
      service_poa->activate_object_with_id(puller_obj_id,
                                           puller_impl_.in());

      service_poa->activate_object_with_id(process_control_id, this);

      CORBA::Object_var puller =
        service_poa->id_to_reference(puller_obj_id);

      puller_corba_ref_ = NewsGate::RSS::Puller::_narrow(puller.in());

      CORBA::Object_var process_control =
        service_poa->id_to_reference(process_control_id);
    
      // Registering reference in IOR table
      
      orb_adapter->add_binding("ProcessControl",
                              process_control,
                              "PersistentIdPOA");
    
      puller_impl_->start();

      // Activating POA manager
      
      poa_manager->activate ();
  
      // Running orb loop

      orb_adapter->orb_run();

//      std::cerr << "Run exited\n";

      // Waiting for application to stop
      
      wait();

//      std::cerr << "Wait exited\n";
      
      // To decrement ORB reference prior destroying
      
//      orb()->unregister_value_factory(
//        "IDL:NewsGate/Message/BankClientSession:1.0");

      puller_corba_ref_ = NewsGate::RSS::Puller::_nil();
      puller_impl_ = 0;
      
      // To stop rotator thread
      logger_.reset(0);

      El::Corba::Adapter::orb_adapter_cleanup();
    }
    catch (const CORBA::Exception& e)
    {
      std::ostringstream ostr;
      ostr << "PullerApp::main: CORBA::Exception caught. Description: \n"
           << e;
        
      Application::logger()->emergency(ostr.str(), ASPECT);
    }
    catch (const xml_schema::exception& e)
    {
      std::ostringstream ostr;
      ostr << "PullerApp::main: xml_schema::exception caught. "
        "Description: \n" << e;
      
      Application::logger()->emergency(ostr.str(), ASPECT);
    }
    catch (const El::Exception& e)
    {
      std::ostringstream ostr;
      ostr << "PullerApp::main: El::Exception caught. Description: \n"
           << e;
      
      Application::logger()->emergency(ostr.str(), ASPECT);
    }
  }
  catch (...)
  {
    Application::logger()->emergency(
      "PullerApp::main: unknown exception caught",
      ASPECT);
  }
  
//  std::cerr << "Main exit\n";    
}

bool
PullerApp::notify(El::Service::Event* event) throw(El::Exception)
{
  El::Service::Error* error = dynamic_cast<El::Service::Error*>(event);
  
  if(error != 0)
  {
    El::Service::log(event,
                     "PullerApp::notify: ",
                     Application::logger(),
                     ASPECT);
    
    return true;
  }
  
  return El::Corba::ProcessCtlImpl::notify(event);
}  

NewsGate::RSS::PullerManager_ptr
PullerApp::puller_manager()
  throw(CORBA::SystemException, Exception, El::Exception)
{
  {
    ReadGuard guard(lock_);
  
    if(!CORBA::is_nil(puller_manager_.in()))
    {
      return NewsGate::RSS::PullerManager::_duplicate(puller_manager_.in());
    }
  }
  
  WriteGuard guard(lock_);
    
  if(!CORBA::is_nil(puller_manager_.in()))
  {
    return NewsGate::RSS::PullerManager::_duplicate(puller_manager_.in());
  }

  std::string puller_manager_ref = config().puller_management().
    puller_manager_ref();

  CORBA::Object_var obj = orb()->string_to_object(puller_manager_ref.c_str());
      
  if (CORBA::is_nil(obj.in()))
  {
    std::ostringstream ostr;
    ostr << "PullerApp::puller_manager: string_to_object() "
      "gives nil reference. Ior: " << puller_manager_ref;

    throw Exception(ostr.str().c_str());
  }
  else
  {
    puller_manager_ = NewsGate::RSS::PullerManager::_narrow (obj.in ());
        
    if (CORBA::is_nil (puller_manager_.in ()))
    {
      std::ostringstream ostr;
      ostr << "PullerApp::puller_manager: RSS::PullerManager::_narrow "
        "gives nil reference. Ior: " << puller_manager_ref;
          
      throw Exception(ostr.str().c_str());
    }
    
    return NewsGate::RSS::PullerManager::_duplicate(puller_manager_.in());
  }  
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
