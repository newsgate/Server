/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Segmentation/Segmentor/SegmentorImpl.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <dlfcn.h>

#include <string>
#include <sstream>

#include <ace/OS.h>

#include <Commons/Message/StoredMessage.hpp>
#include <Services/Segmentation/Commons/TransportImpl.hpp>

#include "SegmentorImpl.hpp"
#include "SegmentorMain.hpp"

namespace
{
  const char ASPECT[] = "State";
}

namespace NewsGate
{
  namespace Segmentation
  {
    //
    // SegmentorImpl class
    //
    SegmentorImpl::SegmentorImpl(El::Service::Callback* callback)
      throw(InvalidArgument, Exception, El::Exception)
        : El::Service::CompoundService<>(callback, "SegmentorImpl")
    {
      El::Service::CompoundServiceMessage_var msg = new LoadSegmentors(this);
      deliver_now(msg.in());
    }

    SegmentorImpl::~SegmentorImpl() throw()
    {
      // Check if state is active, then deactivate and log error

      segmentors_.reset(0);
    }

    char*
    SegmentorImpl::segment_text(
      const char* text,
      ::NewsGate::Segmentation::PositionSeq_out segmentation_positions)
      throw(NewsGate::Segmentation::NotReady,
            NewsGate::Segmentation::InvalidArgument,
            NewsGate::Segmentation::ImplementationException,
            ::CORBA::SystemException)
    {
      CORBA::String_var result;
      
      try
      {
        ReadGuard guard(srv_lock_); 

        if(segmentors_.get() == 0)
        {
          NewsGate::Segmentation::NotReady ex;
        
          ex.reason = "SegmentorImpl::segment_text: "
            "segmentors are not loaded yet";

          throw ex;
        }

        guard.release();

        Transport::Text res;
        res.set(segmentors_->segment_text(text).c_str(), text);

        size_t len = res.inserted_spaces.size();
        
        segmentation_positions = new PositionSeq();
        segmentation_positions->length(len);

        size_t i = 0;
        
        for(Message::SegMarkerPositionSet::const_iterator it =
              res.inserted_spaces.begin(); it != res.inserted_spaces.end();
            it++, i++)
        {
          segmentation_positions[i] = *it;
        }

        result = res.value.c_str();
      }
      catch(const InvalidArgument& e)
      {
        std::ostringstream ostr;
        ostr << "SegmentorImpl::segment_text: InvalidArgument "
          "caught. Description: " << e;
        
        NewsGate::Segmentation::InvalidArgument ex;
        ex.description = ostr.str().c_str();

        throw ex;
      }
      catch(const Transport::InvalidArgument& e)
      {
        std::ostringstream ostr;
        ostr << "SegmentorImpl::segment_text: "
          "Transport::InvalidArgument caught. Description: " << e;
        
        NewsGate::Segmentation::InvalidArgument ex;
        ex.description = ostr.str().c_str();

        throw ex;
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "SegmentorImpl::segment_text: El::Exception "
          "caught. Description: " << e;
        
        NewsGate::Segmentation::ImplementationException ex;        
        ex.description = ostr.str().c_str();

        throw ex;
      }

      return result._retn();
    }
    
    char*
    SegmentorImpl::segment_query(const char* text)
      throw(NewsGate::Segmentation::NotReady,
            NewsGate::Segmentation::InvalidArgument,
            NewsGate::Segmentation::ImplementationException,
            ::CORBA::SystemException)
    {
      CORBA::String_var result;
      
      try
      {
        ReadGuard guard(srv_lock_); 

        if(segmentors_.get() == 0)
        {
          NewsGate::Segmentation::NotReady ex;
        
          ex.reason = "SegmentorImpl::segment_query: "
            "segmentors are not loaded yet";

          throw ex;
        }

        guard.release();
        result = segmentors_->segment_query(text).c_str();
      }
      catch(const InvalidArgument& e)
      {
        std::ostringstream ostr;
        ostr << "SegmentorImpl::segment_query: InvalidArgument "
          "caught. Description: " << e;
        
        NewsGate::Segmentation::InvalidArgument ex;
        ex.description = ostr.str().c_str();

        throw ex;
      }
      catch(const Transport::InvalidArgument& e)
      {
        std::ostringstream ostr;
        ostr << "SegmentorImpl::segment_query: "
          "Transport::InvalidArgument caught. Description: " << e;
        
        NewsGate::Segmentation::InvalidArgument ex;
        ex.description = ostr.str().c_str();

        throw ex;
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "SegmentorImpl::segment_query: El::Exception "
          "caught. Description: " << e;
        
        NewsGate::Segmentation::ImplementationException ex;        
        ex.description = ostr.str().c_str();

        throw ex;
      }

      return result._retn();
    }
    
    ::NewsGate::Segmentation::Transport::SegmentedMessagePack*
    SegmentorImpl::segment_message_components(

      ::NewsGate::Segmentation::Transport::MessagePack* messages)
      throw(NewsGate::Segmentation::NotReady,
            NewsGate::Segmentation::InvalidArgument,
            NewsGate::Segmentation::ImplementationException,
            ::CORBA::SystemException)
    {
      Transport::SegmentedMessagePackImpl::Var result_pack;
      
      try
      {
        ReadGuard guard(srv_lock_); 

        if(segmentors_.get() == 0)
        {
          NewsGate::Segmentation::NotReady ex;
        
          ex.reason = "SegmentorImpl::segment_message_components: "
            "segmentors are not loaded yet";

          throw ex;
        }

        guard.release();

        Transport::MessagePackImpl::Type* message_pack =
          dynamic_cast<Transport::MessagePackImpl::Type*>(messages);

        if(message_pack == 0)
        {
          NewsGate::Segmentation::ImplementationException ex;
        
          ex.description =
            "SegmentorImpl::segment_message_components: dynamic_cast<"
            "Transport::MessagePackImpl::Type*>(messages) failed";

          throw ex;
        }
      
        Transport::MessageArray& message_array = message_pack->entities();

        result_pack =
          new Transport::SegmentedMessagePackImpl::Type(
            new Transport::SegmentedMessageArray());

        Transport::SegmentedMessageArray& res = result_pack->entities();
        res.resize(message_array.size());

        for(size_t i = 0; i < message_array.size(); i++)
        {
          const Transport::Message& message = message_array[i];
          Transport::SegmentedMessage& result = res[i];

          const char* title = message.title.c_str();
          result.title.set(segmentors_->segment_text(title).c_str(), title);

          const char* description = message.description.c_str();

          result.description.set(
            segmentors_->segment_text(description).c_str(),
            description);

          const char* keywords = message.keywords.c_str();
          
          result.keywords.set(segmentors_->segment_text(keywords).c_str(),
                              keywords);
          
          result.images.resize(message.images.size());

          for(size_t i = 0; i < result.images.size(); i++)
          {
            const char* alt = message.images[i].alt.c_str();
              
            result.images[i].alt.set(segmentors_->segment_text(alt).c_str(),
                                     alt);
          }
        }
      }
      catch(const InvalidArgument& e)
      {
        std::ostringstream ostr;
        ostr << "SegmentorImpl::segment_message_components: InvalidArgument "
          "caught. Description: " << e;
        
        NewsGate::Segmentation::InvalidArgument ex;
        ex.description = ostr.str().c_str();

        throw ex;
      }
      catch(const Transport::InvalidArgument& e)
      {
        std::ostringstream ostr;
        ostr << "SegmentorImpl::segment_message_components: "
          "Transport::InvalidArgument caught. Description: " << e;
        
        NewsGate::Segmentation::InvalidArgument ex;
        ex.description = ostr.str().c_str();

        throw ex;
      }
      catch(const El::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "SegmentorImpl::segment_message_components: El::Exception "
          "caught. Description: " << e;
        
        NewsGate::Segmentation::ImplementationException ex;        
        ex.description = ostr.str().c_str();

        throw ex;
      }

      return result_pack._retn();
    }
    
    ::CORBA::Boolean
    SegmentorImpl::is_ready()
      throw(NewsGate::Segmentation::ImplementationException,
            ::CORBA::SystemException)
    {
      ReadGuard guard(srv_lock_); 
      return segmentors_.get() != 0;
    }
    
    bool
    SegmentorImpl::notify(El::Service::Event* event) throw(El::Exception)
    {
      if(El::Service::CompoundService<>::notify(event))
      {
        return true;
      }

      if(dynamic_cast<LoadSegmentors*>(event) != 0)
      {
        load_segmentors();
        return true;
      }

      return false;
    }

    void
    SegmentorImpl::load_segmentors() throw(El::Exception)
    {
      const Server::Config::SegmentorType& config =
        Application::instance()->config();
      
      typedef Server::Config::SegmentorImplementationsType::factory_sequence
        ImplsConf;

      const ImplsConf& factories = config.implementations().factory();
      El::Logging::Logger* logger = Application::logger();

      SegmentorInfoArrayPtr segmentors(new SegmentorInfoArray());
      
      for(ImplsConf::const_iterator it = factories.begin();
          it != factories.end(); it++)
      {
        std::string filename = it->filename();

//        ACE_OS::sleep(20);
        
        {
          std::ostringstream ostr;
          ostr << "SegmentorImpl::load_segmentors: loading " << filename
               << " ...";
          
          logger->info(ostr.str().c_str(), ASPECT);
        }

        std::string create_func_name = it->create_func_name();
        std::string create_func_args = it->create_func_args();
        
        SegmentorInfo segmentor_info(filename.c_str(),
                                     create_func_name.c_str(),
                                     create_func_args.c_str());

        segmentors->push_back(segmentor_info);
      }
      
      {
        WriteGuard guard(srv_lock_); 
        segmentors_.reset(segmentors.release());
      }
      
      logger->info("SegmentorImpl::load_segmentors: loading completed",
                   ASPECT);
    }

    //
    // SegmentorImpl::SegmentorInfo struct
    //
    SegmentorImpl::SegmentorInfo::SegmentorInfo(
      const char* filename,
      const char* create_func_name,
      const char* create_func_args)
      throw(Exception, El::Exception)
    : lib_handle(0),
      segmentor(0)
    {
      lib_handle = dlopen(filename, RTLD_NOW);
      
      if(!lib_handle)
      {
        std::ostringstream ostr;
        ostr << "SegmentorImpl::SegmentorInfo::SegmentorInfo: "
          "dlopen failed for '"
             << filename << "'.\nReason: " << dlerror();
        
        throw Exception(ostr.str());
      }

      CreateSegmentor create_segmentor =
        (CreateSegmentor)dlsym(lib_handle, create_func_name);

      const char* error = dlerror();
      
      if(error)
      {
        std::ostringstream ostr;
        ostr << "SegmentorImpl::SegmentorInfo::SegmentorInfo: "
          "dlsym failed for function '" << create_func_name << "', library '"
             << filename << "'.\nReason: " << error;
        
        release();
        
        throw Exception(ostr.str());
      }

      if(create_segmentor == 0)
      {
        std::ostringstream ostr;
        ostr << "SegmentorImpl::SegmentorInfo::SegmentorInfo: "
          "dlsym returned 0 for function '" << create_func_name
             << "', library '" << filename << "'.\nReason: " << error;
        
        release();
        
        throw Exception(ostr.str());
      }

      segmentor = create_segmentor(create_func_args);

      if(segmentor == 0)
      {
        std::ostringstream ostr;
        ostr << "SegmentorImpl::SegmentorInfo::SegmentorInfo: function call '"
             << create_func_name << "(\"" << create_func_args
             << "\")' returned 0, library '"
             << filename << "'.\nReason: " << error;
        
        release();
        
        throw Exception(ostr.str());
      }

      error = segmentor->creation_error();
      
      if(error && *error != '\0')
      {
        std::ostringstream ostr;
        ostr << "SegmentorImpl::SegmentorInfo::SegmentorInfo: "
          "segmentor returned by '" << create_func_name << "(\""
             << create_func_args << "\")', library '"
             << filename << "' is not functional.\nReason: " << error;
        
        release();
        
        throw Exception(ostr.str());
      }
      
    }
    
    void
    SegmentorImpl::SegmentorInfo::release() throw()
    {
      if(lib_handle)
      {
        if(segmentor)
        {
          segmentor->release();
          segmentor = 0;
        }
        
        dlclose(lib_handle);
        lib_handle = 0;
      }
    }
    
    //
    // SegmentorImpl::SegmentorInfo struct
    //
    SegmentorImpl::SegmentorInfoArray::~SegmentorInfoArray() throw()
    {
      for(iterator it = begin(); it != end(); it++)
      {
        it->release();
      }
    }

    std::string
    SegmentorImpl::SegmentorInfoArray::segment_text(const char* src) const
      throw(InvalidArgument, El::Exception)
    {
      if(empty())
      {
        return src;
      }
      
      std::string text;

//      std::cerr << "segment_text:\n";
        
      for(const_iterator it = begin(); it != end(); it++)
      {
        try
        {
          const char* src_text = it == begin() ? src : text.c_str();
          text = it->segmentor->segment_text(src_text);

//          std::cerr << src_text << std::endl << text << std::endl;
        }
        catch(const Segmentation::Interface::Exception& e)
        {
          std::ostringstream ostr;

          ostr << "SegmentorImpl::SegmentorInfoArray::segment_text: "
            "Segmentation::Interface::segment_text failed. Reason:\n"
               << e.what();
          
          throw InvalidArgument(ostr.str());
        }
      }

      return text;
    }

    std::string
    SegmentorImpl::SegmentorInfoArray::segment_query(const char* src) const
      throw(InvalidArgument, El::Exception)
    {
      if(empty())
      {
        return src;
      }
      
      std::string text;
      
      for(const_iterator it = begin(); it != end(); it++)
      {
        try
        {
          text =
            it->segmentor->segment_query(it == begin() ? src : text.c_str());
        }
        catch(const Segmentation::Interface::Exception& e)
        {
          std::ostringstream ostr;

          ostr << "SegmentorImpl::SegmentorInfoArray::segment_query: "
            "Segmentation::Interface::segment_query failed. Reason:\n"
               << e.what();
          
          throw InvalidArgument(ostr.str());
        }
      }

      return text;
    }
  }
}
