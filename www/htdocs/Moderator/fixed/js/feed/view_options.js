var post_refs = null;

function post(ref_index, new_window, pass_mod_params)
{
  var url = post_refs[ref_index];    
  el_post_url(url, null, new_window ? "_blank" : "");
}

