/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Commons/FraudPrevention/LimitCheck.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <sstream>

#include <El/Exception.hpp>
#include <El/String/Manip.hpp>
#include <El/String/ListParser.hpp>

#include "LimitCheck.hpp"

namespace NewsGate
{
  namespace FraudPrevention
  {
    EventFreq EventFreq::null;

    void
    EventLimitCheckDescArray::add_check_descriptors(
      FraudPrevention::EventType type,
      bool user,
      bool ip,
      bool item,
      const char* freqs)
      throw(Exception, El::Exception)
    {
      FraudPrevention::EventLimitCheckDesc desc;
    
      desc.type = type;
      desc.user = user;
      desc.ip = ip;
      desc.item = item;
      
      El::String::ListParser parser(freqs);

      const char* it = 0;
    
      while((it = parser.next_item()) != 0)
      {
        const char* sep = strchr(it, '/');

        if(sep == 0)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::FraudPrevention::EventLimitCheckDescArray::"
            "add_check_descriptors: invalid fraud prevention configuration "
            "line format '" << freqs << "'";

          throw Exception(ostr.str());
        }

        if(!El::String::Manip::numeric(std::string(it, sep - it).c_str(),
                                       desc.times) ||
           !desc.times)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::FraudPrevention::EventLimitCheckDescArray::"
            "add_check_descriptors: invalid fraud prevention configuration "
            "line format '" << freqs << "'";

          throw Exception(ostr.str());
        }

        if(!El::String::Manip::numeric(sep + 1, desc.interval) ||
           !desc.interval)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::FraudPrevention::EventLimitCheckDescArray::"
            "add_check_descriptors: invalid fraud prevention configuration "
            "line format '" << freqs << "'";

          throw Exception(ostr.str());
        }

        push_back(desc);
      }
    }
  }
}
