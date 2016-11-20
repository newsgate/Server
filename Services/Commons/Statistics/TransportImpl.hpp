/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Services/Commons/Statistics/TransportImpl.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_SERVICES_COMMONS_STATISTICS_TRANSPORTIMPL_HPP_
#define _NEWSGATE_SERVER_SERVICES_COMMONS_STATISTICS_TRANSPORTIMPL_HPP_

#include <stdint.h>

#include <ext/hash_map>

#include <El/Exception.hpp>
#include <El/Hash/Hash.hpp>
#include <El/CRC.hpp>
#include <El/CORBA/Transport/EntityPack.hpp>

#include <Services/Commons/Statistics/StatLogger.hpp>

namespace NewsGate
{
  namespace Statistics
  {
    namespace Transport
    {
      struct RequestInfoPackImpl
      {
        typedef El::Corba::Transport::EntityPack<
          RequestInfoPack,
          RequestInfo,
          RequestInfoArray,
          El::Corba::Transport::TE_GZIP>
        Type;
        
        typedef El::Corba::Transport::EntityPack_init<RequestInfoPack,
                                                      RequestInfoArray,
                                                      Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };

      struct PageImpressionInfoPackImpl
      {
        typedef El::Corba::Transport::EntityPack<
          PageImpressionInfoPack,
          PageImpressionInfo,
          PageImpressionInfoArray,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::EntityPack_init<
          PageImpressionInfoPack,
          PageImpressionInfoArray,
          Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };

      struct MessageImpressionInfoPackImpl
      {
        typedef El::Corba::Transport::EntityPack<
          MessageImpressionInfoPack,
          MessageImpressionInfo,
          MessageImpressionInfoArray,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::EntityPack_init<
          MessageImpressionInfoPack,
          MessageImpressionInfoArray,
          Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };

      struct MessageClickInfoPackImpl
      {
        typedef El::Corba::Transport::EntityPack<
          MessageClickInfoPack,
          MessageClickInfo,
          MessageClickInfoArray,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::EntityPack_init<MessageClickInfoPack,
                                                      MessageClickInfoArray,
                                                      Type>
        Init;

        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };
      
      struct DailyString
      {
        uint32_t date;
        std::string value;

        DailyString() : date(0) {}
        
        bool operator==(const DailyString& val) const throw(El::Exception);
        
        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);
      };

      struct DailyStringHash
      {
        size_t operator()(const DailyString& val) const throw(El::Exception);
      };

      struct DailyNumber
      {
        uint32_t date;
        uint64_t value;

        DailyNumber() : date(0), value(0) {}
        
        bool operator==(const DailyNumber& val) const throw(El::Exception);
        
        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);
      };

      struct DailyNumberHash
      {
        size_t operator()(const DailyNumber& val) const throw(El::Exception);
      };

      struct MessageImpressionClick
      {
        uint64_t impressions;
        uint64_t clicks;

        MessageImpressionClick() : impressions(0), clicks(0) {}

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);
      };

      struct MessageImpressionClickMap :
        public __gnu_cxx::hash_map<DailyNumber,
                                   MessageImpressionClick,
                                   DailyNumberHash>
      { 
        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);
      };

      struct MessageImpressionClickCounterImpl
      {
        typedef El::Corba::Transport::Entity<
          MessageImpressionClickCounter,
          MessageImpressionClickMap,
          El::Corba::Transport::TE_IDENTITY>
        Type;
        
        typedef El::Corba::Transport::Entity_init<
          MessageImpressionClickMap,
          Type>
        Init;
        
        typedef El::Corba::ValueVar<Type> Var;
        typedef El::Corba::ValueOut<Type> Out;
      };

      void register_valuetype_factories(CORBA::ORB* orb)
        throw(CORBA::Exception, El::Exception);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Statistics
  {
    namespace Transport
    {
      //
      // DailyString struct
      //
      inline
      bool
      DailyString::operator==(const DailyString& val) const
        throw(El::Exception)
      {
        return value == val.value && date == val.date;
      }
      
      inline
      void
      DailyString::write(El::BinaryOutStream& bstr) const throw(El::Exception)
      {
        bstr << date << value;
      }
        
      inline
      void
      DailyString::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> date >> value;
      }
        
      //
      // DailyStringHash struct
      //
      inline
      size_t
      DailyStringHash::operator()(const DailyString& val) const
        throw(El::Exception)
      {
        size_t crc = __gnu_cxx::__stl_hash_string(val.value.c_str());
//        size_t crc = StringConstPtrHash()(val.value);
          
        El::CRC(crc, (const unsigned char*)&val.date, sizeof(val.date));
        return crc;
      }
    
      //
      // DailyNumber struct
      //
      inline
      bool
      DailyNumber::operator==(const DailyNumber& val) const
        throw(El::Exception)
      {
        return value == val.value && date == val.date;
      }
      
      inline
      void
      DailyNumber::write(El::BinaryOutStream& bstr) const throw(El::Exception)
      {
        bstr << date << value;
      }
        
      inline
      void
      DailyNumber::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> date >> value;
      }
        
      //
      // DailyNumberHash struct
      //
      inline
      size_t
      DailyNumberHash::operator()(const DailyNumber& val) const
        throw(El::Exception)
      {
        size_t crc = ((size_t)val.value) & SIZE_MAX;
        El::CRC(crc, (const unsigned char*)&val.date, sizeof(val.date));
        return crc;
      }
    
      //
      // MessageImpressionClick struct
      //
      inline
      void
      MessageImpressionClick::write(El::BinaryOutStream& bstr) const
        throw(El::Exception)
      {
        bstr << impressions << clicks;
      }
      
      inline
      void
      MessageImpressionClick::read(El::BinaryInStream& bstr)
        throw(El::Exception)
      {
        bstr >> impressions >> clicks;
      }

      //
      // MessageImpressionClickMap struct
      //
      inline
      void
      MessageImpressionClickMap::write(El::BinaryOutStream& bstr) const
        throw(El::Exception)
      {
        bstr.write_map(*this);
      }

      inline
      void
      MessageImpressionClickMap::read(El::BinaryInStream& bstr)
        throw(El::Exception)
      {
        bstr.read_map(*this);
      }

    }
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_COMMONS_STATISTICS_TRANSPORTIMPL_HPP_
