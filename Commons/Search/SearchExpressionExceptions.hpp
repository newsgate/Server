/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Commons/Search/SearchExpressionException.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_COMMONS_SEARCH_SEARCHEXPRESSIONEXCEPTION_HPP_
#define _NEWSGATE_SERVER_COMMONS_SEARCH_SEARCHEXPRESSIONEXCEPTION_HPP_

#include <El/Exception.hpp>

namespace NewsGate
{
  namespace Search
  {
    EL_EXCEPTION(Exception, El::ExceptionBase);
    
    struct ParseError : public Exception
    {
      enum Code
      {
        NO_WORDS_FOR_ALL,
        NO_WORDS_FOR_ANY,
        NO_HOST_FOR_SITE,
        NO_PATH_FOR_URL,
        NO_MESSAGE_ID,
        NO_EVENT_ID,
        HTTPS_NOT_ALLOWED_FOR_URL,
        BAD_PATH_FOR_URL,
        BAD_MESSAGE_ID,
        BAD_EVENT_ID,
        OPERAND_EXPECTED,
        CLOSE_PARENTHESIS_EXPECTED,
        OPERATION_EXPECTED,
        NO_TIME_FOR_FETCHED,
        WRONG_TIME_FOR_FETCHED,
        NO_TIME_FOR_PUB_DATE,
        WRONG_TIME_FOR_PUB_DATE,
        LANG_OR_NOT_EXPECTED,
        LANG_EXPECTED,
        INVALID_LANG,
        COUNTRY_OR_NOT_EXPECTED,
        COUNTRY_EXPECTED,
        INVALID_COUNTRY,
        RESERVED_FEED_OR_NOT_EXPECTED,
        RESERVED_FEED_EXPECTED,
        RESERVED_UNEXPECTED_FEED,
        SPACE_OR_NOT_EXPECTED,
        SPACE_EXPECTED,
        UNEXPECTED_SPACE,
        DOMAIN_OR_NOT_EXPECTED,
        DOMAIN_EXPECTED,
        CAPACITY_OR_NOT_EXPECTED,
        CAPACITY_EXPECTED,
        WRONG_CAPACITY,
        NO_PATH_FOR_CATEGORY,
        UNEXPECTED_END_OF_EXPRESSION,
        NO_PARAMS_FOR_WITH,
        SIGNATURE_OR_NOT_EXPECTED,
        SIGNATURE_EXPECTED,
        WRONG_SIGNATURE,
        IMPRESSIONS_OR_NOT_EXPECTED,
        IMPRESSIONS_EXPECTED,
        WRONG_IMPRESSIONS,
        CLICKS_OR_NOT_EXPECTED,
        CLICKS_EXPECTED,
        WRONG_CLICKS,
        CTR_OR_NOT_EXPECTED,
        CTR_EXPECTED,
        WRONG_CTR,
        RCTR_OR_NOT_EXPECTED,
        RCTR_EXPECTED,
        WRONG_RCTR,
        WRONG_RIL,
        FEED_IMPRESSIONS_OR_NOT_EXPECTED,
        FEED_IMPRESSIONS_EXPECTED,
        WRONG_FEED_IMPRESSIONS,
        FEED_CLICKS_OR_NOT_EXPECTED,
        FEED_CLICKS_EXPECTED,
        WRONG_FEED_CLICKS,
        FEED_CTR_OR_NOT_EXPECTED,
        FEED_CTR_EXPECTED,
        WRONG_FEED_CTR,
        FEED_RCTR_OR_NOT_EXPECTED,
        FEED_RCTR_EXPECTED,
        WRONG_FEED_RCTR,
        WRONG_FEED_RIL,
        NO_TIME_FOR_VISITED,
        WRONG_TIME_FOR_VISITED
      };

      ParseError(const char* desc, unsigned long pos, Code cod) throw();

      unsigned long position;
      Code code;
    };
    
    struct NoWordsForAll : public ParseError
    {
      NoWordsForAll(const char* desc, unsigned long pos) throw();
    };

    struct NoWordsForAny : public ParseError
    {
      NoWordsForAny(const char* desc, unsigned long pos) throw();
    };

    struct NoHostForSite : public ParseError
    {
      NoHostForSite(const char* desc, unsigned long pos) throw();
    };
    
    struct NoPathForUrl : public ParseError
    {
      NoPathForUrl(const char* desc, unsigned long pos) throw();
    };
    
    struct NoPathForCategory : public ParseError
    {
      NoPathForCategory(const char* desc, unsigned long pos) throw();
    };
    
    struct NoMessageId : public ParseError
    {
      NoMessageId(const char* desc, unsigned long pos) throw();
    };
    
    struct NoEventId : public ParseError
    {
      NoEventId(const char* desc, unsigned long pos) throw();
    };
    
    struct HTTPSNotAllowedForUrl : public ParseError
    {
      HTTPSNotAllowedForUrl(const char* desc, unsigned long pos) throw();
    };

    struct BadPathForUrl : public ParseError
    {
      BadPathForUrl(const char* desc, unsigned long pos) throw();
    };
    
    struct BadMessageId : public ParseError
    {
      BadMessageId(const char* desc, unsigned long pos) throw();
    };
    
    struct BadEventId : public ParseError
    {
      BadEventId(const char* desc, unsigned long pos) throw();
    };
    
    struct OperandExpected : public ParseError
    {
      OperandExpected(const char* desc, unsigned long pos) throw();
    };
    
    struct CloseParenthesisExpected : public ParseError
    {
      CloseParenthesisExpected(const char* desc, unsigned long pos) throw();
    };
    
    struct OperationExpected : public ParseError
    {
      OperationExpected(const char* desc, unsigned long pos) throw();
    };

    struct NoDaysForFresh : public ParseError
    {
      NoDaysForFresh(const char* desc, unsigned long pos) throw();
    };

    struct WrongDaysForFresh : public ParseError
    {
      WrongDaysForFresh(const char* desc, unsigned long pos) throw();
    };    
    
    struct NoDaysForOutdated : public ParseError
    {
      NoDaysForOutdated(const char* desc, unsigned long pos) throw();
    };

    struct WrongDaysForOutdated : public ParseError
    {
      WrongDaysForOutdated(const char* desc, unsigned long pos) throw();
    };    
    
    struct NoTimeForFetched : public ParseError
    {
      NoTimeForFetched(const char* desc, unsigned long pos) throw();
    };

    struct WrongTimeForFetched : public ParseError
    {
      WrongTimeForFetched(const char* desc, unsigned long pos) throw();
    };
    
    struct NoTimeForVisited : public ParseError
    {
      NoTimeForVisited(const char* desc, unsigned long pos) throw();
    };

    struct WrongTimeForVisited : public ParseError
    {
      WrongTimeForVisited(const char* desc, unsigned long pos) throw();
    };
    
    struct NoTimeForPubDate : public ParseError
    {
      NoTimeForPubDate(const char* desc, unsigned long pos) throw();
    };

    struct WrongTimeForPubDate : public ParseError
    {
      WrongTimeForPubDate(const char* desc, unsigned long pos) throw();
    };
    
    struct LangOrNotExpected : public ParseError
    {
      LangOrNotExpected(const char* desc, unsigned long pos) throw();
    };    
    
    struct LangExpected : public ParseError
    {
      LangExpected(const char* desc, unsigned long pos) throw();
    };
    
    struct InvalidLang : public ParseError
    {
      InvalidLang(const char* desc, unsigned long pos) throw();
    };
    
    struct CountryOrNotExpected : public ParseError
    {
      CountryOrNotExpected(const char* desc, unsigned long pos) throw();
    };    
    
    struct CountryExpected : public ParseError
    {
      CountryExpected(const char* desc, unsigned long pos) throw();
    };
    
    struct InvalidCountry : public ParseError
    {
      InvalidCountry(const char* desc, unsigned long pos) throw();
    };
    
    struct FeedOrNotExpected : public ParseError
    {
      FeedOrNotExpected(const char* desc, unsigned long pos) throw();
    };    
    
    struct FeedExpected : public ParseError
    {
      FeedExpected(const char* desc, unsigned long pos) throw();
    };    
    
    struct UnexpectedFeed : public ParseError
    {
      UnexpectedFeed(const char* desc, unsigned long pos) throw();
    };    
    
    struct SpaceOrNotExpected : public ParseError
    {
      SpaceOrNotExpected(const char* desc, unsigned long pos) throw();
    };    
    
    struct SpaceExpected : public ParseError
    {
      SpaceExpected(const char* desc, unsigned long pos) throw();
    };    
    
    struct UnexpectedSpace : public ParseError
    {
      UnexpectedSpace(const char* desc, unsigned long pos) throw();
    };    
    
    struct DomainOrNotExpected : public ParseError
    {
      DomainOrNotExpected(const char* desc, unsigned long pos) throw();
    };    
    
    struct DomainExpected : public ParseError
    {
      DomainExpected(const char* desc, unsigned long pos) throw();
    };
    
    struct CapacityOrNotExpected : public ParseError
    {
      CapacityOrNotExpected(const char* desc, unsigned long pos) throw();
    };    
    
    struct CapacityExpected : public ParseError
    {
      CapacityExpected(const char* desc, unsigned long pos) throw();
    };
    
    struct WrongCapacity : public ParseError
    {
      WrongCapacity(const char* desc, unsigned long pos) throw();
    };

    struct ImpressionsOrNotExpected : public ParseError
    {
      ImpressionsOrNotExpected(const char* desc, unsigned long pos) throw();
    };    
    
    struct ImpressionsExpected : public ParseError
    {
      ImpressionsExpected(const char* desc, unsigned long pos) throw();
    };
    
    struct WrongImpressions : public ParseError
    {
      WrongImpressions(const char* desc, unsigned long pos) throw();
    };

    struct FeedImpressionsOrNotExpected : public ParseError
    {
      FeedImpressionsOrNotExpected(const char* desc,
                                   unsigned long pos) throw();
    };    
    
    struct FeedImpressionsExpected : public ParseError
    {
      FeedImpressionsExpected(const char* desc, unsigned long pos) throw();
    };
    
    struct WrongFeedImpressions : public ParseError
    {
      WrongFeedImpressions(const char* desc, unsigned long pos) throw();
    };

    struct ClicksOrNotExpected : public ParseError
    {
      ClicksOrNotExpected(const char* desc, unsigned long pos) throw();
    };    
    
    struct ClicksExpected : public ParseError
    {
      ClicksExpected(const char* desc, unsigned long pos) throw();
    };
    
    struct WrongClicks : public ParseError
    {
      WrongClicks(const char* desc, unsigned long pos) throw();
    };

    struct FeedClicksOrNotExpected : public ParseError
    {
      FeedClicksOrNotExpected(const char* desc, unsigned long pos) throw();
    };    
    
    struct FeedClicksExpected : public ParseError
    {
      FeedClicksExpected(const char* desc, unsigned long pos) throw();
    };
    
    struct WrongFeedClicks : public ParseError
    {
      WrongFeedClicks(const char* desc, unsigned long pos) throw();
    };

    struct CTR_OrNotExpected : public ParseError
    {
      CTR_OrNotExpected(const char* desc, unsigned long pos) throw();
    };    
    
    struct CTR_Expected : public ParseError
    {
      CTR_Expected(const char* desc, unsigned long pos) throw();
    };
    
    struct WrongCTR : public ParseError
    {
      WrongCTR(const char* desc, unsigned long pos) throw();
    };

    struct FeedCTR_OrNotExpected : public ParseError
    {
      FeedCTR_OrNotExpected(const char* desc, unsigned long pos) throw();
    };    
    
    struct FeedCTR_Expected : public ParseError
    {
      FeedCTR_Expected(const char* desc, unsigned long pos) throw();
    };
    
    struct WrongFeedCTR : public ParseError
    {
      WrongFeedCTR(const char* desc, unsigned long pos) throw();
    };

    struct RCTR_OrNotExpected : public ParseError
    {
      RCTR_OrNotExpected(const char* desc, unsigned long pos) throw();
    };    
    
    struct RCTR_Expected : public ParseError
    {
      RCTR_Expected(const char* desc, unsigned long pos) throw();
    };
    
    struct WrongRCTR : public ParseError
    {
      WrongRCTR(const char* desc, unsigned long pos) throw();
    };

    struct WrongRIL : public ParseError
    {
      WrongRIL(const char* desc, unsigned long pos) throw();
    };

    struct FeedRCTR_OrNotExpected : public ParseError
    {
      FeedRCTR_OrNotExpected(const char* desc, unsigned long pos) throw();
    };    
    
    struct FeedRCTR_Expected : public ParseError
    {
      FeedRCTR_Expected(const char* desc, unsigned long pos) throw();
    };
    
    struct WrongFeedRCTR : public ParseError
    {
      WrongFeedRCTR(const char* desc, unsigned long pos) throw();
    };

    struct WrongFeedRIL : public ParseError
    {
      WrongFeedRIL(const char* desc, unsigned long pos) throw();
    };

    struct SignatureOrNotExpected : public ParseError
    {
      SignatureOrNotExpected(const char* desc, unsigned long pos) throw();
    };    
    
    struct SignatureExpected : public ParseError
    {
      SignatureExpected(const char* desc, unsigned long pos) throw();
    };
    
    struct WrongSignature : public ParseError
    {
      WrongSignature(const char* desc, unsigned long pos) throw();
    };

    struct UnexpectedEndOfExpression : public ParseError
    {
      UnexpectedEndOfExpression(const char* desc, unsigned long pos) throw();
    };

    struct NoParamsForWith : public ParseError
    {
      NoParamsForWith(const char* desc, unsigned long pos) throw();
    };

  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Search
  {
    //
    // ParseError exception
    //
    inline
    ParseError::ParseError(const char* desc, unsigned long pos, Code cod)
      throw()
        : Exception(desc), position(pos), code(cod)
    {
    }

    //
    // NoWordsForAll exception
    //
    inline
    NoWordsForAll::NoWordsForAll(const char* desc, unsigned long pos) throw()
        : ParseError(desc, pos, NO_WORDS_FOR_ALL)
    {
    }

    //
    // NoWordsForAny exception
    //
    inline
    NoWordsForAny::NoWordsForAny(const char* desc, unsigned long pos) throw()
        : ParseError(desc, pos, NO_WORDS_FOR_ANY)
    {
    }
    
    //
    // NoHostForSite exception
    //
    inline
    NoHostForSite::NoHostForSite(const char* desc, unsigned long pos) throw()
        : ParseError(desc, pos, NO_HOST_FOR_SITE)
    {
    }

    //
    // NoPathForUrl exception
    //
    inline
    NoPathForUrl::NoPathForUrl(const char* desc, unsigned long pos) throw()
        : ParseError(desc, pos, NO_PATH_FOR_URL)
    {
    }
    
    //
    // NoPathForCategory exception
    //
    inline
    NoPathForCategory::NoPathForCategory(const char* desc, unsigned long pos)
      throw() : ParseError(desc, pos, NO_PATH_FOR_CATEGORY)
    {
    }
    
    //
    // NoMessageId exception
    //
    inline
    NoMessageId::NoMessageId(const char* desc, unsigned long pos) throw()
        : ParseError(desc, pos, NO_MESSAGE_ID)
    {
    }
    
    //
    // NoEventId exception
    //
    inline
    NoEventId::NoEventId(const char* desc, unsigned long pos) throw()
        : ParseError(desc, pos, NO_EVENT_ID)
    {
    }
    
    //
    // HTTPSNotAllowedForUrl exception
    //
    inline
    HTTPSNotAllowedForUrl::HTTPSNotAllowedForUrl(const char* desc,
                                                 unsigned long pos)
      throw() : ParseError(desc, pos, HTTPS_NOT_ALLOWED_FOR_URL)
    {
    }
    
    //
    // BadPathForUrl exception
    //
    inline
    BadPathForUrl::BadPathForUrl(const char* desc, unsigned long pos)
      throw() : ParseError(desc, pos, BAD_PATH_FOR_URL)
    {
    }
    
    //
    // BadMessageId exception
    //
    inline
    BadMessageId::BadMessageId(const char* desc, unsigned long pos)
      throw() : ParseError(desc, pos, BAD_MESSAGE_ID)
    {
    }
    
    //
    // BadEventId exception
    //
    inline
    BadEventId::BadEventId(const char* desc, unsigned long pos)
      throw() : ParseError(desc, pos, BAD_EVENT_ID)
    {
    }
    
    //
    // OperandExpected exception
    //
    inline
    OperandExpected::OperandExpected(const char* desc, unsigned long pos)
      throw() : ParseError(desc, pos, OPERAND_EXPECTED)
    {
    }
    
    //
    // CloseParenthesisExpected exception
    //
    inline
    CloseParenthesisExpected::CloseParenthesisExpected(const char* desc,
                                                       unsigned long pos)
      throw() : ParseError(desc, pos, CLOSE_PARENTHESIS_EXPECTED)
    {
    }
    
    //
    // OperationExpected exception
    //
    inline
    OperationExpected::OperationExpected(const char* desc,
                                         unsigned long pos)
      throw() : ParseError(desc, pos, OPERATION_EXPECTED)
    {
    }
    
    //
    // NoTimeForFetched exception
    //
    inline
    NoTimeForFetched::NoTimeForFetched(const char* desc, unsigned long pos)
      throw() : ParseError(desc, pos, NO_TIME_FOR_FETCHED)
    {
    }
    
    //
    // WrongTimeForFetched exception
    //
    inline
    WrongTimeForFetched::WrongTimeForFetched(const char* desc,
                                             unsigned long pos)
      throw() : ParseError(desc, pos, WRONG_TIME_FOR_FETCHED)
    {
    }
    
    //
    // NoTimeForVisited exception
    //
    inline
    NoTimeForVisited::NoTimeForVisited(const char* desc, unsigned long pos)
      throw() : ParseError(desc, pos, NO_TIME_FOR_VISITED)
    {
    }
    
    //
    // WrongTimeForVisited exception
    //
    inline
    WrongTimeForVisited::WrongTimeForVisited(const char* desc,
                                             unsigned long pos)
      throw() : ParseError(desc, pos, WRONG_TIME_FOR_VISITED)
    {
    }
    
    //
    // NoTimeForPubDate exception
    //
    inline
    NoTimeForPubDate::NoTimeForPubDate(const char* desc, unsigned long pos)
      throw() : ParseError(desc, pos, NO_TIME_FOR_PUB_DATE)
    {
    }
    
    //
    // WrongTimeForPubDate exception
    //
    inline
    WrongTimeForPubDate::WrongTimeForPubDate(const char* desc,
                                             unsigned long pos)
      throw() : ParseError(desc, pos, WRONG_TIME_FOR_PUB_DATE)
    {
    }
    
    //
    // LangOrNotExpected exception
    //
    inline
    LangOrNotExpected::LangOrNotExpected(const char* desc, unsigned long pos)
      throw() : ParseError(desc, pos, LANG_OR_NOT_EXPECTED)
    {
    }
    
    //
    // InvalidLang exception
    //
    inline
    InvalidLang::InvalidLang(const char* desc, unsigned long pos)
      throw() : ParseError(desc, pos, INVALID_LANG)
    {
    }
    
    //
    // LangExpected exception
    //
    inline
    LangExpected::LangExpected(const char* desc, unsigned long pos)
      throw() : ParseError(desc, pos, LANG_EXPECTED)
    {
    }
    
    //
    // CountryOrNotExpected exception
    //
    inline
    CountryOrNotExpected::CountryOrNotExpected(const char* desc,
                                               unsigned long pos)
      throw() : ParseError(desc, pos, COUNTRY_OR_NOT_EXPECTED)
    {
    }
    
    //
    // InvalidCountry exception
    //
    inline
    InvalidCountry::InvalidCountry(const char* desc, unsigned long pos)
      throw() : ParseError(desc, pos, INVALID_COUNTRY)
    {
    }
    
    //
    // CountryExpected exception
    //
    inline
    CountryExpected::CountryExpected(const char* desc, unsigned long pos)
      throw() : ParseError(desc, pos, COUNTRY_EXPECTED)
    {
    }
    
    //
    // SpaceOrNotExpected exception
    //
    inline
    SpaceOrNotExpected::SpaceOrNotExpected(const char* desc, unsigned long pos)
      throw() : ParseError(desc, pos, SPACE_OR_NOT_EXPECTED)
    {
    }
    
    //
    // SpaceExpected exception
    //
    inline
    SpaceExpected::SpaceExpected(const char* desc, unsigned long pos)
      throw() : ParseError(desc, pos, SPACE_EXPECTED)
    {
    }
    
    //
    // UnexpectedSpace exception
    //
    inline
    UnexpectedSpace::UnexpectedSpace(const char* desc, unsigned long pos)
      throw() : ParseError(desc, pos, UNEXPECTED_SPACE)
    {
    }
    
    //
    // DomainOrNotExpected exception
    //
    inline
    DomainOrNotExpected::DomainOrNotExpected(const char* desc,
                                             unsigned long pos)
      throw() : ParseError(desc, pos, DOMAIN_OR_NOT_EXPECTED)
    {
    }
    
    //
    // DomainExpected exception
    //
    inline
    DomainExpected::DomainExpected(const char* desc, unsigned long pos)
      throw() : ParseError(desc, pos, DOMAIN_EXPECTED)
    {
    }
    
    //
    // CapacityOrNotExpected exception
    //
    inline
    CapacityOrNotExpected::CapacityOrNotExpected(const char* desc,
                                                 unsigned long pos)
      throw() : ParseError(desc, pos, CAPACITY_OR_NOT_EXPECTED)
    {
    }
    
    //
    // CapacityExpected exception
    //
    inline
    CapacityExpected::CapacityExpected(const char* desc, unsigned long pos)
      throw() : ParseError(desc, pos, CAPACITY_EXPECTED)
    {
    }
    
    //
    // WrongCapacity exception
    //
    inline
    WrongCapacity::WrongCapacity(const char* desc, unsigned long pos)
      throw() : ParseError(desc, pos, WRONG_CAPACITY)
    {
    }
    
    //
    // ImpressionsOrNotExpected exception
    //
    inline
    ImpressionsOrNotExpected::ImpressionsOrNotExpected(const char* desc,
                                                       unsigned long pos)
      throw() : ParseError(desc, pos, IMPRESSIONS_OR_NOT_EXPECTED)
    {
    }
    
    //
    // ImpressionsExpected exception
    //
    inline
    ImpressionsExpected::ImpressionsExpected(const char* desc,
                                             unsigned long pos)
      throw() : ParseError(desc, pos, IMPRESSIONS_EXPECTED)
    {
    }
    
    //
    // WrongImpressions exception
    //
    inline
    WrongImpressions::WrongImpressions(const char* desc, unsigned long pos)
      throw() : ParseError(desc, pos, WRONG_IMPRESSIONS)
    {
    }
    
    //
    // FeedImpressionsOrNotExpected exception
    //
    inline
    FeedImpressionsOrNotExpected::FeedImpressionsOrNotExpected(
      const char* desc,
      unsigned long pos)
      throw() : ParseError(desc, pos, FEED_IMPRESSIONS_OR_NOT_EXPECTED)
    {
    }
    
    //
    // FeedImpressionsExpected exception
    //
    inline
    FeedImpressionsExpected::FeedImpressionsExpected(const char* desc,
                                                     unsigned long pos)
      throw() : ParseError(desc, pos, FEED_IMPRESSIONS_EXPECTED)
    {
    }
    
    //
    // WrongFeedImpressions exception
    //
    inline
    WrongFeedImpressions::WrongFeedImpressions(const char* desc,
                                               unsigned long pos)
      throw() : ParseError(desc, pos, WRONG_FEED_IMPRESSIONS)
    {
    }
    
    //
    // ClicksOrNotExpected exception
    //
    inline
    ClicksOrNotExpected::ClicksOrNotExpected(const char* desc,
                                             unsigned long pos)
      throw() : ParseError(desc, pos, CLICKS_OR_NOT_EXPECTED)
    {
    }
    
    //
    // ClicksExpected exception
    //
    inline
    ClicksExpected::ClicksExpected(const char* desc, unsigned long pos)
      throw() : ParseError(desc, pos, CLICKS_EXPECTED)
    {
    }
    
    //
    // WrongClicks exception
    //
    inline
    WrongClicks::WrongClicks(const char* desc, unsigned long pos)
      throw() : ParseError(desc, pos, WRONG_CLICKS)
    {
    }
    
    //
    // FeedClicksOrNotExpected exception
    //
    inline
    FeedClicksOrNotExpected::FeedClicksOrNotExpected(const char* desc,
                                                     unsigned long pos)
      throw() : ParseError(desc, pos, FEED_CLICKS_OR_NOT_EXPECTED)
    {
    }
    
    //
    // FeedClicksExpected exception
    //
    inline
    FeedClicksExpected::FeedClicksExpected(const char* desc, unsigned long pos)
      throw() : ParseError(desc, pos, FEED_CLICKS_EXPECTED)
    {
    }
    
    //
    // WrongFeedClicks exception
    //
    inline
    WrongFeedClicks::WrongFeedClicks(const char* desc, unsigned long pos)
      throw() : ParseError(desc, pos, WRONG_FEED_CLICKS)
    {
    }
    
    //
    // CTR_OrNotExpected exception
    //
    inline
    CTR_OrNotExpected::CTR_OrNotExpected(const char* desc, unsigned long pos)
      throw() : ParseError(desc, pos, CTR_OR_NOT_EXPECTED)
    {
    }
    
    //
    // CTR_Expected exception
    //
    inline
    CTR_Expected::CTR_Expected(const char* desc, unsigned long pos) throw()
        : ParseError(desc, pos, CTR_EXPECTED)
    {
    }
    
    //
    // WrongCTR exception
    //
    inline
    WrongCTR::WrongCTR(const char* desc, unsigned long pos) throw()
        : ParseError(desc, pos, WRONG_CTR)
    {
    }
    
    //
    // FeedCTR_OrNotExpected exception
    //
    inline
    FeedCTR_OrNotExpected::FeedCTR_OrNotExpected(const char* desc,
                                                 unsigned long pos)
      throw() : ParseError(desc, pos, FEED_CTR_OR_NOT_EXPECTED)
    {
    }
    
    //
    // FeedCTR_Expected exception
    //
    inline
    FeedCTR_Expected::FeedCTR_Expected(const char* desc,
                                       unsigned long pos) throw()
        : ParseError(desc, pos, FEED_CTR_EXPECTED)
    {
    }
    
    //
    // WrongFeedCTR exception
    //
    inline
    WrongFeedCTR::WrongFeedCTR(const char* desc, unsigned long pos) throw()
        : ParseError(desc, pos, WRONG_FEED_CTR)
    {
    }
    
    //
    // RCTR_OrNotExpected exception
    //
    inline
    RCTR_OrNotExpected::RCTR_OrNotExpected(const char* desc, unsigned long pos)
      throw() : ParseError(desc, pos, RCTR_OR_NOT_EXPECTED)
    {
    }
    
    //
    // RCTR_Expected exception
    //
    inline
    RCTR_Expected::RCTR_Expected(const char* desc, unsigned long pos) throw()
        : ParseError(desc, pos, RCTR_EXPECTED)
    {
    }
    
    //
    // WrongRCTR exception
    //
    inline
    WrongRCTR::WrongRCTR(const char* desc, unsigned long pos) throw()
        : ParseError(desc, pos, WRONG_RCTR)
    {
    }
    
    //
    // WrongRIL exception
    //
    inline
    WrongRIL::WrongRIL(const char* desc, unsigned long pos) throw()
        : ParseError(desc, pos, WRONG_RIL)
    {
    }
    
    //
    // FeedRCTR_OrNotExpected exception
    //
    inline
    FeedRCTR_OrNotExpected::FeedRCTR_OrNotExpected(const char* desc,
                                                   unsigned long pos)
      throw() : ParseError(desc, pos, FEED_RCTR_OR_NOT_EXPECTED)
    {
    }
    
    //
    // FeedRCTR_Expected exception
    //
    inline
    FeedRCTR_Expected::FeedRCTR_Expected(const char* desc,
                                         unsigned long pos) throw()
        : ParseError(desc, pos, FEED_RCTR_EXPECTED)
    {
    }
    
    //
    // WrongFeedRCTR exception
    //
    inline
    WrongFeedRCTR::WrongFeedRCTR(const char* desc, unsigned long pos) throw()
        : ParseError(desc, pos, WRONG_FEED_RCTR)
    {
    }
    
    //
    // WrongFeedRIL exception
    //
    inline
    WrongFeedRIL::WrongFeedRIL(const char* desc, unsigned long pos) throw()
        : ParseError(desc, pos, WRONG_FEED_RIL)
    {
    }
    
    //
    // SignatureOrNotExpected exception
    //
    inline
    SignatureOrNotExpected::SignatureOrNotExpected(const char* desc,
                                                 unsigned long pos)
      throw() : ParseError(desc, pos, SIGNATURE_OR_NOT_EXPECTED)
    {
    }
    
    //
    // SignatureExpected exception
    //
    inline
    SignatureExpected::SignatureExpected(const char* desc, unsigned long pos)
      throw() : ParseError(desc, pos, SIGNATURE_EXPECTED)
    {
    }
    
    //
    // WrongSignature exception
    //
    inline
    WrongSignature::WrongSignature(const char* desc, unsigned long pos)
      throw() : ParseError(desc, pos, WRONG_SIGNATURE)
    {
    }
    
    //
    // UnexpectedEndOfExpression exception
    //
    inline
    UnexpectedEndOfExpression::UnexpectedEndOfExpression(const char* desc,
                                                         unsigned long pos)
      throw() : ParseError(desc, pos, UNEXPECTED_END_OF_EXPRESSION)
    {
    }

    //
    // NoParamsForWith exception
    //
    inline
    NoParamsForWith::NoParamsForWith(const char* desc, unsigned long pos)
      throw() : ParseError(desc, pos, NO_PARAMS_FOR_WITH)
    {
    }
  }
}

#endif // _NEWSGATE_SERVER_COMMONS_SEARCH_SEARCHEXPRESSIONEXCEPTION_HPP_
