var msg_img_preview_wrp = null;
var msg_img_preview_width = 0;
var img_max_preview_height = 0;
var img_scroll_button_height = 23;

var img_control_bar = null;
var img_control_bar_width = 0;

var scena_width = 0;
var scena_height = 0;

var img_max_height = 0;
var img_max_width = 0;
var img_margin_left = 1;
var img_margin_right = 1;
var img_available_width = 0;

var current_image = -1;
var current_flow_img = false;
var slideshow_timer = 0;
var hidden_images = [];

function init(index)
{
  var msg_img_block = el_by_id("msg_img_block");

  if(msg_img_block)
  {
    msg_img_block.style.display = "block";
  }

  msg_img_preview_wrp = el_by_id("msg_img_preview_wrp");

  if(msg_img_preview_wrp)
  {
    msg_img_preview_width = el_node_rect(msg_img_preview_wrp).w;
    img_margin_left = 3;
  }

  img_control_bar = el_by_id("img_control_bar");

  if(img_control_bar)
  {
    img_control_bar_width = el_node_rect(el_by_id("img_control_bar")).w;
  }

  if(page.message.images.length == 1 && page.message.images[0].alt == "")
  {
    var desc = el_by_id("msg_desc");
    
    if(desc)
    {
      var img = page.message.images[0];

      img.html = true;
      img.alt = desc.innerHTML;

      desc.innerHTML = "";
    }
  }

  for(var i = 0; i < page.message.images.length; ++i)
  {
    var img = page.message.images[i];

    img_max_height = Math.max(img_max_height, img.height);
    img_max_width = Math.max(img_max_width, img.width);

    if(msg_img_preview_wrp)
    {
      img_max_preview_height = 
        Math.max(img_preview_height(i), img_max_preview_height);
    }

    if(img.alt)
    {
      img_margin_right = 5;
    }
  }

  set_scena();
  select_image(index, true, true);

  window.onresize = on_resize;
  window.onorientationchange = on_resize;

  window.setTimeout("preload_image(0)", 3000);
}

function on_destroy_settings()
{
  on_resize();
}

function preload_image(index)
{
  if(index >= page.message.images.length)
  {
    return;
  }

  var img = page.message.images[index];

  if(img.loaded)
  {
    window.setTimeout("preload_image(" + (index + 1) + ")", 0);
  }
  else
  {
    var obj = document.createElement("img");

    var preload_call = function()
    {
      window.setTimeout('preload_image(' + (index + 1) + ')', 1000)
    }

    obj.onload = preload_call;
    obj.onerror = preload_call;

    obj.src = img.src;

    hidden_images.push(obj);
    img.loaded = true;
  }
}

function on_resize()
{
  set_scena();
  select_image(current_image, false, true); // Can reposition img alt text
}

function player_width()
{
  var obj = el_by_id("msg_in");
 
  return obj.innerWidth != 
    undefined ? obj.innerWidth : obj.clientWidth;
}

function set_scena()
{
  img_available_width = player_width() - msg_img_preview_width -
                        img_margin_left - img_margin_right;

  scena_width = 
    Math.max(Math.min(img_available_width, img_max_width),
             img_control_bar_width);

  var body_height = el_window_rect().h;

  scena_height = Math.max(Math.min(body_height / 1.5, img_max_height),
                          img_max_preview_height);

  var max_width = 0;
  var max_height = 0;

  for(var i = 0; i < page.message.images.length; ++i)
  {
    var size = image_adjusted_size(i);
    max_width = Math.max(max_width, size.w);
    max_height = Math.max(max_height, size.h);
  }

  scena_width = Math.round(Math.min(scena_width, max_width));
  scena_height = Math.round(Math.min(scena_height, max_height));

  if(img_control_bar)
  {
    img_control_bar.style.marginLeft = 
      Math.round((scena_width - img_control_bar_width) / 2) + "px";
  }

  if(msg_img_preview_wrp)
  {
    var style = msg_img_preview_wrp.style;
    style.height = scena_height + "px";

    var dn_button_style = el_by_id("img_preview_scroll_dn").style;
    var vp = visible_previews();

    if(vp.first > 0 || vp.last < page.message.images.length - 1)
    {
      style.height = (scena_height - img_scroll_button_height) + "px";
      dn_button_style.display = "block";
    }
    else
    {
      dn_button_style.display = "none";
    }
  }
}

function image_adjusted_size(index)
{
  var img = page.message.images[index];

  var width_aspect = img.width / scena_width;
  var height_aspect = img.height / scena_height;

  var aspect = Math.max(width_aspect, height_aspect);

  return { w: Math.round(img.width / aspect), 
           h: Math.round(img.height / aspect) 
         };
}

function select_image(index, stop, scroll)
{
  if(index < 0 || index >= page.message.images.length)
  {
    return;
  }

  if(stop)
  {
    img_stop();
  }

  if(msg_img_preview_wrp)
  {
    if(current_image >= 0)
    {
      el_by_id("ip_" + current_image).style.background = "";
    }

    el_by_id("ip_" + index).style.background = "red";
  }

  var img = page.message.images[index];
  var sz = image_adjusted_size(index);
  var flow_img = img.alt && img_available_width - sz.w > 100;

  var margin_right = flow_img ? img_margin_right : 1;

  var width = Math.min(flow_img ? sz.w : 
                       (sz.w + img_margin_right - 1), img.width);

  var height = Math.round(img.height * width / img.width);

  if(current_image == index && current_flow_img == flow_img)
  {
    var style = el_by_id("msg_image").style;
    style.width = width + 'px';
    style.height = height + 'px';
  }
  else
  {
    var text = '<img id="msg_image" src="' +
      el_xml_encode(img.src) + '" style="width:' + width + 'px; height:' + 
      height + 'px; margin-left:' + img_margin_left +
      'px; margin-right:' + margin_right + 'px;';

    if(flow_img)
    {
      text += ' float:left;';
    }

    text += '">';

    if(img.alt)
    {
      if(flow_img)
      {
        text += img.html == undefined ? el_xml_encode(img.alt) : img.alt;
      }
      else
      {
        text += '<div id="msg_image_alt" style="margin-left:' + 
                img_margin_left + 'px; text-align: justify;">' + 
                el_xml_encode(img.alt) + '</div>';
      }
    }  

    el_by_id("msg_image_wrp").innerHTML = text;
  }

  current_image = index;
  current_flow_img = flow_img;

  if(scroll)
  {
    make_preview_visible(index);

    if(index)
    {
      make_preview_visible(index - 1);
    }

    if(index < page.message.images.length - 1)
    {
      make_preview_visible(index + 1);
    }
  }

  img.loaded = true;
}

function img_first()
{
  select_image(0, true, true);
}

function img_next()
{
  select_image(current_image < page.message.images.length - 1 ?
                 current_image + 1 : 0, 
               true, 
               true);
}

function img_prev()
{
  select_image(current_image > 0 ?
                 current_image - 1 : page.message.images.length - 1,
               true, 
               true);
}

function img_last()
{
  select_image(page.message.images.length - 1, true, true);
}

function img_play()
{
  if(slideshow_timer == 0)
  {
    var img = el_by_id("img_play");
    img.src = "/fixed/img/pause.png";
    img.title = "<? loc PAUSE ?>"
    img.parentNode.href = "javascript:img_stop();"
  }

  select_image(current_image < page.message.images.length - 1 ?
                 current_image + 1 : 0, 
               false, 
               true);

  slideshow_timer = setTimeout(
    "img_play()", 
    page.message.images[current_image].alt.length > 100 ? 7000 : 4000);
}

function img_stop()
{
  if(slideshow_timer)
  {
    clearTimeout(slideshow_timer);
    slideshow_timer = 0;

    var img = el_by_id("img_play");
    img.src = "/fixed/img/play.png";
    img.title = "<? loc SLIDESHOW ?>"
    img.parentNode.href = "javascript:img_play();"
  }
}

function make_preview_visible(index)
{
  if(msg_img_preview_wrp)
  {
    var pos = img_preview_hpos(index);

    if(msg_img_preview_wrp.scrollTop > pos.top)
    {
      msg_img_preview_wrp.scrollTop = pos.top;
    }
    else
    {
      var shift = pos.bottom - msg_img_preview_wrp.scrollTop - 
                  msg_img_preview_wrp.clientHeight;

      if(shift > 0)
      {
        msg_img_preview_wrp.scrollTop += shift;
      }
    }

    var visible = visible_previews();

    set_scroll_status("up", visible.first >= 1);
    set_scroll_status("dn", visible.last < page.message.images.length - 1);
  }
}

function set_scroll_status(name, show)
{
  var div_style = el_by_id("img_preview_scroll_" + name).style;
  var anc_style = el_by_id("img_preview_scroll_" + name + "_anc").style;

  div_style.padding = show ? (name == "up" ? "0 0 3px" : "3px 0 0") : "0";

  div_style.height = show ? (img_scroll_button_height - 3 + "px") : 
                     (img_scroll_button_height + "px");

  anc_style.display = show ? "inline" : "none";
}

function img_preview_height(index)
{
  return el_node_rect(el_by_id("ip_" + index)).h;
}

function img_preview_hpos(index)
{
  var pos = 0;

  for(var i = 0; i < index; ++i)
  {
    pos += img_preview_height(i);
  }

  return { top: pos, bottom: pos + img_preview_height(i) };
}

function scroll_img_preview_up()
{
  var visible = visible_previews();
  make_preview_visible(Math.max(visible.first - 1, 0));
}

function scroll_img_preview_dn()
{
  var visible = visible_previews();

  make_preview_visible(Math.min(visible.last + 1, 
                       page.message.images.length - 1));
}

function visible_previews()
{
  var first = -1;
  var last = -1;

  if(msg_img_preview_wrp)
  {
    var top = msg_img_preview_wrp.scrollTop;
    var bottom = top + msg_img_preview_wrp.clientHeight;

    var pos = 0;

    for(var i = 0; i < page.message.images.length; ++i)
    {
      if(first < 0 && pos >= top)
      {
        first = i;
        last = first;
      }

      pos += img_preview_height(i);

      if(pos <= bottom)
      {
        last = i;
      }
    }
  }

  return { first: first, last: last };
}
