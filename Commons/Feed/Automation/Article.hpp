/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file Commons/Feed/Automation/Article.hpp
 * @author Karen Arutyunov
 * $id:$
 */

#ifndef _NEWSGATE_SERVER_COMMONS_FEED_AUTOMATION_ARTICLE_HPP_
#define _NEWSGATE_SERVER_COMMONS_FEED_AUTOMATION_ARTICLE_HPP_

#include <stdint.h>

#include <string>
#include <vector>

#include <El/Exception.hpp>
#include <El/String/Manip.hpp>
#include <El/BinaryStream.hpp>

#include <El/Python/Object.hpp>
#include <El/Net/HTTP/Session.hpp>
#include <El/Net/HTTP/Utility.hpp>
#include <El/Net/HTTP/Robots.hpp>

namespace NewsGate
{
  namespace Feed
  {
    namespace Automation
    {
      EL_EXCEPTION(Exception, El::ExceptionBase);

      typedef std::auto_ptr<El::Net::HTTP::Session> HTTPSessionPtr;
      
      class Article
      {
      public:

        Article() throw(El::Exception);
        
        Article(const char* url,
                const El::Net::HTTP::RequestParams& req_params,
                uint64_t max_size,
                const char* cache_dir,
                El::Net::HTTP::Session* session,
                bool read,
                El::Net::HTTP::RobotsChecker* robots_checker,
                const char* encoding)
          throw(Exception, El::Exception);
        
        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);
        
        bool read_to_file() throw(El::Exception);
        void delete_file() throw();
        void close_session() throw() { session_.reset(0); }
          
        PyObject* document() throw(El::Exception);
        const char* error() const throw() { return error_.c_str(); }

        uint32_t outbound_bytes() const throw() { return outbound_bytes_; }
        uint32_t inbound_bytes() const throw() { return inbound_bytes_; }

        void encoding(const char* charset) throw() { file_encoding_=charset; }
        
      private:
        HTTPSessionPtr session_;
        std::string url_;
        std::string permanent_url_;
        std::string file_;
        std::string file_encoding_;
        uint64_t max_size_;
        El::Python::Object_var doc_;
        std::string cache_dir_;
        El::Net::HTTP::RequestParams request_params_;
        El::String::Array all_urls_;
        uint64_t file_crc_;
        El::Net::HTTP::RobotsChecker* robots_checker_;
        std::string error_;
        uint32_t outbound_bytes_;
        uint32_t inbound_bytes_;

      private:
        Article(const Article& );
        void operator=(const Article& );        
      };
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
      // Article class
      //
      inline
      Article::Article() throw(El::Exception)
          : max_size_(0),
            file_crc_(0),
            robots_checker_(0),
            outbound_bytes_(0),
            inbound_bytes_(0)
      {
      }
    }
  }
}

#endif // _NEWSGATE_SERVER_COMMONS_FEED_AUTOMATION_ARTICLE_HPP_
