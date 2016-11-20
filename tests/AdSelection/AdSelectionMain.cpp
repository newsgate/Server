/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/tests/AdSelection/AdSelectionMain.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <stdlib.h>

#include <sstream>
#include <iostream>
#include <cmath>

#include <ace/High_Res_Timer.h>

#include <El/Exception.hpp>
#include <El/String/Manip.hpp>
#include <El/String/ListParser.hpp>
#include <El/Moment.hpp>

#include "AdSelectionMain.hpp"

namespace
{
  const char USAGE[] =
    "Usage: AdSelectionTest ( [-help] | [-verbose] [-not-randomize] )";
}

using namespace NewsGate;

int
main(int argc, char** argv)
{
  Application app;
  return app.run(argc, argv);
}

int
Application::run(int argc, char** argv) throw()
{
  try
  {
    for(int i = 1; i < argc; i++)
    {
      const char* arg = argv[i];
      
      if(!strcmp(arg, "-verbose"))
      {
        verbose_ = true;
      }
      else if(!strcmp(arg, "-not-randomize"))
      {
        randomize_ = false;
      }
      else if(!strcmp(arg, "-help"))
      {
        std::cerr << USAGE << std::endl;
        return 0;
      }
      else
      {
        std::ostringstream ostr;
        ostr << "Application::run: unexpected argument " << arg;
        throw Exception(ostr.str());
      }
    }

    srand(randomize_ ? time(0) : 0);

    test_selector();
    
    return 0;
  }
  catch (const El::Exception& e)
  {
    std::cerr << e.what() << std::endl;
    std::cerr << USAGE << std::endl;
  }
  catch (...)
  {
    std::cerr << "unknown exception caught.\n";
  }
   
  return -1;
}

void
Application::test_selector() throw(Exception, El::Exception)
{
  typedef std::vector<ACE_Time_Value> TimeArray;

  TimeArray tm;

  tm.push_back(test1());
  tm.push_back(test2());
  tm.push_back(test3());
  tm.push_back(test4());
  tm.push_back(test5());
  tm.push_back(test6());
  tm.push_back(test7());
  tm.push_back(test8());
  tm.push_back(test9());
  tm.push_back(test10());
  tm.push_back(test11());
  tm.push_back(test12());
  tm.push_back(test13());
  tm.push_back(test14());
  tm.push_back(test15());
  tm.push_back(test16());
  tm.push_back(test17());
  tm.push_back(test18());
  tm.push_back(test19());
  tm.push_back(test20());
  tm.push_back(test21());
  tm.push_back(test22());
  tm.push_back(test23());
  tm.push_back(test24());
  tm.push_back(test25());

  for(TimeArray::const_iterator i(tm.begin()), e(tm.end()); i != e; ++i)
  {
    std::cerr << (i - tm.begin() + 1) << ": "
              << El::Moment::time(*i) << std::endl;
  }
}

ACE_Time_Value
Application::test1() throw(Exception, El::Exception)
{
  Ad::Selector selector(10, 10, 10, 10, 0);

  selector.pages[Ad::PI_DESK_PAPER].max_ad_num = 3;

  Ad::Creative creative(1, 1, 0, 700, 200, 2, 1, "1", Ad::CI_DIRECT);
  
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative);

  selector.finalize();

  Ad::SelectionContext context(Ad::PI_DESK_PAPER, rand());

  context.slots.push_back(Ad::SI_DESK_PAPER_MSG1);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSG2);

  Ad::SelectionResult result;
  
  ACE_High_Res_Timer timer;
  timer.start();
  
  selector.select(context, result);

  timer.stop();
  ACE_Time_Value tm;
  timer.elapsed_time(tm);          

  Ad::SelectionResult etalon_result;
  
  etalon_result.ads.push_back(Ad::Selection(creative.id,
                                            Ad::SI_DESK_PAPER_MSG1,
                                            creative.width,
                                            creative.height,
                                            creative.text.c_str(),
                                            Ad::CI_DIRECT));

  if(result.ads != etalon_result.ads)
  {
    std::ostringstream ostr;
    ostr << "Application::test1: failed. Result:\n";
    result.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }
  
  return tm;
}

ACE_Time_Value
Application::test2() throw(Exception, El::Exception)
{
  Ad::Selector selector(10, 10, 10, 10, 0);
  selector.pages[Ad::PI_DESK_PAPER].max_ad_num = 3;
  
  Ad::Creative creative1(1, 1, 0, 700, 200, 0.95, 1, "MSG1", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative1);
  
  Ad::Creative creative2(2, 1, 0, 700, 200, 0.80, 1, "MSG2", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG2, creative2);
  
  Ad::Creative creative3(3, 1, 0, 700, 200, 0.65, 1, "MSA1", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA1, creative3);
  
  Ad::Creative creative4(4, 1, 0, 700, 200, 0.75, 1, "MSA2", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA2, creative4);
  
  Ad::Creative creative5(5, 1, 0, 700, 200, 0.50, 1, "RTB1", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_RTB1, creative5);

  selector.finalize();

  Ad::SelectionContext context(Ad::PI_DESK_PAPER, rand());

  context.slots.push_back(Ad::SI_DESK_PAPER_MSG1);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSG2);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSA1);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSA2);
  context.slots.push_back(Ad::SI_DESK_PAPER_RTB1);

  Ad::SelectionResult result;

  ACE_High_Res_Timer timer;
  timer.start();
  
  selector.select(context, result);

  timer.stop();
  ACE_Time_Value tm;
  timer.elapsed_time(tm);          

  Ad::SelectionResult etalon_result;
  
  etalon_result.ads.push_back(Ad::Selection(creative1.id,
                                            Ad::SI_DESK_PAPER_MSG1,
                                            creative1.width,
                                            creative1.height,
                                            creative1.text.c_str(),
                                            Ad::CI_DIRECT));

  etalon_result.ads.push_back(Ad::Selection(creative2.id,
                                            Ad::SI_DESK_PAPER_MSG2,
                                            creative2.width,
                                            creative2.height,
                                            creative2.text.c_str(),
                                            Ad::CI_DIRECT));

  etalon_result.ads.push_back(Ad::Selection(creative4.id,
                                            Ad::SI_DESK_PAPER_MSA2,
                                            creative4.width,
                                            creative4.height,
                                            creative4.text.c_str(),
                                            Ad::CI_DIRECT));

  if(result.ads != etalon_result.ads)
  {
    std::ostringstream ostr;
    ostr << "Application::test2: failed. Result:\n";
    result.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }

  return tm;
}

ACE_Time_Value
Application::test3() throw(Exception, El::Exception)
{
  Ad::Selector selector(10, 10, 10, 10, 0);
  selector.pages[Ad::PI_DESK_PAPER].max_ad_num = 3;

  Ad::Creative creative0(0, 1, 0, 700, 200, 0.97, 1, "MSG1-0", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative0);
  
  Ad::Creative creative1(1, 1, 0, 700, 200, 0.99, 1, "MSG1-1", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative1);
  
  Ad::Creative creative2(2, 1, 0, 700, 200, 0.95, 1, "MSG1-2", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative2);
  
  Ad::Creative creative3(3, 1, 0, 700, 200, 0.80, 1, "MSG2-1", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG2, creative3);
  
  Ad::Creative creative4(4, 1, 0, 700, 200, 0.85, 1, "MSG2-2", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG2, creative4);
  
  Ad::Creative creative5(5, 1, 0, 700, 200, 0.65, 1, "MSA1-1", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA1, creative5);
  
  Ad::Creative creative6(6, 1, 0, 700, 200, 0.70, 1, "MSA1-2", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA1, creative6);
  
  Ad::Creative creative7(7, 1, 0, 700, 200, 0.76, 1, "MSA2-1", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA2, creative7);
  
  Ad::Creative creative8(8, 1, 0, 700, 200, 0.75, 1, "MSA2-2", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA2, creative8);

  selector.finalize();

  Ad::SelectionContext context(Ad::PI_DESK_PAPER, rand());

  context.slots.push_back(Ad::SI_DESK_PAPER_MSG1);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSG2);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSA1);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSA2);

  Ad::SelectionResult result;

  ACE_High_Res_Timer timer;
  timer.start();
  
  selector.select(context, result);

  timer.stop();
  ACE_Time_Value tm;
  timer.elapsed_time(tm);          
  
  Ad::SelectionResult etalon_result;
  
  etalon_result.ads.push_back(Ad::Selection(creative1.id,
                                            Ad::SI_DESK_PAPER_MSG1,
                                            creative1.width,
                                            creative1.height,
                                            creative1.text.c_str(),
                                            Ad::CI_DIRECT));

  etalon_result.ads.push_back(Ad::Selection(creative4.id,
                                            Ad::SI_DESK_PAPER_MSG2,
                                            creative4.width,
                                            creative4.height,
                                            creative4.text.c_str(),
                                            Ad::CI_DIRECT));

  etalon_result.ads.push_back(Ad::Selection(creative7.id,
                                            Ad::SI_DESK_PAPER_MSA2,
                                            creative7.width,
                                            creative7.height,
                                            creative7.text.c_str(),
                                            Ad::CI_DIRECT));

  if(result.ads != etalon_result.ads)
  {
    std::ostringstream ostr;
    ostr << "Application::test3: failed. Result:\n";
    result.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }

  return tm;
}

ACE_Time_Value
Application::test4() throw(Exception, El::Exception)
{
  Ad::Selector selector(10, 10, 10, 10, 0);
  selector.pages[Ad::PI_DESK_PAPER].max_ad_num = 3;

  Ad::Condition condition0(0, 10, 6, 7, 0, 0);
  Ad::Condition condition1(1, 10, 3, 5, 0, 0);
    
  selector.conditions[0] = condition0;
  selector.conditions[1] = condition1;
  
  Ad::Creative creative0(0, 1, 0, 700, 200, 0.97, 1, "MSG1-0", Ad::CI_DIRECT);
  creative0.conditions.push_back(&selector.conditions[0]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative0);
  
  Ad::Creative creative1(1, 1, 0, 700, 200, 0.99, 1, "MSG1-1", Ad::CI_DIRECT);
  creative1.conditions.push_back(&selector.conditions[1]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative1);
  
  Ad::Creative creative2(2, 1, 0, 700, 200, 0.95, 1, "MSG1-2", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative2);
  
  Ad::Creative creative3(3, 1, 0, 700, 200, 0.80, 1, "MSG2-1", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG2, creative3);
  
  Ad::Creative creative4(4, 1, 0, 700, 200, 0.85, 1, "MSG2-2", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG2, creative4);
  
  Ad::Creative creative5(5, 1, 0, 700, 200, 0.65, 1, "MSA1-1", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA1, creative5);
  
  Ad::Creative creative6(6, 1, 0, 700, 200, 0.70, 1, "MSA1-2", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA1, creative6);
  
  Ad::Creative creative7(7, 1, 0, 700, 200, 0.76, 1, "MSA2-1", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA2, creative7);
  
  Ad::Creative creative8(8, 1, 0, 700, 200, 0.75, 1, "MSA2-2", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA2, creative8);

  selector.finalize();

  Ad::SelectionContext context(
    Ad::PI_DESK_PAPER,
    7 * ((unsigned long long)RAND_MAX + 1) / 10 + 1);

  context.add_message_category("/Science/Biology/Botany/");
  context.add_message_category("/Science/Biology/Zoology");

  context.slots.push_back(Ad::SI_DESK_PAPER_MSG1);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSG2);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSA1);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSA2);

  Ad::SelectionResult result;

  ACE_High_Res_Timer timer;
  timer.start();
  
  selector.select(context, result);

  timer.stop();
  ACE_Time_Value tm;
  timer.elapsed_time(tm);          
  
  Ad::SelectionResult etalon_result;
  
  etalon_result.ads.push_back(Ad::Selection(creative0.id,
                                            Ad::SI_DESK_PAPER_MSG1,
                                            creative0.width,
                                            creative0.height,
                                            creative0.text.c_str(),
                                            Ad::CI_DIRECT));

  etalon_result.ads.push_back(Ad::Selection(creative4.id,
                                            Ad::SI_DESK_PAPER_MSG2,
                                            creative4.width,
                                            creative4.height,
                                            creative4.text.c_str(),
                                            Ad::CI_DIRECT));

  etalon_result.ads.push_back(Ad::Selection(creative7.id,
                                            Ad::SI_DESK_PAPER_MSA2,
                                            creative7.width,
                                            creative7.height,
                                            creative7.text.c_str(),
                                            Ad::CI_DIRECT));

  if(result.ads != etalon_result.ads)
  {
    std::ostringstream ostr;
    ostr << "Application::test4: failed. Result:\n";
    result.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }

  return tm;
}

ACE_Time_Value
Application::test5() throw(Exception, El::Exception)
{
  Ad::Selector selector(10, 10, 10, 10, 0);
  selector.pages[Ad::PI_DESK_PAPER].max_ad_num = 4;
  
  selector.pages[Ad::PI_DESK_PAPER].add_max_ad_num(1, 3);
  
  Ad::Condition condition0(0, 10, 6, 7, 0, 0);
  condition0.add_message_category_exclusion("/Science/");
  
  Ad::Condition condition1(1, 10, 3, 5, 0, 0);
  Ad::Condition condition2(2, 0, 0, 0, 0, 0);
  condition2.add_message_category("/Science/Biology/");
  condition2.add_message_category("/Science/Chemistry/");

  condition2.search_engines.insert("yandex");
  condition2.search_engines.insert("google");

  Ad::Condition condition3(3, 0, 0, 0, 0, 0);
  condition3.search_engine_exclusions.insert("google");

  Ad::Condition condition4(4, 0, 0, 0, 0, 0);
  condition4.crawlers.insert("adsense");
  condition4.crawlers.insert("googlebot");

  Ad::Condition condition5(5, 0, 0, 0, 0, 0);
  condition5.crawler_exclusions.insert("googlebot");
  condition5.crawler_exclusions.insert("yandex");
  
  selector.conditions[0] = condition0;
  selector.conditions[1] = condition1;
  selector.conditions[2] = condition2;
  selector.conditions[3] = condition3;
  selector.conditions[4] = condition4;
  selector.conditions[5] = condition5;
  
  Ad::Creative creative0(0, 1, 0, 700, 200, 0.97, 1, "MSG1-0", Ad::CI_DIRECT);
  creative0.conditions.push_back(&selector.conditions[0]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative0);
  
  Ad::Creative creative1(1, 1, 0, 700, 200, 0.99, 1, "MSG1-1", Ad::CI_DIRECT);
  creative1.conditions.push_back(&selector.conditions[1]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative1);
  
  Ad::Creative creative2(2, 1, 0, 700, 200, 0.95, 1, "MSG1-2", Ad::CI_DIRECT);
  creative2.conditions.push_back(&selector.conditions[2]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative2);

  Ad::Creative creative2A(22, 1, 0, 700, 200, 0.96, 1,"MSG1-2A",Ad::CI_DIRECT);
  creative2A.conditions.push_back(&selector.conditions[3]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative2A);
  
  Ad::Creative creative3(3, 1, 0, 700, 200, 0.80, 1, "MSG2-1", Ad::CI_DIRECT);
  creative3.conditions.push_back(&selector.conditions[4]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG2, creative3);
  
  Ad::Creative creative4(4, 1, 0, 700, 200, 0.85, 1, "MSG2-2", Ad::CI_DIRECT);
  creative4.conditions.push_back(&selector.conditions[5]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG2, creative4);
  
  Ad::Creative creative5(5, 1, 0, 700, 200, 0.65, 1, "MSA1-1", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA1, creative5);
  
  Ad::Creative creative6(6, 1, 0, 700, 200, 0.70, 1, "MSA1-2", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA1, creative6);
  
  Ad::Creative creative7(7, 1, 0, 700, 200, 0.76, 1, "MSA2-1", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA2, creative7);
  
  Ad::Creative creative8(8, 1, 0, 700, 200, 0.75, 1, "MSA2-2", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA2, creative8);

  Ad::Creative creative9(9, 1, 0, 700, 200, 0.75, 1, "RTB1", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_RTB1, creative9);

  selector.finalize();

  Ad::SelectionContext context(
    Ad::PI_DESK_PAPER,
    7 * ((unsigned long long)RAND_MAX + 1) / 10 + 1);
  
  context.add_message_category("/Science/Biology/Botany/");
  context.add_message_category("/Science/Biology/Zoology/");
  context.search_engine = "google";
  context.crawler = "googlebot";

  context.slots.push_back(Ad::SI_DESK_PAPER_MSG1);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSG2);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSA1);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSA2);
  context.slots.push_back(Ad::SI_DESK_PAPER_RTB1);

  Ad::SelectionResult result;

  ACE_High_Res_Timer timer;
  timer.start();
  
  selector.select(context, result);

  timer.stop();
  ACE_Time_Value tm;
  timer.elapsed_time(tm);          
  
  Ad::SelectionResult etalon_result;
  
  etalon_result.ads.push_back(Ad::Selection(creative2.id,
                                            Ad::SI_DESK_PAPER_MSG1,
                                            creative2.width,
                                            creative2.height,
                                            creative2.text.c_str(),
                                            Ad::CI_DIRECT));

  etalon_result.ads.push_back(Ad::Selection(creative3.id,
                                            Ad::SI_DESK_PAPER_MSG2,
                                            creative3.width,
                                            creative3.height,
                                            creative3.text.c_str(),
                                            Ad::CI_DIRECT));

  etalon_result.ads.push_back(Ad::Selection(creative7.id,
                                            Ad::SI_DESK_PAPER_MSA2,
                                            creative7.width,
                                            creative7.height,
                                            creative7.text.c_str(),
                                            Ad::CI_DIRECT));

  if(result.ads != etalon_result.ads)
  {
    std::ostringstream ostr;
    ostr << "Application::test5: failed. Result:\n";
    result.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }

  return tm;
}

ACE_Time_Value
Application::test6() throw(Exception, El::Exception)
{
  Ad::Selector selector(10, 10, 10, 10, 0);
  selector.pages[Ad::PI_DESK_PAPER].max_ad_num = 5;
  selector.pages[Ad::PI_DESK_PAPER].add_max_ad_num(1, 5);
  selector.pages[Ad::PI_DESK_PAPER].add_max_ad_num(1, 2, 4);
  
  Ad::Condition condition0(0, 10, 6, 7, 0, 0);
  condition0.add_message_category_exclusion("/Science/");
  
  Ad::Condition condition1(1, 10, 3, 5, 0, 0);
  
  Ad::Condition condition2(2, 0, 0, 0, 0, 0);
  condition2.add_message_category("/Science/Biology/");
  condition2.add_message_category("/Science/Chemistry/");
  condition2.search_engines.insert("yandex");
  condition2.search_engines.insert("google");

  Ad::Condition condition3(3, 0, 0, 0, 0, 0);
  condition3.search_engine_exclusions.insert("google");

  Ad::Condition condition4(4, 0, 0, 0, 0, 0);
  condition4.crawlers.insert("adsense");
  condition4.crawlers.insert("googlebot");

  Ad::Condition condition5(5, 0, 0, 0, 0, 0);
  condition5.crawler_exclusions.insert("googlebot");
  condition5.crawler_exclusions.insert("yandex");
  
  selector.conditions[0] = condition0;
  selector.conditions[1] = condition1;
  selector.conditions[2] = condition2;
  selector.conditions[3] = condition3;
  selector.conditions[4] = condition4;
  selector.conditions[5] = condition5;
  
  Ad::Creative creative0(0, 1, 0, 700, 200, 0.97, 1, "MSG1-0", Ad::CI_DIRECT);
  creative0.conditions.push_back(&selector.conditions[0]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative0);
  
  Ad::Creative creative1(1, 1, 0, 700, 200, 0.99, 1, "MSG1-1", Ad::CI_DIRECT);
  creative1.conditions.push_back(&selector.conditions[1]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative1);
  
  Ad::Creative creative2(2, 1, 0, 700, 200, 0.95, 1, "MSG1-2", Ad::CI_DIRECT);
  creative2.conditions.push_back(&selector.conditions[2]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative2);

  Ad::Creative creative2A(22, 1, 0, 700, 200, 0.96, 1,"MSG1-2A",Ad::CI_DIRECT);
  creative2A.conditions.push_back(&selector.conditions[3]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative2A);
  
  Ad::Creative creative3(3, 1, 0, 700, 200, 0.80, 1, "MSG2-1", Ad::CI_DIRECT);
  creative3.conditions.push_back(&selector.conditions[4]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG2, creative3);
  
  Ad::Creative creative4(4, 1, 0, 700, 200, 0.85, 1, "MSG2-2", Ad::CI_DIRECT);
  creative4.conditions.push_back(&selector.conditions[5]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG2, creative4);
  
  Ad::Creative creative5(5, 1, 0, 700, 200, 0.65, 2, "MSA1-1", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA1, creative5);
  
  Ad::Creative creative6(6, 1, 0, 700, 200, 0.70, 2, "MSA1-2", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA1, creative6);
  
  Ad::Creative creative7(7, 1, 0, 700, 200, 0.76, 1, "MSA2-1", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA2, creative7);
  
  Ad::Creative creative8(8, 1, 0, 700, 200, 0.75, 1, "MSA2-2", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA2, creative8);

  Ad::Creative creative9(9, 1, 0, 700, 200, 0.75, 2, "RTB1", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_RTB1, creative9);

  selector.finalize();

  Ad::SelectionContext context(
    Ad::PI_DESK_PAPER,
    7 * ((unsigned long long)RAND_MAX + 1) / 10 + 1);
  
  context.add_message_category("/Science/Biology/Botany/");
  context.add_message_category("/Science/Biology/Zoology/");
  context.search_engine = "google";
  context.crawler = "googlebot";

  context.slots.push_back(Ad::SI_DESK_PAPER_MSG1);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSG2);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSA1);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSA2);
  context.slots.push_back(Ad::SI_DESK_PAPER_RTB1);

  Ad::SelectionResult result;

  ACE_High_Res_Timer timer;
  timer.start();
  
  selector.select(context, result);

  timer.stop();
  ACE_Time_Value tm;
  timer.elapsed_time(tm);          
  
  Ad::SelectionResult etalon_result;
  
  etalon_result.ads.push_back(Ad::Selection(creative2.id,
                                            Ad::SI_DESK_PAPER_MSG1,
                                            creative2.width,
                                            creative2.height,
                                            creative2.text.c_str(),
                                            Ad::CI_DIRECT));

  etalon_result.ads.push_back(Ad::Selection(creative3.id,
                                            Ad::SI_DESK_PAPER_MSG2,
                                            creative3.width,
                                            creative3.height,
                                            creative3.text.c_str(),
                                            Ad::CI_DIRECT));

  etalon_result.ads.push_back(Ad::Selection(creative7.id,
                                            Ad::SI_DESK_PAPER_MSA2,
                                            creative7.width,
                                            creative7.height,
                                            creative7.text.c_str(),
                                            Ad::CI_DIRECT));

  etalon_result.ads.push_back(Ad::Selection(creative9.id,
                                            Ad::SI_DESK_PAPER_RTB1,
                                            creative9.width,
                                            creative9.height,
                                            creative9.text.c_str(),
                                            Ad::CI_DIRECT));

  if(result.ads != etalon_result.ads)
  {
    std::ostringstream ostr;
    ostr << "Application::test6: failed. Result:\n";
    result.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }

  return tm;
}

ACE_Time_Value
Application::test7() throw(Exception, El::Exception)
{
  Ad::Selector selector(10, 10, 10, 10, 0);
  selector.pages[Ad::PI_DESK_PAPER].max_ad_num = 5;
  selector.pages[Ad::PI_DESK_PAPER].add_max_ad_num(1, 5);
  selector.pages[Ad::PI_DESK_PAPER].add_max_ad_num(1, 2, 4);
  
  Ad::Condition condition0(0, 10, 6, 7, 0, 0);
  condition0.add_message_category_exclusion("/Science/");
  
  Ad::Condition condition1(1, 10, 3, 5, 0, 0);
  Ad::Condition condition2(2, 0, 0, 0, 0, 0);
  condition2.add_message_category("/Science/Biology/");
  condition2.add_message_category("/Science/Chemistry/");
  condition2.search_engines.insert("yandex");
  condition2.search_engines.insert("google");

  Ad::Condition condition3(3, 0, 0, 0, 0, 0);
  condition3.search_engine_exclusions.insert("google");

  Ad::Condition condition4(4, 0, 0, 0, 0, 0);
  condition4.crawlers.insert("adsense");
  condition4.crawlers.insert("googlebot");

  Ad::Condition condition5(5, 0, 0, 0, 0, 0);
  condition5.crawler_exclusions.insert("googlebot");
  condition5.crawler_exclusions.insert("yandex");

  Ad::Condition condition6(6, 0, 0, 0, 0, 0);
  condition6.languages.insert(El::Lang("eng"));
  
  Ad::Condition condition7(7, 0, 0, 0, 0, 0);
  condition7.languages.insert(El::Lang("rus"));
  
  Ad::Condition condition8(8, 0, 0, 0, 0, 0);
  condition8.language_exclusions.insert(El::Lang("eng"));
  
  selector.conditions[0] = condition0;
  selector.conditions[1] = condition1;
  selector.conditions[2] = condition2;
  selector.conditions[3] = condition3;
  selector.conditions[4] = condition4;
  selector.conditions[5] = condition5;
  selector.conditions[6] = condition6;
  selector.conditions[7] = condition7;
  selector.conditions[8] = condition8;
  
  Ad::Creative creative0(0, 1, 0, 700, 200, 0.97, 1, "MSG1-0", Ad::CI_DIRECT);
  creative0.conditions.push_back(&selector.conditions[0]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative0);
  
  Ad::Creative creative1(1, 1, 0, 700, 200, 0.99, 1, "MSG1-1", Ad::CI_DIRECT);
  creative1.conditions.push_back(&selector.conditions[1]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative1);
  
  Ad::Creative creative2(2, 1, 0, 700, 200, 0.95, 1, "MSG1-2", Ad::CI_DIRECT);
  creative2.conditions.push_back(&selector.conditions[2]);
  creative2.conditions.push_back(&selector.conditions[7]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative2);

  Ad::Creative creative2A(22, 1, 0, 700, 200, 0.96, 1,"MSG1-2A",Ad::CI_DIRECT);
  creative2A.conditions.push_back(&selector.conditions[3]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative2A);
  
  Ad::Creative creative3(3, 1, 0, 700, 200, 0.80, 1, "MSG2-1", Ad::CI_DIRECT);
  creative3.conditions.push_back(&selector.conditions[4]);
  creative3.conditions.push_back(&selector.conditions[6]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG2, creative3);
  
  Ad::Creative creative4(4, 1, 0, 700, 200, 0.85, 1, "MSG2-2", Ad::CI_DIRECT);
  creative4.conditions.push_back(&selector.conditions[5]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG2, creative4);
  
  Ad::Creative creative5(5, 1, 0, 700, 200, 0.65, 2, "MSA1-1", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA1, creative5);
  
  Ad::Creative creative6(6, 1, 0, 700, 200, 0.70, 2, "MSA1-2", Ad::CI_DIRECT);
  creative6.conditions.push_back(&selector.conditions[8]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA1, creative6);
  
  Ad::Creative creative7(7, 1, 0, 700, 200, 0.76, 1, "MSA2-1", Ad::CI_DIRECT);
  creative7.conditions.push_back(&selector.conditions[6]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA2, creative7);
  
  Ad::Creative creative8(8, 1, 0, 700, 200, 0.75, 1, "MSA2-2", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA2, creative8);

  Ad::Creative creative9(9, 1, 0, 700, 200, 0.75, 2, "RTB1", Ad::CI_DIRECT);
  creative9.conditions.push_back(&selector.conditions[6]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_RTB1, creative9);

  selector.finalize();

  Ad::SelectionContext context(
    Ad::PI_DESK_PAPER,
    7 * ((unsigned long long)RAND_MAX + 1) / 10 + 1);
  
  context.add_message_category("/Science/Biology/Botany/");
  context.add_message_category("/Science/Biology/Zoology/");
  context.search_engine = "google";
  context.crawler = "googlebot";
  context.language = El::Lang("eng");

  context.slots.push_back(Ad::SI_DESK_PAPER_MSG1);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSG2);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSA1);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSA2);
  context.slots.push_back(Ad::SI_DESK_PAPER_RTB1);

  Ad::SelectionResult result;

  ACE_High_Res_Timer timer;
  timer.start();
  
  selector.select(context, result);

  timer.stop();
  ACE_Time_Value tm;
  timer.elapsed_time(tm);          
  
  Ad::SelectionResult etalon_result;
  
  etalon_result.ads.push_back(Ad::Selection(creative3.id,
                                            Ad::SI_DESK_PAPER_MSG2,
                                            creative3.width,
                                            creative3.height,
                                            creative3.text.c_str(),
                                            Ad::CI_DIRECT));

  etalon_result.ads.push_back(Ad::Selection(creative5.id,
                                            Ad::SI_DESK_PAPER_MSA1,
                                            creative5.width,
                                            creative5.height,
                                            creative5.text.c_str(),
                                            Ad::CI_DIRECT));

  etalon_result.ads.push_back(Ad::Selection(creative7.id,
                                            Ad::SI_DESK_PAPER_MSA2,
                                            creative7.width,
                                            creative7.height,
                                            creative7.text.c_str(),
                                            Ad::CI_DIRECT));

  etalon_result.ads.push_back(Ad::Selection(creative9.id,
                                            Ad::SI_DESK_PAPER_RTB1,
                                            creative9.width,
                                            creative9.height,
                                            creative9.text.c_str(),
                                            Ad::CI_DIRECT));

  if(result.ads != etalon_result.ads)
  {
    std::ostringstream ostr;
    ostr << "Application::test7: failed. Result:\n";
    result.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }

  return tm;
}

ACE_Time_Value
Application::test8() throw(Exception, El::Exception)
{
  Ad::Selector selector(10, 10, 10, 10, 0);
  selector.pages[Ad::PI_DESK_PAPER].max_ad_num = 5;
  selector.pages[Ad::PI_DESK_PAPER].add_max_ad_num(1, 5);
  selector.pages[Ad::PI_DESK_PAPER].add_max_ad_num(1, 2, 4);
  
  Ad::Condition condition0(0, 10, 6, 7, 0, 0);
  condition0.add_message_category_exclusion("/Science/");
  
  Ad::Condition condition1(1, 10, 3, 5, 0, 0);
  Ad::Condition condition2(2, 0, 0, 0, 0, 0);
  condition2.add_message_category("/Science/Biology/");
  condition2.add_message_category("/Science/Chemistry/");
  condition2.search_engines.insert("yandex");
  condition2.search_engines.insert("google");

  Ad::Condition condition3(3, 0, 0, 0, 0, 0);
  condition3.search_engine_exclusions.insert("google");

  Ad::Condition condition4(4, 0, 0, 0, 0, 0);
  condition4.crawlers.insert("adsense");
  condition4.crawlers.insert("googlebot");

  Ad::Condition condition5(5, 0, 0, 0, 0, 0);
  condition5.crawler_exclusions.insert("googlebot");
  condition5.crawler_exclusions.insert("yandex");

  Ad::Condition condition6(6, 0, 0, 0, 0, 0);
  condition6.countries.insert(El::Country("usa"));
  
  Ad::Condition condition7(7, 0, 0, 0, 0, 0);
  condition7.countries.insert(El::Country("rus"));
  
  Ad::Condition condition8(8, 0, 0, 0, 0, 0);
  condition8.country_exclusions.insert(El::Country("usa"));
  
  selector.conditions[0] = condition0;
  selector.conditions[1] = condition1;
  selector.conditions[2] = condition2;
  selector.conditions[3] = condition3;
  selector.conditions[4] = condition4;
  selector.conditions[5] = condition5;
  selector.conditions[6] = condition6;
  selector.conditions[7] = condition7;
  selector.conditions[8] = condition8;
  
  Ad::Creative creative0(0, 1, 0, 700, 200, 0.97, 1, "MSG1-0", Ad::CI_DIRECT);
  creative0.conditions.push_back(&selector.conditions[0]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative0);
  
  Ad::Creative creative1(1, 1, 0, 700, 200, 0.99, 1, "MSG1-1", Ad::CI_DIRECT);
  creative1.conditions.push_back(&selector.conditions[1]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative1);
  
  Ad::Creative creative2(2, 1, 0, 700, 200, 0.95, 1, "MSG1-2", Ad::CI_DIRECT);
  creative2.conditions.push_back(&selector.conditions[2]);
  creative2.conditions.push_back(&selector.conditions[7]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative2);

  Ad::Creative creative2A(22, 1, 0, 700, 200, 0.96, 1,"MSG1-2A",Ad::CI_DIRECT);
  creative2A.conditions.push_back(&selector.conditions[3]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative2A);
  
  Ad::Creative creative3(3, 1, 0, 700, 200, 0.80, 1, "MSG2-1", Ad::CI_DIRECT);
  creative3.conditions.push_back(&selector.conditions[4]);
  creative3.conditions.push_back(&selector.conditions[6]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG2, creative3);
  
  Ad::Creative creative4(4, 1, 0, 700, 200, 0.85, 1, "MSG2-2", Ad::CI_DIRECT);
  creative4.conditions.push_back(&selector.conditions[5]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG2, creative4);
  
  Ad::Creative creative5(5, 1, 0, 700, 200, 0.65, 2, "MSA1-1", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA1, creative5);
  
  Ad::Creative creative6(6, 1, 0, 700, 200, 0.70, 2, "MSA1-2", Ad::CI_DIRECT);
  creative6.conditions.push_back(&selector.conditions[8]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA1, creative6);
  
  Ad::Creative creative7(7, 1, 0, 700, 200, 0.76, 1, "MSA2-1", Ad::CI_DIRECT);
  creative7.conditions.push_back(&selector.conditions[6]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA2, creative7);
  
  Ad::Creative creative8(8, 1, 0, 700, 200, 0.75, 1, "MSA2-2", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA2, creative8);

  Ad::Creative creative9(9, 1, 0, 700, 200, 0.75, 2, "RTB1", Ad::CI_DIRECT);
  creative9.conditions.push_back(&selector.conditions[6]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_RTB1, creative9);

  selector.finalize();

  Ad::SelectionContext context(
    Ad::PI_DESK_PAPER,
    7 * ((unsigned long long)RAND_MAX + 1) / 10 + 1);
  
  context.add_message_category("/Science/Biology/Botany/");
  context.add_message_category("/Science/Biology/Zoology/");
  context.search_engine = "google";
  context.crawler = "googlebot";
  context.country = El::Country("usa");

  context.slots.push_back(Ad::SI_DESK_PAPER_MSG1);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSG2);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSA1);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSA2);
  context.slots.push_back(Ad::SI_DESK_PAPER_RTB1);

  Ad::SelectionResult result;

  ACE_High_Res_Timer timer;
  timer.start();
  
  selector.select(context, result);

  timer.stop();
  ACE_Time_Value tm;
  timer.elapsed_time(tm);          
  
  Ad::SelectionResult etalon_result;
  
  etalon_result.ads.push_back(Ad::Selection(creative3.id,
                                            Ad::SI_DESK_PAPER_MSG2,
                                            creative3.width,
                                            creative3.height,
                                            creative3.text.c_str(),
                                            Ad::CI_DIRECT));

  etalon_result.ads.push_back(Ad::Selection(creative5.id,
                                            Ad::SI_DESK_PAPER_MSA1,
                                            creative5.width,
                                            creative5.height,
                                            creative5.text.c_str(),
                                            Ad::CI_DIRECT));

  etalon_result.ads.push_back(Ad::Selection(creative7.id,
                                            Ad::SI_DESK_PAPER_MSA2,
                                            creative7.width,
                                            creative7.height,
                                            creative7.text.c_str(),
                                            Ad::CI_DIRECT));

  etalon_result.ads.push_back(Ad::Selection(creative9.id,
                                            Ad::SI_DESK_PAPER_RTB1,
                                            creative9.width,
                                            creative9.height,
                                            creative9.text.c_str(),
                                            Ad::CI_DIRECT));

  if(result.ads != etalon_result.ads)
  {
    std::ostringstream ostr;
    ostr << "Application::test8: failed. Result:\n";
    result.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }

  return tm;
}

ACE_Time_Value
Application::test9() throw(Exception, El::Exception)
{
  Ad::Selector selector(10, 10, 10, 10, 0);
  selector.pages[Ad::PI_DESK_PAPER].max_ad_num = 5;
  selector.pages[Ad::PI_DESK_PAPER].add_max_ad_num(1, 5);
  selector.pages[Ad::PI_DESK_PAPER].add_max_ad_num(1, 2, 4);
  
  Ad::Condition condition0(0, 10, 6, 7, 0, 0);
  condition0.add_message_category_exclusion("/Science/");
  
  Ad::Condition condition1(1, 10, 3, 5, 0, 0);
  Ad::Condition condition2(2, 0, 0, 0, 0, 0);
  condition2.add_message_category("/Science/Biology/");
  condition2.add_message_category("/Science/Chemistry/");
  condition2.search_engines.insert("yandex");
  condition2.search_engines.insert("google");

  Ad::Condition condition3(3, 0, 0, 0, 0, 0);
  condition3.search_engine_exclusions.insert("google");

  Ad::Condition condition4(4, 0, 0, 0, 0, 0);
  condition4.ip_masks.push_back(El::Net::IpMask("192.168.3.110/21"));

  Ad::Condition condition5(5, 0, 0, 0, 0, 0);
  condition5.ip_mask_exclusions.push_back(El::Net::IpMask("192.168.3.110/21"));
  
  Ad::Condition condition6(6, 0, 0, 0, 0, 0);
  condition6.countries.insert(El::Country("usa"));
  
  Ad::Condition condition7(7, 0, 0, 0, 0, 0);
  condition7.countries.insert(El::Country("rus"));
  
  Ad::Condition condition8(8, 0, 0, 0, 0, 0);
  condition8.country_exclusions.insert(El::Country("usa"));
  
  selector.conditions[0] = condition0;
  selector.conditions[1] = condition1;
  selector.conditions[2] = condition2;
  selector.conditions[3] = condition3;
  selector.conditions[4] = condition4;
  selector.conditions[5] = condition5;
  selector.conditions[6] = condition6;
  selector.conditions[7] = condition7;
  selector.conditions[8] = condition8;
  
  Ad::Creative creative0(0, 1, 0, 700, 200, 0.97, 1, "MSG1-0", Ad::CI_DIRECT);
  creative0.conditions.push_back(&selector.conditions[0]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative0);
  
  Ad::Creative creative1(1, 1, 0, 700, 200, 0.99, 1, "MSG1-1", Ad::CI_DIRECT);
  creative1.conditions.push_back(&selector.conditions[1]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative1);
  
  Ad::Creative creative2(2, 1, 0, 700, 200, 0.95, 1, "MSG1-2", Ad::CI_DIRECT);
  creative2.conditions.push_back(&selector.conditions[2]);
  creative2.conditions.push_back(&selector.conditions[7]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative2);

  Ad::Creative creative2A(22, 1, 0, 700, 200, 0.96, 1,"MSG1-2A",Ad::CI_DIRECT);
  creative2A.conditions.push_back(&selector.conditions[3]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative2A);
  
  Ad::Creative creative3(3, 1, 0, 700, 200, 0.80, 1, "MSG2-1", Ad::CI_DIRECT);
  creative3.conditions.push_back(&selector.conditions[4]);
  creative3.conditions.push_back(&selector.conditions[6]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG2, creative3);
  
  Ad::Creative creative4(4, 1, 0, 700, 200, 0.85, 1, "MSG2-2", Ad::CI_DIRECT);
  creative4.conditions.push_back(&selector.conditions[5]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG2, creative4);
  
  Ad::Creative creative5(5, 1, 0, 700, 200, 0.65, 2, "MSA1-1", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA1, creative5);
  
  Ad::Creative creative6(6, 1, 0, 700, 200, 0.70, 2, "MSA1-2", Ad::CI_DIRECT);
  creative6.conditions.push_back(&selector.conditions[8]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA1, creative6);
  
  Ad::Creative creative7(7, 1, 0, 700, 200, 0.76, 1, "MSA2-1", Ad::CI_DIRECT);
  creative7.conditions.push_back(&selector.conditions[6]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA2, creative7);
  
  Ad::Creative creative8(8, 1, 0, 700, 200, 0.75, 1, "MSA2-2", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA2, creative8);

  Ad::Creative creative9(9, 1, 0, 700, 200, 0.75, 2, "RTB1", Ad::CI_DIRECT);
  creative9.conditions.push_back(&selector.conditions[6]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_RTB1, creative9);

  selector.finalize();

  Ad::SelectionContext context(
    Ad::PI_DESK_PAPER,
    7 * ((unsigned long long)RAND_MAX + 1) / 10 + 1);
  
  context.add_message_category("/Science/Biology/Botany/");
  context.add_message_category("/Science/Biology/Zoology/");
  
  context.search_engine = "google";

  El::Net::ip("192.168.3.110", &context.ip);
  
  context.country = El::Country("usa");

  context.slots.push_back(Ad::SI_DESK_PAPER_MSG1);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSG2);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSA1);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSA2);
  context.slots.push_back(Ad::SI_DESK_PAPER_RTB1);

  Ad::SelectionResult result;

  ACE_High_Res_Timer timer;
  timer.start();
  
  selector.select(context, result);

  timer.stop();
  ACE_Time_Value tm;
  timer.elapsed_time(tm);          
  
  Ad::SelectionResult etalon_result;
  
  etalon_result.ads.push_back(Ad::Selection(creative3.id,
                                            Ad::SI_DESK_PAPER_MSG2,
                                            creative3.width,
                                            creative3.height,
                                            creative3.text.c_str(),
                                            Ad::CI_DIRECT));

  etalon_result.ads.push_back(Ad::Selection(creative5.id,
                                            Ad::SI_DESK_PAPER_MSA1,
                                            creative5.width,
                                            creative5.height,
                                            creative5.text.c_str(),
                                            Ad::CI_DIRECT));

  etalon_result.ads.push_back(Ad::Selection(creative7.id,
                                            Ad::SI_DESK_PAPER_MSA2,
                                            creative7.width,
                                            creative7.height,
                                            creative7.text.c_str(),
                                            Ad::CI_DIRECT));

  etalon_result.ads.push_back(Ad::Selection(creative9.id,
                                            Ad::SI_DESK_PAPER_RTB1,
                                            creative9.width,
                                            creative9.height,
                                            creative9.text.c_str(),
                                            Ad::CI_DIRECT));

  if(result.ads != etalon_result.ads)
  {
    std::ostringstream ostr;
    ostr << "Application::test9: failed. Result:\n";
    result.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }

  return tm;
}

ACE_Time_Value
Application::test10() throw(Exception, El::Exception)
{
  Ad::Selector selector(10, 10, 10, 10, 0);
  selector.pages[Ad::PI_DESK_PAPER].max_ad_num = 5;
  selector.pages[Ad::PI_DESK_PAPER].add_max_ad_num(1, 5);
  selector.pages[Ad::PI_DESK_PAPER].add_max_ad_num(1, 2, 4);
  
  Ad::Condition condition0(0, 10, 6, 7, 0, 0);
  condition0.add_message_source_exclusion("co.uk");
  
  Ad::Condition condition1(1, 10, 3, 5, 0, 0);
  
  Ad::Condition condition2(2, 0, 0, 0, 0, 0);
  condition2.add_message_source("www.abc.co.uk/aa");
  condition2.add_message_source("www.google.com");
  
  condition2.search_engines.insert("yandex");
  condition2.search_engines.insert("google");

  Ad::Condition condition3(3, 0, 0, 0, 0, 0);
  condition3.search_engine_exclusions.insert("google");

  Ad::Condition condition4(4, 0, 0, 0, 0, 0);
  condition4.ip_masks.push_back(El::Net::IpMask("192.168.3.110/21"));

  Ad::Condition condition5(5, 0, 0, 0, 0, 0);
  condition5.ip_mask_exclusions.push_back(El::Net::IpMask("192.168.3.110/21"));
  
  Ad::Condition condition6(6, 0, 0, 0, 0, 0);
  condition6.countries.insert(El::Country("usa"));
  
  Ad::Condition condition7(7, 0, 0, 0, 0, 0);
  condition7.countries.insert(El::Country("rus"));
  
  Ad::Condition condition8(8, 0, 0, 0, 0, 0);
  condition8.country_exclusions.insert(El::Country("usa"));
  
  selector.conditions[0] = condition0;
  selector.conditions[1] = condition1;
  selector.conditions[2] = condition2;
  selector.conditions[3] = condition3;
  selector.conditions[4] = condition4;
  selector.conditions[5] = condition5;
  selector.conditions[6] = condition6;
  selector.conditions[7] = condition7;
  selector.conditions[8] = condition8;
  
  Ad::Creative creative0(0, 1, 0, 700, 200, 0.97, 1, "MSG1-0", Ad::CI_DIRECT);
  creative0.conditions.push_back(&selector.conditions[0]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative0);
  
  Ad::Creative creative1(1, 1, 0, 700, 200, 0.99, 1, "MSG1-1", Ad::CI_DIRECT);
  creative1.conditions.push_back(&selector.conditions[1]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative1);
  
  Ad::Creative creative2(2, 1, 0, 700, 200, 0.95, 1, "MSG1-2", Ad::CI_DIRECT);
  creative2.conditions.push_back(&selector.conditions[7]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative2);

  Ad::Creative creative2A(22, 1, 0, 700, 200, 0.96, 1,"MSG1-2A",Ad::CI_DIRECT);
  creative2A.conditions.push_back(&selector.conditions[3]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative2A);
  
  Ad::Creative creative3(3, 1, 0, 700, 200, 0.80, 1, "MSG2-1", Ad::CI_DIRECT);
  creative3.conditions.push_back(&selector.conditions[4]);
  creative3.conditions.push_back(&selector.conditions[6]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG2, creative3);
  
  Ad::Creative creative4(4, 1, 0, 700, 200, 0.85, 1, "MSG2-2", Ad::CI_DIRECT);
  creative4.conditions.push_back(&selector.conditions[5]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG2, creative4);
  
  Ad::Creative creative5(5, 1, 0, 700, 200, 0.65, 2, "MSA1-1", Ad::CI_DIRECT);
  creative5.conditions.push_back(&selector.conditions[2]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA1, creative5);
  
  Ad::Creative creative6(6, 1, 0, 700, 200, 0.70, 2, "MSA1-2", Ad::CI_DIRECT);
  creative6.conditions.push_back(&selector.conditions[8]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA1, creative6);
  
  Ad::Creative creative7(7, 1, 0, 700, 200, 0.76, 1, "MSA2-1", Ad::CI_DIRECT);
  creative7.conditions.push_back(&selector.conditions[6]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA2, creative7);
  
  Ad::Creative creative8(8, 1, 0, 700, 200, 0.75, 1, "MSA2-2", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA2, creative8);

  Ad::Creative creative9(9, 1, 0, 700, 200, 0.75, 2, "RTB1", Ad::CI_DIRECT);
  creative9.conditions.push_back(&selector.conditions[6]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_RTB1, creative9);

  selector.finalize();

  Ad::SelectionContext context(
    Ad::PI_DESK_PAPER,
    7 * ((unsigned long long)RAND_MAX + 1) / 10 + 1);
  
  context.add_message_source("www.abc.co.uk/aa/bb?z=1");
  context.add_message_source("yandex.ru");
  
  context.search_engine = "google";

  El::Net::ip("192.168.3.110", &context.ip);
  
  context.country = El::Country("usa");

  context.slots.push_back(Ad::SI_DESK_PAPER_MSG1);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSG2);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSA1);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSA2);
  context.slots.push_back(Ad::SI_DESK_PAPER_RTB1);

  Ad::SelectionResult result;

  ACE_High_Res_Timer timer;
  timer.start();
  
  selector.select(context, result);

  timer.stop();
  ACE_Time_Value tm;
  timer.elapsed_time(tm);          
  
  Ad::SelectionResult etalon_result;
  
  etalon_result.ads.push_back(Ad::Selection(creative3.id,
                                            Ad::SI_DESK_PAPER_MSG2,
                                            creative3.width,
                                            creative3.height,
                                            creative3.text.c_str(),
                                            Ad::CI_DIRECT));

  etalon_result.ads.push_back(Ad::Selection(creative5.id,
                                            Ad::SI_DESK_PAPER_MSA1,
                                            creative5.width,
                                            creative5.height,
                                            creative5.text.c_str(),
                                            Ad::CI_DIRECT));

  etalon_result.ads.push_back(Ad::Selection(creative7.id,
                                            Ad::SI_DESK_PAPER_MSA2,
                                            creative7.width,
                                            creative7.height,
                                            creative7.text.c_str(),
                                            Ad::CI_DIRECT));

  etalon_result.ads.push_back(Ad::Selection(creative9.id,
                                            Ad::SI_DESK_PAPER_RTB1,
                                            creative9.width,
                                            creative9.height,
                                            creative9.text.c_str(),
                                            Ad::CI_DIRECT));

  if(result.ads != etalon_result.ads)
  {
    std::ostringstream ostr;
    ostr << "Application::test10: failed. Result:\n";
    result.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }

  return tm;
}

ACE_Time_Value
Application::test11() throw(Exception, El::Exception)
{
  Ad::Selector selector(3, 10, 10, 10, 0);
  selector.pages[Ad::PI_DESK_PAPER].max_ad_num = 5;
  selector.pages[Ad::PI_DESK_PAPER].add_max_ad_num(1, 5);
  selector.pages[Ad::PI_DESK_PAPER].add_max_ad_num(1, 2, 4);
  
  Ad::Condition condition0(0, 10, 6, 7, 0, 0);
  condition0.add_message_source_exclusion("co.uk");
  
  Ad::Condition condition1(1, 10, 3, 5, 0, 0);
  
  Ad::Condition condition2(2, 0, 0, 0, 0, 0);
  condition2.add_message_source("www.abc.co.uk/aa");
  condition2.add_message_source("www.google.com");
  
  condition2.search_engines.insert("yandex");
  condition2.search_engines.insert("google");

  Ad::Condition condition3(3, 0, 0, 0, 0, 0);
  condition3.search_engine_exclusions.insert("google");

  Ad::Condition condition4(4, 0, 0, 0, 0, 0);
  condition4.ip_masks.push_back(El::Net::IpMask("192.168.3.110/21"));

  Ad::Condition condition5(5, 0, 0, 0, 0, 0);
  condition5.ip_mask_exclusions.push_back(El::Net::IpMask("192.168.3.110/21"));
  
  Ad::Condition condition6(6, 0, 0, 0, 0, 0);
  condition6.countries.insert(El::Country("usa"));
  
  Ad::Condition condition7(7, 0, 0, 0, 0, 0);
  condition7.countries.insert(El::Country("rus"));
  
  Ad::Condition condition8(8, 0, 0, 0, 0, 0);
  condition8.country_exclusions.insert(El::Country("usa"));

  Ad::Condition condition9(0, 0, 0, 0, 2, 0);
  
  selector.conditions[0] = condition0;
  selector.conditions[1] = condition1;
  selector.conditions[2] = condition2;
  selector.conditions[3] = condition3;
  selector.conditions[4] = condition4;
  selector.conditions[5] = condition5;
  selector.conditions[6] = condition6;
  selector.conditions[7] = condition7;
  selector.conditions[8] = condition8;
  selector.conditions[9] = condition9;
  
  Ad::Creative creative0(0, 1, 0, 700, 200, 0.97, 1, "MSG1-0", Ad::CI_DIRECT);
  creative0.conditions.push_back(&selector.conditions[0]);
  creative0.conditions.push_back(&selector.conditions[9]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative0);
  
  Ad::Creative creative1(1, 1, 0, 700, 200, 0.99, 1, "MSG1-1", Ad::CI_DIRECT);
  creative1.conditions.push_back(&selector.conditions[1]);
  creative1.conditions.push_back(&selector.conditions[9]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative1);
  
  Ad::Creative creative2(2, 1, 0, 700, 200, 0.95, 1, "MSG1-2", Ad::CI_DIRECT);
  creative2.conditions.push_back(&selector.conditions[7]);
  creative2.conditions.push_back(&selector.conditions[9]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative2);

  Ad::Creative creative2A(22, 1, 0, 700, 200, 0.96, 1,"MSG1-2A",Ad::CI_DIRECT);
  creative2A.conditions.push_back(&selector.conditions[3]);
  creative2A.conditions.push_back(&selector.conditions[9]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative2A);
  
  Ad::Creative creative3(3, 1, 0, 700, 200, 0.80, 1, "MSG2-1", Ad::CI_DIRECT);
  creative3.conditions.push_back(&selector.conditions[4]);
  creative3.conditions.push_back(&selector.conditions[6]);
  creative3.conditions.push_back(&selector.conditions[9]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG2, creative3);
  
  Ad::Creative creative4(4, 2, 0, 700, 200, 0.85, 1, "MSG2-2", Ad::CI_DIRECT);
  creative4.conditions.push_back(&selector.conditions[5]);
  creative4.conditions.push_back(&selector.conditions[9]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG2, creative4);
  
  Ad::Creative creative5(5, 2, 0, 700, 200, 0.65, 2, "MSA1-1", Ad::CI_DIRECT);
  creative5.conditions.push_back(&selector.conditions[2]);
  creative5.conditions.push_back(&selector.conditions[9]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA1, creative5);
  
  Ad::Creative creative6(6, 2, 0, 700, 200, 0.70, 2, "MSA1-2", Ad::CI_DIRECT);
  creative6.conditions.push_back(&selector.conditions[8]);
  creative6.conditions.push_back(&selector.conditions[9]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA1, creative6);
  
  Ad::Creative creative7(7, 3, 0, 700, 200, 0.76, 1, "MSA2-1", Ad::CI_DIRECT);
  creative7.conditions.push_back(&selector.conditions[6]);
  creative7.conditions.push_back(&selector.conditions[9]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA2, creative7);
  
  Ad::Creative creative8(8, 3, 0, 700, 200, 0.75, 1, "MSA2-2", Ad::CI_DIRECT);
  creative8.conditions.push_back(&selector.conditions[9]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA2, creative8);

  Ad::Creative creative9(9, 4, 0, 700, 200, 0.75, 2, "RTB1", Ad::CI_DIRECT);
  creative9.conditions.push_back(&selector.conditions[6]);
  creative9.conditions.push_back(&selector.conditions[9]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_RTB1, creative9);

  selector.finalize();

  Ad::SelectionContext context(
    Ad::PI_DESK_PAPER,
    7 * ((unsigned long long)RAND_MAX + 1) / 10 + 1);
  
  context.add_message_source("www.abc.co.uk/aa/bb?z=1");
  context.add_message_source("yandex.ru");
  
  context.search_engine = "google";

  El::Net::ip("192.168.3.110", &context.ip);
  
  context.country = El::Country("usa");

  context.slots.push_back(Ad::SI_DESK_PAPER_MSG1);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSG2);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSA1);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSA2);
  context.slots.push_back(Ad::SI_DESK_PAPER_RTB1);

  Ad::SelectionResult result;

  ACE_High_Res_Timer timer;
  timer.start();
  
  selector.select(context, result);

  timer.stop();
  ACE_Time_Value tm;
  timer.elapsed_time(tm);
  
  Ad::SelectionResult etalon_result;
  
  etalon_result.ads.push_back(Ad::Selection(creative3.id,
                                            Ad::SI_DESK_PAPER_MSG2,
                                            creative3.width,
                                            creative3.height,
                                            creative3.text.c_str(),
                                            Ad::CI_DIRECT));

  etalon_result.ads.push_back(Ad::Selection(creative5.id,
                                            Ad::SI_DESK_PAPER_MSA1,
                                            creative5.width,
                                            creative5.height,
                                            creative5.text.c_str(),
                                            Ad::CI_DIRECT));

  etalon_result.ads.push_back(Ad::Selection(creative7.id,
                                            Ad::SI_DESK_PAPER_MSA2,
                                            creative7.width,
                                            creative7.height,
                                            creative7.text.c_str(),
                                            Ad::CI_DIRECT));

  etalon_result.ads.push_back(Ad::Selection(creative9.id,
                                            Ad::SI_DESK_PAPER_RTB1,
                                            creative9.width,
                                            creative9.height,
                                            creative9.text.c_str(),
                                            Ad::CI_DIRECT));

  if(result.ads != etalon_result.ads)
  {
    std::ostringstream ostr;
    ostr << "Application::test11: failed. Result:\n";
    result.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }

  std::string str;
  result.ad_caps.to_string(str);
  context.ad_caps.from_string(str.c_str());
  
  selector.finalize();
  
  Ad::SelectionResult result2;
  ACE_OS::sleep(2);

  context.ad_caps.set_current_time();
  
  selector.select(context, result2);

  if(result2.ads != etalon_result.ads)
  {
    std::ostringstream ostr;
    ostr << "Application::test11: failed. Result 2:\n";
    result2.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }
  
  Ad::Creative creative10(10, 5, 0, 700, 200, 0.80, 1, "MSG2-1", Ad::CI_DIRECT);
  creative10.conditions.push_back(&selector.conditions[9]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG2, creative10);

  Ad::Creative creative11(11, 5, 0, 700, 200, 0.75, 2, "RTB1", Ad::CI_DIRECT);
  creative11.conditions.push_back(&selector.conditions[9]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_RTB1, creative11);
  
  selector.finalize();
  
  Ad::SelectionResult result3;

  result2.ad_caps.to_string(str);
  context.ad_caps.from_string(str.c_str());
  
  context.ad_caps.set_current_time();

  selector.select(context, result3);

  Ad::SelectionResult etalon_result3;
  
  etalon_result3.ads.push_back(Ad::Selection(creative10.id,
                                             Ad::SI_DESK_PAPER_MSG2,
                                             creative10.width,
                                             creative10.height,
                                             creative10.text.c_str(),
                                             Ad::CI_DIRECT));
  
  if(result3.ads != etalon_result3.ads)
  {
    std::ostringstream ostr;
    ostr << "Application::test11: failed. Result 3 :\n";
    result3.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result3.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }

  ACE_OS::sleep(2);
  
  Ad::Selector selector2(2, 10, 10, 10, 0);

  selector2.pages[Ad::PI_DESK_PAPER].max_ad_num = 5;
  selector2.conditions[0] = condition9;

  Ad::Creative creative12(10, 6, 0, 700, 200, 0.80, 1,"MSG2-1",Ad::CI_DIRECT);
  creative12.conditions.push_back(&selector2.conditions[0]);
  
  selector2.add_creative(Ad::PI_DESK_PAPER,
                         Ad::SI_DESK_PAPER_MSG2,
                         creative12);

  selector2.finalize();
  
  Ad::SelectionResult result4;

  result3.ad_caps.to_string(str);
  context.ad_caps.from_string(str.c_str());
  
  context.ad_caps.set_current_time();

  selector2.select(context, result4);

  Ad::SelectionResult etalon_result4;
  
  etalon_result4.ads.push_back(Ad::Selection(creative12.id,
                                             Ad::SI_DESK_PAPER_MSG2,
                                             creative12.width,
                                             creative12.height,
                                             creative12.text.c_str(),
                                             Ad::CI_DIRECT));
  
  if(result4.ads != etalon_result4.ads)
  {
    std::ostringstream ostr;
    ostr << "Application::test11: failed. Result 4 :\n";
    result4.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result4.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }

  Ad::Selector selector3(2, 1, 10, 10, 0);

  selector3.pages[Ad::PI_DESK_PAPER].max_ad_num = 5;
  selector3.conditions[0] = condition9;
  
  Ad::Creative creative13(11, 7, 0, 700, 200, 0.75, 2, "RTB1", Ad::CI_DIRECT);
  creative13.conditions.push_back(&selector3.conditions[0]);
  
  selector3.add_creative(Ad::PI_DESK_PAPER,
                         Ad::SI_DESK_PAPER_RTB1,
                         creative13);

  selector3.finalize();  
  
  Ad::SelectionResult result5;

  result4.ad_caps.to_string(str);
  context.ad_caps.from_string(str.c_str());
  
  ACE_OS::sleep(1);
  context.ad_caps.set_current_time();

  selector3.select(context, result5);
  
  Ad::SelectionResult etalon_result5;
  
  etalon_result5.ads.push_back(Ad::Selection(creative13.id,
                                             Ad::SI_DESK_PAPER_RTB1,
                                             creative13.width,
                                             creative13.height,
                                             creative13.text.c_str(),
                                             Ad::CI_DIRECT));

  if(result5.ads != etalon_result5.ads)
  {
    std::ostringstream ostr;
    ostr << "Application::test11: failed. Result 5 :\n";
    result5.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result5.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }
  
  if(result5.ad_caps.caps.size() != 1)
  {
    std::ostringstream ostr;
    ostr << "Application::test11: failed. "
         << result5.ad_caps.caps.size()
         << " freq caps instead of 1\n";

    throw Exception(ostr.str());
  }

  if(result5.ad_caps.caps.find(7) == result5.ad_caps.caps.end())
  {
    std::ostringstream ostr;
    ostr << "Application::test11: failed. Group 7 not found\n";

    throw Exception(ostr.str());
  }
  
  return tm;
}

ACE_Time_Value
Application::test12() throw(Exception, El::Exception)
{
  Ad::Selector selector(3, 10, 10, 10, 0);
  selector.pages[Ad::PI_DESK_PAPER].max_ad_num = 5;
  selector.pages[Ad::PI_DESK_PAPER].add_max_ad_num(1, 5);
  selector.pages[Ad::PI_DESK_PAPER].add_max_ad_num(1, 2, 4);
  
  Ad::Condition condition0(0, 10, 6, 7, 0, 0);
  condition0.add_message_source_exclusion("co.uk");
  
  Ad::Condition condition1(1, 10, 3, 5, 0, 0);
  
  Ad::Condition condition2(2, 0, 0, 0, 0, 0);
  condition2.add_message_source("www.abc.co.uk/aa");
  condition2.add_message_source("www.google.com");
  
  condition2.search_engines.insert("yandex");
  condition2.search_engines.insert("google");

  Ad::Condition condition3(3, 0, 0, 0, 0, 0);
  condition3.search_engine_exclusions.insert("google");

  Ad::Condition condition4(4, 0, 0, 0, 0, 0);
  condition4.ip_masks.push_back(El::Net::IpMask("192.168.3.110/21"));

  Ad::Condition condition5(5, 0, 0, 0, 0, 0);
  condition5.ip_mask_exclusions.push_back(El::Net::IpMask("192.168.3.110/21"));
  
  Ad::Condition condition6(6, 0, 0, 0, 0, 0);
  condition6.countries.insert(El::Country("usa"));
  
  Ad::Condition condition7(7, 0, 0, 0, 0, 0);
  condition7.countries.insert(El::Country("rus"));
  
  Ad::Condition condition8(8, 0, 0, 0, 0, 0);
  condition8.country_exclusions.insert(El::Country("usa"));

  Ad::Condition condition9(0, 0, 0, 0, 0, 2);
  
  selector.conditions[0] = condition0;
  selector.conditions[1] = condition1;
  selector.conditions[2] = condition2;
  selector.conditions[3] = condition3;
  selector.conditions[4] = condition4;
  selector.conditions[5] = condition5;
  selector.conditions[6] = condition6;
  selector.conditions[7] = condition7;
  selector.conditions[8] = condition8;
  selector.conditions[9] = condition9;
  
  Ad::Creative creative0(0, 1, 0, 700, 200, 0.97, 1, "MSG1-0", Ad::CI_DIRECT);
  creative0.conditions.push_back(&selector.conditions[0]);
  creative0.conditions.push_back(&selector.conditions[9]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative0);
  
  Ad::Creative creative1(1, 1, 0, 700, 200, 0.99, 1, "MSG1-1", Ad::CI_DIRECT);
  creative1.conditions.push_back(&selector.conditions[1]);
  creative1.conditions.push_back(&selector.conditions[9]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative1);
  
  Ad::Creative creative2(2, 1, 0, 700, 200, 0.95, 1, "MSG1-2", Ad::CI_DIRECT);
  creative2.conditions.push_back(&selector.conditions[7]);
  creative2.conditions.push_back(&selector.conditions[9]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative2);

  Ad::Creative creative2A(22, 1, 0, 700, 200, 0.96, 1,"MSG1-2A",Ad::CI_DIRECT);
  creative2A.conditions.push_back(&selector.conditions[3]);
  creative2A.conditions.push_back(&selector.conditions[9]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative2A);
  
  Ad::Creative creative3(3, 1, 0, 700, 200, 0.80, 1, "MSG2-1", Ad::CI_DIRECT);
  creative3.conditions.push_back(&selector.conditions[4]);
  creative3.conditions.push_back(&selector.conditions[6]);
  creative3.conditions.push_back(&selector.conditions[9]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG2, creative3);
  
  Ad::Creative creative4(4, 2, 0, 700, 200, 0.85, 1, "MSG2-2", Ad::CI_DIRECT);
  creative4.conditions.push_back(&selector.conditions[5]);
  creative4.conditions.push_back(&selector.conditions[9]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG2, creative4);
  
  Ad::Creative creative5(5, 2, 0, 700, 200, 0.65, 2, "MSA1-1", Ad::CI_DIRECT);
  creative5.conditions.push_back(&selector.conditions[2]);
  creative5.conditions.push_back(&selector.conditions[9]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA1, creative5);
  
  Ad::Creative creative6(6, 2, 0, 700, 200, 0.70, 2, "MSA1-2", Ad::CI_DIRECT);
  creative6.conditions.push_back(&selector.conditions[8]);
  creative6.conditions.push_back(&selector.conditions[9]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA1, creative6);
  
  Ad::Creative creative7(7, 3, 0, 700, 200, 0.76, 1, "MSA2-1", Ad::CI_DIRECT);
  creative7.conditions.push_back(&selector.conditions[6]);
  creative7.conditions.push_back(&selector.conditions[9]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA2, creative7);
  
  Ad::Creative creative8(8, 3, 0, 700, 200, 0.75, 1, "MSA2-2", Ad::CI_DIRECT);
  creative8.conditions.push_back(&selector.conditions[9]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA2, creative8);

  Ad::Creative creative9(9, 4, 0, 700, 200, 0.75, 2, "RTB1", Ad::CI_DIRECT);
  creative9.conditions.push_back(&selector.conditions[6]);
  creative9.conditions.push_back(&selector.conditions[9]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_RTB1, creative9);

  selector.finalize();

  Ad::SelectionContext context(
    Ad::PI_DESK_PAPER,
    7 * ((unsigned long long)RAND_MAX + 1) / 10 + 1);
  
  context.add_message_source("www.abc.co.uk/aa/bb?z=1");
  context.add_message_source("yandex.ru");
  
  context.search_engine = "google";

  El::Net::ip("192.168.3.110", &context.ip);
  
  context.country = El::Country("usa");

  context.slots.push_back(Ad::SI_DESK_PAPER_MSG1);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSG2);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSA1);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSA2);
  context.slots.push_back(Ad::SI_DESK_PAPER_RTB1);

  Ad::SelectionResult result;

  ACE_High_Res_Timer timer;
  timer.start();
  
  selector.select(context, result);

  timer.stop();
  ACE_Time_Value tm;
  timer.elapsed_time(tm);
  
  Ad::SelectionResult etalon_result;
  
  etalon_result.ads.push_back(Ad::Selection(creative3.id,
                                            Ad::SI_DESK_PAPER_MSG2,
                                            creative3.width,
                                            creative3.height,
                                            creative3.text.c_str(),
                                            Ad::CI_DIRECT));

  etalon_result.ads.push_back(Ad::Selection(creative5.id,
                                            Ad::SI_DESK_PAPER_MSA1,
                                            creative5.width,
                                            creative5.height,
                                            creative5.text.c_str(),
                                            Ad::CI_DIRECT));

  etalon_result.ads.push_back(Ad::Selection(creative7.id,
                                            Ad::SI_DESK_PAPER_MSA2,
                                            creative7.width,
                                            creative7.height,
                                            creative7.text.c_str(),
                                            Ad::CI_DIRECT));

  etalon_result.ads.push_back(Ad::Selection(creative9.id,
                                            Ad::SI_DESK_PAPER_RTB1,
                                            creative9.width,
                                            creative9.height,
                                            creative9.text.c_str(),
                                            Ad::CI_DIRECT));

  if(result.ads != etalon_result.ads)
  {
    std::ostringstream ostr;
    ostr << "Application::test12: failed. Result:\n";
    result.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }

  std::string str;
  result.ad_caps.to_string(str);
  context.ad_caps.from_string(str.c_str());
  
  selector.finalize();
  
  Ad::SelectionResult result2;

  context.ad_caps.set_current_time();
  
  selector.select(context, result2);

  if(result2.ads != etalon_result.ads)
  {
    std::ostringstream ostr;
    ostr << "Application::test12: failed. Result 2:\n";
    result2.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }

  Ad::Creative creative10(10, 5, 0, 700, 200, 0.80, 1,"MSG2-1",Ad::CI_DIRECT);
  creative10.conditions.push_back(&selector.conditions[9]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG2, creative10);

  Ad::Creative creative11(11, 5, 0, 700, 200, 0.75, 2, "RTB1", Ad::CI_DIRECT);
  creative11.conditions.push_back(&selector.conditions[9]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_RTB1, creative11);
  
  selector.finalize();
  
  Ad::SelectionResult result3;

  result2.ad_caps.to_string(str);
  context.ad_caps.from_string(str.c_str());
  
  context.ad_caps.set_current_time();

  selector.select(context, result3);

  Ad::SelectionResult etalon_result3;
  
  etalon_result3.ads.push_back(Ad::Selection(creative10.id,
                                             Ad::SI_DESK_PAPER_MSG2,
                                             creative10.width,
                                             creative10.height,
                                             creative10.text.c_str(),
                                             Ad::CI_DIRECT));
  
  if(result3.ads != etalon_result3.ads)
  {
    std::ostringstream ostr;
    ostr << "Application::test12: failed. Result 3 :\n";
    result3.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result3.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }

  Ad::Selector selector2(2, 10, 10, 10, 0);

  selector2.pages[Ad::PI_DESK_PAPER].max_ad_num = 5;
  selector2.conditions[0] = condition9;

  Ad::Creative creative12(10, 6, 0, 700, 200, 0.80, 1, "MSG2-1",Ad::CI_DIRECT);
  creative12.conditions.push_back(&selector2.conditions[0]);
  
  selector2.add_creative(Ad::PI_DESK_PAPER,
                         Ad::SI_DESK_PAPER_MSG2,
                         creative12);

  selector2.finalize();
  
  Ad::SelectionResult result4;

  result3.ad_caps.to_string(str);
  context.ad_caps.from_string(str.c_str());
  
  context.ad_caps.set_current_time();

  selector2.select(context, result4);

  Ad::SelectionResult etalon_result4;
  
  etalon_result4.ads.push_back(Ad::Selection(creative12.id,
                                             Ad::SI_DESK_PAPER_MSG2,
                                             creative12.width,
                                             creative12.height,
                                             creative12.text.c_str(),
                                             Ad::CI_DIRECT));
  
  if(result4.ads != etalon_result4.ads)
  {
    std::ostringstream ostr;
    ostr << "Application::test12: failed. Result 4 :\n";
    result4.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result4.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }

  Ad::Selector selector3(2, 1, 10, 10, 0);

  selector3.pages[Ad::PI_DESK_PAPER].max_ad_num = 5;
  selector3.conditions[0] = condition9;
  
  Ad::Creative creative13(11, 7, 0, 700, 200, 0.75, 2, "RTB1", Ad::CI_DIRECT);
  creative13.conditions.push_back(&selector3.conditions[0]);
  
  selector3.add_creative(Ad::PI_DESK_PAPER,
                         Ad::SI_DESK_PAPER_RTB1,
                         creative13);

  selector3.finalize();  
  
  Ad::SelectionResult result5;

  result4.ad_caps.to_string(str);
  context.ad_caps.from_string(str.c_str());
  
  ACE_OS::sleep(1);
  context.ad_caps.set_current_time();

  selector3.select(context, result5);
  
  Ad::SelectionResult etalon_result5;
  
  etalon_result5.ads.push_back(Ad::Selection(creative13.id,
                                             Ad::SI_DESK_PAPER_RTB1,
                                             creative13.width,
                                             creative13.height,
                                             creative13.text.c_str(),
                                             Ad::CI_DIRECT));

  if(result5.ads != etalon_result5.ads)
  {
    std::ostringstream ostr;
    ostr << "Application::test12: failed. Result 5 :\n";
    result5.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result5.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }
  
  if(result5.ad_caps.caps.size() != 1)
  {
    std::ostringstream ostr;
    ostr << "Application::test12: failed. "
         << result5.ad_caps.caps.size()
         << " freq caps instead of 1\n";

    throw Exception(ostr.str());
  }

  if(result5.ad_caps.caps.find(7) == result5.ad_caps.caps.end())
  {
    std::ostringstream ostr;
    ostr << "Application::test12: failed. Group 7 not found\n";

    throw Exception(ostr.str());
  }

  return tm;
}

ACE_Time_Value
Application::test13() throw(Exception, El::Exception)
{
  Ad::Selector selector(3, 10, 10, 10, 0);
  selector.pages[Ad::PI_DESK_PAPER].max_ad_num = 5;
  selector.pages[Ad::PI_DESK_PAPER].add_max_ad_num(1, 5);

  Ad::Condition condition0(1, 0, 0, 0, 0, 0);
  condition0.ip_masks.push_back(El::Net::IpMask("195.91.155.98/21"));

  selector.conditions[0] = condition0;

  Ad::Creative creative0(0, 1, 0, 700, 200, 0.97, 1, "MSG1", Ad::CI_DIRECT);
  creative0.conditions.push_back(&selector.conditions[0]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative0);

  selector.finalize();

  Ad::SelectionContext context(Ad::PI_DESK_PAPER, 10);
  
  context.slots.push_back(Ad::SI_DESK_PAPER_MSG1);  
  El::Net::ip("195.91.155.98", &context.ip);

  Ad::SelectionResult result;

  ACE_High_Res_Timer timer;
  timer.start();
  
  selector.select(context, result);

  timer.stop();
  ACE_Time_Value tm;
  timer.elapsed_time(tm);
  
  Ad::SelectionResult etalon_result;
  
  etalon_result.ads.push_back(Ad::Selection(creative0.id,
                                            Ad::SI_DESK_PAPER_MSG1,
                                            creative0.width,
                                            creative0.height,
                                            creative0.text.c_str(),
                                            Ad::CI_DIRECT));  
  if(result.ads != etalon_result.ads)
  {
    std::ostringstream ostr;
    ostr << "Application::test13: failed. Result:\n";
    result.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }

  return tm;
}

ACE_Time_Value
Application::test14() throw(Exception, El::Exception)
{
  Ad::Selector selector(3, 10, 10, 10, 0);
  selector.pages[Ad::PI_DESK_PAPER].max_ad_num = 5;
  selector.pages[Ad::PI_DESK_PAPER].add_max_ad_num(1, 5);

  Ad::Condition condition0(1, 0, 0, 0, 0, 0);
  condition0.ip_masks.push_back(El::Net::IpMask("185.91.155.98/21"));

  selector.conditions[0] = condition0;

  Ad::Creative creative0(0, 1, 0, 700, 200, 0.97, 1, "MSG1", Ad::CI_DIRECT);
  creative0.conditions.push_back(&selector.conditions[0]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative0);

  selector.finalize();

  Ad::SelectionContext context(Ad::PI_DESK_PAPER, 10);
  
  context.slots.push_back(Ad::SI_DESK_PAPER_MSG1);  
  El::Net::ip("195.91.155.98", &context.ip);

  Ad::SelectionResult result;

  ACE_High_Res_Timer timer;
  timer.start();
  
  selector.select(context, result);

  timer.stop();
  ACE_Time_Value tm;
  timer.elapsed_time(tm);
  
  Ad::SelectionResult etalon_result;
  
  if(result.ads != etalon_result.ads)
  {
    std::ostringstream ostr;
    ostr << "Application::test14: failed. Result:\n";
    result.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }

  return tm;
}

ACE_Time_Value
Application::test15() throw(Exception, El::Exception)
{
  Ad::Selector selector(10, 10, 10, 10, 0);
  selector.pages[Ad::PI_DESK_PAPER].max_ad_num = 4;
  
  selector.pages[Ad::PI_DESK_PAPER].add_max_ad_num(1, 3);
  
  Ad::Condition condition0(0, 10, 6, 7, 0, 0);
  condition0.add_tag_exclusion("at1");
  
  Ad::Condition condition1(1, 10, 3, 5, 0, 0);
  Ad::Condition condition2(2, 0, 0, 0, 0, 0);
  condition2.add_tag("at2");
  condition2.add_tag("at3");

  condition2.search_engines.insert("yandex");
  condition2.search_engines.insert("google");

  condition2.add_referer("http://newsfiber.com");
  condition2.add_referer("http://google.com");

  Ad::Condition condition3(3, 0, 0, 0, 0, 0);
  condition3.search_engine_exclusions.insert("google");

  Ad::Condition condition4(4, 0, 0, 0, 0, 0);
  condition4.crawlers.insert("adsense");
  condition4.crawlers.insert("googlebot");
  condition4.add_referer("http://www.abc.com/a/b");

  Ad::Condition condition5(5, 0, 0, 0, 0, 0);
  condition5.crawler_exclusions.insert("googlebot");
  condition5.crawler_exclusions.insert("yandex");

  Ad::Condition condition6(6, 0, 0, 0, 0, 0);
  condition6.add_referer_exclusion("http://www.newsfiber.com/p");
  
  Ad::Condition condition7(7, 0, 0, 0, 0, 0);
  condition7.add_referer("[ANY]");
  
  selector.conditions[0] = condition0;
  selector.conditions[1] = condition1;
  selector.conditions[2] = condition2;
  selector.conditions[3] = condition3;
  selector.conditions[4] = condition4;
  selector.conditions[5] = condition5;
  selector.conditions[6] = condition6;
  selector.conditions[7] = condition7;
  
  Ad::Creative creative0(0, 1, 0, 700, 200, 0.97, 1, "MSG1-0", Ad::CI_DIRECT);
  creative0.conditions.push_back(&selector.conditions[0]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative0);
  
  Ad::Creative creative1(1, 1, 0, 700, 200, 0.99, 1, "MSG1-1", Ad::CI_DIRECT);
  creative1.conditions.push_back(&selector.conditions[1]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative1);
  
  Ad::Creative creative2(2, 1, 0, 700, 200, 0.95, 1, "MSG1-2", Ad::CI_DIRECT);
  creative2.conditions.push_back(&selector.conditions[2]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative2);

  Ad::Creative creative2A(22, 1, 0, 700, 200, 0.96, 1,"MSG1-2A",Ad::CI_DIRECT);
  creative2A.conditions.push_back(&selector.conditions[3]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative2A);
  
  Ad::Creative creative3(3, 1, 0, 700, 200, 0.80, 1, "MSG2-1", Ad::CI_DIRECT);
  creative3.conditions.push_back(&selector.conditions[4]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG2, creative3);
  
  Ad::Creative creative4(4, 1, 0, 700, 200, 0.85, 1, "MSG2-2", Ad::CI_DIRECT);
  creative4.conditions.push_back(&selector.conditions[5]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG2, creative4);
  
  Ad::Creative creative5(5, 1, 0, 700, 200, 0.65, 1, "MSA1-1", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA1, creative5);
  
  Ad::Creative creative6(6, 1, 0, 700, 200, 0.70, 1, "MSA1-2", Ad::CI_DIRECT);
  creative6.conditions.push_back(&selector.conditions[7]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA1, creative6);
  
  Ad::Creative creative7(7, 1, 0, 700, 200, 0.76, 1, "MSA2-1", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA2, creative7);
  
  Ad::Creative creative8(8, 1, 0, 700, 200, 0.75, 1, "MSA2-2", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSA2, creative8);

  Ad::Creative creative9(9, 1, 0, 700, 200, 0.75, 1, "RTB1", Ad::CI_DIRECT);
  creative9.conditions.push_back(&selector.conditions[6]);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_RTB1, creative9);

  selector.finalize();

  Ad::SelectionContext context(
    Ad::PI_DESK_PAPER,
    7 * ((unsigned long long)RAND_MAX + 1) / 10 + 1);
  
  context.add_tag("at1");
  context.add_tag("at2");
  context.search_engine = "google";
  context.crawler = "googlebot";
  context.set_referer("http://www.newsfiber.com/p/s/h?q=ABC&r=10");

  context.slots.push_back(Ad::SI_DESK_PAPER_MSG1);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSG2);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSA1);
  context.slots.push_back(Ad::SI_DESK_PAPER_MSA2);
  context.slots.push_back(Ad::SI_DESK_PAPER_RTB1);

  Ad::SelectionResult result;

  ACE_High_Res_Timer timer;
  timer.start();
  
  selector.select(context, result);

  timer.stop();
  ACE_Time_Value tm;
  timer.elapsed_time(tm);          
  
  Ad::SelectionResult etalon_result;
  
  etalon_result.ads.push_back(Ad::Selection(creative2.id,
                                            Ad::SI_DESK_PAPER_MSG1,
                                            creative2.width,
                                            creative2.height,
                                            creative2.text.c_str(),
                                            Ad::CI_DIRECT));

  etalon_result.ads.push_back(Ad::Selection(creative6.id,
                                            Ad::SI_DESK_PAPER_MSA1,
                                            creative6.width,
                                            creative6.height,
                                            creative6.text.c_str(),
                                            Ad::CI_DIRECT));
  
  etalon_result.ads.push_back(Ad::Selection(creative7.id,
                                            Ad::SI_DESK_PAPER_MSA2,
                                            creative7.width,
                                            creative7.height,
                                            creative7.text.c_str(),
                                            Ad::CI_DIRECT));
  
  if(result.ads != etalon_result.ads)
  {
    std::ostringstream ostr;
    ostr << "Application::test15: failed. Result:\n";
    result.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }

  return tm;
}

ACE_Time_Value
Application::test16() throw(Exception, El::Exception)
{
  Ad::Selector selector(10, 10, 10, 10, 0);
  
  selector.pages[Ad::PI_DESK_PAPER].max_ad_num = 4;  
  selector.pages[Ad::PI_DESK_PAPER].add_max_ad_num(1, 3);

  Ad::Creative creative0(0, 1, 0, 700, 200, 1.0, 1, "0", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative0);
  
  Ad::Creative creative1(1, 1, 0, 700, 200, 0.5, 1, "1", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative1);

  Ad::Creative creative2(2, 1, 0, 700, 200, 0.08, 1, "2", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative2);

  Ad::Creative creative3(3, 1, 0, 700, 200, 0.1, 1, "3", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative3);

  Ad::Creative creative4(4, 1, 0, 700, 200, 0.07, 1, "4", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative4);

  Ad::Creative creative5(5, 1, 0, 700, 200, 0.1, 1, "5", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative5);

  selector.creative_weight_strategy.reset(
    new Ad::Selector::ProbabilisticCreativeWeightStrategy(0.5, 10));
  
  selector.finalize();

  Ad::SelectionContext context(
    Ad::PI_DESK_PAPER,
    7 * ((unsigned long long)RAND_MAX + 1) / 10 + 1);
  
  context.slots.push_back(Ad::SI_DESK_PAPER_MSG1);
  
  const size_t count = 100000;
  size_t counter[6];
  memset(&counter, 0, sizeof(counter));
    
  ACE_Time_Value tm = ACE_OS::gettimeofday();

  for(size_t i = 0; i < count; ++i)
  {
    Ad::SelectionResult result;
    selector.select(context, result);

    Ad::SelectionArray& ads = result.ads;

    if(ads.size() != 1)
    {
      std::ostringstream ostr;
      ostr << "Application::test16: failed. Results: selected ads "
           << ads.size() << "\nExpected: 1\n";
      
      throw Exception(ostr.str());
    }
      
    ++counter[ads[0].id];
  }
  
  tm = ACE_OS::gettimeofday() - tm;

  size_t total = 0;
  
  for(size_t i = 0; i < sizeof(counter) / sizeof(counter[0]); ++i)
  {
//    std::cerr << i << " " << counter[i] << ", ";
    total += counter[i];
  }

  if(total != count)
  {
    std::ostringstream ostr;
    ostr << "Application::test16: failed. Results: " << total
         << "\nExpected: " << count << std::endl;

    throw Exception(ostr.str());
  }

  size_t count1 = 0;
  
  for(size_t i = 0; i < sizeof(counter) / sizeof(counter[0]); ++i)
  {
    if(i > 0)
    {
      count1 += counter[i];
    }
  }

  double freq = (double)counter[0] / count;
  
  if(std::abs(freq - 0.66666) > 0.01)
  {
    std::ostringstream ostr;
    ostr << "Application::test16: failed. Creative #0 selection freq is "
         << freq << " instead of 0.6666";

    throw Exception(ostr.str());
  }  
  
  freq = (double)counter[1] / count1;
  
  if(std::abs(freq - 0.66666) > 0.01)
  {
    std::ostringstream ostr;
    ostr << "Application::test16: failed. Creative #1 selection freq is "
         << freq << " instead of 0.6666";

    throw Exception(ostr.str());
  }  

  if(counter[2] < counter[4])
  {
    std::ostringstream ostr;
    ostr << "Application::test16: failed. Creative #2 selection freq is less "
      "than that of #4: " << counter[2] << " vs " << counter[4];

    throw Exception(ostr.str());
  }
    
  if(counter[3] < counter[2])
  {
    std::ostringstream ostr;
    ostr << "Application::test16: failed. Creative #3 selection freq is less "
      "than that of #2: " << counter[3] << " vs " << counter[2];

    throw Exception(ostr.str());
  }

  double diff = std::abs((double)counter[3]/counter[5] - 1.0);
  
  if(diff > 0.05)
  {
    std::ostringstream ostr;
    ostr << "Application::test16: failed. Creative #3, #5 selection freq "
      "diff is too high: " << diff;

    throw Exception(ostr.str());
  }
    
  unsigned long long avr = (tm.sec() * 1000000 + tm.usec()) / count;
  return ACE_Time_Value(avr / 1000000, avr % 1000000);
}

ACE_Time_Value
Application::test17() throw(Exception, El::Exception)
{
  Ad::Selector selector(10, 10, 10, 10, 0);
  
  selector.pages[Ad::PI_DESK_PAPER].max_ad_num = 4;  
  selector.pages[Ad::PI_DESK_PAPER].add_max_ad_num(1, 3);

  Ad::Creative creative0(0, 1, 0, 700, 200, 0.7, 1, "0", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative0);

  Ad::Creative creative1(1, 1, 0, 700, 200, 0.7, 1, "1", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative1);

  selector.creative_weight_strategy.reset(
    new Ad::Selector::ProbabilisticCreativeWeightStrategy(0.5, 20));

  selector.finalize();

  Ad::SelectionContext context(
    Ad::PI_DESK_PAPER,
    7 * ((unsigned long long)RAND_MAX + 1) / 10 + 1);
  
  context.slots.push_back(Ad::SI_DESK_PAPER_MSG1);
  
  const size_t count = 100000;
  size_t counter[2];
  memset(&counter, 0, sizeof(counter));
    
  ACE_Time_Value tm = ACE_OS::gettimeofday();

  for(size_t i = 0; i < count; ++i)
  {
    Ad::SelectionResult result;
    selector.select(context, result);

    Ad::SelectionArray& ads = result.ads;
    
    if(ads.size() != 1)
    {
      std::ostringstream ostr;
      ostr << "Application::test17: failed. Results: selected ads "
           << ads.size() << "\nExpected: 1\n";
      
      throw Exception(ostr.str());
    }
      
    ++counter[ads[0].id];
  }
  
  tm = ACE_OS::gettimeofday() - tm;

  size_t total = 0;
  
  for(size_t i = 0; i < sizeof(counter) / sizeof(counter[0]); ++i)
  {
    total += counter[i];
  }

  if(total != count)
  {
    std::ostringstream ostr;
    ostr << "Application::test17: failed. Results: " << total
         << "\nExpected: " << count << std::endl;

    throw Exception(ostr.str());
  }

  double freq = (double)counter[0] / count;
  
  if(std::abs(freq - 0.5) > 0.01)
  {
    std::ostringstream ostr;
    ostr << "Application::test17: failed. Creative #0 selection freq is "
         << freq << " instead of 0.5";

    throw Exception(ostr.str());
  }  
    
  unsigned long long avr = (tm.sec() * 1000000 + tm.usec()) / count;
  return ACE_Time_Value(avr / 1000000, avr % 1000000);
}

ACE_Time_Value
Application::test18() throw(Exception, El::Exception)
{
  Ad::Selector selector(10, 10, 10, 10, 0);
  
  selector.pages[Ad::PI_DESK_PAPER].max_ad_num = 4;  
  selector.pages[Ad::PI_DESK_PAPER].add_max_ad_num(1, 3);

  Ad::Creative creative0(0, 1, 0, 700, 200, 0.8, 1, "0", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative0);

  Ad::Creative creative1(1, 1, 0, 700, 200, 0.4, 1, "1", Ad::CI_DIRECT);
  selector.add_creative(Ad::PI_DESK_PAPER, Ad::SI_DESK_PAPER_MSG1, creative1);

  selector.creative_weight_strategy.reset(
    new Ad::Selector::ProbabilisticCreativeWeightStrategy(0.5, 1));

  selector.finalize();

  Ad::SelectionContext context(
    Ad::PI_DESK_PAPER,
    7 * ((unsigned long long)RAND_MAX + 1) / 10 + 1);
  
  context.slots.push_back(Ad::SI_DESK_PAPER_MSG1);
  
  const size_t count = 100000;
  size_t counter[2];
  memset(&counter, 0, sizeof(counter));
    
  ACE_Time_Value tm = ACE_OS::gettimeofday();

  for(size_t i = 0; i < count; ++i)
  {
    Ad::SelectionResult result;
    selector.select(context, result);

    Ad::SelectionArray& ads = result.ads;
    
    if(ads.size() != 1)
    {
      std::ostringstream ostr;
      ostr << "Application::test18: failed. Results: selected ads "
           << ads.size() << "\nExpected: 1\n";
      
      throw Exception(ostr.str());
    }
      
    ++counter[ads[0].id];
  }
  
  tm = ACE_OS::gettimeofday() - tm;

  size_t total = 0;
  
  for(size_t i = 0; i < sizeof(counter) / sizeof(counter[0]); ++i)
  {
    total += counter[i];
  }

  if(total != count)
  {
    std::ostringstream ostr;
    ostr << "Application::test18: failed. Results: " << total
         << "\nExpected: " << count << std::endl;

    throw Exception(ostr.str());
  }

  double freq = (double)counter[0] / count;
  
  if(std::abs(freq - 0.66) > 0.01)
  {
    std::ostringstream ostr;
    ostr << "Application::test18: failed. Creative #0 selection freq is "
         << freq << " instead of 0.66";

    throw Exception(ostr.str());
  }  
    
  unsigned long long avr = (tm.sec() * 1000000 + tm.usec()) / count;
  return ACE_Time_Value(avr / 1000000, avr % 1000000);
}

ACE_Time_Value
Application::test19() throw(Exception, El::Exception)
{
  Ad::Selector selector(10, 10, 3, 10, 0);

  Ad::Condition condition(0, 0, 0, 0, 2, 0);
  selector.conditions[0] = condition;

  Ad::Counter counter(1, 1, 0, 1, "CNTR-1");
  counter.conditions.push_back(&selector.conditions[0]);
  selector.add_counter(Ad::PI_DESK_PAPER, counter);

  selector.finalize();

  Ad::SelectionContext context(Ad::PI_DESK_PAPER, 1);

  Ad::SelectionResult result;

  ACE_Time_Value tm = ACE_OS::gettimeofday();
  
  selector.select(context, result);

  tm = ACE_OS::gettimeofday() - tm;
  
  Ad::SelectionResult etalon_result;

  etalon_result.counters.push_back(Ad::SelectedCounter(counter.id,
                                                       counter.text.c_str()));

  if(result.counters != etalon_result.counters)
  {
    std::ostringstream ostr;
    ostr << "Application::test19: failed. Result:\n";
    result.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }

  std::string str;
  result.counter_caps.to_string(str);
  context.counter_caps.from_string(str.c_str());
  
  selector.finalize();
  
  Ad::SelectionResult result2;
  ACE_OS::sleep(2);

  context.counter_caps.set_current_time();
  
  selector.select(context, result2);

  if(result2.counters != etalon_result.counters)
  {
    std::ostringstream ostr;
    ostr << "Application::test19: failed. Result 2:\n";
    result2.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }

  result2.counter_caps.to_string(str);
  context.counter_caps.from_string(str.c_str());
  
  selector.finalize();
  
  Ad::SelectionResult result3;

  context.counter_caps.set_current_time();
  
  selector.select(context, result3);

  if(!result3.counters.empty())
  {
    std::ostringstream ostr;
    ostr << "Application::test19: failed. Result 3:\n";
    result3.dump(ostr);
    ostr << "\nExpected: empty\n";

    throw Exception(ostr.str());
  }  
  
  return tm;
}

ACE_Time_Value
Application::test20() throw(Exception, El::Exception)
{
  Ad::Selector selector(10, 10, 3, 10, 0);

  Ad::Condition condition(0, 0, 0, 0, 0, 2);
  selector.conditions[0] = condition;

  Ad::Counter counter(1, 1, 0, 1, "CNTR-1");
  counter.conditions.push_back(&selector.conditions[0]);
  selector.add_counter(Ad::PI_DESK_PAPER, counter);

  selector.finalize();

  Ad::SelectionContext context(Ad::PI_DESK_PAPER, 1);

  Ad::SelectionResult result;

  ACE_Time_Value tm = ACE_OS::gettimeofday();
  
  selector.select(context, result);

  tm = ACE_OS::gettimeofday() - tm;
  
  Ad::SelectionResult etalon_result;

  etalon_result.counters.push_back(Ad::SelectedCounter(counter.id,
                                                       counter.text.c_str()));

  if(result.counters != etalon_result.counters)
  {
    std::ostringstream ostr;
    ostr << "Application::test20: failed. Result:\n";
    result.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }

  std::string str;
  result.counter_caps.to_string(str);
  context.counter_caps.from_string(str.c_str());
  
  selector.finalize();
  
  Ad::SelectionResult result2;

  context.counter_caps.set_current_time();
  
  selector.select(context, result2);

  if(result2.counters != etalon_result.counters)
  {
    std::ostringstream ostr;
    ostr << "Application::test20: failed. Result 2:\n";
    result2.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }

  result2.counter_caps.to_string(str);
  context.counter_caps.from_string(str.c_str());
  
  selector.finalize();
  
  Ad::SelectionResult result3;

  context.counter_caps.set_current_time();
  
  selector.select(context, result3);

  if(!result3.counters.empty())
  {
    std::ostringstream ostr;
    ostr << "Application::test20: failed. Result 3:\n";
    result3.dump(ostr);
    ostr << "\nExpected: empty\n";

    throw Exception(ostr.str());
  }  
  
  return tm;
}

ACE_Time_Value
Application::test21() throw(Exception, El::Exception)
{
  Ad::Selector selector(10, 10, 3, 10, 0);

  Ad::Condition condition(0, 0, 0, 0, 3, 0);
  selector.conditions[0] = condition;

  Ad::Counter counter(1, 1, 0, 1, "CNTR-1");
  counter.conditions.push_back(&selector.conditions[0]);
  selector.add_counter(Ad::PI_DESK_PAPER, counter);

  selector.finalize();

  Ad::SelectionContext context(Ad::PI_DESK_PAPER, 1);

  Ad::SelectionResult result;

  ACE_Time_Value tm = ACE_OS::gettimeofday();
  
  selector.select(context, result);

  tm = ACE_OS::gettimeofday() - tm;
  
  Ad::SelectionResult etalon_result;

  etalon_result.counters.push_back(Ad::SelectedCounter(counter.id,
                                                       counter.text.c_str()));

  if(result.counters != etalon_result.counters)
  {
    std::ostringstream ostr;
    ostr << "Application::test21: failed. Result:\n";
    result.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }

  std::string str;
  result.counter_caps.to_string(str);
  context.counter_caps.from_string(str.c_str());

  ACE_OS::sleep(1);
  
  selector.pages[Ad::PI_DESK_PAPER].counters[0].group_cap_min_time =
    ACE_OS::gettimeofday().sec();
  
  selector.finalize();
  
  Ad::SelectionResult result2;    

  context.counter_caps.set_current_time();
  
  selector.select(context, result2);

  if(result2.counters != etalon_result.counters)
  {
    std::ostringstream ostr;
    ostr << "Application::test21: failed. Result 2:\n";
    result2.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }

  result2.counter_caps.to_string(str);
  context.counter_caps.from_string(str.c_str());

  selector.conditions[0].tags.push_back("tg");

  Ad::Counter counter2(2, 2, 0, 1, "CNTR-2");
  selector.add_counter(Ad::PI_DESK_PAPER, counter2);
    
  ACE_OS::sleep(1);

  selector.pages[Ad::PI_DESK_PAPER].counters[0].group_cap_min_time =
    ACE_OS::gettimeofday().sec();
  
  selector.finalize();
  
  Ad::SelectionResult result3;

  context.counter_caps.set_current_time();
  
  selector.select(context, result3);  

  Ad::SelectionResult etalon_result3;

  etalon_result3.counters.push_back(
    Ad::SelectedCounter(counter2.id, counter2.text.c_str()));

  if(result3.counters != etalon_result3.counters)
  {
    std::ostringstream ostr;
    ostr << "Application::test21: failed. Result 3:\n";
    result.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }

  if(!result3.counter_caps.caps.empty())
  {
    std::ostringstream ostr;
    ostr << "Application::test21: failed. No result caps expected\n";

    throw Exception(ostr.str());
  }  

  return tm;
}

ACE_Time_Value
Application::test22() throw(Exception, El::Exception)
{
  Ad::Selector selector;
  
  {
    Ad::Condition condition;
    condition.query_types = Ad::QT_ANY;
    condition.query_type_exclusions = Ad::QT_EVENT;
    selector.conditions[0] = condition;
  }
  
  {
    Ad::Condition condition;
    condition.query_types = Ad::QT_MESSAGE;
    selector.conditions[1] = condition;
  }
  
  {
    Ad::Condition condition;
    condition.query_types = Ad::QT_ANY;
    condition.query_type_exclusions = Ad::QT_SEARCH;
    selector.conditions[2] = condition;
  }
  
  {
    Ad::Condition condition;
    condition.query_types = Ad::QT_NONE;
    selector.conditions[3] = condition;
  }
  
  {
    Ad::Condition condition;
    condition.query_type_exclusions = Ad::QT_NONE;
    selector.conditions[4] = condition;
  }
  
  {
    Ad::Condition condition;
    selector.conditions[5] = condition;
  }
  
  Ad::Counter counter0(0, 1, 0, 1, "CNTR-0");
  counter0.conditions.push_back(&selector.conditions[0]);
  selector.add_counter(Ad::PI_DESK_PAPER, counter0);

  Ad::Counter counter1(1, 1, 0, 1, "CNTR-1");
  counter1.conditions.push_back(&selector.conditions[1]);
  selector.add_counter(Ad::PI_DESK_PAPER, counter1);

  Ad::Counter counter2(2, 1, 0, 1, "CNTR-2");
  counter2.conditions.push_back(&selector.conditions[2]);
  selector.add_counter(Ad::PI_DESK_PAPER, counter2);
  
  Ad::Counter counter3(3, 1, 0, 1, "CNTR-3");
  counter3.conditions.push_back(&selector.conditions[3]);
  selector.add_counter(Ad::PI_DESK_PAPER, counter3);
  
  Ad::Counter counter4(4, 1, 0, 1, "CNTR-4");
  counter4.conditions.push_back(&selector.conditions[4]);
  selector.add_counter(Ad::PI_DESK_PAPER, counter4);
  
  Ad::Counter counter5(5, 1, 0, 1, "CNTR-5");
  counter5.conditions.push_back(&selector.conditions[5]);
  selector.add_counter(Ad::PI_DESK_PAPER, counter5);
  
  selector.finalize();

  Ad::SelectionContext context(Ad::PI_DESK_PAPER);
  context.query_types = Ad::QT_SEARCH | Ad::QT_CATEGORY;

  Ad::SelectionResult result;

  ACE_Time_Value tm = ACE_OS::gettimeofday();
  
  selector.select(context, result);

  tm = ACE_OS::gettimeofday() - tm;
  
  Ad::SelectionResult etalon_result;

  etalon_result.counters.push_back(Ad::SelectedCounter(counter0.id,
                                                       counter0.text.c_str()));

  etalon_result.counters.push_back(Ad::SelectedCounter(counter4.id,
                                                       counter4.text.c_str()));

  etalon_result.counters.push_back(Ad::SelectedCounter(counter5.id,
                                                       counter5.text.c_str()));

  if(result.counters != etalon_result.counters)
  {
    std::ostringstream ostr;
    ostr << "Application::test22: failed. Result:\n";
    result.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }

  return tm;
}

ACE_Time_Value
Application::test23() throw(Exception, El::Exception)
{
  Ad::Selector selector;
  
  {
    Ad::Condition condition;
    condition.query_types = Ad::QT_NONE;
    selector.conditions[0] = condition;
  }
  
  {
    Ad::Condition condition;
    condition.query_type_exclusions = Ad::QT_ANY;
    selector.conditions[1] = condition;
  }
  
  {
    Ad::Condition condition;
    condition.query_type_exclusions = Ad::QT_SEARCH;
    selector.conditions[2] = condition;
  }
  
  {
    Ad::Condition condition;
    condition.query_types = Ad::QT_EVENT;
    selector.conditions[3] = condition;
  }
  
  Ad::Counter counter0(0, 1, 0, 1, "CNTR-0");
  counter0.conditions.push_back(&selector.conditions[0]);
  selector.add_counter(Ad::PI_DESK_PAPER, counter0);
  
  Ad::Counter counter1(1, 1, 0, 1, "CNTR-1");
  counter1.conditions.push_back(&selector.conditions[1]);
  selector.add_counter(Ad::PI_DESK_PAPER, counter1);
  
  Ad::Counter counter2(2, 1, 0, 1, "CNTR-2");
  counter2.conditions.push_back(&selector.conditions[2]);
  selector.add_counter(Ad::PI_DESK_PAPER, counter2);
  
  Ad::Counter counter3(3, 1, 0, 1, "CNTR-3");
  counter3.conditions.push_back(&selector.conditions[3]);
  selector.add_counter(Ad::PI_DESK_PAPER, counter3);
  
  selector.finalize();

  Ad::SelectionContext context(Ad::PI_DESK_PAPER);

  Ad::SelectionResult result;

  ACE_Time_Value tm = ACE_OS::gettimeofday();
  
  selector.select(context, result);

  tm = ACE_OS::gettimeofday() - tm;
  
  Ad::SelectionResult etalon_result;

  etalon_result.counters.push_back(Ad::SelectedCounter(counter0.id,
                                                       counter0.text.c_str()));

  etalon_result.counters.push_back(Ad::SelectedCounter(counter1.id,
                                                       counter1.text.c_str()));

  etalon_result.counters.push_back(Ad::SelectedCounter(counter2.id,
                                                       counter2.text.c_str()));

  if(result.counters != etalon_result.counters)
  {
    std::ostringstream ostr;
    ostr << "Application::test23: failed. Result:\n";
    result.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }

  return tm;
}

ACE_Time_Value
Application::test24() throw(Exception, El::Exception)
{
  Ad::Selector selector;
  
  {
    Ad::Condition condition;
    condition.content_languages.push_back(Ad::Condition::ANY_LANG);
    condition.content_language_exclusions.push_back(El::Lang("ger"));
    selector.conditions[0] = condition;
  }

  {
    Ad::Condition condition;
    condition.content_languages.push_back(El::Lang("ger"));
    selector.conditions[1] = condition;
  }

  {
    Ad::Condition condition;
    condition.content_languages.push_back(Ad::Condition::ANY_LANG);
    condition.content_language_exclusions.push_back(El::Lang("rus"));
    selector.conditions[2] = condition;
  }
  
  {
    Ad::Condition condition;
    condition.content_languages.push_back(Ad::Condition::NONE_LANG);
    selector.conditions[3] = condition;
  }
  
  {
    Ad::Condition condition;
    condition.content_language_exclusions.push_back(Ad::Condition::NONE_LANG);
    selector.conditions[4] = condition;
  }
  
  {
    Ad::Condition condition;
    selector.conditions[5] = condition;
  }
  
  Ad::Counter counter0(0, 1, 0, 1, "CNTR-0");
  counter0.conditions.push_back(&selector.conditions[0]);
  selector.add_counter(Ad::PI_DESK_PAPER, counter0);

  Ad::Counter counter1(1, 1, 0, 1, "CNTR-1");
  counter1.conditions.push_back(&selector.conditions[1]);
  selector.add_counter(Ad::PI_DESK_PAPER, counter1);

  Ad::Counter counter2(2, 1, 0, 1, "CNTR-2");
  counter2.conditions.push_back(&selector.conditions[2]);
  selector.add_counter(Ad::PI_DESK_PAPER, counter2);
  
  Ad::Counter counter3(3, 1, 0, 1, "CNTR-3");
  counter3.conditions.push_back(&selector.conditions[3]);
  selector.add_counter(Ad::PI_DESK_PAPER, counter3);
  
  Ad::Counter counter4(4, 1, 0, 1, "CNTR-4");
  counter4.conditions.push_back(&selector.conditions[4]);
  selector.add_counter(Ad::PI_DESK_PAPER, counter4);
  
  Ad::Counter counter5(5, 1, 0, 1, "CNTR-5");
  counter5.conditions.push_back(&selector.conditions[5]);
  selector.add_counter(Ad::PI_DESK_PAPER, counter5);

  selector.finalize();

  Ad::SelectionContext context(Ad::PI_DESK_PAPER);
  context.content_languages.insert(El::Lang("rus"));
  context.content_languages.insert(El::Lang("eng"));

  Ad::SelectionResult result;

  ACE_Time_Value tm = ACE_OS::gettimeofday();
  
  selector.select(context, result);

  tm = ACE_OS::gettimeofday() - tm;
  
  Ad::SelectionResult etalon_result;

  etalon_result.counters.push_back(Ad::SelectedCounter(counter0.id,
                                                       counter0.text.c_str()));
  
  etalon_result.counters.push_back(Ad::SelectedCounter(counter4.id,
                                                       counter4.text.c_str()));

  etalon_result.counters.push_back(Ad::SelectedCounter(counter5.id,
                                                       counter5.text.c_str()));

  if(result.counters != etalon_result.counters)
  {
    std::ostringstream ostr;
    ostr << "Application::test22: failed. Result:\n";
    result.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }

  return tm;
}

ACE_Time_Value
Application::test25() throw(Exception, El::Exception)
{
  Ad::Selector selector;
  
  {
    Ad::Condition condition;
    condition.content_languages.push_back(Ad::Condition::NONE_LANG);
    selector.conditions[0] = condition;
  }

  {
    Ad::Condition condition;
    condition.content_language_exclusions.push_back(Ad::Condition::ANY_LANG);
    selector.conditions[1] = condition;
  }

  {
    Ad::Condition condition;
    condition.content_language_exclusions.push_back(El::Lang("rus"));
    selector.conditions[2] = condition;
  }

  {
    Ad::Condition condition;
    condition.content_languages.push_back(El::Lang("rus"));
    selector.conditions[3] = condition;
  }

  Ad::Counter counter0(0, 1, 0, 1, "CNTR-0");
  counter0.conditions.push_back(&selector.conditions[0]);
  selector.add_counter(Ad::PI_DESK_PAPER, counter0);

  Ad::Counter counter1(1, 1, 0, 1, "CNTR-1");
  counter1.conditions.push_back(&selector.conditions[1]);
  selector.add_counter(Ad::PI_DESK_PAPER, counter1);

  Ad::Counter counter2(2, 1, 0, 1, "CNTR-2");
  counter2.conditions.push_back(&selector.conditions[2]);
  selector.add_counter(Ad::PI_DESK_PAPER, counter2);
  
  Ad::Counter counter3(3, 1, 0, 1, "CNTR-3");
  counter3.conditions.push_back(&selector.conditions[3]);
  selector.add_counter(Ad::PI_DESK_PAPER, counter3);
  
  selector.finalize();

  Ad::SelectionContext context(Ad::PI_DESK_PAPER);

  Ad::SelectionResult result;

  ACE_Time_Value tm = ACE_OS::gettimeofday();
  
  selector.select(context, result);

  tm = ACE_OS::gettimeofday() - tm;
  
  Ad::SelectionResult etalon_result;

  etalon_result.counters.push_back(Ad::SelectedCounter(counter0.id,
                                                       counter0.text.c_str()));

  etalon_result.counters.push_back(Ad::SelectedCounter(counter1.id,
                                                       counter1.text.c_str()));

  etalon_result.counters.push_back(Ad::SelectedCounter(counter2.id,
                                                       counter2.text.c_str()));

  if(result.counters != etalon_result.counters)
  {
    std::ostringstream ostr;
    ostr << "Application::test25: failed. Result:\n";
    result.dump(ostr);
    ostr << "\nExpected:\n";
    etalon_result.dump(ostr);
    ostr << std::endl;

    throw Exception(ostr.str());
  }

  return tm;
}
