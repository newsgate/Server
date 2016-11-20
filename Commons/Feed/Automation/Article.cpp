/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file Commons/Feed/Automation/Article.cpp
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

#include "Article.hpp"

namespace NewsGate
{
  namespace Feed
  {
    namespace Automation
    {
      //
      // Article struct
      //
      Article::Article(const char* url,
                       const El::Net::HTTP::RequestParams& req_params,
                       uint64_t max_size,
                       const char* cache_dir,
                       El::Net::HTTP::Session* session,
                       bool read,
                       El::Net::HTTP::RobotsChecker* robots_checker,
                       const char* encoding)
        throw(Exception, El::Exception)
          : session_(session),
            url_(url ? url : ""),
            file_encoding_(encoding ? encoding : ""),
            max_size_(max_size),
            cache_dir_(cache_dir),
            request_params_(req_params),
            file_crc_(0),
            robots_checker_(robots_checker),
            outbound_bytes_(0),
            inbound_bytes_(0)
      {
        if(session_.get())
        {
          url_ = session_->url()->string();

          if(read)
          {
            read_to_file();
          }
        }

        permanent_url_ = url_;
        all_urls_.push_back(url_);
      }

      void
      Article::write(El::BinaryOutStream& bstr) const throw(El::Exception)
      {
        bstr << max_size_ << cache_dir_ << url_ << permanent_url_ << file_
             << file_encoding_ << request_params_ << (uint64_t)file_crc_
             << outbound_bytes_ << inbound_bytes_;

        bstr.write_array(all_urls_);
      }
          
      void
      Article::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        uint64_t fc = 0;
        
        bstr >> max_size_ >> cache_dir_ >> url_ >> permanent_url_ >> file_
             >> file_encoding_ >> request_params_ >> fc
             >> outbound_bytes_ >> inbound_bytes_;
        
        bstr.read_array(all_urls_);
        file_crc_ = fc;
      }
      
      bool
      Article::read_to_file() throw(El::Exception)
      {
        if(!file_.empty())
        {
          return true;
        }

        El::Python::AllowOtherThreads guard;
        
        try
        {
          std::string permanent_url;
          
          if(session_.get() == 0 && !url_.empty())
          { 
            El::Net::HTTP::URL_var url = new El::Net::HTTP::URL(url_.c_str());

            El::Net::HTTP::HeaderList headers;

            headers.add(El::Net::HTTP::HD_ACCEPT, "*/*");
            headers.add(El::Net::HTTP::HD_ACCEPT_ENCODING, "gzip,deflate");
            headers.add(El::Net::HTTP::HD_ACCEPT_LANGUAGE, "en-us,*");
              
            headers.add(El::Net::HTTP::HD_USER_AGENT,
                        request_params_.user_agent.c_str());
              
            HTTPSessionPtr session(
              new El::Net::HTTP::Session(url, El::Net::HTTP::HTTP_1_1));

            ACE_Time_Value timeout(request_params_.timeout);
            session->open(&timeout, &timeout, &timeout);
              
            permanent_url =
              session->send_request(El::Net::HTTP::GET,
                                    El::Net::HTTP::ParamList(),
                                    headers,
                                    0,
                                    0,
                                    request_params_.redirects_to_follow);

            session->recv_response_status();

            inbound_bytes_ += session->received_bytes(true);
            outbound_bytes_ += session->sent_bytes(true);
            
            if(session->status_code() < El::Net::HTTP::SC_BAD_REQUEST)
            {
              session_.reset(session.release());
            }
          }

          if(session_.get())
          {
            if(robots_checker_ &&
               !robots_checker_->allowed(session_->url(),
                                         request_params_.user_agent.c_str()))
            {
              std::ostringstream ostr;
              ostr << "::NewsGate::Feed::Automation::Article::"
                "read_to_file: indexing disallowed for "
                   << session_->url()->string();
              
              throw Exception(ostr.str());
            }
            
            if(session_->status_code() != El::Net::HTTP::SC_OK)
            {
              std::ostringstream ostr;
              ostr << "::NewsGate::Feed::Automation::Article::"
                "read_to_file: unexpected response status "
                   << session_->status_code() << " for "
                   << session_->url()->string();
              
              throw Exception(ostr.str());
            }

            std::string url = session_->url()->string();
            
            El::Net::HTTP::Header header;
            while(session_->recv_response_header(header));

            if(file_encoding_.empty())
            {
              file_encoding_ = session_->charset();
            }

            El::Guid guid;
            guid.generate();
                  
            std::ostringstream ostr;
            ostr << cache_dir_ << "/Article."
                 << guid.string(El::Guid::GF_DENSE);
                    
            file_ = ostr.str();
            file_crc_ = 0;
                  
            session_->save_body(file_.c_str(),
                                max_size_,
                                &file_crc_,
                                file_encoding_.c_str());

            inbound_bytes_ += session_->received_bytes(true);
            outbound_bytes_ += session_->sent_bytes(true);

            all_urls_ = session_->all_urls();
            session_.reset(0);

            if(!permanent_url.empty())
            {
              permanent_url_ = permanent_url;
            }
            
            return true;
          }
        }
        catch(const El::Exception& e)
        {
          // Something went wrong while trying to download article
          // (timeout, whong response data, ...); will just consider
          // XML unavailable in this case.

          if(session_.get())
          {
            inbound_bytes_ += session_->received_bytes(true);
            outbound_bytes_ += session_->sent_bytes(true);
          }
          
          error_ = e.what();
          session_.reset(0);
          delete_file();
        }

        return false;
      }

      void
      Article::delete_file() throw()
      {        
        if(!file_.empty())
        {
          unlink(file_.c_str());
          file_.clear();
          file_encoding_.clear();
        }
      }
        
      PyObject*
      Article::document() throw(El::Exception)
      {
        if(doc_.in() != 0)
        {
          return El::Python::add_ref(doc_.in());
        }

        try
        {
          if(read_to_file())
          {
            El::LibXML::ErrorRecorderHandler error_handler;
                
            El::LibXML::Python::HTMLParser_var parser =
              new El::LibXML::Python::HTMLParser();
            
            doc_ = parser->parse_file(file_.c_str(),
                                      file_encoding_.c_str(),
                                      &error_handler,
                                      HTML_PARSE_NONET);
            
            delete_file();
              
            if(doc_.in() != Py_None)
            {
              El::LibXML::Python::Document* d =
                El::LibXML::Python::Document::Type::down_cast(doc_.in());
              
              d->url(permanent_url_.c_str());
              d->all_urls(all_urls_);
              d->crc(file_crc_);
            }
          }
          else
          {
            doc_ = El::Python::add_ref(Py_None);
          }
        }
        catch(const El::Exception& e)
        {
          delete_file();
          error_ = e.what();
          
          // Something went wrong while parsing the article;
          // will just consider XML unavailable in this case.
          doc_ = El::Python::add_ref(Py_None);
        }
        
        return El::Python::add_ref(doc_.in());
      }
    }
  }
}
