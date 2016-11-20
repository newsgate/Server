/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/tests/HashTable/HashTableMain.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <sstream>
#include <iostream>

#include <ext/hash_map>
#include <ext/hash_set>
#include <google/sparse_hash_map>
#include <google/sparse_hash_set>

#include <ace/High_Res_Timer.h>
#include <ace/Synch.h>
#include <ace/Guard_T.h>

#include <El/Exception.hpp>
#include <El/String/Manip.hpp>
#include <El/Hash/Hash.hpp>
#include <El/Utility.hpp>

EL_EXCEPTION(Exception, El::ExceptionBase);


namespace
{
  const char USAGE[] = "Usage: HashTableTest";
}
/*
class NumberSet :
  public __gnu_cxx::hash_set<unsigned long, El::Hash::Numeric<unsigned long> >
{
};
*/

class NumberSet :
  public google::sparse_hash_set<unsigned long,
                                 El::Hash::Numeric<unsigned long> >
{
public:
  NumberSet() throw(El::Exception) { set_deleted_key(ULONG_MAX); }
};
/*
class NumberLightSet : public El::LightSet<unsigned long>
{
};

class NumberLightSet2 :
  public El::LightSet<unsigned long,
                      El::Allocator::Heap<unsigned long> >
{
};
*/
class NumberMap :
  public google::sparse_hash_map<unsigned long,
                                 unsigned long,
                                 El::Hash::Numeric<unsigned long> >
{
public:
  NumberMap() throw(El::Exception) { set_deleted_key(ULONG_MAX); }
};

unsigned long long
avarage_time(const ACE_Time_Value& time, unsigned long checks) throw()
{
  return ((unsigned long long)time.sec() * 1000000 + time.usec()) /
    ((unsigned long long)checks);
}

template<typename Allocator>
void
test_allocator(const char* allocator_name) throw(El::Exception)
{
  Allocator allocator;
  
  unsigned long count = 1000000;
  unsigned long size = 60;
  unsigned long total_size = 0;

  typedef typename Allocator::value_type* ElementPtr;

  ElementPtr* elems = new ElementPtr[count];
  
  ACE_Time_Value alloc_time;
  ACE_Time_Value free_time;
  
  ACE_High_Res_Timer timer;

  unsigned long base = El::Utility::mem_used();

  timer.start();

  for(unsigned long i = 0; i < count; i++)
  {
    unsigned long sz = size + i % 20;
    total_size += sz;
    
    elems[i] = allocator.alloc(sz);
  }

  timer.stop();
  timer.elapsed_time(alloc_time);
  
  unsigned long level = El::Utility::mem_used();

  unsigned long mem_size = level > base ? (level - base) * 1024 : 0;
  
  unsigned long expected_size =
    total_size * sizeof(typename Allocator::value_type);  
/*
  timer.start();

  for(unsigned long i = 0; i < count; i++)
  {
    allocator.free(elems[i], size + i % 20);
  }

  timer.stop();
  timer.elapsed_time(free_time);

  delete [] elems;
*/
  std::cerr << allocator_name << " memory: size " << mem_size << "; overhead "
            << (mem_size > expected_size ?
                (mem_size - expected_size) * 100 / expected_size : 0)
            << "%\n";
  
  std::cerr << allocator_name << " performance: alloc "
            << avarage_time(alloc_time, count) << " usec, free "
            << avarage_time(free_time, count) << " usec\n";
}

template<typename SetType>
void
measure_hash_mem_effectiveness(unsigned long items,
                               const char* hash_type_name,
                               long checks = 100000)
  throw(El::Exception)
{
  ACE_High_Res_Timer timer;
  ACE_Time_Value tm;
  ACE_Time_Value total_time;

  unsigned long long base = El::Utility::mem_used();

  for(long i = 0; i < checks; i++)
  {
    SetType* hash = new SetType();

    timer.start();
    
    for(unsigned long j = 0; j < items; j++)
    {
      hash->insert(j + i);
    }

    timer.stop();
    timer.elapsed_time(tm);
    total_time += tm;
  }

  unsigned long level = El::Utility::mem_used();
  unsigned long size = level > base ? (level - base) * 1024 / checks : 0;
  
  unsigned long expected_size =
    (items ? items : 1) * sizeof(typename SetType::value_type);

  unsigned long long avg_usec = avarage_time(total_time, checks);  
    
  std::cerr << "Avg. " << items << "-item " << hash_type_name << " size: "
            << size << "; overhead "
            << (size > expected_size ?
                (size - expected_size) * 100 / expected_size : 0)
            << "%; fill " << avg_usec << " usec\n";
}

template<typename SetType>
void
test_set(const char* hash_type_name) throw(El::Exception)
{
  for(unsigned long i = 0; i < 10; i++)
  {
    measure_hash_mem_effectiveness<SetType>(i, hash_type_name);
  }
    
  for(unsigned long i = 1; i < 10; i++)
  {
    measure_hash_mem_effectiveness<SetType>(i * 10, hash_type_name);
  }    

  measure_hash_mem_effectiveness<SetType>(100, "SetType");
  measure_hash_mem_effectiveness<SetType>(1000, "SetType");
/*  
  measure_hash_mem_effectiveness<SetType>(10000, "SetType", 1000);
  measure_hash_mem_effectiveness<SetType>(100000, "SetType", 10);
*/
}

int
main(int argc, char** argv)
{
//  std::cerr << A<int>::b.x;
//  return 0;
/*  
  El::LightSet<int> a;
  El::LightSet<int> b(a);

  b.insert(3);
  b.insert(15);
  b.insert(6);
  b.insert(17);
  b.insert(10);
  b.insert(20);
  b.insert(11);

  const int* x = b.find(4);
  assert(x == b.end());

  x = b.find(4);
  assert(x == b.end());
  
  x = b.find(1);
  assert(x == b.end());

  x = b.find(111);
  assert(x == b.end());

  x = b.find(17);
  assert(x != b.end() && *x == 17);

  x = b.find(15);
  assert(x != b.end() && *x == 15);

  b.erase(15);

  x = b.find(15);
  assert(x == b.end());

  x = b.find(3);
  assert(x != b.end() && *x == 3);

  x = b.find(20);
  assert(x != b.end() && *x == 20);

  b.swap(a);
  a.clear();

  std::cerr << "El::LightSet functional test succeeded\n";
*/
  try
  {
    /*
    test_allocator<El::Allocator::Dense<unsigned long> >("Dense");
    test_allocator<El::Allocator::Heap<unsigned long> >("Heap");

    return 0;
    
    test_set<NumberLightSet>("NumberLightSet");
    test_set<NumberLightSet2>("NumberLightSet2");
    */
    test_set<NumberSet>("NumberSet");
    
/*
    NumberSet* plast_hash = 0;
    
    unsigned long count = 1000000;
    unsigned long long base = El::Utility::mem_used();
    
    for(unsigned long i = 0; i < count; i++)
    {
      NumberSet* phash = new NumberSet();
      plast_hash = phash;
    }

    unsigned long long level = El::Utility::mem_used();
    
    std::cerr << "Avg. empty size hashset: " << (level - base) * 1024 / count
              << std::endl;

    base = level;

    for(unsigned long i = 0; i < count; i++)
    {
      NumberSet* phash = new NumberSet();
      phash->insert(1);
      
      plast_hash = phash;
    }

    level = El::Utility::mem_used();
    
    std::cerr << "Avg. 1-item size hashset: " << (level - base) * 1024 / count
              << std::endl;

    base = level;

    for(unsigned long i = 0; i < count; i++)
    {
      NumberSet* phash = new NumberSet();

      for(unsigned long j = 0; j < 6; j++)
      {
        phash->insert(j);
      }
      
      plast_hash = phash;
    }

    level = El::Utility::mem_used();
    
    std::cerr << "Avg. 6-item size hashset: " << (level - base) * 1024 / count
              << std::endl;

    delete plast_hash;

    base = level;

    for(unsigned long i = 0; i < count; i++)
    {
      NumberSet* phash = new NumberSet();

      for(unsigned long j = 0; j < 20; j++)
      {
        phash->insert(j);
      }
      
      plast_hash = phash;
    }

    level = El::Utility::mem_used();
    
    std::cerr << "Avg. 20-item size hashset: " << (level - base) * 1024 / count
              << std::endl;

    delete plast_hash;

    base = level;
    const char* last_ptr = 0;
      
    for(unsigned long i = 0; i < count; i++)
    {
      last_ptr = new char[1];
    }

    level = El::Utility::mem_used();
    
    std::cerr << "Avg. 1-byte buffer size: " << (level - base) * 1024 / count
              << std::endl;


    delete [] last_ptr;
    
    base = level;
    last_ptr = 0;
      
    for(unsigned long i = 0; i < count; i++)
    {
      last_ptr = new char[20];
    }

    level = El::Utility::mem_used();
    
    std::cerr << "Avg. 20-byte buffer size: " << (level - base) * 1024 / count
              << std::endl;


    delete [] last_ptr;

    base = level;
    unsigned long* ulong_ptr = 0;
      
    for(unsigned long i = 0; i < count; i++)
    {
      ulong_ptr = new unsigned long[1];
    }

    level = El::Utility::mem_used();
    
    std::cerr << "Avg. 1-elems ulong buffer size: " << (level - base) * 1024 / count
              << std::endl;

    delete [] ulong_ptr;

    base = level;
    ulong_ptr = 0;
      
    for(unsigned long i = 0; i < count; i++)
    {
      ulong_ptr = new unsigned long[6];
    }

    level = El::Utility::mem_used();
    
    std::cerr << "Avg. 6-elems ulong buffer size: " << (level - base) * 1024 / count
              << std::endl;

    delete [] ulong_ptr;

    base = level;
    ulong_ptr = 0;
      
    for(unsigned long i = 0; i < count; i++)
    {
      ulong_ptr = new unsigned long[20];
    }

    level = El::Utility::mem_used();
    
    std::cerr << "Avg. 20-elems ulong buffer size: " << (level - base) * 1024 / count
              << std::endl;

    delete [] ulong_ptr;
*/
/*
    NumberMap nmap;
    
    typedef std::vector<unsigned long> NumberArray;
    NumberArray values;

    const unsigned long count = 10000;
    
    values.reserve(count);
    
    while(true)
    {
      for(unsigned long i = 0; i < count; i++)
      {
        unsigned long val = rand();
      
        nmap[val] = val;
        nmap.erase(val);

        values.push_back(val);
      }
      
      for(unsigned long i = 0; i < count; i++)
      {
        unsigned long j = (unsigned long long)rand() * values.size() /
          ((unsigned long long)RAND_MAX + 1);

        values.erase(values.begin() + j);
      }
      
    }
*/  
    return 0;
  }
  catch (const El::Exception& e)
  {
    std::cerr << e.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << "unknown exception caught.\n";
  }
  
  return -1;
}
