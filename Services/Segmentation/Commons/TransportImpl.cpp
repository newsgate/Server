/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Segmentation/Commons/TransportImpl.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <El/CORBA/Corba.hpp>

#include <string>
#include <sstream>

#include <El/Exception.hpp>
#include <El/String/Manip.hpp>

#include "TransportImpl.hpp"

namespace NewsGate
{
  namespace Segmentation
  {
    namespace Transport
    {      
      void
      register_valuetype_factories(CORBA::ORB* orb)
        throw(CORBA::Exception, El::Exception)
      {
        CORBA::ValueFactoryBase_var factory =
          new NewsGate::Segmentation::Transport::MessagePackImpl::Init();
      
        CORBA::ValueFactoryBase_var old = orb->register_value_factory(
          "IDL:NewsGate/Segmentation/Transport/MessagePack:1.0",
          factory);

        factory = new NewsGate::Segmentation::Transport::
          SegmentedMessagePackImpl::Init();
      
        old = orb->register_value_factory(
          "IDL:NewsGate/Segmentation/Transport/SegmentedMessagePack:1.0",
          factory);
      }

      void
      Text::set(const char* result, const char* src)
        throw(InvalidArgument, Exception, El::Exception)
      {
        inserted_spaces.clear();

        std::wstring wsrc;
        El::String::Manip::utf8_to_wchar(src, wsrc);
        const wchar_t* wsrc_s = wsrc.c_str();
        
        std::wstring wres;
        El::String::Manip::utf8_to_wchar(result, wres);
        const wchar_t* wres_s = wres.c_str();

        const wchar_t* s = wsrc_s;
        const wchar_t* r = wres_s;
        
        for(; *s != L'\0' && *r != L'\0'; r++)
        {
          if(*r != *s)
          {
            if(*r == L' ')
            {
              inserted_spaces.insert(r - wres_s);
              continue;
            }
            else
            {
              std::ostringstream ostr;
              ostr << "NewsGate::Segmentation::Transport::Text::set: "
                "unexpected wide character '";

              El::String::Manip::wchar_to_utf8(*r, ostr);

              ostr << "' (0x" << std::hex << *r << ") at position "
                   << std::dec << (r - wres_s) <<
                " in segmented text:\n" << result << "\nsource text:\n"
                   << src;

              throw InvalidArgument(ostr.str());
            }
          }

          s++;
        }

        if(*s != L'\0')
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Segmentation::Transport::Text::set: "
            "unexpected end of segmented text:\n" << result
               << "\nsource text:\n" << src;

          throw InvalidArgument(ostr.str());
        }

        value = result;
      }
    }
  }
}
