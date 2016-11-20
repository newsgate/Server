/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   DataFetchApp.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#include <iostream>
#include <sstream>

#include <mysql/mysql.h>

#include <ace/OS.h>
#include <ace/High_Res_Timer.h>

#include <El/Moment.hpp>

namespace
{
  const char USAGE[] =
  "Usage: DataFetchTest";
}

int test_bail_out(MYSQL* mysql);
int test_bail_out_prepared(MYSQL* mysql);

int test_query_msg_by_1000(MYSQL* mysql, bool store_result);
int test_query_msg_by_time(MYSQL* mysql, bool store_result);

int test_query_msg_by_1000_prepared(MYSQL* mysql, bool store_result);

int
main(int argc, char** argv)
{
  int r = -1;
  
  if(mysql_thread_init())
  {
    std::cerr << "my_thread_init failed\n";
    return r;
  }

  MYSQL* mysql = mysql_init(NULL);
  my_bool my_true = true;

  if(mysql)
  {
    if(mysql_options(mysql, MYSQL_OPT_RECONNECT, &my_true))
    {
      std::cerr << "mysql_options(MYSQL_OPT_RECONNECT) failed. Error code " << std::dec
                << mysql_errno(mysql) << ", description:\n"
                << mysql_error(mysql) << std::endl;
    }
    else if(mysql_options(mysql, MYSQL_SET_CHARSET_NAME, "utf8"))
    {
      std::cerr << "mysql_options(MYSQL_SET_CHARSET_NAME) failed. Error code "
                << std::dec
                << mysql_errno(mysql) << ", description:\n"
                << mysql_error(mysql) << std::endl;
    }
    else if(mysql_real_connect(
              mysql,
              "",
              "root",
              "",
              "NewsGate",
              0,
              "/home/karen_arutyunov/projects/NewsGate/Workspace/var/run/"
              "MySQL/mysql.socket",
              0) == 0)
    {
      std::cerr << "mysql_real_connect failed. Error code "  << std::dec
                << mysql_errno(mysql) << ", description:\n"
                << mysql_error(mysql) << std::endl;
    }
    else 
    {
      r = test_query_msg_by_1000(mysql, false);
    }
       
    mysql_close(mysql);    
  }
  else
  {
    std::cerr << "mysql_init failed\n";
  }

  mysql_thread_end();
  
  return r;
}

int
test_bail_out(MYSQL* mysql)
{
  unsigned long long records = 0;

  ACE_High_Res_Timer timer;
  timer.start();

  if(mysql_query(mysql, "select id from Message"))
  {
    std::cerr << "mysql_query failed. Error code "  << std::dec
              << mysql_errno(mysql) << ", description:\n"
              << mysql_error(mysql) << std::endl;

    return -1;
  }

  MYSQL_RES* result = mysql_use_result(mysql);

  if(mysql_field_count(mysql) > 0 && result == NULL)
  {
    std::cerr << "mysql_use_result failed. Error code "  << std::dec
              << mysql_errno(mysql) << ", description:\n"
              << mysql_error(mysql) << std::endl;
    
    return -1;
  }

  MYSQL_ROW row = 0;
  unsigned long long id = 0;

  while((row = mysql_fetch_row(result)) != 0)
  {
      std::istringstream istr(row[0]);
      istr >> id;
      ++records;
//    std::cout << row[0] << std::endl;
  }  

  mysql_free_result(result);

  timer.stop();
  ACE_Time_Value tm;
  timer.elapsed_time(tm);

  std::cerr << records << " recs for " << El::Moment::time(tm) << std::endl;
  
  return 0;
}

int
test_bail_out_prepared(MYSQL* mysql)
{
  int r = -1;
  MYSQL_STMT* statement = mysql_stmt_init(mysql);

  if(statement == 0)
  {
    std::cerr << "mysql_stmt_init failed. Error code "  << std::dec
              << mysql_errno(mysql) << ", description:\n"
              << mysql_error(mysql) << std::endl;
  }

  static const char query[] = "select id from Message";
  
  if(mysql_stmt_prepare(statement, query, strlen(query)))
  {
    std::cerr << "mysql_stmt_prepare failed. Error code "  << std::dec
              << mysql_errno(mysql) << ", description:\n"
              << mysql_error(mysql) << std::endl;
  }
  else if(mysql_stmt_execute(statement))
  {
    std::cerr << "mysql_stmt_execute failed. Error code "  << std::dec
              << mysql_errno(mysql) << ", description:\n"
              << mysql_error(mysql) << std::endl;        
  }
  else
  {
    unsigned long long id = 0;
    
    MYSQL_BIND result;
    memset(&result, 0, sizeof(result));

    result.buffer_type = MYSQL_TYPE_LONGLONG;
    result.buffer = &id;

    if(mysql_stmt_bind_result(statement, &result))
    {
      std::cerr << "mysql_stmt_bind_result failed. Error code "  << std::dec
                << mysql_errno(mysql) << ", description:\n"
                << mysql_error(mysql) << std::endl;        
    }
    else
    {
      int res = 0;

      for(size_t i = 0; (res = mysql_stmt_fetch(statement)) == 0 && i < 10;
          ++i)
      {
        std::cerr << id << std::endl;
      }
      
      mysql_stmt_free_result(statement);

      if(res && res != MYSQL_NO_DATA)
      {
        std::cerr << "mysql_stmt_fetch failed. Error code "  << std::dec
                  << mysql_errno(mysql) << ", description:\n"
                  << mysql_error(mysql) << std::endl;        
      }
      else
      {
        r = 0;
      }
    }
  }
  
  mysql_stmt_close(statement);
    
  return r;
}

int
test_query_msg_by_1000(MYSQL* mysql, bool store_result)
{  
  unsigned long long prev_id = 0;
  unsigned long long records = 0;
   
  ACE_High_Res_Timer timer;
  timer.start();
  
  while(true)
  {    
    std::ostringstream ostr;
    
//    ostr << "select id from Message where id > " << prev_id
//         << " ORDER BY id LIMIT 1000";

    ostr << "select id, event_id, event_capacity, flags, signature, "
            "dict_hash, updated, fetched, type, space, lang, country, "
            "source_title, broken_down, categories from Message where id > "
         << prev_id << " ORDER BY id LIMIT 1000";

    ACE_High_Res_Timer timer;
    timer.start();
    
    if(mysql_query(mysql, ostr.str().c_str()))
    {
      std::cerr << "mysql_query failed. Error code "  << std::dec
                << mysql_errno(mysql) << ", description:\n"
                << mysql_error(mysql) << std::endl;

      return -1;
    }

    MYSQL_RES* result = store_result ? mysql_store_result(mysql) :
      mysql_use_result(mysql);
     

    if(mysql_field_count(mysql) > 0 && result == NULL)
    {
      std::cerr << "mysql_use_result failed. Error code "  << std::dec
                << mysql_errno(mysql) << ", description:\n"
                << mysql_error(mysql) << std::endl;
    
      return -1;
    }

    unsigned long long id = 0;
    MYSQL_ROW row = 0;

    while((row = mysql_fetch_row(result)) != 0)
    {
      std::istringstream istr(row[0]);
      istr >> id;
      ++records;
//      std::cout << id << std::endl;
    }

    mysql_free_result(result);

    timer.stop();
    ACE_Time_Value tm;
    timer.elapsed_time(tm);

//    std::cout << "Reading after " << prev_id << " for "
//              << El::Moment::time(tm) << std::endl;

    if(tm.sec())
    {
      std::cout << tm.sec();
    
      std::cout.width(6);
      std::cout.fill('0');
    }
    
    std::cout << tm.usec() << std::endl;    

    if(id)
    {
      prev_id = id;
    }
    else
    {
      break;
    }
  }
  
  timer.stop();
  ACE_Time_Value tm;
  timer.elapsed_time(tm);

  std::cerr << records << " recs for " << El::Moment::time(tm) << std::endl;
  
  return 0;
}

int
test_query_msg_by_time(MYSQL* mysql, bool store_result)
{  
  unsigned long long records = 0;

  uint64_t time = ACE_OS::gettimeofday().sec();
  uint64_t end_time = time - 86400 * 30 * 6;

  uint64_t min_time = UINT64_MAX;
  uint64_t max_time = 0;
  
  ACE_High_Res_Timer timer;
  timer.start();
  
  while(time > end_time)
  {
    time_t lower_time = time - 1800;
    
    std::ostringstream ostr;
    
//    ostr << "select id from Message where id > " << prev_id
//         << " ORDER BY id LIMIT 1000";

    ostr << "select id, event_id, event_capacity, flags, signature, "
      "dict_hash, updated, fetched, type, space, lang, country, "
      "source_title, broken_down, categories from Message where "
      "updated < " << time << " and updated >= " << lower_time;

    ACE_High_Res_Timer timer;
    timer.start();
    
    if(mysql_query(mysql, ostr.str().c_str()))
    {
      std::cerr << "mysql_query failed. Error code "  << std::dec
                << mysql_errno(mysql) << ", description:\n"
                << mysql_error(mysql) << std::endl;

      return -1;
    }

    MYSQL_RES* result = store_result ? mysql_store_result(mysql) :
      mysql_use_result(mysql);     

    if(mysql_field_count(mysql) > 0 && result == NULL)
    {
      std::cerr << "mysql_use_result failed. Error code "  << std::dec
                << mysql_errno(mysql) << ", description:\n"
                << mysql_error(mysql) << std::endl;
    
      return -1;
    }

    MYSQL_ROW row = 0;

    while((row = mysql_fetch_row(result)) != 0)
    {
      std::istringstream istr(row[6]);

      uint64_t tm = 0;
      istr >> tm;

      min_time = std::min(min_time, tm);
      max_time = std::max(max_time, tm);
      
      ++records;
//      std::cout << id << std::endl;
    }

    mysql_free_result(result);

    timer.stop();
    ACE_Time_Value tm;
    timer.elapsed_time(tm);

//    std::cout << "Reading after " << prev_id << " for "
//              << El::Moment::time(tm) << std::endl;
    
    if(tm.sec())
    {
      std::cout << tm.sec();
    
      std::cout.width(6);
      std::cout.fill('0');
    }
    
    std::cout << tm.usec() << std::endl;    

    time = lower_time;
  }
  
  timer.stop();
  ACE_Time_Value tm;
  timer.elapsed_time(tm);

  std::cerr << records << " recs for " << El::Moment::time(tm) << std::endl
            << "min_time: " << min_time << ", max_time: " << max_time
            << std::endl;
  
  return 0;
}

int
test_query_msg_by_1000_prepared(MYSQL* mysql, bool store_result)
{
  unsigned long long prev_id = 0;
  unsigned long long records = 0;
   
  ACE_High_Res_Timer timer;
  timer.start();
  
  MYSQL_STMT* statement = mysql_stmt_init(mysql);

  if(statement == 0)
  {
    std::cerr << "mysql_stmt_init failed. Error code "  << std::dec
              << mysql_errno(mysql) << ", description:\n"
              << mysql_error(mysql) << std::endl;
    return -1;
  }

  static const char query[] = "select id, event_id, event_capacity, flags, "
    "signature, dict_hash, updated, fetched, type, space, lang, country, "
    "source_title, broken_down, categories "
    " from Message where id > ? ORDER BY id LIMIT 1000";
  
  if(mysql_stmt_prepare(statement, query, strlen(query)))
  {
    std::cerr << "mysql_stmt_prepare failed. Error code "  << std::dec
              << mysql_errno(mysql) << ", description:\n"
              << mysql_error(mysql) << std::endl;
    
    mysql_stmt_close(statement);
    return -1;
  }

  MYSQL_BIND param;
  memset(&param, 0, sizeof(param));
  
  param.buffer_type = MYSQL_TYPE_LONGLONG;
  param.buffer = &prev_id;
  param.is_unsigned = true;
  
  if(mysql_stmt_bind_param(statement, &param))
  {      
    std::cerr << "mysql_stmt_bind_param failed. Error code "  << std::dec
              << mysql_errno(mysql) << ", description:\n"
              << mysql_error(mysql) << std::endl;
      
    mysql_stmt_close(statement);
    return -1;
  }

  unsigned long long id = 0;
  unsigned long long event_id = 0;
  unsigned int event_capacity = 0;
  unsigned char flags = 0;
  unsigned long long signature = 0;
  unsigned int dict_hash = 0;
  unsigned long long updated = 0;
  unsigned long long fetched = 0;
  unsigned short type = 0;
  unsigned short space = 0;
  unsigned short lang = 0;
  unsigned short country = 0;
  
  char source_title[1024 * 6 + 1];
  unsigned long source_title_len = 0;

  unsigned char broken_down[65535];
  unsigned long broken_down_len = 0;
    
  unsigned char categories[65535];
  unsigned long categories_len = 0;
    
  MYSQL_BIND results[15];
  memset(results, 0, sizeof(results));

  results[0].buffer_type = MYSQL_TYPE_LONGLONG;
  results[0].buffer = &id;
  results[0].is_unsigned = true;

  results[1].buffer_type = MYSQL_TYPE_LONGLONG;
  results[1].buffer = &event_id;
  results[1].is_unsigned = true;
  
  results[2].buffer_type = MYSQL_TYPE_LONG;
  results[2].buffer = &event_capacity;
  results[2].is_unsigned = true;

  results[3].buffer_type = MYSQL_TYPE_TINY;
  results[3].buffer = &flags;
  results[3].is_unsigned = true;

  results[4].buffer_type = MYSQL_TYPE_LONGLONG;
  results[4].buffer = &signature;
  results[4].is_unsigned = true;

  results[5].buffer_type = MYSQL_TYPE_LONG;
  results[5].buffer = &dict_hash;
  results[5].is_unsigned = true;

  results[6].buffer_type = MYSQL_TYPE_LONGLONG;
  results[6].buffer = &updated;
  results[6].is_unsigned = true;

  results[7].buffer_type = MYSQL_TYPE_LONGLONG;
  results[7].buffer = &fetched;
  results[7].is_unsigned = true;
  
  results[8].buffer_type = MYSQL_TYPE_SHORT;
  results[8].buffer = &type;
  results[8].is_unsigned = true;

  results[9].buffer_type = MYSQL_TYPE_SHORT;
  results[9].buffer = &space;
  results[9].is_unsigned = true;

  results[10].buffer_type = MYSQL_TYPE_SHORT;
  results[10].buffer = &lang;
  results[10].is_unsigned = true;

  results[11].buffer_type = MYSQL_TYPE_SHORT;
  results[11].buffer = &country;
  results[11].is_unsigned = true;
  
  results[12].buffer_type = MYSQL_TYPE_VAR_STRING;
  results[12].buffer = source_title;
  results[12].buffer_length = sizeof(source_title);
  results[12].length = &source_title_len;

  results[13].buffer_type = MYSQL_TYPE_BLOB;
  results[13].buffer = broken_down;
  results[13].buffer_length = sizeof(broken_down);
  results[13].length = &broken_down_len;

  results[14].buffer_type = MYSQL_TYPE_BLOB;
  results[14].buffer = categories;
  results[14].buffer_length = sizeof(categories);
  results[14].length = &categories_len;

  if(mysql_stmt_bind_result(statement, results))
  {
    std::cerr << "mysql_stmt_bind_result failed. Error code "  << std::dec
              << mysql_errno(mysql) << ", description:\n"
              << mysql_error(mysql) << std::endl;        

    mysql_stmt_close(statement);
    return -1;
  }  
  
  while(true)
  {
    ACE_High_Res_Timer timer;
    timer.start();
    
    if(mysql_stmt_execute(statement))
    {
      std::cerr << "mysql_stmt_execute failed. Error code "  << std::dec
                << mysql_errno(mysql) << ", description:\n"
                << mysql_error(mysql) << std::endl;        
      
      mysql_stmt_close(statement);
      return -1;
    }

    if(store_result && mysql_stmt_store_result(statement))
    {
      std::cerr << "mysql_stmt_store_result failed. Error code "  << std::dec
                << mysql_errno(mysql) << ", description:\n"
                << mysql_error(mysql) << std::endl;        
      
      mysql_stmt_close(statement);
      return -1;
    }

    int res = 0;
    id = 0;
    
    while((res = mysql_stmt_fetch(statement)) == 0)
    {
      ++records;
/*
      std::cout << id << " -->";
      std::cout.write((const char*)categories, categories_len);
      std::cout << "<--" << std::endl;
*/
    }
    
    if(res && res != MYSQL_NO_DATA)
    {
      std::cerr << "mysql_stmt_fetch failed. Error code "  << std::dec
                << mysql_errno(mysql) << "/" << res << ", description:\n"
                << mysql_error(mysql) << std::endl;        

      mysql_stmt_free_result(statement);
      mysql_stmt_close(statement);
      
      return -1;
    }

    mysql_stmt_free_result(statement);

    timer.stop();
    ACE_Time_Value tm;
    timer.elapsed_time(tm);

//    std::cout << "Reading after " << prev_id << " for "
//              << El::Moment::time(tm) << std::endl;

    if(tm.sec())
    {
      std::cout << tm.sec();
    
      std::cout.width(6);
      std::cout.fill('0');
    }
    
    std::cout << tm.usec() << std::endl;

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
    
  timer.stop();
  ACE_Time_Value tm;
  timer.elapsed_time(tm);

  std::cerr << records << " recs for " << El::Moment::time(tm) << std::endl;
  
  return 0;
}

