var post_refs = null;
var post_refs2 = null;
var query_box = null;
var row_style_visible = null;
var visible_messages = null;
var mail_subs_ti = 5;

function init(error_pos)
{
  if(el_by_id("mid_area") == null)
  {
    window.setTimeout("init(" + error_pos + ");", 300);
    return;
  }

  query_box = el_enrich(el_by_id("se"));
  set_focus_next_link(error_pos);
}

function message_visible(id)
{
  return el_visible(document.getElementById("mt_" + id), 0.1) || 
         el_visible(document.getElementById("ma_" + id), 0.1) ||
         el_visible(document.getElementById("mm_" + id), 0.1) ||
         el_visible(document.getElementById("mf_" + id), 1.0);
}

function report_message_impression()
{
  if(page.impression.messages.length == 0)
  {
    return;
  }

  if(visible_messages)
  {
    var stat_url = "";
    var shift = 0;

    for(var i = 0; i < visible_messages.length; ++i)
    {
      var j = visible_messages[i] - shift;
      var id = page.impression.messages[j];

      if(message_visible(id))
      {
        if(stat_url == "")
        {
          stat_url = page.impression.stat_url_prefix + "&m=";
        }
        else
        {
          stat_url += "+";
        }

        stat_url += el_mime_url_encode(id);
        page.impression.messages.splice(j, 1);
        ++shift; 
      }
    }

    if(stat_url != "")
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

      request.open("GET", stat_url, true);
      request.send("");
    }
  }

  visible_messages = new Array();

  for(var i = 0; i < page.impression.messages.length; ++i)
  {
    var id = page.impression.messages[i];

    if(message_visible(id))
    {
      visible_messages.push(i);
    }
  }

  window.setTimeout("report_message_impression();", 1000);
}

function set_focus_next_link(error_pos)
{
  var elems = el_by_name("next_link");

  if(error_pos == 0 && elems !== undefined && elems.length > 0)
  {
    elems[0].tabIndex = "1"

    if(query_box)
    {
      query_box.tabIndex = "2";
    }

    if(page.site.version.current != page.site.version.MOBILE)
    {
      elems[0].focus();
    }
  }
  else if(query_box)
  {
    var sel = error_pos > 0 ? {start:error_pos-1, end:error_pos-1} : 
      {start:0, end:query_box.value.length};

    query_box.el_set_selection(sel, true)
  }
}

function search_key_press(event)
{
  if(event.keyCode == 13)
  {
    search(false);
    return false;
  }

  return true;
}

function search(new_window)
{
  query = extract_param("q=", page.search.query).query;

  var res = extract_param("v=C", query);
  var c1 = res.param;
  query = res.query;
  
  res = extract_param("g=", query);
  var c2 = res.param;
  query = res.query;

  res = extract_param("v=A", query);
  query = res.query;

  res = extract_param("v=S", query);
  var s1 = res.param;
  query = res.query;

  res = extract_param("f=", query);
  var s2 = res.param;
  query = res.query;

  res = extract_param("v=E", query);
  var e1 = res.param;
  query = res.query;

  res = extract_param("h=", query);
  var e2 = res.param;
  query = res.query;

  var q = el_trim(el_by_id("se").value);

  var c = c1 == "" ? c2 : c1;
  var s = s1 == "" ? s2 : s1;
  var e = e1 == "" ? e2 : e1;

  var url = page.search.path + "?";

  if(page.extra_params)
  {
    url += page.extra_params() + "&";
  }

  url += "q=" + el_mime_url_encode(q);

  if(e != "")
  {
    url += (q == "" ? "&v=E" : "&h=") + e;
  }

  if(c != "")
  {
    if(q == "" || c != "%2F")
    {
      url += (q == "" && e == "" && s == "" ? "&v=C" : "&g=") + c;
    }
  }

  if(s != "")
  {
    url += (q == "" && e == "" ? "&v=S" : "&f=") + s;
  }
 
  if(q == "" && e == "" && s == "" && c == "")
  {
    url += "&v=A";
  }

  if(query != "")
  {
    url += "&" + query;
  }

  if(page.translator.query_suffix)
  {
    url += "&" + page.translator.query_suffix;
  }

  navigate(url, new_window);
}

/////////////////////////////////////////////////////////////////////////////
// SearchSettingsDialog
/////////////////////////////////////////////////////////////////////////////

function SearchSettingsDialog(extra_params, on_destroy)
{
  SettingsDialog.call(this, extra_params, on_destroy);
}

el_typedef(SearchSettingsDialog, SettingsDialog,
{
  on_create: function()
  {
    this.el_call(SettingsDialog, "on_create");
    this.set_view_options_visibility();
  },

  search_settings: function()
  {
    var text = "";

    var bool_settings = 
    [ 
      { param:"b", on:"0", off:"3", label: "<? loc DUPLICATES ?>", def: 0 }
    ];

    for(var i = 0; i < bool_settings.length; i++)
    {
      text += this.setting_checkbox(bool_settings[i]);
    }

    return text;
  },

  after_search_settings: function()
  {
    var text =
'<div class="settings_dialog_option_group"><? loc SORT_CONF ?></div>\
<table class="settings_dialog_option_table" cellspacing="0">'

    text += '<tr><td><label for="sort_crit"><? loc SORT_CRIT ?></label></td>\
<td class="settings_select_cell">\
<select id="sort_crit" class="settings_select">';

    var sort_crit = 
    [ 
      { desc:0, asc:5, label: "<? loc SORT_CRIT_REL ?>" }, 
      { desc:1, asc:2, label: "<? loc SORT_CRIT_PUB ?>" }, 
      { desc:3, asc:4, label: "<? loc SORT_CRIT_FETCH ?>" }, 
      { desc:6, asc:7, label: "<? loc SORT_CRIT_EVENT ?>" }, 
      { desc:8, asc:9, label: "<? loc SORT_CRIT_POP ?>" }
    ];

    for(var i = 0; i < sort_crit.length; i++)
    {
      var opt = sort_crit[i];
    
      text += "<option id=\"sc_" + opt.desc + "_" + opt.asc + "\" " + 
              (page.search.sorting_type == opt.desc || 
               page.search.sorting_type == opt.asc ? 
               "selected=\"true\"" : "") + ">" + opt.label + 
              "</option>";
    }

    text += '</select></td></tr><tr><td>\
<label for="sort_dir"><? loc SORT_DIR ?></label></td>\
<td class="settings_select_cell">\
<select id="sort_dir" class="settings_select">';

    var sort_dir = 
    [ 
      { shift:1,
        label:"<? loc SORT_DIR_DESC ?>",
        sort_types:[0, 1, 3, 6, 8]
      },
      { shift:2,
        label:"<? loc SORT_DIR_ASC ?>",
        sort_types:[5, 2, 4, 7, 9]
      }
    ];

    for(var i = 0; i < sort_dir.length; i++)
    {
      var opt = sort_dir[i];
    
      text += "<option id=\"sd_" + opt.shift + "\" " + 
              (el_index_of(opt.sort_types, page.search.sorting_type) >= 0 ? 
              "selected=\"true\"" : "") + ">" + opt.label + 
              "</option>";
    }

    text += '</select></td></tr></table>\
<div class="settings_dialog_option_group"><? loc REPR_CONF ?></div>\
<table class="settings_dialog_option_table" cellspacing="0">'

    text += '<tr><td>\
<label for="mvw_settings"><? loc MESSAGE_VIEW ?></label></td>\
<td class="settings_select_cell">\
<select id="mvw_settings" class="settings_select" onchange="el_dlg_by_id(\'' +
           this.id + '\').message_view_changed()">';

    var msg_views = [ {n:"<? loc MV_PAPER ?>", v:"paper"},
                      {n:"<? loc MV_LINE ?>",  v:"nline"}
                    ];

    for(var i = 0; i < msg_views.length; i++)
    {
      var mv = msg_views[i];
  
      text += "<option id=\"rp_" + mv.v + "\" " + 
              (page.search.message_view == mv.v ? 
               "selected=\"true\"" : "") + ">" + mv.n + "</option>";
    }

    text += "</select></td></tr>";

    text += '<tr><td>\
<label for="rpp_settings"><? loc RESULTS_PER_PAGE ?></label></td>\
<td class="settings_select_cell">\
<select id="rpp_settings" class="settings_select">';

    var results_per_page_options = [ 10, 20, 30, 50, 100 ];

    for(var i = 0; i < results_per_page_options.length; i++)
    {
      var rp = results_per_page_options[i];
  
      text += "<option id=\"rp_" + rp + "\" " + 
              (page.search.results_per_page == rp ? 
               "selected=\"true\"" : "") + ">" + rp + "</option>";
    }

    text += "</select></td></tr>";

    text += '<tr id="aln_row"><td>\
<label for="aln_settings"><? loc ANNOTATION_LEN ?></label></td>\
<td class="settings_select_cell">\
<select id="aln_settings" class="settings_select">';

    var annot_len_options = [ {n:0, v:0}, 
                              {n:100, v:100}, 
                              {n:200, v:200}, 
                              {n:300, v:300}, 
                              {n:400, v:400}, 
                              {n:500, v:500}, 
                              {n:600, v:600}, 
                              {n:700, v:700}, 
                              {n:800, v:800}, 
                              {n:900, v:900}, 
                              {n:"<? loc ANNOTATION_FULL ?>", v:"F"} 
                            ];

    var sel_val = page.search.desc_length < 0 ? 
      "F" : page.search.desc_length;

    var sel_index = annot_len_options.length - 1;

    for(var i = 0; i < annot_len_options.length; i++)
    {
      var rp = annot_len_options[i];

      if(rp.v == sel_val)
      {
        sel_index = i;
        break;
      }
    }

    for(var i = 0; i < annot_len_options.length; i++)
    {
      var rp = annot_len_options[i];
    
      text += "<option id=\"rp_" + rp.v + "\" " + 
              (sel_index == i ? 
               "selected=\"true\"" : "") + ">" + rp.n + "</option>";
    }

    text += "</select></td></tr>";

    bool_settings = 
    [ 
      { param:"c", 
        on:"2", 
        off:"1", 
        label: "<? loc 2_COLUMNS ?>", 
        def: page.site.version.def == page.site.version.MOBILE ? 0 : 1
      },

      { param:"p", on:"l", off:"s", label: "<? loc LARGE_PRINT ?>", def: 0 },
      { param:"i", on:"1", off: "0", label: "<? loc IMAGES ?>", def: 1 }
//      ,{ param:"plb", on:"1", off: "0", label: "<? loc TOP_STORIES ?>", def: 0 }
    ];

    for(var i = 0; i < bool_settings.length; i++)
    {
      text += this.setting_checkbox(bool_settings[i]);            
    }

    text +='</table>';

    if(page.search_mail != undefined)
    {
      text +='<div class="settings_dialog_option_group"><? loc SUBS_CONF ?>\
</div><table class="settings_dialog_option_table" cellspacing="0">\
<tr><td><label for="sort_crit"><? loc SUBS_EMAIL ?></label></td>\
<td class="settings_select_cell">\
<input id="mailer_email" class="settings_edit" type="' + 
       (page.site.version.current == page.site.version.MOBILE ? 
        "email" : "text") + '" value="' +
el_xml_encode(page.search_mail.email) + 
'"></td></tr><tr><td class="settings_select_cell" colspan="2">\
<a href=\'javascript:SearchSettingsDialog.prototype.manage_subscriptions()\'>\
<? loc SUBS_EDIT ?></a></td></tr></table>';
    }

    return text;
  },

  manage_subscriptions: function()
  {
    var path = page.search.path;
    var pos = path.lastIndexOf("/");

    navigate(
      path.substring(0, pos) + "/e?sm_o=g&sm_e=" +
        el_mime_url_encode(el_by_id("mailer_email").value.toLowerCase()), 
      true);
  },

  setting_checkbox: function(opt)
  {
    var id = "bo_" + opt.param + "_" + opt.on + "_" + opt.off + "_" + 
        opt.def;

    var text = "<tr id='bo_" + opt.param + 
               "'><td colspan='2'><input type='checkbox' id='" + id + 
               "' name='bool_settings' class='settings_checkbox'";

    if(page.search.view_option[opt.param] == opt.on)
    {
      text += " checked=\"checked\"";
    }

    text += "/><label for='" + id + "'>" + opt.label + "</label></td></tr>";

    return text;
  },

  message_view_changed: function()
  {
    var index = el_by_id("mvw_settings").selectedIndex == 0 ?
      0 : 3;

    el_by_id("rpp_settings").selectedIndex = index;

    this.set_view_options_visibility();
  },

  site_version_changed: function()
  {
    this.set_view_options_visibility();
  },

  set_view_options_visibility: function()
  {
    var mobile = false;
    var select = el_by_id("sv_settings");
    
    if(select.selectedIndex >= 0)
    {
      var version = 
        parseInt(select.options[select.selectedIndex].id.split("_", 2)[1]);

      mobile = version == page.site.version.MOBILE;
    }

    var paper = el_by_id("mvw_settings").selectedIndex == 0;

    el_by_id("bo_c").style.display = mobile || !paper ? "none" : "table-row";
    el_by_id("aln_row").style.display = paper ? "table-row" : "none";
    el_by_id("bo_p").style.display = paper ? "table-row" : "none";
    el_by_id("bo_i").style.display = paper ? "table-row" : "none";
//    el_by_id("bo_plb").style.display = mobile ? "none" : "table-row";
  },

  default_settings: function()
  {
    this.el_call(SettingsDialog, "default_settings");

    var mobile = page.site.version.def == page.site.version.MOBILE;

    el_by_id("sort_crit").selectedIndex = 0;
    el_by_id("sort_dir").selectedIndex = 0;
    el_by_id("rpp_settings").selectedIndex = 0;

    var settings = el_by_id("mvw_settings");

    if(settings)
    {
      settings.selectedIndex = 0;
    }

    var obj = el_by_id("aln_settings");
    obj.selectedIndex = mobile ? 4 : obj.length - 1;
  
    var elems = el_by_name("bool_settings");

    for(var i = 0; i < elems.length; i++)
    {
      var elem = elems[i];
      var opt_vals = elem.id.split("_", 5);
      elem.checked = opt_vals[4] == "1";
    }

    this.set_view_options_visibility();
  },

  on_ok: function()
  {
    var params = "";

    settings = el_by_id("sort_dir");
    var opt = settings.options[settings.selectedIndex];
    var sort_dir = opt.id.split("_", 2)[1];

    settings = el_by_id("sort_crit");
    opt = settings.options[settings.selectedIndex];
    var sort_mode = opt.id.split("_", 3)[sort_dir];

    params += "a=s-" + sort_mode;

    page.search.query = 
      extract_param("a=", 
                    extract_param("slg=", page.search.query).query).query;

    settings = el_by_id("mvw_settings");

    if(settings && settings.selectedIndex >= 0)
    {
      opt = settings.options[settings.selectedIndex];
      var mvw = opt.id.split("_", 2)[1];
      params += "&mvw=s-" + mvw;
      page.search.query = extract_param("mvw=", page.search.query).query;
    }

    settings = el_by_id("rpp_settings");

    if(settings.selectedIndex >= 0)
    {
      opt = settings.options[settings.selectedIndex];
      var rpp = opt.id.split("_", 2)[1];
      params += "&r=s-" + rpp;
      page.search.query = extract_param("r=", page.search.query).query;
    }

    settings = el_by_id("aln_settings");

    if(settings.selectedIndex >= 0)
    {
      opt = settings.options[settings.selectedIndex];
      var val = opt.id.split("_", 2)[1];
      params += "&dl=s-" + (val == "F" ? -1 : val);
      page.search.query = extract_param("dl=", page.search.query).query;
    }

    elems = el_by_name("bool_settings");

    for(var i = 0; i < elems.length; i++)
    {
      var elem = elems[i];
      var opt_vals = elem.id.split("_", 5);
      var optv = opt_vals[1] + "=";

      params += "&" + optv + "s-" + 
        el_mime_url_encode(elem.checked ? opt_vals[2] : opt_vals[3]);

      page.search.query = extract_param(optv, page.search.query).query;
    }

    if(page.search_mail != undefined)
    {
      params += "&sm_e=s-" + el_by_id("mailer_email").value.toLowerCase();
    }

    this.el_call(SettingsDialog, "on_ok", params);
  }

});

function show_settings_dialog(extra_params, on_destroy)
{
  var dlg = new SearchSettingsDialog(extra_params, on_destroy);
}

/////////////////////////////////////////////////////////////////////////////
// SearchMailDialog
/////////////////////////////////////////////////////////////////////////////

var smd_recaptcha_created = false;
var smd_recaptcha_loaded = false;

function on_smd_recaptcha_load()
{
  smd_recaptcha_loaded = true;

  if(SearchMailDialog.dlg)
  {
    SearchMailDialog.dlg.on_recaptcha_load();
  }
}

function smd_recaptcha_callback(response)
{
  if(SearchMailDialog.dlg)
  {
    SearchMailDialog.dlg.recaptcha_callback(response);
  }
}

function on_smd_destroy()
{
  SearchMailDialog.dlg = null;
}

function SearchMailDialog()
{
  FullScreenDialog.call(this, "", on_smd_destroy);
}

el_typedef(SearchMailDialog, FullScreenDialog,
{
  singleton: true,
  time_num: 0,
  close_name: null,
  ok_name: null,
  recaptcha_resp: "",
  message_count: [10, 20, 30, 50, 100],

  on_recaptcha_load: function()
  { 
    this.recaptcha_render();    
  },

  recaptcha_render: function()
  { 
    grecaptcha.render(
      "mailer_recaptcha_widget", 
      { 'sitekey' : page.search_mail.key,
        'callback' : smd_recaptcha_callback
      });

    el_by_id("mailer_recaptcha_widget").focus();
  },

  recaptcha_callback: function(response)
  {
    this.recaptcha_resp = response;
    this.save();
  },

  on_create: function()
  {
    var times = page.search_mail.times;

    if(times.length)
    {
      for(var i = 0; i < times.length; ++i)
      {
        var t = times[i];
        this.add_time(t.d, t.t);
      }
    }
    else
    {
      this.add_time();
    }

    if(page.site.version.current != page.site.version.MOBILE)
    {
      el_by_id("mailer_email").focus();
    }
  },

  init: function()
  { 
    SearchMailDialog.dlg = this;

    var inner = this.el_call(FullScreenDialog, "init");

    var formats = [ "<? loc MAILER_FORMAT_HTML ?>",
                    "<? loc MAILER_FORMAT_TEXT ?>",
                    "<? loc MAILER_FORMAT_BOTH ?>"
                  ];

    var text =
'<div id="mailer_editor">\
<table class="settings_dialog_option_table" cellspacing="0" id="mailer_table">\
<tr><td><label for="mailer_email"><? loc MAILER_EMAIL_LABEL ?></label></td>\
<td class="settings_select_cell">\
<input id="mailer_email" type="' + 
       (page.site.version.current == page.site.version.MOBILE ? 
        "email" : "text") + '" class="settings_edit" value="' +
el_xml_encode(page.search_mail.email) + '"/>\
</td></tr>\
<tr><td><label for="mailer_title"><? loc MAILER_TITLE_LABEL ?></label></td>\
<td class="settings_select_cell">\
<input id="mailer_title" type="text" class="settings_edit" value="' +
el_xml_encode(page.search_mail.title) + '"></td></tr>\
<tr><td><label for="mailer_messages"><? loc MAILER_MESSAGE_COUNT ?></label></td>\
<td class="settings_select_cell">\
<select id="mailer_messages" class="settings_select">';
    
    for(var i = 0; i < this.message_count.length; ++i)
    {
      var msgs = this.message_count[i];

      text += '<option' + 
        (msgs == page.search_mail.messages ? ' selected="true"' : '') + 
        '>' + msgs + '</option>';
    }

    text += '</select></td></tr>\
<tr><td><label for="mailer_format"><? loc MAILER_FORMAT_LABEL ?></label></td>\
<td class="settings_select_cell">\
<select id="mailer_format" class="settings_select">'
    
    for(var i = 0; i < formats.length; ++i)
    {
      text += '<option' + 
        (i == page.search_mail.format ? ' selected="true"' : '') + 
        '>' + formats[i] + '</option>';
    }

    text += "</select></td></tr>\
<tr><td colspan='2'><label for='mailer_format'><? loc MAILER_TIME_LABEL ?></label>\
<span id='mailer_add_time'>(\
<a href='javascript:el_dlg_by_id(\"" + this.id + "\").add_time()'>\
<? loc MAILER_ADD_TIME ?></a>)</span></td></tr></table>\
<table cellspacing='0' id='mailer_time_table'>\
</table>\
<div class='mailer_manage_link'>\
<a href='javascript:SearchSettingsDialog.prototype.manage_subscriptions()'>\
<? loc SUBS_EDIT ?></a></div>";

    text += "<button onclick='el_dlg_by_id(\"" + 
            this.id + "\").save()'>" + 
            (page.search_mail.id ? "<? loc SAVE_SEARCH_MAIL ?>" : 
             "<? loc CREATE_SEARCH_MAIL ?>") + "</button>&nbsp;\
<button onclick='el_close_dlg_by_id(\"" + this.id + 
          "\", false)'><? loc CANCEL_SEARCH_MAIL ?></button>\
            <div id='mailer_error' class='error'></div></div>\
          <div id='mailer_recaptcha' style='display:none'>\
          <div id='mailer_recaptcha_msg' class='mailer_msg'></div>\
          <div id='mailer_recaptcha_widget'></div>\
          <button type='button' onclick='el_close_dlg_by_id(\"" + this.id + 
          "\", false)'><? loc CANCEL_SEARCH_MAIL ?></button></div>\
          <div id='mailer_saver' style='display:none'>\
          <div id='mailer_saver_msg' class='mailer_msg'></div>\
          <button onclick='el_close_dlg_by_id(\"" + this.id + 
          "\", false)'><? loc CLOSE_SEARCH_MAIL ?></button></div>";

    inner.text = text;

    return inner;
  },

  add_time: function(day, tm)
  {
    var table = el_by_id("mailer_time_table");
    var row = table.insertRow(-1);
    row.id = "mailer_time_" + this.time_num;

    var cell = row.insertCell(0);
    cell.colSpan = 2;

    var id = "mailer_hr_" + this.time_num;
    var text = '<select id="' + id + '" class="mailer_tm">';

    if(day)
    {
      var d = new Date()
      var to = d.getTimezoneOffset();

      var mins = tm - to;

      if(mins < 0)
      {
        if(--day == 0)
        {
          day = 7;
        }
      }
      else if(mins >= 1440)
      {
        if(++day == 8)
        {
          day = 1;
        }
      }
    }

    var time = new Date();
    time.setTime(tm === undefined ? 
                 (time.getTime() + 60 * 10 * 1000) : (tm * 60 * 1000));

    for(var i = 0; i < 24; ++i)
    {    
      text += "<option " + (i == time.getHours() ? 
              'selected="true"' : '') + 
              ">" + (i < 10 ? "0" : "") + i + "</option>";
    }

    id = "mailer_mn_" + this.time_num;
    text += '</select>:<select id="' + id + '" class="mailer_tm">';

    var mins = Math.floor(time.getMinutes() / mail_subs_ti);

    for(var i = 0; i < 60 / mail_subs_ti; ++i)
    {    
      var m = i * mail_subs_ti;

      text += "<option " + (i == mins ? 'selected="true"' : '') + ">" + 
              (m < 10 ? "0" : "") + m + "</option>";
    }

    id = "mailer_dy_" + this.time_num;
    text += '</select>&nbsp;<select id="' + id + '" class="mailer_dy">';

    var days = [ "<? loc MAILER_DAILY ?>", "<? loc MAILER_SUN ?>", 
                 "<? loc MAILER_MON ?>", "<? loc MAILER_TUE ?>", 
                 "<? loc MAILER_WED ?>", "<? loc MAILER_THU ?>", 
                 "<? loc MAILER_FRI ?>", "<? loc MAILER_SAT ?>" ];

    var sel_day = day ? day : 0;

    for(var i = 0; i < days.length; ++i)
    {    
      text += "<option " + (i == sel_day ? 'selected="true"' : '') + ">" + 
              days[i] + "</option>";
    }

    text += '</select>&nbsp;<a href="javascript:el_dlg_by_id(\'' + 
            this.id + '\').del_time(' + this.time_num + 
            ')"><img width="12" height="12" src="' + 
            page.search_mail.del_img + '" class="mailer_tm_del"></a>';

    cell.className = "settings_select_cell";
    cell.innerHTML = text;

    ++this.time_num;

    this.add_time_display();
  },

  del_time: function(num)
  {
    var id = "mailer_time_" + num;

    var table = el_by_id("mailer_time_table");
    var rows = table.rows;

    for(var i = 0; i < rows.length; ++i)
    {
      if(rows[i].id == id)
      {
        table.deleteRow(i);
        break;
      }
    }

    this.add_time_display();
  },

  add_time_display: function(num)
  {
    el_by_id("mailer_add_time").style.display = 
      el_by_id("mailer_time_table").rows.length < 24 ? "inline" : "none";
  },

  save : function()
  {
    var email = el_by_id("mailer_email").value.toLowerCase();
    var title = el_by_id("mailer_title").value;
    var format = el_by_id("mailer_format").selectedIndex;

    var length = el_by_id("mailer_messages").selectedIndex;
    length = length < 0 ? 100 : this.message_count[length];

    var d = new Date()
    var to = d.getTimezoneOffset();

    var params = "sm_o=u&sm_i=" + 
      el_mime_url_encode(page.search_mail.id) + 
      "&sm_e=s-" + el_mime_url_encode(email) +
      "&sm_t=" + el_mime_url_encode(title) + 
      "&sm_f=s-" + format + "&sm_h=s-" + length + "&sm_z=" + to;

    var times = [];

    for(var i = 0; i < this.time_num; ++i)
    {
      var hr = el_by_id("mailer_hr_" + i);

      if(hr)
      {
        var mn = el_by_id("mailer_mn_" + i);
        var day = el_by_id("mailer_dy_" + i).selectedIndex;

        var mins = hr.selectedIndex * 60 + mn.selectedIndex * mail_subs_ti + to;

        if(day)
        {
          if(mins < 0)
          {
            if(--day == 0)
            {
              day = 7;
            }
          }
          else if(mins >= 1440)
          {
            if(++day == 8)
            {
              day = 1;
            }
          }
        }

        mins = (mins  + 1440) % 1440;

        params += "&sm_m=" + day + "-" + mins;
        times.push({ d:day, t:mins });
      }
    }

    params += "&" + page.search.query;

    if(this.recaptcha_resp)
    {
      params += "&sm_r=" + el_mime_url_encode(this.recaptcha_resp)
    }

    var path = page.search.path;
    var pos = path.lastIndexOf("/");
    path = path.substring(0, pos) + "/e";

    el_by_id("mailer_saver_msg").innerHTML = "<? loc MAILER_SAVING ?>";
    el_by_id("mailer_editor").style.display = "none";
    el_by_id("mailer_recaptcha").style.display = "none";
    el_by_id("mailer_saver").style.display = "block";

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
      email: email,
      title: title,
      format: format,
      length: length,
      times: times,
      dlg: this
    };

    request.onreadystatechange = 
     this.get_search_mailer_req_handler(data);    

    request.open("POST", path, true)

    request.setRequestHeader("Content-type", 
                             "application/x-www-form-urlencoded");
  
    request.send(params);
  },

  get_search_mailer_req_handler: function(data)
  {
    var onready = function()
    {
      if(data.req.readyState == 4)
      {
        data.dlg.set_mailer_status(data.req, 
                                   data.email, 
                                   data.title, 
                                   data.format,
                                   data.length,
                                   data.times);
      }
    }

    return onready;
  },

  set_mailer_status: function(req, email, title, format, length, times)
  {
    if(this.closed)
    {
      return;
    }

    var success = req.status == 200;
    var recaptcha = false;
    var error = "";

    if(success)
    {
      var response = eval("( " + req.responseText + ")")
      success = response.success
      recaptcha = response.recaptcha;

      if(success)
      {
        el_by_id("mailer_saver_msg").innerHTML = response.info;
        page.search_mail.email = email;
        page.search_mail.title = title;
        page.search_mail.format = format;
        page.search_mail.messages = length;
        page.search_mail.times = times;
      }
      else
      {
        error = response.info;
      }
    }
    else
    {
      error = "<? loc MAILER_SRV_ERR ?> " + req.status +
        ". <? loc MAILER_TRY_LATER ?>";
    }
    
    if(!success)
    {
      if(recaptcha)
      {
        el_by_id("mailer_recaptcha_msg").innerHTML = el_xml_encode(error);
        el_by_id("mailer_editor").style.display = "none";
        el_by_id("mailer_saver").style.display = "none";
        el_by_id("mailer_recaptcha").style.display = "block";

        if(smd_recaptcha_created)
        {
          if(smd_recaptcha_loaded)
          {
            this.recaptcha_render();
          }
        }
        else
        {
          var url = 'https://www.google.com/recaptcha/api.js?hl=' + 
            page.search_mail.lang +
            '&onload=on_smd_recaptcha_load&render=explicit';

          var head = document.getElementsByTagName('head').item(0);
          var script = document.createElement('script');
          script.setAttribute('type', 'text/javascript');
          script.setAttribute('src', url);
          head.appendChild(script);
          smd_recaptcha_created = true;
        }
      }
      else
      {
        el_by_id("mailer_error").innerHTML = el_xml_encode(error);
        el_by_id("mailer_saver").style.display = "none";
        el_by_id("mailer_recaptcha").style.display = "none";
        el_by_id("mailer_editor").style.display = "block";
      }
    }
  }
});

function show_mail_dialog()
{
  var dlg = new SearchMailDialog();
}
