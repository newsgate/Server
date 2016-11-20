var el = new Object();

function el_typedef(type, p1, p2)
{
  if(typeof(p1) == "function")
  {
    base_type = p1;
    members = p2;
  }
  else
  {
    if(p2 !== undefined)
    {
      throw ("el_typedef: unexpected second param " + p2);
    }

    base_type = undefined;
    members = p1;
  }

// Specifies type prototype not destroying default members (constructor, ...)
// as some code can rely on constructor presence in prototype

  if(members)
  {
    for(var m in members)
    {
      type.prototype[m] = members[m];
    }
  }

  if(base_type)
  {
    for(var m in base_type.prototype)
    {
      if(type.prototype[m] === undefined)
      {
        type.prototype[m] = base_type.prototype[m];
      }
    }
  }

  type.prototype.el_base_type = base_type;

  type.prototype.el_call =
    function(type, method, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
  {
    var t = this.constructor;

    while(t && t != type)
    {
      t = t.prototype.el_base_type;
    }

    if(!t)
    {
      throw "el_call for " + method + ": wrong type";
    }

    while(t.prototype && t.prototype[method] === undefined)
    {
      if(t.prototype.el_base_type === undefined)
      {
        throw "el_call for " + method + ": not found";
      }

      t = t.prototype.el_base_type;
    }

    if(!t.prototype)
    {
        throw "el_call for " + method + ": not found";
    }

    return t.prototype[method].call(
      this, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
  }

}

function el_trim(str)
{
  return str.replace(/(^\s+)|(\s+$)/g, "");
}

var el_whitespaces = " \n\r\t";

function el_isspace(chr)
{
  return el_whitespaces.indexOf(chr) >= 0;
}

function el_isdigit(chr)
{
  return chr.charCodeAt(0) >= String("0").charCodeAt(0) &&
         chr.charCodeAt(0) <= String("9").charCodeAt(0);
}

function el_int_error(str, minval, maxval)
{
  var len = str.length;
  var i = 0;
 
  for(; i < len && el_isspace(str.charAt(i)); i++);

  if(i == len)
  {
    return 0;
  }

  var begin = i;
  var chr;

  for(; i < len && (el_isdigit(chr = str.charAt(i)) || 
      i == begin && (chr == "+" || chr == "-")); i++);

  var end = i;
  for(; i < len && el_isspace(str.charAt(i)); i++);

  if(i != len)
  {
    return i;
  }

  if(minval !== undefined)
  {
    var intval = parseInt(str.substring(begin, end));

    if(intval < minval)
    {
      return begin;
    }

    if(maxval !== undefined && intval > maxval)
    {
      return begin;
    }
  }

  return -1;
}

//
// Simple stream
//

function El_InputStringStream(string)
{
  this.string = string;
  this.position = 0;
  this.eof = false;
  this.fail = false;
}

el_typedef(El_InputStringStream,
{ 
  string : "",
  position : 0,
  eof : false,
  fail : false,

  dump : function() 
  { 
    with(this)
    {
      return "El_InputStringStream: " + position + " " + eof + " " + fail + 
             " '" + string + "'";
    }
  },

  get_char : function() 
  { 
    with(this)
    {
      if(position >= string.length)
      {
        eof = fail = true;
      }

      return eof ? null : string.charAt(position++); 
    }
  },

  putback : function() 
  { 
    with(this)
    {
      if(!eof && position > 0)
      {
        position--;
      }
    }
  }
});

//
// Range
//

el_typedef(El_Range,
{
  start : 0,
  end : 0,

  dump : function()
  {
    return this.start + ":" + this.end;
  }
});

function El_Range(start, end)
{
  this.start = start === undefined ? 0 : start;
  this.end = end === undefined ? start : end;
}

//
// Url
//

el_typedef(El_Url,
{
  path : "",
  params : "",
  anchor : "",

  uri : function()
  {
    return this.path + (this.params ? "?" : "") + this.params + 
           (this.anchor ? "#" : "") + this.anchor;
  }
});

function El_Url(uri)
{ 
  var parts = uri.split("?", 2);

  this.path = parts[0];

  if(parts.length > 1)
  {
    parts = parts[1].split("#", 2);
    this.params = parts[0];

    if(parts.length > 1)
    {
      this.anchor = parts[1];
    }
  }
}

//
// Selection
//

function el_set_selection_(range, focus, scroll_to_view)
{
  if(focus != false)
  {
    this.focus();
  }

  var str = this.value.replace(/\r/g, "");

  if(range.start < 0 || range.end < 0)
  {
    range = new El_Range(range.start < 0 ? str.length : range.start, 
                         range.end < 0 ? str.length : range.end);
  }

  if(this.createTextRange)
  {
    var text_range = this.createTextRange();
    text_range.collapse(true);
    text_range.moveStart("character", range.start);
    text_range.moveEnd("character", range.end - range.start);
    text_range.select();

    if(scroll_to_view != false)
    {
      text_range.scrollIntoView(true);
    }
  }
  else if(this.selectionStart !== undefined)
  { 
    this.selectionStart = range.start;
    this.selectionEnd = range.end;

    if(this.rows)
    {
      var lines = 0;

      for(var i = 0; i < str.length && i < range.start; ++i)
      {
        if(str.charAt(i) == "\n")
        {
          ++lines;
        }
      }

      if(scroll_to_view != false)
      {
        this.scrollTop = Math.floor(this.clientHeight / this.rows) * lines;
      }
    }
  }
}

function el_get_selection_()
{    
  var result = new El_Range();
    
  with(this)
  {
    if(this.selectionStart !== undefined)
    {
      result.start = this.selectionStart;
      result.end = this.selectionEnd;
    }
    else
    {
      result.begin = -1;
      result.end = -1;
  
      var caretPos = document.selection.createRange();
  
      if(caretPos)
      {
        var sel_range = caretPos.duplicate();
        var r = document.body.createTextRange();
        r.moveToElementText(this);

        r.setEndPoint("EndToStart", sel_range);
        result.start = r.text.replace(/\r/g, "").length;

        r.setEndPoint("EndToEnd", sel_range);
        result.end = r.text.replace(/\r/g, "").length;
      }
    }
  }
  
  return result;
}
  
function el_insert_(text, range, scroll_to_view)
{
  with(this)
  {
    if(range !== undefined)
    {
      el_set_selection(range, false, false);
    }

    var sel = el_get_selection();

    if(this.selectionStart !== undefined)
    {
      var top = scrollTop;
      var left = scrollLeft;
      var val = value.replace(/\r/g, "");
      value = val.substr(0, sel.start) + text + val.substr(sel.end);
      
      scrollTop = top;
      scrollLeft = left;
    }
    else
    {
      var caretPos = document.selection.createRange();
 
      if(caretPos)
      {
        caretPos.text = text;
      }
    }

    var new_sel = new El_Range(sel.start + text.length);
    el_set_selection(new_sel, true, scroll_to_view);
  }
}

//
// Enrich objects
//

function el_enrich(object)
{
  if(object !== null)
  {
    if(object.nodeName == "TEXTAREA" || 
       object.nodeName == "INPUT" && object.type == "text")
    {
      el_enrich_edit(object);
    }
  }
  
  return object;
}

function el_enrich_edit(object)
{
  object.el_set_selection = el_set_selection_;
  object.el_get_selection = el_get_selection_;
  object.el_insert = el_insert_;
}

function el_mime_url_encode(text)
{
  return encodeURIComponent(text).replace(/\+/g, "%2B").replace(/\//g, "%2F").
         replace(/'/g, "%27").replace(/"/g, "%22").replace(/%20/g, "+");
}

function el_compact(text)
{
  var result = "";
  var whitespace_last = true;

  for(var i = 0; i < text.length; i++)
  {
    var chr = text.charAt(i);

    if(el_isspace(chr))
    {
      if(!whitespace_last)
      {
        result += " ";
        whitespace_last = true;
      }
    }
    else
    {
      result += chr;
      whitespace_last = false;
    }
  }

  return el_trim(result);
}

var js_spec_chars = "\b\f\n\r\t\'\"\\";

var js_spec_chars_escape = 
[
  "\\b",
  "\\f",
  "\\n",
  "\\r",
  "\\t",
  "\\'",
  "\\\"",
  "\\\\"
];

function el_js_escape(text)
{
  var result = "";

  for(var i = 0; i < text.length; i++)
  {
    var chr = text.charAt(i);
    var pos = js_spec_chars.indexOf(chr);

    if(pos < 0)
    {
      result += chr;
    }
    else
    {
      result += js_spec_chars_escape[pos];
    }
  }

  return result;
}

function el_mime_url_decode(text)
{
  return decodeURIComponent(text.replace(/\+/g, "%20"));
}

function el_xml_encode(text, enforce_numeric)
{
  var enc = text.replace(/&/g, "&amp;").replace(/</g, "&lt;").
            replace(/"/g, "&quot;").replace(/'/g, "&#39;");

  if(enforce_numeric === undefined || !enforce_numeric)
  {
    return enc;
  }

  var res = "";
  for(var i = 0; i < enc.length; ++i)
  {
    var code = enc.charCodeAt(i);
    res += code < 0x80 ? enc.charAt(i) : ("&#x" + code.toString(16) + ";");
  }

  return res;
}

var entities = 
[
  { ent:"&amp;", chr:"&" },
  { ent:"&lt;", chr:"<" },
  { ent:"&gt;", chr:">" },
  { ent:"&quot;", chr:"\"" },
  { ent:"&apos;", chr:"'" }
];

function el_xml_decode(text)
{
  var str = text;
  var new_text = "";

  while(true)
  {
    var pos = str.search(/&#?[a-zA-Z0-9]+;/);

    if(pos < 0)
    {
      new_text += str;
      break;
    }

    var end = str.indexOf(';', pos + 1);

    new_text += str.substr(0, pos);
    var ent = str.substr(pos, end - pos + 1).toLowerCase();
    str = str.substr(end + 1);

    if(ent.substr(1, 2) == "#x")
    {
      new_text += String.fromCharCode(
                    parseInt(ent.replace(/&#x([a-f0-9]+);/, "$1"), 16));

      continue;
    }    

    if(ent.substr(1, 1) == "#")
    {
      var s = String.fromCharCode(
                parseInt(ent.replace(/&#([0-9]+);/, "$1"), 10));

      new_text += s;
      continue;
    }

    var i = 0;
    for(; i < entities.length && entities[i].ent != ent; ++i);
    
    if(i < entities.length)
    {
      new_text += entities[i].chr;
    }
    else
    {
      new_text += ent;
    }
  }

  return new_text;
}

function el_get_selection()
{
  if(window.getSelection)
  {
    return window.getSelection();
  }
  else if(document.getSelection)
  {
    return document.getSelection();
  }
  else if(document.selection)
  {
    return document.selection.createRange().text;
  }

  return "";
}

function el_save_selection()
{
  if(window.getSelection) 
  {
    var sel = window.getSelection();

    if(sel.getRangeAt && sel.rangeCount) 
    {
      return sel.getRangeAt(0);
    }
  } 
  else if(document.selection && document.selection.createRange) 
  {
    return document.selection.createRange();
  }

  return null;
}

function el_restore_selection(range) 
{
  if(range) 
  {
    if(window.getSelection) 
    {
      sel = window.getSelection();
      sel.removeAllRanges();
      sel.addRange(range);
    }
    else if(document.selection && range.select) 
    {
      range.select();
    }
  }
}

function el_attach_event(obj, event, handler) 
{
  if(obj.addEventListener) 
  {
    obj.addEventListener(event, handler, false);
  } 
  else
  {
    if(obj.attachEvent) 
    {
      obj.attachEvent('on' + event, handler);
    }
  }
}

function el_detach_event(obj, event, handler) 
{
  if(obj.removeEventListener) 
  {
    obj.removeEventListener(event, handler, false);
  } 
  else 
  {
    if(obj.detachEvent) 
    {
      obj.detachEvent('on' + event, handler);
    }
  }
}

function el_target(e)
{
  return e.target ? e.target : 
    (e.srcElement ? e.srcElement : window.event.srcElement);
}

function el_child_node(parent, child_name, index)
{
  if(index === undefined)
  {
    index = 0;
  }

  if(!parent || !parent.childNodes)
  {
    return null;
  }

  child_name = child_name.toLowerCase();
  var childs = parent.childNodes;

  for(var i = 0; i < childs.length; i++)
  {
    var child = childs[i];

    if(child.nodeName.toLowerCase() == child_name)
    {
      if(index-- == 0)
      {
        return child;
      }
    }
  }

  return null;
}

function el_child_text(parent, child_name)
{
  try
  {
    return el_text(el_child_node(parent, child_name));
  }
  catch(e)
  {
    return "";
  }
}

function el_text(obj)
{
  var text = "";

  if(obj != null)
  {
    for(var n = obj.firstChild; n != null; n = n.nextSibling)
    {
      try
      {
        text += n.data;
      }
      catch(e)
      {
      }
    }
  }

  return text;
}

function el_full_text(obj)
{
  var text = "";

  if(obj != null)
  {
    for(var n = obj.firstChild; n != null; n = n.nextSibling)
    {
      if(n.nodeType === 1)
      {
        text += el_full_text(n);
      }
      else if(n.nodeType === 3)
      {
        text += n.data;
      }
    }
  }

  return text;
}

function el_post_url(url, form_id, target)
{
  if(!form_id)
  {
    form_id = "post";
  }

  var form = document.getElementById(form_id);

  if(form == null)
  {
    alert("el_post_url: form with id '" + form_id + "' not found");
    return;
  }

  var parts = url.split("?", 2);

  form.action = parts[0];
  var params = "";

  if(parts.length > 1)
  {
    parts = parts[1].split("#");
    params = parts[0];

    if(parts.length > 1)
    {
      form.action += "#" + parts[1];
    }
  }

  while(form.hasChildNodes())
  {
    form.removeChild(form.firstChild);
  }

  if(params)
  {
    var name_values = params.split("&");

    for(var i = 0; i < name_values.length; i++)
    {
      var nv = name_values[i].split("=", 2);

      var element = document.createElement("input");
      element.type = "hidden";
      element.name = el_mime_url_decode(nv[0]);

      if(nv.length > 1)
      {
        element.value = el_mime_url_decode(nv[1]);
      }

      form.appendChild(element);
    }
  }

  var tg = form.target;

  if(target)
  {
    form.target = target;
  }

  form.submit();

  if(target)
  {
    form.target = tg;
  }
}

function el_window_rect()
{
  var d = document;
  var b = d.body;

  var r = (d.doctype || d.documentMode > 7 || d.firstChild.tagName=='!') ? 
          d.documentElement : b;

  return { x:(r.scrollLeft || b.scrollLeft), y:(r.scrollTop || b.scrollTop), 
           w:r.clientWidth, h:r.clientHeight
         };
}

function el_node_rect(n)
{
  if(n == null)
  {
    return null;
  }

  var r = { x:n.offsetLeft, y:n.offsetTop, w:n.offsetWidth, h:n.offsetHeight };

  while(n.offsetParent)
  {
    n = n.offsetParent;
    r.x += n.offsetLeft;
    r.y += n.offsetTop;
  }

  return r;
}

function el_visible(n, p)
{
  if(n == null) 
  {
    return false;
  }

  var r = el_node_rect(n);

  if(!r.w || !r.h)
  {
    return false;
  }

  var w = el_window_rect();

  var v = (r.w + w.w + Math.min(r.x, w.x) - Math.max(r.x + r.w, w.x + w.w)) * 
    (r.h + w.h + Math.min(r.y, w.y) - Math.max(r.y + r.h, w.y + w.h));

  return v >= (r.w * r.h * p);
}

function el_array_find(arr, obj)
{
  for(var i = 0; i < arr.length; i++)
  {
    if(arr[i] == obj) 
    {
      return i;  
    }
  }

  return -1;
}

function el_by_id(id)
{
  return document.getElementById(id);
}

function el_by_name(name, tag)
{
  var elems = document.getElementsByTagName(tag ? tag : '*');
  var result = new Array();

  for(var i = 0; i < elems.length; ++i)
  {
    var elem = elems[i];

    if(elem.getAttribute("name") == name)
    {
      result.push(elem);
    }
  }

  return result;
}

function el_current_msec()
{
  var d = new Date();
  return d.getTime();  
}

// IE of version <= 8 do not support Array::indexOf
function el_index_of(array, value)
{
  for(var i = 0; i < array.length; ++i)
  {
    if(array[i] == value) return i;
  }

  return -1;
}

// Dialogs

var el_dialogs = [];
var el_dialog_number = 1;
var el_close_all_dialogs_hook = null;

el_attach_event(document, "keydown", el_dlg_key_pressed);

function el_dlg_key_pressed(e) 
{
  var evt = e || window.event;

  if(e.returnValue === false)
  {
    return;
  }

  if(evt.keyCode == 27)
  {
    if(el_close_dlg_by_id("", false))
    {
      e.returnValue = false;
    }
  }
}

el_typedef(ElDialog,
{
  close_name: "-",
  on_close: null,

  ok_name: "ok",
  on_ok: null,

  singleton: false,
  closed: false,

  close: function()
  {
    var child_dialogs = el_child_dialogs(this.node);

    for(var i = 0; i < child_dialogs.length; ++i)
    {
      el_close_dlg_by_id(child_dialogs[i].id, false);
    }    

    this.node.parentNode.removeChild(this.node);

    var i = el_index_of(el_dialogs, this);

    if(i >= 0)
    {
      el_dialogs.splice(i, 1);
    }

    if(this.focus_on_close)
    {
      this.focus_on_close.focus();
    }

    if(this.sel_on_close)
    {
      el_restore_selection(this.sel_on_close);
    }

    if(this.on_destroy)
    {
      this.on_destroy();
    }

    this.closed = true;
  },

  close_all: function()
  {
    var closed = false;

    for(var i = 0; i < el_dialogs.length; ++i)
    {
      var d = el_dialogs[i];

      if(d.constructor == this.constructor)
      {
        d.close()
        i = 0;
        closed = true;
      }
    }

    return closed;
  },

  inner : function()
  {
    return el_by_id(this.id + '_inner');
  }
});

function ElDialog(parent)
{
  this.id = "el_dlg_" + el_dialog_number++;
  this.node = document.createElement("div");
  this.node.el_dlg_id = this.id;
  this.node.id = this.id;

  if(this.singleton)
  {
    this.close_all();
  }

  var s = document.createElement("div").style;

  s.padding = "0.7em";
  s.background = "rgb(230, 230, 235)";
  s.position = "absolute";
  s.zIndex = 20 + el_dialog_number;
  s.borderWidth = "2px";
  s.borderColor = "rgb(145, 145, 150)";
  s.borderStyle = "solid";
  s.textAlign = "left";
  s.left = "0px";
  s.top = "0px";

  this.node.style.cssText = s.cssText;

  var inner = this.init();

  if(inner == null)
  {
    return null;
  }

  var style = "";

  if(typeof inner != "string")
  {
    style = ' style="' + inner.style + '"';
    inner = inner.text;
  }
  
  inner = '<div id="' + this.node.id + '_inner"' + style + '>' + inner + 
          '</div>';

  var controls = ""

  if(this.ok_name && this.on_ok)
  {
    controls += 
      '<div style="padding:0.5em 0 0;"><a href="javascript:el_close_dlg_by_id(\'' + this.id + 
      '\', true),undefined">' + el_xml_encode(this.ok_name) + '</a>';
  }

  if(this.close_name)
  {
    if(controls)
    {
      controls += " ";
    }
    else
    {
      controls = '<div style="padding:0.5em 0 0;">';
    }

    controls += '<a href="javascript:el_close_dlg_by_id(\'' + this.id + 
                '\', false),undefined">' + 
                el_xml_encode(this.close_name == "-" ? 
                              (this.on_ok ? "cancel" : "close") : 
                              this.close_name) +
                '</a>';
  }

  if(controls)
  {
    controls += "</div>";
  }

  this.node.innerHTML = inner + controls;

  if(parent)
  {
    parent.style.position = "relative";
  }
  else
  {
    parent = document.getElementsByTagName("body")[0];
  }

  parent.appendChild(this.node);
  el_dialogs.push(this);

  if(this.on_create)
  {
    var dlg = this;
    setTimeout(function() { dlg.on_create(); }, 0);
  }

  return this;
}

function el_child_dialogs(element)
{
  var dialogs = [];

  for(var i = 0; i < element.childNodes.length; ++i)
  {
    var obj = element.childNodes[i];
    dialogs = dialogs.concat(el_child_dialogs(obj))

    if(obj.el_dlg_id)
    {
      var dlg = el_dlg_by_id(obj.el_dlg_id);

      if(dlg)
      {
        dialogs.push(dlg);
      }
    }
  }

  return dialogs;
}

function el_dlg_by_id(id)
{
  for(var i = 0; i < el_dialogs.length; ++i)
  {
    if(el_dialogs[i].id == id)
    {
      return el_dialogs[i];
    }
  }

  return null;
}

function el_close_dlg_by_id(id, ok)
{
  var dlg = null;

  if(id == "")
  {
    if(el_dialogs.length)
    {
      dlg = el_dialogs[el_dialogs.length - 1];
    }
  }
  else
  {
    for(var i = 0; i < el_dialogs.length; ++i)
    {
      if(el_dialogs[i].id == id)
      {
        dlg = el_dialogs[i];
        break;
      }
    }
  }

  if(dlg == null)
  {
    return false;
  }

  if(ok)
  {
    if(dlg.on_ok)
    {
      dlg.on_ok();
    }
  }
  else if(dlg.on_close)
  {
    dlg.on_close();
  }

  dlg.close();
  return true;
}

function el_close_all_dialogs()
{
  while(el_dialogs.length)
  {
    el_dialogs[0].close();
  }

  if(el_close_all_dialogs_hook)
  {
    el_close_all_dialogs_hook();
  }
}
