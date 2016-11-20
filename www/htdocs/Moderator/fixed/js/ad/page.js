/////////////////////////////////////////////////////////////////////////////
// EditJMACDialog
/////////////////////////////////////////////////////////////////////////////

function EditJMACDialog(current_advertiser_id, advertiser_id)
{
  FullScreenDialog.call(this);

  this.current_advertiser_id = current_advertiser_id;
  this.advertiser_id = advertiser_id;
  this.add_mode = advertiser_id == 0;
  this.advertiser_name = "";

  this.max_ad_count = 
    this.add_mode ? 0 : parseInt(el_text(el_by_id("anc_" + advertiser_id)));

  this.new_advertiser_id = 0;
}

el_typedef(EditJMACDialog, FullScreenDialog,
{
  init: function() 
  { 
    var inner = this.el_call(FullScreenDialog, "init");

    var text = "<div id=\"advertisers\">Loading advertisers ...</div>\
<button onclick='el_dlg_by_id(\"" + this.id + "\").confirm_jmac()'>\
Ok</button>&nbsp;\
<button onclick='el_close_dlg_by_id(\"" + 
             this.id + "\", false)'>Cancel</button>";

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

    request.onreadystatechange = this.get_advertisers_req_handler(data);
    request.open("GET", "/psp/ad/json?o=advertisers", true);
    request.send("");
  },

  get_advertisers_req_handler: function(data)
  {
    var onready = function()
    {
      if(data.req.readyState == 4)
      {        
        switch(data.req.status)
        {
          case 200:
          {
            data.dlg.read_advertisers(data.req);
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

  read_advertisers: function(req)
  {
    if(this.closed)
    {
      return;
    }

    var objects = eval("(" + req.responseText + ")");
    var text = '<table id="advs"><tr class="option_row">\
<td colspan="2">Joint Max Ad Count</td>\
<td><input class="text_option" type="text" id="jmac" value="' + 
               this.max_ad_count + '"></td>\
</tr><tr><td colspan="3">&nbsp;</td></tr><tr id="header" align="center">\
<td>&nbsp;</td><td>Advertiser</td><td>Status</td></tr>';

    for(var i = 0; i < objects.length; ++i)
    {
      var obj = objects[i];

      if(el_by_id("an_" + obj.id) != null && obj.id != this.advertiser_id || 
         obj.id == this.current_advertiser_id)
      {
        continue;
      }

      if(el_by_id("cr_" + obj.id) == null)
      {
        text += '<tr class="option_row"><td class="radio_cell">\
<input id="adv_' + obj.id + '" type="radio" ondblclick="el_dlg_by_id(\'' + 
          this.id + 
          '\').confirm_jmac()" name="advertisers"' + 
          (obj.id == this.advertiser_id ? ' checked="checked"' : '') +
          '></td><td><span id="advn_' + 
          obj.id + '">' + 
          el_xml_encode(obj.name) + '</span></td><td class="number_option">' + 
          (obj.status ? "Disabled" : "Enabled") + '</td></tr>';
      }
    }

    text += '</table>';

    el_by_id("advertisers").innerHTML = text;
  },

  confirm_jmac: function()
  {
    this.max_ad_count = parseInt(el_by_id("jmac").value);

    if(isNaN(this.max_ad_count) || this.max_ad_count < 1)
    {
      alert("Joint Max Ad Count should be a positive number");
      return;
    }

    var advertisers = el_by_name("advertisers");
    var i = 0;
    for(; i < advertisers.length && !advertisers[i].checked; ++i);

    if(i < advertisers.length)
    {
      var obj = advertisers[i];
      var id = obj.id.split('_')[1];

      this.new_advertiser_id = id;
      this.advertiser_name = el_text(el_by_id("advn_" + id));

      el_close_dlg_by_id(this.id, true);
    }
    else
    {
      alert("No Advertiser Selected");
    }
  },

  on_ok: function()
  {
    var advertiser_name = el_xml_encode(this.advertiser_name);

    var menu_text = '<a href="javascript:edit_jmac(' + 
      this.current_advertiser_id + ',' + this.new_advertiser_id + 
      ')">edit</a>&nbsp;|&nbsp;<a href="javascript:delete_jmac(' +
      this.new_advertiser_id + 
      ')">delete</a><input type="hidden" value="' +
      this.new_advertiser_id + '_' + this.max_ad_count + '_' + 
      advertiser_name + '" name="an_' + this.new_advertiser_id + '">';

    if(this.add_mode)
    {
      var table = el_by_id("slots");

      for(var i = 0; i < table.rows.length; ++i)
      {
        if(table.rows[i].id == "add_jmac")
        {
          var row = table.insertRow(i);
          row.id = "an_" + this.new_advertiser_id;
          row.className = "option_row";

          var cell = row.insertCell(0);
          cell.innerHTML = advertiser_name;
          cell.id = "ann_" + this.new_advertiser_id;

          cell = row.insertCell(1);
          cell.className = "number_option";
          cell.id = "anc_" + this.new_advertiser_id;
          cell.innerHTML = this.max_ad_count;

          cell = row.insertCell(2);
          cell.id = "anm_" + this.new_advertiser_id;
          cell.innerHTML = menu_text;

          cell = row.insertCell(3);
          cell.colSpan = 3;

          break;
        }
      }
    }
    else
    {
      var obj = el_by_id("ann_" + this.advertiser_id);
      obj.innerHTML = advertiser_name;
      obj.id = "ann_" + this.new_advertiser_id;

      obj = el_by_id("anc_" + this.advertiser_id);
      obj.innerHTML = this.max_ad_count;
      obj.id = "anc_" + this.new_advertiser_id;

      obj = el_by_id("anm_" + this.advertiser_id);
      obj.innerHTML = menu_text;
      obj.id = "anm_" + this.new_advertiser_id;

      obj = el_by_id("an_" + this.advertiser_id);
      obj.id = "an_" + this.new_advertiser_id;
    }
  }

});

function add_jmac(advertiser_id)
{
  var dlg = new EditJMACDialog(advertiser_id, 0);
}

function edit_jmac(current_advertiser_id, advertiser_id)
{
  var dlg = new EditJMACDialog(current_advertiser_id, advertiser_id);
}

function delete_jmac(id)
{
  var table = el_by_id("slots");
  var rid = "an_" + id;

  for(var i = 0; i < table.rows.length; ++i)
  {
    if(table.rows[i].id == rid)
    {
      table.deleteRow(i);
      break;
    }
  }
}