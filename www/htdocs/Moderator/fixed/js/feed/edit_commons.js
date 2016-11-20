var current_feed_index = null;
var current_item_index = null;
var msg_selection_state = 0;
var reset_message_on_load = false;
var url_prefix = null;
var last_focus = null;
var KEEP_UNCHANGED = "<keep unchanged>";
var script_editor_shown_once = false;
var html_feed = null;
var feed_request_timeout = 180 * 2; // sec
var browser = "";
var channel_item_index = new Array();

function init(url_pref, html)
{
  html_feed = html;

  var feeds_select = el_by_id("feeds");

  if(feeds_select != null)
  {
    for(var i = 0; i < feeds.length; ++i)
    {
      var opt = document.createElement('option');
      opt.text = feeds[i].url;
      opt.selected = i == 0;

      try
      {
        feeds_select.add(opt, null); // standards compliant
      }
      catch(ex)
      {
        feeds_select.add(opt); // IE only
      }  
    }
  }

  el_by_id("p").focus();

  url_prefix = url_pref;

  el_enrich_edit(el_by_id("a"));
  script_changed();

  reset_message_on_load = 
    el_text(el_by_id(html_feed ? "chn_error" : "msg_error")) == "";

  create_preview("");      

  if(!reset_message_on_load)
  {
    // Have an error, editor is opened

    script_editor_shown_once = true;

    if(html_feed)
    {
      current_feed_index = 0;
    }
    else
    {
      set_current_feed(0);
    }

    setTimeout("check_select_change();", 500);
  }
}

function keep_unchanged(value)
{
  return value == KEEP_UNCHANGED || ("<" + value + ">") == KEEP_UNCHANGED;
}

function set_current_feed(index)
{
  if(current_feed_index === index)
  {
    return;
  }

  ++msg_selection_state;
  current_feed_index = index;

  var feed = feeds[current_feed_index];

  current_item_index = feed.current_item;

  if(feed.items === null && 
     (feed.timeout == 0 || feed.timeout < el_current_msec()))
  {
    request_channel_items();
  }
  else
  {
    fill_channel_items();
  }
}

function helper_add_code(code)
{
  var edit = el_by_id("a");
  edit.el_insert((edit.value == "" ? "" : "\n\n") + code, {start:-1, end:-1});
  script_changed();
}

function helper_images_from_doc()
{
  helper_add_code("    context.images_from_doc(images = msg.images,\n\
                            doc = context.html_doc(msg.url),\n\
                            op = newsgate.message.OP_REPLACE,\n\
                            img_xpath = '//img',\n\
                            read_size = False,\n\
                            check_ext = None,\n\
                            ancestor_xpath = None,\n\
                            alt_xpath = None,\n\
                            min_width = None,\n\
                            min_height = None,\n\
                            max_width = None,\n\
                            max_height = None,\n\
                            max_count = None,\n\
                            encoding=\"\")");
}

function helper_keywords_from_doc()
{
  helper_add_code("    context.keywords_from_doc(\\\n\
      keywords = msg.keywords,\n\
      doc = context.html_doc(msg.url),\n\
      xpath = '/html/head/meta[translate(@name, \"KEYWORDS\", \"keywords\")=\"keywords\"]/@content',\n\
      op = newsgate.message.OP_ADD_BEGIN,\n\
      separators = ',',\n\
      keyword_override = None)");
}

function helper_new_message()
{
  helper_add_code("context.new_message(url = a.attr('href'),\n\
                    title = None,\n\
                    description = None,\n\
                    images = None,\n\
                    source = None,\n\
                    space = None,\n\
                    lang = None,\n\
                    country = None,\n\
                    keywords = None,\n\
                    feed_domain_only = True,\n\
                    unique_message_url = True,\n\
                    unique_message_doc = True,\n\
                    unique_message_title = False,\n\
                    title_required = True,\n\
                    description_required = True,\n\
                    max_image_count = 1,\n\
                    drop_url_anchor = True,\n\
                    save = True,\n\
                    encoding=\"\")");
}

function helper_html_doc()
{
  helper_add_code("doc = context.html_doc(url = None, encoding=\"\")");
}

function helper_text_from_doc()
{
  helper_add_code("text = \\\n\
  context.text_from_doc(\\\n\
    xpaths = \\\n\
      [ el.libxml.DocText(xpath = '//div',\n\
                          concatenate = False,\n\
                          max_len = None)\n\
      ],\n\
    doc = context.html_doc(url),\n\
    encoding=\"\")");
}

function helper_find_in_article()
{
  helper_add_code("all_divs = context.find_in_article(xpath='//div', encoding='')");
}

function helper_set_keywords()
{
  helper_add_code("context.set_keywords(xpath=\\\n\
'/html/head/meta[translate(@name, \"KEYWORDS\", \"keywords\")=\"keywords\"]/@content',\n\
                     op=newsgate.MA_Context.OP_ADD_BEGIN,\n\
                     separators=',',\n\
                     keywords=None)");
}

function helper_set_images()
{
  helper_add_code("context.set_images(op=newsgate.MA_Context.OP_REPLACE,\n\
                   img_xpath='//img|//meta[@property=\"og:image\"]',\n\
                   read_size=False,\n\
                   check_ext=None,\n\
                   ancestor_xpath=None,\n\
                   alt_xpath=None,\n\
                   min_width=None,\n\
                   min_height=None,\n\
                   max_width=None,\n\
                   max_height=None,\n\
                   max_count=None,\n\
                   encoding='')");
}

function helper_set_src_title()
{
  helper_add_code("context.message.source.title = \\\n  ''");
}

function helper_set_description()
{
  helper_add_code("context.set_description(right_markers=['right1', 'right2'],\n\
                        left_markers=['left1', 'left2'],\n\
                        max_len=None)");
}

function helper_set_description_from_article()
{
  helper_add_code("context.set_description_from_article(xpath='/html/body',\n\
                                     concatenate=False,\n\
                                     default=None,\n\
                                     max_len=None,\n\
                                     encoding='')");
}

function helper_skip_image()
{
  helper_add_code(
    "context.skip_image(origin=newsgate.message.Image.IO_UNKNOWN,\n\
                   others=True,\n\
                   min_width=None,\n\
                   min_height=None,\n\
                   max_width=True,\n\
                   max_height=None,\n\
                   reverse=True)");
}

function helper_reset_image_alt()
{
  helper_add_code(
    "context.reset_image_alt(origin=newsgate.message.Image.IO_UNKNOWN,\n\
                        others=True)");
}

function helper_read_image_size()
{
  helper_add_code("context.read_image_size(image=img_object,\n\
                        check_ext=False)");
}

function image_origin_id(name)
{
  for(var i = 0; i < image_origins.length; ++i)
  {
    var o = image_origins[i];
    if(name == o.name) return o.id;
  }
  
  return 0;
}

function image_origin_name(id)
{
  for(var i = 0; i < image_origins.length; ++i)
  {
    var o = image_origins[i];
    if(id == o.id) return o.name;
  }
  
  return "UNKNOWN"
}

function image_status_id(name)
{
  for(var i = 0; i < image_statuses.length; ++i)
  {
    var o = image_statuses[i];
    if(name == o.name) return o.id;
  }
  
  return 4;
}

function image_status_name(id)
{
  for(var i = 0; i < image_statuses.length; ++i)
  {
    var o = image_statuses[i];
    if(id == o.id) return o.name;
  }
  
  return "SKIP"
}

function reload_feed_items()
{
  request_channel_items();
}

function reset_feed(index)
{
  var feed = feeds[index];

  feed.items = null;
  feed.error = "";
  feed.log = "";
  feed.timeout = 0;
  feed.current_item = 0;
}

function reset_current_feed()
{
  reset_feed(current_feed_index);
}

function save_focus(t)
{
  last_focus = t;
}

function feed_option_changed()
{
  script_editor_shown_once = false;
}

function show_mas_editor(state)
{  
  var script_editor_class;
  var script_viewer_class;

  if(state)
  {
    script_editor_class = "";
    script_viewer_class = "hidden";

    if(!script_editor_shown_once)
    {
      script_editor_shown_once = true;

      for(var i = 0; i < feeds.length; ++i)
      {
        reset_feed(i);
      }

      var index = current_feed_index === null ? 0 : current_feed_index;
      current_feed_index = null;
      set_current_feed(index);

      setTimeout("check_select_change();", 500);
    }
  }
  else
  {
    script_editor_class = "hidden";
    script_viewer_class = "";

    var script = el_by_id("a").value;

    if(script.length > 103)
    {
      script = script.substr(0, 100) + "...";
    }

    el_by_id("script_viewer_code").innerHTML = 
      el_xml_encode(script).replace(/\n/g, "<br>");
  }

  set_controls_visibility(script_editor_class, script_viewer_class);

  if(state)
  {
    el_by_id("a").focus();
  }
}

function sel_space_name()
{
  var sel = el_by_id("p");
  var opt = sel.options[sel.selectedIndex];
  return parseInt(opt.value) ? el_text(opt) : "";
}

function space_name(space)
{
  if(space)
  {
    var options = el_by_id("p").options;

    for(var i = 0; i < options.length; ++i)
    {
      var opt = options[i];

      if(opt.value == space)
      {
        return el_text(opt); 
      }
    }
  }

  return "";
}

function space_id(space)
{
  if(space != "")
  {
    var options = el_by_id("p").options;

    for(var i = 0; i < options.length; ++i)
    {
      var opt = options[i];

      if(el_text(opt) == space)
      {
        return parseInt(opt.value);
      }
    }
  }

  return 0;
}

function lang_name(lang)
{
  if(lang)
  {
    var options = el_by_id("y").options;

    for(var i = 0; i < options.length; ++i)
    {
      var opt = options[i];

      if(opt.title == lang)
      {
        return el_text(opt); 
      }
    }
  }

  return "";
}

function get_selected_param(sel_id, param_name, feed_val)
{
  var sel = el_by_id(sel_id);
  var val = sel.options[sel.selectedIndex].value;
  return "&" + param_name + "=" + (val == 50000 ? feed_val : val);
}

function cleanup_msg_editor()
{
  cleanup_msg("");

  if(!html_feed)
  {
    cleanup_msg("a_");

    el_by_id("i_tit").value = "";
    el_by_id("i_dsc").value = "";
  }
}

function item_option(id, def)
{
  var sel = el_by_id(id);
  var opt = sel.options[sel.selectedIndex];
  return keep_unchanged(opt.text) ? def : opt.value;
}

function request_channel_items()
{
  reset_current_feed();

  var feed = feeds[current_feed_index];
  var params = "";

  if(html_feed)
  {
    var script = el_by_id("a").value;

    if(keep_unchanged(script))
    {
      return;
    }

    params = "scr=" + el_mime_url_encode(script);

    if(el_by_id("usecache").checked)
    {
      params += "&cache=" + el_mime_url_encode(feed.cache);
    }
    else
    {
      feed.cache = "";
    }
  }

  feed.error = "Loading ...";
  feed.log = "Wait, do not refresh ! Can take long for some feeds.";

  fill_channel_items();

  var request;

  try
  {
    request = new ActiveXObject("Msxml2.XMLHTTP");
  }
  catch(e)
  {
    request = new XMLHttpRequest();
  }

  var data = { req:request,
               feed_url:feed.url,
               feed_index:current_feed_index,
               reqnum: ++feed.reqnum
             };

  feed.timeout = el_current_msec() + feed_request_timeout * 1000;

  request.onreadystatechange = 
    get_channel_items_request_onreadystatechange(data);

  if(params)
  {
    params += "&";
  }

  var keywords = el_by_id("k").value;

  if(keep_unchanged(keywords))
  {
    keywords = feed.keywords;
  }

  var space = item_option("p", feed.space);
  var lang = item_option("n", feed.lang);
  var country = item_option("y", feed.country);

  var encoding = el_by_id("e").value;

  if(keep_unchanged(encoding))
  {
    encoding = feed.encoding;
  }

  params += "url=" + el_mime_url_encode(feed.url) + "&ftp=" + feed.type +
            get_selected_param("p", "spc", space) + 
            get_selected_param("n", "lng", lang) +
            get_selected_param("y", "ctr", country) +
            "&kwd=" + el_mime_url_encode(keywords) +
            "&enc=" + el_mime_url_encode(encoding);

  request.open("POST", "/psp/feed/items", true);

  request.setRequestHeader("Content-type", 
                           "application/x-www-form-urlencoded");
  
  request.send(params);
}

function fill_channel_items()
{
  channel_item_index = new Array();

  var feed = feeds[current_feed_index];

  write_feed_logs(feed);

  var items_select = el_by_id("items");
  var options = items_select.options;

  while(options.length)
  {
    items_select.remove(0);
  }

  if(reset_message_on_load)
  {
    cleanup_msg_editor();
  }

  if(reset_message_on_load)
  {
    write_message_logs(null);
  }

  var channel_items = feed.items;

  if(channel_items == null)
  {
    return;
  }

  var can_tint_option = browser != "safari" && browser != "opera" && 
                        browser != "chrome";

  var obj = el_by_id("show_invalid");
  var show_invalid = obj == null || obj.checked;

  var j = 0;

  for(var i = 0; i < channel_items.length; ++i)
  { 
    var message = channel_items[i].message;
    var title = el_trim(message.title);

    if(title == "")
    {
      title = el_trim(message.description);
    }

    var max_title_len = html_feed ? 45 : 90;

    if(title.length > max_title_len + 3)
    {
      title = title.substr(0, max_title_len) + "...";
    }

    var opt = document.createElement('option');
    opt.selected = i == feed.current_item;

    var opt_text = title;

    if(!message.valid)
    {
      if(!show_invalid)
      {
        continue;
      }

      if(can_tint_option)
      {
        opt.style.color = "gray";
      }
      else
      {
        opt_text = "I: " + title;
      }
    }

    opt.text = opt_text;

    try
    {
      items_select.add(opt, null); // standards compliant
    }
    catch(ex)
    {
      items_select.add(opt); // IE only
    }

    channel_item_index[j++] = i;
  }

  if(reset_message_on_load)
  {
    channel_item_selected();
  }
  else
  {
    reset_message_on_load = true;
    current_item_index = el_by_id("items").selectedIndex;
  }
}

function get_channel_items_request_onreadystatechange(data)
{
  var onready = function()
  { 
    if(data.req.readyState == 4)
    {
      set_channel_items(data.req, data.feed_url, data.feed_index, data.reqnum);

      if(data.feed_index == current_feed_index)
      {
        fill_channel_items();
      }

      feeds[data.feed_index].timeout = 0;
    }
  }

  return onready;
}

function write_feed_logs(feed)
{
  write_chn_error(feed.error);
  write_chn_log(feed.log);
}

function write_message_logs(msg)
{
  write_msg_error(msg ? msg.error : "");
  write_msg_log(msg ? msg.log : "");
}

function write_chn_log(text)
{
  write_log("chn_log", text);
}

function write_chn_error(text)
{
  write_log("chn_error", text);
}

function write_msg_log(text)
{
  write_log("msg_log", text);
}

function write_msg_error(text)
{
  write_log("msg_error", text);
}

function write_log(id, text)
{
  var object = el_by_id(id);
  object.innerHTML = el_xml_encode(text).replace(/\n/g, "<br>");
  object.className = text.length ? "" : "hidden";
}

function set_channel_items(request, feed_url, feed_index, reqnum)
{
  var feed = feeds[feed_index];

  if(feed.reqnum != reqnum)
  {
    return;
  }

  feed.items = new Array();

  if(request.status != 200)
  {
    feed.error = "Server error " + request.status;
    if(request.status == 401) feed.error += "; you probably logged out";
    return;
  }

  var result = el_child_node(request.responseXML, "result");

  if(result == null)
  {
    feed.error = "No <result> tag found in server response";
    feed.log = "";
    return;
  }

  feed.error = el_child_text(result, "error");
  feed.log = el_child_text(result, "log");

  if(html_feed)
  {
    feed.cache = el_child_text(result, "cache");
  }

  var elements = result.childNodes;

  for(var i = 0; i < elements.length; ++i)
  {
    var element = elements[i];

    if(element.tagName == "context")
    {
      var elem = el_child_node(element, "item");
      var title = el_child_text(elem, "title");
      var description = el_child_text(elem, "description");

      var item = { title: title,
                   description: description
                 };

      elem = el_child_node(element, "message");
      url = el_child_text(elem, "url");
      title = el_child_text(elem, "title");
      description = el_child_text(elem, "description");
      var space = space_name(parseInt(el_child_text(elem, "space")));
      var lang = el_child_text(elem, "lang");
      var country = el_child_text(elem, "country");
      var keywords = xml_to_keywords(el_child_node(elem, "keywords"));
      var valid = parseInt(el_child_text(elem, "valid"));
      var log = el_child_text(elem, "log");

      var error = valid ? "" : 
        "Message invalid, will be skipped in runtime.";

      var message = { url: url,
                      title: title,
                      description: description,
                      space: space,
                      lang: lang,
                      country: country,
                      source: null,
                      keywords: keywords,
                      valid: valid,
                      log: log,
                      error: error,
                      images: xml_to_images(el_child_node(elem, "images"))
                    };

      elem = el_child_node(elem, "source");
      url = el_child_text(elem, "url");
      title = el_child_text(elem, "title");
      html_link = el_child_text(elem, "html_link");
  
      var source = { url: url,
                     title: title,
                     html_link: html_link
                   };

      message.source = source;

      feed.items.push({ item: item, message: message});
    }
  }
}

function channel_item_selected()
{
  ++msg_selection_state;

  if(current_feed_index === null)
  {
    return;
  }

  var feed = feeds[current_feed_index];
  var channel_items = feed.items;

  if(channel_items === null || channel_items.length == 0)
  {
    return;
  }

  current_item_index = el_by_id("items").selectedIndex;

  if(current_item_index < 0 || current_item_index >= channel_items.length ||
     channel_item_index.length <= current_item_index)
  {
    return;
  }

  current_item_index = channel_item_index[current_item_index];

  feed.current_item = current_item_index;

  var ctx = channel_items[current_item_index];

  var obj = el_by_id("i_tit");

  if(obj != null)
  {
    var item = ctx.item;

    obj.value = item.title;
    el_by_id("i_dsc").value = item.description;
  }

  var message = ctx.message;

  var source = message.source;
  el_by_id("s_url").value = source.url;
  el_by_id("s_tit").value = source.title;
  el_by_id("s_hln").value = source.html_link;

  el_by_id("url").value = message.url;
  el_by_id("tit").value = message.title;
  el_by_id("dsc").value = message.description;
  el_by_id("spc").value = message.space;
  el_by_id("lng").value = message.lang;
  el_by_id("ctr").value = message.country;
  el_by_id("kwd").value = strings_to_text(message.keywords);

  if(!html_feed)
  {
    cleanup_msg("a_");
  }

  el_by_id("img").value = images_to_text(message.images);

  write_message_logs(message);

  var obj = el_by_id("autorun");

  if(obj != null && obj.checked)
  {
    adjust_message();
  }

  create_preview("");
}

function autorun_clicked()
{
  if(el_by_id("autorun").checked)
  {
    el_by_id("items").focus();
    adjust_message();
  }
}

function image_preview_width(img, width)
{
  if(img.width == null)
  {
    return "";
  }

  return ' width="' + (img.width < width ? img.width : width) + '"';
}

function create_preview(id_prefix)
{
  var front_img = null;
  var description = el_trim(el_by_id(id_prefix + "dsc").value);
  var images = text_to_images(el_by_id(id_prefix + "img").value);

  if(description != "")
  {
    for(var i = 0; i < images.length; ++i)
    {
      var img = images[i];

      if(img.status == 0 && img.alt == "")
      {
        front_img = img;
        images.splice(i, 1);
        break;
      }
    }
  }

  el_by_id(id_prefix + "msg_tit").href =
  el_by_id(id_prefix + "msg_rmr").href =
    el_by_id(id_prefix + "url").value;

  el_by_id(id_prefix + "msg_tit").innerHTML =
    el_xml_encode(el_by_id(id_prefix + "tit").value);

  var text = "";

  if(front_img != null)
  {
    text += image_preview(img, 'msg_front_img', 60)
  }

  text += el_xml_encode(description);

  el_by_id(id_prefix + "msg_dsc").innerHTML = text;

  el_by_id(id_prefix + "msg_s_tit").href =
    el_by_id(id_prefix + "s_url").value;

  text = el_trim(el_by_id(id_prefix + "s_tit").value);
  el_by_id(id_prefix + "msg_s_tit").innerHTML = el_xml_encode(text);

  text = "";
  var country = lang_name(el_trim(el_by_id(id_prefix + "ctr").value));

  if(country != "")
  {
    text += " (" + country + ")";
  }

  el_by_id(id_prefix + "msg_s_ctr").innerHTML = el_xml_encode(text);

  text = '<table>';

  for(var i = 0; i < images.length; ++i)
  {
    var img = images[i];

    if(img.status == 0)
    {
      text += '<tr><td class="msg_media_item">' + 
              image_preview(img, 'msg_media_img', 120) + 
              el_xml_encode(img.alt) + "</td></tr>";
    }
  }

  text += "</table>";

  el_by_id(id_prefix + "msg_med").innerHTML = text;
}

function image_preview(img, class_name, max_width)
{
  var create_link = img.width == null || img.width > max_width;
  var encoded_src = el_xml_encode(img.src);
  var text = "";

  if(create_link)
  {
    text += '<a href="' + encoded_src + '" target="_blank">';
  }

  var title = el_xml_encode(image_to_text(img).replace(/\n/g, " * "));

  text += '<img class="' + class_name + '"' + 
          image_preview_width(img, max_width) + ' src="' + encoded_src + 
          '" style="max-width:' + max_width + 'px"';

  if(title.length)
  {
    text += ' title="' + title + '"';
  }

  text += '/>';

  if(create_link)
  {
    text += '</a>';
  }

  return text;
}

function text_to_images(text)
{
  var images = new Array();
  var lines = text.split('\n');

  var image = { src: "", 
                origin: 0, 
                status: 4, 
                alt: "", 
                width: null, 
                height: null
              };

  for(var i = 0; i < lines.length; ++i)
  {
    var line = el_trim(lines[i]);

    if(line == "" && image.src != "")
    {
      images.push(image);

      image = { src: "", 
                origin: 0, 
                status: 4, 
                alt: "", 
                width: null, 
                height: null
              };

      continue;
    }

    var pos = line.indexOf(':');

    if(pos < 0)
    {
      continue;
    }

    try
    {
      var attr = el_trim(line.substr(0, pos));
      var val = el_trim(line.substr(pos + 1));

      if(attr == "src") image.src = val;
      else if(attr == "alt") image.alt = val;
      else if(attr == "width") image.width = parseInt(val);
      else if(attr == "height") image.height = parseInt(val);
      else if(attr == "origin") image.origin = image_origin_id(val);
      else if(attr == "status") image.status = image_status_id(val);
    }
    catch(err)
    {
    }
  }

  if(image.src != "")
  {
    images.push(image);
  }

  return images;
}

function cleanup_msg(prefix)
{
  el_by_id(prefix + "s_url").value = "";
  el_by_id(prefix + "s_tit").value = "";
  el_by_id(prefix + "s_hln").value = "";
  el_by_id(prefix + "url").value = "";
  el_by_id(prefix + "tit").value = "";
  el_by_id(prefix + "dsc").value = "";
  el_by_id(prefix + "spc").value = "";
  el_by_id(prefix + "lng").value = "";
  el_by_id(prefix + "ctr").value = "";
  el_by_id(prefix + "kwd").value = "";
  el_by_id(prefix + "img").value = "";

  create_preview(prefix);
}

function check_select_change()
{
  script_changed();

  var feeds_select = el_by_id("feeds");

  if(feeds_select != null && current_feed_index != feeds_select.selectedIndex)
  {
    set_current_feed(feeds_select.selectedIndex);
  }
  else
  {
    var index = el_by_id("items").selectedIndex;

    if(index < channel_item_index.length && 
       current_item_index != channel_item_index[index])
    {
      channel_item_selected();
    }
  }

  setTimeout("check_select_change();", 500);
}

function script_changed()
{
  var disabled = keep_unchanged(el_by_id("a").value);

  var run = el_by_id("run");

  if(run)
  {
    run.disabled = disabled;
  }

  var autorun = el_by_id("autorun");

  if(autorun)
  {
    autorun.disabled = disabled;
    el_by_id("autorun_label").disabled = disabled;
  }
}

function adjust_message()
{
  var script = el_by_id("a").value;

  if(keep_unchanged(script))
  {
    return;
  }

  cleanup_msg("a_");

  write_msg_log("Executing script ...");

  var request;

  try
  {
    request = new ActiveXObject("Msxml2.XMLHTTP");
  }
  catch(e)
  {
    request = new XMLHttpRequest();
  }

  var data = { req:request, state:msg_selection_state };

  request.onreadystatechange = 
    get_adjust_msg_request_onreadystatechange(data);

  var params = "scr=" + el_mime_url_encode(script) +
               "&fid=" + feeds[current_feed_index].id + 
               "&i_tit=" + el_mime_url_encode(el_by_id("i_tit").value) +
               "&i_dsc=" + el_mime_url_encode(el_by_id("i_dsc").value) +
               "&s_url=" + el_mime_url_encode(el_by_id("s_url").value) +
               "&s_tit=" + el_mime_url_encode(el_by_id("s_tit").value) +
               "&s_hln=" + el_mime_url_encode(el_by_id("s_hln").value) +
               "&url=" + el_mime_url_encode(el_by_id("url").value) +
               "&tit=" + el_mime_url_encode(el_by_id("tit").value) +
               "&dsc=" + el_mime_url_encode(el_by_id("dsc").value) +
               "&spc=" + el_mime_url_encode(space_id(el_by_id("spc").value)) +
               "&lng=" + el_mime_url_encode(el_by_id("lng").value) +
               "&ctr=" + el_mime_url_encode(el_by_id("ctr").value) +
               "&kwd=" + el_mime_url_encode(el_by_id("kwd").value) +
               "&enc=" + el_mime_url_encode(el_by_id("e").value) +
               "&img=" + el_mime_url_encode(el_by_id("img").value);

  request.open("POST", "/psp/feed/adjust_msg", true);

  request.setRequestHeader("Content-type", 
                           "application/x-www-form-urlencoded");
  
  request.send(params);
}

function get_adjust_msg_request_onreadystatechange(data)
{
  var onready = function()
  { 
    if(data.req.readyState == 4)
    {
      if(data.state == msg_selection_state)
      {
        set_adjust_msg_result(data.req);
      }
    }
  }

  return onready;
}

function set_adjust_msg_result(request)
{
  write_msg_log("");

  if(last_focus != null)
  {
    last_focus.focus();
  }

  if(request.status != 200)
  {
    var error = "Server error " + request.status;
    if(request.status == 401) error += "; you probably logged out";
    write_msg_error(error);
    return;
  }
  
  var result = el_child_node(request.responseXML, "result");
  var message = el_child_node(result, "message");  
  var source = el_child_node(message, "source");

  el_by_id("a_s_url").value = el_child_text(source, "url");
  el_by_id("a_s_tit").value = el_child_text(source, "title");
  el_by_id("a_s_hln").value = el_child_text(source, "html_link");
  el_by_id("a_url").value = el_child_text(message, "url");

  el_by_id("a_tit").value = el_child_text(message, "title");
  el_by_id("a_dsc").value = el_child_text(message, "description");

  el_by_id("a_spc").value = 
    space_name(parseInt(el_child_text(message, "space")));

  el_by_id("a_lng").value = el_child_text(message, "lang");
  el_by_id("a_ctr").value = el_child_text(message, "country");

  el_by_id("a_img").value = images_to_text(
    xml_to_images(el_child_node(message, "images")));

  el_by_id("a_kwd").value = strings_to_text(
    xml_to_keywords(el_child_node(message, "keywords")));


  var error = el_child_text(result, "error");

  if(!parseInt(el_child_text(message, "valid")))
  {
    error += "\nAdjusted message invalid; will be skipped in runtime";
  }

  write_msg_error(error);
  write_msg_log(el_child_text(result, "log"));

  create_preview("a_");
}

function xml_to_keywords(kws)
{
  var keywords = new Array();

  if(kws != null)
  {
    kws = kws.childNodes;

    for(var i = 0; i < kws.length; ++i)
    {
      var kw = kws[i];
       
      if(kw.tagName == "keyword")
      {
        keywords.push(el_text(kw));
      }
    }
  }

  return keywords;
}       

function xml_to_images(imgs)
{
  var images = new Array();

  if(imgs != null)
  {
    imgs = imgs.childNodes;

    for(var i = 0; i < imgs.length; ++i)
    {
      var img = imgs[i];
       
      if(img.tagName == "image")
      {
        var src = img.getAttribute("src");
        var alt = img.getAttribute("alt");
        var width = img.getAttribute("width");
        var height = img.getAttribute("height");
        var origin = img.getAttribute("origin");
        var status = img.getAttribute("status");

        images.push( { src: src, 
                       alt: alt, 
                       width: width, 
                       height: height,
                       origin: origin,
                       status: status
                     } );
      }
    }
  }

  return images;
}

function strings_to_text(strings)
{
  var text = "";

  for(var i = 0; i < strings.length; ++i)
  {
    if(text != "") text += "\n";
    text += strings[i];
  }

  return text;
}

function image_to_text(img)
{
  var text = "";
  if(img.src) text += "src:" + img.src + "\n";

  text += "origin:" + image_origin_name(img.origin) + 
          "\nstatus:" + image_status_name(img.status) + "\n";

  if(img.alt) text += "alt:" + img.alt + "\n";
  if(img.width != null) text += "width:" + img.width + "\n";
  if(img.height != null) text += "height:" + img.height + "\n";
  return text;
}

function images_to_text(images)
{
  var text = "";

  for(var i = 0; i < images.length; ++i)
  {
    if(text != "") text += "\n";
    text += image_to_text(images[i]);
  }

  return text;
}

function show_chl_editor(state)
{  
  var script_editor_class;
  var script_viewer_class;

  if(state)
  {
    script_editor_class = "";
    script_viewer_class = "hidden";

    if(!script_editor_shown_once)
    {
      script_editor_shown_once = true;
      set_current_feed(0);
      setTimeout("check_select_change();", 500);
    }
  }
  else
  {
    script_editor_class = "hidden";
    script_viewer_class = "";

    var script = el_by_id("a").value;

    if(script.length > 103)
    {
      script = script.substr(0, 100) + "...";
    }

    el_by_id("script_viewer_code").innerHTML = 
      el_xml_encode(script).replace(/\n/g, "<br>");
  }

  set_controls_visibility(script_editor_class, script_viewer_class);

  if(state)
  {
    el_by_id("a").focus();
  }
}

function set_controls_visibility(script_editor_class, script_viewer_class)
{
  el_by_id("script_editor").className = script_editor_class;

  var obj = el_by_id("feed_id");

  if(obj)
  {
    obj.className = script_viewer_class;
  }

  obj = el_by_id("feed_url");
  
  if(obj)
  {
    obj.className = script_viewer_class;
  }

  el_by_id("feed_space").className = script_viewer_class;
  el_by_id("feed_lang").className = script_viewer_class;
  el_by_id("feed_country").className = script_viewer_class;
  el_by_id("feed_status").className = script_viewer_class;
  el_by_id("script_viewer").className = script_viewer_class;
  el_by_id("keyword_editor").className = script_viewer_class;
  el_by_id("comment_editor").className = script_viewer_class;
  el_by_id("feed_update").className = script_viewer_class;
}

function dummy()
{
    var images = new Array();

    images.push( { src: url_prefix + "/fixed/image/1.jpg", 
                   alt: "",
                   origin: 3,
                   status: 0,
                   width: 80, 
                   height: 60
                 } );
  
    images.push( { src: url_prefix + "/fixed/image/2.jpg",
                   alt: "Text commenting second image",
                   origin: 3,
                   status: 0,
                   width: 80, 
                   height: 64 
                 } );

    var message = { url: "http://weather.yahoo.com/",
                    title: "--- This is TEST news ---",
                    description: "This is TEST news description",
                    space: sel_space_name(),
                    lang: "eng",
                    country: "USA",
                    source: null,
                    keywords: new Array(),
                    valid: true,
                    log: "",
                    error: "",
                    images: images
                  };

    var source = { url: feed_url,
                   title: "This is channel title",
                   html_link: "http://yahoo.com/"
                 };

    message.source = source;

    var item = { title: "--- This is <em>TEST<em> news ---",
                 description: "This is <em>TEST<em> news description"
               };

    feed.items.push({ item: item, message: message});
}
