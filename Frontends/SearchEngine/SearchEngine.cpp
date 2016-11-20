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

#include <unistd.h>

#include <iostream>
#include <string>
#include <sstream>
#include <utility>
#include <vector>

#include <El/Exception.hpp>
#include <El/ArrayPtr.hpp>
#include <El/String/Manip.hpp>
#include <El/Localization/Loc.hpp>
#include <El/Dictionary/Morphology.hpp>
#include <El/Guid.hpp>
#include <El/Stat.hpp>

#include <El/Net/HTTP/Headers.hpp>
#include <El/Net/HTTP/URL.hpp>
#include <El/Net/HTTP/Utility.hpp>

#include <El/Python/Object.hpp>
#include <El/Python/Module.hpp>
#include <El/Python/Utility.hpp>
#include <El/Python/RefCount.hpp>

#include <El/PSP/Config.hpp>
#include <El/PSP/Request.hpp>

#include <Commons/Message/Message.hpp>

#include <Commons/Message/TransportImpl.hpp>
#include <Commons/Event/TransportImpl.hpp>
#include <Commons/Search/TransportImpl.hpp>
#include <Commons/Search/SearchExpression.hpp>
#include <Commons/Ad/Ad.hpp>

#include <Services/Commons/Message/MessageServices.hpp>
#include <Services/Commons/Message/BankClientSessionImpl.hpp>

#include <Services/Dictionary/Commons/TransportImpl.hpp>
#include <Services/Dictionary/Commons/DictionaryServices.hpp>

#include <Services/Commons/Statistics/StatLogger.hpp>
#include <Services/Commons/FraudPrevention/LimitCheck.hpp>

#include <Services/Commons/Ad/AdServices.hpp>
#include <Services/Commons/Ad/TransportImpl.hpp>

#include "SearchEngine.hpp"

//#define SEARCH_PROFILING 100

const size_t LINE_TITLE_CHARS = 60;
const size_t LINE_DESC_CHARS = 75;
const size_t DESC_CHAR_WIDTH = 8;
const size_t DESC_CHAR_HEIGHT = 17;

namespace NewsGate 
{
  SearchMailer::Type SearchMailer::Type::instance;
  SearchEngine::Type SearchEngine::Type::instance;

  SearchContext::Type SearchContext::Type::instance;
  SearchContext::Filter::Type SearchContext::Filter::Type::instance;

  SearchMailTime::Type SearchMailTime::Type::instance;
  SearchMailSubscription::Type SearchMailSubscription::Type::instance;
  
  SearchContext::Suppression::Type SearchContext::Suppression::Type::instance;
  
  SearchContext::Suppression::CoreWords::Type
  SearchContext::Suppression::CoreWords::Type::instance;

  SearchResult::DebugInfo::Type SearchResult::DebugInfo::Type::instance;
  
  SearchResult::DebugInfo::Word::Type
    SearchResult::DebugInfo::Word::Type::instance;

  SearchResult::DebugInfo::Event::Type
    SearchResult::DebugInfo::Event::Type::instance;
  
  SearchResult::DebugInfo::EventOverlap::Type
    SearchResult::DebugInfo::EventOverlap::Type::instance;
  
  SearchResult::DebugInfo::EventSplitPart::Type
    SearchResult::DebugInfo::EventSplitPart::Type::instance;
  
  SearchResult::DebugInfo::EventSplit::Type
    SearchResult::DebugInfo::EventSplit::Type::instance;
  
  SearchResult::DebugInfo::Message::Type
    SearchResult::DebugInfo::Message::Type::instance;
  
  SearchResult::Image::Type SearchResult::Image::Type::instance;
  SearchResult::ImageThumb::Type SearchResult::ImageThumb::Type::instance;
  SearchResult::Message::Type SearchResult::Message::Type::instance;
  
  SearchResult::Message::CoreWord::Type
  SearchResult::Message::CoreWord::Type::instance;
  
  SearchResult::Option::Type SearchResult::Option::Type::instance;
  SearchResult::Type SearchResult::Type::instance;
  SearchSyntaxError::Type SearchSyntaxError::Type::instance;
  Category::Type Category::Type::instance;
  CategoryLocale::Type CategoryLocale::Type::instance;

  static const char ASPECT[] = "SearchEngine";
  
  // rl, nh, st - already taken
  static const char* INFORMER_PARAMS[] =
  {
    "iv",
    "it",
    "vn",
    "iw",
    "is",
    "ip",
    "tl",
    "md",
    "sc",
    "mc",
    "sm",
    "gc",
    "dc",
    "rc",
    "uc",
    "tc",
    "ac",
    "mr",
    "sl",
    "mm",
    "ml",
    "lc",
    "oc",
    "cl",
    "cc",
    "ts",
    "as",
    "ms",
    "cs",
    "tf",
    "af",
    "aa",
    "mf",
    "cf",
    "sp",
    "ct",
    "cp",
    "cl",
    "wp",
    "at",
    ""
  };
  
  //
  // NewsGate::SearchPyModule class
  //
  class SearchPyModule : public El::Python::ModuleImpl<SearchPyModule>
  {
  public:
    static SearchPyModule instance;

    SearchPyModule() throw(El::Exception);

    virtual void initialized() throw(El::Exception);

    PyObject* py_create_engine(PyObject* args) throw(El::Exception);
    PyObject* py_cleanup_engine(PyObject* args) throw(El::Exception);
  
    PY_MODULE_METHOD_VARARGS(
      py_create_engine,
      "create_engine",
      "Creates SearchEngine object");  

    PY_MODULE_METHOD_VARARGS(
      py_cleanup_engine,
      "cleanup_engine",
      "Cleanups SearchEngine object");

    El::Python::Object_var not_ready_ex;
  };

  //
  // SearchEngine::Strategy structs
  //

  SearchEngine::Strategy::Strategy(El::PSP::Config* c) throw(El::Exception)
      : suppress(c),
        sort(c),
        msg_per_event(1)
  {
  }

  SearchEngine::Strategy::Suppress::CoreWords::CoreWords(El::PSP::Config* c)
    throw(El::Exception)
  {
    El::PSP::Config_var conf = c->config("suppress.core_words");
    
    min = conf->number("min");
    intersection = conf->number("intersection");
    containment_level = conf->number("containment_level");
  }

  SearchEngine::Strategy::Sort::Sort(El::PSP::Config* c) throw(El::Exception)
      : by_relevance(c),
        by_capacity(c)
  {
    El::PSP::Config_var conf = c->config("sort");
    
    msg_max_age = conf->number("msg_max_age");
    impression_respected_level = conf->number("impression_respected_level");
  }

  SearchEngine::Strategy::Sort::ByRelevance::ByRelevance(El::PSP::Config* c)
    throw(El::Exception)
  {
    El::PSP::Config_var conf = c->config("sort.by_relevance");
    core_words_prc = conf->number("core_words_prc");
    max_core_words = conf->number("max_core_words");
  }
  
  SearchEngine::Strategy::Sort::ByCapacity::ByCapacity(El::PSP::Config* c)
    throw(El::Exception)
  {
    El::PSP::Config_var conf = c->config("sort.by_capacity");    
    event_max_size = conf->number("event_max_size");
  }

  //
  // SearchEngine::Debug::Event structs
  //
        
  SearchEngine::Debug::Event::Event(El::PSP::Config* c) throw(El::Exception)
  {
    El::PSP::Config_var conf = c->config("debug.event");
    max_size = conf->number("max_size");
    max_message_core_words = conf->number("max_message_core_words");
    max_time_range = conf->number("max_time_range");
    min_rift_time = conf->number("min_rift_time");

    conf = conf->config("merge");
    merge_max_time_diff = conf->number("max_time_diff");
    max_strain = conf->number("max_strain");

    conf = conf->config("level");
    merge_level_base = conf->number("base");
    merge_level_min = conf->number("min");
    
    merge_level_time_based_increment_step =
      conf->double_number("time_based_increment_step") / 86400;
    
    merge_level_range_based_increment_step =
      conf->double_number("range_based_increment_step") / 86400;

    merge_level_strain_based_increment_step =
      conf->double_number("strain_based_increment_step");
    
    merge_level_size_based_decrement_step =
      conf->double_number("size_based_decrement_step");
  }

  size_t
  SearchEngine::Debug::Event::merge_level(size_t event_size,
                                          size_t event_strain,
                                          uint64_t time_diff,
                                          uint64_t time_range) const
    throw()
  {
    return ::NewsGate::Event::EventObject::event_merge_level(
      event_size,
      event_strain,
      time_diff,
      time_range,
      merge_level_base,
      merge_level_min,
      min_rift_time,
      merge_level_size_based_decrement_step,
      merge_level_time_based_increment_step,
      merge_level_range_based_increment_step,
      merge_level_strain_based_increment_step);
  }
  
  size_t
  SearchEngine::Debug::Event::merge_level(
    const ::NewsGate::Event::EventObject& e1,
    const ::NewsGate::Event::EventObject& e2) const throw()
  {
    return merge_level(0,
                       std::max(e1.strain(), e2.strain()),
                       e1.time_diff(e2),
                       e1.time_range(e2));
  }

  bool
  SearchEngine::Debug::Event::can_merge(
    const ::NewsGate::Event::EventObject& event) const throw()
  {
    return event.can_merge(max_strain, max_time_range, max_size);
  }

  bool
  SearchEngine::Debug::Event::can_merge(
    const ::NewsGate::Event::EventObject& e1,
    const ::NewsGate::Event::EventObject& e2,
    bool merge_blacklisted,
    size_t dissenters) const
    throw()
  {
    size_t merged_size = e1.messages().size() + e2.messages().size();
    size_t strain = (size_t)(100.0 * dissenters / merged_size + 0.5);
    
    return e1.lang == e2.lang && !merge_blacklisted &&
      (strain <= max_strain || dissenters < 2 ||
       dissenters < std::min(e1.messages().size(), e2.messages().size())) &&
      can_merge(e1) && can_merge(e2) && merged_size <= max_size &&
      e1.time_diff(e2) <= merge_max_time_diff &&
      e1.time_range(e2) <= max_time_range &&
      e1.words_overlap(e2) >= merge_level(e1, e2);
  }

  //
  // NewsGate::SearchMailer::Type class
  //
  void
  SearchMailer::Type::ready() throw(El::Python::Exception, El::Exception)
  {
    SS_ENABLED_ = PyLong_FromLong(SearchMailing::Subscription::SS_ENABLED);
    SS_DISABLED_ = PyLong_FromLong(SearchMailing::Subscription::SS_DISABLED);
    SS_DELETED_ = PyLong_FromLong(SearchMailing::Subscription::SS_DELETED);

    CO_NOT_FOUND_ = PyLong_FromLong(CO_NOT_FOUND);
    CO_YES_ = PyLong_FromLong(CO_YES);
    CO_EMAIL_CHANGE_ = PyLong_FromLong(CO_EMAIL_CHANGE);
    
    ES_YES_ = PyLong_FromLong(ES_YES);
    ES_CHECK_HUMAN_ = PyLong_FromLong(ES_CHECK_HUMAN);
    ES_MAILED_ = PyLong_FromLong(ES_MAILED);
    ES_ALREADY_ = PyLong_FromLong(ES_ALREADY);
    ES_NOT_FOUND_ = PyLong_FromLong(ES_NOT_FOUND);
    ES_LIMIT_EXCEEDED_ = PyLong_FromLong(ES_LIMIT_EXCEEDED);

    US_YES_ = PyLong_FromLong(US_YES);      
    US_EMAIL_CHANGE_ = PyLong_FromLong(US_EMAIL_CHANGE);
    US_CHECK_HUMAN_ = PyLong_FromLong(US_CHECK_HUMAN);
    US_MAILED_ = PyLong_FromLong(US_MAILED);      
    US_NOT_FOUND_ = PyLong_FromLong(US_NOT_FOUND);      
    US_LIMIT_EXCEEDED_ = PyLong_FromLong(US_LIMIT_EXCEEDED);

    El::Python::ObjectTypeImpl<SearchMailer, SearchMailer::Type>::ready();    
  }

  //
  // NewsGate::SearchMailer class
  //

  SearchMailSubscription::SearchMailSubscription(
    const SearchMailing::Subscription& src) throw(El::Exception)
      : El::Python::ObjectImpl(&SearchMailSubscription::Type::instance),
        times(new El::Python::Sequence()),
        id(src.id.string(El::Guid::GF_DENSE)),
        email(src.email),
        title(src.title),
        query(src.query),
        modifier(src.modifier),
        filter(src.filter),
        format(src.format),
        length(src.length),
        time_offset(src.time_offset),
        status(src.status == SearchMailing::Subscription::SS_ENABLED)
  {
    times->reserve(src.times.size());
    
    for(SearchMailing::TimeSet::const_iterator i(src.times.begin()),
          e(src.times.end()); i != e; ++i)
    {
      SearchMailTime_var tm = new SearchMailTime(i->day, i->time);      
      times->push_back(tm);
    }
  }
  
  //
  // NewsGate::SearchMailer class
  //

  SearchMailer::SearchMailer(PyTypeObject *type,
                             PyObject *args,
                             PyObject *kwds)
    throw(Exception, El::Exception)
      : El::Python::ObjectImpl(type)
  {
    throw Exception("NewsGate::SearchMailer::SearchMailer: "
                    "unforseen way of object creation");
  }

  SearchMailer::SearchMailer(const char* mailer_ref,
                             uint64_t session_timeout,
                             const char* limit_checker_ref,
                             const FraudPrevention::EventLimitCheckDescArray&
                               limit_check_descriptors,
                             CORBA::ORB_ptr orb)
    throw(Exception, El::Exception)
      : El::Python::ObjectImpl(&Type::instance),
        SearchMailing::MailManager(mailer_ref,
                                   limit_checker_ref,
                                   limit_check_descriptors,
                                   orb),
        session_timeout_(session_timeout)
  {
  }
  
  SearchMailer::~SearchMailer() throw()
  {    
  }

  PyObject*
  SearchMailer::py_get_subscription(PyObject* args) throw(El::Exception)
  {
    const char* id = 0;
    
    if(!PyArg_ParseTuple(args,
                         "s:newsgate.search.SearchMailer.get_subscription",
                         &id))
    {
      El::Python::handle_error("NewsGate::SearchMailer::py_get_subscription");
    }    

    std::auto_ptr<SearchMailing::Subscription> subs;
    El::Guid subs_id;

    try
    {
      subs_id = id;
    }
    catch(const El::Exception&)
    {
      return El::Python::add_ref(Py_False);
    }

    {
      El::Python::AllowOtherThreads guard;
      subs.reset(get_subscription(subs_id));
    }

    if(!subs.get())
    {
      return El::Python::add_ref(Py_False);
    }

    return new SearchMailSubscription(*subs);
  }
  
  PyObject*
  SearchMailer::py_get_subscriptions(PyObject* args) throw(El::Exception)
  {
    const char* email = 0;
    const char* token = 0;
    PyObject* obj = 0;
    unsigned char is_human = 0;
    
    if(!PyArg_ParseTuple(args,
                         "ssOb:newsgate.search.SearchMailer.get_subscriptions",
                         &email,
                         &token,
                         &obj,
                         &is_human))
    {
      El::Python::handle_error("NewsGate::SearchMailer::py_get_subscriptions");
    }

    SearchContext* context = SearchContext::Type::down_cast(obj);
    
    El::PSP::Request* request =
      El::PSP::Request::Type::down_cast(context->request.in());

    El::Apache::Request* ap_request = request->request();
    El::Apache::Request::In& in = ap_request->in();

    const char* uid = in.cookies().most_specific("u");
    
    std::string session;
    std::string session_name;
    get_session(email, ap_request, session_name, session);
    
    SearchMailing::SubscriptionArrayPtr subs;

    try
    {
      El::Python::AllowOtherThreads guard;
      
      subs.reset(get_subscriptions(email,
                                   *context->lang,
                                   token,
                                   uid ? uid : "",
                                   ap_request->remote_ip(),
                                   context->user_agent.c_str(),
                                   session,
                                   is_human));

      update_session(session_name.c_str(), session.c_str(), ap_request);      
    }
    catch(const SearchMailing::MailManager::LimitExceeded&)
    {
      return El::Python::add_ref(Py_False);
    }
    catch(const SearchMailing::CheckHuman& e)
    { 
      return El::Python::add_ref(Py_True);
    }

    if(subs.get() == 0)
    {
      return El::Python::add_ref(Py_None);
    }

    El::Python::Sequence_var result = new El::Python::Sequence();
    result->reserve(subs->size());

    for(SearchMailing::SubscriptionArray::const_iterator i(subs->begin()),
          e(subs->end()); i != e; ++i)
    {
      result->push_back(new SearchMailSubscription(*i));
    }
    
    return result.retn();
  }
  
  PyObject*
  SearchMailer::py_enable_subscription(PyObject* args) throw(El::Exception)
  {
    const char* email = 0;
    const char* id = 0;
    unsigned long status = 0;
    PyObject* obj = 0;
    unsigned char is_human = 0;
    
    if(!PyArg_ParseTuple(
         args,
         "sskOb:newsgate.search.SearchMailer.enable_subscription",
         &email,
         &id,
         &status,
         &obj,
         &is_human))
    {
      El::Python::handle_error(
        "NewsGate::SearchMailer::py_enable_subscription");
    }

    SearchContext* context = SearchContext::Type::down_cast(obj);
    
    El::PSP::Request* request =
      El::PSP::Request::Type::down_cast(context->request.in());

    El::Apache::Request* ap_request = request->request();
    El::Apache::Request::In& in = ap_request->in();

    const char* uid = in.cookies().most_specific("u");

    std::string session;
    std::string session_name;
    get_session(email, ap_request, session_name, session);
    
    SearchMailing::Subscription::SubsStatus st =
      status < SearchMailing::Subscription::SS_COUNT ?
      (SearchMailing::Subscription::SubsStatus)status :
      SearchMailing::Subscription::SS_DISABLED;    

    EnablingSubscription res = ES_YES;
    
    {
      El::Python::AllowOtherThreads guard;
      
      res = enable_subscription(email,
                                El::Guid(id),
                                st,
                                uid ? uid : "",
                                ap_request->remote_ip(),
                                context->user_agent.c_str(),
                                session,
                                is_human);

      update_session(session_name.c_str(), session.c_str(), ap_request);      
    }

    return PyLong_FromLong(res);
  }
  
  PyObject*
  SearchMailer::py_update_subscription(PyObject* args) throw(El::Exception)
  {
    PyObject* obj = 0;
    PyObject* ctx = 0;
    unsigned char is_human = 0;
    
    if(!PyArg_ParseTuple(args,
                         "OOb:newsgate.search.SearchMailer.update_subscription",
                         &obj,
                         &ctx,
                         &is_human))
    {
      El::Python::handle_error(
        "NewsGate::SearchMailer::py_update_subscription");
    }

    const SearchMailSubscription& psm =
      *SearchMailSubscription::Type::down_cast(obj);

    const SearchContext& context = *SearchContext::Type::down_cast(ctx);
    
    SearchMailing::Subscription sm;

    sm.reg_time = ACE_OS::gettimeofday();

    if(!psm.id.empty())
    {
      sm.id = psm.id;
    }
    
    sm.email = psm.email;
    
    sm.format = psm.format < SearchMailing::Subscription::MF_COUNT ?
      (SearchMailing::Subscription::MailFormat)psm.format :
      SearchMailing::Subscription::MF_HTML;

    sm.length = psm.length;
    sm.time_offset = psm.time_offset;
    sm.title = psm.title;
    sm.query = psm.query;
    sm.modifier = psm.modifier;
    sm.filter = psm.filter;
    sm.resulted_query = context.query;

    Search::Strategy::Filter& sf = sm.resulted_filter;
    const SearchContext::Filter& psf = *context.filter;

    sf.lang = *psf.lang;
    sf.country = *psf.country;
    sf.feed = psf.feed;
    sf.category = psf.category;
    sf.event.data = psf.event;

    sm.locale.lang = *context.locale->lang;
    sm.locale.country = *context.locale->country;

    sm.lang = *context.lang;

    const El::Python::Sequence& times = *psm.times;

    for(El::Python::Sequence::const_iterator i(times.begin()), e(times.end());
        i != e; ++i)
    {
      const SearchMailTime& pst = *SearchMailTime::Type::down_cast(*i);        
      sm.times.insert(SearchMailing::Time(pst.day % 8,
                                          pst.time % 1440/* / 5 * 5*/));
    }

    if(sm.times.size() > SearchMailing::Subscription::TIMES_MAX)
    {
      throw Exception(
        "NewsGate::SearchMailer::py_add_subscription: "
        "max times number exceeded");
    }
    
    El::PSP::Request* request =
      El::PSP::Request::Type::down_cast(context.request.in());

    El::Apache::Request* ap_request = request->request();
    El::Apache::Request::In& in = ap_request->in();

    const char* uid = in.cookies().most_specific("u");

    if(uid)
    {
      sm.user_id = uid;
    }

    std::string session_name;
    get_session(sm.email.c_str(), ap_request, session_name, sm.user_session);
    
    sm.user_ip = ap_request->remote_ip();
    sm.user_agent = context.user_agent;

    UpdatingSubscription res = US_YES;
    
    {
      El::Python::AllowOtherThreads guard;

      std::string new_session;
      res = update_subscription(sm, is_human, new_session);

      if(!new_session.empty())
      {
        sm.user_session = new_session;
      }

      update_session(session_name.c_str(),
                     sm.user_session.c_str(),
                     ap_request);
    }

    return PyLong_FromLong(res);
  }

  void
  SearchMailer::get_session_name(const char* email,
                                 std::string& session_name) const
    throw(El::Exception)
  {
    uint64_t mail_hash = 0;
    El::CRC(mail_hash, (const unsigned char*)email, strlen(email));

    std::ostringstream ostr;
    ostr << "s_" << std::hex << mail_hash;
    session_name = ostr.str();
  }
  
  void
  SearchMailer::get_session(const char* email,
                            El::Apache::Request* ap_request,
                            std::string& session_name,
                            std::string& session) const
    throw(El::Exception)
  {
    get_session_name(email, session_name);
    
    const char* s =
      ap_request->in().cookies().most_specific(session_name.c_str());
    
    session = s ? s : "";
  }
  
  void
  SearchMailer::update_session(const char* session_name,
                               const char* session,
                               El::Apache::Request* ap_request) const
    throw(El::Exception)
  { 
    El::Moment expiration(ACE_OS::gettimeofday() + session_timeout_);
      
    El::Net::HTTP::CookieSetter cs(session_name, session, &expiration, "/");
    ap_request->out().send_cookie(cs);
  }

  PyObject*
  SearchMailer::py_confirm_operation(PyObject* args) throw(El::Exception)
  {
    PyObject* obj = 0;
    const char* token = 0;
    
    if(!PyArg_ParseTuple(args,
                         "sO:newsgate.search.SearchMailer.confirm_operation",
                         &token,
                         &obj))
    {
      El::Python::handle_error("NewsGate::SearchMailer::py_confirm_operation");
    }

    SearchContext* context = SearchContext::Type::down_cast(obj);
    
    El::PSP::Request* request =
      El::PSP::Request::Type::down_cast(context->request.in());

    El::Apache::Request* ap_request = request->request();
    El::Apache::Request::In& in = ap_request->in();

    const char* uid = in.cookies().most_specific("u");
    
    SearchMailing::Confirmation conf(token,
                                     uid ? uid : "",
                                     ap_request->remote_ip(),
                                     context->user_agent.c_str());
      
    std::string email;
    std::string session;
    ConfirmingOperation confirmed = CO_NOT_FOUND;
    
    {
      El::Python::AllowOtherThreads guard;
      confirmed = confirm_operation(conf, email, session);

      if(confirmed)
      {
        std::string session_name;
        get_session_name(email.c_str(), session_name);
        update_session(session_name.c_str(), session.c_str(), ap_request);
      }
    }

    return PyLong_FromLong(confirmed);
  }
  
  //
  // NewsGate::SearchEngine class
  //
  SearchEngine::SearchEngine(PyTypeObject *type,
                             PyObject *args,
                             PyObject *kwds)
    throw(Exception, El::Exception)
      : El::Python::ObjectImpl(type),
        orb_adapter_(0),
        message_bank_clients_(0),
        message_bank_clients_max_threads_(0),
        search_counter_(0)
  {
    throw Exception("NewsGate::SearchEngine::SearchEngine: "
                    "unforseen way of object creation");
  }
        
  SearchEngine::SearchEngine(PyObject* args)
    throw(Exception, El::Exception)
      : El::Python::ObjectImpl(&Type::instance),
        orb_adapter_(0),
        message_bank_clients_(0),
        message_bank_clients_max_threads_(0),
        search_counter_(0),
        py_search_meter_("SearchEngine::py_search", false),
        search_meter_("SearchEngine::search", false)
  {
    memset(hostname_, 0, sizeof(hostname_));
    
    if(::gethostname(hostname_, sizeof(hostname_) - 1) != 0)
    {
      hostname_[0] = '\0';
    }
    
    PyObject* config = 0;
    PyObject* logger = 0;
    
    if(!PyArg_ParseTuple(args,
                         "OO:newsgate.search.SearchEngine.SearchEngine",
                         &config,
                         &logger))
    {
      El::Python::handle_error("NewsGate::SearchEngine::SearchEngine");
    }

    if(!El::PSP::Config::Type::check_type(config))
    {
      El::Python::report_error(PyExc_TypeError,
                               "1st argument of el.psp.Config expected",
                               "NewsGate::SearchEngine::SearchEngine");
    }

    if(!El::Logging::Python::Logger::Type::check_type(logger))
    {
      El::Python::report_error(PyExc_TypeError,
                               "2nd argument of el.logging.Logger expected",
                               "NewsGate::SearchEngine::SearchEngine");
    }

    config_ = El::PSP::Config::Type::down_cast(config, true);    
    logger_ = El::Logging::Python::Logger::Type::down_cast(logger, true);

    strategy_.reset(new Strategy(config_.in()));
    debug_event_.reset(new Debug::Event(config_.in()));
    
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
      
      Message::Transport::register_valuetype_factories(orb_.in());
      Event::Transport::register_valuetype_factories(orb_.in());
      Search::Transport::register_valuetype_factories(orb_.in());

      Message::BankClientSessionImpl::register_valuetype_factories(
        orb_.in());
      
      Event::BankClientSessionImpl::register_valuetype_factories(
        orb_.in());
      
      Dictionary::Transport::register_valuetype_factories(orb_.in());

      Ad::Transport::register_valuetype_factories(orb_.in());

      word_manager_ = WordManagerRef(
        config_->string("word_manager").c_str(), orb_.in());

      message_bank_clients_ = config_->number("message_bank_clients");
      
      message_bank_clients_max_threads_ =
        config_->number("message_bank_client_session_max_threads");

      if(config_->present("segmentor"))
      {
        std::string segmentor_ref = config_->string("segmentor");
      
        if(!segmentor_ref.empty())
        {
          segmentor_ = SegmentorRef(segmentor_ref.c_str(), orb_.in());
        }  
      }

      std::string ref = config_->string("event_bank_manager");

      if(!ref.empty())
      {
        try
        {
          event_bank_manager_ = EventBankManagerRef(ref.c_str(), orb_.in());
        }
        catch(const CORBA::Exception& e)
        {
          logger_->error("NewsGate::SearchEngine::SearchEngine: "
                         "EventBankManager not found so no debug info will "
                         "be available",
                         ASPECT);
        }
      }
      
      message_bank_manager_ = MessageBankManagerRef(
        config_->string("message_bank_manager").c_str(), orb_.in());

      try
      {
        Message::BankClientSession_var session = bank_client_session();
      }
      catch(const Exception& e)
      {
        std::ostringstream ostr;
        
        ostr << "NewsGate::SearchEngine::SearchEngine: "
          "bank_client_session failed. Reason:\n" << e;
        
        logger_->error(ostr.str().c_str(), ASPECT);
      }

      if(config_->present("stat.processor_ref"))
      {
        ref = config_->string("stat.processor_ref");
        
        std::string limit_checker_ref =
          config_->string("stat.limit_checker_ref");

        if(!ref.empty())
        {
          stat_logger_ =
            new Statistics::StatLogger(
              ref.c_str(),
              limit_checker_ref.c_str(),
              FraudPrevention::EventLimitCheckDescArray(),
              Statistics::StatLogger::IpSet(),
              orb_.in(),
              config_->number("stat.flush_period"),
              this);
          
          stat_logger_->start();
        }
      }

      std::string ad_server_ref = config_->string("ad_server_ref");

      if(!ad_server_ref.empty())
      {
        ad_server_ = AdServerRef(ad_server_ref.c_str(), orb_.in());
      }

      if(config_->present("mail.mailer_ref"))
      {
        ref = config_->string("mail.mailer_ref");
        
        if(!ref.empty())
        {
          std::string limit_checker_ref =
            config_->string("mail.limit_checker_ref");

          FraudPrevention::EventLimitCheckDescArray check_descriptors;

          check_descriptors.add_check_descriptors(
            FraudPrevention::ET_ADD_SEARCH_MAIL,
            true,
            false,
            false,
            config_->string("mail.fraud_prevention_add_mail_user").c_str());
          
          check_descriptors.add_check_descriptors(
            FraudPrevention::ET_ADD_SEARCH_MAIL,
            false,
            false,
            true,
            config_->string("mail.fraud_prevention_add_mail_email").c_str());
          
          check_descriptors.add_check_descriptors(
            FraudPrevention::ET_ADD_SEARCH_MAIL,
            false,
            true,
            false,
            config_->string("mail.fraud_prevention_add_mail_ip").c_str());
          
          check_descriptors.add_check_descriptors(
            FraudPrevention::ET_UPDATE_SEARCH_MAIL,
            true,
            false,
            false,
            config_->string("mail.fraud_prevention_update_mail_user").c_str());
          
          check_descriptors.add_check_descriptors(
            FraudPrevention::ET_UPDATE_SEARCH_MAIL,
            false,
            false,
            true,
            config_->string("mail.fraud_prevention_update_mail_email").c_str());
          
          check_descriptors.add_check_descriptors(
            FraudPrevention::ET_UPDATE_SEARCH_MAIL,
            false,
            true,
            false,
            config_->string("mail.fraud_prevention_update_mail_ip").c_str());
          
          mailer_ = new SearchMailer(ref.c_str(),
                                     config_->number("mail.session_timeout"),
                                     limit_checker_ref.c_str(),
                                     check_descriptors,
                                     orb_.in());
        }
      }
    }
    catch(const CORBA::Exception& e)
    {
      if(orb_adapter_)
      {
        El::Corba::Adapter::orb_adapter_cleanup();
        orb_adapter_ = 0;
      }
      
      std::ostringstream ostr;
      ostr << "NewsGate::SearchEngine::SearchEngine: "
        "CORBA::Exception caught. Description:\n" << e;

      throw Exception(ostr.str());
    }

#   ifdef SEARCH_PROFILING
      py_search_meter_.active(true);
      search_meter_.active(true);
      Message::BankClientSessionImpl::search_meter.active(true);
      Search::Expression::take_top_meter.active(true);

      Message::BankClientSessionImpl::search_meter.reset();
      Search::Expression::take_top_meter.reset();
#   endif      

    
    logger_->info("NewsGate::SearchEngine::SearchEngine: "
                  "search engine constructed",
                  ASPECT);
  }

  SearchEngine::~SearchEngine() throw()
  {    
    cleanup();
    
    logger_->info("NewsGate::SearchEngine::~SearchEngine: "
                  "search engine destructed",
                  ASPECT);
  }

  void
  SearchEngine::cleanup() throw()
  {
    try
    {
      SearchMailer_var mailer;
      Message::BankClientSessionImpl_var bank_client_session;
      Event::BankClientSessionImpl_var event_bank_client_session;
      Statistics::StatLogger_var stat_logger;
      bool cleanup_corba_adapter = false;

      {
        WriteGuard guard(lock_);

        bank_client_session = bank_client_session_._retn();
        event_bank_client_session = event_bank_client_session_._retn();
        stat_logger = stat_logger_.retn();
        mailer = mailer_.retn();
        
        cleanup_corba_adapter = orb_adapter_ != 0;
        orb_adapter_ = 0;
      }

      // Stopping session threads
      bank_client_session = 0;
      event_bank_client_session = 0;

      if(stat_logger.in())
      {
        stat_logger->stop();
        stat_logger->wait();
      }

      logger_->info("NewsGate::SearchEngine::cleanup: done", ASPECT);      
      
      if(cleanup_corba_adapter)
      {
        El::Corba::Adapter::orb_adapter_cleanup();
      }
    }
    catch(...)
    {
    }
  }
  
  bool
  SearchEngine::notify(El::Service::Event* event) throw(El::Exception)
  {
    El::Service::log(event,
                     "NewsGates::SearchEngine::notify: ",
                     logger_.in(),
                     ASPECT);
    
    return true;
  }

  PyObject*
  SearchEngine::py_relax_query(PyObject* args) throw(El::Exception)
  {
    char* text = 0;
    unsigned long level = 0;
    
    if(!PyArg_ParseTuple(args,
                         "sk:newsgate.search.SearchEngine.relax_query",
                         &text,
                         &level))
    {
      El::Python::handle_error("NewsGate::SearchEngine::py_relax_query");
    }

    try
    {
      std::wstring query;
      El::String::Manip::utf8_to_wchar(text, query);
    
      Search::ExpressionParser parser(1);
      std::wistringstream istr(query);
    
      parser.parse(istr);

      Search::Expression_var search_expression = parser.expression();
      Search::Condition* condition = search_expression->condition.in();

      if(level)
      {
        condition->relax(level);
      }
      
      std::ostringstream ostr;
      condition->print(ostr);

      return PyString_FromString(ostr.str().c_str());
    }
    catch(const Search::ParseError& e)
    {
      SearchSyntaxError_var error =
        new SearchSyntaxError(&SearchSyntaxError::Type::instance, 0, 0);

      error->description = e.what();
      error->code = e.code;
      error->position = e.position;
        
      PyErr_SetObject(PyExc_SyntaxError, error.in());      
      return 0;
    }
  }

  PyObject*
  SearchEngine::py_segment_text(PyObject* args) throw(El::Exception)
  {
    char* text = 0;
    
    if(!PyArg_ParseTuple(args,
                         "s:newsgate.search.SearchEngine.segment_text",
                         &text))
    {
      El::Python::handle_error("NewsGate::SearchEngine::py_segment_text");
    }

    if(segmentor_.empty())
    {
      return PyString_FromString(text);
    }

    CORBA::String_var res;
    
    try
    {
      El::Python::AllowOtherThreads guard;  
      Segmentation::PositionSeq_var positions;

      Segmentation::Segmentor_var segmentor = segmentor_.object();
      res = segmentor->segment_text(text, positions.out());      
    }
    catch(const NewsGate::Segmentation::InvalidArgument&)
    {
      PyErr_SetString(PyExc_SyntaxError, "Segmentation error");      
      return 0;
    }
    catch(const NewsGate::Segmentation::NotReady& e)
    {
      El::Python::report_error(
        SearchPyModule::instance.not_ready_ex.in(),
        e.reason.in());
    }
    catch(const Segmentation::ImplementationException& e)
    {
      std::ostringstream ostr;
      ostr << "NewsGate::SearchEngine::py_segment_text: "
        "Segmentation::ImplementationException caught. Description:\n"
           << e.description.in();
      
      throw Exception(ostr.str());
    }
    catch(const CORBA::Exception& e)
    {
      std::ostringstream ostr;
      ostr << "NewsGate::SearchEngine::py_segment_text: "
        "CORBA::Exception caught. Description:\n" << e;
      
      throw Exception(ostr.str());
    }
    
    return PyString_FromString(res.in());
  }

  PyObject*
  SearchEngine::py_segment_query(PyObject* args) throw(El::Exception)
  {
    char* text = 0;
    
    if(!PyArg_ParseTuple(args,
                         "s:newsgate.search.SearchEngine.segment_query",
                         &text))
    {
      El::Python::handle_error("NewsGate::SearchEngine::py_segment_query");
    }
    
    if(segmentor_.empty())
    {
      return PyString_FromString(text);
    }

    CORBA::String_var res;
    
    try
    {
      El::Python::AllowOtherThreads guard;

      Segmentation::Segmentor_var segmentor = segmentor_.object();      
      res = segmentor->segment_query(text);      
    }
    catch(const NewsGate::Segmentation::InvalidArgument&)
    {
      PyErr_SetString(PyExc_SyntaxError, "Segmentation error");
      return 0;
    }
    catch(const NewsGate::Segmentation::NotReady& e)
    {
      El::Python::report_error(
        SearchPyModule::instance.not_ready_ex.in(),
        e.reason.in());
    }
    catch(const Segmentation::ImplementationException& e)
    {
      std::ostringstream ostr;
      ostr << "NewsGate::SearchEngine::py_segment_query: "
        "Segmentation::ImplementationException caught. Description:\n"
           << e.description.in();
      
      throw Exception(ostr.str());
    }
    catch(const CORBA::Exception& e)
    {
      std::ostringstream ostr;
      ostr << "NewsGate::SearchEngine::py_segment_query: "
        "CORBA::Exception caught. Description:\n" << e;
      
      throw Exception(ostr.str());
    }
    
    return PyString_FromString(res.in());
  }

  PyObject*
  SearchEngine::py_search(PyObject* args) throw(El::Exception)
  {
#ifdef USE_HIRES
    ACE_High_Res_Timer timer;
    timer.start();
#else
    ACE_Time_Value start_time = ACE_OS::gettimeofday();
#endif

#   ifdef SEARCH_PROFILING
      El::Stat::TimeMeasurement measurement(py_search_meter_);
#   endif      

      PyObject* context = 0;
    
    if(!PyArg_ParseTuple(args,
                         "O:newsgate.search.SearchEngine.search",
                         &context))
    {
      El::Python::handle_error("NewsGate::SearchEngine::py_search");
    }

    if(!SearchContext::Type::check_type(context))
    {
      El::Python::report_error(
        PyExc_TypeError,
        "argument of newsgate.search.SearchContext expected",
        "NewsGate::SearchEngine::py_search");
    }

    SearchContext& ctx = *SearchContext::Type::down_cast(context);    

    try
    {
      Message::Transport::StoredMessageArrayPtr messages;
      Search::MessageWordPositionMap positions;
      
      SearchResult_var result = search(ctx, messages, positions);
      
      MessageExtMap msg_extras;

      size_t word_max_len = 0;
      bool thumbnail_enable = false;
      unsigned long image_max_width = 0;
      unsigned long image_max_height = 0;
      unsigned long front_image_max_width = 0;
      size_t show_number = 0;

      if(ctx.adjust_images || ctx.in_2_columns)
      {
        El::PSP::Config* conf = config_->config("image");
      
        word_max_len = config_->number("word.max_len");
        thumbnail_enable = conf->number("thumbnail.enable") != 0;
        image_max_width = conf->number("max_width");
        image_max_height = conf->number("max_height");
        front_image_max_width = conf->number("front_image_max_width");
        show_number = conf->number("show_number");
      }
      
      {
// enewsgate crash research
        El::Python::AllowOtherThreads guard;
        
        adjust_images(ctx,
                      thumbnail_enable,
                      image_max_width,
                      image_max_height,
                      front_image_max_width,
                      show_number,
                      *messages,
                      positions,
                      msg_extras);
      
        result->second_column_offset =
          second_column_offset(ctx,
                               word_max_len,
                               thumbnail_enable,
                               show_number,
                               *messages,
                               msg_extras);
      }
      
      copy_messages(ctx,
                    *messages,
                    positions,
                    msg_extras,
                    *result->messages);

      bool debug_info = ctx.gm_flags & Message::Bank::GM_DEBUG_INFO;
      bool extra_msg_info = ctx.gm_flags & Message::Bank::GM_EXTRA_MSG_INFO;
        
      if((debug_info || extra_msg_info) && !messages->empty())
      {
        result->debug_info =
          new SearchResult::DebugInfo(&SearchResult::DebugInfo::Type::instance,
                                      0,
                                      0);

        Event::EventObject event;
        
        if(debug_info && !ctx.msg_id_similar.empty())
        {
          fill_event_debug_info(*result->debug_info->event,
                                *messages,
                                ctx.event_overlap.c_str(),
                                ctx.event_split.c_str(),
                                ctx.event_separate,
                                ctx.event_narrow_separation,
                                event);
        }

        fill_message_debug_info(*result->debug_info->messages,
                                *messages,
                                event);
      }

#   ifdef SEARCH_PROFILING
      measurement.stop();
      
      WriteGuard guard(lock_);

      if(++search_counter_ % SEARCH_PROFILING == 0)
      {
        py_search_meter_.dump(std::cerr);
        search_meter_.dump(std::cerr);
        Message::BankClientSessionImpl::search_meter.dump(std::cerr);
        Search::Expression::take_top_meter.dump(std::cerr);

        py_search_meter_.reset();
        search_meter_.reset();
        Message::BankClientSessionImpl::search_meter.reset();
        Search::Expression::take_top_meter.reset();
      }      
#   endif

#ifdef USE_HIRES
      timer.stop();
      ACE_Time_Value tm;
      timer.elapsed_time(tm);
#else
      ACE_Time_Value tm = ACE_OS::gettimeofday() - start_time;
#endif

      if(tm > ACE_Time_Value(config_->number("trace_duration")))
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchEngine::search: long request "
             << El::Moment::time(tm);
        
        logger_->trace(ostr.str(), ASPECT, El::Logging::HIGH);
      }
      
#     ifdef TRACE_SEARCH_TIME      
      std::cerr << "SearchEngine::py_search: " << El::Moment::time(tm)
                << std::endl;
#     endif

      return result.retn();
    }
    catch(const Search::ParseError& e)
    {
      SearchSyntaxError_var error =
        new SearchSyntaxError(&SearchSyntaxError::Type::instance, 0, 0);

      error->description = e.what();
      error->code = e.code;
      error->position = e.position;
        
      PyErr_SetObject(PyExc_SyntaxError, error.in());      
      return 0;
    }
    catch(const NewsGate::Message::ImplementationException& e)
    {
      std::ostringstream ostr;
      ostr << "NewsGate::SearchEngine::py_search: "
        "NewsGate::Message::ImplementationException caught. Description:\n"
           << e.description.in();
      
      throw Exception(ostr.str());
    }
    catch(const Dictionary::ImplementationException& e)
    {
      std::ostringstream ostr;
      ostr << "NewsGate::SearchEngine::py_search: "
           << "Dictionary::ImplementationException caught. "
        "Description:\n" << e.description.in();

      throw Exception(ostr.str());
    }
    catch(const CORBA::Exception& e)
    {
      std::ostringstream ostr;
      ostr << "NewsGate::SearchEngine::py_search: "
        "CORBA::Exception caught. Description:\n" << e;
      
      throw Exception(ostr.str());
    }
    
    return 0;
  }

  void
  SearchEngine::fill_word_id_to_word(const Message::StoredMessage& msg,
                                     UINT32_ToStringMap& id_to_text)
    throw(Exception, El::Exception)
  {      
    UINT32_ToStringMap pos_to_text;
    const Message::MessageWordPosition& word_positions = msg.word_positions;

//      std::cerr << "\n**********************";
      
    for(size_t j = 0; j < word_positions.size(); ++j)
    {
      const Message::WordPositions& pos = word_positions[j].second;
      const char* text = word_positions[j].first.c_str();

/*      
      std::cerr << std::endl << text;
        
      if(!pos.is_core())
      {
        std::cerr << " notcore";
      }
*/
      uint32_t pseudo_id = El::Dictionary::Morphology::pseudo_id(text);
//      std::cerr << " " << pseudo_id << ":";
      
      id_to_text[pseudo_id] = text;
         
      for(Message::WordPositionNumber p = 0; p < pos.position_count(); ++p)
      {
        Message::WordPosition wp = pos.position(msg.positions, p);

//        std::cerr << " " << wp;
        pos_to_text[wp] = text;
      }
    }
          
//    std::cerr << "\n----------------------";
      
    const Message::NormFormPosition& norm_form_positions =
      msg.norm_form_positions;

    for(size_t j = 0; j < norm_form_positions.size(); ++j)
    {
      const Message::WordPositions& pos = norm_form_positions[j].second;
      uint32_t wid = norm_form_positions[j].first;
        
//        std::cerr << std::endl << wid;
        
      if(!pos.is_core())
      {
//        std::cerr << " notcore";
        continue;
      }        
        
      for(Message::WordPositionNumber p = 0; p < pos.position_count(); ++p)
      {
        Message::WordPosition wp = pos.position(msg.positions, p);
          
        UINT32_ToStringMap::const_iterator it = pos_to_text.find(wp);
        assert(it != pos_to_text.end());

        id_to_text[wid] = it->second;
      }
    }
  }
  
  El::Python::Sequence*
  SearchEngine::core_words(const Message::StoredMessage& msg) const
    throw(Exception, El::Exception)
  {
    UINT32_ToStringMap id_to_text;
    fill_word_id_to_word(msg, id_to_text);
    
    El::Python::Sequence_var core_words_seq = new El::Python::Sequence();
    
    const Message::CoreWords& core_words = msg.core_words;
    core_words_seq->reserve(core_words.size());

    bool broken_coreness = false;
    
    for(Message::CoreWords::const_iterator i(core_words.begin()),
          e(core_words.end()); i != e; ++i)
    {
      uint32_t core_word_id = *i;
      UINT32_ToStringMap::const_iterator cit = id_to_text.find(core_word_id);

      if(cit == id_to_text.end())
      {
        broken_coreness = true;
        continue;
      }
      
//      assert(cit != id_to_text.end());
      
      SearchResult::Message::CoreWord_var word_info =
        new SearchResult::Message::CoreWord(
          &SearchResult::Message::CoreWord::Type::instance, 0, 0);
      
      word_info->id = core_word_id;
      word_info->text = cit->second;

      word_info->weight =
        (unsigned long)((float)(e - i) * 100 / core_words.size() + 0.5);

      core_words_seq->push_back(word_info);
    }

    if(broken_coreness)
    {
      std::cerr << "BCM1 " << msg.id.string() << std::endl;
    }
      
    return core_words_seq.retn();
  }
  
  void
  SearchEngine::fill_message_debug_info(
    El::Python::Map& messages_debug_info,
    const Message::Transport::StoredMessageArray& messages,
    const Event::EventObject& event) const
    throw(CORBA::Exception, Exception, El::Exception)
  {
    for(Message::Transport::StoredMessageArray::const_iterator
          mit(messages.begin()), mie(messages.end()); mit != mie; ++mit)
    {
      const Message::Transport::DebugInfo& debug_info = mit->debug_info;

      El::Python::Object_var id =
        PyString_FromString(mit->message.id.string(true).c_str());

      SearchResult::DebugInfo::Message_var msg =
        new SearchResult::DebugInfo::Message(
          &SearchResult::DebugInfo::Message::Type::instance,
          0,
          0);
      
      messages_debug_info[id] = msg;

      const Message::StoredMessage& src_msg = mit->message;

      msg->id = src_msg.id.string();      
      msg->signature = src_msg.signature;      
      msg->feed_impressions = debug_info.feed_impressions;
      msg->feed_clicks = debug_info.feed_clicks;
      
      if(!debug_info.on)
      {
        continue;
      }

      UINT32_ToStringMap id_to_text;
      fill_word_id_to_word(src_msg, id_to_text);
      
      msg->url_signature = src_msg.url_signature;
      msg->match_weight = debug_info.match_weight;

      const Message::WordsFreqInfo::WordInfoArray& word_infos =
        debug_info.words_freq.word_infos;

      const Message::StoredMessage* msg2 = 0;

      if(event.messages().size() == 2 && messages.size() == 2)
      {
        msg2 = &messages[&messages[0].message == &src_msg ? 1 : 0].message;
      }

      Event::EventWordWeightMap event_words_pw;

      if(msg2)
      {
        {
          const Message::CoreWords& core_words = msg2->core_words;
          size_t core_words_count = core_words.size();

          for(size_t i = 0; i < core_words_count; ++i)
          {
            event_words_pw[core_words[i]] =
              Event::EventWordWC(
                Event::EventObject::core_word_weight(
                  i,
                  core_words_count,
                  debug_event_->max_message_core_words),
                i ? 0 : 1);
          }
        }        

        {
          const Message::CoreWords& core_words = src_msg.core_words;
          size_t core_words_count = core_words.size();

          for(size_t i = 0; i < core_words_count; ++i)
          {
            Event::EventWordWeightMap::iterator it =
              event_words_pw.insert(
                std::make_pair(core_words[i], Event::EventWordWC(0, 0))).first;

            Event::EventWordWC& wc = it->second;

            wc.weight +=
              Event::EventObject::core_word_weight(
                i,
                core_words_count,
                debug_event_->max_message_core_words);

            if(!i)
            {
              ++wc.first_count;
            }
          }
        }
      }
      else
      {
        event.get_word_weights(event_words_pw);
      }
      
      std::ostringstream core_words_stream;
      unsigned long core_words_in_event = 0;
      unsigned long common_event_words = 0;

      bool broken_coreness = false;
      const Message::CoreWords& core_words = src_msg.core_words;
      
      for(size_t i = 0; i < core_words.size(); ++i)
      {
        uint32_t core_word_id = core_words[i];

        SearchResult::DebugInfo::Word_var word_debug_info =
          new SearchResult::DebugInfo::Word(
            &SearchResult::DebugInfo::Word::Type::instance, 0, 0);

        word_debug_info->id = core_word_id;
        
        UINT32_ToStringMap::const_iterator cit = id_to_text.find(core_word_id);

        if(cit == id_to_text.end())
        {
          broken_coreness = true;
        }
        else
        {
          word_debug_info->text = cit->second;
        }
        
//        assert(cit != id_to_text.end());
//        word_debug_info->text = cit->second;
        
        Event::EventWordWeightMap::const_iterator it =
          event_words_pw.find(core_word_id);
      
        bool is_event_word = it != event_words_pw.end();
        word_debug_info->remarkable = is_event_word;
        
        word_debug_info->unknown = (core_word_id & 0x80000000) != 0;

        if(is_event_word)
        {
          core_words_in_event++;

          if(it->second.weight >
             Event::EventObject::core_word_weight(
               i,
               core_words.size(),
               debug_event_->max_message_core_words))
          {
            common_event_words++;
          }
        }

        (*msg->core_words).push_back(word_debug_info);
      }

      if(broken_coreness)
      {
        std::cerr << "BCM2 " << src_msg.id.string() << std::endl;
      }
/*
      if(src_msg.id.string() == "00A452AC50A5D17E")
      {
        std::cerr << "XXX\n";
      }
*/
      msg->core_words_in_event = core_words_in_event;
      msg->common_event_words = common_event_words;

      WORD_OVERLAP_DEBUG_STREAM_DEF

      if(msg2)
      {        
        msg->event_overlap =
          Event::EventObject::words_overlap(
            core_words,
            msg2->core_words,
            debug_event_->max_message_core_words,
            0
            WORD_OVERLAP_DEBUG_STREAM_PASS);
      }
      else
      {        
        msg->event_overlap = event.words_overlap(
          core_words,
          debug_event_->max_message_core_words,
          0,
          0
          WORD_OVERLAP_DEBUG_STREAM_PASS);
      }
      
      msg->extras = WORD_OVERLAP_DEBUG_STREAM_VAL;
      
      msg->event_merge_level =
        debug_event_->merge_level(event.messages().size(),
                                  0,
                                  0,
                                  event.time_range());

      for(Message::WordsFreqInfo::WordInfoArray::const_iterator
            it(word_infos.begin()), ie(word_infos.end()); it != ie; ++it)
      {
        SearchResult::DebugInfo::Word_var word_debug_info =
          new SearchResult::DebugInfo::Word(
            &SearchResult::DebugInfo::Word::Type::instance, 0, 0);

        word_debug_info->text = it->text.c_str();

        El::Dictionary::Morphology::WordId word_id = it->id();        
        
        word_debug_info->id = word_id;
        *word_debug_info->lang = it->lang;

        unsigned char core_word_position = 0;
        bool is_core_word_id = core_words.find(word_id, &core_word_position);

        word_debug_info->remarkable = is_core_word_id;
        word_debug_info->unknown = (word_id & 0x80000000) != 0;

        word_debug_info->cw_weight = it->cw_weight;
        word_debug_info->wp_weight = it->wp_weight;
        word_debug_info->weight = it->weight;

        std::string& token_type = word_debug_info->token_type;
        
        switch(it->token_type)
        {
        case Message::WordPositions::TT_UNDEFINED: token_type = "U"; break;
        case Message::WordPositions::TT_WORD: token_type = "W"; break;
        case Message::WordPositions::TT_NUMBER: token_type = "N"; break;
        case Message::WordPositions::TT_SURROGATE: token_type = "R"; break;
        case Message::WordPositions::TT_FEED_STOP_WORD:
          {
            token_type = "F";
            break;
          }
        case Message::WordPositions::TT_KNOWN_WORD: token_type = "K"; break;
        case Message::WordPositions::TT_STOP_WORD: token_type = "S"; break;
        }
        
        word_debug_info->position = core_word_position;

        uint16_t flags = it->position_flags;

        if(flags & Message::WordsFreqInfo::PositionInfo::PF_CAPITAL)
        {
          word_debug_info->flags += "C";
        }

        if(flags & Message::WordsFreqInfo::PositionInfo::PF_PROPER_NAME)
        {
          word_debug_info->flags += "P";
        }

        if(flags & Message::WordsFreqInfo::PositionInfo::PF_TITLE)
        {
          word_debug_info->flags += "T";
        }

        if(flags & Message::WordsFreqInfo::PositionInfo::PF_ALTERNATE_ONLY)
        {
          word_debug_info->flags += "A";
        }

        if(flags & Message::WordsFreqInfo::PositionInfo::PF_META_INFO)
        {
          word_debug_info->flags += "M";
        }

        (*msg->words).push_back(word_debug_info);
      }
    }
  }
  
  void
  SearchEngine::fill_event_debug_info(
    SearchResult::DebugInfo::Event& event_debug_info,
    const Message::Transport::StoredMessageArray& messages,
    const char* event_overlap,
    const char* event_split,
    unsigned long event_separate,
    bool event_narrow_separation,
    Event::EventObject& event)
    throw(CORBA::Exception, Exception, El::Exception)
  {
    Event::BankClientSession_var session = event_bank_client_session();
    
    if(session.in() == 0)
    {
      return;
    }
       
    const El::Luid& event_id = messages[0].message.event_id;
    event_debug_info.id = event_id.string();

    El::Luid overlap_id;
    
    if(*event_overlap != '\0')
    {
      overlap_id = El::Luid(event_overlap);
    }

    Message::Id split_msg;
    
    if(*event_split != '\0')
    {
      split_msg = Message::Id(event_split);
    }

    Event::Transport::EventIdRelPackImpl::Var ids =
      new Event::Transport::EventIdRelPackImpl::Type(
        new Event::Transport::EventIdRelArray());

    Event::Transport::EventIdRelArray& id_array = ids->entities();

    id_array.push_back(
      Event::Transport::EventIdRel(event_id,
                                   overlap_id,
                                   split_msg,
                                   event_separate,
                                   event_narrow_separation));

    if(overlap_id != El::Luid::null)
    {
      id_array.push_back(Event::Transport::EventIdRel(overlap_id));
    }

    Event::Transport::EventObjectRelPack_var events;

    try
    {
      Event::BankClientSession::RequestResult_var res =
        session->get_events(ids.in(), events.out());
    }
    catch(const NewsGate::Event::NotReady& )
    {
      return;
    }

    Event::Transport::EventObjectRelPackImpl::Type* events_pack =
      dynamic_cast<Event::Transport::EventObjectRelPackImpl::Type*>(
        events.in());

    if(events_pack == 0)
    {
      throw Exception(
        "NewsGate::SearchEngine::fill_event_debug_info: "
        "dynamic_cast<Event::Transport::EventObjectRelPackImpl::"
        "Type*> failed");            
    }

    const Event::EventObject* overlap_event = 0;
    Event::Transport::EventObjectRel event_rel;
      
    const Event::Transport::EventObjectRelArray& event_array = 
      events_pack->entities();

    for(Event::Transport::EventObjectRelArray::const_iterator
          i(event_array.begin()), e(event_array.end()); i != e; ++i)
    {
      if(i->rel.object1.id == event_id)
      {
        event = i->rel.object1;
        event_rel = *i;
      }
      else if(i->rel.object1.id == overlap_id)
      {
        overlap_event = &i->rel.object1;
      }
    }

    if(event.id != event_id)
    {
      return;
    }

    if(overlap_event)
    {
      fill_event_overlap(event,
                         *overlap_event,
                         event_rel.rel,
                         *event_debug_info.overlap);      
    }

    if(split_msg != Message::Id::zero)
    {
      fill_event_split(event_rel.split, *event_debug_info.split, true);
    }    

    if(event_separate)
    {
      fill_event_split(event_rel.separate, *event_debug_info.separate, false);
    }
    
    UINT32_ToStringMap id_to_text;
    
    for(Message::Transport::StoredMessageArray::const_iterator
          i(messages.begin()), e(messages.end()); i != e; ++i)
    {
      fill_word_id_to_word(i->message, id_to_text);
    }
    
    event_debug_info.hash = event.hash();
    
    event_debug_info.flags = event.flags_string();
/*
    std::cerr << event.id.string() << ":"
              << ((event.flags & Event::EventObject::EF_REVISED) ?
                  "R" : "-") << std::endl;
*/
    
    event_debug_info.changed = event_rel.changed;
    event_debug_info.dissenters = event.dissenters();
    event_debug_info.strain = event.strain();
    event_debug_info.size = event.messages().size();
    event_debug_info.time_range = event.time_range();
    event_debug_info.published_min = event.published_min;
    event_debug_info.published_max = event.published_max;
    *event_debug_info.lang = event.lang;
    event_debug_info.can_merge = debug_event_->can_merge(event);

    event_debug_info.merge_level =
      debug_event_->merge_level(0, //event.messages().size(),
                                event.strain(),
                                0,
                                event.time_range());

    const Event::EventWordWeightArray& words = event.words;
    event_debug_info.words->resize(words.size());
          
    for(size_t i = 0; i < words.size(); ++i)
    {
      uint32_t wid = words[i].word_id;

      SearchResult::DebugInfo::Word_var word_debug_info =
        new SearchResult::DebugInfo::Word(
          &SearchResult::DebugInfo::Word::Type::instance, 0, 0);

      word_debug_info->id = wid;
      const char* core_word = 0;

      UINT32_ToStringMap::const_iterator cit = id_to_text.find(wid);

      if(cit != id_to_text.end())
      {
        core_word = cit->second;
      }
      else
      {
        for(Message::Transport::StoredMessageArray::const_iterator
              it(messages.begin()), ie(messages.end()); it != ie &&
              core_word == 0; ++it)
        {
          const Message::WordsFreqInfo::WordInfoArray& word_infos =
            it->debug_info.words_freq.word_infos;
          
          for(Message::WordsFreqInfo::WordInfoArray::const_iterator
                wit(word_infos.begin()), wie(word_infos.end());
              wit != wie; ++wit)
          {
            if(wit->norm_form == wid || wit->pseudo_id == wid)
            {
              core_word = wit->text.c_str();
              break;
            }
          }
        }
      }
      
      if(core_word)
      {
        word_debug_info->text = core_word;
      }

      word_debug_info->unknown = (wid & 0x80000000) != 0;
      word_debug_info->weight = words[i].weight;
      word_debug_info->position = words[i].first_count;

      if(overlap_event)
      {
        const Event::EventWordWeightArray& words = overlap_event->words;
          
        for(size_t i = 0; i < words.size(); ++i)
        {
          if(words[i].word_id == wid)
          {
            word_debug_info->remarkable = true;
            break;
          }
        }
      }

      (*event_debug_info.words)[i] = word_debug_info;
    }
  }

  void
  SearchEngine::fill_event_overlap(
    const Event::EventObject& event,
    const Event::EventObject& overlap_event,
    const Event::Transport::EventRel& event_rel,
    SearchResult::DebugInfo::EventOverlap& overlap)
    const throw(El::Exception)
  {        
    size_t common_words = 0;
    std::ostringstream dstr;

    overlap.level =
      event.words_overlap(overlap_event,
                          &common_words
                          WORD_OVERLAP_DEBUG_STREAM_PASS);

    overlap.extras = dstr.str();
      
    overlap.common_words = common_words;
    overlap.time_diff = event.time_diff(overlap_event);
    overlap.merge_level = debug_event_->merge_level(event, overlap_event);

    overlap.size =
      event.messages().size() + overlap_event.messages().size();

    overlap.strain = event_rel.merge_result.strain();
    overlap.dissenters = event_rel.merge_result.dissenters();

    overlap.time_range = event.time_range(overlap_event);
    overlap.colocated = event_rel.colocated;
    overlap.same_lang = event.lang == overlap_event.lang;
    overlap.merge_blacklist_timeout = event_rel.merge_blacklist_timeout;

    overlap.can_merge =
      debug_event_->can_merge(event,
                              overlap_event,
                              event_rel.merge_blacklist_timeout,
                              event_rel.merge_result.dissenters());
  }
  
  void
  SearchEngine::fill_event_split(const Event::Transport::EventParts& src,
                                 SearchResult::DebugInfo::EventSplit& dest,
                                 bool respect_strain)
    const throw(El::Exception)
  {    
    const Event::EventObject& part1 = src.part1;
    const Event::EventObject& part2 = src.part2;
      
    if(part1.messages().size() && part2.messages().size())
    {      
      size_t common_words = 0;
      std::ostringstream dstr;
      
      dest.overlap_level = part1.words_overlap(part2,
                                               &common_words
                                               WORD_OVERLAP_DEBUG_STREAM_PASS);
      
      dest.common_words = common_words;
      dest.time_diff = part1.time_diff(part2);
      dest.extras = dstr.str();
      
//      dest.merge_level = debug_event_->merge_level(part1, part2);
      
      dest.merge_level =
        debug_event_->merge_level(
          0,
          respect_strain ? std::max(part1.strain(), part2.strain()) : 0,
          part1.time_diff(part2),
          part1.time_range(part2));
      
      fill_event_split(*dest.part1, part1);
      fill_event_split(*dest.part2, part2);
      
      if(src.merge1.object2.id != El::Luid::null)
      {
        fill_event_overlap(part1,
                           src.merge1.object2,
                           src.merge1,
                           *dest.overlap1);
      }
      
      if(src.merge2.object2.id != El::Luid::null)
      {
        fill_event_overlap(part2,
                           src.merge2.object2,
                           src.merge2,
                           *dest.overlap2);
      }
    }
  }
  
  void
  SearchEngine::fill_event_split(SearchResult::DebugInfo::EventSplitPart& part,
                                 const Event::EventObject& event)
    throw(El::Exception)
  {
    part.size = event.messages().size();
    part.strain = event.strain();
    part.dissenters = event.dissenters();
    part.time_range = event.time_range();
    part.published_min = event.published_min;
    part.published_max = event.published_max;        
  }

  void
  SearchEngine::copy_messages(
    const SearchContext& ctx,
    const Message::Transport::StoredMessageArray& messages,
    const Search::MessageWordPositionMap& positions,
    const MessageExtMap& msg_extras,
    El::Python::Sequence& result_messages) const
    throw(El::Exception)
  {
    result_messages.resize(messages.size());

    size_t word_max_len = 0;

    if(ctx.gm_flags & (Message::Bank::GM_TITLE | Message::Bank::GM_DESC |
                       Message::Bank::GM_IMG))
    {
      word_max_len = config_->number("word.max_len");
    }
    
    Message::StoredImageArray* images = 0;
    size_t i = 0;
    
    for(Message::Transport::StoredMessageArray::const_iterator
          it(messages.begin()), ie(messages.end()); it != ie; ++it, ++i)
    {
      const Message::StoredMessage& msg = it->message;

      SearchResult::Message_var result_msg =
        new SearchResult::Message(&SearchResult::Message::Type::instance,
                                  0,
                                  0);

      SearchResult::Message& res_msg = *result_msg;
      
      if(ctx.gm_flags & Message::Bank::GM_CORE_WORDS)
      {
        res_msg.core_words = core_words(msg);
      }
      
      if((ctx.gm_flags & Message::Bank::GM_IMG) &&
         (images = msg.content->images.get()))
      {
        MessageExtMap::const_iterator mit = msg_extras.find(msg.id);

        const MessageExt* msg_ext = mit == msg_extras.end() ?
          0 : &mit->second;
        
        El::Python::Sequence_var res_images = res_msg.images;
        size_t res_image_count = images->size();

        if(msg_ext && msg_ext->front_image.img.width)
        {
          res_msg.front_image = create_image(msg_ext->front_image.img,
                                             msg_ext->front_image.ext.index);
          
          assert(res_image_count);
          res_image_count--;
        }
        
        res_images->resize(res_image_count);
        size_t j = 0;
          
        for(Message::StoredImageArray::const_iterator b(images->begin()), i(b),
              e(images->end()); i != e; ++i)
        {
          const Message::StoredImage& img = *i;
          
          if(!img.width)
          {
            continue; // front image; skip here
          }
          
          (*res_images)[j++] = create_image(img, i - b);
        }
      }

      if(ctx.gm_flags & Message::Bank::GM_CATEGORIES)
      {
        const Message::Categories::CategoryArray& categories =
          msg.categories.array;
          
        El::Python::Sequence_var res_categories = res_msg.categories;
        res_categories->resize(categories.size());
          
        for(size_t i = 0; i < categories.size(); ++i)
        {
          (*res_categories)[i] = PyString_FromString(categories[i].c_str());
        }
      }

      result_messages[i] = result_msg;
    }

    std::string thumbnail_url = config_->string("image.thumbnail.url") + "/";
    
// enewsgate crash research
//    El::Python::AllowOtherThreads guard;
    i = 0;
    
    for(Message::Transport::StoredMessageArray::const_iterator
          it(messages.begin()), ie(messages.end()); it != ie; ++it, ++i)
    {
      const Message::StoredMessage& msg = it->message;
      
      SearchResult::Message& res_msg =
        *SearchResult::Message::Type::down_cast(result_messages[i].in());
      
      if(ctx.gm_flags & (Message::Bank::GM_ID | Message::Bank::GM_LINK))
      {
        res_msg.id = msg.id.string();
        res_msg.encoded_id = msg.id.string(true);
      }
      
      if(ctx.gm_flags & Message::Bank::GM_LINK)
      {
        res_msg.url = msg.content->url.c_str();
      }
      
      if(ctx.gm_flags & Message::Bank::GM_PUB_DATE)
      {
        res_msg.published = msg.published;
      }
      
      if(ctx.gm_flags & Message::Bank::GM_VISIT_DATE)
      {
        res_msg.visited = msg.visited;
      }
      
      if(ctx.gm_flags & Message::Bank::GM_FETCH_DATE)
      {
        res_msg.fetched = msg.fetched;
      }
      
      if(ctx.gm_flags & Message::Bank::GM_EVENT)
      {
        res_msg.event_id = msg.event_id.string();
        res_msg.encoded_event_id = msg.event_id.string(true);
        res_msg.event_capacity = msg.event_capacity;
      }

      if(ctx.gm_flags & Message::Bank::GM_STAT)
      {
        res_msg.impressions = msg.impressions;
        res_msg.clicks = msg.clicks;
      }

      if(ctx.gm_flags &
         (Message::Bank::GM_DEBUG_INFO | Message::Bank::GM_EXTRA_MSG_INFO))
      {
        res_msg.feed_impressions = it->debug_info.feed_impressions;
        res_msg.feed_clicks = it->debug_info.feed_clicks;
      }
      
      if(ctx.gm_flags & Message::Bank::GM_LANG)
      {
        *res_msg.lang = msg.lang;
      }
      
      if(ctx.gm_flags & Message::Bank::GM_SOURCE)
      {
        res_msg.source_id = msg.source_id;
        res_msg.source_url = msg.source_url.c_str();
        res_msg.source_title = msg.source_title.c_str();
        res_msg.source_html_link = msg.content->source_html_link.c_str();
        *res_msg.country = msg.country;
      }

      images = (ctx.gm_flags & Message::Bank::GM_IMG) ?
        msg.content->images.get() : 0;
        
      std::auto_ptr<WordPositionSet> word_positions;

      if(ctx.gm_flags & (Message::Bank::GM_TITLE | Message::Bank::GM_DESC |
                         Message::Bank::GM_KEYWORDS) || images)
      {
        Search::MessageWordPositionMap::const_iterator pit =
          positions.find(msg.id);

        const Search::WordPositionArray* word_pos =
          pit == positions.end() ? 0 : &(pit->second);

        word_positions.reset(new WordPositionSet());
        fill_position_set(word_pos, *word_positions);
      }

      if(images)
      {        
        MessageExtMap::const_iterator mit = msg_extras.find(msg.id);

        const MessageExt* msg_ext = mit == msg_extras.end() ?
          0 : &mit->second;
        
        const ImageExtArray* image_ext = msg_ext ? &msg_ext->images : 0;
          
        El::Python::Sequence* res_images = res_msg.images.in();
        size_t res_image_count = images->size();

        if(msg_ext && msg_ext->front_image.img.width)
        {
           copy_image(ctx,
                      word_max_len,
                      *it,
                      *word_positions,
                      msg_ext->front_image.img,
                      msg_ext->front_image.ext.index,
                      &msg_ext->front_image.ext,
                      thumbnail_url.c_str(),
                      *SearchResult::Image::Type::down_cast(
                        res_msg.front_image.in()));
          
          assert(res_image_count);
          res_image_count--;
        }
        
        res_images->resize(res_image_count);
        size_t j = 0;
          
        for(size_t i = 0; i < images->size(); i++)
        {
          const Message::StoredImage& img = (*images)[i];
          
          if(!img.width)
          {
            continue; // front image; skip here
          }
          
          copy_image(ctx,
                     word_max_len,
                     *it,
                     *word_positions,
                     img,
                     i,
                     image_ext ? &(*image_ext)[i] : 0,
                      thumbnail_url.c_str(),
                     *SearchResult::Image::Type::down_cast(
                       (*res_images)[j++].in()));
        }
      }      

      if(ctx.gm_flags & Message::Bank::GM_TITLE)
      {        
        std::ostringstream ostr;
        MessageWriter writer(&ostr,
                             &(*it),
                             *word_positions,
                             ctx.title_format,
                             word_max_len,
                             ctx.max_title_len,
                             "",
                             "");
      
        msg.assemble_title(writer);
        res_msg.title = ostr.str();

        if(res_msg.title.empty() && ctx.max_title_from_desc_len)
        {
          std::ostringstream ostr;
          MessageWriter writer(&ostr,
                               &(*it),
                               *word_positions,
                               ctx.title_format,
                               word_max_len,
                               ctx.max_title_from_desc_len,
                               "",
                               "");
          
          msg.assemble_description(writer);
          res_msg.title = ostr.str();
        }
      }

      if(ctx.gm_flags & Message::Bank::GM_DESC)
      {
        std::ostringstream ostr;
        MessageWriter writer(&ostr,
                             &(*it),
                             *word_positions,
                             ctx.description_format,
                             word_max_len,
                             ctx.max_desc_len,
                             ctx.in_msg_link_class.c_str(),
                             ctx.in_msg_link_style.c_str());
          
        msg.assemble_description(writer);
        res_msg.description = ostr.str();
      }

      if(ctx.gm_flags & Message::Bank::GM_KEYWORDS)
      {        
        std::ostringstream ostr;
        MessageWriter writer(&ostr,
                             &(*it),
                             *word_positions,
                             ctx.keywords_format,
                             0,
                             0,
                             "",
                             "");
      
        msg.assemble_keywords(writer);
        res_msg.keywords = ostr.str();
      }      
    }      
  }

  SearchResult::Image*
  SearchEngine::create_image(const Message::StoredImage& img,
                             unsigned long index) const
    throw(El::Exception)
  {    
    SearchResult::Image_var res_img =
      new SearchResult::Image(&SearchResult::Image::Type::instance, 0, 0);

    res_img->index = index;

    El::Python::Sequence_var res_thumbs = res_img->thumbs;
    res_thumbs->resize(img.thumbs.size());

    for(size_t j = 0; j < img.thumbs.size(); ++j)
    {
      (*res_thumbs)[j] = new SearchResult::ImageThumb(
        &SearchResult::ImageThumb::Type::instance,
        0,
        0);
    }
            
    return res_img.retn();
  }

  void
  SearchEngine::copy_image(const SearchContext& ctx,
                           size_t word_max_len,
                           const Message::Transport::StoredMessageDebug& msg,
                           WordPositionSet& word_pos,
                           const Message::StoredImage& img,
                           size_t img_index,
                           const ImageExt* img_ext,
                           const char* thumbnail_url,
                           SearchResult::Image& res_img) const
    throw(El::Exception)
  {    
    El::String::Manip::xml_encode(
      img.src.c_str(),
      res_img.src,
      El::String::Manip::XE_TEXT_ENCODING |
      El::String::Manip::XE_ATTRIBUTE_ENCODING |
      El::String::Manip::XE_PRESERVE_UTF8 |
      El::String::Manip::XE_FORCE_NUMERIC_ENCODING);
            
    std::ostringstream ostr;
    MessageWriter writer(&ostr,
                         &msg,
                         word_pos,
                         ctx.img_alt_format,
                         word_max_len,
                         ctx.max_img_alt_len,
                         ctx.in_msg_link_class.c_str(),
                         ctx.in_msg_link_style.c_str());

    const Message::StoredMessage& message = msg.message;
    
    message.assemble_image_alt(writer, img_index);
    res_img.alt = ostr.str();
              
    res_img.width = img.width == UINT16_MAX ? -1 : img.width;
    res_img.height = img.height == UINT16_MAX ? -1 : img.height;

    if(img_ext)
    {
      res_img.orig_width = img_ext->orig_width;
      res_img.orig_height = img_ext->orig_height;
      
      res_img.alt_highlighted =
        img_ext->alt_status == TextStatusChecker::AS_HIGHLIGHTED;
    }
    else
    {
      res_img.orig_width = img.width;
      res_img.orig_height = img.height;
    }

    El::Python::Sequence* res_thumbs = res_img.thumbs.in();

    for(size_t j = 0; j < img.thumbs.size(); ++j)
    {
      const Message::ImageThumb& thumb = img.thumbs[j];
      
      SearchResult::ImageThumb& res_thumb =
        *SearchResult::ImageThumb::Type::down_cast((*res_thumbs)[j].in());
      
      res_thumb.width = thumb.width();
      res_thumb.height = thumb.height();
      res_thumb.cropped = thumb.cropped();

      std::ostringstream ostr;

      ostr << thumbnail_url <<
        El::Moment(ACE_Time_Value(message.published)).dense_format(
          El::Moment::DF_DATE) << "-" << message.id.string() << "-"
           << img_index << "-" << j << "-" << std::uppercase << std::hex
           << thumb.hash << "-" << std::uppercase << std::hex
           << message.signature << "." << thumb.type;

      El::String::Manip::xml_encode(
        ostr.str().c_str(),
        res_thumb.src,
        El::String::Manip::XE_TEXT_ENCODING |
        El::String::Manip::XE_ATTRIBUTE_ENCODING |
        El::String::Manip::XE_PRESERVE_UTF8 |
        El::String::Manip::XE_FORCE_NUMERIC_ENCODING);
    }
  }
  
  void
  SearchEngine::adjust_images(const SearchContext& ctx,
                              bool thumbnail_enable,
                              unsigned long image_max_width,
                              unsigned long image_max_height,
                              unsigned long front_image_max_width,
                              size_t show_number,
                              Message::Transport::StoredMessageArray& messages,
                              const Search::MessageWordPositionMap& positions,
                              MessageExtMap& msg_extras) const
    throw(El::Exception)
  {
    if((ctx.gm_flags & Message::Bank::GM_IMG) == 0)
    {
      return;
    }
    
    for(Message::Transport::StoredMessageArray::iterator
          it(messages.begin()), ie(messages.end()); it != ie; ++it)
    {
      Message::StoredImageArray* images = it->message.content->images.get();

      if(images && images->size() > ctx.max_image_count)
      { 
        images->resize(ctx.max_image_count);
      }
    }
    
    if(!ctx.adjust_images)
    {
      return;
    }

    for(Message::Transport::StoredMessageArray::iterator
          it(messages.begin()), ie(messages.end()); it != ie; ++it)
    {
      Message::StoredMessage& msg = it->message;
      Message::StoredImageArray* images = msg.content->images.get();      
      
      if(images && images->size())
      {        
        bool choose_front_image = false;
          
        if(show_number && (ctx.gm_flags & Message::Bank::GM_DESC))
        {
          EmptyChecker ec;
          msg.assemble_description(ec);
          choose_front_image = !ec.empty;
        }

        Search::MessageWordPositionMap::const_iterator pit =
          positions.find(msg.id);
        
        const Search::WordPositionArray* word_pos =
          pit == positions.end() ? 0 : &(pit->second);
        
        WordPositionSet word_positions;
        fill_position_set(word_pos, word_positions);
        
        unsigned short max_width = 0;
        size_t max_width_img_index = 0;

        MessageExt& msg_ext = msg_extras[msg.id];
        ImageExtArray& images_ext = msg_ext.images;

        images_ext.resize(images->size());
            
        for(size_t i = 0; i < images->size(); ++i)
        {
          Message::StoredImage& img = (*images)[i];
          ImageExt& img_ext = images_ext[i];
          
          TextStatusChecker tsc(word_positions);
          msg.assemble_image_alt(tsc, i);

          img_ext.orig_width = img.width;
          img_ext.orig_height = img.height;
          img_ext.index = i;
          img_ext.alt_status = tsc.status;
            
          double adjust_width_ratio = 1;
          double adjust_height_ratio = 1;
          bool adjust = false;

          if(img.width != UINT16_MAX && img.width > image_max_width)
          {
            adjust_width_ratio = (double)img.width / image_max_width;
            adjust = true;
          }
            
          if(img.height != UINT16_MAX && img.height > image_max_height)
          {
            adjust_height_ratio = (double)img.height / image_max_height;
            adjust = true;
          }

          if(adjust)
          {
            double adjust_ratio =
              std::max(adjust_width_ratio, adjust_height_ratio);
              
            if(img.width != UINT16_MAX)
            {
              img.width =
                (unsigned long)((double)img.width / adjust_ratio + 0.5);
            }
              
            if(img.height != UINT16_MAX)
            {
              img.height =
                (unsigned long)((double)img.height / adjust_ratio + 0.5);
            }            
          }

          if(choose_front_image)
          {
            uint16_t fi_max_width =
              ctx.in_2_columns ? front_image_max_width : UINT16_MAX;
          
            uint16_t img_width = thumbnail_enable && !img.thumbs.empty() ?
              img.thumbs[0].width() : img.width;
          
            if(img_ext.alt_status == TextStatusChecker::AS_EMPTY &&
               img_width <= fi_max_width && img_width > max_width)
            {
              max_width = img_width;
              max_width_img_index = i;
            }
          }
        }

        if(max_width > 0)
        {
          Message::StoredImage& img = (*images)[max_width_img_index];
            
          msg_ext.front_image.img = img;
          msg_ext.front_image.ext = images_ext[max_width_img_index];

          img.width = 0; // Mark as unused
        }
      }
    }
  }
  
  size_t
  SearchEngine::text_with_image_lines(size_t len,
                                      size_t img_width,
                                      size_t img_height)
    throw()
  {
    size_t img_char_width = img_width ? img_width / DESC_CHAR_WIDTH + 1 : 0;
    
    size_t img_char_height =
      img_height ? img_height / DESC_CHAR_HEIGHT + 1 : 0;

    len += img_char_width * img_char_height;
    
    size_t lines = len ? len / LINE_DESC_CHARS + 1 : 0;
    return std::max(lines, img_char_height);
  }

  unsigned long
  SearchEngine::ad_block_height(unsigned long height)
    throw(El::Exception)
  {
    return height ? (height / DESC_CHAR_HEIGHT + 3) : 0;
  }
  
  unsigned long
  SearchEngine::second_column_offset(
    const SearchContext& ctx,
    size_t word_max_len,
    bool show_thumb,
    size_t show_number,
    const Message::Transport::StoredMessageArray& messages,
    const MessageExtMap& msg_extras) const
    throw(El::Exception)
  {
    if(!ctx.in_2_columns)
    {
      return ULONG_MAX;
    }

    typedef std::vector<unsigned long> ULongArray;
    ULongArray sizes(messages.size());

    unsigned long total_lines = 0;
    unsigned long i = 0;

//    std::cerr << "*****************************\n";
    
    for(Message::Transport::StoredMessageArray::const_iterator
          it(messages.begin()), ie(messages.end()); it != ie; ++it)
    {
      const Message::Transport::StoredMessageDebug* msg =
        const_cast<const Message::Transport::StoredMessageDebug*>(&(*it));

      MessageExtMap::const_iterator mit = msg_extras.find(msg->message.id);
      
      const MessageExt* msg_ext = mit == msg_extras.end() ?
        0 : &mit->second;
      
      Message::StoredContent* content = msg->message.content;

      unsigned long lines = 3; // "virtual" text to compensate message
                               // footer, padding ...

      const Message::Categories::CategoryArray& categories =
        msg->message.categories.array;

      if(categories.size() > 0)
      {
        lines += categories.size() / 3 + 1;
      }
      else if(msg->message.event_capacity > 1)
      {
        lines += 1;
      }
          
      if(ctx.gm_flags & Message::Bank::GM_TITLE)
      {
        MessageWriter mw(word_max_len);
        msg->message.assemble_title(mw);

//        unsigned long l = lines;
        
        lines += mw.length / LINE_TITLE_CHARS + 2;

//        std::cerr << "title: " << lines - l << std::endl;
      }

      if(ctx.gm_flags & Message::Bank::GM_DESC)
      {
        MessageWriter mw(word_max_len);
        msg->message.assemble_description(mw);
        
        unsigned long height = 0;
        unsigned long width = 0;
          
        if(msg_ext)
        {
          const Message::StoredImage& img = msg_ext->front_image.img;
          
          if(show_thumb && img.thumbs.size() > 0)
          {
            height = img.thumbs[0].height();
            width = img.thumbs[0].width();
          }
          else
          {
            height = img.height;
            width = img.width;
          }
        }
        
//        unsigned long l = lines;
        
        lines += text_with_image_lines(mw.length, width, height);
        
//        std::cerr << "desc: " << lines - l << "(" << mw.length << ","
//                  << width << "," << height << ")" << std::endl;
      }

      if((ctx.gm_flags & Message::Bank::GM_IMG) &&
         msg->message.content->images.get())
      {            
        const Message::StoredImageArray& images = *(content->images);
        
        size_t to_show_number = show_number;

        if(msg_ext && msg_ext->front_image.img.width)
        {
          to_show_number--;
        }
        
//      std::cerr << "images:";
//      std::cerr << "  highlighted:";
        
        for(size_t j = 0; j < images.size(); ++j)
        {
          image_lines(msg->message,
                      images,
                      j,
                      true,
                      msg_ext,
                      show_thumb,
                      word_max_len,
                      lines,
                      to_show_number);
        }
        
//      std::cerr << "  other:";
        for(size_t j = 0; j < images.size(); ++j)
        {
          if(!image_lines(msg->message,
                          images,
                          j,
                          false,
                          msg_ext,
                          show_thumb,
                          word_max_len,
                          lines,
                          to_show_number))
          {
            lines += 1;
            break;
          }
        }

//        std::cerr << std::endl;
      }
        
      sizes[i++] = lines;
      total_lines += lines;

//      std::cerr << lines << std::endl;
    }

    unsigned long first_column_ad_lines =
      ad_block_height(ctx.first_column_ad_height);

    unsigned long second_column_ad_lines =
      ad_block_height(ctx.second_column_ad_height);

    bool multiple_messages = sizes.size() > 1;

    if(!multiple_messages && !second_column_ad_lines)
    {
      second_column_ad_lines = first_column_ad_lines;
      first_column_ad_lines = 0;
    }

    if(multiple_messages)
    { 
      total_lines += first_column_ad_lines;
    }

    total_lines += second_column_ad_lines;

    unsigned long half_size = total_lines / 2;

    unsigned long size1 = first_column_ad_lines;
    unsigned long i1 = 0;
    for(; i1 < sizes.size() && (size1 += sizes[i1++]) < half_size; );

    unsigned long size2 = second_column_ad_lines;
    unsigned long i2 = sizes.size();
    for(; i2 && (size2 += sizes[--i2]) < half_size; );

    unsigned long offset = (size1 - half_size) <= (size2 - half_size) ? i1: i2;
    
    if(offset == 0 && !sizes.empty())
    {
      offset = 1;
    }

    return offset;
  }

  bool
  SearchEngine::image_lines(const Message::StoredMessage& msg,
                            const Message::StoredImageArray& images,
                            size_t index,
                            bool highlighted,
                            const MessageExt* msg_ext,
                            bool show_thumb,
                            size_t word_max_len,
                            unsigned long& lines,
                            size_t& to_show_number) throw(El::Exception)
  { 
    const Message::StoredImage& img = images[index];

    if(!img.width)
    {
      return true; // front image; skip
    }

    bool highlighted_img = msg_ext &&
      msg_ext->images[index].alt_status == TextStatusChecker::AS_HIGHLIGHTED;

    if(highlighted != highlighted_img)
    {
      return true;
    }

    if(!highlighted_img && !to_show_number)
    {
      return false;
    }
    
    if(to_show_number)
    {
      to_show_number--;
    }
    
//          unsigned long l = lines;
                
    unsigned long height = 0;
    unsigned long width = 0;
          
    if(show_thumb && img.thumbs.size() > 0)
    {
      height = img.thumbs[0].height();
      width = img.thumbs[0].width();
    }
    else
    {
      height = img.height;
      width = img.width;
    }
    
    MessageWriter mw(word_max_len);
    msg.assemble_image_alt(mw, index);
          
    lines += text_with_image_lines(mw.length, width, height);          

//  std::cerr << " " << lines - l;
    return true;
  }
  
  SearchResult*
  SearchEngine::search(SearchContext& ctx,
                       Message::Transport::StoredMessageArrayPtr& messages,
                       Search::MessageWordPositionMap& positions)
    throw(Search::ParseError, CORBA::Exception, El::Exception)
  {
    SearchResult_var result =
      new SearchResult(&SearchResult::Type::instance, 0, 0);

    Strategy strategy(*strategy_);

    switch(ctx.suppression->type)
    {
    case Search::Strategy::ST_COLLAPSE_EVENTS:
      {
        if(ctx.suppression->param.in())
        {
          strategy.msg_per_event =
            std::max(El::Python::ulong_from_number(
                       ctx.suppression->param.in(),
                       "NewsGate::SearchEngine::search"),
                     (unsigned long)1);
        }
      }
    }

    ACE_High_Res_Timer timer;
    ACE_Time_Value search_duration;    
    Search::Expression_var search_expression;
    Message::SearchResult_var search_result;
    std::auto_ptr<Ad::SelectionResult> ads;
    
    std::string optimized_query;

    bool record_stat = ctx.record_stat && stat_logger_.in() != 0;
    
    {
      El::Python::AllowOtherThreads guard;

      if(record_stat)
      {
        timer.start();
      }
    
      std::wstring query;
      El::String::Manip::utf8_to_wchar(ctx.query.c_str(), query);
      
      Search::ExpressionParser parser(strategy.sort.impression_respected_level);
      std::wistringstream istr(query);
    
      parser.parse(istr);

      search_expression = parser.expression();
    
      if(ctx.optimize_query)
      {
        parser.optimize();
      
        if(record_stat || ctx.save_optimized_query)
        {
          std::ostringstream ostr;
          search_expression->condition->print(ostr);
          optimized_query = ostr.str();
        }
      
        if(ctx.save_optimized_query)
        {
          result->optimized_query = optimized_query;
        }
      }

      bool none =
        search_expression->condition->type() == Search::Condition::TP_NONE;
      
      bool search_words = !none && (parser.contain(Search::Condition::TP_ALL) ||
                                    parser.contain(Search::Condition::TP_ANY));

      if(none)
      {
        search_result = new Message::SearchResult();
      
        search_result->messages =
          new Message::Transport::StoredMessagePackImpl::Type(
            new Message::Transport::StoredMessageArray());

        search_result->category_locale =
          Message::Transport::CategoryLocaleImpl::Init::create(
            new Message::Categorizer::Category::Locale());
      
        search_result->stat =
          new Search::Transport::StatImpl::Type(new NewsGate::Search::Stat());

        search_result->total_matched_messages = 0;
        search_result->nochanges = 0;
        search_result->suppressed_messages = 0;
        search_result->etag = 0;

        result->message_load_status = SearchResult::MLS_UNKNOWN;
      }
      else
      {
        search_result =
          search(search_expression,
                 ctx,
                 search_words,
                 strategy,
                 search_duration);

        result->message_load_status = search_result->messages_loaded ?
          SearchResult::MLS_LOADED : SearchResult::MLS_LOADING;
      } 

      result->nochanges = search_result->nochanges;
//    result->search_time = search_result->search_time;
      result->etag = search_result->etag;
      result->total_matched_messages = search_result->total_matched_messages;
      result->suppressed_messages = search_result->suppressed_messages;

      Message::Transport::CategoryLocaleImpl::Type* category_locale =
        dynamic_cast<Message::Transport::CategoryLocaleImpl::Type*>(
          search_result->category_locale.in());
      
      if(category_locale == 0)
      {
        throw Exception(
          "NewsGate::SearchEngine::search: "
          "dynamic_cast<const Message::Transport::"
          "CategoryLocaleImpl::Type*> failed");
      }

      *result->category_locale.in() = category_locale->entity();

      Message::Transport::StoredMessagePackImpl::Type* msg_transport =
        dynamic_cast<Message::Transport::StoredMessagePackImpl::Type*>(
          search_result->messages.in());
      
      if(msg_transport == 0)
      {
        throw Exception(
          "NewsGate::SearchEngine::search: "
          "dynamic_cast<const Message::Transport::"
          "StoredMessagePackImpl::Type*> failed");
      }

      messages.reset(msg_transport->release());

      if(ctx.highlight && search_words)
      {
        Message::SearcheableMessageMap message_map(false, false, 0, 0);

        for(Message::Transport::StoredMessageArray::iterator
              it(messages->begin()), ie(messages->end()); it != ie; ++it)
        {
          message_map.insert(it->message, 0, 0/*, false*/);
        }
      
        Search::Strategy strategy;
        Search::ResultPtr res(
          search_expression->search(message_map, false, strategy, &positions));
      }

      if(ctx.page_ad_id != Ad::PI_UNKNOWN && !ad_server_.empty())
      {
        ads.reset(select_ads(ctx,
                             *messages,
                             search_expression->condition.in()));
      }
    }

    if(ads.get())
    {
      result->ad_selection->init(*ads);   
    }

    if(ctx.sr_flags)
    {
      Search::Transport::StatImpl::Type* stat =
        dynamic_cast<Search::Transport::StatImpl::Type*>(
          search_result->stat.in());
    
      if(stat == 0)
      {
        throw Exception(
          "PageValuesProvider::write_language_options: "
          "dynamic_cast<Search::Transport::StatImpl::Type*> failed");
      }
    
      Search::Stat& search_stat = stat->entity();

//      result->space_filtered = search_stat.space_filtered;

      if(ctx.sr_flags & Search::Strategy::RF_LANG_STAT)
      {
        fill_lang_filter_options(ctx,
                                 search_stat.lang_counter,
                                 *result->lang_filter_options);
      }

      if(ctx.sr_flags & Search::Strategy::RF_COUNTRY_STAT)
      {
        fill_country_filter_options(ctx,
                                    search_stat.country_counter,
                                    *result->country_filter_options);
      }

      if(ctx.sr_flags & Search::Strategy::RF_FEED_STAT)
      {
        fill_feed_filter_options(ctx,
                                 search_stat.feed_counter,
                                 *result->feed_filter_options);
      }

      if(ctx.sr_flags & Search::Strategy::RF_CATEGORY_STAT)
      {
        fill_category_stat(search_stat.category_counter,
                           *result->category_stat);        
      }
    }

    if(record_stat)
    {
      ACE_Time_Value tm;
      timer.stop();
      timer.elapsed_time(tm);
      
      log_stat(ctx,
               result.in(),
               search_result,
               optimized_query.c_str(),
               search_duration,
               tm);
    }    
    
    return result.retn();
  }

  Ad::SelectionResult*
  SearchEngine::select_ads(
    SearchContext& ctx,
    const Message::Transport::StoredMessageArray& messages,
    const Search::Condition* condition) const
    throw(El::Exception)
  {
    if(messages.empty())
    {
      return 0;
    }

    std::auto_ptr<Ad::SelectionContext> sel_ctx(
      new Ad::SelectionContext(ctx.page_ad_id, rand()));

    El::PSP::Request* request =
      El::PSP::Request::Type::down_cast(ctx.request.in());

    El::Apache::Request* ap_request = request->request();

    El::Apache::Request::In& in = ap_request->in();

    const char* referer = in.headers().find(El::Net::HTTP::HD_REFERER);
    
    if(referer)
    {
      El::Net::HTTP::SearchInfo si = El::Net::HTTP::search_info(referer);
      sel_ctx->search_engine = si.engine;
      sel_ctx->set_referer(referer);
    }

    sel_ctx->crawler = El::Net::HTTP::crawler(ctx.user_agent.c_str());
    sel_ctx->language = *ctx.locale->lang;
    sel_ctx->country = *ctx.locale->country;
    
    El::Net::ip(ap_request->remote_ip(), &sel_ctx->ip);

    sel_ctx->ad_caps.from_string(in.cookies().most_specific("gf"));
    sel_ctx->counter_caps.from_string(in.cookies().most_specific("cf"));

    Ad::SlotIdArray& slots = sel_ctx->slots;
    slots.reserve(20);
      
    switch(ctx.page_ad_id)
    {
    case Ad::PI_DESK_PAPER:
      {
        slots.push_back(Ad::SI_DESK_PAPER_MSG1);
        slots.push_back(Ad::SI_DESK_PAPER_MSA2);
          
        if(messages.size() > 1)
        {
          slots.push_back(Ad::SI_DESK_PAPER_MSG2);
          slots.push_back(Ad::SI_DESK_PAPER_MSA1);
        }

        slots.push_back(Ad::SI_DESK_PAPER_RTB1);
        slots.push_back(Ad::SI_DESK_PAPER_RTB2);
        slots.push_back(Ad::SI_DESK_PAPER_ROOF);
        slots.push_back(Ad::SI_DESK_PAPER_BASEMENT);
        break;
      }
    case Ad::PI_DESK_COLUMN:
      {
        slots.push_back(Ad::SI_DESK_COLUMN_MSG1);
        slots.push_back(Ad::SI_DESK_COLUMN_MSA1);
          
        if(messages.size() > 1)
        {
          slots.push_back(Ad::SI_DESK_COLUMN_MSG2);
        }
        
        slots.push_back(Ad::SI_DESK_COLUMN_RTB1);
        slots.push_back(Ad::SI_DESK_COLUMN_RTB2);
        slots.push_back(Ad::SI_DESK_COLUMN_ROOF);
        slots.push_back(Ad::SI_DESK_COLUMN_BASEMENT);
        break;
      }
    case Ad::PI_DESK_NLINE:
      {
        slots.push_back(Ad::SI_DESK_NLINE_MSA1);        
        slots.push_back(Ad::SI_DESK_NLINE_RTB1);
        slots.push_back(Ad::SI_DESK_NLINE_RTB2);
        slots.push_back(Ad::SI_DESK_NLINE_ROOF);
        slots.push_back(Ad::SI_DESK_NLINE_BASEMENT);
        break;
      }
    case Ad::PI_TAB_PAPER:
      {
        slots.push_back(Ad::SI_TAB_PAPER_MSG1);
        slots.push_back(Ad::SI_TAB_PAPER_MSA2);
          
        if(messages.size() > 1)
        {
          slots.push_back(Ad::SI_TAB_PAPER_MSG2);
          slots.push_back(Ad::SI_TAB_PAPER_MSA1);
        }

        slots.push_back(Ad::SI_TAB_PAPER_RTB1);
        slots.push_back(Ad::SI_TAB_PAPER_RTB2);
        slots.push_back(Ad::SI_TAB_PAPER_ROOF);
        slots.push_back(Ad::SI_TAB_PAPER_BASEMENT);
        break;
      }
    case Ad::PI_TAB_COLUMN:
      {
        slots.push_back(Ad::SI_TAB_COLUMN_MSG1);
        slots.push_back(Ad::SI_TAB_COLUMN_MSA1);
          
        if(messages.size() > 1)
        {
          slots.push_back(Ad::SI_TAB_COLUMN_MSG2);
        }
        
        slots.push_back(Ad::SI_TAB_COLUMN_RTB1);
        slots.push_back(Ad::SI_TAB_COLUMN_RTB2);
        slots.push_back(Ad::SI_TAB_COLUMN_ROOF);
        slots.push_back(Ad::SI_TAB_COLUMN_BASEMENT);
        break;
      }
    case Ad::PI_TAB_NLINE:
      {
        slots.push_back(Ad::SI_TAB_NLINE_MSA1);        
        slots.push_back(Ad::SI_TAB_NLINE_RTB1);
        slots.push_back(Ad::SI_TAB_NLINE_RTB2);
        slots.push_back(Ad::SI_TAB_NLINE_ROOF);
        slots.push_back(Ad::SI_TAB_NLINE_BASEMENT);
        break;
      }
    case Ad::PI_MOB_COLUMN:
      {
        slots.push_back(Ad::SI_MOB_COLUMN_MSG1);
        slots.push_back(Ad::SI_MOB_COLUMN_MSA1);
          
        if(messages.size() > 1)
        {
          slots.push_back(Ad::SI_MOB_COLUMN_MSG2);
        }
        
        slots.push_back(Ad::SI_MOB_COLUMN_ROOF);
        slots.push_back(Ad::SI_MOB_COLUMN_BASEMENT);
        break;
      }
    case Ad::PI_MOB_NLINE:
      {
        slots.push_back(Ad::SI_MOB_NLINE_MSA1);
        slots.push_back(Ad::SI_MOB_NLINE_ROOF);
        slots.push_back(Ad::SI_MOB_NLINE_BASEMENT);
        break;
      }
    case Ad::PI_DESK_MESSAGE:
      {
        if(messages[0].message.content->images.get())
        {
          slots.push_back(Ad::SI_DESK_MESSAGE_IMG);
          slots.push_back(Ad::SI_DESK_MESSAGE_MSG2);
        }
        else
        {
          slots.push_back(Ad::SI_DESK_MESSAGE_MSG1);

          if(messages[0].message.event_capacity > 1)
          {
            slots.push_back(Ad::SI_DESK_MESSAGE_MSG2);
          }
        }

        slots.push_back(Ad::SI_DESK_MESSAGE_RTB1);
        slots.push_back(Ad::SI_DESK_MESSAGE_RTB2);
        slots.push_back(Ad::SI_DESK_MESSAGE_ROOF);
        slots.push_back(Ad::SI_DESK_MESSAGE_BASEMENT);
        break;
      }
    case Ad::PI_TAB_MESSAGE:
      {
        if(messages[0].message.content->images.get())
        {
          slots.push_back(Ad::SI_TAB_MESSAGE_IMG);
          slots.push_back(Ad::SI_TAB_MESSAGE_MSG2);
        }
        else
        {
          slots.push_back(Ad::SI_TAB_MESSAGE_MSG1);

          if(messages[0].message.event_capacity > 1)
          {
            slots.push_back(Ad::SI_TAB_MESSAGE_MSG2);
          }
        }

        slots.push_back(Ad::SI_TAB_MESSAGE_RTB1);
        slots.push_back(Ad::SI_TAB_MESSAGE_RTB2);
        slots.push_back(Ad::SI_TAB_MESSAGE_ROOF);
        slots.push_back(Ad::SI_TAB_MESSAGE_BASEMENT);
        break;
      }
    case Ad::PI_MOB_MESSAGE:
      {
        if(messages[0].message.content->images.get())
        {
          slots.push_back(Ad::SI_MOB_MESSAGE_IMG);
          slots.push_back(Ad::SI_MOB_MESSAGE_MSG2);
        }
        else
        {
          slots.push_back(Ad::SI_MOB_MESSAGE_MSG1);

          if(messages[0].message.event_capacity > 1)
          {
            slots.push_back(Ad::SI_MOB_MESSAGE_MSG2);
          }
        }

        slots.push_back(Ad::SI_MOB_MESSAGE_ROOF);
        slots.push_back(Ad::SI_MOB_MESSAGE_BASEMENT);
        break;
      }
    case Ad::PI_UNKNOWN:
    case Ad::PI_COUNT:
      break;
    }

    for(Message::Transport::StoredMessageArray::const_iterator
          i(messages.begin()), e(messages.end()); i != e; ++i)
    {
      const Message::StoredMessage& message = i->message;
      
      const char* src = message.source_url.c_str();
      sel_ctx->add_message_source(src);
      sel_ctx->content_languages.insert(message.lang);

      switch(ctx.page_ad_id)
      {
      case Ad::PI_DESK_MESSAGE:
      case Ad::PI_TAB_MESSAGE:
      case Ad::PI_MOB_MESSAGE:
        {
          sel_ctx->add_page_source(src);
          break;
        }
      default: break;
      }      
      
      const Message::Categories::CategoryArray& categories =
        message.categories.array;

      for(Message::Categories::CategoryArray::const_iterator
            i(categories.begin()), e(categories.end()); i != e; ++i)
      {
        const char* cat = i->c_str();
        sel_ctx->add_message_category(cat);

        switch(ctx.page_ad_id)
        {
        case Ad::PI_DESK_MESSAGE:
        case Ad::PI_TAB_MESSAGE:
        case Ad::PI_MOB_MESSAGE:
          {
            sel_ctx->add_page_category(cat);
            break;
          }
        default: break;
        }
      }
    }

    const Search::Url* url_cond =
      dynamic_cast<const Search::Url*>(condition);

    if(url_cond)
    {      
      for(Search::Url::UrlList::const_iterator
            i(url_cond->urls.begin()), e(url_cond->urls.end()); i != e; ++i)
      {
        sel_ctx->add_page_source(i->c_str());
      }
    }

    const Search::Category* cat_cond =
      dynamic_cast<const Search::Category*>(condition);

    if(cat_cond)
    {      
      for(Search::Category::CategoryArray::const_iterator
            i(cat_cond->categories.begin()),
            e(cat_cond->categories.end()); i != e; ++i)
      {
        sel_ctx->add_page_category(i->c_str());
      }      
    }

    sel_ctx->add_page_category(ctx.filter->category.c_str());
    sel_ctx->add_page_source(ctx.filter->feed.c_str());

    if(url_cond || !ctx.filter->feed.empty())
    {
      sel_ctx->query_types |= Ad::QT_SOURCE;
    }

    if(cat_cond || !ctx.filter->category.empty())
    {
      sel_ctx->query_types |= Ad::QT_CATEGORY;
    }

    if(dynamic_cast<const Search::Event*>(condition) || ctx.filter->event)
    {
      sel_ctx->query_types |= Ad::QT_EVENT;
    }

    if(dynamic_cast<const Search::Words*>(condition))
    {
      sel_ctx->query_types |= Ad::QT_SEARCH;
    }

    switch(ctx.page_ad_id)
    {
    case Ad::PI_DESK_MESSAGE:
    case Ad::PI_TAB_MESSAGE:
    case Ad::PI_MOB_MESSAGE:
      {
        sel_ctx->query_types |= Ad::QT_MESSAGE;
        break;
      }
    default: break;
    }

    const El::Net::HTTP::ParamList& params = in.parameters();

    for(El::Net::HTTP::ParamList::const_iterator i(params.begin()),
          e(params.end()); i != e; ++i)
    {
      if(strcasecmp(i->name.c_str(), "at") == 0)
      {
        sel_ctx->add_tag(i->value.c_str());
      }
    }

    try
    {
      Ad::AdServer_var ad_server = ad_server_.object();

      sel_ctx->ad_caps.set_current_time();
      sel_ctx->counter_caps.current_time = sel_ctx->ad_caps.current_time;
      
      Ad::Transport::SelectionContextImpl::Var
        ctx_transport =
        Ad::Transport::SelectionContextImpl::Init::create(sel_ctx.release());
      
      Ad::Transport::SelectionResult_var res =
        ad_server->select(Ad::AdServer::INTERFACE_VERSION,
                           ctx_transport.in());

      Ad::Transport::SelectionResultImpl::Type* res_transport =
        dynamic_cast<Ad::Transport::SelectionResultImpl::Type*>(res.in());

      if(res_transport == 0)
      {
        logger_->alert("NewsGate::SearchEngine::select_ads: dynamic_cast<"
                       "Ad::Transport::SelectionResultImpl::Type*> failed",
                       ASPECT);        
      }

      const Ad::SelectionResult& sr = res_transport->entity();
      
//      std::cerr  << "ads:" << sr.ads.size() << std::endl;
            
      for(Ad::SelectionArray::const_iterator i(sr.ads.begin()),
            e(sr.ads.end()); i != e; ++i)
      {
        switch(i->slot)
        {
        case Ad::SI_DESK_PAPER_MSG1:
        case Ad::SI_DESK_PAPER_MSA1:
          {
            ctx.first_column_ad_height += i->height;
            break;
          }
        case Ad::SI_DESK_PAPER_MSG2:
        case Ad::SI_DESK_PAPER_MSA2:
          {
            ctx.second_column_ad_height += i->height;
            break;
          }
        case Ad::SI_TAB_PAPER_MSG1:
        case Ad::SI_TAB_PAPER_MSA1:
          {
            ctx.first_column_ad_height += i->height;
            break;
          }
        case Ad::SI_TAB_PAPER_MSG2:
        case Ad::SI_TAB_PAPER_MSA2:
          {
            ctx.second_column_ad_height += i->height;
            break;
          }
        default: break;
        }
      }

      return res_transport->release();
    }
    catch(const Ad::IncompatibleVersion& e)
    {
    }
    catch(const Ad::ImplementationException& e)
    {
      std::ostringstream ostr;
      ostr << "NewsGate::SearchEngine::select_ads: "
        "Ad::ImplementationException caught. Description:\n"
           << e.description.in();
        
      logger_->alert(ostr.str().c_str(), ASPECT);
    }
    catch(const CORBA::Exception& e)
    {
      std::ostringstream ostr;
      ostr << "NewsGate::SearchEngine::select_ads: "
        "CORBA::Exception caught. Description:\n"
           << e;
        
      logger_->alert(ostr.str().c_str(), ASPECT);
    }

    return 0;
  }
  
  void
  SearchEngine::log_stat(const SearchContext& ctx,
                         SearchResult* result,
                         const Message::SearchResult* search_result,
                         const char* optimized_query,
                         const ACE_Time_Value& search_duration,
                         const ACE_Time_Value& request_duration) const
    throw(El::Exception)
  {
    El::PSP::Request* request =
      El::PSP::Request::Type::down_cast(ctx.request.in());

    El::Apache::Request* ap_request = request->request();
        
    Statistics::RequestInfo ri;

    El::Guid guid;
    guid.generate();
      
    strcpy(ri.id, guid.string(El::Guid::GF_DENSE).c_str());
    result->request_id = ri.id;

    ri.time.time(ap_request->time());
      
    strcpy(ri.host, hostname_);

    Statistics::ClientInfo& ci = ri.client;
      
    strcpy(ci.ip, ap_request->remote_ip());
    strcpy(ci.lang, ctx.lang->l3_code());
    strcpy(ci.country, ctx.country->l3_code());
      
    Statistics::StatLogger::guid(
      ap_request->in().cookies().most_specific("u"), ci.id);

/*      
        const char* user_agent =
        ap_request->in().headers().find(El::Net::HTTP::HD_USER_AGENT);

        if(user_agent)
        {
        ci.agent(user_agent); 
        }
*/

    ci.agent(ctx.user_agent.c_str());
      
    const char* referer =
      ap_request->in().headers().find(El::Net::HTTP::HD_REFERER);

    if(referer)
    {
      Statistics::RefererInfo& rf = ri.referer;

      try
      {
        rf.referer(referer,
                   ap_request->in().headers().find(El::Net::HTTP::HD_HOST));
      }
      catch(const El::Net::URL::Exception&)
      {
      }
    }
      
    Statistics::RequestParams& rp = ri.params;

    strncpy(rp.query, ctx.query.c_str(), sizeof(rp.query) - 1);
    rp.columns = ctx.in_2_columns ? 2 : 1;
    rp.sorting_type = ctx.sorting_type;
    rp.suppression_type = ctx.suppression->type;

    const char* proto = ctx.protocol.empty() ? 0 : ctx.protocol.c_str();
    const char* search_modifier = 0;
    const char* ir_param = 0;

    const El::Net::HTTP::ParamList& params = ap_request->in().parameters();

    for(El::Net::HTTP::ParamList::const_iterator i(params.begin()),
          e(params.end()); i != e; ++i)
    {
      const char* name = i->name.c_str();

      if(strcasecmp(name, "t") == 0)
      {
        if(proto == 0)
        {
          proto = i->value.c_str();
        }
      }
      else if(strcasecmp(name, "v") == 0)
      {
        if(search_modifier == 0)
        {
          search_modifier = i->value.c_str();
        }
      }
      else if(strcasecmp(name, "ir") == 0)
      {
        if(ir_param == 0)
        {
          ir_param = i->value.c_str();
        }
      }
    }
      
    if(proto && strlen(proto) == 1)
    {
      switch(proto[0])
      {
      case 'r':
      case 'j':
      case 'o':
      case 'x': rp.protocol = toupper(proto[0]);
      default: break;
      }
    }

    switch(rp.protocol)
    {
    case 'H':
    case 'J':
    case 'O':
      {
        int create_informer = 0;
        El::String::Manip::numeric(ir_param, create_informer);          
        rp.create_informer = create_informer != 0;
          
        break;
      }
    }

    if(rp.create_informer || rp.protocol == 'J' || rp.protocol == 'O')
    {
      std::ostringstream ostr;
        
      for(const char** p(INFORMER_PARAMS); **p != '\0'; ++p)
      {
        const char* val = params.find(*p);

        if(val)
        {
          ostr << "&" << *p << "=";
          El::String::Manip::mime_url_encode(val, ostr);
        }
      }

      std::string params = ostr.str();
        
      strncpy(rp.informer_params,
              params.c_str(),
              sizeof(rp.informer_params) - 1);
    }

    rp.gm_flags = ctx.gm_flags;
    rp.sr_flags = ctx.sr_flags;
    rp.start_item = ctx.start_from;
    rp.item_count = ctx.results_count;

    if(ctx.message_view == "nline")
    {
      rp.message_view = 'L';
    }

    rp.print_left_bar = ctx.print_left_bar;
      
    rp.annotation_len = ctx.gm_flags & Message::Bank::GM_DESC ?
      (ctx.max_desc_len ? ctx.max_desc_len : -1) : 0;

    strncpy(rp.translate_def_lang,
            ctx.translate_def_lang.c_str(),
            sizeof(rp.translate_def_lang) - 1);
      
    strncpy(rp.translate_lang,
            ctx.translate_lang.c_str(),
            sizeof(rp.translate_lang) - 1);
      
    Statistics::Filter& fl = rp.filter;

    fl.event = ctx.filter->event;
      
    strncpy(fl.country,
            ctx.filter->country->l3_code(),
            sizeof(fl.country) - 1);

    strncpy(fl.lang,
            ctx.filter->lang->l3_code(),
            sizeof(fl.lang) - 1);

    strncpy(fl.feed,
            ctx.filter->feed.c_str(),
            sizeof(fl.feed) - 1);

    strncpy(fl.category,
            ctx.filter->category.c_str(),
            sizeof(fl.category) - 1);

    if(search_modifier)
    {
      Statistics::SearchModifier& sm = rp.modifier;
        
      char type = toupper(search_modifier[0]);
        
      switch(type)
      {
      case 'E':
      case 'S':
      case 'C': strncpy(sm.value, search_modifier + 1, sizeof(sm.value) - 1);
      case 'A': sm.type = type; break;
      default: break;
      }

      if(type == 'C')
      {
        size_t len = strlen(sm.value);

        if(len > 1 && sm.value[len - 1] == '/')
        {
          sm.value[len - 1] = '\0';
        }
      }
    }

    Statistics::Locale& lc = rp.locale;

    strncpy(lc.country,
            ctx.locale->country->l3_code(),
            sizeof(fl.country) - 1);

    strncpy(lc.lang,
            ctx.locale->lang->l3_code(),
            sizeof(fl.lang) - 1);


    Statistics::ResponseInfo& rs = ri.response;

    rs.messages_loaded = result->message_load_status;
    rs.total_matched_messages = search_result->total_matched_messages;
    rs.suppressed_messages = search_result->suppressed_messages;

    strncpy(rs.optimized_query,
            optimized_query,
            sizeof(rs.optimized_query) - 1);
        
    ri.search_duration = search_duration.msec();
    ri.request_duration = request_duration.msec();
            
    stat_logger_->search_request(ri);
  }

  Message::SearchResult*
  SearchEngine::search(Search::Expression_var& expression,
                       const SearchContext& ctx,
                       bool search_words,
                       const Strategy& strategy,
                       ACE_Time_Value& search_duration)
    throw(CORBA::Exception, El::Exception)
  {
    Search::Strategy::SortingMode sorting_type =
      (Search::Strategy::SortingMode)ctx.sorting_type;
    
    bool normalize_words = search_words;

    if(!search_words && !ctx.search_hint.empty())
    {
      try
      {
        std::wstring search_hint;
        El::String::Manip::utf8_to_wchar(ctx.search_hint.c_str(), search_hint);

        std::wstring search_hint_lowered;
        El::String::Manip::to_lower(search_hint.c_str(), search_hint_lowered);

        search_hint = L"ANY " + search_hint_lowered;
    
        Search::ExpressionParser search_hint_parser;
        std::wistringstream istr(search_hint);

        search_hint_parser.parse(istr);

        Search::Expression_var search_hint_expression =
          search_hint_parser.expression();

        Search::AnyWords* any_op = dynamic_cast<Search::AnyWords*>(
          search_hint_expression->condition.in());

        assert(any_op != 0);

        size_t word_count = any_op->words.size();
        
        any_op->match_threshold =
          std::min(word_count, std::max((size_t)2, word_count * 3 / 4));

        Search::Or* or_op = new Search::Or();

        or_op->operands.push_back(new Search::Every());
        or_op->operands.push_back(search_hint_expression->condition);
    
        Search::And* and_op = new Search::And();

        and_op->operands.push_back(expression->condition);
        and_op->operands.push_back(or_op);

        expression->condition = and_op;
        sorting_type = Search::Strategy::SM_BY_RELEVANCE_DESC;

        normalize_words = true;
      }
      catch(const El::Exception&)
      {
      }
    }

    Search::Strategy::SuppressionPtr suppression;
      
    switch(ctx.suppression->type)
    {
    case Search::Strategy::ST_NONE:
      {
        suppression.reset(new Search::Strategy::SuppressNone());
        break;
      }
    case Search::Strategy::ST_DUPLICATES:
      {
        suppression.reset(new Search::Strategy::SuppressDuplicates());
        break;
      }      
    case Search::Strategy::ST_SIMILAR:
      {
        SearchContext::Suppression::CoreWords& cwp =
          *ctx.suppression->core_words;
        
        suppression.reset(
          new Search::Strategy::SuppressSimilar(
            cwp.intersection == ULONG_MAX ?
            strategy.suppress.core_words.intersection : cwp.intersection,
            cwp.containment_level == ULONG_MAX ?
            strategy.suppress.core_words.containment_level :
            cwp.containment_level,
            cwp.min_count == ULONG_MAX ? strategy.suppress.core_words.min :
            cwp.min_count));
        
        break;
      }      
    case Search::Strategy::ST_COLLAPSE_EVENTS:
      {
        SearchContext::Suppression::CoreWords& cwp =
          *ctx.suppression->core_words;
        
        suppression.reset(new Search::Strategy::CollapseEvents(
            cwp.intersection == ULONG_MAX ?
            strategy.suppress.core_words.intersection : cwp.intersection,
            cwp.containment_level == ULONG_MAX ?
            strategy.suppress.core_words.containment_level :
            cwp.containment_level,
            cwp.min_count == ULONG_MAX ? strategy.suppress.core_words.min :
            cwp.min_count,
            strategy.msg_per_event));
        
        break;
      }
    default:
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchEngine::search: unexpected value '"
             << ctx.suppression->type
             << "' for SearchContext.Suppression.type";

        throw Exception(ostr.str());
      }
    }

    Search::Strategy::SortingPtr sorting;
      
    switch(sorting_type)
    {
    case Search::Strategy::SM_BY_RELEVANCE_DESC:
      {
        sorting.reset(new Search::Strategy::SortByRelevanceDesc(
                        strategy.sort.msg_max_age,
                        strategy.sort.by_relevance.max_core_words,
                        strategy.sort.impression_respected_level));
        break;
      }
    case Search::Strategy::SM_BY_RELEVANCE_ASC:
      {
        sorting.reset(new Search::Strategy::SortByRelevanceAsc(
                        strategy.sort.msg_max_age,
                        strategy.sort.by_relevance.max_core_words,
                        strategy.sort.impression_respected_level));
        break;
      }
    case Search::Strategy::SM_BY_PUB_DATE_DESC:
      {
        sorting.reset(new Search::Strategy::SortByPubDateDesc(
                        strategy.sort.msg_max_age));
        break;
      }      
    case Search::Strategy::SM_BY_FETCH_DATE_DESC:
      {
        sorting.reset(new Search::Strategy::SortByFetchDateDesc(
                        strategy.sort.msg_max_age));
        break;
      }
    case Search::Strategy::SM_BY_PUB_DATE_ASC:
      {
        sorting.reset(new Search::Strategy::SortByPubDateAcs(
                        strategy.sort.msg_max_age));
        break;
      }      
    case Search::Strategy::SM_BY_FETCH_DATE_ASC:
      {
        sorting.reset(new Search::Strategy::SortByFetchDateAcs(
                        strategy.sort.msg_max_age));
        break;
      }
    case Search::Strategy::SM_BY_EVENT_CAPACITY_DESC:
      {
        sorting.reset(new Search::Strategy::SortByEventCapacityDesc(
                        strategy.sort.msg_max_age,
                        strategy.sort.by_capacity.event_max_size,
                        strategy.sort.impression_respected_level));
        break;
      }
    case Search::Strategy::SM_BY_EVENT_CAPACITY_ASC:
      {
        sorting.reset(new Search::Strategy::SortByEventCapacityAsc(
                        strategy.sort.msg_max_age,
                        strategy.sort.by_capacity.event_max_size,
                        strategy.sort.impression_respected_level));
        break;
      }
    case Search::Strategy::SM_BY_POPULARITY_DESC:
      {
        sorting.reset(
          new Search::Strategy::SortByPopularityDesc(
            strategy.sort.msg_max_age,
            strategy.sort.impression_respected_level));
        
        break;
      }
    case Search::Strategy::SM_BY_POPULARITY_ASC:
      {
        sorting.reset(
          new Search::Strategy::SortByPopularityAsc(
            strategy.sort.msg_max_age,
            strategy.sort.impression_respected_level));
        
        break;
      }
    case Search::Strategy::SM_NONE:
      {
        sorting.reset(new Search::Strategy::SortNone());
        break;
      }
    default:
      {
        std::ostringstream ostr;
        ostr << "NewsGate::SearchEngine::search: unexpected value '"
             << ctx.sorting_type << "' for SearchContext.sorting_type";

        throw Exception(ostr.str());
      }
    }    

    expression->add_ref();

    Search::Transport::ExpressionImpl::Var
      expression_transport =
      Search::Transport::ExpressionImpl::Init::create(
        new Search::Transport::ExpressionHolder(expression));
    
    if(normalize_words)
    {
      try
      {
        NewsGate::Search::Transport::Expression_var result;
      
        Dictionary::WordManager_var word_manager = word_manager_.object();
        
        word_manager->normalize_search_expression(
          expression_transport.in(),
          result.out());
        
        if(dynamic_cast<Search::Transport::ExpressionImpl::Type*>(
             result.in()) == 0)
        {
          throw Exception(
            "NewsGate::SearchEngine::search: dynamic_cast<"
            "Search::Transport::ExpressionImpl::Type*> failed");
        }
        
        expression_transport =
          dynamic_cast<Search::Transport::ExpressionImpl::Type*>(
            result._retn());
        
        expression = expression_transport->entity().expression;
      }
      catch(const Dictionary::NotReady& e)
      {
        // Will use not normalized search meanwhile
        
        std::ostringstream ostr;
        ostr << "NewsGate::SearchEngine::search: word manager is not ready."
          " Reason: " << e.reason.in();
        
        logger_->trace(ostr.str().c_str(), ASPECT, El::Logging::MIDDLE);
      }
    }
    
    Search::Strategy::Filter search_filter;
    
    search_filter.lang = *ctx.filter->lang;
    search_filter.country = *ctx.filter->country;
    search_filter.feed = ctx.filter->feed;
    search_filter.category = ctx.filter->category;
    search_filter.event.data = ctx.filter->event;
/*
    El::String::ListParser parser(ctx.filter->spaces.c_str());
    const char* item = 0;

    while((item = parser.next_item()) != 0)
    {
      NewsGate::Feed::Space fspace = NewsGate::Feed::space(item);

      if(fspace != NewsGate::Feed::SP_NONEXISTENT)
      {
        search_filter.spaces.insert(fspace);
      }
    }
*/
  
    Search::Transport::StrategyImpl::Var strategy_transport = 
      Search::Transport::StrategyImpl::Init::create(
        new Search::Strategy(sorting.release(),
                             suppression.release(),
                             false,
                             search_filter,
                             ctx.sr_flags | Search::Strategy::RF_MESSAGES));
    
    Message::SearchRequest_var search_request =
      new Message::SearchRequest();

    search_request->gm_flags = ctx.gm_flags;
    search_request->thumb_index = -1;
    search_request->expression = expression_transport._retn();
    search_request->strategy = strategy_transport._retn();
    search_request->start_from = ctx.start_from;
    search_request->results_count = ctx.results_count;
      
    search_request->etag = ctx.etag;

    ::NewsGate::Message::SearchLocale& locale = search_request->locale;
    
    locale.lang = ctx.locale->lang->el_code();
    locale.country = ctx.locale->country->el_code();
    
    std::string cpath;
    El::String::Manip::utf8_to_lower(ctx.category_locale.c_str(), cpath);

    if(cpath[0] != '/')
    {
      cpath = std::string("/") + cpath;
    }        

    if(cpath[cpath.length() - 1] != '/')
    {
      cpath += "/";
    }
    
    search_request->category_locale = cpath.c_str();
      
    Message::SearchResult_var search_result;
      
    {
#   ifdef SEARCH_PROFILING
      El::Stat::TimeMeasurement measurement(search_meter_);
#   endif

      Message::BankClientSession_var session = bank_client_session();

      ACE_High_Res_Timer timer2;
      
      bool record_stat = ctx.record_stat && stat_logger_.in() != 0;

      if(record_stat)
      {
        timer2.start();
      }

      session->search(search_request.in(), search_result.out());

      if(record_stat)
      {
        timer2.stop();
        timer2.elapsed_time(search_duration);
      }
    }

    return search_result._retn();
  }
  
  Message::BankClientSession*
  SearchEngine::bank_client_session() throw(Exception, El::Exception)
  {
    {
      ReadGuard guard(lock_);

      if(bank_client_session_.in() != 0)
      {
        guard.release();
        
        bank_client_session_->_add_ref();
        return bank_client_session_.in();
      }
    }

    try
    {
      Message::BankManager_var message_bank_manager =
        message_bank_manager_.object();

      Message::BankClientSession_var bank_client_session =
        message_bank_manager->bank_client_session();
      
      if(bank_client_session.in() == 0)
      {
        throw Exception(
          "NewsGate::SearchEngine::bank_client_session: "
          "bank_client_session.in() == 0");
      }
      
      Message::BankClientSessionImpl* bank_client_session_impl =
        dynamic_cast<Message::BankClientSessionImpl*>(
          bank_client_session.in());
      
      if(bank_client_session_impl == 0)
      {
        throw Exception(
          "NewsGate::SearchEngine::bank_client_session: "
          "dynamic_cast<Message::BankClientSessionImpl*> failed");
      }      
    
      WriteGuard guard(lock_);
      
      if(bank_client_session_.in() != 0)
      {
        guard.release();
        
        bank_client_session_->_add_ref();
        return bank_client_session_.in();
      }
      
      bank_client_session_impl->_add_ref();
      bank_client_session_ = bank_client_session_impl;

      unsigned long threads =
        std::min(bank_client_session_->threads() *
                 message_bank_clients_, message_bank_clients_max_threads_);

      bank_client_session_->init_threads(this, threads);
      
      bank_client_session_->_add_ref();
      return bank_client_session_.in();
    }
    catch(const Message::ImplementationException& e)
    {
      std::ostringstream ostr;
      ostr << "NewsGate::SearchEngine::bank_client_session: "
           << "Message::ImplementationException caught. "
        "Description:\n" << e.description.in();

      throw Exception(ostr.str());
    }
    catch(const CORBA::Exception& e)
    {
      std::ostringstream ostr;
      ostr << "NewsGate::SearchEngine::bank_client_session: "
        "CORBA::Exception caught. Description:\n" << e;

      throw Exception(ostr.str());
    }    
  }

  Event::BankClientSession*
  SearchEngine::event_bank_client_session() throw(Exception, El::Exception)
  {
    if(event_bank_manager_.empty())
    {
      return 0;
    }
    
    {
      ReadGuard guard(lock_);

      if(event_bank_client_session_.in() != 0)
      {
        guard.release();
        
        event_bank_client_session_->_add_ref();
        return event_bank_client_session_.in();
      }
    }

    try
    {
      Event::BankManager_var event_bank_manager =
        event_bank_manager_.object();
      
      Event::BankClientSession_var event_bank_client_session =
        event_bank_manager->bank_client_session();
      
      if(event_bank_client_session.in() == 0)
      {
        throw Exception(
          "NewsGate::SearchEngine::event_bank_client_session: "
          "event_bank_client_session.in() == 0");
      }
      
      Event::BankClientSessionImpl* event_bank_client_session_impl =
        dynamic_cast<Event::BankClientSessionImpl*>(
          event_bank_client_session.in());
      
      if(event_bank_client_session_impl == 0)
      {
        throw Exception(
          "NewsGate::SearchEngine::event_bank_client_session: "
          "dynamic_cast<Event::BankClientSessionImpl*> failed");
      }      
    
      WriteGuard guard(lock_);
      
      if(event_bank_client_session_.in() != 0)
      {
        guard.release();
        
        event_bank_client_session_->_add_ref();
        return event_bank_client_session_.in();
      }
      
      event_bank_client_session_impl->_add_ref();
      event_bank_client_session_ = event_bank_client_session_impl;
      
      event_bank_client_session_->init_threads(this);
      
      event_bank_client_session_->_add_ref();
      return event_bank_client_session_.in();
    }
    catch(const Event::ImplementationException& e)
    {
      std::ostringstream ostr;
      ostr << "NewsGate::SearchEngine::event_bank_client_session: "
           << "Event::ImplementationException caught. "
        "Description:\n" << e.description.in();

      throw Exception(ostr.str());
    }
    catch(const CORBA::Exception& e)
    {
      std::ostringstream ostr;
      ostr << "NewsGate::SearchEngine::event_bank_client_session: "
        "CORBA::Exception caught. Description:\n" << e;

      throw Exception(ostr.str());
    }    
  }

  void
  SearchEngine::fill_category_stat(Search::StringCounterMap& category_counter,
                                   Category& category_stat) const
    throw(Exception, El::Exception)
  {
//    std::cerr << "\nSearchEngine::fill_category_stat:\n";
    
    for(Search::StringCounterMap::const_iterator it(category_counter.begin()),
          ie(category_counter.end()); it != ie; ++it)
    {
//      std::cerr << it->second << " : " << it->first.c_str() << std::endl;    
      Category* cat = category_stat.find(it->first.c_str(), true);

      const Search::Counter& counter = it->second;
      cat->message_count += counter.count1;
      cat->matched_message_count += counter.count2;
      cat->id = counter.id;

      El::String::Manip::utf8_to_wchar(counter.name.c_str(),
                                       cat->localized_name);
    }

    category_stat.sort_by_localized_name();
    category_stat.set_parent(0);
    
//    category_stat.dump(std::cerr, 0);
  }

  void
  SearchEngine::fill_country_filter_options(
    const SearchContext& ctx,
    Search::CountryCounterMap& country_counter,
    El::Python::Sequence& country_filter_options) const
    throw(Exception, El::Exception)
  {
    if(*ctx.lang == El::Lang::null)
    {
      return;
    }
    
    unsigned long total_messages = 0;
    
    for(Search::CountryCounterMap::const_iterator it(country_counter.begin()),
          ie(country_counter.end()); it != ie; ++it)
    {
      total_messages += it->second;
    }

    country_counter[El::Country::null] = total_messages;
    
    if(country_counter.find(*ctx.filter->country) == country_counter.end())
    {
      country_counter[*ctx.filter->country] = 0;
    }

    if(country_counter.find(*ctx.country) == country_counter.end())
    {
      country_counter[*ctx.country] = 0;
    }

    for(Search::CountryCounterMap::const_iterator it(country_counter.begin()),
          ie(country_counter.end()); it != ie; ++it)
    {
      const El::Country& country = it->first;

      std::wstring value;
      
      if(country == El::Country::null)
      {
        El::String::Manip::utf8_to_wchar(ctx.all_option.c_str(), value);
      }
      else
      { 
        std::ostringstream ostr;
        El::Loc::Localizer::instance().country(country, *ctx.lang, ostr);
        El::String::Manip::utf8_to_wchar(ostr.str().c_str(), value);
      }

//      std::wostringstream wostr;
//      wostr << value << L" (" << it->second << L")";
//      value = wostr.str();

      El::Python::Sequence::iterator oit(country_filter_options.begin());
      El::Python::Sequence::iterator oie(country_filter_options.end());

      for(; oit != oie &&
            static_cast<SearchResult::Option*>(oit->in())->value < value;
          ++oit);

      std::wstring id;
      El::String::Manip::utf8_to_wchar(country.l3_code(), id);
      
      SearchResult::Option_var option =
        new SearchResult::Option(
          id.c_str(),
          it->second,
          value.c_str(),
          *ctx.filter->country == country);

      country_filter_options.insert(oit, option);
    }
  }

  void
  SearchEngine::fill_lang_filter_options(
    const SearchContext& ctx,
    Search::LangCounterMap& lang_counter,
    El::Python::Sequence& lang_filter_options) const
    throw(Exception, El::Exception)
  {
    if(*ctx.lang == El::Lang::null)
    {
      return;
    }
    
    unsigned long total_messages = 0;
    
    for(Search::LangCounterMap::const_iterator it(lang_counter.begin()),
          ie(lang_counter.end()); it != ie; ++it)
    {
      total_messages += it->second;
    }

    lang_counter[El::Lang::null] = total_messages;
    
    if(lang_counter.find(*ctx.filter->lang) == lang_counter.end())
    {
      lang_counter[*ctx.filter->lang] = 0;
    }

    if(lang_counter.find(*ctx.lang) == lang_counter.end())
    {
      lang_counter[*ctx.lang] = 0;
    }

    for(Search::LangCounterMap::const_iterator it(lang_counter.begin()),
          ie(lang_counter.end()); it != ie; ++it)
    {
      const El::Lang& lang = it->first;

      std::wstring value;
      
      if(lang == El::Lang::null)
      {
        El::String::Manip::utf8_to_wchar(ctx.all_option.c_str(), value);
      }
      else
      { 
        std::ostringstream ostr;
        El::Loc::Localizer::instance().language(lang, *ctx.lang, ostr);
        El::String::Manip::utf8_to_wchar(ostr.str().c_str(), value);
      }

//      std::wostringstream wostr;
//      wostr << value << L" (" << it->second << L")";
//      value = wostr.str();

      El::Python::Sequence::iterator oit(lang_filter_options.begin());
      El::Python::Sequence::iterator oie(lang_filter_options.end());

      for(; oit != oie &&
            static_cast<SearchResult::Option*>(oit->in())->value < value;
          ++oit);

      std::wstring id;
      El::String::Manip::utf8_to_wchar(lang.l3_code(true), id);

      SearchResult::Option_var option =
        new SearchResult::Option(
          id.c_str(),
          it->second,
          value.c_str(),
          *ctx.filter->lang == lang);

      lang_filter_options.insert(oit, option);
    }
  }

  void
  SearchEngine::fill_feed_filter_options(
    const SearchContext& ctx,
    Search::StringCounterMap& feed_counter,
    El::Python::Sequence& feed_filter_options) const
    throw(Exception, El::Exception)
  {
    unsigned long total_messages = 0;
    
    for(Search::StringCounterMap::const_iterator it(feed_counter.begin()),
          ie(feed_counter.end()); it != ie; ++it)
    {
      total_messages += it->second.count1;
    }

    Message::StringConstPtr empty_str = "";
    
    feed_counter[empty_str.add_ref()] =
      Search::Counter(total_messages, 0, 0, 0);

    Message::StringConstPtr filter_feed = ctx.filter->feed.c_str();
    
    if(feed_counter.find(filter_feed.c_str()) == feed_counter.end())
    {
      feed_counter[filter_feed.add_ref()] = Search::Counter();
    }

    for(Search::StringCounterMap::const_iterator it(feed_counter.begin()),
          ie(feed_counter.end()); it != ie; ++it)
    {
      const Message::SmartStringConstPtr& feed = it->first;
      std::string feed_str = feed.c_str() ? feed.c_str() : "";

      std::wstring value;
      
      if(feed.c_str() == empty_str.c_str())
      {
        El::String::Manip::utf8_to_wchar(ctx.all_option.c_str(), value);
      }
      else
      { 
        El::String::Manip::utf8_to_wchar(it->second.name.c_str(), value);
      }

      std::wstring id;
      El::String::Manip::utf8_to_wchar(feed_str.c_str(), id);      

      El::Python::Sequence::iterator oit(feed_filter_options.begin());
      El::Python::Sequence::iterator oie(feed_filter_options.end());

      for(; oit != oie &&
            static_cast<SearchResult::Option*>(oit->in())->id < id;
          ++oit);

//      std::wostringstream wostr;
//      wostr << value << L" (" << it->second << L")";
//      value = wostr.str();

      SearchResult::Option_var option =
        new SearchResult::Option(
          id.c_str(),
          it->second.count1,
          value.c_str(),
          filter_feed.c_str() == feed.c_str());

      feed_filter_options.insert(oit, option);
    }
/*
    std::cerr << "--------------------------------------------------\n";
    
    for(El::Python::Sequence::iterator oit = feed_filter_options.begin();
        oit != feed_filter_options.end(); oit++)
    {
      El::String::Manip::wchar_to_utf8(
        static_cast<SearchResult::Option*>(oit->in())->value.c_str(),
        std::cerr);
      
      std::cerr << std::endl;
    }
*/
  }

  //
  // NewsGate::SearchContext::Type class
  //
  void
  SearchResult::Type::ready() throw(El::Python::Exception, El::Exception)
  {
    MLS_UNKNOWN_ = PyLong_FromLongLong(SearchResult::MLS_UNKNOWN);
    MLS_LOADING_ = PyLong_FromLongLong(SearchResult::MLS_LOADING);
    MLS_LOADED_ = PyLong_FromLongLong(SearchResult::MLS_LOADED);

    El::Python::ObjectTypeImpl<SearchResult, SearchResult::Type>::ready();
  }
  
  //
  // NewsGate::SearchContext::Type class
  //
  void
  SearchContext::Type::ready() throw(El::Python::Exception, El::Exception)
  {
    FF_HTML_ = PyLong_FromLong(SearchContext::FF_HTML);
    FF_WRAP_LINKS_ = PyLong_FromLong(SearchContext::FF_WRAP_LINKS);
    FF_SEGMENTATION_ = PyLong_FromLong(SearchContext::FF_SEGMENTATION);
    
    FF_FANCY_SEGMENTATION_ =
      PyLong_FromLong(SearchContext::FF_FANCY_SEGMENTATION);
    
    GM_ID_ = PyLong_FromLongLong(Message::Bank::GM_ID);
    GM_LINK_ = PyLong_FromLongLong(Message::Bank::GM_LINK);
    GM_TITLE_ = PyLong_FromLongLong(Message::Bank::GM_TITLE);
    GM_DESC_ = PyLong_FromLongLong(Message::Bank::GM_DESC);
    GM_IMG_ = PyLong_FromLongLong(Message::Bank::GM_IMG);
    GM_KEYWORDS_ = PyLong_FromLongLong(Message::Bank::GM_KEYWORDS);
    GM_STAT_ = PyLong_FromLongLong(Message::Bank::GM_STAT);
    GM_PUB_DATE_ = PyLong_FromLongLong(Message::Bank::GM_PUB_DATE);
    GM_VISIT_DATE_ = PyLong_FromLongLong(Message::Bank::GM_VISIT_DATE);
    GM_FETCH_DATE_ = PyLong_FromLongLong(Message::Bank::GM_FETCH_DATE);
    GM_LANG_ = PyLong_FromLongLong(Message::Bank::GM_LANG);
    GM_SOURCE_ = PyLong_FromLongLong(Message::Bank::GM_SOURCE);
    GM_EVENT_ = PyLong_FromLongLong(Message::Bank::GM_EVENT);
    GM_CORE_WORDS_ = PyLong_FromLongLong(Message::Bank::GM_CORE_WORDS);
    GM_DEBUG_INFO_ = PyLong_FromLongLong(Message::Bank::GM_DEBUG_INFO);
    GM_EXTRA_MSG_INFO_ = PyLong_FromLongLong(Message::Bank::GM_EXTRA_MSG_INFO);
    GM_IMG_THUMB_ = PyLong_FromLongLong(Message::Bank::GM_IMG_THUMB);
    GM_CATEGORIES_ = PyLong_FromLongLong(Message::Bank::GM_CATEGORIES);
    GM_ALL_ = PyLong_FromLongLong(Message::Bank::GM_ALL);

    RF_LANG_STAT_ = PyLong_FromLong(Search::Strategy::RF_LANG_STAT);
    RF_COUNTRY_STAT_ = PyLong_FromLong(Search::Strategy::RF_COUNTRY_STAT);
    RF_FEED_STAT_ = PyLong_FromLong(Search::Strategy::RF_FEED_STAT);
    RF_CATEGORY_STAT_ = PyLong_FromLong(Search::Strategy::RF_CATEGORY_STAT);

    ST_NONE_ = PyLong_FromLong(Search::Strategy::ST_NONE);
    ST_DUPLICATES_ = PyLong_FromLong(Search::Strategy::ST_DUPLICATES);
    ST_SIMILAR_ = PyLong_FromLong(Search::Strategy::ST_SIMILAR);
    
    ST_COLLAPSE_EVENTS_ =
      PyLong_FromLong(Search::Strategy::ST_COLLAPSE_EVENTS);
    
    ST_COUNT_ = PyLong_FromLong(Search::Strategy::ST_COUNT);
    
    SM_NONE_ = PyLong_FromLong(Search::Strategy::SM_NONE);
    
    SM_BY_RELEVANCE_DESC_ =
      PyLong_FromLong(Search::Strategy::SM_BY_RELEVANCE_DESC);
    
    SM_BY_RELEVANCE_ASC_ =
      PyLong_FromLong(Search::Strategy::SM_BY_RELEVANCE_ASC);
    
    SM_BY_PUB_DATE_DESC_ =
      PyLong_FromLong(Search::Strategy::SM_BY_PUB_DATE_DESC);
    
    SM_BY_PUB_DATE_ASC_ =
      PyLong_FromLong(Search::Strategy::SM_BY_PUB_DATE_ASC);
    
    SM_BY_FETCH_DATE_DESC_ =
      PyLong_FromLong(Search::Strategy::SM_BY_FETCH_DATE_DESC);
    
    SM_BY_FETCH_DATE_ASC_ =
      PyLong_FromLong(Search::Strategy::SM_BY_FETCH_DATE_ASC);
    
    SM_BY_EVENT_CAPACITY_DESC_ =
      PyLong_FromLong(Search::Strategy::SM_BY_EVENT_CAPACITY_DESC);
    
    SM_BY_EVENT_CAPACITY_ASC_ =
      PyLong_FromLong(Search::Strategy::SM_BY_EVENT_CAPACITY_ASC);
    
    SM_BY_POPULARITY_DESC_ =
      PyLong_FromLong(Search::Strategy::SM_BY_POPULARITY_DESC);
    
    SM_BY_POPULARITY_ASC_ =
      PyLong_FromLong(Search::Strategy::SM_BY_POPULARITY_ASC);
    
    SM_COUNT_ = PyLong_FromLong(Search::Strategy::SM_COUNT);
    
    El::Python::ObjectTypeImpl<SearchContext, SearchContext::Type>::ready();
  }
  
  //
  // NewsGate::SearchPyModule class
  //
  SearchPyModule SearchPyModule::instance;
    
  SearchPyModule::SearchPyModule() throw(El::Exception)
      : El::Python::ModuleImpl<SearchPyModule>(
        "newsgate.search",
        "Module containing SearchEngine factory method.",
        true)
  {
  }

  void
  SearchPyModule::initialized() throw(El::Exception)
  {
    not_ready_ex = create_exception("NotReady");
  }
  
  PyObject*
  SearchPyModule::py_create_engine(PyObject* args) throw(El::Exception)
  {
    return new SearchEngine(args);
  }

  PyObject*
  SearchPyModule::py_cleanup_engine(PyObject* args)
    throw(El::Exception)
  {
    PyObject* se = 0;
    
    if(!PyArg_ParseTuple(args,
                         "O:newsgate.search.cleanup_engine",
                         &se))
    {
      El::Python::handle_error(
        "NewsGate::SearchPyModule::py_cleanup_engine");
    }

    if(!SearchEngine::Type::check_type(se))
    {
      El::Python::report_error(
        PyExc_TypeError,
        "1st argument of newsgate.search.SearchEngine",
        "NewsGate::SearchPyModule::py_cleanup_engine");
    }

    SearchEngine* search_engine = SearchEngine::Type::down_cast(se);    
    search_engine->cleanup();
    
    return El::Python::add_ref(Py_None);
  }

  //
  // Category class
  //

  void
  Category::dump(std::ostream& ostr, size_t ident) const
    throw(El::Exception)
  {
    for(size_t i = 0; i < ident; ++i)
    {
      ostr << " ";
    }

    std::string sname;
    El::String::Manip::wchar_to_utf8(name.c_str(), sname);
    ostr << sname << "/";
    
    El::String::Manip::wchar_to_utf8(localized_name.c_str(), sname);
    ostr << sname << std::endl;

    Category_var child;
    
    for(El::Python::Sequence::iterator it(categories->begin()),
          ie(categories->end()); it != ie; ++it)
    {
      child = Category::Type::down_cast(it->in(), true);
      assert(child.in() != 0);

      child->dump(ostr, ident + 2);
    }
  }

  void
  Category::set_parent(unsigned long long val) throw()
  {
    parent = val;

    Category_var child;
    
    for(El::Python::Sequence::iterator it(categories->begin()),
          ie(categories->end()); it != ie; ++it)
    {
      child = Category::Type::down_cast(it->in(), true);
      assert(child.in() != 0);

      child->set_parent(id);
    }
  }
  
  void
  Category::sort_by_localized_name() throw(El::Exception)
  {
    El::Python::Sequence_var new_categories(new El::Python::Sequence());
    Category_var child;
    
    for(El::Python::Sequence::iterator it(categories->begin()),
          ie(categories->end()); it != ie; ++it)
    {
      child = Category::Type::down_cast(it->in(), true);
      assert(child.in() != 0);

      child->sort_by_localized_name();

      const wchar_t* localized_name = child->localized_name.c_str();

      El::Python::Sequence::iterator new_it(new_categories->begin());
      El::Python::Sequence::iterator new_ite(new_categories->end());
      
      for(; new_it != new_ite; ++new_it)
      {
        Category* new_child = Category::Type::down_cast(new_it->in());
        assert(new_child != 0);

        if(wcscasecmp(new_child->localized_name.c_str(), localized_name) >= 0)
        {
          break;
        }
      }

      new_categories->insert(new_it, child);
    }

    categories = new_categories.retn();
  }

  Category*
  Category::find(const char* path, bool create) throw(El::Exception)
  {
    assert(*path == '/');
    
    const char* cname = path + 1;

    if(*cname == '\0')
    {
      return this;
    }

    const char* cname_end = strchr(cname, '/');
    assert(cname_end != 0);

    std::string cn(cname, cname_end - cname);
    std::wstring child_name;
    
    El::String::Manip::utf8_to_wchar(cn.c_str(), child_name);
    El::Python::Sequence::iterator it(categories->begin());
    El::Python::Sequence::iterator ie(categories->end());

    Category_var child;
    
    for(; it != ie; ++it)
    {
      child = Category::Type::down_cast(it->in(), true);
      assert(child.in() != 0);

      if(create ? child->name >= child_name : child->name == child_name)
      {
        break;
      }
    }

    if(it == categories->end() || child->name != child_name)
    {
      if(create)
      {
        child = new Category();
        child->name = child_name;
        
        categories->insert(it, child);
      }
      else
      {
        return 0;
      }
    }

    return child->find(cname_end, create);
  }
  
  PyObject*
  Category::py_find(PyObject* args) throw(El::Exception)
  {
    char* path = 0;
    
    if(!PyArg_ParseTuple(args,
                         "s:newsgate.search.Category.find",
                         &path))
    {
      El::Python::handle_error("NewsGate::SearchEngine::py_find");
    }

    std::string cat_path = path[0] == '/' ? path : (std::string("/") + path);
    
    if(cat_path[cat_path.length() - 1] != '/')
    {
      cat_path += "/";
    }

    Category* cat = find(cat_path.c_str(), false);
    return El::Python::add_ref(cat ? cat : Py_None);
  }
  
}
