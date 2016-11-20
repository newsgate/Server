/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Commons/Message/FetchFilter.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_COMMONS_MESSAGE_FETCHFILTER_HPP_
#define _NEWSGATE_SERVER_COMMONS_MESSAGE_FETCHFILTER_HPP_

#include <stdint.h>

#include <string>
#include <vector>

#include <El/Exception.hpp>
#include <El/String/LightString.hpp>
#include <El/BinaryStream.hpp>

namespace NewsGate
{ 
  namespace Message
  {
    struct FetchFilter
    {      
      typedef std::vector<std::string> ExpressionArray;

      std::string source;
      uint64_t stamp;
      uint8_t enforced;
      ExpressionArray expressions;

      FetchFilter() throw(El::Exception) : stamp(0), enforced(0) {}
      
      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };  
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{ 
  namespace Message
  {
    //
    // FetchFilter struct
    //
    inline
    void
    FetchFilter::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << source << stamp << enforced;
      bstr.write_array(expressions);
    }

    inline
    void
    FetchFilter::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> source >> stamp >> enforced;
      bstr.read_array(expressions);
    }
  }
}

#endif // _NEWSGATE_SERVER_COMMONS_MESSAGE_FETCHFILTER_HPP_
