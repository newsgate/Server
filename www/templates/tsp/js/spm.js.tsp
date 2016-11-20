var cat_col_min_width = 200;
var more_category_height = null;

function getElementHeight(obj)
{
  if(obj == null || obj == undefined)
  {
    return 0;
  }

  with(obj)
  {
    if(typeof clip !== "undefined")
    {
      return clip.height;
    } 
    else 
    {
      if(style.pixelHeight) 
      {
        return style.pixelHeight;
      } 
      else 
      {
        return offsetHeight;
      }
    }
  }
}

function sub_cat_cmp(a, b)
{
  var res = b.msg - a.msg;

  if(res)
  {
    return res;
  }

  var an = a.name.toLowerCase();
  var bn = b.name.toLowerCase();

  return an < bn ? -1 : (an > bn ? 1 : 0);
}

function more_element(cell)
{
  var childs = cell.childNodes;

  for(var i = 0; i < childs.length; i++)
  {
    var child = childs[i];

    if(child.className == "more_cat")
    {
      return child;
    }
  }

  return null;
}

var trace_window_resize = false;

function on_window_resize()
{
  if(!auto_size)
  {
    return;
  }

  var rect = el_window_rect();

  var body_width = rect.w;
  var body_height = rect.h;
  var text = "";

  if(trace_window_resize)
  {
    text += "Size: " + document.body.clientWidth + ":" + body_width + 'x' + 
            document.body.clientHeight + ":" + body_height + "; ";
  }

  var table_rows = document.getElementById("categories_table").rows;

  if(trace_window_resize)
  {
    text += "table_rows: " + table_rows.length + "<br>";
  }

  var sub_cats = 0;
  var row_style_visible = null;

  if(table_rows.length && table_rows[0].cells.length)
  {
    var row = table_rows[0];
    row_style_visible = row.style.display;

    if(more_category_height == null)
    {
      more_category_height = 
        Math.max(getElementHeight(more_element(row.cells[0])), 1);
    }

    var subcategory_height = more_category_height;
    var category_height = Math.round(subcategory_height * 11 / 9);

    if(trace_window_resize)
    {
      text += "category height: " + category_height + "/" +
              subcategory_height + "/" + more_category_height + "; ";
    }

    var fr_height = getElementHeight(document.getElementById("fr"));
    var cr_height = getElementHeight(document.getElementById("copyright"));
    var tt_height = getElementHeight(document.getElementById("top_bar"));
    var cn_height = getElementHeight(document.getElementById("counters"));

    var bars_height = fr_height + cr_height + tt_height + cn_height +
      3.3 * category_height; 

    if(trace_window_resize)
    {
      text += "bars_height: " + bars_height + "/" + fr_height + "/" + 
              cr_height + "/" + tt_height + "/" + cn_height + "; ";
    }

    var available_height = Math.max(body_height - bars_height, 0);

    if(trace_window_resize)
    {
      text += "available_height: " + available_height + "; ";
    }

    sub_cats = Math.max(Math.round((available_height / table_rows.length - 
        2 * category_height) / subcategory_height), 0);

    var cat_col_width = body_width / row.cells.length;
    var decr = 0;

    if(cat_col_width < cat_col_min_width)
    {
      decr = (cat_col_min_width - cat_col_width) / 2;
      sub_cats = Math.max(Math.floor(sub_cats - decr), 0);
    }

    if(trace_window_resize)
    {
      text += "sub_cats: " + sub_cats + ":" + body_width + "/" + 
              cat_col_width + "/" + decr + "/" +
              row.cells.length + "; ";
    }
  }

  for(var i = 0; i < table_rows.length; i++)
  {
    var row = table_rows[i];

    for(var j = 0; j < row.cells.length; j++)
    {
      var cell = row.cells[j];

      var category = cell.firstChild;
      var subcategories = new Array();

      for(var subcat = category.nextSibling; subcat != cell.lastChild; 
          subcat = subcat.nextSibling)
      {
        var spans = subcat.getElementsByTagName("span");
        var msg_count = 0;

        for(var k = 0; k < spans.length; k++)
        {
          if(spans[k].className == "cat_msg_count")
          {
            msg_count = eval(el_text(spans[k]));
            break;
          }
        }  

        var link = subcat.getElementsByTagName("a")[0];

        subcategories.push(
          { obj: subcat, name: el_full_text(link), msg: msg_count} );
      }

      subcategories.sort(sub_cat_cmp);

      var threshold = sub_cats < subcategories.length ? 
        sub_cats - 1 : sub_cats;

      var lines = 0;
      var hidden_cats = "";
      var cat_shown = false;
      
      if(trace_window_resize)
      {
        text += " threshold-" + threshold;
      }

      for(var k = 0; k < subcategories.length; k++)
      {
        var s = subcategories[k];

//        if(trace_window_resize)
//        {
//          text += " " + s.name + ":" + s.name.length; //s.msg;
//        }

        var cat_lines = Math.ceil(s.name.length / (no_break_cat_len - 1));
        var show = lines + cat_lines <= threshold; 

        s.obj.style.display = show ? row_style_visible : "none";
        lines += cat_lines;

//        if(trace_window_resize)
//        {
//          text += " " + s.name + ":" + cat_lines;
//        }

        if(!show && s.obj.className != "more_cat")
        {
          if(hidden_cats != "")
          {
            hidden_cats += ", ";
          }

          hidden_cats += s.name + " (" + s.msg + ")"
        }

        if(!cat_shown)
        {
          cat_shown = show;
        }
      }

      var more = more_element(cell);

      more.style.display = 
        hidden_cats != "" && cat_shown ? row_style_visible : "none";

      more.title = hidden_cats;

//      if(i == 0 && j==0)
//        text += cell.lastChild.innerHTML + ":" + cat_hidden + "; ";
    }
  }

  if(trace_window_resize)
  {
    document.getElementById("copyright").innerHTML = text;
  }
}

function on_load()
{
  window.onresize = on_window_resize;
  window.onorientationchange = on_window_resize;
  on_window_resize();

  el_by_id("q").focus();
}

function search()
{
  var params = "";
  var query = el_trim(el_by_id("q").value.replace(/\r/g, ""));

  if(query)
  {
    params += "q=" + el_mime_url_encode(query);
  }

  if(page.search.query)
  {
    if(params)
    {
      params += "&";
    }

    params += page.search.query;
  }

  if(page.translator.query_suffix)
  {
    if(params) params += "&";
    params += page.translator.query_suffix;
  }

  navigate(page.search.path + (params ? "?" + params : ""), false);
  return false;
}

function on_key_press(event)
{ 
  if(event.keyCode == 13)
  {
    search();
    return false;
  }

  return true;
}
