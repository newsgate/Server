/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Commons/Message/Message.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_COMMONS_MESSAGE_MESSAGE_HPP_
#define _NEWSGATE_SERVER_COMMONS_MESSAGE_MESSAGE_HPP_

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <memory>

#include <google/sparse_hash_set>

#include <ace/OS.h>

#include <El/Exception.hpp>
#include <El/CRC.hpp>
#include <El/String/Manip.hpp>
#include <El/String/LightString.hpp>
#include <El/String/ListParser.hpp>
#include <El/BinaryStream.hpp>
#include <El/Moment.hpp>
#include <El/Country.hpp>
#include <El/Lang.hpp>
#include <El/Hash/Hash.hpp>
#include <El/String/SharedString.hpp>
#include <El/LightArray.hpp>
#include <El/ArrayPtr.hpp>
#include <El/SyncPolicy.hpp>

#include <El/RefCount/All.hpp>

#include <Commons/Feed/Types.hpp>

namespace NewsGate
{ 
  namespace Message
  {
    EL_EXCEPTION(Exception, El::ExceptionBase);
    EL_EXCEPTION(InvalidArg, Exception);
    
    typedef uint32_t LocalId;
    
    struct LocalIdArray : public std::vector<Message::LocalId>
    {
      std::string string() const throw(El::Exception);
    };

    typedef std::auto_ptr<LocalIdArray> LocalIdArrayPtr;

    struct LocalCode
    {
      LocalId id;
      uint64_t published;
      
      LocalCode() throw();
        
      void clear() throw();

      El::Moment pub_date() const throw(El::Exception);      
      std::string string() const throw(El::Exception);

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };

    class LocalCodeArray : public std::vector<LocalCode>
    {
    public:
      LocalCodeArray() throw(El::Exception);
      
      std::string string() const throw(El::Exception);
    };

    typedef std::auto_ptr<LocalCodeArray> LocalCodeArrayPtr;

    struct Id
    {
      uint64_t data;

      Id() throw();
      Id(const uint64_t& data_val) throw();
      Id(const char* val, bool dense = false) throw(InvalidArg, El::Exception);

      bool operator==(const Id& id) const throw();
      bool operator!=(const Id& id) const throw();
      bool operator<(const Id& id) const throw();
      bool operator>(const Id& id) const throw();

      bool valid() const throw();
      
      uint32_t src_id() const throw() { return (uint32_t)data; }

      std::string string(bool dense = false) const throw(El::Exception);

      void clear() throw();

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);

      static const Id zero;
      static const Id nonexistent;
    };

    typedef uint64_t SourceId;
    
    struct MessageIdHash
    {
      size_t operator()(const Id& val) const throw();
    };    

    class IdArray : public std::vector<Message::Id>
    {
    public:
      IdArray(size_t size) throw(El::Exception);
      IdArray() throw(El::Exception);
    };

    typedef std::auto_ptr<IdArray> IdArrayPtr;

    struct IdSet : public google::sparse_hash_set<Id, MessageIdHash>
    {
      IdSet() throw(El::Exception) { set_deleted_key(Id::zero); }
    };

    typedef El::String::SharedStringConstPtr<int> StringConstPtr;
    typedef El::Hash::SharedStringConstPtr<int> StringConstPtrHash;
    
    struct Source
    {
      Source() throw(El::Exception);
      
      StringConstPtr title;
      StringConstPtr html_link;

      void clear() throw();

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);

      void copy(const Source& src) throw();
    };

    struct Image
    {
      uint16_t width;
      uint16_t height;
      El::String::LightString src;

      Image() throw(El::Exception);

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };

    struct Content : public virtual El::RefCount::Interface
    {
      Source                  source;
      El::String::LightString url;

      Content() throw(El::Exception);
      virtual ~Content() throw();

      void copy(const Content& src) throw();
      virtual void clear() throw();

      virtual void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      virtual void read(El::BinaryInStream& bstr) throw(El::Exception);

    private:
      void operator=(const Content&);
      Content(const Content&);
    };
    
    struct ImageThumb
    {
    private:
      uint16_t width_;
      uint16_t height_;

    public:
      uint32_t       hash;
      StringConstPtr type;      
      
      typedef El::ArrayPtr<unsigned char> ImagePtr;
      
      ImagePtr image;
      uint32_t length;
      
      ImageThumb() throw(El::Exception);
      ImageThumb(const ImageThumb& src) throw(El::Exception);

      ImageThumb& operator=(const ImageThumb& src) throw(El::Exception);

      uint16_t width() const throw() { return width_; }
      uint16_t height() const throw() { return height_ & 0x7FFF; }
      bool cropped() const throw() { return (height_ & 0x8000) != 0; }
      
      void width(uint16_t value) throw();
      void height(uint16_t value) throw();
      void cropped(bool value) throw();      
      
      bool empty() const throw() { return length == 0; }

      void steal(ImageThumb& src) throw(El::Exception);
      void drop_image() throw();
      void clear() throw();

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);

      void read_image(const char* path, bool calc_crc)
        throw(Exception, El::Exception);

      void read_image(El::BinaryInStream bstr, bool calc_crc)
        throw(El::Exception);
      
      void write_image(const char* path) const throw(Exception, El::Exception);
      void write_image(El::BinaryOutStream bstr) const throw(El::Exception);
    };

    typedef std::auto_ptr<ImageThumb> ImageThumbPtr;
    typedef El::LightArray<ImageThumb, uint16_t> ImageThumbArray; 
    
    struct RawImage : public Image
    {
      El::String::LightString alt;
      ImageThumbArray thumbs;

      RawImage() throw(El::Exception);

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };
    
    typedef std::vector<RawImage> RawImageArray;
    typedef std::auto_ptr<RawImageArray> RawImageArrayPtr;
    
    struct RawContent : public virtual Content,
                        public virtual El::RefCount::DefaultImpl<
      El::Sync::ThreadPolicy>
    {
      El::String::LightString title;
      El::String::LightString description;
      El::String::LightString keywords;
      RawImageArrayPtr        images;

      RawContent() throw() {}
      virtual ~RawContent() throw();

      void copy(const RawContent& src) throw();

      virtual void clear() throw();

      virtual void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      virtual void read(El::BinaryInStream& bstr) throw(El::Exception);      

      void write_images(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read_images(El::BinaryInStream& bstr) throw(El::Exception);

    private:
      RawContent(const RawContent&);
      void operator=(const RawContent&);
    };

    typedef El::RefCount::SmartPtr<RawContent>
    RawContent_var;

    struct RawMessage
    {
      RawMessage() throw(El::Exception);

      Id                      id;
      uint64_t                published;      
      Feed::Space             space;
      El::String::LightString source_url;
      SourceId                source_id;
      RawContent_var          content;
      El::Country             country;
      El::Lang                lang;
      
      void id_from_url(const char* msg_url, const char* src_url) throw();
      void clear() throw();

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);

      const Id& get_id() const throw() { return id; }
    };

    class RawMessageArray : public std::vector<RawMessage>
    {
    public:
      RawMessageArray() throw(El::Exception);
      RawMessageArray(size_t size) throw(El::Exception);
    };

    typedef std::auto_ptr<RawMessageArray> RawMessageArrayPtr;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{ 
  namespace Message
  {
    //
    // LocalCode class
    //
    inline
    LocalCode::LocalCode() throw()
        : id(0),
          published(0)
    {
    }

    inline
    void
    LocalCode::clear() throw()
    {
      id = 0;
      published = 0;
    }

    inline
    El::Moment
    LocalCode::pub_date() const throw(El::Exception)
    {
      return El::Moment(ACE_Time_Value(published));
    }

    inline
    std::string
    LocalCode::string() const throw(El::Exception)
    {
      char buff[26];
      
      sprintf(buff,
              "%08lX:%016llX",
              (unsigned long)id,
              (unsigned long long)published);
      
      return buff;
    }

    inline
    void
    LocalCode::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << id << published;
    }

    inline
    void
    LocalCode::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> id >> published;
    }

    //
    // LocalIdArray class
    //
    inline
    std::string 
    LocalIdArray::string() const throw(El::Exception)
    {
      El::ArrayPtr<char> buff(new char[9 * size() + 1]);
      char* ptr = buff.get();

      for(const_iterator it = begin(); it != end(); it++)
      {
        if(it != begin())
        {
          *ptr++ = ',';
        }
        
        sprintf(ptr, "%08lX", (unsigned long)*it);
        ptr += 9 - 1;
      }

      *ptr = '\0';
      
      return buff.get();
    }

    //
    // LocalCodeArray class
    //
    inline
    LocalCodeArray::LocalCodeArray() throw(El::Exception)
    {
    }

    inline
    std::string 
    LocalCodeArray::string() const throw(El::Exception)
    {
      El::ArrayPtr<char> buff(new char[26 * size() + 1]);
      char* ptr = buff.get();

      for(const_iterator it = begin(); it != end(); it++)
      {
        if(it != begin())
        {
          *ptr++ = ',';
        }
        
        sprintf(ptr,
                "%08lX:%016llX",
                (unsigned long)it->id,
                (unsigned long long)it->published);
        
        ptr += 26 - 1;
      }

      *ptr = '\0';      
      return buff.get();
    }

    //
    // Id class
    //
    inline
    Id::Id() throw() : data(0)
    {
    }

    inline
    Id::Id(const uint64_t& data_val) throw() : data(data_val)
    {
    }

    inline
    Id::Id(const char* val, bool dense) throw(InvalidArg, El::Exception)
    {
      if(val == 0 || *val == '\0')
      {
        throw InvalidArg(
          "NewsGate::Message::Id::Id: argument is null or empty");
      }

      if(dense)
      {
        try
        {
          El::String::Manip::base64_decode(val,
                                           (unsigned char*)&data,
                                           sizeof(data));
        }
        catch(const El::String::Manip::InvalidArg& e)
        {
          std::ostringstream ostr;
          ostr << "NewsGate::Message::Id::Id: invalid format of '"
               << val << "'. Details: " << e;
        
          throw InvalidArg(ostr.str());
        }

        return;
      }

      if(strlen(val) > 16)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::Id::Id: invalid format of '"
             << val << "' (1)";
        
        throw InvalidArg(ostr.str());        
      }

      unsigned long long d = 0;
      
      char rest[17];
      memset(rest, 0, sizeof(rest));
      
      if(sscanf(val, "%llX%s", &d, rest) != 1)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::Message::Id::Id: invalid format of '"
             << val << "' (2)";
        
        throw InvalidArg(ostr.str());
      }

      data = d;
    }

    inline
    bool
    Id::valid() const throw()
    {
      return *this != zero && *this != nonexistent;
    }
    
    inline
    bool
    Id::operator==(const Id& id) const throw()
    {
      return data == id.data;
    }
    
    inline
    bool
    Id::operator!=(const Id& id) const throw()
    {
      return data != id.data;
    }
    
    inline
    bool
    Id::operator<(const Id& id) const throw()
    {
      return data < id.data;
    }
    
    inline
    bool
    Id::operator>(const Id& id) const throw()
    {
      return data > id.data;
    }

    inline
    void
    Id::clear() throw()
    {
      data = 0;
    }

    inline
    void
    Id::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << data;
    }
    
    inline
    void
    Id::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> data;
    }
    
    inline
    std::string
    Id::string(bool dense) const throw(El::Exception)
    {
      if(dense)
      {
        std::string val;
        El::String::Manip::base64_encode((const unsigned char*)&data,
                                         sizeof(data),
                                         val);
        return val;
      }
      
      char buff[17];
      sprintf(buff, "%016llX", (unsigned long long)data);
      
      return buff;
    }

    //
    // MessageIdHash class
    //
    inline
    size_t
    MessageIdHash::operator()(const Id& val) const throw()
    { // Most random bits (CRC of article link) should be the rightmost.
      // Not doing so incredibly degrade google hash tables !
        
      return (val.data << 32) | (val.data >> 32);
    }

    //
    // Source class
    //
    inline
    Source::Source() throw(El::Exception)
    {
    }
    
    inline
    void
    Source::copy(const Source& src) throw()
    {
      title = src.title;
      html_link = src.html_link;
    }
    
    inline
    void
    Source::clear() throw()
    {
      title.clear();
      html_link.clear();
    }
    
    inline
    void
    Source::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << title << html_link;
    }

    inline
    void
    Source::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> title >> html_link;
    }

    //
    // Image struct
    //
    inline
    Image::Image() throw(El::Exception)
        : width(0),
          height(0)
    {
    }

    inline
    void
    Image::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << src << width << height;
    }

    inline
    void
    Image::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> src >> width >> height;
    }

    //
    // Image struct
    //

    inline
    ImageThumb::ImageThumb() throw(El::Exception)
        : width_(0),
          height_(0),
          hash(0),
          length(0)
    {
    }

    inline
    ImageThumb::ImageThumb(const ImageThumb& src) throw(El::Exception)
        : width_(0),
          height_(0),
          hash(0),
          length(0)
    {
      *this = src;
    }

    inline
    ImageThumb&
    ImageThumb::operator=(const ImageThumb& src) throw(El::Exception)
    {
      width_ = src.width_;
      height_ = src.height_;
      type = src.type;
      hash = src.hash;
      length = src.length;

      image.reset(new unsigned char[length]);
      memcpy(image.get(), src.image.get(), length * sizeof(image.get()[0]));

      return *this;
    }

    inline
    void
    ImageThumb::write_image(El::BinaryOutStream bstr) const
      throw(El::Exception)
    {
      bstr.write_bytes(image.get(), length * sizeof(image.get()[0]));
    }

    inline
    void
    ImageThumb::steal(ImageThumb& src) throw(El::Exception)
    {
      width_ = src.width_;
      height_ = src.height_;
      type = src.type;
      hash = src.hash;
      length = src.length;

      image.reset(src.image.release());
    }

    inline
    void
    ImageThumb::drop_image() throw()
    {
      length = 0;
      image.reset(0);
    }

    inline
    void
    ImageThumb::clear() throw()
    {
      drop_image();
      width_ = 0;
      height_ = 0;
      type.clear();
      hash = 0;
    }
    
    inline
    void
    ImageThumb::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << width_ << height_ << type << hash << length;

      if(length)
      {
        bstr.write_raw_bytes(image.get(), length);
      }
    }

    inline
    void
    ImageThumb::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> width_ >> height_ >> type >> hash >> length;

      if(length)
      {
        image.reset(new unsigned char[length]);
        bstr.read_raw_bytes(image.get(), length);
      }
      else
      {
        image.reset(0);
      }      
    }

    inline
    void
    ImageThumb::width(uint16_t value) throw()
    {
      width_ = value;
    }

    inline
    void
    ImageThumb::height(uint16_t value) throw()
    {
      height_ = (0x7FFF & value) | (height_ & 0x8000);
    }
    
    inline
    void
    ImageThumb::cropped(bool value) throw()
    {
      if(value)
      {
        height_ |= 0x8000;
      }
      else
      {
        height_ &= 0x7FFF;
      }
    }
    
    //
    // Content class
    //
    inline
    Content::Content() throw(El::Exception)
    {
    }
    
    inline
    Content::~Content() throw()
    {
    }
    
    inline
    void
    Content::copy(const Content& src) throw()
    {
      url = src.url;
      source = src.source;
    }
    
    inline
    void
    Content::clear() throw()
    {
      source.clear();
      url.clear();
    }

    inline
    void
    Content::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << source << url;
    }    

    inline
    void
    Content::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> source >> url;
    }
    
    //
    // RawImage class
    //
    inline
    RawImage::RawImage() throw(El::Exception)
    {
    }

    inline
    void
    RawImage::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      Image::write(bstr);
      bstr << alt << thumbs;
    }
    
    inline
    void
    RawImage::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      Image::read(bstr);
      bstr >> alt >> thumbs;
    }
    
    //
    // RawContent class
    //
    inline
    RawContent::~RawContent() throw()
    {
    }

    inline
    void
    RawContent::copy(const RawContent& src) throw()
    {
      Content::copy(src);
      title = src.title;
      description = src.description;
      keywords = src.keywords;

      if(src.images.get())
      {
        images.reset(new RawImageArray(*src.images.get()));
      }
      else
      {
        images.reset(0);
      }      
    }
    
    inline
    void
    RawContent::clear() throw()
    {
      title.clear();
      description.clear();
      keywords.clear();
      images.reset(0);
    }

    inline
    void
    RawContent::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      Content::write(bstr);
      bstr << title << description << keywords;
      write_images(bstr);
    }
    
    inline
    void
    RawContent::read(El::BinaryInStream& bstr) throw(El::Exception)
    {      
      Content::read(bstr);
      bstr >> title >> description >> keywords;
      read_images(bstr);
    }    
    
    inline
    void
    RawContent::write_images(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      uint32_t img_count = images.get() ? images->size() : 0;
      bstr << img_count;

      if(img_count)
      {
        for(RawImageArray::const_iterator it = images->begin();
            it != images->end(); it++)
        {
          it->write(bstr);
        }
      }
    }

    inline
    void
    RawContent::read_images(El::BinaryInStream& bstr) throw(El::Exception)
    {      
      uint32_t img_count = 0;
      bstr >> img_count;

      if(img_count)
      {
        images.reset(new RawImageArray(img_count));

        for(uint32_t i = 0; i < img_count; i++)
        {
          (*images)[i].read(bstr);
        }
      }
      else
      {
        images.reset(0);
      }
    }
 
    //
    // RawMessage class
    //
    inline
    RawMessage::RawMessage() throw(El::Exception)
        : published(0),
          space(Feed::SP_UNDEFINED),
          source_url((const char*)""),
          source_id(0),
          content(new RawContent())
    {
    }
    
    inline
    void
    RawMessage::clear() throw()
    {
      id.clear();
      published = 0;
      space = Feed::SP_UNDEFINED;
      source_url.clear();
      source_id = 0;
      country = El::Country();
      lang = El::Lang();

      content = new RawContent();
    }

    inline
    void
    RawMessage::id_from_url(const char* msg_url, const char* src_url) throw()
    {
      uint32_t msg_part = 0;
      
      if(msg_url != 0)
      {
        El::CRC(msg_part, (const unsigned char*)msg_url, strlen(msg_url));
      }

      uint32_t src_part = 0;
      
      if(src_url != 0)
      {
        El::CRC(src_part, (const unsigned char*)src_url, strlen(src_url));
      }

      id = (((uint64_t)msg_part) << 32) | ((uint64_t)src_part);
    }    
    
    inline
    void
    RawMessage::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << id << published << space << source_url << source_id << country
           << lang;

      content->write(bstr);
    }

    inline
    void
    RawMessage::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> id >> published >> space >> source_url >> source_id >> country
           >> lang;
      
      content = new RawContent();
      content->read(bstr);
    }

    //
    // RawMessageArray class
    //
    inline
    RawMessageArray::RawMessageArray() throw(El::Exception)
    {
    }

    inline
    RawMessageArray::RawMessageArray(size_t size)
      throw(El::Exception) : std::vector<RawMessage>(size)
    {
    }

    //
    // IdArray class
    //

    inline
    IdArray::IdArray(size_t size) throw(El::Exception)
        : std::vector<Message::Id>(size)
    {
    }

    inline
    IdArray::IdArray() throw(El::Exception)
    {
    }
  }
}

#endif // _NEWSGATE_SERVER_COMMONS_MESSAGE_MESSAGE_HPP_
