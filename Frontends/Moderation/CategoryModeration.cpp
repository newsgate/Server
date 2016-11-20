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

#include <string>
#include <sstream>

#include <El/Exception.hpp>

#include <El/Python/Object.hpp>
#include <El/Python/Module.hpp>
#include <El/Python/Utility.hpp>
#include <El/Python/RefCount.hpp>

#include <Commons/Search/SearchExpression.hpp>
#include <Commons/Message/Message.hpp>

#include <Services/Moderator/Commons/CategoryManager.hpp>

#include "CategoryModeration.hpp"

namespace
{
//  static char ASPECT[] = "CategoryModeration";
  static const unsigned long MAX_EXPR_LEN = 0x1000000 - 1;
  static const unsigned long MAX_WORDS_LEN = 0x1000000 - 1;
  static const unsigned long MAX_WORDS_NAME_LEN = 128 - 1;
}

namespace NewsGate
{
  namespace CategoryModeration
  {    
    Manager::Type Manager::Type::instance;
    PhraseOccurance::Type PhraseOccurance::Type::instance;
    RelevantPhrases::Type RelevantPhrases::Type::instance;
    WordFinding::Type WordFinding::Type::instance;
    WordListFinding::Type WordListFinding::Type::instance;
    CategoryFinding::Type CategoryFinding::Type::instance;
    CategoryDescriptor::Type CategoryDescriptor::Type::instance;
    LocaleDescriptor::Type LocaleDescriptor::Type::instance;
    ExpressionDescriptor::Type ExpressionDescriptor::Type::instance;
    WordListDescriptor::Type WordListDescriptor::Type::instance;
    CategoryPathElem::Type CategoryPathElem::Type::instance;
    CategoryPath::Type CategoryPath::Type::instance;
    ExpressionError::Type ExpressionError::Type::instance;
    WordListError::Type WordListError::Type::instance;
    WordListNameError::Type WordListNameError::Type::instance;
    NoPath::Type NoPath::Type::instance;
    Cycle::Type Cycle::Type::instance;
    WordListNotFound::Type WordListNotFound::Type::instance;
    ExpressionParseError::Type ExpressionParseError::Type::instance;
    ForbiddenOperation::Type ForbiddenOperation::Type::instance;
    VersionMismatch::Type VersionMismatch::Type::instance;
    
    //
    // NewsGate::CategoryModeration::CategoryModerationPyModule class
    //
    
    class CategoryModerationPyModule :
      public El::Python::ModuleImpl<CategoryModerationPyModule>
    {
    public:
      static CategoryModerationPyModule instance;

      CategoryModerationPyModule() throw(El::Exception);
      
      virtual void initialized() throw(El::Exception);

      El::Python::Object_var expression_error_ex;
      El::Python::Object_var word_list_error_ex;
      El::Python::Object_var word_list_name_error_ex;
      El::Python::Object_var category_not_found_ex;
      El::Python::Object_var no_path_ex;
      El::Python::Object_var cycle_ex;
      El::Python::Object_var word_list_not_found_ex;
      El::Python::Object_var expression_parse_error_ex;
      El::Python::Object_var forbidden_operation_ex;
      El::Python::Object_var version_mismatch_ex;
    };
  
    CategoryModerationPyModule CategoryModerationPyModule::instance;
  
    CategoryModerationPyModule::CategoryModerationPyModule()
      throw(El::Exception)
        : El::Python::ModuleImpl<CategoryModerationPyModule>(
        "newsgate.moderation.category",
        "Module containing Category Moderation types.",
        true)
    {
    }

    void
    CategoryModerationPyModule::initialized() throw(El::Exception)
    {
      add_member(PyLong_FromLong(Moderation::Category::CS_ENABLED),
                 "CS_ENABLED");

      add_member(PyLong_FromLong(Moderation::Category::CS_DISABLED),
                 "CS_DISABLED");

      add_member(PyLong_FromLong(Moderation::Category::CR_YES), "CR_YES");
      add_member(PyLong_FromLong(Moderation::Category::CR_NO), "CR_NO");

      expression_error_ex = create_exception("ExpressionError");
      word_list_error_ex = create_exception("WordListError");
      word_list_name_error_ex = create_exception("WordListNameError");
      category_not_found_ex = create_exception("CategoryNotFound");
      no_path_ex = create_exception("NoPath");
      cycle_ex = create_exception("Cycle");
      word_list_not_found_ex = create_exception("WordListNotFound");
      expression_parse_error_ex = create_exception("ExpressionParseError");
      forbidden_operation_ex = create_exception("ForbiddenOperation");
      version_mismatch_ex = create_exception("VersionMismatch");
    }
    
    //
    // NewsGate::CategoryModeration::Manager class
    //    
    Manager::Manager(PyTypeObject *type, PyObject *args, PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type)
    {
      throw Exception(
        "NewsGate::CategoryModeration::Manager::Manager: unforseen way "
        "of object creation");
    }
    
    Manager::Manager(const ManagerRef& manager)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          manager_(manager)
    {
    }
/*
    PyObject*
    Manager::py_relevant_phrases(PyObject* args) throw(El::Exception)
    {
      const char* query = 0;
      const char* lang = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "ss:newsgate.moderation.category.Manager.relevant_phrases",
           &query,
           &lang))
      {
        El::Python::handle_error(
          "NewsGate::CategoryModeration::Manager::py_relevant_phrases");
      }

      ::NewsGate::Moderation::Category::RelevantPhrases_var res;
      
      try
      {
        El::Python::AllowOtherThreads guard;

        ::NewsGate::Moderation::Category::Manager_var manager =
            manager_.object();
        
        res = manager->relevant_phrases(query, lang);
      }
      catch(const Moderation::Category::ExpressionParseError& e)
      {
        ExpressionParseError_var error = new ExpressionParseError(e);

        PyErr_SetObject(
          CategoryModerationPyModule::instance.expression_parse_error_ex.in(),
          error.in());
        
        return 0;
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::CategoryModeration::Manager::py_relevant_phrases: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::CategoryModeration::Manager::py_relevant_phrases: "
          "CORBA::Exception caught. Description:\n" << e;
        
        throw Exception(ostr.str());
      }

      return new RelevantPhrases(*res);
    }    
    */
    PyObject*
    Manager::py_category_relevant_phrases(PyObject* args) throw(El::Exception)
    {
      unsigned long long id = 0;
      const char* lang = 0;
      PyObject* seq = 0;
      
      if(!PyArg_ParseTuple(
           args,
           "KsO:newsgate.moderation.category.Manager.category_relevant_phrases",
           &id,
           &lang,
           &seq))
      {
        El::Python::handle_error(
          "NewsGate::CategoryModeration::Manager::"
          "py_category_relevant_phrases");
      }

      const El::Python::Sequence& id_seq =
        *El::Python::Sequence::Type::down_cast(seq);
      
      ::NewsGate::Moderation::Category::PhraseIdSeq skip_phrases;

      skip_phrases.length(id_seq.size());
      size_t j = 0;
      
      for(El::Python::Sequence::const_iterator i(id_seq.begin()),
            e(id_seq.end()); i != e; ++i, ++j)
      {
        skip_phrases[j] =
          El::Python::ulonglong_from_number(
            i->in(),
            "NewsGate::CategoryModeration::Manager::"
            "py_category_relevant_phrases");
      }
      
      ::NewsGate::Moderation::Category::RelevantPhrases_var res;
      
      try
      {
        El::Python::AllowOtherThreads guard;

        ::NewsGate::Moderation::Category::Manager_var manager =
            manager_.object();
        
        res = manager->category_relevant_phrases(id, lang, skip_phrases);
      }
      catch(const Moderation::Category::CategoryNotFound& e)
      {
        PyErr_SetObject(
          CategoryModerationPyModule::instance.category_not_found_ex.in(),
          0);
        
        return 0;        
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::CategoryModeration::Manager::"
          "py_category_relevant_phrases: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::CategoryModeration::Manager::"
          "py_category_relevant_phrases: "
          "CORBA::Exception caught. Description:\n" << e;
        
        throw Exception(ostr.str());
      }

      return new RelevantPhrases(*res);
    }    
    
    PyObject*
    Manager::py_create_category(PyObject* args) throw(El::Exception)
    {
      PyObject* pcat = 0;
      unsigned long long moderator_id = 0;
      const char* moderator_name = 0;
      const char* ip = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "OKss:newsgate.moderation.category.Manager.create_category",
           &pcat,
           &moderator_id,
           &moderator_name,
           &ip))
      {
        El::Python::handle_error(
          "NewsGate::CategoryModeration::Manager::py_create_category");
      }

      if(!CategoryDescriptor::Type::check_type(pcat))
      {
        El::Python::report_error(
          PyExc_TypeError,
          "Argument of newsgate.moderation.category.CategoryDescriptor "
            "expected",
          "NewsGate::CategoryModeration::Manager::py_create_category");
      }

      ::NewsGate::Moderation::Category::CategoryDescriptor_var res;
      
      try
      {
        ::NewsGate::Moderation::Category::CategoryDescriptor cat;
        CategoryDescriptor::Type::down_cast(pcat)->copy(cat);

        El::Python::AllowOtherThreads guard;

        ::NewsGate::Moderation::Category::Manager_var manager =
            manager_.object();
        
        res = manager->create_category(cat, moderator_id, moderator_name, ip);
      }
      catch(const CategoryDescriptor::ExpressionError& e)
      {
        ExpressionError_var error = new ExpressionError();
        (CategoryDescriptor::ExpressionError&)(*error) = e;

        PyErr_SetObject(
          CategoryModerationPyModule::instance.expression_error_ex.in(),
          error.in());
        
        return 0;
      }
      catch(const Moderation::Category::NoPath& e)
      {
        NoPath_var error = new NoPath(e);

        PyErr_SetObject(
          CategoryModerationPyModule::instance.no_path_ex.in(),
          error.in());
        
        return 0;
      }
      catch(const Moderation::Category::Cycle& e)
      {
        Cycle_var error = new Cycle(e);

        PyErr_SetObject(
          CategoryModerationPyModule::instance.cycle_ex.in(),
          error.in());
        
        return 0;
      }
      catch(const Moderation::Category::ForbiddenOperation& e)
      {
        ForbiddenOperation_var error = new ForbiddenOperation(e);

        PyErr_SetObject(
          CategoryModerationPyModule::instance.forbidden_operation_ex.in(),
          error.in());
        
        return 0;
      }
      catch(const Moderation::Category::WordListNotFound& e)
      {
        WordListNotFound_var error = new WordListNotFound(e);

        PyErr_SetObject(
          CategoryModerationPyModule::instance.word_list_not_found_ex.in(),
          error.in());
        
        return 0;
      }
      catch(const Moderation::Category::ExpressionParseError& e)
      {
        ExpressionParseError_var error = new ExpressionParseError(e);

        PyErr_SetObject(
          CategoryModerationPyModule::instance.expression_parse_error_ex.in(),
          error.in());
        
        return 0;
      }
      catch(const CategoryDescriptor::WordListNameError& e)
      {
        WordListNameError_var error = new WordListNameError();
        (CategoryDescriptor::WordListNameError&)(*error) = e;

        PyErr_SetObject(
          CategoryModerationPyModule::instance.word_list_name_error_ex.in(),
          error.in());
        
        return 0;
      }
      catch(const CategoryDescriptor::WordListError& e)
      {
        WordListError_var error = new WordListError();
        (CategoryDescriptor::WordListError&)(*error) = e;

        PyErr_SetObject(
          CategoryModerationPyModule::instance.word_list_error_ex.in(),
          error.in());
        
        return 0;
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::CategoryModeration::Manager::py_create_category: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::CategoryModeration::Manager::py_create_category: "
          "CORBA::Exception caught. Description:\n" << e;
        
        throw Exception(ostr.str());
      }

      return new CategoryDescriptor(*res);
    }
    
    PyObject*
    Manager::py_update_word_lists(PyObject* args) throw(El::Exception)
    {
      unsigned long long id = 0;
      PyObject* seq = 0;
      unsigned char is_category_manager = true;
      unsigned long long moderator_id = 0;
      const char* moderator_name = 0;
      const char* ip = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "KObKss:newsgate.moderation.category.Manager.update_word_lists",
           &id,
           &seq,
           &is_category_manager,
           &moderator_id,
           &moderator_name,
           &ip))
      {
        El::Python::handle_error(
          "NewsGate::CategoryModeration::Manager::py_update_word_lists");
      }

      const El::Python::Sequence& word_lists =
        *El::Python::Sequence::Type::down_cast(seq);

      ::NewsGate::Moderation::Category::WordListDescriptorSeq wl;
      wl.length(word_lists.size());
      size_t j = 0;
      
      for(El::Python::Sequence::const_iterator i(word_lists.begin()),
            e(word_lists.end()); i != e; ++i)
      {
        const WordListDescriptor& wd =
          *WordListDescriptor::Type::down_cast(*i);

        ::NewsGate::Moderation::Category::WordListDescriptor& wdd = wl[j++];

        wd.copy(wdd);
/*        
        wdd.name = wd.name.c_str();
        wdd.words = wd.words.c_str();
        wdd.version = wd.version;
        wdd.updated = wd.updated;
*/
      }

      ::NewsGate::Moderation::Category::CategoryDescriptor_var res;
      
      try
      {
        El::Python::AllowOtherThreads guard;

        ::NewsGate::Moderation::Category::Manager_var manager =
            manager_.object();
        
        res = manager->update_word_lists(id,
                                         wl,
                                         is_category_manager,
                                         moderator_id,
                                         moderator_name,
                                         ip);
      }
      catch(const Moderation::Category::ForbiddenOperation& e)
      {
        ForbiddenOperation_var error = new ForbiddenOperation(e);

        PyErr_SetObject(
          CategoryModerationPyModule::instance.forbidden_operation_ex.in(),
          error.in());
        
        return 0;
      }
      catch(const Moderation::Category::WordListNotFound& e)
      {
        WordListNotFound_var error = new WordListNotFound(e);

        PyErr_SetObject(
          CategoryModerationPyModule::instance.word_list_not_found_ex.in(),
          error.in());
        
        return 0;
      }
      catch(const Moderation::Category::VersionMismatch& e)
      {
        VersionMismatch_var error = new VersionMismatch(e);

        PyErr_SetObject(
          CategoryModerationPyModule::instance.version_mismatch_ex.in(),
          error.in());
        
        return 0;
      }
      catch(const Moderation::Category::ExpressionParseError& e)
      {
        ExpressionParseError_var error = new ExpressionParseError(e);

        PyErr_SetObject(
          CategoryModerationPyModule::instance.expression_parse_error_ex.in(),
          error.in());
        
        return 0;
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::CategoryModeration::Manager::py_update_word_lists: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::CategoryModeration::Manager::py_update_word_lists: "
          "CORBA::Exception caught. Description:\n" << e;
        
        throw Exception(ostr.str());
      }

      return new CategoryDescriptor(*res);
    }

    PyObject*
    Manager::py_update_category(PyObject* args) throw(El::Exception)
    {
      PyObject* pcat = 0;
      unsigned long long moderator_id = 0;
      const char* moderator_name = 0;
      const char* ip = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "OKss:newsgate.moderation.category.Manager.update_category",
           &pcat,
           &moderator_id,
           &moderator_name,
           &ip))
      {
        El::Python::handle_error(
          "NewsGate::CategoryModeration::Manager::py_update_category");
      }

      if(!CategoryDescriptor::Type::check_type(pcat))
      {
        El::Python::report_error(
          PyExc_TypeError,
          "Argument of newsgate.moderation.category.CategoryDescriptor "
            "expected",
          "NewsGate::CategoryModeration::Manager::py_update_category");
      }

      ::NewsGate::Moderation::Category::CategoryDescriptor_var res;
      
      try
      {
        ::NewsGate::Moderation::Category::CategoryDescriptor cat;
        CategoryDescriptor::Type::down_cast(pcat)->copy(cat);

        El::Python::AllowOtherThreads guard;

        ::NewsGate::Moderation::Category::Manager_var manager =
            manager_.object();
        
        res = manager->update_category(cat, moderator_id, moderator_name, ip);
      }
      catch(const CategoryDescriptor::ExpressionError& e)
      {
        ExpressionError_var error = new ExpressionError();
        (CategoryDescriptor::ExpressionError&)(*error) = e;

        PyErr_SetObject(
          CategoryModerationPyModule::instance.expression_error_ex.in(),
          error.in());
        
        return 0;
      }
      catch(const Moderation::Category::NoPath& e)
      {
        NoPath_var error = new NoPath(e);

        PyErr_SetObject(
          CategoryModerationPyModule::instance.no_path_ex.in(),
          error.in());
        
        return 0;
      }
      catch(const Moderation::Category::Cycle& e)
      {
        Cycle_var error = new Cycle(e);

        PyErr_SetObject(
          CategoryModerationPyModule::instance.cycle_ex.in(),
          error.in());
        
        return 0;
      }
      catch(const Moderation::Category::CategoryNotFound& e)
      {
        PyErr_SetObject(
          CategoryModerationPyModule::instance.category_not_found_ex.in(),
          0);
        
        return 0;        
      }
      catch(const Moderation::Category::ForbiddenOperation& e)
      {
        ForbiddenOperation_var error = new ForbiddenOperation(e);

        PyErr_SetObject(
          CategoryModerationPyModule::instance.forbidden_operation_ex.in(),
          error.in());
        
        return 0;
      }
      catch(const Moderation::Category::WordListNotFound& e)
      {
        WordListNotFound_var error = new WordListNotFound(e);

        PyErr_SetObject(
          CategoryModerationPyModule::instance.word_list_not_found_ex.in(),
          error.in());
        
        return 0;
      }
      catch(const Moderation::Category::ExpressionParseError& e)
      {
        ExpressionParseError_var error = new ExpressionParseError(e);

        PyErr_SetObject(
          CategoryModerationPyModule::instance.expression_parse_error_ex.in(),
          error.in());
        
        return 0;
      }
      catch(const Moderation::Category::VersionMismatch& e)
      {
        VersionMismatch_var error = new VersionMismatch(e);

        PyErr_SetObject(
          CategoryModerationPyModule::instance.version_mismatch_ex.in(),
          error.in());
        
        return 0;
      }
      catch(const CategoryDescriptor::WordListNameError& e)
      {
        WordListNameError_var error = new WordListNameError();
        (CategoryDescriptor::WordListNameError&)(*error) = e;

        PyErr_SetObject(
          CategoryModerationPyModule::instance.word_list_name_error_ex.in(),
          error.in());
        
        return 0;
      }
      catch(const CategoryDescriptor::WordListError& e)
      {
        WordListError_var error = new WordListError();
        (CategoryDescriptor::WordListError&)(*error) = e;

        PyErr_SetObject(
          CategoryModerationPyModule::instance.word_list_error_ex.in(),
          error.in());
        
        return 0;
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::CategoryModeration::Manager::py_update_category: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::CategoryModeration::Manager::py_update_category: "
          "CORBA::Exception caught. Description:\n" << e;
        
        throw Exception(ostr.str());
      }

      return new CategoryDescriptor(*res);
    }
    
    PyObject*
    Manager::py_get_category(PyObject* args) throw(El::Exception)
    {
      unsigned long long id = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "K:newsgate.moderation.category.Manager.get_category",
           &id))
      {
        El::Python::handle_error(
          "NewsGate::CategoryModeration::Manager::py_get_category");
      }

      ::NewsGate::Moderation::Category::CategoryDescriptor_var res;
      
      try
      {
        El::Python::AllowOtherThreads guard;

        ::NewsGate::Moderation::Category::Manager_var manager =
            manager_.object();
        
        res = manager->get_category(id);
      }
      catch(const Moderation::Category::CategoryNotFound& e)
      {
        PyErr_SetObject(
          CategoryModerationPyModule::instance.category_not_found_ex.in(),
          0);
        
        return 0;        
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::CategoryModeration::Manager::py_get_category: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::CategoryModeration::Manager::py_get_category: "
          "CORBA::Exception caught. Description:\n" << e;
        
        throw Exception(ostr.str());
      }

      return new CategoryDescriptor(*res);
    }

    PyObject*
    Manager::py_get_category_version(PyObject* args) throw(El::Exception)
    {
      unsigned long long id = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "K:newsgate.moderation.category.Manager.get_category_version",
           &id))
      {
        El::Python::handle_error(
          "NewsGate::CategoryModeration::Manager::py_get_category_version");
      }

      ::NewsGate::Moderation::Category::CategoryDescriptor_var res;
      
      try
      {
        El::Python::AllowOtherThreads guard;

        ::NewsGate::Moderation::Category::Manager_var manager =
            manager_.object();
        
        res = manager->get_category_version(id);
      }
      catch(const Moderation::Category::CategoryNotFound& e)
      {
        PyErr_SetObject(
          CategoryModerationPyModule::instance.category_not_found_ex.in(),
          0);
        
        return 0;        
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::CategoryModeration::Manager::"
          "py_get_category_version: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::CategoryModeration::Manager::"
          "py_get_category_version: "
          "CORBA::Exception caught. Description:\n" << e;
        
        throw Exception(ostr.str());
      }

      return new CategoryDescriptor(*res);
    }

    PyObject*
    Manager::py_find_category(PyObject* args) throw(El::Exception)
    {
      const char* path = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "s:newsgate.moderation.category.Manager.find_category",
           &path))
      {
        El::Python::handle_error(
          "NewsGate::CategoryModeration::Manager::py_find_category");
      }

      ::NewsGate::Moderation::Category::CategoryDescriptor_var res;
      
      try
      {
        El::Python::AllowOtherThreads guard;

        ::NewsGate::Moderation::Category::Manager_var manager =
            manager_.object();
        
        res = manager->find_category(path);
      }
      catch(const Moderation::Category::CategoryNotFound& e)
      {
        PyErr_SetObject(
          CategoryModerationPyModule::instance.category_not_found_ex.in(),
          0);
        
        return 0;        
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::CategoryModeration::Manager::py_find_category: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::CategoryModeration::Manager::py_find_category: "
          "CORBA::Exception caught. Description:\n" << e;
        
        throw Exception(ostr.str());
      }

      return new CategoryDescriptor(*res);
    }

    PyObject*
    Manager::py_find_text(PyObject* args) throw(El::Exception)
    {
      const char* text = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "s:newsgate.moderation.category.Manager.find_text",
           &text))
      {
        El::Python::handle_error(
          "NewsGate::CategoryModeration::Manager::py_find_category");
      }

      ::NewsGate::Moderation::Category::CategoryFindingSeq_var res;
      
      try
      {
        El::Python::AllowOtherThreads guard;

        ::NewsGate::Moderation::Category::Manager_var manager =
            manager_.object();
        
        res = manager->find_text(text);
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::CategoryModeration::Manager::py_find_text: "
          "ImplementationException exception caught. Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::CategoryModeration::Manager::py_find_text: "
          "CORBA::Exception caught. Description:\n" << e;
        
        throw Exception(ostr.str());
      }

      El::Python::Sequence_var seq = new El::Python::Sequence();
      seq->resize(res->length());

      for(size_t i = 0; i < res->length(); ++i)
      {
        (*seq)[i] = new CategoryFinding((*res)[i]);
      }

      return seq.retn();
    }

    PyObject*
    Manager::py_add_category_message(PyObject* args) throw(El::Exception)
    {
      unsigned long long cat_id = 0;
      const char* msg_id = 0;
      const char* relation = 0;
      unsigned long long moderator_id = 0;
      const char* moderator_name = 0;
      const char* ip = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "KssKss:newsgate.moderation.category.Manager.add_category_message",
           &cat_id,
           &msg_id,
           &relation,
           &moderator_id,
           &moderator_name,
           &ip))
      {
        El::Python::handle_error(
          "NewsGate::CategoryModeration::Manager::py_add_category_message");
      }

      ::NewsGate::Message::Id message_id(msg_id, true);

      try
      {
        El::Python::AllowOtherThreads guard;

        ::NewsGate::Moderation::Category::Manager_var manager =
            manager_.object();
        
        manager->add_category_message(cat_id,
                                      message_id.data,
                                      relation[0],
                                      moderator_id,
                                      moderator_name,
                                      ip);
      }
      catch(const Moderation::Category::CategoryNotFound& e)
      {
        PyErr_SetObject(
          CategoryModerationPyModule::instance.category_not_found_ex.in(),
          0);
        
        return 0;        
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::CategoryModeration::Manager::"
          "py_add_category_message: ImplementationException exception caught. "
          "Description:\n"
             << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::CategoryModeration::Manager::"
          "py_add_category_message: CORBA::Exception caught. "
          "Description:\n" << e;

        throw Exception(ostr.str());
      }      
          
      return El::Python::add_ref(Py_None);
    }
    
    PyObject*
    Manager::py_delete_categories(PyObject* args) throw(El::Exception)
    {
      PyObject* category_ids = 0;
      unsigned long long moderator_id = 0;
      const char* moderator_name = 0;
      const char* ip = 0;
    
      if(!PyArg_ParseTuple(
           args,
           "OKss:newsgate.moderation.category.Manager.delete_categories",
           &category_ids,
           &moderator_id,
           &moderator_name,
           &ip))
      {
        El::Python::handle_error(
          "NewsGate::CategoryModeration::Manager::py_delete_categories");
      }

      if(!PySequence_Check(category_ids))
      {
        El::Python::report_error(PyExc_TypeError,
                                 "argument expected to be of sequence type",
                                 "NewsGate::CategoryModeration::Manager::"
                                 "py_delete_categories");
      }

      int len = PySequence_Size(category_ids);

      if(len < 0)
      {
        El::Python::handle_error(
          "NewsGate::CategoryModeration::Manager::py_delete_categories");
      }
      
      try
      {
        Moderation::Category::CategoryIdSeq_var seq =
          new Moderation::Category::CategoryIdSeq();
        
        seq->length(len);
      
        for(CORBA::ULong i = 0; i < (CORBA::ULong)len; i++)
        {
          El::Python::Object_var item = PySequence_GetItem(category_ids, i);
        
          seq[i] = El::Python::ulonglong_from_number(
            item.in(),
            "NewsGate::CategoryModeration::Manager::py_delete_categories");
        }

        {
          El::Python::AllowOtherThreads guard;

        ::NewsGate::Moderation::Category::Manager_var manager =
            manager_.object();
          
          manager->delete_categories(seq.in(),
                                     moderator_id,
                                     moderator_name,
                                     ip);
        }
      }
      catch(const Moderation::Category::ForbiddenOperation& e)
      {
        ForbiddenOperation_var error = new ForbiddenOperation(e);

        PyErr_SetObject(
          CategoryModerationPyModule::instance.forbidden_operation_ex.in(),
          error.in());
        
        return 0;
      }
      catch(const Moderation::ImplementationException& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::CategoryModeration::Manager::"
          "py_delete_categories: ImplementationException exception caught. "
          "Description:\n" << e.description.in();
        
        throw Exception(ostr.str());
      }
      catch(const CORBA::Exception& e)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::CategoryModeration::Manager::"
          "py_delete_categories: CORBA::Exception caught. "
          "Description:\n" << e;

        throw Exception(ostr.str());
      }

      return El::Python::add_ref(Py_None);
    }

    //
    // NewsGate::CategoryModeration::PhraseOccurance class
    //
    
    PhraseOccurance::PhraseOccurance(PyTypeObject *type,
                                     PyObject *args,
                                     PyObject *kwds)
        throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          occurances(0),
          occurances_irrelevant(0),
          total_irrelevant_occurances(0),
          occurances_freq_excess(0)
    {
    }

    PhraseOccurance::PhraseOccurance(
      const ::NewsGate::Moderation::Category::PhraseOccurance& src)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          phrase(src.phrase.in()),
          id(src.id),
          occurances(src.occurances),
          occurances_irrelevant(src.occurances_irrelevant),
          total_irrelevant_occurances(src.total_irrelevant_occurances),
          occurances_freq_excess(src.occurances_freq_excess)
    {
    }
    
    //
    // NewsGate::CategoryModeration::RelevantPhrases class
    //
    
    RelevantPhrases::RelevantPhrases(PyTypeObject *type,
                                     PyObject *args,
                                     PyObject *kwds)
        throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          phrases(new El::Python::Sequence()),
          relevant_message_count(0),
          irrelevant_message_count(0),
          search_time(0),
          total_time(0),
          phrase_counting_time(0),
          phrase_sorting_time(0),
          category_parsing_time(0),
          usefulness_calc_time(0)
          
    {
    }

    RelevantPhrases::RelevantPhrases(
      const ::NewsGate::Moderation::Category::RelevantPhrases& src)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          phrases(new El::Python::Sequence()),
          relevant_message_count(src.relevant_message_count),
          irrelevant_message_count(src.irrelevant_message_count),
          search_time(src.search_time),
          total_time(src.total_time),
          phrase_counting_time(src.phrase_counting_time),
          phrase_sorting_time(src.phrase_sorting_time),
          category_parsing_time(src.category_parsing_time),
          usefulness_calc_time(src.usefulness_calc_time),
          query(src.query.in())
    {
      phrases->reserve(src.phrases.length());

      for(size_t i = 0; i < src.phrases.length(); ++i)
      {
        phrases->push_back(new PhraseOccurance(src.phrases[i]));
      }
    }
    
    //
    // NewsGate::CategoryModeration::WordFinding class
    //
    
    WordFinding::WordFinding(PyTypeObject *type,
                             PyObject *args,
                             PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          f(0),
          t(0),
          s(0),
          e(0)
    {
    }

    WordFinding::WordFinding(
      const ::NewsGate::Moderation::Category::WordFinding& src)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          text(src.text.in()),
          f(src.from),
          t(src.to),
          s(src.start),
          e(src.end)
    {
    }
    
    //
    // NewsGate::CategoryModeration::WordListFinding class
    //
    
    WordListFinding::WordListFinding(PyTypeObject *type,
                                     PyObject *args,
                                     PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          words(new El::Python::Sequence())
    {
    }

    WordListFinding::WordListFinding(
      const ::NewsGate::Moderation::Category::WordListFinding& src)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          name(src.name.in()),
          words(new El::Python::Sequence())
    {
      words->resize(src.words.length());

      for(size_t i = 0; i < src.words.length(); ++i)
      {
        (*words)[i] = new WordFinding(src.words[i]);
      }
    }
    
    //
    // NewsGate::CategoryModeration::CategoryFinding class
    //
    
    CategoryFinding::CategoryFinding(PyTypeObject *type,
                                     PyObject *args,
                                     PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          word_lists(new El::Python::Sequence())
    {
    }

    CategoryFinding::CategoryFinding(
      const ::NewsGate::Moderation::Category::CategoryFinding& src)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          id(src.id),
          path(src.path.in()),
          word_lists(new El::Python::Sequence())
    {
      word_lists->resize(src.word_lists.length());

      for(size_t i = 0; i < src.word_lists.length(); ++i)
      {
        (*word_lists)[i] = new WordListFinding(src.word_lists[i]);
      }
    }
    
    //
    // NewsGate::CategoryModeration::LocaleDescriptor class
    //
    
    LocaleDescriptor::LocaleDescriptor(PyTypeObject *type,
                                       PyObject *args,
                                       PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          lang(new El::Python::Lang()),
          country(new El::Python::Country())
    {
    }

    LocaleDescriptor::LocaleDescriptor(
        const ::NewsGate::Moderation::Category::LocaleDescriptor& src)
        throw(Exception, El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          lang(new El::Python::Lang((El::Lang::ElCode)src.lang)),
          country(new El::Python::Country((El::Country::ElCode)src.country)),
          name(src.name.in()),
          title(src.title.in()),
          short_title(src.short_title.in()),
          description(src.description.in()),
          keywords(src.keywords.in())
    {      
    }
    
    void
    LocaleDescriptor::copy(
      ::NewsGate::Moderation::Category::LocaleDescriptor& dest) const
      throw(Exception, El::Exception)
    {
      dest.lang = lang->el_code();
      dest.country = country->el_code();
      dest.name = name.c_str();
      dest.title = title.c_str();
      dest.short_title = short_title.c_str();
      dest.description = description.c_str();
      dest.keywords = keywords.c_str();
    }
    
    //
    // NewsGate::CategoryModeration::ExpressionDescriptor class
    //
    
    ExpressionDescriptor::ExpressionDescriptor(PyTypeObject *type,
                                               PyObject *args,
                                               PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance)
    {
    }

    ExpressionDescriptor::ExpressionDescriptor(
        const ::NewsGate::Moderation::Category::ExpressionDescriptor& src)
        throw(Exception, El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          expression(src.expression.in()),
          description(src.description)
    {
    }
    
    void
    ExpressionDescriptor::copy(
      ::NewsGate::Moderation::Category::ExpressionDescriptor& dest) const
      throw(Exception, El::Exception)
    {
      dest.expression = expression.c_str();
      dest.description = description.c_str();
    }
    
    //
    // NewsGate::CategoryModeration::WordListDescriptor class
    //
    
    WordListDescriptor::WordListDescriptor(PyTypeObject *type,
                                           PyObject *args,
                                           PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          version(0),
          updated(false)
    {
    }

    WordListDescriptor::WordListDescriptor(
        const ::NewsGate::Moderation::Category::WordListDescriptor& src)
        throw(Exception, El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          name(src.name.in()),
          words(src.words.in()),
          version(src.version),
          updated(src.updated),
          description(src.description),
          id("")
    {
    }
    
    void
    WordListDescriptor::copy(
      ::NewsGate::Moderation::Category::WordListDescriptor& dest) const
      throw(Exception, El::Exception)
    {
      dest.name = name.c_str();
      dest.words = words.c_str();
      dest.version = version;
      dest.updated = updated;
      dest.description = description.c_str();
    }

    //
    // NewsGate::CategoryModeration::CategoryPathElem class
    //    
    CategoryPathElem::CategoryPathElem(PyTypeObject *type,
                                       PyObject *args,
                                       PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance)
    {
    }

    CategoryPathElem::CategoryPathElem(
        const ::NewsGate::Moderation::Category::CategoryPathElem& src)
        throw(Exception, El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          name(src.name),
          id(src.id)
    {
    }
    
    void
    CategoryPathElem::copy(
      ::NewsGate::Moderation::Category::CategoryPathElem& dest) const
      throw(Exception, El::Exception)
    {
      dest.name = name.c_str();
      dest.id = id;
    }
    
    //
    // NewsGate::CategoryModeration::CategoryPath class
    //    
    CategoryPath::CategoryPath(PyTypeObject *type,
                               PyObject *args,
                               PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          elems(new El::Python::Sequence())
    {
    }

    CategoryPath::CategoryPath(
        const ::NewsGate::Moderation::Category::CategoryPath& src)
        throw(Exception, El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          elems(new El::Python::Sequence()),
          path(src.path)
    {
      unsigned long len = src.elems.length();
      elems->resize(len);

      for(unsigned long i = 0; i < len; i++)
      {
        (*elems)[i] = new CategoryPathElem(src.elems[i]);
      }
    }
    
    void
    CategoryPath::copy(
      ::NewsGate::Moderation::Category::CategoryPath& dest) const
      throw(Exception, El::Exception)
    {
      dest.path = path.c_str();

      dest.elems.length(elems->size());
      
      for(unsigned long i = 0; i < elems->size(); i++)
      {
        CategoryPathElem* elem =
          CategoryPathElem::Type::down_cast((*elems)[i].in());
        
        elem->copy(dest.elems[i]);
      }

    }
    
    //
    // NewsGate::CategoryModeration::CategoryDescriptor class
    //
    
    CategoryDescriptor::CategoryDescriptor(PyTypeObject *type,
                                           PyObject *args,
                                           PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0),
          status(::NewsGate::Moderation::Category::CS_ENABLED),
          searcheable(::NewsGate::Moderation::Category::CR_YES),
          locales(new El::Python::Sequence()),
          expressions(new El::Python::Sequence()),
          word_lists(new El::Python::Sequence()),
          excluded_messages(new El::Python::Sequence()),
          included_messages(new El::Python::Sequence()),
          parents(new El::Python::Sequence()),
          children(new El::Python::Sequence()),
          creator_id(0),
          version(0),
          paths(new El::Python::Sequence())
    {
    }

    CategoryDescriptor::CategoryDescriptor(
        const ::NewsGate::Moderation::Category::CategoryDescriptor& src)
        throw(Exception, El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          id(src.id),
          name(src.name.in()),
          status(src.status),
          searcheable(src.searcheable),
          locales(new El::Python::Sequence()),
          expressions(new El::Python::Sequence()),
          word_lists(new El::Python::Sequence()),
          excluded_messages(new El::Python::Sequence()),
          included_messages(new El::Python::Sequence()),
          parents(new El::Python::Sequence()),
          children(new El::Python::Sequence()),
          description(src.description.in()),
          creator_id(src.creator_id),
          version(src.version),
          paths(new El::Python::Sequence())
    {
      unsigned long len = src.locales.length();
      locales->resize(len);

      for(unsigned long i = 0; i < len; i++)
      {
        (*locales)[i] = new LocaleDescriptor(src.locales[i]);
      }

      len = src.expressions.length();
      expressions->resize(len);

      for(unsigned long i = 0; i < len; i++)
      {
        (*expressions)[i] = new ExpressionDescriptor(src.expressions[i]);
      }

      len = src.word_lists.length();
      word_lists->resize(len);

      for(unsigned long i = 0; i < len; i++)
      {
        (*word_lists)[i] = new WordListDescriptor(src.word_lists[i]);
      }

      len = src.excluded_messages.length();
      excluded_messages->resize(len);
      
      for(unsigned long i = 0; i < len; i++)
      {
        Message::Id id((uint64_t)src.excluded_messages[i]);
        (*excluded_messages)[i] = PyString_FromString(id.string().c_str());
      }

      len = src.included_messages.length();
      included_messages->resize(len);
      
      for(unsigned long i = 0; i < len; i++)
      {
        Message::Id id((uint64_t)src.included_messages[i]);
        (*included_messages)[i] = PyString_FromString(id.string().c_str());
      }

      len = src.parents.length();
      parents->resize(len);

      for(unsigned long i = 0; i < len; i++)
      {
        (*parents)[i] = new CategoryDescriptor(src.parents[i]);
      }

      len = src.children.length();
      children->resize(len);

      for(unsigned long i = 0; i < len; i++)
      {
        (*children)[i] = new CategoryDescriptor(src.children[i]);
      }

      len = src.paths.length();
      paths->resize(len);

      for(unsigned long i = 0; i < len; i++)
      {
        (*paths)[i] = new CategoryPath(src.paths[i]);
      }
    }

    void
    CategoryDescriptor::copy(
      ::NewsGate::Moderation::Category::CategoryDescriptor& dest) const
      throw(ExpressionError, WordListError, Exception, El::Exception)
    {
      dest.id = id;
      dest.name = name.c_str();
      dest.description = description.c_str();
      dest.creator_id = creator_id;
      dest.version = version;
      
      if(status >= ::NewsGate::Moderation::Category::CS_COUNT)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::CategoryModeration::CategoryDescriptor::copy: "
          "unexpected status " << status;
        
        throw Exception(ostr.str());
      }
      
      dest.status = (::NewsGate::Moderation::Category::CategoryStatus)status;
      
      if(searcheable >= ::NewsGate::Moderation::Category::CR_COUNT)
      {
        std::ostringstream ostr;
        ostr << "NewsGate::CategoryModeration::CategoryDescriptor::copy: "
          "unexpected searcheable flag " << searcheable;
        
        throw Exception(ostr.str());
      }
      
      dest.searcheable =
        (::NewsGate::Moderation::Category::CategorySearcheable)searcheable;

      dest.locales.length(locales->size());
      
      for(unsigned long i = 0; i < locales->size(); i++)
      {
        LocaleDescriptor* loc =
          LocaleDescriptor::Type::down_cast((*locales)[i].in());

        loc->copy(dest.locales[i]);
      }
      
      dest.word_lists.length(word_lists->size());
      
      for(unsigned long i = 0; i < word_lists->size(); i++)
      {
        WordListDescriptor* wl =
          WordListDescriptor::Type::down_cast((*word_lists)[i].in());

        std::wstring wname;
        El::String::Manip::utf8_to_wchar(wl->name.c_str(), wname);

        if(wname.length() > MAX_WORDS_NAME_LEN)
        {
          std::ostringstream ostr;
          ostr << "max word list name lenght (" << MAX_WORDS_NAME_LEN
               << ") exceeded: " << wname.length() << " chars";

          throw WordListNameError(wl->name.c_str(),
                                  MAX_WORDS_NAME_LEN + 1,
                                  ostr.str().c_str());
        }
        
        std::wstring words;
        El::String::Manip::utf8_to_wchar(wl->words.c_str(), words);

        if(words.length() > MAX_WORDS_LEN)
        {
          std::ostringstream ostr;
          ostr << "max word list lenght (" << MAX_WORDS_LEN
               << ") exceeded: " << words.length() << " chars";

          throw WordListError(wl->name.c_str(),
                              MAX_WORDS_LEN + 1,
                              ostr.str().c_str());
        }

        wl->copy(dest.word_lists[i]);
      }
      
      dest.expressions.length(expressions->size());
      
      for(unsigned long i = 0; i < expressions->size(); i++)
      {
        ExpressionDescriptor* exp =
          ExpressionDescriptor::Type::down_cast((*expressions)[i].in());

        std::wstring query;
        El::String::Manip::utf8_to_wchar(exp->expression.c_str(),
                                         query);

        if(query.length() > MAX_EXPR_LEN)
        {
          std::ostringstream ostr;
          ostr << "max expression lenght (" << MAX_EXPR_LEN
               << ") exceeded: " << query.length() << " chars";

          throw ExpressionError(i, MAX_EXPR_LEN + 1, ostr.str().c_str());
        }

/*        
        std::string expression;
        El::String::Manip::suppress(exp->expression.c_str(), expression, "\r");
        El::String::Manip::utf8_to_wchar(expression.c_str(), query);

        Search::ExpressionParser parser;
        std::wistringstream istr(query);

        try
        {
          parser.parse(istr);
        }
        catch(const Search::ParseError& e)
        {
          const char* desc = strstr(e.what(), ": ");
          throw ExpressionError(i, e.position, desc ? desc + 2 : e.what());
        }        
*/      
        exp->copy(dest.expressions[i]);
      }

      dest.parents.length(parents->size());
      
      for(unsigned long i = 0; i < parents->size(); i++)
      {
        CategoryDescriptor* cat =
          CategoryDescriptor::Type::down_cast((*parents)[i].in());
        
        cat->copy(dest.parents[i]);
      }

      dest.children.length(children->size());
      
      for(unsigned long i = 0; i < children->size(); i++)
      {
        CategoryDescriptor* cat =
          CategoryDescriptor::Type::down_cast((*children)[i].in());
        
        cat->copy(dest.children[i]);
      }

      dest.paths.length(paths->size());
      
      for(unsigned long i = 0; i < paths->size(); i++)
      {
        CategoryPath* path = CategoryPath::Type::down_cast((*paths)[i].in());
        path->copy(dest.paths[i]);
      }
    }
    
  }
}
