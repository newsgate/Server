/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/tests/AdSelection/AdSelectionMain.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_TESTS_ADSELECTION_ADSELECTIONTEST_HPP_
#define _NEWSGATE_SERVER_TESTS_ADSELECTION_ADSELECTIONTEST_HPP_

#include <iostream>
#include <string>

#include <El/Exception.hpp>

#include <Commons/Ad/Ad.hpp>

class Application
{
public:
  EL_EXCEPTION(Exception, El::ExceptionBase);

  Application() throw();
  
  int run(int argc, char** argv) throw();

private:

  void test_selector() throw(Exception, El::Exception);

  ACE_Time_Value test1() throw(Exception, El::Exception);
  ACE_Time_Value test2() throw(Exception, El::Exception);
  ACE_Time_Value test3() throw(Exception, El::Exception);
  ACE_Time_Value test4() throw(Exception, El::Exception);
  ACE_Time_Value test5() throw(Exception, El::Exception);
  ACE_Time_Value test6() throw(Exception, El::Exception);
  ACE_Time_Value test7() throw(Exception, El::Exception);
  ACE_Time_Value test8() throw(Exception, El::Exception);
  ACE_Time_Value test9() throw(Exception, El::Exception);
  ACE_Time_Value test10() throw(Exception, El::Exception);
  ACE_Time_Value test11() throw(Exception, El::Exception);
  ACE_Time_Value test12() throw(Exception, El::Exception);
  ACE_Time_Value test13() throw(Exception, El::Exception);
  ACE_Time_Value test14() throw(Exception, El::Exception);
  ACE_Time_Value test15() throw(Exception, El::Exception);
  ACE_Time_Value test16() throw(Exception, El::Exception);
  ACE_Time_Value test17() throw(Exception, El::Exception);
  ACE_Time_Value test18() throw(Exception, El::Exception);
  ACE_Time_Value test19() throw(Exception, El::Exception);
  ACE_Time_Value test20() throw(Exception, El::Exception);
  ACE_Time_Value test21() throw(Exception, El::Exception);
  ACE_Time_Value test22() throw(Exception, El::Exception);
  ACE_Time_Value test23() throw(Exception, El::Exception);
  ACE_Time_Value test24() throw(Exception, El::Exception);
  ACE_Time_Value test25() throw(Exception, El::Exception);

private:
  bool verbose_;
  bool randomize_;  
};

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

//
// Application class
//
inline
Application::Application() throw()
    : verbose_(false),
      randomize_(true)
{
}

#endif // _NEWSGATE_SERVER_TESTS_ADSELECTION_ADSELECTIONTEST_HPP_
