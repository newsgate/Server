/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file xsd/DataFeed/RSS/MsgAdjustment.cpp
 * @author Karen Arutyunov
 * $id:$
 */

#include <Python.h>
#include <frameobject.h>
#include <traceback.h>
//#include <marshal.h>

#include <string>
#include <map>

#include <El/Exception.hpp>
#include <El/Logging/Logger.hpp>
#include <El/Logging/StreamLogger.hpp>
#include <El/String/Unicode.hpp>
#include <El/String/Manip.hpp>

#include <El/Guid.hpp>
#include <El/String/ListParser.hpp>

#include <El/Python/RefCount.hpp>
#include <El/Python/Object.hpp>
#include <El/Python/Utility.hpp>
#include <El/Python/Logger.hpp>
#include <El/Python/Code.hpp>
#include <El/Python/Module.hpp>
#include <El/Python/Sandbox.hpp>

#include <El/LibXML/Python/HTMLParser.hpp>

#include <El/Net/HTTP/StatusCodes.hpp>
#include <El/Net/HTTP/Headers.hpp>

#include <El/HTML/LightParser.hpp>

#include "HTMLFeedParser.hpp"

namespace NewsGate
{
  namespace RSS
  {
    namespace HTMLFeed
    {
      //
      // Cache class
      //

      void
      Cache::cleanup() throw(El::Exception)
      {
        cleanup(urls_);
        cleanup(docs_);
        cleanup(titles_);        
      }
      
      void
      Cache::cleanup(TimedHashMap& map) throw(El::Exception)
      {
        if(timeout_ == 0)
        {
          return;
        }
        
        uint64_t updated = ACE_OS::gettimeofday().sec() - timeout_;
        
        for(TimedHashMap::iterator i(map.begin()), e(map.end()); i != e; )
        {
          if(i->second <= updated)
          {
            TimedHashMap::iterator cur = i++;
            map.erase(cur);
          }
          else
          {
            ++i;
          }
        }        
      }
      
      void
      Cache::resize(uint64_t val) throw(El::Exception)
      {
        if(max_size_ == 0)
        {
          return;
        }
        
        while(size() > max_size_)
        {
          remove_oldest(urls_);
          remove_oldest(docs_);
          remove_oldest(titles_);
        }
      }
      
      void
      Cache::remove_oldest(TimedHashMap& map) throw(El::Exception)
      {
        uint64_t updated = UINT64_MAX;
        TimedHashMap::iterator oldest = map.end();
          
        for(TimedHashMap::iterator i(map.begin()), e(map.end()); i != e; ++i)
        {
          if(i->second < updated)
          {
            updated = i->second;
            oldest = i;
          }
        }

        if(oldest != map.end())
        {
          map.erase(oldest);
        }
      }
      
      bool
      Cache::url_present(const char* url) throw(El::Exception)
      {
//        std::cerr << "PRE: " << url << std::endl;

        uint64_t crc = 0;
        El::CRC(crc, (const unsigned char*)url, strlen(url));

        TimedHashMap::iterator i = urls_.find(crc);
        uint64_t updated = ACE_OS::gettimeofday().sec();

        if(i != urls_.end())
        {
          i->second = updated;
          return true;
        }

        urls_[crc] = updated;
        resize(max_size_);
        
        return false;
      }

      void
      Cache::add_url(const char* url) throw(El::Exception)
      {
//        std::cerr << "ADD: " << url << std::endl;
        
        uint64_t crc = 0;
        El::CRC(crc, (const unsigned char*)url, strlen(url));

        TimedHashMap::iterator i = urls_.find(crc);
        uint64_t updated = ACE_OS::gettimeofday().sec();

        if(i == urls_.end())
        {
          urls_[crc] = updated;
          resize(max_size_);
        }
        else
        {
          i->second = updated;
        }
      }

      bool
      Cache::url_present(PyObject* url) throw(El::Exception)
      {
        if(!PyString_Check(url))
        {
          return false;
        }
        
        size_t len = 0;              
        std::string u = El::Python::string_from_string(url, len);
        return url_present(u.c_str());
      }

      bool
      Cache::doc_present(uint64_t crc) throw(El::Exception)
      {
        TimedHashMap::iterator i = docs_.find(crc);
        uint64_t updated = ACE_OS::gettimeofday().sec();

        if(i != docs_.end())
        {
          i->second = updated;
          return true;
        }

        docs_[crc] = updated;
        resize(max_size_);
        
        return false;
      }

      bool
      Cache::title_present(const char* title) throw(El::Exception)
      {
        uint64_t crc = 0;
        El::CRC(crc, (const unsigned char*)title, strlen(title));

        TimedHashMap::iterator i = titles_.find(crc);
        uint64_t updated = ACE_OS::gettimeofday().sec();

        if(i != titles_.end())
        {
          i->second = updated;
          return true;
        }

        titles_[crc] = updated;
        resize(max_size_);
        
        return false;
      }

      void
      Cache::title_erase(const char* title) throw(El::Exception)
      {
        uint64_t crc = 0;
        El::CRC(crc, (const unsigned char*)title, strlen(title));        
        titles_.erase(crc);        
      }      
      
      std::string
      Cache::stringify() const throw(El::Exception)
      {
        std::ostringstream ostr;
        
        {
          El::BinaryOutStream bstr(ostr);
          write_data(bstr);
        }

        std::string res;
        std::string val = ostr.str();
        
        El::String::Manip::base64_encode((const unsigned char*)val.c_str(),
                                         val.length(),
                                         res);
        return res;
      }

      void
      Cache::destringify(const char* val) throw(El::Exception)
      {
        clear();
        
        if(val == 0 || *val == '\0')
        {
          return;
        }

        std::string decoded;
        El::String::Manip::base64_decode(val, decoded);

        std::istringstream istr(decoded);
        
        {
          El::BinaryInStream bstr(istr);
          read_data(bstr);
        }
      }

      //
      // Context class
      //
      void
      Context::fill(const char* url,
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
        throw(El::Exception)
      {
        feed_url = url;
        encoding = encoding_val;

        space = space_val;

        RSS::Channel channel;

        channel.adjust(url, url, country_val, lang_val, address_info);

        lang = channel.lang;
        country = channel.country;
        
        request_params.timeout = timeout.sec();
        
        request_params.user_agent = user_agent;
        request_params.redirects_to_follow = follow_redirects;
        request_params.referer = image_referrer;
        request_params.recv_buffer_size = image_recv_buffer_size;

        Message::Automation::ImageRestrictions& ir =
          message_restrictions.image_restrictions;
        
        ir.min_width = image_min_width;
        ir.min_height = image_min_height;
        ir.black_list = image_black_list;
        ir.extensions = image_extensions;

        message_restrictions.max_title_len = max_title_len;
        message_restrictions.max_desc_len = max_desc_len;
        message_restrictions.max_desc_chars = max_desc_chars;
        message_restrictions.max_image_count = max_image_count;

        keywords = kwds;
        
        article_max_size = article_max_size_val;
        cache_dir = cache_dir_val;

        try
        {
          cache.destringify(cache_val);
        }
        catch(const El::Exception&)
        {
          cache.clear();
        }
        
        cache.max_size(cache_max_size);
        cache.timeout(cache_timeout);
      }
        
      namespace Python
      {
        Interceptor::Type Interceptor::Type::instance;
        Context::Type Context::Type::instance;

        //
        // Interceptor struct
        //
        void
        Interceptor::pre_run(El::Python::Sandbox* sandbox,
                             El::Python::SandboxService::ObjectMap& objects)
          throw(El::Exception)
        {          
          stream_.reset(new std::ostringstream());
          
          logger_.reset(new El::Logging::StreamLogger(
                          *stream_,
                          El::Logging::TRACE + El::Logging::HIGH));
          
          El::Python::Object_var obj =
            new El::Logging::Python::Logger(logger_.get());
          
          objects["logger"] = obj;

//          std::cerr << "----------------------------------------" << std::endl;
          
          El::Python::SandboxService::ObjectMap::iterator i =
            objects.find("context");

          if(i == objects.end())
          {
            throw Exception("no context object passes");
          }
          
          Python::Context* context =
            Python::Context::Type::down_cast(i->second.in());

          context->pre_run(sandbox);
        }
  
        void
        Interceptor::post_run(
          El::Python::SandboxService::ExceptionType exception_type,
          El::Python::SandboxService::ObjectMap& objects,
          El::Python::Object_var& result)
          throw(El::Exception)
        {
          interrupted = exception_type ==
            El::Python::SandboxService::ET_EXECUTION_INTERRUPTED;
          
          objects.erase("logger");

//          if(interrupted)
//          {
//            logger_->info("Script execution interrupted", "SCRIPTING");
//          }
          
          logger_.reset(0);

          El::Python::SandboxService::ObjectMap::iterator i =
            objects.find("context");

          if(i == objects.end())
          {
            throw Exception("no context object found in the result");
          }
          
          Python::Context* context =
            Python::Context::Type::down_cast(i->second.in());

          context->apply_restrictions();
          log = stream_->str();
        }

        void
        Interceptor::write(El::BinaryOutStream& bstr) const
          throw(El::Exception)
        {
          bstr << log << interrupted;
        }

        void
        Interceptor::read(El::BinaryInStream& bstr) throw(El::Exception)
        {
          bstr >> log >> interrupted;
        }

        //
        // Context struct
        //
        Context::Context(const ::NewsGate::RSS::HTMLFeed::Context& context,
                         El::Net::HTTP::Session* session)
          throw(Exception, El::Exception)
            : El::Python::ObjectImpl(&Type::instance),
              feed_url(context.feed_url),
              article_max_size(context.article_max_size),
              cache_dir(context.cache_dir),
              space(context.space),
              outbound_bytes(context.outbound_bytes),
              inbound_bytes(context.inbound_bytes),
              encoding(context.encoding),
              execution_end_(0),
              message_restrictions_(context.message_restrictions)
        {
          std::auto_ptr<El::Net::HTTP::Session> sess(session);
          
          init_subobj(&context);

          El::Net::HTTP::URL_var url =
            new El::Net::HTTP::URL(feed_url.c_str());

          feed_host_ = url->host();

          feed_article.reset(
            session ?
            new Feed::Automation::Article(context.feed_url.c_str(),
                                          context.request_params,
                                          context.article_max_size,
                                          context.cache_dir.c_str(),
                                          sess.release(),
                                          true,
                                          0,
                                          encoding.c_str()) :
            0);

          if(feed_article.get())
          {
            outbound_bytes += feed_article->outbound_bytes();
            inbound_bytes += feed_article->inbound_bytes();
          }
        }

        void
        Context::init_subobj(
          const ::NewsGate::RSS::HTMLFeed::Context* context)
          throw(El::Exception)
        {
          El::Python::Lang_var l = new El::Python::Lang();
          El::Python::Country_var c = new El::Python::Country();
          El::Python::Sequence_var k = new El::Python::Sequence();
          
          El::Python::Sequence_var seq = new El::Python::Sequence();

          if(context)
          {
            seq->from_container<
            ::NewsGate::Message::Automation::Python::Message,
              ::NewsGate::Message::Automation::MessageArray>(
                context->messages);

            cache_ = context->cache;

            *l = context->lang;
            *c = context->country;

            for(El::String::Array::const_iterator i(context->keywords.begin()),
                  e(context->keywords.end()); i != e; ++i)
            {
              k->push_back(PyString_FromString(i->c_str()));
            }
          }
          
          init(lang, l.retn());
          init(country, c.retn());
          init(keywords, k.retn());

          init(messages, seq.retn());

          init(request_params,
               context ? new El::Net::HTTP::Python::RequestParams(
                 context->request_params) :
               new El::Net::HTTP::Python::RequestParams(),
               true);

          init(message_restrictions,
               context ?
               new Message::Automation::Python::MessageRestrictions(
                 context->message_restrictions) :
               new Message::Automation::Python::MessageRestrictions(),
               true);
        }

        const El::Net::HTTP::Python::RequestParams*
        Context::adjust_request_params() throw(El::Exception)
        {          
          if(request_params.in() && execution_end_)
          {
            unsigned long tm = ACE_OS::gettimeofday().sec();
            
            unsigned long left =
              execution_end_ < tm ? 0 : (execution_end_ - tm);
            
            request_params->timeout = std::min(left, request_params->timeout);
          }

          return request_params.in();
        }
        
        void
        Context::apply_restrictions() throw(El::Exception)
        {
          for(El::Python::Sequence::iterator i(messages->begin()),
                e(messages->end()); i != e; ++i)
          {
            ::NewsGate::Message::Automation::Python::Message* msg =
              ::NewsGate::Message::Automation::Python::Message::Type::
              down_cast(i->in(), false, false);

            if(msg == 0)
            {
              *i = 0;
              continue;              
            }

            if(msg->valid)
            {
              normalize_msg_url(msg, "validating messages", 0);
            }

            El::Python::Sequence::iterator j(messages->begin());
            
            for(; j != i; ++j)
            {
              if(j->in() == 0)
              {
                continue;
              }
              
              ::NewsGate::Message::Automation::Python::Message* msg2 =
                ::NewsGate::Message::Automation::Python::Message::Type::
                down_cast(j->in());
              
              if(msg->url == msg2->url)
              {
                break;
              }
            }

            if(j != i)
            {
              *i = 0;
              continue;
            }
            
            std::string truncated;
          
            El::String::Manip::truncate(msg->title.c_str(),
                                        truncated,
                                        SIZE_MAX,
                                        true,
                                        El::String::Manip::UAC_XML_1_0,
                                        message_restrictions->max_title_len);

            if(msg->valid && truncated.empty())
            {
              El::String::Manip::truncate(msg->description.c_str(),
                                          truncated,
                                          100,
                                          true,
                                          El::String::Manip::UAC_XML_1_0);
            }
            
            msg->title = truncated;
              
            El::String::Manip::truncate(msg->description.c_str(),
                                        truncated,
                                        message_restrictions->max_desc_chars,
                                        true,
                                        El::String::Manip::UAC_XML_1_0,
                                        message_restrictions->max_desc_len);

            msg->description = truncated;

            if(msg->valid && msg->title.empty() && msg->description.empty())
            {
              invalidate_msg(msg,
                             "validating messages: no title, no description");
            }

            if(!msg->valid && msg->title.empty())
            {
              msg->title = msg->url;
            }

            if(msg->title == msg->description)
            {
              msg->description.clear();
            }
              
            Message::Automation::Python::Source& src = *msg->source;

            if(src.url.empty())
            {
              src.url = feed_url;
            }

            if(src.html_link.empty())
            {
              src.html_link = feed_url;
            }

            ::NewsGate::Message::Automation::ImageRestrictions& ir =
                message_restrictions_.image_restrictions;
            
            for(El::Python::Sequence::iterator i(msg->images->begin()),
                  e(msg->images->end()); i != e; ++i)
            {
              ::NewsGate::Message::Automation::Python::Image& img =
                *::NewsGate::Message::Automation::Python::Image::Type::
                  down_cast((*i).in());

              // No image download here as too slowdown the function;
              // if need it should be done explicitly in script
              img.adjust(ir, /*request_params.in()*/ 0, false);
            }

            size_t image_count = message_restrictions->max_image_count;

            for(El::Python::Sequence::iterator i(msg->images->begin()),
                  e(msg->images->end()); i != e; ++i)
            {
              ::NewsGate::Message::Automation::Python::Image& img =
                *::NewsGate::Message::Automation::Python::Image::Type::
                  down_cast((*i).in());
          
              if(img.status ==
                 ::NewsGate::Message::Automation::Image::IS_VALID)
              {
                for(El::Python::Sequence::iterator j(msg->images->begin());
                    j != i; ++j)
                {
                  ::NewsGate::Message::Automation::Python::Image& img2 =
                    *::NewsGate::Message::Automation::Python::Image::Type::
                      down_cast((*j).in());
                  
                  if(img.src == img2.src &&
                     img2.status == Message::Automation::Image::IS_VALID)
                  {
                    img.status = Message::Automation::Image::IS_DUPLICATE;
                    break;
                  }
                }
              }
          
              if(img.status != Message::Automation::Image::IS_VALID)
              {
                continue;
              }

              if(!image_count)
              {
                img.status = Message::Automation::Image::IS_TOO_MANY;
                continue;
              }          

              if(img.height == Message::Automation::Image::DIM_UNDEFINED ||
                 img.width == Message::Automation::Image::DIM_UNDEFINED)
              {
                img.status = Message::Automation::Image::IS_UNKNOWN_DIM;
                continue;
              }
          
              if(img.height < ir.min_height || img.width < ir.min_width)
              {
                img.status = Message::Automation::Image::IS_TOO_SMALL;
                continue;
              }
              
              --image_count;
            }
          }

          El::Python::Sequence_var new_messages = new El::Python::Sequence();
          new_messages->reserve(messages->size());
          
          for(El::Python::Sequence::iterator i(messages->begin()),
                e(messages->end()); i != e; ++i)
          {
            El::Python::Object_var obj = *i;

            if(obj.in())
            {
              new_messages->push_back(obj);
            }
          }

          messages->swap(*new_messages);
          cache_.cleanup();
        }

        void
        Context::write(El::BinaryOutStream& ostr) const throw(El::Exception)
        {
          ostr << messages << request_params << message_restrictions
               << message_restrictions_ << feed_url << article_max_size
               << cache_dir << feed_host_ << cache_ << (uint32_t)space
               << lang << country << keywords << execution_end_
               << outbound_bytes << inbound_bytes << encoding
               << (uint8_t)(feed_article.get() ? 1 : 0);

          if(feed_article.get())
          {
            ostr << *feed_article;
          }
        }
        
        void
        Context::read(El::BinaryInStream& istr) throw(El::Exception)
        {
          uint32_t sp = 0;
          uint8_t fa = 0;
          
          istr >> messages >> request_params >> message_restrictions
               >> message_restrictions_ >> feed_url >> article_max_size
               >> cache_dir >> feed_host_ >> cache_ >> sp >> lang >> country
               >> keywords >> execution_end_ >> outbound_bytes
               >> inbound_bytes >> encoding >> fa;

          space = (Feed::Space)sp;
          feed_article.reset(fa ? new Feed::Automation::Article() : 0);
          
          if(feed_article.get())
          {
            istr >> *feed_article;
          }
        }
          
        void
        Context::save(::NewsGate::RSS::HTMLFeed::Context& ctx) const
          throw(El::Exception)
        {
          ctx.feed_url = feed_url;
          ctx.encoding = encoding;
          ctx.space = space;
          ctx.lang = *lang;
          ctx.country = *country;
          ctx.outbound_bytes = outbound_bytes;
          ctx.inbound_bytes = inbound_bytes;

          ctx.keywords.clear();
          ctx.keywords.reserve(keywords->size());

          for(El::Python::Sequence::const_iterator i(keywords->begin()),
                e(keywords->end()); i != e; ++i)
          {
            size_t len = 0;
            
            std::string kwd =
              El::Python::string_from_string(
                i->in(),
                len,
                "::NewsGate::RSS::HTMLFeed::Context::save");

            ctx.keywords.push_back(kwd);
          }
          
          ctx.cache = cache_;

          ctx.messages.clear();
          ctx.messages.reserve(messages->size());

          for(El::Python::Sequence::const_iterator i(messages->begin()),
                e(messages->end()); i != e; ++i)
          {
            ::NewsGate::Message::Automation::Python::Message* msg =
              ::NewsGate::Message::Automation::Python::Message::Type::
              down_cast((*i).in());

            ctx.messages.push_back(
              ::NewsGate::Message::Automation::Message());
              
            ::NewsGate::Message::Automation::MessageArray::reverse_iterator r =
              ctx.messages.rbegin();

            msg->save(*r);
          }
        }

        PyObject*
        Context::normalize_url(PyObject* url,
                               bool drop_url_anchor,
                               const char* context,
                               const char* encoding)
          throw(El::Exception)
        {
          if(PyString_Check(url))
          {
            El::Python::Object_var feed_doc =
              html_doc(Py_None, context, encoding);
            
            El::LibXML::Python::Document* fd =
              El::LibXML::Python::Document::Type::down_cast(feed_doc,
                                                            false,
                                                            false);
            
            if(fd)
            {
              size_t len = 0;              
              std::string u = El::Python::string_from_string(url,len,context);
              
              u = fd->abs_url(u.c_str(), true);

              if(drop_url_anchor)
              {
                std::string::size_type pos = u.find('#');

                if(pos != std::string::npos)
                {
                  u.resize(pos);
                }                
              }
              
              return PyString_FromString(u.c_str());
            }
          }
          
          return El::Python::add_ref(url);          
        }

        PyObject*
        Context::py_html_doc(PyObject* args, PyObject* kwds)
          throw(El::Exception)
        {
          const char* kwlist[] =
            { "url",
              "encoding",
              NULL
            };

          PyObject* url = Py_None;
          const char* encoding = 0;
          
          if(!PyArg_ParseTupleAndKeywords(
               args,
               kwds,
               "|Os:newsgate.message.HF_Context.html_doc",
               (char**)kwlist,
               &url,
               &encoding))
          {
            El::Python::handle_error(
              "NewsGate::RSS::HTMLFeed::Python::Context::py_html_doc");
          }

          El::Python::Object_var new_url =
            normalize_url(url,
                          false,
                          "newsgate.message.HF_Context.html_doc",
                          encoding);
          
          return html_doc(new_url.in(),
                          "NewsGate::RSS::HTMLFeed::Python::Context::"
                          "py_html_doc",
                          encoding);
        }

        void
        Context::pre_run(El::Python::Sandbox* sandbox) throw(El::Exception)
        {
          execution_end_ = sandbox ?
            (ACE_OS::gettimeofday().sec() + sandbox->timeout().sec()) : 0;
  
          if(feed_article.get())
          {
            El::Python::Object_var doc = feed_article->document();

            feed_article->delete_file();
            feed_article.reset(0);
            
            doc =
              html_doc(doc.in(),
                       "NewsGate::RSS::HTMLFeed::Python::Context::pre_run",
                       0);
          }

          ACE_Time_Value tm(sandbox ? sandbox->timeout().sec() : 86400);
          
          robots_checker_.reset(
            new El::Net::HTTP::RobotsChecker(
              ACE_Time_Value(request_params->timeout),
              request_params->redirects_to_follow,
              tm,
              tm));
        }
        
        PyObject*
        Context::html_doc(PyObject* doc,
                          const char* context,
                          const char* charset,
                          std::string* error)
          throw(El::Exception)
        {
          if(!PyString_Check(doc) && doc != Py_None)
          {
            El::LibXML::Python::Document_var document =
              El::LibXML::Python::Document::Type::down_cast(doc, true);

            std::string url = document->url();

            if(!url.empty())
            {              
              add_doc(document.in(), url.c_str());

              El::Python::Object_var res = find_doc(url.c_str());
              assert(res.in());
              
              return res.retn();
            }

            return document.retn();
          }
          
          size_t len = 0;
          
          const char* article_url = doc == Py_None ? feed_url.c_str() :
            El::Python::string_from_string(
              doc,
              len,
              context);

          El::Python::Object_var res = find_doc(article_url);

          if(res.in())
          {
            return res.retn();
          }

          Feed::Automation::Article article(
            article_url,
            *adjust_request_params(),
            article_max_size,
            cache_dir.c_str(),
            0,
            false,
            robots_checker_.get(),
            charset && *charset != '\0' ? charset : encoding.c_str());

          res = article.document();
          add_doc(res.in(), article_url);

          outbound_bytes += article.outbound_bytes();
          inbound_bytes += article.inbound_bytes();
          
          if(error)
          {
            *error = article.error();
          }
          
          res = find_doc(article_url);
          assert(res.in());
          
          return res.retn();
        }

        PyObject*
        Context::find_doc(const char* url) throw(El::Exception)
        {
          ObjectMap::iterator i = documents_.find(url);
          return i == documents_.end() ? 0 : i->second.add_ref();
        }

        bool
        Context::overlap(const El::String::Set& s1, const El::String::Set& s2)
          throw(El::Exception)
        {
          El::String::Set::const_iterator e2(s2.end());
          
          for(El::String::Set::const_iterator i(s1.begin()), e(s1.end());
              i != e; ++i)
          {
            if(s2.find(*i) != e2)
            {
              return true;
            }
          }

          return false;
        }
          
        void
        Context::add_doc(PyObject* doc, const char* url) throw(El::Exception)
        {
          typedef std::map<PyObject*, El::String::Set> DocUrls;

          DocUrls doc_urls;
          doc_urls[doc].insert(url);

          El::LibXML::Python::Document* d =
            El::LibXML::Python::Document::Type::down_cast(doc, false, false);
          
          if(d)
          {
            const El::String::Array& urls = d->all_urls();

            for(El::String::Array::const_iterator i(urls.begin()),
                  e(urls.end()); i != e; ++i)
            {
              doc_urls[doc].insert(*i);              
//              std::cerr << "NEW: " << *i << std::endl;
            }

            El::Python::Sequence_var seq = new El::Python::Sequence();

            seq->push_back(
              new El::LibXML::Python::DocText(
                "/html/head/link[translate(@rel,'CANONICAL','canonical')='canonical']/@href"));
          
            seq->push_back(
              new El::LibXML::Python::DocText(
                "/html/head/meta[translate(@property,'OG:URL','og:url')='og:url']//@content"));

            El::Python::Object_var res = d->text(seq.in());
          
            size_t len = 0;
            std::string u = El::Python::string_from_string(res.in(), len);

            try
            {
              El::Net::HTTP::URL_var url = new El::Net::HTTP::URL(u.c_str());
              const char* canonical = url->string();
                  
              d->url(canonical);
              doc_urls[doc].insert(canonical);
            }
            catch(const El::Net::HTTP::URL::InvalidArg&)
            {
            }            
          }
          
          for(ObjectMap::iterator i(documents_.begin()), e(documents_.end());
              i != e; ++i)
          {
            doc_urls[i->second.in()].insert(i->first);          
//            std::cerr << "OLD: " << i->first << std::endl;
          }

          while(true)
          {
            bool merge = false;
            
            for(DocUrls::iterator i(doc_urls.begin()), e(doc_urls.end());
                i != e && !merge; ++i)
            {
              bool ovl = false;
              
              for(DocUrls::iterator j(doc_urls.begin()); j != i; ++j)
              {
                ovl = overlap(j->second, i->second);
              
                if(ovl)
                {
                  if(j->first != Py_None)
                  {
                    j->second.insert(i->second.begin(), i->second.end());
                    doc_urls.erase(i);
                  }
                  else
                  {
                    i->second.insert(j->second.begin(), j->second.end());
                    doc_urls.erase(j);
                  }

                  merge = true;
                  break;
                }
                
              }
            }

            if(!merge)
            {
              break;
            }
          }

          ObjectMap documents;

          for(DocUrls::iterator i(doc_urls.begin()), e(doc_urls.end());
              i != e; ++i)
          {
            El::Python::Object_var doc = El::Python::add_ref(i->first);

            El::LibXML::Python::Document* d =
              El::LibXML::Python::Document::Type::down_cast(doc, false, false);

            const El::String::Set& url_set = i->second;
              
            if(d)
            {
              d->all_urls().clear();
              
              for(El::String::Set::iterator j(url_set.begin()),
                    je(url_set.end()); j != je; ++j)
              {
                d->all_urls().push_back(*j);
              }
            }

            for(El::String::Set::iterator j(url_set.begin()),
                  je(url_set.end()); j != je; ++j)
            {
              documents[*j] = doc;
            }
          }

          swap(documents, documents_);            
        }
        
        void
        Context::normalize_msg_url(Message::Automation::Python::Message* msg,
                                   const char* context,
                                   const char* encoding)
          throw(El::Exception)
        {
          El::Python::Object_var url = PyString_FromString(msg->url.c_str());

          std::string error;
          
          El::Python::Object_var doc =
            html_doc(url, context, encoding, &error);

          if(doc == Py_None)
          {
            std::ostringstream ostr;
            ostr << context << ": document is not available for url '"
                 << msg->url << "'";

            if(!error.empty())
            {
              ostr << "; reason: " << error;
            }

            invalidate_msg(msg, ostr.str().c_str());
            return;
          }

          El::LibXML::Python::Document* d =
            El::LibXML::Python::Document::Type::down_cast(doc);
              
          std::string doc_url = d->url();

          if(doc_url.length() <=
             Message::Automation::Message::MAX_MSG_URL_LEN)
          {
            msg->url = doc_url;
          }

          if(msg->url.length() > Message::Automation::Message::MAX_MSG_URL_LEN)
          {
            std::ostringstream ostr;
            ostr << context << ": message url '"
                 << msg->url << "' length (" << msg->url.length()
                 << ") exeeds "
                 << Message::Automation::Message::MAX_MSG_URL_LEN << std::endl;

            msg->log = ostr.str();
            msg->valid = false;
          }
        }

        PyObject*
        Context::py_new_message(PyObject* args, PyObject* kwds)
          throw(El::Exception)
        {
          const char* kwlist[] =
            {
              "url",
              "title",
              "description",
              "images",
              "source",
              "space",
              "lang",
              "country",
              "keywords",
              "feed_domain_only",
              "unique_message_url",
              "unique_message_doc",
              "unique_message_title",
              "title_required",
              "description_required",
              "max_image_count",
              "drop_url_anchor",
              "save",
              "encoding",
              NULL
            };

          PyObject* url = Py_None;
          PyObject* title = Py_None;
          PyObject* desc = Py_None;
          PyObject* images = Py_None;
          PyObject* source = Py_None;
          PyObject* spc = Py_None;
          PyObject* lng = Py_None;
          PyObject* ctr = Py_None;
          PyObject* kwd = Py_None;
          unsigned char feed_domain_only = 1;
          unsigned char unique_message_url = 1;
          unsigned char unique_message_doc = 1;
          unsigned char unique_message_title = 0;
          unsigned char title_required = 1;
          unsigned char description_required = 1;
          unsigned long max_image_count = 1;
          unsigned char drop_url_anchor = 1;
          unsigned char save = 1;
          const char* encoding = 0;
          
          if(!PyArg_ParseTupleAndKeywords(
               args,
               kwds,
               "|OOOOOOOOOBBBBBBkBBs:newsgate.message.HF_Context.new_message",
               (char**)kwlist,
               &url,
               &title,
               &desc,
               &images,
               &source,
               &spc,
               &lng,
               &ctr,
               &kwd,
               &feed_domain_only,
               &unique_message_url,
               &unique_message_doc,
               &unique_message_title,
               &title_required,
               &description_required,
               &max_image_count,
               &drop_url_anchor,
               &save,
               &encoding))
          {
            El::Python::handle_error(
              "NewsGate::RSS::HTMLFeed::Python::Context::py_new_message");
          }

          Message::Automation::Python::Message_var msg =
            new Message::Automation::Python::Message();

          msg->valid = true;
          
          bool url_def = url != Py_None;
          El::Python::Object_var new_url;
          
          if(url_def)
          {
            new_url = normalize_url(url,
                                    drop_url_anchor,
                                    "newsgate.message.HF_Context.new_message",
                                    encoding);

            url = new_url.in();

            if(unique_message_url && cache_.url_present(url))
            {
              return El::Python::add_ref(Py_None);
            }
            
            size_t len = 0;
            
            msg->url =
              El::Python::string_from_string(
                url,
                len,
                "newsgate.message.HF_Context.new_message");
            
            normalize_msg_url(msg, "creating message", encoding);

            if(msg->url == feed_url)
            {
              return El::Python::add_ref(Py_None);              
            }

            if(feed_domain_only)
            {
              try
              {
                El::Net::HTTP::URL_var url =
                  new El::Net::HTTP::URL(msg->url.c_str());

                if(feed_host_ != url->host() &&
                   !El::Net::subdomain(feed_host_.c_str(), url->host()))
                {
                  invalidate_msg(msg,
                                 "domain differes from that of the feed",
                                 save);
                  
                  return msg.retn();
                }
              }
              catch(...)
              {
                  invalidate_msg(msg, "can't detect domain", save);
                  return msg.retn();
              }
            }

            El::Python::Object_var url = PyString_FromString(msg->url.c_str());

            El::Python::Object_var doc =
              html_doc(url,
                       "newsgate.message.HF_Context.new_message",
                       encoding);

            El::LibXML::Python::Document* d =
              El::LibXML::Python::Document::Type::down_cast(doc,
                                                            false,
                                                            false);
            if(d)
            {
              El::String::Set urls;
              uint64_t crc = d->crc();

              if(unique_message_url)
              {
                for(El::String::Array::const_iterator i(d->all_urls().begin()),
                      e(d->all_urls().end()); i != e; ++i)
                {
                  const char* url = i->c_str();
                  urls.insert(url);
                  cache_.add_url(url);
                }
              }
              
              if(unique_message_doc && crc && cache_.doc_present(crc))
              {
                invalidate_msg(msg, "document duplicate", save);
                return msg.retn();
              }
              
              for(El::Python::Sequence::iterator i(messages->begin()),
                    e(messages->end()); i != e; ++i)
              {
                ::NewsGate::Message::Automation::Python::Message* m =
                  ::NewsGate::Message::Automation::Python::Message::Type::
                  down_cast(i->in(), false, false);

                if(m == 0)
                {
                  continue;
                }
                
                El::Python::Object_var url =
                  PyString_FromString(m->url.c_str());
              
                El::Python::Object_var doc =
                  html_doc(url,
                           "newsgate.message.HF_Context.new_message",
                           encoding);

                El::LibXML::Python::Document* d =
                  El::LibXML::Python::Document::Type::down_cast(doc,
                                                                false,
                                                                false);
                
                if(d)
                {
                  if(unique_message_doc && crc && crc == d->crc())
                  {
                    return El::Python::add_ref(Py_None);
                  }

                  if(unique_message_url)
                  {
                    for(El::String::Array::const_iterator
                          i(d->all_urls().begin()), e(d->all_urls().end());
                        i != e; ++i)
                    {
                      if(urls.find(*i) != urls.end())
                      {
                        return El::Python::add_ref(Py_None);
                      }
                    }
                  }
                }
              }
            }
          }
          
          if(PyString_Check(title))
          {
            size_t len = 0;

            msg->title = 
              El::Python::string_from_string(
                title,
                len,
                "newsgate.message.HF_Context.new_message");
          }
          else if(url_def)
          {
            if(title == Py_None)
            {
              msg->title = title_from_doc(url, encoding);
            }
            else
            {
              El::Python::Object_var t = text_from_doc(title, url, encoding);
              
              size_t len = 0;
              std::string title = El::Python::string_from_string(t.in(), len);
              El::String::Manip::trim(title.c_str(), msg->title);
            }
          }

          if(unique_message_title && !msg->title.empty())
          {
            if(cache_.title_present(msg->title.c_str()))
            {
              return El::Python::add_ref(Py_None);
            }
            
            for(El::Python::Sequence::iterator i(messages->begin()),
                  e(messages->end()); i != e; ++i)
            {
              ::NewsGate::Message::Automation::Python::Message* m =
                ::NewsGate::Message::Automation::Python::Message::Type::
                down_cast(i->in(), false, false);
              
              if(m && m->title == msg->title)
              {
                invalidate_msg(msg, "title duplicate", save);
                return msg.retn();
              }
            }
          }

          if(title_required && msg->title.empty())
          {
            invalidate_msg(msg, "creating message: no title");
          }
            
          if(PyString_Check(desc))
          {
            size_t len = 0;
            
            msg->description =
              El::Python::string_from_string(
                desc,
                len,
                "newsgate.message.HF_Context.new_message");
          }
          else if(url_def)            
          {
            if(desc == Py_None)
            {
              msg->description = description_from_doc(url, encoding);
            }
            else
            {
              El::Python::Object_var t = text_from_doc(desc, url, encoding);
              
              size_t len = 0;
              std::string desc = El::Python::string_from_string(t.in(), len);
              El::String::Manip::trim(desc.c_str(), msg->description);
            }

            if(msg->title == msg->description)
            {
              msg->description.clear();
            }
          }

          if(description_required && msg->description.empty())
          {            
            invalidate_msg(msg, "creating message: no description");

            if(unique_message_title)
            {
              cache_.title_erase(msg->title.c_str());
            }            
          }

          if(source == Py_None)
          {
            Message::Automation::Python::Source& src = *msg->source;

            src.url = feed_url;
            src.html_link = feed_url;
            src.title = title_from_doc(Py_None, encoding);
          }
          else
          {
            msg->source =
              Message::Automation::Python::Source::Type::down_cast(source,
                                                                   true);
          }

          if(images != Py_None)
          {
            int len = PySequence_Size(images);

            for(int i = 0; i < len; ++i)
            {
              El::Python::Object_var obj = PySequence_GetItem(images, i); 

              ::NewsGate::Message::Automation::Python::Image* img =
                  ::NewsGate::Message::Automation::Python::Image::Type::
                    down_cast(obj.in(), true);
              
              msg->images->push_back(img);
            }
          }
          else if(!msg->url.empty() && max_image_count)
          {
            El::Python::Object_var url = PyString_FromString(msg->url.c_str());

            El::Python::Object_var d =
              html_doc(url.in(),
                       "newsgate.message.HF_Context.new_message",
                       encoding);

            El::Python::Object_var max_count =
              PyLong_FromLongLong(max_image_count);
            
            Message::Automation::Python::Image::images_from_doc(
              msg->images.in(),
              d.in(),
              Message::Automation::OP_ADD_BEGIN,
              "/html/head/meta[translate(@property, 'OG:IMAGE', 'og:image')='og:image']",
              Py_True,
              Py_None,
              Py_None,
              Py_False,
              Py_None,
              Py_None,
              Py_None,
              Py_None,
              max_count.in(),
              message_restrictions_.image_restrictions,
              adjust_request_params(),
              execution_end_,
              "newsgate.message.HF_Context.new_message");

            Message::Automation::Python::Image::images_from_doc(
              msg->images.in(),
              d.in(),
              Message::Automation::OP_ADD_TO_EMPTY,
              "/html/head/link[translate(@rel, 'IMAGE_SRC', 'image_src')='image_src']",
              Py_True,
              Py_None,
              Py_None,
              Py_False,
              Py_None,
              Py_None,
              Py_None,
              Py_None,
              max_count.in(),
              message_restrictions_.image_restrictions,
              adjust_request_params(),
              execution_end_,
              "newsgate.message.HF_Context.new_message");
            
            {
              std::ostringstream ostr;
              ostr << "//img[contains(normalize-space(";
              
              El::String::Manip::xpath_escape(
                title_from_doc(url, encoding).c_str(),
                '\'',
                ostr);

              ostr << "), normalize-space(@alt)) and string-length(normalize-space(@alt))>10]";
              
              Message::Automation::Python::Image::images_from_doc(
                msg->images.in(),
                d.in(),
                Message::Automation::OP_ADD_TO_EMPTY,
                ostr.str().c_str(),
//                Py_True,
                Py_None,
                Py_None,
                Py_None,
                Py_False,
                Py_None,
                Py_None,
                Py_None,
                Py_None,
                max_count.in(),
                message_restrictions_.image_restrictions,
                adjust_request_params(),
                execution_end_,
                "newsgate.message.HF_Context.new_message");
            }
            
            {
              El::Python::Object_var min_width = PyLong_FromLongLong(200);
              El::Python::Object_var min_height = PyLong_FromLongLong(200);
                
              Message::Automation::Python::Image::images_from_doc(
                msg->images.in(),
                d.in(),
                Message::Automation::OP_ADD_TO_EMPTY,
                "//img",
//                Py_True,
                Py_None,
                Py_None,
                Py_None,
                Py_False,
                min_width.in(),
                min_height.in(),
                Py_None,
                Py_None,
                max_count.in(),
                message_restrictions_.image_restrictions,
                adjust_request_params(),
                execution_end_,
                "newsgate.message.HF_Context.new_message");
            }
          }

          if(spc == Py_None)
          {
            msg->space = space;
          }
          else
          {
            unsigned long val =
              El::Python::ulong_from_number(
                spc,
                "newsgate.message.HF_Context.new_message");
            
            msg->space = NewsGate::Feed::space(val);
          }

          if(lng == Py_None)
          {
            *(msg->lang) = *lang;
          }
          else
          {
            El::Python::Lang* val = El::Python::Lang::Type::down_cast(lng);
            *(msg->lang) = *val;
          }
          
          if(ctr == Py_None)
          {
            *(msg->country) = *country;
          }
          else
          {
            El::Python::Country* val =
              El::Python::Country::Type::down_cast(ctr);
            
            *(msg->country) = *val;
          }

          if(kwd == Py_None)
          {
            *(msg->keywords) = *keywords;
          }
          else
          {
            msg->keywords->from_sequence(kwd);
          }

          if(save)
          {
            messages->push_back(msg);
          }
          
          return msg.retn();
        }

        void
        Context::invalidate_msg(Message::Automation::Python::Message* msg,
                                const char* error,
                                bool save)
          throw(El::Exception)
        {
          msg->valid = false;

          if(error && *error != '\0')
          {
            if(!msg->log.empty())
            {
              msg->log += "\n";
            }

            msg->log += error;
          }

          if(save)
          {
            messages->push_back(El::Python::add_ref(msg));
          }
        }
        
        PyObject*
        Context::py_images_from_doc(PyObject* args, PyObject *kwds)
          throw(El::Exception)
        {
          const char* kwlist[] =
            { "images",
              "doc",
              "op",
              "img_xpath",
              "read_size",
              "check_ext",
              "ancestor_xpath",
              "alt_xpath",
              "min_width",
              "min_height",
              "max_width",
              "max_height",
              "max_count",
              "encoding",
              NULL
          };
          
          PyObject* images = Py_None;
          PyObject* document = Py_None;
          unsigned long op = Message::Automation::OP_REPLACE;
          const char* img_xpath = "//img";
          PyObject* read_size = Py_False;
          PyObject* check_ext = Py_None; // Autodecision
          PyObject* ancestor_xpath = Py_None;
          PyObject* alt_xpath = Py_None;
          PyObject* min_width = Py_None;
          PyObject* min_height = Py_None;
          PyObject* max_width = Py_None;
          PyObject* max_height = Py_None;
          PyObject* max_count = Py_None;
          const char* encoding = 0;
            
          if(!PyArg_ParseTupleAndKeywords(
               args,
               kwds,
               "|OOksOOOOOOOOOs:newsgate.message.HF_Context.images_from_doc",
               (char**)kwlist,
               &images,
               &document,
               &op,
               &img_xpath,
               &read_size,
               &check_ext,
               &ancestor_xpath,
               &alt_xpath,
               &min_width,
               &min_height,
               &max_width,
               &max_height,
               &max_count,
               &encoding))
          {
            El::Python::handle_error(
              "NewsGate::RSS::HTMLFeed::Python::Context::py_images_from_doc");
          }

          El::Python::Sequence_var new_images;
          
          if(images == Py_None)
          {
            new_images = new El::Python::Sequence();
            images = new_images.in();
          }

          El::Python::Object_var d =
            html_doc(document,
                     "NewsGate::RSS::HTMLFeed::Python::Context::"
                     "py_images_from_doc",
                     encoding);          

          ::NewsGate::Message::Automation::Python::Image::images_from_doc(
            images,
            d.in(),
            op,
            img_xpath,
            read_size,
            check_ext,
            ancestor_xpath,
            alt_xpath,
            min_width,
            min_height,
            max_width,
            max_height,
            max_count,
            message_restrictions_.image_restrictions,
            adjust_request_params(),
            execution_end_,
            "newsgate.message.HF_Context.images_from_doc");

          return El::Python::add_ref(images);
        }

        PyObject*
        Context::py_keywords_from_doc(PyObject* args, PyObject *kwds)
          throw(El::Exception)
        {
          const char* kwlist[] =
            { "keywords",
              "doc",
              "xpath",
              "op",
              "separators",
              "keyword_override",
              NULL
            };

          PyObject* keywords = Py_None;
          PyObject* document = Py_None;

          const char* xpath =
            "/html/head/meta[translate(@name, 'KEYWORDS', 'keywords')="
            "'keywords']/@content";
          
          unsigned long op = Message::Automation::OP_ADD_BEGIN;
          const char* separators = ",";
          PyObject* keyword_override = Py_None;
            
          if(!PyArg_ParseTupleAndKeywords(
               args,
               kwds,
               "|OOsksO:newsgate.message.HF_Context.keywords_from_doc",
               (char**)kwlist,
               &keywords,
               &document,
               &xpath,
               &op,
               &separators,
               &keyword_override))
          {
            El::Python::handle_error(
              "NewsGate::RSS::HTMLFeed::Python::Context::"
              "py_keywords_from_doc");
          }

          El::Python::Sequence_var new_keywords;
          
          if(keywords == Py_None)
          {
            new_keywords = new El::Python::Sequence();
            keywords = new_keywords.in();
          }          

          ::NewsGate::Message::Automation::Python::Message::keywords_from_doc(
            keywords,
            document,
            xpath,
            op,
            separators,
            keyword_override,
            "newsgate.message.HF_Context.keywords_from_doc");
          
          return El::Python::add_ref(keywords);
        }
        
        PyObject*
        Context::py_text_from_doc(PyObject* args, PyObject* kwds)
          throw(El::Exception)
        {
          const char* kwlist[] =
            { "xpaths",
              "doc",
              "encoding",
              NULL
            };

          PyObject* xpaths = 0;
          PyObject* doc = Py_None;
          const char* encoding = 0;
          
          if(!PyArg_ParseTupleAndKeywords(
               args,
               kwds,
               "O|Os:newsgate.message.HF_Context.text_from_doc",
               (char**)kwlist,
               &xpaths,
               &doc,
               &encoding))
          {
            El::Python::handle_error(
              "NewsGate::RSS::HTMLFeed::Python::Context::py_text_from_doc");
          }

          return text_from_doc(xpaths, doc, encoding);
        }

        PyObject*
        Context::text_from_doc(PyObject* text_doc_seq,
                               PyObject* doc,
                               const char* encoding)
          throw(El::Exception)
        {
          El::Python::Object_var d =
            html_doc(doc,
                     "NewsGate::RSS::HTMLFeed::Python::Context::"
                     "py_text_from_doc",
                     encoding);

          if(d.in() == Py_None)
          {
            return PyString_FromString("");
          }

          El::LibXML::Python::Document* document =
            El::LibXML::Python::Document::Type::down_cast(d.in());

          El::Python::Sequence_var seq;

          El::LibXML::Python::DocText* dc =
            El::LibXML::Python::DocText::Type::down_cast(text_doc_seq,
                                                         false,
                                                         false);
          if(dc)
          {
            seq = new El::Python::Sequence();
            seq->push_back(El::Python::add_ref(dc));
            text_doc_seq = seq.in();
          }
          else if(PyString_Check(text_doc_seq))
          {
            size_t len = 0;
            
            std::string xpath =
              El::Python::string_from_string(text_doc_seq, len);
            
            seq = new El::Python::Sequence();
            seq->push_back(new El::LibXML::Python::DocText(xpath.c_str()));
            text_doc_seq = seq.in();            
          }
          
          return document->text(text_doc_seq);
        }
      
        std::string
        Context::title_from_doc(PyObject* doc, const char* encoding)
          throw(El::Exception)
        {
          El::Python::Sequence_var seq = new El::Python::Sequence();

          seq->push_back(
            new El::LibXML::Python::DocText(
              "/html/head/meta[translate(@property, 'OG:TITLE', 'og:title')='og:title']//@content"));
          
          seq->push_back(new El::LibXML::Python::DocText("/html/head/title"));

          El::Python::Object_var res = text_from_doc(seq.in(), doc, encoding);
          
          size_t len = 0;
          std::string title = El::Python::string_from_string(res.in(), len);

          std::string trimmed;
          El::String::Manip::trim(title.c_str(), trimmed);
          return trimmed;
        }

        std::string
        Context::description_from_doc(PyObject* doc, const char* encoding)
          throw(El::Exception)
        {
          El::Python::Sequence_var seq = new El::Python::Sequence();

          seq->push_back(
            new El::LibXML::Python::DocText(
              "/html/head/meta[translate(@property, 'OG:DESCRIPTION', 'og:description')='og:description']//@content"));
          
          seq->push_back(
            new El::LibXML::Python::DocText(
              "/html/head/meta[translate(@name, 'DESCRIPTION', 'description')='description']//@content"));

          El::Python::Object_var res = text_from_doc(seq.in(), doc, encoding);
          
          size_t len = 0;
          std::string desc = El::Python::string_from_string(res.in(), len);

          seq->clear();

          seq->push_back(
            new El::LibXML::Python::DocText(
              "/html/head/meta[translate(@name, 'KEYWORDS', 'keywords')='keywords']//@content"));

          res = text_from_doc(seq.in(), doc, encoding);
          
          len = 0;
          std::string kwds = El::Python::string_from_string(res.in(), len);

          if(desc == kwds)
          {
            return "";
          }

          std::string trimmed;
          El::String::Manip::trim(desc.c_str(), trimmed);
          return trimmed;
        }
      }
      
      //
      // Code class
      //
      void
      Code::compile(const char* value,
                    unsigned long feed_id,
                    const char* feed_url)
        throw(Exception, El::Exception)
      {
        std::string feed_name;
        
        {
          std::ostringstream ostr;
          ostr << "feed ";
          
          if(feed_id)
          {
            ostr << feed_id << "; ";
          }
          
          ostr << feed_url;
          feed_name = ostr.str();
        }

        El::Python::EnsureCurrentThread guard;
        code_.compile(value ? value : "", feed_name.c_str());
      }

      void
      Code::run(Context& context,
                El::Net::HTTP::Session* session,
                El::Python::SandboxService* sandbox_service,
                El::Python::Sandbox* sandbox,
                El::Logging::Logger* logger) const
        throw(Exception, El::Exception)
      {
        std::auto_ptr<El::Net::HTTP::Session> sess(session);
        
        El::Python::EnsureCurrentThread guard;
        
        try
        {
          Python::Interceptor_var interceptor = new Python::Interceptor();
          
          Python::Context_var py_context =
            new Python::Context(context, sess.release());

          El::Python::SandboxService::ObjectMap objects;
          objects["context"] = py_context;

          bool interrupted = false;
          El::Python::ExecutionInterrupted interrupt("");
            
          try
          {
            El::Python::Object_var result =
              sandbox_service->run(code_,
                                   sandbox,
                                   &objects,
                                   interceptor.in(),
                                   0,
                                   false);
          }
          catch(const El::Python::ExecutionInterrupted& e)
          {
            interrupted = true;
            interrupt = e;
          }

          if(!interceptor->log.empty())
          {
            logger->raw_log(interceptor->log);
          }

          El::Python::SandboxService::ObjectMap::iterator i =
            objects.find("context");

          if(i == objects.end())
          {
            throw Exception("no context object found in the result");
          }
          
          py_context = Python::Context::Type::down_cast(i->second.in(), true);

          context.interrupted = interceptor->interrupted;
          py_context->save(context);

          if(interrupted)
          {
            throw interrupt;
          }
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::RSS::HTMLFeed::Code::run: "
            "El::Exception caught. Description:\n" << e;
          
          throw Exception(ostr.str());
        } 
      }
      
    }
  }
}
