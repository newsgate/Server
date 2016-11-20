/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file xsd/DataFeed/RSS/MsgAdjustment.hpp
 * @author Karen Arutyunov
 * $id:$
 */

#ifndef _NEWSGATE_SERVER_XSD_DATAFEED_RSS_MSGADJUSTMENT_HPP_
#define _NEWSGATE_SERVER_XSD_DATAFEED_RSS_MSGADJUSTMENT_HPP_

#include <stdint.h>

#include <string>
#include <vector>

#include <El/Exception.hpp>
#include <El/String/Manip.hpp>
#include <El/Logging/Logger.hpp>
#include <El/Lang.hpp>
#include <El/Country.hpp>
#include <El/BinaryStream.hpp>
#include <El/Geography/AddressInfo.hpp>

#include <El/Python/Object.hpp>
#include <El/Python/Sequence.hpp>
#include <El/Python/Lang.hpp>
#include <El/Python/Country.hpp>
#include <El/Python/Sandbox.hpp>
#include <El/Python/Code.hpp>
#include <El/Python/SandboxService.hpp>

#include <El/HTML/LightParser.hpp>
#include <El/Net/HTTP/Session.hpp>
#include <El/LibXML/Python/HTMLParser.hpp>

#include <Commons/Feed/Types.hpp>
#include <Commons/Feed/Automation/Automation.hpp>
#include <Commons/Message/Automation/Automation.hpp>

#include <xsd/DataFeed/RSS/Data.hpp>

namespace NewsGate
{
  namespace RSS
  {
    namespace MsgAdjustment
    {
      EL_EXCEPTION(Exception, El::ExceptionBase);

      class Context
      {
      public:
        Message::Automation::Message& message;
        Feed::Automation::Item& item;
        El::Net::HTTP::RequestParams request_params;
        Message::Automation::MessageRestrictions message_restrictions;
        bool item_article_requested;
        std::string encoding;

        Context(Message::Automation::Message& msg,
                Feed::Automation::Item& it,
                bool item_article_req,
                const char* enc) throw();

        Context() throw();        

        Context(const Context& src) throw();
        Context& operator=(const Context& src) throw();

        bool fill(
          const Item& channel_item,
          const Channel& channel,
          const char* feed_url,
          const char* enc,
          Feed::Space space,
          const ACE_Time_Value& timeout,
          const char* user_agent,
          size_t follow_redirects,
          El::Net::HTTP::Session::Interceptor* interceptor,
          const char* image_referrer,
          unsigned long image_recv_buffer_size,
          unsigned long image_min_width,
          unsigned long image_min_height,
          const ::NewsGate::Message::Automation::StringArray& image_black_list,
          const ::NewsGate::Message::Automation::StringSet& image_extensions,
          size_t max_title_len,
          size_t max_desc_len,
          size_t max_desc_chars,
          size_t max_image_count)
          throw(Exception, El::Exception);
        
        void write(El::BinaryOutStream& ostr) const
          throw(El::Exception);

        void read(El::BinaryInStream& istr) throw(El::Exception);

        El::Net::HTTP::Session*
        normalize_message_url(std::string& message_url,
                              El::Net::HTTP::Session::Interceptor* interceptor,
                              std::ostream& log)
          throw(Exception, El::Exception);

      private:
        
        static bool image_present(
          const Message::Automation::Image& img,
          const Message::Automation::ImageArray& images)
          throw(El::Exception);        

      private:
        
        private:
        ::NewsGate::Message::Automation::Message message_;
        ::NewsGate::Feed::Automation::Item item_;
      };

      typedef std::vector<Context> ContextArray;

      class Code
      {
      public:
        Code() throw(Exception, El::Exception);
        ~Code() throw();
        
        void compile(const char* value,
                     unsigned long feed_id,
                     const char* feed_url) throw(Exception, El::Exception);

        void run(Context& context,
                 El::Python::SandboxService* sandbox_service,
                 El::Python::Sandbox* sandbox,
                 El::Logging::Logger* logger) const
          throw(Exception, El::Exception);

        bool compiled() const throw() { return code_.compiled(); }

      private:
        El::Python::Code code_;
      };

      namespace Python
      {
        class Interceptor : public El::Python::SandboxService::Interceptor
        {
        public:
          std::string log;
  
        public:

          Interceptor(PyTypeObject *type = 0,
                      PyObject *args = 0,
                      PyObject *kwds = 0)
            throw(El::Exception);
  
          virtual ~Interceptor() throw() {}

          virtual void pre_run(El::Python::Sandbox* sandbox,
                               El::Python::SandboxService::ObjectMap& objects)
            throw(El::Exception);
  
          virtual void post_run(
            El::Python::SandboxService::ExceptionType exception_type,
            El::Python::SandboxService::ObjectMap& objects,
            El::Python::Object_var& result)
            throw(El::Exception);  

          virtual void write(El::BinaryOutStream& bstr) const
            throw(El::Exception);
          
          virtual void read(El::BinaryInStream& bstr) throw(El::Exception);

          class Type :
            public El::Python::ObjectTypeImpl<Interceptor, Interceptor::Type>
          {
          public:
            Type() throw(El::Python::Exception, El::Exception);
            static Type instance;    
          };

        private:
          std::auto_ptr<std::ostringstream> stream_;
          std::auto_ptr<El::Logging::Logger> logger_;
        };

        typedef El::Python::SmartPtr<Interceptor> Interceptor_var;
        
        class Context : public El::Python::ObjectImpl
        {
        public:
          Context(PyTypeObject *type = 0,
                  PyObject *args = 0,
                  PyObject *kwds = 0)
            throw(El::Exception);

          Context(const ::NewsGate::RSS::MsgAdjustment::Context& context)
            throw(Exception, El::Exception);

          virtual ~Context() throw() { item->article.delete_file(); }

          void save(::NewsGate::RSS::MsgAdjustment::Context& ctx) const
            throw(El::Exception);
          
          void apply_restrictions() throw(El::Exception);

          PyObject* py_find_in_article(PyObject* args, PyObject* kwds)
            throw(El::Exception);

          PyObject* py_set_description(PyObject* args, PyObject* kwds)
            throw(El::Exception);

          PyObject* py_set_description_from_article(PyObject* args,
                                                    PyObject* kwds)
            throw(El::Exception);

          PyObject* py_set_keywords(PyObject* args, PyObject* kwds)
            throw(El::Exception);

          PyObject* py_set_images(PyObject* args, PyObject* kwds)
            throw(El::Exception);
          
          PyObject* py_skip_image(PyObject* args, PyObject* kwds)
            throw(El::Exception);
          
          PyObject* py_reset_image_alt(PyObject* args, PyObject* kwds)
            throw(El::Exception);
          
          PyObject* py_read_image_size(PyObject* args, PyObject* kwds)
            throw(El::Exception);
          
          virtual void write(El::BinaryOutStream& ostr) const
            throw(El::Exception);
          
          virtual void read(El::BinaryInStream& istr) throw(El::Exception);
          
          class Type :
            public El::Python::ObjectTypeImpl<Context, Context::Type>
          {
          public:
            Type() throw(El::Python::Exception, El::Exception);
            static Type instance;

            PY_TYPE_MEMBER_STRING(encoding,
                                  "encoding",
                                  "Feed default encoding",
                                  true);

            PY_TYPE_MEMBER_OBJECT(
              message,
              Message::Automation::Python::Message::Type,
              "message",
              "Message to adjust",
              false);
            
            PY_TYPE_MEMBER_OBJECT(
              item,
              Feed::Automation::Python::Item::Type,
              "item",
              "Feed item",
              false);

            PY_TYPE_MEMBER_OBJECT(
              request_params,
              El::Net::HTTP::Python::RequestParams::Type,
              "request_params",
              "HTTP request params to use",
              false);
            
            PY_TYPE_MEMBER_OBJECT(
              message_restrictions,
              Message::Automation::Python::MessageRestrictions::Type,
              "message_restrictions",
              "Message restrictions to apply",
              false);
            
            PY_TYPE_METHOD_KWDS(
              py_find_in_article,
              "find_in_article",
              "Searches in item article document; return empty sequence if "
              "document not exist");

            PY_TYPE_METHOD_KWDS(
              py_set_keywords,
              "set_keywords",
              "Sets message keywords from article tags");

            PY_TYPE_METHOD_KWDS(
              py_set_images,
              "set_images",
              "Sets message images from article tags");

            PY_TYPE_METHOD_KWDS(
              py_set_description,
              "set_description",
              "Resets message description applying restrictions");

            PY_TYPE_METHOD_KWDS(
              py_set_description_from_article,
              "set_description_from_article",
              "Resets message description from article tags");

            PY_TYPE_METHOD_KWDS(
              py_skip_image,
              "skip_image",
              "Skip message images with origin specified");

            PY_TYPE_METHOD_KWDS(
              py_reset_image_alt,
              "reset_image_alt",
              "Reset image alt text for origin specified");

            PY_TYPE_METHOD_KWDS(
              py_read_image_size,
              "read_image_size",
              "Reads image size in a safe manner");

            PY_TYPE_STATIC_MEMBER(OP_ADD_BEGIN_, "OP_ADD_BEGIN");
            PY_TYPE_STATIC_MEMBER(OP_ADD_END_, "OP_ADD_END");
            PY_TYPE_STATIC_MEMBER(OP_ADD_TO_EMPTY_, "OP_ADD_TO_EMPTY");
            PY_TYPE_STATIC_MEMBER(OP_REPLACE_, "OP_REPLACE");

          private:
            El::Python::Object_var OP_ADD_BEGIN_;
            El::Python::Object_var OP_ADD_END_;
            El::Python::Object_var OP_ADD_TO_EMPTY_;
            El::Python::Object_var OP_REPLACE_;
          };
          
          Message::Automation::Python::Message_var message;
          Feed::Automation::Python::Item_var item;
          El::Net::HTTP::Python::RequestParams_var request_params;
          
          Message::Automation::Python::MessageRestrictions_var
          message_restrictions;

          std::string encoding;
          
        private:
          void init_subobj(
            const ::NewsGate::RSS::MsgAdjustment::Context* context)
            throw(El::Exception);
          
          Message::Automation::Image::Origin
          cast_origin(unsigned long orig, const char* context)
            throw(Exception, El::Exception);

          void truncate(std::string& text, PyObject* markers, bool left)
            throw(El::Exception);

        private:
          Message::Automation::MessageRestrictions message_restrictions_;
        };
      
        typedef El::Python::SmartPtr<Context> Context_var;
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace RSS
  {
    namespace MsgAdjustment
    {
      //
      // Code class
      //
      inline
      Code::Code() throw(Exception, El::Exception)
      {
      }

      inline
      Code::~Code() throw()
      {
        El::Python::EnsureCurrentThread guard;
        code_.clear();
      }

      //
      // Context class
      //
      inline
      Context::Context(::NewsGate::Message::Automation::Message& msg,
                       ::NewsGate::Feed::Automation::Item& it,
                       bool item_article_req,
                       const char* enc) throw()
          : message(msg),
            item(it),
            item_article_requested(item_article_req),
            encoding(enc)
      {
      }

      inline
      Context::Context() throw()
          : message(message_),
            item(item_),
            item_article_requested(false)
      {
      }

      inline
      Context::Context(const Context& src) throw()
         : message(message_),
           item(item_),
           item_article_requested(src.item_article_requested),
           encoding(src.encoding)
      {
        message = src.message;
        item = src.item;
        request_params = src.request_params;
        message_restrictions = src.message_restrictions;
      }
      
      inline
      Context&
      Context::operator=(const Context& src) throw()
      {
        message = src.message;
        item = src.item;
        request_params = src.request_params;
        message_restrictions = src.message_restrictions;
        item_article_requested = src.item_article_requested;
        encoding = src.encoding;
        
        return *this;
      }
      
      inline
      void
      Context::write(El::BinaryOutStream& ostr) const throw(El::Exception)
      {
        ostr << message << item << request_params << message_restrictions
             << (uint8_t)item_article_requested << encoding;
      }

      inline
      void
      Context::read(El::BinaryInStream& istr) throw(El::Exception)
      {
        uint8_t iar = 0;
        
        istr >> message >> item >> request_params >> message_restrictions
             >> iar >> encoding;

        item_article_requested = iar;
      }
      
      inline
      bool
      Context::image_present(const Message::Automation::Image& img,
                             const Message::Automation::ImageArray& images)
        throw(El::Exception)
      {
        for(Message::Automation::ImageArray::const_iterator
              i(images.begin()), e(images.end()); i != e; ++i)
        {
          if(i->src == img.src)
          {
            return true;
          }
        }

        return false;
      }        
        
      namespace Python
      {
        //
        // Interceptor class
        //
        inline
        Interceptor::Interceptor(PyTypeObject *type,
                                 PyObject *args,
                                 PyObject *kwds)
          throw(El::Exception) :
            El::Python::SandboxService::Interceptor(type ?
                                                    type : &Type::instance)
        {
        }

        inline
        Interceptor::Type::Type() throw(El::Python::Exception, El::Exception)
            : El::Python::ObjectTypeImpl<Interceptor, Interceptor::Type>(
              "newsgate.MA_Interceptor",
              "Interceptor object")
        {
        }

        //
        // Context class
        //
        inline
        Context::Context(PyTypeObject *type, PyObject *args, PyObject *kwds)
          throw(El::Exception) :
            El::Python::ObjectImpl(type ? type : &Type::instance)
        {
          init_subobj(0);
        }

        inline
        Message::Automation::Image::Origin
        Context::cast_origin(unsigned long orig, const char* context)
          throw(Exception, El::Exception)
        {
          if(orig >= Message::Automation::Image::IO_COUNT)
          {
            std::ostringstream ostr;
            ostr << context << ": invalid origin value " << orig;

            throw Exception(ostr.str());
          }

          return (Message::Automation::Image::Origin)orig;
        }

        //
        // Context::Type class
        //
        inline
        Context::Type::Type() throw(El::Python::Exception, El::Exception)
            : El::Python::ObjectTypeImpl<Context, Context::Type>(
              "newsgate.MA_Context",
              "Object representing adjustment script execution context")
        {
          OP_ADD_BEGIN_ = PyLong_FromLong(Message::Automation::OP_ADD_BEGIN);
          OP_ADD_END_ = PyLong_FromLong(Message::Automation::OP_ADD_END);
          
          OP_ADD_TO_EMPTY_ =
            PyLong_FromLong(Message::Automation::OP_ADD_TO_EMPTY);
          
          OP_REPLACE_ = PyLong_FromLong(Message::Automation::OP_REPLACE);
        }
      }
    }
  }
}

#endif // _NEWSGATE_SERVER_XSD_DATAFEED_RSS_MSGADJUSTMENT_HPP_
