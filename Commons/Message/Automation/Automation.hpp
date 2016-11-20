/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Commons/Message/Automation/Automation.hpp
 * @author Karen Arutyunov
 * $id:$
 */

#ifndef _NEWSGATE_SERVER_COMMONS_MESSAGE_AUTOMATION_AUTOMATION_HPP_
#define _NEWSGATE_SERVER_COMMONS_MESSAGE_AUTOMATION_AUTOMATION_HPP_

#include <stdint.h>

#include <string>
#include <vector>
#include <ext/hash_set>

#include <El/Exception.hpp>
#include <El/Lang.hpp>
#include <El/Country.hpp>
#include <El/BinaryStream.hpp>
#include <El/Hash/Hash.hpp>

#include <El/Python/Object.hpp>
#include <El/Python/Sequence.hpp>
#include <El/Python/Lang.hpp>
#include <El/Python/Country.hpp>

#include <El/LibXML/Python/Node.hpp>

#include <El/HTML/LightParser.hpp>

#include <El/Net/HTTP/Utility.hpp>
#include <El/Net/HTTP/Python/Utility.hpp>

#include <Commons/Feed/Types.hpp>

namespace NewsGate
{
  namespace Message
  {
    namespace Automation
    {
      EL_EXCEPTION(Exception, El::ExceptionBase);

      typedef std::vector<std::string> StringArray;
      typedef __gnu_cxx::hash_set<std::string, El::Hash::String> StringSet;

      struct ImageRestrictions
      {
        unsigned long min_width;
        unsigned long min_height;
        StringArray black_list;
        StringSet extensions;

        ImageRestrictions() : min_width(0), min_height(0) {}

        void write(El::BinaryOutStream& ostr) const
          throw(El::Exception);

        void read(El::BinaryInStream& istr) throw(El::Exception);
      };

      struct MessageRestrictions
      {
        MessageRestrictions() throw();
        
        size_t max_title_len;
        size_t max_desc_len;
        size_t max_desc_chars;
        ImageRestrictions image_restrictions;
        size_t max_image_count;

        void write(El::BinaryOutStream& ostr) const throw(El::Exception);
        void read(El::BinaryInStream& istr) throw(El::Exception);        
      };

      enum Operation
      {
        OP_ADD_BEGIN,
        OP_ADD_END,
        OP_ADD_TO_EMPTY,
        OP_REPLACE,
        OP_COUNT
      };

      Operation cast_operation(unsigned long op,
                               const char* context)
        throw(Exception, El::Exception);
      
      struct Image
      {
        enum Origin
        {
          IO_UNKNOWN,
          IO_DESC_IMG,
          IO_DESC_LINK,
          IO_ENCLOSURE,
          IO_COUNT
        };

        enum Status
        {
          IS_VALID,
          IS_BAD_SRC,
          IS_BLACKLISTED,
          IS_BAD_EXTENSION,
          IS_TOO_SMALL,
          IS_SKIP,
          IS_DUPLICATE,
          IS_TOO_MANY,
          IS_TOO_BIG,
          IS_UNKNOWN_DIM,
          IS_COUNT
        };
        
        std::string src;
        std::string alt;
        Origin origin;
        Status status;
        unsigned long width;
        unsigned long height;

        const static uint16_t DIM_UNDEFINED = UINT16_MAX;

        Image(const char* url = 0,
              const char* alt_txt = 0,
              Origin orig = IO_UNKNOWN,
              Status stat = IS_VALID,
              unsigned long w = DIM_UNDEFINED,
              unsigned long h = DIM_UNDEFINED)
          throw();
        
        Image(const El::HTML::LightParser::Image& img,
              Origin orig,
              Status stat) throw();

        bool read_size(const El::Net::HTTP::RequestParams& params)
          throw(El::Exception);
        
        void write(El::BinaryOutStream& ostr) const
          throw(El::Exception);

        void read(El::BinaryInStream& istr) throw(El::Exception);

        void adjust(const ImageRestrictions& restrictions,
                    const El::Net::HTTP::RequestParams* req_params,
                    bool check_extensions) throw(El::Exception);

      private:
        
        bool do_read_size(const El::Net::HTTP::RequestParams& params)
          throw(El::Exception);
      };

      typedef std::vector<Image> ImageArray;
      
      struct Source
      {
        std::string title;
        std::string url;
        std::string html_link;

        void write(El::BinaryOutStream& ostr) const
          throw(El::Exception);

        void read(El::BinaryInStream& istr) throw(El::Exception);
      };

      struct Message
      {
        std::string title;
        std::string description;
        std::string url;
        ImageArray images;
        StringArray keywords;
        Source source;
        El::Lang lang;
        El::Country country;
        Feed::Space space;
        bool valid;
        std::string log;

        static const size_t MAX_MSG_URL_LEN = 2048;
        
        Message() : space(Feed::SP_UNDEFINED), valid(false) {}

        void write(El::BinaryOutStream& ostr) const
          throw(El::Exception);

        void read(El::BinaryInStream& istr) throw(El::Exception);
      };

      typedef std::vector<Message> MessageArray;
      
      namespace Python
      {
        class ImageRestrictions : public El::Python::ObjectImpl
        {
        public:
          ImageRestrictions(PyTypeObject *type = 0,
                            PyObject *args = 0,
                            PyObject *kwds = 0)
            throw(El::Exception);

          ImageRestrictions(
            const ::NewsGate::Message::Automation::ImageRestrictions& src)
            throw(El::Exception);
          
          virtual void write(El::BinaryOutStream& ostr) const
            throw(El::Exception);
          
          virtual void read(El::BinaryInStream& istr) throw(El::Exception);

          void save(::NewsGate::Message::Automation::ImageRestrictions& img)
            throw(El::Exception);
          
          class Type :
            public El::Python::ObjectTypeImpl<ImageRestrictions,
                                              ImageRestrictions::Type>
          {
          public:
            Type() throw(El::Python::Exception, El::Exception);
            static Type instance;

            PY_TYPE_MEMBER_ULONG(min_width, "min_width", "Image min width");
            PY_TYPE_MEMBER_ULONG(min_height, "min_height", "Image min height");

            PY_TYPE_MEMBER_OBJECT(black_list,
                                  El::Python::Sequence::Type,
                                  "black_list",
                                  "Image url prefix blacklist",
                                  false);

            PY_TYPE_MEMBER_OBJECT(extensions,
                                  El::Python::Sequence::Type,
                                  "extensions",
                                  "Image expected extensions",
                                  false);
          };
          
          unsigned long min_width;
          unsigned long min_height;
          El::Python::Sequence_var black_list;
          El::Python::Sequence_var extensions;

        private:
          void init_subobj(
            const ::NewsGate::Message::Automation::ImageRestrictions* src)
            throw(El::Exception);
        };
        
        typedef El::Python::SmartPtr<ImageRestrictions> ImageRestrictions_var;
        
        class MessageRestrictions : public El::Python::ObjectImpl
        {
        public:
          MessageRestrictions(PyTypeObject *type = 0,
                              PyObject *args = 0,
                              PyObject *kwds = 0)
            throw(El::Exception);

          MessageRestrictions(
            const ::NewsGate::Message::Automation::MessageRestrictions& src)
            throw(El::Exception);
          
          virtual void write(El::BinaryOutStream& ostr) const
            throw(El::Exception);
          
          virtual void read(El::BinaryInStream& istr) throw(El::Exception);

          class Type :
            public El::Python::ObjectTypeImpl<MessageRestrictions,
                                              MessageRestrictions::Type>
          {
          public:
            Type() throw(El::Python::Exception, El::Exception);
            static Type instance;

            PY_TYPE_MEMBER_ULONG(max_title_len,
                                 "max_title_len",
                                 "Message title max len");
            
            PY_TYPE_MEMBER_ULONG(max_desc_len,
                                 "max_desc_len",
                                 "Message description max length");

            PY_TYPE_MEMBER_ULONG(max_desc_chars,
                                 "max_desc_chars",
                                 "Message description max characters");

            PY_TYPE_MEMBER_OBJECT(image_restrictions,
                                  ImageRestrictions::Type,
                                  "image_restrictions",
                                  "Image restrictions",
                                  false);
            
            PY_TYPE_MEMBER_ULONG(max_image_count,
                                 "max_image_count",
                                 "Message number of images in a message");
          };
          
          unsigned long max_title_len;
          unsigned long max_desc_len;
          unsigned long max_desc_chars;
          ImageRestrictions_var image_restrictions;
          size_t max_image_count;

        private:
          void init_subobj(
            const ::NewsGate::Message::Automation::MessageRestrictions* src)
            throw(El::Exception);
        };
        
        typedef El::Python::SmartPtr<MessageRestrictions>
        MessageRestrictions_var;
        
        class Source : public El::Python::ObjectImpl,
          public ::NewsGate::Message::Automation::Source
        {
        public:
          Source(PyTypeObject *type = 0,
                 PyObject *args = 0,
                 PyObject *kwds = 0)
            throw(El::Exception);

          Source(const ::NewsGate::Message::Automation::Source& src)
            throw(Exception, El::Exception);

          void save(::NewsGate::Message::Automation::Source& src) const
            throw(El::Exception);

          virtual ~Source() throw() {}

          virtual void write(El::BinaryOutStream& ostr) const
            throw(El::Exception);
          
          virtual void read(El::BinaryInStream& istr) throw(El::Exception);
          
          class Type :
            public El::Python::ObjectTypeImpl<Source, Source::Type>
          {
          public:
            Type() throw(El::Python::Exception, El::Exception);
            static Type instance;

            PY_TYPE_MEMBER_STRING(title, "title", "Source title", true);
          
            PY_TYPE_MEMBER_STRING(url, "url", "Source url", false);
            
            PY_TYPE_MEMBER_STRING(html_link,
                                  "html_link",
                                  "Source channel link to html page",
                                  true);
          };
        };
      
        typedef El::Python::SmartPtr<Source> Source_var;
        
        class Image : public El::Python::ObjectImpl,
          public ::NewsGate::Message::Automation::Image
        {
        public:
          Image(PyTypeObject *type = 0,
                PyObject *args = 0,
                PyObject *kwds = 0)
            throw(El::Exception);

          Image(const El::LibXML::Python::Node* node) throw(El::Exception);

          Image(const ::NewsGate::Message::Automation::Image& img)
            throw(Exception, El::Exception);

          void save(::NewsGate::Message::Automation::Image& img) const
            throw(El::Exception);

          virtual ~Image() throw() {}

          virtual void write(El::BinaryOutStream& ostr) const
            throw(El::Exception);
          
          virtual void read(El::BinaryInStream& istr) throw(El::Exception);
          
          PyObject* py_read_size(PyObject* args) throw(El::Exception);

          static void images_from_doc(
            PyObject* images,
            PyObject* document,
            unsigned long operation,
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
            throw(Exception, El::Exception);

          static void insert_image(
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
            throw(El::Exception);
          
          class Type : public El::Python::ObjectTypeImpl<Image, Image::Type>
          {
          public:
            Type() throw(El::Python::Exception, El::Exception);
            static Type instance;

            PY_TYPE_METHOD_VARARGS(
              py_read_size,
              "read_size",
              "Reads image dimensions");

            PY_TYPE_MEMBER_STRING(src, "src", "Image url", false);
            PY_TYPE_MEMBER_STRING(alt, "alt", "Image alternate text", true);
            PY_TYPE_MEMBER_ULONG(width, "width", "Image width");
            PY_TYPE_MEMBER_ULONG(height, "height", "Image height");
            
            PY_TYPE_MEMBER_ENUM(origin,
                                Origin,
                                IO_ENCLOSURE,
                                "origin",
                                "Image origin");
            
            PY_TYPE_MEMBER_ENUM(status,
                                Status,
                                IS_SKIP,
                                "status",
                                "Image status");
            
            PY_TYPE_STATIC_MEMBER(DIM_UNDEFINED_, "DIM_UNDEFINED");
            PY_TYPE_STATIC_MEMBER(IO_UNKNOWN_, "IO_UNKNOWN");
            PY_TYPE_STATIC_MEMBER(IO_DESC_IMG_, "IO_DESC_IMG");
            PY_TYPE_STATIC_MEMBER(IO_DESC_LINK_, "IO_DESC_LINK");
            PY_TYPE_STATIC_MEMBER(IO_ENCLOSURE_, "IO_ENCLOSURE");
            PY_TYPE_STATIC_MEMBER(IS_VALID_, "IS_VALID");
            PY_TYPE_STATIC_MEMBER(IS_BAD_SRC_, "IS_BAD_SRC");
            PY_TYPE_STATIC_MEMBER(IS_BLACKLISTED_, "IS_BLACKLISTED");
            PY_TYPE_STATIC_MEMBER(IS_BAD_EXTENSION_, "IS_BAD_EXTENSION");
            PY_TYPE_STATIC_MEMBER(IS_TOO_SMALL_, "IS_TOO_SMALL");
            PY_TYPE_STATIC_MEMBER(IS_SKIP_, "IS_SKIP");
            PY_TYPE_STATIC_MEMBER(IS_DUPLICATE_, "IS_DUPLICATE");
            PY_TYPE_STATIC_MEMBER(IS_TOO_MANY_, "IS_TOO_MANY");
            PY_TYPE_STATIC_MEMBER(IS_TOO_BIG_, "IS_TOO_BIG");
            PY_TYPE_STATIC_MEMBER(IS_UNKNOWN_DIM_, "IS_UNKNOWN_DIM");

          private:
            El::Python::Object_var DIM_UNDEFINED_;
            El::Python::Object_var IO_UNKNOWN_;
            El::Python::Object_var IO_DESC_IMG_;
            El::Python::Object_var IO_DESC_LINK_;
            El::Python::Object_var IO_ENCLOSURE_;
            El::Python::Object_var IS_VALID_;
            El::Python::Object_var IS_BAD_SRC_;
            El::Python::Object_var IS_BLACKLISTED_;
            El::Python::Object_var IS_BAD_EXTENSION_;
            El::Python::Object_var IS_TOO_SMALL_;
            El::Python::Object_var IS_SKIP_;
            El::Python::Object_var IS_DUPLICATE_;
            El::Python::Object_var IS_TOO_MANY_;
            El::Python::Object_var IS_TOO_BIG_;
            El::Python::Object_var IS_UNKNOWN_DIM_;
          };

        private:
          void init(const El::LibXML::Python::Node* node) throw(El::Exception);

          static El::Net::HTTP::RequestParams* adjust_request_params(
            El::Net::HTTP::RequestParams* request_params,
            unsigned long execution_end) throw(El::Exception);
        };
      
        typedef El::Python::SmartPtr<Image> Image_var;
        
        class Message : public El::Python::ObjectImpl
        {
        public:
          Message(PyTypeObject *type = 0,
                  PyObject *args = 0,
                  PyObject *kwds = 0)
            throw(El::Exception);

          Message(const ::NewsGate::Message::Automation::Message& message)
            throw(Exception, El::Exception);

          void save(::NewsGate::Message::Automation::Message& message) const
            throw(El::Exception);

          virtual ~Message() throw() {}

          virtual void write(El::BinaryOutStream& ostr) const
            throw(El::Exception);
          
          virtual void read(El::BinaryInStream& istr) throw(El::Exception);
          
          static void keywords_from_doc(
            PyObject* keywords,
            PyObject* document,
            const char* xpath,            
            unsigned long operation,
            const char* separators,
            PyObject* keyword_override,
            const char* context)
            throw(Exception, El::Exception);

          class Type :
            public El::Python::ObjectTypeImpl<Message, Message::Type>
          {
          public:
            Type() throw(El::Python::Exception, El::Exception);
            static Type instance;

            PY_TYPE_MEMBER_STRING(title, "title", "Message title", true);
          
            PY_TYPE_MEMBER_STRING(description,
                                  "description",
                                  "Message description",
                                  true);

            PY_TYPE_MEMBER_STRING(url, "url", "Message url", false);          

            PY_TYPE_MEMBER_OBJECT(images,
                                  El::Python::Sequence::Type,
                                  "images",
                                  "Message images",
                                  false);

            PY_TYPE_MEMBER_OBJECT(keywords,
                                  El::Python::Sequence::Type,
                                  "keywords",
                                  "Message keywords",
                                  false);

            PY_TYPE_MEMBER_OBJECT(source,
                                  Source::Type,
                                  "source",
                                  "Message source",
                                  false);
            
            PY_TYPE_MEMBER_OBJECT(lang,
                                  El::Python::Lang::Type,
                                  "lang",
                                  "Message language",
                                  false);
            
            PY_TYPE_MEMBER_OBJECT(country,
                                  El::Python::Country::Type,
                                  "country",
                                  "Message country",
                                  false);
            
            PY_TYPE_MEMBER_ULONG(space, "space", "Message feed space");
            
            PY_TYPE_MEMBER_BOOL(valid,
                                "valid",
                                "Flags if message is a valid one");
            
            PY_TYPE_MEMBER_STRING(log,
                                  "log",
                                  "Message parsing log",
                                  true);
          };
    
          std::string title;
          std::string description;
          std::string url;
          El::Python::Sequence_var images;
          El::Python::Sequence_var keywords;
          Source_var source;
          El::Python::Lang_var lang;
          El::Python::Country_var country;
          unsigned long space;
          bool valid;
          std::string log;
        };
      
        typedef El::Python::SmartPtr<Message> Message_var;      
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Message
  {
    namespace Automation
    {
      //
      // ImageRestrictions class
      //
      inline
      void
      ImageRestrictions::write(El::BinaryOutStream& ostr) const
        throw(El::Exception)
      {
        ostr << (uint64_t)min_width << (uint64_t)min_height;
        ostr.write_array(black_list);
        ostr.write_set(extensions);
      }
      
      inline
      void
      ImageRestrictions::read(El::BinaryInStream& istr) throw(El::Exception)
      {
        uint64_t w = 0;
        uint64_t h = 0;
        
        istr >> w >> h;
        
        min_width = w;
        min_height = h;
        istr.read_array(black_list);
        istr.read_set(extensions);
      }

      //
      // MessageRestrictions class
      //
      inline
      MessageRestrictions::MessageRestrictions() throw()
          : max_title_len(0),
            max_desc_len(0),
            max_desc_chars(0)
      {
      }
      
      inline
      void
      MessageRestrictions::write(El::BinaryOutStream& ostr) const
        throw(El::Exception)
      {
        ostr << (uint64_t)max_title_len << (uint64_t)max_desc_len
             << (uint64_t)max_desc_chars << image_restrictions
             << (uint64_t)max_image_count;
      }

      inline
      void
      MessageRestrictions::read(El::BinaryInStream& istr) throw(El::Exception)
      {
        uint64_t tl = 0;
        uint64_t dl = 0;
        uint64_t dc = 0;
        uint64_t ic = 0;
        
        istr >> tl >> dl >> dc >> image_restrictions >> ic;

        max_title_len = tl;
        max_desc_len = dl;
        max_desc_chars = dc;
        max_image_count = ic;
      }      
        
      //
      // Image struct
      //
      inline
      Image::Image(const El::HTML::LightParser::Image& img,
                   Origin orig,
                   Status stat) throw()
          : src(img.src.c_str()),
            alt(img.alt.c_str()),
            origin(orig),
            status(stat),
            width(img.width),
            height(img.height)
      {
      }

      inline
      Image::Image(const char* url,
                   const char* alt_txt,
                   Origin orig,
                   Status stat,
                   unsigned long w,
                   unsigned long h) throw()
          : src(url ? url : ""),
            alt(alt_txt ? alt_txt : ""),
            origin(orig),
            status(stat),
            width(w),
            height(h)
      {
      }

      inline
      void
      Image::write(El::BinaryOutStream& ostr) const
        throw(El::Exception)
      {
        ostr << (uint64_t)width << (uint64_t)height << src << alt
             << (uint32_t)origin << (uint32_t)status;
      }

      inline
      void
      Image::read(El::BinaryInStream& istr) throw(El::Exception)
      {
        uint64_t w = 0;
        uint64_t h = 0;
        uint32_t orig = 0;
        uint32_t stat = 0;
        
        istr >> w >> h >> src >> alt >> orig >> stat;

        width = w;
        height = h;
        origin = (Origin)orig;
        status = (Status)stat;
      }

      //
      // Source struct
      //
      inline
      void
      Source::write(El::BinaryOutStream& ostr) const
        throw(El::Exception)
      {
        ostr << title << url << html_link;        
      }

      inline
      void
      Source::read(El::BinaryInStream& istr) throw(El::Exception)
      {
        istr >> title >> url >> html_link;        
      }      
      
      //
      // Message struct
      //
      inline
      void
      Message::write(El::BinaryOutStream& ostr) const
        throw(El::Exception)
      {
        ostr << title << description << url << source << lang << country
             << (uint32_t)space << (uint8_t)valid << log;
        
        ostr.write_array(images);
        ostr.write_array(keywords);        
      }

      inline
      void
      Message::read(El::BinaryInStream& istr) throw(El::Exception)
      {
        uint8_t iv = 0;
        uint32_t sp = 0;
        
        istr >> title >> description >> url >> source >> lang >> country
             >> sp >> iv >> log;

        valid = iv;
        space = (Feed::Space)sp;
        
        istr.read_array(images);
        istr.read_array(keywords);
      }
      
      namespace Python
      {
        //
        // ImageRestrictions::Type class
        //
        inline
        ImageRestrictions::Type::Type()
          throw(El::Python::Exception, El::Exception)
            : El::Python::ObjectTypeImpl<ImageRestrictions,
                                         ImageRestrictions::Type>(
              "newsgate.message.ImageRestrictions",
              "Object representing image restrictions")
        {
        }
      
        //
        // MessageRestrictions::Type class
        //
        inline
        MessageRestrictions::Type::Type()
          throw(El::Python::Exception, El::Exception)
            : El::Python::ObjectTypeImpl<MessageRestrictions,
                                         MessageRestrictions::Type>(
              "newsgate.message.MessageRestrictions",
              "Object representing message restrictions")
        {
        }
      
        //
        // Image::Type class
        //
        inline
        Image::Type::Type() throw(El::Python::Exception, El::Exception)
            : El::Python::ObjectTypeImpl<Image, Image::Type>(
              "newsgate.message.Image",
              "Object representing message image properties")
        {
          DIM_UNDEFINED_ = PyLong_FromLong(Automation::Image::DIM_UNDEFINED);
          IO_UNKNOWN_ = PyLong_FromLong(Automation::Image::IO_UNKNOWN);
          IO_DESC_IMG_ = PyLong_FromLong(Automation::Image::IO_DESC_IMG);
          IO_DESC_LINK_ = PyLong_FromLong(Automation::Image::IO_DESC_LINK);
          IO_ENCLOSURE_ = PyLong_FromLong(Automation::Image::IO_ENCLOSURE);
          IS_VALID_ = PyLong_FromLong(Automation::Image::IS_VALID);
          IS_BAD_SRC_ = PyLong_FromLong(Automation::Image::IS_BAD_SRC);
          
          IS_BLACKLISTED_ =
            PyLong_FromLong(Automation::Image::IS_BLACKLISTED);

          IS_BAD_EXTENSION_ =
            PyLong_FromLong(Automation::Image::IS_BAD_EXTENSION);
          
          IS_TOO_SMALL_ = PyLong_FromLong(Automation::Image::IS_TOO_SMALL);
          IS_SKIP_ = PyLong_FromLong(Automation::Image::IS_SKIP);
          IS_DUPLICATE_ = PyLong_FromLong(Automation::Image::IS_DUPLICATE);
          IS_TOO_MANY_ = PyLong_FromLong(Automation::Image::IS_TOO_MANY);
          IS_TOO_BIG_ = PyLong_FromLong(Automation::Image::IS_TOO_BIG);
          IS_UNKNOWN_DIM_ = PyLong_FromLong(Automation::Image::IS_UNKNOWN_DIM);
        }
      
        //
        // Python::Source class
        //
        inline
        Source::Source(PyTypeObject *type, PyObject *args, PyObject *kwds)
          throw(El::Exception) :
            El::Python::ObjectImpl(type ? type : &Type::instance)
        {
        }

        inline
        void
        Source::write(El::BinaryOutStream& ostr) const throw(El::Exception)
        {
          ostr << title << url << html_link;
        }

        inline
        void
        Source::read(El::BinaryInStream& istr) throw(El::Exception)
        {
          istr >> title >> url >> html_link;
        }
        
        //
        // Source::Type class
        //
        inline
        Source::Type::Type() throw(El::Python::Exception, El::Exception)
            : El::Python::ObjectTypeImpl<Source, Source::Type>(
              "newsgate.message.Source",
              "Object representing message source properties")
        {
        }
      
        //
        // Python::Message class
        //
        inline
        Message::Message(PyTypeObject *type, PyObject *args, PyObject *kwds)
          throw(El::Exception) :
            El::Python::ObjectImpl(type ? type : &Type::instance),
            images(new El::Python::Sequence()),
            keywords(new El::Python::Sequence()),
            source(new Source()),
            lang(new El::Python::Lang()),
            country(new El::Python::Country()),
            space(Feed::SP_UNDEFINED),
            valid(false)
        {
        }

        //
        // Message::Type class
        //
        inline
        Message::Type::Type() throw(El::Python::Exception, El::Exception)
            : El::Python::ObjectTypeImpl<Message, Message::Type>(
              "newsgate.message.Message",
              "Object representing message properties")
        {
        }      
      }
    }
  }
}

#endif // _NEWSGATE_SERVER_COMMONS_MESSAGE_AUTOMATION_AUTOMATION_HPP_
