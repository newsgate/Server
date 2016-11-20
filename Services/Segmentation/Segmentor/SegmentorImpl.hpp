/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Segmentation/Segmentor/SegmentorImpl.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_SEGMENTATION_SEGMENTOR_SEGMENTORIMPL_HPP_
#define _NEWSGATE_SERVER_SERVICES_SEGMENTATION_SEGMENTOR_SEGMENTORIMPL_HPP_

#include <memory>
#include <vector>

#include <ext/hash_map>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/RefCount/All.hpp>

#include <NewsGate/Segmentation.hpp>

#include <El/Service/Service.hpp>
#include <El/Service/CompoundService.hpp>

#include <Services/Segmentation/Commons/SegmentationServices_s.hpp>

namespace NewsGate
{
  namespace Segmentation
  {
    class SegmentorImpl :
      public virtual POA_NewsGate::Segmentation::Segmentor,
      public virtual El::Service::CompoundService<>
    {
    public:
      EL_EXCEPTION(Exception, El::ExceptionBase);
      EL_EXCEPTION(InvalidArgument, Exception);

    public:

      SegmentorImpl(El::Service::Callback* callback)
        throw(InvalidArgument, Exception, El::Exception);

      virtual ~SegmentorImpl() throw();

    protected:

      //
      // IDL:NewsGate/Segmentation/Segmentor/is_ready:1.0
      //
      virtual ::CORBA::Boolean is_ready()
        throw(NewsGate::Segmentation::ImplementationException,
              ::CORBA::SystemException);
      
      //
      // IDL:NewsGate/Segmentation/Segmentor/segment_message_components:1.0
      //
      virtual ::NewsGate::Segmentation::Transport::SegmentedMessagePack*
      segment_message_components(
        ::NewsGate::Segmentation::Transport::MessagePack* messages)
        throw(NewsGate::Segmentation::NotReady,
              NewsGate::Segmentation::InvalidArgument,
              NewsGate::Segmentation::ImplementationException,
              ::CORBA::SystemException);
      
      //
      // IDL:NewsGate/Segmentation/Segmentor/segment_text:1.0
      //
      virtual char* segment_text(
        const char* text,
        ::NewsGate::Segmentation::PositionSeq_out segmentation_positions)
        throw(NewsGate::Segmentation::NotReady,
              NewsGate::Segmentation::InvalidArgument,
              NewsGate::Segmentation::ImplementationException,
              ::CORBA::SystemException);
      
      //
      // IDL:NewsGate/Segmentation/Segmentor/segment_query:1.0
      //
      virtual char* segment_query(const char* text)
        throw(NewsGate::Segmentation::NotReady,
              NewsGate::Segmentation::InvalidArgument,
              NewsGate::Segmentation::ImplementationException,
              ::CORBA::SystemException);
      
      virtual bool notify(El::Service::Event* event) throw(El::Exception);
      
    private:

      struct LoadSegmentors : public El::Service::CompoundServiceMessage
      {
        LoadSegmentors(SegmentorImpl* service) throw(El::Exception);
      };
      
      void load_segmentors() throw(El::Exception);

    private:

      struct SegmentorInfo
      {
        void* lib_handle;
        Segmentation::Interface* segmentor;

        SegmentorInfo(const char* filename,
                      const char* create_func_name,
                      const char* create_func_args)
          throw(Exception, El::Exception);

        void release() throw();
      };
      
      struct SegmentorInfoArray : public std::vector<SegmentorInfo>
      {
        ~SegmentorInfoArray() throw();

        std::string segment_text(const char* src) const
          throw(InvalidArgument, El::Exception);

        std::string segment_query(const char* src) const
          throw(InvalidArgument, El::Exception);
      };
      
      typedef std::auto_ptr<SegmentorInfoArray> SegmentorInfoArrayPtr;

      SegmentorInfoArrayPtr segmentors_;
    };

    typedef El::RefCount::SmartPtr<SegmentorImpl> SegmentorImpl_var;

  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Segmentation
  {   
    //
    // SegmentorImpl::LoadDicts class
    //
    inline
    SegmentorImpl::LoadSegmentors::LoadSegmentors(SegmentorImpl* state)
      throw(El::Exception)
        : El__Service__CompoundServiceMessageBase(state, state, false),
          El::Service::CompoundServiceMessage(state, state)
    {
    }
   
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_SEGMENTATION_SEGMENTOR_SEGMENTORIMPL_HPP_
