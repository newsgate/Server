/////////////////////////////////////////////////////////////////////////////
// SelectPageDialog
/////////////////////////////////////////////////////////////////////////////

function SelectPageDialog()
{
  FullScreenDialog.call(this);
  this.page_id = 0;
}

el_typedef(SelectPageDialog, FullScreenDialog,
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

    request.onreadystatechange = this.get_pages_req_handler(data);
    request.open("GET", "/psp/ad/json?o=pages", true);
    request.send("");
  },

  init: function() 
  { 
    var inner = this.el_call(FullScreenDialog, "init");

    var text = "<div id=\"pages\">Loading pages ...</div>\
<button onclick='el_dlg_by_id(\"" + this.id + "\").confirm_page()'>\
Select</button>&nbsp;\
<button onclick='el_close_dlg_by_id(\"" + 
               this.id + "\", false)'>Cancel</button>";

    inner.text = text;
    return inner;
  },

  get_pages_req_handler: function(data)
  {
    var onready = function()
    {
      if(data.req.readyState == 4)
      {        
        switch(data.req.status)
        {
          case 200:
          {
            data.dlg.read_pages(data.req);
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

  read_pages: function(req)
  {
    if(this.closed)
    {
      return;
    }

    var page_id = el_by_id("pg").value;

    var objects = eval("(" + req.responseText + ")");
    var text = '<table id="pgt"><tr id="header" align="center">\
<td>&nbsp;</td><td>Name</td></td><td>Status</td></tr>';

    var prev_page = 0;
    var cr = parseInt(el_by_id("cr").value);

    for(var i = 0; i < objects.length; ++i)
    {
      var obj = objects[i];

      text += '<tr class="option_row"><td class="radio_cell"><input id="pg_'+
              obj.id + '" type="radio" name="pages"' + 
              (obj.id == page_id ? ' checked="checked"' : '') + 
              ' ondblclick="el_dlg_by_id(\'' + this.id + 
              '\').confirm_page()"></td><td id="n_' + 
              obj.id + '">' + el_xml_encode(obj.name) +
              '</td><td>' + (obj.status ? "Disabled" : "Enabled") + 
              '</td></tr>';
    }

    text += '</table>';

    el_by_id("pages").innerHTML = text;
  },

  confirm_page: function()
  {
    var pages = el_by_name("pages");
    var i = 0;
    for(; i < pages.length && !pages[i].checked; ++i);

    if(i < pages.length)
    {
      el_close_dlg_by_id(this.id, true);
    }
    else
    {
      alert("No Page Selected");
    }
  },

  on_ok: function()
  {
    var pages = el_by_name("pages");

    for(var i = 0; i < pages.length; ++i)
    {
      var obj = pages[i];

      if(obj.checked)
      {
        var id = obj.id.split("_")[1];
        var name = el_text(el_by_id("n_" + id));

        el_by_id("pg").value = id;
        el_by_id("gn").value = name;
        el_by_id("cur_page").innerHTML = el_xml_encode(name);
      }
    }
  }

});

/////////////////////////////////////////////////////////////////////////////
// SelectCounterDialog
/////////////////////////////////////////////////////////////////////////////

function SelectCounterDialog()
{
  FullScreenDialog.call(this);
  this.counter_id = 0;
}

el_typedef(SelectCounterDialog, FullScreenDialog,
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

    request.onreadystatechange = this.get_counter_req_handler(data);
    request.open("GET", "/psp/ad/json?o=counters", true);
    request.send("");
  },

  init: function() 
  { 
    var inner = this.el_call(FullScreenDialog, "init");

    var text = "<div id=\"counters\">Loading counters ...</div>\
<button onclick='el_dlg_by_id(\"" + this.id + "\").confirm_counter()'>\
Select</button>&nbsp;\
<button onclick='el_close_dlg_by_id(\"" + 
               this.id + "\", false)'>Cancel</button>";

    inner.text = text;
    return inner;
  },

  get_counter_req_handler: function(data)
  {
    var onready = function()
    {
      if(data.req.readyState == 4)
      {        
        switch(data.req.status)
        {
          case 200:
          {
            data.dlg.read_counters(data.req);
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

  read_counters: function(req)
  {
    if(this.closed)
    {
      return;
    }

    var counter_id = el_by_id("cr").value;

    var objects = eval("(" + req.responseText + ")");
    var text = '<table id="crt"><tr id="header" align="center">\
<td>&nbsp;</td><td>Name</td><td>Status</td></tr>';

    for(var i = 0; i < objects.length; ++i)
    {
      var obj = objects[i];

      text += '<tr class="option_row"><td class="radio_cell"><input id="cr_'+
              obj.id + '" type="radio" name="counters"' + 
              (obj.id == counter_id ? ' checked="checked"' : '') + 
              ' ondblclick="el_dlg_by_id(\'' + this.id + 
              '\').confirm_counter()"></td><td><a id="n_' + 
              obj.id + '"href="/psp/ad/counter?id=' + obj.id + '">' + 
              el_xml_encode(obj.name) + '</a></td><td>' + 
              (obj.status ? "Disabled" : "Enabled") + 
              '</td></tr>';
    }

    text += '</table>';

    el_by_id("counters").innerHTML = text;
  },

  confirm_counter: function()
  {
    var counters = el_by_name("counters");
    var i = 0;
    for(; i < counters.length && !counters[i].checked; ++i);

    if(i < counters.length)
    {
      el_close_dlg_by_id(this.id, true);
    }
    else
    {
      alert("No Counter Selected");
    }
  },

  on_ok: function()
  {
    var counters = el_by_name("counters");

    for(var i = 0; i < counters.length; ++i)
    {
      var obj = counters[i];

      if(obj.checked)
      {
        var id = obj.id.split("_")[1];
        var name = el_text(el_by_id("n_" + id));

        el_by_id("cr").value = id;
        el_by_id("cn").value = name;

        el_by_id("cur_counter").innerHTML = el_xml_encode(name);
      }
    }
  }

});

function set_page()
{
  var dlg = new SelectPageDialog(); 
}

function set_counter()
{
  var dlg = new SelectCounterDialog(); 
}

function set_name(id)
{ 
  el_by_id("pn").value = el_full_text(el_by_id(id));
}

function set_mix(id)
{ 
  var text = el_by_id('cn').value;
  var page = el_by_id('gn').value;

  if(text && page) text += " on ";
  text += page;

  el_by_id("pn").value = text;
}
