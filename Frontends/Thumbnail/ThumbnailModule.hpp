/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Frontends/Thumbnail/ThumbnailModule.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_FRONTENDS_THUMBNAIL_THUMBNAILMODULE_HPP_
#define _NEWSGATE_SERVER_FRONTENDS_THUMBNAIL_THUMBNAILMODULE_HPP_

#include <string>
#include <memory>

#include <Python.h>

#include <El/Exception.hpp>
#include <El/Cache/BinaryFileCache.hpp>
#include <El/Apache/Module.hpp>
#include <El/Service/Service.hpp>

#include <El/Directive.hpp>
#include <El/Logging/LoggerBase.hpp>
#include <El/Net/HTTP/MimeTypeMap.hpp>

#include <Services/Commons/Message/BankClientSessionImpl.hpp>

namespace NewsGate
{
  struct ThumbnailModuleConfig
  {
    static El::Apache::ModuleRef mod_ref;    
    
    EL_EXCEPTION(Exception, El::ExceptionBase);
    
    ThumbnailModuleConfig() throw(El::Exception);
    
    // Config merging constructor
    ThumbnailModuleConfig(const ThumbnailModuleConfig& cf_base,
                          const ThumbnailModuleConfig& cf_new)
      throw(El::Exception);

    std::string message_bank_manager_ref;
    std::string mirrored_message_bank_manager_ref;
    unsigned long message_bank_clients;
    unsigned long message_bank_clients_max_threads;
    unsigned long mirrored_message_bank_clients_max_threads;
    std::string thumb_path;
    std::string mime_types_path;
    unsigned long cache_timeout;
    unsigned long message_max_age;
    unsigned long proxy_request_timeout;
  };
    
  class ThumbnailModule : public El::Apache::Module<ThumbnailModuleConfig>,
                          public El::Service::Callback
  {
  public:
    EL_EXCEPTION(Exception, El::ExceptionBase);
      
  public:
    ThumbnailModule(El::Apache::ModuleRef& mod_ref) throw(El::Exception);
  
  private:
    virtual bool notify(El::Service::Event* event) throw(El::Exception);

    virtual void directive(const El::Directive& directive,
                           Config& config)
      throw(El::Exception);

    virtual int handler(Context& context) throw(El::Exception);

    virtual void child_init(Config& conf) throw(El::Exception);      
    virtual void child_init() throw(El::Exception);      
    virtual void child_cleanup() throw(El::Exception);

    int get_message_image(Context& context,
                          const ThumbnailModuleConfig& config,
                          const char* content_type,
                          const Message::Id& id,
                          uint32_t img_index,
                          uint32_t thm_index,
                          uint32_t hash,
                          const char* hash_str,
                          Message::Signature signature,
                          const std::string& date)
      throw(Exception, El::Exception);

    bool query_remote_colo(Message::BankClientSessionImpl* session,
                           Message::Transport::ColoFrontend::Relation relation,
                           Context& context,
                           const ThumbnailModuleConfig& config,
                           const char* content_type,
                           const char* hash_str)
      throw(Exception, El::Exception);

    void write_headers(const ThumbnailModuleConfig& config,
                       const char* thumb_hash,
                       El::Apache::Request::Out& output) const
      throw(El::Exception);

    void send_file(const char* file_name,
                   const char* content_type,
                   const char* thumb_hash,
                   Context& context,
                   const ThumbnailModuleConfig& config)
      throw(El::Cache::NotFound, El::Exception);

    void send_data(const unsigned char* data,
                   size_t data_len,
                   const char* content_type,
                   const char* thumb_hash,
                   Context& context,
                   const ThumbnailModuleConfig& config)
      throw(El::Exception);
    
    Message::BankClientSessionImpl* bank_client_session()
      throw(Exception, El::Exception);

  public:
    virtual ~ThumbnailModule() throw();

  private:

    typedef ACE_RW_Thread_Mutex   Mutex;
    typedef ACE_Read_Guard<Mutex> ReadGuard;
    typedef ACE_Write_Guard<Mutex> WriteGuard;

    Mutex lock_;

    std::auto_ptr<El::Logging::LoggerBase> logger_;
    std::auto_ptr<El::Net::HTTP::MimeTypeMap> mime_types_;
    std::auto_ptr<El::Cache::BinaryFileCache> thumbs_;

    typedef El::Corba::SmartRef<Message::BankManager> BankManagerRef;
    
    BankManagerRef message_bank_manager_;
    BankManagerRef mirrored_message_bank_manager_;

    CORBA::ORB_var orb_;
    
    Message::BankClientSessionImpl_var bank_client_session_;
    unsigned long message_bank_clients_;
    unsigned long message_bank_clients_max_threads_;
    unsigned long cache_clear_time_;
  };
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  //
  // ThumbnailModuleConfig class
  //
  inline
  ThumbnailModuleConfig::ThumbnailModuleConfig() throw(El::Exception)
      : message_bank_clients(0),
        message_bank_clients_max_threads(0),
        cache_timeout(ULONG_MAX),
        message_max_age(ULONG_MAX),
        proxy_request_timeout(0)
  {
  }
  
  inline
  ThumbnailModuleConfig::ThumbnailModuleConfig(
    const ThumbnailModuleConfig& cf_base,
    const ThumbnailModuleConfig& cf_new) throw(El::Exception)
  {
    message_bank_clients = cf_new.message_bank_clients ?
      cf_new.message_bank_clients : cf_base.message_bank_clients;
      
    message_bank_clients_max_threads =
      cf_new.message_bank_clients_max_threads ?
      cf_new.message_bank_clients_max_threads :
      cf_base.message_bank_clients_max_threads;
      
    message_bank_manager_ref = cf_new.message_bank_manager_ref.empty() ?
      cf_base.message_bank_manager_ref : cf_new.message_bank_manager_ref;

    thumb_path = cf_new.thumb_path.empty() ?
      cf_base.thumb_path : cf_new.thumb_path;
    
    mime_types_path = cf_new.mime_types_path.empty() ?
      cf_base.mime_types_path : cf_new.mime_types_path;

    cache_timeout = cf_new.cache_timeout == ULONG_MAX ?
      cf_base.cache_timeout : cf_new.cache_timeout;

    message_max_age = cf_new.message_max_age == ULONG_MAX ?
      cf_base.message_max_age : cf_new.message_max_age;

    proxy_request_timeout = cf_new.proxy_request_timeout ?
      cf_new.proxy_request_timeout : cf_base.proxy_request_timeout;
  }

  //
  // Module class
  //
  inline
  ThumbnailModule::~ThumbnailModule() throw()
  {
  }

  inline
  void
  ThumbnailModule::send_data(const unsigned char* data,
                             size_t data_len,
                             const char* content_type,
                             const char* thumb_hash,
                             Context& context,
                             const ThumbnailModuleConfig& config)
    throw(El::Exception)
  {
    El::Apache::Request::Out& output = context.request.out();
    output.content_type(content_type);
    write_headers(config, thumb_hash, output);
    output.stream().write((const char*)data, data_len);
  }

  inline
  void
  ThumbnailModule::send_file(const char* file_name,
                             const char* content_type,
                             const char* thumb_hash,
                             Context& context,
                             const ThumbnailModuleConfig& config)
    throw(El::Cache::NotFound, El::Exception)
  {
    El::Cache::BinaryFile_var file = thumbs_->get(file_name);
    
    send_data(file->buff(),
              file->size(),
              content_type,
              thumb_hash,
              context,
              config);
  }
}
  
#endif // _NEWSGATE_SERVER_FRONTENDS_THUMBNAIL_THUMBNAILMODULE_HPP_
