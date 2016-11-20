/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Commons/Ad/Python/Ad.hpp
 * @author Karen Arutyunov
 * $id:$
 */

#ifndef _NEWSGATE_SERVER_COMMONS_AD_PYTHON_AD_HPP_
#define _NEWSGATE_SERVER_COMMONS_AD_PYTHON_AD_HPP_

#include <stdint.h>

#include <El/Exception.hpp>

#include <El/Python/Object.hpp>
#include <El/Python/Sequence.hpp>

#include <Commons/Ad/Ad.hpp>

namespace NewsGate
{
  namespace Ad
  {
    namespace Python
    {
      EL_EXCEPTION(Exception, El::ExceptionBase);

      class Selection : public El::Python::ObjectImpl,
        public ::NewsGate::Ad::Selection
      {
      public:
        Selection(PyTypeObject *type = 0,
                  PyObject *args = 0,
                  PyObject *kwds = 0)
          throw(El::Exception);
        
        Selection(const ::NewsGate::Ad::Selection& src)
          throw(Exception, El::Exception);

        virtual ~Selection() throw() {}
          
        class Type :
          public El::Python::ObjectTypeImpl<Selection, Selection::Type>
        {
        public:
          Type() throw(El::Python::Exception, El::Exception);
          static Type instance;

          PY_TYPE_MEMBER_ULONGLONG(id, "id", "Ad placement id");
          PY_TYPE_MEMBER_ULONG(slot, "slot", "Slot id");
          PY_TYPE_MEMBER_ULONG(width, "width", "Ad width");
          PY_TYPE_MEMBER_ULONG(height, "height", "Ad height");         
          PY_TYPE_MEMBER_STRING(text, "text", "Ad text", true);
          
          PY_TYPE_MEMBER_ENUM(inject,
                              CreativeInjection,
                              CI_COUNT - 1,
                              "inject",
                              "Creative injection");
        };
      };
      
      typedef El::Python::SmartPtr<Selection> Selection_var;
      
      class SelectedCounter : public El::Python::ObjectImpl,
        public ::NewsGate::Ad::SelectedCounter
      {
      public:
        SelectedCounter(PyTypeObject *type = 0,
                        PyObject *args = 0,
                        PyObject *kwds = 0)
          throw(El::Exception);
        
        SelectedCounter(const ::NewsGate::Ad::SelectedCounter& src)
          throw(Exception, El::Exception);

        virtual ~SelectedCounter() throw() {}
          
        class Type :
          public El::Python::ObjectTypeImpl<SelectedCounter,
                                            SelectedCounter::Type>
        {
        public:
          Type() throw(El::Python::Exception, El::Exception);
          static Type instance;

          PY_TYPE_MEMBER_ULONGLONG(id, "id", "Ad placement id");
          PY_TYPE_MEMBER_STRING(text, "text", "Ad text", true);
        };
      };
      
      typedef El::Python::SmartPtr<SelectedCounter> SelectedCounter_var;
      
      class SelectionResult : public El::Python::ObjectImpl
      {
      public:
        SelectionResult(PyTypeObject *type = 0,
                        PyObject *args = 0,
                        PyObject *kwds = 0)
          throw(El::Exception);

        SelectionResult(const Ad::SelectionResult& src) throw(El::Exception);

        virtual ~SelectionResult() throw() {}
        
        void init(const Ad::SelectionResult& src) throw(El::Exception);
        
        class Type :
          public El::Python::ObjectTypeImpl<SelectionResult,
                                            SelectionResult::Type>
        {
        public:
          Type() throw(El::Python::Exception, El::Exception);
          static Type instance;

          PY_TYPE_MEMBER_OBJECT(ads,
                                El::Python::Sequence::Type,
                                "ads",
                                "Selected ads",
                                false);

          PY_TYPE_MEMBER_OBJECT(counters,
                                El::Python::Sequence::Type,
                                "counters",
                                "Selected counters",
                                false);

          PY_TYPE_MEMBER_STRING(ad_caps,
                                "ad_caps",
                                "Ad group caps",
                                true);

          PY_TYPE_MEMBER_STRING(counter_caps,
                                "counter_caps",
                                "Ad counter group caps",
                                true);
        };
          
        El::Python::Sequence_var ads;
        El::Python::Sequence_var counters;
        std::string ad_caps;
        std::string counter_caps;
      };
        
      typedef El::Python::SmartPtr<SelectionResult> SelectionResult_var;      
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Ad
  {
    namespace Python
    {
      //
      // Selection::Type class
      //
      inline
      Selection::Type::Type()
        throw(El::Python::Exception, El::Exception)
          : El::Python::ObjectTypeImpl<Selection,
                                       Selection::Type>(
                                         "newsgate.ad.Selection",
                                         "Object representing ad selection")
      {
      }
      
      //
      // SelectedCounter::Type class
      //
      inline
      SelectedCounter::Type::Type()
        throw(El::Python::Exception, El::Exception)
          : El::Python::ObjectTypeImpl<
              SelectedCounter,
              SelectedCounter::Type>("newsgate.ad.SelectedCounter",
                                     "Object representing counter selection")
      {
      }
      
      //
      // SelectionResult::Type class
      //
      inline
      SelectionResult::Type::Type()
        throw(El::Python::Exception, El::Exception)
          : El::Python::ObjectTypeImpl<SelectionResult,
                                       SelectionResult::Type>(
                                         "newsgate.ad.SelectionResult",
                                         "Object representing ad selection "
                                         "result")
      {
      }      
    }
  }
}

#endif // _NEWSGATE_SERVER_COMMONS_AD_PYTHON_AD_HPP_
