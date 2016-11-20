/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/tests/SearchExpression/SearchExpressionMain.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_TESTS_SEARCHEXPRESSION_HPP_
#define _NEWSGATE_SERVER_TESTS_SEARCHEXPRESSION_HPP_

#include <memory>
#include <string>
#include <list>
#include <vector>

#include <El/Exception.hpp>
#include <El/Lang.hpp>
#include <El/Dictionary/Morphology.hpp>
#include <El/String/ListParser.hpp>
#include <El/Luid.hpp>

#include <Commons/Search/SearchExpression.hpp>
#include <Commons/Message/Message.hpp>
#include <Commons/Feed/Types.hpp>

#include <tests/Commons/SourceText.hpp>
#include <tests/Commons/SourceUrls.hpp>

class Application
{
public:
  EL_EXCEPTION(Exception, El::ExceptionBase);

  Application() throw();
  
  int run(int argc, char** argv) throw();

public:
  
  typedef std::auto_ptr< ::NewsGate::Search::MessageWordPositionMap >
  MessageWordPositionMapPtr;
  
  struct PositiveParseSample
  {
    const wchar_t* source;
    const wchar_t* compiled;
    const wchar_t* printed;
  };

  struct NegativeParseSample
  {
    const wchar_t* source;
    unsigned long position;
    NewsGate::Search::ParseError::Code code;
  };

  struct Msg
  {
    std::string description;
    NewsGate::Message::Id id;
    unsigned long updated;
    unsigned long fetched;
    std::string source_url;
    El::Lang lang;
    El::Luid event_id;
    unsigned long event_capacity;

    Msg(const char* desc,
        const NewsGate::Message::Id& id_val,
        const char* source_url_val = 0,
        unsigned long updated_val = 0,
        const El::Lang& lang_val = El::Lang::null,
        const El::Luid& event_id_val = El::Luid::null,
        unsigned long event_capacity_val = 0,
        unsigned long fetched_val = 0)
      throw(El::Exception)
        : description(desc),
          id(id_val),
          updated(updated_val),
          fetched(fetched_val),
          source_url(source_url_val ? source_url_val : "www.google.co.uk/A/C"),
          lang(lang_val),
          event_id(event_id_val),
          event_capacity(event_capacity_val)
    {
    }
  };

  typedef std::list<Msg> MessageList;

  struct ExpectedResult
  {
    ::NewsGate::Message::Id msg_id;
    ::NewsGate::Search::WordPositionArray positions;

    EL_EXCEPTION(Exception, El::ExceptionBase);
    EL_EXCEPTION(InvalidArgument, Exception);

    ExpectedResult(const NewsGate::Message::Id& id,
                   const char* positions)
      throw(InvalidArgument, El::Exception);
  };

  typedef std::vector<ExpectedResult> ExpectedResultArray;

  struct EvaluateSample
  {
    std::wstring     expression;
    MessageList      messages;
    ::NewsGate::Search::Strategy strategy;
    ExpectedResultArray search_result;

    EvaluateSample() throw();
  };

  typedef std::list<EvaluateSample> EvaluateSampleList;

private:
  void test_compilation_positive() throw(El::Exception);
  
  void test_compilation_positive(
    const Application::PositiveParseSample& sample)
    throw(El::Exception);
  
  void test_compilation_negative() throw(El::Exception);
  void test_search() throw(El::Exception);

  void insert_message(const char* description,
                      const char* source_url,
                      const NewsGate::Message::Id& id,
                      unsigned long updated,
                      unsigned long fetched,
                      const El::Lang& lang,
                      const El::Luid& event_id,
                      unsigned long event_capacity,
                      NewsGate::Message::SearcheableMessageMap& message_map)
    throw(El::Exception);

  void test_search(const EvaluateSample& sample,
                   unsigned long sample_num)
    throw(El::Exception);

  void test_performance() throw(El::Exception);

  bool search_words(
    bool any,
    const NewsGate::Message::SearcheableMessageMap& message_map)
    throw(El::Exception);

  std::wstring random_words(unsigned long max_expr_words)
    throw(El::Exception);
  
  void create_messages(NewsGate::Message::SearcheableMessageMap& message_map)
    throw(El::Exception);
  
  void test_all_condition(NewsGate::Message::SearcheableMessageMap&
                          message_map)
    throw(El::Exception);
  
  void test_any_condition(NewsGate::Message::SearcheableMessageMap&
                          message_map)
    throw(El::Exception);
  
  void test_complex_condition(NewsGate::Message::SearcheableMessageMap&
                              message_map)
    throw(El::Exception);
  
  bool search_complex(NewsGate::Message::SearcheableMessageMap& message_map,
                      unsigned long long& total_results)
    throw(El::Exception);
  
  NewsGate::Search::Result* evaluate(
    NewsGate::Search::Expression* condition,
    NewsGate::Search::Expression* condition_optimized,
    const NewsGate::Message::SearcheableMessageMap& message_map,
    const char* expression,
    ::NewsGate::Search::MessageWordPositionMap* positions,
    unsigned long long* total_results = 0,
    const ::NewsGate::Search::Strategy* pstrategy = 0)
    throw(El::Exception);
  
  std::wstring random_expression(unsigned long max_operands_in_complex_cond)
    throw(El::Exception);

private:
  bool verbose_;
  unsigned long min_message_len_;
  unsigned long max_message_len_;
  unsigned long message_count_;
  unsigned long search_count_;
  unsigned long result_count_;
  unsigned long max_all_expr_words_;
  unsigned long max_any_expr_words_;
  unsigned long max_domain_expr_words_;
  unsigned long max_complex_expr_words_;
  unsigned long max_operands_in_complex_cond_;
  unsigned long max_lang_expr_words_;
  unsigned long max_country_expr_words_;
  bool pauses_;
  bool positions_;
  bool randomize_;
  
  NewsGate::Test::SourceText_var source_text_;
  NewsGate::Test::SourceUrls_var source_urls_;

  El::Dictionary::Morphology::WordInfoManager word_info_manager_;
};

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

//
// Application::EvaluateSample class
//
inline
Application::EvaluateSample::EvaluateSample() throw()
{
}

//
// Application::ExpectedResult class
//
inline
Application::ExpectedResult::ExpectedResult(const NewsGate::Message::Id& id,
                                            const char* pos)
  throw(InvalidArgument, El::Exception)
  : msg_id(id)
{
  El::String::ListParser parser(pos);

  const char* item = 0;

  while((item = parser.next_item()) != 0)
  {
    unsigned long val = 0;

    if(!El::String::Manip::numeric(item, val))
    {
      std::ostringstream ostr;
      ostr << "Application::ExpectedResult::ExpectedResult: invalid position "
        "array '" << pos << "'";

      throw InvalidArgument(ostr.str());
    }
    
    positions.push_back(val);  
  }
}

//
// Application class
//
inline
Application::Application() throw()
    : verbose_(false),
      min_message_len_(1024), // 10
      max_message_len_(1024), // 10
      message_count_(100000), //  100000
      search_count_(1000),
      result_count_(10000),
      max_all_expr_words_(10), // 2
      max_any_expr_words_(4), // 2
      max_domain_expr_words_(10),
      max_complex_expr_words_(5),
      max_operands_in_complex_cond_(5), // 4
      max_lang_expr_words_(5),
      max_country_expr_words_(5),
      pauses_(false),
      positions_(false),
      randomize_(true),
      word_info_manager_(10, 51, 51)
{
}

#endif // _NEWSGATE_SERVER_TESTS_SEARCHEXPRESSION_HPP_

