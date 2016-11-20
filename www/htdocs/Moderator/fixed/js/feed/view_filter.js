var rule_number = 0;
var page_loaded = 0;

var post_refs = null;

function post(ref_index, new_window, pass_mod_params)
{
  var url = post_refs[ref_index];    
  el_post_url(url, null, new_window ? "_blank" : "");
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
}

function add_rule()
{
  var rules_choice = document.getElementById("rules_choice");
  var option = rules_choice.options[rules_choice.selectedIndex];

  var empty_array = new Array();
  add_specific_rule(option.value, empty_array, empty_array, false);
}

function add_specific_rule(name, operation, value, error)
{ 
  var new_rule_type = filter_rule_types[name];

  var filter_rules = document.getElementById("filter_rules");

  var row = filter_rules.insertRow(0);
  row.className = "option_row";

  row.id = "filter_rule_" + rule_number;

  var cell = row.insertCell(0);
  cell.innerHTML = new_rule_type.label;

  cell = row.insertCell(1);
  cell.innerHTML = new_rule_type.select_control(rule_number, name, operation);

  if(error)
  {
    var select = cell.getElementsByTagName('select');
    select[0].focus();
  }

  cell = row.insertCell(2);
  cell.innerHTML = new_rule_type.value_control(rule_number, name, value);

  if(error)
  {
    cell.className = "option_cell_error";
  }

  cell = row.insertCell(3);

  cell.innerHTML = '<a href="javascript:delete_rule(\''+ 
    row.id + '\');">delete</a>'

  rule_number = rule_number + 1;
}
