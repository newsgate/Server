function filter_tables()
{
  var tables = document.getElementsByTagName('table');

  for(var i = 0; i < tables.length; ++i)
  {
    filter_table(tables[i]);
  }

  setTimeout("filter_tables()", 500);
}

function filter_table(table)
{
  var rows = table.rows;

  for(var i = 0; i < rows.length; ++i)
  {
    var row = rows[i];

    var classes = row.className.split(' ');
    var filter_row = false;

    for(var j = 0; j < classes.length && !filter_row; ++j)
    {
      if(classes[j] == "filter_row")
      {
        filter_row = true;
      }
    }

    if(!filter_row)
    {
      continue;
    }

    var display = true;

    for(var k = 0; k < row.cells.length && display; ++k)
    {
      var cell = row.cells[k];
      classes = cell.className.split(' ');

      for(var l = 0; l < classes.length && display; ++l)
      {
        var cl = classes[l];

        if(cl.substr(0, 11) == "filter_row_")
        {
          var edit = el_by_id(cl.substr(11));

          if(edit)
          {
            if(edit.value == undefined) alert(edit.id);

//            var filter = edit.value.toLowerCase();

            var filter = edit.value;

            if(filter == "")
            {
              continue;
            }

            filter = new RegExp(filter, "i");
            display = filter.test(el_full_text(cell));

//            var cell_text = el_full_text(cell).toLowerCase();
//            display = filter == "" || cell_text.indexOf(filter) >= 0;
          }
        }
      }
    }

    row.style.display = display ? "table-row" : "none";

  }  
}

/////////////////////////////////////////////////////////////////////////////
// FullScreenDialog
/////////////////////////////////////////////////////////////////////////////

function FullScreenDialog(on_destroy)
{
  ElDialog.call(this);

  var s = this.node.style;
  s.margin = "0";
  s.padding = "0";
  s.borderWidth = 0;
  s.minHeight = "100%";
  s.minWidth = "100%";
  s.fontSize = "80%";
  s.left = 0;
  s.top = 0;
}

el_typedef(FullScreenDialog, ElDialog,
{
  close_name: null,
  ok_name: null,
  singleton: true,

  init: function() 
  { 
    var d = document;
    var b = d.body;

    this.doc = (d.doctype || d.documentMode > 7 || d.firstChild.tagName=='!') ?
               d.documentElement : b;

    this.scrollTop = this.doc.scrollTop;

    this.elems =
    [ 
      { id:"skeleton" }
    ];

    for(var i = 0; i < this.elems.length; ++i)
    {
      var elem = this.elems[i];
      var obj = el_by_id(elem.id);

      if(obj && obj.style.display != "none")
      {
        elem.display = obj.style.display;
        obj.style.display = "none";
      }
    }

    this.doc.scrollTop = 0;

    return { text: "", style: 'padding:0.7em;' };
  },

  on_destroy: function()
  {
    for(var i = 0; i < this.elems.length; ++i)
    {
      var elem = this.elems[i];
    
      if(elem.display != undefined)
      {
        var obj = el_by_id(elem.id);

        if(obj)
        {
          obj.style.display = elem.display;
        }
      }
    }

    this.doc.scrollTop = this.scrollTop;
  }
});

