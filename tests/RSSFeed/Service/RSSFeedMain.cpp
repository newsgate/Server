/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/tests/RSSFeed/Service/RSSFeedMain.cpp
 * @author Karen Arutyunov
 * $Id:$
 */
 
#include <El/CORBA/Corba.hpp>

#include <stdlib.h>
#include <time.h>

#include <El/Exception.hpp>

#include <El/Logging/FileLogger.hpp>
#include <El/Logging/StreamLogger.hpp>

#include "RSSFeedMain.hpp"

namespace
{
  const char ASPECT[]  = "RSSFeed";
  const char USAGE[] = "Usage: RSSFeed <config_file> <orb options>";
}

RSSFeedApp::RSSFeedApp() throw (El::Exception)
    : logger_(new El::Logging::StreamLogger(std::cerr))
{
}

void
RSSFeedApp::terminate_process() throw(El::Exception)
{
  if(feed_impl_.in() != 0)
  {
    feed_impl_->stop();
    feed_impl_->wait();
  }
}

void
RSSFeedApp::main(int& argc, char** argv) throw()
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
        ostr << "RSSFeedApp::main: config file is not specified\n" << USAGE;
        throw InvalidArgument(ostr.str());
      }

      config_ = ConfigPtr(Server::TestRSSFeed::Config::config(argv[1]));

      const Server::TestRSSFeed::Config::LoggerType& logger_cfg =
        config().logger();

      El::Logging::FileLogger::RotatingPolicyList logger_policies;

      if(logger_cfg.time_span_policy().present ())
      {
        logger_policies.push_back(
          new El::Logging::FileLogger::RotatingByTimePolicy(
            ACE_Time_Value(logger_cfg.time_span_policy()->time())));
      }

      if(logger_cfg.size_span_policy().present ())
      {
        logger_policies.push_back(
          new El::Logging::FileLogger::RotatingBySizePolicy(
            logger_cfg.size_span_policy()->size()));
      }

      logger_ =
        El::Logging::LoggerPtr(
          new El::Logging::FileLogger(logger_cfg.filename().c_str(),
                                      logger_cfg.log_level(),
                                      "*",
                                      &logger_policies));

      // Getting poa & poa manager references

      CORBA::Object_var poa_object =
        orb()->resolve_initial_references("RootPOA");
        
      PortableServer::POA_var root_poa =
        PortableServer::POA::_narrow (poa_object.in ());
    
      if (CORBA::is_nil (root_poa.in ()))
      {
        throw Exception("RSSFeedApp::main: "
                        "Unable to resolve RootPOA.");
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
                             poa_manager, policies);
      policies[0]->destroy();
      policies[1]->destroy();

      // Creating servant
      
      feed_impl_ =
        new NewsGate::RSS::FeedServiceImpl(this, config().rss_feeds());

      // Creating Object Ids

      PortableServer::ObjectId_var feed_obj_id =
        PortableServer::string_to_ObjectId("RSSFeed");
      
      PortableServer::ObjectId_var process_control_id =
        PortableServer::string_to_ObjectId("ProcessControl");
    
      // Activating CORBA objects
      
      service_poa->activate_object_with_id(feed_obj_id, feed_impl_.in());

      service_poa->activate_object_with_id(process_control_id, this);

      poa_manager->activate();
    
      CORBA::Object_var process_control =
        service_poa->id_to_reference(process_control_id);
    
      CORBA::Object_var feed_obj =
        service_poa->id_to_reference(feed_obj_id);

      // Registering reference in IOR table
      
      orb_adapter->add_binding("ProcessControl",
                               process_control,
                               "PersistentIdPOA");
      
      orb_adapter->add_binding("RSSFeed",
                               feed_obj,
                               "PersistentIdPOA");
    
      feed_impl_->start();

      // Activating POA manager
      
      poa_manager->activate ();
  
      // Running orb loop
      
      orb()->run ();
  
      // Waiting for application to stop

      wait();

      feed_impl_ = 0;

      // To stop rotator thread
      logger_.reset(0);

      El::Corba::Adapter::orb_adapter_cleanup();
    }
    catch (const CORBA::Exception& e)
    {
      std::ostringstream ostr;
      ostr << "RSSFeedApp::main: CORBA::Exception caught. Description: \n"
           << e;
        
      Application::logger()->emergency(ostr.str(), ASPECT);
    }
    catch (const xml_schema::exception& e)
    {
      std::ostringstream ostr;
      ostr << "RSSFeedApp::main: xml_schema::exception caught. "
        "Description: \n" << e;
      
      Application::logger()->emergency(ostr.str(), ASPECT);
    }
    catch (const El::Exception& e)
    {
      std::ostringstream ostr;
      ostr << "RSSFeedApp::main: El::Exception caught. Description: \n"
           << e;
      
      Application::logger()->emergency(ostr.str(), ASPECT);
    }
  }
  catch (...)
  {
    Application::logger()->emergency(
      "RSSFeedApp::main: unknown exception caught",
      ASPECT);
  }
}

bool
RSSFeedApp::notify(El::Service::Event* event) throw(El::Exception)
{
  El::Service::Error* error = dynamic_cast<El::Service::Error*>(event);
  
  if(error != 0)
  {
    El::Service::log(event,
                     "RSSFeedApp::notify: ",
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
