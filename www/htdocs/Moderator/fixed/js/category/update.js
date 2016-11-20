var locale_number = 1;
var expression_number = 1;
var word_list_number = 1;
var newsgate_link = null;

var normalization_dialog_number = 1;
var normalization_dialog_holder = null;

var languages = null;
var countries = null;
var included_messages = null;
var excluded_messages = null;

var deleted_word_lists = new Array();

var last_key_press = new Date();
var last_key_press_timeout = 3;

el_close_all_dialogs_hook = function() {  }

function prefix_exp(text)
{
  return prefix_text(text, 40);
}

function prefix_desc(text)
{
  return prefix_text(text, 40);
}

function prefix_text(text, len)
{
  if(text.length > len + 3)
  {
    return text.substr(0, len) + "...";
  }
  else
  {
    return text;
  }
}

// 
// Category management
//

var logger_num = 0;

function keyPressHandler(e)
{
  var evt = e || window.event;

  last_key_press = new Date();

  if(e.returnValue === false)
  {
    return;
  }

  if(evt.keyCode == 27)
  {
    if(FindDialog.prototype.close_all())
    { 
      e.returnValue = false;
    }
    else if(el_close_dlg_by_id("", false))
    {
      e.returnValue = false;
    }
    else if(update_dialog_holder != null)
    { 
      close_update_dialog_ex();
      e.returnValue = false;
    }
  }
  else if(evt.keyCode == 13)
  {
    if(update_dialog_holder != null)
    {
      if(update_dialog_holder.type == "locale")
      {
        update_locale_dialog_ok();
        e.returnValue = false;
        return;
      }

      if(update_dialog_holder.type == "word_list" &&
         evt.target == update_dialog_holder.name)
      {
        update_word_list_dialog_ok();
        e.returnValue = false;
      }
    }
  }
}

function close_update_dialog_ex()
{
  FindDialog.prototype.close_all();
  WordNormDialog.prototype.close_all();
  close_update_dialog();
}

function category_selected(categories_selected, update_dialog_holder)
{
  if(update_dialog_holder.category_id)
  {
    unlink_category(update_dialog_holder.category_id, 
                    update_dialog_holder.id_prefix);
  }

  for(var i = 0; i < categories_selected.length; i++)
  {
    var info = categories_selected[i];

    unlink_category(info.id, update_dialog_holder.id_prefix);

    add_category(info.name, 
                 info.id, 
                 update_dialog_holder.id_prefix, 
                 update_dialog_holder.category_parent,
                 true);
  }
}

function add_category(name, id, prefix, parent, edit)
{
  var table = document.getElementById(prefix + "_table");

  var index = 0;
  for(; index < table.rows.length && table.rows[index].name < name; index++);

  var cat_id = prefix + "_" + id;
  var row = table.insertRow(index);

  row.className = "option_row";
  row.id = cat_id;
  row.name = name;
  
  var catn_id = prefix + "n_" + id;
  var cell = row.insertCell(0);

  cell.innerHTML = "<span id=\"" + prefix + "p_" + id + "\"></span>" + 
    "<a href=\"view?c=" + id + "\">" +
    el_xml_encode(name) + "</a><input type=\"hidden\" id=\"" + 
    catn_id + "\" name=\"" + catn_id + "\" value=\"" + el_xml_encode(name) + 
    "\"/>";

  cell = row.insertCell(1);
  
  var text = "";

  if(edit)
  {
    text += "<a href=\"javascript:select_category_dialog('" + 
      id + "', '" + prefix + 
      "', " + parent + ", '" + prefix + "p_" + id + "'" +
      ", false, false, 'category_selected')\">change</a>&#xA0;|&#xA0;<a href=\"javascript:unlink_category('" + 
      id + "', '" + prefix + "')\">unlink</a>";
  }

  if(id > 1)
  {
    if(edit)
    {
      text += "&#xA0;|&#xA0;";
    }

    text += "<a href=\"update?c=" + id + "\">edit</a>";
  }

  cell.innerHTML = text;
  category_changed("1");
}

function unlink_category(id, prefix)
{
  var table = document.getElementById(prefix + "_table");
  var exp_id = prefix + "_" + id;

  for(var i = 0; i < table.rows.length; i++)
  {
    if(table.rows[i].id == exp_id)
    {
      table.deleteRow(i);
      category_changed("1");
      return;
    }
  }
}

function category_changed(value)
{
  document.getElementById("ch").value = value;
}

function word_list_changed(value)
{
  document.getElementById("wh").value = value;
}

function name_cmp(l1, l2)
{
  var a = l1.name.toLowerCase();
  var b = l2.name.toLowerCase();

  return a == b ? 0 : a < b ? -1 : 1;
}

// 
// Expression management
//

function add_expression(expression, description, edit)
{
  var expressions = document.getElementById("expressions");

  var exp_id = "exp_" + expression_number;
  var row = expressions.insertRow(0);

  row.className = "option_row";
  row.id = exp_id;
  
  var expv_id = "expv_" + expression_number;
  var cell = row.insertCell(0);

  cell.innerHTML = "<span id=\"uep_" + expression_number + "\"></span>" + 
    "<span id=\"evl_" + expression_number + "\">" +
    el_xml_encode(prefix_exp(expression)) + 
    "</span><input type=\"hidden\" id=\"" + 
    expv_id + "\" name=\"" + expv_id + "\"/>";

  document.getElementById(expv_id).value = expression;

  var expd_id = "expd_" + expression_number;
  cell = row.insertCell(1);

  cell.innerHTML = "<span id=\"edl_" + expression_number + "\">" + 
    el_xml_encode(prefix_desc(description)) + 
    "</span><input type=\"hidden\" id=\"" + 
    expd_id + "\" name=\"" + expd_id + "\"/>";

  document.getElementById(expd_id).value = description;

  cell = row.insertCell(2);

  if(edit)
  {
    cell.innerHTML = "<a href=\"javascript:update_expression_dialog('" + 
      expression_number + 
      "', true)\">edit</a>&#xA0;|&#xA0;<a href=\"javascript:delete_expression('" + 
      expression_number + "')\">delete</a>";
  }
  else
  {
    cell.innerHTML = "<a href=\"javascript:update_expression_dialog('" + 
      expression_number + "', false)\">view</a>";
  }

  expression_number++;
}

function delete_expression(number)
{
  var expressions = document.getElementById("expressions");
  var exp_id = "exp_" + number;

  for(var i = 0; i < expressions.rows.length; i++)
  {
    if(expressions.rows[i].id == exp_id)
    {
      expressions.deleteRow(i);
      category_changed("1");
      return;
    }
  }
}

function find_word_list(name)
{
  for(var i = 1; i < word_list_number; i++)
  {
    var wl_name = document.getElementById("wlsn_" + i);

    if(wl_name != null && wl_name.value == name)
    {
      return document.getElementById("wlsv_" + i).value;
    }
  }

  return null;
}

function expand_word_lists(exp)
{
  var str = exp;
  var new_exp = "";

  while(true)
  {
    var pos = str.search(/{{LIST:.+}}/);

    if(pos < 0)
    {
      new_exp += str;
      break;
    }

    var end = str.indexOf('}}', pos);
    new_exp += str.substr(0, pos);

    pos += 7;
    var name = str.substr(pos, end - pos);
    str = str.substr(end + 2);

    var wl = find_word_list(name);

    if(wl == null)
    {
      alert("Can't find word list '" + name + "'");
      return null;
    }
    else
    {
      new_exp += wl;
    }
  }

  return new_exp;
}

function search_category_expression(relax_search_level)
{
  var expression = ""
  var expression_count = 0;

  for(var i = 1; i < expression_number; i++)
  {
    var exp = document.getElementById("expv_" + i);

    if(exp != null)
    {
      exp = expand_word_lists(exp.value);

      if(exp == null)
      {
        return;
      }

      if(expression_count == 0)
      {
        expression = exp;
      }
      else if(expression_count == 1)
      {
        expression = "( " + expression + " )\nOR\n( " + exp + " )";
      }
      else
      {
        expression = expression + "\nOR\n( " + exp + " )";
      }

      expression_count++;
    }
  }

  if(excluded_messages.length)
  {
    if(expression == "")
    {
      expression = "NONE";
    }

    expression = "( " + expression + " ) EXCEPT MSG";

    for(var i = 0; i < excluded_messages.length; ++i)
    {
      expression += " " + excluded_messages[i];
    }
  }

  if(included_messages.length)
  {
    if(expression != "")
    {
      expression += " OR ";
    }

    expression += "MSG"

    for(var i = 0; i < included_messages.length; ++i)
    {
      expression += " " + included_messages[i];
    }
  }

  if(expression)
  {
    search_expression_query(expression, undefined, relax_search_level);
  }
}

function search_expression(expand_lists)
{
  var sel = update_dialog_holder.expression.el_get_selection();
  var text = update_dialog_holder.expression.value.replace(/\r/g, "");

  if(sel.start == sel.end)
  {
    search_expression_query(text, expand_lists);
  }
  else
  {
    search_expression_query(text.substr(sel.start, sel.end - sel.start), 
                            expand_lists);
  }  
}

function update_locale_dialog(number)
{
  close_update_dialog_ex();

  var uep_id = "ulp_" + number;
  var place_holder = document.getElementById(uep_id);

  var ok_label = "ok";

  var locale_lang = number ?
    document.getElementById("locl_" + number).value : -1;

  var locale_country = number ?
    document.getElementById("locc_" + number).value : -1;

  var text = 
"<div class=\"update_locale_dialog dialog\">\n\
<table class=\"update_locale_dialog_table\">"

  text += "<tr><td>Language:</td><td><select class='locale_lang' id=\"ull_" + 
          number + "\">";

  for(var i = 0; i < languages.length; i++)
  {
    var l = languages[i];

    text += "\n<option id='ullo_" + l.id + "'" + 
            (l.id == locale_lang ? ' selected="selected"' : "") +
            ">" + el_xml_encode(l.name) + "</option>";
  }

  text += "\n</select></td></tr>";

  text += "<tr><td>Country:</td><td><select class='locale_country' id=\"ulc_" +
          number + "\">";

  for(var i = 0; i < countries.length; i++)
  {
    var c = countries[i];

    text += "\n<option id='ulco_" + c.id + "'" + 
            (c.id == locale_country ? ' selected="selected"' : "") +
            ">" + el_xml_encode(c.name) + "</option>";
  }

  text += "\n</select></td></tr>";

  text +=
"<tr><td>Name:</td><td><input type=\"text\" id=\"uln_" + number + 
      "\" class=\"update_locale_dialog_name\"/></td></tr>\
<tr><td valign=\"top\">Title:</td><td><textarea id=\"ult_" + number + 
      "\" class=\"update_locale_dialog_meta\" rows=\"3\"></textarea></td></tr>\
<tr><td valign=\"top\">Short Title:</td><td><textarea id=\"uls_" + number + 
      "\" class=\"update_locale_dialog_meta\" rows=\"3\"></textarea></td></tr>\
<tr><td valign=\"top\">Description:</td><td><textarea id=\"uld_" + number + 
      "\" class=\"update_locale_dialog_meta\" rows=\"3\"></textarea></td></tr>\
<tr><td valign=\"top\">Keywords:</td><td><textarea id=\"ulk_" + number + 
      "\" class=\"update_locale_dialog_meta\" rows=\"3\"></textarea></td></tr>\
</table>\n<a href=\"javascript:update_locale_dialog_ok();\">" + ok_label + 
      "</a>\
    <a href=\"javascript:close_update_dialog_ex();\">cancel</a></div>"

  place_holder.innerHTML = text;

  place_holder.locale_name = 
    el_enrich(document.getElementById("uln_" + number));

  place_holder.locale_title = 
    el_enrich(document.getElementById("ult_" + number));

  place_holder.locale_short_title = 
    el_enrich(document.getElementById("uls_" + number));

  place_holder.locale_description = 
    el_enrich(document.getElementById("uld_" + number));

  place_holder.locale_keywords = 
    el_enrich(document.getElementById("ulk_" + number));

  place_holder.locale_lang = 
    document.getElementById("ull_" + number);

  place_holder.locale_country = 
    document.getElementById("ulc_" + number);

  place_holder.locale_lang.focus();

  if(number)
  {
    place_holder.locale_name.value = 
      document.getElementById("locn_" + number).value;

    place_holder.locale_title.value = 
      document.getElementById("loct_" + number).value;

    place_holder.locale_short_title.value = 
      document.getElementById("locs_" + number).value;

    place_holder.locale_description.value = 
      document.getElementById("locd_" + number).value;

    place_holder.locale_keywords.value = 
      document.getElementById("lock_" + number).value;
  }

  place_holder.number = number;
  place_holder.type = "locale";

  update_dialog_holder = place_holder;
}

function update_locale_dialog_ok()
{
  if(update_dialog_holder == null)
  {
    return;
  }

  var sel_lang = update_dialog_holder.locale_lang.selectedIndex;

  if(sel_lang < 0)
  {
    alert("No language selected");
    update_dialog_holder.locale_lang.focus();
    return;
  }

  var sel_country = update_dialog_holder.locale_country.selectedIndex;

  if(sel_country < 0)
  {
    alert("No country selected");
    update_dialog_holder.locale_country.focus();
    return;
  }

  var lang_opt = update_dialog_holder.locale_lang[sel_lang];
  var lang = lang_opt.id.split("_")[1];

  var country_opt = update_dialog_holder.locale_country[sel_country];
  var country = country_opt.id.split("_")[1];

  var number = update_dialog_holder.number;
  var locales = document.getElementById("locales");

  for(var i = 0; i < locales.rows.length; i++)
  {
    var row_num = locales.rows[i].id.split("_")[1];

    if(row_num != number)
    {
      var l = document.getElementById("locl_" + row_num).value;
      var c = document.getElementById("locc_" + row_num).value;
      
      if(lang == l && country == c)
      {
        alert("Category name for language " + lang_opt.value + ", country " + 
              country_opt.value + " already specified");

        return;
      }
    }
  }

  var name = el_trim(update_dialog_holder.locale_name.value);
/*  
  if(name == "")
  {
    alert("Category localized name can't be empty");
    update_dialog_holder.locale_name.focus();
    return;
  }
*/

  var title = el_trim(update_dialog_holder.locale_title.value);
  var short_title = el_trim(update_dialog_holder.locale_short_title.value);
  var description = el_trim(update_dialog_holder.locale_description.value);
  var keywords = el_trim(update_dialog_holder.locale_keywords.value);

  if(number)
  {
    document.getElementById("lll_" + number).innerHTML = 
      el_xml_encode(lang_opt.value);

    document.getElementById("lcl_" + number).innerHTML = 
      el_xml_encode(country_opt.value);

    document.getElementById("locl_" + number).value = lang;
    document.getElementById("locc_" + number).value = country;

    document.getElementById("lnl_" + number).innerHTML = 
      el_xml_encode(name);

    document.getElementById("locn_" + number).value = name;
    document.getElementById("loct_" + number).value = title;
    document.getElementById("locs_" + number).value = short_title;
    document.getElementById("locd_" + number).value = description;
    document.getElementById("lock_" + number).value = keywords;
  }
  else
  {
    update_dialog_holder.number = locale_number;

    add_locale(lang, 
               country, 
               name, 
               title, 
               short_title, 
               description, 
               keywords, 
               true);
  }

  category_changed("1");

  close_update_dialog_ex();  
  document.getElementById("add_locale_link").focus();
}

function add_locale(lang, 
                    country, 
                    name, 
                    title, 
                    short_title, 
                    description, 
                    keywords, 
                    edit)
{
  var i = 0;
  for(; i < languages.length && languages[i].id != lang; i++);

  if(i == languages.length)
  {
    return;
  }

  var j = 0;
  for(; j < countries.length && countries[j].id != country; j++);

  if(j == countries.length)
  {
    return;
  }

  var locales = document.getElementById("locales");

  var loc_id = "loc_" + locale_number;
  var row = locales.insertRow(0);

  row.className = "option_row";
  row.id = loc_id;
  
  var locl_id = "locl_" + locale_number;
  var cell = row.insertCell(0);

  cell.innerHTML = "<span id=\"ulp_" + locale_number + "\"></span>" + 
    "<span id=\"lll_" + locale_number + "\">" +
    el_xml_encode(languages[i].name) + 
    "</span><input type=\"hidden\" id=\"" + 
    locl_id + "\" name=\"" + locl_id + "\"/>";

  document.getElementById(locl_id).value = lang;

  var locc_id = "locc_" + locale_number;
  cell = row.insertCell(1);

  cell.innerHTML = "<span id=\"lcl_" + locale_number + "\">" +
    el_xml_encode(countries[j].name) + 
    "</span><input type=\"hidden\" id=\"" + 
    locc_id + "\" name=\"" + locc_id + "\"/>";

  document.getElementById(locc_id).value = country;

  var locn_id = "locn_" + locale_number;
  var loct_id = "loct_" + locale_number;
  var locs_id = "locs_" + locale_number;
  var locd_id = "locd_" + locale_number;
  var lock_id = "lock_" + locale_number;

  cell = row.insertCell(2);

  cell.innerHTML = "<span id=\"lnl_" + locale_number + "\">" + 
    el_xml_encode(name) +
    "</span><input type=\"hidden\" id=\"" + 
    locn_id + "\" name=\"" + locn_id + "\"/><input type=\"hidden\" id=\"" + 
    loct_id + "\" name=\"" + loct_id + "\"/><input type=\"hidden\" id=\"" + 
    locs_id + "\" name=\"" + locs_id + "\"/><input type=\"hidden\" id=\"" + 
    locd_id + "\" name=\"" + locd_id + "\"/><input type=\"hidden\" id=\"" + 
    lock_id + "\" name=\"" + lock_id + "\"/>";

  document.getElementById(locn_id).value = name;
  document.getElementById(loct_id).value = title;
  document.getElementById(locs_id).value = short_title;
  document.getElementById(locd_id).value = description;
  document.getElementById(lock_id).value = keywords;

  if(edit)
  {
    cell = row.insertCell(3);
    cell.innerHTML = "<a href=\"javascript:update_locale_dialog('" + 
      locale_number + 
      "')\">edit</a>&#xA0;|&#xA0;<a href=\"javascript:delete_locale('" + 
      locale_number + 
      "')\">delete</a>";
  }

  locale_number++;
}

function delete_locale(number)
{
  var locales = document.getElementById("locales");
  var loc_id = "loc_" + number;

  for(var i = 0; i < locales.rows.length; i++)
  {
    if(locales.rows[i].id == loc_id)
    {
      locales.deleteRow(i);
      category_changed("1");
      return;
    }
  }
}

// 
// Word list management
//

function add_word_list(id,
                       name, 
                       expression, 
                       version, 
                       updated, 
                       description,
                       del, 
                       edit)
{
  var word_lists = document.getElementById("word_lists");

  var wls_id = "wls_" + word_list_number;
  var row = word_lists.insertRow(0);

  row.className = "option_row";
  row.id = wls_id;
  
  var wlsn_id = "wlsn_" + word_list_number;
  var wlsv_id = "wlsv_" + word_list_number;
  var wlsr_id = "wlsr_" + word_list_number;
  var wlsu_id = "wlsu_" + word_list_number;
  var wlsi_id = "wlsi_" + word_list_number;

  var cell = row.insertCell(0);

  cell.innerHTML = "<span id=\"uwlp_" + word_list_number + "\"></span>" + 
    "<span id=\"wlnl_" + word_list_number + "\">" +
    el_xml_encode(name) + 
    "</span><input type=\"hidden\" id=\"" + 
    wlsn_id + "\" name=\"" + wlsn_id + "\"/><input type=\"hidden\" id=\"" + 
    wlsv_id + "\" name=\"" + wlsv_id + "\"/><input type=\"hidden\" id=\"" + 
    wlsr_id + "\" name=\"" + wlsr_id + "\"/><input type=\"hidden\" id=\"" + 
    wlsu_id + "\" name=\"" + wlsu_id + "\"/><input type=\"hidden\" id=\"" +
    wlsi_id + "\" name=\"" + wlsi_id + "\"/>" +
    "</span>";

  document.getElementById(wlsn_id).value = name;
  document.getElementById(wlsv_id).value = expression;
  document.getElementById(wlsr_id).value = version;
  document.getElementById(wlsu_id).value = updated;
  document.getElementById(wlsi_id).value = id ? id : name;

  var wlsd_id = "wlsd_" + word_list_number;
  cell = row.insertCell(1);

  cell.innerHTML = "<span id=\"wldl_" + word_list_number + "\">" + 
    el_xml_encode(prefix_desc(description)) + 
    "</span><input type=\"hidden\" id=\"" + 
    wlsd_id + "\" name=\"" + wlsd_id + "\"/>";

  document.getElementById(wlsd_id).value = description;

  cell = row.insertCell(2);

  var text = "";

  if(edit)
  {
    text = "<a href=\"javascript:update_word_list_dialog(" + 
      word_list_number + ", " + (del ? "true" : "false") + 
      ", true)\">edit</a>";
  }
  else
  {
    text = "<a href=\"javascript:update_word_list_dialog(" + 
      word_list_number + ", false, false)\">view</a>";
  }

  if(del)
  {
    text += "&#xA0;|&#xA0;<a href=\"javascript:delete_word_list(" + 
    word_list_number + ")\">delete</a>";
  }

  cell.innerHTML = text;

  if(version == 0)
  {
    document.getElementById("wlsu_" + word_list_number).value = "1";
  }

  word_list_number++;
  category_changed("1");
}

function update_word_list_dialog(number,
                                 rename,
                                 edit,
                                 error_msg, 
                                 error_pos, 
                                 name_error,
                                 sel,
                                 add_text,
                                 clean_source,
                                 save)
{
  close_update_dialog_ex();

  var ph_id = "uwlp_" + number;
  var place_holder = document.getElementById(ph_id);

  var ok_label = "ok";
  var ta_id = "uwlv_" + number;
  var name_id = "uwln_" + number;
  var menu_id = "uwlm_" + number;

  var text = 
"<div class=\"update_word_list_dialog dialog\">\n\
Name:<input type=\"text\" id=\"" + name_id + 
      "\" class=\"update_word_list_dialog_edit\"";

  if(!rename)
  {
    text += " readonly=\"readonly\"";
  }

  text += "/>\n<div id=\"" + menu_id + "\">Word List:\
      <a href=\"javascript:search_expression(false)\" class=\"mod_menu_link\">search</a>\n\
&#xA0;|&#xA0;<a href=\"javascript:google_selection()\" class=\"mod_menu_link\">google</a>\n\
&#xA0;|&#xA0;<a href=\"javascript:translate_selection()\" class=\"mod_menu_link\">translate</a>\n\
&#xA0;|&#xA0;<a href=\"javascript:find_selection('" + menu_id +
"')\" class=\"mod_menu_link\">find</a>";

  if(edit)
  {
    text += "\n<br><a href=\"javascript:toupper_words(false)\" class=\"mod_menu_link\">lower</a>\n\
&#xA0;|&#xA0;<a href=\"javascript:toupper_words(true)\" class=\"mod_menu_link\">upper</a>\n\
&#xA0;|&#xA0;<a href=\"javascript:capitalize_words()\" class=\"mod_menu_link\">capitalize</a>\n\
&#xA0;|&#xA0;<a href=\"javascript:normalize_words('" + menu_id +
"')\" class=\"mod_menu_link\">normalize</a>\n\
&#xA0;|&#xA0;<a href=\"javascript:sort_words()\" class=\"mod_menu_link\">sort</a>\n\
&#xA0;|&#xA0;<a href=\"javascript:dedup_words()\" class=\"mod_menu_link\">dedup</a>"
  }

  text += "</div>\n<textarea id=\"" + ta_id + 
"\" class=\"update_word_list_dialog_edit\" rows=\"20\" \
onfocus=\"mod_edit_set_focus('" + ta_id + "', true)\" \
onblur=\"mod_edit_set_focus('" + ta_id + "', false)\"";

  if(!edit)
  {
    text += " readonly=\"readonly\"";
  }

  text += "></textarea>\n";

  if(error_msg)
  {
    text += "<div class=\"error\">" + error_msg + "</div>";
  }

  text += "     Description:\n\
      <textarea id=\"uwld_" + number + 
      "\" class=\"update_word_list_dialog_edit\" rows=\"2\"";

  if(!rename)
  {
    text += " readonly=\"readonly\"";
  }

  text += "></textarea>\n      <br>";

  if(edit)
  {
    text += "<a href=\"javascript:update_word_list_dialog_ok();\">" + 
            ok_label + "</a>\
    <a href=\"javascript:close_update_dialog_ex();\">cancel</a>\n\
    <a href=\"javascript:update_word_list_dialog_apply();\">apply</a>";
  }
  else
  {
    text += "<a href=\"javascript:close_update_dialog_ex();\">close</a>";
  }

  text += "</div>";

  place_holder.innerHTML = text;

  place_holder.name = el_enrich(document.getElementById("uwln_" + number));

  place_holder.expression = 
    el_enrich(document.getElementById("uwlv_" + number));

  place_holder.description = 
    document.getElementById("uwld_" + number);

  if(number)
  {
    place_holder.name.value = document.getElementById("wlsn_" + number).value;

    place_holder.expression.value = 
      document.getElementById("wlsv_" + number).value;

    place_holder.description.value = 
      document.getElementById("wlsd_" + number).value;
  }

  place_holder.last_expression = "";
  place_holder.number = number;

  if(error_pos > 0)
  {
    sel = { start:error_pos - 1, end:error_pos - 1};
  }
  else if(add_text)
  {
    add_text = add_text.replace(/\r/g, "");

    if(add_text)
    {
      place_holder.expression.el_insert(add_text + "\n", { start:0, end:0});
      sel = { start:0, end:add_text.length};

      if(clean_source && window.opener != null)
      {
        var source = window.opener.document.getElementById(clean_source);

        if(el_trim(source.value.replace(/\r/g, "")) == el_trim(add_text))
        {
          source.value = "";
        }
      }

      last_focus_edit = place_holder.expression;
    }
  }
  else if(sel === undefined || sel == null)
  {
    sel = { start:0, end:0};
  }

  if(name_error || !number)
  {
    place_holder.name.el_set_selection(sel, true);
  }
  else
  {
    place_holder.expression.el_set_selection(sel, true);
  }

  place_holder.type = "word_list";

  update_dialog_holder = place_holder;
  refresh_session();

  if(save && error_pos <= 0)
  {
    update_word_list_dialog_ok();
    setTimeout('el_by_id("u").click();', 1);
  }
}

function update_word_list_dialog_ok()
{
  if(update_word_list())
  {
    close_update_dialog_ex();
  }
}

function update_word_list_dialog_apply()
{
  update_word_list();
}

function update_word_list()
{
  if(update_dialog_holder == null)
  {
    return true;
  }

  var number = update_dialog_holder.number;
  var name = el_trim(update_dialog_holder.name.value);

  if(name == "")
  {
    alert("Name is not specified");
    update_dialog_holder.name.focus();
    return false;
  }

  var word_lists = document.getElementById("word_lists");

  for(var i = 0; i < word_lists.rows.length; i++)
  {
    var row_num = word_lists.rows[i].id.split("_")[1];

    if(row_num != number)
    {
      var nm = document.getElementById("wlsn_" + row_num).value;

      if(name == nm)
      {
        alert("Word list name " + name + " is already taken");
        update_dialog_holder.name.focus();
        return false;
      }
    }
  }

  var exp = dedup_words(sort_words(update_dialog_holder.expression.value));
  var desc = update_dialog_holder.description.value.replace(/\r/g, "");
  
  if(number)
  {
    document.getElementById("wlnl_" + number).innerHTML = 
      el_xml_encode(name);

    var name_obj = document.getElementById("wlsn_" + number);
    var old_name = name_obj.value;
    name_obj.value = name;

    var value_obj = document.getElementById("wlsv_" + number);
    var old_value = value_obj.value.replace(/\r/g, "");
    value_obj.value = exp;

    document.getElementById("wldl_" + number).innerHTML = 
      el_xml_encode(prefix_desc(desc));

    var desc_obj = document.getElementById("wlsd_" + number);
    var old_desc = desc_obj.value.replace(/\r/g, "");
    desc_obj.value = desc;

    if(old_name != name)
    {
      category_changed("1");
      document.getElementById("wlsu_" + number).value = "1";
    }
    
    if(old_value != exp || old_desc != desc)
    { 
      word_list_changed("1");
      document.getElementById("wlsu_" + number).value = "1";
    }
  }
  else
  {
    update_dialog_holder.number = word_list_number;
    add_word_list("", name, exp, 0, 0, desc, true, true);
  }

  update_dialog_holder.expression.focus();
  return true;
}

function delete_word_list(number)
{
  var word_lists = document.getElementById("word_lists");
  var wls_id = "wls_" + number;

  for(var i = 0; i < word_lists.rows.length; i++)
  {
    if(word_lists.rows[i].id == wls_id)
    {
      deleted_word_lists.push(el_by_id("wlsi_" + number).value);
      word_lists.deleteRow(i);
      category_changed("1");
      return;
    }
  }
}


function update_expression_dialog(number, edit, error_msg, error_pos)
{
  close_update_dialog_ex();

  var uep_id = "uep_" + number;
  var place_holder = document.getElementById(uep_id);

  var ok_label = "ok";
  var ta_id = "uev_" + number;
  var menu_id = "uwlm_" + number;
  
  var text = 
"<div class=\"update_expression_dialog dialog\">\n<div id=\"" + menu_id + 
"\">Expression:\
      <a href=\"javascript:search_expression(true)\" class=\"mod_menu_link\">search</a>\n\
&#xA0;|&#xA0;<a href=\"javascript:google_selection()\" class=\"mod_menu_link\">google</a>\n\
&#xA0;|&#xA0;<a href=\"javascript:translate_selection()\" class=\"mod_menu_link\">translate</a>\n\
&#xA0;|&#xA0;<a href=\"javascript:find_selection('" + menu_id +
"')\" class=\"mod_menu_link\">find</a>";

  if(edit)
  {
    text += "\n<br><a href=\"javascript:toupper_words(false)\" class=\"mod_menu_link\">lower</a>\n\
&#xA0;|&#xA0;<a href=\"javascript:toupper_words(true)\" class=\"mod_menu_link\">upper</a>\n\
&#xA0;|&#xA0;<a href=\"javascript:capitalize_words()\" class=\"mod_menu_link\">capitalize</a>\n\
&#xA0;|&#xA0;<a href=\"javascript:normalize_words('" + menu_id +
"')\" class=\"mod_menu_link\">normalize</a>\n\
&#xA0;|&#xA0;<a href=\"javascript:sort_words()\" class=\"mod_menu_link\">sort</a>\n\
&#xA0;|&#xA0;<a href=\"javascript:dedup_words()\" class=\"mod_menu_link\">dedup</a>";
  }

  text += "</div>\n<textarea id=\"" + ta_id  + 
"\" class=\"update_expression_dialog_edit\" rows=\"20\" \
onfocus=\"mod_edit_set_focus('" + ta_id + "', true)\" \
onblur=\"mod_edit_set_focus('" + ta_id + "', false)\"";

  if(!edit)
  {
    text += " readonly=\"readonly\"";
  }

  text += "></textarea>\n";

  if(error_msg)
  {
    text += "<div class=\"error\">" + error_msg + "</div>";
  }

  text +=
"     Description:\n\
      <textarea id=\"ued_" + number + 
      "\" class=\"update_expression_dialog_edit\" rows=\"2\"";

  if(!edit)
  {
    text += " readonly=\"readonly\"";
  }

  text += "></textarea>";

  if(edit)
  {
    text += "\n      <br><a href=\"javascript:update_expression_dialog_ok();\">" + ok_label + 
      "</a>\
    <a href=\"javascript:close_update_dialog_ex();\">cancel</a>\n\
    <a href=\"javascript:update_expression_dialog_apply();\">apply</a>";
  }
  else
  {
    text += "\n      <br><a href=\"javascript:close_update_dialog_ex();\">close</a>";
  }  

  text += "</div>";

  place_holder.innerHTML = text;

  place_holder.expression = 
    el_enrich(document.getElementById("uev_" + number));

  place_holder.description = 
    document.getElementById("ued_" + number);

  if(number)
  {
    place_holder.expression.value = 
      document.getElementById("expv_" + number).value;

    place_holder.description.value = 
      document.getElementById("expd_" + number).value;
  }

  place_holder.last_expression = "";
  place_holder.number = number;

  var sel = null;

  if(error_pos)
  {
    sel = { start:error_pos - 1, end:error_pos - 1};
  }
  else
  {
    sel = { start:0, end:0};
  }

  place_holder.expression.el_set_selection(sel, true)
  place_holder.type = "expression";

  update_dialog_holder = place_holder;
  refresh_session();
}

function update_expression_dialog_apply()
{
  if(update_dialog_holder == null)
  {
    return;
  }

  var number = update_dialog_holder.number;

  var exp = update_dialog_holder.expression.value.replace(/\r/g, "");
  var desc = update_dialog_holder.description.value.replace(/\r/g, "");

  if(number)
  {
    document.getElementById("evl_" + number).innerHTML = 
      el_xml_encode(prefix_exp(exp));

    var exp_obj = document.getElementById("expv_" + number);
    var old_exp = exp_obj.value.replace(/\r/g, "");
    exp_obj.value = exp;

    document.getElementById("edl_" + number).innerHTML = 
      el_xml_encode(prefix_desc(desc));

    var desc_obj = document.getElementById("expd_" + number);
    var old_desc = desc_obj.value.replace(/\r/g, "");
    desc_obj.value = desc;

    if(old_exp != exp || old_desc != desc)
    { 
      category_changed("1");
    }
  }
  else
  {
    update_dialog_holder.number = expression_number;
    add_expression(exp, desc, true);
    category_changed("1");
  }

  update_dialog_holder.expression.focus();
}

function update_expression_dialog_ok()
{
  update_expression_dialog_apply();
  close_update_dialog_ex();
}

function check_versions()
{
  var obj = el_by_id("c");

  if(obj == null)
  {
    return;
  }

  var params = "c=" + obj.value;
  var request;

  try
  {
    request = new ActiveXObject("Msxml2.XMLHTTP");
  }
  catch(e)
  {
    request = new XMLHttpRequest();
  }

  var data = { req:request };

  request.onreadystatechange = 
    get_cat_version_onreadystatechange(data);

  request.open("POST", "/psp/category/versions", true);

  request.setRequestHeader("Content-type", 
                           "application/x-www-form-urlencoded");
  
  request.send(params);
}

function get_cat_version_onreadystatechange(data)
{
  var onready = function()
  { 
    if(data.req.readyState == 4)
    {
      cat_version_check_result(data.req);
      setTimeout('check_versions();', 5000);
    }
  }

  return onready;
}

function cat_version_check_result(request)
{
  var msg_obj = el_by_id("version_check");

  if(request.status != 200)
  {
    if(request.status == 0)
    {
      var cur_time = new Date();

      if(cur_time - last_key_press < last_key_press_timeout * 1000)
      {
        return;
      }
    }

    var error = "Server communication failure. Error code: " + 
                request.status + ".";

    if(request.status == 403)
    {
      error += " You probably logged out or have not enough permissions.";
    }

    msg_obj.innerHTML = error;
    return;
  }

  var text = ""

  var category = el_child_node(request.responseXML, "c");
  var current_version = parseInt(category.getAttribute("v"));
  var version = parseInt(el_by_id("v").value);

  if(version != current_version)
  {
    if(current_version)
    {
      text += "Category changed."
    }
    else
    {
      text += "Category deleted."
      msg_obj.innerHTML = text;
      return;
    }
  }

  var inputs = document.getElementsByTagName("input");
  var word_lists = new Array();

  for(var i = 0; i < inputs.length; i++)
  {
    var obj = inputs[i];
    var parts = obj.id.split("_");
    var prefix = parts[0];

    if(prefix == "wlsn")
    {
      var number = parts[1];

      word_lists.push( { name: el_by_id("wlsi_" + number).value, 
                         version: el_by_id("wlsr_" + number).value 
                       } );
    }
  }

  var new_word_lists = new Array();
  var word_lists_nodes = category.childNodes;

  for(var i = 0; i < word_lists_nodes.length; ++i)
  {
    var wl = word_lists_nodes[i];

    if(wl.tagName == "l")
    {
      new_word_lists.push( { name: wl.getAttribute("n"), 
                             version: wl.getAttribute("v")
                           } );
    }
  }

  var word_list_change_text = "";
  var word_list_deleted_text = "";
  var word_list_added_text = "";

  for(var i = 0; i < word_lists.length; ++i)
  {
    var wl = word_lists[i];
    var j = 0;

    for(; j < new_word_lists.length; ++j)
    {
      var nwl = new_word_lists[j];

      if(wl.name == nwl.name)
      {
        if(wl.version != nwl.version)
        {
          if(word_list_change_text)
          {
            word_list_change_text += ","
          }

          word_list_change_text += " " + wl.name;
        }

        break;
      }
    }

    if(j == new_word_lists.length && wl.version != "0")
    {
      if(word_list_deleted_text)
      {
        word_list_deleted_text += ","
      }

      word_list_deleted_text += " " + wl.name;
    }
  }

  for(var i = 0; i < new_word_lists.length; ++i)
  {
    var nwl = new_word_lists[i];
    var j = 0;

    for(; j < word_lists.length && word_lists[j].name != nwl.name; ++j);

    if(j == word_lists.length && 
       deleted_word_lists.indexOf(nwl.name) < 0)
    {
      if(word_list_added_text)
      {
        word_list_added_text += ","
      }

      word_list_added_text += " " + nwl.name;
    }
  }

  if(word_list_change_text)
  {
    text += " Word Lists changed:" + word_list_change_text + ".";
  }

  if(word_list_deleted_text)
  {
    text += " Word Lists deleted:" + word_list_deleted_text + ".";
  }

  if(word_list_added_text)
  {
    text += " Word Lists added:" + word_list_added_text + ".";
  }

  if(text)
  {
    var cat_id = el_by_id("c").value;
    var parent_id = el_by_id("p").value;

    text += ' <a href="/psp/category/update?c=' + cat_id + '&p=' + 
            parent_id + '">reload&nbsp;category</a>'
  }

  msg_obj.innerHTML = text;
}
