/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file NewsGate/Server/Commons/Ad/Python/Ad.cpp
 * @author Karen Arutyunov
 * $id:$
 */

#include <Python.h>

#include <El/Exception.hpp>

#include <El/Python/Object.hpp>
#include <El/Python/Module.hpp>

#include "Ad.hpp"

namespace NewsGate
{
  namespace Ad
  {
    namespace Python
    {
      Selection::Type Selection::Type::instance;
      SelectedCounter::Type SelectedCounter::Type::instance;
      SelectionResult::Type SelectionResult::Type::instance;

      //
      // AdPyModule class
      //
      class AdPyModule : public El::Python::ModuleImpl<AdPyModule>
      {
      public:
        static AdPyModule instance;
        
        AdPyModule() throw(El::Exception);
        
        virtual void initialized() throw(El::Exception);
      };

      AdPyModule::AdPyModule() throw(El::Exception)
          : El::Python::ModuleImpl<AdPyModule>(
            "newsgate.ad",
            "Module containing ad related types.",
            true)
      {
      }
        
      void
      AdPyModule::initialized() throw(El::Exception)
      {
        add_member(PyLong_FromLong(QT_NONE), "QT_NONE");
        add_member(PyLong_FromLong(QT_ANY), "QT_ANY");
        add_member(PyLong_FromLong(QT_SEARCH), "QT_SEARCH");
        add_member(PyLong_FromLong(QT_EVENT), "QT_EVENT");
        add_member(PyLong_FromLong(QT_CATEGORY), "QT_CATEGORY");
        add_member(PyLong_FromLong(QT_SOURCE), "QT_SOURCE");
        add_member(PyLong_FromLong(QT_MESSAGE), "QT_MESSAGE");

        add_member(PyLong_FromLong(PI_UNKNOWN), "PI_UNKNOWN");
        add_member(PyLong_FromLong(PI_DESK_PAPER), "PI_DESK_PAPER");
        add_member(PyLong_FromLong(PI_DESK_NLINE), "PI_DESK_NLINE");
        add_member(PyLong_FromLong(PI_DESK_COLUMN), "PI_DESK_COLUMN");
        add_member(PyLong_FromLong(PI_TAB_PAPER), "PI_TAB_PAPER");
        add_member(PyLong_FromLong(PI_TAB_NLINE), "PI_TAB_NLINE");
        add_member(PyLong_FromLong(PI_TAB_COLUMN), "PI_TAB_COLUMN");
        add_member(PyLong_FromLong(PI_MOB_NLINE), "PI_MOB_NLINE");
        add_member(PyLong_FromLong(PI_MOB_COLUMN), "PI_MOB_COLUMN");
        add_member(PyLong_FromLong(PI_DESK_MESSAGE), "PI_DESK_MESSAGE");
        add_member(PyLong_FromLong(PI_TAB_MESSAGE), "PI_TAB_MESSAGE");
        add_member(PyLong_FromLong(PI_MOB_MESSAGE), "PI_MOB_MESSAGE");

        add_member(PyLong_FromLong(SI_UNKNOWN), "SI_UNKNOWN");
        
        add_member(PyLong_FromLong(SI_DESK_PAPER_FIRST),"SI_DESK_PAPER_FIRST");
        add_member(PyLong_FromLong(SI_DESK_PAPER_MSG1), "SI_DESK_PAPER_MSG1");
        add_member(PyLong_FromLong(SI_DESK_PAPER_MSG2), "SI_DESK_PAPER_MSG2");
        add_member(PyLong_FromLong(SI_DESK_PAPER_MSA1), "SI_DESK_PAPER_MSA1");
        add_member(PyLong_FromLong(SI_DESK_PAPER_MSA2), "SI_DESK_PAPER_MSA2");
        add_member(PyLong_FromLong(SI_DESK_PAPER_RTB1), "SI_DESK_PAPER_RTB1");
        add_member(PyLong_FromLong(SI_DESK_PAPER_ROOF), "SI_DESK_PAPER_ROOF");
        
        add_member(PyLong_FromLong(SI_DESK_PAPER_BASEMENT),
                   "SI_DESK_PAPER_BASEMENT");
        
        add_member(PyLong_FromLong(SI_DESK_PAPER_LAST), "SI_DESK_PAPER_LAST");
      
        add_member(PyLong_FromLong(SI_DESK_NLINE_FIRST),"SI_DESK_NLINE_FIRST");
      
        add_member(PyLong_FromLong(SI_DESK_NLINE_MSA1),
                   "SI_DESK_NLINE_MSA1");
      
        add_member(PyLong_FromLong(SI_DESK_NLINE_RTB1), "SI_DESK_NLINE_RTB1");
        add_member(PyLong_FromLong(SI_DESK_NLINE_RTB2), "SI_DESK_NLINE_RTB2");
        add_member(PyLong_FromLong(SI_DESK_NLINE_ROOF), "SI_DESK_NLINE_ROOF");
        
        add_member(PyLong_FromLong(SI_DESK_NLINE_BASEMENT),
                   "SI_DESK_NLINE_BASEMENT");
        
        add_member(PyLong_FromLong(SI_DESK_NLINE_LAST), "SI_DESK_NLINE_LAST");
        
        add_member(PyLong_FromLong(SI_DESK_COLUMN_FIRST),
                   "SI_DESK_COLUMN_FIRST");
        
        add_member(PyLong_FromLong(SI_DESK_COLUMN_MSG1),"SI_DESK_COLUMN_MSG1");
        add_member(PyLong_FromLong(SI_DESK_COLUMN_MSG2),"SI_DESK_COLUMN_MSG2");
        add_member(PyLong_FromLong(SI_DESK_COLUMN_MSA1),"SI_DESK_COLUMN_MSA1");
        add_member(PyLong_FromLong(SI_DESK_COLUMN_RTB1),"SI_DESK_COLUMN_RTB1");
        add_member(PyLong_FromLong(SI_DESK_COLUMN_RTB2),"SI_DESK_COLUMN_RTB2");
        add_member(PyLong_FromLong(SI_DESK_COLUMN_ROOF),"SI_DESK_COLUMN_ROOF");
        
        add_member(PyLong_FromLong(SI_DESK_COLUMN_BASEMENT),
                   "SI_DESK_COLUMN_BASEMENT");
        
        add_member(PyLong_FromLong(SI_DESK_COLUMN_LAST),"SI_DESK_COLUMN_LAST");

        add_member(PyLong_FromLong(SI_TAB_PAPER_FIRST),"SI_TAB_PAPER_FIRST");
        add_member(PyLong_FromLong(SI_TAB_PAPER_MSG1), "SI_TAB_PAPER_MSG1");
        add_member(PyLong_FromLong(SI_TAB_PAPER_MSG2), "SI_TAB_PAPER_MSG2");
        add_member(PyLong_FromLong(SI_TAB_PAPER_MSA1), "SI_TAB_PAPER_MSA1");
        add_member(PyLong_FromLong(SI_TAB_PAPER_MSA2), "SI_TAB_PAPER_MSA2");
        add_member(PyLong_FromLong(SI_TAB_PAPER_RTB1), "SI_TAB_PAPER_RTB1");
        add_member(PyLong_FromLong(SI_TAB_PAPER_RTB2), "SI_TAB_PAPER_RTB2");
        add_member(PyLong_FromLong(SI_TAB_PAPER_ROOF), "SI_TAB_PAPER_ROOF");
        
        add_member(PyLong_FromLong(SI_TAB_PAPER_BASEMENT),
                   "SI_TAB_PAPER_BASEMENT");
        
        add_member(PyLong_FromLong(SI_TAB_PAPER_LAST), "SI_TAB_PAPER_LAST");

        add_member(PyLong_FromLong(SI_TAB_NLINE_FIRST),"SI_TAB_NLINE_FIRST");
      
        add_member(PyLong_FromLong(SI_TAB_NLINE_MSA1),
                   "SI_TAB_NLINE_MSA1");
      
        add_member(PyLong_FromLong(SI_TAB_NLINE_RTB1), "SI_TAB_NLINE_RTB1");
        add_member(PyLong_FromLong(SI_TAB_NLINE_RTB2), "SI_TAB_NLINE_RTB2");
        add_member(PyLong_FromLong(SI_TAB_NLINE_ROOF), "SI_TAB_NLINE_ROOF");
        
        add_member(PyLong_FromLong(SI_TAB_NLINE_BASEMENT),
                   "SI_TAB_NLINE_BASEMENT");
        
        add_member(PyLong_FromLong(SI_TAB_NLINE_LAST), "SI_TAB_NLINE_LAST");
        
        add_member(PyLong_FromLong(SI_TAB_COLUMN_FIRST),
                   "SI_TAB_COLUMN_FIRST");
        
        add_member(PyLong_FromLong(SI_TAB_COLUMN_MSG1),"SI_TAB_COLUMN_MSG1");
        add_member(PyLong_FromLong(SI_TAB_COLUMN_MSG2),"SI_TAB_COLUMN_MSG2");
        add_member(PyLong_FromLong(SI_TAB_COLUMN_MSA1),"SI_TAB_COLUMN_MSA1");
        add_member(PyLong_FromLong(SI_TAB_COLUMN_RTB1),"SI_TAB_COLUMN_RTB1");
        add_member(PyLong_FromLong(SI_TAB_COLUMN_RTB2),"SI_TAB_COLUMN_RTB2");
        
        add_member(PyLong_FromLong(SI_TAB_COLUMN_ROOF),
                   "SI_TAB_COLUMN_ROOF");
        
        add_member(PyLong_FromLong(SI_TAB_COLUMN_BASEMENT),
                   "SI_TAB_COLUMN_BASEMENT");
        
        add_member(PyLong_FromLong(SI_TAB_COLUMN_LAST),"SI_TAB_COLUMN_LAST");

        add_member(PyLong_FromLong(SI_MOB_NLINE_FIRST),"SI_MOB_NLINE_FIRST");
      
        add_member(PyLong_FromLong(SI_MOB_NLINE_MSA1),
                   "SI_MOB_NLINE_MSA1");
      
        add_member(PyLong_FromLong(SI_MOB_NLINE_ROOF), "SI_MOB_NLINE_ROOF");
        
        add_member(PyLong_FromLong(SI_MOB_NLINE_BASEMENT),
                   "SI_MOB_NLINE_BASEMENT");
        
        add_member(PyLong_FromLong(SI_MOB_NLINE_LAST), "SI_MOB_NLINE_LAST");
        
        add_member(PyLong_FromLong(SI_MOB_COLUMN_FIRST),
                   "SI_MOB_COLUMN_FIRST");
        
        add_member(PyLong_FromLong(SI_MOB_COLUMN_MSG1),"SI_MOB_COLUMN_MSG1");
        add_member(PyLong_FromLong(SI_MOB_COLUMN_MSG2),"SI_MOB_COLUMN_MSG2");
        add_member(PyLong_FromLong(SI_MOB_COLUMN_MSA1),"SI_MOB_COLUMN_MSA1");
        add_member(PyLong_FromLong(SI_MOB_COLUMN_ROOF),"SI_MOB_COLUMN_ROOF");
        
        add_member(PyLong_FromLong(SI_MOB_COLUMN_BASEMENT),
                   "SI_MOB_COLUMN_BASEMENT");
        
        add_member(PyLong_FromLong(SI_MOB_COLUMN_LAST),"SI_MOB_COLUMN_LAST");
          
        add_member(PyLong_FromLong(SI_DESK_MESSAGE_FIRST),
                   "SI_DESK_MESSAGE_FIRST");
        
        add_member(PyLong_FromLong(SI_DESK_MESSAGE_IMG),"SI_DESK_MESSAGE_IMG");
        
        add_member(PyLong_FromLong(SI_DESK_MESSAGE_MSG1),
                   "SI_DESK_MESSAGE_MSG1");
        
        add_member(PyLong_FromLong(SI_DESK_MESSAGE_MSG2),
                   "SI_DESK_MESSAGE_MSG2");
        
        add_member(PyLong_FromLong(SI_DESK_MESSAGE_RTB1),
                   "SI_DESK_MESSAGE_RTB1");
        
        add_member(PyLong_FromLong(SI_DESK_MESSAGE_RTB2),
                   "SI_DESK_MESSAGE_RTB2");
        
        add_member(PyLong_FromLong(SI_DESK_MESSAGE_ROOF),
                   "SI_DESK_MESSAGE_ROOF");
          
        add_member(PyLong_FromLong(SI_DESK_MESSAGE_BASEMENT),
                   "SI_DESK_MESSAGE_BASEMENT");
          
        add_member(PyLong_FromLong(SI_DESK_MESSAGE_LAST),
                   "SI_DESK_MESSAGE_LAST");
          
        add_member(PyLong_FromLong(SI_TAB_MESSAGE_FIRST),
                   "SI_TAB_MESSAGE_FIRST");
        
        add_member(PyLong_FromLong(SI_TAB_MESSAGE_IMG),"SI_TAB_MESSAGE_IMG");
        
        add_member(PyLong_FromLong(SI_TAB_MESSAGE_MSG1),
                   "SI_TAB_MESSAGE_MSG1");
        
        add_member(PyLong_FromLong(SI_TAB_MESSAGE_MSG2),
                   "SI_TAB_MESSAGE_MSG2");
        
        add_member(PyLong_FromLong(SI_TAB_MESSAGE_RTB1),
                   "SI_TAB_MESSAGE_RTB1");
        
        add_member(PyLong_FromLong(SI_TAB_MESSAGE_RTB2),
                   "SI_TAB_MESSAGE_RTB2");
        
        add_member(PyLong_FromLong(SI_TAB_MESSAGE_ROOF),
                   "SI_TAB_MESSAGE_ROOF");

        add_member(PyLong_FromLong(SI_TAB_MESSAGE_BASEMENT),
                   "SI_TAB_MESSAGE_BASEMENT");

        add_member(PyLong_FromLong(SI_TAB_MESSAGE_LAST),
                   "SI_TAB_MESSAGE_LAST");

        add_member(PyLong_FromLong(SI_MOB_MESSAGE_FIRST),
                   "SI_MOB_MESSAGE_FIRST");
        
        add_member(PyLong_FromLong(SI_MOB_MESSAGE_IMG),"SI_MOB_MESSAGE_IMG");
        
        add_member(PyLong_FromLong(SI_MOB_MESSAGE_MSG1),
                   "SI_MOB_MESSAGE_MSG1");
        
        add_member(PyLong_FromLong(SI_MOB_MESSAGE_MSG2),
                   "SI_MOB_MESSAGE_MSG2");
        
        add_member(PyLong_FromLong(SI_MOB_MESSAGE_ROOF),
                   "SI_MOB_MESSAGE_ROOF");
        
        add_member(PyLong_FromLong(SI_MOB_MESSAGE_BASEMENT),
                   "SI_MOB_MESSAGE_BASEMENT");
        
        add_member(PyLong_FromLong(SI_MOB_MESSAGE_LAST),
                   "SI_MOB_MESSAGE_LAST");
        
        add_member(PyLong_FromLong(CI_DIRECT), "CI_DIRECT");
        add_member(PyLong_FromLong(CI_FRAME), "CI_FRAME");
      }
        
      AdPyModule AdPyModule::instance;

      //
      // Selection class
      //
      Selection::Selection(PyTypeObject *type,
                           PyObject *args,
                           PyObject *kwds)
        throw(El::Exception)
          : El::Python::ObjectImpl(type ? type : &Type::instance)
      {
      }

      Selection::Selection(const ::NewsGate::Ad::Selection& src)
        throw(Exception, El::Exception)
          : El::Python::ObjectImpl(&Type::instance)
      {
        (::NewsGate::Ad::Selection&)*this = src;
      }

      //
      // SelectedCounter class
      //
      SelectedCounter::SelectedCounter(PyTypeObject *type,
                                       PyObject *args,
                                       PyObject *kwds)
        throw(El::Exception)
          : El::Python::ObjectImpl(type ? type : &Type::instance)
      {
      }

      SelectedCounter::SelectedCounter(
        const ::NewsGate::Ad::SelectedCounter& src)
        throw(Exception, El::Exception)
          : El::Python::ObjectImpl(&Type::instance)
      {
        (::NewsGate::Ad::SelectedCounter&)*this = src;
      }

      //
      // SelectionResult class
      //
      SelectionResult::SelectionResult(PyTypeObject *type,
                                       PyObject *args,
                                       PyObject *kwds)
        throw(El::Exception)
          : El::Python::ObjectImpl(type ? type : &Type::instance),
            ads(new El::Python::Sequence()),
            counters(new El::Python::Sequence())
      {
      }
      
      SelectionResult::SelectionResult(const Ad::SelectionResult& src)
        throw(El::Exception)
          : El::Python::ObjectImpl(&Type::instance),
            ads(new El::Python::Sequence()),
            counters(new El::Python::Sequence())
      {
        init(src);
      }

      void
      SelectionResult::init(const Ad::SelectionResult& src)
        throw(El::Exception)
      {
//        El::Python::Object_var a = new Selection();
        
        {
          ads->resize(src.ads.size());
          El::Python::Sequence::iterator j(ads->begin());        
        
          for(Ad::SelectionArray::const_iterator i(src.ads.begin()),
                e(src.ads.end()); i != e; ++i)
          {
            *j++ = new Selection(*i);
          }
        }
        
        {
          counters->resize(src.counters.size());
          El::Python::Sequence::iterator j(counters->begin());        
        
          for(Ad::SelectedCounterArray::const_iterator i(src.counters.begin()),
                e(src.counters.end()); i != e; ++i)
          {
            *j++ = new SelectedCounter(*i);
          }
        }
        
        src.ad_caps.to_string(ad_caps);
        src.counter_caps.to_string(counter_caps);
      }
    }
  }
}
