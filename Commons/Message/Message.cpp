/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Server/Commons/Message/Message.cpp
 * @author Karen Arutyunov
 * $Id:$
 */
#include <El/CORBA/Corba.hpp>

#include <unistd.h>
#include <sys/stat.h>

#include <sstream>
#include <fstream>

#include <El/Exception.hpp>
#include <El/CRC.hpp>

#include "Message.hpp"

namespace NewsGate
{
  namespace Message
  {
    const Id Id::zero;
    const Id Id::nonexistent(UINT64_MAX);

    void
    ImageThumb::read_image(const char* path, bool calc_crc)
      throw(Exception, El::Exception)
    {
      struct stat64 stat;
      memset(&stat, 0, sizeof(stat));
      
      if(stat64(path, &stat) != 0)
      {
        int error = ACE_OS::last_error();

        std::ostringstream ostr;
        ostr << "NewsGate::Message::ImageThumb::read_image: stat64 failed "
          "for '"  << path << "'. Reason: code " << error << ", desc. "
             << ACE_OS::strerror(error);
        
        throw Exception(ostr.str());
      }
      
      if(!stat.st_size)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::ImageThumb::read_image: unexpected "
          "size for '" << path << "'";
        
        throw Exception(ostr.str());
      }

      std::fstream file(path, ios::in);
        
      if(!file.is_open())
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::ImageThumb::read_image: failed to open '"
             << path << "' for read access";
        
        throw Exception(ostr.str());
      }

      size_t size = stat.st_size;

      ImagePtr buff(new unsigned char[size]);
      file.read((char*)buff.get(), size * sizeof(buff.get()[0]));

      if(calc_crc)
      {
        hash = 0; 
      }

      if(file.fail())
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::ImageThumb::read_image: failed to read "
          "from '" << path << "'";
        
        throw Exception(ostr.str());
      }

      image.reset(buff.release());
      length = size;

      if(calc_crc)
      {
        El::CRC(hash, image.get(), length);
        El::CRC(hash, (unsigned char *)&length, sizeof(length));
      }
    }

    void
    ImageThumb::read_image(El::BinaryInStream bstr, bool calc_crc)
      throw(El::Exception)
    {
      unsigned char* buff = 0;
      size_t len = 0;
      
      bstr.read_bytes(buff, len);
      
      image.reset(buff);
      length = len;

      if(calc_crc)
      {
        El::CRC(hash, image.get(), length);
        El::CRC(hash, (unsigned char *)&length, sizeof(length));
      }      
    }
    
    void
    ImageThumb::write_image(const char* path) const
      throw(Exception, El::Exception)
    {
      std::fstream file(path, ios::out);
        
      if(!file.is_open())
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::ImageThumb::write_image: failed to open '"
             << path << "' for write access";
        
        throw Exception(ostr.str());
      }

      file.write((char*)image.get(), length * sizeof(image.get()[0]));
      
      if(file.fail())
      {
        file.close();
        unlink(path);      
        
        std::ostringstream ostr;
        ostr << "NewsGate::Message::ImageThumb::write_image: failed to write "
          "to '" << path << "'";
        
        throw Exception(ostr.str());
      }
    }
  }
}

