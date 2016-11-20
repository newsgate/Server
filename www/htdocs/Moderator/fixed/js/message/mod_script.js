function mod_init(params)
{
  params = mod_base_init(params);

  var mid_area = document.getElementById("mid_area");

  if(mid_area != null)
  {
    var element = document.createElement("div");
    element.className = "msg_man_buttons";

    element.innerHTML = '<input type="button" value="Delete marked messages" onclick="msg_man_delete_marked();"/>\
<input type="button" value="Select all" onclick="msg_man_select_all(true);"/>\
<input type="button" value="Deselect all" onclick="msg_man_select_all(false);"/>';

    mid_area.parentNode.insertBefore(element, mid_area);
  }

  var divs = document.getElementsByTagName("div");

  for(var i = 0; i < divs.length; ++i)
  {
    var d = divs[i];

    if(d.className == "msg_in")
    {
      var el_id = d.parentNode.id;
      var msg_id = el_id.substring(el_id.lastIndexOf('_') + 1);

      var element = document.createElement("div");

      element.className = "man_msg";
      element.id = "man_" + el_id;

      element.innerHTML = '<input type="checkbox" class="man_chk_msg" id="man_chk_' + el_id +
                          '"/> &nbsp;| &nbsp;<a href="javascript:msg_man_manage(\'del\', [\'' + msg_id +
                          '\']);">delete</a>';
     
      d.insertBefore(element, d.firstChild);
    }
  }
}

function msg_man_select_all(val)
{
  var controls = document.getElementsByTagName("input");

  for(var i = 0; i < controls.length; i++)
  {
    var input = controls[i];

    if(input.className == "man_chk_msg")
    {
      input.checked = val; 
    }
  }
}

function msg_man_delete_marked()
{
  var controls = document.getElementsByTagName("input");
  var msg_ids = new Array();
  var j = 0;

  for(var i = 0; i < controls.length; i++)
  {
    var input = controls[i];

    if(input.className == "man_chk_msg" && input.checked)
    {
      msg_ids[j++] = input.id.substring(input.id.lastIndexOf('_') + 1);
    }
  }

  if(msg_ids.length == 0)
  {
    alert("Nothing to delete");
    return;
  }

  var text = "You are about to delete " + msg_ids.length + " messages";

  if(!confirm(text))
  {
    return;
  }

  msg_man_manage('del', msg_ids);
}

function msg_man_manage(operation, msg_ids)
{
  if(operation != "del")
  {
    return;
  }

  var url = "/psp/message/manage?op=" + operation + "&ids=";
  var first = true;
  var deleted_msg_ids = [];

  for(var i = 0; i < msg_ids.length; ++i)
  {
    var id = msg_ids[i];
    var el = document.getElementById("man_msg_" + id);
      
    if(el != undefined)
    {
      el.innerHTML = "Deleting ...";
    }

    if(first)
    {
      first = false;
    }
    else
    {
      url += ',';
    }

    url += el_mime_url_encode(id);
    deleted_msg_ids.push(id);
  }

  var request;

  try
  {
    request = new ActiveXObject("Msxml2.XMLHTTP");
  }
  catch(e)
  {
    request = new XMLHttpRequest();
  }

  var data = { req:request, ids:deleted_msg_ids };

  request.onreadystatechange = 
    msg_man_request_onreadystatechange(data);

  request.open("GET", url, true);
  request.send("");
}

function msg_man_request_onreadystatechange(data)
{
  var onready = function()
  {
    if(data.req.readyState == 4)
    {
      switch(data.req.status)
      {
        case 200:
        {
          setTimeout(function() { msg_man_mark_deleted(data.ids) }, 5000 );
          break;
        }
        default:
        {
          msg_man_mark_failed(data.ids);

          var error = "Server communication failure. Error code: " + 
                      data.req.status + ".";

          if(data.req.status == 403)
          {
            error += 
              " You probably logged out or have not enough permissions.";
          }

          alert(error);
          break;
        }
      }
    }
  }

  return onready;
}

function msg_man_mark_failed(ids)
{
  for(var i = 0; i < ids.length; ++i)
  {
    var id = ids[i];
    var el = document.getElementById("man_msg_" + id);

    if(el != undefined)
    {
      el.innerHTML = "<span class='man_msg_error'>Failed to delete.\
</span> Probably you are not logged in.";
    }
  }
}

function msg_man_mark_deleted(ids)
{
  for(var i = 0; i < ids.length; ++i)
  {
    var id = ids[i];
    var el = document.getElementById("man_msg_" + id);

    if(el != undefined)
    {
      el.innerHTML = '<span class="man_msg_success">Deleted.</span>';
      el.className = el.className + " msg_del";

      el = document.getElementById("msg_" + id);

      if(el != undefined)
      {
        el.className = el.className + " msg_del";
      }
    }
  }
}
