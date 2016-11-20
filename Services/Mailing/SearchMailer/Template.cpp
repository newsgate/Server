/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Mailing/SearchMailer/Template.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <sstream>
#include <string>
#include <iostream>

#include <El/Exception.hpp>
#include <El/Lang.hpp>
#include <El/String/Template.hpp>
#include <El/Cache/EncodingAwareLocalizedTemplate.hpp>
#include <El/Localization/Loc.hpp>

#include <Commons/Message/TransportImpl.hpp>

#include "Template.hpp"

namespace NewsGate
{
  namespace SearchMailing
  {
    //
    // MessageWriter class
    //
    class MessageWriter : public Message::StoredMessage::MessageBuilder
    {      
    public:

      MessageWriter(std::ostream& ostr,
                    El::Cache::EncodableVariables::ValueEncoding encoding)
        throw(El::Exception);

      virtual ~MessageWriter() throw() {}

    public:

      bool empty;
      
    private:

      virtual bool word(const char* text, Message::WordPosition position)
        throw(El::Exception);

      virtual bool interword(const char* text) throw(El::Exception);
      virtual bool segmentation() throw(El::Exception) { return true; }

    private:
    
      std::ostream& output_;
      El::Cache::EncodableVariables::ValueEncoding encoding_;
    };

    MessageWriter::MessageWriter(
      std::ostream& ostr,
      El::Cache::EncodableVariables::ValueEncoding encoding)
      throw(El::Exception)
        : empty(true),
          output_(ostr),
          encoding_(encoding)
    {
    }
    
    bool
    MessageWriter::word(const char* text, Message::WordPosition position)
      throw(El::Exception)
    {
      empty = false;
      
      El::Cache::EncodableVariables::encode(text, output_, encoding_);
      return true;
    }

    bool
    MessageWriter::interword(const char* text) throw(El::Exception)
    {
      empty = false;
      
      El::Cache::EncodableVariables::encode(text, output_, encoding_);
      return true;
    }
      
    //
    // RuntimeVariables class
    //
    class RuntimeVariables : public El::Cache::EncodableVariables
    {
    public:
      RuntimeVariables(const Subscription& subscription,
                       const char* endpoint,
                       const Message::Transport::StoredMessageArray& messages,
                       const El::Cache::TextTemplateFile* message_template,
                       bool prn_src,
                       bool prn_country)
        throw();
      
      virtual ~RuntimeVariables() throw() {}
      
    private:
      
      virtual bool write(const El::String::Template::Chunk& chunk,
                         std::ostream& output) const
        throw(El::Exception);

      bool write_message(std::ostream& output) const throw(El::Exception);
      
    private:
      const Subscription& subscription_;
      std::string endpoint_;
      const Message::Transport::StoredMessageArray& messages_;
      const El::Cache::TextTemplateFile* message_template_;
      std::string dot_zw_space_;
      bool prn_src_;
      bool prn_country_;

      mutable const Message::StoredMessage* current_message_;
    };
    
    RuntimeVariables::RuntimeVariables(
      const Subscription& subscription,
      const char* endpoint,
      const Message::Transport::StoredMessageArray& messages,
      const El::Cache::TextTemplateFile* message_template,
      bool prn_src,
      bool prn_country)
        throw()
        : subscription_(subscription),
          endpoint_(endpoint),
          messages_(messages),
          message_template_(message_template),
          prn_src_(prn_src),
          prn_country_(prn_country),
          current_message_(0)
    {
      El::String::Manip::wchar_to_utf8(L"\u200B.\u200B", dot_zw_space_);
    }

    bool
    RuntimeVariables::write(const El::String::Template::Chunk& chunk,
                            std::ostream& output) const
      throw(El::Exception)
    {
      const char* var_name = chunk.text.c_str();
      
      if(strcmp(var_name, "endpoint") == 0)
      {
        return encode(endpoint_.c_str(), output, chunk);
      }
      else if(strcmp(var_name, "subs_lang") == 0)
      {
        return encode(subscription_.lang.l3_code(), output, chunk);
      }
      else if(strcmp(var_name, "subs_id") == 0)
      {
        return encode(subscription_.id.string(El::Guid::GF_DENSE).c_str(),
                      output,
                      chunk);
      }
      else if(strcmp(var_name, "subs_email") == 0)
      {
        return encode(subscription_.email.c_str(), output, chunk);
      }
      else if(strcmp(var_name, "subs_query") == 0)
      {
        return encode(subscription_.query.c_str(), output, chunk);
      }
      else if(strcmp(var_name, "subs_modifier") == 0)
      {
        if(!subscription_.modifier.empty())
        {
          output << "&";
          encode(subscription_.modifier.c_str(), output, chunk);
        }
        
        return true;
      }
      else if(strcmp(var_name, "subs_filter") == 0)
      {
        if(!subscription_.filter.empty())
        {
          output << "&";
          encode(subscription_.filter.c_str(), output, chunk);
        }
        
        return true;
      }
      else if(strcmp(var_name, "messages") == 0)
      {
        return write_message(output);
      }
      else if(strcmp(var_name, "msg_title") == 0)
      {
        if(current_message_)
        {
          MessageWriter writer(
            output,
            (El::Cache::EncodableVariables::ValueEncoding)reinterpret_cast<
              unsigned long>(chunk.tag));
          
          current_message_->assemble_title(writer);

          if(writer.empty)
          {
            output << "---";
          }
          
          return true;
        }
      }
      else if(strcmp(var_name, "msg_encoded_id") == 0)
      {
        if(current_message_)
        {
          return encode(current_message_->id.string(true).c_str(),
                        output,
                        chunk);
        }
      }
      else if(strcmp(var_name, "msg_url") == 0)
      {
        if(current_message_)
        {
          return encode(current_message_->content->url.c_str(), output, chunk);
        }
      }
      else if(strcmp(var_name, "msg_src_title_line") == 0)
      {
        if(current_message_)
        {
          if(!prn_src_)
          {
            return true;
          }

          const char* src_title = current_message_->source_title.c_str();
          bool has_dot = strchr(src_title, '.') != 0;
          bool printed = false;

          if(has_dot)
          {
            unsigned long enc = reinterpret_cast<unsigned long>(chunk.tag);

            switch(enc)
            {
            case VE_XML:
            case VE_XML_STRICT:  
              {
                std::ostringstream ostr;
                encode(src_title, ostr, chunk);
                
                El::String::Manip::replace(ostr.str().c_str(),
                                           ".",
                                           "&#x200B;.&#x200B;",
                                           output);
                printed = true;                
                break;
              }
            case VE_NONE:
              {
                El::String::Manip::replace(src_title,
                                           ".",
                                           dot_zw_space_.c_str(),
                                           output);
                printed = true;
                break;
              }
            default:
              {
                break;
              }
            }
          }

          if(!printed)
          {
            encode(*src_title == '\0' ? "---" : src_title, output, chunk);
          }

          if(prn_country_)
          {
            std::ostringstream ostr;
            
            ostr << " (";
            
            El::Loc::Localizer::instance().country(current_message_->country,
                                                   subscription_.lang,
                                                   ostr);

            ostr << ")";

            encode(ostr.str().c_str(), output, chunk);
          }

          output << std::endl;          
          return true;
        }
      }
      else if(*var_name == '\0')
      {
        return encode("<?", output, chunk);        
      }
      
      return false;
    }

    bool
    RuntimeVariables::write_message(std::ostream& output) const
      throw(El::Exception)
    {
      for(Message::Transport::StoredMessageArray::const_iterator
            i(messages_.begin()), e(messages_.end()); i != e; ++i)
      {
        current_message_ = &i->message;
        message_template_->instantiate(*this, output);
      }
      
      return true;
    }
    
    //
    // TemplateCache::TemplateParseInterceptor class
    //
    void
    TemplateCache::TemplateParseInterceptor::update(
      const std::string& tag,
      const std::string& value,
      El::String::Template::Chunk& chunk) const
      throw(El::String::Template::ParsingFailed,
            El::String::Template::Exception,
            El::Exception)
    {
      if(tag == "endpoint" || tag == "subs_lang" || tag == "subs_id" ||
         tag == "subs_query" || tag == "subs_modifier" ||
         tag == "subs_filter" || tag == "subs_email" || tag == "messages" ||
         tag == "msg_title" || tag == "msg_encoded_id" || tag == "msg_url" ||
         tag == "msg_src_title_line")
      {
        chunk.text = tag;
      }
      else
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchMailing::TemplateCache::"
          "TemplateParseInterceptor::update: not valid tag '" << chunk.text
             << "'";

        throw El::String::Template::ParsingFailed(ostr.str());
      }
    }
    
    //
    // TemplateCache class
    //
    void
    TemplateCache::run(const char* template_path,
                       const Subscription& subscription,
                       const char* url,
                       const Message::Transport::StoredMessageArray& messages,
                       bool prn_src,
                       bool prn_country,
                       std::ostream& ostr)
      throw(El::Cache::Exception, El::Exception)
    {
      El::Cache::LocalizationInstantiationInterceptor
        localization_interceptor(template_cache_.var_left_marker(),
                                 template_cache_.var_right_marker());

      El::Cache::TextTemplateFile_var localized_template =
        template_cache_.get(template_path,
                            subscription.lang,
                            &localization_interceptor);

      std::string path = std::string(template_path) + ".message";
        
      El::Cache::TextTemplateFile_var message_template =
        template_cache_.get(path.c_str(),
                            subscription.lang,
                            &localization_interceptor);

      RuntimeVariables vars(subscription,
                            url,
                            messages,
                            message_template.in(),
                            prn_src,
                            prn_country);
      
      localized_template->instantiate(vars, ostr);
    }
  }
}
