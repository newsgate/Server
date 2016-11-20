
/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Tools/ExportMessage/MessageExportApp.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <stdint.h>

#include <iostream>
#include <fstream>

#include <mysql/mysql.h>

#include <El/Exception.hpp>
#include <El/BinaryStream.hpp>
#include <El/String/Manip.hpp>

#include <Commons/Message/StoredMessage.hpp>

using namespace ::NewsGate;

namespace
{
  const char USAGE[] = "Usage: MessageExport --output <file> "
    "[--msg <msg per file>] --socket <socket> "
    "--user <user> [--password <password>] [--thumbs <thumb_dir>] "
    "[--feed <feed url>]\n";
}

EL_EXCEPTION(Exception, El::ExceptionBase);

int dump_messages(MYSQL* mysql,
                  const char* thumbnail_dir,
                  const char* filename,
                  size_t msg_per_file,
                  const char* feed)
  throw(El::Exception);

int
main(int argc, char** argv)
{
  srand(time(0));
  
  int r = -1;

  std::string filename;
  std::string db_socket;
  std::string db_user;
  std::string db_password;
  std::string thumbnail_dir;
  size_t msg_per_file = SIZE_MAX;
  std::string feed;

  for(int i = 1; i < argc - 1; ++i)
  {
    const char* arg = argv[i];
    
    if(strcmp(arg, "--socket") == 0)
    {
      db_socket = argv[++i];
    }
    else if(strcmp(arg, "--user") == 0)
    {
      db_user = argv[++i];
    }
    else if(strcmp(arg, "--password") == 0)
    {
      db_password = argv[++i];
    }
    else if(strcmp(arg, "--thumbs") == 0)
    {
      thumbnail_dir = argv[++i];
    }
    else if(strcmp(arg, "--output") == 0)
    {
      filename = argv[++i];
    }
    else if(strcmp(arg, "--feed") == 0)
    {
      feed = argv[++i];
    }
    else if(strcmp(arg, "--msg") == 0)
    {
      if(!El::String::Manip::numeric(argv[++i], msg_per_file) || !msg_per_file)
      {
        std::cerr << "MessageExport: invalid --msg value specified\n" << USAGE;
        return r;
      }
    }
    else
    {
      std::cerr << "MessageExport: unexpected argument " << arg
                << " specified\n" << USAGE;
      return r;
    }
  }
  
  if(filename.empty())
  {
    std::cerr << "MessageExport: output file name not specified\n" << USAGE;
    return r;
  }

  if(db_socket.empty())
  {
    std::cerr << "MessageExport: DB socket not specified\n" << USAGE;
    return r;
  }
  
  if(db_user.empty())
  {
    std::cerr << "MessageExport: DB user not specified\n" << USAGE;
    return r;
  }

  if(thumbnail_dir.empty())
  {
    std::cerr << "MessageExport warning: thumbnails directory not specified;"
      " messages will be exported with no thumbnails.\n";
  }

  
  if(mysql_thread_init())
  {
    std::cerr << "MessageExport: my_thread_init failed\n";
    return r;
  }

  MYSQL* mysql = mysql_init(NULL);

  if(mysql == 0)
  {
    std::cerr << "mysql_init failed\n";
    mysql_thread_end();
    return -1;
  }
  
  my_bool my_true = true;
  
  if(mysql_options(mysql, MYSQL_OPT_RECONNECT, &my_true))
  {
    std::cerr << "MessageExport: mysql_options(MYSQL_OPT_RECONNECT) failed. "
      "Error code " << std::dec << mysql_errno(mysql) << ", description:\n"
              << mysql_error(mysql) << std::endl;
  }
  else if(mysql_options(mysql, MYSQL_SET_CHARSET_NAME, "utf8"))
  {
    std::cerr << "MessageExport: mysql_options(MYSQL_SET_CHARSET_NAME) "
      "failed. Error code " << std::dec << mysql_errno(mysql)
              << ", description:\n" << mysql_error(mysql) << std::endl;
  }
  else if(mysql_real_connect(
            mysql,
            "",
            db_user.c_str(),
            db_password.c_str(),
            "NewsGate",
            0,
            db_socket.c_str(),
            0) == 0)
  {
    std::cerr << "MessageExport: mysql_real_connect failed. Error code "
              << std::dec << mysql_errno(mysql) << ", description:\n"
              << mysql_error(mysql) << std::endl;
  }
  else
  {
    try
    {
      r = dump_messages(mysql,
                        thumbnail_dir.c_str(),
                        filename.c_str(),
                        msg_per_file,
                        feed.c_str());
    }
    catch(const El::Exception& e)
    {
      std::cerr << "MessageExport: " << e << std::endl;
    }
  }
  
  mysql_close(mysql);
  mysql_thread_end();
  
  return r;
}

int
dump_messages(MYSQL* mysql,
              const char* thumbnail_dir,
              const char* filename,
              size_t msg_per_file,
              const char* feed) throw(El::Exception)
{
  unsigned long long prev_id = 0;
  size_t records = 0;
  size_t skipped = 0;
  size_t thumbnails = 0;
  size_t total_lost_thumbs = 0;
  size_t records_in_file = 0;
  size_t zombi_messages = 0;
   
  MYSQL_STMT* statement = mysql_stmt_init(mysql);

  if(statement == 0)
  {
    std::cerr << "MessageExport: mysql_stmt_init failed. Error code "
              << std::dec << mysql_errno(mysql) << ", description:\n"
              << mysql_error(mysql) << std::endl;
    return -1;
  }

  static const char query[] = "select Message.id, event_id, event_capacity, "
    "flags, signature, url_signature, source_id, MessageDict.hash, "
    "impressions, clicks, published, fetched, visited, space, complements, "
    "url, lang, country, source_title, source_html_link, broken_down, "
    "categorizer_hash, categories "
    "from Message left join MessageStat on Message.id=MessageStat.id "
    "left join MessageCat on Message.id=MessageCat.id "
    "left join MessageDict on Message.id=MessageDict.id "
    "where Message.id > ? ORDER BY Message.id LIMIT 10000";
  
  if(mysql_stmt_prepare(statement, query, strlen(query)))
  {
    std::cerr << "MessageExport: mysql_stmt_prepare failed. Error code "
              << std::dec << mysql_stmt_errno(statement) << ", description:\n"
              << mysql_stmt_error(statement) << std::endl;
    
    mysql_stmt_close(statement);
    return -1;
  }

  MYSQL_BIND param;
  memset(&param, 0, sizeof(param));
  
  param.buffer_type = MYSQL_TYPE_LONGLONG;
  param.buffer = &prev_id;
  param.is_unsigned = true;

  std::fstream output;
  El::BinaryOutStream bstr(output);
  
  if(mysql_stmt_bind_param(statement, &param))
  {      
    std::cerr << "MessageExport: mysql_stmt_bind_param failed. Error code "
              << std::dec << mysql_stmt_errno(statement) << ", description:\n"
              << mysql_stmt_error(statement) << std::endl;
      
    mysql_stmt_close(statement);
    return -1;
  }

  unsigned long long id = 0;
  unsigned long long event_id = 0;
  unsigned int event_capacity = 0;
  unsigned char flags = 0;
  unsigned long long signature = 0;
  unsigned long long url_signature = 0;
  unsigned long long source_id = 0;
  unsigned int dict_hash = 0;
  unsigned long long impressions = 0;
  unsigned long long clicks = 0;
  unsigned long long published = 0;
  unsigned long long fetched = 0;
  unsigned long long visited = 0;
  unsigned short space = 0;
  unsigned short lang = 0;
  unsigned short country = 0;
  unsigned int categorizer_hash;
  
  char url[2048 * 6 + 1];
  unsigned long url_len = 0;

  char source_title[1024 * 6 + 1];
  unsigned long source_title_len = 0;

  char source_html_link[2048 * 6 + 1];
  unsigned long source_html_link_len = 0;

  unsigned long complements_len = 0;
  unsigned char complements[65535];
    
  unsigned long broken_down_len = 0;
  unsigned char broken_down[65535];
    
  unsigned long categories_len = 0;
  unsigned char categories[65535];
    
  MYSQL_BIND results[23];
  memset(results, 0, sizeof(results));

  size_t i = 0;

  results[i].buffer_type = MYSQL_TYPE_LONGLONG;
  results[i].buffer = &id;
  results[i++].is_unsigned = true;

  results[i].buffer_type = MYSQL_TYPE_LONGLONG;
  results[i].buffer = &event_id;
  results[i++].is_unsigned = true;
  
  results[i].buffer_type = MYSQL_TYPE_LONG;
  results[i].buffer = &event_capacity;
  results[i++].is_unsigned = true;

  results[i].buffer_type = MYSQL_TYPE_TINY;
  results[i].buffer = &flags;
  results[i++].is_unsigned = true;

  results[i].buffer_type = MYSQL_TYPE_LONGLONG;
  results[i].buffer = &signature;
  results[i++].is_unsigned = true;

  results[i].buffer_type = MYSQL_TYPE_LONGLONG;
  results[i].buffer = &url_signature;
  results[i++].is_unsigned = true;

  results[i].buffer_type = MYSQL_TYPE_LONGLONG;
  results[i].buffer = &source_id;
  results[i++].is_unsigned = true;

  results[i].buffer_type = MYSQL_TYPE_LONG;
  results[i].buffer = &dict_hash;
  results[i++].is_unsigned = true;

  results[i].buffer_type = MYSQL_TYPE_LONGLONG;
  results[i].buffer = &impressions;
  results[i++].is_unsigned = true;

  results[i].buffer_type = MYSQL_TYPE_LONGLONG;
  results[i].buffer = &clicks;
  results[i++].is_unsigned = true;

  results[i].buffer_type = MYSQL_TYPE_LONGLONG;
  results[i].buffer = &published;
  results[i++].is_unsigned = true;

  results[i].buffer_type = MYSQL_TYPE_LONGLONG;
  results[i].buffer = &fetched;
  results[i++].is_unsigned = true;
  
  results[i].buffer_type = MYSQL_TYPE_LONGLONG;
  results[i].buffer = &visited;
  results[i++].is_unsigned = true;
  
  results[i].buffer_type = MYSQL_TYPE_SHORT;
  results[i].buffer = &space;
  results[i++].is_unsigned = true;

  results[i].buffer_type = MYSQL_TYPE_BLOB;
  results[i].buffer = complements;
  results[i].buffer_length = sizeof(complements);
  results[i++].length = &complements_len;

  results[i].buffer_type = MYSQL_TYPE_VAR_STRING;
  results[i].buffer = url;
  results[i].buffer_length = sizeof(url);
  results[i++].length = &url_len;

  results[i].buffer_type = MYSQL_TYPE_SHORT;
  results[i].buffer = &lang;
  results[i++].is_unsigned = true;

  results[i].buffer_type = MYSQL_TYPE_SHORT;
  results[i].buffer = &country;
  results[i++].is_unsigned = true;
  
  results[i].buffer_type = MYSQL_TYPE_VAR_STRING;
  results[i].buffer = source_title;
  results[i].buffer_length = sizeof(source_title);
  results[i++].length = &source_title_len;

  results[i].buffer_type = MYSQL_TYPE_VAR_STRING;
  results[i].buffer = source_html_link;
  results[i].buffer_length = sizeof(source_html_link);
  results[i++].length = &source_html_link_len;

  results[i].buffer_type = MYSQL_TYPE_BLOB;
  results[i].buffer = broken_down;
  results[i].buffer_length = sizeof(broken_down);
  results[i++].length = &broken_down_len;
  
  results[i].buffer_type = MYSQL_TYPE_LONG;
  results[i].buffer = &categorizer_hash;
  results[i++].is_unsigned = true;
    
  results[i].buffer_type = MYSQL_TYPE_BLOB;
  results[i].buffer = categories;
  results[i].buffer_length = sizeof(categories);
  results[i++].length = &categories_len;

  assert(i == sizeof(results) / sizeof(results[0]));

  if(mysql_stmt_bind_result(statement, results))
  {
    std::cerr << "MessageExport: mysql_stmt_bind_result failed. Error code "
              << std::dec << mysql_stmt_errno(statement) << ", description:\n"
              << mysql_stmt_error(statement) << std::endl;        

    mysql_stmt_close(statement);
    return -1;
  }  
  
  while(true)
  {
    if(mysql_stmt_execute(statement))
    {
      std::cerr << "MessageExport: mysql_stmt_execute failed. Error code "
                << std::dec << mysql_stmt_errno(statement)
                << ", description:\n"
                << mysql_stmt_error(statement) << std::endl;        
      
      mysql_stmt_close(statement);
      return -1;
    }

    if(mysql_stmt_store_result(statement))
    {
      std::cerr << "MessageExport: mysql_stmt_store_result failed. "
        "Error code "  << std::dec << mysql_stmt_errno(statement)
                << ", description:\n"
                << mysql_stmt_error(statement) << std::endl;        
      
      mysql_stmt_close(statement);
      return -1;
    }
    
    int res = 0;
    id = 0;
    size_t lost_thumbs = 0;
    
    while((res = mysql_stmt_fetch(statement)) == 0)
    {
      if(!published)
      {
        // Skip message updated in DB post-mortem; read more in
        // NewsGate::Message::MessageLoader::load_messages.
        ++zombi_messages;

        event_id = 0;
        event_capacity = 0;
        impressions = 0;
        clicks = 0;
        visited = 0;
        
        categories_len = 0;
        categorizer_hash = 0;
        dict_hash = 0;
            
        continue;
      }
      
      if(feed && *feed != '\0')
      {
        std::string broken_down_msg((char*)broken_down, broken_down_len); 
        std::istringstream istr(broken_down_msg);

        ::NewsGate::Message::StoredMessage msg;        
        msg.read_broken_down(istr);
        
        if(strcmp(msg.source_url.c_str(), feed))
        {
          ++skipped;
        
          event_id = 0;
          event_capacity = 0;
          impressions = 0;
          clicks = 0;
          visited = 0;
        
          categories_len = 0;
          categorizer_hash = 0;
          dict_hash = 0;

          continue;
        }
      }
      
      if(records_in_file == msg_per_file)
      {
        output.close();
      }
      
      if(!output.is_open())
      {
        std::string full_name = filename;

        if(msg_per_file == SIZE_MAX)
        {
          full_name = filename;
        }
        else
        {
          std::ostringstream ostr;
          
          ostr << full_name << "."
               << El::Moment(ACE_OS::gettimeofday()).dense_format()
               << "." << rand();

          full_name = ostr.str();
        }
        
        output.open(full_name.c_str(), std::ios::out);

        if(!output.is_open())
        {
          std::cerr << "MessageExport: failed to open file '" << full_name
                    << "' for write access\n";
          
          mysql_stmt_free_result(statement);
          mysql_stmt_close(statement);
          
          return -1;
        }

        records_in_file = 0;
        
        bstr << (uint32_t)0  // file type
             << (uint32_t)6; // file version
      }

      url[url_len] = '\0';      
      source_title[source_title_len] = '\0';
      source_html_link[source_html_link_len] = '\0';

      bstr << (uint64_t)id << (uint64_t)event_id << (uint32_t)event_capacity
           << (uint8_t)flags << (uint64_t)signature << (uint64_t)url_signature
           << (uint64_t)source_id << (uint32_t)dict_hash
           << (uint64_t)impressions << (uint64_t)clicks
           << (uint64_t)published << (uint64_t)fetched << (uint64_t)visited
           << (uint16_t)space;

      // Values from MessageStat reset as can be absent for next message
      event_id = 0;
      event_capacity = 0;
      impressions = 0;
      clicks = 0;
      visited = 0;
 
      // Values from MessageDict reset as can be absent for next message
      dict_hash = 0;
      
      bstr << (uint64_t)complements_len;
      bstr.write_raw_bytes(complements, complements_len);
      
      bstr.write_string_buff(url);
      
      bstr << (uint16_t)lang << (uint16_t)country;
      
      bstr.write_string_buff(source_title);
      bstr.write_string_buff(source_html_link);

      bstr << (uint64_t)broken_down_len;
      bstr.write_raw_bytes(broken_down, broken_down_len);

      // TODO: uncomment together with importing code
//      bstr << (uint32_t)categorizer_hash;
      
      bstr << (uint64_t)categories_len;
      bstr.write_raw_bytes(categories, categories_len);

      // Values from MessageCat reset as can be absent for next message
      categories_len = 0;
      categorizer_hash = 0;
      
      if(flags & Message::StoredMessage::MF_HAS_THUMBS)
      {
        try
        {
          Message::StoredMessage msg;
          
          msg.id = Message::Id(id);
          msg.published = published;
          
          Message::StoredContent_var content = new Message::StoredContent();
          msg.content = content;
        
          std::string compl_str((const char*)complements, complements_len);
          std::istringstream istr(compl_str);

          Message::WordPosition description_base = 0;
          content->read_complements(istr, description_base);

          Message::StoredImageArray& images = *content->images.get();
          
          try
          {
            msg.read_image_thumbs(thumbnail_dir, -1, -1);
          }
          catch(const El::Exception& e)
          {
          }
          
          for(size_t i = 0; i < images.size(); i++)
          {
            Message::ImageThumbArray& thumbs = images[i].thumbs;
              
            assert(!thumbs.empty());
              
            for(size_t j = 0; j < thumbs.size(); j++)
            {
              Message::ImageThumb& thumb = thumbs[j];

              if(thumb.empty())
              {
                bstr << (uint32_t)0;
                
                ++lost_thumbs;
                ++total_lost_thumbs;
              }
              else
              {
                bstr << thumb.length;
                bstr.write_raw_bytes(thumb.image.get(), thumb.length);
                
                ++thumbnails;
              }
            }
          }
          
        }
        catch(...)
        {
          mysql_stmt_free_result(statement);
          mysql_stmt_close(statement);
          throw;
        }
      }
        
      ++records;
      ++records_in_file;

      if(records % 100000 == 0)
      {
        std::cerr << records << " dumped";

        if(lost_thumbs)
        {
          std::cerr << ", " << lost_thumbs << " thums lost";
          lost_thumbs = 0;
        }

        std::cerr << std::endl;
      }
    }
    
    if(res && res != MYSQL_NO_DATA)
    {
      std::cerr << "MessageExport: mysql_stmt_fetch failed. Error code "
                << std::dec << mysql_stmt_errno(statement) << "/" << res
                << ", description:\n" << mysql_stmt_error(statement)
                << std::endl;

      mysql_stmt_free_result(statement);
      mysql_stmt_close(statement);
      
      return -1;
    }

    mysql_stmt_free_result(statement);

    if(id)
    {
      prev_id = id;
    }
    else
    {
      break;
    }    
  }
  
  mysql_stmt_close(statement);
    
  std::cerr << records << " messages, " << skipped << " skipped, "
            << thumbnails << " thumbs, " << total_lost_thumbs
            << " lost thumbs, " << zombi_messages
            << " zombi-messages\n";
  
  return 0;
}
