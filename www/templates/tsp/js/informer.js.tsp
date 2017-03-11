function informer_init(error_pos, reload)
{
  init(error_pos);

  if(!error_pos)
  {
    informer_changed(reload);
  }
}

function informer_params(just_style_flag)
{
  if(el_by_id("ir_msg_count") == null)
  {
    return "";
  }

  var res = just_style_flag === undefined ? "ir=1" : "";

  for(var i = 0; i < page.informer.options.length; ++i)
  {
    var io = page.informer.options[i];

    if(just_style_flag !== undefined && just_style_flag != io.just_style)
    {
      continue;
    }

    var value = informer_option(io.id);

    if(value == io.def)
    {
      continue;
    }

    res += (res == "" ? "" : "&") + io.param + "=" + el_mime_url_encode(value);
  }

  return res;
}

page.extra_params = informer_params;

function informer_changed(reload)
{ 
  var params = informer_params() + "&" + 
               extract_param("r=", page.informer.search.query).query;

  var msg_count = informer_option("ir_msg_count");

  if(reload || msg_count != page.search.results_per_page)
  {
    var url = page.search.path + "?" + params;

    if(page.translator.query_suffix)
    {
      url += "&" + page.translator.query_suffix;
    }

    el_post_url(url);
    return; 
  }

  var external_style = informer_option("ir_style_placement") == 1;

  var obj = el_by_id("ir_msg_count");

  if(row_style_visible == null) 
  {
    row_style_visible = obj.style.display;
  }

  obj = el_by_id("ir_title_underline_color_row");

  obj.style.display = 
    parseInt(informer_option("ir_img_pos")) < 2 ? row_style_visible : "none";

  obj = el_by_id("ir_style");
  obj.style.width = el_by_id("ir_script").offsetWidth;

  var image_controls_visible = informer_option("ir_img_size") != "0x0";
  var visual_controls_visible = informer_option("ir_type") == 'j';
  var object_controls_visible = !visual_controls_visible;
  var meta_controls_visible = informer_option("ir_metainfo_columns") != 0;
  var catchword_controls_visible = informer_option("ir_catchword_pos") != 0;

  var json_format = object_controls_visible && 
    el_trim(informer_option("ir_var_name")) == "";

  var rows = document.getElementsByTagName("tr");

  for(var i = 0; i < rows.length; ++i)
  {
    var obj = rows[i];
    var visible = true;

    if(obj.className.indexOf("ir_meta_control") >= 0)
    {
      visible &= meta_controls_visible;
    }

    if(obj.className.indexOf("ir_catchword_control") >= 0)
    {
      visible &= catchword_controls_visible;
    }

    if(obj.className.indexOf("ir_visual_control") >= 0)
    {
      visible &= visual_controls_visible;
    }

    if(obj.className.indexOf("ir_object_control") >= 0)
    {
      visible &= object_controls_visible;
    }

    if(obj.className.indexOf("ir_image_control") >= 0)
    {
      visible &= image_controls_visible;
    }

    if(obj.className.indexOf("ir_ext_css_control") >= 0)
    {
      visible &= external_style;
    }

    obj.style.display = visible ? row_style_visible : "none";
  }

  el_by_id("ir_desc").innerHTML = json_format ? "<? loc IR_JSON_DESC ?>" : 
    (visual_controls_visible ? "<? loc IR_VISUAL_DESC ?>" :
     "<? loc IR_OBJECT_DESC ?>");

  var script_code = "";

  var informer_url = visual_controls_visible ? 
    page.informer.visual_url : page.informer.object_url;

  var url = informer_url + informer_params() + "&" + 
    page.informer.search.query;

  if(page.translator.query_suffix)
  {
    url += "&" + page.translator.query_suffix;
  }
  
  if(create_informer_script(url))
  { 
    var extras = "";

    if(external_style)
    {
      params = informer_params(false) + "&" + page.informer.search.query;
      extras = el_xml_encode(informer_params(true))
    }
    else
    {
      params = extract_param("cp=", params).query;
      params = extract_param("ir=", params).query;
    }

    if(json_format)
    {
      script_code = informer_url + params;
    }
    else
    {
      script_code = '<script charset="utf-8" src="' + informer_url +
                    params + '"></script>'

      if(extras != "")
      {
        script_code += "<!--IR&amp;" + extras + "IR-->";
      }
    }

    if(visual_controls_visible)
    {
      var parts = page.search.path.split('/');
      var schema = parts[0];
      var hostname = parts[2];

      var div_style = "";
      var a_style = "";
      var div_class = "";

      if(informer_option("ir_style_placement") == 0)
      {
        div_style += ' style="font-weight: 400; text-align: left; ' + 
                     'padding: 0.2em;';

        var option = informer_option("ir_catchword_background_color");
        if(option != "") div_style += ' background-color: ' + option + ';'

        var option = informer_option("ir_catchword_color");
        if(option != "") div_style += ' color: ' + option + ';'

        var option = informer_option("ir_catchword_size");
        if(option != "") div_style += ' font-size: ' + option + ';'

        var option = informer_option("ir_catchword_font");
        if(option != "") div_style += ' font-family: ' + option + ';'

        div_style += '"';

        a_style += ' style="text-decoration: none;';

        var option = informer_option("ir_catchword_link_color");
        if(option != "") a_style += ' color: ' + option + ';'

        a_style += '"';
      }
      else
      {
        div_class += ' class="' + informer_option("ir_class_prefix") + 
                     'footer"';
      }

      var id="_dsnnginf_634080201465X579_";

      script_code += '<div name="' + id + '" id="' + id + '"' +
                     div_style + div_class +'>';

      var cwt = el_trim(informer_option("ir_catchword_link_text"));

      if(cwt != "")
      {
        script_code += '<a' + a_style + ' href="' + page.informer.search.path + 
                       "?" + page.informer.search.query + '" target="_blank">'+
                       el_xml_encode(cwt, true) + 
                       '</a> <? loc:xml-strict ON_SITE ?> ';
      }

      script_code += '<a' + a_style + ' href="' + schema + '//' + hostname +
                     '/" target="_blank">' + hostname + '</a></div>';
    }
  }

  set_informer_option("ir_script", script_code);
}

function create_informer_script(url)
{
  var scr = el_by_id("informer");

  if(scr !== undefined && scr !== null)
  {
    scr.parentNode.removeChild(scr);
  }

  if(url.length > max_get_length)
  {
    el_by_id("ir_placeholder").innerHTML = "<? loc IR_REQUEST_TOO_LARGE ?>";
    return false;
  }
  else
  {
    var element = document.createElement("script");
    element.type = "text/javascript";
    element.id = "informer";
    element.src = url;

    document.getElementsByTagName('head')[0].appendChild(element);
    return true;
  }
}

function add_informer_option(param_name, element_id, def_value, just_stl)
{
  page.informer.options.push( { param: param_name, 
                                id: element_id, 
                                def: def_value,
                                just_style: just_stl
                              } );
}

function set_informer_option(elem_id, value)
{
  var obj = el_by_id(elem_id);

  if(obj == null)
  {
    return false;
  }

  if(value == null)
  {
    for(var i = 0; i < page.informer.options.length; ++i)
    {
      var io = page.informer.options[i];

      if(io.id == elem_id)
      {
        value = io.def;
        break;
      }
    }
  }

  if(obj.tagName == "SELECT")
  {
    for(var i = 0; i < obj.length; ++i)
    {
      if(obj.options[i].value == value)
      {
        obj.selectedIndex = i;
        return true;
      }
    }

    return false;
  }

  if(obj.type == "checkbox")
  {
    if(value != "1" && value == "0")
    {
      return false;
    }

    obj.checked = value == "1";
    return true;
  }

  obj.value = value;
  return true;
}

function informer_option(elem_id)
{
  var obj = el_by_id(elem_id);

  if(obj == null)
  {
    alert("Not found " + elem_id);
  }

  return value = obj.tagName == "SELECT" ? 
    obj.options[obj.selectedIndex].value : 
   (obj.type == "checkbox" ? (obj.checked ? "1" : "0") : obj.value);
}

function script_changed()
{
  var script = el_trim(el_by_id("ir_script").value);
  var b = script.indexOf("/search?t=j&");

  if(b >= 0)
  {
    b += 8;
  }
  else
  {
    b = script.indexOf("/p/s/j?");

    if(b >= 0)
    {
      b += 7;
    }
    else
    {
      b = script.indexOf("/p/s/o?");

      if(b >= 0)
      {
        b += 7;
      }
    }
  }

  var query = "";

  if(b >= 0)
  {
    var json = script.substr(0, 7) == "http://" || 
               script.substr(0, 8) == "https://";

    script = script.substring(b);

    var e = json ? script.length : script.indexOf("\">");

    if(e > 0)
    {
      query = script.substring(0, e);
    }
  }

  if(query == "")
  {
    alert("<? loc IR_BAD_CODE ?>");
  }
  else
  {
    if(query.indexOf("&amp;") >= 0 || query.indexOf("&#") >= 0)
    {
      query = el_xml_decode(query);
    }

    query = extract_param("t=", query).query;
    query = extract_param("iv=", query).query;

    var b = script.indexOf("<!--IR");
    var e = script.indexOf("IR-->");

    if(b >= 0)
    {
      b += 6;
    }

    if(b >= 0 && e > b)
    {
      query += el_xml_decode(script.substring(b, e));
    }

    var url = page.search.path + "?lang=<? lang ?>&ir=1&" + query;

    if(page.translator.query_suffix)
    {
      url += "&" + page.translator.query_suffix;
    }

    el_post_url(url);
    return; 
  }

  informer_changed();
}

function show_settings_dialog(extra_params, on_destroy)
{
  var dlg = new SettingsDialog(extra_params, on_destroy);
}

