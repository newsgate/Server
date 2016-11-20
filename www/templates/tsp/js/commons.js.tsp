var browser = "<? browser ?>";
var max_get_length = 1536; // 1.5KB
var category_map = undefined;

function extract_param(param, query)
{
  var pos = query.indexOf(param);

  if(pos > 0)
  {
    param = "&" + param;
    pos = query.indexOf(param);
  }

  if(pos < 0)
  {
    return { param:"", found:false, query:query };
  }

  var offset = pos + param.length;
  var end = query.indexOf("&", offset);

  var res = { param:"", found:true, query:"" };

  if(end < 0)
  {
    res.param = query.slice(offset);
    res.query = query.slice(0, pos);
  }
  else
  {
    res.param = query.slice(offset, end);
    if(pos == 0) end++;
    res.query = query.slice(0, pos) + query.slice(end);
  }
  
  return res;
}

function navigate(url, new_window)
{
  var target = new_window ? "_blank" : "";

  if(el.mod_prepost)
  {
    url = el.mod_prepost(url);
  }

  if(url.length > max_get_length)
  {
    el_post_url(url, "post", target);
  }
  else
  {
    if(new_window)
    {
      window.open(url, target);
    }
    else
    {
      window.location = url;
    }
  }  
}

function post(ref_index, extra_post_refs)
{
  var refs = extra_post_refs ? post_refs2 : post_refs;
  var url = "";

  if(page.extra_params)
  {
    var parts = refs[ref_index].split("?", 2);
    url = parts[0] + "?" + page.extra_params();

    if(parts.length > 1)
    {
      url += "&" + parts[1];
    }
  }
  else
  {
    url = refs[ref_index];
  }
    
  if(el.mod_prepost)
  {
    url = el.mod_prepost(url);
  }

  el_post_url(url);
}

/////////////////////////////////////////////////////////////////////////////
// FullScreenDialog
/////////////////////////////////////////////////////////////////////////////

function FullScreenDialog(extra_params, on_destroy)
{
  ElDialog.call(this);

  this.extra_params = extra_params;
  this.on_destroy_ex = on_destroy;

  this.node.className = "notranslate";

  var s = this.node.style;
  s.margin = "0";
  s.padding = "0";
  s.borderWidth = 0;
  s.minHeight = "100%";
  s.minWidth = "100%";
  s.fontSize = "80%";
  s.left = 0;
  s.top = 0;
}

el_typedef(FullScreenDialog, ElDialog,
{
  init: function() 
  { 
    this.elems =
    [ 
      { id:"mid_area" },
      { id:"top_bar" }, 
      { id:"bottom_bar" }, 
      { id:"copyright" },
      { id:"counters" }
    ];

    for(var i = 0; i < this.elems.length; ++i)
    {
      var elem = this.elems[i];
      var obj = el_by_id(elem.id);

      if(obj && obj.style.display != "none")
      {
        elem.display = obj.style.display;
        obj.style.display = "none";
      }
    }

    return { text: "", style: 'padding:0.7em;' };
  },

  on_destroy: function()
  {
    for(var i = 0; i < this.elems.length; ++i)
    {
      var elem = this.elems[i];
    
      if(elem.display != undefined)
      {
        var obj = el_by_id(elem.id);

        if(obj)
        {
          obj.style.display = elem.display;
        }
      }
    }

    if(this.on_destroy_ex)
    {
      this.on_destroy_ex();
    }
  }
});

/////////////////////////////////////////////////////////////////////////////
// SettingsDialog
/////////////////////////////////////////////////////////////////////////////

function SettingsDialog(extra_params, on_destroy)
{
  FullScreenDialog.call(this, extra_params, on_destroy);

  this.filter_settings = null;
  this.filter_req_seq_number = 1;
}

el_typedef(SettingsDialog, FullScreenDialog,
{
  singleton: true,

  close_name: null,
  ok_name: null,

  on_create: function()
  { 
    this.fill_languages();
    this.filter_option_changed();
  },

  init: function() 
  { 
    var inner = this.el_call(FullScreenDialog, "init");

    var text =
      '<div id="debug"></div><div class="settings_dialog_option_group"><? loc SITE_CONF ?></div>'

    text +='<table class="settings_dialog_option_table" cellspacing="0">\
<tr><td><label for="interface_lang"><? loc INTERFACE_LANG ?></label></td>\
<td class="settings_select_cell"><select id="interface_lang" \
class="settings_select" onchange="el_dlg_by_id(\'' + this.id + 
           '\').interface_lang_changed()">';

    var interface_langs = 
    [ 
      { id:"eng", label: "<? loc LANG_ENG ?>" }, 
      { id:"rus", label: "<? loc LANG_RUS ?>" }
    ];

    for(var i = 0; i < interface_langs.length; i++)
    {
      var opt = interface_langs[i];
    
      text += "<option id=\"il_" + opt.id + "\" " + 
              (page.site.language.current == opt.id ? 
              "selected=\"true\"" : "") + ">" + opt.label + 
              "</option>";
    }

    text += '</select></td></tr><tr><td>\
<label for="sv_settings"><? loc SITE_VER ?></label></td>\
<td class="settings_select_cell">\
<select id="sv_settings" class="settings_select" onchange="el_dlg_by_id(\'' + 
           this.id + '\').site_version_changed()">';

    var sv_options =
    [ 
      { id:page.site.version.DESKTOP, label: "<? loc SITE_VER_DESKTOP ?>" }, 
      { id:page.site.version.TABLET, label: "<? loc SITE_VER_TAB ?>" },
      { id:page.site.version.MOBILE, label: "<? loc SITE_VER_MOBILE ?>" }
    ];

    for(var i = 0; i < sv_options.length; i++)
    {
      var opt = sv_options[i];
    
      text += "<option id=\"sv_" + opt.id + "\" " + 
               (page.site.version.current == opt.id ? 
                "selected=\"true\"" : "") + ">" + opt.label + "</option>";
    }

    text += '</select></td></tr>' + this.site_settings() + '</table>' + 
            this.after_site_settings();

    text += '<div class="settings_dialog_option_group"><? loc SEARCH_CONF ?></div>\
<table class="settings_dialog_option_table" cellspacing="0">';

    var options = 
    [ 
      { param:"n", 
        label:"<? loc LANGUAGE ?>", 
        current:page.search.filter.language,
        xml:"language"
      },
      { param:"y", 
        label:"<? loc COUNTRY ?>", 
        current:page.search.filter.country,
        xml:"country"
      }
    ];

    for(var i = 0; i < options.length; i++)
    {
      var opt = options[i];
      var id = "fo_" + opt.param + "_" + opt.xml;

      text += '<tr><td><label for="' + id + '">' + opt.label + 
              '</label></td><td class="settings_select_cell"><select id="' + 
              id + '" name="filter_options" onchange="el_dlg_by_id(\'' + 
              this.id + 
              '\').filter_option_changed()" class="settings_select">' +
              '<option id="' + opt.current + 
              '" selected="true" disabled="true"><? loc LOADING ?></option>' +
              '</select></td></tr>';
    }

    text += this.search_settings() + '</table>' + this.after_search_settings()+
     '<div class="settings_dialog_option_group"><? loc TRANSLATE_CONF ?></div>\
<table class="settings_dialog_option_table" cellspacing="0">'

    text += '<tr><td>\
<label for="aln_settings"><? loc TRANSLATE_SRV ?></label></td>\
<td class="settings_select_cell">\
<select id="trs_settings" class="settings_select" onchange="el_dlg_by_id(\'' + 
             this.id + '\').fill_languages()">';

    for(var i = 0; i < page.translator.options.length; i++)
    {
      var ts = page.translator.options[i];
    
      text += "<option id=\"ts_" + ts.v + "\" " + 
              (ts.v == page.translator.current ? 
               "selected=\"true\"" : "") + ">" + ts.n + "</option>";
    }

    text += "</select></td></tr>";

    text += '<tr><td>\
<label for="aln_settings"><? loc TRANSLATE_LANG ?></label></td>\
<td class="settings_select_cell">\
<select id="trl_settings" class="settings_select"></select></td></tr>' +
      this.translate_settings() + "</table>" + this.after_translate_settings();

    text += "<div id='settings_dialog_warn'>\
<? loc COOKIE_WARNING ?></div><button onclick='el_close_dlg_by_id(\"" + 
            this.id + "\", true);'><? loc SAVE_SETTINGS ?></button>&nbsp;\
<button onclick='el_dlg_by_id(\"" + this.id + 
            "\").default_settings();'><? loc DEFAULT_SETTINGS ?></button>&nbsp;\
<button onclick='el_close_dlg_by_id(\"" + this.id + 
          "\", false);'><? loc CLOSE_SETTINGS ?></button>";

    inner.text = text;
    return inner;
  },

  default_settings: function()
  {
    var select = el_by_id("interface_lang");
    var elems = select.options;

    var eng_index = -1;
    var i = 0;

    for(; i < elems.length; i++)
    {
      var elem = elems[i];
      var opt_vals = elem.id.split("_", 2);
      var lang = opt_vals[1];
    
      if(lang == page.site.language.def)
      {
        select.selectedIndex = i;
        break;
      }
      else if(lang == "eng")
      {
        eng_index = i;
      }
    }

    if(i == elems.length && eng_index >= 0)
    {
      select.selectedIndex = eng_index;
    }

    el_by_id("sv_settings").selectedIndex = page.site.version.def;

    elems = el_by_name("filter_options");
    var sel_changed = false;

    for(var i = 0; i < elems.length; i++)
    {
      var elem = elems[i]

      var def_filter_value = elem.id ? page.search.filter.def[elem.id] :
        undefined;

      if(def_filter_value !== undefined)
      {
        for(var j = 0; j < elem.options.length; ++j)
        {
          if(elem.options[j].id == def_filter_value)
          {
            elem.selectedIndex = j;
            sel_changed = true;    
            break;
          }
        }
      }
      else if(elem.selectedIndex)
      {
        elem.selectedIndex = 0;
        sel_changed = true;
      }
    }

    if(sel_changed)
    {
      this.filter_option_changed();
    }

    select = el_by_id("trs_settings");
    elems = select.options;

    for(var i = 0; i < elems.length; i++)
    {
      var elem = elems[i];
      var opt_vals = elem.id.split("_", 2);

      if(opt_vals[1] == page.translator.def)
      {
        select.selectedIndex = i;
        break;
      }
    }

    this.fill_languages();

    select = el_by_id("trl_settings");
    elems = select.options;

    for(var i = 0; i < elems.length; i++)
    {
      var elem = elems[i];
      var opt_vals = elem.id.split("_", 2);

      if(opt_vals[1] == page.translator.def_lang)
      {
        select.selectedIndex = i;
        break;
      }
    }
  },

  on_ok: function(params)
  {
    var settings = el_by_id("interface_lang");
    var lang = settings.options[settings.selectedIndex].id.split("_", 2)[1];
  
    var lang_set = false;
    var current_path = window.location.href.split("?")[0].split("#")[0];

    if(page.search.path == current_path)
    {
      var parts = page.search.path.split("/");
      current_path = "";

      for(var i = 0; i < parts.length; i++)
      {
        if(i) current_path += "/";

        if(i == 4 && parts[i] != "search" && parts[i].length == 3)
        {
          current_path += lang;
          lang_set = true;
        }
        else
        {
          current_path += parts[i];
        }
      }
    }

    current_path += "?";

    if(!lang_set)
    {
      current_path += "lang=" + lang + "&";
    }

    if(this.extra_params != "")
    {
      current_path += this.extra_params + "&";
    }

//    alert(this.extra_params);

    var url = current_path + "slg=1";

    var elems = el_by_name("filter_options");

    for(var i = 0; i < elems.length; i++)
    {
      var elem = elems[i];
      var opt_vals = elem.id.split("_", 3);
      var optv = opt_vals[1] + "=";

      url += "&" + optv + "s-" + 
        el_mime_url_encode(elem.options[elem.selectedIndex].id);

      page.search.query = extract_param(optv, page.search.query).query;
    }

    settings = el_by_id("sv_settings");
    opt = settings.options[settings.selectedIndex];

    if(settings.selectedIndex >= 0)
    {
      var sv = opt.id.split("_", 2)[1];
      url += "&sv=s-" + sv;
      page.search.query = extract_param("sv=", page.search.query).query;
    }

    var tr_srv = "";
    settings = el_by_id("trs_settings");

    if(settings.selectedIndex >= 0)
    {
      opt = settings.options[settings.selectedIndex];
      tr_srv = opt.id.split("_", 2)[1];
      url += "&trs=s-" + tr_srv;
      page.search.query = extract_param("trs=", page.search.query).query;
    }

    var tr_lang = "";
    settings = el_by_id("trl_settings");

    if(settings.selectedIndex >= 0)
    {
      opt = settings.options[settings.selectedIndex];
      tr_lang = opt.id.split("_", 2)[1];
      url += "&trl=s-" + tr_lang;
      page.search.query = extract_param("trl=", page.search.query).query;
    }

    var tr_cur_lang = page.translator.cur_lang;

    if(tr_lang != page.translator.lang)
    {
      tr_cur_lang = tr_lang;
    }

    if(tr_cur_lang && tr_srv)
    {
      url += "&tr=" + tr_srv + "-|" + tr_lang;
    }

    if(page.search.query != "")
    {
      url += "&" + page.search.query;
    }

    if(params)
    {
      url += "&" + params;
    }

    navigate(url, false);
  },

  site_settings: function()
  {
    return "";
  },

  after_site_settings: function()
  {
    return "";
  },

  search_settings: function()
  {
    return "";
  },

  after_search_settings: function()
  {
    return "";
  },

  translate_settings: function()
  {
    return "";
  },

  after_translate_settings: function()
  {
    return "";
  },

  fill_languages: function()
  {
    var lang_sel = el_by_id("trl_settings");
    var options = lang_sel.options;

    while(options.length)
    {
      lang_sel.remove(0);
    }

    if(this.languages === undefined)
    {
      var request;

      try
      {
        request = new ActiveXObject("Msxml2.XMLHTTP");
      }
      catch(e)
      {
        request = new XMLHttpRequest();
      }

      var data = 
      { req: request,
        dlg:this
      };

      request.onreadystatechange = this.get_languages_req_handler(data);
      request.open("GET", page.translator.lang_url, true);
      request.send("");
      return;
    }

    var settings = el_by_id("trs_settings");

    var valid_langs = settings.selectedIndex >= 0 ?
      page.translator.options[settings.selectedIndex].s : new Array();

    for(var i = 0; i < this.languages.length; ++i)
    {
      var lang = this.languages[i];

      var j = 0;
      for(; j < valid_langs.length && valid_langs[j] != lang.c; ++j);

      if(j == valid_langs.length)
      {
        continue;
      }

      var opt = document.createElement('option');
      opt.text = lang.n;
      opt.id = "trl_" + lang.c;

      if(lang.c == page.translator.lang)
      {
        opt.selected = true;
      }

      try
      {
        lang_sel.add(opt, null); // standards compliant
      }
      catch(ex)
      {
        lang_sel.add(opt); // IE only
      }
    }
  },

  read_languages: function(req)
  {
    if(this.closed)
    {
      return;
    }

    this.languages = eval("(" + req.responseText + ")");
    this.fill_languages();
  },

  get_languages_req_handler: function(data)
  {
    var onready = function()
    {
      if(data.req.readyState == 4)
      {        
        switch(data.req.status)
        {
          case 200:
          {
            data.dlg.read_languages(data.req);
            break;
          }
          default:
          {
//            alert("<? loc SERVER_ERROR ?> " + 
//                  data.req.status);
            break;
          }
        }    
      }
    }

    return onready;
  },

  interface_lang_changed: function()
  {
    var select = el_by_id("interface_lang");
    
    if(select.selectedIndex >= 0)
    {
      var lang = select.options[select.selectedIndex].id.split("_", 2)[1];

      select = el_by_id("trl_settings");
      var options = select.options;

      for(var i = 0; i < options.length; ++i)
      {
        if(options[i].id.split("_", 2)[1] == lang)
        {
          select.selectedIndex = i;
          break;
        }
      }
    }
  },

  site_version_changed: function()
  {
  },

  filter_option_changed: function()
  {
    var elems = el_by_name("filter_options");
    var changed = false;

    for(var i = 0; i < elems.length; i++)
    {
      var elem = elems[i];
      var options = elem.options;

      for(var j = 0; j < options.length; j++)
      {
        var opt = options[j];
        var pos = opt.text.lastIndexOf("(");
      
        if(pos >= 0)
        {
          opt.text = opt.text.slice(0, pos);
        }
      }
    }

    this.filter_settings = new Array();
    this.filter_settings.req_number = this.filter_req_seq_number++;

    this.send_filter_settings_request(false);
  },

  send_filter_settings_request: function(every)
  {
    var elems = el_by_name("filter_options");

    var request;

    try
    {
      request = new ActiveXObject("Msxml2.XMLHTTP");
    }
    catch(e)
    {
      request = new XMLHttpRequest();
    }

    var data = 
    { req: request,
      dlg:this,
      sel_indexes: new Array(),
      sequence_number: this.filter_settings.req_number,
      every: every
    };

    request.onreadystatechange = 
     this.get_update_filter_items_req_handler(data);

    var params = "t=x&z=bh&s=1&r=0&lang=<? lang ?>";

    for(var i = 0; i < elems.length; i++)
    {
      var elem = elems[i];
      var p = elem.id.split("_", 3)[1];
      var id = elem.options[elem.selectedIndex].id;

      params += "&" + p + "=" + el_mime_url_encode(id);
      data.sel_indexes[p] = id;
    }

    if(every)
    {
      params += "&q=EVERY";
    }
    else
    {
      params += "&" + page.search.query;
    }

    request.open("POST", page.search.path, true)

    request.setRequestHeader("Content-type", 
                             "application/x-www-form-urlencoded");
  
    request.send(params);
  },

  get_update_filter_items_req_handler: function(data)
  {
    var onready = function()
    {
      if(data.req.readyState == 4)
      {        
        switch(data.req.status)
        {
          case 200:
          {
            var dlg = data.dlg;

            if(dlg.filter_settings != null &&
               dlg.filter_settings.req_number == data.sequence_number)
            {
              dlg.update_filter_items(data.req, data.sel_indexes, data.every);
            }

            break;
          }
          default:
          {
//            alert("<? loc SERVER_ERROR ?> " + 
//                  data.req.status);
            break;
          }
        }    
      }
    }

    return onready;
  },

  update_filter_items: function(request, sel_indexes, every)
  {
    if(this.closed)
    {
      return;
    }

    var elems = el_by_name("filter_options");
    var changed = false;

    for(var i = 0; i < elems.length; i++)
    {
      var elem = elems[i];
      var elem_id = elem.id;

      var j = 0;

      for(; j < this.filter_settings.length && 
          this.filter_settings[j].id != elem_id; j++);

      var filter;

      if(j < this.filter_settings.length)
      {
        filter = this.filter_settings[j];
      }
      else
      {
        filter = { id: elem_id, options: new Array() };
        this.filter_settings.push(filter);
      }

      var vals = elem_id.split("_", 3);
      var param = vals[1];
      var xml_elem_name = vals[2];

      var elements = el_child_node(el_child_node(el_child_node(
                                   request.responseXML, 
                                   "result"), 
                                   "stat"), 
                                   xml_elem_name).childNodes;

      var selected_item_id = sel_indexes[param];
      var filter_options = filter.options;

      for(var j = 0; j < elements.length; j++)
      {
        var element = elements[j];

        if(element.tagName == "item")
        {
          var id = el_child_text(element, "id");
          var value = el_child_text(element, "val");
          var count = parseInt(element.getAttribute("count"));

          var k = 0;
          for(; k < filter_options.length && filter_options[k].id != id; k++);

          var option;
          if(k < filter_options.length)
          {
            option = filter_options[k];
          }
          else
          {
            if(!every && !count)
            {
              continue;
            }

            option = { id: id, 
                       value: value, 
                       req_count: 0, 
                       total_count: 0, 
                       selected: id == selected_item_id
                     };

            filter_options.push(option);
          }

          if(every)
          {
            option.total_count = count;
          }
          else
          {
            option.req_count = count;
          }
        }

      }
    }

    if(!every)
    {
      this.send_filter_settings_request(true);
      return;
    }

    var changed = false;
    var is_msie = browser == "msie";

    for(var i = 0; i < this.filter_settings.length; i++)
    {
      var elem_id = this.filter_settings[i].id;
      var elem = el_by_id(elem_id);

      var options = elem.options;

      while(options.length)
      {
        elem.remove(0);
      }

      var filter_options = this.filter_settings[i].options;
      var selected = false;

      var j = 1;
      for(; j < filter_options.length && filter_options[j].req_count; j++);

      var highlight = j < filter_options.length;

      for(j = 0; j < filter_options.length; j++)
      {
        var fopt = filter_options[j];

        if(!is_msie && j && filter_options[j - 1].req_count && 
           fopt.req_count == 0)
        {
          var opt = document.createElement('option');
          opt.innerHTML = "&mdash;";
          opt.disabled = true;

          try
          {
            elem.add(opt, null); // standards compliant
          }
          catch(ex)
          {
            elem.add(opt); // IE only
          }
        }

        var opt = document.createElement('option');
        opt.id = fopt.id;
      
        var text = fopt.value + " (";

        if(fopt.total_count != fopt.req_count)
        {
          text += fopt.req_count + " of ";
        }

        text += fopt.total_count + ")";
        opt.text = text;

        if(fopt.selected)
        {
          opt.selected = true;
          selected = true;
        }
   
        try
        {
          elem.add(opt, null); // standards compliant
        }
        catch(ex)
        {
          elem.add(opt); // IE only
        }

        if(is_msie && j && highlight && !fopt.req_count)
        {
          options[options.length - 1].style.color = "rgb(70,70,70)";
        }
      }

      elem.disabled = false;      

      if(!selected)
      {
        elem.selectedIndex = 0;
        changed = true;
      }
    }

    if(changed)
    {
      this.filter_option_changed();
    }
  }

});

function show_settings_dialog(extra_params, on_destroy)
{
  var dlg = new SettingsDialog(extra_params, on_destroy);
}

///////////////////////////////////////////////////////////////////////////////
// Paging dialog
///////////////////////////////////////////////////////////////////////////////

function PagingDialog(xml_element_name, extra_params, on_destroy)
{
  this.xml_element_name = xml_element_name;
  this.search = "";

  FullScreenDialog.call(this, extra_params, on_destroy);
}

el_typedef(PagingDialog, FullScreenDialog,
{
  items_per_page: 10,
  singleton: true,

  close_name: null,
  ok_name: null,

  init: function() 
  { 
    var inner = this.el_call(FullScreenDialog, "init");

    inner.text = "<? loc LOADING ?>";
    return inner;
  },

  on_create: function()
  { 
    this.fill(null);
    this.search_items();
  },

  fill: function(index)
  {
    if(this.items === undefined)
    {
      this.inner().innerHTML = "<? loc LOADING ?>";

      var request;

      try
      {
        request = new ActiveXObject("Msxml2.XMLHTTP");
      }
      catch(e)
      {
        request = new XMLHttpRequest();
      }

      var data = { req:request, dlg:this };
      request.onreadystatechange = this.get_items_req_handler(data);

      res = extract_param("f=", page.search.query);
      var f = res.param;
      var query = res.query;

      if(f != "")
      {
        var new_q = "q=URL " + f;

        res = extract_param("q=", res.query);
        var q = res.param;
        query = res.query;

        if(q != "")
        {
          new_q += " AND ( " + q + " )";
        }

        query = new_q + "&" + query;
      }

      var params = "t=x&z=k&s=1&r=0&lang=<? lang ?>&" + query;

      request.open("POST", page.search.path, true);

      request.setRequestHeader("Content-type", 
                               "application/x-www-form-urlencoded");
  
      request.send(params);
      return;
    }

    var text = "";

    text += "<div id='paging_dialog_header'><? loc SEARCH ?> \
<input type='text' id='paging_dialog_search'/>\
<span id='paging_dialog_stat'></span></div>";

    text += "<div id='paging_dialog_area'></div>\
<div style='padding:0.5em 0 0'>\
<button onclick='el_close_dlg_by_id(\"" + this.id + 
          "\", false);'><? loc CLOSE_PAGING ?></button></div>";

    this.inner().innerHTML = text;

    var edit = el_by_id("paging_dialog_search");
    edit.focus();

    this.fill_items_table(index);
  },

  get_items_req_handler: function(data)
  {
    var onready = function()
    { 
      if(data.req.readyState == 4)
      {
        switch(data.req.status)
        {
          case 200:
          {
            data.dlg.inner().innerHTML = "<? loc READING ?>";

            setTimeout(function() 
                       { 
                         data.dlg.process_items_request(data.req); 
                       }, 
                       0);

            break;
          }
          default:
          {
//            alert("<? loc SERVER_ERROR ?> " + data.req.status);
            break;
          }
        }   
      }
    }

    return onready;
  },

  process_items_request: function(request)
  {
    var elements = 
      el_child_node(
        el_child_node(
          el_child_node(request.responseXML, "result"), 
          "stat"), 
        this.xml_element_name).childNodes;

    this.items = { values:new Array(), current:-1};

    for(var i = 0; i < elements.length; i++)
    {
      var element = elements[i];

      if(element.tagName == "item")
      {
        var item = 
        { id:el_child_text(element, "id"), 
          value:el_child_text(element, "val"),
          count:element.getAttribute("count")
        };

        if(item.id != "")
        {
          if(element.getAttribute("selected") == "true")
          {
            this.items.current = this.items.values.length;
          }

          this.items.values.push(item);
        }
      }
    }

    this.fill(null);
  },

  fill_items_table: function(index)
  {
    var area = el_by_id("paging_dialog_area");

    var searched_items = null;
    var search = this.search.toLowerCase();

    if(search == "")
    {
      searched_items = this.items;
    }
    else
    {
      searched_items = { values:new Array(), current:-1};

      for(var i = 0; i < this.items.values.length; i++)
      {
        var id = this.items.values[i].id;

        if(id == "" || id.toLowerCase().indexOf(search) >= 0 ||
           this.items.values[i].value.toLowerCase().indexOf(search) >= 0)
        {
          if(i == this.items.current)
          {
            searched_items.current = searched_items.values.length;
          }

          searched_items.values.push(this.items.values[i]);
        }
      }
    }

    var paging_dialog = el_by_id("paging_dialog_stat");

    paging_dialog.innerHTML = searched_items.values.length + "/" + 
    this.items.values.length;

    if(index == null)
    {
      index = searched_items.current < 0 ? 0 :
         (Math.floor(searched_items.current / this.items_per_page) *
          this.items_per_page);
    }

//    var page_panel = "<? loc PAGE ?>";
    var page_panel = "";

    var pages = Math.ceil(searched_items.values.length / this.items_per_page);

    if(pages > 1)
    {
      page_panel += '<div class="paging_dialog_pages">';

      var curr_page = Math.floor(index / this.items_per_page);

      if(curr_page)
      {
        page_panel += "&#xA0;<a href='javascript:el_dlg_by_id(\"" + this.id + 
                      "\").fill_items_table(" + 
                      (curr_page - 1) * this.items_per_page + ")'>&lt;</a>";
      }

      var pages_in_bar = 10;

      var from = Math.max(curr_page - pages_in_bar / 2, 0);
      var to = Math.min(from + pages_in_bar, pages);

      for(var i = from; i < to; i++)
      {
        if(i == curr_page)
        {
          page_panel += "&#xA0;" + (i + 1);
        }
        else
        {
          page_panel += "&#xA0;<a href='javascript:el_dlg_by_id(\"" + this.id +
                        "\").fill_items_table(" + 
                        i * this.items_per_page + ")'>" + (i + 1) + "</a>";
        }
      }

      if(curr_page + 1 < pages)
      {
        page_panel += "&#xA0;<a href='javascript:el_dlg_by_id(\"" + this.id + 
                      "\").fill_items_table(" + 
                      (curr_page + 1) * this.items_per_page + ")'>&gt;</a>";
      }

      page_panel += "</div>";
    }

    var text = page_panel;

    var last = Math.min(searched_items.values.length, 
                        index + this.items_per_page);

    text += "<table class=\"option_select_table\">";

    for(var i = index; i < last; i++)
    {
      var item = searched_items.values[i];

      text += "<tr class='" + (i == searched_items.current ? 
              "paging_dialog_selected_row" : "paging_dialog_row") + 
              "'><td align='right' class='paging_dialog_counter_cell'>" +
              "<a href=\"" + this.make_source_url(item.id, true) + "\">" + 
              item.count +
              '</a></td><td class="paging_dialog_cell"><div class="paging_dialog_value">' +
              this.highlight_search(item.value, search) + 
              '</div><div class="paging_dialog_id">' +
              "<a href=\"" + this.make_source_url(item.id, false) + "\">" + 
              this.highlight_search(item.id, search) + 
              "</a></div></td></tr>";
    }

    text += "</table>" + page_panel;
    area.innerHTML = text;
//    this.adjust_size();
  },

  make_source_url: function(feed, as_filter)
  {
    var url = this.source_url(feed, as_filter);

    if(url.length > max_get_length || el.mod_prepost)
    {
      return "javascript:navigate(el_dlg_by_id('" + this.id + 
             "').source_url('" + el_mime_url_encode(feed) + 
             "'," + as_filter + "), true);";
    }
    else
    {
      return url;
    }  
  },

  source_url: function(feed, as_filter)
  {
    var query = "";

    if(as_filter)
    {
      var res = extract_param("v=", page.search.query);

      if((res.param == "A" || res.param == "") && 
         extract_param("q=", res.query).param == "")
      {
        query = "v=S" + el_mime_url_encode(feed);

        if(res.query)
        {
          query += "&" + res.query;
        }
      }
      else
      {
        var res = extract_param("f=", page.search.query);
        query = res.query;

        if(query)
        {
          query += "&";
        }

        query += "f=" + el_mime_url_encode(feed);
      }
    }
    else
    {
      query = "v=S" + el_mime_url_encode(feed);

      var res = extract_param("v=", page.search.query);
      res = extract_param("f=", res.query);
      res = extract_param("g=", res.query);
      res = extract_param("h=", res.query);

      if(res.query)
      {
        query += "&" + res.query;
      }
    }

    if(this.extra_params)
    {
      if(query)
      {
        query += "&";
      }

      query += this.extra_params;
    }

    var url = query ? page.search.path + "?" + query : page.search.path;

    if(page.translator.query_suffix)
    {
      url += "&" + page.translator.query_suffix;
    }
  
    return url;
  },

  search_items: function()
  {
    if(this.closed)
    {
      return;
    } 

    var edit = el_by_id("paging_dialog_search");

    if(edit != null)
    {
      if(edit.value == this.search)
      {
        if(this.search_changed)
        {
          this.search_changed = false;
          this.fill_items_table(null);
        }
      }
      else
      {
        this.search = edit.value;
        this.search_changed = true;
      }
    }

    var dlg = this;
    setTimeout(function() { dlg.search_items() }, 500);
  },

  highlight_search: function(text, search)
  {
    if(search == "")
    {
      return el_xml_encode(text);
    }
  
    var res = "";
    var lwr_text = text.toLowerCase();
    var search_len = search.length;
  
    var begin = 0;
    var pos;

    while((pos = lwr_text.indexOf(search, begin)) >= 0)
    {
      res += el_xml_encode(text.slice(begin, pos)) + 
             "<span class='found_word'>" + 
             el_xml_encode(text.slice(pos, pos + search_len)) + 
             "</span>";

      begin = pos + search_len;
    }

    if(begin != lwr_text.length)
    {
      res += el_xml_encode(text.slice(begin));
    }

    return res;
  }

});

function show_paging_dialog(xml_element_name, extra_params, on_destroy)
{
  var dlg = new PagingDialog(xml_element_name, extra_params, on_destroy);
}

///////////////////////////////////////////////////////////////////////////////
// Search dialog
///////////////////////////////////////////////////////////////////////////////

function SearchDialog(extra_params, on_destroy)
{
  FullScreenDialog.call(this, extra_params, on_destroy);
}

el_typedef(SearchDialog, FullScreenDialog,
{
  singleton: true,

  close_name: null,
  ok_name: null,

  init: function() 
  { 
    var inner = this.el_call(FullScreenDialog, "init");

    inner.text = page.search_bar.tagline;

    if(page.search_bar.subcats)
    {
      inner.text += page.search_bar.subcats;
    }

    if(page.search_bar.controls)
    {
      inner.text += page.search_bar.controls;
    }

    inner.text += 
      "<div style='padding:0.5em 0 0'><button onclick='el_close_dlg_by_id(\"" +
      this.id + "\", false);'><? loc CLOSE_SEARCH ?></button></div>";

    return inner;
  }
});

function show_search_dialog(extra_params, on_destroy)
{
  var dlg = new SearchDialog(extra_params, on_destroy);
}

///////////////////////////////////////////////////////////////////////////////
// Categories dialog
///////////////////////////////////////////////////////////////////////////////

function CategoriesDialog(extra_params, on_destroy)
{
  FullScreenDialog.call(this, extra_params, on_destroy);

  var s = this.node.style;
  s.backgroundColor = "#FAFAFA";
}

el_typedef(CategoriesDialog, FullScreenDialog,
{
  singleton: true,

  close_name: null,
  ok_name: null,

  init: function() 
  { 
    var inner = this.el_call(FullScreenDialog, "init");

    inner.text = page.category_bar.text + 
      "<div style='padding:0.5em 0 0'><button onclick='el_close_dlg_by_id(\""+
      this.id + "\", false);'><? loc CLOSE_PAGING ?></button></div>";

    return inner;
  }
});

function show_categories_dialog(extra_params, on_destroy)
{
  var dlg = new CategoriesDialog(extra_params, on_destroy);
}

///////////////////////////////////////////////////////////////////////////////
// Category bar
///////////////////////////////////////////////////////////////////////////////

function LoadCategoryDialog(cat_location)
{
  ElDialog.call(this);

  this.node.className = "notranslate";

  var parts = cat_location.split("_");
  this.cat_id = parts[0];
  this.cat_parent_id = parts[1];

  var rect = el_node_rect(document.getElementById("cat_exp_" + cat_location));

  var s = this.node.style;
  s.left = rect.x + "px";
  s.top = rect.y + "px";
  s.position = "absolute";
  s.background = "rgb(230, 230, 236)";
  s.padding = "0.4em";
  s.borderWidth = "1px";
  s.fontSize = "80%";
  s.whiteSpace = "nowrap";
}

el_typedef(LoadCategoryDialog, ElDialog,
{
  singleton: true,
  close_name: null,
  ok_name: null,

  init: function() 
  {
    return "<? loc LOADING ?>";
  },

  on_create: function()
  {
    var request;

    try
    {
      request = new ActiveXObject("Msxml2.XMLHTTP");
    }
    catch(e)
    {
      request = new XMLHttpRequest();
    }

    var data = { req: request, dlg: this };

    request.onreadystatechange = this.get_categories_req_handler(data);
    request.open("POST", page.search.path, true);
    
    request.setRequestHeader("Content-type", 
                             "application/x-www-form-urlencoded");

    var url = "t=x&z=j&lang=<? lang ?>&" + page.search.cat_query_prefix;

    if(page.translator.cur_lang)
    {
      url += "&tr=" + page.translator.cur_lang;
    }

    request.send(url);
  },
  
  get_categories_req_handler: function(data)
  {
    var onready = function()
    { 
      if(data.req.readyState == 4)
      {
        switch(data.req.status)
        {
          case 200:
          {
            var dlg = data.dlg;

            fill_category_bar(data.req, dlg.cat_id, dlg.cat_parent_id);
            dlg.close();

            break;
          }
          default:
          {
            category_map = undefined;
            break;
          }
        }      
      }
    }

    return onready;
  }

});

function fill_category_bar(request, cat_id, cat_parent_id)
{
  if(category_map != null)
  {
    return;
  }

  category_map = new Array();

  var elements = el_child_node(el_child_node(el_child_node(
                               request.responseXML, 
                               "result"), 
                               "stat"), 
                               "category").childNodes;

  for(var j = 0; j < elements.length; j++)
  {
    var element = elements[j];

    if(element.tagName == "item")
    {      
      if(parseInt(element.getAttribute("lcCount")) < 1)
      {
        continue;
      }

      var parent = parseInt(element.getAttribute("parent"));
      
      if(category_map[parent] === undefined)
      {
        category_map[parent] = { subcats: new Array() };
      }

      var names = el_child_text(element, "val").split("/");

      category_map[parent].subcats.push(
        { id: parseInt(element.getAttribute("self")),
          name: names[names.length - 1],
          path: el_child_text(element, "id")
        });
    }
  }

  show_cat(cat_id, cat_parent_id, true);
}

function show_cat(cat_id, cat_parent_id, visibility)
{
  var cat_location = cat_id + "_" + cat_parent_id;

  if(visibility)
  {
    if(category_map === null)
    {
      return;
    }

    if(category_map === undefined)
    {
      category_map = null;
      var dlg = new LoadCategoryDialog(cat_location);

      return;
    }

    var cat = null;

    try
    {
      cat = category_map[cat_id];
      category_map[cat_id] = null;
    }
    catch(e)
    {
    }

    if(cat)
    {
      var plus = 
        el_child_node(document.getElementById("cat_exp_" + cat_location), 
                      "img").getAttribute("src");

      var minus = 
        el_child_node(document.getElementById("cat_col_" + cat_location), 
                      "img").getAttribute("src");

      var href = 
        document.getElementById("lnk_cat_" + cat_location).
          getAttribute("href");

      var post_prefix = null;
      var post_suffix = null;

      if(href != null && href.slice(0, 11) == "javascript:")
      {
        var pos1 = href.indexOf("(");
        
        if(pos1 > 0)
        {
          var pos2 = href.indexOf(",", pos1);

          if(pos2 > 0)
          {
            post_prefix = href.substr(0, pos1 + 1);
            post_suffix = href.substr(pos2);
          }
        }
      }

      var subcats = cat.subcats;

      for(var i = 0; i < subcats.length; ++i)
      {
        var subcat = subcats[i];
        var subcat_location = subcat.id + "_" + cat_id;
      
        if(document.getElementById("cat_" + subcat_location) != null)
        {
          continue;
        }

        var el = document.createElement("div");

        el.id = "cat_" + subcat_location;
        el.className = "right_block_subcat";

        var prefix = category_map[subcat.id] === undefined ?
          '<span class="right_block_cpad"></span>' :
          '<a id="cat_exp_' + subcat_location + 
          '" href="javascript:show_cat(' + 
          subcat.id + ", " + cat_id + ', true)"><img src="' + plus + 
          '" width="11" height="11" class="right_block_img"/></a>' + 
          '<a id="cat_col_' + subcat_location +
          '" style="display:none" href="javascript:show_cat(' + subcat.id +
          ', ' + cat_id + ', false)"><img src="' + minus + 
          '" width="11" height="11" class="right_block_img"/></a>';

        href = page.search.path + "?" + "v=C" + 
               el_mime_url_encode(subcat.path);

        if(page.search.cat_query_prefix)
        {
          href += "&" + page.search.cat_query_prefix;
        }

        if(page.translator.query_suffix)
        {
          href += "&" + page.translator.query_suffix;
        }

        if(post_prefix != null)
        {
          var index = post_refs.length;
          post_refs.push(href);

          href = post_prefix + index + post_suffix;
        }

        el.innerHTML = prefix + '<a id="lnk_cat_' + subcat_location + 
                       '" href="' + href + '">' + el_xml_encode(subcat.name) +
                       '</a>';

        var cat_div = document.getElementById("cat_" + cat_location);

        var e = cat_div.firstChild;
        for(; e != null; e = e.nextSibling)
        {
          if(e.className == "right_block_subcat")
          {
            if(el_text(document.getElementById("lnk_" + e.id)) > subcat.name)
            {
              break;
            }
          }
        }

        cat_div.insertBefore(el, e);
      }
    }
  }

  var cat = document.getElementById("cat_" + cat_location);

  for(var el = cat.firstChild; el != null; el = el.nextSibling)
  {
    var id = el.id;

    if(id === undefined || id == null)
    {
      continue;
    }

    var id_prefix = id.substr(0, 8);

    if(id_prefix == "cat_exp_")
    {
      if(id.split("_")[2] != cat_id)
      {
        continue;
      }

      el.style.display = visibility ? "none" : "inline";
    }
    else if(id_prefix == "cat_col_")
    {
      if(id.split("_")[2] != cat_id)
      {
        continue;
      }

      el.style.display = visibility ? "inline" : "none";
    }
    else if(el.className == "right_block_subcat")
    {
      el.style.display = visibility ? "block" : "none";
    }
  }
}

function get_categories_onreadystatechange(data)
{
  var onready = function()
  { 
    if(data.req.readyState == 4)
    {
      switch(data.req.status)
      {
        case 200:
        {
          fill_category_bar(data.req, data.cat_id, data.cat_parent_id);
          break;
        }
        default:
        {
          category_map = undefined;
          break;
        }
      }      
    }
  }

  return onready;
}
