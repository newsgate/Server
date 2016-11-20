/////////////////////////////////////////////////////////////////////////////
// SelectSlotDialog
/////////////////////////////////////////////////////////////////////////////

function SelectSlotDialog()
{
  FullScreenDialog.call(this);
  this.slot_id = 0;
}

el_typedef(SelectSlotDialog, FullScreenDialog,
{
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

  init: function() 
  { 
    var inner = this.el_call(FullScreenDialog, "init");

    var text = "<div id=\"slots\">Loading slots ...</div>\
<button onclick='el_dlg_by_id(\"" + this.id + "\").confirm_slot()'>\
Select</button>&nbsp;\
<button onclick='el_close_dlg_by_id(\"" + 
               this.id + "\", false)'>Cancel</button>";

    inner.text = text;
    return inner;
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

    var slot_id = el_by_id("sl").value;

    var objects = eval("(" + req.responseText + ")");
    var text = '<table id="slt"><tr id="header" align="center">\
<td>&nbsp;</td><td>Name</td></td><td>Min Width</td><td>Max Width</td>\
<td>Min Height</td><td>Max Height</td><td>Status</td></tr>';

    var prev_page = 0;
    var width = parseInt(el_by_id("zw").value);
    var height = parseInt(el_by_id("zh").value);
    var ad = parseInt(el_by_id("ad").value);

    for(var i = 0; i < objects.length; ++i)
    {
      var obj = objects[i];

      if(prev_page != obj.page)
      {
        text += '<tr><td></td><td class="page_name">' + 
                el_xml_encode(obj.page_name) + 
                '</td><td colspan="5"></td></tr>';
      }

      var style = ad && (width < obj.min_width || width > obj.max_width ||
         height < obj.min_height || height > obj.max_height) ?
         ' style="background-color:rgb(240, 240, 240);"' : "";

      var pos = obj.name.indexOf(" of ");
      var name = obj.name.substr(0, pos);

      text += '<tr class="option_row"' + style + 
              '><td class="radio_cell"><input id="sz_'+
              obj.id + '" type="radio" name="slots"' + 
              (obj.id == slot_id ? ' checked="checked"' : '') + 
              ' ondblclick="el_dlg_by_id(\'' + this.id + 
              '\').confirm_slot()"><span id="fn_' + obj.id +
              '" style="display:none">' + el_xml_encode(obj.name) +
              '</span></td><td id="n_' + 
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

    el_by_id("slots").innerHTML = text;
  },

  confirm_slot: function()
  {
    var slots = el_by_name("slots");
    var i = 0;
    for(; i < slots.length && !slots[i].checked; ++i);

    if(i < slots.length)
    {
      el_close_dlg_by_id(this.id, true);
    }
    else
    {
      alert("No Slot Selected");
    }
  },

  on_ok: function()
  {
    var slots = el_by_name("slots");

    for(var i = 0; i < slots.length; ++i)
    {
      var obj = slots[i];

      if(obj.checked)
      {
        var id = obj.id.split("_")[1];
        var name = el_text(el_by_id("fn_" + id));
        var min_width = el_text(el_by_id("wn_" + id));
        var max_width = el_text(el_by_id("wx_" + id));
        var min_height = el_text(el_by_id("hn_" + id));
        var max_height = el_text(el_by_id("hx_" + id));

        el_by_id("sl").value = id;
        el_by_id("sn").value = name;
        el_by_id("wn").value = min_width;
        el_by_id("wx").value = max_width;
        el_by_id("hn").value = min_height;
        el_by_id("hx").value = max_height;

        el_by_id("cur_slot").innerHTML = el_xml_encode(name) + ", [" + 
          min_width + " - " + max_width + "] x [" + min_height + 
          " - " + max_height + "]";
      }
    }
  }

});

/////////////////////////////////////////////////////////////////////////////
// SelectAdDialog
/////////////////////////////////////////////////////////////////////////////

function SelectAdDialog()
{
  FullScreenDialog.call(this);
  this.ad_id = 0;
}

el_typedef(SelectAdDialog, FullScreenDialog,
{
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

    request.onreadystatechange = this.get_ad_req_handler(data);
    request.open("GET", "/psp/ad/json?o=ads", true);
    request.send("");
  },

  init: function() 
  { 
    var inner = this.el_call(FullScreenDialog, "init");

    var text = "<div id=\"ads\">Loading ads ...</div>\
<button onclick='el_dlg_by_id(\"" + this.id + "\").confirm_ad()'>\
Select</button>&nbsp;\
<button onclick='el_close_dlg_by_id(\"" + 
               this.id + "\", false)'>Cancel</button>";

    inner.text = text;
    return inner;
  },

  get_ad_req_handler: function(data)
  {
    var onready = function()
    {
      if(data.req.readyState == 4)
      {        
        switch(data.req.status)
        {
          case 200:
          {
            data.dlg.read_ads(data.req);
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

  read_ads: function(req)
  {
    if(this.closed)
    {
      return;
    }

    var ad_id = el_by_id("ad").value;

    var objects = eval("(" + req.responseText + ")");
    var text = '<table id="adt"><tr id="header" align="center">\
<td>&nbsp;</td><td>Name</td><td>Size Name</td><td>Width</td><td>Height</td>\
<td>Status</td></tr>';

    var min_width = parseInt(el_by_id("wn").value);
    var max_width = parseInt(el_by_id("wx").value);
    var min_height = parseInt(el_by_id("hn").value);
    var max_height = parseInt(el_by_id("hx").value);
    var slot = parseInt(el_by_id("sl").value);

    for(var i = 0; i < objects.length; ++i)
    {
      var obj = objects[i];

      var style = slot && (obj.width < min_width || obj.width > max_width ||
         obj.height < min_height || obj.height > max_height) ?
         ' style="background-color:rgb(240, 240, 240);"' : "";

      text += '<tr class="option_row"' + style +
              '><td class="radio_cell"><input id="sz_'+
              obj.id + '" type="radio" name="ads"' + 
              (obj.id == ad_id ? ' checked="checked"' : '') + 
              ' ondblclick="el_dlg_by_id(\'' + this.id + 
              '\').confirm_ad()"></td><td><a  id="n_' + 
              obj.id + '"href="/psp/ad/ad?id=' + obj.id + '">' + 
              el_xml_encode(obj.name) + '</a></td><td id="zn_' + 
              obj.id + '">' + el_xml_encode(obj.size_name) +
              '</td><td class="number_option" id="zw_' + 
              obj.id + '">' + obj.width + 
              '</td><td class="number_option" id="zh_' + 
              obj.id + '">' + obj.height + 
              '</td><td>' + (obj.status ? "Disabled" : "Enabled") + 
              '</td></tr>';
    }

    text += '</table>';

    el_by_id("ads").innerHTML = text;
  },

  confirm_ad: function()
  {
    var ads = el_by_name("ads");
    var i = 0;
    for(; i < ads.length && !ads[i].checked; ++i);

    if(i < ads.length)
    {
      el_close_dlg_by_id(this.id, true);
    }
    else
    {
      alert("No Ad Selected");
    }
  },

  on_ok: function()
  {
    var ads = el_by_name("ads");

    for(var i = 0; i < ads.length; ++i)
    {
      var obj = ads[i];

      if(obj.checked)
      {
        var id = obj.id.split("_")[1];
        var name = el_text(el_by_id("n_" + id));
        var size_name = el_text(el_by_id("zn_" + id));
        var width = el_text(el_by_id("zw_" + id));
        var height = el_text(el_by_id("zh_" + id));

        el_by_id("ad").value = id;
        el_by_id("an").value = name;
        el_by_id("zn").value = size_name;
        el_by_id("zw").value = width;
        el_by_id("zh").value = height;

        el_by_id("cur_ad").innerHTML = el_xml_encode(name) + ", " +
          el_xml_encode(size_name) + ", " + width + "x" + height;
      }
    }
  }

});

function set_slot()
{
  var dlg = new SelectSlotDialog(); 
}

function set_ad()
{
  var dlg = new SelectAdDialog(); 
}

function set_name(id)
{ 
  el_by_id("pn").value = el_by_id(id).value;
}

function set_mix(id)
{ 
  var text = el_by_id('an').value;
  var slot = el_by_id('sn').value;

  if(text && slot) text += " in ";
  text += slot;

  el_by_id("pn").value = text;
}
