/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/tests/RSSFeed/Service/RSSFeedImpl.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <El/CORBA/Corba.hpp>

#include <string>
#include <sstream>
#include <fstream>

#include <ace/OS.h>

#include <El/String/ListParser.hpp>
#include <El/String/Manip.hpp>

#include <El/Moment.hpp>
#include <El/Net/HTTP/StatusCodes.hpp>

#include <tests/Commons/SourceText.hpp>

#include "RSSFeedImpl.hpp"
#include "RSSFeedMain.hpp"

namespace
{
  const char WHITESPACES[]=" \t\n\r";
  const char ASPECT[]  = "RSSFeedImpl";
}

namespace NewsGate
{
  namespace RSS
  {
    //
    // FeedImpl class
    //
    FeedImpl::FeedImpl(El::Service::GenericService* container,
                       const Config& config)
      throw(Exception, El::Exception)
        : container_(container),
          config_(config),
          id_(config.id().c_str()),
          msg_number_(1),
          update_number_(0)
    {
      std::fstream file(config_.word_file().c_str(), std::ios::in);

      if(!file.is_open())
      {
        std::ostringstream ostr;
        ostr << "NewsGate::RSS::FeedImpl::FeedImpl: failed to open file "
             << config_.word_file();

        throw Exception(ostr.str());
      }

      source_text_ = new Test::SourceText(file);
      generate_channel_info();
    }    
    
    FeedImpl::FeedImpl(const FeedImpl& feed) throw(El::Exception)
        : container_(feed.container_),
          config_(feed.config_),
          source_text_(feed.source_text_),
          id_(feed.config_.id().c_str()),
          msg_number_(1)
    {
      generate_channel_info();
    }
    
    void
    FeedImpl::get(const ::NewsGate::RSS::FeedService::Request& req,
                  ::NewsGate::RSS::FeedService::Response* res)
      throw(Exception, El::Exception)
    {

      if(config_.http_response().etag() && *req.if_none_match.in() != '\0')
      {
        const char* item = 0;
        El::String::ListParser parser(req.if_none_match.in(), ", \t\n");

        ReadGuard_ guard(lock_i());

        std::ostringstream ostr;
        ostr << creation_time_.sec() << ":" << creation_time_.usec() << ":"
             << update_number_;

        std::string etag = ostr.str();

        while((item = parser.next_item()) != 0)
        {
          if(etag == item)
          {
            res->etag = etag.c_str();
            res->status_code = El::Net::HTTP::SC_NOT_MODIFIED;
            return;
          }
        }
      }

      if(config_.http_response().last_modified() &&
         *req.if_modified_since.in() != '\0')
      {
        ReadGuard_ guard(lock_i());

        if(last_build_date_ == req.if_modified_since.in())
        {
          res->status_code = El::Net::HTTP::SC_NOT_MODIFIED;
          return;       
        }
      }
      
      static const char prologue[] =
        "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n"
        "<rss version=\"2.0\">\n<channel>\n";
      
      static const char epilogue[] =
        "</channel>\n</rss>\n";

      std::ostringstream ostr;
      ostr << prologue;

      ReadGuard_ guard(lock_i());
      
      if(!title_.empty())
      {
        ostr << "<title><![CDATA[" << title_ << "]]></title>\n";
      }

      if(!description_.empty())
      {
        ostr << "<description><![CDATA[" << description_ <<
          "]]></description>\n";
      }

      if(!last_build_date_.empty())
      {
        ostr << "<lastBuildDate>" << last_build_date_ << "</lastBuildDate>\n";
      }

      if(config_.channel().ttl())
      {
        ostr << "<ttl>" << config_.channel().ttl() << "</ttl>\n";
      }

      if(config_.channel().lang().present())
      {
        ostr << "<language>" << config_.channel().lang()->c_str()
             << "</language>\n";
      }

      for(MessageArray::const_iterator it = messages_.begin();
          it != messages_.end(); it++)
      {
        const Message& msg = *it;
        
        ostr << "<item>\n";

        if(!msg.title.empty())
        {
          ostr << "<title><![CDATA[" << msg.title << "]]></title>\n";
        }

        if(!msg.description.empty())
        {
          char buff[3];
          buff[0] = 0xC2;
          buff[1] = 0xA0;
          buff[2] = 0x00;
          
          ostr << "<description>" << msg.description << " -" << buff
               << "- </description>\n";
        }
        
        if(!msg.category.empty())
        {
          ostr << "<category><![CDATA[" << msg.category << "]]></category>\n";
        }
        
        if(!msg.pub_date.empty())
        {
          ostr << "<pubDate>" << msg.pub_date << "</pubDate>\n";
        }

        if(!msg.link.empty())
        {
          ostr << "<link>" << msg.link << "</link>\n";
        }

        if(!msg.guid.empty())
        {
          ostr << "<guid isPermaLink=\"false\">" << msg.guid << "</guid>\n";
        }

        ostr << "</item>\n";
      }

      ostr << epilogue;

      res->body = CORBA::string_dup(ostr.str().c_str());
      res->content_length = config_.http_response().content_length();
      res->chunked = config_.http_response().chunked();

      if(config_.http_response().etag())
      {
        std::ostringstream ostr;
        ostr << creation_time_.sec() << ":" << creation_time_.usec() << ":"
             << update_number_;
        
        res->etag = ostr.str().c_str();
      }

      if(config_.http_response().last_modified())
      {
        res->last_modified = last_build_date_.c_str();
      }

      res->status_code = El::Net::HTTP::SC_OK;
    }

    FeedImpl*
    FeedImpl::clone() const throw(El::Exception)
    {
      FeedImpl_var feed = new FeedImpl(*this);

      return feed.retn();
    }
    
    void
    FeedImpl::generate_channel_info() throw(Exception, El::Exception)
    {
      ACE_Time_Value cur_time = ACE_OS::gettimeofday();

      WriteGuard_ guard(lock_i());

      creation_time_ = cur_time;
      
      if(config_.channel().title_len())
      {
        title_ =
          source_text_->get_random_substr(config_.channel().title_len());
      }
      
      if(config_.channel().description_len())
      {
        description_ =
          source_text_->get_random_substr(config_.channel().description_len());
      }

      set_last_build_date(cur_time);

      messages_.clear();
      unsigned long message_count = 0;
      
      ACE_Time_Value time = cur_time - ACE_Time_Value(config_.time_frame());

      for(; time < cur_time; message_count++)
      {
        Message msg;
        fill_message(msg, time);
        
        switch(config_.msg_sort())
        {
        case Server::TestRSSFeed::Config::SortMode::random:
          {
            unsigned long index = (unsigned long long)rand() * message_count /
              ((unsigned long long)RAND_MAX + 1);

            MessageArray::iterator it = messages_.begin();
            for(unsigned long i = 0; i < index; i++, it++);

            messages_.insert(it, msg);
            break;
          }
        case Server::TestRSSFeed::Config::SortMode::descending:
          {
            messages_.insert(messages_.begin(), msg);
            break;
          }
        case Server::TestRSSFeed::Config::SortMode::ascending:
          {
            messages_.push_back(msg);
            break;
          }
        }

        unsigned long diapason =
          config_.max_msg_freq() > config_.min_msg_freq() ?
          config_.max_msg_freq() - config_.min_msg_freq() : 0;
          
        unsigned long time_shift = (unsigned long long)rand() * diapason /
        ((unsigned long long)RAND_MAX + 1);
        
        time += ACE_Time_Value(config_.min_msg_freq() + time_shift);
      }

      El::Service::CompoundServiceMessage_var msg =
        new AddMessage(this, container_);
      
      container_->deliver_at_time(msg.in(), time);
    }

    void
    FeedImpl::set_last_build_date(const ACE_Time_Value& cur_time)
      throw(Exception, El::Exception)
    {
      std::string last_build_date = config_.channel().last_build_date();

      if(!last_build_date.empty())
      {
        long time_shift = 0;

        if(last_build_date != "GMT")
        {
          time_shift = last_build_date[0] == '+' ? 60 : -60;

          char hours_s[3];
          strncpy(hours_s, last_build_date.c_str() + 1, 2);
        
          time_shift *= atol(hours_s) * 60 + atol(last_build_date.c_str() + 3);
        }

        if(time_shift)
        {
          El::Moment tm(ACE_Time_Value(cur_time.sec() + time_shift));
          last_build_date_ = tm.rfc0822(false) + " " + last_build_date;
        }
        else
        {
          last_build_date_ = El::Moment(cur_time).rfc0822();
        }
      }

      update_number_++;
    }

    void
    FeedImpl::fill_message_pub_date(Message& msg, const ACE_Time_Value& time)
      throw(Exception, El::Exception)
    {
      long time_shift = 0;
      std::string pub_date = config_.item().pub_date();
      
      if(pub_date != "GMT")
      {
        time_shift = pub_date[0] == '+' ? 60 : -60;

        char hours_s[3];
        strncpy(hours_s, pub_date.c_str() + 1, 2);
        
        time_shift *= atol(hours_s) * 60 + atol(pub_date.c_str() + 3);
      }
      
      if(!pub_date.empty())
      {
        if(time_shift)
        {
          El::Moment tm(ACE_Time_Value(time.sec() + time_shift));
          msg.pub_date = tm.rfc0822(false) + " " + pub_date;
        }
        else
        {
          msg.pub_date = El::Moment(time).rfc0822();
        }
      }

      msg.pub_moment = time;
    }
    
    void
    FeedImpl::fill_message(Message& msg, const ACE_Time_Value& time)
      throw(Exception, El::Exception)
    {
      msg = Message();

      fill_message_pub_date(msg, time);
        
      std::string link_prefix = config_.item().link_prefix();

      if(!link_prefix.empty())
      {
        std::ostringstream ostr;
        ostr << link_prefix << "?msg=" << creation_time_.sec() << ":"
             << creation_time_.usec() << ":" << msg_number_;
          
        msg.link = ostr.str();
      }
          
      std::string guid_prefix = config_.item().guid_prefix();

      if(!guid_prefix.empty())
      {
        std::ostringstream ostr;
        ostr << guid_prefix << "?msg=" << creation_time_.sec() << ":"
             << creation_time_.usec() << ":" << msg_number_;
          
        msg.guid = ostr.str();
      }
          
      if(config_.item().title_len())
      {
        msg.title =
          source_text_->get_random_substr(config_.item().title_len());
      }

      if(config_.item().category_len())
      {
        msg.category =
          source_text_->get_random_substr(config_.item().category_len());
      }

      if(config_.item().description_len())
      {
        std::wstring wstr;
        
        El::String::Manip::utf8_to_wchar(
          source_text_->get_random_substr(
            config_.item().description_len()).c_str(), wstr);        

        std::string text;
        El::String::Manip::xml_encode(wstr.c_str(),
                                      text,
                                      El::String::Manip::XE_TEXT_ENCODING);

        if(config_.item().description_format() ==
           Server::TestRSSFeed::Config::DescriptionFormats::html)
        {
          msg.description = std::string("<p>") + text + "</p>";
        }
        else
        {
          msg.description = text;  
        } 
      }

      msg_number_++;
    }
    
    void
    FeedImpl::add_message() throw()
    {
      try
      {
        ACE_Time_Value cur_time = ACE_OS::gettimeofday();
        ACE_Time_Value time = cur_time - ACE_Time_Value(config_.time_frame());

        if(Application::will_trace(El::Logging::HIGH))
        {
          std::ostringstream ostr;
          ostr << "FeedImpl::add_message: id=" << id_ << std::endl;
            
          Application::logger()->trace(ostr.str(),
                                       ASPECT,
                                       El::Logging::HIGH);
        }

        unsigned long prc =
          (unsigned long long)rand() * 100 /
          ((unsigned long long)RAND_MAX + 1);
        
        bool perform_retiming = config_.msg_retiming() > prc;
        bool message_filled = false;
/*
        std::cerr << "config_.msg_retiming()/prc/perform_retiming: "
                  << (unsigned long)config_.msg_retiming() << "/" << prc << "/"
                  << perform_retiming << std::endl;
*/        
        {
          WriteGuard_ guard(lock_i());
        
          set_last_build_date(cur_time);

          unsigned long message_count = 0;
        
          Message msg;

          for(MessageArray::iterator it = messages_.begin();
              it != messages_.end(); )
          {
            if(time > it->pub_moment)
            {
              if(perform_retiming && !message_filled)
              {
                msg = *it;
                fill_message_pub_date(msg, cur_time);
                
                message_filled = true;
              }
              
              messages_.erase(it);
              it = messages_.begin();
              message_count = 0;
            }
            else
            {
              it++;
              message_count++;
            }
          }

          if(!message_filled)
          {
            fill_message(msg, cur_time);
          }
        
          switch(config_.msg_sort())
          {
          case Server::TestRSSFeed::Config::SortMode::random:
            {
              unsigned long index =
                (unsigned long long)rand() * message_count /
                ((unsigned long long)RAND_MAX + 1);

              MessageArray::iterator it = messages_.begin();
              for(unsigned long i = 0; i < index; i++, it++);

              messages_.insert(it, msg);
              break;
            }
          case Server::TestRSSFeed::Config::SortMode::descending:
            {
              messages_.insert(messages_.begin(), msg);
              break;
            }
          case Server::TestRSSFeed::Config::SortMode::ascending:
            {
              messages_.push_back(msg);
              break;
            }
          }
        }
      
        unsigned long diapason =
          config_.max_msg_freq() > config_.min_msg_freq() ?
          config_.max_msg_freq() - config_.min_msg_freq() : 0;
          
        unsigned long time_shift = (unsigned long long)rand() * diapason /
          ((unsigned long long)RAND_MAX + 1);
        
        El::Service::CompoundServiceMessage_var msg =
          new AddMessage(this, container_);
        
        container_->deliver_at_time(
          msg.in(),
          cur_time + ACE_Time_Value(config_.min_msg_freq() + time_shift));
        
      }
      catch(const El::Exception& e)
      {
        try
        {
          std::ostringstream ostr;
          ostr << "FeedImpl::add_message: "
            "El::Exception caught. Description:" << std::endl << e;

          El::Service::Error error(ostr.str(), 0);

          container_->notify(&error);
        }
        catch(...)
        {
          El::Service::Error error(
            "FeedImpl::add_message: unexpected exception caught", 0);
          
          container_->notify(&error);
        }
      }
      
    }
    
  }  
}
