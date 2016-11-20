var category_id = 0;
var rel_query = "";
var relevant_message_count = 0;
var not_viewed_phrases = [];
var viewed_phrases = [];

function on_load()
{
  window.setTimeout("check_viewed_phrases();", 1000);  
}

function search_rel_phrase(phrase)
{
  var query = rel_query;

  if(phrase)
  {
    query = "CORE '" + phrase + "' AND ( " + query + " )";
  }

  newsgate_query(query);
}

function search_irrel_phrase(phrase)
{
  var query = (phrase ? ("CORE '" + phrase + "'") : "EVERY") +
      " EXCEPT ( " + rel_query + " )";

  newsgate_query(query);
}

function recommend_not_viewed()
{
  var url = "/psp/category/recommend?c=" + category_id + "&n=" + 
            relevant_message_count;

  for(var i = 0; i < viewed_phrases.length; ++i)
  {
    url += "&p=" + viewed_phrases[i];
  }
  
  el_post_url(url);
}

function recommend()
{
  var url = "/psp/category/recommend?c=" + category_id + "&n=" + 
            relevant_message_count;

  window.location = url;
}

function check_viewed_phrases()
{
  if(el_dialogs.length == 0)
  {
    for(var i = 0; i < not_viewed_phrases.length; )
    {
      var id = not_viewed_phrases[i];

      if(el_visible(el_by_id("pi_" + id), 0.9))
      {
        viewed_phrases.push(id); 
        not_viewed_phrases.splice(i, 1);
      }
      else
      {
        ++i;
      }
    }
  }

  window.setTimeout("check_viewed_phrases();", 300);  
}