//
// Page Manager
//

PageManager.prototype = 
{
  query_box : null,
  output : null,
  debug : null,
  message_count : null,

  initialized : function()
  {
    return this.message_count_box !== null;
  },

  onload : function()
  {
    with(this)
    {
      message_count_box = el_enrich(document.getElementById("mc"));
      request_box = document.getElementById("rq");
      output = document.getElementById("o");
      debug = document.getElementById("d");
      
      message_count_box.focus();

      message_count_box.
        el_set_selection({ start: 0, 
                           end: message_count_box.value.length });

      create_rss_link();
    }
  },

  on_key_press : function(event)
  {
    with(this)
    {
      if(event.keyCode == 13)
      {
        output.className = "error";
        return false;
      }

      return true;
    }
  },
  
  create_rss_link : function()
  {
    with(this)
    {
      var mc_value = message_count_box.value;

      if(message_count != mc_value)
      { 
        var text;
        var error_pos = el_int_error(mc_value, 1, 100);

        if(error_pos < 0)
        {
          output.className = "";
          var url = request_box.value + "&r=" + parseInt(mc_value);
          text = "<? loc COPY_LINK ?><br><a href='" + url + "'>" + url + "</a>";
        }
        else
        {
          output.className = "error";
          text = "<? loc MESSAGE_COUNT_EXPECTED ?>";
          message_count_box.el_set_selection({ start: error_pos, 
                                               end: error_pos});
        }

        output.innerHTML = text;
        message_count = mc_value;
      }

      setTimeout("pm.create_rss_link()", 500);
    }
  }
  
};
  
function PageManager()
{
}
  
//
// Creating singleton
//
var pm = new PageManager();
