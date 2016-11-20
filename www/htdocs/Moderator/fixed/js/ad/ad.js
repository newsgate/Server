/////////////////////////////////////////////////////////////////////////////
// SelectSizeDialog
/////////////////////////////////////////////////////////////////////////////

function SelectSizeDialog()
{
  FullScreenDialog.call(this);
  this.size_id = 0;
}

el_typedef(SelectSizeDialog, FullScreenDialog,
{
  init: function() 
  { 
    var inner = this.el_call(FullScreenDialog, "init");

    var text = "<div id=\"sizes\">Loading sizes ...</div>";
    inner.text = text;

    return inner;
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

    var data = 
    { req: request,
      dlg:this
    };

    request.onreadystatechange = this.get_slots_req_handler(data);
    request.open("GET", "/psp/ad/json?o=slots", true);
    request.send("");
  },

  get_slots_req_handler: function(data)
  {
    var onready = function()
    {
      if(data.req.readyState == 4)
      {        
        switch(data.req.status)
        {
          case 200:
          {
            data.dlg.read_slots(data.req);
            break;
          }
          default:
          {
            break;
          }
        }    
      }
    }

    return onready;
  },

  read_slots: function(req)
  {
    if(this.closed)
    {
      return;
    }

    this.slots = eval("(" + req.responseText + ")");

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
      dlg: this
    };

    request.onreadystatechange = this.get_sizes_req_handler(data);
    request.open("GET", "/psp/ad/json?o=sizes", true);
    request.send("");
  },

  get_sizes_req_handler: function(data)
  {
    var onready = function()
    {
      if(data.req.readyState == 4)
      {        
        switch(data.req.status)
        {
          case 200:
          {
            data.dlg.read_sizes(data.req);
            break;
          }
          default:
          {
            break;
          }
        }    
      }
    }

    return onready;
  },

  read_sizes: function(req)
  {
    if(this.closed)
    {
      return;
    }

    var text = '<table id="choice"><tr><td valign="top">' +
      this.sizes_table(eval("(" + req.responseText + ")")) + 
      '</td><td valign="top">' +
      this.slots_table(this.slots) + 
      '</td></tr></table>';

    el_by_id("sizes").innerHTML = text;
    this.highlight_rows();
  },

  slots_table: function(objects)
  {
    var text = '<table id="slt"><tr id="header" align="center">\
<td>&nbsp;</td><td>Name</td></td><td>Min Width</td><td>Max Width</td>\
<td>Min Height</td><td>Max Height</td><td>Status</td></tr>';

    var prev_page = 0;

    for(var i = 0; i < objects.length; ++i)
    {
      var obj = objects[i];

      if(prev_page != obj.page)
      {
        text += '<tr><td></td><td class="page_name">' + 
                el_xml_encode(obj.page_name) + 
                '</td><td colspan="5"></td></tr>';
      }

      var pos = obj.name.indexOf(" of ");
      var name = obj.name.substr(0, pos);

      text += '<tr class="option_row" id="rsl_' + obj.id + 
              '"><td class="radio_cell"><input id="sl_'+ obj.id + 
              '" type="checkbox" name="slots" onclick="el_dlg_by_id(\'' + 
              this.id + '\').highlight_rows()"></td><td id="n_' + 
              obj.id + '">' + el_xml_encode(name) +
              '</td><td class="number_option" id="wn_' + obj.id + '">' + 
              obj.min_width + 
              '</td><td class="number_option" id="wx_' + obj.id + '">' + 
              obj.max_width + 
              '</td><td class="number_option" id="hn_' + obj.id + '">' + 
              obj.min_height + 
              '</td><td class="number_option" id="hx_' + obj.id + '">' + 
              obj.max_height + 
              '</td><td>' + (obj.status ? "Disabled" : "Enabled") + 
              '</td></tr>';

      prev_page = obj.page;
    }

    text += '</table>';

    return text;
  },

  sizes_table: function(objects)
  {
    var size_id = el_by_id("sz").value;

    var text = '<table id="szt"><tr id="header" align="center">\
<td>&nbsp;</td><td>Name</td><td>Width</td><td>Height</td><td>Status</td></tr>';

    for(var i = 0; i < objects.length; ++i)
    {
      var obj = objects[i];

      text += '<tr class="option_row" id="rsz_' + obj.id + 
              '"><td class="radio_cell"><input id="sz_'+
              obj.id + '" type="radio" name="sizes"' + 
              (obj.id == size_id ? ' checked="checked"' : '') + 
              ' ondblclick="el_dlg_by_id(\'' + this.id + 
              '\').confirm_size()" onclick="el_dlg_by_id(\'' + 
              this.id + '\').highlight_rows()"></td><td id="n_' + 
              obj.id + '">' + 
              el_xml_encode(obj.name) +
              '</td><td class="number_option" id="w_' + obj.id + '">' + 
              obj.width + '</td><td class="number_option" id="h_' + obj.id + 
              '">' + obj.height + '</td><td>' + 
              (obj.status ? "Disabled" : "Enabled") + '</td></tr>';
    }

    text += '<tr><td colspan="2" style="padding:1em 0 0"><button onclick="el_dlg_by_id(\'' + 
            this.id + '\').confirm_size()">Select</button>&nbsp;\
<button onclick="el_close_dlg_by_id(\'' + 
            this.id + '\', false)">Cancel</button></td></tr>';

    text += '</table>';

    return text;
  },

  highlight_rows: function()
  {
    var sizes = el_by_name("sizes");
    var slots = el_by_name("slots");

    for(var i = 0; i < sizes.length; ++i)
    {
      var sz = sizes[i];
      var id = sz.id.split("_")[1];
      var width = parseInt(el_text(el_by_id("w_" + id)));
      var height = parseInt(el_text(el_by_id("h_" + id)));
      var fit = true;

      for(var j = 0; j < slots.length; ++j)
      {
        var sl = slots[j];

        if(sl.checked)
        {
          var sid = sl.id.split("_")[1];

          var min_width = parseInt(el_text(el_by_id("wn_" + sid)));
          var max_width = parseInt(el_text(el_by_id("wx_" + sid)));
          var min_height = parseInt(el_text(el_by_id("hn_" + sid)));
          var max_height = parseInt(el_text(el_by_id("hx_" + sid)));

          if(width < min_width || width > max_width ||
             height < min_height || height > max_height)
          {
            fit = false;
            break;
          }
        }
      }

      el_by_id("rsz_" + id).style.backgroundColor = 
        fit ? "" : "rgb(240, 240, 240)";
    }

    for(var j = 0; j < slots.length; ++j)
    {
      var sl = slots[j];
      var sid = sl.id.split("_")[1];

      var min_width = parseInt(el_text(el_by_id("wn_" + sid)));
      var max_width = parseInt(el_text(el_by_id("wx_" + sid)));
      var min_height = parseInt(el_text(el_by_id("hn_" + sid)));
      var max_height = parseInt(el_text(el_by_id("hx_" + sid)));
      var fit = true;

      for(var i = 0; i < sizes.length; ++i)
      {
        var sz = sizes[i];

        if(sz.checked)
        {
          var id = sz.id.split("_")[1];
          var width = parseInt(el_text(el_by_id("w_" + id)));
          var height = parseInt(el_text(el_by_id("h_" + id)));

          if(width < min_width || width > max_width ||
             height < min_height || height > max_height)
          {
            fit = false;
            break;
          }
        }
      }

      el_by_id("rsl_" + sid).style.backgroundColor = 
        fit ? "" : "rgb(240, 240, 240)";
    }
  },

  confirm_size: function()
  {
    var sizes = el_by_name("sizes");
    var i = 0;
    for(; i < sizes.length && !sizes[i].checked; ++i);

    if(i < sizes.length)
    {
      el_close_dlg_by_id(this.id, true);
    }
    else
    {
      alert("No Size Selected");
    }
  },

  on_ok: function()
  {
    var sizes = el_by_name("sizes");

    for(var i = 0; i < sizes.length; ++i)
    {
      var obj = sizes[i];

      if(obj.checked)
      {
        var id = obj.id.split("_")[1];
        var width = el_text(el_by_id("w_" + id));
        var height = el_text(el_by_id("h_" + id));
        var name = el_text(el_by_id("n_" + id));

        el_by_id("sz").value = id;
        el_by_id("sn").value = name;
        el_by_id("wt").value = width;
        el_by_id("ht").value = height;

        el_by_id("cur_sz").innerHTML = el_xml_encode(name) + ", " + 
          width + "x" + height;
      }
    }
  }

});

function set_ad_size()
{
  var dlg = new SelectSizeDialog(); 
}

function preview_ad(link)
{
  var url = link + "/p/a?n=" + 
    el_mime_url_encode(el_by_id("an").value) + "&a=" + 
    el_mime_url_encode(el_by_id("tx").value);

  el_post_url(url, "prv", "_blank");
  return true;
}