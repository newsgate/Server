var mod_top_bar = "";
var mod_privileges = "";
var focus_edit = null;
var last_focus_edit = null;
var refresh_period = 60;
var last_refresh = new Date();

function translate_query(text, lang)
{
  var url = "http://www.google.com/translate_t?q=" + el_mime_url_encode(text);

  if(lang)
  {
    url += "&sl=" + lang;
  }

  open_window(url);
}

function google_query(text)
{
  var url = "http://www.google.com/search?q=" + el_mime_url_encode(text);
  open_window(url, "Request is too long. Select a smaller text.");
}

function newsgate_query(exp, relax_search_level)
{
  var url = "/psp/search?q=" + el_mime_url_encode(exp) + 
            "&mod_init=mod_init&mod_script=" +
            el_mime_url_encode("/fixed/js/category/mod_script.js") + 
            "&mod_css=" + 
            el_mime_url_encode("/fixed/css/category/mod_css.css");

  if(search_menu)
  {
    url += "&" + search_menu;
  }

  if(relax_search_level)
  {
    url += "&rl=" + relax_search_level;
  }

  open_window(url);
}

function mod_edit_set_focus(id, set)
{
  var obj = document.getElementById(id);

  if(set)
  {
    focus_edit = obj;
  }
  else if(focus_edit == obj)
  {
    focus_edit = null;
  }
}

function mod_base_mouse_pressed(e)
{
  refresh_session();

  var evt = e || window.event;
  var el = evt.target ? evt.target : evt.srcElement;

  last_focus_edit = el.className == "mod_menu_link" ? focus_edit : null;
}

function mod_base_key_pressed(e) 
{ 
  refresh_session();
}

function get_phrases(text)
{
  text = text.replace(/\r/g, "").replace(/\n|\t/g, " ");

  var words = text.split(" ");

  var quoting = "";
  var phrase = "";
  var phrases = new Array();

  for(var i = 0; i < words.length; i++)
  {
    var w = words[i];

    if(w.length == 0)
    {
      continue;
    }

    var chr = w.charAt(0);

    if(quoting == "" && (chr == "'" || chr == "\""))
    {
      quoting = chr;
      w = w.substr(1, w.length - 1);

      if(w == "")
      {
        continue;       
      }
    }

    if(quoting == "")
    {
      phrases[phrases.length] = w + " ";
      continue;
    }

    var chr = w.charAt(w.length - 1);

    if(chr == quoting)
    {
      quoting = "";
      w = w.substr(0, w.length - 1);
    }

    if(w != "")
    {
      if(phrase != "")
      {
        phrase += " ";
      }

      phrase += w;
    }

    if(quoting == "")
    {
      if(phrase != "")
      {
        phrases[phrases.length] = phrase + chr;
      }

      phrase = "";
    }
  }

  if(phrase.length > 1)
  {
    phrases[phrases.length] = phrase + " ";
  }
  
  return phrases;
}

function phrases_text(phrases)
{
  var text = "";

  for(var i = 0; i < phrases.length; i++)
  {
    if(text != "")
    {
      text += "\n";
    }

    var phrase = phrases[i];
    var len = phrase.length - 1;
    var chr = phrase.charAt(len);

    if(chr == " ")
    {
      text += phrase.substr(0, len);
    }
    else
    {
      text += chr + phrase;
    }
  }

  return text;
}

function get_selected_phrases()
{
  var sel = null;
  var text = "";
  var sel_text = "";

  if(last_focus_edit)
  {
    sel = last_focus_edit.el_get_selection();
    text = last_focus_edit.value.replace(/\r/g, "");
    sel_text = text.substr(sel.start, sel.end - sel.start);
  }

  if(sel_text == "")
  {
    alert("No words selected");
    return new Array();
  }

  return get_phrases(sel_text);
}

function set_phrases(phrases)
{
  if(last_focus_edit == null)
  {
    return;
  }

  var sel = last_focus_edit.el_get_selection();
  var text = last_focus_edit.value.replace(/\r/g, "");
  var sel_text = text.substr(sel.start, sel.end - sel.start);

  var prefix = "";
  var suffix = "";

  if(sel_text.charAt(0) == "\n")
  {
    prefix = "\n";
  }

  if(sel_text.charAt(sel_text.length - 1) == "\n")
  {
    suffix = "\n";
  }

  text = prefix + phrases_text(phrases) + suffix;

  last_focus_edit.el_insert(text, sel, false);

  sel.end = sel.start + text.length;
  last_focus_edit.el_set_selection(sel, true, false);

  last_focus_edit.focus();
}

function word_cmp(s1, s2)
{
  var a = s1.toLowerCase();
  var b = s2.toLowerCase();

  return a == b ? 0 : a < b ? -1 : 1;
}

function sort_words(text)
{
  if(text === undefined)
  {
    var phrases = get_selected_phrases();

    if(phrases.length > 0)
    {
      phrases.sort(word_cmp);
      set_phrases(phrases);
    }
  }
  else
  {
    var phrases = get_phrases(text);
    phrases.sort(word_cmp);
    text = phrases_text(phrases);    
  }

  return text;
}

function capitalize_words()
{
  var phrases = get_selected_phrases();

  if(phrases.length > 0)
  {
    for(var i = 0; i < phrases.length; i++)
    {
      var phrase = phrases[i].split(" ");
      var new_phrase = "";

      for(var j = 0; j < phrase.length; j++)
      {
        var word = phrase[j];
        word = word.charAt(0).toUpperCase() + word.slice(1).toLowerCase();

        if(new_phrase != "")
        {
          new_phrase += " ";
        }

        new_phrase += word;
      }

      phrases[i] = new_phrase;
    }
    
    set_phrases(phrases);
  }
}

function dedup_words(text)
{
  var phrases = text === undefined ? 
    get_selected_phrases() : get_phrases(text);

  if(phrases.length > 0)
  {
    var lower_phrases = new Array();

    for(var i = 0; i < phrases.length; i++)
    {
      lower_phrases[i] = phrases[i].toLowerCase();
    }

    var dedup_phrases = new Array();

    for(var i = 0; i < lower_phrases.length; i++)
    {
      var phrase = lower_phrases[i];

      for(var j = 0; j < i; j++)
      {
        if(lower_phrases[j] == phrase)
        {
          break;
        }
      }

      if(j == i)
      {
        dedup_phrases[dedup_phrases.length] = phrases[i];
      }
    }

    if(text === undefined)
    {
      set_phrases(dedup_phrases);
    }
    else
    {
      text = phrases_text(dedup_phrases)
    }
  }

  return text;
}

function toupper_words(upper)
{
  var sel = null;
  var text = "";
  var sel_text = "";

  if(last_focus_edit)
  {
    sel = last_focus_edit.el_get_selection();
    text = last_focus_edit.value.replace(/\r/g, "");
    sel_text = text.substr(sel.start, sel.end - sel.start);
  }

  if(sel_text == "")
  {
    alert("No words selected");
    return;
  }

  last_focus_edit.el_insert(
    upper ? sel_text.toUpperCase() : sel_text.toLowerCase(), sel, false);  

  sel.end = sel.start + sel_text.length;
  last_focus_edit.el_set_selection(sel, true, false);
}

function mod_base_add_params(url)
{
  var u = new El_Url(url);

  u.params += "&mod_params=priv-" + mod_privileges.length + "-" + 
              el_mime_url_encode(mod_privileges) +
              "topbar-" + mod_top_bar.length + "-" + 
              el_mime_url_encode(mod_top_bar);

  return u.uri();
}

function mod_extract_param(params, name)
{
  name += "-";
  var name_len = name.length;

  var result = { value:null, params:params};

  if(params.substr(0, name_len) == name)
  {
    params = params.substr(name_len);
    var len = params.indexOf('-');

    if(len > 0)
    {
      var nlen = params.substr(0, len);
      params = params.substr(len + 1);
      var len = parseInt(nlen);

      result.value = params.substr(0, len);
      result.params = params.substr(len);
    }
  }

  return result;
}

function mod_base_init(params)
{
  el_attach_event(document, "mousedown", mod_base_mouse_pressed);
  el_attach_event(document, "keydown", mod_base_key_pressed);

  if(params == null)
  {
    return null;
  }

  el.mod_prepost = mod_base_add_params;

  var res = mod_extract_param(params, "priv");
  params = res.params;
  mod_privileges = res.value == null ? "" : res.value;

  res = mod_extract_param(params, "topbar");
  params = res.params;
  mod_top_bar = res.value == null ? "" : res.value;

  var top_bar = document.getElementById("top_bar");

  if(top_bar != null)
  {
    var row = top_bar.insertRow(0);
    var cell = row.insertCell(0);
    cell.colSpan = 4;
    cell.innerHTML = mod_top_bar;
  }

  var spans = document.getElementsByTagName("span");

  for(var i = 0; i < spans.length; i++) 
  {
    var obj = spans[i];

    if(obj.className == "mod_topbar_current_page_name" && 
       el_text(obj) == "Search")
    {
      obj.innerHTML = '<a href="javascript:search(true)">Search</a>';
      break
    }
  }

  return params;  
}

function normalize_text(text, multiline)
{
  if(multiline)
  {
    var lines = text.split('\n');
    text = ""

    for(var i = 0; i < lines.length; ++i)
    {
      var l = el_trim(lines[i].replace(/(\s+)/g, " "));

      if(l != "")
      {
        if(text != "")
        {
          text += '\n';
        }

        text += l;
      }
    }
  }
  else
  {
    text = el_trim(text.replace(/(\s+)/g, " "));
  }

  return text;
}

function get_selection(multiline, full_text)
{
  var text = "";

  if(last_focus_edit)
  {
    var sel = last_focus_edit.el_get_selection();
    text = last_focus_edit.value.replace(/\r/g, "");

    if(sel.start != sel.end)
    {
      text = text.substr(sel.start, sel.end - sel.start);
    }
    else if(!full_text)
    {
      text = "";
    }
  }
  else
  {
    text = el_trim(new String(el_get_selection()));
  }

  return normalize_text(text, multiline);
}

function refresh_session()
{
  var cur_time = new Date();

  if(cur_time - last_refresh < refresh_period * 1000)
  {
    return;
  }

  last_refresh = cur_time;

  var request;

  try
  {
    request = new ActiveXObject("Msxml2.XMLHTTP");
  }
  catch(e)
  {
    request = new XMLHttpRequest();
  }

  request.open("GET", "/psp/refresh_session", true);
  request.send("");
}

//
// Find moderation item
//

function FindDialog(parent, on_destroy, text)
{
  this.on_destroy = on_destroy;
  this.text = text;

  ElDialog.call(this, parent);
}

el_typedef(FindDialog, ElDialog,
{
  singleton: true,

  init: function() 
  {
    el_close_all_dialogs();

    this.last_find_num = 0;
    this.focus_on_close = last_focus_edit;
    this.sel_on_close = el_save_selection();

    var lines = 
      (this.text ? this.text : get_selection(true, false)).split('\n');

    var selection = ""

    for(var i = 0; i < lines.length; ++i)
    {
      var l = el_trim(lines[i].replace(/(^')|('$)|(^")|("$)/g, ""));

      if(l != "")
      {
        if(selection != "")
        {
          selection += '\n';
        }

        selection += l;
      }
    }

    var text = 
      '<table style="font-size:100%" cellspacing="0"><tr valign="top"><td>\
<textarea id="' + 
      this.id + '_edit" class="mod_find_edit" rows="3">' + 
      el_xml_encode(selection) + 
      '</textarea></td><td><input type="button" onclick="el_dlg_by_id(\'' +
      this.id + '\').find();" value="Find"></td></tr></table><div id="' + 
      this.id + '_result" style="padding:0.3em 0 0"></div>';

    return text;
  },

  on_create: function()
  {
    this.find();
  },
  
  find: function()
  {
    var edit = el_by_id(this.id + '_edit');
    edit.focus();

    var text = normalize_text(edit.value, true);
    el_by_id(this.id + '_result').innerHTML = "Searching ...";
  
    var request;

    try
    {
      request = new ActiveXObject("Msxml2.XMLHTTP");
    }
    catch(e)
    {
      request = new XMLHttpRequest();
    }

    var data = { req:request, dlg:this, num:++this.last_find_num };
    request.onreadystatechange = this.get_find_req_handler(data);

    var params = "q=" + encodeURIComponent(text);

    request.open("POST", "/psp/find", true);

    request.setRequestHeader("Content-type", 
                             "application/x-www-form-urlencoded");
  
    request.send(params);
  },

  get_find_req_handler: function(data)
  {
    var onready = function()
    { 
      if(data.req.readyState == 4)
      {
        switch(data.req.status)
        {
          case 200:
          {
            if(data.dlg.last_find_num == data.num)
            {
              data.dlg.found(data.req);
            }

            break;
          }
          default:
          { 
            el_by_id(this.id + '_result').innerHTML = 
              "Server communication failure. Error code: " + data.req.status;

            break;
          }
        }
      }
    }

    return onready;
  },

  found: function(req)
  {
    var inner = "";
    var result = el_child_node(req.responseXML, "result");
    var categories = el_child_node(result, "categories").childNodes;

    for(var i = 0; i < categories.length; ++i)
    {
      var cat = categories[i];

      if(cat.tagName != "cat")
      {
        continue;
      }
    
      inner += '<div class="mod_find_cat"><a href="/psp/category/update?c=' + 
               cat.getAttribute("id") + '" target="_blank">' + 
               el_xml_encode(cat.getAttribute("path")) + '</a></div>';

      var word_lists = cat.childNodes;
    
      for(var j = 0; j < word_lists.length; ++j)
      {
        var wl = word_lists[j];

        if(wl.tagName != "wl")
        {
          continue;
        }

        var wl_link = '/psp/category/update?c=' + 
                      cat.getAttribute("id") + '&wl=' + 
                      el_mime_url_encode(wl.getAttribute("name"));

        inner += '<div class="mod_find_wl"><a href="' + wl_link +
                 '" target="_blank">' + 
                 el_xml_encode(wl.getAttribute("name")) +
                 '</a></div>';

        var words = wl.childNodes;
    
        for(var k = 0; k < words.length; k++)
        {
          var w = words[k];

          if(w.tagName != "w")
          {
            continue;
          }

          var word = el_text(w);

          var from = parseInt(w.getAttribute("f"), 10);
          var to = parseInt(w.getAttribute("t"), 10);
          var start = parseInt(w.getAttribute("s"), 10);
          var end = parseInt(w.getAttribute("e"), 10);

          inner += '<div class="mod_find_word"><a href="' + wl_link +
                   '&ss=' + start + '&se=' + end +
                   '" target="_blank">' + word.substr(0, from) +
                   '<span class="mod_find_word_sel">' + 
                   word.substr(from, to - from) + '</span>' +
                   word.substr(to, word.length - to) +
                   '</a></div>';
        }

      }
    }
  
    if(inner == "")
    {
      inner = "No entries found";
    }

    el_by_id(this.id + '_result').innerHTML = inner;
  }
});

function find_selection(id, on_destroy)
{ 
  var dlg = new FindDialog(el_by_id(id), on_destroy, null);
}

function find_text(text, id, on_destroy)
{ 
  var dlg = new FindDialog(el_by_id(id), on_destroy, text);
}

//
// Word normalization
//

Word.prototype = 
{ 
  text : "",
  origin_text : "",
  selected : false,

  dump : function()
  {
    var result;

    with(this)
    {
      result = "text " + text + ", origin_text " + origin_text + 
               ", selected " + selected;
    }
  
    return result;
  }
};

function Word(text_val, origin_text_val)
{
  with(this)
  {
    text = text_val;
    origin_text = origin_text_val == undefined ? text : origin_text_val;
    selected = false;
  }
}

Lemma.prototype = 
{ 
  lang : "?",
  id : "",
  norm_form : "",
  origin_norm_form : "",
  word_forms : null,
  known : true,

  dump : function()
  {
    var result;

    with(this)
    {
      result = "norm " + norm_form + ", origin_norm " + 
               origin_norm_form + ", id " + id + ", lang " + lang + 
               (known ? ", known" : ", guessed") + ", word_forms:";

      for(var i = 0; i < word_forms.length; i++)
      {
        result += "\n" + word_forms[i].dump();
      }
    }
  
    return result;
  }
};

function Lemma(norm_form_val, 
               id_val, 
               lang_val, 
               known_val,
               origin_norm_form_val)
{
  with(this)
  {
    if(lang_val != null)
    {
      lang = lang_val;
    }

    norm_form = norm_form_val;

    origin_norm_form = 
      origin_norm_form_val == undefined ? norm_form : origin_norm_form_val;

    id = id_val;
    word_forms = new Array();
    known = known_val;
  }
}

function search_dict_lemma(number)
{
  var text = el_get_selection();
  
  if(text == "")
  {
    if(!number)
    {
      alert("No word selected");
      return;
    }

    text = document.getElementById("lmp_" + number).title;
  }

  search_expression_query(text);
}

function search_expression_query(exp, expand_lists, relax_search_level)
{
  if(expand_lists)
  {
    exp = expand_word_lists(exp);

    if(exp == null)
    {
      return;
    }
  }

  newsgate_query(exp, relax_search_level);
}

function google_dict_lemma(number)
{
  var text = el_get_selection();
  
  if(text == "")
  {
    if(!number)
    {
      alert("No word selected");
      return;
    }

    text = document.getElementById("lmp_" + number).title;
  }

  google_query(text);
}

function translate_dict_lemma(number, lang, ignore_sel)
{
  var text = ignore_sel ? "" : el_get_selection();
  
  if(text == "")
  { 
    if(!number)
    {
      alert("No word selected");
      return;
    }

    text = document.getElementById("lmp_" + number).title;
  }

  var l = lang.length > 0 && lang.charAt(lang.length - 1) == "?" ? 
    lang.slice(0, lang.length - 1) : lang;

  translate_query(text, l);
}

function capitalize(text, num)
{
  var cap = "";

  for(var i = 0; i < num; i++)
  {
    cap += text.charAt(i).toUpperCase();
  }

  return cap + text.slice(i);
}

function WordNormDialog(parent, on_destroy)
{
  ElDialog.call(this, parent);

  this.node.style.width = "45em";
  this.on_destroy = on_destroy;
}

el_typedef(WordNormDialog, ElDialog,
{
  singleton: true,

  init: function()
  {
    el_close_all_dialogs();

    this.focus_on_close = last_focus_edit;
    this.phrase_text = ""

    if(last_focus_edit)
    {
      this.phrase_text = phrases_text(get_selected_phrases())
    }
    else
    {
      alert("No words selected");
    }

    if(this.phrase_text == "")
    {
      return null;
    }

    this.text_area = last_focus_edit;
    this.phrase_range = null;
    this.range = last_focus_edit.el_get_selection();

    this.sel_text = last_focus_edit.value.replace(/\r/g, "").
      substr(this.range.start, this.range.end - this.range.start);

    var text = '<div id="' + this.id + '_inner">Loading ...</div>';
    return text;
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

    var data = { req:request, dlg:this };

    request.onreadystatechange = this.get_word_norm_req_handler(data);

    var body = "g=2&q=" + encodeURIComponent(this.phrase_text);

    var params = "m=POST&p=" + encodeURIComponent('/p/w') + "&b=" + 
      encodeURIComponent(body) + "&h=" + 
      encodeURIComponent("Content-type:application/x-www-form-urlencoded");

    request.open("POST", "/psp/proxy", true);

    request.setRequestHeader("Content-type", 
                             "application/x-www-form-urlencoded");
  
    request.send(params);
  },

  get_word_norm_req_handler: function(data)
  {
    var onready = function()
    {
      if(data.req.readyState == 4)
      {
        switch(data.req.status)
        {
          case 200:
          {
            data.dlg.fill_normalization_pane(data.req);
            break;
          }
          case 400:
          {
            data.dlg.set_error("Seems you have not selected any word");
            break;
          }
          default:
          {
            dlg.set_error("Server communication failure. Error code: " +
                          data.req.status);
            break;
          }
        }      
      }
    }

    return onready;
  },

  set_error: function(text)
  {
    var obj = el_by_id(this.id + '_inner');

    if(obj)
    {
      obj.innerHTML = '<span class="error">' + el_xml_encode(text) + '</span>';
    }
  },

  fill_normalization_pane: function(request)
  {
    var error = el_child_text(request.responseXML, "error");

    if(error != "")
    {
      this.set_error(error);
      return;
    }

    var text = 
      "<a href='javascript:show_dict_lemma(0, \"?\");'>guessed lemmas</a>\
<span id=\"lmp_0\"></span><br/><table class=\"word_forms\">";

    var result = el_child_node(request.responseXML, "result");
    var items = el_child_node(result, "items").childNodes;
    var word_index = 0;

    var phrase_start = 0;
    var phrase_quote = "";
    var dict_lemma_number = 1;

    this.phrase_range = new Array();

    for(var i = 0; i < items.length; i++)
    {
      var item = items[i];

      if(item.tagName == "known" || item.tagName == "guessed")
      {
        var known = item.tagName == "known";
        var word = item.getAttribute("w");
        var name = "wrb_" + word_index;
        var word_id = "wsp_" + word_index + "_";

        var lemmas = item.childNodes;
        var first = true;
        var word_form_index = 0;

        var radio_selected = false;
        var lemma_array = new Array();

        for(var j = 0; j < lemmas.length; j++)
        {
          var lm = lemmas[j];

          if(lm.tagName != "l")
          {
            continue;
          }

          var lemma = new Lemma(lm.getAttribute("w"),
                                lm.getAttribute("i"),
                                lm.getAttribute("n"),
                                known);

          var left_quote = "";
          var chr = word.charAt(0);
          var first_char = 0;

          if((chr == "\"" || chr == "'") && lemma.norm_form.charAt(0) != chr)
          {
            left_quote = chr;
            first_char++;

            if(phrase_quote == "")
            {
              phrase_quote = left_quote;
              phrase_start = word_index;
            }
          }

          var capitalized = 0;

          for(var k = first_char; k < word.length; k++)
          {          
            var chr = word.charAt(k);

            if(chr != chr.toUpperCase())
            {
              break;
            }

            capitalized++;
          }

          var right_quote = "";
          var chr = word.charAt(word.length - 1);

          if((chr == "\"" || chr == "'") && 
             lemma.norm_form.charAt(lemma.norm_form.length - 1) != chr)
          {
            right_quote = chr;

            if(phrase_quote == right_quote)
            {
              phrase_quote = "";
            
              if(phrase_start != word_index)
              {
                this.phrase_range[this.phrase_range.length] = 
                  { begin:phrase_start, end:word_index };
              }
            }
          }

          lemma.norm_form = left_quote + lemma.norm_form + right_quote;

          var forms = lm.childNodes;

          for(var k = 0; k < forms.length; k++)
          {
            var form = forms[k];

            if(form.tagName != "f")
            {
              continue;
            }

            var word_form = 
              new Word(left_quote + form.firstChild.data + right_quote, 
                       form.firstChild.data);


            if(radio_selected == false && lemma.norm_form == word && 
               word_form.text == word)
            {
              word_form.selected = true;
              radio_selected = true;
            }

            lemma.word_forms[lemma.word_forms.length] = word_form;
          }
  
          lemma_array[lemma_array.length] = lemma;
        }

        for(var j = 0; j < lemma_array.length && radio_selected == false; j++)
        {
          var word_forms = lemma_array[j].word_forms;

          for(var k = 0; k < word_forms.length && radio_selected == false; k++)
          {
            if(word_forms[k].text == word)
            {
              word_forms[k].selected = true;
              radio_selected = true;
            }
          }
        }

        for(var j = 0; j < lemma_array.length; j++)
        {
          text += '<tr class="option_row" valign="top">';

          if(first)
          {
            var id = name + "_" + word_form_index;
            var wid = word_id + word_form_index;

            word_form_index++;

            text += '<td class="radio_cell">';

            text += '<input type="radio" ' + 
                    (radio_selected ? '' : 'checked="checked" ') + 'name="' +
                    name + '" id="' + id + '"/>';

            text += '</td><td><input type="text" id="' + wid + '" value="' +
                    el_xml_encode(word) + 
                    '"  onfocus="document.getElementById(\''+ id +
                    '\').checked=\'checked\'"/></td>';
          }
          else
          {
            text += '<td></td><td></td>';
          }

          text += '<td><table class="lemma_forms">';

          var lemma = lemma_array[j];
          var forms = lemma.word_forms;

          for(var k = 0; k < forms.length; k++)
          {
            var form = forms[k];

            var id = name + "_" + word_form_index;
            var wid = word_id + word_form_index;

            word_form_index++;

            var word_class = "";

            if(form.text == lemma.norm_form)
            {
              word_class = ' class="norm_form"';
            }

            var radio_check = "";

            if(form.selected)
            {
              radio_check = " checked=\"checked\"";
            }

            text += '<tr><td class="radio_cell"><input type="radio"' + 
                    radio_check + ' name="' + name + '" id="' + id + 
                    '"/></td><td><span' + word_class + ' id="' + wid + '">' + 
                    el_xml_encode(form.text) + '</span></td></tr>';
          }

          text += '</table></td><td><a href="javascript:translate_dict_lemma('+
                  dict_lemma_number + ", '" + lemma.lang + "', true);\">" + 
                  lemma.lang;

          if(!lemma.known)
          {
            text += "?";
          }

          var dict_lemma = capitalize(lemma.origin_norm_form, capitalized);

          for(var k = 0; k < forms.length; k++)
          {
            var form = forms[k];

            if(lemma.origin_norm_form != form.origin_text)
            {
              dict_lemma += " " + capitalize(form.origin_text, capitalized);
            }
          }

          text += "</a></td><td><div><a href='javascript:show_dict_lemma(" + 
                  dict_lemma_number + ", \"" + lemma.lang + "\"" + ");'>" + 
                  lemma.id + "</a><span id=\"lmp_" + dict_lemma_number + "\"";
        
          if(!lemma.known)
          {
            text += " name=\"glp\"";
          }

          text += " title=\"" + el_xml_encode(dict_lemma) + 
                  "\"></span></div></td><td>";

          dict_lemma_number++;

          if(first)
          {
            var id = name + "_" + word_form_index;
            var wid = word_id + word_form_index;
            word_form_index++;

            var quote = ""

            if(left_quote == "" || right_quote == "")
            {
              quote = left_quote;
          
              if(quote == "")
              {
                quote = right_quote;
              }
            }

            text += '<input type="radio" name="' + name + '" id="' + id + 
                    '" class=\"drop\"/>drop<span id="' + wid + 
                    '" class=\"hidden\">' + quote + 
                    '</span>';

            first = false;
          }

          text += '</td></tr>';
        }

        word_index++;
      }
      else if(item.tagName == "unknown")
      {
        var lang = item.getAttribute("n");

        if(lang == null)
        {
          lang = "?";
        }
        else
        {
          lang += "?";
        }

        var word = item.getAttribute("w");
        var lemma_pseudo_id = item.getAttribute("i");

        var lower = word.toLowerCase();
        var name = "wrb_" + word_index;
        var word_id = "wsp_" + word_index + "_";

        var word_form_index = 0;
        var id = name + "_" + word_form_index;
        var wid = word_id + word_form_index;

        word_form_index++;

        text += '<tr class="option_row"><td class="radio_cell">' +
                '<input type="radio" checked="checked" name="' + name + 
                '" id="' + id + '"/></td><td>';

        text += '<input type="text" id="' + wid + '" value="' + 
                el_xml_encode(word) + '"  onfocus="document.getElementById(\''+
                id + '\').checked=\'checked\'"/>';

        text += '</td><td><table class="lemma_forms">';

        var origin_word = word;
        var chr = word.charAt(0);

        if((chr == "\"" || chr == "'") && phrase_quote == "")
        {
          phrase_quote = chr;
          phrase_start = word_index;
          origin_word = origin_word.slice(1);
        }

        chr = word.charAt(word.length - 1);

        if((chr == "\"" || chr == "'") && phrase_quote == chr)
        {
          phrase_quote = "";
            
          if(phrase_start != word_index)
          {
            this.phrase_range[this.phrase_range.length] = 
              { begin:phrase_start, end:word_index };
          }

          origin_word = origin_word.slice(0, origin_word.length - 1);
        }

        if(lower != word)
        {
          id = name + "_" + word_form_index;
          var wid = word_id + word_form_index;
          word_form_index++;

          text += '<td class="radio_cell"><input type="radio" name="' + name + 
                  '" id="' + id + '"/></td><td><span id="' + wid + '">' + 
                  el_xml_encode(lower) + '</span></td>';
        }

        id = name + "_" + word_form_index;
        var wid = word_id + word_form_index;
        word_form_index++;

        var quote = ""

        if(left_quote == "" || right_quote == "")
        {
          quote = left_quote;
        
          if(quote == "")
          {
            quote = right_quote;
          }
        }

        text += '</table></td><td><a href="javascript:translate_dict_lemma(' + 
                dict_lemma_number + ", '" + lang + "', true);\">" + lang + 
                "</a></td><td><div><a href='javascript:show_dict_lemma(" + 
                dict_lemma_number + ", \"" + lang + "\");'>" + 
                lemma_pseudo_id + 
                "</a><span id=\"lmp_" + dict_lemma_number + 
                "\" name=\"glp\" title=\"" + el_xml_encode(origin_word) + 
                "\"></span></div></td><td><input type=\"radio\" name=\"" + name + 
                "\" id=\"" + id + '" class=\"drop\"/>drop<span id="' + wid + 
                '" class=\"hidden\">' + quote + '</span></td></tr>';

        dict_lemma_number++;
        word_index++;
      }
    }

    text += "\n</table>";

    var obj = el_by_id(this.id + '_inner');

    if(obj)
    {
      obj.innerHTML = text;
    }
  },

  on_ok: function()
  {
    if(this.phrase_range == null)
    {
      return;
    }

    var text = "";
    var boxes = document.getElementsByTagName("input");

    for(var i = 0; i < boxes.length; i++)
    {
      var elem = boxes[i];

      if(elem.type == "radio" && elem.checked && 
         elem.name.indexOf("wrb_") == 0)
      {
        var parts = elem.id.split("_");

        if(parts[0] == "wrb")
        {
          var word_index = parts[1];
          var wo =
            document.getElementById("wsp_" + word_index + "_" + parts[2]);
   
          var word = wo.tagName == "SPAN" ? el_text(wo) : wo.value;

          if(word == "")
          {
            continue;
          }

          if(text != "" && elem.className != "drop")
          {
            var separator = "\n";

            for(var j = 0; j < this.phrase_range.length; j++)
            {
              if(word_index > this.phrase_range[j].begin && 
                 word_index <= this.phrase_range[j].end)
              {
                separator = " ";
                break;
              }
            }

            text += separator;
          }
  
          text += word;
        }
      }
    }

    if(this.sel_text != "")
    {
      if(this.sel_text.charAt(0) == "\n")
      {
        text = "\n" + text;
      }

      if(this.sel_text.charAt(this.sel_text.length - 1) == "\n")
      {
        text = text + "\n";
      }

      if(text.length == 2 && text.charAt(0) == "\n" && text.charAt(1) == "\n")
      {
        text = "\n";
      }

      if(text == "")
      {
        var all_text = this.text_area.value;

        if(this.range.start > 0 && all_text.charAt(this.range.start - 1) == "\n")
        {
          this.range.start--;
        }
        else
        {
          var pos = all_text.length - 1;
 
          if(this.range.end < pos && all_text.charAt(pos) == "\n")
          {
            this.range.end++;
          }
        }
      }
    }

    this.text_area.el_insert(text, this.range);
    this.range.end = this.range.start + text.length;
    this.text_area.el_set_selection(this.range, true, false);
  }
});

function normalize_words(id, on_destroy)
{
  var dlg = new WordNormDialog(el_by_id(id), on_destroy);
}

//
// Show dict lemma
//

function LemmaDialog(parent, number, lang)
{
  this.number = number;
  this.lang = lang;

  ElDialog.call(this, parent);

  this.node.style.width = "20em";
}

el_typedef(LemmaDialog, ElDialog,
{
  singleton: true,

  init: function()
  {
    var lemmas;

    if(this.number)
    {
      lemmas = new Array();
      lemmas[0] = el_by_id("lmp_" + this.number);
    }
    else
    {
      lemmas = el_by_name("glp");
    }

    var text = "<a href=\"javascript:search_dict_lemma(" + this.number + 
        ");\">search</a>\
        <a href=\"javascript:google_dict_lemma(" + this.number + 
        ");\">google</a>\
        <a href=\"javascript:translate_dict_lemma(" + this.number + 
        ", '" + this.lang + "', false);\">translate</a><br>";

    for(var i = 0; i < lemmas.length; i++)
    {
      text += '\n<div class="option_row dict_lemma_row">' + 
              el_xml_encode(lemmas[i].title) + "</div>";
    }

    return text;
  }
});


function show_dict_lemma(number, lang)
{
  var dlg = new LemmaDialog(el_by_id("lmp_" + number).parentNode, 
                            number, 
                            lang);
}
