/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Services/Moderator/ChangeLog/CategoryChange.cpp
 * @author Karen Aroutiounov
 * $Id: $
 */

#include <El/CORBA/Corba.hpp>

#include <string>

#include <El/Exception.hpp>
#include <El/String/Manip.hpp>
#include <El/String/ListParser.hpp>
#include <El/Lang.hpp>
#include <El/Country.hpp>

#include <Commons/Message/Message.hpp>

#include "CategoryChange.hpp"

namespace NewsGate
{
  namespace Moderation
  {    
    namespace Category
    {
      //
      // CategoryChange struct
      //

      CategoryChange::ValueName
      CategoryChange::statuses_[] =
      {
        ValueName(CS_ENABLED, "Enabled"),
        ValueName(CS_DISABLED, "Disabled"),
        ValueName(0, (const char*)0)
      };
      
      CategoryChange::ValueName
      CategoryChange::searcheables_[] =
      {
        ValueName(CR_YES, "Yes"),
        ValueName(CR_NO, "No"),
        ValueName(0, (const char*)0)
      };      

      CategoryChange::CategoryChange() throw(El::Exception)
          : Change(),
            category_id(0),
            fields_change(0),
            change_subtype(CS_UNKNOWN),
            new_version(0),
            old_version(0),
            new_status(CS_COUNT),
            old_status(CS_COUNT),
            new_searcheable(CR_COUNT),
            old_searcheable(CR_COUNT)
      {
      }

      CategoryChange::CategoryChange(const CategoryDescriptor& new_cat,
                                     const CategoryDescriptor& old_cat,
                                     uint64_t mod_id,
                                     const char* mod_name,
                                     const char* ip_address)
        throw(El::Exception)
          : Change(mod_id, mod_name, ip_address),
            category_id(new_cat.id ? new_cat.id : old_cat.id),
            fields_change(0),
            change_subtype(CS_UNKNOWN),
            new_version(0),
            old_version(0),
            new_status(CS_COUNT),
            old_status(CS_COUNT),
            new_searcheable(CR_COUNT),
            old_searcheable(CR_COUNT)
      {
        assert((new_cat.id || old_cat.id) &&
               (!new_cat.id || !old_cat.id || new_cat.id == old_cat.id));

        seq_to_set(new_cat.word_lists, added_word_lists);        
        seq_to_set(old_cat.word_lists, removed_word_lists);
        find_word_lists_diff();

        seq_to_set(new_cat.included_messages, added_included_messages);
        seq_to_set(old_cat.included_messages, removed_included_messages);

        find_rel_messages_diff(added_included_messages,
                               removed_included_messages,
                               FC_INCLUDED_MSG);
        
        seq_to_set(new_cat.excluded_messages, added_excluded_messages);
        seq_to_set(old_cat.excluded_messages, removed_excluded_messages);
        
        find_rel_messages_diff(added_excluded_messages,
                               removed_excluded_messages,
                               FC_EXCLUDED_MSG);
        
        if(new_cat.version != old_cat.version)
        {
          old_version = old_cat.version;
          new_version = new_cat.version;
          fields_change |= FC_VERSION;

          change_subtype = !old_version ? CS_CAT_CREATED :
            (new_version ? CS_CAT_UPDATED : CS_CAT_DELETED);
        }
        else if(!changed_word_lists.empty())
        {
          change_subtype = CS_WL_UPDATED;
        }
        else if(!added_included_messages.empty() ||
                !removed_included_messages.empty() ||
                !added_excluded_messages.empty() ||
                !removed_excluded_messages.empty())
        {
          change_subtype = CS_RM_UPDATED;
        }
        
        if(strcmp(new_cat.name.in(), old_cat.name.in()))
        {
          old_name = old_cat.name.in();
          new_name = new_cat.name.in();
          fields_change |= FC_NAME;
        }        
        
        if(strcmp(new_cat.description.in(), old_cat.description.in()))
        {
          old_description = old_cat.description.in();
          new_description = new_cat.description.in();
          fields_change |= FC_DESCRIPTION;
        }        

        if(new_cat.status != old_cat.status)
        {
          old_status = old_cat.status;
          new_status = new_cat.status;
          fields_change |= FC_STATUS;
        }
        
        if(new_cat.searcheable != old_cat.searcheable)
        {
          old_searcheable = old_cat.searcheable;
          new_searcheable = new_cat.searcheable;
          fields_change |= FC_SEARCHEABLE;
        }

        seq_to_set(new_cat.locales, added_locales);        
        seq_to_set(old_cat.locales, removed_locales);
        find_locales_diff();

        seq_to_set(new_cat.expressions, added_expressions);
        seq_to_set(old_cat.expressions, removed_expressions);
        find_expressions_diff();

        seq_to_set(new_cat.parents, added_parents);
        seq_to_set(old_cat.parents, removed_parents);

        if(change_subtype == CS_CAT_DELETED)
        {
          if(!removed_parents.empty())
          {
            category_path = removed_parents.begin()->path;
          }
          
          category_path += old_cat.name.in();
        }
        else
        {
          if(!added_parents.empty())
          {
            category_path = added_parents.begin()->path;
          }
          
          category_path += new_cat.name.in();
        }
        
        find_parent_diff();

        seq_to_set(new_cat.children, added_children);
        seq_to_set(old_cat.children, removed_children);        
        find_children_diff();
      }

      std::string
      CategoryChange::url() const throw(El::Exception)
      {
        std::ostringstream ostr;
        ostr << "/psp/category/update?c=" << category_id;
        return ostr.str();
      }
      
      void
      CategoryChange::details(std::ostream& ostr) const
        throw(Exception, El::Exception)
      {
        ostr << "Category '";
            
        El::String::Manip::xml_encode(category_path.c_str(),
                                      ostr,
                                      El::String::Manip::XE_TEXT_ENCODING |
                                      El::String::Manip::XE_PRESERVE_UTF8);
        
        ostr  << "' (" << category_id << ") ";
        
        switch(change_subtype)
        {
        case CS_CAT_DELETED:
          {
            ostr << "deleted: ";
            break;
          }
        case CS_CAT_CREATED:
          {            
            ostr << "created: ";
            break;
          }
        case CS_CAT_UPDATED:
          {
            ostr << "updated: ";
            break;
          }
        case CS_WL_UPDATED:
          {
            ostr << "word list updated: ";
            break;
          }
        case CS_RM_UPDATED:
          {
            ostr << "messages updated: ";
            break;
          }
        default:
          {
            ostr << "not changed";
            break;
          }
        }
        
        version_details(ostr);
        name_details(ostr);
        status_details(ostr);
        searcheable_details(ostr);
        description_details(ostr);
        locales_details(ostr);
        word_lists_details(ostr);
        expressions_details(ostr);
        included_messages_details(ostr);
        excluded_messages_details(ostr);
        parents_details(ostr);
        children_details(ostr);
      }
      
      void
      CategoryChange::status_details(std::ostream& ostr) const
        throw(El::Exception)
      {
        if(fields_change & FC_STATUS)
        {
          ostr << "<br>\nstatus ";
          
          if(change_subtype != CS_CAT_DELETED)
          {
            ostr << status(new_status);
          }

          if(change_subtype == CS_CAT_UPDATED)
          {
            ostr << " &lt;- ";
          }
          
          if(change_subtype != CS_CAT_CREATED)
          {
            ostr << status(old_status);
          }          
        }        
      }
      
      void
      CategoryChange::write(El::BinaryOutStream& bstr) const
        throw(El::Exception)
      {
        Change::write(bstr);

        bstr << (uint32_t)1 << category_id << category_path << change_subtype
             << fields_change;
        
        if(fields_change & FC_VERSION)
        {
          bstr << old_version << new_version;
        }
        
        if(fields_change & FC_NAME)
        {
          bstr << old_name << new_name;
        }        
        
        if(fields_change & FC_DESCRIPTION)
        {
          bstr << old_description << new_description;
        }        

        if(fields_change & FC_STATUS)
        {
          bstr << old_status << new_status;
        }
        
        if(fields_change & FC_SEARCHEABLE)
        {
          bstr << old_searcheable << new_searcheable;
        }

        if(fields_change & FC_LOCALES)
        {
          bstr.write_set(added_locales, LocaleDescriptorSerializer());
          bstr.write_set(removed_locales, LocaleDescriptorSerializer());
          bstr.write_array(changed_locales);
        }
        
        if(fields_change & FC_WORD_LISTS)
        {          
          bstr.write_set(added_word_lists, WordListDescriptorSerializer());
          bstr.write_set(removed_word_lists, WordListDescriptorSerializer());
          bstr.write_array(changed_word_lists);
        }

        if(fields_change & FC_EXPRESSIONS)
        {
          bstr.write_set(added_expressions, ExpressionDescriptorSerializer());
          
          bstr.write_set(removed_expressions,
                         ExpressionDescriptorSerializer());
          
          bstr.write_array(changed_expressions);
        }

        if(fields_change & FC_INCLUDED_MSG)
        {
          bstr.write_set(added_included_messages);
          bstr.write_set(removed_included_messages);
        }

        if(fields_change & FC_EXCLUDED_MSG)
        {
          bstr.write_set(added_excluded_messages);
          bstr.write_set(removed_excluded_messages);
        }        

        if(fields_change & FC_PARENTS)
        {
          bstr.write_set(added_parents);
          bstr.write_set(removed_parents);
        }
        
        if(fields_change & FC_CHILDREN)
        {
          bstr.write_set(added_children);
          bstr.write_set(removed_children);
        }
      }
      
      void
      CategoryChange::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        Change::read(bstr);
        
        uint32_t version = 0;
        bstr >> version;

        if(version != 1)
        {
          std::ostringstream ostr;
          ostr << "::NewsGate::Moderation::Category::CategoryChange::read: "
            "unexpected version " << version;
            
          throw Exception(ostr.str());
        }

        bstr >> category_id >> category_path >> change_subtype
             >> fields_change;

        if(fields_change & FC_VERSION)
        {
          bstr >> old_version >> new_version;
        }
        else
        {
          old_version = 0;
          new_version = 0;
        }
        
        if(fields_change & FC_NAME)
        {
          bstr >> old_name >> new_name;
        }
        else
        {
          old_name.clear();
          new_name.clear();
        }
        
        if(fields_change & FC_DESCRIPTION)
        {
          bstr >> old_description >> new_description;
        }
        else
        {
          old_description.clear();
          new_description.clear();
        }

        if(fields_change & FC_STATUS)
        {
          bstr >> old_status >> new_status;
        }
        else
        {
          old_status = CS_COUNT;
          new_status = CS_COUNT;
        }
        
        if(fields_change & FC_SEARCHEABLE)
        {
          bstr >> old_searcheable >> new_searcheable;
        }
        else
        {
          old_searcheable = CR_COUNT;
          new_searcheable = CR_COUNT;
        }

        if(fields_change & FC_LOCALES)
        {
          bstr.read_set(added_locales, LocaleDescriptorSerializer());
          bstr.read_set(removed_locales, LocaleDescriptorSerializer());
          bstr.read_array(changed_locales);
        }
        else
        {
          added_locales.clear();
          removed_locales.clear();
          changed_locales.clear();
        }
        
        if(fields_change & FC_WORD_LISTS)
        {          
          bstr.read_set(added_word_lists, WordListDescriptorSerializer());
          bstr.read_set(removed_word_lists, WordListDescriptorSerializer());
          bstr.read_array(changed_word_lists);
        }
        else
        {
          added_word_lists.clear();
          removed_word_lists.clear();
          changed_word_lists.clear();
        }
          
        if(fields_change & FC_EXPRESSIONS)
        {
          bstr.read_set(added_expressions, ExpressionDescriptorSerializer());
          bstr.read_set(removed_expressions, ExpressionDescriptorSerializer());
          bstr.read_array(changed_expressions);
        }
        else
        {
          added_expressions.clear();
          removed_expressions.clear();
          changed_expressions.clear();
        }

        if(fields_change & FC_INCLUDED_MSG)
        {
          bstr.read_set(added_included_messages);
          bstr.read_set(removed_included_messages);
        }
        else
        {
          added_included_messages.clear();
          removed_included_messages.clear();
        }

        if(fields_change & FC_EXCLUDED_MSG)
        {
          bstr.read_set(added_excluded_messages);
          bstr.read_set(removed_excluded_messages);
        }
        else
        {
          added_excluded_messages.clear();
          removed_excluded_messages.clear();
        }

        if(fields_change & FC_PARENTS)
        {
          bstr.read_set(added_parents);
          bstr.read_set(removed_parents);
        }
        else
        {
          added_parents.clear();
          removed_parents.clear();
        }
        
        if(fields_change & FC_CHILDREN)
        {
          bstr.read_set(added_children);
          bstr.read_set(removed_children);
        }
        else
        {
          added_children.clear();
          removed_children.clear();
        }
      }

      void
      CategoryChange::summary(std::ostream& ostr) const
        throw(Exception, El::Exception)
      {
        bool written = false;

        ostr << "Category '" << category_path
             << "' (" << category_id << ") ";
        
        switch(change_subtype)
        {
        case CS_CAT_DELETED:
          {
            ostr << "deleted: ";
            break;
          }
        case CS_CAT_CREATED:
          {            
            ostr << "created: ";
            break;
          }
        case CS_CAT_UPDATED:
          {
            ostr << "updated: ";
            break;
          }
        case CS_WL_UPDATED:
          {
            ostr << "word list updated: ";
            break;
          }
        case CS_RM_UPDATED:
          {
            ostr << "messages updated: ";
            break;
          }
        default:
          {
            ostr << "not changed";
            break;
          }
        }
        
        version_summary(ostr, written);
        name_summary(ostr, written);
        status_summary(ostr, written);
        searcheable_summary(ostr, written);
        description_summary(ostr, written);
        locales_summary(ostr, written);
        word_lists_summary(ostr, written);
        expressions_summary(ostr, written);
        included_messages_summary(ostr, written);
        excluded_messages_summary(ostr, written);
        parents_summary(ostr, written);
        children_summary(ostr, written);
      }

      void
      CategoryChange::version_details(std::ostream& ostr) const
        throw(El::Exception)
      {
        if(fields_change & FC_VERSION)
        {
          ostr << "<br>\nversion ";

          if(change_subtype != CS_CAT_DELETED)
          {
            ostr << new_version;
          }

          if(change_subtype == CS_CAT_UPDATED)
          {
            ostr << " &lt;- ";
          }

          if(change_subtype != CS_CAT_CREATED)
          {
            ostr << old_version;
          }           
        }
      }
        
      void
      CategoryChange::version_summary(std::ostream& ostr,
                                      bool& write_comma) const
        throw(El::Exception)
      {
        if(fields_change & FC_VERSION)
        {
          if(write_comma)
          {
            ostr << ", ";
          }
          
          ostr << "version ";

          if(change_subtype != CS_CAT_DELETED)
          {
            ostr << new_version;
          }

          if(change_subtype == CS_CAT_UPDATED)
          {
            ostr << " <- ";
          }

          if(change_subtype != CS_CAT_CREATED)
          {
            ostr << old_version;
          }           
          
          write_comma = true;
        }
      }
        
      void
      CategoryChange::name_details(std::ostream& ostr) const
        throw(El::Exception)
      {
        if(fields_change & FC_NAME)
        {
          ostr << "<br>\nname ";

          if(change_subtype != CS_CAT_DELETED)
          {
            ostr << "'";

            El::String::Manip::xml_encode(new_name.c_str(),
                                          ostr,
                                          El::String::Manip::XE_TEXT_ENCODING |
                                          El::String::Manip::XE_PRESERVE_UTF8);
            
            ostr << "'";
          }
          
          if(change_subtype == CS_CAT_UPDATED)
          {
            ostr << " &lt;- ";
          }

          if(change_subtype != CS_CAT_CREATED)
          {
            ostr << "'";

            El::String::Manip::xml_encode(old_name.c_str(),
                                          ostr,
                                          El::String::Manip::XE_TEXT_ENCODING |
                                          El::String::Manip::XE_PRESERVE_UTF8);
            
            ostr << "'";
          }          
        }        
      }
      
      void
      CategoryChange::name_summary(std::ostream& ostr, bool& write_comma) const
        throw(El::Exception)
      {
        if(fields_change & FC_NAME)
        {
          if(write_comma)
          {
            ostr << ", ";
          }

          ostr << "name ";

          if(change_subtype != CS_CAT_DELETED)
          {
            ostr << "'" << new_name << "'";
          }
          
          if(change_subtype == CS_CAT_UPDATED)
          {
            ostr << " <- ";
          }

          if(change_subtype != CS_CAT_CREATED)
          {
            ostr << "'" << old_name << "'";
          }
          
          write_comma = true;
        }        
      }
      
      void
      CategoryChange::status_summary(std::ostream& ostr,
                                     bool& write_comma) const
        throw(El::Exception)
      {
        if(fields_change & FC_STATUS)
        {
          if(write_comma)
          {
            ostr << ", ";
          }

          ostr << "status ";
          
          if(change_subtype != CS_CAT_DELETED)
          {
            ostr << status(new_status);
          }

          if(change_subtype == CS_CAT_UPDATED)
          {
            ostr << " <- ";
          }
          
          if(change_subtype != CS_CAT_CREATED)
          {
            ostr << status(old_status);
          }
          
          write_comma = true;
        }
      }
      
      void
      CategoryChange::searcheable_details(std::ostream& ostr) const
        throw(El::Exception)
      {
        if(fields_change & FC_SEARCHEABLE)
        {
          ostr << "<br>\nsearcheable ";

          if(change_subtype != CS_CAT_DELETED)
          {
            ostr << searcheable(new_searcheable);
          }

          if(change_subtype == CS_CAT_UPDATED)
          {
            ostr << " &lt;- ";
          }
          
          if(change_subtype != CS_CAT_CREATED)
          {
            ostr << searcheable(old_searcheable);
          }          
        }
      }

      void
      CategoryChange::searcheable_summary(std::ostream& ostr,
                                          bool& write_comma) const
        throw(El::Exception)
      {
        if(fields_change & FC_SEARCHEABLE)
        {
          if(write_comma)
          {
            ostr << ", ";
          }

          ostr << "searcheable ";

          if(change_subtype != CS_CAT_DELETED)
          {
            ostr << searcheable(new_searcheable);
          }

          if(change_subtype == CS_CAT_UPDATED)
          {
            ostr << " <- ";
          }
          
          if(change_subtype != CS_CAT_CREATED)
          {
            ostr << searcheable(old_searcheable);
          }
          
          write_comma = true;
        }
      }

      void
      CategoryChange::description_details(std::ostream& ostr) const
        throw(El::Exception)
      {
        if(fields_change & FC_DESCRIPTION)
        {
          ostr << "<br>\ndescription ";
          
          if(change_subtype != CS_CAT_DELETED)
          {
            ostr << "'";

            El::String::Manip::xml_encode(new_description.c_str(),
                                          ostr,
                                          El::String::Manip::XE_TEXT_ENCODING |
                                          El::String::Manip::XE_PRESERVE_UTF8);
            
            ostr << "'";
          }

          if(change_subtype == CS_CAT_UPDATED)
          {
            ostr << " &lt;- ";
          }
          
          if(change_subtype != CS_CAT_CREATED)
          {
            ostr << "'";
            
            El::String::Manip::xml_encode(old_description.c_str(),
                                          ostr,
                                          El::String::Manip::XE_TEXT_ENCODING |
                                          El::String::Manip::XE_PRESERVE_UTF8);
            
            ostr << "'";
          }
        }        
      }
      
      void
      CategoryChange::description_summary(std::ostream& ostr,
                                          bool& write_comma) const
        throw(El::Exception)
      {
        if(fields_change & FC_DESCRIPTION)
        {
          if(write_comma)
          {
            ostr << ", ";
          }

          ostr << "description ";
          
          if(change_subtype != CS_CAT_DELETED)
          {
            ostr << "'" << new_description << "'";
          }

          if(change_subtype == CS_CAT_UPDATED)
          {
            ostr << " <- ";
          }
          
          if(change_subtype != CS_CAT_CREATED)
          {
            ostr << "'" << old_description << "'";
          }
          
          write_comma = true;
        }        
      }
      
      void
      CategoryChange::children_summary(std::ostream& ostr,
                                       bool& write_comma) const
        throw(El::Exception)
      {
        if(fields_change & FC_CHILDREN)
        {
          if(write_comma)
          {
            ostr << ", ";
          }

          ostr << "children";

          if(!added_children.empty())
          {
            ostr << " added " << added_children.size();
          }
          
          if(!removed_children.empty())
          {
            ostr << " removed " << removed_children.size();
          }

          write_comma = true;
        }
      }
        
      void
      CategoryChange::children_details(std::ostream& ostr) const
        throw(El::Exception)
      {
        if(fields_change & FC_CHILDREN)
        {
          ostr << "<br>\nchildren";

          if(!added_children.empty())
          {
            ostr << "<br>\n&nbsp;added";
              
            for(NameIdSet::const_iterator i(added_children.begin()),
                  e(added_children.end()); i != e; ++i)
            {
              const NameId& p = *i;
              ostr << "<br>\n&nbsp; '";

              El::String::Manip::xml_encode(
                p.name.c_str(),
                ostr,
                El::String::Manip::XE_TEXT_ENCODING |
                El::String::Manip::XE_PRESERVE_UTF8);
              
              ostr << "' (" << p.category_id << ")";
            }
          }
          
          if(!removed_children.empty())
          {
            ostr << "<br>\n&nbsp;removed";
              
            for(NameIdSet::const_iterator i(removed_children.begin()),
                  e(removed_children.end()); i != e; ++i)
            {
              const NameId& p = *i;
              ostr << "<br>\n&nbsp; '";

              El::String::Manip::xml_encode(
                p.name.c_str(),
                ostr,
                El::String::Manip::XE_TEXT_ENCODING |
                El::String::Manip::XE_PRESERVE_UTF8);
              
              ostr << "' (" << p.category_id << ")";
            }
          }
        }
      }
        
      void
      CategoryChange::parents_summary(std::ostream& ostr,
                                      bool& write_comma) const
        throw(El::Exception)
      {
        if(fields_change & FC_PARENTS)
        {
          if(write_comma)
          {
            ostr << ", ";
          }

          ostr << "parents";

          if(!added_parents.empty())
          {
            CategoryIdSet ids;
              
            for(PathIdSet::const_iterator i(added_parents.begin()),
                  e(added_parents.end()); i != e; ++i)
            {
              ids.insert(i->category_id);
            }

            ostr << " added " << ids.size();
          }
          
          if(!removed_parents.empty())
          {
            CategoryIdSet ids;
              
            for(PathIdSet::const_iterator i(removed_parents.begin()),
                  e(removed_parents.end()); i != e; ++i)
            {
              ids.insert(i->category_id);
            }

            ostr << " removed " << ids.size();
          }

          write_comma = true;
        }        
      }
        
      void
      CategoryChange::parents_details(std::ostream& ostr) const
        throw(El::Exception)
      {
        if(fields_change & FC_PARENTS)
        {
          ostr << "<br>\nparents";

          if(!added_parents.empty())
          {
            ostr << "<br>\n&nbsp;added";
              
            for(PathIdSet::const_iterator i(added_parents.begin()),
                  e(added_parents.end()); i != e; ++i)
            {
              const PathId& p = *i;
              ostr << "<br>\n&nbsp; '";

              El::String::Manip::xml_encode(
                p.path.c_str(),
                ostr,
                El::String::Manip::XE_TEXT_ENCODING |
                El::String::Manip::XE_PRESERVE_UTF8);
              
              ostr << "' (" << p.category_id << ")";
            }
          }
          
          if(!removed_parents.empty())
          {
            ostr << "<br>\n&nbsp;removed";
              
            for(PathIdSet::const_iterator i(removed_parents.begin()),
                  e(removed_parents.end()); i != e; ++i)
            {
              const PathId& p = *i;
              ostr << "<br>\n&nbsp; '";

              El::String::Manip::xml_encode(
                p.path.c_str(),
                ostr,
                El::String::Manip::XE_TEXT_ENCODING |
                El::String::Manip::XE_PRESERVE_UTF8);
              
              ostr << "' (" << p.category_id << ")";
            }
          }
        }        
      }
        
      void
      CategoryChange::excluded_messages_summary(std::ostream& ostr,
                                                bool& write_comma) const
        throw(El::Exception)
      {
        if(fields_change & FC_EXCLUDED_MSG)
        {
          if(write_comma)
          {
            ostr << ", ";
          }

          ostr << "excluded messages";
          
          if(!added_excluded_messages.empty())
          {
            ostr << " added " << added_excluded_messages.size();
          }

          if(!removed_excluded_messages.empty())
          {
            ostr << " removed " << removed_excluded_messages.size();
          }

          write_comma = true;
        }
      }
      
      void
      CategoryChange::included_messages_summary(std::ostream& ostr,
                                                bool& write_comma) const
        throw(El::Exception)
      {
        if(fields_change & FC_INCLUDED_MSG)
        {
          if(write_comma)
          {
            ostr << ", ";
          }          

          ostr << "included messages";
          
          if(!added_included_messages.empty())
          {
            ostr << " added " << added_included_messages.size();
          }

          if(!removed_included_messages.empty())
          {
            ostr << " removed " << removed_included_messages.size();
          }

          write_comma = true;
        }
      }
      
      void
      CategoryChange::included_messages_details(std::ostream& ostr) const
        throw(El::Exception)
      {
        if(fields_change & FC_INCLUDED_MSG)
        {
          ostr << "<br>\nincluded messages";
          
          if(!added_included_messages.empty())
          {
            ostr << "<br>\n&nbsp;added";

            for(MessageIdSet::const_iterator
                  i(added_included_messages.begin()),
                  e(added_included_messages.end()); i != e; ++i)
            {              
              ostr << "<br>\n&nbsp; " << Message::Id(*i).string();
            }            
          }

          if(!removed_included_messages.empty())
          {
            ostr << "<br>\n&nbsp;removed";

            for(MessageIdSet::const_iterator
                  i(removed_included_messages.begin()),
                  e(removed_included_messages.end()); i != e; ++i)
            {              
              ostr << "<br>\n&nbsp; " << Message::Id(*i).string();
            }            
          }
        }
      }
      
      void
      CategoryChange::excluded_messages_details(std::ostream& ostr) const
        throw(El::Exception)
      {
        if(fields_change & FC_EXCLUDED_MSG)
        {
          ostr << "<br>\nexcluded messages";
          
          if(!added_excluded_messages.empty())
          {
            ostr << "<br>\n&nbsp;added";

            for(MessageIdSet::const_iterator
                  i(added_excluded_messages.begin()),
                  e(added_excluded_messages.end()); i != e; ++i)
            {              
              ostr << "<br>\n&nbsp; " << Message::Id(*i).string();
            }            
          }

          if(!removed_excluded_messages.empty())
          {
            ostr << "<br>\n&nbsp;removed";

            for(MessageIdSet::const_iterator
                  i(removed_excluded_messages.begin()),
                  e(removed_excluded_messages.end()); i != e; ++i)
            {              
              ostr << "<br>\n&nbsp; " << Message::Id(*i).string();
            }            
          }
        }
      }
      
      void
      CategoryChange::expressions_summary(std::ostream& ostr,
                                          bool& write_comma) const
        throw(El::Exception)
      {
        if(fields_change & FC_EXPRESSIONS)
        {
          if(write_comma)
          {
            ostr << ", ";
          }          

          ostr << "expressions";
          
          if(!added_expressions.empty())
          {
            ostr << " added " << added_expressions.size();
          }

          if(!removed_expressions.empty())
          {
            ostr << " removed " << removed_expressions.size();
          }

          if(!changed_expressions.empty())
          {
            ostr << " changed " << changed_expressions.size();
          }

          write_comma = true;
        }
      }
      
      void
      CategoryChange::expressions_details(const ExpressionDescriptor& desc,
                                          std::ostream& ostr) const
        throw(El::Exception)
      {
        ostr << "<br>\n&nbsp;expression<br>\n&nbsp;";

        El::String::Manip::xml_encode(desc.expression.in(),
                                      ostr,
                                      El::String::Manip::XE_TEXT_ENCODING |
                                      El::String::Manip::XE_PRESERVE_UTF8);

        ostr << "<br>\n&nbsp;description '";

        El::String::Manip::xml_encode(desc.description.in(),
                                      ostr,
                                      El::String::Manip::XE_TEXT_ENCODING |
                                      El::String::Manip::XE_PRESERVE_UTF8);
        ostr << "'";
      }
      
      void
      CategoryChange::expressions_details(std::ostream& ostr) const
        throw(El::Exception)
      {
        if(fields_change & FC_EXPRESSIONS)
        {
          ostr << "<br>\nexpressions";          
          
          if(!added_expressions.empty())
          {
            for(ExpressionDescriptorSet::const_iterator
                  i(added_expressions.begin()), e(added_expressions.end());
                i != e; ++i)
            {
              ostr << "<br>\n&nbsp;added";
              expressions_details(*i, ostr);
            }            
          }

          if(!removed_expressions.empty())
          {
            for(ExpressionDescriptorSet::const_iterator
                  i(removed_expressions.begin()), e(removed_expressions.end());
                i != e; ++i)
            {
              ostr << "<br>\n&nbsp;removed";
              expressions_details(*i, ostr);
            }            
          }

          if(!changed_expressions.empty())
          {
            for(ExpressionChangeArray::const_iterator
                  i(changed_expressions.begin()), e(changed_expressions.end());
                i != e; ++i)
            {
              ostr << "<br>\n&nbsp;changed";
              i->details(ostr);
            }
          }
        }
        
      }
      
      void
      CategoryChange::word_lists_summary(std::ostream& ostr,
                                         bool& write_comma) const
        throw(El::Exception)
      {
        if(fields_change & FC_WORD_LISTS)
        {
          if(write_comma)
          {
            ostr << ", ";
          }          

          ostr << "word lists";
          
          if(!added_word_lists.empty())
          {
            size_t words = 0;
            
            for(WordListDescriptorSet::const_iterator
                  i(added_word_lists.begin()), e(added_word_lists.end());
                i != e; ++i)
            {
              El::String::Set words_set;
              WordListChange::words_to_set(i->words.in(), words_set);
              words += words_set.size();
            }
            
            ostr << " added " << added_word_lists.size() << "/" << words;
          }

          if(!removed_word_lists.empty())
          {
            size_t words = 0;
            
            for(WordListDescriptorSet::const_iterator
                  i(removed_word_lists.begin()), e(removed_word_lists.end());
                i != e; ++i)
            {
              El::String::Set words_set;
              WordListChange::words_to_set(i->words.in(), words_set);
              words += words_set.size();
            }
            
            ostr << " removed " << removed_word_lists.size() << "/" << words;
          }

          if(!changed_word_lists.empty())
          {
            size_t added_words = 0;
            size_t removed_words = 0;

            for(WordListChangeArray::const_iterator
                  i(changed_word_lists.begin()), e(changed_word_lists.end());
                i != e; ++i)
            {
              added_words += i->added_words.size();
              removed_words += i->removed_words.size();
            }
                  
            ostr << " changed " << changed_word_lists.size() << "/"
                 << added_words << "/" << removed_words;
          }

          write_comma = true;
        }
      }

              
      void
      CategoryChange::word_list_details(const WordListDescriptor& desc,
                                        std::ostream& ostr) const
        throw(El::Exception)
      {
        ostr << "<br>\n&nbsp;name '";

        El::String::Manip::xml_encode(desc.name.in(),
                                      ostr,
                                      El::String::Manip::XE_TEXT_ENCODING |
                                      El::String::Manip::XE_PRESERVE_UTF8);

        ostr << "'<br>\n&nbsp;version " << desc.version
             << "<br>\n&nbsp;description '";

        El::String::Manip::xml_encode(desc.description.in(),
                                      ostr,
                                      El::String::Manip::XE_TEXT_ENCODING |
                                      El::String::Manip::XE_PRESERVE_UTF8);
        
        ostr << "'<br>\n&nbsp;words";
        
        El::String::ListParser parser(desc.words.in(), "\n\r");

        const char* item = 0;

        while((item = parser.next_item()) != 0)
        {
          std::string word;
          El::String::Manip::trim(item, word);

          if(!word.empty())
          {
            ostr << "<br>\n&nbsp; ";
            
            El::String::Manip::xml_encode(word.c_str(),
                                          ostr,
                                          El::String::Manip::XE_TEXT_ENCODING |
                                          El::String::Manip::XE_PRESERVE_UTF8);
          }
        }
      }
      
      void
      CategoryChange::word_lists_details(std::ostream& ostr) const
        throw(El::Exception)
      {
        if(fields_change & FC_WORD_LISTS)
        {
          ostr << "<br>\nword lists";
          
          if(!added_word_lists.empty())
          {            
            for(WordListDescriptorSet::const_iterator
                  i(added_word_lists.begin()), e(added_word_lists.end());
                i != e; ++i)
            {
              ostr << "<br>\n&nbsp;added";
              word_list_details(*i, ostr);
            }            
          }

          if(!removed_word_lists.empty())
          {
            for(WordListDescriptorSet::const_iterator
                  i(removed_word_lists.begin()), e(removed_word_lists.end());
                i != e; ++i)
            {
              ostr << "<br>\n&nbsp;removed";
              word_list_details(*i, ostr);
            }            
          }

          if(!changed_word_lists.empty())
          {
            for(WordListChangeArray::const_iterator
                  i(changed_word_lists.begin()), e(changed_word_lists.end());
                i != e; ++i)
            {
              ostr << "<br>\n&nbsp;changed";
              i->details(ostr);
            }
          }
        }
      }

      void
      CategoryChange::locales_summary(std::ostream& ostr,
                                      bool& write_comma) const
        throw(El::Exception)
      {
        if(fields_change & FC_LOCALES)
        {
          if(write_comma)
          {
            ostr << ", ";
          }
          
          ostr << "locales";
          
          if(!added_locales.empty())
          {
            ostr << " added " << added_locales.size();
          }
          
          if(!removed_locales.empty())
          {
            ostr << " removed " << removed_locales.size();
          }
          
          if(!changed_locales.empty())
          {
            ostr << " changed " << changed_locales.size();
          }

          write_comma = true;
        }
      }
      
      void
      CategoryChange::locales_details(const LocaleDescriptorSet& locales,
                                      std::ostream& ostr) const
        throw(El::Exception)
      {
        for(LocaleDescriptorSet::const_iterator i(locales.begin()),
              e(locales.end()); i != e; ++i)
        {
          const LocaleDescriptor& desc = *i;
          
          ostr << " (lang:"
               << (desc.lang ?
                   El::Lang((El::Lang::ElCode)desc.lang).l3_code() :
                   "-Any-") << ", country:"
               << (desc.country ?
                   El::Country((El::Country::ElCode)desc.country).l3_code() :
                   "-Any-") << ", name:";
        
          El::String::Manip::xml_encode(desc.name.in(),
                                        ostr,
                                        El::String::Manip::XE_TEXT_ENCODING |
                                        El::String::Manip::XE_PRESERVE_UTF8);
          
          ostr << ")";
        }
      }

      void
      CategoryChange::locales_details(const LocaleChangeArray& locales,
                                      std::ostream& ostr) const
        throw(El::Exception)
      {
        for(LocaleChangeArray::const_iterator i(locales.begin()),
              e(locales.end()); i != e; ++i)
        {
          ostr << " ";
          i->details(ostr);
        }
      }
      
      void
      CategoryChange::locales_details(std::ostream& ostr) const
        throw(El::Exception)
      {
        if(fields_change & FC_LOCALES)
        {
          ostr << "<br>\nlocales";
          
          if(!added_locales.empty())
          {
            ostr << "<br>\n&nbsp;added";
            locales_details(added_locales, ostr);
          }

          if(!removed_locales.empty())
          {
            ostr << "<br>\n&nbsp;removed";
            locales_details(removed_locales, ostr);            
          }
          
          if(!changed_locales.empty())
          {
            ostr << "<br>\n&nbsp;changed";
            locales_details(changed_locales, ostr);            
          }
        }
      }
      
      const char*
      CategoryChange::value_name(uint64_t val,
                                 const ValueName* value_names) throw()
      {
        size_t i = 0;
        for(; value_names[i].first != val && value_names[i].second != 0; ++i);
        return value_names[i].second ? value_names[i].second : "Unknown";
      }

      void
      CategoryChange::find_locales_diff() throw(El::Exception)
      {
        for(LocaleDescriptorSet::iterator i(added_locales.begin());
            i != added_locales.end(); )
        {
          LocaleDescriptorSet::iterator j = removed_locales.find(*i);

          if(j == removed_locales.end())
          {
            ++i;
          }
          else
          {
            LocaleChange lc(*i, *j);
            
            if(lc.fields_change)
            {
              changed_locales.push_back(lc);
            }
            
            removed_locales.erase(j);

            LocaleDescriptorSet::iterator cur = i++;
            added_locales.erase(cur);
          }
        }

        if(!added_locales.empty() || !removed_locales.empty() ||
           !changed_locales.empty())
        {
          fields_change |= FC_LOCALES;
        }
      }

      void
      CategoryChange::find_rel_messages_diff(
        MessageIdSet& added_rel_messages,
        MessageIdSet& removed_rel_messages,
        unsigned long long fc)
        throw(El::Exception)
      {
        for(MessageIdSet::iterator i(added_rel_messages.begin());
            i != added_rel_messages.end(); )
        {
          MessageIdSet::iterator j = removed_rel_messages.find(*i);

          if(j == removed_rel_messages.end())
          {
            ++i;
          }
          else
          {            
            removed_rel_messages.erase(j);

            MessageIdSet::iterator cur = i++;
            added_rel_messages.erase(cur);
          }
        }

        if(!added_rel_messages.empty() || !removed_rel_messages.empty())
        {
          fields_change |= fc;
        }
      }

      void
      CategoryChange::find_word_lists_diff() throw(El::Exception)
      {
        for(WordListDescriptorSet::iterator i(added_word_lists.begin());
            i != added_word_lists.end(); )
        {
          WordListDescriptorSet::iterator j = removed_word_lists.find(*i);

          if(j == removed_word_lists.end())
          {
            ++i;
          }
          else
          {
            WordListChange wlc(*i, *j);
            
            if(wlc.fields_change)
            {
              changed_word_lists.push_back(wlc);
            }
            
            removed_word_lists.erase(j);

            WordListDescriptorSet::iterator cur = i++;
            added_word_lists.erase(cur);
          }
        }

        if(!added_word_lists.empty() || !removed_word_lists.empty() ||
           !changed_word_lists.empty())
        {
          fields_change |= FC_WORD_LISTS;
        }
      }

      void
      CategoryChange::find_expressions_diff() throw(El::Exception)
      {
        for(ExpressionDescriptorSet::iterator i(added_expressions.begin());
            i != added_expressions.end(); )
        {
          ExpressionDescriptorSet::iterator j = removed_expressions.find(*i);

          if(j == removed_expressions.end())
          {
            ++i;
          }
          else
          {
            ExpressionChange ec(*i, *j);
            
            if(ec.fields_change)
            {
              changed_expressions.push_back(ec);
            }
            
            removed_expressions.erase(j);

            ExpressionDescriptorSet::iterator cur = i++;
            added_expressions.erase(cur);
          }
        }

        if(!added_expressions.empty() || !removed_expressions.empty() ||
           !changed_expressions.empty())
        {
          fields_change |= FC_EXPRESSIONS;
        }
      }

      void
      CategoryChange::find_parent_diff() throw(El::Exception)
      {
        for(PathIdSet::iterator i(added_parents.begin());
            i != added_parents.end(); )
        {
          PathIdSet::iterator j = removed_parents.find(*i);

          if(j == removed_parents.end())
          {
            ++i;
          }
          else
          {
            removed_parents.erase(j);

            PathIdSet::iterator cur = i++;
            added_parents.erase(cur);
          }
        }

        if(!added_parents.empty() || !removed_parents.empty())
        {
          fields_change |= FC_PARENTS;
        }
      }

      void
      CategoryChange::find_children_diff() throw(El::Exception)
      {
        for(NameIdSet::iterator i(added_children.begin());
            i != added_children.end(); )
        {
          NameIdSet::iterator j = removed_children.find(*i);

          if(j == removed_children.end())
          {
            ++i;
          }
          else
          {            
            removed_children.erase(j);

            NameIdSet::iterator cur = i++;
            added_children.erase(cur);
          }
        }

        if(!added_children.empty() || !removed_children.empty())
        {
          fields_change |= FC_CHILDREN;
        }
      }

      //
      // LocaleChange struct
      //

      LocaleChange::LocaleChange() throw()
          : fields_change(0),
            new_lang(0),
            old_lang(0),
            new_country(0),
            old_country(0)
      {
      }

      LocaleChange::LocaleChange(const LocaleDescriptor& new_loc,
                                 const LocaleDescriptor& old_loc)
        throw(El::Exception)
          : fields_change(0),
            new_lang(0),
            old_lang(0),
            new_country(0),
            old_country(0)
      {        
        if(strcmp(new_loc.name.in(), old_loc.name.in()))
        {
          old_name = old_loc.name.in();
          new_name = new_loc.name.in();
          fields_change |= FC_NAME;
        }
        
        // Set always as it part of identification
        new_lang = new_loc.lang;

        if(new_loc.lang != old_loc.lang)
        {
          old_lang = old_loc.lang;
          fields_change |= FC_LANG;
        }
        
        // Set always as it part of identification
        new_country = new_loc.country;
        
        if(new_loc.country != old_loc.country)
        {
          old_country = old_loc.country;
          fields_change |= FC_COUNTRY;
        }
      }

      void
      LocaleChange::details(std::ostream& ostr) const throw(El::Exception)
      {
        ostr << "(lang:"
             << (new_lang ? El::Lang((El::Lang::ElCode)new_lang).l3_code() :
                 "-Any-");
        
        if(fields_change & FC_LANG)
        {          
          ostr << " &lt;- "
               << (old_lang ? El::Lang((El::Lang::ElCode)old_lang).l3_code() :
                   "-Any-");          
        }

        ostr << ", country:"
             << (new_country ?
                 El::Country((El::Country::ElCode)new_country).l3_code() :
                 "-Any-");
          
        if(fields_change & FC_COUNTRY)
        {          
          ostr << " &lt;- "
               << (old_country ?
                   El::Country((El::Country::ElCode)old_country).l3_code() :
                   "-Any-");
        }
        
        if(fields_change & FC_NAME)
        {
          ostr << ", name: '";

          El::String::Manip::xml_encode(new_name.c_str(),
                                        ostr,
                                        El::String::Manip::XE_TEXT_ENCODING |
                                        El::String::Manip::XE_PRESERVE_UTF8);

          ostr << "' &lt;- '";
          
          El::String::Manip::xml_encode(old_name.c_str(),
                                        ostr,
                                        El::String::Manip::XE_TEXT_ENCODING |
                                        El::String::Manip::XE_PRESERVE_UTF8);

          ostr << "'";
        }

        ostr << ")";
      }
      
      void
      LocaleChange::write(El::BinaryOutStream& bstr) const
        throw(El::Exception)
      {
        bstr << fields_change;
        
        if(fields_change & FC_NAME)
        {
          bstr << old_name << new_name;
        }

        bstr << new_lang;
        
        if(fields_change & FC_LANG)
        {
          bstr << old_lang;
        }

        bstr << new_country;
        
        if(fields_change & FC_COUNTRY)
        {
          bstr << old_country;
        }
      }
      
      void
      LocaleChange::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> fields_change;
        
        if(fields_change & FC_NAME)
        {
          bstr >> old_name >> new_name;
        }
        else
        {
          old_name.clear();
          new_name.clear();
        }

        bstr >> new_lang;
        
        if(fields_change & FC_LANG)
        {
          bstr >> old_lang;
        }
        else
        {
          old_lang = 0;
        }

        bstr >> new_country;
        
        if(fields_change & FC_COUNTRY)
        {
          bstr >> old_country;
        }
        else
        {
          old_country = 0;
        }
      }

      //
      // WordListChange struct
      //
      WordListChange::WordListChange(const WordListDescriptor& new_wl,
                                     const WordListDescriptor& old_wl)
        throw(El::Exception)
          : fields_change(0),
            new_version(0),
            old_version(0)
      {        
        if(new_wl.version != old_wl.version)
        {
          old_version = old_wl.version;
          new_version = new_wl.version;
          fields_change |= FC_VERSION;
        }

        // Set always as it is identification
        new_name = new_wl.name.in();
        
        if(strcmp(new_wl.name.in(), old_wl.name.in()))
        {
          old_name = old_wl.name.in();
          fields_change |= FC_NAME;
        }
        
        if(strcmp(new_wl.description.in(), old_wl.description.in()))
        {
          old_description = old_wl.description.in();
          new_description = new_wl.description.in();
          fields_change |= FC_DESCRIPTION;
        }

        words_to_set(new_wl.words.in(), added_words);
        words_to_set(old_wl.words.in(), removed_words);
        find_words_diff();
      }

      WordListChange::WordListChange() throw()
          : fields_change(0),
            new_version(0),
            old_version(0)
      {
      }

      void
      WordListChange::details(std::ostream& ostr) const
        throw(El::Exception)
      {
        ostr << "<br>\n&nbsp;name '";

        El::String::Manip::xml_encode(new_name.c_str(),
                                      ostr,
                                      El::String::Manip::XE_TEXT_ENCODING |
                                      El::String::Manip::XE_PRESERVE_UTF8);

        ostr << "'";

        if(fields_change & FC_NAME)
        {
          ostr << " &lt;- '";
          
          El::String::Manip::xml_encode(old_name.c_str(),
                                        ostr,
                                        El::String::Manip::XE_TEXT_ENCODING |
                                        El::String::Manip::XE_PRESERVE_UTF8);
          ostr << "'";
        }        

        if(fields_change & FC_VERSION)
        {
          ostr << "<br>\n&nbsp;version " << new_version
               << " &lt;- " << old_version;
        }

        if(fields_change & FC_DESCRIPTION)
        {
          ostr << "<br>\n&nbsp;description '";

          El::String::Manip::xml_encode(new_description.c_str(),
                                        ostr,
                                        El::String::Manip::XE_TEXT_ENCODING |
                                        El::String::Manip::XE_PRESERVE_UTF8);

          ostr << "' &lt;- '";
        
          El::String::Manip::xml_encode(old_description.c_str(),
                                        ostr,
                                        El::String::Manip::XE_TEXT_ENCODING |
                                        El::String::Manip::XE_PRESERVE_UTF8);

          ostr << "'";
        }

        if(!added_words.empty())
        {
          ostr << "<br>\n&nbsp;added words";

          for(El::String::Set::const_iterator i(added_words.begin()),
                e(added_words.end()); i != e; ++i)
          {
            ostr << "<br>\n&nbsp; ";
            
            El::String::Manip::xml_encode(i->c_str(),
                                          ostr,
                                          El::String::Manip::XE_TEXT_ENCODING |
                                          El::String::Manip::XE_PRESERVE_UTF8);
          }
        }
        
        if(!removed_words.empty())
        {
          ostr << "<br>\n&nbsp;removed words";

          for(El::String::Set::const_iterator i(removed_words.begin()),
                e(removed_words.end()); i != e; ++i)
          {
            ostr << "<br>\n&nbsp; ";
            
            El::String::Manip::xml_encode(i->c_str(),
                                          ostr,
                                          El::String::Manip::XE_TEXT_ENCODING |
                                          El::String::Manip::XE_PRESERVE_UTF8);
          }
        }
      }

      void
      WordListChange::write(El::BinaryOutStream& bstr) const
        throw(El::Exception)
      {
        bstr << fields_change;

        if(fields_change & FC_VERSION)
        {
          bstr << old_version << new_version;
        }

        bstr << new_name;
        
        if(fields_change & FC_NAME)
        {
          bstr << old_name;
        }        
        
        if(fields_change & FC_DESCRIPTION)
        {
          bstr << old_description << new_description;
        }        

        if(fields_change & FC_WORDS)
        {
          bstr.write_set(added_words);
          bstr.write_set(removed_words);
        }
      }
      
      void
      WordListChange::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> fields_change;

        if(fields_change & FC_VERSION)
        {
          bstr >> old_version >> new_version;
        }

        bstr >> new_name;
        
        if(fields_change & FC_NAME)
        {
          bstr >> old_name;
        }        
        
        if(fields_change & FC_DESCRIPTION)
        {
          bstr >> old_description >> new_description;
        }        

        if(fields_change & FC_WORDS)
        {
          bstr.read_set(added_words);
          bstr.read_set(removed_words);
        }
      }

      void
      WordListChange::find_words_diff() throw(El::Exception)
      {
        for(El::String::Set::iterator i(added_words.begin());
            i != added_words.end(); )
        {
          El::String::Set::iterator j = removed_words.find(*i);

          if(j == removed_words.end())
          {
            ++i;
          }
          else
          {            
            removed_words.erase(j);

            El::String::Set::iterator cur = i++;
            added_words.erase(cur);
          }
        }

        if(!added_words.empty() || !removed_words.empty())
        {
          fields_change |= FC_WORDS;
        }
      }

      void
      WordListChange::words_to_set(const char* words,
                                   El::String::Set& words_set)
        throw(El::Exception)
      {
        El::String::ListParser parser(words, "\n\r");

        const char* item = 0;

        while((item = parser.next_item()) != 0)
        {
          std::string word;
          El::String::Manip::trim(item, word);

          if(!word.empty())
          {
            words_set.insert(word);
          }   
        }
      }
      
      //
      // ExpressionChange struct
      //

      ExpressionChange::ExpressionChange() throw()
          : fields_change(0)
      {
      }
      
      ExpressionChange::ExpressionChange(const ExpressionDescriptor& new_ex,
                                         const ExpressionDescriptor& old_ex)
        throw(El::Exception)
          : fields_change(0)
      {
        // Set always as it is identification
        new_expression = new_ex.expression.in();
        
        if(strcmp(new_ex.expression.in(), old_ex.expression.in()))
        {
          old_expression = old_ex.expression.in();
          fields_change |= FC_EXPRESSION;
        }
        
        if(strcmp(new_ex.description.in(), old_ex.description.in()))
        {
          old_description = old_ex.description.in();
          new_description = new_ex.description.in();
          fields_change |= FC_DESCRIPTION;
        }
      }

      void
      ExpressionChange::details(std::ostream& ostr) const throw(El::Exception)
      {
        ostr << "<br>\n&nbsp;expression<br>\n&nbsp;";

        El::String::Manip::xml_encode(new_expression.c_str(),
                                      ostr,
                                      El::String::Manip::XE_TEXT_ENCODING |
                                      El::String::Manip::XE_PRESERVE_UTF8);

        if(fields_change & FC_EXPRESSION)
        {
          ostr << "\n&nbsp;&lt;-<br>\n&nbsp;";
          
          El::String::Manip::xml_encode(old_expression.c_str(),
                                        ostr,
                                        El::String::Manip::XE_TEXT_ENCODING |
                                        El::String::Manip::XE_PRESERVE_UTF8);
        }

        if(fields_change & FC_DESCRIPTION)
        {
          ostr << "<br>\n&nbsp;description '";

          El::String::Manip::xml_encode(new_description.c_str(),
                                        ostr,
                                        El::String::Manip::XE_TEXT_ENCODING |
                                        El::String::Manip::XE_PRESERVE_UTF8);

          ostr << "' &lt;- '";
        
          El::String::Manip::xml_encode(old_description.c_str(),
                                        ostr,
                                        El::String::Manip::XE_TEXT_ENCODING |
                                        El::String::Manip::XE_PRESERVE_UTF8);

          ostr << "'";
        }
      }      

      void
      ExpressionChange::write(El::BinaryOutStream& bstr) const
        throw(El::Exception)
      {
        bstr << fields_change << new_expression;

        if(fields_change & FC_EXPRESSION)
        {
          bstr << old_expression;
        }

        if(fields_change & FC_DESCRIPTION)
        {
          bstr << old_description << new_description;
        }
      }
      
      void
      ExpressionChange::read(El::BinaryInStream& bstr) throw(El::Exception)
      {
        bstr >> fields_change >> new_expression;

        if(fields_change & FC_EXPRESSION)
        {
          bstr >> old_expression;
        }
        else
        {
          old_expression.clear();
        }

        if(fields_change & FC_DESCRIPTION)
        {
          bstr >> old_description >> new_description;
        }
        else
        {
          old_description.clear();
          new_description.clear();
        }
      }

      //
      // seq_to_set functions
      //
      template<>
      void
      seq_to_set<CategoryDescriptorSeq,PathIdSet>(
        const CategoryDescriptorSeq& desc_seq,
        PathIdSet& desc_set) throw(El::Exception)
      {
        for(size_t i = 0; i < desc_seq.length(); ++i)
        {
          const CategoryDescriptor& cd = desc_seq[i];

          for(size_t j = 0; j < cd.paths.length(); ++j)
          {
            desc_set.insert(PathId(cd.paths[j].path.in(), cd.id));
          }
        }
      }
      
      template<>
      void
      seq_to_set<CategoryDescriptorSeq, NameIdSet>(
        const CategoryDescriptorSeq& desc_seq,
        NameIdSet& desc_set) throw(El::Exception)
      {
        for(size_t i = 0; i < desc_seq.length(); ++i)
        {
          const CategoryDescriptor& cd = desc_seq[i];
          desc_set.insert(NameId(cd.name.in(), cd.id));
        }
      }
      
    }
  }
}
