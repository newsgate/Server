/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/FeedManager/LinkGrabber.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <iostream>
#include <string>
#include <sstream>

#include <El/String/ListParser.hpp>

#include "LinkGrabber.hpp"

namespace NewsGate
{
  namespace Moderation
  {
    void
    LinkGrabber::parse(const char* body,
                       const char* url,
                       const char* charset)
      throw(Exception, El::Exception)
    {
      El::HTML::LightParser parser;

      try
      {
        parser.parse(body,
                     charset,
                     url,
                     El::HTML::LightParser::PF_LAX |
                     El::HTML::LightParser::PF_PARSE_LINKS |
                     El::HTML::LightParser::PF_PARSE_FRAMES);

        El::String::ListParser list_parser(parser.text.c_str());

        const char* item = 0;
        while((item = list_parser.next_item()) != 0)
        {
          if(strncasecmp(item, "http://", 7) == 0)
          {
            normalized_refs.insert(item);
          }
        }
        
      }
      catch(const El::HTML::LightParser::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Moderation::LinkGrabber::parse: "
             << "parsing failed. Reason: " << e;
        
        throw Exception(ostr.str());
      }

      for(El::HTML::LightParser::LinkArray::const_iterator it =
            parser.links.begin(); it != parser.links.end(); it++)
      {
        normalized_refs.insert(it->url.c_str());
      }
        
      for(El::HTML::LightParser::FrameArray::const_iterator it =
            parser.frames.begin(); it != parser.frames.end(); it++)
      {
        normalized_frames.insert(it->url.c_str());
      }
    }
    
  }
}
