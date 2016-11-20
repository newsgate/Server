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
#include <string>

#include <frameobject.h>
#include <traceback.h>
//#include <marshal.h>

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

#include "MsgAdjustment.hpp"

namespace NewsGate
{
  namespace RSS
  {
    namespace MsgAdjustment
    {
      //
      // Context class
      //
      typedef std::auto_ptr<El::Net::HTTP::Session> HTTPSessionPtr;

      bool
      Context::fill(
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
        const Message::Automation::StringArray& image_black_list,
        const Message::Automation::StringSet& image_extensions,
        size_t max_title_len,
        size_t max_desc_len,
        size_t max_desc_chars,
        size_t max_image_count)
        throw(Exception, El::Exception)
      {
        encoding = enc ? enc : "";
        
        request_params.user_agent = user_agent;
        request_params.referer = image_referrer;
        request_params.timeout = timeout.sec();
        request_params.redirects_to_follow = follow_redirects;
        request_params.recv_buffer_size = image_recv_buffer_size;
        request_params.interceptor = interceptor;

        message_restrictions.max_title_len = max_title_len;
        message_restrictions.max_desc_len = max_desc_len;
        message_restrictions.max_desc_chars = max_desc_chars;
        message_restrictions.max_image_count = max_image_count;
        
        Message::Automation::ImageRestrictions& image_restrictions =
          message_restrictions.image_restrictions;
        
        image_restrictions.min_width = image_min_width;
        image_restrictions.min_height = image_min_height;
        image_restrictions.black_list = image_black_list;
        image_restrictions.extensions = image_extensions;

        El::String::Manip::compact(channel_item.description.c_str(),
                                   item.description);
        
        El::String::Manip::compact(channel_item.title.c_str(),
                                   item.title);
        
        Message::Automation::Source& source = message.source;
        Message::Automation::ImageArray& images = message.images;
        
        source.title = channel.title;
        source.html_link = channel.html_link;
        source.url = feed_url;

        message.valid = false;
        message.space = space;
        message.url = channel_item.url;
        message.lang = channel.lang;
        message.country = channel.country;

        std::ostringstream ostr;
        
        try
        {
          El::HTML::LightParser title_parser;
        
          std::wstring title;
          El::String::Manip::utf8_to_wchar(channel_item.title.c_str(), title);

          title_parser.parse(title.c_str(),
                             feed_url,
                             El::HTML::LightParser::PF_LAX,
                             max_title_len);

          El::String::Manip::compact(title_parser.text.c_str(),
                                     message.title);
        }
        catch(const El::Exception& e)
        {
          ostr << "title parsing error:\n" << e
               << "\nCode: '" << channel_item.code.string()
               << "'\nTitle: '" << channel_item.title << "'\n";
        }
        
        El::HTML::LightParser desc_parser;
        
        try
        {          
          std::wstring desc;
          El::String::Manip::utf8_to_wchar(channel_item.description.c_str(),
                                           desc);
          
          desc_parser.parse(desc.c_str(),
                            feed_url,
                            El::HTML::LightParser::PF_LAX |
                            El::HTML::LightParser::PF_PARSE_IMAGES |
                            El::HTML::LightParser::PF_PARSE_LINKS,
                            max_desc_len,
                            max_desc_chars);

          El::String::Manip::compact(desc_parser.text.c_str(),
                                     message.description);          
        }
        catch(const El::Exception& e)
        {
          ostr << "description parsing error:\n" << e
               << "\nCode: '" << channel_item.code.string()
               << "'\nTitle: '" << channel_item.title.c_str()
               << "'\nDescription: '" << channel_item.description.c_str()
               << "'\n";
        }

        ACE_Time_Value end_time = ACE_OS::gettimeofday() + timeout;
        
        for(El::HTML::LightParser::ImageArray::const_iterator
              i(desc_parser.images.begin()), e(desc_parser.images.end());
            i != e && ACE_OS::gettimeofday() <= end_time; ++i)
        {
          ::NewsGate::Message::Automation::Image im(
            *i,
            ::NewsGate::Message::Automation::Image::IO_DESC_IMG,
            ::NewsGate::Message::Automation::Image::IS_VALID);

          if(!image_present(im, images))
          {
            im.adjust(image_restrictions, &request_params, false);
            images.push_back(im);
          }
        }

        El::Net::HTTP::URL_var source_url = new El::Net::HTTP::URL(feed_url);
      
        for(EnclosureArray::const_iterator i(channel_item.enclosures.begin()),
              e(channel_item.enclosures.end()); i != e &&
              ACE_OS::gettimeofday() <= end_time; ++i)
        {
          const RSS::Enclosure& enc = *i;
          
          if(strncmp(enc.type.c_str(), "image/", 6) == 0)
          {
            try
            {
              std::string url = source_url->abs_url(enc.url.c_str());
              
              ::NewsGate::Message::Automation::Image
                  im(url.c_str(),
                     0,
                     ::NewsGate::Message::Automation::Image::IO_ENCLOSURE);

              if(!image_present(im, images))
              { 
                im.adjust(image_restrictions, &request_params, false);
                images.push_back(im);
              }
            }
            catch(...)
            {
            }
          }
        }

        for(El::HTML::LightParser::LinkArray::const_iterator
              i(desc_parser.links.begin()), e(desc_parser.links.end());
            i != e && ACE_OS::gettimeofday() <= end_time; ++i)
        {
          try
          {
            std::string img_url = source_url->abs_url(i->url.c_str());

            ::NewsGate::Message::Automation::Image
                im(img_url.c_str(),
                   0,
                   ::NewsGate::Message::Automation::Image::IO_DESC_LINK);

            if(!image_present(im, images))
            { 
              im.adjust(image_restrictions, &request_params, true);

              if(im.status !=
                 ::NewsGate::Message::Automation::Image::IS_BAD_EXTENSION)
              {
                images.push_back(im);
              }
            }
          }
          catch(...)
          {
          }
        }        

        HTTPSessionPtr article_session(
          normalize_message_url(message.url,
                                interceptor,
                                ostr));

        El::String::Manip::trim(ostr.str().c_str(), message.log);

        if(article_session.get())
        {
          message.valid = true;

          // Session can be created but closed if schema (like https) is
          // unsupported. This way normalize_message_url reports that link
          // is ok, but we can't go inside the document
          if(article_session->opened())
          {
            item.article_session.reset(article_session.release());
          }
        }
        
        return message.valid;
      }

      El::Net::HTTP::Session*
      Context::normalize_message_url(
        std::string& message_url,
        El::Net::HTTP::Session::Interceptor* interceptor,
        std::ostream& log)
        throw(Exception, El::Exception)
      {
        try
        {
          El::Net::HTTP::URL_var url =
            new El::Net::HTTP::URL(message_url.c_str());

          El::Net::HTTP::HeaderList headers;

          headers.add(El::Net::HTTP::HD_ACCEPT, "*/*");
          headers.add(El::Net::HTTP::HD_ACCEPT_ENCODING, "gzip,deflate");
          headers.add(El::Net::HTTP::HD_ACCEPT_LANGUAGE, "en-us,*");
          
          headers.add(El::Net::HTTP::HD_USER_AGENT,
                      request_params.user_agent.c_str());

          HTTPSessionPtr session(
            new El::Net::HTTP::Session(url,
                                       El::Net::HTTP::HTTP_1_1,
                                       interceptor));

          ACE_Time_Value timeout(request_params.timeout);
          session->open(&timeout, &timeout, &timeout);
            
          std::string new_permanent_url;

          try
          {
            new_permanent_url =
              session->send_request(El::Net::HTTP::GET,
                                    El::Net::HTTP::ParamList(),
                                    headers,
                                    0,
                                    0,
                                    request_params.redirects_to_follow);
          }
          catch(const El::Net::HTTP::UnsupportedSchema&)
          {
            session->close();
          }
          
          if(!new_permanent_url.empty())
          {
            if(new_permanent_url.length() >
               Message::Automation::Message::MAX_MSG_URL_LEN)
            {
              log << "normalizing message url: new location '"
                  << new_permanent_url << "' length ("
                  << new_permanent_url.length() << ") exeeds "
                  << Message::Automation::Message::MAX_MSG_URL_LEN
                  << "; old url left\n";
            }
            else
            {
              message_url = new_permanent_url;
            }
          }

          if(message_url.length() >
             Message::Automation::Message::MAX_MSG_URL_LEN)
          {
            log << "normalizing message url error: url '"
                << message_url << "' length (" << message_url.length()
                << ") exeeds "
                << Message::Automation::Message::MAX_MSG_URL_LEN << std::endl;
            
            return 0;
          }

          if(session->opened())
          {
            session->recv_response_status();

            if(session->status_code() >= El::Net::HTTP::SC_BAD_REQUEST)
            {
              log << "normalizing message url error: unexpected response code "
                  << session->status_code() << " for " << message_url
                  << std::endl;
              
              return 0;
            }
          }

          return session.release();
        }
        catch(const El::Exception& e)
        {
          log << "normalizing message url error: for RSS item link '"
              << message_url << "' can't validate. Error description:\n"
              << e << std::endl;
          
          return 0;
        }
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
        }
  
        void
        Interceptor::post_run(
          El::Python::SandboxService::ExceptionType exception_type,
          El::Python::SandboxService::ObjectMap& objects,
          El::Python::Object_var& result)
          throw(El::Exception)
        {
          objects.erase("logger");
          logger_.reset(0);

          log = stream_->str();
        }

        void
        Interceptor::write(El::BinaryOutStream& bstr) const
          throw(El::Exception)
        {
          bstr << log;
        }

        void
        Interceptor::read(El::BinaryInStream& bstr) throw(El::Exception)
        {
          bstr >> log;
        }

        //
        // Context struct
        //
        Context::Context(
          const ::NewsGate::RSS::MsgAdjustment::Context& context)
          throw(Exception, El::Exception)
            : El::Python::ObjectImpl(&Type::instance),
              encoding(context.encoding),
              message_restrictions_(context.message_restrictions)
        {
          init_subobj(&context);
        }

        void
        Context::init_subobj(
          const ::NewsGate::RSS::MsgAdjustment::Context* context)
          throw(El::Exception)
        {
          init(message,
               context ? new Message::Automation::Python::Message(
                 context->message) :
               new Message::Automation::Python::Message());

          init(item,
               context ?
               new Feed::Automation::Python::Item(
                 context->item,
                 context->request_params,
                 context->item_article_requested) :
               new Feed::Automation::Python::Item());

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

        void
        Context::write(El::BinaryOutStream& ostr) const throw(El::Exception)
        {
          ostr << message << item << request_params << message_restrictions
               << message_restrictions_ << encoding;
        }
        
        void
        Context::read(El::BinaryInStream& istr) throw(El::Exception)
        {
          istr >> message >> item >> request_params >> message_restrictions
               >> message_restrictions_ >> encoding;
        }

        void
        Context::apply_restrictions() throw(El::Exception)
        {
          std::string truncated;
          
          El::String::Manip::truncate(message->title.c_str(),
                                      truncated,
                                      SIZE_MAX,
                                      true,
                                      El::String::Manip::UAC_XML_1_0,
                                      message_restrictions_.max_title_len);

          message->title = truncated;
          
          El::String::Manip::truncate(message->description.c_str(),
                                      truncated,
                                      message_restrictions_.max_desc_chars,
                                      true,
                                      El::String::Manip::UAC_XML_1_0,
                                      message_restrictions_.max_desc_len);

          message->description = truncated;
        }
        
        void
        Context::save(::NewsGate::RSS::MsgAdjustment::Context& ctx) const
          throw(El::Exception)
        {
          message->save(ctx.message);
          item->save(ctx.item);
          ctx.item_article_requested = item->article_requested;
          ctx.encoding = encoding;
        }

        PyObject*
        Context::py_find_in_article(PyObject* args, PyObject* kwds)
          throw(El::Exception)
        {
          const char* kwlist[] =
            { "xpath",
              "encoding",
              NULL
            };

          const char* xpath = 0;
          const char* enc = 0;
          
          if(!PyArg_ParseTupleAndKeywords(
               args,
               kwds,
               "s|s:newsgate.message.MA_Context.find_in_article",
               (char**)kwlist,
               &xpath,
               &enc))
          {
            El::Python::handle_error(
              "NewsGate::RSS::MsgAdjustment::Python::Context::"
              "py_find_in_article");
          }

          El::Python::Object_var doc =
            item->article_doc(enc && *enc != '\0' ? enc : encoding.c_str());

          if(doc.in() == Py_None)
          {
            return new El::Python::Sequence();
          }
          
          El::LibXML::Python::Document* document =
            El::LibXML::Python::Document::Type::down_cast(doc.in());

          return document ? document->find(xpath) : new El::Python::Sequence();
        }

        PyObject*
        Context::py_set_keywords(PyObject* args, PyObject* kwds)
          throw(El::Exception)
        {
          const char* kwlist[] =
            { "xpath",
              "op",
              "separators",
              "keywords",
              "encoding",
              NULL
            };

          char* xpath = 0;
          unsigned long op = Message::Automation::OP_ADD_BEGIN;
          const char* separators = ",";
          PyObject* keywords = Py_None;
          const char* enc = 0;
            
          if(!PyArg_ParseTupleAndKeywords(
               args,
               kwds,
               "s|ksOs:newsgate.message.MA_Context.set_keywords",
               (char**)kwlist,
               &xpath,
               &op,
               &separators,
               &keywords,
               &enc))
          {
            El::Python::handle_error(
              "NewsGate::RSS::MsgAdjustment::Python::Context::"
              "py_set_keywords");
          }
          
          El::Python::Object_var doc =
            item->article_doc(enc && *enc != '\0' ? enc : encoding.c_str());

          ::NewsGate::Message::Automation::Python::Message::keywords_from_doc(
            message->keywords.in(),
            doc.in(),
            xpath,
            op,
            separators,
            keywords,
            "newsgate.message.MA_Context.set_keywords");
          
          return El::Python::add_ref(Py_None);
        }
        
        PyObject*
        Context::py_skip_image(PyObject* args, PyObject* kwds)
          throw(El::Exception)
        {
          const char* kwlist[] =
            { "origin",
              "others",
              "min_width",
              "min_height",
              "max_width",
              "max_height",
              "reverse",
              NULL
          };
          
          PyObject* orig = Py_None;
          unsigned char others = 0;
          PyObject* min_width = Py_None;
          PyObject* min_height = Py_None;
          PyObject* max_width = Py_None;
          PyObject* max_height = Py_None;
          unsigned char reverse = 0;
            
          if(!PyArg_ParseTupleAndKeywords(
               args,
               kwds,
               "|OBOOOOB:newsgate.message.MA_Context.skip_image",
               (char**)kwlist,
               &orig,
               &others,
               &min_width,
               &min_height,
               &max_width,
               &max_height,
               &reverse))
          {
            El::Python::handle_error(
              "NewsGate::RSS::MsgAdjustment::Python::Context::"
              "py_skip_image");
          }

          bool skip_others = others != 0;

          Message::Automation::Image::Origin origin_val =
            Message::Automation::Image::IO_UNKNOWN;
          
          if(orig == Py_None)
          {
            orig = 0;
          }
          else
          {
            origin_val =
              cast_origin(El::Python::ulong_from_number(
                            orig,
                            "newsgate.message.MA_Context.skip_image"),
                          "newsgate.message.MA_Context.skip_image");
          }

          unsigned long min_img_width = ULONG_MAX;
          unsigned long max_img_width = 0;
          unsigned long min_img_height = ULONG_MAX;
          unsigned long max_img_height = 0;
            
          for(El::Python::Sequence::iterator i(message->images->begin()),
                e(message->images->end()); i != e; ++i)
          {
            Message::Automation::Python::Image* img =
              Message::Automation::Python::Image::Type::down_cast(i->in());

            if(img->status != Message::Automation::Image::IS_VALID)
            {
              continue;
            }

            if(orig == 0 || (img->origin == origin_val) != skip_others)
            {
              if(img->width != Message::Automation::Image::DIM_UNDEFINED)
              {
                max_img_width = std::max(img->width, max_img_width);
                min_img_width = std::min(img->width, min_img_width);
              }
              
              if(img->height != Message::Automation::Image::DIM_UNDEFINED)
              {
                max_img_height = std::max(img->height, max_img_height);
                min_img_height = std::min(img->height, min_img_height);
              }
            }
          }

          for(El::Python::Sequence::iterator i(message->images->begin()),
                e(message->images->end()); i != e; ++i)
          {
            Message::Automation::Python::Image* img =
              Message::Automation::Python::Image::Type::down_cast(i->in());

            if(img->status != Message::Automation::Image::IS_VALID)
            {
              continue;
            }            

            bool match =
              (orig == 0 || (img->origin == origin_val) != skip_others) &&
              !((min_width == Py_True && img->width != min_img_width) ||
                (min_width != Py_None && min_width != Py_True && img->width <
                 El::Python::ulong_from_number(
                   min_width,
                   "newsgate.message.MA_Context.skip_image: "
                   "invalid min_width")) ||
                
                 (max_width == Py_True && img->width != max_img_width) ||
                 (max_width != Py_None && max_width != Py_True && img->width >
                 El::Python::ulong_from_number(
                   max_width,
                   "newsgate.message.MA_Context.skip_image: "
                   "invalid max_width")) ||
                
                (min_height == Py_True && img->height != min_img_height) ||
                (min_height != Py_None && min_height != Py_True && img->height<
                 El::Python::ulong_from_number(
                   min_height,
                   "newsgate.message.MA_Context.skip_image: "
                   "invalid min_height")) ||
                
                 (max_height == Py_True && img->height != max_img_height) ||
                 (max_height != Py_None && max_height != Py_True && img->height>
                 El::Python::ulong_from_number(
                   max_height,
                   "newsgate.message.MA_Context.skip_image: "
                   "invalid max_height")));

            if(reverse)
            {
              match = !match;
            }
            
            if(match)
            {
              img->status = Message::Automation::Image::IS_SKIP;
            }
          }

          return El::Python::add_ref(Py_None);
        }
        
        PyObject*
        Context::py_reset_image_alt(PyObject* args, PyObject* kwds)
          throw(El::Exception)
        {
          const char* kwlist[] =
            { "origin",
              "others",
              NULL
          };
          
          PyObject* orig = Py_None;
          unsigned char others = 0;
            
          if(!PyArg_ParseTupleAndKeywords(
               args,
               kwds,
               "|OB:newsgate.message.MA_Context.reset_image_alt",
               (char**)kwlist,
               &orig,
               &others))
          {
            El::Python::handle_error(
              "NewsGate::RSS::MsgAdjustment::Python::Context::"
              "py_reset_image_alt");
          }

          bool skip_others = others != 0;

          Message::Automation::Image::Origin origin_val =
            Message::Automation::Image::IO_UNKNOWN;
          
          if(orig == Py_None)
          {
            orig = 0;
          }
          else
          {
            origin_val =
              cast_origin(El::Python::ulong_from_number(
                            orig,
                            "newsgate.message.MA_Context.reset_image_alt"),
                          "newsgate.message.MA_Context.reset_image_alt");
          }

          for(El::Python::Sequence::iterator i(message->images->begin()),
                e(message->images->end()); i != e; ++i)
          {
            Message::Automation::Python::Image* img =
              Message::Automation::Python::Image::Type::down_cast(i->in());

            if(orig == 0 || (img->origin == origin_val) != skip_others)
            {
              img->alt.clear();
            }
          }

          return El::Python::add_ref(Py_None);
        }
        
        PyObject*
        Context::py_read_image_size(PyObject* args, PyObject* kwds)
          throw(El::Exception)
        {
          const char* kwlist[] =
            { "image",
              "check_ext",
              NULL
          };
          
          PyObject* image = 0;
          unsigned char check_ext = 0;

          if(!PyArg_ParseTupleAndKeywords(
               args,
               kwds,
               "O|B:newsgate.message.MA_Context.read_image_size",
               (char**)kwlist,
               &image,
               &check_ext))
          {
            El::Python::handle_error(
              "NewsGate::RSS::MsgAdjustment::Python::Context::"
              "py_read_image_size");
          }

          Message::Automation::Python::Image* img =
            Message::Automation::Python::Image::Type::down_cast(image);

          {
            El::Python::AllowOtherThreads guard;

            img->adjust(message_restrictions_.image_restrictions,
                        request_params.in(),
                        check_ext);
          }
          
          return El::Python::add_ref(Py_None);          
        }
        
        PyObject*
        Context::py_set_description(PyObject* args, PyObject *kwds)
          throw(El::Exception)
        {
          const char* kwlist[] =
            { "right_markers",
              "left_markers",
              "max_len",
              NULL
            };
          
          PyObject* right_markers = Py_None;
          PyObject* left_markers = Py_None;
          PyObject* max_len = Py_None;
            
          if(!PyArg_ParseTupleAndKeywords(
               args,
               kwds,
               "|OOO:newsgate.message.MA_Context.set_description",
               (char**)kwlist,
               &right_markers,
               &left_markers,
               &max_len))
          {
            El::Python::handle_error(
              "NewsGate::RSS::MsgAdjustment::Python::Context::"
              "py_set_description");
          }

          size_t max_length = max_len == Py_None ? SIZE_MAX :
            El::Python::ulong_from_number(
              max_len,
              "NewsGate::RSS::MsgAdjustment::Python::Context::"
              "py_set_description");
          
          El::Python::Object_var doc = item->py_description_doc();
          
          if(doc.in() == Py_None)
          {
            return El::Python::add_ref(Py_None);
          }

          El::LibXML::Python::Document* doc_node =
            El::LibXML::Python::Document::Type::down_cast(doc);

          assert(doc_node != 0);

          std::string text;
          El::Python::Object_var res = doc_node->find("/html/body");
            
          if(res.in() != Py_None)
          {
            El::Python::Sequence& seq =
              *El::Python::Sequence::Type::down_cast(res.in());

            if(!seq.empty())
            {
              El::LibXML::Python::Node* node =
                El::LibXML::Python::Node::Type::down_cast(seq[0].in());
              
              El::String::Manip::compact(node->text(true).c_str(), text);
            }
          }

          truncate(text, right_markers, false);
          truncate(text, left_markers, true);

          if(max_length != SIZE_MAX)
          {
            std::string truncated;
            
            El::String::Manip::truncate(text.c_str(),
                                        truncated,
                                        max_length,
                                        true,
                                        El::String::Manip::UAC_XML_1_0);

            text = truncated;
          }

          message->description = text;
          
          return El::Python::add_ref(Py_None);
        }

        PyObject*
        Context::py_set_description_from_article(PyObject* args,
                                                 PyObject *kwds)
          throw(El::Exception)
        {
          const char* kwlist[] =
            { "xpath",
              "concatenate",
              "default",
              "max_len",
              "encoding",
              NULL
            };

          char* xpath = 0;
          unsigned char concat = 0;
          PyObject* def = Py_None;
          PyObject* max_len = Py_None;
          const char* enc = 0;
          
          if(!PyArg_ParseTupleAndKeywords(
               args,
               kwds,
               "s|BOOs:newsgate.message.MA_Context."
               "set_description_from_article",
               (char**)kwlist,
               &xpath,
               &concat,
               &def,
               &max_len,
               &enc))
          {
            El::Python::handle_error(
              "NewsGate::RSS::MsgAdjustment::Python::Context::"
              "py_set_description_from_article");
          }

          size_t max_length = max_len == Py_None ? SIZE_MAX :
            El::Python::ulong_from_number(
              max_len,
              "NewsGate::RSS::MsgAdjustment::Python::Context::"
              "py_set_description_from_article");

          El::Python::Object_var doc =
            item->article_doc(enc && *enc != '\0' ? enc : encoding.c_str());

          if(doc.in() == Py_None)
          {
            return new El::Python::Sequence();
          }
          
          El::LibXML::Python::Document* document =
            El::LibXML::Python::Document::Type::down_cast(doc.in());

          El::Python::Sequence_var seq;

          if(document)
          {
            El::Python::Object_var nodes = document->find(xpath);
            seq = El::Python::Sequence::Type::down_cast(nodes.in(), true);
          }
          else
          {
            seq = new El::Python::Sequence();
          }

          std::string text;

          for(El::Python::Sequence::const_iterator i(seq->begin()),
                e(seq->end()); i != e; ++i)
          {
            El::LibXML::Python::Node* node =
              El::LibXML::Python::Node::Type::down_cast(i->in());
              
            std::string trimmed;
            El::String::Manip::trim(node->text(true).c_str(), trimmed);

            if(trimmed.empty())
            {
              continue;
            }
            
            if(concat)
            {
              if(!text.empty())
              {
                text += " ";
              }

              text += trimmed;
            }
            else
            {
              text = trimmed;
              break;
            }
          }

          if(text.empty())
          {
            if(def != Py_None)
            {
              El::Python::Object_var obj =
                El::Python::string_from_object(
                  def,
                  "NewsGate::RSS::MsgAdjustment::Python::Context::"
                  "py_set_description_from_article");

              size_t len = 0;
              const char* str =
                El::Python::string_from_string(
                  obj,
                  len,
                  "NewsGate::RSS::MsgAdjustment::Python::Context::"
                  "py_set_description_from_article");

              message->description = str;
            }
          }
          else
          {
            if(max_length != SIZE_MAX)
            {
              std::string truncated;
          
              El::String::Manip::truncate(text.c_str(),
                                          truncated,
                                          max_length,
                                          true,
                                          El::String::Manip::UAC_XML_1_0);

              text = truncated;
            }
              
            message->description = text;
          }          

          return El::Python::add_ref(Py_None);
        }

        void
        Context::truncate(std::string& text, PyObject* markers, bool left)
          throw(El::Exception)
        {
          if(markers == Py_None)
          {
            return;
          }

          if(!PySequence_Check(markers))
          {
            throw Exception("newsgate.message.MA_Context.set_description: "
                            "marker argument should be of sequence "
                            "type or None");
          }

          int len = PySequence_Size(markers);

          for(int i = 0; i < len; ++i)
          {
            El::Python::Object_var o = PySequence_GetItem(markers, i);

            if(o.in() == 0)
            {
              continue;
            }

            size_t len = 0;
            
            const char* mr =
              El::Python::string_from_string(
                o.in(),
                len,
                "newsgate.message.MA_Context.set_description: ");

            std::string::size_type pos = left ? text.find(mr) : text.rfind(mr);
            
            if(pos != std::string::npos)
            {
              if(left)
              {
                text = text.substr(pos + strlen(mr));
              }
              else
              {
                text.resize(pos);
              }
            }
          }
        }

        PyObject*
        Context::py_set_images(PyObject* args, PyObject *kwds)
          throw(El::Exception)
        {
          const char* kwlist[] =
            { "op",
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
          const char* enc = 0;
            
          if(!PyArg_ParseTupleAndKeywords(
               args,
               kwds,
               "|ksOOOOOOOOOs:newsgate.message.MA_Context.set_images",
               (char**)kwlist,
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
               &enc))
          {
            El::Python::handle_error(
              "NewsGate::RSS::MsgAdjustment::Python::Context::"
              "py_set_images");
          }

          El::Python::Object_var d =
            item->article_doc(enc && *enc != '\0' ? enc : encoding.c_str());
          
          ::NewsGate::Message::Automation::Python::Image::images_from_doc(
            message->images.in(),
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
            request_params,
            0,
            "newsgate.message.MA_Context.set_images");

          return El::Python::add_ref(Py_None);
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
                El::Python::SandboxService* sandbox_service,
                El::Python::Sandbox* sandbox,
                El::Logging::Logger* logger) const
        throw(Exception, El::Exception)
      {
        El::Python::EnsureCurrentThread guard;        
        
        try
        {
          Python::Interceptor_var interceptor = new Python::Interceptor();
          
          Python::Context_var py_context =
            new Python::Context(context);

//          py_context->item->article_session.reset(0);
          py_context->item->article.close_session();
          py_context->item->constant(true);

          El::Python::SandboxService::ObjectMap objects;
          objects["context"] = py_context;

          El::Python::Object_var result =
            sandbox_service->run(code_,
                                 sandbox,
                                 &objects,
                                 interceptor.in(),
                                 0,
                                 false);

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

          py_context->apply_restrictions();
          py_context->message->save(context.message);
          context.item_article_requested = py_context->item->article_requested;
        }
        catch(const El::Exception& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::RSS::MsgAdjustment::Code::run: "
            "El::Exception caught. Description:\n" << e;
          
          throw Exception(ostr.str());
        } 
      }
      
    }
  }
}
