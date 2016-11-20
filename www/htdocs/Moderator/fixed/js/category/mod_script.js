var mod_left_block_offset = 10;
var acode = "z".charCodeAt(0);
var Acode = "Z".charCodeAt(0);
var zcode = "0".charCodeAt(0);
var ncode = "9".charCodeAt(0);
var mod_words_end_marker = "~|mod_words_end|~";
var max_recent_cats = 20;

var select_category_dialog_number = 1;
var update_dialog_holder = null;
var last_added_msg_cat = null;

var search_menu = "";

el_close_all_dialogs_hook = function() { close_update_dialog(); }

function open_window(url, post_alert)
{
  if(url.length > 1600)
  {
    if(post_alert === undefined)
    {
      el_post_url(url, undefined, "_blank");
    }
    else
    {
      alert(post_alert);
    }
  }
  else
  {
    window.open(url, "_blank");
  }
}

function newsgate_selection()
{
  var text = get_selection(false, false);

  if(text != "" && text.indexOf(' ') > 0 && text.charAt(0) != "'" && 
     text.charAt(0) != '"')
  {
    text = "'" + text + "'";
  }

  newsgate_query(text);
}

function google_selection()
{
  var text = get_selection(false, false);

  if(text == "")
  {
    alert("No text selected");
  }
  else
  {
    google_query(text);
  }
}

function translate_selection()
{
  var text = get_selection(false, false);

  if(text == "")
  {
    alert("No text selected");
  }
  else
  {
    translate_query(text);
  }
}

function mod_add_params(url)
{
  var u = new El_Url(mod_base_add_params(url));
  u.params += el_mime_url_encode(mod_word_infos());
  return u.uri();
}

function mod_init(params)
{
  post_refs2 = new Array();
  params = mod_base_init(params);
  el.mod_prepost = mod_add_params;

  el_attach_event(document, "keydown", modKeyPressHandler);
//  el_attach_event(window, "beforeunload", modBeforeUnload);

  window.onresize = position_mod_left_block;
  window.onscroll = position_mod_left_block;
  
  var page_mid_area = document.getElementById("mid_area");
  var row = page_mid_area.rows[0];
  var cell = row.insertCell(0);
  cell.id = "mod_left_bar";

  cell.innerHTML = '<div id="mod_left_block_holder">\
<table id="mod_left_block" cellspacing="0">\
<tr><td><div id="mod_word_menu">\
<a href="javascript:newsgate_selection()" class="mod_menu_link">search</a>&#xA0;|&#xA0;\
<a href="javascript:google_selection()" class="mod_menu_link">google</a>&#xA0;|&#xA0;\
<a href="javascript:translate_selection()" class="mod_menu_link">translate</a>&#xA0;|&#xA0;\
<a href="javascript:find_selection(\'mod_word_menu\', position_mod_left_block)" class="mod_menu_link">find</a>\
&#xA0;|&#xA0;<a href="javascript:normalize_words(\'mod_word_menu\', position_mod_left_block)" class="mod_menu_link">normalize</a><br>\n\
<a href="javascript:toupper_words(false)" class="mod_menu_link">lower</a>\n\
&#xA0;|&#xA0;<a href="javascript:toupper_words(true)" class="mod_menu_link">upper</a>\
&#xA0;|&#xA0;<a href="javascript:capitalize_words()" class="mod_menu_link">capitalize</a>\
&#xA0;|&#xA0;<a href="javascript:sort_words()" class="mod_menu_link">sort</a>\
&#xA0;|&#xA0;<a href="javascript:dedup_words()" class="mod_menu_link">dedup</a>\
</div></td></tr>\
<tr><td><table id="mod_word_block_group"><tr><td></td></tr>\
<tr><td></td></tr><tr><td></td></tr><tr><td></td></tr><tr><td></td></tr>\
<tr><td></td></tr><tr><td></td></tr><tr><td></td></tr><tr><td></td></tr>\
<tr><td></td></tr></table></td></td>\
<tr><td><div id="mod_word_hint">Press Ctrl+<span class="mod_definition">digit</span> \
to add selected text into a proper word box</div></td></tr>\
</table></div>';

  position_mod_left_block();
  var added = false;

  var pos = params.indexOf('|');
    
  if(pos >= 0)
  {
    last_added_msg_cat = parseInt(params.substr(0, pos));
    params = params.substr(pos + 1);
  }

  while(true)
  {  
    var pos1 = params.indexOf('|');
    
    if(pos1 < 0)
    {
      break;
    }

    var index = parseInt(params.substr(0, pos1));
    var pos2 = params.indexOf('|', pos1 + 1);

    if(pos2 < 0)
    {
      break;
    }

    var category = params.substr(pos1 + 1, pos2 - pos1 - 1);
    var pos3 = params.indexOf('|', pos2 + 1);

    if(pos3 < 0)
    {
      break;
    }

    var word_list = params.substr(pos2 + 1, pos3 - pos2 - 1);
    var pos4 = params.indexOf('|', pos3 + 1);
    
    if(pos4 < 0)
    {
      break;
    }

    var cat_path = params.substr(pos3 + 1, pos4 - pos3 - 1);
    var pos5 = params.indexOf(mod_words_end_marker, pos4 + 1);
    
    if(pos5 < 0)
    {
      break;
    }

    var text = params.substr(pos4 + 1, pos5 - pos4 - 1);
    add_word(index, text);

    var move_cat_ph = "mod_move_ph_" + index;
    var holder = document.getElementById(move_cat_ph);

    if(category)
    {
      holder.last_category = category;
    }

    if(word_list)
    {
      holder.last_word_list = word_list;
    }

    if(cat_path)
    {
      holder.last_category_path = cat_path;
      set_word_list_path(index, cat_path, word_list);
    }

    params = params.substr(pos5 + mod_words_end_marker.length);
    added = true;
  }

  if(!added)
  {
    add_word(0, "");
  }

  var request;

  try
  {
    request = new ActiveXObject("Msxml2.XMLHTTP");
  }
  catch(e)
  {
    request = new XMLHttpRequest();
  }

  var data = { req:request };

  request.onreadystatechange = 
    get_ed_cat_msg_request_onreadystatechange(data);

  var links = document.getElementsByTagName("a");
  var categories = new Array();

  for(var i = 0; i < links.length; ++i)
  {
    var link = links[i];

    if(link.className == "msg_category")
    {
      var id = link.id.substr(3);
      var pos = id.indexOf("_");

      if(pos > 0)
      {
        categories[id.substr(pos + 1)] = 1;
      }
    }
  }

  var params = "";
  for(var c in categories)
  {
    if(params)
    {
      params += "&";
    }

    params += "c=" + encodeURIComponent(c);
  }

  request.open("POST", "/psp/category/ed_cat_msg", true);

  request.setRequestHeader("Content-type", 
                           "application/x-www-form-urlencoded");
  
  request.send(params);

  var cells = document.getElementsByTagName("td");

  for(var i = 0; i < cells.length; ++i)
  {
    var td = cells[i];

    if(td.className == "msg_pub")
    {
      var msg_id = td.id.substr(4);
      var span_id = "cat_" + msg_id;

      if(document.getElementById(span_id) == null)
      {
        var span = document.createElement('span');
        span.id = span_id;
        span.innerHTML = "<br>Categories: ";
        td.appendChild(span);
      }

      span_id = "add_cat_ph_" + msg_id;
      var add_cat = document.createElement('a');

      add_cat.href = 
        "javascript:select_msg_category_dialog(0, '', 1, '" + span_id +
        "', false, false, 'add_msg_category')";

      add_cat.innerHTML = "<br>[ADD&nbsp;CATEGORY]";
      add_cat.id = "ac_" + msg_id;
      td.appendChild(add_cat);

      if(mod_privileges.indexOf("F") >= 0)
      {
        var obj = el_by_id("msr_" + msg_id);
        
        if(obj)
        {
          var edit_feed = document.createElement('a');
          edit_feed.href = "/psp/feed/edit?id=" + el_text(obj);
          edit_feed.target = "_blank";

          edit_feed.innerHTML = "<br>[EDIT&nbsp;SOURCE]";
          td.appendChild(edit_feed);
        }
      }

      var span = document.createElement('span');
      span.id = span_id;
      span.style.fontSize = "80%";

      var msg_obj = document.getElementById("msg_" + msg_id);
      msg_obj.appendChild(span);
    }
  }
}

function add_msg_category(categories_selected, update_dialog_holder)
{
  last_added_msg_cat = update_dialog_holder.last_category;

  var cat = categories_selected[0];
  var msg = update_dialog_holder.id.substr(11);

  cat_msg(cat.path, cat.id, msg, 'I');
}

function add_msg_cat_links(request)
{
  var categories = new Array();
  var nodes = el_child_node(request.responseXML, "result").childNodes;

  for(var i = 0; i < nodes.length; i++)
  {
    var node = nodes[i];

    if(node.tagName == "category")
    {
      categories[el_text(node)] = node.getAttribute("id");
    }
  }
  
  var links = document.getElementsByTagName("a");
  var cat_links = [];

  for(var i = 0; i < links.length; ++i)
  {
    var link = links[i];

    if(link.className == "msg_category")
    {
      var id = link.id.substr(3);
      var pos = id.indexOf("_");

      if(pos > 0)
      {
        cat = id.substr(pos + 1);

        if(cat in categories)
        {
          cat_links.push( { link:link, id:categories[cat] } );
        }
      }
    }
  }

  for(var i = 0; i < cat_links.length; ++i)
  {
    var cl = cat_links[i];

    var link = cl.link;
    var id = link.id.substr(3);

    var pos = id.indexOf("_");

    if(pos > 0)
    {
      var cat_menu = document.createElement('span');
      cat_menu.id = "cm_" + id;

      cat_menu.innerHTML = 
        "&nbsp;[<a href=\"" +
        el_xml_encode("javascript:cat_msg('" + 
                      el_js_escape(id.substr(pos + 1)) +
                      "', " + cl.id + ", '" + el_js_escape(id.substr(0, pos)) +
                      "', 'E')") + 
        "\">REMOVE</a>]" +
        "&nbsp;[<a target = \"_blank\" href=\"" +
        el_xml_encode("/psp/category/update?c=" + cl.id) +
        "\">EDIT</a>]";

      link.parentNode.insertBefore(cat_menu, link.nextSibling);
    }
  }
}

function get_ed_cat_msg_request_onreadystatechange(data)
{
  var onready = function()
  { 
    if(data.req.readyState == 4)
    {
      switch(data.req.status)
      {
        case 200:
        {
          add_msg_cat_links(data.req);
          break;
        }
        default:
        {
          alert("Server communication failure. Error code: " + 
                data.req.status);

          break;
        }
      }
    }
  }

  return onready;
}

function add_msg_cat_link(message, category)
{ 
  var enc_cat = el_xml_encode(category.path);
  var enc_msg = el_xml_encode(message);
  var id_suffix = enc_msg + "_" + enc_cat;
  var id = "mc_" + id_suffix;

  if(document.getElementById(id) != null)
  {
    return;
  }

  var ac = document.getElementById("ac_" + message);
  var span = document.createElement('span');

  var index = post_refs2.length;

  post_refs2.push(page.search.path + "?" + page.search.cat_query_prefix + 
                  "&v=C" + el_mime_url_encode(category.path));

  span.innerHTML = "&nbsp;&#xB7; <a id='" + id + 
    "' class='msg_category' href='javascript:post(" + index + 
    ",0,1);'>" + enc_cat + "</a><span id=\"cm_" + id_suffix + "\">" +
    "&nbsp;<a href=\"" +
    el_xml_encode("javascript:cat_msg('" + el_js_escape(category.path) +
                  "', " + category.id + ", '" + el_js_escape(message) +
    "', 'E')") + "\">[REMOVE]</a>" +
    "&nbsp;<a target = \"_blank\" href=\"" +
    el_xml_encode("/psp/category/update?c=" + category.id) +
    "\">[EDIT]</a></span>";

/*
    "<a id='ed_" + id_suffix + 
    "' href='/psp/category/update?c=" + category.id +
    "' target='_blank'>&nbsp;[EDIT]</a>" +
    "<a id='rc_" + id_suffix + 
    "' href='javascript:cat_msg(\"" + enc_cat + "\", "+ category.id + 
    ", \"" + enc_msg +
    "\", \"E\")'>&nbsp;[REMOVE]</a>";
*/

  ac.parentNode.insertBefore(span, ac);
}

function cat_msg_request_onreadystatechange(data)
{
  var onready = function()
  {
    if(data.req.readyState == 4)
    {
      switch(data.req.status)
      {
        case 200:
        {
          if(data.rel == 'E')
          {
            var id = data.msg + "_" + data.cat.path;

            var obj = document.getElementById("mc_" + id);
            obj.parentNode.removeChild(obj);

            obj = document.getElementById("cm_" + id);
            obj.parentNode.removeChild(obj);
          }
          else
          {
            add_msg_cat_link(data.msg, data.cat);
          }

          break;
        }
        default:
        {
          if(data.rel == "E")
          {
            document.getElementById("cm_" + data.msg + "_" + data.cat.path).
              style.display = "inline";
          }

          if(data.req.status == 404)
          {
            alert("Category " + data.cat.path + " do not exist");
            break;
          }

          if(data.req.status == 403)
          {
            alert("You probably logged out or have no permission to edit " +
                  data.cat.path + " category.");

            break;
          }

          alert("Server communication failure. Error code: " + 
                data.req.status + ".");

          break;
        }
      }
    }
  }

  return onready;
}

function cat_msg(cat, cat_id, msg, rel)
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

  var data = { req:request, msg:msg, cat:{path:cat, id:cat_id}, rel:rel };

  request.onreadystatechange = cat_msg_request_onreadystatechange(data);

  var url = "/psp/category/cat_msg?msg=" + el_mime_url_encode(msg) + 
            "&cat_id=" + cat_id + "&rel=" + 
             el_mime_url_encode(rel);

  request.open("GET", url, true);
  request.send("");

  if(rel == "E")
  {
    document.getElementById("cm_" + msg + "_" + cat).style.display = "none";
  }
}

function modBeforeUnload(e)
{
  var evt = e || window.event;

  alert(evt.target);
  evt.returnValue = "AAA"
}

function modKeyPressHandler(e) 
{
  refresh_session();

  var evt = e || window.event;

  if(evt.keyCode == 27)
  {
    if(close_update_dialog())
    {
      e.returnValue = false;
    }

    return;
  }

  if(evt.ctrlKey)
  {
    if(evt.keyCode >= zcode && evt.keyCode <= ncode)
    {
      var shift = evt.keyCode - zcode;
      add_word(shift ? shift - 1 : 9, null);
      e.returnValue = false;
//      return false;
    }
    else if(evt.keyCode == acode || evt.keyCode == Acode)
    { 
      for(var i = 0; i < 10; ++i)
      {
        if(document.getElementById(textarea_id(i)) == null)
        {
          add_word(i, null);
          e.returnValue = false;
//          return false;
        }
      }
    }
  }

//  return true;
}

function position_mod_left_block()
{
  if(update_dialog_holder != null || el_dialogs.length)
  {
    return;
  }

  var win_rect = el_window_rect();
  var bar_rect = el_node_rect(document.getElementById("mod_left_bar"));

  var block = document.getElementById("mod_left_block");

  block.style.left = mod_left_block_offset + "px";
  block.style.top = Math.max(win_rect.y, bar_rect.y) + "px";
  block.style.width = bar_rect.w - mod_left_block_offset + "px";
}

function textarea_id(index)
{
  return 'mod_word_area_' + index;
}

function add_word(index, word)
{
  var ta_id = textarea_id(index);

  var cell = 
    document.getElementById("mod_word_block_group").rows[index].cells[0];

  if(cell.innerHTML == "")
  {
    var move_cat_ph = "mod_move_ph_" + index;

//  wrap="off"

    cell.innerHTML =
      '<table class="mod_word_block" cellspacing="0"><tr><td colspan="2" id="mod_wl_' + 
      index + '"></td></tr><tr><td style="width:100%;"><textarea id="' + ta_id + 
      '" class="mod_word_area" rows="5" onfocus="mod_edit_set_focus(\'' + 
      ta_id + '\', true)" onblur="mod_edit_set_focus(\'' + ta_id + 
      '\', false)"></textarea></td><td valign="top" style="padding: 0 0.3em">\
<a href="javascript:add_word(' + index + 
', null)" title="Add selected word(s) to the text area">add</a>\
<br><a href="javascript:select_category_dialog(0, \'\', 1, \'' + move_cat_ph +
'\', false, true, \'move_category_selected\')" \
title="Move text area content to category word list">move</a>\
<br><a href="javascript:select_category_dialog(0, \'\', 1, \'' + move_cat_ph +
'\', false, true, \'put_category_selected\')" \
title="Move and save text area content to category word list">put</a><span id="' +
move_cat_ph + '"></span>\
<br><a href="javascript:remove_word_block(' + 
      index + ')" title="Close word box">close</a><br>' + 
(index == 9 ? 0 : index + 1 ) + '</td></tr></table>';

    document.getElementById(move_cat_ph).expression = 
      el_enrich(document.getElementById(ta_id));
  }
  
  if(word == null)
  {
    word = el_trim(new String(el_get_selection()));
    word = word.replace(/(\s+)/g, " ");  
    words = word.split(" ");

    if(words.length > 1)
    {
      word = "";
      var phrase = "";

      for(var i = 0; i < words.length; i++)
      {
        var item = words[i];

        if(item.split('://').length > 1)
        {
          if(phrase != "") 
          {
            word += (word == "" ? "" : "\n") + phrase + 
              (phrase.charAt(0) == "'" ? "'" : "");

            phrase = "";
          }

          word += (word == "" ? "" : "\n") + item;
        }
        else
        {
          phrase = (phrase == "" ? "" : 
            (phrase.charAt(0) == "'" ? "" : "'") + phrase + " ") + item;
        }
      }

      if(phrase != "")
      {
        word += (word == "" ? "" : "\n") + phrase + 
          (phrase.charAt(0) == "'" ? "'" : "");
      }
    }
  }

  if(word != "")
  {
    var textarea = document.getElementById(ta_id);
    var text = el_trim(textarea.value);

    textarea.value = word + (text == "" ? "" : "\n") + text;
  }

  refresh_session();
}

function remove_word_block(index)
{
  var textarea = document.getElementById(textarea_id(index));

  if(textarea.value != "" && 
     !confirm("Do you really want to close non-empty word box ?"))
  {
    return;
  }

  var cell = 
    document.getElementById("mod_word_block_group").rows[index].cells[0];

  cell.innerHTML = "";
}

function mod_word_infos()
{
  var word_info = "";

  word_info += (last_added_msg_cat ? last_added_msg_cat : 0) + "|";

  for(var i = 0; i < 10; ++i)
  {
    var textarea = document.getElementById(textarea_id(i));

    if(textarea != null)
    {
      var text = el_trim(textarea.value);
      var ph = document.getElementById("mod_move_ph_" + i);

      word_info += i + "|" + 
                   (ph.last_category === undefined ? "" : ph.last_category) + 
                   "|" + (ph.last_word_list === undefined ? 
                   "" : ph.last_word_list) + 
                   "|" + (ph.last_category_path === undefined ? 
                   "" : ph.last_category_path) + 
                   "|" + text + mod_words_end_marker;
    }
  }

  return word_info;
}

function load_category_navigation_pane(area_id, id, word_lists)
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

  var data = { req:request, area_id:area_id, cat_id:id };

  request.onreadystatechange = 
    get_category_request_onreadystatechange(data);

  var url = "/psp/category/get?c=" + (id ? id : 1);
  var recent_word_lists = get_recent_word_lists(word_lists);

  for(var i = 0; i < recent_word_lists.length; ++i)
  {
    var rwl_id = recent_word_lists[i].id;

    if(rwl_id != id)
    {
      url += " " + rwl_id;
    }
  }

  request.open("GET", url, true);
  request.send("");
}

function get_category_request_onreadystatechange(data)
{
  var onready = function()
  {
    if(data.req.readyState == 4)
    {
      var area = document.getElementById(data.area_id);

      if(area != null)
      {
        switch(data.req.status)
        {
          case 200:
          {
            fill_category_navigation_pane(data.req, data.area_id, data.cat_id);
            break;
          }
          default:
          {
            var error = "Server communication failure. Error code: " + 
                        data.req.status + ".";

            if(data.req.status == 403)
            {
              error += 
                " You probably logged out or have not enough permissions.";
            }

            area.innerHTML = error;

            break;
          }
        }
      }
    }
  }

  return onready;
}

function parse_path_elems(elems)
{
  var result = new Array();
  var elem = null;

  for(var i = 0; (elem = el_child_node(elems, "elem", i)) != null; ++i)
  {
    result.push( { id: el_child_text(elem, "id"), 
                   name: el_child_text(elem, "name") 
                 });
  }

  return result;
}

function update_recent_word_list(rwl, result, wlst)
{
  var category = null;

  for(var i = 0; (category = el_child_node(result, "category", i)) != null; 
      ++i)
  {
    var id = el_child_text(category, "id");

    if(rwl.id == id)
    {
      var paths = el_child_node(category, "paths");
      var path = null;

      for(var j = 0; (path = el_child_node(paths, "path", j)) != null; ++j)
      {
        if(rwl.path == el_child_text(path, "path"))
        {
          rwl.path_elems = parse_path_elems(el_child_node(path, "elems"));
          break;
        }
      }

      if(path == null)
      { 
        path = el_child_node(paths, "path");
        rwl.path = el_child_text(path, "path");
        rwl.path_elems = parse_path_elems(el_child_node(path, "elems"));
      }

      if(!wlst)
      {
        rwl.ed_msg = el_child_text(category, "ed_msg") == "1";
        return rwl;
      }

      var current_word_lists = new Array();
      var word_lists = el_child_node(category, "word_lists");
      var wl = null;

      for(var j = 0; (wl = el_child_node(word_lists, "word_list", j)) != null;
          ++j)
      {
        current_word_lists.push(el_child_text(wl, "name"));
      }

      for(var j = 0; j < rwl.wl_names.length; )
      {
        var name = rwl.wl_names[j];

        var k = 0;
        for(; k < current_word_lists.length && current_word_lists[k] != name; 
            ++k);

        if(k < current_word_lists.length)
        {
          ++j;
        }
        else
        {
          rwl.wl_names.splice(j, 1);
        }
      }

      return rwl.wl_names.length ? rwl : null;
    }
  }

  return null;
}

function fill_category_navigation_pane(request, area_id, cat_id)
{
  var area = document.getElementById(area_id);

  if(area == null)
  {
    return;
  }

  var result = el_child_node(request.responseXML, "result");
  var category = el_child_node(result, "category");

  if(category == null)
  {
    if(cat_id != 1)
    {
      load_category_navigation_pane(area_id, 
                                    1, 
                                    update_dialog_holder.word_lists);
    }
    else
    {
      area.innerHTML = "<span class=\"error\">No category can be loaded</span>";
    }

    return;
  }

  var paths = el_child_node(category, "paths").childNodes;
  var prefix = update_dialog_holder.id_prefix;
  var check_box = update_dialog_holder.multiple ? "checkbox" : "radio";

  var ondblclick_attr = update_dialog_holder.multiple ? "" : 
    ' ondblclick="select_category_dialog_ok()"';

  var first_path = true;
  var text = '<table class="cat_nav_pane"><tr><td valign="top">\
<table class="breadcrumps">';

  var cat_path = "";
  var checked = false;

  for(var i = 0; i < paths.length; i++)
  {
    var path = paths[i];

    if(path.tagName == "path")
    {
      text += '\n  <tr>';

      if(!update_dialog_holder.word_lists)
      {
        text += '<td class="radio_cell">';
      }

      if(cat_path == "")
      {
        cat_path = el_child_text(path, "path");
      }

      var elems = el_child_node(path, "elems").childNodes;
      var elem_array = new Array();

      for(var j = 0; j < elems.length; j++)
      {
        var elem = elems[j];

        if(elem.tagName == "elem")
        {
          elem_array[elem_array.length] = 
            { id:el_child_text(elem, "id"), 
              name:el_xml_encode(el_child_text(elem, "name")) 
            };
        }
      }

      if(!update_dialog_holder.word_lists)
      {
        var last_elem = elem_array[elem_array.length - 1];
        var ed_msg = el_child_text(category, "ed_msg") == "1";

        if(ed_msg && first_path && (last_elem.id != 1 || prefix != "chl") &&
           last_elem.id != update_dialog_holder.category_parent &&
           document.getElementById("parn_" + last_elem.id) == null &&
           document.getElementById("chln_" + last_elem.id) == null)
        {
          var cat_name = update_dialog_holder.id_prefix == "par" ? 
            el_xml_encode(el_child_text(path, "path")) : 
            last_elem.name;

          text += "<input type=\"" + check_box + 
                  "\" name=\"ucdr\" id=\"ucdr_" + 
                  last_elem.id + "\" alt=\"" + cat_name + "\" title=\"" +
                  el_xml_encode(cat_path) + "\"";

          if(!checked && update_dialog_holder.category_id == last_elem.id)
          {
            text += " checked=\"checked\"";
            checked = true;
          }

          text += ondblclick_attr + '/> ';
        }

        text += "</td>"
      }

      text += "<td>"

      for(var j = 0; j < elem_array.length; j++)
      {
        if(j < elem_array.length - 1)
        {
          text += "<a href=\"javascript:load_category_navigation_pane('" + 
                  area_id + "', " + elem_array[j].id + ", " + 
                  (update_dialog_holder.word_lists ? "true" : "false") +
                  ");\">" + elem_array[j].name + "/</a> ";
        }
        else
        {
          text += elem_array[j].name + "/";
        }
      }

      text += '</td></tr>';
      first_path = false;
    }
  }

  text += '</table>\n<table class="child_categories">';

  var children = el_child_node(category, "children").childNodes;
  var child_array = new Array();

  for(var i = 0; i < children.length; i++)
  {
    var child = children[i];

    if(child.tagName == "category")
    {
      var name = el_child_text(child, "name");
      var ed_msg = el_child_text(child, "ed_msg") == "1";

      var j = 0;
      for(; j < child_array.length && child_array[j].name < name; j++);

      var tmp = child_array.slice(0, j);

      tmp[j] = 
      { 
        id:el_child_text(child, "id"),
        name:name,
        path:el_xml_encode(el_child_text(el_child_node(
                           el_child_node(child, "paths"), "path"), "path")),
        ed_msg: ed_msg
      };

      child_array = tmp.concat(child_array.slice(j));
    }
  }

  for(var i = 0; i < child_array.length; i++)
  {
    var child = child_array[i];
    var encoded_name = el_xml_encode(child.name);

    var cat_name = prefix == "par" ? child.path : encoded_name;

    text += "<tr class=\"option_row\">";

    if(!update_dialog_holder.word_lists)
    {
      text += "<td class=\"radio_cell\">";

      if(child.ed_msg && child.id != update_dialog_holder.category_parent &&
         document.getElementById("parn_" + child.id) == null &&
         document.getElementById("chln_" + child.id) == null)
      {
        text += "<input type=\"" + check_box + "\" id=\"ucdr_" + 
                child.id + "\" name=\"ucdr\" alt=\"" + 
                cat_name + "\" title=\"" + el_xml_encode(child.path) + "\"";

        if(!checked && update_dialog_holder.category_id == child.id)
        {
          text += " checked=\"checked\"";
          checked = true;
        }

        text += ondblclick_attr + '/>';
      }

      text += "</td>";
    }
    
    text += "<td><a href=\"javascript:load_category_navigation_pane('" + 
            area_id + "', " + child.id + ", " + 
            (update_dialog_holder.word_lists ? "true" : "false") + ");\">" + 
            encoded_name + "</a></td>" + "</tr>";
  }

  text += '\n</table>';

  if(update_dialog_holder.word_lists)
  {
    var word_lists = el_child_node(category, "word_lists").childNodes;

    text += '\n<table class="word_lists">';

    for(var i = 0; i < word_lists.length; i++)
    {
      var word_list = word_lists[i];

      if(word_list.tagName == "word_list")
      {
        var name = el_child_text(word_list, "name");

        text += "\n<tr class=\"option_row\"><td class=\"radio_cell\">" +
                "<input type=\"" + check_box + "\" id=\"wl_" + 
                cat_id + "_" + name + "\" name=\"wrls\" alt=\"" + 
                cat_path + "\"";

        if(!checked && update_dialog_holder.last_word_list == name &&
           update_dialog_holder.last_category == cat_id)
        {
          text += " checked=\"checked\"";
          checked = true;
        }

        text += ondblclick_attr + '/></td><td>' + el_xml_encode(name) + 
                "</td></tr>";
      }
    }

    text += '\n</table>';
  }

  text += "</td>";

  var recent_word_lists =
    get_recent_word_lists(update_dialog_holder.word_lists);

  if(recent_word_lists.length)
  {
    text += '<td valign="top"><table class="recent_cats_label"><tr><td>\
Recent:</td></tr><table class="recent_cats">';

    for(var i = 0; i < recent_word_lists.length;)
    {
      var rwl = update_recent_word_list(recent_word_lists[i], 
                                        result, 
                                        update_dialog_holder.word_lists);

      if(rwl == null)
      {
        recent_word_lists.splice(i, 1);
        continue;
      }
      else
      {
        ++i;
      }

      text += "\n<tr class=\"option_row\">";

      if(!update_dialog_holder.word_lists)
      {
        text += "<td class=\"radio_cell\">";

        if(rwl.ed_msg && rwl.id != update_dialog_holder.category_parent &&
           document.getElementById("parn_" + rwl.id) == null &&
           document.getElementById("chln_" + rwl.id) == null)
        {
          var cat_name = 
            el_xml_encode(rwl.path_elems[rwl.path_elems.length - 1].name);

          text += "<input type=\"" + check_box + "\" id=\"ucdr_" + 
                  rwl.id + "\" name=\"ucdr\" alt=\"" + 
                  cat_name + "\" title=\"" + el_xml_encode(rwl.path) + "\"";

          if(!update_dialog_holder.multiple && !checked && 
              update_dialog_holder.last_category == rwl.id)
          {
            text += " checked=\"checked\"";
            checked = true;
          }

          text += ondblclick_attr + '/>';
        }
  
        text += "</td>";
      }

      text += "<td>";

      cat_id = 0;

      for(var j = 0; j < rwl.path_elems.length; ++j)
      {
        var elem = rwl.path_elems[j];

        text += "<a href=\"javascript:load_category_navigation_pane('" + 
                area_id + "', " + elem.id + ", true);\">" + 
                el_xml_encode(elem.name) + "/</a> "

        cat_id = elem.id;
      }

      text += "</td>"

      if(update_dialog_holder.word_lists)
      {
        text += "<td>";

        var names = rwl.wl_names;

        for(var j = 0; j < names.length; ++j)
        {
          var name = names[j];

          text += "<input type=\"" + check_box + "\" id=\"wl_" + 
                rwl.id + "_" + el_xml_encode(name) + 
                "\" name=\"wrls\" alt=\"" + 
                rwl.path + "\"";

          if(!checked && update_dialog_holder.last_word_list == name &&
             update_dialog_holder.last_category == cat_id)
          {
            text += " checked=\"checked\"";
            checked = true;
          }

          text += ondblclick_attr + '/>' + el_xml_encode(name);
        }

        text += '</td>';
      }

      text += '</tr>';
    }

    text += "</table></td>";

    set_recent_word_lists(recent_word_lists, 
                          update_dialog_holder.word_lists);
  }

  text += "</tr></table>";

  area.innerHTML = text;
}

function select_msg_category_dialog(id,
                                    prefix,
                                    parent,
                                    ph_id,
                                    multiple,
                                    word_lists, 
                                   ok_func)
{
  var place_holder = document.getElementById(ph_id);
  place_holder.last_category = last_added_msg_cat;

  select_category_dialog(id,
                         prefix,
                         parent,
                         ph_id,
                         multiple,
                         word_lists, 
                         ok_func);
}

function select_category_dialog(id,
                                prefix,
                                parent,
                                ph_id,
                                multiple,
                                word_lists, 
                                ok_func)
{
  close_update_dialog();

  var area_id = "ucda_" + select_category_dialog_number++;
  var place_holder = document.getElementById(ph_id);

  var ok_label = "ok";
  
  var text = 
"<div class=\"select_category_dialog dialog\">\n\
      <div id=\"" + area_id + "\">Loading ...</div>\n\
      <br><a href=\"javascript:select_category_dialog_ok();\">" + ok_label + 
      "</a>\
    <a href=\"javascript:close_update_dialog();undefined;\">cancel</a>\
    &nbsp;<span id=\"ucd_error\" class=\"error\"></span>\n\
    </div>"

  if(place_holder.last_category !== undefined)
  {
    id = place_holder.last_category;
  }

  place_holder.innerHTML = text;
  place_holder.category_id = id;
  place_holder.id_prefix = prefix;
  place_holder.category_parent = parent;
  place_holder.type = "category";
  place_holder.multiple = multiple;
  place_holder.word_lists = word_lists;
  place_holder.ok_func = ok_func;

  if(document.getElementById("mod_left_block") != null)
  {
    place_holder.post_close = position_mod_left_block;
  }

  update_dialog_holder = place_holder;

  load_category_navigation_pane(area_id, id, word_lists);
}

function set_recent_word_lists(recent_word_lists, wl)
{
  var assignment = wl ? "rwl=" : "rcat=";
  var cookie = assignment;

  for(var i = 0; i < recent_word_lists.length; ++i)
  {
    var item = recent_word_lists[i];
    cookie += (i ? '"' : '') + item.id + "'" + item.path;

    for(var j = 0; j < item.wl_names.length; ++j)
    {
      cookie += "'" + item.wl_names[j];
    }
  }

  document.cookie = cookie;
}

function get_recent_word_lists(wl)
{
  var rwl = null;
  var cookies = document.cookie.split(";");
  var cookie_name = wl ? "rwl" : "rcat";

  for(var i = 0; i < cookies.length; ++i)
  {
    var cookie = el_trim(cookies[i]);
    var pos = cookie.indexOf("=");

    if(pos > 0 && cookie.substr(0, pos) == cookie_name)
    {
      rwl = cookie.substr(pos + 1).split('"');
      break;
    }
  }

  var word_lists = new Array();

  if(rwl == null)
  {
    return word_lists;
  }

  for(var i = 0; i < rwl.length; ++i)
  {
    var items = rwl[i].split("'");
    var wl_names = new Array();

    for(var j = 2; j < items.length; ++j)
    {
      wl_names.push(items[j]);
    }

    word_lists.push( { id : items[0], 
                       path : items[1], 
                       wl_names : wl_names, 
                       ed_msg: true });
  }

  return word_lists;
}

function add_recent_word_list(item, recent_word_lists)
{
  var rwl = null;

  for(var i = 0; i < recent_word_lists.length; ++i)
  {
    var entry = recent_word_lists[i];

    if(entry.id == item.id)
    {
      rwl = recent_word_lists[i];
      recent_word_lists.splice(i, 1);
      break;
    }
  }

  if(rwl == null)
  {
    rwl = { id: item.id, 
            path: item.path, 
            wl_names : new Array(), 
            ed_msg: true 
          };
  }
  else
  {
    rwl.path = item.path;
  }
  
  var i = 0;
  for(; i < rwl.wl_names.length && rwl.wl_names[i] < item.wl_name; ++i);

  if(i == rwl.wl_names || rwl.wl_names[i] != item.wl_name)
  {
    rwl.wl_names.splice(i, 0, item.wl_name);
  }

  recent_word_lists.splice(0, 0, rwl);

  if(recent_word_lists.length > max_recent_cats)
  {
    recent_word_lists.splice(max_recent_cats, 
                             recent_word_lists.length - max_recent_cats);
  }
}

function select_category_dialog_ok()
{
  if(update_dialog_holder == null)
  {
    return;
  }

  var categories_selected = new Array();

  var recent_word_lists = 
    get_recent_word_lists(update_dialog_holder.word_lists);

  if(update_dialog_holder.word_lists)
  {
    var boxes = el_by_name("wrls");

    for(var i = 0; i < boxes.length; i++)
    {
      var bx = boxes[i];

      if(bx.checked)
      {
        var id_parts = bx.id.split("_");

        var item = 
        { 
          id   : id_parts[1],
          path : bx.alt,
          wl_name : id_parts[2]
        };

        add_recent_word_list(item, recent_word_lists);
        categories_selected.push(item);
      }
    }
  }
  else
  {
    var boxes = el_by_name("ucdr");

    for(var i = 0; i < boxes.length; i++)
    {
      var bx = boxes[i];

      if(bx.checked)
      {
        var id_parts = bx.id.split("_");

        categories_selected.push(
          { 
            id   : id_parts[1],
            name : bx.alt,
            path : bx.title
          });

        var item = 
        { 
          id   : id_parts[1],
          path : bx.alt,
          wl_name : ""
        };

        add_recent_word_list(item, recent_word_lists);
      }
    }
  }

  var last_cat = 
    categories_selected.length > 0 && !update_dialog_holder.multiple ? 
    categories_selected[0].id : 0;

  set_recent_word_lists(recent_word_lists, 
                        update_dialog_holder.word_lists);

  if(!categories_selected.length)
  {
    document.getElementById("ucd_error").innerHTML =
      update_dialog_holder.word_lists ? "Please select a word list." :
      "Please select a category.";

    return;
  }

  var ok_func = null;
  eval("ok_func=" + update_dialog_holder.ok_func + ";");

  var cat = categories_selected[0];

  update_dialog_holder.last_category = cat.id;
  update_dialog_holder.last_category_path = cat.path;
  update_dialog_holder.last_word_list = cat.wl_name;

  ok_func(categories_selected, update_dialog_holder);

  close_update_dialog();
}

function close_update_dialog()
{
  if(update_dialog_holder != null)
  {
    var post_close = update_dialog_holder.post_close === undefined ?
      null : update_dialog_holder.post_close;

    update_dialog_holder.innerHTML = "";
    update_dialog_holder = null;

    if(post_close)
    {
      post_close();
    }

    return true;
  }

  return false;
}

function move_category_selected(categories_selected, update_dialog_holder)
{
  category_selected(categories_selected, update_dialog_holder, false);
}

function put_category_selected(categories_selected, update_dialog_holder)
{
  category_selected(categories_selected, update_dialog_holder, true);
}

function category_selected(categories_selected, update_dialog_holder, save)
{
  var index = parseInt(update_dialog_holder.id.split("_")[3]);

  set_word_list_path(index, 
                     update_dialog_holder.last_category_path, 
                     update_dialog_holder.last_word_list);

  move_word_index(index, save);
}

function set_word_list_path(index, cat_path, word_list)
{
  var label = document.getElementById("mod_wl_" + index);
 
  if(label != null)
  {
    var cp = cat_path.split("/");
    cat_path = "/";

    for(var i = 0; i < cp.length; ++i)
    {
      var name = cp[i];

      if(name)
      {
        cat_path += name + "/ ";
      }
    }

    label.innerHTML = 
      '<a href="javascript:move_word_index(' + index + 
      ', false)">move</a>, <a href="javascript:move_word_index(' + index + 
      ', true)">put</a> to ' + el_xml_encode(cat_path + word_list);
  }
}

function move_word_index(index, save)
{
  var ph = document.getElementById("mod_move_ph_" + index);

  var url = "/psp/category/update?c=" + ph.last_category + "&wl=" + 
             el_mime_url_encode(ph.last_word_list);

  var textarea = document.getElementById(textarea_id(index));

  if(textarea != null)
  {
    var text = el_trim(textarea.value);

    if(text != "")
    {
      url += "&wl_add=" + el_mime_url_encode(text + "\n") + 
             "&wl_cs=mod_word_area_" + index;
    }
  }

  if(save)
  {
    url += "&wl_save=1";
  }

  open_window(url);
}
