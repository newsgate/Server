/*
 * product   : NewsGate - news search WEB server
 * copyright : Copyright (c) 2005-2016 Karen Arutyunov
 * licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
 *             Commercial; contact karen.arutyunov@gmail.com
 */

/**
 * @file   NewsGate/Server/Frontends/SearchEngine/SearchEngine.hpp
 * @author Karen Arutyunov
 * $Id:$
 */

#ifndef _NEWSGATE_SERVER_FRONTENDS_SEARCHENGINE_SEARCHENGINE_HPP_
#define _NEWSGATE_SERVER_FRONTENDS_SEARCHENGINE_SEARCHENGINE_HPP_

#include <stdint.h>

#include <string>
#include <iostream>
#include <ext/hash_map>

#include <ace/Synch.h>
#include <ace/Guard_T.h>

#include <google/dense_hash_map>

#include <El/Exception.hpp>
#include <El/Stat.hpp>

#include <El/Python/Exception.hpp>
#include <El/Python/Object.hpp>
#include <El/Python/RefCount.hpp>
#include <El/Python/Logger.hpp>
#include <El/Python/Lang.hpp>
#include <El/Python/Locale.hpp>
#include <El/Python/Country.hpp>
#include <El/Python/Sequence.hpp>
#include <El/Python/Map.hpp>

#include <El/Apache/Request.hpp>
#include <El/PSP/Config.hpp>

#include <Commons/Event/Event.hpp>
#include <Commons/Ad/Ad.hpp>
#include <Commons/Ad/Python/Ad.hpp>

#include <Services/Commons/Message/BankClientSessionImpl.hpp>

#include <Services/Commons/Event/BankClientSessionImpl.hpp>
#include <Services/Commons/Event/EventServices.hpp>
#include <Services/Commons/Statistics/StatLogger.hpp>
#include <Services/Commons/Ad/AdServices.hpp>
#include <Services/Commons/SearchMailing/SearchMailing.hpp>

#include <Services/Dictionary/Commons/DictionaryServices.hpp>
#include <Services/Segmentation/Commons/SegmentationServices.hpp>

#include "Helpers.hpp"

namespace NewsGate
{
  class Category : public El::Python::ObjectImpl
  {
  public:
    Category(PyTypeObject *type = 0, PyObject *args = 0, PyObject *kwds = 0)
      throw(El::Exception);

    virtual ~Category() throw() {}

    Category* find(const char* path, bool create) throw(El::Exception);    
    void sort_by_localized_name() throw(El::Exception);
    
    void dump(std::ostream& ostr, size_t ident) const throw(El::Exception);
    
    PyObject* py_find(PyObject* args) throw(El::Exception);

    void set_parent(unsigned long long val) throw();

    class Type : public El::Python::ObjectTypeImpl<Category, Category::Type>
    {
    public:
      Type() throw(El::Python::Exception, El::Exception);
      static Type instance;

      PY_TYPE_MEMBER_WSTRING(name,
                             "name",
                             "Category name",
                             true);
    
      PY_TYPE_MEMBER_WSTRING(localized_name,
                             "localized_name",
                             "Localized category name",
                             true);
    
      PY_TYPE_MEMBER_ULONGLONG(id, "id", "Category id");
      
      PY_TYPE_MEMBER_ULONGLONG(parent, "parent", "Parent category id");
      
      PY_TYPE_MEMBER_ULONG(message_count,
                           "message_count",
                           "Category message count");
      
      PY_TYPE_MEMBER_ULONG(matched_message_count,
                           "matched_message_count",
                           "Category matched message count");
      
      PY_TYPE_MEMBER_OBJECT(categories,
                            El::Python::Sequence::Type,
                            "categories",
                            "Category subcategories",
                            false);

      PY_TYPE_METHOD_VARARGS(py_find, "find", "Finds category info");
    };
    
    std::wstring name;
    std::wstring localized_name;
    unsigned long long id;
    unsigned long long parent;
    unsigned long message_count;
    unsigned long matched_message_count;
    El::Python::Sequence_var categories;
  };

  typedef El::Python::SmartPtr<Category> Category_var;

  class CategoryLocale : public El::Python::ObjectImpl
  {
  public:
    
    CategoryLocale(PyTypeObject *type = 0,
                   PyObject *args = 0,
                   PyObject *kwds = 0)
      throw(El::Exception);

    virtual ~CategoryLocale() throw() {}

    CategoryLocale& operator=(const Message::Categorizer::Category::Locale& val)
      throw(El::Exception);

    class Type : public El::Python::ObjectTypeImpl<CategoryLocale,
                                                   CategoryLocale::Type>
    {
    public:
      Type() throw(El::Python::Exception, El::Exception);
      static Type instance;

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

    std::string name;
    std::string title;
    std::string short_title;
    std::string description;
    std::string keywords;    
  };

  typedef El::Python::SmartPtr<CategoryLocale> CategoryLocale_var;

  class SearchResult : public El::Python::ObjectImpl
  {
  public:
    
    SearchResult(PyTypeObject *type, PyObject *args, PyObject *kwds)
      throw(El::Exception);
    
    virtual ~SearchResult() throw(){}

    class DebugInfo : public El::Python::ObjectImpl
    {
    public:
      DebugInfo(PyTypeObject *type, PyObject *args, PyObject *kwds)
        throw(El::Exception);
    
      virtual ~DebugInfo() throw(){}

      class Word : public El::Python::ObjectImpl
      {
      public:
        Word(PyTypeObject *type, PyObject *args, PyObject *kwds)
          throw(El::Exception);
    
        virtual ~Word() throw(){}
        
        class Type : public El::Python::ObjectTypeImpl<Word, Word::Type>
        {
        public:
          Type() throw(El::Python::Exception, El::Exception);
          static Type instance;
          
          PY_TYPE_MEMBER_ULONG(id, "id", "Word id");
          PY_TYPE_MEMBER_STRING(text, "text", "Word text", true);
          
          PY_TYPE_MEMBER_BOOL(remarkable,
                              "remarkable",
                              "Word is remarkable by some reason");
          
          PY_TYPE_MEMBER_BOOL(unknown, "unknown", "Word is not in dictionary");
          PY_TYPE_MEMBER_FLOAT(weight, "weight", "Word weight");
          
          PY_TYPE_MEMBER_FLOAT(wp_weight,
                               "wp_weight",
                               "Word pair weight");
          
          PY_TYPE_MEMBER_FLOAT(cw_weight,
                               "cw_weight",
                               "Core word weight");
          
          PY_TYPE_MEMBER_ULONG(position, "position", "Word position");
          
          PY_TYPE_MEMBER_OBJECT(lang,
                                El::Python::Lang::Type,
                                "lang",
                                "Word language",
                                false);
          
          PY_TYPE_MEMBER_STRING(token_type, "token_type", "Token type", false);
          PY_TYPE_MEMBER_STRING(flags, "flags", "Flags", true);
        };

        unsigned long id;
        std::string text;
        bool remarkable;
        bool unknown;
        float weight;
        float cw_weight;
        float wp_weight;
        unsigned long position;
        El::Python::Lang_var lang;
        std::string token_type;
        std::string flags;
      };

      typedef El::Python::SmartPtr<Word> Word_var;

      class EventOverlap : public El::Python::ObjectImpl
      {
      public:
        EventOverlap(PyTypeObject *type, PyObject *args, PyObject *kwds)
          throw(El::Exception);
    
        virtual ~EventOverlap() throw(){}
        
        class Type :
          public El::Python::ObjectTypeImpl<EventOverlap, EventOverlap::Type>
        {
        public:
          Type() throw(El::Python::Exception, El::Exception);
          static Type instance;
          
          PY_TYPE_MEMBER_ULONG(level, "level", "Event overlap level");
          
          PY_TYPE_MEMBER_ULONG(common_words,
                               "common_words",
                               "Event common words overlap");
          
          PY_TYPE_MEMBER_ULONG(time_diff,
                               "time_diff",
                               "Overlapped events time difference");
          
          PY_TYPE_MEMBER_ULONG(merge_level,
                               "merge_level",
                               "Overlapped events required merge level");
          
          PY_TYPE_MEMBER_ULONG(size,
                               "size",
                               "Merged event size");
          
          PY_TYPE_MEMBER_ULONG(strain,
                               "strain",
                               "Merged event strain");
          
          PY_TYPE_MEMBER_ULONG(dissenters,
                               "dissenters",
                               "Merged event dissenters");
          
          PY_TYPE_MEMBER_ULONG(time_range,
                               "time_range",
                               "Merged event time range");
          
          PY_TYPE_MEMBER_BOOL(colocated,
                              "colocated",
                              "Overlapped event colocated flag");

          PY_TYPE_MEMBER_BOOL(same_lang,
                              "same_lang",
                              "Overlapped event has same language");

          PY_TYPE_MEMBER_BOOL(can_merge,
                              "can_merge",
                              "If events can be merged");
          
          PY_TYPE_MEMBER_ULONGLONG(merge_blacklist_timeout,
                                   "merge_blacklist_timeout",
                                   "Merge with overlaped event timeout");

          PY_TYPE_MEMBER_STRING(extras, "extras", "Extra debug info", true);
        };

        unsigned long level;
        unsigned long common_words;
        unsigned long time_diff;
        unsigned long merge_level;
        unsigned long size;
        unsigned long strain;
        unsigned long dissenters;
        unsigned long time_range; 
        bool colocated;
        bool same_lang;
        bool can_merge;
        unsigned long long merge_blacklist_timeout;

        std::string extras;        
      };

      typedef El::Python::SmartPtr<EventOverlap> EventOverlap_var;

      class EventSplitPart : public El::Python::ObjectImpl
      {
      public:
        EventSplitPart(PyTypeObject *type, PyObject *args, PyObject *kwds)
          throw(El::Exception);
    
        virtual ~EventSplitPart() throw(){}
        
        class Type :
          public El::Python::ObjectTypeImpl<EventSplitPart,
                                            EventSplitPart::Type>
        {
        public:
          Type() throw(El::Python::Exception, El::Exception);
          static Type instance;
          
          PY_TYPE_MEMBER_ULONG(dissenters, "dissenters", "Event dissenters");
          PY_TYPE_MEMBER_ULONG(strain, "strain", "Event strain");
          PY_TYPE_MEMBER_ULONG(size, "size", "Event size");
          PY_TYPE_MEMBER_ULONG(time_range, "time_range", "Event time range");
          
          PY_TYPE_MEMBER_ULONG(published_min,
                               "published_min",
                               "Event min pub date");
          
          PY_TYPE_MEMBER_ULONG(published_max,
                               "published_max",
                               "Event max pub date");
        };

        unsigned long dissenters;
        unsigned long strain;
        unsigned long size;
        unsigned long time_range;
        unsigned long published_min;
        unsigned long published_max;
      };
      
      typedef El::Python::SmartPtr<EventSplitPart> EventSplitPart_var;      

      class EventSplit : public El::Python::ObjectImpl
      {
      public:
        EventSplit(PyTypeObject *type, PyObject *args, PyObject *kwds)
          throw(El::Exception);
    
        virtual ~EventSplit() throw(){}
        
        class Type :
          public El::Python::ObjectTypeImpl<EventSplit, EventSplit::Type>
        {
        public:
          Type() throw(El::Python::Exception, El::Exception);
          static Type instance;
          
          PY_TYPE_MEMBER_ULONG(overlap_level,
                               "overlap_level",
                               "Event overlap level");
          
          PY_TYPE_MEMBER_ULONG(merge_level,
                               "merge_level",
                               "Overlapped events required merge level");
          
          PY_TYPE_MEMBER_ULONG(common_words,
                               "common_words",
                               "Event common words overlap");
          
          PY_TYPE_MEMBER_ULONG(time_diff,
                               "time_diff",
                               "Overlapped events time difference");

          PY_TYPE_MEMBER_OBJECT(part1,
                                EventSplitPart::Type,
                                "part1",
                                "Event split part1 info",
                                false);

          PY_TYPE_MEMBER_OBJECT(part2,
                                EventSplitPart::Type,
                                "part2",
                                "Event split part2 info",
                                false);
          
          PY_TYPE_MEMBER_STRING(extras, "extras", "Extra debug info", true);

          PY_TYPE_MEMBER_OBJECT(overlap1,
                                EventOverlap::Type,
                                "overlap1",
                                "Part 1 event overlap with 'eo' event denoted",
                                false);          

          PY_TYPE_MEMBER_OBJECT(overlap2,
                                EventOverlap::Type,
                                "overlap2",
                                "Part 2 event overlap with 'eo' event denoted",
                                false);
        };

        unsigned long overlap_level;
        unsigned long merge_level;
        unsigned long common_words;
        unsigned long time_diff;
        EventSplitPart_var part1;
        EventSplitPart_var part2;
        EventOverlap_var overlap1;
        EventOverlap_var overlap2;
        std::string extras;
      };
      
      typedef El::Python::SmartPtr<EventSplit> EventSplit_var;

      class Event : public El::Python::ObjectImpl
      {
      public:
        Event(PyTypeObject *type, PyObject *args, PyObject *kwds)
          throw(El::Exception);
    
        virtual ~Event() throw(){}
        
        class Type : public El::Python::ObjectTypeImpl<Event, Event::Type>
        {
        public:
          Type() throw(El::Python::Exception, El::Exception);
          static Type instance;
          
          PY_TYPE_MEMBER_STRING(id, "id", "Event id", true);
          PY_TYPE_MEMBER_ULONG(hash, "hash", "Event hash");

          PY_TYPE_MEMBER_OBJECT(lang,
                                El::Python::Lang::Type,
                                "lang",
                                "Event language",
                                false);

          PY_TYPE_MEMBER_STRING(flags, "flags", "Event flags", true);

          PY_TYPE_MEMBER_BOOL(changed,
                              "changed",
                              "If event belongs to changed events set");

          PY_TYPE_MEMBER_OBJECT(overlap,
                                EventOverlap::Type,
                                "overlap",
                                "Event overlap info",
                                false);
          
          PY_TYPE_MEMBER_OBJECT(split,
                                EventSplit::Type,
                                "split",
                                "Event split info",
                                false);
          
          PY_TYPE_MEMBER_OBJECT(separate,
                                EventSplit::Type,
                                "separate",
                                "Event separation info",
                                false);
          
          PY_TYPE_MEMBER_ULONG(dissenters, "dissenters", "Event dissenters");
          PY_TYPE_MEMBER_ULONG(strain, "strain", "Event strain");
          PY_TYPE_MEMBER_ULONG(size, "size", "Event size");
          PY_TYPE_MEMBER_ULONG(time_range, "time_range", "Event time range");
          
          PY_TYPE_MEMBER_ULONG(published_min,
                               "published_min",
                               "Event min pub date");
          
          PY_TYPE_MEMBER_ULONG(published_max,
                               "published_max",
                               "Event max pub date");
          
          PY_TYPE_MEMBER_BOOL(can_merge,
                              "can_merge",
                              "If event can be merged with othe ones");

          PY_TYPE_MEMBER_ULONG(merge_level,
                               "merge_level",
                               "Events merge level");
          
          PY_TYPE_MEMBER_OBJECT(words,
                                El::Python::Sequence::Type,
                                "words",
                                "Event words",
                                false);
        };

        std::string id;
        unsigned long hash;
        El::Python::Lang_var lang;
        EventOverlap_var overlap;
        EventSplit_var split;
        EventSplit_var separate;
        std::string flags;
        bool changed;
        unsigned long dissenters;
        unsigned long strain;
        unsigned long size;
        unsigned long time_range;
        unsigned long published_min;
        unsigned long published_max;
        bool can_merge;
        unsigned long merge_level;
        El::Python::Sequence_var words;
      };

      typedef El::Python::SmartPtr<Event> Event_var;
      
      class Message : public El::Python::ObjectImpl
      {
      public:
        Message(PyTypeObject *type, PyObject *args, PyObject *kwds)
          throw(El::Exception);
    
        virtual ~Message() throw(){}
        
        class Type : public El::Python::ObjectTypeImpl<Message, Message::Type>
        {
        public:
          Type() throw(El::Python::Exception, El::Exception);
          static Type instance;
          
          PY_TYPE_MEMBER_STRING(id, "id", "Message id", false);

          PY_TYPE_MEMBER_ULONGLONG(signature,
                                   "signature",
                                   "Message signature");

          PY_TYPE_MEMBER_ULONGLONG(url_signature,
                                   "url_signature",
                                   "Message URL signature");

          PY_TYPE_MEMBER_ULONG(match_weight,
                               "match_weight",
                               "Word matching weight in message event");

          PY_TYPE_MEMBER_ULONGLONG(feed_impressions,
                                   "feed_impressions",
                                   "Feed message impression count");

          PY_TYPE_MEMBER_ULONGLONG(feed_clicks,
                                   "feed_clicks",
                                   "Feed message click count");          
          
          PY_TYPE_MEMBER_ULONG(core_words_in_event,
                               "core_words_in_event",
                               "Number of message core words belonging "
                               "to events word set");
      
          PY_TYPE_MEMBER_ULONG(common_event_words,
                               "common_event_words",
                               "Number of message core words belonging "
                               "to events word set and other message "
                               "word set");
          
          PY_TYPE_MEMBER_ULONG(event_overlap,
                               "event_overlap",
                               "Message-Event words overlap level");

          PY_TYPE_MEMBER_ULONG(event_merge_level,
                               "event_merge_level",
                               "Message-Event merge level");
          
          PY_TYPE_MEMBER_OBJECT(core_words,
                                El::Python::Sequence::Type,
                                "core_words",
                                "Message core words",
                                false);

          PY_TYPE_MEMBER_OBJECT(words,
                                El::Python::Sequence::Type,
                                "words",
                                "Message words",
                                false);

          PY_TYPE_MEMBER_STRING(extras, "extras", "Extra debug info", true);
        };

        std::string id;
        unsigned long long signature;
        unsigned long long url_signature;
        unsigned long match_weight;
        unsigned long feed_impressions;
        unsigned long feed_clicks;
        unsigned long core_words_in_event;
        unsigned long common_event_words;
        unsigned long event_overlap;
        unsigned long event_merge_level;
        El::Python::Sequence_var core_words;
        El::Python::Sequence_var words;

        std::string extras;
      };

      typedef El::Python::SmartPtr<Message> Message_var;
      
      class Type : public El::Python::ObjectTypeImpl<DebugInfo,
                                                     DebugInfo::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_OBJECT(event,
                              Event::Type,
                              "event",
                              "Event-level debug information",
                              false);

        PY_TYPE_MEMBER_OBJECT(messages,
                              El::Python::Map::Type,
                              "messages",
                              "Message-level debug information",
                              false);
      };

      Event_var event;
      El::Python::Map_var messages;
    };
    
    typedef El::Python::SmartPtr<DebugInfo> DebugInfo_var;
    
    enum MessageLoadStatus
    {
      MLS_UNKNOWN,
      MLS_LOADING,
      MLS_LOADED,
      MLS_COUNT
    };    

    class Type : public El::Python::ObjectTypeImpl<SearchResult,
                                                   SearchResult::Type>
    {
    public:
      Type() throw(El::Python::Exception, El::Exception);
      static Type instance;

      PY_TYPE_MEMBER_BOOL(nochanges,
                          "nochanges",
                          "No changes");

      PY_TYPE_MEMBER_ULONG(total_matched_messages,
                           "total_matched_messages",
                           "Total matched messages");
      
      PY_TYPE_MEMBER_ULONG(suppressed_messages,
                           "suppressed_messages",
                           "Suppressed message count");
/*      
      PY_TYPE_MEMBER_ULONG(space_filtered,
                           "space_filtered",
                           "Space-filtered message count");
*/
      
      PY_TYPE_MEMBER_ULONG(message_load_status,
                           "message_load_status",
                           "Message load status");
      
      PY_TYPE_MEMBER_ULONGLONG(etag,
                               "etag",
                               "Resource identifier");
      
      PY_TYPE_MEMBER_OBJECT(messages,
                            El::Python::Sequence::Type,
                            "messages",
                            "Messages found",
                            false);

      PY_TYPE_MEMBER_ULONG(second_column_offset,
                           "second_column_offset",
                           "Index starting second column");

      PY_TYPE_MEMBER_OBJECT(lang_filter_options,
                            El::Python::Sequence::Type,
                            "lang_filter_options",
                            "Language filtering options",
                            false);

      PY_TYPE_MEMBER_OBJECT(country_filter_options,
                            El::Python::Sequence::Type,
                            "country_filter_options",
                            "Country filtering options",
                            false);
      
      PY_TYPE_MEMBER_OBJECT(feed_filter_options,
                            El::Python::Sequence::Type,
                            "feed_filter_options",
                            "Feed filtering options",
                            false);
      
      PY_TYPE_MEMBER_OBJECT(category_stat,
                            Category::Type,
                            "category_stat",
                            "Category statistics",
                            false);
/*
      PY_TYPE_MEMBER_OBJECT(space_filter_options,
                            El::Python::Sequence::Type,
                            "space_filter_options",
                            "Space filtering options",
                            false);
*/
      PY_TYPE_MEMBER_OBJECT(ad_selection,
                            Ad::Python::SelectionResult::Type,
                            "ad_selection",
                            "Ad selection",
                            false);

      PY_TYPE_MEMBER_OBJECT(category_locale,
                            CategoryLocale::Type,
                            "category_locale",
                            "Category localized info",
                            false);

      PY_TYPE_MEMBER_OBJECT(debug_info,
                            DebugInfo::Type,
                            "debug_info",
                            "Debug information",
                            true);

      PY_TYPE_MEMBER_STRING(request_id, "request_id", "Request id", true);
      
      PY_TYPE_MEMBER_STRING(optimized_query,
                            "optimized_query",
                            "Optimized search query",
                            true);

      PY_TYPE_STATIC_MEMBER(MLS_UNKNOWN_, "MLS_UNKNOWN");
      PY_TYPE_STATIC_MEMBER(MLS_LOADING_, "MLS_LOADING");
      PY_TYPE_STATIC_MEMBER(MLS_LOADED_, "MLS_LOADED");

    protected:      
      virtual void ready() throw(El::Python::Exception, El::Exception);

    private:
      El::Python::Object_var MLS_UNKNOWN_;
      El::Python::Object_var MLS_LOADING_;
      El::Python::Object_var MLS_LOADED_;
    };
    
    bool nochanges;
    unsigned long total_matched_messages;
    unsigned long suppressed_messages;
//    unsigned long space_filtered;
    unsigned long message_load_status;
    unsigned long long etag;
    El::Python::Sequence_var messages;
    unsigned long second_column_offset;
    El::Python::Sequence_var lang_filter_options;
    El::Python::Sequence_var country_filter_options;
    El::Python::Sequence_var feed_filter_options;
    Category_var category_stat;
    Ad::Python::SelectionResult_var ad_selection;
    CategoryLocale_var category_locale;
    
//    El::Python::Sequence_var space_filter_options;
    std::string request_id;
    std::string optimized_query;
    DebugInfo_var debug_info;
    
    class Option : public El::Python::ObjectImpl
    {
    public:
      Option(PyTypeObject *type, PyObject *args, PyObject *kwds)
        throw(El::Exception);

      Option(const wchar_t* id_val,
             unsigned long count_val,
             const wchar_t* val,
             bool selected_val)
        throw(El::Exception);

      virtual ~Option() throw(){}
      
      class Type : public El::Python::ObjectTypeImpl<Option, Option::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_WSTRING(id, "id", "Option id", true);
        PY_TYPE_MEMBER_ULONG(count, "count", "Items count");
        PY_TYPE_MEMBER_WSTRING(value, "value", "Option value", true);
        PY_TYPE_MEMBER_BOOL(selected, "selected", "Option selection state");
      };

      std::wstring id;
      unsigned long count;
      std::wstring value;
      bool selected;
    };
    
    typedef El::Python::SmartPtr<Option> Option_var;

    class ImageThumb : public El::Python::ObjectImpl
    {
    public:
      ImageThumb(PyTypeObject *type, PyObject *args, PyObject *kwds)
        throw(El::Exception);
    
      virtual ~ImageThumb() throw(){}

      class Type :
        public El::Python::ObjectTypeImpl<ImageThumb, ImageThumb::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_STRING(src, "src", "ImageThumb src", false);
        PY_TYPE_MEMBER_INT(width, "width", "ImageThumb width");
        PY_TYPE_MEMBER_INT(height, "height", "ImageThumb height");
        PY_TYPE_MEMBER_BOOL(cropped, "cropped", "ImageThumb cropped flage");
      };

      std::string src;
      int width;
      int height;
      bool cropped;
    };
    
    typedef El::Python::SmartPtr<ImageThumb> ImageThumb_var;

    class Image : public El::Python::ObjectImpl
    {
    public:
      Image(PyTypeObject *type, PyObject *args, PyObject *kwds)
        throw(El::Exception);
    
      virtual ~Image() throw(){}

      class Type : public El::Python::ObjectTypeImpl<Image, Image::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_STRING(src,
                              "src",
                              "Image src",
                              false);
 
        PY_TYPE_MEMBER_STRING(alt,
                              "alt",
                              "Image alternate text",
                              false);
 
        PY_TYPE_MEMBER_INT(width,
                           "width",
                           "Image width");
        
        PY_TYPE_MEMBER_INT(height,
                           "height",
                           "Image height");

        PY_TYPE_MEMBER_ULONG(orig_width,
                             "orig_width",
                             "Image original width");
        
        PY_TYPE_MEMBER_ULONG(orig_height,
                             "orig_height",
                             "Image original height");

        PY_TYPE_MEMBER_OBJECT(thumbs,
                              El::Python::Sequence::Type,
                              "thumbs",
                              "Message thumbnail",
                              false);
        
        PY_TYPE_MEMBER_ULONG(alt_highlighted,
                             "alt_highlighted",
                             "Image alternate text has highlighted words");
        
        PY_TYPE_MEMBER_ULONG(index,
                             "index",
                             "Image index");
      };

      std::string src;
      std::string alt;
      int width;
      int height;
      unsigned long orig_width;
      unsigned long orig_height;
      El::Python::Sequence_var thumbs;
      bool alt_highlighted;
      unsigned long index;
    };
    
    typedef El::Python::SmartPtr<Image> Image_var;

    class Message : public El::Python::ObjectImpl
    {
    public:
      Message(PyTypeObject *type, PyObject *args, PyObject *kwds)
        throw(El::Exception);
    
      virtual ~Message() throw() {}

      class CoreWord : public El::Python::ObjectImpl
      {
      public:
        CoreWord(PyTypeObject *type, PyObject *args, PyObject *kwds)
          throw(El::Exception);
    
        virtual ~CoreWord() throw(){}
        
        class Type :
          public El::Python::ObjectTypeImpl<CoreWord, CoreWord::Type>
        {
        public:
          Type() throw(El::Python::Exception, El::Exception);
          static Type instance;
          
          PY_TYPE_MEMBER_STRING(text, "text", "Word text", true);
          PY_TYPE_MEMBER_ULONG(id, "id", "Word id");
          PY_TYPE_MEMBER_ULONG(weight, "weight", "Word weight");
        };

        std::string text;
        unsigned long id;
        unsigned long weight;
      };

      typedef El::Python::SmartPtr<CoreWord> CoreWord_var;

      class Type : public El::Python::ObjectTypeImpl<Message, Message::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_STRING(id,
                              "id",
                              "Message id",
                              false);
 
        PY_TYPE_MEMBER_STRING(encoded_id,
                              "encoded_id",
                              "Message id encoded",
                              false);
          
        PY_TYPE_MEMBER_STRING(event_id,
                              "event_id",
                              "Message event id",
                              false);
 
        PY_TYPE_MEMBER_STRING(encoded_event_id,
                              "encoded_event_id",
                              "Message event id encoded",
                              false);
 
        PY_TYPE_MEMBER_STRING(url,
                              "url",
                              "Message content url",
                              false);
  
        PY_TYPE_MEMBER_STRING(title,
                              "title",
                              "Message title",
                              true);
 
        PY_TYPE_MEMBER_STRING(description,
                              "description",
                              "Message description",
                              true);
 
        PY_TYPE_MEMBER_OBJECT(images,
                              El::Python::Sequence::Type,
                              "images",
                              "Message images",
                              false);

        PY_TYPE_MEMBER_STRING(keywords,
                              "keywords",
                              "Message Keywords",
                              true);
 
        PY_TYPE_MEMBER_ULONG(event_capacity,
                             "event_capacity",
                             "Event capacity");
        
        PY_TYPE_MEMBER_ULONGLONG(impressions,
                                 "impressions",
                                 "Message impression count");

        PY_TYPE_MEMBER_ULONGLONG(clicks,
                                 "clicks",
                                 "Message click count");

        PY_TYPE_MEMBER_ULONGLONG(feed_impressions,
                                 "feed_impressions",
                                 "Message feed impression count");

        PY_TYPE_MEMBER_ULONGLONG(feed_clicks,
                                 "feed_clicks",
                                 "Message feed click count");

        PY_TYPE_MEMBER_ULONGLONG(published,
                                 "published",
                                 "Message publish time");
        
        PY_TYPE_MEMBER_ULONGLONG(visited,
                                 "visited",
                                 "Message visit time");
        
        PY_TYPE_MEMBER_ULONGLONG(fetched,
                                 "fetched",
                                 "Message fetch time");
        
        PY_TYPE_MEMBER_ULONGLONG(source_id,
                                 "source_id",
                                 "Message source id");
        
        PY_TYPE_MEMBER_STRING(source_url,
                              "source_url",
                              "Message source url",
                              false);

        PY_TYPE_MEMBER_STRING(source_title,
                              "source_title",
                              "Message source title",
                              true);
        
        PY_TYPE_MEMBER_STRING(source_html_link,
                              "source_html_link",
                              "Message source html link",
                              true);
        
        PY_TYPE_MEMBER_OBJECT(lang,
                              El::Python::Lang::Type,
                              "lang",
                              "Message language",
                              false);

        PY_TYPE_MEMBER_OBJECT(country,
                              El::Python::Country::Type,
                              "country",
                              "Message country",
                              false);

        PY_TYPE_MEMBER_OBJECT(core_words,
                              El::Python::Sequence::Type,
                              "core_words",
                              "Message core words",
                              true);

        PY_TYPE_MEMBER_OBJECT(categories,
                              El::Python::Sequence::Type,
                              "categories",
                              "Message categories",
                              true);
        
        PY_TYPE_MEMBER_OBJECT(front_image,
                              El::Python::AnyType,
                              "front_image",
                              "Message front image",
                              true);        
      };

      std::string id;
      std::string encoded_id;
      std::string event_id;
      std::string encoded_event_id;
      std::string url;
      std::string title;
      std::string description;
      El::Python::Sequence_var images;
      std::string keywords;
      unsigned long event_capacity;
      uint64_t impressions;
      uint64_t clicks;
      uint64_t feed_impressions;
      uint64_t feed_clicks;
      uint64_t published;
      uint64_t visited;
      uint64_t fetched;
      uint64_t source_id;
      std::string source_url;
      std::string source_title;
      std::string source_html_link;
      El::Python::Lang_var lang;
      El::Python::Country_var country;
      El::Python::Sequence_var core_words;
      El::Python::Sequence_var categories;
      El::Python::Object_var front_image;
    };

    typedef El::Python::SmartPtr<Message> Message_var;
  };

  typedef El::Python::SmartPtr<SearchResult> SearchResult_var;
  
  class SearchContext : public El::Python::ObjectImpl
  {
  public:
    SearchContext(PyTypeObject *type, PyObject *args, PyObject *kwds)
      throw(El::Exception);
    
    virtual ~SearchContext() throw() {}

    enum FormatFlag
    {
      FF_HTML = 0x1,
      FF_WRAP_LINKS = 0x2,
      FF_SEGMENTATION = 0x4,
      FF_FANCY_SEGMENTATION = 0x8
    };

    struct Filter : public El::Python::ObjectImpl
    {
      Filter(PyTypeObject *type, PyObject *args, PyObject *kwds)
        throw(El::Exception);

      ~Filter() throw() {}
      
      El::Python::Lang_var lang;
      El::Python::Country_var country;
      std::string feed;
      std::string category;
      unsigned long long event;
//      std::string spaces;

      class Type : public El::Python::ObjectTypeImpl<Filter, Filter::Type>
      {
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_OBJECT(lang,
                              El::Python::Lang::Type,
                              "lang",
                              "Language to filter by",
                              false);
        
        PY_TYPE_MEMBER_OBJECT(country,
                              El::Python::Country::Type,
                              "country",
                              "Country to filter by",
                              false);
        
        PY_TYPE_MEMBER_STRING(feed,
                              "feed",
                              "Feed to filter by",
                              true);
        
        PY_TYPE_MEMBER_STRING(category,
                              "category",
                              "Category to filter by",
                              true);

        PY_TYPE_MEMBER_ULONGLONG(event,
                                 "event",
                                 "Event to filter by");
/*        
        PY_TYPE_MEMBER_STRING(spaces,
                              "spaces",
                              "Spaces to filter by",
                              true);
*/
      };
    };

    typedef El::Python::SmartPtr<Filter> Filter_var;    

    struct Suppression : public El::Python::ObjectImpl
    {
      struct CoreWords : public El::Python::ObjectImpl
      {
        CoreWords(PyTypeObject *type, PyObject *args, PyObject *kwds)
          throw(El::Exception);

        ~CoreWords() throw() {}

        unsigned long intersection;
        unsigned long containment_level;
        unsigned long min_count;

        class Type : public El::Python::ObjectTypeImpl<CoreWords,
                                                       CoreWords::Type>
        {        
        public:
          Type() throw(El::Python::Exception, El::Exception);
          static Type instance;

          PY_TYPE_MEMBER_ULONG(
            intersection,
            "intersection",
            "Message suppression core words min intersection");
          
          PY_TYPE_MEMBER_ULONG(
            containment_level,
            "containment_level",
            "Message suppression core words min containment level");
        
          PY_TYPE_MEMBER_ULONG(min_count,
                               "min_count",
                               "Message suppression core words min count");
        };
      };

      typedef El::Python::SmartPtr<CoreWords> CoreWords_var;
      
      Suppression(PyTypeObject *type, PyObject *args, PyObject *kwds)
        throw(El::Exception);

      ~Suppression() throw() {}

      unsigned long type;
      El::Python::Object_var param;
      CoreWords_var core_words;

      class Type : public El::Python::ObjectTypeImpl<Suppression,
                                                     Suppression::Type>
      {        
      public:
        Type() throw(El::Python::Exception, El::Exception);
        static Type instance;

        PY_TYPE_MEMBER_ULONG(type,
                             "type",
                             "Message suppression type");
        
        PY_TYPE_MEMBER_OBJECT(param,
                              El::Python::AnyType,
                              "param",
                              "Message suppression parameter",
                              true);

      PY_TYPE_MEMBER_OBJECT(core_words,
                            CoreWords::Type,
                            "core_words",
                            "Message suppression core words options",
                            false);        
      };
    };

    typedef El::Python::SmartPtr<Suppression> Suppression_var;
    
    class Type : public El::Python::ObjectTypeImpl<SearchContext,
                                                   SearchContext::Type>
    {
    public:
      Type() throw(El::Python::Exception, El::Exception);
      static Type instance;

      PY_TYPE_MEMBER_STRING(query,
                            "query",
                            "Search expression",
                            true);
      
      PY_TYPE_MEMBER_BOOL(optimize_query,
                          "optimize_query",
                          "If to perform query static optimization");
      
      PY_TYPE_MEMBER_BOOL(save_optimized_query,
                          "save_optimized_query",
                          "Return optimized query");
      
      PY_TYPE_MEMBER_STRING(search_hint,
                            "search_hint",
                            "Additional search hint",
                            true);
 
      PY_TYPE_MEMBER_BOOL(in_2_columns,
                          "in_2_columns",
                          "Show in 2 columns");
      
      PY_TYPE_MEMBER_ULONG(sorting_type,
                           "sorting_type",
                           "Resulted messages sorting type");
      
      PY_TYPE_MEMBER_OBJECT(suppression,
                            Suppression::Type,
                            "suppression",
                            "Message suppression strategy",
                            false);      

      PY_TYPE_MEMBER_STRING(protocol,
                            "protocol",
                            "Search request protocol",
                            true);
      
      PY_TYPE_MEMBER_ULONGLONG(
        gm_flags,
        "gm_flags",
        "Specifies which message information to be retreived");

      PY_TYPE_MEMBER_ULONG(
        sr_flags,
        "sr_flags",
        "Specifies which search result information to be retreived");

      PY_TYPE_MEMBER_LONG(start_from,
                          "start_from",
                          "Display starting from");

      PY_TYPE_MEMBER_LONG(results_count,
                          "results_count",
                          "Display results number");
      
      PY_TYPE_MEMBER_ULONGLONG(etag,
                               "etag",
                               "Resource identifier");
      
      PY_TYPE_MEMBER_OBJECT(filter,
                            Filter::Type,
                            "filter",
                            "Search filter options",
                            false);

      PY_TYPE_MEMBER_OBJECT(locale,
                            El::Python::Locale::Type,
                            "locale",
                            "User locale",
                            false);

      PY_TYPE_MEMBER_STRING(msg_id_similar,
                            "msg_id_similar",
                            "If not empty search for messegaes similar to "
                            "specified one is required",
                            true);

      PY_TYPE_MEMBER_STRING(category_locale,
                            "category_locale",
                            "Category path for which to obtain localized info",
                            true);
      
      PY_TYPE_MEMBER_OBJECT(lang,
                            El::Python::Lang::Type,
                            "lang",
                            "UI language",
                            false);

      PY_TYPE_MEMBER_OBJECT(country,
                            El::Python::Country::Type,
                            "country",
                            "User country",
                            false);

      PY_TYPE_MEMBER_STRING(all_option,
                            "all_option",
                            "Text to appear in 'all' option of "
                            "filter selection boxes",
                            true);
       
      PY_TYPE_MEMBER_STRING(event_overlap,
                            "event_overlap",
                            "Event id to measure overlap with",
                            true);
     
      PY_TYPE_MEMBER_STRING(event_split,
                            "event_split",
                            "Message id to view split parts",
                            true);
     
      PY_TYPE_MEMBER_ULONG(event_separate,
                           "event_separate",
                           "Word id to view separation parts");

      PY_TYPE_MEMBER_BOOL(event_narrow_separation,
                          "event_narrow_separation",
                          "Use narrow event separation method");
     
      PY_TYPE_MEMBER_STRING(in_msg_link_class,
                            "in_msg_link_class",
                            "Class of in-message links",
                            true);

      PY_TYPE_MEMBER_STRING(in_msg_link_style,
                            "in_msg_link_style",
                            "Style of in-message links",
                            true);

      PY_TYPE_MEMBER_BOOL(adjust_images,
                          "adjust_images",
                          "Adjust image sizes");
      
      PY_TYPE_MEMBER_BOOL(highlight,
                          "highlight",
                          "Highlight matched words");
      
/*      
      PY_TYPE_MEMBER_BOOL(fill_filter_options,
                          "fill_filter_options",
                          "Fill filter options list");
*/
      PY_TYPE_MEMBER_ULONG(title_format, "title_format", "Title format");
      
      PY_TYPE_MEMBER_ULONG(keywords_format,
                           "keywords_format",
                           "Keywords format");
      
      PY_TYPE_MEMBER_ULONG(description_format,
                           "description_format",
                           "Description format");
      
      PY_TYPE_MEMBER_ULONG(img_alt_format,
                           "img_alt_format",
                           "Image alternate format");
      
      PY_TYPE_MEMBER_ULONG(max_image_count,
                           "max_image_count",
                           "Max image count");
       
      PY_TYPE_MEMBER_BOOL(record_stat,
                          "record_stat",
                          "Record request statistics");

      PY_TYPE_MEMBER_OBJECT(request,
                            El::Python::AnyType,
                            "request",
                            "HTTP request object (used for statistics)",
                            true);

      PY_TYPE_MEMBER_ULONG(max_title_len,
                           "max_title_len",
                           "Max title length");
      
      PY_TYPE_MEMBER_ULONG(max_desc_len,
                           "max_desc_len",
                           "Max description length");
      
      PY_TYPE_MEMBER_ULONG(max_img_alt_len,
                           "max_img_alt_len",
                           "Max image alt length");

      PY_TYPE_MEMBER_ULONG(max_title_from_desc_len,
                           "max_title_from_desc_len",
                           "Max title made from description length");

      PY_TYPE_MEMBER_ULONG(first_column_ad_height,
                           "first_column_ad_height",
                           "Height of ad for the first message column");
      
      PY_TYPE_MEMBER_ULONG(second_column_ad_height,
                           "second_column_ad_height",
                           "Height of ad for the second message column");

      PY_TYPE_MEMBER_STRING(user_agent,
                            "user_agent",
                            "User Agent",
                            true);
      
      PY_TYPE_MEMBER_STRING(translate_def_lang,
                            "translate_def_lang",
                            "Default translation language",
                            true);
      
      PY_TYPE_MEMBER_STRING(translate_lang,
                            "translate_lang",
                            "Translation language",
                            true);
      
      PY_TYPE_MEMBER_STRING(message_view,
                            "message_view",
                            "Message view",
                            true);
      
      PY_TYPE_MEMBER_BOOL(print_left_bar,
                          "print_left_bar",
                          "Print left bar");

      PY_TYPE_MEMBER_ENUM(page_ad_id,
                          Ad::PageIdValue,
                          Ad::PI_COUNT - 1,
                          "page_ad_id",
                          "Page ad id");

      PY_TYPE_STATIC_MEMBER(FF_HTML_, "FF_HTML");
      PY_TYPE_STATIC_MEMBER(FF_WRAP_LINKS_, "FF_WRAP_LINKS");
      PY_TYPE_STATIC_MEMBER(FF_SEGMENTATION_, "FF_SEGMENTATION");
      PY_TYPE_STATIC_MEMBER(FF_FANCY_SEGMENTATION_, "FF_FANCY_SEGMENTATION");

      PY_TYPE_STATIC_MEMBER(GM_ID_, "GM_ID");
      PY_TYPE_STATIC_MEMBER(GM_LINK_, "GM_LINK");
      PY_TYPE_STATIC_MEMBER(GM_TITLE_, "GM_TITLE");
      PY_TYPE_STATIC_MEMBER(GM_DESC_, "GM_DESC");
      PY_TYPE_STATIC_MEMBER(GM_IMG_, "GM_IMG");
      PY_TYPE_STATIC_MEMBER(GM_KEYWORDS_, "GM_KEYWORDS");
      PY_TYPE_STATIC_MEMBER(GM_STAT_, "GM_STAT");
      PY_TYPE_STATIC_MEMBER(GM_PUB_DATE_, "GM_PUB_DATE");
      PY_TYPE_STATIC_MEMBER(GM_VISIT_DATE_, "GM_VISIT_DATE");
      PY_TYPE_STATIC_MEMBER(GM_FETCH_DATE_, "GM_FETCH_DATE");
      PY_TYPE_STATIC_MEMBER(GM_LANG_, "GM_LANG");
      PY_TYPE_STATIC_MEMBER(GM_SOURCE_, "GM_SOURCE");
      PY_TYPE_STATIC_MEMBER(GM_EVENT_, "GM_EVENT");
      PY_TYPE_STATIC_MEMBER(GM_CORE_WORDS_, "GM_CORE_WORDS");
      PY_TYPE_STATIC_MEMBER(GM_DEBUG_INFO_, "GM_DEBUG_INFO");
      PY_TYPE_STATIC_MEMBER(GM_EXTRA_MSG_INFO_, "GM_EXTRA_MSG_INFO");
      PY_TYPE_STATIC_MEMBER(GM_IMG_THUMB_, "GM_IMG_THUMB");
      PY_TYPE_STATIC_MEMBER(GM_CATEGORIES_, "GM_CATEGORIES");
      PY_TYPE_STATIC_MEMBER(GM_ALL_, "GM_ALL");

      PY_TYPE_STATIC_MEMBER(RF_LANG_STAT_, "RF_LANG_STAT");
      PY_TYPE_STATIC_MEMBER(RF_COUNTRY_STAT_, "RF_COUNTRY_STAT");
      PY_TYPE_STATIC_MEMBER(RF_FEED_STAT_, "RF_FEED_STAT");
      PY_TYPE_STATIC_MEMBER(RF_CATEGORY_STAT_, "RF_CATEGORY_STAT");

      PY_TYPE_STATIC_MEMBER(ST_NONE_, "ST_NONE");
      PY_TYPE_STATIC_MEMBER(ST_DUPLICATES_, "ST_DUPLICATES");
      PY_TYPE_STATIC_MEMBER(ST_SIMILAR_, "ST_SIMILAR");
      PY_TYPE_STATIC_MEMBER(ST_COLLAPSE_EVENTS_, "ST_COLLAPSE_EVENTS");      
      PY_TYPE_STATIC_MEMBER(ST_COUNT_, "ST_COUNT");      
      
      PY_TYPE_STATIC_MEMBER(SM_NONE_, "SM_NONE");
      PY_TYPE_STATIC_MEMBER(SM_BY_RELEVANCE_DESC_, "SM_BY_RELEVANCE_DESC");
      PY_TYPE_STATIC_MEMBER(SM_BY_PUB_DATE_DESC_, "SM_BY_PUB_DATE_DESC");
      PY_TYPE_STATIC_MEMBER(SM_BY_PUB_DATE_ASC_, "SM_BY_PUB_DATE_ASC");
      PY_TYPE_STATIC_MEMBER(SM_BY_FETCH_DATE_DESC_, "SM_BY_FETCH_DATE_DESC");
      PY_TYPE_STATIC_MEMBER(SM_BY_FETCH_DATE_ASC_, "SM_BY_FETCH_DATE_ASC");
      PY_TYPE_STATIC_MEMBER(SM_BY_RELEVANCE_ASC_, "SM_BY_RELEVANCE_ASC");
      PY_TYPE_STATIC_MEMBER(SM_BY_EVENT_CAPACITY_DESC_, "SM_BY_EVENT_CAPACITY_DESC");
      PY_TYPE_STATIC_MEMBER(SM_BY_EVENT_CAPACITY_ASC_, "SM_BY_EVENT_CAPACITY_ASC");
      PY_TYPE_STATIC_MEMBER(SM_BY_POPULARITY_DESC_, "SM_BY_POPULARITY_DESC");
      PY_TYPE_STATIC_MEMBER(SM_BY_POPULARITY_ASC_, "SM_BY_POPULARITY_ASC");
      PY_TYPE_STATIC_MEMBER(SM_COUNT_, "SM_COUNT");

    protected:      
      virtual void ready() throw(El::Python::Exception, El::Exception);
      
    private:
      El::Python::Object_var FF_HTML_;
      El::Python::Object_var FF_WRAP_LINKS_;
      El::Python::Object_var FF_SEGMENTATION_;
      El::Python::Object_var FF_FANCY_SEGMENTATION_;
      
      El::Python::Object_var GM_ID_;
      El::Python::Object_var GM_LINK_;
      El::Python::Object_var GM_TITLE_;
      El::Python::Object_var GM_DESC_;
      El::Python::Object_var GM_IMG_;
      El::Python::Object_var GM_KEYWORDS_;
      El::Python::Object_var GM_STAT_;
      El::Python::Object_var GM_PUB_DATE_;
      El::Python::Object_var GM_VISIT_DATE_;
      El::Python::Object_var GM_FETCH_DATE_;
      El::Python::Object_var GM_LANG_;
      El::Python::Object_var GM_SOURCE_;
      El::Python::Object_var GM_EVENT_;
      El::Python::Object_var GM_CORE_WORDS_;
      El::Python::Object_var GM_DEBUG_INFO_;
      El::Python::Object_var GM_EXTRA_MSG_INFO_;
      El::Python::Object_var GM_IMG_THUMB_;
      El::Python::Object_var GM_CATEGORIES_;
      El::Python::Object_var GM_ALL_;
      
      El::Python::Object_var RF_LANG_STAT_;
      El::Python::Object_var RF_COUNTRY_STAT_;
      El::Python::Object_var RF_FEED_STAT_;
      El::Python::Object_var RF_CATEGORY_STAT_;

      El::Python::Object_var ST_NONE_;
      El::Python::Object_var ST_DUPLICATES_;
      El::Python::Object_var ST_SIMILAR_;
      El::Python::Object_var ST_COLLAPSE_EVENTS_;
      El::Python::Object_var ST_COUNT_;

      El::Python::Object_var SM_NONE_;
      El::Python::Object_var SM_BY_RELEVANCE_DESC_;
      El::Python::Object_var SM_BY_RELEVANCE_ASC_;
      El::Python::Object_var SM_BY_PUB_DATE_DESC_;
      El::Python::Object_var SM_BY_PUB_DATE_ASC_;
      El::Python::Object_var SM_BY_FETCH_DATE_DESC_;
      El::Python::Object_var SM_BY_FETCH_DATE_ASC_;
      El::Python::Object_var SM_BY_EVENT_CAPACITY_ASC_;
      El::Python::Object_var SM_BY_EVENT_CAPACITY_DESC_;
      El::Python::Object_var SM_BY_POPULARITY_ASC_;
      El::Python::Object_var SM_BY_POPULARITY_DESC_;
      El::Python::Object_var SM_COUNT_;
    };

    std::string query;
    bool optimize_query;
    bool save_optimized_query;
    std::string search_hint;
    bool adjust_images;
    bool highlight;
    bool in_2_columns;
    std::string protocol;
    unsigned long long gm_flags;
    unsigned long sr_flags;
    unsigned long sorting_type;
    Suppression_var suppression;
    unsigned long start_from;
    unsigned long results_count;
    unsigned long long etag;
    Filter_var filter;
    El::Python::Locale_var locale;
    std::string category_locale;
    std::string msg_id_similar;
    El::Python::Lang_var lang;
    El::Python::Country_var country;
    std::string all_option;
    std::string event_overlap;
    std::string event_split;
    unsigned long event_separate;
    bool event_narrow_separation;
    std::string in_msg_link_class;
    std::string in_msg_link_style;
    unsigned long title_format;
    unsigned long description_format;
    unsigned long img_alt_format;
    unsigned long keywords_format;
    unsigned long max_image_count;
    bool record_stat;
    El::Python::Object_var request;
    unsigned long max_title_len;
    unsigned long max_desc_len;    
    unsigned long max_img_alt_len;
    unsigned long max_title_from_desc_len;
    unsigned long first_column_ad_height;
    unsigned long second_column_ad_height;
    std::string user_agent;
    std::string translate_def_lang;
    std::string translate_lang;
    std::string message_view;
    bool print_left_bar;
    Ad::PageIdValue page_ad_id;
  };

  typedef El::Python::SmartPtr<SearchContext> SearchContext_var;
  
  class SearchMailTime : public El::Python::ObjectImpl,
                         public SearchMailing::Time
  {
  public:
    SearchMailTime(PyTypeObject *type, PyObject *args, PyObject *kwds)
      throw(El::Exception);

    SearchMailTime(uint8_t d, uint16_t t) throw(El::Exception);
    
    virtual ~SearchMailTime() throw() {}

    class Type : public El::Python::ObjectTypeImpl<SearchMailTime,
                                                   SearchMailTime::Type>
    {
    public:
      Type() throw(El::Python::Exception, El::Exception);
      static Type instance;
      
      PY_TYPE_MEMBER_ULONG(time, "time", "Search mail time");
      PY_TYPE_MEMBER_ULONG(day, "day", "Search mail day");
    };
  };

  typedef El::Python::SmartPtr<SearchMailTime> SearchMailTime_var;

  class SearchMailSubscription : public El::Python::ObjectImpl
  {
  public:
    SearchMailSubscription(PyTypeObject *type, PyObject *args, PyObject *kwds)
      throw(El::Exception);
    
    SearchMailSubscription(const SearchMailing::Subscription& src)
      throw(El::Exception);

    virtual ~SearchMailSubscription() throw() {}

    class Type : public El::Python::ObjectTypeImpl<
      SearchMailSubscription,
      SearchMailSubscription::Type>
    {
    public:
      Type() throw(El::Python::Exception, El::Exception);
      static Type instance;
      
      PY_TYPE_MEMBER_OBJECT(times,
                            El::Python::Sequence::Type,
                            "times",
                            "Search mail times",
                            false);
      
      PY_TYPE_MEMBER_STRING(id,
                            "id",
                            "Search mail subscription identifier",
                            true);
      
      PY_TYPE_MEMBER_STRING(email,
                            "email",
                            "Search mail email address",
                            true);
      
      PY_TYPE_MEMBER_ULONG(format,
                           "format",
                           "Search mail format");
      
      PY_TYPE_MEMBER_ULONG(length,
                           "length",
                           "Search mail message count");
      
      PY_TYPE_MEMBER_LONG(time_offset,
                          "time_offset",
                          "Local time zone offset");
      
      PY_TYPE_MEMBER_STRING(title,
                            "title",
                            "Search mail title",
                            true);
      
      PY_TYPE_MEMBER_STRING(query,
                            "query",
                            "Search mail query",
                            true);
      
      PY_TYPE_MEMBER_STRING(modifier,
                            "modifier",
                            "Search mail search modifier",
                            true);
      
      PY_TYPE_MEMBER_STRING(filter,
                            "filter",
                            "Search mail search filter",
                            true);
      
      PY_TYPE_MEMBER_BOOL(status,
                          "status",
                          "Search subscription status");
    };

    El::Python::Sequence_var times;
    std::string id;
    std::string email;
    std::string title;
    std::string query;
    std::string modifier;
    std::string filter;
    unsigned long format;
    unsigned long length;
    long time_offset;
    bool status;
  };

  typedef El::Python::SmartPtr<SearchMailSubscription>
  SearchMailSubscription_var;

  class SearchSyntaxError : public El::Python::ObjectImpl
  {
  public:
    SearchSyntaxError(PyTypeObject *type, PyObject *args, PyObject *kwds)
      throw(El::Exception);
    
    virtual ~SearchSyntaxError() throw(){}

    class Type : public El::Python::ObjectTypeImpl<SearchSyntaxError,
                                                   SearchSyntaxError::Type>
    {
    public:
      Type() throw(El::Python::Exception, El::Exception);
      static Type instance;

      PY_TYPE_MEMBER_STRING(description,
                            "description",
                            "Error description",
                            false);
    
      PY_TYPE_MEMBER_INT(code,
                         "code",
                         "Error code");
      
      PY_TYPE_MEMBER_INT(position,
                         "position",
                         "Error position");
    };
    
    virtual PyObject* str() throw(El::Exception);
    
    std::string description;
    unsigned long code;
    unsigned long position;
  };

  typedef El::Python::SmartPtr<SearchSyntaxError> SearchSyntaxError_var;

  class SearchMailer : public El::Python::ObjectImpl,
                       public SearchMailing::MailManager
  {
  public:

    SearchMailer(PyTypeObject *type, PyObject *args, PyObject *kwds)
      throw(Exception, El::Exception);
    
    SearchMailer(const char* mailer_ref,
                 uint64_t session_timeout,
                 const char* limit_checker_ref,
                 const FraudPrevention::EventLimitCheckDescArray&
                 limit_check_descriptors,
                 CORBA::ORB_ptr orb)
      throw(Exception, El::Exception);
      
    virtual ~SearchMailer() throw();
    
    PyObject* py_get_subscription(PyObject* args) throw(El::Exception);      
    PyObject* py_get_subscriptions(PyObject* args) throw(El::Exception);      
    PyObject* py_update_subscription(PyObject* args) throw(El::Exception);      
    PyObject* py_enable_subscription(PyObject* args) throw(El::Exception);      
    PyObject* py_confirm_operation(PyObject* args) throw(El::Exception);      

    class Type : public El::Python::ObjectTypeImpl<SearchMailer,
                                                   SearchMailer::Type>
    {
    public:
      Type() throw(El::Python::Exception, El::Exception);
      static Type instance;
      
      PY_TYPE_METHOD_VARARGS(py_get_subscription,
                             "get_subscription",
                             "Gets search mailing subscription");
      
      PY_TYPE_METHOD_VARARGS(py_get_subscriptions,
                             "get_subscriptions",
                             "Gets search mailing subscriptions");
      
      PY_TYPE_METHOD_VARARGS(py_update_subscription,
                             "update_subscription",
                             "Adds search mailing subscription");
      
      PY_TYPE_METHOD_VARARGS(py_confirm_operation,
                             "confirm_operation",
                             "Confirm operation identified by token");
      
      PY_TYPE_METHOD_VARARGS(py_enable_subscription,
                             "enable_subscription",
                             "Enables/disables search mailing subscription");

      PY_TYPE_STATIC_MEMBER(SS_ENABLED_, "SS_ENABLED");
      PY_TYPE_STATIC_MEMBER(SS_DISABLED_, "SS_DISABLED");
      PY_TYPE_STATIC_MEMBER(SS_DELETED_, "SS_DELETED");
      
      PY_TYPE_STATIC_MEMBER(CO_NOT_FOUND_, "CO_NOT_FOUND");
      PY_TYPE_STATIC_MEMBER(CO_YES_, "CO_YES");
      PY_TYPE_STATIC_MEMBER(CO_EMAIL_CHANGE_, "CO_EMAIL_CHANGE");

      PY_TYPE_STATIC_MEMBER(ES_YES_, "ES_YES");
      PY_TYPE_STATIC_MEMBER(ES_CHECK_HUMAN_, "ES_CHECK_HUMAN");
      PY_TYPE_STATIC_MEMBER(ES_MAILED_, "ES_MAILED");
      PY_TYPE_STATIC_MEMBER(ES_ALREADY_, "ES_ALREADY");
      PY_TYPE_STATIC_MEMBER(ES_NOT_FOUND_, "ES_NOT_FOUND");
      PY_TYPE_STATIC_MEMBER(ES_LIMIT_EXCEEDED_, "ES_LIMIT_EXCEEDED");

      PY_TYPE_STATIC_MEMBER(US_YES_, "US_YES");
      PY_TYPE_STATIC_MEMBER(US_EMAIL_CHANGE_, "US_EMAIL_CHANGE");
      PY_TYPE_STATIC_MEMBER(US_CHECK_HUMAN_, "US_CHECK_HUMAN");
      PY_TYPE_STATIC_MEMBER(US_MAILED_, "US_MAILED");
      PY_TYPE_STATIC_MEMBER(US_NOT_FOUND_, "US_NOT_FOUND");
      PY_TYPE_STATIC_MEMBER(US_LIMIT_EXCEEDED_, "US_LIMIT_EXCEEDED");      

    private:
      virtual void ready() throw(El::Python::Exception, El::Exception);
      
    private:
      El::Python::Object_var SS_ENABLED_;
      El::Python::Object_var SS_DISABLED_;
      El::Python::Object_var SS_DELETED_;

      El::Python::Object_var CO_NOT_FOUND_;
      El::Python::Object_var CO_YES_;
      El::Python::Object_var CO_EMAIL_CHANGE_;
      
      El::Python::Object_var ES_YES_;
      El::Python::Object_var ES_CHECK_HUMAN_;
      El::Python::Object_var ES_MAILED_;
      El::Python::Object_var ES_ALREADY_;
      El::Python::Object_var ES_NOT_FOUND_;
      El::Python::Object_var ES_LIMIT_EXCEEDED_;

      El::Python::Object_var US_YES_;      
      El::Python::Object_var US_EMAIL_CHANGE_;      
      El::Python::Object_var US_CHECK_HUMAN_;
      El::Python::Object_var US_MAILED_;
      El::Python::Object_var US_NOT_FOUND_;
      El::Python::Object_var US_LIMIT_EXCEEDED_;      
    };
    
  private:

    void get_session_name(const char* email,
                          std::string& session_name) const
      throw(El::Exception);
    
    void get_session(const char* email,
                     El::Apache::Request* ap_request,
                     std::string& session_name,
                     std::string& session) const
      throw(El::Exception);
    
    void update_session(const char* session_name,
                        const char* session,
                        El::Apache::Request* ap_request) const
      throw(El::Exception);
    
  private:
    ACE_Time_Value session_timeout_;
  };
    
  typedef El::Python::SmartPtr<SearchMailer> SearchMailer_var;

  class SearchEngine : public El::Python::ObjectImpl,
                       public El::Service::Callback
  {
  public:
    EL_EXCEPTION(Exception, El::ExceptionBase);
    
  public:
    SearchEngine(PyTypeObject *type, PyObject *args, PyObject *kwds)
      throw(Exception, El::Exception);
    
    SearchEngine(PyObject* args) throw(Exception, El::Exception);
      
    virtual ~SearchEngine() throw();

    void cleanup() throw();

    PyObject* py_search(PyObject* args) throw(El::Exception);      
    PyObject* py_segment_text(PyObject* args) throw(El::Exception);      
    PyObject* py_segment_query(PyObject* args) throw(El::Exception);      
    PyObject* py_relax_query(PyObject* args) throw(El::Exception);      

    class Type : public El::Python::ObjectTypeImpl<SearchEngine,
                                                   SearchEngine::Type>
    {
    public:
      Type() throw(El::Python::Exception, El::Exception);
      static Type instance;
      
      PY_TYPE_METHOD_VARARGS(py_search,
                             "search",
                             "Search for matching posts");
      
      PY_TYPE_METHOD_VARARGS(py_segment_text, "segment_text", "Segment text");
      
      PY_TYPE_METHOD_VARARGS(py_segment_query,
                             "segment_query",
                             "Segment search query");

      PY_TYPE_METHOD_VARARGS(py_relax_query,
                             "relax_query",
                             "Relax search query");

      PY_TYPE_MEMBER_OBJECT(mailer_,
                            SearchMailer::Type,
                            "mailer",
                            "Search mailer",
                            false);      
    };

  private:

    virtual bool notify(El::Service::Event* event) throw(El::Exception);
    
    SearchResult* search(SearchContext& ctx,
                         Message::Transport::StoredMessageArrayPtr& messages,
                         Search::MessageWordPositionMap& positions)
      throw(Search::ParseError, CORBA::Exception, El::Exception);
    
    struct Strategy
    {
      struct Suppress
      {
        struct CoreWords
        {
          uint32_t min;
          uint32_t intersection;
          uint32_t containment_level;

          CoreWords(El::PSP::Config* c) throw(El::Exception);
        };

        CoreWords core_words;

        Suppress(El::PSP::Config* c) throw(El::Exception) : core_words(c) {}
      };
      
      struct Sort
      {
        struct ByRelevance
        {
          uint32_t core_words_prc;
          uint32_t max_core_words;

          ByRelevance(El::PSP::Config* c) throw(El::Exception);
        };
        
        struct ByCapacity
        {
          uint32_t event_max_size;

          ByCapacity(El::PSP::Config* c) throw(El::Exception);
        };

        uint32_t msg_max_age;
        uint32_t impression_respected_level;
        ByRelevance by_relevance;
        ByCapacity by_capacity;

        Sort(El::PSP::Config* c) throw(El::Exception);
      };

      Suppress suppress;
      Sort sort;
      uint32_t msg_per_event;

      Strategy(El::PSP::Config* c) throw(El::Exception);
    };

    struct Debug
    {
      struct Event
      {
        size_t max_size;
        size_t max_message_core_words;
        size_t max_time_range;
        size_t max_strain;
        uint64_t min_rift_time;
        uint64_t merge_max_time_diff;
        size_t merge_level_base;
        size_t merge_level_min;
        float merge_level_size_based_decrement_step;
        float merge_level_time_based_increment_step;
        float merge_level_range_based_increment_step;
        float merge_level_strain_based_increment_step;
        
        Event(El::PSP::Config* c) throw(El::Exception);

        size_t merge_level(size_t event_size,
                           size_t event_strain,
                           uint64_t time_diff,
                           uint64_t time_range) const
          throw();

        size_t merge_level(const ::NewsGate::Event::EventObject& e1,
                           const ::NewsGate::Event::EventObject& e2) const
          throw();

        bool can_merge(const ::NewsGate::Event::EventObject& event) const
          throw();

        bool can_merge(const ::NewsGate::Event::EventObject& e1,
                       const ::NewsGate::Event::EventObject& e2,
                       bool merge_blacklisted,
                       size_t dissenters) const
          throw();
      };
    };
    
    Message::SearchResult* search(
      Search::Expression_var& expression,
      const SearchContext& ctx,
      bool search_words,
      const Strategy& strategy,
      ACE_Time_Value& search_duration)
      throw(CORBA::Exception, El::Exception);
    
    struct ImageExt
    {
      uint16_t orig_width;
      uint16_t orig_height;
      size_t index;
      TextStatusChecker::AltStatus alt_status;

      ImageExt() throw();
    };

    typedef El::LightArray<ImageExt, size_t> ImageExtArray;

    struct MessageExt
    {
      struct Image
      {
        Message::StoredImage img;
        ImageExt ext;
      };
      
      Image front_image;
      ImageExtArray images;
    };
    
    typedef __gnu_cxx::hash_map<Message::Id,
                                MessageExt,
                                Message::MessageIdHash>
    MessageExtMap;
    
    void adjust_images(
      const SearchContext& ctx,
      bool thumbnail_enable,
      unsigned long image_max_width,
      unsigned long image_max_height,
      unsigned long front_image_max_width,
      size_t show_number,
      Message::Transport::StoredMessageArray& messages,
      const Search::MessageWordPositionMap& positions,
      MessageExtMap& msg_extras) const
      throw(El::Exception);

    unsigned long second_column_offset(
      const SearchContext& ctx,
      size_t word_max_len,
      bool show_thumb,
      size_t show_number,
      const Message::Transport::StoredMessageArray& messages,
      const MessageExtMap& msg_extras) const
      throw(El::Exception);

    static unsigned long ad_block_height(unsigned long height)
      throw(El::Exception);

    void fill_event_overlap(const Event::EventObject& event,
                            const Event::EventObject& overlap_event,
                            const Event::Transport::EventRel& event_rel,
                            SearchResult::DebugInfo::EventOverlap& overlap)
      const throw(El::Exception);
    
    void fill_event_split(const Event::Transport::EventParts& src,
                          SearchResult::DebugInfo::EventSplit& dest,
                          bool respect_strain) const
      throw(El::Exception);
    
    static void fill_event_split(SearchResult::DebugInfo::EventSplitPart& part,
                                 const Event::EventObject& event)
      throw(El::Exception);
    
    void copy_messages(const SearchContext& ctx,
                       const Message::Transport::StoredMessageArray& messages,
                       const Search::MessageWordPositionMap& positions,
                       const MessageExtMap& msg_extras,
                       El::Python::Sequence& result_messages) const
      throw(El::Exception);

    SearchResult::Image* create_image(const Message::StoredImage& img,
                                      unsigned long index) const
      throw(El::Exception);
    
    void copy_image(
      const SearchContext& ctx,
      size_t word_max_len,
      const Message::Transport::StoredMessageDebug& msg,
      WordPositionSet& word_pos,
      const Message::StoredImage& img,
      size_t img_index,
      const ImageExt* img_ext,
      const char* thumbnail_url,
      SearchResult::Image& res_img) const
      throw(El::Exception);
    
    void fill_lang_filter_options(const SearchContext& ctx,
                                  Search::LangCounterMap& lang_counter,
                                  El::Python::Sequence& lang_filter_options)
      const throw(Exception, El::Exception);
    
    void fill_country_filter_options(
      const SearchContext& ctx,
      Search::CountryCounterMap& country_counter,
      El::Python::Sequence& country_filter_options)
      const throw(Exception, El::Exception);
    
    void fill_event_debug_info(
      SearchResult::DebugInfo::Event& event_debug_info,
      const Message::Transport::StoredMessageArray& messages,
      const char* event_overlap,
      const char* event_split,
      unsigned long event_separate,
      bool event_narrow_separation,
      Event::EventObject& event)
      throw(CORBA::Exception, Exception, El::Exception);
    
    void fill_feed_filter_options(
      const SearchContext& ctx,
      Search::StringCounterMap& feed_counter,
      El::Python::Sequence& feed_filter_options) const
      throw(Exception, El::Exception);

    void fill_category_stat(Search::StringCounterMap& category_counter,
                            Category& category_stat) const
      throw(Exception, El::Exception);

    void fill_message_debug_info(
      El::Python::Map& messages_debug_info,
      const Message::Transport::StoredMessageArray& messages,
      const Event::EventObject& event) const
      throw(CORBA::Exception, Exception, El::Exception);
    
    El::Python::Sequence* core_words(const Message::StoredMessage& msg) const
      throw(Exception, El::Exception);

    void log_stat(const SearchContext& ctx,
                  SearchResult* result,
                  const Message::SearchResult* search_result,
                  const char* optimized_query,
                  const ACE_Time_Value& search_duration,
                  const ACE_Time_Value& request_duration) const
      throw(El::Exception);

    Ad::SelectionResult* select_ads(
      SearchContext& ctx,
      const Message::Transport::StoredMessageArray& messages,
      const Search::Condition* condition) const
    throw(El::Exception);
    
    class UINT32_ToStringMap : 
      public google::dense_hash_map<uint32_t,
                                    const char*,
                                    El::Hash::Numeric<uint32_t> >
    {
    public:
      UINT32_ToStringMap() throw(El::Exception);
    };

    static void fill_word_id_to_word(const Message::StoredMessage& msg,
                                     UINT32_ToStringMap& id_to_text)
      throw(Exception, El::Exception);

    static size_t text_with_image_lines(size_t len,
                                        size_t img_width,
                                        size_t img_height)
      throw();

    static bool image_lines(const Message::StoredMessage& msg,
                            const Message::StoredImageArray& images,
                            size_t index,
                            bool highlighted,
                            const MessageExt* msg_ext,
                            bool show_thumb,
                            size_t word_max_len,
                            unsigned long& lines,
                            size_t& to_show_number)
      throw(El::Exception);

    Message::BankClientSession* bank_client_session()
      throw(Exception, El::Exception);
    
    Event::BankClientSession* event_bank_client_session()
      throw(Exception, El::Exception);
    
  private:

    El::PSP::Config_var config_;
    El::Logging::Python::Logger_var logger_;
    El::Corba::OrbAdapter* orb_adapter_;
    CORBA::ORB_var orb_;
    std::auto_ptr<Strategy> strategy_;
    std::auto_ptr<Debug::Event> debug_event_;
    SearchMailer_var mailer_;

    typedef El::Corba::SmartRef<Dictionary::WordManager> WordManagerRef;
    WordManagerRef word_manager_;
    
    typedef El::Corba::SmartRef<Segmentation::Segmentor> SegmentorRef;
    SegmentorRef segmentor_;
    
    typedef El::Corba::SmartRef<Event::BankManager> EventBankManagerRef;
    EventBankManagerRef event_bank_manager_;
    
    typedef El::Corba::SmartRef<Message::BankManager> MessageBankManagerRef;
    MessageBankManagerRef message_bank_manager_;
    
    Event::BankClientSessionImpl_var event_bank_client_session_;
    Message::BankClientSessionImpl_var bank_client_session_;

    typedef El::Corba::SmartRef<Ad::AdServer> AdServerRef;
    AdServerRef ad_server_;

    Statistics::StatLogger_var stat_logger_;
    char hostname_[257];

    typedef ACE_RW_Thread_Mutex Mutex;
    typedef ACE_Read_Guard<Mutex> ReadGuard;
    typedef ACE_Write_Guard<Mutex> WriteGuard;

    Mutex lock_;

    unsigned long message_bank_clients_;
    unsigned long message_bank_clients_max_threads_;
    unsigned long long search_counter_;
    El::Stat::TimeMeter py_search_meter_;
    El::Stat::TimeMeter search_meter_;
  };
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace NewsGate
{
  //
  // NewsGate::SearchMailer::Type class
  //
  inline
  SearchMailer::Type::Type() throw(El::Python::Exception, El::Exception)
      : El::Python::ObjectTypeImpl<SearchMailer, SearchMailer::Type>(
        "newsgate.search.SearchMailer",
        "Object search mailing management functionality")
  {
    tp_new = 0;
  }

  //
  // NewsGate::SearchEngine::Type class
  //
  inline
  SearchEngine::Type::Type() throw(El::Python::Exception, El::Exception)
      : El::Python::ObjectTypeImpl<SearchEngine, SearchEngine::Type>(
        "newsgate.search.SearchEngine",
        "Object providing post search functionality")
  {
    tp_new = 0;
  }

  //
  // SearchEngine::UINT32_ToStringMap class
  //
  inline
  SearchEngine::UINT32_ToStringMap::UINT32_ToStringMap() throw(El::Exception)
  {
    // This class is of very limited usage - maps word ids to word and
    // word position to words - the empty key choice is very specific
    set_empty_key((uint32_t)0x80000000 - 1);
  }

  //
  // SearchEngine::ImageExt class
  //
  inline
  SearchEngine::ImageExt::ImageExt() throw()
      : orig_width(0),
        orig_height(0),
        index(0),
        alt_status(TextStatusChecker::AS_EMPTY)
  {
  }

  //
  // NewsGate::SearchContext class
  //
  inline
  SearchContext::SearchContext(PyTypeObject *type,
                               PyObject *args,
                               PyObject *kwds)
    throw(El::Exception)
      : El::Python::ObjectImpl(type ? type : &SearchContext::Type::instance),
        optimize_query(true),
        save_optimized_query(false),
        adjust_images(false),
        highlight(false),
        in_2_columns(false),
        gm_flags(0),
        sr_flags(0),
        sorting_type(Search::Strategy::SM_BY_RELEVANCE_DESC),
        suppression(new Suppression(&Suppression::Type::instance, 0, 0)),
        start_from(0),
        results_count(0),
        etag(0),
        filter(new Filter(&Filter::Type::instance, 0, 0)),
        locale(new El::Python::Locale()),
        lang(new El::Python::Lang()),
        country(new El::Python::Country()),
        event_separate(0),
        event_narrow_separation(false),
        in_msg_link_class("in_msg_link"),
        title_format(FF_HTML),
        description_format(FF_HTML | FF_WRAP_LINKS),
        img_alt_format(FF_HTML | FF_WRAP_LINKS),
        keywords_format(FF_HTML),
        max_image_count(ULONG_MAX),
        record_stat(false),
        request(El::Python::add_ref(Py_None)),
        max_title_len(0),
        max_desc_len(0),
        max_img_alt_len(0),
        max_title_from_desc_len(0),
        first_column_ad_height(0),
        second_column_ad_height(0),
        print_left_bar(false),
        page_ad_id(Ad::PI_UNKNOWN)
  {
  }

  //
  // NewsGate::SearchContext::Filter class
  //
  inline
  SearchContext::Filter::Filter(PyTypeObject *type,
                                PyObject *args,
                                PyObject *kwds)
    throw(El::Exception)
      : El::Python::ObjectImpl(type),
        lang(new El::Python::Lang()),
        country(new El::Python::Country()),
        event(0)
  {
  }

  //
  // NewsGate::SearchContext::Suppression class
  //
  inline
  SearchContext::Suppression::Suppression(PyTypeObject *tp,
                                          PyObject *args,
                                          PyObject *kwds)
    throw(El::Exception)
      : El::Python::ObjectImpl(tp),
        type(Search::Strategy::ST_NONE),
        core_words(new CoreWords(&CoreWords::Type::instance, 0, 0))
  {
  }

  //
  // NewsGate::SearchContext::Suppression class
  //
  inline
  SearchContext::Suppression::CoreWords::CoreWords(PyTypeObject *type,
                                                   PyObject *args,
                                                   PyObject *kwds)
    throw(El::Exception)
      : El::Python::ObjectImpl(type),
        intersection(ULONG_MAX),
        containment_level(ULONG_MAX),
        min_count(ULONG_MAX)
  {
  }
  
  //
  // NewsGate::SearchMailTime class
  //
  inline
  SearchMailTime::SearchMailTime(PyTypeObject *type,
                                 PyObject *args,
                                 PyObject *kwds)
      throw(El::Exception)
      : El::Python::ObjectImpl(type ? type : &SearchMailTime::Type::instance)
  {
  }

  inline
  SearchMailTime::SearchMailTime(uint8_t d, uint16_t t) throw(El::Exception)
      : El::Python::ObjectImpl(&SearchMailTime::Type::instance),
        SearchMailing::Time(d, t)
  {
  }  
  
  //
  // NewsGate::SearchMailSubscription class
  //
  inline
  SearchMailSubscription::SearchMailSubscription(PyTypeObject *type,
                                                 PyObject *args,
                                                 PyObject *kwds)
      throw(El::Exception)
      : El::Python::ObjectImpl(type ? type :
                               &SearchMailSubscription::Type::instance),
        times(new El::Python::Sequence()),
        format(0),
        length(0),
        time_offset(0),
        status(true)
  {
  }
  
  //
  // NewsGate::SearchContext::Type class
  //
  inline
  SearchContext::Type::Type() throw(El::Python::Exception, El::Exception)
      : El::Python::ObjectTypeImpl<SearchContext, SearchContext::Type>(
        "newsgate.search.SearchContext",
        "Object aggregating search parameters")
  {
  }
  
  //
  // NewsGate::SearchContext::Filter::Type class
  //
  inline
  SearchContext::Filter::Type::Type()
    throw(El::Python::Exception, El::Exception)
      : El::Python::ObjectTypeImpl<Filter, Filter::Type>(
        "newsgate.search.SearchFilter",
        "Object aggregating search filtering parameters")
  {
  }  

  //
  // NewsGate::SearchContext::Suppression::Type class
  //
  inline
  SearchContext::Suppression::Type::Type()
    throw(El::Python::Exception, El::Exception)
      : El::Python::ObjectTypeImpl<Suppression, Suppression::Type>(
        "newsgate.search.SearchSuppression",
        "Object aggregating search suppression parameters")
  {
  }

  //
  // NewsGate::SearchMailTime::Type class
  //
  inline
  SearchMailTime::Type::Type()
      throw(El::Python::Exception, El::Exception)
      : El::Python::ObjectTypeImpl<SearchMailTime, SearchMailTime::Type>(
        "newsgate.search.SearchMailTime",
        "Object aggregating search time params")
  {
  }

  //
  // NewsGate::SearchMailSubscription::Type class
  //
  inline
  SearchMailSubscription::Type::Type()
      throw(El::Python::Exception, El::Exception)
      : El::Python::ObjectTypeImpl<SearchMailSubscription,
                                   SearchMailSubscription::Type>(
        "newsgate.search.SearchMailSubscription",
        "Object aggregating search parameters")
  {
  }
  
  //
  // NewsGate::SearchContext::Suppression::CoreWords::Type class
  //
  inline
  SearchContext::Suppression::CoreWords::Type::Type()
    throw(El::Python::Exception, El::Exception)
      : El::Python::ObjectTypeImpl<Suppression::CoreWords,
                                   Suppression::CoreWords::Type>(
        "newsgate.search.SearchSuppressionCoreWords",
        "Object aggregating search suppression core words related  parameters")
  {
  }

  //
  // NewsGate::SearchResult::DebugInfo class
  //
  inline
  SearchResult::DebugInfo::DebugInfo(PyTypeObject *type,
                                     PyObject *args,
                                     PyObject *kwds)
    throw(El::Exception)
      : El::Python::ObjectImpl(type),
        event(new Event(&Event::Type::instance, 0, 0)),
        messages(new El::Python::Map(&El::Python::Map::Type::instance, 0, 0))
  {
  }
  
  //
  // NewsGate::SearchResult::DebugInfo::Type class
  //
  inline
  SearchResult::DebugInfo::Type::Type()
    throw(El::Python::Exception, El::Exception)
      : El::Python::ObjectTypeImpl<DebugInfo, DebugInfo::Type>(
        "newsgate.search.SearchDebugInfo",
        "Object aggregating search result debug info")
  {
  }
  
  //
  // NewsGate::SearchResult::DebugInfo::Word class
  //
  inline
  SearchResult::DebugInfo::Word::Word(PyTypeObject *type,
                                      PyObject *args,
                                      PyObject *kwds)
    throw(El::Exception)
      : El::Python::ObjectImpl(type),
        id(0),
        remarkable(false),
        unknown(true),
        weight(0),
        cw_weight(0),
        wp_weight(0),
        position(0),
        lang(new El::Python::Lang())
  {
  }
  
  //
  // NewsGate::SearchResult::DebugInfo::Word::Type class
  //
  inline
  SearchResult::DebugInfo::Word::Type::Type()
    throw(El::Python::Exception, El::Exception)
      : El::Python::ObjectTypeImpl<Word, Word::Type>(
        "newsgate.search.SearchDebugInfoWord",
        "Object aggregating search result word-level debug info")
  {
  }
  
  //
  // NewsGate::SearchResult::DebugInfo::Event class
  //
  inline
  SearchResult::DebugInfo::Event::Event(PyTypeObject *type,
                                        PyObject *args,
                                        PyObject *kwds)
    throw(El::Exception)
      : El::Python::ObjectImpl(type),
        hash(0),
        lang(new El::Python::Lang()),
        overlap(new EventOverlap(&EventOverlap::Type::instance, 0, 0)),
        split(new EventSplit(&EventSplit::Type::instance, 0, 0)),
        separate(new EventSplit(&EventSplit::Type::instance, 0, 0)),
        changed(false),
        dissenters(0),
        strain(0),
        size(0),
        time_range(0),
        published_min(0),
        published_max(0),
        can_merge(false),
        merge_level(0),
        words(new El::Python::Sequence())
  {
  }

  //
  // NewsGate::SearchResult::DebugInfo::Event::Type class
  //
  inline
  SearchResult::DebugInfo::Event::Type::Type()
    throw(El::Python::Exception, El::Exception)
      : El::Python::ObjectTypeImpl<Event, Event::Type>(
        "newsgate.search.SearchDebugInfoEvent",
        "Object aggregating search result event-level debug info")
  {
  }
  
  //
  // NewsGate::SearchResult::DebugInfo::EventOverlap class
  //
  inline
  SearchResult::DebugInfo::EventOverlap::EventOverlap(PyTypeObject *type,
                                                      PyObject *args,
                                                      PyObject *kwds)
    throw(El::Exception)
      : El::Python::ObjectImpl(type),
        level(101),
        common_words(0),
        time_diff(0),
        merge_level(0),
        size(0),
        strain(0),
        dissenters(0),
        time_range(0),
        colocated(false),
        same_lang(false),
        can_merge(false),
        merge_blacklist_timeout(0)
  {
  }
  
  //
  // NewsGate::SearchResult::DebugInfo::EventOverlap::Type class
  //
  inline
  SearchResult::DebugInfo::EventOverlap::Type::Type()
    throw(El::Python::Exception, El::Exception)
      : El::Python::ObjectTypeImpl<EventOverlap, EventOverlap::Type>(
        "newsgate.search.SearchDebugInfoEventOverlap",
        "Object aggregating search result event overlapping debug info")
  {
  }  
  
  //
  // NewsGate::SearchResult::DebugInfo::EventSplitPart class
  //
  inline
  SearchResult::DebugInfo::EventSplitPart::EventSplitPart(PyTypeObject *type,
                                                          PyObject *args,
                                                          PyObject *kwds)
    throw(El::Exception)
      : El::Python::ObjectImpl(type),
        dissenters(0),
        strain(0),
        size(0),
        time_range(0),
        published_min(0),
        published_max(0)
  {
  }
  
  //
  // NewsGate::SearchResult::DebugInfo::EventSplitPart::Type class
  //
  inline
  SearchResult::DebugInfo::EventSplitPart::Type::Type()
    throw(El::Python::Exception, El::Exception)
      : El::Python::ObjectTypeImpl<EventSplitPart, EventSplitPart::Type>(
        "newsgate.search.SearchDebugInfoEventSplitPart",
        "Object aggregating search result event split part debug info")
  {
  }
  
  //
  // NewsGate::SearchResult::DebugInfo::EventSplit class
  //
  inline
  SearchResult::DebugInfo::EventSplit::EventSplit(PyTypeObject *type,
                                                  PyObject *args,
                                                  PyObject *kwds)
    throw(El::Exception)
      : El::Python::ObjectImpl(type),
        overlap_level(0),
        merge_level(0),
        common_words(0),
        time_diff(0),
        part1(new EventSplitPart(&EventSplitPart::Type::instance, 0, 0)),
        part2(new EventSplitPart(&EventSplitPart::Type::instance, 0, 0)),
        overlap1(new EventOverlap(&EventOverlap::Type::instance, 0, 0)),
        overlap2(new EventOverlap(&EventOverlap::Type::instance, 0, 0))

  {
  }
  
  //
  // NewsGate::SearchResult::DebugInfo::EventSplit::Type class
  //
  inline
  SearchResult::DebugInfo::EventSplit::Type::Type()
    throw(El::Python::Exception, El::Exception)
      : El::Python::ObjectTypeImpl<EventSplit, EventSplit::Type>(
        "newsgate.search.SearchDebugInfoEventSplit",
        "Object aggregating search result event split debug info")
  {
  }
  
  //
  // NewsGate::SearchResult::DebugInfo::Message class
  //
  inline
  SearchResult::DebugInfo::Message::Message(PyTypeObject *type,
                                            PyObject *args,
                                            PyObject *kwds)
    throw(El::Exception)
      : El::Python::ObjectImpl(type),
        signature(0),
        url_signature(0),
        match_weight(0),
        feed_impressions(0),
        feed_clicks(0),
        core_words_in_event(0),
        common_event_words(0),
        event_overlap(0),
        event_merge_level(0),
        core_words(new El::Python::Sequence()),
        words(new El::Python::Sequence())
  {
  }
  
  //
  // NewsGate::SearchResult::DebugInfo::Message::Type class
  //
  inline
  SearchResult::DebugInfo::Message::Type::Type()
    throw(El::Python::Exception, El::Exception)
      : El::Python::ObjectTypeImpl<Message, Message::Type>(
        "newsgate.search.SearchDebugInfoMessage",
        "Object aggregating search result message-level debug info")
  {
  }
  
  //
  // NewsGate::SearchResult::Message class
  //
  inline
  SearchResult::Message::Message(PyTypeObject *type,
                                 PyObject *args,
                                 PyObject *kwds)
    throw(El::Exception)
      : El::Python::ObjectImpl(type),
        images(new El::Python::Sequence()),
        event_capacity(0),
        impressions(0),
        clicks(0),
        feed_impressions(0),
        feed_clicks(0),
        published(0),
        visited(0),
        fetched(0),
        source_id(0),
        lang(new El::Python::Lang()),
        country(new El::Python::Country()),
        core_words(new El::Python::Sequence()),
        categories(new El::Python::Sequence()),
        front_image(El::Python::add_ref(Py_None))
  {
  }
  
  //
  // NewsGate::SearchResult::Message::Type class
  //
  inline
  SearchResult::Message::Type::Type()
    throw(El::Python::Exception, El::Exception)
      : El::Python::ObjectTypeImpl<Message, Message::Type>(
        "newsgate.search.Message",
        "Object representing message post")
  {
  }
  
  //
  // SearchResult::Message::CoreWord class
  //
  inline
  SearchResult::Message::CoreWord::CoreWord(PyTypeObject *type,
                                            PyObject *args,
                                            PyObject *kwds)
    throw(El::Exception)
      : El::Python::ObjectImpl(type),
        id(0),
        weight(0)
  {
  }
  
  //
  // SearchResult::Message::CoreWord::Type class
  //
  inline
  SearchResult::Message::CoreWord::Type::Type()
    throw(El::Python::Exception, El::Exception)
      : El::Python::ObjectTypeImpl<CoreWord, CoreWord::Type>(
        "newsgate.search.MessageCoreWord",
        "Object aggregating search result message core word-level info")
  {
  }
  
  //
  // NewsGate::SearchResult::Option class
  //
  inline
  SearchResult::Option::Option(PyTypeObject *type,
                               PyObject *args,
                               PyObject *kwds)
    throw(El::Exception)
      : El::Python::ObjectImpl(type),
        count(0),
        selected(false)
  {
  }

  inline
  SearchResult::Option::Option(const wchar_t* id_val,
                               unsigned long count_val,
                               const wchar_t* val,
                               bool selected_val)
    throw(El::Exception)
      : El::Python::ObjectImpl(&SearchResult::Option::Type::instance),
        id(id_val),
        count(count_val),
        value(val),
        selected(selected_val)
  {
  }

  //
  // NewsGate::SearchResult::Option::Type class
  //
  inline
  SearchResult::Option::Type::Type()
    throw(El::Python::Exception, El::Exception)
      : El::Python::ObjectTypeImpl<Option, Option::Type>(
        "newsgate.search.Option",
        "Object representing an option")
  {
  }
  
  //
  // NewsGate::SearchResult::Image class
  //
  inline
  SearchResult::Image::Image(PyTypeObject *type,
                             PyObject *args,
                             PyObject *kwds)
    throw(El::Exception)
      : El::Python::ObjectImpl(type),
        width(0),
        height(0),
        orig_width(0),
        orig_height(0),
        thumbs(new El::Python::Sequence()),
        alt_highlighted(false),
        index(0)
  {
  }
  
  //
  // NewsGate::SearchResult::Image::Type class
  //
  inline
  SearchResult::Image::Type::Type()
    throw(El::Python::Exception, El::Exception)
      : El::Python::ObjectTypeImpl<Image, Image::Type>(
        "newsgate.search.Image",
        "Object representing image")
  {
  }
  
  //
  // NewsGate::SearchResult::ImageThumb class
  //
  inline
  SearchResult::ImageThumb::ImageThumb(PyTypeObject *type,
                                       PyObject *args,
                                       PyObject *kwds)
    throw(El::Exception)
      : El::Python::ObjectImpl(type),
        width(0),
        height(0),
        cropped(false)
  {
  }
  
  //
  // NewsGate::SearchResult::ImageThumb::Type class
  //
  inline
  SearchResult::ImageThumb::Type::Type()
    throw(El::Python::Exception, El::Exception)
      : El::Python::ObjectTypeImpl<ImageThumb, ImageThumb::Type>(
        "newsgate.search.ImageThumb",
        "Object representing image thumbnail")
  {
  }
  
  //
  // NewsGate::SearchResult class
  //
  inline
  SearchResult::SearchResult(PyTypeObject *type,
                             PyObject *args,
                             PyObject *kwds)
    throw(El::Exception)
      : El::Python::ObjectImpl(type),
        nochanges(false),
        total_matched_messages(0),
        suppressed_messages(0),
//        space_filtered(0),
        message_load_status(MLS_UNKNOWN),
//        search_time(0),
        etag(0),
        messages(new El::Python::Sequence()),
        second_column_offset(0),
        lang_filter_options(new El::Python::Sequence()),
        country_filter_options(new El::Python::Sequence()),
        feed_filter_options(new El::Python::Sequence()),
        category_stat(new Category()),
        ad_selection(new Ad::Python::SelectionResult()),
        category_locale(new CategoryLocale())
         /*, space_filter_options(new El::Python::Sequence())*/
  {
  }
 
  //
  // NewsGate::SearchResult::Type class
  //
  inline
  SearchResult::Type::Type()
    throw(El::Python::Exception, El::Exception)
      : El::Python::ObjectTypeImpl<SearchResult, SearchResult::Type>(
        "newsgate.search.SearchResult",
        "Object representing search result")
  {
  }
  
  //
  // NewsGate::SearchSyntaxError class
  //
  inline
  SearchSyntaxError::SearchSyntaxError(PyTypeObject *type,
                                       PyObject *args,
                                       PyObject *kwds)
    throw(El::Exception) : El::Python::ObjectImpl(type)
  {
  }

  inline
  PyObject*
  SearchSyntaxError::str() throw(El::Exception)
  {
    return PyString_FromString(description.c_str());
  }
  
  //
  // NewsGate::SearchSyntaxError::Type class
  //
  inline
  SearchSyntaxError::Type::Type() throw(El::Python::Exception, El::Exception)
      : El::Python::ObjectTypeImpl<SearchSyntaxError, SearchSyntaxError::Type>(
        "newsgate.search.SyntaxError",
        "Object representing search expression syntax error")
  {
  }
  
  //
  // NewsGate::Category class
  //
  inline
  Category::Category(PyTypeObject *type, PyObject *args, PyObject *kwds)
    throw(El::Exception) :
      El::Python::ObjectImpl(type ? type : &Type::instance),
      id(0),
      parent(0),
      message_count(0),
      matched_message_count(0),
      categories(new El::Python::Sequence())
  {
  }

  //
  // NewsGate::Category::Type class
  //
  inline
  Category::Type::Type() throw(El::Python::Exception, El::Exception)
      : El::Python::ObjectTypeImpl<Category, Category::Type>(
        "newsgate.search.Category",
        "Object representing search category info")
  {
  }
  
  //
  // NewsGate::CategoryLocale class
  //
  inline
  CategoryLocale::CategoryLocale(PyTypeObject *type,
                                 PyObject *args,
                                 PyObject *kwds)
    throw(El::Exception) :
      El::Python::ObjectImpl(type ? type : &Type::instance)
  {
  }

  inline
  CategoryLocale&
  CategoryLocale::operator=(const Message::Categorizer::Category::Locale& val)
    throw(El::Exception)
  {
    name = val.name.c_str() ? val.name.c_str() : "";
    title = val.title.c_str() ? val.title.c_str() : "";
    short_title = val.short_title.c_str() ? val.short_title.c_str() : "";
    description = val.description.c_str() ? val.description.c_str() : "";
    keywords = val.keywords.c_str() ? val.keywords.c_str() : "";
    return *this;
  }
  
  //
  // NewsGate::CategoryLocale::Type class
  //
  inline
  CategoryLocale::Type::Type() throw(El::Python::Exception, El::Exception)
      : El::Python::ObjectTypeImpl<CategoryLocale, CategoryLocale::Type>(
        "newsgate.search.CategoryLocale",
        "Object representing category localized info")
  {
  }
}
  
#endif // _NEWSGATE_SERVER_FRONTENDS_SEARCHENGINE_SEARCHENGINE_HPP_
