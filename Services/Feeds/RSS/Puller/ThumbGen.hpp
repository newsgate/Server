/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Feeds/RSS/Puller/ThumbGen.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_PULLER_THUMBGEN_HPP_
#define _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_PULLER_THUMBGEN_HPP_

#include <stdint.h>

#include <string>
#include <vector>

#include <ace/OS.h>
#include <ace/Synch.h>
#include <ace/Guard_T.h>

#include <El/Exception.hpp>

#include <El/Service/Service.hpp>
#include <El/Service/ProcessPool.hpp>
#include <El/Net/HTTP/MimeTypeMap.hpp>

namespace NewsGate
{
  namespace RSS
  {
    struct ThumbGenTask :
      public virtual El::Service::ProcessPool::TaskBase
    {
      EL_EXCEPTION(Exception, El::Service::Exception);

      struct Thumb
      {
        uint32_t width;
        uint32_t height;
        uint8_t crop;
        std::string path;
        std::string type;

        Thumb() throw(El::Exception) : width(0), height(0), crop(0){}
        Thumb(uint32_t w, uint32_t h, uint8_t c) throw(El::Exception);

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);
      };

      typedef std::vector<Thumb> ThumbArray;
      
      std::string image_path;
      uint32_t image_min_width;
      uint32_t image_min_height;
      El::Net::HTTP::MimeTypeMap mime_types;
      ThumbArray thumbs;
      
      uint8_t skip;
      std::string issue;
      uint32_t image_width;
      uint32_t image_height;      
      
      ThumbGenTask(const char* path = 0,
                   uint32_t image_mw = 0,
                   uint32_t image_mh = 0,
                   const El::Net::HTTP::MimeTypeMap* mt = 0)
        throw(El::Exception);
      
      ~ThumbGenTask() throw() {}

      virtual const char* type_id() const throw(El::Exception);
    
      virtual void execute() throw(Exception, El::Exception);

      virtual void write_arg(El::BinaryOutStream& bstr) const
        throw(El::Exception);
        
      virtual void read_arg(El::BinaryInStream& bstr) throw(El::Exception);
      virtual void write_res(El::BinaryOutStream& bstr) throw(El::Exception);
        
      virtual void read_res(El::BinaryInStream& bstr) throw(El::Exception);
    };

    typedef El::RefCount::SmartPtr<ThumbGenTask> ThumbGenTask_var;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace RSS
  {
//
// ThumbGenTask class
//
    inline
    ThumbGenTask::Thumb::Thumb(uint32_t w, uint32_t h, uint8_t c)
      throw(El::Exception)
        : width(w),
          height(h),
          crop(c)
    {
    }
    
    inline
    void
    ThumbGenTask::Thumb::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr << width << height << crop << path << type;
    }
    
    inline
    void
    ThumbGenTask::Thumb::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> width >> height >> crop >> path >> type;
    }
    
//
// ThumbGenTask class
//
    inline
    ThumbGenTask::ThumbGenTask(const char* path,
                               uint32_t image_mw,
                               uint32_t image_mh,
                               const El::Net::HTTP::MimeTypeMap* mt)
      throw(El::Exception)
        : El::Service::ProcessPool::TaskBase(false),
          image_path(path ? path : ""),
          image_min_width(image_mw),
          image_min_height(image_mh),
          skip(0),
          image_width(0),
          image_height(0)
    {
      if(mt)
      {
        mime_types = *mt;
      }
    }

    inline
    const char*
    ThumbGenTask::type_id() const throw(El::Exception)
    {
      return "ThumbGenTask";
    }

    inline
    void
    ThumbGenTask::write_arg(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr << image_path << image_min_width << image_min_height;
      bstr.write_map(mime_types);
      bstr.write_array(thumbs);
    }

    inline
    void
    ThumbGenTask::read_arg(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> image_path >> image_min_width >> image_min_height;
      bstr.read_map(mime_types);
      bstr.read_array(thumbs);
    }

    inline
    void
    ThumbGenTask::write_res(El::BinaryOutStream& bstr) throw(El::Exception)
    {
      bstr << skip << issue << image_width << image_height;
      bstr.write_array(thumbs);
    }
        
    inline
    void
    ThumbGenTask::read_res(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> skip >> issue >> image_width >> image_height;
      bstr.read_array(thumbs);
    }
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_FEEDS_RSS_PULLER_THUMBGEN_HPP_
