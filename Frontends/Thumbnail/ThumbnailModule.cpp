/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Frontends/Thumbnail/ThumbnailModule.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <Python.h>
#include <El/CORBA/Corba.hpp>

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>

#include <ace/OS.h>

#include <httpd/httpd.h>

#include <El/Exception.hpp>
#include <El/Logging/StreamLogger.hpp>
#include <El/Cache/ObjectCache.hpp>
#include <El/String/Manip.hpp>
#include <El/Guid.hpp>

#include <El/Net/HTTP/Session.hpp>
#include <El/Net/HTTP/StatusCodes.hpp>

#include <Commons/Message/StoredMessage.hpp>
#include <Commons/Message/TransportImpl.hpp>

#include <Services/Commons/Message/MessageServices.hpp>
#include <Services/Commons/Message/BankClientSessionImpl.hpp>

#include "ThumbnailModule.hpp"

EL_APACHE_MODULE_INSTANCE(NewsGate::ThumbnailModule, thumbnail_module);

namespace Aspect
{
  const char MODULE[] = "ThumbnailModule";
}

namespace NewsGate
{
  const ACE_Time_Value FILE_REVIEW_PERIOD(3);
  
  const char HD_PROXY[] = "NG-Proxy";
  const char HD_PROXY_VAL[] = "yes";
  
  //
  // NewsGate::ThumbnailModule class
  //
  ThumbnailModule::ThumbnailModule(El::Apache::ModuleRef& mod_ref)
    throw(El::Exception)
      : El::Apache::Module<ThumbnailModuleConfig>(mod_ref,
                                                  false,
                                                  APR_HOOK_FIRST,
                                                  "Thumbnail_Path"),
        message_bank_clients_(0),
        message_bank_clients_max_threads_(0),
        cache_clear_time_(0)
  {
    register_directive("Thumbnail_MessageBankManagerRef",
                       "string:nws",
                       "Thumbnail_MessageBankManagerRef <corba ref>.",
                       1,
                       1);

    register_directive("Thumbnail_MessageBankClients",
                       "numeric:1",
                       "Thumbnail_MessageBankClients <virtual clients number>.",
                       1,
                       1);

    register_directive("Thumbnail_MessageBankClientSessionMaxThreads",
                       "numeric:1",
                       "Thumbnail_MessageBankClientSessionMaxThreads <threads>.",
                       1,
                       1);    

    register_directive("Thumbnail_ThumbPath",
                       "string:nws",
                       "Thumbnail_ThumbPath <path>.",
                       1,
                       1);

    register_directive("Thumbnail_MimeTypesPath",
                       "string:nws",
                       "Thumbnail_MimeTypesPath <path>.",
                       1,
                       1);

    register_directive("Thumbnail_CacheTimeout",
                       "numeric:0",
                       "Thumbnail_CacheTimeout <seconds>.",
                       1,
                       1);
    
    register_directive("Thumbnail_MessageMaxAge",
                       "numeric:0",
                       "Thumbnail_MessageMaxAge <seconds>.",
                       1,
                       1);
    
    register_directive("Thumbnail_ProxyRequestTimeout",
                       "numeric:1",
                       "Thumbnail_ProxyRequestTimeout <seconds>.",
                       1,
                       1);
  }
 
  void
  ThumbnailModule::directive(const El::Directive& directive, Config& config)
    throw(El::Exception)
  {
    const El::Directive::Arg& arg = directive.arguments[0];
//    const El::Directive::ArgArray& args = directive.arguments;
    
    const std::string& dname = directive.name;
    
    if(dname == "Thumbnail_MessageBankManagerRef")
    {
      config.message_bank_manager_ref = arg.string();
    }
    else if(dname == "Thumbnail_MessageBankClients")
    {
      config.message_bank_clients = arg.numeric();
    }
    else if(dname == "Thumbnail_MessageBankClientSessionMaxThreads")
    {
      config.message_bank_clients_max_threads = arg.numeric();
    }    
    else if(dname == "Thumbnail_ThumbPath")
    {
      config.thumb_path = arg.string();
    }
    else if(dname == "Thumbnail_MimeTypesPath")
    {
      config.mime_types_path = arg.string();
    }
    else if(dname == "Thumbnail_CacheTimeout")
    {
      config.cache_timeout = arg.numeric();
    }
    else if(dname == "Thumbnail_MessageMaxAge")
    {
      config.message_max_age = arg.numeric();
    }
    else if(dname == "Thumbnail_ProxyRequestTimeout")
    {
      config.proxy_request_timeout = arg.numeric();
    }
  }

  void
  ThumbnailModule::child_init() throw(El::Exception)
  {
    logger_->info(
      "NewsGate::ThumbnailModule::child_init: module initialized ...",
      Aspect::MODULE);
  }
    
  void
  ThumbnailModule::child_init(Config& conf) throw(El::Exception)
  {
    if(logger_.get() == 0)
    {
      logger_.reset(
        new El::Logging::StreamLogger(std::cerr, ULONG_MAX));  
    }

    mime_types_.reset(
      new El::Net::HTTP::MimeTypeMap(conf.mime_types_path.c_str()));
    
    thumbs_.reset(
      new El::Cache::BinaryFileCache(FILE_REVIEW_PERIOD,
                                     ACE_Time_Value(conf.cache_timeout)));
    
    message_bank_clients_ = conf.message_bank_clients;
    message_bank_clients_max_threads_ = conf.message_bank_clients_max_threads;
    
    try
    {
/*      
      char* argv[] =
        {
          "--corba-thread-pool",
          "50"
        };
*/      
      /*
      char* argv[] =
        {
          "--corba-reactive"
        };
      */

      char* argv[] =
        {
        };

      orb_ = ::CORBA::ORB::_duplicate(
        El::Corba::Adapter::orb_adapter(
          sizeof(argv) / sizeof(argv[0]), argv)->orb());
      
      Message::Transport::register_valuetype_factories(orb_.in());
      Search::Transport::register_valuetype_factories(orb_.in());

      Message::BankClientSessionImpl::register_valuetype_factories(
        orb_.in());

      message_bank_manager_ =
        BankManagerRef(conf.message_bank_manager_ref.c_str(), orb_.in());
    }
    catch(const CORBA::Exception& e)
    {
      std::ostringstream ostr;
      ostr << "NewsGate::ThumbnailModule::child_init: "
        "CORBA::Exception caught. Description:\n" << e;

      throw Exception(ostr.str());
    }
  } 

  void
  ThumbnailModule::child_cleanup() throw(El::Exception)
  {
    try
    {
      Message::BankClientSessionImpl_var bank_client_session;

      {
        WriteGuard guard(lock_);
        bank_client_session = bank_client_session_._retn();
      }

      bank_client_session = 0;
      El::Corba::Adapter::orb_adapter_cleanup();
    }
    catch(const El::Exception& e)
    {
      std::ostringstream ostr;
      ostr << "NewsGate::ThumbnailModule::child_cleanup: "
        "El::Exception caught. Description:\n" << e;

      logger_->critical(ostr.str(), Aspect::MODULE);
    }
    catch(const CORBA::Exception& e)
    {
      std::ostringstream ostr;
      ostr << "NewsGate::ThumbnailModule::child_cleanup: "
        "CORBA::Exception caught. Description:\n" << e;

      logger_->critical(ostr.str(), Aspect::MODULE);
    }
    
    logger_->info(
      "NewsGate::ThumbnailModule::child_cleanup: module terminated",
      Aspect::MODULE);
  }
    
  bool
  ThumbnailModule::notify(El::Service::Event* event) throw(El::Exception)
  {
    El::Service::log(event,
                     "NewsGate::ThumbnailModule::notify: ",
                     logger_.get(),
                     Aspect::MODULE);
    
    return true;
  }

  void
  ThumbnailModule::write_headers(const ThumbnailModuleConfig& config,
                                 const char* thumb_hash,
                                 El::Apache::Request::Out& output) const
    throw(El::Exception)
  {
    {
      std::ostringstream ostr;
      ostr << "max-age=" << config.message_max_age;
      
      output.send_header(El::Net::HTTP::HD_CACHE_CONTROL, ostr.str().c_str());
      output.send_header(El::Net::HTTP::HD_ETAG, thumb_hash);
    }
  }
    
  int
  ThumbnailModule::handler(Context& context) throw(El::Exception)
  {
    El::Apache::Request& request = context.request;

    const char* thumb = strrchr(request.uri(), '/');
    
    if(thumb++ == 0)
    {
      return HTTP_BAD_REQUEST;
    }

    const char* message_id = strchr(thumb, '-');

    if(message_id == 0)
    {
      return HTTP_BAD_REQUEST;
    }
    
    const char* image_index = strchr(++message_id, '-');
    
    if(image_index == 0)
    {
      return HTTP_BAD_REQUEST;
    }

    const char* thumb_index = strchr(++image_index, '-');
    
    if(thumb_index == 0)
    {
      return HTTP_BAD_REQUEST;
    }

    const char* hash = strchr(++thumb_index, '-');

    if(hash == 0)
    {
      return HTTP_BAD_REQUEST;
    }
    
    const char* message_sign = strchr(++hash, '-');

    if(message_sign)
    {
      ++message_sign;
    }

    const char* ext = strchr(message_sign ? message_sign : hash, '.');

    if(ext == 0)
    {
      return HTTP_BAD_REQUEST;
    }

    std::string msg_id(message_id, image_index - message_id - 1);
    std::string im_index(image_index, thumb_index - image_index - 1);
    std::string th_index(thumb_index, hash - thumb_index - 1);
    
    std::string th_hash(hash, message_sign ? (message_sign - hash - 1) :
                        (ext - hash));

    const ThumbnailModuleConfig* config = context.config();
    
    const char* current_etag =
      request.in().headers().find(El::Net::HTTP::HD_IF_NONE_MATCH);

    if(current_etag && current_etag == th_hash)
    {
      write_headers(*config, th_hash.c_str(), request.out());
      return HTTP_NOT_MODIFIED;
    }
    
    unsigned long img_index = 0;
    unsigned long thm_index = 0;
    uint32_t thm_hash = 0;

    if(!El::String::Manip::numeric(im_index.c_str(), img_index) ||
       !El::String::Manip::numeric(th_index.c_str(), thm_index) ||
       !El::String::Manip::numeric(th_hash.c_str(),
                                   thm_hash,
                                   El::String::Manip::NF_HEX))
    {
      return HTTP_BAD_REQUEST;
    }

    Message::Signature message_signature = 0;
    
    if(message_sign)
    {
      std::string msg_sign;
      msg_sign.assign(message_sign, ext - message_sign);

      if(!El::String::Manip::numeric(msg_sign.c_str(),
                                     message_signature,
                                     El::String::Manip::NF_HEX))
      {
        return HTTP_BAD_REQUEST;
      }
    }    

    Message::Id id;
    
    try
    {
      id = Message::Id(msg_id.c_str());
    }
    catch(const Message::InvalidArg&)
    {
      return HTTP_BAD_REQUEST;
    }

    El::Net::HTTP::MimeTypeMap::const_iterator mit = mime_types_->find(++ext);

    if(mit == mime_types_->end())
    {
      return HTTP_BAD_REQUEST;      
    }

    const std::string& content_type = mit->second;
    std::string date(thumb, message_id - thumb - 1);
    
    std::string file_name =
      config->thumb_path + "/" + date + "/" + message_id;

    int result = OK;

    try
    {
      send_file(file_name.c_str(),
                content_type.c_str(),
                th_hash.c_str(),
                context,
                *config);
    }
    catch(const El::Cache::NotFound&)
    {
      result = get_message_image(context,
                                 *config,
                                 content_type.c_str(),
                                 id,
                                 img_index,
                                 thm_index,
                                 thm_hash,
                                 th_hash.c_str(),
                                 message_signature,
                                 date);
    }

    unsigned long cur_time = request.time().sec();

    ReadGuard guard(lock_);
    
    if(cur_time - cache_clear_time_ > 86400)
    {
      guard.release();
      WriteGuard guard2(lock_);
      
      if(cur_time - cache_clear_time_ > 86400)
      {
        cache_clear_time_ = cur_time;
        guard2.release();
      
        Message::StoredMessage::cleanup_thumb_files(config->thumb_path.c_str(),
                                                    config->message_max_age);
      }
    }
    
    return result;
  }

  bool
  ThumbnailModule::query_remote_colo(
    Message::BankClientSessionImpl* session,
    Message::Transport::ColoFrontend::Relation relation,
    Context& context,
    const ThumbnailModuleConfig& config,
    const char* content_type,
    const char* thumb_hash)
    throw(Exception, El::Exception)
  {
    Message::Transport::ColoFrontendArray frontends;
    
    try
    {
      frontends = session->colo_frontends(relation);
    }
    catch(const CORBA::Exception& e)
    {
    }
    
    if(frontends.empty())
    {
      return false;
    }

    unsigned long index = (unsigned long long)rand() * frontends.size() /
      ((unsigned long long)RAND_MAX + 1);    
    
    const Message::Transport::EndpointArray&
      endpoints = frontends[index].limited_endpoints;

    if(endpoints.empty())
    {
      return false;
    }
    
    index = (unsigned long long)rand() * endpoints.size() /
      ((unsigned long long)RAND_MAX + 1);

    const std::string& endpoint = endpoints[index];
    
    std::string url = std::string("http://") + endpoint +
      context.request.unparsed_uri();

    const char* date = strrchr(context.request.uri(), '/');
    
    if(date++ == 0)
    {
      return false;
    }

    const char* date_end = strchr(date, '-');

    if(date_end == 0)
    {
      return false;
    }

    std::string thumb_date(date, date_end - date);
    
    std::string dir_path = std::string(config.thumb_path) + "/" +
      thumb_date;      

    std::string file_name = dir_path + std::string("/") + (date_end + 1);

    El::Guid guid;
    guid.generate();
    
    std::string temp_file_name = file_name + "." +
      guid.string(El::Guid::GF_DENSE) + ".tmp";
    
    try
    {
      El::Net::HTTP::HeaderList headers;
      headers.add(HD_PROXY, HD_PROXY_VAL);
      
      El::Net::HTTP::Session session(url.c_str());

      ACE_Time_Value timeout(config.proxy_request_timeout);      
      session.open(&timeout, &timeout, &timeout);
      
      session.send_request(El::Net::HTTP::GET,
                           El::Net::HTTP::ParamList(),
                           headers);
      
      session.recv_response_status();

      if(session.status_code() != El::Net::HTTP::SC_OK)
      {
        return false;
      }

      El::Net::HTTP::Header header;
      while(session.recv_response_header(header));
      
      mkdir(dir_path.c_str(), 0755);    
      session.save_body(temp_file_name.c_str());      
    }
    catch(const El::Net::Exception&)
    {
      unlink(temp_file_name.c_str());
      return false;
    }

    if(rename(temp_file_name.c_str(), file_name.c_str()) < 0)
    {
      int error = ACE_OS::last_error();

      std::ostringstream ostr;
                
      ostr <<"NewsGate::ThumbnailModule::query_remote_colo: "
        "rename '" << temp_file_name << "' to '" << file_name << "' failed. "
        "Errno " << error << ". Description:\n"
           << ACE_OS::strerror(error);

      throw Exception(ostr.str());
    }

    try
    {
      // To erase OBJECT UNEXISTENT cached state
      thumbs_->erase(file_name.c_str());
      
      send_file(file_name.c_str(),
                content_type,
                thumb_hash,
                context,
                config);
    }
    catch(const El::Cache::NotFound& e)
    {
      return false;
    }
    
    return true;
  }
  
  int
  ThumbnailModule::get_message_image(Context& context,
                                     const ThumbnailModuleConfig& config,
                                     const char* content_type,
                                     const Message::Id& id,
                                     uint32_t img_index,
                                     uint32_t thm_index,
                                     uint32_t hash,
                                     const char* hash_str,
                                     Message::Signature signature,
                                     const std::string& date)
    throw(Exception, El::Exception)
  {
    try
    {
      const char* proxy_val = context.request.in().headers().find(HD_PROXY);
      bool proxied_req = proxy_val != 0 && !strcmp(proxy_val, HD_PROXY_VAL);
        
      NewsGate::Message::Transport::IdImpl::Var msg_id =
        new NewsGate::Message::Transport::IdImpl::Type(new Message::Id(id));

      Message::BankClientSessionImpl_var session = bank_client_session();
      Message::Transport::StoredMessage_var result;

      try
      {
        result = session->get_message(msg_id.in(),
                                      Message::Bank::GM_IMG |
                                      Message::Bank::GM_IMG_THUMB,
                                      img_index,
                                      thm_index,
                                      signature);
      }
      catch(const CORBA::Exception& e)
      {
        if(!proxied_req && (query_remote_colo(
                              session.in(),
                              Message::Transport::ColoFrontend::RL_MASTER,
                              context,
                              config,
                              content_type,
                              hash_str) ||
                            query_remote_colo(
                              session.in(),
                              Message::Transport::ColoFrontend::RL_MIRROR,
                              context,
                              config,
                              content_type,
                              hash_str)))
        {
          return OK;
        }
        
        throw;
      }
      
      NewsGate::Message::Transport::StoredMessageImpl::Type* msg =
        dynamic_cast<NewsGate::Message::Transport::StoredMessageImpl::Type*>(
          result.in());
      
      if(msg == 0)
      {
        throw Exception(
          "NewsGate::ThumbnailModule::get_message_image: "
          "dynamic_cast<NewsGate::Message::Transport::StoredMessageImpl::"
          "Type*> failed");
      }

      const Message::StoredMessage& message = msg->entity().message;
      
/*
      Redundant check. Can lead to message image not showing under
      some tricky conditions: it can happen that system have temporary
      2 messages with same ID but different date on different boxes.

      std::string msg_date = El::Moment(ACE_Time_Value(message.updated)).
        dense_format(El::Moment::DF_DATE);

      if(msg_date != date)
      {
        return HTTP_BAD_REQUEST;
      }
*/

      if((message.flags & Message::StoredMessage::MF_HAS_THUMBS) == 0)
      {
        return HTTP_NOT_FOUND;
      }
      
      Message::StoredImageArray* images = message.content->images.get();
      assert(images != 0);
      
      for(size_t i = 0; i < images->size(); i++)
      {
        const Message::ImageThumbArray& thumbs = (*images)[i].thumbs;

        for(size_t j = 0; j < thumbs.size(); j++)
        {
          const Message::ImageThumb& thumb = thumbs[j];

          if(thumb.empty())
          {
            continue;
          }

          std::ostringstream ostr;
          
          ostr << message.image_thumb_name(config.thumb_path.c_str(),
                                           i,
                                           j,
                                           true)
               << "-" << std::uppercase << std::hex << thumb.hash;

          if(signature)
          {
            ostr << "-" << std::uppercase << std::hex << signature;
          }
          
          ostr << "." << thumb.type;

          std::string name = ostr.str();
          std::fstream file(name.c_str(), std::ios::out);

          if(!file.is_open())
          {
            std::ostringstream ostr;
            ostr << "NewsGate::ThumbnailModule::get_message_image: "
              "failed to open '" << name << "' for write access";
            
            throw Exception(ostr.str());
          }
          
          file.write((const char*)thumb.image.get(), thumb.length);

          if(file.fail())
          {
            file.close();
            unlink(name.c_str());
          
            std::ostringstream ostr;
            ostr << "NewsGate::ThumbnailModule::get_message_image: "
              "failed to write to '" << name << "'";
            
            throw Exception(ostr.str());
          }
        }
      }
      
      if(images->size() <= img_index)
      {
        return HTTP_NOT_FOUND;
      }

      const Message::ImageThumbArray& thumbs = (*images)[img_index].thumbs;

      if(thumbs.size() <= thm_index)
      {
        return HTTP_NOT_FOUND;
      }
        
      const Message::ImageThumb& thumb = thumbs[thm_index];

      if(thumb.empty())
      {
        return HTTP_NOT_FOUND;
      }

      if(hash != thumb.hash)
      {
        return HTTP_BAD_REQUEST;
      }
      
      const char* type = thumb.type.c_str();
      
      El::Net::HTTP::MimeTypeMap::const_iterator mit =
        mime_types_->find(type);

      if(mit == mime_types_->end())
      {
        std::ostringstream ostr;
        ostr << "NewsGate::ThumbnailModule::get_message_image: "
          "no mime type defined for image type '" << type << "'";

        logger_->critical(ostr.str(), Aspect::MODULE);
        
        return HTTP_INTERNAL_SERVER_ERROR;
      }

      send_data(thumb.image.get(),
                thumb.length,
                mit->second.c_str(),
                hash_str,
                context,
                config);
    }
    catch(const NewsGate::Message::NotFound&)
    {
      return HTTP_NOT_FOUND;
    }
    catch(const NewsGate::Message::NotReady& e)
    {
      std::ostringstream ostr;
      ostr << "NewsGate::ThumbnailModule::get_message_image: "
        "NewsGate::Message::NotReady caught. Reason:\n" << e.reason;

      logger_->warning(ostr.str(), Aspect::MODULE);
      
      return HTTP_SERVICE_UNAVAILABLE;
    }
    catch(const NewsGate::Message::ImplementationException& e)
    {
      std::ostringstream ostr;
      ostr << "NewsGate::ThumbnailModule::get_message_image: "
        "NewsGate::Message::ImplementationException caught. "
        "Description:\n" << e.description;

      throw Exception(ostr.str());
    }
    catch(const CORBA::Exception& e)
    {
      std::ostringstream ostr;
      ostr << "NewsGate::ThumbnailModule::get_message_image: "
        "CORBA::Exception caught. Description:\n" << e;

      throw Exception(ostr.str());
    }

    return OK;
  }
  
  Message::BankClientSessionImpl*
  ThumbnailModule::bank_client_session() throw(Exception, El::Exception)
  {
    Message::BankClientSessionImpl_var& bcs = bank_client_session_;
      
    {
      ReadGuard guard(lock_);

      if(bcs.in() != 0)
      {
        guard.release();
        
        bcs->_add_ref();
        return bcs.in();
      }
    }

    try
    {
      Message::BankManager_var message_bank_manager =
        message_bank_manager_.object();
        
      Message::BankClientSession_var bank_client_session =
        message_bank_manager->bank_client_session();
      
      if(bank_client_session.in() == 0)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::ThumbnailModule::bank_client_session: "
          "bank_client_session.in() == 0";
        
        throw Exception(ostr.str());
      }
      
      Message::BankClientSessionImpl* bank_client_session_impl =
        dynamic_cast<Message::BankClientSessionImpl*>(
          bank_client_session.in());
      
      if(bank_client_session_impl == 0)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::ThumbnailModule::bank_client_session: "
          "dynamic_cast<Message::BankClientSessionImpl*> failed";
        
        throw Exception(ostr.str());
      }      
    
      WriteGuard guard(lock_);
      
      if(bcs.in() != 0)
      {
        guard.release();
        
        bcs->_add_ref();
        return bcs.in();
      }
      
      bank_client_session_impl->_add_ref();
      bcs = bank_client_session_impl;
      
      unsigned long threads =
        std::min(bcs->threads() * message_bank_clients_,
                 message_bank_clients_max_threads_);

      bcs->init_threads(this, threads);
      bcs->_add_ref();
      
      return bcs.in();
    }
    catch(const Message::ImplementationException& e)
    {
      std::ostringstream ostr;
      ostr << "NewsGate::ThumbnailModule::bank_client_session: "
        "Message::ImplementationException caught. Description:\n"
           << e.description.in();

      throw Exception(ostr.str());
    }
    catch(const CORBA::Exception& e)
    {
      std::ostringstream ostr;
      ostr << "NewsGate::ThumbnailModule::bank_client_session): "
        "CORBA::Exception caught. Description:\n" << e;

      throw Exception(ostr.str());
    }
  }
}
