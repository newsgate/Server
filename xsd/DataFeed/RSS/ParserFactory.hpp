/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   xsd/DataFeed/RSS/ParserFactory.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

//#define RSS_PARSER_TRACE

#ifndef _NEWSGATE_SERVER_XSD_DATAFEED_RSS_PARSERFACTORY_HPP_
#define _NEWSGATE_SERVER_XSD_DATAFEED_RSS_PARSERFACTORY_HPP_

#include <sstream>

#include <Commons/Feed/Types.hpp>

#include <xsd/DataFeed/RSS/Parser.hpp>
#include <xsd/DataFeed/RSS/AtomParser.hpp>
#include <xsd/DataFeed/RSS/RDFParser.hpp>
#include <xsd/DataFeed/RSS/RSSParser.hpp>
#include <xsd/DataFeed/RSS/UniParser.hpp>

namespace NewsGate
{ 
  namespace RSS
  {
    class ParserFactory
    {
    public:
      EL_EXCEPTION(Exception, RSS::Exception);

    public:
      static Parser* create(NewsGate::Feed::Type feed_type,
                            Parser::Interceptor* interceptor
/*,
  bool new_parser = false*/)
        throw(Exception, El::Exception);
    };
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{ 
  namespace RSS
  {
    inline
    Parser*
    ParserFactory::create(NewsGate::Feed::Type feed_type,
                          Parser::Interceptor* interceptor/*,
                                                            bool new_parser*/)
      throw(Exception, El::Exception)
    {
      switch(feed_type)
      {
      case NewsGate::Feed::TP_UNDEFINED:
        {
          return new UniParser(interceptor);
        }
      case NewsGate::Feed::TP_RSS:
        {
          return new RSSParser(interceptor);
        }
      case NewsGate::Feed::TP_ATOM:
        {
          return new AtomParser(interceptor);
        }
      case NewsGate::Feed::TP_RDF:
        {
          return new RDFParser(interceptor);
        }
      default:
        {
          break;
        }
      }

      std::ostringstream ostr;
      ostr << "NewsGate::RSS::ParserFactory::create: unexpected feed type "
           << feed_type;
          
      throw Exception(ostr.str());      
    }
  }
}

#endif // _NEWSGATE_SERVER_XSD_DATAFEED_RSS_PARSERFACTORY_HPP_
