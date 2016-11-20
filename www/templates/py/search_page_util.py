import el
import newsgate
import search_util
import informer_util

class SearchPageRender(search_util.SearchPageContext,
                       informer_util.InformerRender):

  def __init__(this, context, search_engine, protocol, informer):

    this.base_render_type = informer and informer_util.InformerRender or \
                            search_util.SearchPageContext
    
    this.base_render_type.__init__(this,
                                   context,
                                   search_engine,
                                   protocol,
                                   True)

    conf = context.config.get

    try:
      tmp = this.static.debug
    except:
      this.static.debug = conf("debug") == "1"
    
    this.search_hint = ""
    this.highlight = True
    this.relax_search_level = 0
    this.require_debug_info = False
    this.show_segmentation = False

    if informer:
      this.start_from = 0
    else:

      if this.prefs != None and this.search_engine_refered == False:
        
        this.require_debug_info = this.sticky_switch("d")
        this.show_segmentation = this.sticky_switch("m")
        this.extra_msg_info = this.sticky_switch("emi")

      if this.sorting == \
         newsgate.search.SearchContext.SM_BY_RELEVANCE_DESC:

        this.search_hint = \
          this.search_engine_refered == False and this.raw_param("st", None) \
          or None

        if this.search_hint == None:
          this.search_hint = \
            this.search_info != None and this.search_info.query or ''

      this.highlight = int(this.raw_param("nh", 0)) != 1
      this.relax_search_level = int(this.raw_param("rl", 0))
      if abs(this.relax_search_level) > 5: this.exit(400) #bad request

    this.informer = informer

    this.moderation_url = this.raw_param("mod_url")
    this.mod_proxy_url = this.raw_param("mod_proxy_url")
    this.moderation_css = this.raw_param("mod_css")
    this.moderation_base_css = this.raw_param("mod_base_css")
    this.moderation_script = this.raw_param("mod_script")
    this.moderation_base_script = this.raw_param("mod_base_script")
    this.moderation_init = this.raw_param("mod_init")
    this.moderation_params = this.raw_param("mod_params")
    this.moderation_menu = this.raw_param("mod_menu")

    this.moderation_options = ""

    if this.moderation_url:
      this.endpoint = this.static.canonoical_endpoint
      this.site = 'http://' + this.endpoint
      this.service = this.moderation_url

      this.moderation_options = "mod_url=" + \
        el.string.manip.mime_url_encode(this.moderation_url)

      this.cookie_num = 0
      
    else:
      this.msg_url = this.site + '/p/s/m'      

    if this.moderation_css:
      if this.moderation_options: this.moderation_options += "&"
      
      this.moderation_options += "mod_css=" + \
        el.string.manip.mime_url_encode(this.moderation_css)

    if this.moderation_base_css:
      if this.moderation_options: this.moderation_options += "&"
      
      this.moderation_options += "mod_base_css=" + \
        el.string.manip.mime_url_encode(this.moderation_base_css)

    if this.moderation_base_css:
      if this.moderation_options: this.moderation_options += "&"
      
      this.moderation_options += "mod_base_css=" + \
        el.string.manip.mime_url_encode(this.moderation_base_css)
  
    if this.moderation_script: 
      if this.moderation_options: this.moderation_options += "&"
      
      this.moderation_options += "mod_script=" + \
        el.string.manip.mime_url_encode(this.moderation_script)

    if this.moderation_base_script:
      if this.moderation_options: this.moderation_options += "&"
      
      this.moderation_options += "mod_base_script=" + \
        el.string.manip.mime_url_encode(this.moderation_base_script)

    if this.moderation_init != "": 
      if this.moderation_options: this.moderation_options += "&"

      this.moderation_options += "mod_init=" + \
        el.string.manip.mime_url_encode(this.moderation_init)

    if this.moderation_menu:
      if this.moderation_options: this.moderation_options += "&"
      
      this.moderation_options += "mod_menu=" + \
        el.string.manip.mime_url_encode(this.moderation_menu)

    this.paging_panel = ""
    this.new_search_link = None

    if this.query and this.relax_search_level:
      
      try:

        if this.relax_search_level > 0:
        
          this.query = search_engine.relax_query(this.query,
                                                 this.relax_search_level)
        else:

          this.query = "( " + \
            search_engine.relax_query(this.query,
                                      -this.relax_search_level) + \
            " ) EXCEPT ( " + \
            search_engine.relax_query(this.query,
                                      -this.relax_search_level - 1) + " )"
     
      except SyntaxError:
        this.exit(400) #bad request

  def sticky_switch(this, param):

    value = this.prefs.value(param) == "1"
    prm = this.raw_param(param, None)
        
    if prm != None:
      value = prm == "1"
      this.prefs_updated = True
      this.prefs_updated_params[param] = 1

    if value == True: this.prefs[param] = "1"
    else: del this.prefs[param]
    
    return value

  def resource_url(this, path):
    if this.mod_proxy_url == "": return this.site + path

    return this.mod_proxy_url + "?m=POST&p=" + \
           el.string.manip.mime_url_encode(path) + "&h=" + \
           el.string.manip.mime_url_encode(\
             "Content-type:application/x-www-form-urlencoded")    

  def send_cookie(this, cookie_setter):
    if this.moderation_url:
      this.context.request.output.send_header(\
        "Search-SC-" + str(this.cookie_num),
        cookie_setter.string())
      this.cookie_num += 1
    else:
      this.context.request.output.send_cookie(cookie_setter)

  def search_context(this):
    ctx = this.base_render_type.search_context(this)
    ctx.search_hint = this.search_hint
    ctx.highlight = this.highlight
    return ctx

  def search(this, search_context):
    
    res = this.base_render_type.search(this, search_context)
    
    if res:

      if this.translator:
        if this.translator.to_lang != this.locale.lang or \
           this.locale.lang.l3_code() not in this.static.main_languages:
          this.need_translation = True
        else:
          for msg in res.messages:
            if msg.lang != this.translator.to_lang:
              this.need_translation = True
              break

      elif this.default_translator:

        if this.default_translator.to_lang != this.locale.lang or \
           this.locale.lang.l3_code() not in this.static.main_languages:
          this.can_translate = True          
        elif search_context.gm_flags & newsgate.search.SearchContext.GM_LANG:
          for msg in res.messages:
            if msg.lang != this.default_translator.to_lang:
              this.can_translate = True
              break
      
    return res

  def prn_css(this):

    if this.static.development or this.moderation_url:
      
      this.base_render_type.prn_css(this)
      
      this.prn(\
        '\n<link rel="stylesheet" type="text/css" href="',
        this.resource_url('/fixed/css/search.css'),
        '" media="all"/>')

      if this.site_version == search_util.SearchPageContext.SV_MOB:
        this.prn(\
          '\n<link rel="stylesheet" type="text/css" href="',
          this.resource_url('/fixed/css/search-mob.css'),
          '" media="all"/>')
        
    else:

      if this.site_version == search_util.SearchPageContext.SV_MOB:
        this.prn(\
          '\n<link rel="stylesheet" type="text/css" href="',
          this.compound_resource_url('css/search-mob.css'),
          '" media="all"/>')
        
      else:
        this.prn(\
          '\n<link rel="stylesheet" type="text/css" href="',
          this.compound_resource_url('css/search.css'),
          '" media="all"/>')

    if this.moderation_base_css != "":
      this.prn('\n<link rel="stylesheet" type="text/css" href="',
               el.string.manip.xml_encode(this.moderation_base_css),
               '" media="all"/>')

    if this.moderation_css != "":
      this.prn('\n<link rel="stylesheet" type="text/css" href="',
               el.string.manip.xml_encode(this.moderation_css),
               '" media="all"/>')

  def prn_script(this,
                 search_path = "",
                 search_query_extra_params = "",
                 view_options = True):

    this.base_render_type.prn_script(\
      this,
      this.moderation_url or this.service,
      this.search_hint and \
        ("st=" + el.string.manip.mime_url_encode(this.search_hint)) or '',
      view_options)

    this.prn('\n<script src="',
             this.resource_url('/tsp/js/' + this.language_l3_code +
                               '/search.js'),
             '" type="text/javascript"></script>')

    if this.moderation_base_script != "":
      this.prn('\n<script src="',
               el.string.manip.xml_encode(this.moderation_base_script), 
               '" type="text/javascript"></script>')

    if this.moderation_script != "":
      this.prn('\n<script src="',
               el.string.manip.xml_encode(this.moderation_script),
               '" type="text/javascript"></script>')

    if this.moderation_menu:
      this.prn(R'''
<script type="text/javascript">
search_menu = "mod_menu=''',
               el.string.manip.js_escape(this.moderation_menu), R'''";
</script>''')

  def prn_counters(this):

    if this.moderation_url == "": this.base_render_type.prn_counters(this)

  def prn_share_page(this):
    pass

  def prn_search_bar(this, error_code, part):

    result_from = this.start_from + 1
    
    sources = ""
    query_text = this.tagline(lang_switchers = True)
      
    if this.search_result == None:
      messages_count = 0
      total_matched_messages = 0
      suppressed_messages = 0
      
    else:
      total_matched_messages = this.search_result.total_matched_messages
      suppressed_messages = this.search_result.suppressed_messages    
      messages_count = this.search_result.messages.size()

    if total_matched_messages > 0:
      if messages_count > 0:

        if this.crawler and this.brief_text_for_crawler:
          page_heading = this.get_template("CRAWLER_RESULTS_HEADING")
          page_heading_vars = { "QUERY_IN_PAGE_TOP_BAR":query_text }
          
        else:
          page_heading = this.get_template("RESULTS_HEADING")
          total_results = str(total_matched_messages)

          if not this.informer_create_mode:

            if suppressed_messages:

              if this.crawler == None:
              
                suppressed_info = this.get_template("SUPPRESSED_INFO")

                suppressed_info_vars = \
                { "SUPPRESSED_RESULTS":suppressed_messages,
                  "ENABLE_DUPS_QUERY": 
                    el.string.manip.xml_encode(\
                      this.make_ref(this.search_link(extra_params = 'b=s-0'),
                                    False))
                }

                total_results +=\
                  suppressed_info.instantiate(suppressed_info_vars)
  
            elif this.suppression == newsgate.search.SearchContext.ST_NONE:
 
              disable_dups = this.get_template("DISABLE_DUPS")
  
              disable_dups_vars = \
              { 
                "DISABLE_DUPS_QUERY": 
                    el.string.manip.xml_encode(\
                      this.make_ref(this.search_link(extra_params = 'b=s-3'),
                                    False))
              }
 
              total_results += disable_dups.instantiate(disable_dups_vars)

          if this.crawler == None and this.modifier.source == "" and \
             this.filter.feed == "":
            sources = '&#xA0;<span class="sources">\
(<a class="watered_link" href="javascript:show_paging_dialog(\'feed\', page.extra_params ? page.extra_params() : \'\')" title="'+\
                      this.loc("SOURCES_TITLE") + '">' + this.loc("SOURCES") +\
                      '</a>)</span>'

          page_heading_vars = { "RESULT_FROM":result_from, 
                                "RESULT_TO":this.start_from + messages_count, 
                                "TOTAL_RESULTS":total_results,
                                "QUERY_IN_PAGE_TOP_BAR":query_text
                              }

      else:
        page_heading = this.get_template("NO_MORE_MESSAGE_HEADING")

        page_heading_vars = { "QUERY_IN_PAGE_TOP_BAR":query_text }
    else:
      if error_code < 0:
        page_heading = this.get_template("NO_RESULTS_HEADING")

        page_heading_vars = { "QUERY_IN_PAGE_TOP_BAR":query_text }
      else:
        page_heading = this.get_template("EXPRESSION_ERROR")

        page_heading_vars = \
          { "ERROR_DESC" : this.loc("PARSE_ERROR_" + str(error_code)) }

    if this.site_version == search_util.SearchPageContext.SV_MOB:
      if part == 0:
        this.prn('<div class="page_title">', page_heading, page_heading_vars,
                 sources, '</div>')
      else:
        this.prn(R'''
        <table class="page_title_search_table" cellspacing="0">
        <tr>
          <td><form onsubmit="search(false); return false;">
              <input id="se" type="search" autocomplete="off" autocorrect="off"
                 onkeypress="return search_key_press(event)" value="''', 
            this.crawler == None and \
             el.string.manip.xml_encode(\
               this.query.replace('\n', ' ').replace('\r', ' ').\
               replace('\t', ' ')) or '', R'''"/></form>
          </td>
          <td><input type="button" value="''', this.lm, "SEARCH_BUTTON", 
                 R'''" onclick="search(false);"/>
          </td>
          <td><a href="/p/h/s" target="_blank" title="''', this.lm,
             "SEARCH_HELP", R'''">?</a>
          </td>
        </tr>
        </table>''')
        
    else:
      this.prn(R'''
  <tr>
  <td colspan="4">
    <table class="page_title_table" cellspacing="0">
    <tr>
      <td class="page_title"''', 
      str(error_code >= 0 and ' style="text-align:right;"' or ''), R'''>
  ''', page_heading, page_heading_vars, sources, R'''
      </td>
      <td class="page_title_search">
        <table class="page_title_search_table" cellspacing="0">
        <tr>
          <td><input id="se" type="text"
                 onkeypress="return search_key_press(event)" value="''', 
            this.crawler == None and \
             el.string.manip.xml_encode(\
               this.query.replace('\n', ' ').replace('\r', ' ').\
               replace('\t', ' ')) or '', R'''"/>
          </td>
          <td><input type="button" value="''', this.lm, "SEARCH_BUTTON", 
               R'''" onclick="search(false);"/>
          </td>
          <td><a href="''', this.site, R'''/p/h/s" target="_blank" title="''',
               this.lm,
               "SEARCH_HELP", R'''">?</a>
          </td>
        </tr>
        </table>
      </td>
    </tr>
    </table>
  </td>
  </tr>''')

  def prn_subcat_bar(this):

    if this.crawler == None and this.search_result != None:
    
      category_path = this.modifier.category or this.filter.category or "/"
      category = this.search_result.category_stat.find(category_path)
      first = True

      cats = category and category.categories or []
      for cat in cats:

        if cat.matched_message_count == 0: continue
    
        cat_localized_name = cat.localized_name.encode("utf-8")
        if cat_localized_name == "": continue

        encoded_name = el.string.manip.xml_encode(cat_localized_name)
        cat_name = cat.name.encode("utf-8")

        cat_path = category_path + \
          (category_path[-1] != "/" and "/" or "") + cat_name

        title = (this.modifier.category and \
                 this.loc("CATEGORY_BAR_MOVE_TITLE") or\
                 this.loc("CATEGORY_BAR_SEARCH_TITLE")) + " " + \
            el.string.manip.xml_encode(this.translate_category_name(cat_path))
        
        if this.modifier.category:
          mod = this.modifier.clone()
          mod.category = cat_path
          cat_url = this.search_link(modifier = mod)
        else:
          fl = this.filter.clone()
          fl.category = cat_path
          cat_url = this.search_link(filter = fl)

        if first:
          first = False

          if this.site_version == search_util.SearchPageContext.SV_MOB:
            this.prn('<div class="subcategory_bar">')
            
          else:
            
            this.prn(R'''
  <tr>
  <td colspan="4" class="subcategory_bar print_''',
                   (this.large_print and "l" or "s"), '">')
        else:
          this.prn("&#xA0;&#xB7; ")

        classname = "bar_category"
        
        if this.translator and \
             (this.translator.to_lang != this.locale.lang or \
              this.locale.lang.l3_code() not in this.static.main_languages):
            classname += " " + this.translator.translate_class()
            
        this.prn('<a class="', classname, '" href="',
          el.string.manip.xml_encode(this.make_ref(cat_url, False)),
          '" title="', title, '">',
          el.string.manip.xml_encode(cat_localized_name),
          '</a>&#xA0;<span class="cat_msg_count')
        
        this.prn_notranslate_class()

        this.prn('">(',
          cat.matched_message_count, ')</span>')
        
      if first == False:
        if this.site_version == search_util.SearchPageContext.SV_MOB:
          this.prn('</div>')
        else:
          this.prn(R'''
  </td>
  </tr>''')

  def prn_page_sharing(this):
        
    this.prn(R'''
  <td class="page_sharing_bar">''')

    this.prn_share_page()
    
    this.prn(R'''
  </td>
  <td class="custom_sharing_bar">''')
    
    if this.interceptor and this.informer_create_mode == False and \
       this.moderation_url == "":
      share_page = this.interceptor.share_page()
      if share_page != None: this.prn(share_page)

    this.prn('  </td>')
        
  
  def prn_top_bar(this, error_code):

    if this.site_version == search_util.SearchPageContext.SV_MOB:
      this.prn(R'''
<script type="text/javascript">
page.search_bar = { tagline:"''', el.psp.JS_EscapeMarker)
      
      this.prn_search_bar(error_code, 0)
      
      this.prn(el.psp.NoJS_EscapeMarker, '", controls:"',
               el.psp.JS_EscapeMarker)

      this.prn_search_bar(error_code, 1)

      this.prn(el.psp.NoJS_EscapeMarker, '", subcats:"',
               el.psp.JS_EscapeMarker)

      this.prn_subcat_bar()
      
      this.prn(el.psp.NoJS_EscapeMarker, '"};\n</script>')

    this.prn('\n<table id="top_bar" cellspacing="0"')
    this.prn_notranslate_class()
    this.prn('>')

    if this.site_version != search_util.SearchPageContext.SV_MOB:
      this.prn_search_bar(error_code, 0)
      this.prn_subcat_bar()

    if this.crawler == None and this.informer == False:
      this.paging_panel = this.create_paging_panel()

    this.prn('\n  <tr>')

    if this.paging_panel or \
       this.site_version != search_util.SearchPageContext.SV_MOB:
      
      this.prn('\n  <td class="page_navigation_bar">''', this.paging_panel,
               '</td>')

    if this.site_version != search_util.SearchPageContext.SV_MOB:
      this.prn_page_sharing()
    
    this.prn('\n  <td class="page_menu">')
    
    if this.search_result != None:

      menu_pos = 0
      first = True
      
      if this.moderation_url == "":

        if this.interceptor:

          first = \
            this.prn_menu(this.interceptor.top_bar_menu(menu_pos, "home"),
                          first)
          menu_pos += 1

        link = this.translate_link(this.site + "/")
        
        this.new_search_link = '<a href="' + \
          el.string.manip.xml_encode(link) + '">'

        if this.site_version == search_util.SearchPageContext.SV_MOB:
          this.new_search_link += '<img src="' + \
            this.resource_url('/fixed/img/home32.png') + \
            '" width="32" height="32" class="mob_menu_img"/>'
        else:
          this.new_search_link += this.loc("NEW_SEARCH")

        this.new_search_link += '</a>'

        first = this.prn_menu(this.new_search_link, first)

      if not this.crawler and (this.need_translation or this.can_translate):

        if this.interceptor:

          first = \
            this.prn_menu(\
              this.interceptor.top_bar_menu(\
                menu_pos, 
                this.can_translate and "translate" or "original"), 
                            first)
          menu_pos += 1

        trans_extra = this.start_from and \
          ('s=' + str(this.start_from + 1)) or ''            

        link = this.search_link(\
          translator = this.can_translate and this.default_translator,
          extra_params = trans_extra)

        text = '<a href="' + el.string.manip.xml_encode(\
                this.make_ref(link, False)) + '">'
        
        if this.site_version == search_util.SearchPageContext.SV_MOB:
          text += '<img src="' + \
            this.resource_url(\
              '/fixed/img/' + \
              (this.can_translate and "translate" or "original") + \
              '32.png') + '" width="32" height="32" class="mob_menu_img"/>'
          separator = ""
        else:
          text += this.loc(this.translator and "ORIGINAL" or "TRANSLATE")
          separator = " | "

        text += '</a>'
        
        first = this.prn_menu(text, first, separator)

      if not this.crawler and not this.informer and \
         this.site_version != search_util.SearchPageContext.SV_MOB:

        nline = this.message_view == "nline"
        
        if this.interceptor:

          first = \
            this.prn_menu(\
              this.interceptor.top_bar_menu(\
                menu_pos, 
                nline and "nline" or "paper"), 
                first)
          
          menu_pos += 1

        extra_params = "mvw=s-" + (nline and "paper&r=s-10" or "nline&r=s-50")

        link = this.search_link(extra_params = extra_params)

        text = '<a href="' + el.string.manip.xml_encode(\
                this.make_ref(link, False)) + '">'
        
        if this.site_version == search_util.SearchPageContext.SV_MOB:
          text += ''
          separator = ""
        else:
          text += this.loc(nline and "MV_PAPER" or "MV_LINE")
          separator = " | "

        text += '</a>'
        
        first = this.prn_menu(text, first, separator)

      if this.moderation_url == "":

        if this.informer:

          if this.site_version != search_util.SearchPageContext.SV_MOB:
          
            link = this.make_ref(this.search_link(path = '/p/s/h'),
                                 False,
                                 False,
                                 False)
          
            text = this.loc("SEARCH")
            menu_id = "search"
            
          else:
            link = ""          
          
        elif this.static.informer_enable and \
             this.site_version != search_util.SearchPageContext.SV_MOB:

          link = this.make_ref(this.crawler and (this.site + '/p/s/i') or\
                                 this.search_link(path = '/p/s/i'),
                               False)
          
          text = this.loc("INFORMER")
          menu_id = "informer"
          
        else:
          link = ""
          
        if link:

          if this.interceptor:

            first = \
              this.prn_menu(this.interceptor.top_bar_menu(menu_pos, menu_id),
                            first)
            menu_pos += 1

          first = \
              this.prn_menu('<a href="' + el.string.manip.xml_encode(link) + \
                              '">' + text + '</a>',
                            first)
          
      if this.crawler == None:

        if this.site_version == search_util.SearchPageContext.SV_MOB:
          
          if this.interceptor:

            first = \
              this.prn_menu(this.interceptor.top_bar_menu(menu_pos, "search"),
                            first)
            menu_pos += 1

          text = '\
<a href="javascript:show_search_dialog(\
page.extra_params ? page.extra_params() : \'\')"><img src="' + \
                this.resource_url('/fixed/img/search32.png') + \
                '" width="32" height="32" class="mob_menu_img"/></a>'

          first = this.prn_menu(text, first, "")

          if this.interceptor:

            first = \
              this.prn_menu(this.interceptor.top_bar_menu(menu_pos,
                                                          "sources"),
                            first)
            menu_pos += 1

          text = '\
<a href="javascript:show_paging_dialog(\'feed\', \
page.extra_params ? page.extra_params() : \'\')"><img src="' + \
                this.resource_url('/fixed/img/sources32.png') + \
                '" width="32" height="32" class="mob_menu_img"/></a>'

          first = this.prn_menu(text, first, "")        

          if this.interceptor:

            first = \
              this.prn_menu(this.interceptor.top_bar_menu(menu_pos,
                                                          "categories"),
                            first)
            menu_pos += 1

          text = '\
<a href="javascript:show_categories_dialog(\
page.extra_params ? page.extra_params() : \'\')"><img src="' + \
                this.resource_url('/fixed/img/categories32.png') + \
                '" width="32" height="32" class="mob_menu_img"/></a>'

          first = this.prn_menu(text, first, "")        

        if this.interceptor:

          first = \
            this.prn_menu(this.interceptor.top_bar_menu(menu_pos, "settings"),
                          first)
          menu_pos += 1

        text = '<a href="javascript:show_settings_dialog(page.extra_params ? page.extra_params() : \'\')">'
        
        if this.site_version == search_util.SearchPageContext.SV_MOB:
          text += '<img src="' + \
            this.resource_url('/fixed/img/settings32.png') + \
            '" width="32" height="32" class="mob_menu_img"/>'
          separator = ""
        else:
          text += this.loc("SETTINGS")
          separator = " | "

        text += '</a>'

        first = this.prn_menu(text, first, separator)

      if (this.moderation_url or this.extra_msg_info) and \
        this.site_version == search_util.SearchPageContext.SV_DESK:

        start_param = this.start_from and \
                      ('s=' + str(this.start_from + 1) + '&') or ''
        
        link = el.string.manip.xml_encode(this.make_ref(
          this.search_link(\
            extra_params = start_param + 'emi=' + \
                           (this.extra_msg_info and "0" or "1")),
          False))

        if this.interceptor:

          first = \
            this.prn_menu(this.interceptor.top_bar_menu(menu_pos, "emi"),
                          first)
          menu_pos += 1

        first = \
          this.prn_menu(\
            '<a href="' + link + '">' + \
              (this.extra_msg_info and 'Emi&nbsp;OFF' or 'Emi&nbsp;ON') + \
              '</a>',
            first)            

      if (this.static.debug or this.require_debug_info or \
          this.show_segmentation) and \
        this.site_version == search_util.SearchPageContext.SV_DESK:

        start_param = this.start_from and \
                      ('s=' + str(this.start_from + 1) + '&') or ''
        
        link = el.string.manip.xml_encode(this.make_ref(
          this.search_link(\
            extra_params = start_param + 'd=' + \
                           (this.require_debug_info and "0" or "1")),
          False))

        if this.interceptor:

          first = \
            this.prn_menu(this.interceptor.top_bar_menu(menu_pos, "dbg"),
                          first)
          menu_pos += 1

        first = \
          this.prn_menu(\
            '<a href="' + link + '">' + \
              (this.require_debug_info and 'Db&nbsp;OFF' or 'Db&nbsp;ON') + \
              '</a>',
            first)   
            
        if this.interceptor:

          first = \
            this.prn_menu(this.interceptor.top_bar_menu(menu_pos, "seg"),
                          first)
          menu_pos += 1

        link = el.string.manip.xml_encode(this.make_ref(
          this.search_link(\
            extra_params = start_param + 'm=' + \
                           (this.show_segmentation and "0" or "1")),
          False))

        first = \
          this.prn_menu(\
            '<a href="' + link + '">' + \
              (this.show_segmentation and 'Sg&nbsp;OFF' or 'Sg&nbsp;ON') + \
              '</a>',
            first)
            
      if this.interceptor:

        first = \
          this.prn_menu(this.interceptor.top_bar_menu(menu_pos, "end"),
                        first)

        page_marker = this.interceptor.page_marker()
        if page_marker: this.prn(page_marker)

    this.prn(R'''
  </td>
  </tr>
</table>''')

  def paging_extra_params(this, start = None):
    if start == None: start = this.start_from + 1
    param = start > 1 and ("s=" + str(start)) or ''

    if this.highlight == False:
      if param: param += "&"
      param += "nh=1"
    
    if this.search_hint:
      if param: param += "&"
      param += "st=" + el.string.manip.mime_url_encode(this.search_hint)

    return param
    
  def create_paging_panel(this):

    results = this.search_result and \
              min(this.search_result.total_matched_messages, \
                  this.static.max_results) or 0
    
    if results == 0: return ""

    if this.results_per_page == 0: total_pages = 0
    else:    
      total_pages = results / this.results_per_page
      if results % this.results_per_page: total_pages += 1

    if total_pages < 2 and \
       this.site_version == search_util.SearchPageContext.SV_MOB:
      return ""

    text = ""
  
    if this.search_result.messages.size():
      if this.results_per_page == 0: current_page = 0
      else: current_page = this.start_from / this.results_per_page
    else: current_page = total_pages

    page_links = this.site_version == search_util.SearchPageContext.SV_MOB and\
                 1 or 10
    
    shift = page_links / 2

    if current_page < shift: start_page_number = 0
    else: start_page_number = current_page - shift

    last_page_number = start_page_number + page_links

    if last_page_number > total_pages: last_page_number = total_pages

    if last_page_number - start_page_number < page_links:
      if last_page_number > page_links:
        start_page_number = last_page_number - page_links
      else: start_page_number = 0

    if this.site_version != search_util.SearchPageContext.SV_MOB:
      text += this.loc("SEARCH_PAGE_LINKS_LABEL") + ' '

    if current_page:

      start = (current_page - 1) * this.results_per_page + 1
      
      link = el.string.manip.xml_encode(this.make_ref(\
        this.search_link(extra_params = this.paging_extra_params(start)),
        False))
      
      text += '<a href="' + link + '" class="page_prev">' + \
              this.loc("PREV_PAGE_LINK") + '</a> '

    for page_num in range(start_page_number, last_page_number):

      if page_num > start_page_number: text += " "

      if page_num == current_page:
        text += '<span class="page_current_number">' + str(page_num + 1) + \
                '</span>'
      else:

        start = page_num * this.results_per_page + 1
        
        link = el.string.manip.xml_encode(this.make_ref(\
          this.search_link(extra_params = this.paging_extra_params(start)),
          False))
        
        text += '<a href="' + link + '">' + str(page_num + 1) + '</a>'

#    if current_page < last_page_number - 1:
    if current_page < total_pages - 1:
      
      link = el.string.manip.xml_encode(this.make_ref(\
        this.search_link(extra_params = \
          this.paging_extra_params((current_page + 1) * this.results_per_page+\
                                   1)),
        False))

      text += ' <a name="next_link" class="page_next" href="' + \
              link + '">' + this.loc("NEXT_PAGE_LINK") + '</a>'

    return text

  def prn_bottom_bar(this):

    this.prn(R'''
<table id="bottom_bar" cellspacing="0"''')

    this.prn_notranslate_class()

    this.prn(R'''>
  <tr>
  <td class="page_navigation_bar">
  ''')

    if this.paging_panel:
      this.prn(this.paging_panel)

    this.prn(R'</td>')

    if this.site_version == search_util.SearchPageContext.SV_MOB:
      this.prn_page_sharing()

    this.prn('\n  <td class="page_menu">')

    if this.new_search_link:
      this.prn(this.new_search_link)

    this.prn(R'''
  </td>
</table>''')

  def message_click_url(this, message, message_url = ""):

    if not message_url: message_url = message.url
    id = el.string.manip.mime_url_encode(message.encoded_id)
    proto = this.block and "z" or this.static.protocol
    
    return this.static.stat_url + '?s=U' + \
           el.string.manip.mime_url_encode(message_url) + \
           (this.moderation_url == "" and \
            ('&e=c&t=' + proto + '&r=' + \
             (this.test_mode and "000" or this.search_result.request_id) + \
             '&m=' + id + '&e=v&m=' + id) or '')

  def search_query(this,
                   export = False,
                   modifier = True,
                   query = True,
                   filter = True,
                   extra_params = "",
                   debug = False):

    if this.moderation_options:
      if extra_params: extra_params += '&'
      extra_params += this.moderation_options
    
    return this.base_render_type.search_query(this,
                                              export,
                                              modifier,
                                              query,
                                              filter,
                                              extra_params,
                                              debug)
