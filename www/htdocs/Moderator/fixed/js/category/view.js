function unlink_category(id)
{
  var table = document.getElementById("child_categories");
  var row_id = "cat_" + id;

  for(var i = 0; i < table.rows.length; i++)
  {
    if(table.rows[i].id == row_id)
    {
      if(!confirm("You are about to delete category " + 
                  document.getElementById("crf_" + id).title + 
                  " with all its descendants"))
      {
        return;
      }

      table.rows[i].cells[1].innerHTML = "deleting ...";

      var request;

      try
      {
        request = new ActiveXObject("Msxml2.XMLHTTP");
      }
      catch(e)
      {
        request = new XMLHttpRequest();
      }

      var data = { req:request, id:id };

      request.onreadystatechange = 
        delete_category_request_onreadystatechange(data);

      request.open("GET", "delete?c=" + id, true);
      request.send("");

      break;
    }
  }
}

function delete_category_request_onreadystatechange(data)
{
  var onready = function()
  {
    if(data.req.readyState == 4)
    {
      var table = document.getElementById("child_categories");
      var row_id = "cat_" + data.id;

      for(var i = 0; i < table.rows.length; i++)
      {
        if(table.rows[i].id == row_id)
        {
          switch(data.req.status)
          {
            case 200:
            {
              table.deleteRow(i);
              break;
            }
            default:
            {
              table.rows[i].cells[1].innerHTML = 
                "Server communication failure. Error code: " + 
                data.req.status;

              break;
            }
          }

          return;
        }
      }
    }
  }

  return onready;
}
