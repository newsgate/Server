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

#include <El/Exception.hpp>
#include <El/Image/ImageInfo.hpp>
#include <El/String/Manip.hpp>

#include <El/Net/Socket/Stream.hpp>
#include <El/Net/HTTP/Session.hpp>
#include <El/Net/HTTP/StatusCodes.hpp>
#include <El/Net/HTTP/Headers.hpp>
#include <El/LibXML/Python/Node.hpp>

#include <El/Python/RefCount.hpp>
#include <El/Python/Object.hpp>
#include <El/Python/Utility.hpp>
#include <El/Python/Module.hpp>
#include <El/Python/Sandbox.hpp>

#include "Automation.hpp"

namespace NewsGate
{
  namespace Message
  {
    namespace Automation
    {
      Operation
      cast_operation(unsigned long op, const char* context)
        throw(Exception, El::Exception)
      {
        if(op >= OP_COUNT)
        {
          std::ostringstream ostr;
          ostr << context << ": invalid operation value " << op;
          
          throw Exception(ostr.str());
        }
        
        return (Operation)op;
      }

      struct RequestInterceptor
        : public virtual El::Net::HTTP::Session::Interceptor
      {
        uint32_t recv_buffer_size;

        RequestInterceptor() throw() : recv_buffer_size(0) {}
        ~RequestInterceptor() throw() {}
        
        virtual void socket_stream_connected(El::Net::Socket::Stream& stream)
          throw(El::Exception) {}
        
        virtual void socket_stream_read(const unsigned char* buff, size_t size)
          throw(El::Exception) {}

        virtual void socket_stream_write(const unsigned char* buff,
                                         size_t size)
          throw(El::Exception) {}

        virtual void chunk_begins(unsigned long long size)
          throw(El::Exception) {}

        virtual void socket_stream_created(El::Net::Socket::Stream& stream)
          throw(El::Exception);
      };

      void
      RequestInterceptor::socket_stream_created(
        El::Net::Socket::Stream& stream) throw(El::Exception)
      {
        if(!recv_buffer_size)
        {
          return;
        }
      
        ACE_SOCK_Stream* socket = &stream.socket();
        
        unsigned int bufsize = 0;
        int buflen = sizeof(bufsize);
        
        if(socket->get_option(SOL_SOCKET, SO_RCVBUF, &bufsize, &buflen) == -1)
        {
          return;
        }

        if(bufsize > recv_buffer_size)
        {
          bufsize = recv_buffer_size;
          
          if(socket->set_option(SOL_SOCKET,
                                SO_RCVBUF,
                                &bufsize,
                                sizeof(bufsize)) == -1)
          {
            return;
          }
        }
      }

      //
      // Image struct
      //
      void
      Image::adjust(const ImageRestrictions& restrictions,
                    const El::Net::HTTP::RequestParams* req_params,
                    bool check_extensions) throw(El::Exception)
      {
        std::string compacted;
        El::String::Manip::compact(alt.c_str(), compacted);
        alt = compacted;
          
        if(src.empty())
        {
          status = ::NewsGate::Message::Automation::Image::IS_BAD_SRC;
          return;
        }

        const char* url = src.c_str();
        
        for(::NewsGate::Message::Automation::StringArray::const_iterator
              i(restrictions.black_list.begin()),
              e(restrictions.black_list.end()); i != e; ++i)
        {
          const std::string& prefix = *i;
          
          if(strncmp(prefix.c_str(), url, prefix.length()) == 0)
          {
            status = ::NewsGate::Message::Automation::Image::IS_BLACKLISTED;
            return;
          }
        }

        if(check_extensions)
        {
          El::Net::HTTP::URL_var u;
          
          try
          {
            u = new El::Net::HTTP::URL(url);
          }
          catch(...)
          {
          }
          
          const char* ext = u.in() ? strrchr(u->path(), '.') : 0;
          std::string file_ext;
          
          if(ext)
          {
            El::String::Manip::to_lower(ext + 1, file_ext);
          }

          if(restrictions.extensions.find(file_ext) ==
             restrictions.extensions.end())
          {
            status = ::NewsGate::Message::Automation::Image::IS_BAD_EXTENSION;
            return;            
          }
        }        

        if(req_params)
        {
          if(!read_size(*req_params))
          {
            status = ::NewsGate::Message::Automation::Image::IS_BAD_SRC;
            return;
          }
        }

        if(width < restrictions.min_width || height < restrictions.min_height)
        {
          status = ::NewsGate::Message::Automation::Image::IS_TOO_SMALL;
          return;
        }
      }
      
      bool
      Image::read_size(const El::Net::HTTP::RequestParams& params)
        throw(El::Exception)
      {
        if(src.empty())
        {
          return false;
        }

        if(params.referer.empty())
        {
          return do_read_size(params);
        }

        Image im1(*this);
        
        if(!im1.do_read_size(params))
        {
          return false;
        }

        El::Net::HTTP::RequestParams prm(params);
        prm.referer.clear();
        
        Image im2(*this);

        if(!im2.do_read_size(prm))
        {
          return false;
        }

        if(im1.height != im2.height || im1.width != im2.width)
        {
          return false;
        }

        height = im1.height;
        width = im1.width;
        
        return true;
      }
      
      bool
      Image::do_read_size(const El::Net::HTTP::RequestParams& params)
        throw(El::Exception)
      {
        RequestInterceptor interceptor;
        El::Net::HTTP::Session::Interceptor* pinterceptor = 0;

        if(params.interceptor)
        {
          pinterceptor = params.interceptor;
        }
        else
        {
          pinterceptor = &interceptor;
          interceptor.recv_buffer_size = params.recv_buffer_size;
        }
      
        try
        {
          El::Net::HTTP::HeaderList headers;

          headers.add(El::Net::HTTP::HD_ACCEPT, "*/*");
          headers.add(El::Net::HTTP::HD_ACCEPT_ENCODING, "gzip,deflate");
          headers.add(El::Net::HTTP::HD_ACCEPT_LANGUAGE, "en-us,*");

          if(!params.user_agent.empty())
          {
            headers.add(El::Net::HTTP::HD_USER_AGENT,
                        params.user_agent.c_str());
          }          

          if(!params.referer.empty())
          {
            headers.add(El::Net::HTTP::HD_REFERER, params.referer.c_str());
          }
        
          El::Net::HTTP::Session session(src.c_str(),
                                         El::Net::HTTP::HTTP_1_1,
                                         pinterceptor);

          ACE_Time_Value timeout(params.timeout);

          session.open(&timeout,
                       &timeout,
                       &timeout,
                       1024,
                       params.recv_buffer_size ?
                       params.recv_buffer_size : 1024);

          session.send_request(El::Net::HTTP::GET,
                               El::Net::HTTP::ParamList(),
                               headers,
                               0,
                               0,
                               params.redirects_to_follow);
      
          session.recv_response_status();

          if(session.status_code() != El::Net::HTTP::SC_OK)
          {
            return false;
          }

          El::Net::HTTP::Header header;
          while(session.recv_response_header(header));
        
          El::Image::ImageInfo info;
        
          if(info.read(session.response_body()))
          {
            width = info.width;
            height = info.height;          
          }
          else
          {
            return false;
          }        
        }
        catch(const ::El::Net::Exception& e)
        {
          return false;
        }

        return true;
      }
      
      namespace Python
      {
        class MessagePyModule : public El::Python::ModuleImpl<MessagePyModule>
        {
        public:
          static MessagePyModule instance;

          MessagePyModule() throw(El::Exception);
      
          virtual void initialized() throw(El::Exception);
        };

        MessagePyModule::MessagePyModule() throw(El::Exception)
            : El::Python::ModuleImpl<MessagePyModule>(
              "newsgate.message",
              "Module containing message related types.",
              true)
        {
        }
        
        void
        MessagePyModule::initialized() throw(El::Exception)
        {
          add_member(PyLong_FromLong(OP_ADD_BEGIN), "OP_ADD_BEGIN");
          add_member(PyLong_FromLong(OP_ADD_END), "OP_ADD_END");
          add_member(PyLong_FromLong(OP_ADD_TO_EMPTY), "OP_ADD_TO_EMPTY");
          add_member(PyLong_FromLong(OP_REPLACE), "OP_REPLACE");
        }
        
        MessagePyModule MessagePyModule::instance;

        ImageRestrictions::Type ImageRestrictions::Type::instance;
        MessageRestrictions::Type MessageRestrictions::Type::instance;
        Image::Type Image::Type::instance;
        Message::Type Message::Type::instance;
        Source::Type Source::Type::instance;

        //
        // ImageRestrictions class
        //
        ImageRestrictions::ImageRestrictions(PyTypeObject *type,
                                             PyObject *args,
                                             PyObject *kwds)
          throw(El::Exception)
            : El::Python::ObjectImpl(type ? type : &Type::instance),
              min_width(0),
              min_height(0)            
        {
          init_subobj(0);
        }
        
        ImageRestrictions::ImageRestrictions(
          const ::NewsGate::Message::Automation::ImageRestrictions& src)
          throw(El::Exception)
            : El::Python::ObjectImpl(&Type::instance),
              min_width(src.min_width),
              min_height(src.min_height)
        {
          init_subobj(&src);
        }

        void
        ImageRestrictions::init_subobj(
          const ::NewsGate::Message::Automation::ImageRestrictions* src)
          throw(El::Exception)
        {
          init(black_list, new El::Python::Sequence());          
          init(extensions, new El::Python::Sequence());

          if(src)
          {
            black_list->reserve(src->black_list.size());

            for(StringArray::const_iterator i(src->black_list.begin()),
                  e(src->black_list.end()); i != e; ++i)
            {
              black_list->push_back(PyString_FromString(i->c_str()));
            }

            extensions->reserve(src->extensions.size());

            for(StringSet::const_iterator i(src->extensions.begin()),
                  e(src->extensions.end()); i != e; ++i)
            {
              extensions->push_back(PyString_FromString(i->c_str()));
            }
            
          }
        }

        void
        ImageRestrictions::save(
          ::NewsGate::Message::Automation::ImageRestrictions& img)
          throw(El::Exception)
        {
          img.min_width = min_width;
          img.min_height = min_height;
              
          for(El::Python::Sequence::const_iterator i(black_list->begin()),
                e(black_list->end()); i != e; ++i)
          {
            size_t len = 0;
            
            const char* str =
              El::Python::string_from_string(
                i->in(),
                len,
                "::NewsGate::Message::Automation::Python::"
                "ImageRestrictions::save");
            
            img.black_list.push_back(str);
          }
          
          for(El::Python::Sequence::const_iterator i(extensions->begin()),
                e(extensions->end()); i != e; ++i)
          {
            size_t len = 0;
            
            const char* str =
              El::Python::string_from_string(
                i->in(),
                len,
                "::NewsGate::Message::Automation::Python::"
                "ImageRestrictions::save");
            
            img.extensions.insert(str);
          }
        }
          
        void
        ImageRestrictions::write(El::BinaryOutStream& bstr) const
          throw(El::Exception)
        {
          bstr << (uint64_t)min_width << (uint64_t)min_height << black_list
               << extensions;
        }
          
        void
        ImageRestrictions::read(El::BinaryInStream& bstr) throw(El::Exception)
        {
          uint64_t mw = 0;
          uint64_t mh = 0;
          
          bstr >> mw >> mh >> black_list >> extensions;

          min_width = mw;
          min_height = mh;          
        }
          
        //
        // MessageRestrictions class
        //
        MessageRestrictions::MessageRestrictions(PyTypeObject *type,
                                                 PyObject *args,
                                                 PyObject *kwds)
          throw(El::Exception)
            : El::Python::ObjectImpl(type ? type : &Type::instance),
              max_title_len(0),
              max_desc_len(0),
              max_desc_chars(0),
              max_image_count(0)
        {
          init_subobj(0);
        }
        
        MessageRestrictions::MessageRestrictions(
          const ::NewsGate::Message::Automation::MessageRestrictions& src)
          throw(El::Exception)
            : El::Python::ObjectImpl(&Type::instance),
              max_title_len(src.max_title_len),
              max_desc_len(src.max_desc_len),
              max_desc_chars(src.max_desc_chars),
              max_image_count(src.max_image_count)
        {
          init_subobj(&src);
        }

        void
        MessageRestrictions::init_subobj(
          const ::NewsGate::Message::Automation::MessageRestrictions* src)
          throw(El::Exception)
        {
          if(src)
          {
            init(image_restrictions,
                 new ImageRestrictions(src->image_restrictions));
          }
          else
          {
            init(image_restrictions, new ImageRestrictions());
          }
        }
          
        void
        MessageRestrictions::write(El::BinaryOutStream& ostr) const
          throw(El::Exception)
        {
          ostr << (uint64_t)max_title_len << (uint64_t)max_desc_len
               << (uint64_t)max_desc_chars
               << image_restrictions << (uint64_t)max_image_count;
        }
          
        void
        MessageRestrictions::read(El::BinaryInStream& istr)
          throw(El::Exception)
        {
          uint64_t mtl = 0;
          uint64_t mdl = 0;
          uint64_t mdc = 0;
          uint64_t mic = 0;
            
          istr >> mtl >> mdl >> mdc >> image_restrictions >> mic;

          max_title_len = mtl;
          max_desc_len = mdl;
          max_desc_chars = mdc;
          max_image_count = mic;
        }

        //
        // Image struct
        //
        Image::Image(const ::NewsGate::Message::Automation::Image& img)
          throw(Exception, El::Exception)
            : El::Python::ObjectImpl(&Type::instance)
        {
          ((::NewsGate::Message::Automation::Image&)*this) = img;
        }

        //
        // Python::Image class
        //
        Image::Image(PyTypeObject *type, PyObject *args, PyObject *kwds)
          throw(El::Exception) :
            El::Python::ObjectImpl(type ? type : &Type::instance)
        {
          if(args == 0)
          {
            return;
          }
          
          PyObject* node = 0;
          
          if(!PyArg_ParseTuple(args,
                               "|O:newsgate.message.Image.Image",
                               &node))
          {
            El::Python::handle_error(
              "NewsGate::Message::Automation::Python::Image::Image");
          }

          if(node == 0)
          {
            return;
          }

          init(El::LibXML::Python::Node::Type::down_cast(node));
        }

        Image::Image(const El::LibXML::Python::Node* node) throw(El::Exception)
            : El::Python::ObjectImpl(&Type::instance)
        {
          init(node);
        }

        void
        Image::init(const El::LibXML::Python::Node* node) throw(El::Exception)
        {
          const char* val = node->attr("src", false);
          El::String::Manip::trim(val, src);

          if(src.empty())
          {
            const char* val = node->attr("href", false);
            El::String::Manip::trim(val, src);

            if(src.empty())
            {
              const char* val = node->attr("content", false);
              El::String::Manip::trim(val, src);

              if(src.empty())
              {
                status = IS_BAD_SRC;
                return;
              }
            }
          }

          El::LibXML::Python::Document* doc = node->document();

          if(doc)
          { 
            try
            {
              El::Net::HTTP::URL_var url =
                new El::Net::HTTP::URL(node->document()->base_url());

              src = url->abs_url(src.c_str());
            }
            catch(...)
            {
            }
          }

          val = node->attr("alt", false);
          
          if(val == 0)
          {
            val = node->attr("title", false);
          }

          if(val)
          {
            El::String::Manip::compact(val, alt, true);
          }

          unsigned long v = 0;
          
          if(El::String::Manip::numeric(node->attr("width", false), v))
          {
            width = v;
          }
            
          if(El::String::Manip::numeric(node->attr("height", false), v))
          {
            height = v;
          }
        }
      
        void
        Image::Image::write(El::BinaryOutStream& ostr) const
          throw(El::Exception)
        {
          ::NewsGate::Message::Automation::Image::write(ostr);
        }
          
        void
        Image::read(El::BinaryInStream& istr) throw(El::Exception)
        {
          ::NewsGate::Message::Automation::Image::read(istr);
        }
          
        void
        Image::save(::NewsGate::Message::Automation::Image& img) const
          throw(El::Exception)
        {
          img = *this;

          El::String::Manip::trim(src.c_str(), img.src);
          El::String::Manip::compact(alt.c_str(), img.alt, true);
          
          if(img.src.empty())
          {
            img.status = IS_BAD_SRC;
          }
        }
        
        PyObject*
        Image::py_read_size(PyObject* args) throw(El::Exception)
        {
          if(El::Python::Sandbox::thread_sandboxed())
          {
            El::Python::Interceptor::Interruption_var obj =
              new El::Python::Interceptor::Interruption();
            
            obj->reason = "newsgate.message.Image.read_size is not safe";
            PyErr_SetObject(PyExc_SystemExit, obj.in());
            return 0;
          }
          
          PyObject* params = 0;
          
          if(!PyArg_ParseTuple(args,
                               "O:newsgate.message.Image.read_size",
                               &params))
          {
            El::Python::handle_error(
              "NewsGate::Message::Automation::Python::Image::py_read_size");
          }

          El::Net::HTTP::Python::RequestParams* req_params =
            El::Net::HTTP::Python::RequestParams::Type::down_cast(params);

          bool success = false;
          
          {
            El::Python::AllowOtherThreads guard;
            success = read_size(*req_params);
          }          

          return El::Python::add_ref(success ? Py_True : Py_False);
        }

        El::Net::HTTP::RequestParams*
        Image::adjust_request_params(
          El::Net::HTTP::RequestParams* request_params,
          unsigned long execution_end) throw(El::Exception)
        {
          if(request_params && execution_end)
          {
            unsigned long tm = ACE_OS::gettimeofday().sec();
            
            unsigned long left =
              execution_end < tm ? 0 : (execution_end - tm);
            
            request_params->timeout = std::min(left, request_params->timeout);
          }

          return request_params;
        }
        
        void
        Image::images_from_doc(
          PyObject* images,
          PyObject* document,
          unsigned long op,
          const char* img_xpath,
          PyObject* read_size,
          PyObject* check_ext, // Autodecision if Py_None
          PyObject* ancestor_xpath,
          PyObject* alt_xpath,
          PyObject* min_width,
          PyObject* min_height,
          PyObject* max_width,
          PyObject* max_height,
          PyObject* max_count,
          const ::NewsGate::Message::Automation::ImageRestrictions&
          image_restrictions,
          const El::Net::HTTP::RequestParams* request_params,
          unsigned long execution_end,
          const char* context)
          throw(Exception, El::Exception)
        {          
          std::auto_ptr<El::Net::HTTP::RequestParams>
            req_params(request_params ?
                       new El::Net::HTTP::RequestParams(*request_params) : 0);
            
          El::Python::Sequence* image_seq =
            El::Python::Sequence::Type::down_cast(images);

          std::string ctx = context;

          unsigned long img_max_count = ULONG_MAX;
          unsigned long img_count = 0;
          
          if(max_count != Py_None)
          {  
            img_max_count = El::Python::ulong_from_number(
              max_count,
              std::string(ctx + ": invalid max_count").c_str());
          }          

          for(El::Python::Sequence::iterator i(image_seq->begin()),
                e(image_seq->end()); i != e; ++i)
          {
            Image& img = *Image::Type::down_cast((*i).in());
            
            if(img.status == IS_VALID && img_count >= img_max_count)
            {
              img.status = IS_TOO_MANY;
            }

            if(img.status == IS_VALID)
            {
              ++img_count;
            }
          }
            
          Operation operation = cast_operation(op, context);

          if(document == Py_None || (operation == OP_ADD_TO_EMPTY && img_count))
          {
            return;
          }

          El::LibXML::Python::Document* doc_node =
            El::LibXML::Python::Document::Type::down_cast(document);
          
          size_t erase = operation == OP_REPLACE ? image_seq->size(): 0;
          size_t offset = 0;
          
          if(ancestor_xpath == Py_None)
          { 
            El::Python::Object_var res = doc_node->find(img_xpath);
            
            if(res.in() != Py_None)
            {
              El::Python::Sequence& seq =
                *El::Python::Sequence::Type::down_cast(res.in());

              for(El::Python::Sequence::const_iterator i(seq.begin()),
                    e(seq.end()); i != e; ++i)
              {
                El::LibXML::Python::Node* node =
                  El::LibXML::Python::Node::Type::down_cast(i->in());

                Image_var img = new Image(node);
                
                if(alt_xpath == Py_False)
                {
                  img->alt.clear();
                }

                insert_image(*image_seq,
                             img.in(),
                             node,
                             op,
                             read_size,
                             check_ext,
                             min_width,
                             min_height,
                             max_width,
                             max_height,
                             img_max_count,
                             image_restrictions,
                             adjust_request_params(req_params.get(),
                                                   execution_end),
                             erase,
                             offset,
                             img_count,
                             context);
              }
            }
          }
          else
          {
            if(!PyString_Check(ancestor_xpath))
            {
              std::ostringstream ostr;
              ostr << context
                   << ": ancestor_xpath argument should be of string "
                "type or None";
              
              throw Exception(ostr.str());
            }

            const char* anc_xpath = PyString_AsString(ancestor_xpath);

            if(anc_xpath == 0)
            {
              El::Python::handle_error(context);
            }
            
            El::Python::Object_var res = doc_node->find(anc_xpath);
            
            if(res.in() != Py_None)
            {
              El::Python::Sequence& seq =
                *El::Python::Sequence::Type::down_cast(res.in());

              for(El::Python::Sequence::const_iterator i(seq.begin()),
                    e(seq.end()); i != e; ++i)
              {
                El::LibXML::Python::Node* anc_node =
                  El::LibXML::Python::Node::Type::down_cast(i->in());

                El::Python::Object_var res = anc_node->find(img_xpath);
            
                if(res.in() == Py_None)
                {
                  continue;
                }
                
                El::Python::Sequence& seq =
                  *El::Python::Sequence::Type::down_cast(res.in());

                if(seq.size() == 0)
                {
                  continue;
                }
                
                El::LibXML::Python::Node* node =
                  El::LibXML::Python::Node::Type::down_cast(seq[0].in());

                Image_var img = new Image(node);

                if(alt_xpath != Py_None)
                {
                  std::string alt;
                  
                  if(alt_xpath != Py_False)
                  {
                    if(!PyString_Check(alt_xpath))
                    {
                      std::ostringstream ostr;
                      ostr << context
                           << ": alt_xpath argument should be of string "
                        "type or None or False";
                      
                      throw Exception(ostr.str());
                    }

                    const char* xpath = PyString_AsString(alt_xpath);

                    if(xpath == 0)
                    {
                      El::Python::handle_error(context);
                    }

                    El::Python::Object_var res = anc_node->find(xpath);
                      
                    if(res.in() != Py_None)
                    {
                      El::Python::Sequence& seq =
                        *El::Python::Sequence::Type::down_cast(res.in());

                      for(El::Python::Sequence::const_iterator i(seq.begin()),
                            e(seq.end()); i != e; ++i)
                      {
                        El::LibXML::Python::Node* node =
                          El::LibXML::Python::Node::Type::down_cast(i->in());
                        
                        alt += " " + node->text(true);
                      }
                    }
                  }
                  
                  El::String::Manip::compact(alt.c_str(), img->alt);
                }

                insert_image(*image_seq,
                             img.in(),
                             node,
                             op,
                             read_size,
                             check_ext,
                             min_width,
                             min_height,
                             max_width,
                             max_height,
                             img_max_count,
                             image_restrictions,
                             adjust_request_params(req_params.get(),
                                                   execution_end),
                             erase,
                             offset,
                             img_count,
                             context);
              }
            }            
          }  
        }
        
        void
        Image::insert_image(
          El::Python::Sequence& images,
          Image* img,
          El::LibXML::Python::Node* node,
          unsigned long op,
          PyObject* read_size,
          PyObject* check_ext,
          PyObject* min_width,
          PyObject* min_height,
          PyObject* max_width,
          PyObject* max_height,
          unsigned long max_count,
          const ::NewsGate::Message::Automation::ImageRestrictions&
          image_restrictions,
          const El::Net::HTTP::RequestParams* request_params,
          size_t& erase,
          size_t& offset,
          unsigned long& count,
          const char* context)
          throw(El::Exception)
        {
          if(erase == 0 && img->status == IS_VALID && count >= max_count)
          {
            img->status = IS_TOO_MANY;
          }

/*          
          if(img->status != IS_VALID)
          {
            return;
          }
*/
          
          for(El::Python::Sequence::const_iterator
                i(images.begin() + erase), e(images.end()); i != e; ++i)
          {
            try
            {
              Image* img_obj = Image::Type::down_cast(i->in());
              
              if(img_obj->src == img->src && img_obj->status == IS_VALID)
              {
                img->status = IS_DUPLICATE;
                return;
              }
            }
            catch(...)
            {
            }
          }

          if(img->status == IS_VALID)
          {
            bool width_undef = img->width ==
              NewsGate::Message::Automation::Image::DIM_UNDEFINED;
          
            bool height_undef = img->height ==
              NewsGate::Message::Automation::Image::DIM_UNDEFINED;
          
            bool do_read_size = read_size == Py_True ||
              (read_size != Py_False && (width_undef || height_undef)) ||
              max_count != ULONG_MAX ||
              (width_undef && (min_width != Py_None || max_width != Py_None)) ||
              (height_undef && (min_height!=Py_None || max_height != Py_None));

            std::string node_name = node->name();
            const char* property_attr = node->attr("property", false);
            const char* rel_attr = node->attr("rel", false);

            bool img_tag = strcasecmp(node_name.c_str(), "img") == 0 ||
              (strcasecmp(node_name.c_str(), "meta") == 0 &&
               property_attr && strcasecmp(property_attr, "og:image") == 0) ||
              (strcasecmp(node_name.c_str(), "link") == 0 &&
               rel_attr && strcasecmp(rel_attr, "image_src") == 0);

            bool do_check_ext = check_ext == Py_True ||
              (check_ext != Py_False && !img_tag);

            {
              El::Python::AllowOtherThreads guard;
                  
              img->adjust(image_restrictions,
                          do_read_size ? request_params : 0,
                          do_check_ext);
            }

            std::string ctx = context;  

            if(img->width < image_restrictions.min_width ||
               img->height < image_restrictions.min_height ||
               (min_width != Py_None && img->width <
                El::Python::ulong_from_number(
                  min_width,
                  std::string(ctx + ": invalid min_width").c_str())) ||
               (min_height != Py_None && img->height <
                El::Python::ulong_from_number(
                  min_height,
                  std::string(ctx + ": invalid min_height").c_str())))
            {
              img->status = IS_TOO_SMALL;
            }
            else if((max_width != Py_None && img->width >
                     El::Python::ulong_from_number(
                       max_width,
                       std::string(ctx + ": invalid max_width").c_str())) ||
                    (max_height != Py_None && img->height >
                     El::Python::ulong_from_number(
                       max_height,
                       std::string(ctx + ": invalid max_height").c_str())))
            {
              img->status = IS_TOO_BIG;
            }
          }
        
          if(erase && img->status == IS_VALID)
          {
            images.erase(images.begin(), images.begin() + erase);
            erase = 0;
            count = 0;
          }

          if(img->status == IS_VALID && count >= max_count)
          {
            img->status = IS_TOO_MANY;
          }
          
          if(op == OP_ADD_BEGIN)
          {
            images.insert(images.begin() + offset++, El::Python::add_ref(img));
          }
          else
          {
            images.push_back(El::Python::add_ref(img));
          }

          if(img->status == IS_VALID)
          {
            ++count;
          }
        }
      
        //
        // Source struct
        //
        Source::Source(const ::NewsGate::Message::Automation::Source& src)
          throw(Exception, El::Exception)
            : El::Python::ObjectImpl(&Type::instance)
        {
          ((::NewsGate::Message::Automation::Source&)*this) = src;
        }
        
        void
        Source::save(::NewsGate::Message::Automation::Source& src) const
          throw(El::Exception)
        {
          src = *this;
          El::String::Manip::compact(title.c_str(), src.title, true);
          
          El::String::Manip::trim(html_link.c_str(), src.html_link);
        }
        
        //
        // Message struct
        //
        Message::Message(
          const ::NewsGate::Message::Automation::Message& message)
          throw(Exception, El::Exception)
            : El::Python::ObjectImpl(&Type::instance),
              title(message.title),
              description(message.description),
              url(message.url),
              images(new El::Python::Sequence()),
              keywords(new El::Python::Sequence()),
              source(new Source(message.source)),
              lang(new El::Python::Lang(message.lang)),
              country(new El::Python::Country(message.country)),
              space(message.space),
              valid(message.valid),
              log(message.log)
        {
          images->reserve(message.images.size());
            
          for(ImageArray::const_iterator i(message.images.begin()),
                e(message.images.end()); i != e; ++i)
          {
            images->push_back(new Image(*i));
          }

          keywords->reserve(message.keywords.size());

          for(StringArray::const_iterator i(message.keywords.begin()),
                e(message.keywords.end()); i != e; ++i)
          {
            keywords->push_back(PyString_FromString(i->c_str()));
          }
        }

        void
        Message::write(El::BinaryOutStream& ostr) const throw(El::Exception)
        {
          ostr << title << description << url << images << keywords << source
               << lang << country << (uint32_t)space << (uint8_t)valid
               << log;
        }
          
        void
        Message::read(El::BinaryInStream& istr) throw(El::Exception)
        {
          uint32_t sp = 0;
          uint8_t vl  = 0;
        
          istr >> title >> description >> url >> images >> keywords >> source
               >> lang >> country >> sp >> vl >> log;

          space = sp;
          valid = vl;
        }
          
        void
        Message::save(::NewsGate::Message::Automation::Message& message) const
          throw(El::Exception)
        {
          ImageArray dest_images;
          dest_images.reserve(images->size());
          
          for(El::Python::Sequence::const_iterator i(images->begin()),
                e(images->end()); i != e; ++i)
          {
            Image* img = Image::Type::down_cast(i->in());
            
            dest_images.push_back(Image());
            img->save(*dest_images.rbegin());
          }

          StringArray dest_keywords;
          dest_keywords.reserve(keywords->size());
          
          for(El::Python::Sequence::const_iterator i(keywords->begin()),
                e(keywords->end()); i != e; ++i)
          {
            size_t len = 0;
            
            dest_keywords.push_back(
              El::Python::string_from_string(
                i->in(),
                len,
                "NewsGate::Message::Automation::Python::Message::save"));
          }

          source->save(message.source);
          
          El::String::Manip::compact(title.c_str(), message.title, true);
          
          El::String::Manip::compact(description.c_str(),
                                     message.description,
                                     true);          
          
          El::String::Manip::trim(url.c_str(), message.url);
          
          message.lang = *lang;
          message.country = *country;
          
          message.space = space  < Feed::SP_SPACES_COUNT ?
            (Feed::Space)space :  Feed::SP_UNDEFINED;

          message.valid = valid;
          message.log = log;

          message.images.swap(dest_images);
          message.keywords.swap(dest_keywords);          
        }

        void
        Message::keywords_from_doc(PyObject* keywords,
                                   PyObject* document,
                                   const char* xpath,            
                                   unsigned long op,
                                   const char* separators,
                                   PyObject* keyword_override,
                                   const char* context)
          throw(Exception, El::Exception)
        {
          El::Python::Sequence* keyword_seq =
            El::Python::Sequence::Type::down_cast(keywords);

          Operation operation = cast_operation(op, context);

          std::string ctx = context;

          if(keyword_override != Py_None &&
             !PySequence_Check(keyword_override))
          {
            std::ostringstream ostr;
            ostr << ctx << ": keyword_override is not a sequence";
            throw Exception(ostr.str());
          }

          if((operation == OP_ADD_TO_EMPTY && !keyword_seq->empty()) ||
             document == Py_None)
          {
            return;
          }

          El::Python::Object_var res =
            El::LibXML::Python::Document::Type::down_cast(document)->
            find(xpath);
            
          if(res.in() == Py_None)
          {
            return;
          }
        
          bool erase = operation == OP_REPLACE;
          size_t offset = 0;

          El::Python::Sequence& seq =
            *El::Python::Sequence::Type::down_cast(res.in());
              
          if(keyword_override == Py_None)
          {
            for(El::Python::Sequence::const_iterator i(seq.begin()),
                  e(seq.end()); i != e; ++i)
            {
              El::LibXML::Python::Node* node =
                El::LibXML::Python::Node::Type::down_cast(i->in());

              std::string text = node->text(true);

              El::String::ListParser parser(text.c_str(), separators);
              const char* item = 0;
                
              while((item = parser.next_item()) != 0)
              {
                std::string kw;
                El::String::Manip::trim(item, kw);

                if(!kw.empty())
                {
                  if(erase)
                  {
                    keyword_seq->clear();
                    erase = false;
                  }
                    
                  El::Python::Object_var k =
                    PyString_FromString(kw.c_str());

                  if(operation == OP_ADD_BEGIN)
                  {
                    keyword_seq->insert(keyword_seq->begin() + offset++, k);
                  }
                  else
                  {
                    keyword_seq->push_back(k);
                  }
                }
              }
            }
          }
          else if(!seq.empty())
          {
            int len = PySequence_Size(keyword_override);

            for(int i = 0; i < len; ++i)
            {
              El::Python::Object_var o =
                PySequence_GetItem(keyword_override, i);

              El::Python::Object_var os =
                El::Python::string_from_object(o.in(), context);

              size_t slen = 0;
                  
              const char* str =
                El::Python::string_from_string(os.in(), slen, context);
                  
              std::string kw;
              El::String::Manip::trim(str, kw);

              if(!kw.empty())
              {
                if(erase)
                {
                  keyword_seq->clear();
                  erase = false;
                }
                    
                El::Python::Object_var k =
                  PyString_FromString(kw.c_str());

                if(operation == OP_ADD_BEGIN)
                {
                  keyword_seq->insert(keyword_seq->begin() + offset++, k);
                }
                else
                {
                  keyword_seq->push_back(k);
                }
              }
            }                  
          }
        }          
      }
    }
  }
}
