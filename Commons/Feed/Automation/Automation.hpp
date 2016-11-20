/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file Commons/Feed/Automation/Automation.hpp
 * @author Karen Arutyunov
 * $id:$
 */

#ifndef _NEWSGATE_SERVER_COMMONS_FEED_AUTOMATION_AUTOMATION_HPP_
#define _NEWSGATE_SERVER_COMMONS_FEED_AUTOMATION_AUTOMATION_HPP_

#include <stdint.h>

#include <string>
#include <vector>

#include <El/Exception.hpp>
#include <El/String/Manip.hpp>
#include <El/BinaryStream.hpp>

#include <El/Python/Object.hpp>
#include <El/Net/HTTP/Session.hpp>
#include <El/Net/HTTP/Utility.hpp>

#include <Commons/Feed/Automation/Article.hpp>

namespace NewsGate
{
  namespace Feed
  {
    namespace Automation
    {
      EL_EXCEPTION(Exception, El::ExceptionBase);

      typedef std::auto_ptr<El::Net::HTTP::Session> HTTPSessionPtr;
      
      struct Item
      {
        std::string title;
        std::string description;
        HTTPSessionPtr article_session;
        unsigned long article_max_size;
        std::string cache_dir;
        unsigned long long feed_id;
        std::string encoding;

        Item() throw();        
        Item(const Item& item) throw(El::Exception);
        
        Item& operator=(const Item& item) throw(El::Exception);
        
        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);
      };

      namespace Python
      {
        class Item : public El::Python::ObjectImpl
        {
        public:
          Item(PyTypeObject *type = 0, PyObject *args = 0, PyObject *kwds = 0)
            throw(El::Exception);

          Item(::NewsGate::Feed::Automation::Item& item,
               const El::Net::HTTP::RequestParams& req_params,
               bool read_art)
            throw(Exception, El::Exception);
          
          virtual ~Item() throw() {}

          virtual void write(El::BinaryOutStream& bstr)
            const throw(El::Exception);
          
          virtual void read(El::BinaryInStream& bstr)
            throw(El::Exception);          

          void save(::NewsGate::Feed::Automation::Item& item) const
            throw(El::Exception);

          PyObject* article_doc(const char* encoding) throw(El::Exception);
          
          PyObject* py_article_doc(PyObject* args) throw(El::Exception);
          PyObject* py_description_doc() throw(El::Exception);
          
          class Type :
            public El::Python::ObjectTypeImpl<Item, Item::Type>
          {
          public:
            Type() throw(El::Python::Exception, El::Exception);
            static Type instance;

            PY_TYPE_MEMBER_STRING(title,
                                  "title",
                                  "Item html title",
                                  true);

            PY_TYPE_MEMBER_STRING(description,
                                  "description",
                                  "Item html description",
                                  true);

            PY_TYPE_MEMBER_ULONGLONG(feed_id,
                                     "feed_id",
                                     "Item feed identifier");

            PY_TYPE_METHOD_VARARGS(
              py_article_doc,
              "article_doc",
              "Returns article XML document object");
            
            PY_TYPE_METHOD_NOARGS(
              py_description_doc,
              "description_doc",
              "Returns description XML document object");
          };
    
          std::string title;
          std::string description;
          Article article;
          
          El::Python::Object_var description_doc;
          unsigned long long feed_id;
          bool article_requested;
        };
      
        typedef El::Python::SmartPtr<Item> Item_var;
      
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Feed
  {
    namespace Automation
    {
      //
      // Item struct
      //
      inline
      Item::Item() throw()
          : article_max_size(0),
            feed_id(0)
      {
      }

      inline
      Item::Item(const Item& item) throw(El::Exception)
      {
        *this = item;
      }
        
      inline
      Item&
      Item::operator=(const Item& item) throw(El::Exception)
      {
        title = item.title;
        description = item.description;
        article_max_size = item.article_max_size;
        cache_dir = item.cache_dir;
        feed_id = item.feed_id;
        encoding = item.encoding;

        article_session.reset(0);
        return *this;
      }
      
      inline
      void
      Item::write(El::BinaryOutStream& ostr) const throw(El::Exception)
      {
        ostr << title << description << (uint64_t)feed_id << encoding;
      }

      inline
      void
      Item::read(El::BinaryInStream& istr) throw(El::Exception)
      {
        uint64_t fi = 0;
        istr >> title >> description >> fi >> encoding;
        
        feed_id = fi;
        article_max_size = 0;
        cache_dir.clear();
        article_session.reset(0);
      }

      namespace Python
      {
        //
        // Python::Item class
        //
        inline
        Item::Item(PyTypeObject *type, PyObject *args, PyObject *kwds)
          throw(El::Exception) :
            El::Python::ObjectImpl(type ? type : &Type::instance),
            feed_id(0)
        {
        }

        //
        // Item::Type class
        //
        inline
        Item::Type::Type() throw(El::Python::Exception, El::Exception)
            : El::Python::ObjectTypeImpl<Item, Item::Type>(
              "newsgate.feed.Item",
              "Object representing feed item properties")
        {
        }
      
      }
    }
  }
}

#endif // _NEWSGATE_SERVER_COMMONS_FEED_AUTOMATION_AUTOMATION_HPP_
