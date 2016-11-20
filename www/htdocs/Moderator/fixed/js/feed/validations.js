var page_loaded;
var is_ie = false;

//
// TaskInfoManager
//

function dummy()
{
}

function get_taskop_request_onreadystatechange(data)
{
  var onready = function()
  {
    if(data.req.readyState == 4)
    {
      switch(data.req.status)
      {
        case 200:
        {
          if(data.subs != undefined)
          {
            var show_subordinates_tasks = 
              document.getElementById("show_subordinates_tasks");

            var new_arg = data.subs == 1 ? 0 : 1;
            var new_text = data.subs == 1 ? "Hide" : "Show";

            show_subordinates_tasks.innerHTML = 
              "<a href=\"javascript:task_info_manager.task_op('subs', " + 
              new_arg + ");\">" + new_text + "</a>";
          }
      
          task_info_manager.make_taskinfo_request(false);
          break;
        }
        case 401: 
        {
          window.location = "validations";
          break;
        }
      }
    }
  }

  return onready;
}

function get_taskinfo_request_onreadystatechange(data)
{
  var onready = function()
  {
    if(data.req.readyState == 4)
    {
      switch(data.req.status)
      {
        case 200:
        {
          task_info_manager.onload(data.req);
          break;
        }
        case 401: 
        {
          window.location = "validations";
          break;
        }
      }

      if(data.multiple)
      {
        setTimeout('task_info_manager.make_taskinfo_request(true);', 2000);
      }

      delete data.req.onreadystatechange;
      data.req.onreadystatechange = dummy;

      delete data.req;
      data.req = null;

      delete data;
    }
  }

  return onready;
}

TaskInfoManager.prototype = 
{
  validation_op_url : "/psp/feed/validation_op",

  task_op : function(op, arg)
  {
    with(this)
    {
      if(op == "interrupt" || op == "delete")
      {
        var id = arg;
        var operations_id = document.getElementById("op_" + id);
      
        if(operations_id != undefined)
        {
          operations_id.innerHTML = "";
        }

        var status_id = document.getElementById("status_" + id);
      
        if(status_id != undefined)
        {
          status_id.innerHTML = "...";
        }
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

      var interrupt_url = validation_op_url + "?o=" + op + "&a=" + arg;

      var data = { req:request, subs:undefined };
        
      if(op == "subs")
      {
        data.subs = arg;
      }

      request.onreadystatechange = 
        get_taskop_request_onreadystatechange(data);

      request.open("GET", interrupt_url, true);
      request.send("");
    }
  },

  make_taskinfo_request : function(send_multiple)
  {
    with(this)
    {
      var request;

      try
      {
        request = new ActiveXObject("Msxml2.XMLHTTP");
        is_ie = true;
      }
      catch(e)
      {
        request = new XMLHttpRequest();
      }

      try
      {
        var data = { req:request, multiple:send_multiple };
        
        request.onreadystatechange = 
          get_taskinfo_request_onreadystatechange(data);

        request.open("GET", "/psp/feed/validation_tasks", true);
        request.send("");
      }
      catch(e)
      {
        alert(e);
      }
    }
  },

  onload : function(request)
  {
    if(page_loaded == undefined)
    {
      return;
    }

    with(this)
    {
      var tasks_table = document.getElementById("tasks");

      var text = '<tbody><tr align="center" id="header"><td>User</td><td>Started</td><td>Feeds\
</td><td>Pending</td><td>Requests</td><td>Traffic</td><td>Status</td><td>Operations</td></tr>\n';
      var new_location;

      if(request.responseXML != null)
      {
        var tasks_xml = request.responseXML.getElementsByTagName("task");

        for(var i = 0; i < tasks_xml.length; i++)
        {
          var node = tasks_xml[i];
          var status = node.getAttribute("status");
          var id = node.getAttribute("id");
          var class_val = "option_row";

          if(wait_task == id)
          {
            class_val = " wait_task";

            if(status == "S")
            {
              new_location = "/psp/feed/validation_result?a=" + wait_task;
            }
          }

          text += "<tr title=\"" + node.getAttribute("title") + 
                  "\" class=\"" + class_val + "\"><td class='cell'>" + 
                  node.getAttribute("creator") + "</td><td class='cell'>" + 
                  node.getAttribute("started") + "</td><td class='cell'>" +
                  node.getAttribute("feeds") + "</td><td class='cell'>" + 
                  node.getAttribute("pending_urls") + "</td><td class='cell'>" + 
                  node.getAttribute("processed_urls") + "</td><td class='cell'>" + 
                  node.getAttribute("received_bytes") + 
                  "</td><td class='cell' id='status_" + id + "'>" + 
                  status + "</td><td class='cell' id='op_" + id + "'>";

          var submit_link = "<a href=\"" + validation_op_url + 
            "?o=submit&a=" + id + "\">submit</a>";

          var interrupt_link = 
            "<a href=\"javascript:task_info_manager.task_op('interrupt', '" +
            id + "');\">interrupt</a>";

          var delete_link = 
            "<a href=\"javascript:task_info_manager.task_op('delete', '" +
            id + "');\">delete</a>";

          if(status == "A")
          {
            text += interrupt_link + " | " + delete_link;
          }
          else if(status == "S")
          {
            text += submit_link + " | " + delete_link;
          }
          else if(status == "E")
          {
            text += delete_link;
          }
          else if(status == "I")
          {
            text += submit_link + " | " + delete_link;
          }

          text += "</td></tr>";
        }
      }

      text += "</tbody>";

      if(is_ie)
      {
        tasks_table.outerHTML = '<table id="tasks">' + text + "</table>";
      }
      else
      {
        tasks_table.innerHTML = text;
      }

      if(new_location != undefined)
      {
        window.location = new_location;
      }
    }
  }

};

function TaskInfoManager()
{
}

var task_info_manager = new TaskInfoManager();
task_info_manager.make_taskinfo_request(true);

