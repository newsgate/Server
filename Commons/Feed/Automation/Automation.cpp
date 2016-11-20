/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file Commons/Feed/Automation/Automation.cpp
 * @author Karen Arutyunov
 * $id:$
 */

#include <Python.h>

#include <string>

#include <El/Exception.hpp>
#include <El/Guid.hpp>

#include <El/Python/RefCount.hpp>
#include <El/Python/Object.hpp>
#include <El/Python/Module.hpp>
#include <El/Python/Utility.hpp>

#include <El/LibXML/Python/HTMLParser.hpp>

#include <El/Net/HTTP/StatusCodes.hpp>
#include <El/Net/HTTP/Headers.hpp>

#include <Commons/Feed/Types.hpp>

#include "Automation.hpp"

namespace NewsGate
{
  namespace Feed
  {
    namespace Automation
    {
      namespace Python
      {
        class FeedAutomationPyModule :
          public El::Python::ModuleImpl<FeedAutomationPyModule>
        {
        public:
          static FeedAutomationPyModule instance;

          FeedAutomationPyModule() throw(El::Exception);
          virtual void initialized() throw(El::Exception);
        };  
  
        FeedAutomationPyModule::FeedAutomationPyModule()
          throw(El::Exception)
            : El::Python::ModuleImpl<FeedAutomationPyModule>(
              "newsgate.feed",
              "Module containing feed related types.",
              true)
        {
        }

        void
        FeedAutomationPyModule::initialized() throw(El::Exception)
        {
          add_member(PyLong_FromLong(SP_UNDEFINED), "SP_UNDEFINED");
          add_member(PyLong_FromLong(SP_NEWS), "SP_NEWS");
          add_member(PyLong_FromLong(SP_TALK), "SP_TALK");
          add_member(PyLong_FromLong(SP_AD), "SP_AD");
          add_member(PyLong_FromLong(SP_BLOG), "SP_BLOG");
          add_member(PyLong_FromLong(SP_ARTICLE), "SP_ARTICLE");
          add_member(PyLong_FromLong(SP_PHOTO), "SP_PHOTO");
          add_member(PyLong_FromLong(SP_VIDEO), "SP_VIDEO");
          add_member(PyLong_FromLong(SP_AUDIO), "SP_AUDIO");
          add_member(PyLong_FromLong(SP_PRINTED), "SP_PRINTED");
        }

        FeedAutomationPyModule FeedAutomationPyModule::instance;
        Item::Type Item::Type::instance;

        //
        // Item struct
        //
        Item::Item(::NewsGate::Feed::Automation::Item& item,
                   const El::Net::HTTP::RequestParams& req_params,
                   bool read_art)
          throw(Exception, El::Exception)
            : El::Python::ObjectImpl(&Type::instance),
              title(item.title),
              description(item.description),
              article(0,
                      req_params,
                      item.article_max_size,
                      item.cache_dir.c_str(),
                      item.article_session.release(),
                      read_art,
                      0,
                      item.encoding.c_str()),
              feed_id(item.feed_id),
              article_requested(false)
        {
        }

        void
        Item::write(El::BinaryOutStream& bstr) const throw(El::Exception)
        {
          bstr << title << description << article
               << (uint64_t)feed_id << (uint8_t)article_requested;
        }
          
        void
        Item::read(El::BinaryInStream& bstr) throw(El::Exception)
        {
          uint64_t fi = 0;
          uint8_t rr = 0;
          
          bstr >> title >> description >> article >> fi >> rr;

          feed_id = fi;
          article_requested = rr;
        }
        
        void
        Item::save(::NewsGate::Feed::Automation::Item& item) const
          throw(El::Exception)
        {
          El::String::Manip::compact(title.c_str(), item.title, true);
          
          El::String::Manip::compact(description.c_str(),
                                     item.description,
                                     true);
          
          item.feed_id = feed_id;
        }

        PyObject*
        Item::py_description_doc() throw(El::Exception)
        {
          if(description_doc.in() == 0)
          {
            El::LibXML::ErrorRecorderHandler error_handler;
                
            El::LibXML::Python::HTMLParser_var parser =
              new El::LibXML::Python::HTMLParser();

            description_doc = parser->parse(description.c_str(),
                                            description.length(),
                                            "",
                                            "UTF-8",
                                            &error_handler,
                                            HTML_PARSE_NONET);
          }
          
          return El::Python::add_ref(description_doc.in());
        }        

        PyObject*
        Item::article_doc(const char* encoding) throw(El::Exception)
        {
          article_requested = true;

          if(encoding && *encoding != '\0')
          {
            article.encoding(encoding);
          }
          
          return article.document();
        }
        
        PyObject*
        Item::py_article_doc(PyObject* args) throw(El::Exception)
        {
          const char* encoding = 0;
          
          if(!PyArg_ParseTuple(args,
                               "|s:newsgate.feed.Item.article_doc",
                               &encoding))
          {
            El::Python::handle_error(
              "NewsGate::Feed::Automation::Python::Item::py_article_doc");
          }
          
          return article_doc(encoding);
        }
      }
    }
  }
}
