/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/ChangeLog/CategoryChange.hpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#ifndef _NEWSGATE_SERVER_SERVICES_MODERATOR_CHANGELOG_CATEGORYCHANGE_HPP_
#define _NEWSGATE_SERVER_SERVICES_MODERATOR_CHANGELOG_CATEGORYCHANGE_HPP_

#include <stdint.h>

#include <string>
#include <vector>
#include <set>

#include <El/Exception.hpp>
#include <El/String/Manip.hpp>
#include <El/BinaryStream.hpp>

#include <Services/Moderator/ChangeLog/Change.hpp>
#include <Services/Moderator/Commons/CategoryManager.hpp>

namespace NewsGate
{
  namespace Moderation
  {
    namespace Category
    {
      struct LocaleChange
      {
        enum FieldChange
        {
          FC_LANG = 0x1,
          FC_COUNTRY = 0x2,
          FC_NAME = 0x4
        };

        uint64_t fields_change;

        uint16_t new_lang;
        uint16_t old_lang;

        uint16_t new_country;
        uint16_t old_country;

        std::string new_name;
        std::string old_name;

        LocaleChange(const LocaleDescriptor& new_loc,
                     const LocaleDescriptor& old_loc)
          throw(El::Exception);

        LocaleChange() throw();

        void details(std::ostream& ostr) const throw(El::Exception);
        
        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);
      };

      typedef std::vector<LocaleChange> LocaleChangeArray;
        
      struct WordListChange
      {
        enum FieldChange
        {
          FC_NAME = 0x1,
          FC_WORDS = 0x2,
          FC_DESCRIPTION = 0x4,
          FC_VERSION = 0x8
        };

        uint64_t fields_change;

        uint64_t new_version;
        uint64_t old_version;

        std::string new_name;
        std::string old_name;

        El::String::Set added_words;
        El::String::Set removed_words;

        std::string new_description;
        std::string old_description;

        WordListChange(const WordListDescriptor& new_wl,
                       const WordListDescriptor& old_wl)
          throw(El::Exception);

        WordListChange() throw();

        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);

        static void words_to_set(const char* words,
                                 El::String::Set& words_set)
          throw(El::Exception);

        void details(std::ostream& ostr) const throw(El::Exception);
        
      private:
        void find_words_diff() throw(El::Exception);
      };

      typedef std::vector<WordListChange> WordListChangeArray;
        
      struct ExpressionChange
      {
        enum FieldChange
        {
          FC_EXPRESSION = 0x1,
          FC_DESCRIPTION = 0x2
        };

        uint64_t fields_change;

        std::string new_expression;
        std::string old_expression;

        std::string new_description;
        std::string old_description;

        ExpressionChange(const ExpressionDescriptor& new_ex,
                         const ExpressionDescriptor& old_ex)
          throw(El::Exception);

        ExpressionChange() throw();

        void details(std::ostream& ostr) const throw(El::Exception);
        
        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);
      };

      typedef std::vector<ExpressionChange> ExpressionChangeArray;
        
      struct PathId
      {
        std::string path;
        uint64_t category_id;
        
        PathId(const char* cp, CategoryId ci) throw(El::Exception);
        PathId() throw() : category_id(0) {}
        
        bool operator<(const PathId& p2) const throw();
        
        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);
      };
      
      typedef std::set<PathId> PathIdSet;
      
      struct NameId
      {
        std::string name;
        uint64_t category_id;
        
        NameId(const char* cn, CategoryId ci) throw(El::Exception);
        NameId() throw() : category_id(0) {}

        bool operator<(const NameId& val) const throw();
        
        void write(El::BinaryOutStream& bstr) const throw(El::Exception);
        void read(El::BinaryInStream& bstr) throw(El::Exception);
      };
 
      typedef std::set<NameId> NameIdSet;

      struct CategoryChange : public Change
      {        
        enum FieldChange
        {
          FC_NAME = 0x1,
          FC_STATUS = 0x2,
          FC_SEARCHEABLE = 0x4,
          FC_LOCALES = 0x8,
          FC_WORD_LISTS = 0x10,
          FC_EXPRESSIONS = 0x20,
          FC_INCLUDED_MSG = 0x40,
          FC_EXCLUDED_MSG = 0x80,
          FC_PARENTS = 0x100,
          FC_CHILDREN = 0x200,
          FC_DESCRIPTION = 0x400,
          FC_VERSION = 0x800
        };

        enum ChangeSubType
        {
          CS_CAT_CREATED,
          CS_CAT_DELETED,
          CS_CAT_UPDATED,
          CS_WL_UPDATED,
          CS_RM_UPDATED,
          CS_UNKNOWN
        };

        struct LocaleDescriptorComp
        {
          bool operator()(const LocaleDescriptor& d1,
                          const LocaleDescriptor& d2) throw();
        };
        
        typedef std::set<LocaleDescriptor, LocaleDescriptorComp>
        LocaleDescriptorSet;

        struct LocaleDescriptorSerializer
        {
          void write(const LocaleDescriptor& ld, El::BinaryOutStream& bstr)
            const throw(El::Exception);
          
          void read(LocaleDescriptor& ld, El::BinaryInStream& bstr) const
            throw(El::Exception);
        };          

        struct WordListDescriptorComp
        {
          bool operator()(const WordListDescriptor& d1,
                          const WordListDescriptor& d2) throw();
        };
        
        typedef std::set<WordListDescriptor, WordListDescriptorComp>
        WordListDescriptorSet;
        
        struct WordListDescriptorSerializer
        {
          void write(const WordListDescriptor& wd, El::BinaryOutStream& bstr)
            const throw(El::Exception);
          
          void read(WordListDescriptor& wd, El::BinaryInStream& bstr) const
            throw(El::Exception);
        };          

        struct ExpressionDescriptorComp
        {
          bool operator()(const ExpressionDescriptor& e1,
                          const ExpressionDescriptor& e2) throw();
        };
        
        typedef std::set<ExpressionDescriptor, ExpressionDescriptorComp>
        ExpressionDescriptorSet;

        struct ExpressionDescriptorSerializer
        {
          void write(const ExpressionDescriptor& ed, El::BinaryOutStream& bstr)
            const throw(El::Exception);
          
          void read(ExpressionDescriptor& ed, El::BinaryInStream& bstr) const
            throw(El::Exception);
        };

        typedef std::set<uint64_t> MessageIdSet;
        typedef std::set<uint64_t> CategoryIdSet;

        uint64_t category_id;
        std::string category_path;

        uint64_t fields_change;
        uint32_t change_subtype;

        uint64_t new_version;
        uint64_t old_version;
        
        std::string new_name;
        std::string old_name;

        std::string new_description;
        std::string old_description;
        
        uint16_t new_status;
        uint16_t old_status;
        
        uint16_t new_searcheable;
        uint16_t old_searcheable;

        LocaleDescriptorSet added_locales;
        LocaleDescriptorSet removed_locales;
        LocaleChangeArray changed_locales;

        WordListDescriptorSet added_word_lists;
        WordListDescriptorSet removed_word_lists;
        WordListChangeArray changed_word_lists;

        ExpressionDescriptorSet added_expressions;
        ExpressionDescriptorSet removed_expressions;
        ExpressionChangeArray changed_expressions;

        MessageIdSet added_included_messages;
        MessageIdSet removed_included_messages;
        
        MessageIdSet added_excluded_messages;
        MessageIdSet removed_excluded_messages;

        PathIdSet added_parents;
        PathIdSet removed_parents;
        
        NameIdSet added_children;
        NameIdSet removed_children;

        CategoryChange(const CategoryDescriptor& new_cat,
                       const CategoryDescriptor& old_cat,
                       uint64_t mod_id,
                       const char* mod_name,
                       const char* ip_address)
          throw(El::Exception);

        CategoryChange() throw(El::Exception);

        virtual ~CategoryChange() throw() {}

        virtual Type type() const throw() { return CT_CATEGORY_CHANGE; }
        virtual uint32_t subtype() const throw() { return change_subtype; }
        
        virtual void write(El::BinaryOutStream& bstr) const
          throw(El::Exception);
        
        virtual void read(El::BinaryInStream& bstr) throw(El::Exception);
        virtual std::string url() const throw(El::Exception);
        
        virtual void summary(std::ostream& ostr) const
          throw(Exception, El::Exception);
        
        virtual void details(std::ostream& ostr) const
          throw(Exception, El::Exception);

        static const char* status(uint64_t val) throw();
        static const char* searcheable(uint64_t val) throw();
        
      private:
        
        void find_locales_diff() throw(El::Exception);
        void find_word_lists_diff() throw(El::Exception);
        void find_expressions_diff() throw(El::Exception);

        void find_rel_messages_diff(MessageIdSet& added_rel_messages,
                                    MessageIdSet& removed_rel_messages,
                                    unsigned long long fc)
          throw(El::Exception);
        
        void find_parent_diff() throw(El::Exception);
        void find_children_diff() throw(El::Exception);
        
        void version_summary(std::ostream& ostr, bool& write_comma) const
          throw(El::Exception);
        
        void version_details(std::ostream& ostr) const throw(El::Exception);

        void name_summary(std::ostream& ostr, bool& write_comma) const
          throw(El::Exception);
        
        void name_details(std::ostream& ostr) const throw(El::Exception);
        
        void status_summary(std::ostream& ostr, bool& write_comma) const
          throw(El::Exception);
        
        void status_details(std::ostream& ostr) const throw(El::Exception);
        
        void searcheable_summary(std::ostream& ostr, bool& write_comma) const
          throw(El::Exception);
        
        void searcheable_details(std::ostream& ostr) const
          throw(El::Exception);
        
        void description_summary(std::ostream& ostr, bool& write_comma) const
          throw(El::Exception);

        void description_details(std::ostream& ostr) const
          throw(El::Exception);

        void locales_summary(std::ostream& ostr, bool& write_comma) const
          throw(El::Exception);

        void locales_details(const LocaleChangeArray& locales,
                             std::ostream& ostr) const
          throw(El::Exception);

        void locales_details(const LocaleDescriptorSet& locales,
                             std::ostream& ostr) const
          throw(El::Exception);
        
        void locales_details(std::ostream& ostr) const throw(El::Exception);

        void word_lists_summary(std::ostream& ostr, bool& write_comma) const
          throw(El::Exception);

        void word_lists_details(std::ostream& ostr) const throw(El::Exception);

        void word_list_details(const WordListDescriptor& desc,
                               std::ostream& ostr) const
          throw(El::Exception);

        void expressions_summary(std::ostream& ostr, bool& write_comma) const
          throw(El::Exception);

        void expressions_details(std::ostream& ostr) const
          throw(El::Exception);

        void expressions_details(const ExpressionDescriptor& desc,
                                 std::ostream& ostr) const
          throw(El::Exception);

        void included_messages_summary(std::ostream& ostr,
                                       bool& write_comma) const
          throw(El::Exception);

        void included_messages_details(std::ostream& ostr) const
          throw(El::Exception);
        
        void excluded_messages_summary(std::ostream& ostr,
                                       bool& write_comma) const
          throw(El::Exception);

        void excluded_messages_details(std::ostream& ostr) const
          throw(El::Exception);
        
        void parents_summary(std::ostream& ostr, bool& write_comma) const
          throw(El::Exception);

        void parents_details(std::ostream& ostr) const throw(El::Exception);
        
        void children_summary(std::ostream& ostr, bool& write_comma) const
          throw(El::Exception);
        
        void children_details(std::ostream& ostr) const
          throw(El::Exception);

        typedef std::pair<uint64_t, const char*> ValueName;

        static const char* value_name(uint64_t val,
                                      const ValueName* value_names) throw();

      private:
        static ValueName statuses_[];
        static ValueName searcheables_[];
      };
      
      template<typename SEQ, typename SET>
      void seq_to_set(const SEQ& desc_seq, SET& desc_set)
        throw(El::Exception);

      template<>
      void seq_to_set<CategoryDescriptorSeq,PathIdSet>(
        const CategoryDescriptorSeq& desc_seq,
        PathIdSet& desc_set)
          throw(El::Exception);

      template<>
      void seq_to_set<CategoryDescriptorSeq,NameIdSet>(
        const CategoryDescriptorSeq& desc_seq,
        NameIdSet& desc_set)
        throw(El::Exception);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  namespace Moderation
  {
    namespace Category
    {

      //
      // CategoryChange class
      //
      inline
      const char*
      CategoryChange::status(uint64_t val) throw()
      {
        return value_name(val, statuses_);
      }
      
      inline
      const char*
      CategoryChange::searcheable(uint64_t val) throw()
      {
        return value_name(val, searcheables_);
      }
      
      //
      // LocaleDescriptorSerializer struct
      //
      inline
      void
      CategoryChange::LocaleDescriptorSerializer::write(
        const LocaleDescriptor& ld,
        El::BinaryOutStream& bstr) const
        throw(El::Exception)
      {
        bstr << (uint16_t)ld.lang << (uint16_t)ld.country << ld.name.in();
      }

      inline
      void
      CategoryChange::LocaleDescriptorSerializer::read(
        LocaleDescriptor& ld, El::BinaryInStream& bstr)
        const throw(El::Exception)
      {
        uint16_t lang = 0;
        uint16_t country = 0;
        std::string name;
        
        bstr >> lang >> country >> name;
        
        ld.lang = lang;
        ld.country = country;
        ld.name = CORBA::string_dup(name.c_str());
      }
      
      //
      // WordListDescriptorSerializer struct
      //
      inline
      void
      CategoryChange::WordListDescriptorSerializer::write(
        const WordListDescriptor& wd,
        El::BinaryOutStream& bstr) const
        throw(El::Exception)
      {
        bstr << wd.name.in() << wd.words.in() << (uint64_t)wd.version
             << (uint8_t)wd.updated << wd.description.in();
      }

      inline
      void
      CategoryChange::WordListDescriptorSerializer::read(
        WordListDescriptor& wd, El::BinaryInStream& bstr)
        const throw(El::Exception)
      {
        std::string name;
        std::string words;
        uint64_t version = 0;
        uint8_t updated = 0;
        std::string description;
        
        bstr >> name >> words >> version >> updated >> description;
        
        wd.name = CORBA::string_dup(name.c_str());
        wd.words = CORBA::string_dup(words.c_str());
        wd.version = version;
        wd.updated = updated;
        wd.description = CORBA::string_dup(description.c_str());
      }
      
      //
      // ExpressionDescriptorSerializer struct
      //
      inline
      void
      CategoryChange::ExpressionDescriptorSerializer::write(
        const ExpressionDescriptor& ed,
        El::BinaryOutStream& bstr) const
        throw(El::Exception)
      {
        bstr << ed.expression.in() << ed.description.in();
      }

      inline
      void
      CategoryChange::ExpressionDescriptorSerializer::read(
        ExpressionDescriptor& ed, El::BinaryInStream& bstr)
        const throw(El::Exception)
      {
        std::string expression;
        std::string description;
        bstr >> expression >> description;
        
        ed.expression = CORBA::string_dup(expression.c_str());
        ed.description = CORBA::string_dup(description.c_str());
      }
      
      //
      // LocaleDescriptorComp struct
      //
      inline
      bool
      CategoryChange::LocaleDescriptorComp::operator()(
        const LocaleDescriptor& d1,
        const LocaleDescriptor& d2)
        throw()
      {
        if(d1.lang < d2.lang) return true;
        if(d1.lang > d2.lang) return false;
        return d1.country < d2.country;
      }

      //
      // WordListDescriptorComp struct
      //
      inline
      bool
      CategoryChange::WordListDescriptorComp::operator()(
        const WordListDescriptor& d1,
        const WordListDescriptor& d2)
        throw()
      {
        return strcmp(d1.name.in(), d2.name.in()) < 0;
      }

      //
      // ExpressionDescriptorComp struct
      //
      inline
      bool
      CategoryChange::ExpressionDescriptorComp::operator()(
        const ExpressionDescriptor& e1,
        const ExpressionDescriptor& e2)
        throw()
      {
        return strcmp(e1.expression.in(), e2.expression.in()) < 0;
      }

      //
      // PathId struct
      //
      inline
      PathId::PathId(const char* cp, CategoryId ci) throw(El::Exception)
          : path(cp),
            category_id(ci)
      {
      }
      
      inline
      void
      PathId::write(El::BinaryOutStream& bstr) const throw(El::Exception)
      {
        bstr << path << category_id;
      }

      inline
      void
      PathId::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> path >> category_id;
      }
      
      inline
      bool
      PathId::operator<(const PathId& val) const throw()
      {
        int d = strcmp(path.c_str(), val.path.c_str());        
        return d < 0 || (d == 0 && category_id < val.category_id);
      }

      //
      // NameId struct
      //
      inline
      NameId::NameId(const char* cn, CategoryId ci) throw(El::Exception)
          : name(cn),
            category_id(ci)
      {
      }

      inline
      void
      NameId::write(El::BinaryOutStream& bstr) const throw(El::Exception)
      {
        bstr << name << category_id;
      }

      inline
      void
      NameId::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> name >> category_id;
      }
      
      inline
      bool
      NameId::operator<(const NameId& val) const throw()
      {
        int d = strcmp(name.c_str(), val.name.c_str());        
        return d < 0 || (d == 0 && category_id < val.category_id);
      }

      //
      // seq_to_set function
      //
      template<typename SEQ, typename SET>
      void
      seq_to_set(const SEQ& desc_seq, SET& desc_set) throw(El::Exception)
      {
        for(size_t i = 0; i < desc_seq.length(); ++i)
        {
          desc_set.insert(desc_seq[i]);
        }
      }

    }
  }
}

#endif // _NEWSGATE_SERVER_SERVICES_MODERATOR_CHANGELOG_CATEGORYCHANGE_HPP_
