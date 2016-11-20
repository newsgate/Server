/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Feeds/RSS/Puller/Task.hpp
 * @author Karen Arutyunov
 * $Id:$
 */


#include <sstream>

#include <Magick++.h>

#include <El/Service/ProcessPool.hpp>
#include <El/String/Manip.hpp>

#include "ThumbGen.hpp"

namespace NewsGate
{
  namespace RSS
  {
    //
    // TaskFactory class
    //
    class TaskFactory : public ::El::Service::ProcessPool::TaskFactoryInterface
    {
    public:
      EL_EXCEPTION(Exception, El::Service::Exception);

    public:
      TaskFactory() throw(Exception, El::Exception);

    private:
      virtual ~TaskFactory() throw() {}
      
      virtual const char* creation_error() throw() { return 0; }
  
      virtual void release() throw(Exception) { delete this; }
  
      virtual ::El::Service::ProcessPool::Task* create_task(const char* id)
        throw(El::Exception);
    };

    TaskFactory::TaskFactory() throw(Exception, El::Exception)
    {
      rlimit limit;
      memset(&limit, 0, sizeof(limit));
      setrlimit(RLIMIT_CORE, &limit);
      
      try 
      {
        Magick::InitializeMagick(0);
      }
      catch(const Magick::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "TaskFactory::TaskFactory: InitializeMagick failed. Reason: "
             << e;
        
        throw Exception(ostr.str());
      }
    }

    ::El::Service::ProcessPool::Task*
    TaskFactory::create_task(const char* id) throw(El::Exception) 
    {
      if(strcmp(id, "ThumbGenTask") == 0)
      {
        return new ThumbGenTask();
      }

      std::ostringstream ostr;
      ostr << "TaskFactory::create_task: unknown task id '" << id << "'";
      
      throw Exception(ostr.str());
    }
    
    //
    // ThumbGenTask class
    //
    
    void
    ThumbGenTask::execute() throw(Exception, El::Exception)
    {
      Magick::Image image;

      try
      {
        Magick::Image image;
        std::string thumb_type_lower;
          
        image.read(image_path.c_str());
            
        image_width = image.baseColumns();
        image_height = image.baseRows();
        
        if(image_height < 1 || image_width < 1)
        {
          std::ostringstream ostr;
          ostr << "zero size image (" << image_width << "x"
               << image_height << ")";
          
          issue = ostr.str();
          skip = 1;
          return;
        }
        
        if(image_height < image_min_height || image_width < image_min_width)
        {
          std::ostringstream ostr;
          ostr << "small image (" << image_width << "x"
               << image_height << "<" << image_min_width
               << "x" << image_min_height << ")";
              
          issue = ostr.str();
          skip = 1;
          return;
        }

        std::string thumb_type = image.magick();

        if(thumb_type.empty())
        {
          issue = "unknown image type";
          skip = 1;
          return;
        }
            
        El::String::Manip::to_lower(thumb_type.c_str(), thumb_type_lower);

        if(mime_types.find(thumb_type_lower) == mime_types.end())
        {
          image.magick("JPEG");
          thumb_type = image.magick();

          if(thumb_type.empty())
          {
            issue = "can't convert to JPEG";
            skip = 1;
            return;
          }

          El::String::Manip::to_lower(thumb_type.c_str(), thumb_type_lower);
        }

        bool image_as_thumbnail = true;
        bool crop_max = true;
        size_t thumb_num = 0;
          
        for(ThumbArray::iterator i(thumbs.begin()), e(thumbs.end());
            i != e; ++i)
        {
          Thumb& thumb = *i;
          
          uint16_t max_width = thumb.width;
          uint16_t max_height = thumb.height;

          if(!max_width || !max_height)
          {
            continue;
          }
            
          bool crop = thumb.crop;
          thumb.type = thumb_type_lower;

          try
          {
            if(image_height <= max_height && image_width <= max_width)
            {
              if(!crop ||
                 (image_height == max_height && image_width == max_width))
              {    
                if(image_as_thumbnail)
                {
                  image.write(image_path.c_str());
                  
                  thumb.height = image_height;
                  thumb.width = image_width;
                  thumb.crop = 0;
                  thumb.path = image_path;
                  
                  image_as_thumbnail = false;
                }
              
                continue;
              }
              else            
              {
                if(crop_max)
                {
                  if(max_width > max_height)
                  {
                    max_height =
                      std::min((uint16_t)(((double)image_width /  max_width) *
                                          max_height + 0.5),
                               (uint16_t)image_height);
                      
                    max_width = image_width;
                  }
                  else
                  {
                    max_width =
                      std::min((uint16_t)(((double)image_height / max_height) *
                                          max_width + 0.5),
                               (uint16_t)image_width);
                      
                    max_height = image_height;
                  }

                  if(!max_width || !max_height)
                  {
                    continue;
                  }
                    
                  crop_max = false;
                }
                else
                {
                  continue;
                }
              }
            }
              
            Magick::Image copy(image);

            double aspect = (double)max_width / max_height;
            double img_aspect = (double)image_width / image_height;

            if(crop)
            {
              if(image_height < max_height)
              {
                copy.crop(Magick::Geometry(max_width,
                                           image_height,
                                           (image_width - max_width) / 2,
                                           0));
              }
              else if(image_width < max_width)
              {
                copy.crop(Magick::Geometry(image_width,
                                           max_height,
                                           0,
                                           (image_height - max_height) / 2));
              }
              else
              {
                if(img_aspect <= aspect)
                {
                  unsigned short height =
                    (unsigned short)((double)image_width / aspect);

                  copy.crop(Magick::Geometry(image_width,
                                             height,
                                             0,
                                             (image_height - height) / 2));
                }
                else
                {
                  unsigned short width =
                    (unsigned short)((double)image_height * aspect);
                
                  copy.crop(Magick::Geometry(width,
                                             image_height,
                                             (image_width - width) / 2,
                                             0));
                }

                copy.scale(Magick::Geometry(max_width, max_height));
              }
            }
            else
            {
              unsigned long width = 0;
              unsigned long height = 0;

              if(aspect <= img_aspect)
              {
                // Shrink image width to max_width, height calc accordingly
                width = max_width;
                height = (unsigned short)((double)image_width / img_aspect);
              }
              else
              {
                // Shrink image height to max_height, width calc accordingly
              
                height = max_height;
                width = (unsigned short)(img_aspect * image_height);
              }

              copy.scale(Magick::Geometry(width, height));
            }
            
            {
              std::ostringstream ostr;
              ostr << image_path << "." << thumb_num++;
              thumb.path = ostr.str();
            }
            
            
            copy.write(thumb.path.c_str());

            //
            // Get exact dimensions of resulted thumbnail
            //
            copy.read(thumb.path.c_str());              
            thumb.width = copy.baseColumns();
            thumb.height = copy.baseRows();
          }
          catch(const Magick::Exception& e)
          {
            std::ostringstream ostr;
            ostr << "ImageMagick operation failed for thumb " << thumb.width
                 << "x" << thumb.height << (crop ? "C" : "") << e << "; ";

            issue += ostr.str();
          }
        }        
      }
      catch(const Magick::Exception& e)
      {
        std::ostringstream ostr;        
        ostr << "ThumbGenTask::execute: Magick::Exception "
          "caught while processing image. Description: " << e;

        throw Exception(ostr.str());
      }
      catch(...)
      {
        std::ostringstream ostr;
        
        ostr << "ThumbGenTask::execute: unknown exception "
          "caught while reading image " << image_path;

        throw Exception(ostr.str());
      }      
    }
  }
}

extern "C"
::El::Service::ProcessPool::TaskFactoryInterface*
create_task_factory(const char* args)
{
  return new NewsGate::RSS::TaskFactory();
}
