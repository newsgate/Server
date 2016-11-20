/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Commons/FraudPrevention/LimitCheck.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_SERVICES_COMMONS_FRAUDPREVENTION_LIMITCHECK_HPP_
#define _NEWSGATE_SERVER_SERVICES_COMMONS_FRAUDPREVENTION_LIMITCHECK_HPP_

#include <stdint.h>

#include <vector>

#include <El/Exception.hpp>
#include <El/Hash/Hash.hpp>
#include <El/CRC.hpp>
#include <El/BinaryStream.hpp>

namespace NewsGate
{
  namespace FraudPrevention
  {
    EL_EXCEPTION(Exception, El::ExceptionBase);
    
    enum EventType
    {
      ET_CLICK,
      ET_ADD_SEARCH_MAIL,
      ET_UPDATE_SEARCH_MAIL
    };
    
    struct EventLimitCheckDesc
    {
      uint8_t type;
      
      bool user;
      bool ip;
      bool item;
      uint32_t interval;
      uint32_t times;

      EventLimitCheckDesc() throw();
    };

    struct EventLimitCheckDescArray: public std::vector<EventLimitCheckDesc>
    {
      void add_check_descriptors(FraudPrevention::EventType type,
                                 bool user,
                                 bool ip,
                                 bool item,
                                 const char* freqs)
        throw(Exception, El::Exception);
    };

    struct EventFreq
    {
      uint64_t event;
      uint32_t interval;
      uint32_t times;

      static EventFreq null;

      EventFreq() : event(0), interval(0), times(0) {}
      EventFreq(uint64_t e, uint32_t i, uint32_t t) throw();        
      bool operator==(const EventFreq& val) const throw();

      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);

      void event_from_string(const char* name) throw();        
    };

    struct EventFreqHash
    {
      size_t operator()(const EventFreq& val) const throw(El::Exception);
    };
      
    struct EventLimitCheck
    {
      EventFreq event_freq;
      uint32_t count;

      EventLimitCheck() throw() : count(0) {}
      
      EventLimitCheck(const char* e,
                      uint32_t c,
                      uint32_t t,
                      uint32_t i) throw();
      
      EventLimitCheck(uint64_t e, uint32_t c, uint32_t t, uint32_t i) throw();

      void update_event(const unsigned char* buff, size_t size)
        throw(El::Exception);
        
      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };

    typedef std::vector<EventLimitCheck> EventLimitCheckArray;

    struct EventLimitCheckResult
    {
      uint8_t limit_exceeded;

      EventLimitCheckResult() : limit_exceeded(0) {}
      EventLimitCheckResult(uint8_t e) : limit_exceeded(e) {}
        
      void write(El::BinaryOutStream& bstr) const throw(El::Exception);
      void read(El::BinaryInStream& bstr) throw(El::Exception);
    };

    typedef std::vector<EventLimitCheckResult> EventLimitCheckResultArray;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace FraudPrevention
  {
    //
    // EventLimitCheckDesc struct
    //
    inline
    EventLimitCheckDesc::EventLimitCheckDesc() throw()
        : type(ET_CLICK),
          user(false),
          ip(false),
          item(false),
          interval(0),
          times(0)
    {
    }

    //
    // EventInterval struct
    //
    inline
    EventFreq::EventFreq(uint64_t e, uint32_t i, uint32_t t) throw()
        : event(e),
          interval(i),
          times(t)
    {
    }
    
    inline
    bool
    EventFreq::operator==(const EventFreq& val) const throw()
    {
      return event == val.event && interval == val.interval &&
        times == val.times;
    }

    inline
    void
    EventFreq::write(El::BinaryOutStream& bstr) const throw(El::Exception)
    {
      bstr << event << interval << times;
    }
      
    inline
    void
    EventFreq::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> event >> interval >> times;
    }
      
    inline
    void
    EventFreq::event_from_string(const char* name) throw()
    {
      event = 0;
      El::CRC(event, (const unsigned char*)name, strlen(name));        
    }

    //
    // EventFreqHash struct
    //
    inline
    size_t
    EventFreqHash::operator()(const EventFreq& val) const
      throw(El::Exception)
    {
      size_t crc = 0;
      El::CRC(crc, (const unsigned char*)&val.event, sizeof(val.event));
        
      El::CRC(crc,
              (const unsigned char*)&val.interval,
              sizeof(val.interval));
        
      El::CRC(crc, (const unsigned char*)&val.times, sizeof(val.times));
      
      return crc;
    }
      
    //
    // EventLimitCheck struct
    //
    inline
    EventLimitCheck::EventLimitCheck(const char* e,
                                     uint32_t c,
                                     uint32_t t,
                                     uint32_t i) throw()
        : event_freq(0, i, t),
          count(c)
    {
      event_freq.event_from_string(e);
    }
      
    inline
    EventLimitCheck::EventLimitCheck(uint64_t e,
                                     uint32_t c,
                                     uint32_t t,
                                     uint32_t i) throw()
        : event_freq(e, i, t),
          count(c)
    {
    }
      
    inline
    void
    EventLimitCheck::update_event(const unsigned char* buff,
                                  size_t size)
      throw(El::Exception)
    {
      El::CRC(event_freq.event, buff, size);
    }
    
    inline
    void
    EventLimitCheck::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr << event_freq << count;
    }
        
    inline
    void
    EventLimitCheck::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> event_freq >> count;
    }

    //
    // EventLimitCheckResult struct
    //
    inline
    void
    EventLimitCheckResult::write(El::BinaryOutStream& bstr) const
      throw(El::Exception)
    {
      bstr << limit_exceeded;
    }
        
    inline
    void
    EventLimitCheckResult::read(El::BinaryInStream& bstr) throw(El::Exception)
    {
      bstr >> limit_exceeded;
    }
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_COMMONS_FRAUDPREVENTION_LIMITCHECK_HPP_
