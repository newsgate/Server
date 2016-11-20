/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Frontends/Moderation/CategoryModeration.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_FRONTENDS_MODERATION_CATEGORYMODERATION_HPP_
#define _NEWSGATE_SERVER_FRONTENDS_MODERATION_CATEGORYMODERATION_HPP_

#include <string>

#include <El/Exception.hpp>

#include <El/Python/Exception.hpp>
#include <El/Python/Object.hpp>
#include <El/Python/RefCount.hpp>
#include <El/Python/Logger.hpp>
#include <El/Python/Sequence.hpp>
#include <El/Python/Lang.hpp>
#include <El/Python/Country.hpp>

#include <Services/Moderator/Commons/CategoryManager.hpp>

namespace NewsGate
{  
  namespace CategoryModeration
  {
    EL_EXCEPTION(Exception, El::ExceptionBase);
    
    typedef El::Corba::SmartRef<NewsGate::Moderation::Category::Manager>
    ManagerRef;

    class Manager : public El::Python::ObjectImpl
    {
    public:
      Manager(PyTypeObject *type, PyObject *args, PyObject *kwds)
        throw(Exception, El::Exception);
    
      Manager(const ManagerRef& manager) throw(Exception, El::Exception);

      virtual ~Manager() throw() {}

      PyObject* py_create_category(PyObject* args) throw(El::Exception);
      PyObject* py_update_category(PyObject* args) throw(El::Exception);
      PyObject* py_update_word_lists(PyObject* args) throw(El::Exception);
      PyObject* py_get_category(PyObject* args) throw(El::Exception);
      PyObject* py_get_category_version(PyObject* args) throw(El::Exception);
      PyObject* py_find_category(PyObject* args) throw(El::Exception);
      PyObject* py_delete_categories(PyObject* args) throw(El::Exception);
      PyObject* py_add_category_message(PyObject* args) throw(El::Exception);
      PyObject* py_find_text(PyObject* args) throw(El::Exception);
//      PyObject* py_relevant_phrases(PyObject* args) throw(El::Exception);
      
      PyObject* py_category_relevant_phrases(PyObject* args)
        throw(El::Exception);

      class Type : public El::Python::ObjectTypeImpl<Manager, Manager::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_METHOD_VARARGS(py_create_category,
                               "create_category",
                               "Creates category");

        PY_TYPE_METHOD_VARARGS(py_update_category,
                               "update_category",
                               "Updates category");

        PY_TYPE_METHOD_VARARGS(py_update_word_lists,
                               "update_word_lists",
                               "Updates category word lists");

        PY_TYPE_METHOD_VARARGS(py_get_category,
                               "get_category",
                               "Gets category");

        PY_TYPE_METHOD_VARARGS(py_get_category_version,
                               "get_category_version",
                               "Gets category and word lists version");

        PY_TYPE_METHOD_VARARGS(py_find_category,
                               "find_category",
                               "Finds category");

        PY_TYPE_METHOD_VARARGS(py_delete_categories,
                               "delete_categories",
                               "Delete categories");
        
        PY_TYPE_METHOD_VARARGS(py_add_category_message,
                               "add_category_message",
                               "Add category message");
        
        PY_TYPE_METHOD_VARARGS(py_find_text,
                               "find_text",
                               "Finds text lines in word lists");

        PY_TYPE_METHOD_VARARGS(py_category_relevant_phrases,
                               "category_relevant_phrases",
                               "Finds category relevant phrases");
      };

    private:
      ManagerRef manager_;
    };

    typedef El::Python::SmartPtr<Manager> Manager_var;

    class PhraseOccurance : public El::Python::ObjectImpl
    {
    public:
      std::string phrase;
      unsigned long long id;
      unsigned long occurances;
      unsigned long occurances_irrelevant;
      unsigned long total_irrelevant_occurances;
      float occurances_freq_excess;
      
    public:
      PhraseOccurance(PyTypeObject *type = 0,
                      PyObject *args = 0,
                      PyObject *kwds = 0)
        throw(Exception, El::Exception);
    
      PhraseOccurance(
        const ::NewsGate::Moderation::Category::PhraseOccurance& src)
        throw(Exception, El::Exception);

      virtual ~PhraseOccurance() throw() {}

      class Type :
        public El::Python::ObjectTypeImpl<PhraseOccurance,
                                          PhraseOccurance::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_STRING(phrase,
                              "phrase",
                              "Phrase",
                              false);
        
        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Phrase id");
        
        PY_TYPE_MEMBER_ULONG(occurances,
                             "occurances",
                             "Phrase occurance count");
        
        PY_TYPE_MEMBER_ULONG(total_irrelevant_occurances,
                             "total_irrelevant_occurances",
                             "Total phrase irrelevant occurances");
        
        PY_TYPE_MEMBER_ULONG(
          occurances_irrelevant,
          "occurances_irrelevant",
          "Phrase occurance count in irrelevant message queried");

        PY_TYPE_MEMBER_FLOAT(occurances_freq_excess,
                             "occurances_freq_excess",
                             "Phrase occurance frequence excess rate");
      };
    };

    typedef El::Python::SmartPtr<PhraseOccurance> PhraseOccurance_var;    
    
    class RelevantPhrases : public El::Python::ObjectImpl
    {
    public:
      El::Python::Sequence_var phrases;
      unsigned long relevant_message_count;
      unsigned long irrelevant_message_count;
      unsigned long search_time;
      unsigned long total_time;
      unsigned long phrase_counting_time;
      unsigned long phrase_sorting_time;
      unsigned long category_parsing_time;
      unsigned long usefulness_calc_time;
      std::string query;
      
    public:
      RelevantPhrases(PyTypeObject *type = 0,
                      PyObject *args = 0,
                      PyObject *kwds = 0)
        throw(Exception, El::Exception);
    
      RelevantPhrases(
        const ::NewsGate::Moderation::Category::RelevantPhrases& src)
        throw(Exception, El::Exception);

      virtual ~RelevantPhrases() throw() {}

      class Type :
        public El::Python::ObjectTypeImpl<RelevantPhrases,
                                          RelevantPhrases::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_OBJECT(phrases,
                              El::Python::Sequence::Type,
                              "phrases",
                              "Relevant phrases",
                              false);
        
        PY_TYPE_MEMBER_ULONG(relevant_message_count,
                             "relevant_message_count",
                             "Relevant message count");
        
        PY_TYPE_MEMBER_ULONG(irrelevant_message_count,
                             "irrelevant_message_count",
                             "Irrelevant message count");

        PY_TYPE_MEMBER_ULONG(search_time,
                             "search_time",
                             "Search time");

        PY_TYPE_MEMBER_ULONG(total_time,
                             "total_time",
                             "Total time");

        PY_TYPE_MEMBER_ULONG(phrase_counting_time,
                             "phrase_counting_time",
                             "Phrase Counting time");

        PY_TYPE_MEMBER_ULONG(phrase_sorting_time,
                             "phrase_sorting_time",
                             "Phrase Sorting time");

        PY_TYPE_MEMBER_ULONG(category_parsing_time,
                             "category_parsing_time",
                             "Category Parsing time");

        PY_TYPE_MEMBER_ULONG(usefulness_calc_time,
                             "usefulness_calc_time",
                             "Phrase usefulness calculation time");

        PY_TYPE_MEMBER_STRING(query, "query", "Resulted query", true);        
      };
    };

    typedef El::Python::SmartPtr<RelevantPhrases> RelevantPhrases_var;
    
    class WordFinding : public El::Python::ObjectImpl
    {
    public:
      std::string text;
      unsigned long f;
      unsigned long t;
      unsigned long s;
      unsigned long e;
      
    public:
      WordFinding(PyTypeObject *type = 0,
                  PyObject *args = 0,
                  PyObject *kwds = 0)
        throw(Exception, El::Exception);
    
      WordFinding(
        const ::NewsGate::Moderation::Category::WordFinding& src)
        throw(Exception, El::Exception);

      virtual ~WordFinding
      () throw() {}

      class Type :
        public El::Python::ObjectTypeImpl<WordFinding,
                                          WordFinding::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_STRING(text,
                              "text",
                              "Word text",
                              false);

        PY_TYPE_MEMBER_ULONG(f, "f", "Finding word offset");
        PY_TYPE_MEMBER_ULONG(t, "t", "Finding word end");
        PY_TYPE_MEMBER_ULONG(s, "s", "Finding word list offset");
        PY_TYPE_MEMBER_ULONG(e, "e", "Finding word list end");
      };
    };

    typedef El::Python::SmartPtr<WordFinding> WordFinding_var;

    class WordListFinding : public El::Python::ObjectImpl
    {
    public:
      std::string name;
      El::Python::Sequence_var words;
      
    public:
      WordListFinding(PyTypeObject *type = 0,
                      PyObject *args = 0,
                      PyObject *kwds = 0)
        throw(Exception, El::Exception);
    
      WordListFinding(
        const ::NewsGate::Moderation::Category::WordListFinding& src)
        throw(Exception, El::Exception);

      virtual ~WordListFinding() throw() {}

      class Type :
        public El::Python::ObjectTypeImpl<WordListFinding,
                                          WordListFinding::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_STRING(name,
                              "name",
                              "Word list name",
                              false);
        
        PY_TYPE_MEMBER_OBJECT(words,
                              El::Python::Sequence::Type,
                              "words",
                              "Words found",
                              false);
      };
    };

    typedef El::Python::SmartPtr<WordListFinding> WordListFinding_var;

    class CategoryFinding : public El::Python::ObjectImpl
    {
    public:
      unsigned long long id;
      std::string path;
      El::Python::Sequence_var word_lists;
      
    public:
      CategoryFinding(PyTypeObject *type = 0,
                      PyObject *args = 0,
                      PyObject *kwds = 0)
        throw(Exception, El::Exception);
    
      CategoryFinding(
        const ::NewsGate::Moderation::Category::CategoryFinding& src)
        throw(Exception, El::Exception);

      virtual ~CategoryFinding() throw() {}

      class Type :
        public El::Python::ObjectTypeImpl<CategoryFinding,
                                          CategoryFinding::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONGLONG(id,
                                 "id",
                                 "Category id");

        PY_TYPE_MEMBER_STRING(path,
                              "path",
                              "Category path",
                              false);
        
        PY_TYPE_MEMBER_OBJECT(word_lists,
                              El::Python::Sequence::Type,
                              "word_lists",
                              "Word list findings",
                              false);
      };
    };

    typedef El::Python::SmartPtr<CategoryFinding> CategoryFinding_var;

    class LocaleDescriptor : public El::Python::ObjectImpl
    {
    public:
      El::Python::Lang_var lang;
      El::Python::Country_var country;
      std::string name;
      std::string title;
      std::string short_title;
      std::string description;
      std::string keywords;
      
    public:
      LocaleDescriptor(PyTypeObject *type = 0,
                       PyObject *args = 0,
                       PyObject *kwds = 0)
        throw(Exception, El::Exception);
    
      LocaleDescriptor(
        const ::NewsGate::Moderation::Category::LocaleDescriptor& src)
        throw(Exception, El::Exception);

      virtual ~LocaleDescriptor() throw() {}

      void copy(::NewsGate::Moderation::Category::LocaleDescriptor& dest)
        const throw(Exception, El::Exception);

      class Type :
        public El::Python::ObjectTypeImpl<LocaleDescriptor,
                                          LocaleDescriptor::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_OBJECT(lang,
                              El::Python::Lang::Type,
                              "lang",
                              "Localization language",
                              false);

        PY_TYPE_MEMBER_OBJECT(country,
                              El::Python::Country::Type,
                              "country",
                              "Localization country",
                              false);

        PY_TYPE_MEMBER_STRING(name,
                              "name",
                              "Localized category name",
                              true);

        PY_TYPE_MEMBER_STRING(title,
                              "title",
                              "Localized category title",
                              true);

        PY_TYPE_MEMBER_STRING(short_title,
                              "short_title",
                              "Localized category short title",
                              true);

        PY_TYPE_MEMBER_STRING(description,
                              "description",
                              "Localized category description",
                              true);

        PY_TYPE_MEMBER_STRING(keywords,
                              "keywords",
                              "Localized category keywords",
                              true);
      };
    };

    typedef El::Python::SmartPtr<LocaleDescriptor>
    LocaleDescriptor_var;
    
    class ExpressionDescriptor : public El::Python::ObjectImpl
    {
    public:
      std::string expression;
      std::string description;
      
    public:
      ExpressionDescriptor(PyTypeObject *type = 0,
                           PyObject *args = 0,
                           PyObject *kwds = 0)
        throw(Exception, El::Exception);
    
      ExpressionDescriptor(
        const ::NewsGate::Moderation::Category::ExpressionDescriptor& src)
        throw(Exception, El::Exception);

      virtual ~ExpressionDescriptor() throw() {}

      void copy(::NewsGate::Moderation::Category::ExpressionDescriptor& dest)
        const throw(Exception, El::Exception);

      class Type :
        public El::Python::ObjectTypeImpl<ExpressionDescriptor,
                                          ExpressionDescriptor::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_STRING(expression,
                              "expression",
                              "Expression value",
                              true);

        PY_TYPE_MEMBER_STRING(description,
                              "description",
                              "Expression description",
                              true);
      };
    };

    typedef El::Python::SmartPtr<ExpressionDescriptor>
    ExpressionDescriptor_var;
    
    class WordListDescriptor : public El::Python::ObjectImpl
    {
    public:
      std::string name;
      std::string words;
      unsigned long long version;
      bool updated;
      std::string description;
      std::string id;
      
    public:
      WordListDescriptor(PyTypeObject *type = 0,
                         PyObject *args = 0,
                         PyObject *kwds = 0)
        throw(Exception, El::Exception);
    
      WordListDescriptor(
        const ::NewsGate::Moderation::Category::WordListDescriptor& src)
        throw(Exception, El::Exception);

      virtual ~WordListDescriptor() throw() {}

      void copy(::NewsGate::Moderation::Category::WordListDescriptor& dest)
        const throw(Exception, El::Exception);

      class Type :
        public El::Python::ObjectTypeImpl<WordListDescriptor,
                                          WordListDescriptor::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_STRING(name,
                              "name",
                              "Word list name",
                              false);

        PY_TYPE_MEMBER_STRING(words,
                              "words",
                              "Word list",
                              true);
        
        PY_TYPE_MEMBER_ULONGLONG(version, "version", "Word list version");

        PY_TYPE_MEMBER_BOOL(updated, "updated", "Word list update flag");

        PY_TYPE_MEMBER_STRING(description,
                              "description",
                              "Word list description",
                              true);

        PY_TYPE_MEMBER_STRING(id,
                              "id",
                              "Word list id",
                              true);
      };
    };

    typedef El::Python::SmartPtr<WordListDescriptor>
    WordListDescriptor_var;
    
    class CategoryPathElem : public El::Python::ObjectImpl
    {
    public:
      std::string name;
      unsigned long long id;
      
    public:
      CategoryPathElem(PyTypeObject *type = 0,
                       PyObject *args = 0,
                       PyObject *kwds = 0)
        throw(Exception, El::Exception);
    
      CategoryPathElem(
        const ::NewsGate::Moderation::Category::CategoryPathElem& src)
        throw(Exception, El::Exception);
      
      virtual ~CategoryPathElem() throw() {}
      
      void copy(::NewsGate::Moderation::Category::CategoryPathElem& dest)
        const throw(Exception, El::Exception);
      
      class Type :
        public El::Python::ObjectTypeImpl<CategoryPathElem,
                                          CategoryPathElem::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_STRING(name,
                              "name",
                              "Category path element name",
                              true);
        
        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Category path element id");
      };
    };

    typedef El::Python::SmartPtr<CategoryPathElem> CategoryPathElem_var;
    
    class CategoryPath : public El::Python::ObjectImpl
    {
    public:
      El::Python::Sequence_var elems;
      std::string path;
      
    public:
      CategoryPath(PyTypeObject *type = 0,
                   PyObject *args = 0,
                   PyObject *kwds = 0)
        throw(Exception, El::Exception);
    
      CategoryPath(
        const ::NewsGate::Moderation::Category::CategoryPath& src)
        throw(Exception, El::Exception);
      
      virtual ~CategoryPath() throw() {}
      
      void copy(::NewsGate::Moderation::Category::CategoryPath& dest)
        const throw(Exception, El::Exception);
      
      class Type :
        public El::Python::ObjectTypeImpl<CategoryPath,
                                          CategoryPath::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_OBJECT(elems,
                              El::Python::Sequence::Type,
                              "elems",
                              "Category path elements",
                              false);

        PY_TYPE_MEMBER_STRING(path,
                              "path",
                              "Category path",
                              true);
      };
    };

    typedef El::Python::SmartPtr<CategoryPath> CategoryPath_var;
    
    class CategoryDescriptor : public El::Python::ObjectImpl
    {
    public:

      struct ExpressionError
      {
        unsigned long number;
        unsigned long position;
        std::string description;

        ExpressionError(unsigned long num = 0,
                        unsigned long pos = 0,
                        const char* desc = 0)
          throw(El::Exception);
      };
      
      struct WordListError
      {
        std::string name;
        unsigned long position;
        std::string description;

        WordListError(const char* nm = 0,
                      unsigned long pos = 0,
                      const char* desc = 0)
          throw(El::Exception);
      };
      
      struct WordListNameError : public WordListError
      {
        WordListNameError(const char* nm = 0,
                          unsigned long pos = 0,
                          const char* desc = 0)
          throw(El::Exception);
      };
      
    public:
      unsigned long long id;
      std::string name;
      unsigned long status;
      unsigned long searcheable;
      El::Python::Sequence_var locales;
      El::Python::Sequence_var expressions;
      El::Python::Sequence_var word_lists;
      El::Python::Sequence_var excluded_messages;
      El::Python::Sequence_var included_messages;
      El::Python::Sequence_var parents;
      El::Python::Sequence_var children;
      std::string description;
      unsigned long long creator_id;
      unsigned long long version;
      El::Python::Sequence_var paths;
      
    public:
      CategoryDescriptor(PyTypeObject *type = 0,
                         PyObject *args = 0,
                         PyObject *kwds = 0)
        throw(Exception, El::Exception);
    
      CategoryDescriptor(
        const ::NewsGate::Moderation::Category::CategoryDescriptor& src)
        throw(Exception, El::Exception);

      void copy(::NewsGate::Moderation::Category::CategoryDescriptor& dest)
        const throw(ExpressionError, WordListError, Exception, El::Exception);

      virtual ~CategoryDescriptor() throw() {}

      class Type : public El::Python::ObjectTypeImpl<CategoryDescriptor,
                                                     CategoryDescriptor::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Category id");
        PY_TYPE_MEMBER_STRING(name, "name", "Category name", true);
        PY_TYPE_MEMBER_ULONG(status, "status", "Category status");
        PY_TYPE_MEMBER_ULONG(searcheable, "searcheable", "Searcheable flag");

        PY_TYPE_MEMBER_OBJECT(locales,
                              El::Python::Sequence::Type,
                              "locales",
                              "Category locales",
                              false);
        
        PY_TYPE_MEMBER_OBJECT(expressions,
                              El::Python::Sequence::Type,
                              "expressions",
                              "Category expressions",
                              false);
        
        PY_TYPE_MEMBER_OBJECT(word_lists,
                              El::Python::Sequence::Type,
                              "word_lists",
                              "Category word lists",
                              false);
        
        PY_TYPE_MEMBER_OBJECT(excluded_messages,
                              El::Python::Sequence::Type,
                              "excluded_messages",
                              "Category excluded messages",
                              false);
        
        PY_TYPE_MEMBER_OBJECT(included_messages,
                              El::Python::Sequence::Type,
                              "included_messages",
                              "Category included messages",
                              false);
        
        PY_TYPE_MEMBER_OBJECT(parents,
                              El::Python::Sequence::Type,
                              "parents",
                              "Parent categories",
                              false);
        
        PY_TYPE_MEMBER_OBJECT(children,
                              El::Python::Sequence::Type,
                              "children",
                              "Children categories",
                              false);
        
        PY_TYPE_MEMBER_STRING(description,
                              "description",
                              "Category description",
                              true);

        PY_TYPE_MEMBER_ULONGLONG(creator_id, "creator_id", "Creator id");
        PY_TYPE_MEMBER_ULONGLONG(version, "version", "Category version");
        
        PY_TYPE_MEMBER_OBJECT(paths,
                              El::Python::Sequence::Type,
                              "paths",
                              "Category paths",
                              false);
      };
    };

    typedef El::Python::SmartPtr<CategoryDescriptor> CategoryDescriptor_var;

    class ExpressionError : public El::Python::ObjectImpl,
                            public CategoryDescriptor::ExpressionError
    {
    public:
      ExpressionError(PyTypeObject *type = 0,
                      PyObject *args = 0,
                      PyObject *kwds = 0)
        throw(Exception, El::Exception);

      virtual ~ExpressionError() throw() {}

      class Type :
        public El::Python::ObjectTypeImpl<ExpressionError,
                                          ExpressionError::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;
        
        PY_TYPE_MEMBER_ULONG(number, "number", "Expression number");

        PY_TYPE_MEMBER_ULONG(position,
                             "position",
                             "Expression error position");

        PY_TYPE_MEMBER_STRING(description,
                              "description",
                              "Expression error description",
                              true);
      };
    };

    typedef El::Python::SmartPtr<ExpressionError> ExpressionError_var;
    
    class NoPath : public El::Python::ObjectImpl,
                   public CategoryDescriptor::ExpressionError
    {
    public:
      unsigned long long id;
      std::string name;
      
    public:
      NoPath(PyTypeObject *type = 0,
             PyObject *args = 0,
             PyObject *kwds = 0)
        throw(El::Exception);

      NoPath(const Moderation::Category::NoPath& e) throw(El::Exception);

      virtual ~NoPath() throw() {}

      class Type :
        public El::Python::ObjectTypeImpl<NoPath, NoPath::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;
        
        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Category id");
        PY_TYPE_MEMBER_STRING(name, "name", "Category name", true);
      };
    };

    typedef El::Python::SmartPtr<NoPath> NoPath_var;

    class Cycle : public El::Python::ObjectImpl,
                   public CategoryDescriptor::ExpressionError
    {
    public:
      unsigned long long id;
      std::string name;
      
    public:
      Cycle(PyTypeObject *type = 0, PyObject *args = 0, PyObject *kwds = 0)
        throw(El::Exception);

      Cycle(const Moderation::Category::Cycle& e) throw(El::Exception);

      virtual ~Cycle() throw() {}

      class Type :
        public El::Python::ObjectTypeImpl<Cycle, Cycle::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;
        
        PY_TYPE_MEMBER_ULONGLONG(id, "id", "Category id");
        PY_TYPE_MEMBER_STRING(name, "name", "Category name", true);
      };
    };

    typedef El::Python::SmartPtr<Cycle> Cycle_var;

    class ForbiddenOperation : public El::Python::ObjectImpl,
                               public CategoryDescriptor::ExpressionError
    {
    public:
      ForbiddenOperation(PyTypeObject *type = 0,
                         PyObject *args = 0,
                         PyObject *kwds = 0)
        throw(El::Exception);

      ForbiddenOperation(const Moderation::Category::ForbiddenOperation& e)
        throw(El::Exception);

      virtual ~ForbiddenOperation() throw() {}

      class Type :
        public El::Python::ObjectTypeImpl<ForbiddenOperation,
                                          ForbiddenOperation::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;
        
        PY_TYPE_MEMBER_STRING(description,
                              "description",
                              "Exception description",
                              true);
      };
    };

    typedef El::Python::SmartPtr<ForbiddenOperation> ForbiddenOperation_var;

    class VersionMismatch : public El::Python::ObjectImpl,
                               public CategoryDescriptor::ExpressionError
    {
    public:
      VersionMismatch(PyTypeObject *type = 0,
                         PyObject *args = 0,
                         PyObject *kwds = 0)
        throw(El::Exception);

      VersionMismatch(const Moderation::Category::VersionMismatch& e)
        throw(El::Exception);

      virtual ~VersionMismatch() throw() {}

      class Type :
        public El::Python::ObjectTypeImpl<VersionMismatch,
                                          VersionMismatch::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;
        
        PY_TYPE_MEMBER_STRING(description,
                              "description",
                              "Exception description",
                              true);
      };
    };

    typedef El::Python::SmartPtr<VersionMismatch> VersionMismatch_var;

    class WordListNotFound : public El::Python::ObjectImpl,
                             public CategoryDescriptor::ExpressionError
    {      
    public:
      WordListNotFound(PyTypeObject *type = 0,
                       PyObject *args = 0,
                       PyObject *kwds = 0)
        throw(El::Exception);

      WordListNotFound(const Moderation::Category::WordListNotFound& e)
        throw(El::Exception);

      virtual ~WordListNotFound() throw() {}

      class Type :
        public El::Python::ObjectTypeImpl<WordListNotFound,
                                          WordListNotFound::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;
        
        PY_TYPE_MEMBER_ULONG(number, "number", "Expression number");
        
        PY_TYPE_MEMBER_ULONG(position,
                             "position",
                             "Expression error position");
        
        PY_TYPE_MEMBER_STRING(description,
                              "description",
                              "Error description",
                              true);
      };
    };

    typedef El::Python::SmartPtr<WordListNotFound> WordListNotFound_var;

    class ExpressionParseError : public El::Python::ObjectImpl,
                                 public CategoryDescriptor::ExpressionError
    {      
    public:
      std::string name;
      
    public:
      ExpressionParseError(PyTypeObject *type = 0,
                           PyObject *args = 0,
                           PyObject *kwds = 0)
        throw(El::Exception);

      ExpressionParseError(const Moderation::Category::ExpressionParseError& e)
        throw(El::Exception);

      virtual ~ExpressionParseError() throw() {}

      class Type :
        public El::Python::ObjectTypeImpl<ExpressionParseError,
                                          ExpressionParseError::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;
        
        PY_TYPE_MEMBER_ULONG(number, "number", "Expression number");
        
        PY_TYPE_MEMBER_STRING(name,
                              "name",
                              "Word list name",
                              true);
        
        PY_TYPE_MEMBER_ULONG(position,
                             "position",
                             "Expression error position");
        
        PY_TYPE_MEMBER_STRING(description,
                              "description",
                              "Error description",
                              true);
      };
    };

    typedef El::Python::SmartPtr<ExpressionParseError>
    ExpressionParseError_var;

    class WordListError : public El::Python::ObjectImpl,
                          public CategoryDescriptor::WordListError
    {
    public:
      WordListError(PyTypeObject *type = 0,
                    PyObject *args = 0,
                    PyObject *kwds = 0)
        throw(Exception, El::Exception);

      virtual ~WordListError() throw() {}

      class Type :
        public El::Python::ObjectTypeImpl<WordListError,
                                          WordListError::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;
        
        PY_TYPE_MEMBER_STRING(name,
                              "name",
                              "Word list name",
                              false);

        PY_TYPE_MEMBER_ULONG(position,
                             "position",
                             "Word list error position");

        PY_TYPE_MEMBER_STRING(description,
                              "description",
                              "Word list error description",
                              true);
      };
    };

    typedef El::Python::SmartPtr<WordListError> WordListError_var;    

    class WordListNameError : public El::Python::ObjectImpl,
                              public CategoryDescriptor::WordListNameError
    {
    public:
      WordListNameError(PyTypeObject *type = 0,
                        PyObject *args = 0,
                        PyObject *kwds = 0)
        throw(Exception, El::Exception);

      virtual ~WordListNameError() throw() {}

      class Type :
        public El::Python::ObjectTypeImpl<WordListNameError,
                                          WordListNameError::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;
        
        PY_TYPE_MEMBER_STRING(name,
                              "name",
                              "Word list name",
                              false);

        PY_TYPE_MEMBER_ULONG(position,
                             "position",
                             "Word list error position");

        PY_TYPE_MEMBER_STRING(description,
                              "description",
                              "Word list error description",
                              true);
      };
    };

    typedef El::Python::SmartPtr<WordListNameError> WordListNameError_var;    
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{  
  namespace CategoryModeration
  {
    //
    // NewsGate::CategoryModeration::PhraseOccurance::Type class
    //
    inline
    PhraseOccurance::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<PhraseOccurance,
                                     PhraseOccurance::Type>(
          "newsgate.moderation.category.PhraseOccurance",
          "Object representing phrase occurance info")
    {
    }
 
    //
    // NewsGate::CategoryModeration::RelevantPhrases::Type class
    //
    inline
    RelevantPhrases::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<RelevantPhrases,
                                     RelevantPhrases::Type>(
          "newsgate.moderation.category.RelevantPhrases",
          "Object representing relevant phrases info")
    {
    }
 
    //
    // NewsGate::CategoryModeration::Manager::Type class
    //
    inline
    Manager::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<Manager, Manager::Type>(
          "newsgate.moderation.category.Manager",
          "Object representing category management functionality")
    {
      tp_new = 0;
    }

    //
    // NewsGate::CategoryModeration::WordFinding::Type class
    //
    inline
    WordFinding::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<WordFinding,
                                     WordFinding::Type>(
          "newsgate.moderation.category.WordFinding",
          "Object representing word finding")
    {
    }
 
    //
    // NewsGate::CategoryModeration::WordListFinding::Type class
    //
    inline
    WordListFinding::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<WordListFinding,
                                     WordListFinding::Type>(
          "newsgate.moderation.category.WordListFinding",
          "Object representing word list finding")
    {
    }
 
    //
    // NewsGate::CategoryModeration::CategoryFinding::Type class
    //
    inline
    CategoryFinding::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<CategoryFinding,
                                     CategoryFinding::Type>(
          "newsgate.moderation.category.CategoryFinding",
          "Object representing category finding")
    {
    }

    //
    // NewsGate::CategoryModeration::CategoryDescriptor::Type class
    //
    inline
    CategoryDescriptor::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<CategoryDescriptor,
                                     CategoryDescriptor::Type>(
          "newsgate.moderation.category.CategoryDescriptor",
          "Object representing category options")
    {
    }

    //
    // NewsGate::CategoryModeration::LocaleDescriptor::Type class
    //
    inline
    LocaleDescriptor::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<LocaleDescriptor,
                                     LocaleDescriptor::Type>(
          "newsgate.moderation.category.LocaleDescriptor",
          "Object representing locale options")
    {
    }
    
    //
    // NewsGate::CategoryModeration::ExpressionDescriptor::Type class
    //
    inline
    ExpressionDescriptor::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<ExpressionDescriptor,
                                     ExpressionDescriptor::Type>(
          "newsgate.moderation.category.ExpressionDescriptor",
          "Object representing expression options")
    {
    }
    
    //
    // NewsGate::CategoryModeration::WordListDescriptor::Type class
    //
    inline
    WordListDescriptor::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<WordListDescriptor,
                                     WordListDescriptor::Type>(
          "newsgate.moderation.category.WordListDescriptor",
          "Object representing word list options")
    {
    }
    
    //
    // NewsGate::CategoryModeration::CategoryPathElem::Type class
    //
    inline
    CategoryPathElem::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<CategoryPathElem,
                                     CategoryPathElem::Type>(
          "newsgate.moderation.category.CategoryPathElem",
          "Object representing category path element options")
    {
    }

    //
    // NewsGate::CategoryModeration::CategoryPath::Type class
    //
    inline
    CategoryPath::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<CategoryPath,
                                     CategoryPath::Type>(
          "newsgate.moderation.category.CategoryPath",
          "Object representing category path options")
    {
    }

    //
    // NewsGate::CategoryModeration::ExpressionError::Type class
    //
    inline
    ExpressionError::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<ExpressionError,
                                     ExpressionError::Type>(
          "newsgate.moderation.category.ExpressionError",
          "Object representing expression error information")
    {
    }

    //
    // NewsGate::CategoryModeration::WordListError::Type class
    //
    inline
    WordListError::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<WordListError,
                                     WordListError::Type>(
          "newsgate.moderation.category.WordListError",
          "Object representing word list error information")
    {
    }

    //
    // NewsGate::CategoryModeration::WordListNameError::Type class
    //
    inline
    WordListNameError::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<WordListNameError,
                                     WordListNameError::Type>(
          "newsgate.moderation.category.WordListNameError",
          "Object representing word list name error information")
    {
    }

    //
    // NewsGate::CategoryModeration::NoPath::Type class
    //
    inline
    NoPath::Type::Type() throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<NoPath, NoPath::Type>(
          "newsgate.moderation.category.NoPath",
          "Object representing no-path-to-root error information")
    {
    }

    //
    // NewsGate::CategoryModeration::Cycle::Type class
    //
    inline
    Cycle::Type::Type() throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<Cycle, Cycle::Type>(
          "newsgate.moderation.category.Cycle",
          "Object representing cycle error information")
    {
    }

    //
    // NewsGate::CategoryModeration::WordListNotFound::Type class
    //
    inline
    WordListNotFound::Type::Type() throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<WordListNotFound, WordListNotFound::Type>(
          "newsgate.moderation.category.WordListNotFound",
          "Object representing word list resolution error information")
    {
    }

    //
    // NewsGate::CategoryModeration::ExpressionParseError::Type class
    //
    inline
    ExpressionParseError::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<ExpressionParseError,
                                     ExpressionParseError::Type>(
          "newsgate.moderation.category.ExpressionParseError",
          "Object representing expression parsing error information")
    {
    }

    //
    // NewsGate::CategoryModeration::ForbiddenOperation::Type class
    //
    inline
    ForbiddenOperation::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<ForbiddenOperation,
                                     ForbiddenOperation::Type>(
          "newsgate.moderation.category.ForbiddenOperation",
          "Object representing forbidden operation exception description")
    {
    }

    //
    // NewsGate::CategoryModeration::VersionMismatch::Type class
    //
    inline
    VersionMismatch::Type::Type()
      throw(El::Python::Exception, El::Exception)
        : El::Python::ObjectTypeImpl<VersionMismatch,
                                     VersionMismatch::Type>(
          "newsgate.moderation.category.VersionMismatch",
          "Object representing version mismatch exception description")
    {
    }

    //
    // NewsGate::CategoryModeration::CategoryDescriptor::ExpressionError class
    //
    inline
    CategoryDescriptor::ExpressionError::ExpressionError(unsigned long num,
                                                         unsigned long pos,
                                                         const char* desc)
      throw(El::Exception)
        : number(num),
          position(pos),
          description(desc ? desc : "")
    {
    }
    
    //
    // NewsGate::CategoryModeration::CategoryDescriptor::WordListError class
    //
    inline
    CategoryDescriptor::WordListError::WordListError(const char* nm,
                                                     unsigned long pos,
                                                     const char* desc)
      throw(El::Exception)
        : name(nm ? nm : ""),
          position(pos),
          description(desc ? desc : "")
    {
    }
    
    //
    // NewsGate::CategoryModeration::CategoryDescriptor::WordListNameError class
    //
    inline
    CategoryDescriptor::WordListNameError::WordListNameError(const char* nm,
                                                             unsigned long pos,
                                                             const char* desc)
      throw(El::Exception) : WordListError(nm, pos, desc)
    {
    }
    
    //
    // NewsGate::CategoryModeration::ExpressionError class
    //
    inline
    ExpressionError::ExpressionError(PyTypeObject *type,
                                     PyObject *args,
                                     PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance)
    {
    }

    //
    // NewsGate::CategoryModeration::WordListError class
    //
    inline
    WordListError::WordListError(PyTypeObject *type,
                                 PyObject *args,
                                 PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance)
    {
    }

    //
    // NewsGate::CategoryModeration::WordListNameError class
    //
    inline
    WordListNameError::WordListNameError(PyTypeObject *type,
                                         PyObject *args,
                                         PyObject *kwds)
      throw(Exception, El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance)
    {
    }

    //
    // NewsGate::CategoryModeration::NoPath class
    //
    inline
    NoPath::NoPath(PyTypeObject *type, PyObject *args, PyObject *kwds)
      throw(El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0)
    {
    }

    inline
    NoPath::NoPath(const Moderation::Category::NoPath& e) throw(El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          id(e.id),
          name(e.name.in())
    {
    }

    //
    // NewsGate::CategoryModeration::WordListNotFound class
    //
    inline
    WordListNotFound::WordListNotFound(PyTypeObject *type,
                                       PyObject *args,
                                       PyObject *kwds)
      throw(El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance)
    {
    }

    inline
    WordListNotFound::WordListNotFound(
      const Moderation::Category::WordListNotFound& e) throw(El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          ExpressionError(e.expression_index, e.position)
    {
      std::ostringstream ostr;
      ostr << "word list with name '" << e.name.in() << "' not found";
      description = ostr.str().c_str();
    }

    //
    // NewsGate::CategoryModeration::ExpressionParseError class
    //
    inline
    ExpressionParseError::ExpressionParseError(PyTypeObject *type,
                                               PyObject *args,
                                               PyObject *kwds)
      throw(El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance)
    {
    }

    inline
    ExpressionParseError::ExpressionParseError(
      const Moderation::Category::ExpressionParseError& e) throw(El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          ExpressionError(e.expression_index,
                          e.position,
                          e.description.in()),
          name(e.word_list_name.in())
    {
    }

    //
    // NewsGate::CategoryModeration::Cycle class
    //
    inline
    Cycle::Cycle(PyTypeObject *type, PyObject *args, PyObject *kwds)
      throw(El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance),
          id(0)
    {
    }

    inline
    Cycle::Cycle(const Moderation::Category::Cycle& e) throw(El::Exception)
        : El::Python::ObjectImpl(&Type::instance),
          id(e.id),
          name(e.name.in())
    {
    }

    //
    // NewsGate::CategoryModeration::ForbiddenOperation class
    //
    inline
    ForbiddenOperation::ForbiddenOperation(PyTypeObject *type,
                                           PyObject *args,
                                           PyObject *kwds)
      throw(El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance)
    {
    }

    inline
    ForbiddenOperation::ForbiddenOperation(
      const Moderation::Category::ForbiddenOperation& e) throw(El::Exception)
        : El::Python::ObjectImpl(&Type::instance)
    {
      const char* desc = strstr(e.description.in(), ": ");
      description = desc ? desc + 2 : e.description.in();
    }

    //
    // NewsGate::CategoryModeration::VersionMismatch class
    //
    inline
    VersionMismatch::VersionMismatch(PyTypeObject *type,
                                     PyObject *args,
                                     PyObject *kwds)
      throw(El::Exception)
        : El::Python::ObjectImpl(type ? type : &Type::instance)
    {
    }

    inline
    VersionMismatch::VersionMismatch(
      const Moderation::Category::VersionMismatch& e) throw(El::Exception)
        : El::Python::ObjectImpl(&Type::instance)
    {
      const char* desc = strstr(e.description.in(), ": ");
      description = desc ? desc + 2 : e.description.in();
    }
  }
}

#endif // _NEWSGATE_SERVER_FRONTENDS_MODERATION_CATEGORYMODERATION_HPP_
