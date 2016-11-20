/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Commons/Message/Categorizer.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_COMMONS_MESSAGE_CATEGORIZER_HPP_
#define _NEWSGATE_SERVER_COMMONS_MESSAGE_CATEGORIZER_HPP_

#include <stdint.h>

#include <string>
#include <vector>

#include <ext/hash_map>

#include <El/Exception.hpp>
#include <El/Locale.hpp>
#include <El/String/LightString.hpp>
#include <El/BinaryStream.hpp>
#include <El/Hash/Hash.hpp>

#include <Commons/Message/Message.hpp>

namespace NewsGate
{ 
  namespace Message
  {
    struct Categorizer
    {
      struct Category
      {
        struct Locale
        {
          Message::StringConstPtr name;
          Message::StringConstPtr title;
          Message::StringConstPtr short_title;
          Message::StringConstPtr description;
          Message::StringConstPtr keywords;

          Locale() throw() {}
          
          Locale(const char* nm,
                 const char* tl,
                 const char* st,
                 const char* ds,
                 const char* kw) throw();

          void swap(Locale& l) throw();
          void absorb(const Locale& l) throw(El::Exception);
          
          void write(El::BinaryOutStream& bstr) const throw(El::Exception);
          void read(El::BinaryInStream& bstr) throw(El::Exception);
        };
        
        typedef __gnu_cxx::hash_map<El::Locale,
                                    Locale,
                                    El::Hash::Locale>
        LocaleMap;
        
        typedef uint64_t Id;
        typedef std::vector<Id> IdArray;
        typedef std::vector<std::string> ExpressionArray;

        struct RelMsg
        {
          ::NewsGate::Message::Id id;
          uint64_t updated;

          RelMsg() throw() : updated(0) {}
          
          void write(El::BinaryOutStream& bstr) const throw(El::Exception);
          void read(El::BinaryInStream& bstr) throw(El::Exception);
        };

        typedef std::vector<RelMsg> RelMsgArray;
        
        Message::StringConstPtr name;
        uint8_t searcheable;
        IdArray children;
        LocaleMap locales;
        ExpressionArray expressions;
        RelMsgArray included_messages;
        RelMsgArray excluded_messages;

        Category() throw(El::Exception) : searcheable(0) {}

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);
      };

      typedef __gnu_cxx::hash_map<Category::Id,
                                  Category,
                                  El::Hash::Numeric<Category::Id> >
      CategoryMap;

      uint64_t    stamp;
      std::string source;
      uint8_t     enforced;
      CategoryMap categories;

      Categorizer() throw(El::Exception) : stamp(0), enforced(0) {}
      
      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };  
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
    // Categorizer::Category::Locale struct
    //
    inline
    Categorizer::Category::Locale::Locale(const char* nm,
                                          const char* tl,
                                          const char* st,
                                          const char* ds,
                                          const char* kw) throw()
        : name(nm),
          title(tl),
          short_title(st),
          description(ds),
          keywords(kw)
    {
    }

    inline
    void
    Categorizer::Category::Locale::swap(Locale& l) throw()
    {
      name.swap(l.name);
      title.swap(l.title);
      short_title.swap(l.short_title);
      description.swap(l.description);
      keywords.swap(l.keywords);
    }
    
    inline
    void
    Categorizer::Category::Locale::absorb(const Locale& l) throw(El::Exception)
    {
      if(name.empty() && !l.name.empty())
      {
        name = l.name;
      }

      if(title.empty() && !l.title.empty())
      {
        title = l.title;
      }

      if(short_title.empty() && !l.short_title.empty())
      {
        short_title = l.short_title;
      }

      if(description.empty() && !l.description.empty())
      {
        description = l.description;
      }

      if(keywords.empty() && !l.keywords.empty())
      {
        keywords = l.keywords;
      }
    }

    inline
    void
    Categorizer::Category::Locale::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr << name << title << short_title << description << keywords;
    }

    inline
    void
    Categorizer::Category::Locale::read(El::BinaryInStream& bstr)
      throw(El::Exception)
    {
      bstr >> name >> title >> short_title >> description >> keywords;      
    }
    
    //
    // Categorizer struct
    //
    inline
    void
    Categorizer::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << stamp << enforced << source;
      bstr.write_map(categories);
    }

    inline
    void
    Categorizer::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> stamp >> enforced >> source;
      bstr.read_map(categories);
    }

    //
    // Categorizer::Category struct
    //
    inline
    void
    Categorizer::Category::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr << name << searcheable;
      bstr.write_array(children);
      bstr.write_map(locales);
      bstr.write_array(expressions);
      bstr.write_array(included_messages);
      bstr.write_array(excluded_messages);
    }

    inline
    void
    Categorizer::Category::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> name >> searcheable;
      bstr.read_array(children);
      bstr.read_map(locales);
      bstr.read_array(expressions);
      bstr.read_array(included_messages);
      bstr.read_array(excluded_messages);
    }

    //
    // Categorizer::Category::RelMsg struct
    //
    inline
    void
    Categorizer::Category::RelMsg::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr << id << updated;
    }
    
    inline
    void
    Categorizer::Category::RelMsg::read(El::BinaryInStream& bstr)
      throw(El::Exception)
    {
      bstr >> id >> updated;
    }
  }
}

#endif // _NEWSGATE_SERVER_COMMONS_MESSAGE_CATEGORIZER_HPP_
