/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Frontends/SearchEngine/SearchEngine.cpp
 * @author Karen Arutyunov
 * $Id:$
 */

#include <Python.h>
#include <El/CORBA/Corba.hpp>

#include <iostream>
#include <string>
#include <sstream>
#include <utility>
#include <vector>

#include <El/Exception.hpp>
#include <El/Dictionary/Morphology.hpp>

#include <El/Python/Object.hpp>
#include <El/Python/Module.hpp>
#include <El/Python/Utility.hpp>
#include <El/Python/RefCount.hpp>
#include <El/Python/Lang.hpp>

#include <El/Dictionary/Python/Morphology.hpp>

#include <El/PSP/Config.hpp>

#include <Services/Dictionary/Commons/TransportImpl.hpp>
#include <Services/Dictionary/Commons/DictionaryServices.hpp>

#include "WordEngine.hpp"

namespace NewsGate
{
  WordEngine::Type WordEngine::Type::instance;

  static char ASPECT[] = "WordEngine";
  
  //
  // NewsGate::WordPyModule class
  //
  class WordPyModule : public El::Python::ModuleImpl<WordPyModule>
  {
  public:
    static WordPyModule instance;

    WordPyModule() throw(El::Exception);

    virtual void initialized() throw(El::Exception);

    PyObject* py_create_engine(PyObject* args) throw(El::Exception);
    PyObject* py_cleanup_engine(PyObject* args) throw(El::Exception);
  
    PY_MODULE_METHOD_VARARGS(
      py_create_engine,
      "create_engine",
      "Creates WordEngine object");

    PY_MODULE_METHOD_VARARGS(
      py_cleanup_engine,
      "cleanup_engine",
      "Cleanups WordEngine object");  

    El::Python::Object_var not_ready_ex;
  };

  //
  // NewsGate::WordEngine class
  //
  WordEngine::WordEngine(PyTypeObject *type,
                         PyObject *args,
                         PyObject *kwds)
    throw(Exception, El::Exception)
      : El::Python::ObjectImpl(type),
        orb_adapter_(0)
  {
    throw Exception("NewsGate::WordEngine::WordEngine: "
                    "unforseen way of object creation");
  }
        
  WordEngine::WordEngine(PyObject* args)
    throw(Exception, El::Exception)
      : El::Python::ObjectImpl(&Type::instance),
        orb_adapter_(0)
  {
    PyObject* config = 0;
    PyObject* logger = 0;
    
    if(!PyArg_ParseTuple(args,
                         "OO:newsgate.search.WordEngine.WordEngine",
                         &config,
                         &logger))
    {
      El::Python::handle_error("NewsGate::WordEngine::WordEngine");
    }

    if(!El::PSP::Config::Type::check_type(config))
    {
      El::Python::report_error(PyExc_TypeError,
                               "1st argument of el.psp.Config expected",
                               "NewsGate::WordEngine::WordEngine");
    }

    if(!El::Logging::Python::Logger::Type::check_type(logger))
    {
      El::Python::report_error(PyExc_TypeError,
                               "2nd argument of el.logging.Logger expected",
                               "NewsGate::WordEngine::WordEngine");
    }

    config_ = El::PSP::Config::Type::down_cast(config, true);
    logger_ = El::Logging::Python::Logger::Type::down_cast(logger, true);

    try
    {
/*      
      char* argv[] =
        {
          "--corba-thread-pool",
          "50"
        };
*/    
      /*
      char* argv[] =
        {
          "--corba-reactive"
        };
      */

      char* argv[] =
        {
        };

      orb_adapter_ = El::Corba::Adapter::orb_adapter(
        sizeof(argv) / sizeof(argv[0]), argv);
      
      orb_ = ::CORBA::ORB::_duplicate(orb_adapter_->orb());

      Dictionary::Transport::register_valuetype_factories(orb_.in());

      word_manager_ = WordManagerRef(
        config_->string("word_manager").c_str(), orb_.in());
    }
    catch(const CORBA::Exception& e)
    {
      if(orb_adapter_)
      {
        El::Corba::Adapter::orb_adapter_cleanup();
        orb_adapter_ = 0;
      }

      std::ostringstream ostr;
      ostr << "NewsGate::WordEngine::WordEngine: "
        "CORBA::Exception caught. Description:\n" << e;

      throw Exception(ostr.str());
    }

    logger_->info("NewsGate::WordEngine::WordEngine: "
                  "word engine constructed",
                  ASPECT);
  }

  WordEngine::~WordEngine() throw()
  {
    cleanup();

    logger_->info("NewsGate::WordEngine::~WordEngine: "
                  "stat logger destructed",
                  ASPECT);
  }

  void
  WordEngine::cleanup() throw()
  {
    try
    {
      bool cleanup_corba_adapter = false;

      {
        WriteGuard guard(lock_);        
        cleanup_corba_adapter = orb_adapter_ != 0;
        orb_adapter_ = 0;
      }

      logger_->info("NewsGate::WordEngine::cleanup: done", ASPECT);      
      
      if(cleanup_corba_adapter)
      {
        El::Corba::Adapter::orb_adapter_cleanup();
      }
    }
    catch(...)
    {
    }
  }  
  
  PyObject*
  WordEngine::py_get_lemmas(PyObject* args) throw(El::Exception)
  {
    PyObject* wrd = 0;
    PyObject* lng = 0;
    
    unsigned long guess_strategy =
      ::El::Dictionary::Morphology::Lemma::GS_NONE;
    
    if(!PyArg_ParseTuple(args,
                         "O|Ok:newsgate.word.WordEngine.get_lemmas",
                         &wrd,
                         &lng,
                         &guess_strategy))
    {
      El::Python::handle_error("NewsGate::WordEngine::py_get_lemmas");
    }

    if(guess_strategy >= ::El::Dictionary::Morphology::Lemma::GS_COUNT)
    {
      El::Python::report_error(
        PyExc_TypeError,
        "Unexpected value for 3rd argument",
        "NewsGate::WordEngine::py_get_lemmas");      
    }

    if(!PySequence_Check(wrd))
    {
      El::Python::report_error(
        PyExc_TypeError,
        "1st argument of sequence type expected",
        "NewsGate::WordEngine::py_get_lemmas");
    }

    int words_count = PySequence_Size(wrd);

    if(words_count < 0)
    {
      El::Python::handle_error("NewsGate::WordEngine.py_get_lemmas");
    }  

    if(lng && !El::Python::Lang::Type::check_type(lng))
    {
      El::Python::report_error(
        PyExc_TypeError,
        "2nd argument of el.Lang type expected",
        "NewsGate::WordEngine::py_get_lemmas");
    }

    try
    {
      Dictionary::Transport::GetLemmasParamsImpl::Var
        params_transport =
        Dictionary::Transport::GetLemmasParamsImpl::Init::create(
          new Dictionary::Transport::GetLemmasParamsStruct());

      Dictionary::Transport::GetLemmasParamsStruct& params =
        params_transport->entity();

      if(lng)
      {
        params.lang = *El::Python::Lang::Type::down_cast(lng);
      }

      params.guess_strategy =
        (::El::Dictionary::Morphology::Lemma::GuessStrategy)guess_strategy;

      Dictionary::Transport::WordsArray& words = params.words;
      words.resize(words_count);

      for(int i = 0; i < words_count; i++)
      {
        El::Python::Object_var item = PySequence_GetItem(wrd, i);

        item =
          El::Python::string_from_object(
            item.in(),
            "NewsGate::WordEngine::py_get_lemmas");

        size_t slen = 0;
        words[i] = El::Python::string_from_string(item.in(), slen);
      }
      
      El::Python::AllowOtherThreads guard;
      Dictionary::Transport::LemmaPack_var result;

      Dictionary::WordManager_var word_manager = word_manager_.object();
      word_manager->get_lemmas(params_transport.in(), result.out());

      Dictionary::Transport::LemmaPackImpl::Type* lemmas =
        dynamic_cast<Dictionary::Transport::LemmaPackImpl::Type*>(result.in());
        
      if(lemmas == 0)
      {
        throw Exception(
          "NewsGate::WordEngine::py_get_lemmas: dynamic_cast<"
          "Dictionary::Transport::LemmaPackImpl::Type*> failed");
      }

      const El::Dictionary::Morphology::LemmaInfoArrayArray& laa =
        lemmas->entities();

      El::Python::Sequence_var res = new El::Python::Sequence();
      res->reserve(laa.size());

      for(El::Dictionary::Morphology::LemmaInfoArrayArray::const_iterator
            it(laa.begin()), ie(laa.end()); it != ie; ++it)
      {
        El::Python::Sequence_var word_lemmas = new El::Python::Sequence();
        
        word_lemmas->
          from_container<El::Dictionary::Morphology::Python::Lemma>(*it);
          
        res->push_back(word_lemmas);
      }

      return res.retn();
    }
    catch(const Dictionary::NotReady& e)
    {
      std::ostringstream ostr;
      ostr << "NewsGate::WordEngine::py_get_lemmas: word manager is not ready."
        " Reason: " << e.reason.in();
        
      logger_->trace(ostr.str().c_str(), ASPECT, El::Logging::MIDDLE);

      El::Python::report_error(
        WordPyModule::instance.not_ready_ex.in(),
        e.reason.in());
    }
    catch(const Dictionary::ImplementationException& e)
    {
      std::ostringstream ostr;
      ostr << "NewsGate::WordEngine::py_get_lemmas: "
        "NewsGate::Dictionary::ImplementationException caught. Description:\n"
           << e.description.in();
      
      throw Exception(ostr.str());
    }
    catch(const CORBA::Exception& e)
    {
      std::ostringstream ostr;
      ostr << "NewsGate::WordEngine::py_get_lemmas: "
        "CORBA::Exception caught. Description:\n" << e;
      
      throw Exception(ostr.str());
    }
    
    return 0;
  }
  
  //
  // NewsGate::WordPyModule class
  //

  WordPyModule WordPyModule::instance;
    
  WordPyModule::WordPyModule() throw(El::Exception)
      : El::Python::ModuleImpl<WordPyModule>(
        "newsgate.word",
        "Module containing WordEngine factory method.",
        true)
  {
  }

  void
  WordPyModule::initialized() throw(El::Exception)
  {
    not_ready_ex = create_exception("NotReady");
  }
  
  PyObject*
  WordPyModule::py_create_engine(PyObject* args) throw(El::Exception)
  {
    return new WordEngine(args);
  }

  PyObject*
  WordPyModule::py_cleanup_engine(PyObject* args)
    throw(El::Exception)
  {
    PyObject* se = 0;
    
    if(!PyArg_ParseTuple(args,
                         "O:newsgate.word.cleanup_engine",
                         &se))
    {
      El::Python::handle_error(
        "NewsGate::WordPyModule::py_cleanup_engine");
    }

    if(!WordEngine::Type::check_type(se))
    {
      El::Python::report_error(
        PyExc_TypeError,
        "1st argument of newsgate.word.WordEngine",
        "NewsGate::WordPyModule::py_cleanup_engine");
    }

    WordEngine* engine = WordEngine::Type::down_cast(se);    
    engine->cleanup();
    
    return El::Python::add_ref(Py_None);
  }  
}
