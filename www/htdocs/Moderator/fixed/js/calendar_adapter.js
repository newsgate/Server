var calendars = new Array();
var format = "%Y-%m-%j";

function dwc_on_change(field)
{
  if(field.options[field.selectedIndex].value != "c" ||
     calendars[field.id] != null)
  {
    field.current_selection = field.selectedIndex;
    return;
  }

  var calendar = new RichCalendar();
  calendar.field = field;

  calendar.start_week_day = 0;
  calendar.language = 'en';
  calendar.user_onchange_handler = calendar_on_change;
  calendar.user_onclose_handler = calendar_on_close;
  calendar.user_onautoclose_handler = calendar_on_autoclose;
  calendar.parse_date(field.value, format);
  calendar.show_at_element(field, "adj_right-bottom");

  calendars[field.id] = calendar;
}

function calendar_on_change(calendar, object_code) 
{
  if(object_code == 'day') 
  {
    with(this)
    {
      hide();

      var option = document.createElement("option");
      option.value = get_formatted_date(format);
      option.text = option.value;

      try
      {
        field.add(option, null);
      }
      catch(e)
      {
        field.add(option);
      }

      field.selectedIndex = field.length - 1;

      calendars[field.id] = null;
    }
  }
}

function calendar_on_close(calendar) 
{
  with(this)
  {
    hide();
    field.selectedIndex = field.current_selection;
    calendars[field.id] = null;
  }
}

function calendar_on_autoclose(calendar) 
{
  with(this)
  {
    field.selectedIndex = field.current_selection;
    calendars[field.id] = null;
  }
}

function dwc_on_focus(field)
{
  field.current_selection = field.selectedIndex;
}
