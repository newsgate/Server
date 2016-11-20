/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   RSSParserMain.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <locale.h>

#include <sstream>
#include <iostream>
#include <list>
#include <fstream>

#include <El/Exception.hpp>
#include <El/Moment.hpp>

#include <El/Net/HTTP/Session.hpp>
#include <El/Net/HTTP/StatusCodes.hpp>
#include <El/Net/HTTP/Headers.hpp>

#include <El/HTML/LightParser.hpp>

#include <Commons/Feed/Types.hpp>

#include <xsd/DataFeed/RSS/Data.hpp>
#include <xsd/DataFeed/RSS/ParserFactory.hpp>

EL_EXCEPTION(Exception, El::ExceptionBase);

namespace
{
  const char USAGE[] =
  "Usage: RSSParserTest [--dump] [--unparsed] ( [--type=(rss|atom|rdf)] <feed_file> )+";
}

using namespace NewsGate;

int
main(int argc, char** argv)
{
  try
  {
    if (!::setlocale(LC_CTYPE, "en_US.utf8"))
    {
      throw Exception("cannot set locale.");
    }

    // Checking params
    if (argc < 2)
    {
      std::ostringstream ostr;
      ostr << "rss file is not specified\n" << USAGE;
      throw Exception(ostr.str());
    }

    Feed::Type type = Feed::TP_RSS;
    bool dump = false;
    bool unparsed = false;
      
    for(int i = 1; i < argc; i++)
    {
      const char* arg = argv[i];
      
      if(!strncmp(arg, "--type=", 7))
      {
        const char* stype = arg + 7;

        if(!strcasecmp(stype, "rss"))
        {
          type = Feed::TP_RSS;
        }
        else if(!strcasecmp(stype, "atom"))
        {
          type = Feed::TP_ATOM;
        }
        else if(!strcasecmp(stype, "rdf"))
        {
          type = Feed::TP_RDF;
        }
        else
        {
          std::ostringstream ostr;
          ostr << "unexpected feed type " << stype;
          throw Exception(ostr.str());
        }

        continue;
      }
      else if(!strcmp(arg, "--dump"))
      {
        dump = true;
        continue;
      }
      else if(!strcmp(arg, "--unparsed"))
      {
        unparsed = true;
        continue;
      }
      
      std::cerr << "Pulling " << arg << " ...\n";

      RSS::ParserPtr parser(RSS::ParserFactory::create(type, 0));

      try
      {
//        parser->parse_file(argv[i]);

        const char* file = argv[i];
        const char* url = 0;

        ACE_Time_Value timeout(120);
        
        std::auto_ptr<El::XML::EntityResolver> er;

        er.reset(new El::XML::EntityResolver(
          El::XML::EntityResolver::NetStrategy(&timeout,
                                               &timeout,
                                               &timeout,
                                               2),
          El::XML::EntityResolver::FileStrategy("/tmp/ER",
                                                180,
                                                60,
                                                1024 * 1024 * 10,
                                                60,
                                                600)));
        
        if(strncmp(file, "http://", 7) == 0)
        {
          url = file;
          
          El::Net::HTTP::Session session(file);

          El::Net::HTTP::HeaderList headers;
          headers.add(El::Net::HTTP::HD_ACCEPT,
                      "text/xml,application/xml,application/atom+xml,"
                      "application/rss+xml,application/rdf+xml,*/*;q=0.1");

          headers.add(El::Net::HTTP::HD_ACCEPT_ENCODING, "gzip,deflate");
          headers.add(El::Net::HTTP::HD_ACCEPT_LANGUAGE, "en-us,*");
      
          headers.add(
            El::Net::HTTP::HD_USER_AGENT,
            "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; "
            ".NET CLR 1.1.4322)");

          session.open(&timeout, &timeout, &timeout, 1024, 128);

          session.send_request(El::Net::HTTP::GET,
                               El::Net::HTTP::ParamList(),
                               headers);

          session.recv_response_status();
      
          if(session.status_code() != El::Net::HTTP::SC_OK)
          {
            std::ostringstream ostr;
            ostr << "bad status for " << file << ": "
                 << session.status_code() << ", " << session.status_text();
            
            throw Exception(ostr.str());
          }

          El::Net::HTTP::Header header;              
          while(session.recv_response_header(header));
          
          parser->parse(session.response_body(),
                        session.url()->string(),
                        0,
                        128,
                        false,
                        er.get());
          
          session.test_completion();
        }
        else
        {
          std::fstream istr(file, std::ios::in);
          
          parser->parse(istr,
                        file,
                        0,
                        128,
                        false,
                        er.get());
        }
        
        const RSS::Channel& channel = parser->rss_channel();
        El::HTML::LightParser parser;

        std::wstring val;
        El::String::Manip::utf8_to_wchar(channel.title.c_str(), val);
            
        parser.parse(val.c_str(),
                     url,
                     El::HTML::LightParser::PF_LAX,
                     10 * 1024);

        if(dump)
        {
          std::cerr << std::endl << "Channel title: " << parser.text
                    << std::endl;
        }

        if(dump)
        {
          std::cerr << "Channel lastBuildDate: "
                    << channel.last_build_date.rfc0822()
                    << std::endl;
        }
        
        for(RSS::ItemList::const_iterator it = channel.items.begin();
            it != channel.items.end(); it++)
        {
          const RSS::Item& message = *it;

          if(dump)
          {
            std::cerr
              << "Item guid: permalink=" << message.guid.is_permalink
              << ", val='" << message.guid.value << "'\n"
              << "Item pubDate: " << message.code.pub_date().rfc0822()
              << std::endl;
          }

          std::wstring title;
          El::String::Manip::utf8_to_wchar(message.title.c_str(), title);
          
          parser.parse(title.c_str(),
                       url,
                       El::HTML::LightParser::PF_LAX,
                       10 * 1024);

          if(dump)
          {
            std::cerr << "Item title: " <<
              (unparsed ? message.title : parser.text) << std::endl;
          }
          
          std::wstring desc;
          El::String::Manip::utf8_to_wchar(message.description.c_str(), desc);
          
          parser.parse(desc.c_str(),
                       url,
                       El::HTML::LightParser::PF_LAX |
                       El::HTML::LightParser::PF_PARSE_IMAGES,
                       10 * 1024);
          
          if(dump)
          {
            std::cerr << "Item desc: " <<
              (unparsed ? message.description : parser.text) << std::endl;

            if(!unparsed && !parser.images.empty())
            {
              std::cerr << "Item images:\n";
              
              for(El::HTML::LightParser::ImageArray::const_iterator
                    it = parser.images.begin(); it != parser.images.end();
                  it++)
              {
                std::cerr << "  image: size=" << it->width << "x" << it->height
                          << " url=" << it->src << " alt=" << it->alt
                          << std::endl;
              }
              
            }
          }
        }
        
        std::cerr << "Success\n";
      }
      catch(const El::Exception& e)
      {
        std::cerr << "Failed to parse: " << e << std::endl;
      }
    }
    
    return 0;
  }
  catch (const El::Exception& e)
  {
    std::cerr << e.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << "unknown exception caught.\n";
  }
  
  return -1;
}
