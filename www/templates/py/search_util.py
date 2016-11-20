import os
import sys
import math
import base64
import smtplib
import email.Message
import email.Charset
import re

import el
import newsgate
import search_intr

class ImageDim:

  def __init__(this, w, h):
    this.width = w
    this.height = h

class ImageDesc:

  def __init__(this, u, w, h, l):
    this.url = u
    this.width_attr = w
    this.height_attr = h
    this.make_link = l

class ThumbInfo:

  def __init__(this, thumb, w, h):
    this.thumb = thumb
    this.width = w
    this.height = h

class Source:
  title = ""
  html_link = ""

  def __init__(this, tl, hl):
    this.title = tl
    this.html_link = hl

def cmp_core_word(w1, w2):
  if w1.weight > w2.weight: return -1
  if w1.weight < w2.weight: return 1
  return 0

def cmp_cat_relevance(cat1, cat2):
  if cat1.matched_message_count < cat2.matched_message_count: return 1
  if cat1.matched_message_count > cat2.matched_message_count: return -1

  if cat1.localized_name < cat2.localized_name: return -1
  if cat1.localized_name > cat2.localized_name: return 1
  return 0

def cmp_sources(src1, src2):
  if src1.title < src2.title: return -1
  if src1.title > src2.title: return 1
  return 0

def cmp_langs(l1, l2):
  l1 = l1[0]
  l2 = l2[0]
  if l1 < l2: return -1
  if l1 > l2: return 1
  return 0

def xml_smart_encode(text):

  if text.find("]]>") < 0:
# TODO: replace with a commented variant when frontend transition complete
# This DONE
    return text != "" and "<![CDATA[" + text + "]]>" or ""
#    return "<![CDATA[" + text + "]]>"
  else:
    return el.string.manip.xml_encode(text, 
                                      el.string.manip.XE_TEXT_ENCODING | \
                                      el.string.manip.XE_PRESERVE_UTF8)

def truncate_text(text, max_len = 70):

  unicode_text = ""

  try:
    unicode_text = text.decode("utf8", 'strict')
  except:
    return ""

  if len(unicode_text) > max_len + 3:
    trunc_to_space = unicode_text[max_len] != " "
    unicode_text = unicode_text[0:max_len]

    if trunc_to_space:
      spos = unicode_text.rfind(" ")
      
      if spos > 0 and unicode_text.rfind("://") < spos:
        unicode_text = unicode_text[0:spos] + " "
    
    return unicode_text.encode("utf8") + "..."
  else:
    return text

def normalize_category(path):
  if path[0] != "/": path = "/" + path
  if path[-1:] != "/": path += "/"
  return path
  
class Message:

  def __init__(this):
    this.message_id = ""
    this.event_id = ""
    this.title = ""
    this.lang = el.Lang()
    this.expired = False
    this.loaded = False
    this.event_canonized = False

  def clone(this):
    msg = Message()

    msg.message_id = this.message_id
    msg.event_id = this.event_id
    msg.title = this.title
    msg.lang = this.lang
    msg.expired = this.expired
    msg.loaded = this.loaded
    msg.event_canonized = this.event_canonized
    
    return msg

  def pair(this):
    return this.message_id and (this.message_id + " " + this.event_id) or ''

class Modifier:
  
  def __init__(this):
    this.all = False
    this.similar = Message()
    this.source = ""
    this.category = ""

  def clone(this):
    modifier = Modifier()
    modifier.all = this.all
    modifier.similar = this.similar.clone()
    modifier.source = this.source
    modifier.category = this.category
    return modifier

  def empty(this):
    return this.all == False and this.similar.message_id == "" and \
           this.source == "" and this.category == ""
    
  def query(this):

    if this.all: return "EVERY"
    elif this.similar.message_id != "":
      return "EVENT " + (this.similar.event_id or "0")
    elif this.source != "":
      return "URL " + this.source
    elif this.category != "":
      return "CATEGORY \"" + normalize_category(this.category) + "\""
    else: return ""

  def param_all(this):
    return this.all and "v=A" or ""

  def param_similar(this):
    return this.similar.message_id and ("v=E" + \
             el.string.manip.mime_url_encode(this.similar.pair())) or ""
  
  def param_source(this):
    return this.source and ("v=S" + \
             el.string.manip.mime_url_encode(this.source)) or ''

  def param_category(this):
    return this.category and ("v=C" + \
      el.string.manip.mime_url_encode(normalize_category(this.category))) or ''
  
  def params(this):

    if this.all: return this.param_all()
    elif this.similar.message_id: return this.param_similar()
    elif this.source: return this.param_source()
    elif this.category: return this.param_category()
    else: return ""

class Filter:

  def __init__(this):
    this.lang = el.Lang.null
    this.country = el.Country.null
    this.feed = ""
    this.category = ""
    this.event = Message()
#    this.spaces = ""

  def clone(this):
    fl = Filter()
    fl.lang = this.lang
    fl.country = this.country
    fl.feed = this.feed
    fl.category = this.category
    fl.event = this.event.clone()
#    fl.spaces = this.spaces
    return fl

  def empty(this):
    return this.lang == el.Lang.null and this.country == el.Country.null and \
           not this.feed and not this.category and \
           not this.event.message_id
#  and not this.spaces

  def sticky(this):
    fl = Filter()
    fl.lang = this.lang
    fl.country = this.country
    return fl
    
  def params(this, crawler, main_languages):

    res = ""

    if this.event.message_id:
      if res: res += "&"
      res += "h=" + \
             el.string.manip.mime_url_encode(this.event.message_id + \
                                             " " + this.event.event_id)
      
    if this.category:
      if res: res += "&"
      
      res += "g=" + \
        el.string.manip.mime_url_encode(normalize_category(this.category))

#    if this.spaces:
#      if res: res += "&"
#      res += "se=" + el.string.manip.mime_url_encode(this.spaces)

    if this.lang != el.Lang.nonexistent and \
       (this.lang != el.Lang.null or crawler == None or main_languages):

      code = this.lang.l3_code(bool(main_languages))
      if crawler and code not in main_languages: code = "zzz"
        
      if res: res += "&"
      res += "n=" + code

    if this.country != el.Country.nonexistent and \
       (this.country != el.Country.null or crawler == None):
      if res: res += "&"
      res += "y=" + this.country.l3_code()

    if this.feed:
      if res: res += "&"
      res += "f=" + el.string.manip.mime_url_encode(this.feed)

    return res

class Crawler:

  def __init__(this, name = None, context = None):

    if name and context:
      conf = context.config.get("search.crawler." + name)
      other_conf = context.config.get("search.crawler.other")

      this.message_max_age = \
        long(this.get_value("message_max_age", conf, other_conf, -1))
      
      this.results_per_page = \
        long(this.get_value("results_per_page", conf, other_conf, -1))
      
    else:
      this.message_max_age = -1
      this.results_per_page = -1

  def clone(this):
    
    cr = Crawler()

    cr.message_max_age = this.message_max_age
    cr.results_per_page = this.results_per_page
    
    return cr
    
  def get_value(this, key, conf, other_conf, def_val):
    
    if conf: val = conf.get(key)
    else: val = None
    
    if val == None and other_conf: val = other_conf.get(key, def_val)
    if val == None: val = def_val
    
    return val

class Static:

  def __init__(this, context, protocol):
    
    conf = context.config.get

    file = open(conf("version_file"), "r")
    this.server_version = file.readline()
    file.close()
    
    this.address_info = el.geography.AddressInfo()
    this.stat_url = conf("search.stat.url")

    this.crawlers = {}
    this.crawlers["googlebot"] = Crawler("googlebot", context)
    this.crawlers["yandex"] = Crawler("yandex", context)
    this.crawlers["other"] = Crawler("other", context)

    try:
      this.raw_stat_log_enabled = conf("search.stat.processor_ref") != "" and \
                             int(conf("search.stat.raw_stat_keep_days")) > 0
    except:
      this.raw_stat_log_enabled = False

    conf_name_max_results = \
      "search.max_results." + \
      (protocol == "h" and "html" or \
       (protocol == "j" or protocol == "o") and "jsearch" or \
       protocol == "r" and "rss" or \
       protocol == "x" and "xsearch" or \
       "")

    try:
      this.max_results = int(conf(conf_name_max_results))
    except:
      this.max_results = 0
      
    this.protocol = protocol
    
    this.canonoical_endpoint = conf("endpoint")
    this.canonize_event = int(conf("search.canonize_event", 0))

    this.not_crawler_ips = {}

    not_crawler_ips = conf("search.not_crawler_ips")

    if not_crawler_ips:
    
      for ip in not_crawler_ips.split():
        if ip == "*":
          this.not_crawler_ips = {}
          this.not_crawler_ips[ip] = 1
          break
        else:
          this.not_crawler_ips[ip] = 1

    this.logger = el.psp.Logger(int(conf("log_level")), conf("log_aspects"))

    this.language_filters = {}
    this.main_languages = {}
    
    lf_list = conf("language_filters")
    if isinstance(lf_list, str): lf_list = [ lf_list ]
    
    for lf in lf_list:
      kv = lf.split(":")
      lang = kv[0]
      
      if lang == "none":
        this.language_filters = {}
        this.main_languages = {}
        break
        
      lang_filter = kv[1]
      this.language_filters[lang] = el.Lang(lang_filter)
      this.main_languages[lang_filter] = lang != "*"

    this.country_filters = {}
    
    cf_list = conf("country_filters")
    if isinstance(cf_list, str): cf_list = [ cf_list ]
    
    for cf in cf_list:

      kv = cf.split(":")
      lang = kv[0]
      
      if lang == "none":
        this.country_filters = {}
        break

      this.country_filters[lang] = []

      for cr in kv[1].split(','):
        this.country_filters[lang].append(cr);

#    if True:
    try:
      import search_interceptor
      interceptor_factory_exist = True
    except:
      interceptor_factory_exist = False

    if interceptor_factory_exist:
      this.interceptor_factory = search_interceptor.create_interceptor
    else:
      this.interceptor_factory = None

  def crawler(this, name):

    if name in this.crawlers:
      cr = this.crawlers[name]
    else:
      cr = this.crawlers["other"]
      
    cr = cr.clone()
    cr.name = name
    
    return cr
  
class RequestContext:

  def __init__(this, context):
    
    this.context = context

  def set_cookie(this, name, value):

    request = this.context.request
    conf = this.context.config.get

    expiration = \
      el.Moment(el.TimeValue(request.time().sec() + 2 * 365 * 86400))
    
    domain = ""
  
    hostname_domain_offset = int(conf("cookie_domain_offset"))
    endpoint = request.endpoint()

    if endpoint.find(':') >= 0:
      endpoint = endpoint.split(':')[0]

    canonical_endpoint = conf("endpoint")

    if canonical_endpoint.find(':') >= 0:
      canonical_endpoint = canonical_endpoint.split(':')[0]

    if hostname_domain_offset and endpoint == canonical_endpoint:

      names = endpoint.split('.', hostname_domain_offset)
      names_pos = len(names)

      if names_pos > 1:
        names_pos -= 1
        domain = "." + names[names_pos]
      
    this.send_cookie(el.net.http.CookieSetter(name,
                                              value,
                                              expiration,
                                              "/",
                                              domain))

  def send_cookie(this, cookie_setter):
    this.context.request.output.send_cookie(cookie_setter)

  def set_uid(this):

    try:
      uid = el.Guid(this.context.request.input.cookies().most_specific("u"))
    except:
      uid = el.Guid()
      uid.generate()

    this.set_cookie("u", uid.string(el.Guid.GF_DENSE))    

class SearchRequestContext(RequestContext):

  search_request_context_base = RequestContext
  
  def __init__(this,
               context,
               search_engine,
               protocol,
               read_prefs):


    this.search_request_context_base.__init__(this, context)

    try:
      this.static = context.cache["render_static_info"]
    except:
      this.static = Static(context, protocol)
      context.cache["render_static_info"] = this.static

    this.legacy_metas = False
    this.trace_uri = 0

    this.search_engine = search_engine

    request = context.request
    header = request.input.headers().find

    this.prn = request.output.stream.prn
    this.loc = context.localization.get
    this.lm = el.psp.LocalizationMarker
    
    st = this.static
    
    this.endpoint = request.endpoint()
    if this.endpoint == "": this.endpoint = st.canonoical_endpoint

    this.site = 'http://' + this.endpoint
    this.service = this.site + request.uri()
    
    this.suppression = newsgate.search.SearchContext.ST_COLLAPSE_EVENTS
    this.suppression_param = None

    this.sorting = newsgate.search.SearchContext.SM_BY_RELEVANCE_DESC
#    this.sorting_param = None

    this.block = ""
    this.start_from = 0
    this.results_per_page = 10
#    this.desc_length = -1
    this.total_matched_messages = None

    this.query = ""
    this.optimize_query = True

    this.modifier = Modifier()
    this.filter = Filter()
    this.locale = el.Locale()
    this.language = request.input.lang
    this.country = st.address_info.country(request.remote_ip())
    this.language_l3_code = this.language.l3_code()
    this.country_l3_code = this.country.l3_code()

    this.etag = 0
    this.text_segmentation = False
    this.max_image_count = 0xFFFFFFFF
    this.localizer = el.loc.localizer()
    this.cat_translation = {}
    this.prefs = None
    this.lang_prefs = None
    this.persist_prefs = None
    this.persist_lang_prefs = None
    
    this.extra_msg_info = False
    this.search_result = None
    this.prefs_updated = False
    this.prefs_updated_params = {}

    this.country_filters = None

    this.test_mode = this.raw_param("TEST") == "1"
    
    this.search_info = el.psp.search_info(header("referer"))

#    if this.raw_param("st") == "" and this.raw_param("s") == "":
#      this.search_info = el.psp.search_info("http://yandex.ru/yandsearch?text=Superjet-100&lr=2")
#      this.search_info = el.psp.search_info("http://www.google.ru/url?sa=t&rct=j&q=newsfiber%20%D0%B0%D0%B0%D0%B0&source=web&cd=1&ved=0CFMQFjAA&url=http%3A%2F%2Fwww.newsfiber.com%2Fpsp%2Feng%2Fsearch%3Fq%3D%26r%3D10%26p%3Ds%26c%3D2%26i%3D1%26e%3D0%26b%3D3%26a%3D0%26n%3D%26y%3D%26se%3D%26v%3DEgGjp3hkQNOI%253D%2BMgzz%252BLJsYPc%253D&ei=kQDaT8HPBefU4QTd8_GIAw&usg=AFQjCNGCPr6fyaVuCLycp2EyMOcBgRlmqg&cad=rjt")      

    try: this.etag = long(header("If-None-Match"))
    except: pass

    this.agent = header("user-agent")
    crawler_name = el.psp.crawler(this.agent)

    valid_xsearch_client = \
      request.remote_ip() in this.static.not_crawler_ips or \
      '*' in this.static.not_crawler_ips
    
    if valid_xsearch_client: crawler_name = ""

    if not crawler_name:

      if read_prefs:
        cookies = request.input.cookies()

        this.prefs = el.NameValueMap(cookies.most_specific("pf"), ':', '-')
        
        this.lang_prefs = \
          el.NameValueMap(cookies.most_specific("lp"), ':', '-')

        if this.raw_param("slg") == "1":
          this.lang_prefs["g"] = request.input.lang.l3_code()

      if this.prefs != None:
        this.persist_prefs = this.prefs
        this.persist_lang_prefs = this.lang_prefs

      ua = this.raw_param("ua", None)

      if ua == None:
        if this.prefs != None:          
          ua = el.string.manip.mime_url_decode(this.prefs.value("ua"))
          if ua:
            this.agent = ua
            crawler_name = el.psp.crawler(this.agent)
            
      else:
        
        if ua:
          this.agent = ua
          crawler_name = el.psp.crawler(this.agent)

        if this.prefs != None:
          
          this.prefs_updated = True
          this.prefs_updated_params["ua"] = 1
          
          if ua:
            
            this.prefs["ua"] = el.string.manip.mime_url_encode(ua)
            
            if crawler_name:
              this.prefs = None
              this.lang_prefs = None
              
          else:
            del this.prefs["ua"]  

    this.browser = el.psp.browser(this.agent)
    this.tab = el.psp.tab(this.agent)
    this.phone = el.psp.phone(this.agent)
    
    if crawler_name:
      this.crawler = st.crawler(crawler_name)
    else:
      this.crawler = None    

    this.search_engine_refered = \
      this.search_info != None or crawler_name != ""

    if True:
#    try:

      suppression = \
        this.string_param(\
          "b",
          str(newsgate.search.SearchContext.ST_COLLAPSE_EVENTS),
          display_param = True).split("-")

      this.suppression = int(suppression[0])

      if this.suppression < 0 or \
         this.suppression >= newsgate.search.SearchContext.ST_COUNT:
        this.exit(400) # bad request

      l = len(suppression)
      if l == 2: this.suppression_param = int(suppression[1])
      elif l > 2: this.exit(404) # bad request

      sorting = \
        this.string_param(\
          "a",
          str(newsgate.search.SearchContext.SM_NONE),
          display_param = True).split("-")

      try: this.sorting = int(sorting[0]) + 1
      except: pass

      if this.sorting <= newsgate.search.SearchContext.SM_NONE or \
         this.sorting >= newsgate.search.SearchContext.SM_COUNT:
        this.exit(400) #bad request

#      l = len(sorting)
#      if l == 2: this.sorting_param = int(sorting[1])
#      elif l > 2: this.exit(400) # bad request

      this.results_per_page = \
        int(this.string_param("r", 10, display_param = True))
      
      if this.results_per_page < 0: this.exit(400) # bad request

      try:
        this.total_matched_messages = int(this.raw_param("tmm"))
      except: pass

      if this.total_matched_messages <= 0: this.total_matched_messages = None

      if this.results_per_page > st.max_results:
        this.results_per_page = st.max_results

      try:
        this.start_from = int(this.raw_param("s", 1)) - 1
      except: pass
      
      if this.start_from < 0: this.start_from = 0

      if this.total_matched_messages and this.results_per_page:
        
        this.start_from = \
          min(this.start_from,
              (this.total_matched_messages - 1) / \
                this.results_per_page * this.results_per_page)    

      results = this.start_from + this.results_per_page

      if results > st.max_results: 
        this.start_from -= results - st.max_results
        if this.start_from < 0: this.start_from = 0

#      this.desc_length = int(this.string_param("dl", -1, display_param = True))

      this.optimize_query = int(this.raw_param("j", 1)) and True or False
      this.text_segmentation = this.raw_param("m") == "1"

      this.query = el.string.manip.suppress(this.raw_param("q"), "\r").strip()

      try:
        this.max_image_count = \
          int(this.string_param("i", str(this.max_image_count), None, True))
      except:
        this.max_image_count = 0

      search_modifier_param = this.raw_param("v")

      try: val_type = search_modifier_param[0].strip()
      except: val_type = ""

      try: val = search_modifier_param[1:]
      except: val = ""

      md = this.modifier

      if val_type == 'E':
        md.similar = this.resolve_event(context, val)

      elif val_type == 'S': md.source = val
      elif val_type == 'C' or val_type == 'c':

        if val != "" and val != "/":
          l = len(val)
          if val[l-1:] == "/": val = val[0:l-1]

        md.category = val

      elif val_type == 'A':
        md.all = True

      if this.query == "" and md.similar.message_id == "" and \
         md.source == "" and md.category == "":
        md.all = True        

      fl = this.filter

      try:

        lf = this.string_param("n")

        if lf:
          fl.lang = el.Lang(lf)

          if fl.lang == el.Lang.null:

            uri = this.context.request.uri()

            if uri[0:5] == '/psp/' and uri[8:] == '/search':
              try:
                fl.lang = el.Lang(uri[5:8])
              except: pass
          
        elif this.static.main_languages:
          fl.lang = this.default_lang_filter()
        
      except: pass

      try: fl.country = el.Country(this.string_param("y"))
      except: pass

#      if crawler_name == "":
        # Decrease link number variations being indexed
#        try: fl.country = el.Country(this.string_param("y"))
#        except: pass

      fl.feed = this.raw_param("f")
      fl.category = this.raw_param("g")
      fl.event = this.resolve_event(context, this.raw_param("h"))

    this.orig_filter = this.filter.clone()
    
#    except:
#      this.exit(400) # bad request


    this.lang_filter_switched = False
    
    if this.filter.lang != el.Lang.null or this.crawler:
      lang = \
        this.modifier.similar.event_id and \
        not this.modifier.similar.expired and this.modifier.similar.lang or \
        this.filter.event.event_id and \
        not this.filter.event.expired and this.filter.event.lang or \
        this.filter.lang

      this.lang_filter_switched = this.filter.lang != lang
      this.filter.lang = lang

    if this.static.country_filters and \
       this.filter.lang.l3_code() in this.static.country_filters:
      this.country_filters = \
        this.static.country_filters[this.filter.lang.l3_code()]

    this.locale = \
      context.request.input.locale(this.filter.lang, this.filter.country)    

    if this.query and not this.block:
      try: this.query = search_engine.segment_query(this.query)
      except SyntaxError: this.exit(400) #bad request
      except newsgate.search.NotReady:
        request.output.send_header("Retry-After", "60")
        this.exit(503) #service unavailable

    this.interceptor = st.interceptor_factory != None and \
      st.interceptor_factory(context, st.logger, None, this) or None

#    if st.raw_stat_log_enabled == False and \
#       st.logger.will_trace(el.logging.HIGH):

#      referer = header("referer")

#      st.logger.trace(el.logging.HIGH, request.uri(),
#                      "ip=", request.remote_ip(),
#                      " || lang=", this.language_l3_code,
#                      " || cntr=", this.country_l3_code,
#                      " || loc=", this.locale.lang.l3_code(), 
#                      "-", this.locale.country.l3_code(),
#                      " || br=", this.browser, ":", crawler_name,
#                      " || suppr=", this.suppression,
#                      " || sort=", this.sorting,
#                      " || start=", this.start_from,
#                      " || res=", this.results_per_page,
#                      " || mod=", this.raw_param("v"),
#                      " || query=", this.query, 
#                      " || ua=", this.agent,
#                      " || ref=", referer,
#                      this.custom_log(2))

  def exit(this, result = None):    
    this.interceptor = None # to break circular references
    if result != None: el.exit(result)

  def category(this):
    return this.modifier.category or this.filter.category

  def event(this):
    return this.modifier.similar.message_id and this.modifier.similar or \
            this.filter.event.message_id and this.filter.event or None

  def event_message_id(this):
    return this.modifier.similar.message_id or this.filter.event.message_id

  def event_id(this):
    return this.modifier.similar.event_id or this.filter.event.event_id

  def source(this):
    return this.modifier.source or this.filter.feed

  def set_language(this, new_lang):
    
    if this.language_l3_code == new_lang or new_lang == "" or this.block:
      return

    lang = el.Lang.null

    if new_lang == "auto":

      if this.filter.lang != el.Lang.null:
        lang = this.filter.lang
      elif this.modifier.similar.message_id:
        lang = this.modifier.similar.lang
      elif this.filter.event.event_id:
        lang = this.filter.event.lang
      else:
        search_context = newsgate.search.SearchContext()
        search_context.query = this.resulted_query()
        search_context.results_count = 1
        search_context.title_format = 0
        search_context.gm_flags = newsgate.search.SearchContext.GM_LANG

        try:
          search_result = this.search_engine.search(search_context)
          lang = search_result.messages[0].lang
        except: pass

    else:
      lang = el.Lang(new_lang)

    if lang == el.Lang.null: return

    try:
      this.context.language(lang)
    except:
      return

    this.loc = this.context.localization.get
    this.language = this.context.request.input.lang
    this.language_l3_code = this.language.l3_code()
 
    this.locale = \
      this.context.request.input.locale(this.filter.lang,
                                        this.filter.country)

    if this.interceptor != None:
      this.interceptor.loc = None

  def default_lang_filter(this, accept_languages_only = False):
    
    lang = el.Lang.null

    if not accept_languages_only:
      lp = this.string_param("lang", None)

      if lp:
        try:
          lang = el.Lang(lp)
        except: pass

      if lang == el.Lang.null:

        uri = this.context.request.uri()

        if uri[0:5] == '/psp/' and uri[8:] == '/search':
          try:
            lang = el.Lang(uri[5:8])
          except: pass

      if lang == el.Lang.null:
        try:
          lang = el.Lang(this.lang_prefs["g"])
        except: pass
    
    if lang == el.Lang.null:
      ac = this.context.request.input.accept_languages()
      if len(ac): lang = ac[0].language

    if lang != el.Lang.null:

      lf = this.static.language_filters
      code = lang.l3_code()

      if code in lf: lang = lf[code]
      elif '*' in lf: lang = lf['*']
      else: lang = el.Lang.null
    
    return lang
  
  def source_cat_link_filter(this):

    if not this.crawler:
      fl = this.filter.sticky()
    elif this.static.main_languages:
      fl = Filter()
      fl.lang = this.filter.lang
      if this.country_filters: fl.country = this.filter.country
    else:        
      fl = False

    return fl
  
  def search_link(this,
                  export = False,
                  modifier = True,
                  query = True,
                  filter = True,
                  extra_params = "",
                  path = True,
                  debug = False):

    url = path == True and this.service or \
          path.find("://") > 0 and path or (this.site + path)
    
    params = this.search_query(export,
                               modifier,
                               query,
                               filter,
                               extra_params,
                               debug)
    
    if params: url += "?" + params
    return url
    
  def search_query(this,
                   export = False,
                   modifier = True,
                   query = True,
                   filter = True,
                   extra_params = "",
                   debug = False):
    
    if modifier == True: modifier = this.modifier    
    if query == True: query = this.query
    if filter == True: filter = this.filter

#    test = ""

    if filter and modifier and \
       (modifier.all or query == '' and modifier.empty()):

#      test = str(modifier.all)
      
      if filter.event.message_id:
        
        modifier = Modifier()
        modifier.similar = filter.event
        filter = filter.clone()
        filter.event = Message()
      
      elif filter.feed:
        
        modifier = Modifier()
        modifier.source = filter.feed
        filter = filter.clone()
        filter.feed = ""

      elif filter.category:
        
        modifier = Modifier()
        modifier.category = filter.category
        filter = filter.clone()
        filter.category = ""

    if query == '' and modifier and modifier.empty():
      # Save query string for user convenience
      query = this.query
      modifier = Modifier()
      modifier.all = True
 
    mod = modifier and modifier.params() or ''

    if export:
      if mod: query = ''
    elif mod:
      if this.crawler: query = ''
      elif query == '':
        # Save query string for user convenience
        query = this.query
     
    if query == '' and modifier and modifier.all: mod = ''
      
    params = extra_params
    
    if mod:
      if params: params += "&"
      params += mod
        
    if query:
      if params: params += "&"
      params += "q=" + el.string.manip.mime_url_encode(query)

    fl = ""

    if filter:

      if modifier and modifier.similar.event_id or filter.event.event_id:
        filter = filter.clone()
        filter.lang = el.Lang.nonexistent

      fl = filter.params(this.crawler, this.static.main_languages)

    if fl:
      if params: params += "&"
      params += fl

#    if test:
#      if params: params += "&"
#      params += "ttt=" + test      

    return params
  
  def resulted_query(this):
    mod_query = this.modifier.query()
    return mod_query != "" and mod_query or this.query

  def resulted_suppression(this):

    return (this.modifier.similar.message_id or this.filter.event.event_id) \
           and this.suppression == \
             newsgate.search.SearchContext.ST_COLLAPSE_EVENTS and \
             newsgate.search.SearchContext.ST_SIMILAR or this.suppression

  def resulted_sorting(this):

    if this.modifier.similar.message_id != "":

      if this.sorting == \
           newsgate.search.SearchContext.SM_BY_RELEVANCE_DESC or \
         this.sorting == \
           newsgate.search.SearchContext.SM_BY_EVENT_CAPACITY_DESC or \
         this.sorting == \
           newsgate.search.SearchContext.SM_BY_EVENT_CAPACITY_ASC:
        return newsgate.search.SearchContext.SM_BY_PUB_DATE_DESC

      elif this.sorting == newsgate.search.SearchContext.SM_BY_RELEVANCE_ASC:
        return newsgate.search.SearchContext.SM_BY_PUB_DATE_ASC

    elif this.modifier.source != "":

      if this.sorting == newsgate.search.SearchContext.SM_BY_RELEVANCE_DESC:
        return newsgate.search.SearchContext.SM_BY_PUB_DATE_DESC

      elif this.sorting == newsgate.search.SearchContext.SM_BY_RELEVANCE_ASC:
        return newsgate.search.SearchContext.SM_BY_PUB_DATE_ASC 

    return this.sorting

  def query_title(this):
    
    title = ""
    
    if this.modifier.similar.message_id:
      
      title = this.modifier.similar.title
      
    elif this.modifier.source:
      
      if this.search_result != None and this.search_result.messages:

        title = \
          el.string.manip.xml_encode(truncate_text(
            this.search_result.messages[0].source_title))

      if not title:
        title = \
          el.string.manip.xml_encode(truncate_text(this.modifier.source))
      
    elif this.modifier.category:

      if this.search_result != None:

        title = this.search_result.category_locale.title

        if not title:
          title = \
            this.translate_category_name(this.modifier.category).\
              replace("/", " / ")
        
        title = el.string.manip.xml_encode(title)
        
    elif this.modifier.all:

      title = this.loc("QUERY_TITLE_ALL")
      
    else:
      
      title = el.string.manip.xml_encode(truncate_text(this.query))

    if this.filter.event.event_id:
      title += " " + this.loc("QUERY_TITLE_IN_SIMILAR") + " '" + \
               this.filter.event.title + "'"
    
    if this.filter.feed:

      src_title = ""

      if this.search_result != None and this.search_result.messages:

        src_title = \
          "'" + truncate_text(this.search_result.messages[0].source_title) + "'"

      if not src_title: src_title = truncate_text(this.filter.feed)

      title += " " + this.loc("QUERY_TITLE_IN_SOURCE") + " " + \
               el.string.manip.xml_encode(src_title)
    
    if this.filter.category and this.search_result != None:
      title += " " + this.loc("QUERY_TITLE_IN_CATEGORY") + " " + \
        this.translate_category_name(this.filter.category).replace("/", " / ")

#    if this.filter.lang != el.Lang.null and \
#       not this.modifier.similar.event_id and \
#       not this.filter.event.event_id:
#      title += \
#        ", " + this.loc("QUERY_TITLE_IN_LANG") + " " + \
#        el.string.manip.xml_encode(\
#          this.localizer.language(this.filter.lang, this.language))
    
#    if this.filter.country != el.Country.null:
#      title += \
#        ", " + this.loc("QUERY_TITLE_IN_COUNTRY") + " " + \
#        el.string.manip.xml_encode(\
#          this.localizer.country(this.filter.country, this.language))
    
    return title

  def query_description(this):

    description = ""
    
    if this.search_result != None:

      if this.legacy_metas: 

        if this.modifier.category:
          description = this.search_result.category_locale.description

        return el.string.manip.xml_encode(description) or this.query_title()
      
      if this.modifier.source:

        if len(this.search_result.messages) > 0:
          msg = this.search_result.messages[0]
          description = msg.source_title or msg.source_html_link
        
      elif this.modifier.category:
        description = this.translate_category_name(this.modifier.category).\
            replace("/", " / ")

        cat = this.search_result.category_stat.find(this.modifier.category)

        if cat != None:

          first = True

          for c in cat.categories:
            if first:
              first = False
              description += " : "
            else:
              description += ", "
              
            description += c.localized_name.encode("utf-8")
        
      elif len(this.search_result.messages) > 0 and \
           (this.modifier.similar.message_id or \
            len(this.search_result.messages) == 1 or \
            this.modifier.empty() and this.query):
      
        description = \
          truncate_text(el.string.manip.xml_decode(\
            this.search_result.messages[0].description), 200)

      else:

        cat = this.search_result.category_stat.find("/")

        if cat != None:
          for c in cat.categories:
            if description: description += ", "
            else: description += this.loc("QUERY_DESCRIPTION_ALL") + " "
            description += c.localized_name.encode("utf-8")

    return el.string.manip.xml_encode(description)

  def raw_param(this, name, def_val = ""):
    value = this.context.request.input.parameters().find(name, def_val)

    if isinstance(value, str) and \
       el.string.manip.utf8_valid(value, el.string.manip.UAC_XML_1_0) ==\
       False:
      value = ""

    return value
  
  def string_param(this,
                   name,
                   def_val = "",
                   prefs = True,
                   display_param = False):
    
    if this.crawler: prefs = None
    elif prefs == True: prefs = this.prefs

    if this.search_engine_refered and display_param:
      value = None
    else:
      value = this.raw_param(name, None)
    
    setter = value != None and value[0:2] == "s-"

    if setter:
      val = value[2:]
      
      if prefs != None:
        if this.search_engine_refered == False: prefs[name] = val
        this.prefs_updated = True
        this.prefs_updated_params[name] = 1
        
    elif value != None:
      val = value
    elif prefs != None and prefs.present(name):
      val = prefs[name]
    else:
      val = def_val

    return val

  def bool_param(this,
                 name,
                 true_val,
                 false_val,
                 def_val = True,
                 prefs = True,
                 display_param = False):

    if this.crawler: prefs = None
    elif prefs == True: prefs = this.prefs

    if this.search_engine_refered and display_param:
      value = None
    else:
      value = this.raw_param(name, None)    
    
    setter = value != None and value[0:2] == "s-"

    if setter:
      val = value[2:]
      if val == true_val: val = True
      else: val = False

      if prefs != None:
        
        if this.search_engine_refered == False:
          if val: prefs[name] = true_val
          else: prefs[name] = false_val
          
        this.prefs_updated = True
        this.prefs_updated_params[name] = 1
      
    elif value != None:    
      if value == true_val: val = True
      else: val = False
    
    elif prefs != None and prefs.present(name):
      val = prefs[name]
      if val == true_val: val = True
      else: val = False

    else:
      val = def_val

    return val

  def resolve_event(this, context, msg_event_param):
    
    result = Message()
    
    msg_event_param = msg_event_param.strip()
    if not msg_event_param or this.block: return result
    
    search_context = newsgate.search.SearchContext()

    msg_event_pair = msg_event_param.split(" ")

    if len(msg_event_pair) > 1:
      result.message_id = msg_event_pair[0]
      result.event_id = msg_event_pair[1]
    else:
      result.message_id = msg_event_param

    search_context.query = "MSG " + result.message_id
    search_context.results_count = 1
    search_context.title_format = 0

    search_context.gm_flags = newsgate.search.SearchContext.GM_EVENT |\
                              newsgate.search.SearchContext.GM_TITLE |\
                              newsgate.search.SearchContext.GM_ID |\
                              newsgate.search.SearchContext.GM_LANG |\
                              newsgate.search.SearchContext.GM_PUB_DATE    

    search_result = None

#    if True:
    try:

      req_time = this.context.request.time().sec()

      search_result = this.search_engine.search(search_context)

      msg = len(search_result.messages) and search_result.messages[0] or None
      event_title = ""
      
      if msg == None or msg.event_id == "0000000000000000":
        event_id = result.event_id
        
      elif this.crawler and this.crawler.message_max_age >= 0 and \
           msg.published + this.crawler.message_max_age < req_time:
        event_id = msg.event_capacity > 1 and msg.event_id or result.event_id

      else:
        event_id = ""

      event_canonized = this.static.canonize_event and \
         event_id == "" and msg != None and \
         msg.published + this.static.canonize_event < req_time
                      
      if event_canonized:
        event_id = msg.event_id
        event_title = msg.title

      if event_id:
        search_context.query = this.crawler_query("EVENT " + event_id)

        search_context.sorting_type = \
          newsgate.search.SearchContext.SM_BY_RELEVANCE_DESC

        search_result = this.search_engine.search(search_context)

      result.loaded = search_result.message_load_status == \
        newsgate.search.SearchResult.MLS_LOADED

#      if True:
      try:
        msg = search_result.messages[0]
        result.message_id = msg.encoded_id
        result.event_id = msg.encoded_event_id
        result.event_canonized = event_canonized
        
        result.title = \
          el.string.manip.xml_encode(\
            truncate_text(event_title and event_title or msg.title))
        
        result.lang = msg.lang
      
      except:
        result.expired = True

    except:
      result.loaded = True
      result.expired = True

    return result

  def search_context(this):

    request = this.context.request
    search_context = newsgate.search.SearchContext()

    search_context.protocol = this.static.protocol
    search_context.optimize_query = this.optimize_query
    search_context.record_stat = True
    search_context.request = request
    
    search_context.msg_id_similar = this.event_message_id()
    
    search_context.query = this.resulted_query()
    search_context.etag = this.etag
    
    search_context.suppression.type = this.resulted_suppression()

    if this.suppression_param != None:
      search_context.suppression.param = this.suppression_param

    search_context.sorting_type = this.resulted_sorting()

#    if this.sorting_param != None:
#      search_context.sort_param = this.sorting_param

    search_context.start_from = this.start_from
    search_context.results_count = this.results_per_page

    filter = this.filter

#    search_context.filter.lang = \
#      filter.lang == el.Lang.null and filter.lang or \
#      this.modifier.similar.event_id and this.modifier.similar.lang or \
#      filter.event.event_id and filter.event.lang or \
#      filter.lang

    search_context.filter.lang = filter.lang
    search_context.filter.country = filter.country
    search_context.filter.feed = filter.feed
    search_context.filter.category = filter.category
#    search_context.filter.spaces = filter.spaces

    search_context.filter.event = filter.event.event_id and \
      el.string.manip.base64_to_ulong(filter.event.event_id) or 0

    search_context.lang = this.language
    search_context.country = this.country
    search_context.locale = this.locale
    search_context.max_image_count = this.max_image_count

    search_context.user_agent = this.agent

    return search_context
  
  def save_prefs(this):
    
    this.context.request.output.send_header("Cache-Control",
                                            "no-cache, must-revalidate")

    if this.persist_prefs != None:
      this.set_cookie("pf", this.persist_prefs.string())
      this.set_cookie("lp", this.persist_lang_prefs.string())

    if this.crawler == None: this.set_uid()

  def redirect_to_canonical(this, location = '', path = '', extra_params = ''):
    this.context.request.output.send_location(\
      location or this.search_link(path = path, extra_params = extra_params))
    
    this.exit(this.crawler and 301 or 302)

  def channel_ttl(this):
    default_msg_period = 300
    max_request_period = 86400
    min_request_period = 300
#    msg_delay = 300
    approximate_messages = 3

    req_time = this.context.request.time().sec()

    msg_dates = []
    for msg in this.search_result.messages:
      if msg.published: msg_dates.append(msg.published)

    msg_dates.sort(reverse=True)

    msg_period = 0
    approximate_messages = min(approximate_messages, len(msg_dates))

    for i in range(0, approximate_messages):
      if i: msg_period += msg_dates[i-1] - msg_dates[i]

    if msg_period: msg_period /= approximate_messages - 1
    if msg_period == 0: msg_period = default_msg_period

    if approximate_messages: next_message_time = msg_dates[0] + msg_period
    else: next_message_time = req_time + max_request_period

#    next_message_time += msg_delay

    if next_message_time < req_time: 
      next_message_time = 2 * req_time - next_message_time
    
    tm = req_time + max_request_period
    if next_message_time > tm: next_message_time = tm

    tm = req_time + min_request_period
    if next_message_time < tm: next_message_time = tm

    return (next_message_time - req_time) / 60


  def crawler_query(this, query):
    
    if this.crawler and this.crawler.message_max_age >= 0:
      return query + " DATE " + str(this.crawler.message_max_age) + "S"
    else:
      return query

  def custom_log(this, level):
    return ""

  def search(this, search_context):

    this.ads = {}
    this.ad_counters = []
    
    this.search_result = None
    search_context.query = this.crawler_query(search_context.query)

    this.search_result = \
      not this.block and this.search_engine.search(search_context) or None

    if this.search_result:
      
      for ad in this.search_result.ad_selection.ads:
        if ad.inject == newsgate.ad.CI_FRAME:
          ad.text = \
            '<iframe src="/p/a?p=' + str(ad.id) + \
            '" class="ad_frame" height="' + str(ad.height) + \
            '" width="' + str(ad.width) + \
            '" allowtransparency="allowtransparency" scrolling="no"></iframe>'
        
        this.ads[ad.slot] = ad

      for cr in this.search_result.ad_selection.counters:
        this.ad_counters.append(cr)

      if not this.crawler:
        this.set_cookie("gf", this.search_result.ad_selection.ad_caps)
        this.set_cookie("cf", this.search_result.ad_selection.counter_caps)

    st = this.static
    request = this.context.request

    if st.logger.will_trace(el.logging.HIGH):
      if st.raw_stat_log_enabled:
        if this.trace_uri == 1 and this.crawler == None or \
           this.trace_uri == 2 and this.crawler or this.trace_uri == 3:
          st.logger.trace(el.logging.HIGH, this.context.request.uri(),
                          "r=", this.search_result and \
                            this.search_result.request_id or '',
                          ", B:C=", this.browser, ":",
                          this.block or this.crawler and this.crawler.name or \
                            '',
                          ", IP=", request.remote_ip(),
                          ", URI='", request.unparsed_uri(), "'",
                          this.custom_log(2))
        elif this.search_result:
          st.logger.trace(el.logging.HIGH, this.context.request.uri(),
                          "r=", this.search_result.request_id,
                          this.custom_log(1))
        else:
          lg = this.custom_log(0)

          if lg:
            st.logger.trace(el.logging.HIGH,
                            this.context.request.uri(),
                            this.custom_log(0))

      else:

        referer = request.input.headers().find("referer")
        
        st.logger.trace(el.logging.HIGH, request.uri(),
                        "ip=", request.remote_ip(),
                        " || lang=", this.language_l3_code,
                        " || cntr=", this.country_l3_code,
                        " || loc=", this.locale.lang.l3_code(), 
                        "-", this.locale.country.l3_code(),
                        " || br=", this.browser, ":",
                        this.block or this.crawler and this.crawler.name or "",
                        " || suppr=", this.suppression,
                        " || sort=", this.sorting,
                        " || start=", this.start_from,
                        " || res=", this.results_per_page,
                        " || mod=", this.raw_param("v"),
                        " || query=", this.query, 
                        " || ua=", this.agent,
                        " || ref=", referer,
                        this.custom_log(3))

    if this.total_matched_messages != None and \
       this.total_matched_messages < \
       this.search_result.total_matched_messages and \
       this.total_matched_messages >= \
       this.start_from + this.search_result.messages.size():

      diff = this.search_result.total_matched_messages - \
             this.total_matched_messages
      
      this.search_result.suppressed_messages += diff
      this.search_result.total_matched_messages -= diff      

    return this.search_result
  
  def translate_category_name(this, path):

    path = normalize_category(path)

    try:
      localized_name = this.cat_translation[path]
      
    except:

      if this.search_result == None: return ""
      
      subpath = ""
      localized_subpath = ""
      path_elems = path.split("/")
      path_elems.pop()

      for elem in path_elems:
            
        subpath += elem + "/"
        cat_info = this.search_result.category_stat.find(subpath)

        if cat_info == None: return ""

        localized_name = cat_info != None and \
          cat_info.localized_name.encode("utf-8") or ""

        if elem != "" and localized_name == "": return ""

        localized_subpath += localized_name + "/"      
        this.cat_translation[subpath] = localized_subpath

      localized_name = this.cat_translation[path]

    localized_name = localized_name[1:]
    if localized_name[-1:] == "/": localized_name = localized_name[0:-1]
  
    return localized_name

  def message_click_url(this, message, message_url = ""):

    if not message_url: message_url = message.url
    id = el.string.manip.mime_url_encode(message.encoded_id)
    proto = this.block and "z" or this.static.protocol
    
    return this.static.stat_url + '?s=U' + \
           el.string.manip.mime_url_encode(message_url) + \
           '&e=c&t=' + proto + '&r=' + \
           (this.test_mode and "000" or this.search_result.request_id) + \
           '&m=' + id + '&e=v&m=' + id

  def get_template(this, id):

    cache = this.context.cache
    lang = this.language_l3_code
  
    tname = lang + "." + id

    if tname not in cache:
      cache[tname] = el.string.template.Parser(this.loc(id), "{{", "}}")

    return cache[tname]

class Translator:

  languages = {}
  
  def __init__(this, from_lang, to_lang):
    this.from_lang = from_lang
    this.to_lang = to_lang
    this.prefered_source_lang = el.Lang.null

  def enrich_link(this, link):
    return link

  def enrich_outer_link(this, link):
    return link
  
  def headers(this):
    return ""

  def body(this):
    return ""

  def param(this):
    return "";

  def notranslate_class(this):
    return ""

  def translate_class(this):
    return ""

  def service(this, lang):
    return ""

  def logo(this, lang):
    return ""

class GoogleTranslator(Translator):

  google_translator_base = Translator

  languages = \
    {
      "afr":0,"alb":0,"ara":0,"arm":0,"aze":0,"baq":0,"bel":0,"ben":0,
      "bos":0,"bul":0,"cat":0,"ceb":0,"chi":0,"scr":0,"cze":0,"dan":0,"dut":0,
      "eng":0,"epo":0,"est":0,"tgl":0,"fin":0,"fre":0,"glg":0,"geo":0,"ger":0,
      "gre":0,"guj":0,"hat":0,"heb":0,"hin":0,"hmn":0,"hun":0,"ice":0,"ind":0,
      "gle":0,"ita":0,"jpn":0,"jav":0,"kan":0,"khm":0,"kor":0,"lao":0,"lat":0,
      "lav":0,"lit":0,"mac":0,"mal":0,"mlt":0,"mar":0,"nor":0,"per":0,"pol":0,
      "por":0,"rum":0,"rus":0,"scc":0,"slo":0,"slv":0,"spa":0,"swa":0,"swe":0,
      "tam":0,"tel":0,"tha":0,"tur":0,"ukr":0,"urd":0,"vie":0,"wel":0,"yid":0
    }

  def __init__(this, args, id):

    arg_list = args.split("|")
  
    this.google_translator_base.__init__(this,
                                         el.Lang(arg_list[0]),
                                         el.Lang(arg_list[1]))

    this.id = id
    # Language google translate best from
    this.prefered_source_lang = el.Lang("eng")

  def param(this):
    return "tr=google-" + this.from_lang.l3_code() + "%7C" + \
           this.to_lang.l3_code()

  def enrich_link(this, link):

    from_l2 = this.from_lang.l2_code()
    to_l2 = this.to_lang.l2_code()
    if to_l2 == "zh": to_l2 += "-CN"

    if link.find("&tr=") < 0 and link.find("?tr=") < 0:
      if link: link += (link.find('?') < 0 and "?" or "&")
      link += this.param()
    
    link += "#googtrans%28" + (from_l2 or "auto") + "%7C" + \
            to_l2 + "%29"
    
    return link

  def enrich_outer_link(this, link):

    from_l2 = this.from_lang.l2_code()
    to_l2 = this.to_lang.l2_code()
    if to_l2 == "zh": to_l2 += "-CN"

    return "http://translate.google.ru/translate?sl=" + (from_l2 or "auto") +\
           "&tl=" + to_l2 + "&u=" + el.string.manip.mime_url_encode(link)

  def headers(this):
    return '\n<meta name="google-translate-customization" content="' + \
           this.id + R'''"/>
<style>
.goog-tooltip
{
  display: none !important;
}
.goog-tooltip:hover
{
  display: none !important;
}
.goog-text-highlight
{
  background-color: transparent !important;
  border: none !important; 
  box-shadow: none !important;
}           
.goog-te-banner-frame
{
  display: none !important;
}
#body
{
  top:0px !important;
}
</style>'''

  def body(this):
    return R'''
<script type="text/javascript">
function googleTranslateElementInit() {
  new google.translate.TranslateElement({pageLanguage: 'en', layout: google.translate.TranslateElement.InlineLayout.HORIZONTAL, autoDisplay: false, multilanguagePage: true}, 'google_translate_element');
}
</script><script type="text/javascript" src="//translate.google.com/translate_a/element.js?cb=googleTranslateElementInit"></script>
'''

  def notranslate_class(this):
    return "notranslate"

  def translate_class(this):
    return "translate"

  def service(this, lang):
    return "http://translate.google.com/"

  def logo(this, lang):
    return "http://www.google.com/intl/" + (lang.l2_code() or "en") + \
           "/images/logos/translate_logo_sm.png"

class SearchPageContext(SearchRequestContext):

  search_page_context_base = SearchRequestContext
  
  RBR_HIDE = 0
  RBR_SHOW = 1
  RBR_NOINDEX = 2

  SV_DESK = 0
  SV_TAB  = 1
  SV_MOB  = 2

  def __init__(this,
               context,
               search_engine,
               protocol,
               read_prefs):

    this.search_page_context_base = SearchRequestContext
    
    this.search_page_context_base.__init__(this,
                                           context,
                                           search_engine,
                                           protocol,
                                           read_prefs)
    
    try:
      tmp = this.static.informer_enable
    except:
      conf = context.config.get

      this.static.development = conf("development") == "1"

      try:
        thumb_mob_dims = conf("search.image.thumbnail.mobile").split('x')
        thumb_mob_width = int(thumb_mob_dims[0])
        thumb_mob_height = int(thumb_mob_dims[1])
      except:
        thumb_mob_width = 0
        thumb_mob_height = 0

      this.static.thumb_mobile = thumb_mob_width and thumb_mob_height and \
        ImageDim(thumb_mob_width, thumb_mob_height) or None
      
      this.static.spam_patterns = this.compile_spam_patterns(conf)
      this.static.data_root = conf("data_root")
      this.static.server_instance_name = conf("server_instance_name")
      this.static.copyright_note = conf("copyright_note")

      try:
        this.static.show_thumb = \
          int(conf("search.image.thumbnail.enable")) == 1
      except:
        this.static.show_thumb = False
      
      this.static.email = conf("server_instance_email")

      this.static.translator_default = conf("translator_default")
      this.static.translator_google_id = conf("translator_google_id")      

      try:
        this.static.informer_enable = int(conf("search.informer.enable")) != 0
      except:
        this.static.informer_enable = False

    request = context.request

    this.ad_counters = []
    this.brief_text_for_crawler = False
    this.google_ad_section = True
    this.informer_create_mode = False
    this.anchor_refs = []  
    this.msg_url = 'http://' + this.static.canonoical_endpoint + '/p/s/m'
    this.h1_tagline = False
    
    this.translator = None
    this.default_translator = None
    this.can_translate = False
    this.need_translation = False

    this.lang_counter = {}
    this.country_counter = {}
    
    # 1.5KB
    this.max_get_length = 1536

    this.posted = request.method_number() == 2
    this.uid = request.input.cookies().most_specific("u")

    prefs = this.crawler == None and this.prefs or None
    
    site_version = \
      this.string_param(\
        "sv",
        str(this.def_site_version()),
        display_param = True)

    this.site_version = int(site_version)

    this.with_images = \
      this.bool_param(\
        "i",
        "1",
        "0",
        (prefs != None and prefs.value("i") or "") != "0",
        prefs,
        True)

    def_cols = prefs != None and prefs.value("c") or ""
    if not def_cols: def_cols = this.phone and "1" or "2"

    this.page_ad_id = newsgate.ad.PI_UNKNOWN

    this.message_view = \
      this.string_param("mvw", "paper", True, True)    

    if this.message_view == "nline":

      this.results_per_page = \
        int(this.string_param("r", 100, display_param = True))

    this.print_left_bar = \
      this.bool_param("plb", "1", "0", False, True, True)      

    this.in_2_columns = \
      this.bool_param(\
        "c",
        "2",
        "1",
        def_cols == "2",
        prefs,
        True)
                      
    this.large_print = \
      this.bool_param(\
        "p",
        "l",
        "s",
        (prefs != None and prefs.value("p") or "") == "l",
        prefs,
        True)

    this.desc_length = \
      int(this.string_param("dl",
                            this.site_version == SearchPageContext.SV_MOB and \
                              400 or -1,
                            display_param = True))

    param = this.raw_param("tr")
    
    if param[0:7] == "google-":
      if this.static.translator_google_id:
        this.translator = GoogleTranslator(param[7:],
                                           this.static.translator_google_id)

    this.translator_service = \
      this.string_param("trs",
                        this.static.translator_default,
                        display_param = True)

    #  or this.crawler
    if this.translator_service == "google" and \
       not this.static.translator_google_id:
      this.translator_service = ""

    this.translation_def_lang = ""
    ac = this.context.request.input.accept_languages()
    if len(ac): this.translation_def_lang = ac[0].language.l3_code()

    if not this.translation_def_lang:
      
      this.translation_def_lang = \
        this.translator and this.translator.to_lang.l3_code() or \
        this.language_l3_code or "eng"

    this.translation_lang = \
      this.string_param("trl",
                        this.translation_def_lang,
                        display_param = True)      

    if this.translator_service:

      if this.translator_service == "google":
        if this.static.translator_google_id:
          this.default_translator = \
            GoogleTranslator("|" + this.translation_lang,
                             this.static.translator_google_id)      

    if this.translator and this.translator.to_lang == this.filter.lang and \
       this.translator.to_lang.l3_code() in this.static.main_languages:
      this.translator = None

    this.translate_locale()

  def translate_locale(this):
    
    if this.translator:
      
      lang = None
      
      if this.translator.to_lang.l3_code() in this.static.main_languages:
        lang = this.translator.to_lang
      elif this.translator.prefered_source_lang.l3_code() in \
           this.static.main_languages:
        lang = this.translator.prefered_source_lang

      if lang:
        this.locale = \
          this.context.request.input.locale(lang,
                                            this.filter.country)

  def search(this, search_context):
    
    this.search_page_context_base.search(this, search_context)

    if this.search_result:

      for l in this.search_result.lang_filter_options:
        if l.count: this.lang_counter[l.id] = l.count

      for c in this.search_result.country_filter_options:
        if c.count: this.country_counter[c.id] = c.count

    return this.search_result

  def set_language(this, new_lang):
    
    this.search_page_context_base.set_language(this, new_lang)
    this.translate_locale()
    
  def translate_link(this, link, translator = True):
    if translator == True: translator = this.translator
    if translator and not this.crawler: link = translator.enrich_link(link)
    return link

  def search_link(this,
                  export = False,
                  modifier = True,
                  query = True,
                  filter = True,
                  extra_params = "",
                  path = True,
                  debug = False,
                  translator = True):

    link = \
      this.search_page_context_base.search_link(this,
                                                export,
                                                modifier,
                                                query,
                                                filter,
                                                extra_params,
                                                path,
                                                debug)

    return this.translate_link(link, translator)
    
  def find_thumbnail(this, img, thumb_width, thumb_height):

    selected_thumb = None
    adjust_ratio = None
    min_square_dif = sys.maxint
  
    image_proportion = float(img.width) / float(img.height)
    thumb_square = thumb_width * thumb_height

    # Searching for best match with at least one dimension equal
    for thumb in img.thumbs:
      if (thumb.width == thumb_width and thumb.height <= thumb_height or \
          thumb.height == thumb_height and thumb.width <= thumb_width):

        square_dif = thumb_square - thumb.width * thumb.height

        if square_dif < min_square_dif:
          selected_thumb = thumb
          min_square_dif = square_dif
          if min_square_dif == 0: break

    # Searching for best match with one dimensions bigger; 
    # thumb size to be adjusted
    if selected_thumb == None:

      min_square_dif = sys.maxint
      min_ratio = float(sys.maxint)

      for thumb in img.thumbs:
        if thumb.width >= thumb_width or thumb.height >= thumb_height:

          wratio = float(thumb.width) / thumb_width
          hratio = float(thumb.height) / thumb_height
          ratio = max(wratio, hratio)

          square_dif = thumb_square - round(float(thumb.width) / ratio) * \
            round(float(thumb.height) / ratio)

          if ratio < min_ratio or \
             ratio == min_ratio and square_dif < min_square_dif:

            selected_thumb = thumb
            min_ratio = ratio
            if min_ratio == 1: break
            min_square_dif = square_dif

      if selected_thumb != None: adjust_ratio = min_ratio

    # Searching for best match with dimensions smaller
    if selected_thumb == None:

      min_square_dif = sys.maxint

      for thumb in img.thumbs:
        if thumb.width <= thumb_width and thumb.height <= thumb_height:

          square_dif = thumb_square - thumb.width * thumb.height

          if square_dif < min_square_dif:
            selected_thumb = thumb
            min_square_dif = square_dif
            if min_square_dif == 0: break
          
    if selected_thumb == None: return None

    if adjust_ratio == None:
      return ThumbInfo(selected_thumb,
                       selected_thumb.width,
                       selected_thumb.height)

    return ThumbInfo(selected_thumb, 
                     round(float(selected_thumb.width) / adjust_ratio),
                     round(float(selected_thumb.height) / adjust_ratio))

  def create_image_desc(this, img):

    thumb = None

    if this.static.show_thumb and img.thumbs.size() > 0:
      if this.site_version == SearchPageContext.SV_MOB and \
         this.static.thumb_mobile != None:
        
        thumb = this.find_thumbnail(img,
                                    this.static.thumb_mobile.width,
                                    this.static.thumb_mobile.height)

        if thumb: thumb.src = thumb.thumb.src
        
      else:
        thumb = img.thumbs[0]

    if thumb != None:

      return ImageDesc(thumb.src,
                       'width="' + str(thumb.width) + '"',
                       'height="' + str(thumb.height) + '"',
                       thumb.width < img.orig_width or \
                         thumb.height < img.orig_height)
    else:

      return ImageDesc(\
        img.src,
        img.width >= 0 and ('width="' + str(img.width) + '"') or '',
        img.height >= 0 and ('height="' + str(img.height) + '"') or '',
        img.width < img.orig_width or img.height < img.orig_height)

  def def_site_version(this):

    return this.tab and SearchPageContext.SV_TAB or \
             this.phone and SearchPageContext.SV_MOB or \
             SearchPageContext.SV_DESK

  def prn_menu(this, menu, first, separator=" | "):

    if menu:
      if first: first = False
      else: this.prn(separator)
      this.prn(menu)
      
    return first

  def prn_notranslate_class(this):

    if this.translator:
      this.prn(' class="', this.translator.notranslate_class(), '"')

  def ad_section_start(this):
    return this.crawler and this.crawler.name == "adsensebot" and \
           this.google_ad_section and '<!-- google_ad_section_start -->' or ''

  def ad_section_end(this):
    return this.crawler and this.crawler.name == "adsensebot" and \
           this.google_ad_section and '<!-- google_ad_section_end -->' or ''

  def round_time(this, seconds):
    if seconds < 60:
      return this.localizer.plural("second", seconds, this.language)

    if seconds < 3600:
      return this.localizer.plural("minute",
                                   (seconds + 30) / 60,
                                   this.language)

    if seconds < 86400:
      return this.localizer.plural("hour",
                                   (seconds + 3600 / 2) / 3600,
                                   this.language)

    if seconds < 86400 * 60:
      return this.localizer.plural("day",
                                   (seconds + 86400 / 2) / 86400,
                                   this.language)

    return this.localizer.plural("month",
                                 (seconds + 86400 * 30 / 2) / (86400 * 30),
                                 this.language)

  def friendly_time(this, seconds, short):

    text = ""

    sign = seconds < 0 and "-" or ""
    seconds = abs(seconds)
    
    days = seconds / 86400

    if days:
      seconds -= days * 86400
      
      if short:
        text += str(days) + "d"
      else:
        text = this.localizer.plural("day", days, this.language)

    hours = seconds / 3600

    if hours:
      seconds -= hours * 3600
      if text: text += " "

      if short:
        text += str(hours) + "h"
      else:
        text += this.localizer.plural("hour", hours, this.language)

    minutes = seconds / 60

    if minutes:
      seconds -= minutes * 60
      if text: text += " "

      if short:
        text += str(minutes) + "m"
      else:
        text += this.localizer.plural("minute", minutes, this.language)

    if seconds or text == "":
      if text: text += " "
      
      if short:
        text += str(seconds) + "s"
      else:
        text += this.localizer.plural("second", seconds, this.language)

    return sign + text.strip()

  def category_url(this, cat_path, path = True, export = False, ad_tag = None):

    mod = Modifier()
    mod.category = cat_path

    extra_params = \
      ad_tag and ('at=' + el.string.manip.mime_url_encode(ad_tag)) or ''

    return this.search_link(\
      modifier = mod, 
      filter = this.source_cat_link_filter(), 
      path = path,
      export = export,
      extra_params = extra_params)    

  def source_url(this, msg, path = True, export = False, ad_tag = None):

    mod = Modifier()
    mod.source = msg.source_url

    extra_params = \
      ad_tag and ('at=' + el.string.manip.mime_url_encode(ad_tag)) or ''

    return this.search_link(\
      modifier = mod, 
      filter = this.source_cat_link_filter(), 
      path = path,
      export = export,
      extra_params = extra_params)
    
  def story_url(this, msg, path = True, export = False, ad_tag = None):
    
    mod = Modifier()
    mod.similar.message_id = msg.encoded_id
    mod.similar.event_id = msg.encoded_event_id
    
    extra_params = \
      ad_tag and ('at=' + el.string.manip.mime_url_encode(ad_tag)) or ''

    if this.crawler:
      if this.country_filters: 
        filter = Filter()
        filter.country = this.filter.country
      else:
        filter = False
    else:
      filter = this.filter.sticky()

    return this.search_link(\
      modifier = mod, 
      filter = filter, 
      path = path,
      export = export,
      extra_params = extra_params)

  def message_url(this,
                  msg,
                  img = None,
                  last_resort = None,
                  translator = True,
                  ad_tag = None):
    
    link = this.msg_url + '?msg=' + \
           el.string.manip.mime_url_encode(msg.encoded_id) + \
           (img and img.index and ('&img=' + str(img.index)) or "") + \
           (last_resort and \
            ('&lr=' + el.string.manip.mime_url_encode(last_resort)) or "") + \
            (ad_tag and ('&at=' + el.string.manip.mime_url_encode(ad_tag)) or \
             '')

    return this.translate_link(link, translator)

  def page_impression_url(this):

    proto = this.block and "z" or this.static.protocol

    return this.crawler == None and this.search_result != None and \
           (this.static.stat_url + '?e=p&t=' + proto + '&r=' + \
            this.search_result.request_id) or ''

  def page_impression_image(this, extra_params = ""):
    url = this.page_impression_url()

    return url and ('<img width="0" height="0" \
style="padding:0; margin:0; border:0px; display:none;" src="' + \
           el.string.manip.xml_encode(\
             url + (extra_params and ('&' + extra_params) or '')) + '"/>') or \
             ''
  
  def make_ref(this,
               ref,
               extra_post_refs,
               post = None,
               informer_create_mode = None):

    if len(ref) > this.max_get_length: post = True
    elif post == None: post = this.posted

    if informer_create_mode == None:
      informer_create_mode = this.informer_create_mode
    
    if post or informer_create_mode:

      try:
        index = this.anchor_refs.index(ref)
      except:
        this.anchor_refs.append(ref)
        index = len(this.anchor_refs) - 1
    
      return "javascript:post(" + str(index) + "," +\
             str(extra_post_refs and "1" or "0") + ");"
    else:
      return ref

  def create_visible_cat_white_list(this, cat, current_cat):
    if current_cat == None: return None

    if cat.id == current_cat.id:
      white_list = {}
      white_list[cat.id] = 1
      return white_list

    for subcat in cat.categories:
      white_list = this.create_visible_cat_white_list(subcat, current_cat)
      if white_list != None:
        white_list[cat.id] = 1
        return white_list

    return None

  def show_cat(this,
               cat,
               visible_cat_white_list,
               current_cat,
               parent,
               level,
               msg_count_threshold):

    return cat.id in visible_cat_white_list or \
      current_cat != None and (\
      current_cat.id == cat.parent or current_cat.parent == cat.parent) or \
      parent == "" or cat.matched_message_count >= msg_count_threshold or \
      this.informer_create_mode and level == 2

  def prn_right_bar_cat(this,
                        cat,
                        parent,
                        visible_cat_white_list,
                        msg_count_threshold,
                        level,
                        current_cat,
                        path):

    cat_name = cat.localized_name.encode("utf-8")

    if not cat_name or not cat.message_count: return

#    show_cat = cat.id in visible_cat_white_list or \
#      current_cat != None and (\
#      current_cat.id == cat.parent or current_cat.parent == cat.parent) or \
#      parent == "" or cat.matched_message_count >= msg_count_threshold or \
#      this.informer_create_mode and level == 2

    if this.show_cat(cat,
                     visible_cat_white_list,
                     current_cat,
                     parent,
                     level,
                     msg_count_threshold) == False: return

    cat_location = str(cat.id) + '_' + str(cat.parent)
    cat_path = parent + cat.name.encode("utf-8")
    
    visible_subcats = 0
    hidden_subcats = 0

    for subcat in cat.categories:
      if subcat.localized_name != "" and subcat.message_count:
        if this.show_cat(subcat,
                         visible_cat_white_list,
                         current_cat,
                         cat_path + '/',
                         level + 1,
                         msg_count_threshold):
#        if subcat.id in visible_cat_white_list or \
#           subcat.matched_message_count >= msg_count_threshold or \
#           this.informer_create_mode and level == 1:
          visible_subcats += 1
        else:
          hidden_subcats += 1

    this.prn('\n<div id="cat_', cat_location, '" class="', 
             parent == "" and 'right_block_cat' or 'right_block_subcat', 
             '">')

    if this.crawler == None:

      if hidden_subcats == 0 and visible_subcats == 0:
        this.prn('<span class="right_block_cpad"></span>')

      else:
        this.prn('<a href="javascript:show_cat(', cat.id, ',', cat.parent,
                 ',true)" id="cat_exp_', cat_location, 
                 '"', hidden_subcats == 0 and ' style="display:none"' or '',
                 '><img src="', this.resource_url('/fixed/img/plus.png'), 
                 '" width="11" height="11" class="right_block_img"/></a>')

        this.prn('<a href="javascript:show_cat(', cat.id, ',', cat.parent,
                 ',false)" id="cat_col_', cat_location,
                 '"', hidden_subcats and ' style="display:none"' or '', 
                 '><img src="', this.resource_url('/fixed/img/minus.png'),
                 '" width="11" height="11" class="right_block_img"/></a>')

    if current_cat != None and current_cat.id == cat.id:
      this.prn('<span id="lnk_cat_', cat_location, 
               '" class="right_bar_cat_current">',
               el.string.manip.xml_encode(cat_name), '</span>')
#               cat.matched_message_count, "/", cat.message_count)
    else:

      cat_url = this.category_url('/' + cat_path, path = path)

      this.prn('<a id="lnk_cat_', cat_location, '" href="',
               el.string.manip.xml_encode(this.make_ref(cat_url, False)),
               '">', el.string.manip.xml_encode(cat_name), '</a>')
#               cat.matched_message_count, "/", cat.message_count)

    parent = cat_path + '/'

    for subcat in cat.categories:
      this.prn_right_bar_cat(subcat, 
                             parent,
                             visible_cat_white_list,
                             msg_count_threshold,
                             level + 1,
                             current_cat,
                             path)
 
    this.prn('</div>')

  def prn_right_bar_source(this, src, nofollow, bullet):

    link = src.html_link
    
    this.prn(R'''
  <tr valign="top">
  <td>''', bullet and R'''<span class="right_block_source_bullet">&#xB7;</span>''' or '',
             R'''
  </td>
  <td class="right_block_source"><a href="''',
             el.string.manip.xml_encode(link),
             '"', nofollow and ' rel="nofollow"' or '', ' target="_blank">',
             el.string.manip.xml_encode(src.title), '</a></td></tr>')

  def prn_right_bar_ad(this, pos):

    if this.page_ad_id == newsgate.ad.PI_UNKNOWN:    
      return

    if this.page_ad_id == newsgate.ad.PI_DESK_PAPER:
      slot = newsgate.ad.SI_DESK_PAPER_RTB1 + pos
    elif this.page_ad_id == newsgate.ad.PI_DESK_NLINE:
      slot = newsgate.ad.SI_DESK_NLINE_RTB1 + pos
    elif this.page_ad_id == newsgate.ad.PI_DESK_COLUMN:
      slot = newsgate.ad.SI_DESK_COLUMN_RTB1 + pos
    elif this.page_ad_id == newsgate.ad.PI_TAB_PAPER:
      slot = newsgate.ad.SI_TAB_PAPER_RTB1 + pos
    elif this.page_ad_id == newsgate.ad.PI_TAB_NLINE:
      slot = newsgate.ad.SI_TAB_NLINE_RTB1 + pos
    elif this.page_ad_id == newsgate.ad.PI_TAB_COLUMN:
      slot = newsgate.ad.SI_TAB_COLUMN_RTB1 + pos
    elif this.page_ad_id == newsgate.ad.PI_DESK_MESSAGE:
      slot = newsgate.ad.SI_DESK_MESSAGE_RTB1 + pos
    elif this.page_ad_id == newsgate.ad.PI_TAB_MESSAGE:
      slot = newsgate.ad.SI_TAB_MESSAGE_RTB1 + pos
    else:
      return
    
    if slot in this.ads:

      this.prn('\n<div class="ad_right_', pos == 0 and '1st_' or '',
               'block">', this.ads[slot].text, '</div>')

  def prn_right_bar(this, resources_block, advertise, path = True):
    
    categories = []
    msg_count_threshold = 0
    current_cat = None
    visible_cat_white_list = {}

    if this.search_result != None:
      category = this.search_result.category_stat.find('/')
      if category != None: categories = category.categories

      msg_count_threshold = \
        math.ceil(float(this.search_result.total_matched_messages + \
                  this.search_result.suppressed_messages) / 25)

      cat = this.modifier.category or this.filter.category      
      if cat != "": current_cat = this.search_result.category_stat.find(cat)

    msg_count_threshold = max(1, msg_count_threshold)

    visible_cat_white_list = None
    relevant_cats = []
    irrelevant_cats = []

    for cat in categories:
    
      if visible_cat_white_list == None:
        visible_cat_white_list = \
          this.create_visible_cat_white_list(cat, current_cat)
    
      if cat.localized_name and cat.message_count:
        if this.informer_create_mode or \
           cat.matched_message_count >= msg_count_threshold or \
           visible_cat_white_list != None and cat.id in visible_cat_white_list:
          relevant_cats.append(cat)
        else:
          irrelevant_cats.append(cat)

    if visible_cat_white_list == None:
      visible_cat_white_list = {}

    if this.informer_create_mode == False:
      relevant_cats.sort(cmp_cat_relevance)

    if this.site_version != SearchPageContext.SV_MOB:            
      this.prn('\n<td class="right_bar">')

    if advertise: this.prn_right_bar_ad(0)

    rbt_classname = "right_block_title"
    rb_classname = "right_block"
      
    if this.translator:
      rbt_classname += " " + this.translator.notranslate_class()

      if this.translator.to_lang == this.locale.lang and \
         this.locale.lang.l3_code() in this.static.main_languages:
        rb_classname += " " + this.translator.notranslate_class()
      else:
        rb_classname += " " + this.translator.translate_class()
  
    if len(relevant_cats):

      this.prn('\n<div class="', rbt_classname, '">''', this.lm, 
               this.informer_create_mode and "RIGHT_BAR_CATEGORIES" or \
               "RIGHT_BAR_READ_ALSO", 
               '</div>\n<div class="', rb_classname, '">')

      for cat in relevant_cats:
        this.prn_right_bar_cat(cat,
                               "",
                               visible_cat_white_list,
                               msg_count_threshold,
                               1,
                               current_cat,
                               path)
    
      this.prn('\n</div>')

    if len(irrelevant_cats):
      this.prn('\n<div class="', rbt_classname, '">''', this.lm, 
               len(relevant_cats) and "RIGHT_BAR_OTHER_SUBJECTS" or \
               "RIGHT_BAR_CATEGORIES",
               '</div>\n<div class="', rb_classname, '">')

      for cat in irrelevant_cats:
        this.prn_right_bar_cat(cat,
                               "",
                               {},
                               msg_count_threshold,
                               1,
                               current_cat,
                               path)

      this.prn('\n</div>')

    source_map = {}
    sources = []

    if this.search_result != None and resources_block:
      for msg in this.search_result.messages:
        link = msg.source_html_link or msg.source_url
        source_map[link] = msg.source_title or link

    for (k, v) in source_map.items():
      sources.append(Source(v, k))

    sources.sort(cmp_sources)
    source_count = len(sources)
    
    if source_count:

      if resources_block == SearchPageContext.RBR_NOINDEX:
        this.prn('\n<noindex>')

      classname = "right_block"
      
      if this.translator:
        classname += " " + this.translator.notranslate_class()
      
      this.prn('\n<div class="', rbt_classname, '">', this.lm, 
               len(sources) > 1 and "RIGHT_BAR_HTML_SOURCES" or \
               "RIGHT_BAR_HTML_SOURCE", R'''</div>
          <div class="''', classname, R'''">
          <table class="right_block_sources" cellspacing="0">''')

      for src in sources:
        this.prn_right_bar_source(\
          src,
          resources_block == SearchPageContext.RBR_NOINDEX,
          source_count > 1)

      this.prn('\n</table></div>')
    
      if resources_block == SearchPageContext.RBR_NOINDEX:
        this.prn('\n</noindex>')

    if advertise: this.prn_right_bar_ad(1)

    if this.site_version != SearchPageContext.SV_MOB:
      this.prn('\n</td>\n')

  def prn_msg_category(this,
                       msg,
                       cat_path,
                       encoded_cat_name,
                       path = True,
                       ad_tag = None):

    link = this.make_ref(\
      this.category_url(cat_path, path = path, ad_tag = ad_tag), False)

    classname = "msg_category"
    
    if this.translator and \
       this.translator.to_lang.l3_code() in this.static.main_languages:
      classname += " " + this.translator.notranslate_class()    
    
    this.prn('<a class="', classname, '" href="',
             el.string.manip.xml_encode(link), '" id="mc_', msg.encoded_id,
             '_', cat_path, '">', this.ad_section_start(),
             encoded_cat_name, this.ad_section_end(), '</a>')
    
  def prn_msg_categories(this, msg, path = True, ad_tag = None):
  
    first = True
    prev_parent_cat = ""

    for cat in msg.categories:

      if this.extra_msg_info:

        if first:
          if this.crawler == None or this.brief_text_for_crawler == False:
            this.prn("<span id='cat_", msg.encoded_id)

            if this.translator:
              this.prn("' class='", this.translator.notranslate_class())
            
            this.prn("'>", this.lm, "CATEGORIES", " </span>")
          
          first = False
        else:
          this.prn("&#xA0;&#xB7; ")

        
        localized_cat = cat
        
      else:
        localized_cat = this.translate_category_name(cat)
        if localized_cat == "": continue

        cat_parts = localized_cat.rsplit('/', 1)
        short_cat_name = False

        if len(cat_parts) == 2:
          parent_cat = cat_parts[0]
          short_cat_name = parent_cat == prev_parent_cat
          if short_cat_name: localized_cat = cat_parts[1]
        else:
          parent_cat = ""

        prev_parent_cat = parent_cat

        if first:
          if this.crawler == None or this.brief_text_for_crawler == False:
            this.prn("<span id='cat_", msg.encoded_id)

            if this.translator:
              this.prn("' class='", this.translator.notranslate_class())
            
            this.prn("'>", this.lm, "CATEGORIES", " </span>")
          
          first = False
        else:
          this.prn(short_cat_name and ", " or "&#xA0;&#xB7; ")

      this.prn_msg_category(msg,
                            cat,
                            el.string.manip.xml_encode(localized_cat),
                            path,
                            ad_tag)

  def resource_url(this, path):
    return this.site + path

  def compound_resource_url(this, path):
    return this.site + "/comp/" + this.static.server_version + "/" + path

  def search_context(this):

    ctx = this.search_page_context_base.search_context(this)

    ctx.in_2_columns = this.in_2_columns
    ctx.message_view = this.message_view;
    ctx.print_left_bar = this.print_left_bar

    if this.default_translator:
      ctx.translate_def_lang = this.default_translator.to_lang.l3_code()

    if this.translator:
      ctx.translate_lang = this.translator.to_lang.l3_code()

    if this.static.main_languages:
      ctx.sr_flags |= newsgate.search.SearchContext.RF_LANG_STAT
      
    if this.country_filters:
      ctx.sr_flags |= newsgate.search.SearchContext.RF_COUNTRY_STAT
      
    return ctx

  def prn_html_open(this):
    this.prn(R'''<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html prefix="og: http://ogp.me/ns#">''')

  def prn_html_close(this):
    this.prn('\n</html>')

  def prn_head_open(this):

    this.prn(R'''
<head>
<meta http-equiv="content-type" content="text/html; charset=UTF-8">''')

    if this.site_version == SearchPageContext.SV_MOB:
      this.prn('\n<meta name="viewport" content="width=device-width"/>')

  def prn_head_close(this):

    if this.interceptor:
      head = this.interceptor.head()
      if head != None: this.prn('\n', head)
    
    this.prn('\n</head>')

  def prn_css(this):
      
    this.prn(\
      '\n<link rel="stylesheet" type="text/css" href="',
      this.resource_url('/fixed/css/commons.css'), '" media="all"/>')

    if this.site_version == SearchPageContext.SV_MOB:
    
      this.prn(\
        '\n<link rel="stylesheet" type="text/css" href="',
        this.resource_url('/fixed/css/commons-mob.css'), '" media="all"/>')

  def prn_script(this,
                 search_path = "",
                 search_query_extra_params = "",
                 view_options = True):

    request = this.context.request
    accept_languages = request.input.accept_languages()

    if search_path == "": search_path = this.service
 
    fl = this.filter.sticky()

    this.prn(R'''
<script src="''', this.resource_url('/fixed/js/elements.js'),
             R'''" type="text/javascript"></script>
<script type="text/javascript">
var page =
{
  site:
  {
    language:
    {
      current: "''', this.language_l3_code, R'''",
      def: "''', len(accept_languages) > 0 and \
                 this.context.valid_language(\
                   accept_languages[0].language).l3_code() or \
                 this.language_l3_code, R'''"
    },
    version:
    {
      DESKTOP: ''', this.SV_DESK, R''',
      TABLET: ''',  this.SV_TAB, R''',
      MOBILE: ''',  this.SV_MOB, R''',      
      current: ''', this.site_version, R''',
      def: ''', this.def_site_version(), R'''
    }
  },
  search:
  {
    path: "''',  el.string.manip.js_escape(search_path), R'''",
    query: "''', el.string.manip.js_escape(\
                   this.search_query(\
                     extra_params = search_query_extra_params)), R'''",
    cat_query_prefix: "''',
                 el.string.manip.js_escape(\
                   this.search_query(modifier = None, filter = fl)), R'''",
    filter:
    {
      language: "''', this.filter.lang.l3_code(True), R'''",
      country: "''', this.filter.country.l3_code(), R'''",
      def:
      {
        fo_n_language: "''',
             this.static.main_languages and \
             this.default_lang_filter(True).l3_code() or "zzz", R'''"
      }
    },
    message_view: "''', this.message_view, R'''",
    results_per_page: ''', this.results_per_page, R''',
    desc_length: ''', this.desc_length, R''',
    sorting_type: ''', this.sorting - 1)

    if view_options:
      this.prn(R''',
    view_option:
    {
      i: "''', this.with_images and "1" or "0", R'''",
      c: "''', this.in_2_columns and "2" or "1", R'''",
      p: "''', this.large_print and "l" or "s", R'''",
      b: "''', this.suppression == \
               newsgate.search.SearchContext.ST_NONE and \
               str(newsgate.search.SearchContext.ST_NONE) or \
               str(newsgate.search.SearchContext.ST_COLLAPSE_EVENTS), R'''",
      plb: "''', this.print_left_bar and "1" or "0", R'''"
    }''')

    this.prn(R'''
  },
  translator:
  {
    current: "''', this.translator_service, R'''",
    def: "''', this.static.translator_default, R'''",
    options:
    [
      { v:"", n:"''', this.lm, "TRANSLATE_NONE", '", s:[] }')

    if this.static.translator_google_id:
      this.prn(R''',
      { v:"google", n:"''', this.lm, "TRANSLATE_GOOGLE",
               '", s:[''')

      first = True
      
      for l in GoogleTranslator.languages:
      
        if first: first = False
        else: this.prn(",")

        this.prn('"', l, '"')

      this.prn('] }')
    
    this.prn(R'''
    ],
    lang: "''', this.translation_lang, R'''",
    def_lang: "''', this.translation_def_lang, R'''",
    cur_lang: "''', this.translator and this.translator.to_lang.l3_code() or \
             "", R'''",
    query_suffix: "''', this.translate_link(""), R'''",
    lang_url: "''', this.resource_url('/p/l?l=' + this.language_l3_code), R'''"
  },
  extra_params: null''')

    if this.site_version == SearchPageContext.SV_MOB:
      this.prn(R''',
  category_bar: { text:"''', el.psp.JS_EscapeMarker)

      this.prn_right_bar(SearchPageContext.RBR_HIDE, False, search_path)
      this.prn(el.psp.NoJS_EscapeMarker, '" }')

    this.prn(R'''  
};
</script>
<script src="''', this.resource_url('/tsp/js/' + \
                                    this.language_l3_code + '/commons.js'),
             '" type="text/javascript"></script>')

  def prn_title(this):

    title = this.modifier.all and this.loc("NEWS_TITLE") or \
            this.query_title()

    translate_title = this.translator and \
      (this.modifier.category and
       (this.translator.to_lang != this.locale.lang or \
       this.locale.lang.l3_code() not in this.static.main_languages) or \
       this.modifier.similar.event_id)    
      
    this.prn('\n<title', this.translator and not translate_title and \
             ' class="notranslate"' or '', '>', title, ' - ',
             el.string.manip.xml_encode(this.static.server_instance_name),
             '</title>')

  def prn_description(this):

    description = this.modifier.all and this.loc("NEWS_DESCRIPTION") or \
                  this.query_description()
    
    if description:
      this.prn('\n<meta name="description" content="', description, '">')

  def prn_meta_keywords(this, keyword_count):
    
    if this.search_result == None: return

    if this.modifier.category:

      keywords = this.search_result.category_locale.keywords
  
      if keywords:
        this.prn('\n<meta name="keywords" content="', 
                  el.string.manip.xml_encode(keywords), '">')

    elif this.modifier.similar.message_id and keyword_count:

      core_word_map = {}

      for msg in this.search_result.messages:    
        for word in msg.core_words:

          if word.id in core_word_map:
            core_word_map[word.id].weight += word.weight
          else:
            w = newsgate.search.MessageCoreWord()
            w.id = word.id
            w.text = word.text
            w.weight = word.weight
            core_word_map[word.id] = w

      if len(core_word_map):

        core_words = []
        for key, value in core_word_map.items(): core_words.append(value)
        core_words.sort(cmp_core_word)

        this.prn('\n<meta name="keywords" content="')

        index = 0
        for word in core_words:
          if index: this.prn(', ')
          this.prn(el.string.manip.xml_encode(word.text))

          index += 1
          if index == keyword_count: break

        this.prn('">')    

  def prn_body_open(this, onload = ""):
    this.prn(R'''
<body id="body"''', onload and (' onload="' + el.string.manip.xml_encode(onload) + '"') \
             or '', R'''>
<form id="post" style="display:none;" method="post"></form>''')

  def prn_body_close(this):

    if this.interceptor:
      body = this.interceptor.body()
      if body: this.prn('\n', body)
    
    this.prn('\n</body>')

  def prn_copyright(this, contacts = True):

    this.prn('\n<div id="copyright"')
    this.prn_notranslate_class()
    this.prn('>', this.static.copyright_note, " ", this.lm, "COPYRIGHT")

    bullet = False
    
    if contacts:
      this.prn('<br><a href="', this.site, '/p/c" target="_blank">', this.lm,
               "CONTACT", '</a>')
      
      bullet = True

    if this.interceptor:
      
      anchors = this.interceptor.footer_anchors()
      
      if anchors:
        for anc in anchors:

          this.prn(bullet and ' &#xB7;&nbsp;' or '<br>', anc)
          bullet = True

    this.prn('</div>')

    if this.translator:
      this.prn('<div id="tr_ref"><a href="',
               this.translator.service(this.translator.to_lang),
               '" target="_blank"><img src="',
               this.translator.logo(this.translator.to_lang),
               '" height="16"/></a></div>')
      

  def prn_counters(this):

    counters = ""
    
    if this.interceptor:

      counters = this.interceptor.counters(\
        this.modifier.category or this.filter.category or "/") or ""

    for cr in this.ad_counters: counters += "\n" + cr.text
      
    if counters:
      this.prn('\n<div id="counters">', counters, '</div>')

  def del_button(this, url, title):
    return '<span class="del_search_component"><a title="' + \
           (title and this.loc(title) or '') + '" href="' + \
           el.string.manip.xml_encode(this.make_ref(url, False)) + \
           '"><img width="12" height="12" src="' + \
           this.resource_url('/fixed/img/delete.png') + '"/></a></span>'

  def del_search_component(this, component, title):

    if this.crawler: return ''
    
    url = ""

    if component == "q": url = this.search_link(query = "")
    elif component == "v":
      mod = Modifier()
      mod.all = True
      url = this.search_link(modifier = mod)
    elif component == "h":
      fl = this.filter.clone()
      fl.event = Message()
      url = this.search_link(filter = fl)
    elif component == "f":
      fl = this.filter.clone()
      fl.feed = ""
      url = this.search_link(filter = fl)
    elif component == "g":
      fl = this.filter.clone()
      fl.category = ""
      url = this.search_link(filter = fl)
    elif component == "n":
      fl = this.filter.clone()
      fl.lang = el.Lang.null
      url = this.search_link(filter = fl, extra_params = "n=s-zzz")
    elif component == "y":
      fl = this.filter.clone()
      fl.country = el.Country.null
      url = this.search_link(filter = fl, extra_params = "y=s-ZZZ")

    return this.del_button(url, title)

  def tagline_header(this, text):
    if this.h1_tagline: return "<h1>" + text + "</h1>"
    else: return text
  
  def tagline(this,
              modifier = True,
              query = True,
              filter = True,
              lang_switchers = False):

    if modifier == True: modifier = this.modifier
    if query == True: query = this.query
    if filter == True: filter = this.filter    

    query_text = ""
    has_search_component = False

    if modifier.similar.message_id:
      
      query_text = \
        ' <span class="search_component">' + \
        this.loc("SIMILAR_TO") + " '" + \
        this.ad_section_start();

      translate = this.translator and \
        this.translator.to_lang != modifier.similar.lang

      if translate:
        query_text += '<span class="' + this.translator.translate_class() +'">'

      query_text += this.tagline_header(modifier.similar.title)
      
      if translate: query_text += '</span>'

      query_text += this.ad_section_end() + "'"

      if translate and this.static.main_languages and this.need_translation:
        query_text += '&nbsp;[&nbsp;' + this.loc("IN_LANGUAGE_TR") + \
          '&nbsp;]'

      query_text += \
        this.del_search_component("v", "EXCLUDE_SIMILAR") + '</span>'

      has_search_component = True
      
    elif modifier.source:

      query_text = \
        ' <span class="search_component">' + \
        this.loc("FROM_SOURCE") + \
        ' <a href="' + el.string.manip.xml_encode(modifier.source) + '">' + \
        this.tagline_header(el.string.manip.xml_encode(\
          truncate_text(modifier.source))) + \
        '</a>' + this.del_search_component("v", "EXCLUDE_SOURCE") + '</span>'

      has_search_component = True

    elif modifier.category:

      query_text = \
        ' <span class="search_component">' + this.loc("FROM_CATEGORY") + " " +\
        this.tagline_header(this.make_category_breadcrumb(modifier.category,
                                                          False)) + \
        this.del_search_component("v", "EXCLUDE_CATEGORY") + '</span>'

      has_search_component = True

    elif modifier.all:

      query_text = ' <span class="search_component">' + \
        this.tagline_header(this.loc("ALL_MSG")) + '</span>'

    elif query:

      query_text = ' <span class="search_component">' + \
        this.ad_section_start() + \
        this.tagline_header(el.string.manip.xml_encode(truncate_text(query)))+\
        this.ad_section_end() + \
        this.del_search_component("q", "EXCLUDE_SEARCH") + '</span>'

      has_search_component = True

    if filter.event.message_id:

      query_text += ' <span class="search_component">' + \
        this.loc("IN_SIMILAR") + \
        " '" + this.ad_section_start() + filter.event.title + \
        this.ad_section_end() + "'" + \
        this.del_search_component("h", "EXCLUDE_SIMILAR") + "</span>"

      has_search_component = True

    if filter.feed:

      query_text += ' <span class="search_component">' + \
        this.loc("IN_SOURCE") + \
        ' <a href="' + el.string.manip.xml_encode(filter.feed) + \
        '" target="_blank">' + el.string.manip.xml_encode(\
          truncate_text(filter.feed)) + \
        '</a>' + this.del_search_component("f", "EXCLUDE_SOURCE") + '</span>'

      has_search_component = True

    if filter.category:

      query_text += ' <span class="search_component">' + \
        this.loc("IN_CATEGORY") + \
        " " + this.make_category_breadcrumb(filter.category, True) + \
        this.del_search_component("g", "EXCLUDE_CATEGORY") + '</span>'
      
      has_search_component = True

    lang_filtered = False
    
    if filter.lang != el.Lang.null and not modifier.similar.event_id and \
       not filter.event.event_id:

      if query_text: query_text += ','
      
      query_text += ' <span class="search_component">' + \
        this.loc("IN_LANGUAGE") + " " + \
        el.string.manip.xml_encode(\
          this.localizer.language(filter.lang, this.language))

      if this.need_translation:
        query_text += "&nbsp;[&nbsp;" + this.loc("IN_LANGUAGE_TR") + "&nbsp;]"
      
      has_search_component = True
      lang_filtered = True

    if lang_switchers and this.static.main_languages and \
       (not this.crawler or filter.country == el.Country.null) and \
       not this.event_message_id():
      
      filter_lang_l3_code = filter.lang.l3_code()
    
      if filter_lang_l3_code not in this.static.main_languages or \
         len(this.static.main_languages) > 1:
      
        if not lang_filtered:        
          if query_text: query_text += ', '
          query_text += this.loc("ALL_LANGUAGE")

          if this.need_translation:
            query_text += "&nbsp;[&nbsp;" + this.loc("IN_LANGUAGE_TR") + \
                          "&nbsp;]"

        langs = []
        for lang in this.static.main_languages:

          if lang == filter_lang_l3_code or lang not in this.lang_counter:
            continue            

          langs.append((lang,
                        this.localizer.language(el.Lang(lang), this.language)))
          
        langs.sort(cmp_langs)
        lang_switch = ""
        
        for lang in langs:
          if lang_switch: lang_switch += ", "
          else: lang_switch = ' <span class="languages">(&nbsp;'

          lcode = lang[0]

          if this.crawler:
            fl = filter.clone()
            fl.lang = el.Lang(lcode)
            extra_params = ""
          else:
            fl = filter.clone()
            fl.lang = el.Lang.null
            extra_params = "n=s-" + lcode

          translator = \
            this.translator and this.translator.to_lang.l3_code() != lcode
            
          url = \
            this.search_link(\
              filter = fl,
              extra_params = extra_params,
              translator = translator)
            
          lang_switch += '<a class="watered_link" href="' + \
            el.string.manip.xml_encode(this.make_ref(url, False)) + '">' + \
            el.string.manip.xml_encode(lang[1]) + '</a>'
            
          if not this.crawler and not this.translator and \
             this.default_translator and \
             this.default_translator.to_lang.l3_code() != lcode and \
             lcode in this.default_translator.languages:

            url = this.default_translator.enrich_link(url)
               
            lang_switch += '&nbsp;[&nbsp;<a class="watered_link" href="' + \
              el.string.manip.xml_encode(this.make_ref(url, False)) + '">' +\
              this.loc("IN_LANGUAGE_TR") + '</a>&nbsp;]'

        if lang_switch:
          lang_switch += "&nbsp;)</span>"
          query_text += lang_switch

    if lang_filtered:
      query_text += this.del_search_component("n", "EXCLUDE_LANG") + '</span>'

    country_filtered = False

    if filter.country != el.Country.null:
  
      if query_text: query_text += ','

      query_text += ' <span class="search_component">' + \
        this.loc("IN_COUNTRY") + " " + \
        el.string.manip.xml_encode(\
          this.localizer.country(filter.country, this.language))

      has_search_component = True
      country_filtered= True

    if lang_switchers and this.country_filters:

      filter_country_l3_code = filter.country.l3_code()
    
      if filter_country_l3_code not in this.country_filters or \
         len(this.country_filters) > 1:
        
        if not country_filtered:
          if query_text: query_text += ', '
          query_text += this.loc("ALL_COUNTRY")

        countries = []
        for country in this.country_filters:

          if country == filter_country_l3_code or \
            country not in this.country_counter:
            continue            

          countries.append((country,
                            this.localizer.country(el.Country(country), 
                                                   this.language)))

        country_switch = ""

        for country in countries:
          if country_switch: country_switch += ", "
          else: country_switch = ' <span class="countries">(&nbsp;'

          ccode = country[0]

          if this.crawler:
            fl = filter.clone()
            fl.country = el.Country(ccode)
            extra_params = ""
          else:
            fl = filter.clone()
            fl.country = el.Country.null
            extra_params = "y=s-" + ccode

          url = \
            this.search_link(\
              filter = fl,
              extra_params = extra_params)
            
          country_switch += '<a class="watered_link" href="' + \
            el.string.manip.xml_encode(this.make_ref(url, False)) + '">' + \
            el.string.manip.xml_encode(country[1]) + '</a>'            

        if country_switch:
          country_switch += "&nbsp;)</span>"
          query_text += country_switch


    if country_filtered:
      query_text += \
        this.del_search_component("y", "EXCLUDE_COUNTRY") + '</span>'

    if has_search_component and this.crawler:

      if this.static.main_languages:

#        fl = Filter()
#        fl.lang = this.filter.lang
#        url = this.search_link(modifier = False, filter = fl)
        
#        if url == this.site + this.context.request.unparsed_uri():
#          url = this.search_link(modifier = False, filter = Filter())

        fl = this.filter.clone()
        fl.lang = el.Lang.null

        mod = this.modifier.clone()
        mod.all = False

        if not mod.empty() or not fl.empty():
          fl = Filter()
          fl.lang = this.filter.lang
          
        url = this.search_link(modifier = False, filter = fl)
          
      else:
        url = this.service
        
      if url: query_text += this.del_button(url, "")

    return query_text

  def prn_final_script(this):
    
    if len(this.anchor_refs) > 0:

      this.prn(R'''
<script type="text/javascript">
// ''', len(this.anchor_refs), R''' links
post_refs = new Array();''')

      for ref in this.anchor_refs:
        this.prn("\npost_refs.push('", el.string.manip.js_escape(ref), "');")

      this.prn('\n</script>')

  def make_category_breadcrumb(this, cat_path, search_in_cat):

    if this.search_result == None: return ""

    if cat_path == "/": cat_path = [ "" ]
    else:
      if cat_path[0] != "/": cat_path = "/" + cat_path
      if cat_path[-1] == "/": cat_path = cat_path[0:-1]
      cat_path = cat_path.split("/")

    name_count = len(cat_path)

    modifier = this.modifier
    filter = this.filter

    if search_in_cat:
      filter = filter.clone()
      filter.category = ""
      
    else:
      modifier = modifier.clone()
      modifier.category = ""

    translate_breadcrump = this.translator and \
      (this.translator.to_lang != this.locale.lang or \
       this.locale.lang.l3_code() not in this.static.main_languages)
    
    breadcrumb = '<span class="breadcrumb'
    
    if translate_breadcrump:
      breadcrumb += " " + this.translator.translate_class()
      
    breadcrumb += '">'
    path = ""
    
    for name in cat_path:

      if search_in_cat:
        filter.category += name + "/"
      else:
        modifier.category += name + "/"

      path += name
      
      name = el.string.manip.xml_encode(name)
      name_count -= 1

      cat = this.search_result.category_stat.find(path)

      localized_name = this.ad_section_start() + \
        (cat == None and name or \
         el.string.manip.xml_encode(cat.localized_name.encode("utf-8"))) + \
        this.ad_section_end()

      if name_count:

        if path != "":

          localized_path = this.translate_category_name(path)

          title = (search_in_cat and \
                   this.loc("CATEGORY_BAR_SEARCH_TITLE") or\
                   this.loc("CATEGORY_BAR_MOVE_TITLE")) + " " + localized_path

          link = this.search_link(modifier = modifier, filter = filter)

          breadcrumb += '<a href="' + \
            el.string.manip.xml_encode(this.make_ref(link, False)) + \
            '" title="' + title + '">' + localized_name + \
            '</a>' + (translate_breadcrump and \
            '<span class="notranslate"> / </span>' or ' / ')

          path += "/" 
      else:
        breadcrumb += name == "" and "/" or localized_name

    breadcrumb += "</span>"

    return breadcrumb

  def compile_spam_patterns(this, conf):

    patterns = []
    
    for n in range(1,100):

      pattern = conf("spam_pattern_" + str(n))
      if not pattern: break

      patterns.append(re.compile(pattern))

    return patterns

  def is_spam(this, text):
    
    for pattern in this.static.spam_patterns:
      if pattern.match(text): return True

    return False

  def save_file(this, text, type, send, delete_sent):

    spam_text = this.is_spam(text)
    date = el.Moment(el.gettimeofday()).dense_format()

    if spam_text:
      file_path = this.static.data_root + "/Feedback/Spam/" + type + "." + date
    else:
      file_path = this.static.data_root + "/Feedback/" + type + "." + date    
    
    request = this.context.request
    
    file = open(file_path, "w")

    file.writelines(\
      [ text,
        "\n\nIP: ", request.remote_ip(), 
        "\nCountry: ", 
        this.static.address_info.country(request.remote_ip()).description(), 
        "\nURI: ", request.unparsed_uri(), "\n\nHeaders:\n" ])

    for h in request.input.headers():
      file.writelines([h.name, ": ", h.value, "\n"])

    file.writelines( [ "\nParams:\n" ] )

    for p in request.input.parameters():
      file.writelines([p.name, ": ", p.value, "\n"])

    file.close()
  
    if not spam_text and send and this.static.email:

      msg = email.Message.Message()
      
      charset = email.Charset.Charset("utf-8")
#      charset.header_encoding = email.Charset.BASE64
      charset.body_encoding = email.Charset.BASE64
      msg.set_charset(charset)

      file = open(file_path, "r")
      msg.set_payload(base64.b64encode(file.read()))
      file.close()
   
      pos = text.find('\n')

      msg['Subject'] = \
        truncate_text(pos >= 0 and text[0:pos] or text) + \
        " (" + date + ")"

      from_addr = type + "." + this.static.email
      msg['From'] = from_addr

      msg['To'] = this.static.email

      server = smtplib.SMTP('localhost')

      server.sendmail(from_addr, 
                      [ this.static.email ],
                      msg.as_string())
 
      server.quit()

      if delete_sent: os.unlink(file_path)

    return not spam_text
