var post_refs = null;

function post(ref_index, new_window, pass_mod_params, add_vf_param)
{
  var url = post_refs[ref_index];

  if(vf_params != "")
  {
    url += (url.indexOf("?") < 0 ? "?" : "&") + vf_params;
  }
    
  if(pass_mod_params && el.mod_prepost)
  {
    url = el.mod_prepost(url);
  }

  el_post_url(url, null, new_window ? "_blank" : "");
}

function bulk_edit()
{
  var form = document.getElementById("bulk_edit");

  if(el.mod_prepost)
  {
    var params = el.mod_prepost("");
    var name_values = params.split("&");

    for(var i = 0; i < name_values.length; i++)
    {
      var nvp = name_values[i];

      if(nvp == "")
      {
        continue;
      }

      var nv = nvp.split("=", 2);
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

  form.submit();
}

function select_all(val)
{
  var controls = document.getElementsByTagName("input");

  for(var i = 0; i < controls.length; i++)
  {
    var input = controls[i];
    if(input.type == "checkbox" && input.id != "inplace_mode")
    {
      input.checked = val; 
    }
  }
}

function set_opt(control, operation)
{
  var feed_id = control.parentNode.parentNode.getAttribute("feed_id");
  var value = control.options[control.selectedIndex].value;

  send_request(operation, feed_id + "_" + value);
  control.parentNode.setAttribute("value", value);
}

function select_control(onchange, options, selected)
{
  var text = '<select onchange="' + onchange + '">'

  for(var i = 0; i < options.length; i++)
  {
    var o = options[i];
    text += '\n<option value="' + o.name + '"';

    if(o.name == selected)
    {
      text += ' selected="selected"';
    }

    text += '>' + o.label + '</option>';
  }

  return text + '</select>';
}

function plain_text(options, selected)
{
  for(var i = 0; i < options.length; i++)
  {
    var o = options[i];

    if(o.name == selected)
    {
      return o.label;
    }
  }

  return "???";
}

function get_inplace_mode_request_onreadystatechange(data)
{
  var onready = function()
  {
    if(data.req.readyState == 4)
    {
      try
      {
        if(data.req.status != 200)
        {
          if(data.req.status == 401)
          {
            window.location = "view";
          }
          else
          {
            alert("Request failed, status code " + data.req.status);
          }
        }
      }
      catch(e)
      {
        alert("Request failed");
      }
    }
  }

  return onready;
}

function send_request(op, arg)
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

  try
  {
    var data = { req:request };
        
    request.onreadystatechange = 
      get_inplace_mode_request_onreadystatechange(data);

    request.open("GET", 
                 "/psp/feed/view_op?o=" + op + "&a=" + arg, 
                 true);

    request.send("");
  }
  catch(e)
  {
    alert(e);
  }
}

function set_inplace_mode()
{
  do_set_inplace_mode(space_options, 
                      space_texts, 
//                      lang_options, 
//                      country_options, 
                      status_options)
}

function swap_inplace_mode()
{
  do_swap_inplace_mode(space_options, 
                       space_texts, 
//                       lang_options, 
//                       country_options, 
                       status_options);
}

function do_swap_inplace_mode(space_options, 
                              space_texts, 
//                              lang_options, 
//                              country_options, 
                              status_options)
{
  var inplace = document.getElementById("inplace_mode").checked;
  send_request("inplace", inplace ? "1" : "0");

  do_set_inplace_mode(space_options, 
                      space_texts, 
//                      lang_options, 
//                      country_options, 
                      status_options)
}

function do_set_inplace_mode(space_options, 
                             space_texts, 
//                             lang_options, 
//                             country_options, 
                             status_options)
{
  var inplace = document.getElementById("inplace_mode").checked;
  var controls = document.getElementsByTagName("td");

  for(var i = 0; i < controls.length; i++)
  {
    var cell = controls[i];
    var class_name = cell.className;

    // Works tooooo slow for lang and country due to big number of items
    if(class_name == "space" || /*class_name == "lang" || 
       class_name == "country" ||*/ class_name == "status")
    { 
      var selected = cell.getAttribute("value");

      var options = null;

      if(class_name == "space")
      {
        options = inplace ? space_options : space_texts;
      }
//      else if(class_name == "lang")
//      {
//        options = lang_options;
//      }
//      else if(class_name == "country")
//      {
//        options = country_options;
//      }
      else if(class_name == "status")
      {
        options = status_options;
      }

      if(inplace)
      {
        cell.innerHTML = 
          select_control("set_opt(this, '" + class_name + "');", 
                         options,
                         selected);
      }
      else
      {
        cell.innerHTML = 
          plain_text(options, selected);
      }
    }
  }

  if(browser == "safari")
  {
    var table = document.getElementById("feeds");
    table.innerHTML = table.innerHTML;
  }
}