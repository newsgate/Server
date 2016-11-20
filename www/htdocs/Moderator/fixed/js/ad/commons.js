/////////////////////////////////////////////////////////////////////////////
// AddConditionDialog
/////////////////////////////////////////////////////////////////////////////

function AddConditionDialog()
{
  FullScreenDialog.call(this);

  this.statuses = [ "Enabled", "Disabled", "Deleted" ];
  this.condition = 0;
  this.condition_name = "";
  this.condition_status = 0;

  var table = el_by_id("tbl");

  for(var i = 0; i < table.rows.length; ++i)
  {
    if(table.rows[i].id == "add_cond")
    {
      this.colSpan = table.rows[i].cells[0].colSpan - 2;
    }
  }
}

el_typedef(AddConditionDialog, FullScreenDialog,
{
  init: function() 
  { 
    var inner = this.el_call(FullScreenDialog, "init");

    var text = "<div id=\"conditions\">Loading conditions ...</div>\
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

    request.onreadystatechange = this.get_conditions_req_handler(data);
    request.open("GET", "/psp/ad/json?o=conditions", true);
    request.send("");
  },

  get_conditions_req_handler: function(data)
  {
    var onready = function()
    {
      if(data.req.readyState == 4)
      {        
        switch(data.req.status)
        {
          case 200:
          {
            data.dlg.read_conditions(data.req);
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

  read_conditions: function(req)
  {
    if(this.closed)
    {
      return;
    }

    var objects = eval("(" + req.responseText + ")");
    var text = '<table id="cndt"><tr id="header" align="center">\
<td>Name</td><td>Status</td></tr>';

    for(var i = 0; i < objects.length; ++i)
    {
      var obj = objects[i];

      if(el_by_id("cr_" + obj.id) == null)
      {
        text += '<tr class="option_row"><td><a href="javascript:el_dlg_by_id(\''+ 
                this.id + '\').add(' + obj.id + ', \'' + 
                el_js_escape(obj.name) + '\', ' + obj.status + ')">' + 
                el_xml_encode(obj.name) +
                '</a></td><td class="number_option">' + 
                this.statuses[obj.status] + '</td></tr>';
      }
    }

    text += '</table>';

    el_by_id("conditions").innerHTML = text;
  },

  add: function(id, name, status)
  {
    this.condition = id;
    this.condition_name = name;
    this.condition_status = status;

    el_close_dlg_by_id(this.id, true);
  },

  on_ok: function()
  {
    var table = el_by_id("tbl");
    var cname = el_xml_encode(this.condition_name);

    for(var i = 0; i < table.rows.length; ++i)
    {
      if(table.rows[i].id == "add_cond")
      {
        var row = table.insertRow(i);
        row.id = "cr_" + this.condition;
        row.className = "option_row";

        var cell = row.insertCell(0);

        cell.innerHTML = '<a href="/psp/ad/condition?id=' + this.condition +
          '">' + cname + '</a>';

        cell.colSpan = this.colSpan;

        cell = row.insertCell(1);
        cell.innerHTML = this.statuses[this.condition_status];

        cell = row.insertCell(2);
        cell.innerHTML = '<a href="javascript:delete_condition(' + 
          this.condition + ')">delete</a>\n\
<input type="hidden" value="' + this.condition_status + '_' + cname +
          '" name="cn_' + this.condition + '">'

        break;
      }
    }
  }

});

function add_condition()
{
  var dlg = new AddConditionDialog(); 
}

function delete_condition(id)
{
  var table = el_by_id("tbl");
  var rid = "cr_" + id;

  for(var i = 0; i < table.rows.length; ++i)
  {
    if(table.rows[i].id == rid)
    {
      table.deleteRow(i);
      break;
    }
  }
}