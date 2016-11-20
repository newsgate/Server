/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file xsd/DataFeed/RSS/HTMLFeedParser.hpp
 * @author Karen Arutyunov
 * $id:$
 */

#ifndef _NEWSGATE_SERVER_XSD_DATAFEED_RSS_HTMLFEEDPARSER_HPP_
#define _NEWSGATE_SERVER_XSD_DATAFEED_RSS_HTMLFEEDPARSER_HPP_

#include <stdint.h>

#include <string>
#include <vector>
#include <map>
#include <ext/hash_map>

#include <El/Exception.hpp>
#include <El/CRC.hpp>
#include <El/Hash/Hash.hpp>
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

#include <El/Net/HTTP/Session.hpp>
#include <El/Net/HTTP/Robots.hpp>
#include <El/LibXML/Python/HTMLParser.hpp>

#include <Commons/Feed/Types.hpp>
#include <Commons/Feed/Automation/Automation.hpp>
#include <Commons/Feed/Automation/Article.hpp>
#include <Commons/Message/Automation/Automation.hpp>

#include <xsd/DataFeed/RSS/Data.hpp>

namespace NewsGate
{
  namespace RSS
  {
    namespace HTMLFeed
    {
      EL_EXCEPTION(Exception, El::ExceptionBase);

      class Cache
      {
      public:
        
        typedef __gnu_cxx::hash_map<uint64_t,
                                    uint64_t,
                                    El::Hash::Numeric<uint64_t> >
        TimedHashMap;

        Cache(uint64_t max_size = 0) throw(El::Exception);        
        
        void write(El::BinaryOutStream& ostr) const throw(El::Exception);
        void read(El::BinaryInStream& istr) throw(El::Exception);

        bool url_present(const char* url) throw(El::Exception);
        bool url_present(PyObject* url) throw(El::Exception);
        void add_url(const char* url) throw(El::Exception);

        bool doc_present(uint64_t crc) throw(El::Exception);
        bool title_present(const char* title) throw(El::Exception);
        void title_erase(const char* title) throw(El::Exception);
        
        std::string stringify() const throw(El::Exception);
        void destringify(const char* val) throw(El::Exception);

        void max_size(uint64_t val) throw(El::Exception);
        void timeout(uint64_t val) throw(El::Exception);
        void resize(uint64_t val) throw(El::Exception);
        void cleanup() throw(El::Exception);
        void clear() throw();
        size_t size() const throw();

      private:
        void write_data(El::BinaryOutStream& ostr) const throw(El::Exception);
        void read_data(El::BinaryInStream& istr) throw(El::Exception);
        void remove_oldest(TimedHashMap& map) throw(El::Exception);
        void cleanup(TimedHashMap& map) throw(El::Exception);
        
      private:        
        TimedHashMap urls_;
        TimedHashMap docs_; 
        TimedHashMap titles_;
        uint64_t max_size_;
        uint64_t timeout_;
      };
      
      class Context
      {
      public:
        Message::Automation::MessageArray messages;
        El::Net::HTTP::RequestParams request_params;
        Message::Automation::MessageRestrictions message_restrictions;
        std::string feed_url;
        uint64_t article_max_size;
        std::string cache_dir;
        Cache cache;
        uint8_t interrupted;
        Feed::Space space;
        El::Lang lang;
        El::Country country;
        El::String::Array keywords;
        uint32_t outbound_bytes;
        uint32_t inbound_bytes;
        std::string encoding;

        Context() throw();

        Context(const Context& src) throw();
        Context& operator=(const Context& src) throw();

        void fill(const char* url,
                  NewsGate::Feed::Space space_val,
                  El::Country country_val,
                  El::Lang lang_val,
                  const El::String::Array& kwds,
                  El::Geography::AddressInfo* address_info,
                  const ACE_Time_Value& timeout,
                  const char* user_agent,
                  size_t follow_redirects,
                  const char* image_referrer,
                  unsigned long image_recv_buffer_size,
                  unsigned long image_min_width,
                  unsigned long image_min_height,
                  const Message::Automation::StringArray& image_black_list,
                  const Message::Automation::StringSet& image_extensions,
                  size_t max_title_len,
                  size_t max_desc_len,
                  size_t max_desc_chars,
                  size_t max_image_count,
                  uint64_t article_max_size_val,
                  const char* cache_dir_val,
                  const char* cache_val,
                  uint64_t cache_max_size,
                  uint64_t cache_timeout,
                  const char* encoding_val)
          throw(El::Exception);
        
        void write(El::BinaryOutStream& ostr) const
          throw(El::Exception);

        void read(El::BinaryInStream& istr) throw(El::Exception);
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
                 El::Net::HTTP::Session* session,
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
          uint8_t interrupted;
  
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

          Context(const ::NewsGate::RSS::HTMLFeed::Context& context,
                  El::Net::HTTP::Session* session)
            throw(Exception, El::Exception);

          virtual ~Context() throw();

          void save(::NewsGate::RSS::HTMLFeed::Context& ctx) const
            throw(El::Exception);
          
          void pre_run(El::Python::Sandbox* sandbox) throw(El::Exception);
          void apply_restrictions() throw(El::Exception);
          
          virtual void write(El::BinaryOutStream& ostr) const
            throw(El::Exception);
          
          virtual void read(El::BinaryInStream& istr) throw(El::Exception);

          PyObject* py_html_doc(PyObject* args, PyObject* kwds)
            throw(El::Exception);
          
          PyObject* py_new_message(PyObject* args, PyObject* kwds)
            throw(El::Exception);
          
          PyObject* py_text_from_doc(PyObject* args, PyObject* kwds)
            throw(El::Exception);
          
          PyObject* py_images_from_doc(PyObject* args, PyObject* kwds)
            throw(El::Exception);
          
          PyObject* py_keywords_from_doc(PyObject* args, PyObject* kwds)
            throw(El::Exception);
          
          class Type :
            public El::Python::ObjectTypeImpl<Context, Context::Type>
          {
          public:
            Type() throw(El::Python::Exception, El::Exception);
            static Type instance;

            PY_TYPE_MEMBER_OBJECT(
              messages,
              El::Python::Sequence::Type,
              "messages",
              "Resulted messages",
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

            PY_TYPE_MEMBER_STRING(feed_url, "feed_url", "Feed url", false);

            PY_TYPE_MEMBER_STRING(encoding, "encoding", "Feed encoding", true);

            PY_TYPE_MEMBER_ENUM(space,
                                Feed::Space,
                                Feed::SP_SPACES_COUNT - 1,
                                "space",
                                "Feed space");

            PY_TYPE_MEMBER_OBJECT(lang,
                                  El::Python::Lang::Type,
                                  "lang",
                                  "Feed language",
                                  false);
            
            PY_TYPE_MEMBER_OBJECT(country,
                                  El::Python::Country::Type,
                                  "country",
                                  "Feed country",
                                  false);            
            
            PY_TYPE_MEMBER_OBJECT(keywords,
                                  El::Python::Sequence::Type,
                                  "keywords",
                                  "Feed keywords",
                                  false);            
            
            PY_TYPE_METHOD_KWDS(
              py_html_doc,
              "html_doc",
              "Downloads and parses HTML document");

            PY_TYPE_METHOD_KWDS(
              py_new_message,
              "new_message",
              "Creates new message object");

            PY_TYPE_METHOD_KWDS(
              py_text_from_doc,
              "text_from_doc",
              "Extracts plain text from HTML document");

            PY_TYPE_METHOD_KWDS(
              py_images_from_doc,
              "images_from_doc",
              "Create images from document nodes");

            PY_TYPE_METHOD_KWDS(
              py_keywords_from_doc,
              "keywords_from_doc",
              "Create keywords from document nodes");

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
          
          El::Python::Sequence_var messages;
          El::Net::HTTP::Python::RequestParams_var request_params;
          
          Message::Automation::Python::MessageRestrictions_var
          message_restrictions;

          std::string feed_url;
          uint64_t article_max_size;
          std::string cache_dir;
          Feed::Space space;
          El::Python::Lang_var lang;
          El::Python::Country_var country;
          El::Python::Sequence_var keywords;
          std::auto_ptr<Feed::Automation::Article> feed_article;
          uint32_t outbound_bytes;
          uint32_t inbound_bytes;
          std::string encoding;
          
        private:
          void init_subobj(
            const ::NewsGate::RSS::HTMLFeed::Context* context)
            throw(El::Exception);

          PyObject* html_doc(PyObject* doc,
                             const char* context,
                             const char* encoding,
                             std::string* error = 0)
            throw(El::Exception);

          PyObject* text_from_doc(PyObject* text_doc_seq,
                                  PyObject* doc,
                                  const char* encoding)
            throw(El::Exception);

          std::string title_from_doc(PyObject* doc, const char* encoding)
            throw(El::Exception);
          
          std::string description_from_doc(PyObject* doc,
                                           const char* encoding)
            throw(El::Exception);
          
          void normalize_msg_url(Message::Automation::Python::Message* msg,
                                 const char* context,
                                 const char* encoding)
            throw(El::Exception);
          
          PyObject* find_doc(const char* url) throw(El::Exception);
          
          void add_doc(PyObject* doc, const char* url) throw(El::Exception);

          static bool overlap(const El::String::Set& s1,
                              const El::String::Set& s2)
            throw(El::Exception);

          PyObject* normalize_url(PyObject* url,
                                  bool drop_url_anchor,
                                  const char* context,
                                  const char* encoding)
            throw(El::Exception);

          void invalidate_msg(Message::Automation::Python::Message* msg,
                              const char* error,
                              bool save = false)
            throw(El::Exception);

          const El::Net::HTTP::Python::RequestParams* adjust_request_params()
            throw(El::Exception);

        private:  
          typedef std::map<std::string, El::Python::Object_var> ObjectMap;
          ObjectMap documents_;
          std::string feed_host_;
          Cache cache_;          
          uint64_t execution_end_;
          std::auto_ptr<El::Net::HTTP::RobotsChecker> robots_checker_;
          
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
    namespace HTMLFeed
    {
      inline
      Cache::Cache(uint64_t max_size) throw(El::Exception)
          : max_size_(max_size),
            timeout_(0)
      {
      }

      void
      Cache::max_size(uint64_t val) throw(El::Exception)
      {
        max_size_ = val;
      }
      
      void
      Cache::timeout(uint64_t val) throw(El::Exception)
      {
        timeout_ = val;
      }
      
      inline
      void
      Cache::write(El::BinaryOutStream& ostr) const throw(El::Exception)
      {
        ostr << (uint32_t)1 << max_size_ << timeout_;
        write_data(ostr);
      }
      
      inline
      void
      Cache::read(El::BinaryInStream& istr) throw(El::Exception)
      {
        uint32_t version = 0;
        istr >> version >> max_size_ >> timeout_;
        read_data(istr);
      }

      inline
      void
      Cache::write_data(El::BinaryOutStream& ostr) const throw(El::Exception)
      {
        ostr << (uint32_t)1;
        
        ostr.write_map(urls_);
        ostr.write_map(docs_);
        ostr.write_map(titles_);
      }
      
      inline
      void
      Cache::read_data(El::BinaryInStream& istr) throw(El::Exception)
      {
        uint32_t version = 0;
        istr >> version;

        istr.read_map(urls_);
        istr.read_map(docs_);
        istr.read_map(titles_);
      }

      inline
      void
      Cache::clear() throw()
      {
        urls_.clear();
        docs_.clear();
        titles_.clear();
      }

      inline
      size_t
      Cache::size() const throw()
      {
        size_t bin_len = (urls_.size() + docs_.size() + titles_.size()) *
          sizeof(TimedHashMap::value_type) + 3 * sizeof(uint64_t) +
          sizeof(uint32_t);

        return (bin_len / 3 + (bin_len % 3 ? 1 : 0)) * 4;
      }
      
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
      Context::Context() throw()
          : article_max_size(0),
            interrupted(0),
            space(Feed::SP_UNDEFINED),
            outbound_bytes(0),
            inbound_bytes(0)
      {
      }
      
      inline
      Context::Context(const Context& src) throw()
          : messages(src.messages),
            request_params(src.request_params),
            message_restrictions(src.message_restrictions),
            feed_url(src.feed_url),
            article_max_size(src.article_max_size),
            cache_dir(src.cache_dir),
            cache(src.cache),
            interrupted(src.interrupted),
            space(src.space),
            lang(src.lang),
            country(src.country),
            keywords(src.keywords),
            outbound_bytes(src.outbound_bytes),
            inbound_bytes(src.inbound_bytes),
            encoding(src.encoding)
      {
      }

      inline
      Context&
      Context::operator=(const Context& src) throw()
      {
        messages = src.messages;
        request_params = src.request_params;
        message_restrictions = src.message_restrictions;
        feed_url = src.feed_url;        
        article_max_size = src.article_max_size;
        cache_dir = src.cache_dir;        
        cache = src.cache;
        interrupted = src.interrupted;
        space = src.space;
        lang = src.lang;
        country = src.country;
        keywords = src.keywords;
        outbound_bytes = src.outbound_bytes;
        inbound_bytes = src.inbound_bytes;
        encoding = src.encoding;        
        
        return *this;
      }
      
      inline
      void
      Context::write(El::BinaryOutStream& ostr) const throw(El::Exception)
      {
        ostr.write_array(messages);
        
        ostr << request_params << message_restrictions << feed_url
             << article_max_size << cache_dir << cache << interrupted
             << (uint32_t)space << lang << country << outbound_bytes
             << inbound_bytes << encoding;

        ostr.write_array(keywords);
      }

      inline
      void
      Context::read(El::BinaryInStream& istr) throw(El::Exception)
      {
        istr.read_array(messages);

        uint32_t sp = 0;
        
        istr >> request_params >> message_restrictions >> feed_url
             >> article_max_size >> cache_dir >> cache >> interrupted
             >> sp >> lang >> country >> outbound_bytes >> inbound_bytes
             >> encoding;

        istr.read_array(keywords);

        space = (Feed::Space)sp;
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
                                                    type : &Type::instance),
            interrupted(0)
        {
        }

        inline
        Interceptor::Type::Type() throw(El::Python::Exception, El::Exception)
            : El::Python::ObjectTypeImpl<Interceptor, Interceptor::Type>(
              "newsgate.HF_Interceptor",
              "Interceptor object")
        {
        }

        //
        // Context class
        //
        inline
        Context::Context(PyTypeObject *type, PyObject *args, PyObject *kwds)
          throw(El::Exception) :
            El::Python::ObjectImpl(type ? type : &Type::instance),
            space(Feed::SP_UNDEFINED),
            outbound_bytes(0),
            inbound_bytes(0),
            execution_end_(0)
        {
          init_subobj(0);
        }

        inline
        Context::~Context() throw()
        {
          if(feed_article.get())
          {
            feed_article->delete_file();  
          }
        }
        
        //
        // Context::Type class
        //
        inline
        Context::Type::Type() throw(El::Python::Exception, El::Exception)
            : El::Python::ObjectTypeImpl<Context, Context::Type>(
              "newsgate.HF_Context",
              "Object representing HTML feed script execution context")
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

#endif // _NEWSGATE_SERVER_XSD_DATAFEED_RSS_HTMLFEEDPARSER_HPP_
