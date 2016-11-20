/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/tests/SearchExpression/SearchExpressionMain.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <El/CORBA/Corba.hpp>

#include <stdlib.h>
#include <unistd.h>

#include <sstream>
#include <iostream>
#include <list>
#include <fstream>

#ifdef USE_HIRES
# include <ace/High_Res_Timer.h>
#endif

#include <El/Exception.hpp>
#include <El/String/Manip.hpp>
#include <El/String/ListParser.hpp>
#include <El/String/Unicode.hpp>
#include <El/Utility.hpp>

#include <El/Net/URL.hpp>
#include <El/Net/HTTP/URL.hpp>

#include <Commons/Message/StoredMessage.hpp>

#include "SearchExpressionMain.hpp"

namespace
{
  const char USAGE[] =
    "Usage: SearchExpressionTest ( [-help] | [-verbose] [-pause] [-positions] "
    "[-not-randomize] [-msg-count=<message count>] "
    "[-result-count=<search results count>] "
    "[-search-count=<search count>] [-msg-file=<message source>] "
    "[-url-file=<url source>] (-dict=<dictionary file>)* )";
  
  const Application::NegativeParseSample NEGATIVE_PARSE_SAMPLES[] =
  {
    { L"middle: \"east west\", war",
      25,
      NewsGate::Search::ParseError::UNEXPECTED_END_OF_EXPRESSION
    },
    { L"middle: 'east west war",
      23,
      NewsGate::Search::ParseError::UNEXPECTED_END_OF_EXPRESSION
    },
    { L"middle AND \"(\" east OR west )",
      29,
      NewsGate::Search::ParseError::OPERATION_EXPECTED
    },
    { L"ANY ?-",
      7,
      NewsGate::Search::ParseError::NO_WORDS_FOR_ANY
    },
    { L"ALL ?",
      6,
      NewsGate::Search::ParseError::NO_WORDS_FOR_ALL
    },
    { L"middle OR ALL EXCEPT east",
      15,
      NewsGate::Search::ParseError::NO_WORDS_FOR_ALL
    },
    { L"ANY DOMAIN",
      5,
      NewsGate::Search::ParseError::NO_WORDS_FOR_ANY
    },
    { L"middle east AND SITE AND",
      22,
      NewsGate::Search::ParseError::NO_HOST_FOR_SITE
    },
    { L"middle EXCEPT URL OR",
      19,
      NewsGate::Search::ParseError::NO_PATH_FOR_URL
    },
    { L"middle EXCEPT URL https://www.gnu.com",
      19,
      NewsGate::Search::ParseError::HTTPS_NOT_ALLOWED_FOR_URL
    },
    { L"middle EXCEPT URL ftp://www.gnu.com",
      19,
      NewsGate::Search::ParseError::BAD_PATH_FOR_URL
    },
    { L"middle EXCEPT  AND",
      16,
      NewsGate::Search::ParseError::OPERAND_EXPECTED
    },
    { L"middle EXCEPT ( ( ALL far east OR west ) AND war",
      49,
      NewsGate::Search::ParseError::CLOSE_PARENTHESIS_EXPECTED
    },
    { L"middle ANY east",
      8,
      NewsGate::Search::ParseError::OPERATION_EXPECTED
    },
    { L"middle  AND west DATE  OR",
      24,
      NewsGate::Search::ParseError::NO_TIME_FOR_PUB_DATE
    },
    { L"\"middle \" AND \" west\" DATE 1a",
      28,
      NewsGate::Search::ParseError::WRONG_TIME_FOR_PUB_DATE
    },
    { L"car COUNTRY EXCEPT rus",
      13,
      NewsGate::Search::ParseError::COUNTRY_OR_NOT_EXPECTED
    },
    { L"car COUNTRY NOT lguhih",
      17,
      NewsGate::Search::ParseError::INVALID_COUNTRY
    },
    { L"\"middle \"  LANGUAGE   EXCEPT",
      23,
      NewsGate::Search::ParseError::LANG_OR_NOT_EXPECTED
    },
    { L"middle LANGUAGE  NOT govyazhii",
      22,
      NewsGate::Search::ParseError::INVALID_LANG
    },
    { L"middle LANGUAGE  NOT EXCEPT",
      22,
      NewsGate::Search::ParseError::LANG_EXPECTED
    },
    { L"middle  DOMAIN AND",
      16,
      NewsGate::Search::ParseError::DOMAIN_OR_NOT_EXPECTED
    },
    { L"middle  DOMAIN   NOT  OR ",
      23,
      NewsGate::Search::ParseError::DOMAIN_EXPECTED
    },
    { L"middle  DOMAIN   NOT \"google.co.uk ",
      36,
      NewsGate::Search::ParseError::UNEXPECTED_END_OF_EXPRESSION
    },
    { L"EVERY FETCHED",
      14,
      NewsGate::Search::ParseError::NO_TIME_FOR_FETCHED
    },
    { L"EVERY FETCHED a",
      15,
      NewsGate::Search::ParseError::WRONG_TIME_FOR_FETCHED
    },
    { L"EVERY FETCHED 1a",
      15,
      NewsGate::Search::ParseError::WRONG_TIME_FOR_FETCHED
    },
    { L"EVERY FETCHED 2002-10-11.s8:03:01",
      15,
      NewsGate::Search::ParseError::WRONG_TIME_FOR_FETCHED
    },
    { L"EVERY FETCHED 2002-10-11.08:0d:01",
      15,
      NewsGate::Search::ParseError::WRONG_TIME_FOR_FETCHED
    },
    { L"EVERY FETCHED 2002-10-11.08:01:0c",
      15,
      NewsGate::Search::ParseError::WRONG_TIME_FOR_FETCHED
    },
    { L"EVERY FETCHED 2002-1e-11",
      15,
      NewsGate::Search::ParseError::WRONG_TIME_FOR_FETCHED
    }
  };

  const Application::PositiveParseSample POSITIVE_PARSE_NORM_SAMPLES[] =
  {
    {
      L"ANY 2 'ever best build more bests build less' 'bests built'",
      L"ANY 2 'bests built'\n",
      L"ANY 2 'bests built'"
    },
    {
      L"ANY 2 'ever best build' 'bests built'",
      L"ANY 2 'ever best build' 'bests built'\n",
      L"ANY 2 'ever best build' 'bests built'"
    },
    {
      L"ANY 2 'high build car green' \"built cars\"",
      L"ANY 2 'high build car green' \"built cars\"\n",
      L"ANY 2 'high build car green' \"built cars\""
    },    
    {
      L"ANY 2 'built cars' 'high build car green'",
      L"ANY 2 'built cars'\n",
      L"ANY 2 'built cars'"
    },    
    {
      L"ANY 2 'high build car green' 'built cars'",
      L"ANY 2 'built cars'\n",
      L"ANY 2 'built cars'"
    },    
    {
      L"ANY 2 'high buildings building' builds",
      L"ANY 2 'high buildings building' builds\n",
      L"ANY 2 'high buildings building' builds"
    },    
    {
      L"ANY 2 'high building' buildings",
      L"ANY 2 'high building' buildings\n",
      L"ANY 2 'high building' buildings"
    },    
    {
      L"ANY 2 'high building' built",
      L"ANY 2 'high building' built\n",
      L"ANY 2 'high building' built"
    },    
    {
      L"ANY 2 car abc32",
      L"ANY 2 car abc32\n",
      L"ANY 2 car abc32"
    },    
    {
      L"ANY 2 car world",
      L"ANY 2 car world\n",
      L"ANY 2 car world"
    },    
    {
      L"ANY 2 bound binding",
      L"ANY 2 bound binding\n",
      L"ANY 2 bound binding"
    },    
    {
      L"ANY 2 'buildings' built",
      L"ANY 2 buildings built\n",
      L"ANY 2 buildings built"
    },
    {
      L"ANY 2 ax1 bx1",
      L"ANY 2 ax1 bx1\n",
      L"ANY 2 ax1 bx1"
    },
    {
      L"ANY 2 ax1 \"ax1\"",
      L"ANY 2 ax1\n",
      L"ANY 2 ax1"
    },
    {
      L"ANY 2 \"ax1\" ax1",
      L"ANY 2 \"ax1\"\n",
      L"ANY 2 \"ax1\""
    },
    {
      L"ANY 2 \"buildings\" buildings",
      L"ANY 2 buildings\n",
      L"ANY 2 buildings"
    },
    {
      L"ANY 2 buildings \"buildings\"",
      L"ANY 2 buildings\n",
      L"ANY 2 buildings"
    },
    {
      L"ANY 2 buildings building",
      L"ANY 2 building\n",
      L"ANY 2 building"
    },
    {
      L"ANY 2 buildings \"building\"",
      L"ANY 2 buildings\n",
      L"ANY 2 buildings"
    }
  };
  
  const Application::PositiveParseSample POSITIVE_PARSE_SAMPLES[] =
  {
    {
      L"ANY 2 'A B' 'A C' D",
      L"ANY 2 'a b' 'a c' d\n",
      L"ANY 2 'a b' 'a c' d"
    },
    {
      L"ANY 2 'A B' 'X Y Z' 'X A B C' 'Z A B' 'B C' B b",
      L"ANY 2 'x y z' b\n",
      L"ANY 2 'x y z' b"
    },
    {
      L"ANY 3 CORE 'A B' 'A B C' A",
      L"ANY 3 CORE a\n",
      L"ANY 3 CORE a"
    },
    {
      L"EVERY DOMAIN b",
      L"DOMAIN\n  EVERY\n  b\n",
      L"EVERY DOMAIN b"
    },
    {
      L"EVERY RCTR 10 50",
      L"RCTR\n  EVERY\n  10\n  50\n",
      L"EVERY RCTR 10 50"
    },
    {
      L"EVERY RCTR 10",
      L"RCTR\n  EVERY\n  10\n",
      L"EVERY RCTR 10"
    },
    {
      L"EVERY RCTR NOT 10 50",
      L"RCTR NOT\n  EVERY\n  10\n  50\n",
      L"EVERY RCTR NOT 10 50"
    },
    {
      L"EVERY RCTR NOT 10",
      L"RCTR NOT\n  EVERY\n  10\n",
      L"EVERY RCTR NOT 10"
    },
    {
      L"EVERY F-RCTR 10",
      L"F-RCTR\n  EVERY\n  10\n",
      L"EVERY F-RCTR 10"
    },
    {
      L"EVERY F-RCTR NOT 10 50",
      L"F-RCTR NOT\n  EVERY\n  10\n  50\n",
      L"EVERY F-RCTR NOT 10 50"
    },
    {
      L"EVERY F-RCTR NOT 10",
      L"F-RCTR NOT\n  EVERY\n  10\n",
      L"EVERY F-RCTR NOT 10"
    },
    {
      L"EVERY CTR 10",
      L"CTR\n  EVERY\n  10\n",
      L"EVERY CTR 10"
    },
    {
      L"EVERY CTR NOT 10",
      L"CTR NOT\n  EVERY\n  10\n",
      L"EVERY CTR NOT 10"
    },
    {
      L"EVERY F-CTR 10",
      L"F-CTR\n  EVERY\n  10\n",
      L"EVERY F-CTR 10"
    },
    {
      L"EVERY F-CTR NOT 10",
      L"F-CTR NOT\n  EVERY\n  10\n",
      L"EVERY F-CTR NOT 10"
    },
    {
      L"EVERY IMPRESSIONS 10",
      L"IMPRESSIONS\n  EVERY\n  10\n",
      L"EVERY IMPRESSIONS 10"
    },
    {
      L"EVERY IMPRESSIONS NOT 10",
      L"IMPRESSIONS NOT\n  EVERY\n  10\n",
      L"EVERY IMPRESSIONS NOT 10"
    },
    {
      L"EVERY F-IMPRESSIONS 10",
      L"F-IMPRESSIONS\n  EVERY\n  10\n",
      L"EVERY F-IMPRESSIONS 10"
    },
    {
      L"EVERY F-IMPRESSIONS NOT 10",
      L"F-IMPRESSIONS NOT\n  EVERY\n  10\n",
      L"EVERY F-IMPRESSIONS NOT 10"
    },
    {
      L"EVERY CLICKS 10",
      L"CLICKS\n  EVERY\n  10\n",
      L"EVERY CLICKS 10"
    },
    {
      L"EVERY CLICKS NOT 10",
      L"CLICKS NOT\n  EVERY\n  10\n",
      L"EVERY CLICKS NOT 10"
    },
    {
      L"EVERY F-CLICKS 10",
      L"F-CLICKS\n  EVERY\n  10\n",
      L"EVERY F-CLICKS 10"
    },
    {
      L"EVERY F-CLICKS NOT 10",
      L"F-CLICKS NOT\n  EVERY\n  10\n",
      L"EVERY F-CLICKS NOT 10"
    },
    {
      L"MSG 123 345 SIGNATURE 10 20",
      L"SIGNATURE\n  MSG 0000000000000123 0000000000000345\n  10\n  20\n",
      L"MSG 0000000000000123 0000000000000345 SIGNATURE 10 20"
    },
    {
      L"MSG 123 345 WITH IMAGE IMAGE",
      L"WITH\n  MSG 0000000000000123 0000000000000345\n  IMAGE\n  IMAGE\n",
      L"MSG 0000000000000123 0000000000000345 WITH IMAGE IMAGE"
    },
    {
      L"EVENT 123 345 CAPACITY 10",
      L"CAPACITY\n  EVENT 0000000000000123 0000000000000345\n  10\n",
      L"EVENT 0000000000000123 0000000000000345 CAPACITY 10"
    },
    {
      L"( URL s/1 a/2 OR EVENT 123 345 ) DOMAIN b",
      L"DOMAIN\n  EVENT 0000000000000123 0000000000000345\n  b\n",
      L"EVENT 0000000000000123 0000000000000345 DOMAIN b"
    },
    {
      L"( MSG 123 345 OR SITE a ) DOMAIN b",
      L"DOMAIN\n  MSG 0000000000000123 0000000000000345\n  b\n",
      L"MSG 0000000000000123 0000000000000345 DOMAIN b"
    },
    {
      L"( a SPACE news talk OR b OR ( c SPACE NOT ad ) ) "
      L"SPACE news",
      L"SPACE\n  OR\n    SPACE\n      ALL a\n      1\n    "
      L"ALL b\n    SPACE\n      ALL c\n      1\n  1\n",
      L"( a SPACE news ) OR ( b ) OR ( c SPACE news ) SPACE news"
    },
    {
      L"( a COUNTRY RUS BLR OR b OR ( c COUNTRY NOT ITA ) ) "
      L"COUNTRY RUS",
      L"COUNTRY\n  OR\n    COUNTRY\n      ALL a\n      RUS\n    "
      L"ALL b\n    COUNTRY\n      ALL c\n      RUS\n  RUS\n",
      L"( a COUNTRY RUS ) OR ( b ) OR ( c COUNTRY RUS ) COUNTRY RUS"
    },
    {
      L"( a LANGUAGE rus eng OR b OR ( c LANGUAGE NOT it ) ) "
      L"LANGUAGE rus",
      L"LANGUAGE\n  OR\n    LANGUAGE\n      ALL a\n      rus\n    "
      L"ALL b\n    LANGUAGE\n      ALL c\n      rus\n  rus\n",
      L"( a LANGUAGE rus ) OR ( b ) OR ( c LANGUAGE rus ) LANGUAGE rus"
    },
    {
      L"( a LANGUAGE rus eng OR ( b LANGUAGE por ) OR ( c LANGUAGE NOT it ) ) "
      L"LANGUAGE rus",
      L"OR\n  LANGUAGE\n    ALL a\n    rus\n  LANGUAGE\n    ALL c\n    rus\n",
      L"( a LANGUAGE rus ) OR ( c LANGUAGE rus )"
    },
    {
      L"( should OR x LANGUAGE NOT it LANGUAGE NOT snk ) LANGUAGE gaa",
      L"LANGUAGE\n  OR\n    ALL should\n    ALL x\n  gaa\n",
      L"( should ) OR ( x ) LANGUAGE gaa"
    },
    { L"obama LANGUAGE NOT rus eng LANGUAGE NOT rus por",
      L"LANGUAGE NOT\n  ALL obama\n  rus eng por\n",
      L"obama LANGUAGE NOT rus eng por"
    },
    {
      L"URL a/1 b/2 c/4 DATE 1D EXCEPT SITE y a x b",
      L"DATE\n  URL http://c/4\n  1D\n",
      L"URL http://c/4 DATE 1D"
    },
    {
      L"URL a/1 b/2 c/4 DATE 1D EXCEPT URL y a/1 x/3 b/2",
      L"DATE\n  URL http://c/4\n  1D\n",
      L"URL http://c/4 DATE 1D"
    },
    {
      L"URL a/1 b/2 DATE 1D EXCEPT URL y a/1 x/3 b/2",
      L"NONE\n",
      L"NONE"
    },
    {
      L"URL a/1 b/2 DATE 1D EXCEPT SITE y a x b",
      L"NONE\n",
      L"NONE"
    },
    {
      L"SITE x y z FETCHED 1D EXCEPT SITE y a x b",
      L"FETCHED\n  SITE z\n  1D\n",
      L"SITE z FETCHED 1D"
    },
    {
      L"SITE x y DATE 1D EXCEPT SITE y a x b",
      L"NONE\n",
      L"NONE"
    },
    {
      L"SITE x y EXCEPT obama EXCEPT SITE y a x b",
      L"NONE\n",
      L"NONE"
    },
    {
      L"( obama AND SITE x y ) EXCEPT SITE y a x b",
      L"NONE\n",
      L"NONE"
    },
    {
      L"( SITE a b OR SITE x y ) EXCEPT SITE y a x b",
      L"NONE\n",
      L"NONE"
    },
    {
      L"SITE a b EXCEPT SITE y a x b",
      L"NONE\n",
      L"NONE"
    },
    {
      L"SITE a b EXCEPT SITE a x",
      L"SITE b\n",
      L"SITE b"
    },
    {
      L"URL co.uk/a www.ru/b d.amazon.com/c DOMAIN NOT ru co.uk amazon.com",
      L"NONE\n",
      L"NONE"
    },
    {
      L"obama AND URL a.com/a n.com/b DOMAIN ru co.uk amazon.com",
      L"NONE\n",
      L"NONE"
    },
    {
      L"SITE co.uk www.ru d.amazon.com DOMAIN NOT ru co.uk amazon.com",
      L"NONE\n",
      L"NONE"
    },
    {
      L"obama AND SITE a.com n.com DOMAIN ru co.uk amazon.com",
      L"NONE\n",
      L"NONE"
    },
    {
      L"obama AND SITE a b AND ( URL x/q y/2 EXCEPT cat )",
      L"NONE\n",
      L"NONE"
    },
    {
      L"obama AND SITE a AND ( URL x/q DATE 1D )",
      L"NONE\n",
      L"NONE"
    },
    {
      L"obama AND SITE a AND ( SITE b AND car )",
      L"NONE\n",
      L"NONE"
    },
    { L"( \"Gonzalo Miranda\" ) LANGUAGE pt AND "
      L"URL http://www.obaoba.com.br/rss-magazines AND "
      L"( SITE hardwarezone.com razor.tv www.17.com.my "
      L"www.businesstimes.com.sg www.eh.com.my www.femalemag.com.my OR "
      L"URL http://blogs.straitstimes.com/feed/atom.xml "
      L"http://blogs.todayonline.com/forartssake/feed/ "
      L"http://blogs.todayonline.com/makankaki/feed/ "
      L"http://blogs.todayonline.com/ontheroad/feed/ )",
      L"NONE\n",
      L"NONE"
    },
    { L"( \"Universidade de Yoga-Unidade Kobrasol\" ) LANGUAGE pt AND "
      L"SITE esporteinterativo.terra.com.br AND "
      L"( SITE hardwarezone.com razor.tv www.17.com.my "
      L"www.businesstimes.com.sg www.eh.com.my www.femalemag.com.my OR "
      L"URL http://blogs.straitstimes.com/feed/atom.xml "
      L"http://blogs.todayonline.com/forartssake/feed/ "
      L"http://blogs.todayonline.com/makankaki/feed/ )",
      L"NONE\n",
      L"NONE"
    },
    { L"obama LANGUAGE NOT rus eng DOMAIN ru LANGUAGE NOT rus por",
      L"DOMAIN\n  LANGUAGE NOT\n    ALL obama\n    rus eng por\n  ru\n",
      L"obama LANGUAGE NOT rus eng por DOMAIN ru"
    },
    { L"obama LANGUAGE NOT rus eng DOMAIN ru LANGUAGE NOT eng rus por",
      L"DOMAIN\n  LANGUAGE NOT\n    ALL obama\n    rus eng por\n  ru\n",
      L"obama LANGUAGE NOT rus eng por DOMAIN ru"
    },
    { L"obama LANGUAGE NOT rus eng DOMAIN ru LANGUAGE rus por",
      L"DOMAIN\n  LANGUAGE\n    ALL obama\n    por\n  ru\n",
      L"obama LANGUAGE por DOMAIN ru"
    },
    { L"obama LANGUAGE NOT rus eng DOMAIN ru LANGUAGE rus",
      L"NONE\n",
      L"NONE"
    },
    { L"obama LANGUAGE rus LANGUAGE eng",
      L"NONE\n",
      L"NONE"
    },
    { L"obama LANGUAGE pt LANGUAGE en",
      L"NONE\n",
      L"NONE"
    },
    { L"obama LANGUAGE rus LANGUAGE NOT rus eng",
      L"NONE\n",
      L"NONE"
    },
    { L"obama LANGUAGE rus DOMAIN ru LANGUAGE NOT rus eng",
      L"NONE\n",
      L"NONE"
    },
    { L"CATEGORY a AND CATEGORY b",
      L"AND\n  CATEGORY \"/a/\"\n  CATEGORY \"/b/\"\n",
      L"( CATEGORY \"/a/\" ) AND ( CATEGORY \"/b/\" )"
    },
    { L"CATEGORY \"a b\" AND CATEGORY b",
      L"AND\n  CATEGORY \"/a b/\"\n  CATEGORY \"/b/\"\n",
      L"( CATEGORY \"/a b/\" ) AND ( CATEGORY \"/b/\" )"
    },
    { L"obama AND EVERY",
      L"ALL obama\n",
      L"obama"
    },
    { L"EVERY AND obama",
      L"ALL obama\n",
      L"obama"
    },
    { L"EVERY AND obama OR car AND EVERY",
      L"OR\n  ALL obama\n  ALL car\n",
      L"( obama ) OR ( car )"    },
    { L"obama AND SITE a.com AND URL http://b.com/rss/feed",
      L"NONE\n",
      L"NONE"
    },
    { L"( \"Jorge de Lima\" ) LANGUAGE pt AND SITE abril.fp.oix.net AND ( SITE abril.fp.oix.net )",
      L"AND\n  LANGUAGE\n    ALL \"jorge de lima\"\n    por\n  SITE abril.fp.oix.net\n",
      L"( \"jorge de lima\" LANGUAGE por ) AND ( SITE abril.fp.oix.net )"
    },
    {
      L"ANY 1 3 obama",
      L"ANY \"3\" obama\n",
      L"ANY \"3\" obama"
    },
    {
      L"ANY 2 3 obama",
      L"ANY 2 3 obama\n",
      L"ANY 2 3 obama"
    },
    {
      L"ANY CORE 3 obama",
      L"ANY 3 CORE obama\n",
      L"ANY 3 CORE obama"
    },
    {
      L"ANY 1 CORE 3 obama",
      L"ANY CORE \"3\" obama\n",
      L"ANY CORE \"3\" obama"
    },
    {
      L"ANY 2 CORE 3 obama",
      L"ANY 2 CORE 3 obama\n",
      L"ANY 2 CORE 3 obama"
    },
    {
      L"ANY CORE 2 3 obama",
      L"ANY 2 CORE 3 obama\n",
      L"ANY 2 CORE 3 obama"
    },
    { L"obama AND SITE a.com AND SITE b.com",
      L"NONE\n",
      L"NONE"
    },
    { L"( \"Matthew Broderick\" AND ANY Celebridade Celebridades famoso famosos ) LANGUAGE pt AND URL http://www.obaoba.com.br/rss-magazines AND ( SITE www.dailyexpress.co.uk ) ",
      L"NONE\n",
      L"NONE"
    },
    { L"( \"Praia Galheta\" ) LANGUAGE pt AND SITE uol.fp.oix.net AND ( SITE www.dailyexpress.co.uk ) ",
      L"NONE\n",
      L"NONE"
    },
    { L"SITE b c AND URL a/1 x/1",
      L"NONE\n",
      L"NONE"
    },
    { L"SITE a b AND URL a/1 x/1",
      L"URL http://a/1\n", L"URL http://a/1"
    },
    { L"SITE c b AND SITE a x",
      L"NONE\n",
      L"NONE"    },
    { L"SITE c b AND SITE a b",
      L"SITE b\n",
      L"SITE b"
    },
    { L"URL a/1 x/1 AND URL y/1 a/1",
      L"URL http://a/1\n",
      L"URL http://a/1"    },
    { L"URL a/1 x/1 AND SITE a b",
      L"URL http://a/1\n",
      L"URL http://a/1"
    },
    { L"URL a b c AND obama AND URL x y z OR car",
      L"ALL car\n",
      L"car"
    },
    { L"NONE AND obama",
      L"NONE\n",
      L"NONE"
    },
    { L"obama AND NONE",
      L"NONE\n",
      L"NONE"
    },
    { L"obama OR NONE",
      L"ALL obama\n",
      L"obama"
    },
    { L"putin AND ( NONE COUNTRY rus EXCEPT abc ) OR obama EXCEPT NONE",
      L"ALL obama\n",
      L"obama"
    },
    { L" 15\" \"13\". 1\"1 14\"\" A\"B",
      L"ALL 15 \"13 1\"1 14\" a\"b\n",
      L"15 \"13 1\"1 14\" a\"b"
    },
    { L"A SPACE news talk",
      L"SPACE\n  ALL a\n  1 2\n",
      L"a SPACE news talk"
    },
    { L"\"y\"",
      L"ALL \"y\"\n",
      L"\"y\""
    },
    { L" 13 \"14\" b",
      L"ALL 13 \"14\" b\n",
      L"13 \"14\" b"
    },
    { L" '13' \"14\" a",
      L"ALL 13 \"14\" a\n",
      L"13 \"14\" a"
    },
    { L" \"13\" \"14\" a",
      L"ALL \"13\" \"14\" a\n",
      L"\"13\" \"14\" a"
    },
    { L" \"13 14\" a",
      L"ALL \"13 14\" a\n",
      L"\"13 14\" a"
    },
    { L"ANY ' middle  east '",
      L"ANY 'middle east'\n",
      L"ANY 'middle east'"
    },
    { L"war bin",
      L"ALL war bin\n",
      L"war bin"
    },
    { L"ALL middle east COUNTRY \"Albania\" US OR ( electronics COUNTRY NOT china )",
      L"OR\n  COUNTRY\n    ALL middle east\n    ALB USA\n  COUNTRY NOT\n    ALL electronics\n    CHN\n",
      L"( middle east COUNTRY ALB USA ) OR ( electronics COUNTRY NOT CHN )"
    },
    { L"ipod LANGUAGE \"En\" French OR electronics LANGUAGE NOT chn indic german",
      L"LANGUAGE NOT\n  OR\n    LANGUAGE\n      ALL ipod\n      eng fre\n    ALL electronics\n  chn inc ger\n",
      L"( ipod LANGUAGE eng fre ) OR ( electronics ) LANGUAGE NOT chn inc ger"
    },
    { L"SITE www.perl.co.uk EXCEPT URL \"http://www.gnu.org/index.html\" www.perl.com/doc/main+page.htm EXCEPT \"war\"",
      L"EXCEPT\n  EXCEPT\n    SITE www.perl.co.uk\n    URL http://www.gnu.org/index.html http://www.perl.com/doc/main+page.htm\n  ALL \"war\"\n",
      L"( ( SITE www.perl.co.uk ) EXCEPT ( URL http://www.gnu.org/index.html http://www.perl.com/doc/main+page.htm ) ) EXCEPT ( \"war\" )"
    },
    { L"SITE \"www.gnu.org\" www.perl.com OR URL \"http://www.gnu.org\" EXCEPT \"war\"",
      L"OR\n  SITE www.gnu.org www.perl.com\n  EXCEPT\n    URL http://www.gnu.org/\n    ALL \"war\"\n",
      L"( SITE www.gnu.org www.perl.com ) OR ( ( URL http://www.gnu.org/ ) EXCEPT ( \"war\" ) )"
    },
    { L"east DATE \"03D\" DOMAIN NOT \"co.uk\" info biz  LANGUAGE \"En\" DOMAIN uk com",
      L"DOMAIN\n  LANGUAGE\n    DOMAIN NOT\n      DATE\n        ALL east\n        3D\n      co.uk info biz\n    eng\n  uk com\n",
      L"east DATE 3D DOMAIN NOT co.uk info biz LANGUAGE eng DOMAIN uk com"
    },
    { L"middle: \"east  west\", war\"",
      L"ALL middle \"east west war\"\n",
      L"middle \"east west war\""
    },
    { L"ANY middle \"OR\" east",
      L"ANY middle \"or\" east\n",
      L"ANY middle \"or\" east"
    },
    { L" \"aa, b:b ? :: cc\" .xx; - ?? yy zz! OR - 1.1.. \".\" \".45 67-\"",
      L"OR\n  ALL \"aa b:b cc\" xx yy zz\n  ALL 1.1 \"45 67\"\n",
      L"( \"aa b:b cc\" xx yy zz ) OR ( 1.1 \"45 67\" )"
    },
    { L"SITE \"www.gnu.org\" OR URL \"http://www.gnu.org\" EXCEPT \"war\"",
      L"OR\n  SITE www.gnu.org\n  EXCEPT\n    URL http://www.gnu.org/\n    ALL \"war\"\n",
      L"( SITE www.gnu.org ) OR ( ( URL http://www.gnu.org/ ) EXCEPT ( \"war\" ) )"
    },
    { L"ANY oo \"very  mery \" \" middle\" ee \" east  west\"",
      L"ANY oo \"very mery\" \"middle\" ee \"east west\"\n",
      L"ANY oo \"very mery\" \"middle\" ee \"east west\""
    },
    { L"ANY oo \"very mery \" \" middle far far\" ee \"east  west\"",
      L"ANY oo \"very mery\" \"middle far far\" ee \"east west\"\n",
      L"ANY oo \"very mery\" \"middle far far\" ee \"east west\""
    },
    { L"ANY \"middle east\"",
      L"ANY \"middle east\"\n",
      L"ANY \"middle east\""
    },
    { L"ANY \" middle  east \"",
      L"ANY \"middle east\"\n",
      L"ANY \"middle east\""
    },
    { L"( middle east OR iraq war ) DOMAIN ru OR ( far west OR war and peace DOMAIN NOT yahoo.com )",
      L"OR\n  DOMAIN\n    OR\n      ALL middle east\n      ALL iraq war\n    ru\n  DOMAIN NOT\n    OR\n      ALL far west\n      ALL war and peace\n    yahoo.com\n",
      L"( ( middle east ) OR ( iraq war ) DOMAIN ru ) OR ( ( far west ) OR ( war and peace ) DOMAIN NOT yahoo.com )"
    },
    { L"middle east AND iraq war LANGUAGE arabic OR ( far west EXCEPT war and peace LANGUAGE NOT english )",
      L"OR\n  LANGUAGE\n    AND\n      ALL middle east\n      ALL iraq war\n    ara\n  LANGUAGE NOT\n    EXCEPT\n      ALL far west\n      ALL war and peace\n    eng\n",
      L"( ( middle east ) AND ( iraq war ) LANGUAGE ara ) OR ( ( far west ) EXCEPT ( war and peace ) LANGUAGE NOT eng )"
    },
    { L"middle east OR iraq war DATE 2D OR ( far west OR war and peace DATE BEFORE 10D )",
      L"OR\n  DATE\n    OR\n      ALL middle east\n      ALL iraq war\n    2D\n  DATE BEFORE\n    OR\n      ALL far west\n      ALL war and peace\n    10D\n",
      L"( ( middle east ) OR ( iraq war ) DATE 2D ) OR ( ( far west ) OR ( war and peace ) DATE BEFORE 10D )"
    },
    { L"ALL middle east OR iraq war AND Saddam EXCEPT 1991 OR Hussein EXCEPT president",
      L"OR\n  ALL middle east\n  EXCEPT\n    AND\n      ALL iraq war\n      ALL saddam\n    ALL 1991\n  EXCEPT\n    ALL hussein\n    ALL president\n",
      L"( middle east ) OR ( ( ( iraq war ) AND ( saddam ) ) EXCEPT ( 1991 ) ) OR ( ( hussein ) EXCEPT ( president ) )"
    },
    { L"ALL middle east OR iraq war AND Saddam EXCEPT 1991 AND Hussein EXCEPT president",
      L"OR\n  ALL middle east\n  EXCEPT\n    AND\n      EXCEPT\n        AND\n          ALL iraq war\n          ALL saddam\n        ALL 1991\n      ALL hussein\n    ALL president\n",
      L"( middle east ) OR ( ( ( ( ( iraq war ) AND ( saddam ) ) EXCEPT ( 1991 ) ) AND ( hussein ) ) EXCEPT ( president ) )"
    },
    { L"ALL middle east OR iraq war EXCEPT 1991",
      L"OR\n  ALL middle east\n  EXCEPT\n    ALL iraq war\n    ALL 1991\n",
      L"( middle east ) OR ( ( iraq war ) EXCEPT ( 1991 ) )"
    },
    { L"ALL middle east OR URL rss.yahoo.com/middle/east.xml OR SITE rss.middle.east",
      L"OR\n  ALL middle east\n  URL http://rss.yahoo.com/middle/east.xml\n  SITE rss.middle.east\n",
      L"( middle east ) OR ( URL http://rss.yahoo.com/middle/east.xml ) OR ( SITE rss.middle.east )"
    },
    { L"( ( ANY middle east ) AND ( ( iraq war OR other war ) AND victory ) )",
      L"AND\n  ANY middle east\n  AND\n    OR\n      ALL iraq war\n      ALL other war\n    ALL victory\n",
      L"( ANY middle east ) AND ( ( ( iraq war ) OR ( other war ) ) AND ( victory ) )"
    },
    { L"ANY middle east AND ( ( iraq war OR other war ) AND victory )",
      L"AND\n  ANY middle east\n  AND\n    OR\n      ALL iraq war\n      ALL other war\n    ALL victory\n",
      L"( ANY middle east ) AND ( ( ( iraq war ) OR ( other war ) ) AND ( victory ) )"
    },
    { L"ANY middle east AND ( iraq war OR other war )",
      L"AND\n  ANY middle east\n  OR\n    ALL iraq war\n    ALL other war\n",
      L"( ANY middle east ) AND ( ( iraq war ) OR ( other war ) )"
    },
    { L"( iraq war OR other war ) AND ANY middle east",
      L"AND\n  OR\n    ALL iraq war\n    ALL other war\n  ANY middle east\n",
      L"( ( iraq war ) OR ( other war ) ) AND ( ANY middle east )"
    },
    { L"ANY middle east AND iraq war OR other war",
      L"OR\n  AND\n    ANY middle east\n    ALL iraq war\n  ALL other war\n",
      L"( ( ANY middle east ) AND ( iraq war ) ) OR ( other war )"
    },
    { L"other war OR ANY middle east AND iraq war",
      L"OR\n  ALL other war\n  AND\n    ANY middle east\n    ALL iraq war\n",
      L"( other war ) OR ( ( ANY middle east ) AND ( iraq war ) )"
    },
    { L"ANY middle east AND iraq war",
      L"AND\n  ANY middle east\n  ALL iraq war\n",
      L"( ANY middle east ) AND ( iraq war )"
    },
    { L"ANY middle east OR iraq war",
      L"OR\n  ANY middle east\n  ALL iraq war\n",
      L"( ANY middle east ) OR ( iraq war )"
    },
    { L"east",
      L"ALL east\n",
      L"east"
    },
    { L"middle east",
      L"ALL middle east\n",
      L"middle east"
    },
    { L"ALL middle east",
      L"ALL middle east\n",
      L"middle east"
    },
    { L"ANY middle east",
      L"ANY middle east\n",
      L"ANY middle east"
    },
    { L"EVERY FETCHED 20",
      L"FETCHED\n  EVERY\n  1970-01-01.00:00:20\n",
      L"EVERY FETCHED 1970-01-01.00:00:20"
    },
    { L"EVERY FETCHED 50D",
      L"FETCHED\n  EVERY\n  50D\n",
      L"EVERY FETCHED 50D"
    },
  };
  
}

using namespace NewsGate;

class IdSet : public google::dense_hash_set<Message::Id,
                                            Message::MessageIdHash>
{
public:
  IdSet() throw(El::Exception);
  IdSet(unsigned long size) throw(El::Exception);
};

//
// IdSet class
//

IdSet::IdSet() throw(El::Exception)
{
  set_empty_key(Message::Id::zero);
  set_deleted_key(Message::Id::nonexistent);
}
    
IdSet::IdSet(unsigned long size) throw(El::Exception)
    : google::dense_hash_set<Message::Id, Message::MessageIdHash>(size)
{
  set_empty_key(Message::Id::zero);
  set_deleted_key(Message::Id::nonexistent);
}

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
      else if(!strcmp(arg, "-positions"))
      {
        positions_ = true;
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
      else if(!strncmp("-msg-count=", arg, 11))
      {
        if(!El::String::Manip::numeric(arg + 11, message_count_))
        {
          throw Exception("Application::run: number expected for "
                          "-msg-count argument value");
        }
      }
      else if(!strncmp("-result-count=", arg, 14))
      {
        if(!El::String::Manip::numeric(arg + 14, result_count_))
        {
          throw Exception("Application::run: number expected for "
                          "-result-count argument value");
        }
      }
      else if(!strncmp("-search-count=", arg, 14))
      {
        if(!El::String::Manip::numeric(arg + 14, search_count_))
        {
          throw Exception("Application::run: number expected for "
                          "-search-count argument value");
        }
      }
      else if(!strncmp("-msg-file=", arg, 10))
      {
        if(source_text_.in() == 0)
        {
          source_text_ = new NewsGate::Test::SourceText();
        }

        std::string filename(arg + 10);
        
        std::fstream file(filename.c_str(), std::ios::in);

        if(!file.is_open())
        {
          std::ostringstream ostr;
          ostr << "Application::run: failed to open file " << filename;
          
          throw Exception(ostr.str());
        }

        source_text_->load(file);
      }
      else if(!strncmp("-url-file=", arg, 10))
      {
        if(source_urls_.in() == 0)
        {
          source_urls_ = new NewsGate::Test::SourceUrls();
        }

        std::string filename(arg + 10);
        
        std::fstream file(filename.c_str(), std::ios::in);

        if(!file.is_open())
        {
          std::ostringstream ostr;
          ostr << "Application::run: failed to open file " << filename;
          
          throw Exception(ostr.str());
        }

        source_urls_->load(file);
      }
      else if(!strcmp(arg, "-pause"))
      {
        pauses_ = true;
      }
      else if(!strncmp("-dict=", arg, 6))
      {
        std::string filename(arg + 6);

        std::cerr << "Loading dictionary from " << filename << " ...\n";
        word_info_manager_.load(filename.c_str(), &std::cerr);
      }
      else
      {
        std::ostringstream ostr;
        ostr << "Application::run: unexpected argument " << arg;
        throw Exception(ostr.str());
      }
    }

    if((source_text_.in() == 0 || source_text_->size() == 0) !=
       (source_urls_.in() == 0 || source_urls_->size() == 0))
    {
      std::ostringstream ostr;
      ostr << "Application::run: url and message sets should be "
        "both non empty";
      
      throw Exception(ostr.str());      
    }

    srand(randomize_ ? time(0) : 0);

    test_compilation_positive();
    test_compilation_negative();
    test_search();

    if(source_text_.in() != 0 && source_text_->size() != 0)
    {
      test_performance();
    }
    
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
Application::test_compilation_positive() throw(El::Exception)
{
  if(word_info_manager_.is_loaded(El::Lang::EC_ENG))
  {
    for(unsigned long i = 0;
        i < sizeof(POSITIVE_PARSE_NORM_SAMPLES) /
          sizeof(POSITIVE_PARSE_NORM_SAMPLES[0]); i++)
    {
      test_compilation_positive(POSITIVE_PARSE_NORM_SAMPLES[i]);
    }
  }
  
  for(unsigned long i = 0;
      i < sizeof(POSITIVE_PARSE_SAMPLES) /
        sizeof(POSITIVE_PARSE_SAMPLES[0]); i++)
  {
    test_compilation_positive(POSITIVE_PARSE_SAMPLES[i]);
  }
}

void 
Application::test_compilation_positive(
  const Application::PositiveParseSample& sample)
  throw(El::Exception)
{
  const wchar_t* source = sample.source;
      
  std::wistringstream istr(source);

  NewsGate::Search::ExpressionParser parser;
  
  try
  {
    parser.parse(istr);
  }
  catch(const El::Exception& e)
  {
    std::wstring error;
    El::String::Manip::utf8_to_wchar(e.what(), error);
        
    std::wostringstream ostr;
    ostr << L"test_positive: parsing failed for '"
         << source << L"'. Description:\n" << error;

    {
      std::string error;
      El::String::Manip::wchar_to_utf8(ostr.str().c_str(), error);

      throw Exception(error);
    }
        
  }

  parser.optimize();
  parser.expression()->normalize(word_info_manager_);

  std::wostringstream ostr;
  parser.expression()->dump(ostr);

  std::wstring compiled(ostr.str());
      
  if(verbose_)
  {
    std::wcout << L"\nSource: " << source << std::endl;
    std::cout << "Compiled:\n";
    std::wcout << compiled;
  }

  const wchar_t* expected = sample.compiled;
      
  if(compiled != expected)
  {
    std::wostringstream ostr;
    ostr << L"test_positive: for '" << source
         << L"'\nunexpected compilation result \n'" << compiled
         << L"'\ninstead of \n'" << expected << "'";

    std::string error;
    El::String::Manip::wchar_to_utf8(ostr.str().c_str(), error);

    throw Exception(error);
  }

  const wchar_t* printed = sample.printed;
    
  if(*printed != L'\0')
  {
    std::wstring printed_w;
      
    {
      std::ostringstream ostr;
      parser.expression()->condition->print(ostr);

      El::String::Manip::utf8_to_wchar(ostr.str().c_str(), printed_w);
    }
      
    if(printed_w != printed)
    {
      std::wostringstream ostr;
        
      ostr << L"test_positive: for '" << source
           << L"'\ncompiled: " << compiled
           << L"unexpected print result \n'" << printed_w
           << L"'\ninstead of \n'" << printed << "'";

      std::string error;
      El::String::Manip::wchar_to_utf8(ostr.str().c_str(), error);

      throw Exception(error);        
    }

    std::wistringstream istr(printed);

    try
    {
      parser.parse(istr);
    }
    catch(const El::Exception& e)
    {
      std::wstring error;
      El::String::Manip::utf8_to_wchar(e.what(), error);
        
      std::wostringstream ostr;
      ostr << L"test_positive: parsing failed for printed '"
           << printed_w << L"'; source '" << source << L"'. Description:\n"
           << error;

      {
        std::string error;
        El::String::Manip::wchar_to_utf8(ostr.str().c_str(), error);

        throw Exception(error);
      }        
    }

    parser.expression()->normalize(word_info_manager_);
      
    std::wstring recompiled;
    {
        
      std::wostringstream ostr;
      parser.expression()->dump(ostr);
      recompiled = ostr.str();
    }

    if(recompiled != compiled)
    {
      std::wostringstream ostr;
        
      ostr << L"test_positive: for '" << source
           << L"'\nunexpected recompilation result:\n" << recompiled
           << L"instead of:\n" << compiled
           << L"print result\n'" << printed_w << "'";
        
      std::string error;
      El::String::Manip::wchar_to_utf8(ostr.str().c_str(), error);

      throw Exception(error);
    }
      
  }
}

void 
Application::test_compilation_negative() throw(El::Exception)
{
  NewsGate::Search::ExpressionParser parser;

  for(unsigned long i = 0;
      i < sizeof(NEGATIVE_PARSE_SAMPLES) / sizeof(NEGATIVE_PARSE_SAMPLES[0]);
      i++)
  {
    const wchar_t* source = NEGATIVE_PARSE_SAMPLES[i].source;
    unsigned long position = NEGATIVE_PARSE_SAMPLES[i].position;
    NewsGate::Search::ParseError::Code code = NEGATIVE_PARSE_SAMPLES[i].code;

    try
    {  
      std::wistringstream istr(source);
      parser.parse(istr);
    }
    catch(const NewsGate::Search::ParseError& e)
    {
      if(e.code != code)
      {
        std::wostringstream ostr;
        ostr << L"test_negative: for '" << source
             << L"'\nunexpected error code " << e.code
             << L"'\ninstead of " << code;

        std::string error;
        El::String::Manip::wchar_to_utf8(ostr.str().c_str(), error);

        throw Exception(error);
      }
        
      if(e.position != position)
      {
        std::wostringstream ostr;
        ostr << L"test_negative: for '" << source
             << L"'\nunexpected error position " << e.position
             << L"'\ninstead of " << position;

        std::string error;
        El::String::Manip::wchar_to_utf8(ostr.str().c_str(), error);

        throw Exception(error);
      }
        
      continue;
    }

    parser.expression()->normalize(word_info_manager_);    

    std::wostringstream ostr;
    ostr << L"test_negative: for '" << source
         << L"'\nunexpectedly no error";

    std::string error;
    El::String::Manip::wchar_to_utf8(ostr.str().c_str(), error);

    throw Exception(error);
  }  
}

void 
Application::test_search() throw(El::Exception)
{
  EvaluateSampleList samples;

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"ANY 'A' A";

    sample.messages.push_back(
      Msg("A B", Message::Id(1)));
  
    sample.messages.push_back(
      Msg("Y X", Message::Id(2)));

    sample.search_result.push_back(ExpectedResult(Message::Id(1), "1"));
  }  

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"ALL DESCRIPTION Y";

    sample.messages.push_back(
      Msg("X Y A Y Z", Message::Id(1)));
  
    sample.messages.push_back(
      Msg("Y Z Z Y YY", Message::Id(2)));

    sample.search_result.push_back(ExpectedResult(Message::Id(1), "2 4"));
    sample.search_result.push_back(ExpectedResult(Message::Id(2), "1 4"));
  }
  
  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"\"Y Y Z\"";

    sample.messages.push_back(
      Msg("X Y Y Y Z", Message::Id(1)));
  
    sample.messages.push_back(
      Msg("Y Y Y Z", Message::Id(2)));

    sample.messages.push_back(
      Msg("Y Y Z M N R", Message::Id(3)));
    
    sample.search_result.push_back(ExpectedResult(Message::Id(1), "3 4 5"));
    sample.search_result.push_back(ExpectedResult(Message::Id(2), "2 3 4"));
    sample.search_result.push_back(ExpectedResult(Message::Id(3), "1 2 3"));
  }
  
  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression =
      L"ANY X C";
  
    sample.messages.push_back(
      Msg("C", Message::Id(1)));
  
    sample.messages.push_back(
      Msg("X", Message::Id(2)));

    sample.search_result.push_back(ExpectedResult(Message::Id(1), "1"));
    sample.search_result.push_back(ExpectedResult(Message::Id(2), "1"));
  }
  
  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression =
      L"ZZ AND ( NONE COUNTRY rus EXCEPT abc ) OR ANY C X EXCEPT NONE";
  
    sample.messages.push_back(
      Msg("C", Message::Id(1)));
  
    sample.messages.push_back(
      Msg("X", Message::Id(2)));

    sample.search_result.push_back(ExpectedResult(Message::Id(1), "1"));
    sample.search_result.push_back(ExpectedResult(Message::Id(2), "1"));
  }
  
  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.strategy.suppression.reset(
      new Search::Strategy::CollapseEvents(75, 90, 4));    

    sample.expression = L"AAA";

    unsigned long time = ACE_OS::gettimeofday().sec() - 41111;

    Msg msg1("AAA BBB CCC DDD XXX MMM",
              Message::Id(1),
              "http://www.auto.ru",
              time);

    sample.messages.push_back(msg1);

    Msg msg2("AAA BBB CCC DDD YYY NNN",
            Message::Id(2),
            "http://www.auto.com",
            time - 1000);

    sample.messages.push_back(msg2);
    
    sample.search_result.push_back(ExpectedResult(Message::Id(1), "1"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression =
      L"URL http://212.48.137.54/pop/1.0.43.0/general/vislist.xsql?id=1&pwd=anton&host=212.48.137.54&page=/&param=&total=16 http://www.envolo-dev.com:81/bugzilla/show_bug.cgi?id=63 EXCEPT  ALL  1history the 1requirement DOMAIN ru microsoft.com rbc.ru org ru 54  OR  ANY ""  variable 1got DOMAIN NOT 48.137.39 rbc.ru ru hotmail.msn.com ";
  
    sample.messages.push_back(
      Msg("",
          Message::Id(1),
          "http://212.48.137.54/pop/1.0.43.0/general/vislist.xsql?id=1&pwd=anton&host=212.48.137.54&page=/&param=&total=16",
          1291442330,
          El::Lang("ale")
          ));  
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression =
      L"SITE 212.48.137.54 DOMAIN NOT 137.54";
  
    sample.messages.push_back(
      Msg("Invoking",
          Message::Id(1),
          "http://212.48.137.54/pop/1.0.54.0/reglog/login.xsql?email=anton@ipmce.ru&pwd=anton",
          1291442330,
          El::Lang("ale")
          ));
  
    sample.search_result.push_back(ExpectedResult(Message::Id(1), ""));
  }
  
  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression =
      L"( should OR x "
      L"LANGUAGE NOT it LANGUAGE NOT snk )  "
      L"LANGUAGE gaa";
  
    sample.messages.push_back(
      Msg("should be",
          Message::Id(1),
          "http://aaa",
          1290913344,
          El::Lang("gaa")
          ));
  
    sample.search_result.push_back(ExpectedResult(Message::Id(1), "1"));
  }
  
  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"ANY C X";
  
    sample.messages.push_back(
      Msg("C", Message::Id(1)));
  
    sample.messages.push_back(
      Msg("X", Message::Id(2)));

    sample.search_result.push_back(ExpectedResult(Message::Id(1), "1"));
    sample.search_result.push_back(ExpectedResult(Message::Id(2), "1"));
  }
  
  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"( A AND B OR C EXCEPT D ) EXCEPT ( A OR X )";
  
    sample.messages.push_back(
      Msg("A B C D", Message::Id(1)));
  
    sample.messages.push_back(
      Msg("X C", Message::Id(2)));
   
    sample.messages.push_back(
      Msg("C D", Message::Id(3)));
   
    sample.messages.push_back(
      Msg("C L K", Message::Id(4)));
   
    sample.search_result.push_back(ExpectedResult(Message::Id(4), "1"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"ANY 2 expressions LANGUAGE rus";
  
    sample.messages.push_back(
      Msg("expressions one",
          Message::Id(1)));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"ANY 2 'space shuttle' atlantis astronaut";
  
    sample.messages.push_back(
      Msg("space shuttle",
          Message::Id(1)));

    sample.messages.push_back(
      Msg("space shuttle atlantis",
          Message::Id(2)));
    
    sample.messages.push_back(
      Msg("space shuttle abc xez space shuttle",
          Message::Id(3)));

    sample.search_result.push_back(ExpectedResult(Message::Id(2), "1 2 3"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"ANY 2 nasa space shuttle car";
  
    sample.messages.push_back(
      Msg("space",
          Message::Id(1)));

    sample.messages.push_back(
      Msg("NASA set February 7 on a space shuttle",
          Message::Id(2)));
    
    sample.search_result.push_back(ExpectedResult(Message::Id(2), "1 7 8"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"ANY \"3\" foot car \"airplane\" train bike";
  
    sample.messages.push_back(
      Msg("like to 3",
          Message::Id(1)));
    
    sample.messages.push_back(
      Msg("like to go by cars airplanes but not by train",
          Message::Id(2)));
    
    sample.search_result.push_back(ExpectedResult(Message::Id(1), "3"));
    
    sample.search_result.push_back(
      ExpectedResult(Message::Id(2),
                     word_info_manager_.is_loaded(El::Lang::EC_ENG) ?
                     "5 10" : "10"));    
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"ANY foot car 3 \"airplane\" train bike";
  
    sample.messages.push_back(
      Msg("like to 3",
          Message::Id(1)));
    
    sample.messages.push_back(
      Msg("like to go by cars airplanes but not by train",
          Message::Id(2)));
    
    sample.search_result.push_back(ExpectedResult(Message::Id(1), "3"));
    
    sample.search_result.push_back(
      ExpectedResult(Message::Id(2),
                     word_info_manager_.is_loaded(El::Lang::EC_ENG) ?
                     "5 10" : "10"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"ANY 3 foot car \"airplane\" train bike";
  
    sample.messages.push_back(
      Msg("like to go by cars airplane but not by train",
          Message::Id(1)));

    sample.messages.push_back(
      Msg("like to go by car airplane but not by train",
          Message::Id(2)));

    sample.messages.push_back(
      Msg("like to go by cars airplanes but not by train",
          Message::Id(3)));
     
    if(word_info_manager_.is_loaded(El::Lang::EC_ENG))
    {
      sample.search_result.push_back(ExpectedResult(Message::Id(1), "5 6 10"));
    }

    sample.search_result.push_back(ExpectedResult(Message::Id(2), "5 6 10"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"'militants urge'";
  
    sample.messages.push_back(
      Msg("al Qaeda militant urged Islamist militants",
          Message::Id(1)));
    
    sample.messages.push_back(
      Msg("al Qaeda militants urge Obama",
          Message::Id(2)));
    
    if(word_info_manager_.is_loaded(El::Lang::EC_ENG))
    {
      sample.search_result.push_back(ExpectedResult(Message::Id(1), "3 4"));
    }
    
    sample.search_result.push_back(ExpectedResult(Message::Id(2), "3 4"));    
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.strategy.suppression.reset(
      new Search::Strategy::CollapseEvents(75, 90, 4));
    
    sample.expression = L"ramirez began";
  
    sample.messages.push_back(
      Msg("rosa maria ramirez began look for something 18 months",
          Message::Id(1),
          0,
          0,
          El::Lang::null,
          10,
          2));
    
    sample.messages.push_back(
      Msg("ramirez began look for rosa ramrez 18 months",
          Message::Id(2),
          0,
          0,
          El::Lang::null,
          10,
          2));

    sample.search_result.push_back(ExpectedResult(Message::Id(1), "3 4"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"\"ramirez began\"";
  
    sample.messages.push_back(
      Msg("rosa ramirez began look for rosa ramrez 18 months",
          Message::Id(1)));
    
    sample.search_result.push_back(ExpectedResult(Message::Id(1), "2 3"));    
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"ANY by DATE BEFORE 6D OR ALL  \" the request\"";

    unsigned long cur_time = ACE_OS::gettimeofday().sec();

    Msg msg("by the request",
            Message::Id(1),
            "http://www.auto.ru/wwwboards/gai/0487/145559.shtml",
            cur_time,
            El::Lang("nwc"));

    sample.messages.push_back(msg);

    sample.search_result.push_back(ExpectedResult(Message::Id(1), "2 3"));
  }
  
  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"SITE 212.48.137.61";

    sample.messages.push_back(
      Msg("A C D", Message::Id(1), "212.48.137.61/X"));
    
    sample.search_result.push_back(ExpectedResult(Message::Id(1), ""));
  }
  
  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"ANY long \"middle east\" war";
  
    sample.messages.push_back(
      Msg("long middle east west war", Message::Id(1)));
    
    sample.search_result.push_back(ExpectedResult(Message::Id(1),
                                                  "1 2 3 5"));    
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"\"middle west\" war";
  
    sample.messages.push_back(
      Msg("middle way east war with middle west", Message::Id(2)));
   
    sample.search_result.push_back(
      ExpectedResult(Message::Id(2), "4 6 7"));
  }
  
  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"SITE www.passionpartiesbyrae.com www.citycat.ru www.rambler.ru labunion.agava.ru win.mail.ru www.strana.ru ikea.ru www.gazeta.ru www.auto.ru www.rambler.ru OR  ALL  \" o\" EXCEPT  ANY  rule \" may be \" used AND  SITE www.auto.ru www.utro.ru corp.peopleonpage.com www.rambler.ru www.cat-scan.com www.webboard.ru www.byttehnika.ru www.tours.ru.";
  
    sample.messages.push_back(
      Msg("utils.o o", Message::Id(1), "www.auto.ru"));
  
    sample.search_result.push_back(ExpectedResult(Message::Id(1), "2"));
  }
  
  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"SITE www.auto.ru OR o EXCEPT SITE www.auto.ru";
  
    sample.messages.push_back(
      Msg("o utils.o", Message::Id(1), "www.auto.ru"));
  
    sample.search_result.push_back(ExpectedResult(Message::Id(1), ""));
  }
  
  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"file OR a AND all";
  
    sample.messages.push_back(
      Msg("in a file name", Message::Id(1)));
  
    sample.search_result.push_back(ExpectedResult(Message::Id(1), "3"));
  }
  
  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression =
      L"ALL  hints OR  ALL  \" the\" OR  ALL  \" force\" EXCEPT  ALL  to";
//    sample.expression = L"the OR force EXCEPT to";
  
    sample.messages.push_back(
      Msg("To force the", Message::Id(1)));
  
    sample.search_result.push_back(ExpectedResult(Message::Id(1), "3"));
  }
  
  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"ALL  the OR  ALL  \" help' \" whereas EXCEPT  "
      "ALL  the OR  ALL  \" to \" invoke \" the \" program";
    
//    sample.expression = L"the OR a EXCEPT the";
  
    sample.messages.push_back(
      Msg("The mode line", Message::Id(1)));
  
    sample.search_result.push_back(ExpectedResult(Message::Id(1), "1"));
  }
  
  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"( A OR B ) DOMAIN com DOMAIN NOT google.com";
  
    sample.messages.push_back(
      Msg("A C D", Message::Id(1), "www.google.ru/X"));
  
    sample.messages.push_back(
      Msg("A B C",
          Message::Id(2),
          "www.google.ru.com",
          ACE_OS::gettimeofday().sec() - 100000));

    sample.messages.push_back(
      Msg("X Y Z", Message::Id(3), "www.google.com"));

    sample.messages.push_back(
      Msg("B B", Message::Id(4), "www.google.com/A/A"));

    sample.messages.push_back(
      Msg("B B", Message::Id(5), "google.com/A/B"));

    sample.messages.push_back(
      Msg("A", Message::Id(6), "google.com/A/B"));

    sample.search_result.push_back(ExpectedResult(Message::Id(2), "1 2"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"ANY A B DOMAIN NOT google.com ru";
  
    sample.messages.push_back(
      Msg("A C D", Message::Id(1), "www.google.ru/X"));
  
    sample.messages.push_back(
      Msg("A B C",
          Message::Id(2),
          "www.google.ru.com",
          ACE_OS::gettimeofday().sec() - 100000));

    sample.messages.push_back(
      Msg("X Y Z", Message::Id(3), "www.google.com"));

    sample.messages.push_back(
      Msg("B B", Message::Id(4), "www.google.biz/A/A"));

    sample.search_result.push_back(ExpectedResult(Message::Id(2), "1 2"));
    sample.search_result.push_back(ExpectedResult(Message::Id(4), "1 2"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"middle east AND \"middle west\" war";
  
    sample.messages.push_back(
      Msg("middle way east war with middle the west", Message::Id(1)));
  
    sample.messages.push_back(
      Msg("middle way east war with middle west", Message::Id(2)));
   
    sample.search_result.push_back(
      ExpectedResult(Message::Id(2), "1 3 4 6 7"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"ANY big car \"happy\" owner";
  
    sample.messages.push_back(
      Msg("red cars race", Message::Id(1)));
  
    sample.messages.push_back(
      Msg("happiest person", Message::Id(2)));

    sample.messages.push_back(
      Msg("biggest plane", Message::Id(3)));

    sample.messages.push_back(
      Msg("happy new year", Message::Id(4)));

    sample.messages.push_back(
      Msg("happy owners", Message::Id(5)));

    if(word_info_manager_.is_loaded(El::Lang::EC_ENG))
    {
      sample.search_result.push_back(ExpectedResult(Message::Id(1), "2"));
      sample.search_result.push_back(ExpectedResult(Message::Id(3), "1"));
    }

    sample.search_result.push_back(ExpectedResult(Message::Id(4), "1"));
    
    sample.search_result.push_back(
      ExpectedResult(Message::Id(5),
                     word_info_manager_.is_loaded(El::Lang::EC_ENG) ?
                     "1 2" : "1"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"rocket launch \"far island\"";
  
    sample.messages.push_back(
      Msg("rockets were launched from far island", Message::Id(1)));
  
    sample.messages.push_back(
      Msg("rocket launch from far islands", Message::Id(2)));

    if(word_info_manager_.is_loaded(El::Lang::EC_ENG))
    {
      sample.search_result.push_back(ExpectedResult(Message::Id(1),
                                                    "1 3 5 6"));
    }
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"A B LANGUAGE eng arabic";

    sample.messages.push_back(
      Msg("A B C",
          Message::Id(1),
          0,
          0,
          El::Lang::EC_ENG));
    
    sample.messages.push_back(
      Msg("A B C X",
          Message::Id(2),
          0,
          0,
          El::Lang::EC_IND));
    
    sample.messages.push_back(
      Msg("A B C Y",
          Message::Id(3),
          0,
          0,
          El::Lang::EC_ARA));
    
    sample.messages.push_back(
      Msg("A B C Z",
          Message::Id(4),
          0,
          0,
          El::Lang::EC_NUL));

    sample.search_result.push_back(ExpectedResult(Message::Id(1), "1 2"));
    sample.search_result.push_back(ExpectedResult(Message::Id(3), "1 2"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"A B LANGUAGE NOT eng arabic";

    sample.messages.push_back(
      Msg("A B C 1",
          Message::Id(1),
          0,
          0,
          El::Lang::EC_ENG));
    
    sample.messages.push_back(
      Msg("A B C 2",
          Message::Id(2),
          0,
          0,
          El::Lang::EC_IND));
    
    sample.messages.push_back(
      Msg("A B C 3",
          Message::Id(3),
          0,
          0,
          El::Lang::EC_ARA));
    
    sample.search_result.push_back(ExpectedResult(Message::Id(2), "1 2"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"A B LANGUAGE NOT eng";

    sample.messages.push_back(
      Msg("A B C 1",
          Message::Id(1),
          0,
          0,
          El::Lang::EC_ENG));
    
    sample.messages.push_back(
      Msg("A C",
          Message::Id(2),
          0,
          0,
          El::Lang::EC_ENG));
    
    sample.messages.push_back(
      Msg("A B C 3",
          Message::Id(3),
          0,
          0,
          El::Lang::EC_ARA));
    
    sample.search_result.push_back(ExpectedResult(Message::Id(3),
                                                  "1 2"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"A B LANGUAGE eng";

    sample.messages.push_back(
      Msg("A B C",
          Message::Id(1),
          0,
          0,
          El::Lang::EC_ENG));
    
    sample.messages.push_back(
      Msg("A C",
          Message::Id(2),
          0,
          0,
          El::Lang::EC_ENG));
    
    sample.messages.push_back(
      Msg("A B C",
          Message::Id(3),
          0,
          0,
          El::Lang::EC_ARA));
    
    sample.search_result.push_back(ExpectedResult(Message::Id(1), "1 2"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression =
      L"URL www.google.com/a/b/c \"www.gnu-site.org\" EXCEPT A B";

    sample.messages.push_back(
      Msg("A C D", Message::Id(1), "http://www.google.com/a/b/c"));
    
    sample.messages.push_back(
      Msg("A C B", Message::Id(2), "http://www.gnu-site.org/"));
    
    sample.messages.push_back(
      Msg("C B", Message::Id(3), "http://www.gnu-site.org/"));
    
    sample.search_result.push_back(ExpectedResult(Message::Id(1), ""));
    sample.search_result.push_back(ExpectedResult(Message::Id(3), ""));
  }
  
  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"A B AND SITE www.google.com www.gnu.org";

    sample.messages.push_back(
      Msg("A C D", Message::Id(1), "www.google.com/A/B"));
    
    sample.messages.push_back(
      Msg("A C B", Message::Id(2), "www.google.com/X/Y"));
    
    sample.messages.push_back(
      Msg("A B", Message::Id(3), "gnu.org"));
    
    sample.messages.push_back(
      Msg("A B C", Message::Id(4), "www.gnu.org/A/B"));
    
    sample.search_result.push_back(ExpectedResult(Message::Id(2), "1 3"));
    sample.search_result.push_back(ExpectedResult(Message::Id(4), "1 2"));
  }
  
  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"A DOMAIN 212.48.137.61";

    sample.messages.push_back(
      Msg("A C D", Message::Id(1), "212.48.137.61/X"));
    
    sample.messages.push_back(
      Msg("A C D", Message::Id(2), "100.212.48.137.61/X"));
  }
  
  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"A DOMAIN ru biz";

    sample.messages.push_back(
      Msg("A C D", Message::Id(1), "www.google.ru/X"));
    
    sample.messages.push_back(
      Msg("A C D", Message::Id(2), "www.google.co.uk/X"));
    
    sample.search_result.push_back(ExpectedResult(Message::Id(1), "1"));
  }
  
  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"A DOMAIN ru biz";

    sample.messages.push_back(
      Msg("A C D", Message::Id(1), "www.google.ru/X"));
    
    sample.search_result.push_back(ExpectedResult(Message::Id(1), "1"));
  }
  
  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"A OR B DOMAIN ru biz co.uk";
  
    sample.messages.push_back(
      Msg("A C D", Message::Id(1), "www.google.ru/X"));
  
    sample.messages.push_back(
      Msg("A B C",
          Message::Id(2),
          "www.google.ru.com",
          ACE_OS::gettimeofday().sec() - 100000));

    sample.messages.push_back(
      Msg("X Y Z", Message::Id(3), "www.google.co.uk"));

    sample.messages.push_back(
      Msg("B B", Message::Id(4), "www.google.biz/A/A"));

    sample.search_result.push_back(ExpectedResult(Message::Id(1), "1"));
    sample.search_result.push_back(ExpectedResult(Message::Id(4), "1 2"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"( A OR B ) DOMAIN ru";
  
    sample.messages.push_back(
      Msg("A C D", Message::Id(1), "www.google.ru/A/B"));
  
    sample.messages.push_back(
      Msg("A B C",
          Message::Id(2),
          "www.google.ru.com",
          ACE_OS::gettimeofday().sec() - 100000));

    sample.messages.push_back(
      Msg("X Y Z", Message::Id(3), "www.rbc.ru"));

    sample.search_result.push_back(ExpectedResult(Message::Id(1), "1"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"( A OR B ) DATE 3D DATE BEFORE 1D";
  
    sample.messages.push_back(
      Msg("A C D", Message::Id(1), 0, ACE_OS::gettimeofday().sec() - 1000));
  
    sample.messages.push_back(
      Msg("B C", Message::Id(2), 0, ACE_OS::gettimeofday().sec() - 100000));

    sample.messages.push_back(
      Msg("B C A",
          Message::Id(3),
          0,
          ACE_OS::gettimeofday().sec() - 300000));

    sample.search_result.push_back(ExpectedResult(Message::Id(2), "1"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"( A OR B ) DATE 1D";
  
    sample.messages.push_back(
      Msg("A C D", Message::Id(1), 0, ACE_OS::gettimeofday().sec() - 1000));
  
    sample.messages.push_back(
      Msg("B C", Message::Id(2), 0, ACE_OS::gettimeofday().sec() - 100000));

    sample.search_result.push_back(ExpectedResult(Message::Id(1), "1"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"middle AND east EXCEPT west";
  
    sample.messages.push_back(
      Msg("middle way east to middle the west", Message::Id(1)));
  
    sample.messages.push_back(
      Msg("middle way to middle east", Message::Id(2)));
   
    sample.search_result.push_back(ExpectedResult(Message::Id(2), "1 4 5"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"middle EXCEPT east";
  
    sample.messages.push_back(
      Msg("middle way east to middle the west", Message::Id(1)));
  
    sample.messages.push_back(
      Msg("middle way to middle west", Message::Id(2)));
   
    sample.search_result.push_back(ExpectedResult(Message::Id(2), "1 4"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"ALL A B OR ANY C D";
  
    sample.messages.push_back(
      Msg("C", Message::Id(1)));
  
    sample.messages.push_back(
      Msg("A B", Message::Id(2)));
  
    sample.messages.push_back(
      Msg("D", Message::Id(3)));

    sample.messages.push_back(
      Msg("A B C", Message::Id(4)));
  
    sample.messages.push_back(
      Msg("A C", Message::Id(5)));
  
    sample.search_result.push_back(ExpectedResult(Message::Id(1), "1"));
    sample.search_result.push_back(ExpectedResult(Message::Id(2), "1 2"));
    sample.search_result.push_back(ExpectedResult(Message::Id(3), "1"));
    sample.search_result.push_back(ExpectedResult(Message::Id(4), "1 2 3"));
    sample.search_result.push_back(ExpectedResult(Message::Id(5), "2"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"X AND ( A OR B AND ( C AND D OR E ) )";
  
    sample.messages.push_back(
      Msg("X B C", Message::Id(1)));
  
    sample.messages.push_back(
      Msg("A C E X", Message::Id(2)));
  
    sample.messages.push_back(
      Msg("X D", Message::Id(3)));
  
    sample.messages.push_back(
      Msg("A B", Message::Id(4)));
  
    sample.messages.push_back(
      Msg("X C D", Message::Id(5)));
  
    sample.messages.push_back(
      Msg("B E X", Message::Id(6)));
  
    sample.search_result.push_back(ExpectedResult(Message::Id(2), "1 4"));
    sample.search_result.push_back(ExpectedResult(Message::Id(6), "1 2 3"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"middle AND ( east OR west )";
  
    sample.messages.push_back(
      Msg("middle way east to middle the west", Message::Id(1)));
  
    sample.messages.push_back(
      Msg("middle way eastss to middle", Message::Id(2)));
   
    sample.messages.push_back(
      Msg("middless west to middless", Message::Id(3)));
   
    sample.search_result.push_back(ExpectedResult(Message::Id(1),
                                                  "1 3 5 7"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"middle east AND middle west";
  
    sample.messages.push_back(
      Msg("middle way east to middle the west", Message::Id(1)));
  
    sample.messages.push_back(
      Msg("middle way east to middle", Message::Id(2)));
   
    sample.messages.push_back(
      Msg("middle west to middle", Message::Id(3)));
   
    sample.search_result.push_back(ExpectedResult(Message::Id(1),
                                                  "1 3 5 7"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"middle east AND \"middle west\" AND war long";
  
    sample.messages.push_back(
      Msg("middle way east long war with middle the west", Message::Id(1)));
  
    sample.messages.push_back(
      Msg("middle way east war long with middle west", Message::Id(2)));
   
    sample.search_result.push_back(ExpectedResult(Message::Id(2),
                                                  "1 3 4 5 7 8"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"ANY middle AND ( ANY east west AND long long war )";
  
    sample.messages.push_back(
      Msg("middle east long war", Message::Id(1)));
  
    sample.messages.push_back(
      Msg("long war of middle west", Message::Id(2)));
   
    sample.messages.push_back(
      Msg("long war of west", Message::Id(3)));
   
    sample.messages.push_back(
      Msg("long middle war", Message::Id(4)));
   
    sample.search_result.push_back(ExpectedResult(Message::Id(1),
                                                  "1 2 3 4"));
    
    sample.search_result.push_back(ExpectedResult(Message::Id(2),
                                                  "1 2 4 5"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"ANY long: \"middle east\" , \"middle - west\" war";
  
    sample.messages.push_back(
      Msg("middle \"east west\"", Message::Id(1)));

    sample.messages.push_back(
      Msg("middle: east west", Message::Id(2)));
    
    sample.messages.push_back(
      Msg("long middle east west war", Message::Id(3)));
    
    sample.messages.push_back(
      Msg("middle way east to middle the west", Message::Id(4)));
  
    sample.messages.push_back(
      Msg("middle war east", Message::Id(5)));
    
    sample.messages.push_back(
      Msg("middle long east", Message::Id(6)));

    sample.messages.push_back(
      Msg(" east middle, west middle", Message::Id(7)));
   
    sample.search_result.push_back(ExpectedResult(Message::Id(1), "1 2"));
    
    sample.search_result.push_back(ExpectedResult(Message::Id(3),
                                                  "1 2 3 5"));
    
    sample.search_result.push_back(ExpectedResult(Message::Id(5), "2"));
    sample.search_result.push_back(ExpectedResult(Message::Id(6), "2"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"\"middle east\" \"middle west\"";
  
    sample.messages.push_back(
      Msg("middle \"east\" or middle west", Message::Id(1)));
    
    sample.messages.push_back(
      Msg("middle, east and middle west", Message::Id(2)));
  
    sample.messages.push_back(
      Msg("go \" middle east\" and middle  west try to find the best",
          Message::Id(3)));
    
    sample.messages.push_back(
      Msg("go \" middle\" east and middle  west yes", Message::Id(4)));
    
    sample.messages.push_back(
      Msg("go \" middle.\" east and middle  west", Message::Id(5)));
    
    sample.messages.push_back(
      Msg("go \" middle \" east and \"middle;\".? west", Message::Id(6)));
    
    sample.search_result.push_back(ExpectedResult(Message::Id(1),
                                                  "1 2 4 5"));
    
    sample.search_result.push_back(ExpectedResult(Message::Id(3),
                                                  "2 3 5 6"));
    
    sample.search_result.push_back(ExpectedResult(Message::Id(4),
                                                  "2 3 5 6 "));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"\"middle east\" \"middle west\"";
  
    sample.messages.push_back(
      Msg("middle east west", Message::Id(1)));
    
    sample.messages.push_back(
      Msg("middle way east to the middle west", Message::Id(2)));
  
    sample.messages.push_back(
      Msg("go  middle east and middle  west", Message::Id(3)));
    
    sample.search_result.push_back(ExpectedResult(Message::Id(3),
                                                  "2 3 5 6"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"ALL bi bup a lula";
    
    sample.messages.push_back(
      Msg("mama mila ramu", Message::Id(1)));
    
    sample.messages.push_back(
      Msg("papa pilesosil", Message::Id(2)));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"\"the cat\"";
    
    sample.messages.push_back(
      Msg(" la dog lo lu hourse", Message::Id(1)));
    
    sample.messages.push_back(
      Msg(" x  dog y cat   z hourse", Message::Id(2)));
    
    sample.messages.push_back(
      Msg(" The  dog the cat   the hourse", Message::Id(3)));
    
    sample.messages.push_back(
      Msg(" The  dog  cat   the hourse", Message::Id(4)));

    sample.search_result.push_back(ExpectedResult(Message::Id(3),
                                                  "3 4"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"middle east";
    
    sample.messages.push_back(
      Msg("go to the east", Message::Id(1)));
    
    sample.messages.push_back(
      Msg("middle way to the east", Message::Id(2)));
    
    sample.messages.push_back(
      Msg("east and middle west", Message::Id(3)));
    
    sample.search_result.push_back(ExpectedResult(Message::Id(2), "1 5"));
    sample.search_result.push_back(ExpectedResult(Message::Id(3), "1 3"));
  }

  {
    samples.push_back(EvaluateSample());    
    EvaluateSample& sample = *samples.rbegin();

    sample.expression = L"ANY  by \" default wget\" DATE BEFORE  6D  COUNTRY  \"Djibouti\" \"SLE\" \"GF\" \"Costa Rica\" OR  (  ANY  \" amagazine inside \" asian DOMAIN NOT org ru  OR  ALL  \" the request\" DOMAIN oracle.com oracle.com ru www.rbc.ru mc.peopleonpage.com lenta.ru com lenta.ru  )  AND  SITE www.martalog.com www.rambler.ru www.rambler.ru www.auto.ru books.stankin.ru www.auto.ru";

    unsigned long cur_time = ACE_OS::gettimeofday().sec() - 41111;

    Msg msg("22 State 14.1.23 Termination-Action 14.1.24 User-Name 14.1.25 User-Password 14.1.26 Vendor-Specific -------------------------------------------------------------------------------- [ &#x3c; ] [ &#x3e; ] [ &#x3c;&#x3c; ] [ Up ] [ &#x3e;&#x3e; ] [Top] [Contents] [Index] [ ? ] 14.1.1 CHAP-Password ATTRIBUTE CHAP-Password 3 string Users: L- Hints: -- Huntgroups: -- Additivity: N/A Proxy propagated: No This attribute indicates the response value provided by a PPP Challenge-Handshake Authentication Protocol (CHAP) user in response to the challenge. It is only used in Access-Request packets. The CHAP challenge value is found in the CHAP-Challenge attribute (60) if present in the packet, otherwise in the request authenticator field. -------------------------------------------------------------------------------- [ &#x3c; ] [ &#x3e; ] [ &#x3c;&#x3c; ] [ Up ] [ &#x3e;&#x3e; ] [Top] [Contents] [Index] [ ? ] 14.1.2 Callback-Id ATTRIBUTE Callback-Id 20 string Users: -R Hints: --",
            Message::Id(1),
            "http://www.auto.ru/wwwboards/gai/0487/145559.shtml",
            cur_time - 473164,
            El::Lang("nwc"));

    sample.messages.push_back(msg);

    msg.id = 2;
    msg.source_url += ".2";
    
    sample.messages.push_back(msg);
    
    sample.search_result.push_back(ExpectedResult(Message::Id(2), "96 97"));
  }

  //
  // Running searches
  //
  unsigned long i = 0;
  for(EvaluateSampleList::const_iterator it = samples.begin();
      it != samples.end(); it++, i++)
  {
    test_search(*it, i);
  }
}

void 
Application::test_search(const EvaluateSample& sample,
                         unsigned long sample_num)
  throw(El::Exception)
{
  Search::Expression_var expr;
  
  {
    NewsGate::Search::ExpressionParser parser;
    std::wistringstream istr(sample.expression);
    parser.parse(istr);

    expr = parser.expression();
    expr->normalize(word_info_manager_);
  }

  Search::Expression_var expr_optimized;
  
  {
    std::ostringstream ostr;
    expr->condition->print(ostr);

    std::wstring parsed_exp;
    El::String::Manip::utf8_to_wchar(ostr.str().c_str(), parsed_exp);
    
    std::wistringstream istr(parsed_exp);

    NewsGate::Search::ExpressionParser parser;
    parser.parse(istr);

    parser.optimize();
    
    expr_optimized = parser.expression();
    expr_optimized->normalize(word_info_manager_);
  }

  Message::SearcheableMessageMap message_map(false, false, 100, 0);

  for(MessageList::const_iterator it = sample.messages.begin();
      it != sample.messages.end(); it++)
  {
    insert_message(it->description.c_str(),
                   it->source_url.c_str(),
                   it->id,
                   it->updated,
                   it->fetched,
                   it->lang,
                   it->event_id,
                   it->event_capacity,
                   message_map);
  }

  std::string expression;
  El::String::Manip::wchar_to_utf8(sample.expression.c_str(), expression);

  MessageWordPositionMapPtr positions;

  if(positions_)
  {
    positions.reset(new ::NewsGate::Search::MessageWordPositionMap());
  }
  
  Search::ResultPtr result(evaluate(expr.in(),
                                    expr_optimized.in(),
                                    message_map,
                                    expression.c_str(),
                                    positions.get(),
                                    0,
                                    &sample.strategy));

  const Search::MessageInfoArray& message_infos = *(result->message_infos);
    
  IdSet res_ids(message_infos.size());

  for(Search::MessageInfoArray::const_iterator it = message_infos.begin();
      it != message_infos.end(); ++it)
  {
    res_ids.insert(it->wid.id);
  }

  for(ExpectedResultArray::const_iterator it =
        sample.search_result.begin(); it != sample.search_result.end(); it++)
  {
    IdSet::iterator res = res_ids.find(it->msg_id);

    if(res == res_ids.end())
    {
      std::ostringstream ostr;
      ostr << "test_search: for search '" << expression << "' ("<< sample_num
           << ") msg id " << it->msg_id.string()
           << " not found in search result\n"
           << "Expression:\n";

      expr->dump(ostr);

      throw Exception(ostr.str());
    }

    res_ids.erase(res);

    if(positions_)
    {
      const ::NewsGate::Search::WordPositionArray& expected_positions =
        it->positions;
      
      ::NewsGate::Search::MessageWordPositionMap::const_iterator pit =
          positions->find(it->msg_id);

      if(pit == positions->end())
      {
        if(expected_positions.size())
        {
          std::ostringstream ostr;
          ostr << "test_search: word positions not found for search '"
               << expression << "' ("<< sample_num
               << ") msg id " << it->msg_id.string() << "\nExpression:\n";
          
          expr->dump(ostr);

          throw Exception(ostr.str());
        }

        continue;
      }

      const ::NewsGate::Search::WordPositionArray& positions = pit->second;

      bool positions_match = positions.size() == expected_positions.size();

      if(positions_match)
      {
        unsigned long i = 0;
        for(; i < positions.size() &&
              positions[i] == expected_positions[i]; i++);
      
        positions_match = i == positions.size();
      }

      if(!positions_match)
      {
        std::ostringstream ostr;
        ostr << "test_search: invalid word positions for search '"
             << expression << "' ("<< sample_num
             << ") msg id " << it->msg_id.string() << "\nExpression:\n";

        expr->dump(ostr);

        ostr << "\nExpected positions (" << expected_positions.size() << "):";

        for(unsigned long i = 0; i < expected_positions.size(); i++)
        {
          ostr << " " << expected_positions[i];
        }

        ostr << "\nPositions (" << positions.size() << "):";

        for(unsigned long i = 0; i < positions.size(); i++)
        {
          ostr << " " << positions[i];
        }

        ostr << std::endl;
          
        throw Exception(ostr.str());
      }
      
    }
  }

  if(!res_ids.empty())
  {
    std::ostringstream ostr;
    ostr << "test_search: for search '" << expression << "' ("<< sample_num
         << ") msg id " << res_ids.begin()->string()
         << " unexpectedly found in search result. Expression dump:\n";

    expr->dump(ostr);

    throw Exception(ostr.str());
  }
    
  for(MessageList::const_iterator it = sample.messages.begin();
      it != sample.messages.end(); it++)
  {
    message_map.remove(it->id);
  }
}

void 
Application::insert_message(const char* description,
                            const char* source_url,
                            const Message::Id& id,
                            unsigned long published,
                            unsigned long fetched,
                            const El::Lang& lang,
                            const El::Luid& event_id,
                            unsigned long event_capacity,
                            Message::SearcheableMessageMap& message_map)
  throw(El::Exception)
{
  Message::StoredMessage message;
  message.content = new Message::StoredContent();

  message.id = id;
  message.set_source_url(source_url);
  message.published = published;
  message.fetched = fetched;
  message.lang = lang;
  message.event_id = event_id;
  message.event_capacity = event_capacity;
  
  {
    std::ostringstream ostr;
    ostr << source_url << "?id=" << id.string();

    std::string url = ostr.str();
    
    message.set_url_signature(url.c_str());
    message.content->url = url;
  }
  
  message.break_down("", description, 0, "");


  const Message::MessageWordPosition& word_positions =
    message.word_positions;

  El::Dictionary::Morphology::WordArray words(word_positions.size());
            
  for(unsigned long i = 0; i < word_positions.size(); i++)
  {
    words[i] = word_positions[i].first.c_str();
  }

  El::Dictionary::Morphology::WordInfoArray word_infos;      
  word_info_manager_.normal_form_ids(words, word_infos, 0, true);
  
  Message::WordPositionArray new_positions;
  Message::StoredMessage::set_normal_forms(word_infos,
                                           message.word_positions,
                                           message.positions,
                                           message.norm_form_positions,
                                           new_positions);
  
  message.positions = new_positions;
  message_map.insert(message, 75, 20/*, true*/);
}

void 
Application::test_performance() throw(El::Exception)
{
  if(pauses_)
  {
    std::cerr << "Hit ENTER to continue\n";
    getchar();
  }
  
  Message::SearcheableMessageMap message_map(false, false, 100, 0);
  create_messages(message_map);
  
  if(pauses_)
  {
    std::cerr << "Hit ENTER to continue\n";
    getchar();  
  }
  
  std::cerr << "Searching\n";

//  test_all_condition(message_map);
//  test_any_condition(message_map);
  
  test_complex_condition(message_map);
}

bool
Application::search_words(bool any,
                          const Message::SearcheableMessageMap& message_map)
  throw(El::Exception)
{
  std::wstring expression =
    random_words(any ? max_any_expr_words_ : max_all_expr_words_);
  
  if(any)
  {
    expression = std::wstring(L"ANY \"\" ") + expression;
  }

  std::string expr;
  El::String::Manip::wchar_to_utf8(expression.c_str(), expr);
    
  NewsGate::Search::ExpressionParser search_parser;

  std::wistringstream istr(expression);

  try
  {
    search_parser.parse(istr);
  }
  catch(const El::Exception& e)
  {
    std::ostringstream ostr;
    ostr << e << "\nExpression: '" << expr << "'";

    throw Exception(ostr.str());
  }
  
  search_parser.expression()->normalize(word_info_manager_);
  
  Search::Expression_var condition = search_parser.expression();
  
  MessageWordPositionMapPtr positions;

  if(positions_)
  {
    positions.reset(new ::NewsGate::Search::MessageWordPositionMap());
  }
  
  Search::ResultPtr result(evaluate(condition.in(),
                                    condition.in(),
                                    message_map,
                                    expr.c_str(),
                                    positions.get()));

  return !result->message_infos->empty();
}

std::wstring
Application::random_words(unsigned long max_expr_words)
  throw(El::Exception)
{
  typedef std::vector<std::string> WordArray;

  WordArray word_array;
  unsigned long expr_words = 0;

  while(true)
  {
    word_array.clear();
    
    std::string str =
      source_text_->get_random_substr(100) + " " +
      source_text_->get_random_substr(100) + " " +
      source_text_->get_random_substr(100) + " " +
      source_text_->get_random_substr(100);

    std::wstring wstr;
    El::String::Manip::utf8_to_wchar(str.c_str(), wstr);

    Message::StoredMessage::normalize_word(wstr);

    unsigned long cat_flags =
      El::String::Unicode::EC_QUOTE | El::String::Unicode::EC_BRACKET |
      El::String::Unicode::EC_STOP;
    
    for(std::wstring::iterator it = wstr.begin(); it != wstr.end(); it++)
    {
      wchar_t& chr = *it;

      if(El::String::Unicode::CharTable::el_categories(chr) & cat_flags)
      {
        chr = L' ';
      }
    }
      
    El::String::Manip::wchar_to_utf8(wstr.c_str(), str);
    
//    El::String::Manip::replace(str, "\"()!:,.?;-'", "           ");

    std::string substr;
    El::String::Manip::trim(str.c_str(), substr);

    if(substr.empty())
    {
      continue;
    }

    std::string text;
    El::String::Manip::utf8_to_lower(substr.c_str(), text);

    El::String::ListParser parser(text.c_str());

    const char* word;
    
    while((word = parser.next_item()) != 0)
    {
      word_array.push_back(word);
    }

    expr_words =
      1 + (unsigned long long)rand() * max_expr_words /
      ((unsigned long long)RAND_MAX + 1);
    
    if(expr_words > word_array.size())
    {
      expr_words = word_array.size();
    }

    if(!expr_words)
    {
      continue;
    }

    break;
  }

  unsigned long index =
    (unsigned long long)rand() * (word_array.size() - expr_words + 1) /
    ((unsigned long long)RAND_MAX + 1);

  std::ostringstream ostr;

  char quote = '\0';
      
  for(unsigned long i = 0; i < expr_words; i++)
  {
    int r = rand() % 3;    
    bool cur_quote = r ? (r == 1 ? '\'' : '"') : '\0';

    if(cur_quote != quote && quote != '\0')
    {
      ostr << quote;
    }
    
    ostr << " ";
      
    if(cur_quote != quote && cur_quote != '\0')
    {
      ostr << cur_quote;
    }
      
    ostr << word_array[index + i];

    quote = cur_quote;
  }

  if(quote != '\0')
  {
    ostr << quote;
  }

  std::string words = ostr.str();

  std::wstring wwords;
  El::String::Manip::utf8_to_wchar(words.c_str(), wwords);

  return wwords;
}

void
Application::create_messages(Message::SearcheableMessageMap& message_map)
  throw(El::Exception)
{
  std::cerr << "Creating messages ...\n";
  
  Message::StoredMessage::break_down_meter.active(true);
  Message::SearcheableMessageMap::insert_meter.active(true);
  Message::SearcheableMessageMap::remove_meter.active(true);
  
  unsigned long long total_len = 0;
  
  unsigned long progress_period =
    std::max(message_count_ / 10, (unsigned long)1);

  unsigned long cur_time = ACE_OS::gettimeofday().sec() - 41111;  
  unsigned long vm_size1 = El::Utility::mem_used();

  for(uint64_t i = 1; i <= message_count_; i++)
  {
    unsigned long message_len = min_message_len_ + 
      (unsigned long long)rand() * (max_message_len_ - min_message_len_) /
              ((unsigned long long)RAND_MAX + 1);
      
    std::string message = source_text_->get_random_substr(message_len);

    total_len += message.length() + 1;
      
    Message::Id id((i << 32) | i);
    
    unsigned long freshness = 86400 *
      ((unsigned long long)rand() * 7 /
              ((unsigned long long)RAND_MAX + 1));

    unsigned long lang_number = 1 +
      ((unsigned long long)rand() * El::Lang::languages_count() /
              ((unsigned long long)RAND_MAX + 1));

    unsigned long long event_id = 
      (unsigned long long)rand() * 1000 /
      ((unsigned long long)RAND_MAX + 1) + 2;
    
    unsigned long event_capacity = (unsigned long long)rand() * 10 /
      ((unsigned long long)RAND_MAX + 1);

    unsigned long published = cur_time - freshness;

    unsigned long fetched = published + (unsigned long long)rand() * freshness /
      ((unsigned long long)RAND_MAX + 1);

    insert_message(message.c_str(),
                   source_urls_->get_random_url().c_str(),
                   id,
                   published,
                   fetched,
                   El::Lang::ElCode(El::Lang::EC_NUL + lang_number),
                   event_id,
                   event_capacity, 
                   message_map);
/*
    id.feed = 2;
    
    insert_message(message.c_str(),
                   source_urls_->get_random_url().c_str(),
                   id,
                   cur_time - freshness,
                   NewsGate::Feed::TP_RSS,
                   El::Lang::ElCode(El::Lang::EC_NUL + lang_number),
                   message_map);
*/
    if((i % progress_period) == 0)
    {
      std::cerr << i << " messages created\n";
    }
  }

  unsigned long vm_size2 = El::Utility::mem_used();
  
  Message::StoredMessage::break_down_meter.dump(std::cerr);
  Message::SearcheableMessageMap::insert_meter.dump(std::cerr);
  Message::SearcheableMessageMap::remove_meter.dump(std::cerr);
  
  unsigned long mem_use = vm_size2 - vm_size1;
  
  std::cerr << "Memory used: " << mem_use << " KB ("
            << (float)mem_use / message_count_ << " per message)\n";
  
  std::cerr << "Words: " << message_map.words.size() << "\nTotal length: "
            << total_len;

  unsigned long total_pos = 0;
  unsigned long total_words = 0;
  unsigned long total_words_len = 0;
  
  for(Message::StoredMessageMap::iterator it = message_map.messages.begin();
      it != message_map.messages.end(); it++)
  {
    Message::MessageWordPosition& pos =
      const_cast<Message::MessageWordPosition&>(it->second->word_positions);

    total_words += pos.size();
    
    for(unsigned long i = 0; i < pos.size(); i++)
    {
      total_words_len += pos[i].first.length() + 1;
      total_pos += pos[i].second.position_count();
    }
  }

  std::cerr << "\nPositions: " << total_pos
            << "\nTotal words: " << total_words
            << "\nTotal words len: " << total_words_len
            << std::endl;  
}

void
Application::test_all_condition(Message::SearcheableMessageMap& message_map)
  throw(El::Exception)
{
  std::cerr << "ALL condition ...\n";

  Search::AllWords::evaluate_meter.active(true);
  
  unsigned long matched = 0;
  unsigned long unmatched = 0;

  unsigned long progress_period =
    std::max(search_count_ / 10, (unsigned long)1);
  
  for(unsigned long i = 0; i < search_count_; )
  {
    if(search_words(false, message_map))
    {
      matched++;
    }
    else
    {
      unmatched++;
    }

    if((++i % progress_period) == 0)
    {
      std::cerr << i << " searches performed\n";
    }
  }
  
  Search::AllWords::evaluate_meter.dump(std::cerr);

  std::cerr << "From " << search_count_ << " searches " << matched
            << " matched " << unmatched << " not matched\n";
}

void
Application::test_any_condition(Message::SearcheableMessageMap& message_map)
  throw(El::Exception)
{
  std::cerr << "ANY condition ...\n";

  Search::AnyWords::evaluate_meter.active(true);

  unsigned long matched = 0;
  unsigned long unmatched = 0;
  
  unsigned long progress_period =
    std::max(search_count_ / 10, (unsigned long)1);

  for(unsigned long i = 0; i < search_count_;)
  {
    if(search_words(true, message_map))
    {
      matched++;
    }
    else
    {
      unmatched++;
    }

    if((++i % progress_period) == 0)
    {
      std::cerr << i << " searches performed\n";
    }
    
  }
  
  Search::AnyWords::evaluate_meter.dump(std::cerr);

  std::cerr << "From " << search_count_ << " searches " << matched
            << " matched " << unmatched << " not matched\n";  
}

void
Application::test_complex_condition(
  Message::SearcheableMessageMap& message_map)
  throw(El::Exception)
{
  std::cerr << "Complex condition ...\n";

  Search::AllWords::evaluate_meter.reset();
  Search::AnyWords::evaluate_meter.reset();
  Search::Site::evaluate_meter.active(true);
  Search::Url::evaluate_meter.active(true);
  Search::AllWords::evaluate_meter.active(true);
  Search::AnyWords::evaluate_meter.active(true);

  Search::And::evaluate_simple_meter.active(true);
  Search::And::evaluate_meter.active(true);
  Search::Or::evaluate_simple_meter.active(true);
  Search::Or::evaluate_meter.active(true);
  Search::Except::evaluate_simple_meter.active(true);
  Search::Except::evaluate_meter.active(true);
  Search::PubDate::evaluate_simple_meter.active(true);
  Search::PubDate::evaluate_meter.active(true);
  Search::Fetched::evaluate_simple_meter.active(true);
  Search::Fetched::evaluate_meter.active(true);
  Search::Domain::evaluate_simple_meter.active(true);
  Search::Domain::evaluate_meter.active(true);
  Search::Lang::evaluate_simple_meter.active(true);
  Search::Lang::evaluate_meter.active(true);
  Search::Country::evaluate_simple_meter.active(true);
  Search::Country::evaluate_meter.active(true);
  Search::Expression::search_meter.active(true);
  Search::Expression::search_p1_meter.active(true);
  Search::Expression::search_p2_meter.active(true);
  Search::Expression::search_simple_meter.active(true);
  Search::Expression::take_top_meter.active(true);
  Search::Expression::sort_skip_cut_meter.active(true);
  Search::Expression::sort_and_cut_meter.active(true);
  Search::Expression::remove_similar_meter.active(true);
  Search::Expression::remove_similar_p1_meter.active(true);
  Search::Expression::remove_similar_p2_meter.active(true);
  Search::Expression::collapse_events_meter.active(true);
  Search::Expression::copy_range_meter.active(true);

  unsigned long matched = 0;
  unsigned long unmatched = 0;
  
  unsigned long progress_period =
    std::max(search_count_ / 10, (unsigned long)1);
  
  unsigned long long total_results = 0;

#ifdef USE_HIRES
  ACE_High_Res_Timer timer;
  timer.start();
#else
  ACE_Time_Value start_time = ACE_OS::gettimeofday();
#endif
  
  for(unsigned long i = 0; i < search_count_;)
  {
    if(search_complex(message_map, total_results))
    {
      matched++;
    }
    else
    {
      unmatched++;
    }

    if((++i % progress_period) == 0)
    {
#ifdef USE_HIRES
      timer.stop();

      ACE_Time_Value tm;
      timer.elapsed_time(tm);
#else
        ACE_Time_Value tm = ACE_OS::gettimeofday() - start_time;
#endif
      
      std::cerr << i << " searches performed (" << El::Moment::time(tm)
                << ")\n";

#ifdef USE_HIRES
      timer.start();
#else
      start_time = ACE_OS::gettimeofday();
#endif
      
    }
  }
  
  Search::AllWords::evaluate_meter.dump(std::cerr);
  Search::AnyWords::evaluate_meter.dump(std::cerr);
  Search::Site::evaluate_meter.dump(std::cerr);
  Search::Url::evaluate_meter.dump(std::cerr);
  Search::And::evaluate_meter.dump(std::cerr);
  Search::And::evaluate_simple_meter.dump(std::cerr);
  Search::Or::evaluate_meter.dump(std::cerr);
  Search::Or::evaluate_simple_meter.dump(std::cerr);
  Search::Except::evaluate_meter.dump(std::cerr);
  Search::Except::evaluate_simple_meter.dump(std::cerr);
  Search::PubDate::evaluate_meter.dump(std::cerr);
  Search::PubDate::evaluate_simple_meter.dump(std::cerr);
  Search::Fetched::evaluate_meter.dump(std::cerr);
  Search::Fetched::evaluate_simple_meter.dump(std::cerr);
  Search::Domain::evaluate_meter.dump(std::cerr);
  Search::Domain::evaluate_simple_meter.dump(std::cerr);
  Search::Lang::evaluate_meter.dump(std::cerr);
  Search::Lang::evaluate_simple_meter.dump(std::cerr);
  Search::Country::evaluate_meter.dump(std::cerr);
  Search::Country::evaluate_simple_meter.dump(std::cerr);
  Search::Expression::search_meter.dump(std::cerr);
  Search::Expression::search_p1_meter.dump(std::cerr);
  Search::Expression::search_p2_meter.dump(std::cerr);
  Search::Expression::search_simple_meter.dump(std::cerr);
  Search::Expression::take_top_meter.dump(std::cerr);
  Search::Expression::sort_skip_cut_meter.dump(std::cerr);
  Search::Expression::sort_and_cut_meter.dump(std::cerr);
  Search::Expression::remove_similar_meter.dump(std::cerr);
  Search::Expression::remove_similar_p1_meter.dump(std::cerr);
  Search::Expression::remove_similar_p2_meter.dump(std::cerr);
  Search::Expression::collapse_events_meter.dump(std::cerr);
  Search::Expression::copy_range_meter.dump(std::cerr);

  std::cerr << "From " << search_count_ << " searches " << matched
            << " matched " << unmatched << " not matched\n"
            << (matched ? total_results / matched : 0)
            << " results in average for matched ones\n";
}

bool
Application::search_complex(Message::SearcheableMessageMap& message_map,
                            unsigned long long& total_results)
  throw(El::Exception)
{
  std::wstring expression = random_expression(max_operands_in_complex_cond_);

  if(verbose_)
  {
    std::wcout << L"RndExp: " << expression << std::endl;
  }

//  std::wcerr << expression << std::endl;

  std::string expr;
  El::String::Manip::wchar_to_utf8(expression.c_str(), expr);

  Search::Expression_var condition;
  
  {
    NewsGate::Search::ExpressionParser search_parser;

    std::wistringstream istr(expression);

    try 
    {
      search_parser.parse(istr);
    }
    catch(const El::Exception& e)
    {    
      std::ostringstream ostr;
      ostr << e << "\nExpression: '" << expr << "'";

      throw Exception(ostr.str());
    }

    condition = search_parser.expression();
    condition->normalize(word_info_manager_);
  }
  
  Search::Expression_var condition_optimized;

  {
    NewsGate::Search::ExpressionParser search_parser;

    std::wistringstream istr(expression);

    try 
    {
      search_parser.parse(istr);
    }
    catch(const El::Exception& e)
    {    
      std::ostringstream ostr;
      ostr << e << "\nExpression: '" << expr << "'";

      throw Exception(ostr.str());
    }

    search_parser.optimize();
    condition_optimized = search_parser.expression();
    condition_optimized->normalize(word_info_manager_);
  }
  
  MessageWordPositionMapPtr positions;
  
  if(positions_)
  {
    positions.reset(new ::NewsGate::Search::MessageWordPositionMap());
  }
  
  Search::ResultPtr result(evaluate(condition.in(),
                                    condition_optimized.in(),
                                    message_map,
                                    expr.c_str(),
                                    positions.get(),
                                    &total_results));
    
  return !result->message_infos->empty();
}

std::wstring
Application::random_expression(unsigned long max_operands_in_complex_cond)
  throw(El::Exception)
{
  unsigned long cur_time = ACE_OS::gettimeofday().sec();
  
  if(max_operands_in_complex_cond == 1)
  {
    int op_type = (unsigned long long)rand() * 4 /
      ((unsigned long long)RAND_MAX + 1);

    switch(op_type)
    {
    case 0:
      {
        return std::wstring(L" ALL ") +
          random_words(max_complex_expr_words_);
      }    
    case 1:
      {
        int threshold = (unsigned long long)rand() * 3 /
          ((unsigned long long)RAND_MAX + 1) + 1;

        if(threshold > 1)
        {
          std::wostringstream ostr;
          ostr << L" ANY " << threshold << L" \"\" " <<
            random_words(max_complex_expr_words_);

          return ostr.str();
        }
        else
        {
          return std::wstring(L" ANY \"\" ") +
            random_words(max_complex_expr_words_);
        }
      }

    case 2:
      {
        std::wstring expr = L" SITE";

        unsigned long hosts = 1 +
          (unsigned long long)rand() * max_domain_expr_words_ /
          ((unsigned long long)RAND_MAX + 1);
            
        for(unsigned long i = 0; i < hosts; i++)
        {
          El::Net::HTTP::URL url(source_urls_->get_random_url().c_str());
        
          std::wstring host;
          El::String::Manip::utf8_to_wchar(url.host(), host);
              
          expr += std::wstring(L" ") + host;
        }
        
        return expr;
      }
      
    case 3:
      {
        std::wstring expr = L" URL";

        unsigned long urls = 1 +
          (unsigned long long)rand() * max_domain_expr_words_ /
          ((unsigned long long)RAND_MAX + 1);
            
        for(unsigned long i = 0; i < urls; i++)
        {
          El::Net::HTTP::URL url(source_urls_->get_random_url().c_str());
        
          std::wstring urlpath;
          El::String::Manip::utf8_to_wchar(url.string(), urlpath);
              
          expr += std::wstring(L" ") + urlpath;
        }
        
        return expr;
      }
      
    }
  }
  
  std::wstring expr;

  int operation_type = -1;
  
  for(unsigned long i = 0; i < max_operands_in_complex_cond; i++)
  {
    switch(operation_type)
    {
    case 2:
      {
        // PubDate
        unsigned long freshness = 1 + (unsigned long long)rand() * 6 /
          ((unsigned long long)RAND_MAX + 1);

        std::wostringstream ostr;
        ostr << L" " << freshness << L"D ";

        expr += ostr.str();
        
        break;
      }
    case 6:
      {
        // Domain/Domain not

        unsigned long domains = 1 +
          (unsigned long long)rand() * max_domain_expr_words_ /
          ((unsigned long long)RAND_MAX + 1);

        // TODO: REMOVE
//        std::cerr << domains << ": ";
        
        for(unsigned long i = 0; i < domains; i++)
        {
          El::Net::HTTP::URL url(source_urls_->get_random_url().c_str());
        
          std::string hostname = url.host();

          // TODO: REMOVE
//          std::cerr << hostname << "#";

          unsigned short level = El::Net::domain_level(hostname.c_str());

          // TODO: REMOVE
//          std::cerr << level << "/";
          
          level = 1 + (unsigned long long)rand() * level /
            ((unsigned long long)RAND_MAX + 1);

          // TODO: REMOVE
//          std::cerr << level << "#";
          
          const char* dmn = El::Net::domain(hostname.c_str(), level);

          // TODO: REMOVE
//          std::cerr << dmn << " ";
          
          std::wstring domain;
          
          El::String::Manip::utf8_to_wchar(
            dmn,
            domain);
        
          expr += domain + L" ";
        }
        
        break;
      }
    case 7:
      {
        // Lang/Lang not
        
        unsigned long languages = 1 +
          (unsigned long long)rand() * max_lang_expr_words_ /
          ((unsigned long long)RAND_MAX + 1);

        for(unsigned long i = 0; i < languages; i++)
        {
          unsigned long lang_number = 1 +
            ((unsigned long long)rand() * El::Lang::languages_count() /
             ((unsigned long long)RAND_MAX + 1));

          El::Lang::ElCode el_code(
            El::Lang::ElCode(El::Lang::EC_NUL + lang_number));
        
          El::Lang lang(el_code);

          const char* lang_str = 0;
          unsigned long type = rand() % 3;

          switch(type)
          {
          case 0: lang_str = lang.l2_code(); break;
          case 1: lang_str = lang.l3_code(); break;
          case 2: lang_str = lang.name(); break;
          }

          if(*lang_str == '\0')
          {
            // Can be if type == 0 and 2-letter code is absent
            type = 1;
            lang_str = lang.l3_code();
          }

          std::wstring language;
          El::String::Manip::utf8_to_wchar(lang_str, language);

          expr += L" ";

          if(type == 2)
          {
            expr += L"\"";
          }

          expr += language;

          if(type == 2)
          {
            expr += L"\"";
          }
        }
        
        break;
      }
    case 8:
      {
        // Country/Country not
        
        unsigned long countries = 1 +
          (unsigned long long)rand() * max_country_expr_words_ /
          ((unsigned long long)RAND_MAX + 1);

        for(unsigned long i = 0; i < countries; i++)
        {
          unsigned long country_number = 1 +
            ((unsigned long long)rand() * El::Country::countries_count() /
             ((unsigned long long)RAND_MAX + 1));

          El::Country::ElCode el_code(
            El::Country::ElCode(El::Country::EC_NUL + country_number));
        
          El::Country country(el_code);

          const char* country_str = 0;
          unsigned long type = rand() % 4;

          switch(type)
          {
          case 0: country_str = country.l2_code(); break;
          case 1: country_str = country.l3_code(); break;
          case 2: country_str = country.d3_code(); break;
          case 3: country_str = country.name(); break;
          }

          std::wstring country_wstr;
          El::String::Manip::utf8_to_wchar(country_str, country_wstr);

          expr += L" \"";
          expr += country_wstr;
          expr += L"\"";
        }
        
        break;
      }
    case 9:
      {
        // Fetched

        std::wostringstream ostr;

        if(rand() % 2)
        {
          unsigned long freshness = 86400 *
            ((unsigned long long)rand() * 7 /
             ((unsigned long long)RAND_MAX + 1));

          unsigned long time = cur_time -
            (unsigned long long)rand() * freshness /
            ((unsigned long long)RAND_MAX + 1);

          ostr << L" " << time << L" ";
        }
        else
        {
          unsigned long time = 1 + (unsigned long long)rand() * 6 /
            ((unsigned long long)RAND_MAX + 1);
          
          ostr << L" " << time << L"D ";
        }
        
        expr += ostr.str();
        
        break;
      }
    default:
      {
        int operand_type = (unsigned long long)rand() * 5 /
          ((unsigned long long)RAND_MAX + 1);

        switch(operand_type)
        {
        case 0:
          {
            expr += std::wstring(L" ALL ") +
              random_words(max_complex_expr_words_);
      
            break;
          }    
        case 1:
          {
            expr += std::wstring(L" ( ") +
              random_expression(max_operands_in_complex_cond - 1) +
              std::wstring(L" ) ");
      
            break;
          }
        case 2:
          {
            expr += std::wstring(L" ANY \"\" ") +
              random_words(max_complex_expr_words_);
      
            break;
          }
        case 3:
          {
            expr += std::wstring(L" SITE");

            unsigned long hosts = 1 +
              (unsigned long long)rand() * max_domain_expr_words_ /
              ((unsigned long long)RAND_MAX + 1);
            
            for(unsigned long i = 0; i < hosts; i++)
            {
              El::Net::HTTP::URL url(source_urls_->get_random_url().c_str());
        
              std::wstring host;
              El::String::Manip::utf8_to_wchar(url.host(), host);
              
              expr += std::wstring(L" ") + host;
            }
      
            break;
          }
        case 4:
          {
            expr += std::wstring(L" URL");

            unsigned long urls = 1 +
              (unsigned long long)rand() * max_domain_expr_words_ /
              ((unsigned long long)RAND_MAX + 1);
            
            for(unsigned long i = 0; i < urls; i++)
            {
              El::Net::HTTP::URL url(source_urls_->get_random_url().c_str());
        
              std::wstring urlpath;
              El::String::Manip::utf8_to_wchar(url.string(), urlpath);
              
              expr += std::wstring(L" ") + urlpath;
            }
        
            break;
          }
          
        }
      }
    }
    
    if(i < max_operands_in_complex_cond - 1)
    {
      operation_type = (unsigned long long)rand() * 10 /
        ((unsigned long long)RAND_MAX + 1);

      int reverse = rand() % 2;
    
      switch(operation_type)
      {
      case 0:
      case 1:
        {
          expr += std::wstring(L" OR "); 
          break;
        }    
      case 2:
        {
          expr += reverse ? std::wstring(L" DATE BEFORE ") :
            std::wstring(L" DATE ");          
          break;
        }
      case 3:
      case 4:
        {
          expr += std::wstring(L" AND "); 
          break;
        }
      case 5:
        {
          expr += std::wstring(L" EXCEPT "); 
          break;
        }
      case 6:
        {
          expr += reverse ? std::wstring(L" DOMAIN NOT ") :
            std::wstring(L" DOMAIN ");
          
          break;
        }
      case 7:
        {
          expr += reverse ? std::wstring(L" LANGUAGE NOT ") :
            std::wstring(L" LANGUAGE ");
          
          break;
        }
      case 8:
        {
          expr += reverse ? std::wstring(L" COUNTRY NOT ") :
            std::wstring(L" COUNTRY ");
          
          break;
        }
      case 9:
        {
          expr += reverse ? std::wstring(L" FETCHED BEFORE ") :
            std::wstring(L" FETCHED ");
          
          break;
        }
      }
    }
  }

  return expr;
}

Search::Result*
Application::evaluate(Search::Expression* condition,
                      NewsGate::Search::Expression* condition_optimized,
                      const Message::SearcheableMessageMap& message_map,
                      const char* expression,
                      ::NewsGate::Search::MessageWordPositionMap* positions,
                      unsigned long long* total_results,
                      const ::NewsGate::Search::Strategy* pstrategy)
  throw(El::Exception)
{
  Search::Strategy strategy(new Search::Strategy::SortByRelevanceDesc(),
                            new Search::Strategy::CollapseEvents(75, 90, 4));
  
  if(pstrategy == 0)
  {
    pstrategy = &strategy;
  }
  
  MessageWordPositionMapPtr pos1;
  MessageWordPositionMapPtr pos2;

  if(positions)
  {
    pos1.reset(new ::NewsGate::Search::MessageWordPositionMap());
    pos2.reset(new ::NewsGate::Search::MessageWordPositionMap());
  }

  time_t cur_time = ACE_OS::gettimeofday().sec();
  
  Search::ResultPtr result2(condition_optimized->search(message_map,
                                                        true,
                                                        *pstrategy,
                                                        pos2.get(),
                                                        &cur_time));
  
  Search::ResultPtr result1(condition->search_simple(message_map,
                                                     true,
                                                     *pstrategy,
                                                     pos1.get(),
                                                     &cur_time));
  
  if(total_results)
  {
    *total_results += result1->message_infos->size();
  }

  size_t suppressed1 = 0;
  size_t suppressed2 = 0;

  size_t total_messages = result1->message_infos->size();
  
  result1->take_top(0, result_count_, *pstrategy, &suppressed1);
  result2->take_top(0, result_count_, *pstrategy, &suppressed2);
  
  Search::MessageInfoArray& message_infos1 = *result1->message_infos;
  Search::MessageInfoArray& message_infos2 = *result2->message_infos;

  IdSet res2(message_infos2.size());

  for(Search::MessageInfoArray::const_iterator
        it = message_infos2.begin(); it != message_infos2.end(); ++it)
  {
    res2.insert(it->wid.id);
  }
  
  for(Search::MessageInfoArray::const_iterator
        it1 = message_infos1.begin(); it1 != message_infos1.end(); ++it1)
  {
    const Message::Id& id = it1->wid.id;
    
    IdSet::iterator it2 = res2.find(id);

    if(it2 == res2.end())
    {
      std::cout << "***********************\n";

      for(Search::MessageInfoArray::const_iterator
            it = message_infos1.begin(); it != message_infos1.end(); ++it)
      {
        std::cout << it->wid.id.string() << std::endl;
      }

      std::cout << "***********************\n";
      
      for(Search::MessageInfoArray::const_iterator
            it = message_infos2.begin(); it != message_infos2.end(); ++it)
      {
        std::cout << it->wid.id.string() << std::endl;
      }
      
      std::ostringstream ostr;
      ostr << "Application::evaluate: message " << id.string()
           << " found by evaluate_simple (tr:"
           << result1->message_infos->size()
           << "), not found by evaluate (tr:" << result2->message_infos->size()
           << "). Expression:\n"
           << expression << "\n\nExpression dump:\n";

      condition->dump(ostr);

      ostr << "\nOptimized expression dump:\n";
        
      condition_optimized->dump(ostr);

      const Message::StoredMessage& stored_msg =
        *message_map.messages.find(message_map.id_to_number.find(id)->second)->
        second;
      
      ostr << "\nMessage update freshness: " << cur_time - stored_msg.published
           << " (" << stored_msg.published << ")"
           << "\nMessage fetch freshness: " << cur_time - stored_msg.fetched
           << " (" << stored_msg.fetched << ")"
           << "\nMessage url: " << stored_msg.source_url
           << "\nMessage lang: " << stored_msg.lang
           << "\nMessage country: " << stored_msg.country
           << "\nMessage text:\n";

      Message::StoredMessage::DefaultMessageBuilder builder(ostr);
      stored_msg.assemble_description(builder);
        
      throw Exception(ostr.str());
    }

    res2.erase(it2);
  }

  for(IdSet::iterator it2 = res2.begin(); it2 != res2.end(); ++it2)
  {
    const Message::Id& id = *it2;

    std::ostringstream ostr;
    ostr << "Application::evaluate: message " << id.string()
         << " found by evaluate, not found by evaluate_simple. Expression:\n"
         << expression << "\nExpression dump:\n";

    condition->dump(ostr);

    ostr << "\nOptimized expression dump:\n";
        
    condition_optimized->dump(ostr);

    const Message::StoredMessage& stored_msg =
      *message_map.messages.find(message_map.id_to_number.find(id)->second)->
      second;
      
    ostr << "\n\nMessage update freshness: " << cur_time - stored_msg.published
         << " (" << stored_msg.published << ")"
         << "\nMessage fetch freshness: " << cur_time - stored_msg.fetched
         << " (" << stored_msg.fetched << ")"
         << "\nMessage url: " << stored_msg.source_url
         << "\nMessage lang: " << stored_msg.lang
         << "\nMessage country: " << stored_msg.country
         << "\nMessage text:\n";

    Message::StoredMessage::DefaultMessageBuilder builder(ostr);
    stored_msg.assemble_description(builder);

    throw Exception(ostr.str());
  }

  if(positions)
  {
    ::NewsGate::Search::MessageWordPositionMap* min_pos = 0;
    ::NewsGate::Search::MessageWordPositionMap* max_pos = 0;

    std::string max_func;
    std::string min_func;    
    
    if(pos1->size() < pos2->size())
    {
      min_pos = pos1.get();
      max_pos = pos2.get();
      max_func = "search";
      min_func = "search_simple";
    }
    else
    {
      min_pos = pos2.get();
      max_pos = pos1.get();      
      max_func = "search_simple";
      min_func = "search";
    }  
    
    for(::NewsGate::Search::MessageWordPositionMap::const_iterator it =
          max_pos->begin(); it != max_pos->end(); it++)
    {
      const ::NewsGate::Search::WordPositionArray& max_set = it->second;

      const Message::StoredMessage& stored_msg =
        *message_map.messages.find(
          message_map.id_to_number.find(it->first)->second)->second;      

      if(max_set.empty())
      {
        std::ostringstream ostr;
        ostr << "Application::evaluate: word positions set for message "
             << it->first.string() << " provided by " << max_func
             << " is empty. Expression:\n" << expression
             << "\nExpression dump:\n";

        condition->dump(ostr);
        
        ostr << "\nMessage:\n";

        Message::StoredMessage::DefaultMessageBuilder builder(ostr);
        stored_msg.assemble_description(builder);

        throw Exception(ostr.str());
      }
      
      ::NewsGate::Search::MessageWordPositionMap::const_iterator it2 =
          min_pos->find(it->first);

      if(it2 == pos2->end())
      {
        std::ostringstream ostr;
        ostr << "Application::evaluate: word positions for message "
             << it->first.string() << " provided by " << max_func
             << ", not found in map provided by " << min_func
             << ". Expression:\n" << expression << "\nExpression dump:\n";

        condition->dump(ostr);
        
        ostr << "\nMessage:\n";

        Message::StoredMessage::DefaultMessageBuilder builder(ostr);
        stored_msg.assemble_description(builder);

        throw Exception(ostr.str());      
      }

      const ::NewsGate::Search::WordPositionArray& min_set = it2->second;

      if(max_set.size() != min_set.size())
      {
        std::ostringstream ostr;
        ostr << "Application::evaluate: word positions for message "
             << it->first.string() << " provided by " << max_func << " and "
          "by " << min_func << " are of different sizes (" << max_set.size()
             << "/" << min_set.size() << "). Expression:\n" << expression
             << "\nExpression dump:\n";

        condition->dump(ostr);
        
        ostr << "\nMessage:\n";

        Message::StoredMessage::DefaultMessageBuilder builder(ostr);
        stored_msg.assemble_description(builder);
        
        throw Exception(ostr.str());        
      }

      unsigned long i = 0;
      for(; i < max_set.size() && max_set[i] == min_set[i]; i++);

      if(i < max_set.size())
      {
        std::ostringstream ostr;
        ostr << "Application::evaluate: word positions for message "
             << it->first.string() << " provided by search_simple different "
          "from those provided by search. Expression:\n"
             << expression << "\nExpression dump:\n";

        condition->dump(ostr);
        
        throw Exception(ostr.str());        
      }

    }
    
    *positions = *min_pos;
  }  
/*
  if(suppressed1 != suppressed2)
  {
    std::ostringstream ostr;
    ostr << "Application::evaluate: suppressed message count differes for "
      "search_simple and search: " << suppressed1 << " and " << suppressed2
         << " respectivelly. Expression:\n"
         << expression << "\nExpression dump:\n";
    
    condition->dump(ostr);
    
    throw Exception(ostr.str());        
  }
*/
  if(total_messages != result1->stat.total_messages)
  {
    std::ostringstream ostr;
    ostr << "Application::evaluate: total message count unexpectedly "
      "differes from stat.total_messages: " << total_messages << " and "
         << result1->stat.total_messages << " respectivelly. Expression:\n"
         << expression << "\nExpression dump:\n";
    
    condition->dump(ostr);
    
    throw Exception(ostr.str());        
  }
/*
  unsigned long guessed_total_messages = message_infos1.size() + suppressed1;
  
  if(guessed_total_messages != result1->stat.total_messages)
  {
    std::ostringstream ostr;
    ostr << "Application::evaluate: guessed_total_messages unexpectedly "
      "differes from stat.total_messages: " << guessed_total_messages
         << " and " << result1->stat.total_messages
         << " respectivelly. Expression:\n"
         << expression << "\nExpression dump:\n";
    
    condition->dump(ostr);
    
    throw Exception(ostr.str());        
  }
*/
  return result1.release();
}
