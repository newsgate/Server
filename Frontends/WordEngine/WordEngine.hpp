/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Frontends/WordEngine/WordEngine.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_FRONTENDS_WORDENGINE_WORDENGINE_HPP_
#define _NEWSGATE_SERVER_FRONTENDS_WORDENGINE_WORDENGINE_HPP_

#include <string>

#include <El/Exception.hpp>

#include <El/Python/Exception.hpp>
#include <El/Python/Object.hpp>
#include <El/Python/RefCount.hpp>
#include <El/Python/Logger.hpp>
#include <El/Python/Lang.hpp>
#include <El/Python/Country.hpp>
#include <El/Python/Sequence.hpp>
#include <El/Python/Map.hpp>

#include <El/PSP/Config.hpp>

#include <Services/Dictionary/Commons/DictionaryServices.hpp>

namespace NewsGate
{
  class WordEngine : public El::Python::ObjectImpl
  {
  public:
    EL_EXCEPTION(Exception, El::ExceptionBase);
    
  public:
    WordEngine(PyTypeObject *type, PyObject *args, PyObject *kwds)
      throw(Exception, El::Exception);
    
    WordEngine(PyObject* args) throw(Exception, El::Exception);
      
    virtual ~WordEngine() throw();

    void cleanup() throw();
    
    PyObject* py_get_lemmas(PyObject* args) throw(El::Exception);      

    class Type : public El::Python::ObjectTypeImpl<WordEngine,
                                                   WordEngine::Type>
    {
    public:
      Type() throw(El::Python::Exception, El::Exception);
      static Type instance;
      
      PY_TYPE_METHOD_VARARGS(py_get_lemmas,
                             "get_lemmas",
                             "Get lemmas for words specified");
    };

  private:
    El::PSP::Config_var config_;
    El::Logging::Python::Logger_var logger_;
    El::Corba::OrbAdapter* orb_adapter_;
    CORBA::ORB_var orb_;

    typedef El::Corba::SmartRef<Dictionary::WordManager> WordManagerRef;
    WordManagerRef word_manager_;

    typedef ACE_RW_Thread_Mutex Mutex;
    typedef ACE_Read_Guard<Mutex> ReadGuard;
    typedef ACE_Write_Guard<Mutex> WriteGuard;

    Mutex lock_;    
  };  
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  //
  // NewsGate::WordEngine::Type class
  //
  inline
  WordEngine::Type::Type() throw(El::Python::Exception, El::Exception)
      : El::Python::ObjectTypeImpl<WordEngine, WordEngine::Type>(
        "newsgate.word.WordEngine",
        "Object providing word information retreival functionality")
  {
    tp_new = 0;
  }

}
  
#endif // _NEWSGATE_SERVER_FRONTENDS_WORDENGINE_WORDENGINE_HPP_
