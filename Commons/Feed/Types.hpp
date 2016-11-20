/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   Commons/Feed/Types.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_COMMONS_FEED_TYPES_HPP_
#define _NEWSGATE_SERVER_COMMONS_FEED_TYPES_HPP_

#include <stdint.h>

#include <string>
#include <sstream>

#include <El/Exception.hpp>
#include <El/BinaryStream.hpp>

namespace NewsGate
{
  namespace Feed
  {
    EL_EXCEPTION(Exception, El::ExceptionBase);
    EL_EXCEPTION(InvalidArg, Exception);
    
    enum Type
    {
      TP_UNDEFINED,
      TP_RSS,
      TP_ATOM,
      TP_RDF,
      TP_HTML,
      TP_TYPES_COUNT
    };

    enum Space
    {
      SP_UNDEFINED,
      SP_NEWS,
      SP_TALK,
      SP_AD,
      SP_BLOG,
      SP_ARTICLE,
      SP_PHOTO,
      SP_VIDEO,
      SP_AUDIO,
      SP_PRINTED,
      SP_SPACES_COUNT,
      SP_NONEXISTENT
    };

    enum Status
    {
      ST_ENABLED,
      ST_DISABLED,
      ST_PENDING,
      ST_DELETED,
      ST_STATUSES_COUNT
    };

    typedef uint64_t Id;

    extern Space space(const char* value) throw();
    extern const char* space(NewsGate::Feed::Space value) throw();

    Type type(unsigned long val) throw(InvalidArg, El::Exception);
    Space space(unsigned long val) throw(InvalidArg, El::Exception);
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

inline
El::BinaryOutStream&
operator<<(El::BinaryOutStream& ostr, const NewsGate::Feed::Type& type)
  throw(El::BinaryOutStream::Exception, El::Exception)
{
  ostr << (uint32_t)type;
  return ostr;
}

inline
El::BinaryInStream&
operator>>(El::BinaryInStream& istr,
           NewsGate::Feed::Type& type)
  throw(El::BinaryInStream::Exception, El::Exception)
{  
  uint32_t tp = 0;
  istr >> tp;

  if(tp >= NewsGate::Feed::TP_TYPES_COUNT)
  {
    std::ostringstream ostr;
    ostr << "operator>>(NewsGate::Feed::Type): unexpected type " << tp;
    
    throw El::BinaryInStream::Exception(ostr.str());
  }

  type = (NewsGate::Feed::Type)tp;
  return istr;
}

inline
El::BinaryOutStream&
operator<<(El::BinaryOutStream& ostr, const NewsGate::Feed::Space& space)
  throw(El::BinaryOutStream::Exception, El::Exception)
{
  ostr << (uint32_t)space;
  return ostr;
}

inline
El::BinaryInStream&
operator>>(El::BinaryInStream& istr, NewsGate::Feed::Space& space)
  throw(El::BinaryInStream::Exception, El::Exception)
{  
  uint32_t sp = 0;
  istr >> sp;

  if(sp >= NewsGate::Feed::SP_SPACES_COUNT)
  {
    std::ostringstream ostr;
    ostr << "operator>>(NewsGate::Feed::Space): unexpected space " << sp;
    
    throw El::BinaryInStream::Exception(ostr.str());
  }

  space = (NewsGate::Feed::Space)sp;
  return istr;
}

namespace NewsGate
{
  namespace Feed
  {
    inline
    Type
    type(unsigned long val) throw(InvalidArg, El::Exception)
    {
      bool valid = val >= TP_UNDEFINED && val < TP_TYPES_COUNT;

      if(!valid)
      {
        std::ostringstream ostr;
        ostr << "::NewsGate::Feed::type: invalid space " << val;
        throw InvalidArg(ostr.str());
      }

      return (Type)val;
    }

    inline
    Space
    space(unsigned long val) throw(InvalidArg, El::Exception)
    {
      bool valid = val >= SP_UNDEFINED && val < SP_SPACES_COUNT;

      if(!valid)
      {
        std::ostringstream ostr;
        ostr << "::NewsGate::Feed::space: invalid space " << val;
        throw InvalidArg(ostr.str());
      }

      return (Space)val;
    }
  }
}


#endif // _NEWSGATE_SERVER_COMMONS_FEED_TYPES_HPP_
