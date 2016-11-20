var rule_number = 0;

function set_cursor_pos(text_area, pos)
{
  if(text_area.selectionStart !== undefined)
  {
    text_area.selectionStart = pos;
    text_area.selectionEnd = pos;
  }
  else
  {
    var text_range = text_area.createTextRange();
    text_range.collapse(true);
    text_range.moveStart("character", pos);
    text_range.moveEnd("character", 0);
    text_range.select();     
  }
}

function delete_rule(rule_id)
{
  var filter_rules = document.getElementById("filter_rules");
  var rows = filter_rules.rows;

  for(var i = 0; i < rows.length; i++)
  {
    if(rows[i].id == rule_id)
    {
      filter_rules.deleteRow(i);
      break;
    }
  }

  if(filter_rules.rows.length == 1)
  {
    filter_rules.deleteRow(0);
  }
}

function add_rule()
{
  add_specific_rule("", "", -1);
}

function add_specific_rule(expression, description, error_pos)
{ 
  var filter_rules = document.getElementById("filter_rules");

  if(filter_rules.rows.length == 0)
  {
    var row = filter_rules.insertRow(0);
    row.id = "header";

    var cell = row.insertCell(0);
    cell.innerHTML = "Expression";
    
    cell = row.insertCell(1);
    cell.innerHTML = "Description";
    
  }

  var row = filter_rules.insertRow(1);
  row.className = "option_row";

  row.id = "filter_rule_" + rule_number;

  var cell = row.insertCell(0);
  cell.innerHTML = "<textarea name=\"e_" + rule_number + 
                   "\" rows=\"5\" cols=\"50\">" + expression +
                   "</textarea>";

  if(error_pos > 0)
  {
    cell.className = "rule_cell_error";
    var expr_area = cell.getElementsByTagName('textarea');

    expr_area[0].focus();
    set_cursor_pos(expr_area[0], error_pos - 1);
  }

  cell = row.insertCell(1);
  cell.innerHTML = "<textarea name=\"d_" + rule_number + 
                   "\" rows=\"5\" cols=\"50\">" + description +
                   "</textarea>";

  cell = row.insertCell(2);

  cell.innerHTML = '<a href="javascript:delete_rule(\''+ 
    row.id + '\');">delete</a>'

  rule_number = rule_number + 1;
}
