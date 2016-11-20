/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/FeedManager/LinkGrabber.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MODERATOR_FEEDMANAGER_LINKGRABBER_HPP_
#define _NEWSGATE_SERVER_SERVICES_MODERATOR_FEEDMANAGER_LINKGRABBER_HPP_

#include <string>
#include <ext/hash_set>

#include <El/Exception.hpp>
#include <El/Hash/Hash.hpp>

#include <El/HTML/LightParser.hpp>

namespace NewsGate
{
  namespace Moderation
  {
    class LinkGrabber
    {
    public:      
      EL_EXCEPTION(Exception, El::ExceptionBase);
      
    public:
      typedef __gnu_cxx::hash_set<std::string, El::Hash::String> StringSet;
      
      StringSet normalized_refs;
      StringSet normalized_frames;

      void parse(const char* body, const char* url, const char* charset)
        throw(Exception, El::Exception);

    private:
      El::HTML::LightParser parser;
    };
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_MODERATOR_FEEDMANAGER_LINKGRABBER_HPP_
