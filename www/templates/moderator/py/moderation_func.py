import re
import el
import newsgate

class SetValueCtx:

  def __init__(this):
    this.error = ""
    this.bad_values = {}

def set_ulong_value(prm, val, min_val, max_val, err_text, svc):

  lval = 0
  err = ""

  try:
    lval = long(val)
    if lval < min_val or lval > max_val: err = err_text
  except: 
    err = err_text

  if err:
    if not svc.error: svc.error = err
    svc.bad_values[prm] = val
    lval = min_val

  return lval

def ulong_value(prm, val, svc):
  if prm in svc.bad_values: return svc.bad_values[prm]
  return val

def set_float_value(prm, val, min_val, max_val, err_text, svc):

  lval = 0
  err = ""

  try:
    lval = float(val)
    if lval < min_val or lval > max_val: err = err_text
  except: 
    err = err_text

  if err:
    if not svc.error: svc.error = err
    svc.bad_values[prm] = val
    lval = min_val

  return lval

def float_value(prm, val, svc):
  if prm in svc.bad_values: return svc.bad_values[prm]
  return val

def filter_rules_version(): return "5"

def refresh_session(request, moderator):
  
  expiration = el.Moment(el.TimeValue(request.time().sec() + 365 * 86400))

  request.output.send_cookie(\
    el.net.http.CookieSetter("ms",
                             moderator.session_id,
                             expiration,
                             "/",
                             "",
                             request.secure()))

def connect_moderator(request,
                      moderator_connector,
                      login_if_required = False,
                      advance_sess_timeout = True,
                      exit_on_failure = True):

  moderator = None
  
  try:
    moderator = moderator_connector.connect(request,
                                            login_if_required,
                                            advance_sess_timeout)

  except newsgate.moderation.InvalidUsername:
    if exit_on_failure:
      el.exit(el.psp.Forward(\
        "/psp/login.psp", 
        "Non-existent user name or invalid password specified.<br>Please, try again."))

  except newsgate.moderation.InvalidSession:
    if exit_on_failure:
      el.exit(el.psp.Forward("/psp/login.psp", 
                             "Your session expired.<br>Please, relogin."))

  except newsgate.moderation.NotReady:
    if exit_on_failure:
      el.exit(el.psp.Forward(\
        "/psp/error.psp", 
        "The system is not ready to serve your request.<br>Please, try again in several minutes."))

  except newsgate.moderation.AccountDisabled:
    if exit_on_failure:
      el.exit(el.psp.Forward("/psp/error.psp", "User account is disabled."))

  if moderator and advance_sess_timeout:
    refresh_session(request, moderator)
  
  return moderator

def clear_session(request, moderator_connector):

  moderator_connector.logout(request)  
  expiration = el.Moment(el.TimeValue(request.time().sec() - 86400 * 1000))

  request.output.send_cookie(\
    el.net.http.CookieSetter("ms", "x", expiration, "/", "", request.secure()))

def create_ad_menus(moderator, current, id = None):

  ad_manager = \
    moderator.has_privilege(newsgate.moderation.moderator.PV_AD_MANAGER)

  menu = []

  if moderator.advertiser_id:
    menu.append( ("Campaigns",
                  current != "Campaigns" and "/psp/ad/campaigns" or "") )

    if current == "Campaign" or current == "New Campaign":
      menu.append((current, ""))

    if current == "Campaigns" or current == "Campaign":
      menu.append(("New Campaign", "/psp/ad/campaign"))

    if current == "Group" or current == "New Group":
      menu.append((current, ""))

    if current == "Group" and id:
      menu.append(("New Group", "/psp/ad/group?cid=" + str(id)))

    if current == "Placement" or current == "New Placement":
      menu.append((current, ""))

    if current == "Placement" and id:
      menu.append(("Clone Placement", "/psp/ad/placement?cl=" + str(id)))

    if current == "Counter Placement" or current == "New Counter Placement":
      menu.append((current, ""))

    if current == "Counter Placement" and id:
      menu.append(("Clone Counter Placement",
                   "/psp/ad/counter_placement?cl=" + str(id)))

    menu.append( ("Conditions",
                  current != "Conditions" and "/psp/ad/conditions" or "") )

    if current == "Condition" or current == "New Condition":
      menu.append((current, ""))

    if current == "Condition" and id:
      menu.append(("Clone Condition", "/psp/ad/condition?cl=" + str(id)))

    if current == "Conditions" or current == "Condition":
      menu.append(("New Condition", "/psp/ad/condition"))

    menu.append( ("Ads", current != "Ads" and "/psp/ad/ads" or "") )
    if current == "Ad" or current == "New Ad": menu.append((current, ""))
    
    if current == "Ad" and id:
      menu.append(("Clone Ad", "/psp/ad/ad?cl=" + str(id)))

    if current == "Ads" or current == "Ad":
      menu.append(("New Ad", "/psp/ad/ad"))      

    menu.append( ("Counters", current != "Counters" and \
                  "/psp/ad/counters" or "") )
    
    if current == "Counter" or current == "New Counter":
      menu.append((current, ""))
    
    if current == "Counter" and id:
      menu.append(("Clone Counter", "/psp/ad/counter?cl=" + str(id)))

    if current == "Counters" or current == "Counter":
      menu.append(("New Counter", "/psp/ad/counter"))

  if ad_manager or moderator.advertiser_id:
    menu.append(("Pages", current != "Pages" and "/psp/ad/pages" or ""))
    if current == "Page": menu.append(("Page", ""))
    menu.append(("Sizes", current != "Sizes" and "/psp/ad/sizes" or ""))
    
  if ad_manager:
    menu.append(("Global", current != "Global" and "/psp/ad/global" or ""))

    text = "Advertiser"
    if moderator.advertiser_name: text += " " + moderator.advertiser_name
    menu.append((text, current != "Advertiser" and "/psp/ad/advertiser" or ""))

  return menu

def create_topbar_main_menus(context, moderator, area, menus):

  conf = context.config.get
  
  middle_menu = []

  if moderator:
    middle_menu.append( ["Home", "/psp/home"] )
  
  if moderator and \
     (moderator.has_privilege(
        newsgate.moderation.moderator.PV_ACCOUNT_MANAGER) or \
      moderator.has_privilege(\
        newsgate.moderation.moderator.PV_CUSTOMER_MANAGER) and \
      conf("moderation.customer_moderating") == "1"):
    middle_menu.append( ["Accounts", "/psp/accounts"] )

  if moderator and \
     (moderator.has_privilege(\
        newsgate.moderation.moderator.PV_FEED_CREATOR) or \
      moderator.has_privilege(newsgate.moderation.moderator.PV_FEED_MANAGER)):
    middle_menu.append( ["Feeds", "/psp/feeds"] )

  if moderator and \
     moderator.has_privilege(newsgate.moderation.moderator.PV_FEED_MANAGER):
    middle_menu.append( ["Messages", "/psp/messages"] )

  if moderator and \
     (moderator.has_privilege(\
       newsgate.moderation.moderator.PV_CATEGORY_MANAGER) or \
       moderator.has_privilege(\
         newsgate.moderation.moderator.PV_CATEGORY_EDITOR)):
    middle_menu.append( ["Categories", "/psp/categories"] )

  if moderator and \
     (moderator.has_privilege(newsgate.moderation.moderator.PV_AD_MANAGER) or \
      moderator.has_privilege(newsgate.moderation.moderator.PV_ADVERTISER)) \
     and conf("moderation.ad_management") == "1":
    middle_menu.append( ["Ads", "/psp/ad"] )

  if moderator and \
     moderator.has_privilege(newsgate.moderation.moderator.PV_CLIENT_MANAGER) \
     and conf("moderation.client_moderating") == "1":
    middle_menu.append( ["Clients", "/psp/clients"] )

  if moderator:
    right_menu = [ ("Logout " + moderator.name, "/psp/logout"),
                   ["My Settings", "/psp/mysettings"],
                   ["Help", "/psp/help"]
                 ]
  else:
    right_menu = []

  menus.append(middle_menu)
  menus.append(right_menu)

  for m in menus:
    for s in m:
      if s[0] == area: s[1] = ""

def create_topbar(left_menu, middle_menu, right_menu, page_panel = None):

  xml_encode_flags = el.string.manip.XE_TEXT_ENCODING | \
                     el.string.manip.XE_ATTRIBUTE_ENCODING | \
                     el.string.manip.XE_PRESERVE_UTF8
  
  topbar = R'''  <tr><td id="topbar">
    <table cellspacing="0">
      <tr><td id="topbar_left_menu">'''
  
  add_separator = False
  current_left_menu = ""
  
  for mi in left_menu:

    name = \
      el.string.manip.xml_encode(mi[0], xml_encode_flags).replace(' ', "&nbsp;")
    
    url = el.string.manip.xml_encode(mi[1], xml_encode_flags)
    
    if add_separator: topbar += " | "
    else: add_separator = True
    
    if url == "":
      topbar += '<span class="topbar_current_page_name">' + name + '</span>'
      current_left_menu = name
    else:
      topbar += '<a href="' + url + '">' + name + '</a>'

  topbar += R'''</td>
      <td id="topbar_middle_menu">'''

  add_separator = False
  current_mid_menu = ""
  
  for mi in middle_menu:

    name = \
      el.string.manip.xml_encode(mi[0], xml_encode_flags).\
        replace(' ', "&nbsp;")
    
    url = el.string.manip.xml_encode(mi[1], xml_encode_flags)
    
    if add_separator: topbar += " | "
    else: add_separator = True
    
    if len(url) == 0:
      topbar += '<span class="topbar_current_page_name">' + name + '</span>'
      current_mid_menu = name
    else:
      topbar += '<a href="' + url + '">' + name + '</a>'

  topbar += R'''</td>
      <td id="topbar_right_menu">'''
    
  add_separator = False
  for mi in right_menu:

    menu_name = mi[0]
    
    name = \
      el.string.manip.xml_encode(menu_name,
                                 xml_encode_flags).replace(' ', "&nbsp;")

    menu_url = mi[1]

    if menu_url == "/psp/help":
      anchor = ""
      if current_mid_menu: anchor += current_mid_menu
      
      if current_left_menu:
        if anchor: anchor += "_"
        anchor += current_left_menu

      if anchor:
        menu_url = menu_url + "#" + anchor
    
    url = el.string.manip.xml_encode(menu_url, xml_encode_flags)
    
    if add_separator: topbar += " | "
    else: add_separator = True
    
    if len(url) == 0:
      topbar += '<span class="topbar_current_page_name">' + name + '</span>'
    else:
      topbar += '<a href="' + url + '">' + name + '</a>'
    
  topbar += "</td></tr>"

  if page_panel != None:
    topbar += '<tr><td colspan="3" class="page_panel">' + page_panel + \
              '</td></tr>'
    
  topbar += R'''
    </table>
  </td></tr>'''

  return topbar

def create_copyright(context=None):
  copyright = context and context.config.get("copyright_note") or ""
  
  return '  <tr><td id="copyright" valign="bottom">' + copyright +\
         ' All rights reserved.</td></tr>'

def get_privileges(context, account):

  conf = context.config.get
  customer_moderating = conf("moderation.customer_moderating") == "1"
  client_moderating = conf("moderation.client_moderating") == "1"
  ad_management = conf("moderation.ad_management") == "1"
  
  parameters = context.request.input.parameters()
  privileges = el.Sequence()
  
  for param in parameters:

    privilege = newsgate.moderation.moderator.Privilege()
    
    if param.name == "pv_FeedCreator":
      privilege.id = newsgate.moderation.moderator.PV_FEED_CREATOR
    elif param.name == "pv_FeedManager":
      privilege.id = newsgate.moderation.moderator.PV_FEED_MANAGER
    elif param.name == "pv_AccountManager":
      privilege.id = newsgate.moderation.moderator.PV_ACCOUNT_MANAGER
    elif param.name == "pv_CategoryManager":
      privilege.id = newsgate.moderation.moderator.PV_CATEGORY_MANAGER
    elif param.name == "pv_ClientManager":
      if client_moderating:
        privilege.id = newsgate.moderation.moderator.PV_CLIENT_MANAGER
    elif param.name == "pv_CustomerManager":
      if customer_moderating:
        privilege.id = newsgate.moderation.moderator.PV_CUSTOMER_MANAGER
    elif param.name == "pv_Customer":
      if customer_moderating:
        privilege.id = newsgate.moderation.moderator.PV_CUSTOMER
    elif param.name == "pv_AdManager":
      if ad_management:
        privilege.id = newsgate.moderation.moderator.PV_AD_MANAGER
    elif param.name == "pv_Advertiser":
      if ad_management and customer_moderating:
        privilege.id = newsgate.moderation.moderator.PV_ADVERTISER
    elif param.name == "pv_CategoryEditor":
      privilege.id = newsgate.moderation.moderator.PV_CATEGORY_EDITOR

    if privilege.id: privileges.append(privilege)
  
  for param in parameters:
    if param.name == "pva_CategoryEditor":
      for p in privileges:
        if p.id == newsgate.moderation.moderator.PV_CATEGORY_EDITOR:
          p.args = param.value

  if customer_moderating == False and account != None:
    if account.has_privilege(\
       newsgate.moderation.moderator.PV_CUSTOMER_MANAGER):

      privilege = newsgate.moderation.moderator.Privilege()    
      privilege.id = newsgate.moderation.moderator.PV_CUSTOMER_MANAGER
      privileges.append(privilege)

    if account.has_privilege(newsgate.moderation.moderator.PV_CUSTOMER):

      privilege = newsgate.moderation.moderator.Privilege()    
      privilege.id = newsgate.moderation.moderator.PV_CUSTOMER
      privileges.append(privilege)
    
  if client_moderating == False and account != None and \
     account.has_privilege(newsgate.moderation.moderator.PV_CLIENT_MANAGER):

    privilege = newsgate.moderation.moderator.Privilege()    
    privilege.id = newsgate.moderation.moderator.PV_CLIENT_MANAGER
    privileges.append(privilege)
    
  if ad_management == False and account != None and \
     account.has_privilege(newsgate.moderation.moderator.PV_AD_MANAGER):

    privilege = newsgate.moderation.moderator.Privilege()    
    privilege.id = newsgate.moderation.moderator.PV_AD_MANAGER
    privileges.append(privilege)
    
  if (ad_management == False or customer_moderating == False) and \
     account != None and \
     account.has_privilege(newsgate.moderation.moderator.PV_ADVERTISER):

    privilege = newsgate.moderation.moderator.Privilege()    
    privilege.id = newsgate.moderation.moderator.PV_ADVERTISER
    privileges.append(privilege)

  customer_privilege = False
  advertiser_privilege = False
  
  for p in privileges:
    if p.id == newsgate.moderation.moderator.PV_CUSTOMER:
      customer_privilege = True
    elif p.id == newsgate.moderation.moderator.PV_ADVERTISER:
      advertiser_privilege = True

  if advertiser_privilege and not customer_privilege:
      privilege = newsgate.moderation.moderator.Privilege()    
      privilege.id = newsgate.moderation.moderator.PV_CUSTOMER
      privileges.append(privilege)
    
  return privileges

def priv_cmp(p1, p2):
  return p1.name < p2.name and -1 or  p1.name > p2.name and 1 or 0

def create_account_form(context,
                        moderator,
                        action_url,
                        name,
                        password,
                        password2,
                        email,
                        privileges,
                        status,
                        submit_input,
                        hidden_inputs,
                        message,
                        error_occured):

  conf = context.config.get

  customer_moderating = conf("moderation.customer_moderating") == "1"
  client_moderating = conf("moderation.client_moderating") == "1"
  ad_management = conf("moderation.ad_management") == "1"
  
  xml_encode_flags = el.string.manip.XE_TEXT_ENCODING | \
                     el.string.manip.XE_ATTRIBUTE_ENCODING | \
                     el.string.manip.XE_PRESERVE_UTF8
  
  form = \
R'''
  <tr><td id="main_area" align="center" valign="top">

  <form action="''' + action_url +  R'''">
    <table id="edit_form" cellspacing="0">
    <tr><td>Name:</td><td><input class="edit" type="text" name="n" id="n" value="''' + \
    el.string.manip.xml_encode(name, xml_encode_flags) + R'''"/></td></tr>
    <tr><td>Password:</td><td><input class="edit" type="password" name="p" id="p"  value="''' + \
    el.string.manip.xml_encode(password, xml_encode_flags) + R'''"/></td></tr>
    <tr><td>Reenter Password:</td><td><input class="edit" type="password" name="p2" id="p2" value="''' + \
    el.string.manip.xml_encode(password2, xml_encode_flags) + R'''"/></td></tr>
    <tr><td>Email:</td><td><input class="edit" type="text" name="e" id="e" value="''' + \
    el.string.manip.xml_encode(email, xml_encode_flags) + R'''"/></td></tr>
    <tr><td valign="top">Privileges:</td><td>
'''

  moderator_privileges = []
  
  for priv in moderator.privileges:
    moderator_privileges.append(priv)

  moderator_privileges.sort(priv_cmp)

  for priv in moderator_privileges:

    if customer_moderating == False and ( \
       priv.priv.id == newsgate.moderation.moderator.PV_CUSTOMER_MANAGER or \
       priv.priv.id == newsgate.moderation.moderator.PV_CUSTOMER ) or \
       client_moderating == False and \
       priv.priv.id == newsgate.moderation.moderator.PV_CLIENT_MANAGER or \
       ad_management == False and \
       priv.priv.id == newsgate.moderation.moderator.PV_AD_MANAGER or \
       (ad_management == False or customer_moderating == False) and \
       priv.priv.id == newsgate.moderation.moderator.PV_ADVERTISER:      
      continue

    priv_name = el.string.manip.xml_encode(priv.name, xml_encode_flags)
    form += '      <div><input type="checkbox" name="pv_' + priv_name + '"'

    checked = False
    args = ""
    
    for p in privileges:
      if p.id == priv.priv.id:      
        form += ' checked="checked"'
        checked = True
        args = p.args
        break
    
    form += '/>' + priv_name + '</div>\n'

    if priv.name == "CategoryEditor":
      form += '      <div><textarea class="edit" name="pva_' + priv_name + \
              '" rows="3">' + \
              el.string.manip.xml_encode(args, xml_encode_flags) + \
              '</textarea></div>\n'

  form += R'''    </td></tr>
    <tr><td valign="top">Status:</td><td><select name="s">
      <option''' + \
str(status == newsgate.moderation.moderator.MS_ENABLED and ' selected="selected"' or ' ') + \
R'''>Enabled</option>
      <option''' + \
str(status == newsgate.moderation.moderator.MS_DISABLED and ' selected="selected"' or ' ') + \
R'''>Disabled</option></select>'''

  form += R'''
    </td></tr>''' + \
R'''
    <tr><td></td><td><input type="submit" value="''' + \
    el.string.manip.xml_encode(submit_input[1], xml_encode_flags) + \
    '" name="' + el.string.manip.xml_encode(submit_input[0], xml_encode_flags) + R'''"/></td></tr>
    <tr><td colspan="2" class="''' + str(error_occured and "error" or "success") + R'''">'''

  if message != None:
    form += el.string.manip.xml_encode(message, xml_encode_flags)

  form += \
R'''</td></tr>
    </table>'''

  for input in hidden_inputs:
    form += R'''
    <input type="hidden" value="''' + \
    el.string.manip.xml_encode(input[1], xml_encode_flags) + '" name="' + \
    el.string.manip.xml_encode(input[0], xml_encode_flags) + '"/>'

  form += R'''
  </form>

  </td></tr>
'''

  return form

def result_description(result):
  validation_result = result.result
  res_type = result.type

  if validation_result == newsgate.moderation.feed.VR_VALID:
    
    if res_type == newsgate.moderation.feed.RT_RSS: res_desc = "RSS feed"
    elif res_type == newsgate.moderation.feed.RT_ATOM: res_desc = "ATOM feed"
    elif res_type == newsgate.moderation.feed.RT_RDF: res_desc = "RDF feed"
    elif res_type == newsgate.moderation.feed.RT_HTML_FEED:
      res_desc = "HTML feed"
    elif res_type == newsgate.moderation.feed.RT_HTML: 
      res_desc = "HTML page"

      if result.feed_reference_count > 0: 
        res_desc += " (" + str(result.feed_reference_count) + \
                     " feeds referenced)"

    elif res_type == newsgate.moderation.feed.RT_META_URL: res_desc = "Meta-url"
    else: res_desc = "resource"
 
  elif validation_result == newsgate.moderation.feed.VR_ALREADY_IN_THE_SYSTEM:
    if res_type == newsgate.moderation.feed.RT_RSS: res_desc = "RSS"
    elif res_type == newsgate.moderation.feed.RT_ATOM: res_desc = "ATOM"
    elif res_type == newsgate.moderation.feed.RT_RDF: res_desc = "RDF"
    elif res_type == newsgate.moderation.feed.RT_HTML_FEED: res_desc = "HTML"
    else: res_desc = "unknown"

    res_desc += " feed already registered"

  elif validation_result == newsgate.moderation.feed.VR_UNRECOGNIZED_RESOURCE_TYPE:
    res_desc = "unrecognized"
  elif validation_result == newsgate.moderation.feed.VR_NO_VALID_RESPONSE:
    res_desc = "no valid response"
  elif validation_result == newsgate.moderation.feed.VR_TOO_LONG_URL:
    res_desc = "too long url"
  else:
    res_desc = "unexpected result"

  return res_desc

def truncate_phrase(text, size):

  encoded_text = text.decode("utf8", 'replace')
  words = encoded_text.split()
  
  count = len(words)
  new_size = size + 3

  new_text = ""
  new_len = 0
  i = 0
  
  for word in words:
    new_text += word.encode("utf8") + " "
    new_len += len(word)
    i += 1

    if new_len >= new_size: break

  if i == count: return text

  return new_text + "..."

def truncate_string(text, size):
  encoded_text = text.decode("utf8", 'replace')
  
  if len(encoded_text) <= size + 2: return text
  return encoded_text[0:size].encode("utf8") + "..."

def get_pref(name, prefs, def_val, param = None):
  try: val = prefs[name]
  except: val = def_val

  if param != None: val = param(name, val)
  
  prefs[name] = val
  return val

def get_date(name, prefs):

  val = get_pref(name, prefs, "t")
  today = el.gettimeofday().sec() / 86400 * 86400

  if val == "t" or val == "":
    return el.Moment(el.TimeValue(today))
  elif val == "y":
    return el.Moment(el.TimeValue(today - 86400))
  elif val == "w":
    return el.Moment(el.TimeValue(today - 86400 * 7))
  elif val == "m":
    return el.Moment(el.TimeValue(today - 86400 * 30))
  elif val == "r":
    return el.Moment(el.TimeValue(today - 86400 * 365))
  
  return el.Moment(val, "ISO_8601")

def get_pref_add_header(name,
                        prefs,
                        def_val,
                        header,
                        header_info,
                        is_stat,
                        curr_sort_id = None,
                        curr_sort_desc = None,
                        sort_id = None,
                        url = None,
                        add_vf_param = None,
                        post = None,
                        anchor_refs = None):
  val = get_pref(name, prefs, def_val)

  if val != "h":
    if curr_sort_id == None:
      header_info[0] += '<td>' + header + '</td>'
    else:
      
      if sort_id == curr_sort_id and curr_sort_desc == 0: sort_desc = "1"
      else: sort_desc = "0"

      ref = make_reference(url + "sort=" + str(sort_id) + '&sdesc=' +sort_desc,
                           False,
                           True,
                           add_vf_param,
                           post,
                           anchor_refs)
      
      header_info[0] += '<td><a href="' + ref + '">' + header + '</a></td>'

    if is_stat: header_info[1] = True
    
  return val
  
def get_option_select(label,
                      name,
                      value,
                      options = None,
                      select_extra_text = None):

  if options == None:
    options = [ ("Hide", "h"), ("Show", "s") ]

  text = '\n      <tr class="option_row"><td valign="top" class="option_label">' + label + \
         ':</td><td class="option_control"><select name="' + name + '" ' + \
         str(select_extra_text != None and select_extra_text or "") + '>'

  for opt in options:
    text += '\n        <option' + \
            str(value == opt[1] and '  selected="selected"' or ' ') + \
            ' value="' + opt[1] + '">' + opt[0] + '</option>'

  text += '\n      </select></td></tr>'

  return text

def get_date_with_calendar(label, name, value):

  options = [ ("Today", "t"), 
              ("Yesterday", "y"), 
              ("Week Ago", "w"), 
              ("Month Ago", "m"), 
              ("Year Ago", "r"),
              ("Calendar", "c")
            ]

  try:
    if value[0].isdigit():
      options.append( (str(value), str(value)) )
  except: pass
  
  text = \
    get_option_select(\
      label,
      name,
      value,
      options,
      "id=\"dwc_" + name + "\" onchange=\"dwc_on_change(this)\" onfocus=\"dwc_on_focus(this)\"")
  
  return text

def get_cell_value(value, cell_variants, extra = None):

  for cell in cell_variants:
    if cell[0] == value:

      try: right_aligned = cell[2]
      except: right_aligned = False
      
      val = cell[1]
      if type(val) is float: val = "%.2f" % val
      else: val = str(val)
      
      return "<td " + str(extra != None and extra or "") + \
             str(right_aligned and ' align="right"' or "") + ">" + \
             val + "</td>"

  return ""

def make_reference(url, new_window, pass_mod_params, add_vf_param, post, anchor_refs):
  if post:
    anchor_refs.append(url)
    return "javascript:post(" + str(len(anchor_refs) - 1) + ", " + \
           (new_window and "true" or "false") + ", " + \
           (pass_mod_params and "true" or "false") + ", " + \
           (add_vf_param and "true" or "false") + ");"
  else:
    return url

def create_page_panel(start_from,
                      items_count, 
                      total_items_count, 
                      results_per_page, 
                      page_links,
                      url,
                      post,
                      anchor_refs):

  if total_items_count == 0: return ""
  
  total_pages = total_items_count / results_per_page
  if total_items_count % results_per_page: total_pages += 1

  if items_count: current_page = start_from / results_per_page
  else: current_page = total_pages

  shift = page_links / 2

  if current_page < shift: start_page_number = 0
  else: start_page_number = current_page - shift

  last_page_number = start_page_number + page_links

  if last_page_number > total_pages: last_page_number = total_pages

  if last_page_number - start_page_number < page_links:
    if last_page_number > page_links:
      start_page_number = last_page_number - page_links
    else: start_page_number = 0

  text = 'Result Page [' + str(start_from) + '-' + \
         str(start_from + items_count - 1) + ' of ' + \
         str(total_items_count) + '] : '

  if current_page:
    text += '<a href="' + make_reference(url + "s=" + \
            str((current_page - 1) * results_per_page + 1) + \
            '&r=' + str(results_per_page), False, True, True, post, anchor_refs) + \
            '">Previous</a>  '

  for page_num in range(start_page_number, last_page_number):

    if page_num > start_page_number: text += " "

    if page_num == current_page:
      text += '<span class="page_current_number">' + str(page_num + 1) + \
              '</span>'
    else:
      text += '<a href="' + make_reference(url + "s=" + \
              str(page_num * results_per_page + 1) + \
              '&r=' + str(results_per_page), False, True, True, post, anchor_refs) + \
              '">' + str(page_num + 1) + '</a>'

  if current_page < last_page_number - 1:
    text += '  <a name="next_link" href="' + make_reference(url + "s=" + \
            str((current_page + 1) * results_per_page + 1) + \
            '&r=' + str(results_per_page), False, True, True, post, anchor_refs) + \
            '">Next</a>'
    
  return text

def traffic(value):
  if value < 1024: return str(value)
  elif value < 1024 * 1024: return str(value / 1024) + " K"
  elif value < 1024 * 1024 * 1024:
    return str(value / (1024 * 1024)) + " M"

  return str(value / (1024 * 1024 * 1024)) + " G"


usec_in_minute = 1000000 * 60
usec_in_hour = usec_in_minute * 60
usec_in_day = usec_in_hour * 24

def time(value):

  if value < 1000000: return str(value) + " U"
  elif value < usec_in_minute: return "%.3f" % (float(value) / 1000000) + " S"
  elif value < usec_in_hour:
    return "%.1f" % (float(value) / usec_in_minute) + " M"
  elif value < usec_in_day:
    return "%.1f" % (float(value) / usec_in_hour) + " H"
  
  return "%.1f" % (float(value) / usec_in_day) + " D"

def filter_rules_from_prefs(prefs):

  rules = {}
  version_checked = False
  
  for k in prefs.keys():
    value = el.string.manip.mime_url_decode(prefs[k])

    if k == "ver":
      if value != filter_rules_version(): return {}
      else:
        version_checked = True
        continue
    
    rule_info = k.split("/")

    try:
      rule_id = int(rule_info[0])

      rule = newsgate.moderation.feed.FilterRule()
      rule.id = rule_id
      rule.field = int(rule_info[1])
      rule.operation = int(rule_info[2])

      if rule.operation in [newsgate.moderation.feed.FO_ANY_OF, 
                            newsgate.moderation.feed.FO_NONE_OF]:
        for val in value.split(" "):
          if val != "":

            values = val.split("-")
          
            try:
              v0 = long(values[0])
              v1 = long(values[1])
            except:
              rule.args.append(val)
              continue

            for v in range(min(v0, v1), max(v0, v1) + 1):
              rule.args.append(str(v))
      elif rule.operation in [newsgate.moderation.feed.FO_REGEXP,
                              newsgate.moderation.feed.FO_NOT_REGEXP,
                              newsgate.moderation.feed.FO_LIKE,
                              newsgate.moderation.feed.FO_NOT_LIKE]:
        for val in value.split("\n"):
          if val != "": rule.args.append(val)        
      else:
        rule.args.append(value)

      rules[rule_id] = rule
      
    except:
      continue

  return version_checked and rules or {}

def prefs_from_filter_rules(rules):
  prefs = el.NameValueMap("", ':', '~')

  for id in rules.keys():
    
    r = rules[id]
    key = str(r.id) + "/" + str(r.field) + "/" + str(r.operation) + "/";

    value = ""    
    first = True

    if r.operation in [newsgate.moderation.feed.FO_REGEXP,
                       newsgate.moderation.feed.FO_NOT_REGEXP,
                       newsgate.moderation.feed.FO_LIKE,
                       newsgate.moderation.feed.FO_NOT_LIKE]:
      sep = '\n'
    else:
      sep = ' '
    
    for v in r.args:
      if first: first = False  
      else: value += sep
      value += v

    prefs[key] = el.string.manip.mime_url_encode(value)

  prefs["ver"] = filter_rules_version()

  return prefs

def xml_smart_encode(text):

  if text.find("]]>") < 0:
    return "<![CDATA[" + text + "]]>"
  else:
    return el.string.manip.xml_encode(text, 
                                      el.string.manip.XE_TEXT_ENCODING | \
                                      el.string.manip.XE_PRESERVE_UTF8)

def prn_message_items(message, prn):
  
  xml_attr_encode_flags = el.string.manip.XE_TEXT_ENCODING | \
                          el.string.manip.XE_PRESERVE_UTF8 | \
                          el.string.manip.XE_ATTRIBUTE_ENCODING
  
  source = message.source

  prn(R'''
    <message>
      <source>
        <url>''', xml_smart_encode(source.url), R'''</url>
        <title>''', xml_smart_encode(source.title), R'''</title>
        <html_link>''', xml_smart_encode(source.html_link), R'''</html_link>
      </source>
      <url>''', xml_smart_encode(message.url), R'''</url>
      <title>''', xml_smart_encode(message.title), R'''</title>
      <description>''', xml_smart_encode(message.description), R'''</description>
      <valid>''', message.valid, R'''</valid>
      <log>''', xml_smart_encode(message.log), R'''</log>
      <space>''', message.space, R'''</space>
      <lang>''', message.lang.l3_code(), R'''</lang>
      <country>''', message.country.l3_code(), '</country>')

  if len(message.keywords):
    prn(R'''
      <keywords>''')

    for kw in message.keywords:
      prn(R'''
        <keyword>''', xml_smart_encode(kw), '</keyword>')

    prn(R'''
      </keywords>''')

  if len(message.images):
    prn(R'''
      <images>''')

    for im in message.images:
      prn(R'''
        <image src="''', 
            el.string.manip.xml_encode(im.src, xml_attr_encode_flags),
          '" origin="', im.origin, '" status="', im.status, '"')

      if im.alt:
        prn(' alt="''', 
              el.string.manip.xml_encode(im.alt, xml_attr_encode_flags), '"')

      if im.width != newsgate.message.Image.DIM_UNDEFINED:
        prn(' width="', im.width, '"')

      if im.height != newsgate.message.Image.DIM_UNDEFINED:
        prn(' height="', im.height, '"')

      prn('/>')
      
    prn(R'''
      </images>''')

  prn(R'''
    </message>''')

def image_origins():
  origins = [ ( newsgate.message.Image.IO_UNKNOWN, "UNKNOWN" ),
              ( newsgate.message.Image.IO_DESC_IMG, "DESC_IMG" ),
              ( newsgate.message.Image.IO_DESC_LINK, "DESC_LINK" ),
              ( newsgate.message.Image.IO_ENCLOSURE, "ENCLOSURE" )
            ]
  
  return origins

def image_origin_id(name):
  for o in image_origins():
    if name == o[1]: return o[0]
  return newsgate.message.Image.IO_UNKNOWN

def image_origin_name(id):
  for o in image_origins():
    if id == o[0]: return o[1]
  return "UNKNOWN"

def image_statuses():
  origins = [ ( newsgate.message.Image.IS_VALID, "VALID" ),
              ( newsgate.message.Image.IS_BAD_SRC, "BAD_SRC" ),
              ( newsgate.message.Image.IS_BLACKLISTED, "BLACKLISTED" ),
              ( newsgate.message.Image.IS_BAD_EXTENSION, "BAD_EXTENSION" ),
              ( newsgate.message.Image.IS_TOO_SMALL, "TOO_SMALL" ),
              ( newsgate.message.Image.IS_SKIP, "SKIP" ),
              ( newsgate.message.Image.IS_DUPLICATE, "DUPLICATE" ),
              ( newsgate.message.Image.IS_TOO_MANY, "TOO_MANY" ),
              ( newsgate.message.Image.IS_TOO_BIG, "TOO_BIG" ),
              ( newsgate.message.Image.IS_UNKNOWN_DIM, "UNKNOWN_DIM" )
            ]
  
  return origins

def image_status_id(name):
  for o in image_statuses():
    if name == o[1]: return o[0]
  return newsgate.message.Image.IO_SKIP

def image_status_name(id):
  for o in image_statuses():
    if id == o[0]: return o[1]
  return "SKIP"

def get_spaces():
  return [ ( newsgate.moderation.feed.SP_UNDEFINED, "undefined" ),
           ( newsgate.moderation.feed.SP_NEWS, "news" ),
           ( newsgate.moderation.feed.SP_TALK, "talk" ),
           ( newsgate.moderation.feed.SP_AD, "ad" ),
           ( newsgate.moderation.feed.SP_BLOG, "blog" ),
           ( newsgate.moderation.feed.SP_ARTICLE, "article" ),
           ( newsgate.moderation.feed.SP_PHOTO, "photo" ),
           ( newsgate.moderation.feed.SP_VIDEO, "video" ),
           ( newsgate.moderation.feed.SP_AUDIO, "audio" ),
           ( newsgate.moderation.feed.SP_PRINTED, "printed" )
         ]

def space_name(space):

  if space != newsgate.moderation.feed.SP_UNDEFINED:
    for sp in get_spaces():
      if sp[0] == space: return sp[1]

  return ""

def space_id(space):

  for sp in get_spaces():
    if sp[1] == space: return sp[0]

  return newsgate.moderation.feed.SP_UNDEFINED

def prn_message_option(prn,
                       adjustment_result,
                       browser,
                       label,
                       colspan,
                       opt,
                       name,
                       onchange):

  xml_encode_flags = el.string.manip.XE_TEXT_ENCODING | \
                     el.string.manip.XE_PRESERVE_UTF8

  xml_attr_encode_flags = xml_encode_flags | \
                          el.string.manip.XE_ATTRIBUTE_ENCODING

  is_html = type(adjustment_result) is newsgate.GetHTMLItemsResult

  if label != None:
    label = el.string.manip.xml_encode(label, xml_encode_flags)
    if label: label = label + ":"

    prn('<tr valign="top"><td class="options_lable">', label, '</td>')

  if opt != None:

    prn('<td', colspan and ' colspan="2"' or '', ' class="option_cell">')
  
    if type(opt) is el.Lang: opt = opt != el.Lang.null and opt.l3_code() or ""

    if type(opt) is el.Country:
      opt = opt != el.Country.null and opt.l3_code() or ""

    if type(opt) is str:
      if opt == "<EMPTY>": pass
      elif opt == "<FEED_LIST>":
        prn('<select id="feeds" onfocus="save_focus(this)"></select>')
      elif opt == "<ITEM_LIST>":
        prn(R'''<select id="items" onfocus="save_focus(this)">
</select>''');
#<option>Loading...</option></select>''');
#      elif opt == "<ITEMS_LOAD_STATUS>":
#        prn(R'''<div id="items_load_error"></div>
#                <div id="items_load_log"></div>''')
      elif opt == "<EDITOR_BUTTONS>":
        prn(R'''<input type="button" onclick="adjust_message()" 
                       value="Run script" id="run"/>
                <input type="checkbox" id="autorun" onclick="autorun_clicked()" />
                <label for="autorun" id="autorun_label">Autorun</label>&nbsp;&nbsp;
                <input type="button" onclick="reload_feed_items()"
                       value="Reload items" id="reload_items"/>''')
      elif opt == "<VIEWER_BUTTONS>":
        prn(R'''<input type="button" onclick="reload_feed_items()" 
                       value="Run script" id="run"/>
                <input type="checkbox" id="usecache"/>
                <label for="usecache" id="usecache_label">Use&nbsp;cache</label>
                <input type="checkbox" id="show_invalid" onclick="fill_channel_items()"/>
                <label for="show_invalid">Show&nbsp;invalid</label>''')
        
      elif opt == "<MSG_ERRORS>":
        
        error = adjustment_result != None and not is_html and \
                adjustment_result.error or ""
        
        prn('<div id="msg_error"', error == "" and ' class="hidden"' or '', '>')

        if error:
          prn(el.string.manip.xml_encode(error, xml_encode_flags).\
              replace("\n", "<br>"))

        prn('</div>')
        
      elif opt == "<MSG_LOG>":
        
        log = adjustment_result != None and not is_html and \
              adjustment_result.log or ""
        
        prn('<div id="msg_log"', log == "" and ' class="hidden"' or '',
            browser != "msie" and ' style="max-height:7em;height:auto;"' or '',
            '>')

        if adjustment_result != None:
          prn(el.string.manip.xml_encode(log, xml_encode_flags).\
              replace("\n", "<br>"))

        prn('</div>')

      elif opt == "<CHN_ERRORS>":
        
        error = adjustment_result != None and is_html and \
                adjustment_result.error or ""
        
        prn('<div id="chn_error"', error == "" and ' class="hidden"' or '', '>')

        if error:
          prn(el.string.manip.xml_encode(error, xml_encode_flags).\
              replace("\n", "<br>"))

        prn('</div>')        

      elif opt == "<CHN_LOG>":
        
        log = adjustment_result != None and is_html and \
              adjustment_result.log or ""
        
        prn('<div id="chn_log"', log == "" and ' class="hidden"' or '',
            browser != "msie" and ' style="max-height:7em;height:auto;"' or '',
            '>')

        if adjustment_result != None:
          prn(el.string.manip.xml_encode(log, xml_encode_flags).\
              replace("\n", "<br>"))

        prn('</div>')

      else:
        prn('<input type="text" class="option_text"')

        if onchange != None:
          prn(" onchange='", onchange, "'")

        prn(' value="', 
            el.string.manip.xml_encode(opt.replace("\n", " "), 
                                       xml_attr_encode_flags), 
            '"')

        if name != "": prn(' name="', name, '" id="', name, '"')
        prn(' onfocus="save_focus(this)"/>')

    elif type(opt) is el.Sequence:
      prn('<textarea rows="5" class="option_textarea" wrap="off"')
      if name != "": prn(' name="', name, '" id="', name, '"')

      if onchange != None:
        prn(" onchange='", onchange, "'")

      prn(' onfocus="save_focus(this)"/>')

      if len(opt) == 0 or type(opt[0]) is str:
        prn(el.string.manip.xml_encode("\n".join(opt), xml_encode_flags))

      elif type(opt[0]) is newsgate.message.Image:
        first = True
        for img in opt:
          if first: first = False
          else: prn('\n\n')

          prn('src:', img.src.replace("\n", " "), '\norigin:', 
              image_origin_name(img.origin), '\nstatus:', 
              image_status_name(img.status))

          if img.alt: prn('\nalt:', img.alt.replace("\n", " "))

          if img.width != newsgate.message.Image.DIM_UNDEFINED:
            prn('\nwidth:', img.width)
        
          if img.height != newsgate.message.Image.DIM_UNDEFINED:
            prn('\nheight:', img.height)
        
      prn('</textarea>')
    elif type(opt) is newsgate.message.Message:
      prn_message_preview(prn, name)

    prn('</td>')

  if label == None:
    prn('</tr>')

def prn_message_preview(prn, name):
  prn('<div id="', name, R'''" class="msg"><div class="msg_in">
<table class="msg_cont" cellspacing="0"><tr><td class="msg_title">
<a target="_blank" id="''', name, R'''_tit"></a>
</td></tr><tr><td class="msg_desc" id="''', name, R'''_dsc">
</td></tr>
<tr><td class="msg_media" id="''', name, R'''_med"></td></tr>
</table>
<table class="msg_metainfo" cellspacing="0"><tr><td class="msg_more"><a target="_blank" id="''', 
      name, R'''_rmr">Read more ...</a></td>
<td class="msg_pub">Fetched 1 minute ago from <a target="_blank" id="''', 
      name, '_s_tit"></a>&nbsp;<span id="', name,
      R'''_s_ctr"><span></td></tr></table>
</div></div>''')

def prn_init_script(context, feed_infos):

  prn = context.request.output.stream.prn
  header = context.request.input.headers().find
  
  prn('\nbrowser = "', el.psp.browser(header("user-agent")),
      '";\nvar feeds =\n[')

  first = True
  for f in feed_infos:

    if first: first = False
    else: prn(',')

    prn('\n  { url:"', el.string.manip.js_escape(f.url), '", reqnum: 0, type:',
        f.type, ', id:', f.id, ', space:', f.space, ', lang:',
        f.lang.el_code(), ', country:', f.country.el_code(),
        ', keywords:"',
        el.string.manip.js_escape(el.string.manip.suppress(f.keywords, '\r')),
        '", cache:"", items:null, log:"", error:"", timeout:0, current_item:0 }')

  prn(R'''
];

var image_origins = [''')

  first = True
  for o in image_origins():
    if first: first = False
    else: prn(',')
    prn(' { id:', o[0], ', name:"', o[1], '" }')

  prn(R''' ];

var image_statuses = [''')

  first = True
  for o in image_statuses():
    if first: first = False
    else: prn(',')
    prn(' { id:', o[0], ', name:"', o[1], '" }')

  prn(' ];\n')

def prn_message_editor(context,
                       feeds,
                       adjustment_result,
                       src_item,
                       src_msg):

  user_agent = context.request.input.headers().find("user-agent")
  browser = el.psp.browser(user_agent)

  prn = context.request.output.stream.prn

  if adjustment_result != None:
    dst_msg = adjustment_result.message
  else:
    dst_msg = newsgate.message.Message()

  editor_row_def = \
  [ 
#    ("item", "<ITEM_LIST>", "<ITEMS_LOAD_STATUS>", None, None, None, None),
    ("item", "<ITEM_LIST>", None, None, None, None, None),
    ("", "<EDITOR_BUTTONS>", None, None, None, None, None),
    ("", "<CHN_ERRORS>", None, None, None, None, None),
    ("", "<CHN_LOG>", None, None, None, None, None),
    ("", "<MSG_ERRORS>", None, None, None, None, None),
    ("", "<MSG_LOG>", None, None, None, None, None),
    ("context.message", "context.message (adjusted)"),
    ("preview", src_msg, dst_msg, "msg", "a_msg", None, None),
    ("url", 
     src_msg.url,
     dst_msg.url,
     "url", 
     "a_url", 
     'create_preview("")',
     'create_preview("a_")'
    ),
    ("title",
     src_msg.title, 
     dst_msg.title, 
     "tit", 
     "a_tit", 
     'create_preview("")',
     'create_preview("a_")'
     ), 
    ("description", 
     src_msg.description, 
     dst_msg.description, 
     "dsc", 
     "a_dsc",
     'create_preview("")',
     'create_preview("a_")'
    ),
    ("space", 
     space_name(src_msg.space), 
     space_name(dst_msg.space), 
     "spc", 
     "a_spc",
     None, 
     None),
    ("language", src_msg.lang, dst_msg.lang, "lng", "a_lng", None, None),
    ("country", 
     src_msg.country,
     dst_msg.country, 
     "ctr", 
     "a_ctr", 
     'create_preview("")',
     'create_preview("a_")'
    ),
    ("images", 
     src_msg.images, 
     dst_msg.images, 
     "img",
     "a_img",
     'create_preview("")',
     'create_preview("a_")'
    ),
    ("keywords",
     src_msg.keywords,
     dst_msg.keywords,
     "kwd",
     "a_kwd",
     None,
     None
    ),
    ("context.message.source", "context.message.source (adjusted)"),
    ("url", 
     src_msg.source.url, 
     dst_msg.source.url, 
     "s_url", 
     "a_s_url", 
     'create_preview("")',
     'create_preview("a_")'
    ),
    ("title", 
     src_msg.source.title, 
     dst_msg.source.title, 
     "s_tit", 
     "a_s_tit", 
     'create_preview("")',
     'create_preview("a_")'
    ), 
    ("html_link", 
     src_msg.source.html_link, 
     dst_msg.source.html_link, 
     "s_hln",
     "a_s_hln",
     None,
     None
    ),
    ("context.item", ""),
    ("title", src_item.title, None, "i_tit", None, None, None),
    ("description", 
     src_item.description, 
     None, 
     "i_dsc", 
     None, 
     None, 
     None
    )
  ]

  if len(feeds) > 1:
    editor_row_def.insert(0,
                        ("feed",
                         "<FEED_LIST>",
#                         "<EMPTY>",
                         None,
                         None,
                         None,
                         None,
                         None))    

  prn('\n<table class="options" id="options" cellspacing="0">')

  for a in editor_row_def:

    if len(a) == 2:
      prn('\n<tr valign="top"><td></td><td class="options_title nowrap">', 
          a[0], '</td><td class="options_title nowrap">', a[1], '</td></tr>')
      continue

    prn_message_option(prn,
                       adjustment_result,
                       browser,
                       a[0],
                       a[2] == None,
                       a[1],
                       a[3],
                       a[5])
    
    prn_message_option(prn,
                       adjustment_result,
                       browser,
                       None,
                       False,
                       a[2],
                       a[4],
                       a[6])

  prn('\n</table>')

def prn_adjustment_script_editor(context, 
                                 show, 
                                 script,
                                 feeds,
                                 adjustment_result,
                                 src_item,
                                 src_msg):

  try:
    unicode_script = script.decode("utf8", 'strict')
  except:
    unicode_script = ""  

  if len(unicode_script) > 103:
    unicode_script = unicode_script[0:100] + "..."

  truncated_adjustment_script = unicode_script.encode("utf8")

  if show:
    script_editor_class = ''
    script_viewer_class = ' class="hidden"'
  else:    
    script_editor_class = ' class="hidden"'
    script_viewer_class = ''

  xml_encode_flags = el.string.manip.XE_TEXT_ENCODING | \
                     el.string.manip.XE_PRESERVE_UTF8

  prn = context.request.output.stream.prn

  prn(R'''
    <tr valign="top" id="script_viewer" valign="top"''', script_viewer_class, 
    R''' name="ef_option"><td>Script:</td><td id="script_edit_link"><div id="script_viewer_code">''', 
    el.string.manip.xml_encode(truncated_adjustment_script, 
                               xml_encode_flags).replace("\n", "<br>"),
    R'''</div><div>
    <a href="javascript:show_mas_editor(true)">edit</a></div></td><td></td></tr>

    <tr id="script_editor" valign="top"''', script_editor_class,
    R'''><td class="script_editor_cell"><textarea rows="50"
    name="a" id="a" wrap="off" onfocus="save_focus(this)" 
    onchange="script_changed();">''', 
    el.string.manip.xml_encode(script, xml_encode_flags),
    R'''</textarea><div class="helpers">Helper functions: 
    <a href="javascript:helper_find_in_article();"
    title="Searches in item article document; return empty sequence if document not exist">
    find_in_article</a>&#xA0;&#xB7;
    <a href="javascript:helper_read_image_size();"
    title="Read width and height from image source file, check extension if required">
    read_image_size</a>&#xA0;&#xB7;
    <a href="javascript:helper_reset_image_alt();"
    title="Clear alt for images with (or different from) origin specified">
    reset_image_alt</a>&#xA0;&#xB7;
    <a href="javascript:helper_set_description();"
    title="Resets message description applying restrictions">
    set_description</a>&#xA0;&#xB7;
    <a href="javascript:helper_set_description_from_article();"
    title="Resets message description from article tags">
    set_description_from_article</a>&#xA0;&#xB7;
    <a href="javascript:helper_set_images();"
    title="Read message images from article tags matching xpath expression">
    set_images</a>&#xA0;&#xB7;
    <a href="javascript:helper_set_keywords();"
    title="Read message keywords from article nodes or add specific ones if nodes present">
    set_keywords</a>&#xA0;&#xB7;
    <a href="javascript:helper_set_src_title();"
    title="Sets message source title">
    set source title</a>&#xA0;&#xB7;
    <a href="javascript:helper_skip_image();"
    title="Set newsgate.message.Image.IS_SKIP status for images satisfying condition specified">
    skip_image</a></div><br><input type="button" 
    onclick="show_mas_editor(false)" value="Hide Script Editor"/></td>
    <td class="script_editor_cell">''')

  prn_message_editor(context,
                     feeds,
                     adjustment_result,                     
                     src_item,
                     src_msg)

  prn('</td></tr>')

def fill_src_message_item(context, src_message, src_item):

  param = context.request.input.parameters().find
  
  src_item.title = param("i_tit").strip()
  src_item.description = param("i_dsc").strip()

  src_message.source.title = param("s_tit").strip()
  src_message.source.html_link = param("s_hln").strip()
  
  try:
    src_message.source.url = param("s_url").strip()
  except: pass

  src_message.title = param("tit").strip()
  src_message.description = param("dsc").strip()

  try: src_message.url = param("url").strip()
  except: pass

  src_message.space = space_id(param("spc"))

  try: src_message.lang = el.Lang(param("lng").strip())
  except: pass

  try: src_message.country = el.Country(param("ctr").strip())
  except: pass

  for k in param("kwd").split('\n'):
    k = k.strip()
    if k: src_message.keywords.append(k)

  image = newsgate.message.Image()

  for iml in param("img").split('\n'):
    iml = iml.strip()

    if iml == "" and image.src != "":
      src_message.images.append(image)
      image = newsgate.message.Image()
      continue
    
    iml_pair = iml.split(':', 1)

    try:
      attr = iml_pair[0].strip()
      val = iml_pair[1].strip()

      if attr == "src": image.src = val
      elif attr == "alt": image.alt = val
      elif attr == "width": image.width = long(val)
      elif attr == "height": image.height = long(val)
      elif attr == "origin": image.origin = image_origin_id(val)
      elif attr == "status": image.status = image_status_id(val)
    except: pass

  if image.src != "":
    src_message.images.append(image)

def is_customer(privileges):

  for p in privileges:
    if p.id == newsgate.moderation.moderator.PV_CUSTOMER:
      return True

  return False

def prn_channel_script_editor(context, 
                              show, 
                              script,
                              script_result,
                              feeds):
  try:
    unicode_script = script.decode("utf8", 'strict')
  except:
    unicode_script = ""  

  if len(unicode_script) > 103:
    unicode_script = unicode_script[0:100] + "..."

  truncated_adjustment_script = unicode_script.encode("utf8")

  if show:
    script_editor_class = ''
    script_viewer_class = ' class="hidden"'
  else:    
    script_editor_class = ' class="hidden"'
    script_viewer_class = ''

  xml_encode_flags = el.string.manip.XE_TEXT_ENCODING | \
                     el.string.manip.XE_PRESERVE_UTF8

  prn = context.request.output.stream.prn

  prn(R'''
    <tr valign="top" id="script_viewer" valign="top"''', script_viewer_class, 
    R''' name="ef_option"><td>Script:</td><td id="script_edit_link"><div id="script_viewer_code">''', 
    el.string.manip.xml_encode(truncated_adjustment_script, 
                               xml_encode_flags).replace("\n", "<br>"),
    R'''</div><div>
    <a href="javascript:show_chl_editor(true)">edit</a></div></td><td></td></tr>

    <tr id="script_editor" valign="top"''', script_editor_class,
    R'''><td class="script_editor_cell"><textarea rows="50"
    name="a" id="a" wrap="off" onfocus="save_focus(this)" 
    onchange="script_changed();">''', 
    el.string.manip.xml_encode(script, xml_encode_flags),
    R'''</textarea><div class="helpers">Helper functions:
    <a href="javascript:helper_html_doc();" title="Provides html document for url">
    html_doc</a>&#xA0;&#xB7;
    <a href="javascript:helper_images_from_doc();" title="Updates sequence with images from html document nodes">
    images_from_doc</a>&#xA0;&#xB7;
    <a href="javascript:helper_keywords_from_doc();" title="Updates sequence with keywords from html document nodes">
    keywords_from_doc</a>&#xA0;&#xB7;
    <a href="javascript:helper_new_message();" title="Creates new feed message">
    new_message</a>&#xA0;&#xB7;
    <a href="javascript:helper_text_from_doc();" title="Produces text from html document nodes">
    text_from_doc</a>
</div><br><input type="button" 
    onclick="show_chl_editor(false)" value="Hide Script Editor"/></td>
    <td class="script_editor_cell">''')
  
  prn_message_viewer(context,
                     script_result,
                     feeds)

  prn('</td></tr>')

def prn_message_viewer(context,
                       script_result,
                       feeds):

  user_agent = context.request.input.headers().find("user-agent")
  browser = el.psp.browser(user_agent)

  prn = context.request.output.stream.prn
  seq = el.Sequence()

  editor_row_def = \
  [ 
#    ("item", "<ITEM_LIST>", "<ITEMS_LOAD_STATUS>", None, None, None, None),
    ("item", "<ITEM_LIST>", None, None, None, None, None),
    ("", "<VIEWER_BUTTONS>", None, None, None, None, None),
    ("", "<CHN_ERRORS>", None, None, None, None, None),
    ("", "<CHN_LOG>", None, None, None, None, None),
    ("", "<MSG_ERRORS>", None, None, None, None, None),
    ("", "<MSG_LOG>", None, None, None, None, None),
    ("context.message", ""),
    ("preview", newsgate.message.Message(), None, "msg", "", None, None),
    ("url", 
     "",
     "",
     "url", 
     "", 
     'create_preview("")',
     ''
    ),
    ("title",
     "", 
     "", 
     "tit", 
     "", 
     'create_preview("")',
     ''
     ), 
    ("description", 
     "", 
     "", 
     "dsc", 
     "",
     'create_preview("")',
     ''
    ),
    ("space", 
     "",
     "",
     "spc", 
     "",
     None, 
     None),
    ("language", el.Lang.null, el.Lang.null, "lng", "", None, None),
    ("country", 
     el.Country.null, 
     el.Country.null, 
     "ctr", 
     "",
     'create_preview("")',
     ''
    ),
    ("images", 
     seq, 
     seq, 
     "img", 
     "",
     'create_preview("")',
     ''
    ),
    ("keywords",
     seq,
     seq,
     "kwd",
     "",
     None,
     None
    ),
    ("context.message.source", ""),
    ("url", 
     "",
     "", 
     "s_url", 
     "", 
     'create_preview("")',
     ''
    ),
    ("title", 
     "",
     "", 
     "s_tit",
     "", 
     'create_preview("")',
     ''
    ), 
    ("html_link", 
     "",
     "", 
     "s_hln",
     "",
     None,
     None
    ),
  ]

  if len(feeds) > 1:
    editor_row_def.insert(0,
                        ("feed",
                         "<FEED_LIST>",
#                         "<EMPTY>",
                         None,
                         None,
                         None,
                         None,
                         None))    

  prn('\n<table class="options" id="options" cellspacing="0">')

  for a in editor_row_def:

    if len(a) == 2:
      prn('\n<tr valign="top"><td></td><td class="options_title nowrap">', 
          a[0], '</td><td class="options_title nowrap">', a[1], '</td></tr>')
      continue

    prn_message_option(prn,
                       script_result,
                       browser,
                       a[0],
                       False,
                       a[1],
                       a[3],
                       a[5])

#    if a[2] == "<ITEMS_LOAD_STATUS>":
#      prn_message_option(prn,
#                         script_result,
#                         browser,
#                         None,
#                         False,
#                         a[2],
#                         a[4],
#                         a[6])
    
    prn('</tr>')

  prn('\n</table>')

def editable_word_lists(moderator, category, context):
  prn = context.request.output.stream.prn
  word_lists = set()

  if moderator.has_privilege(\
       newsgate.moderation.moderator.PV_CATEGORY_MANAGER):

    for wl in category.word_lists:
       word_lists.add(wl.name)

    return word_lists

  args = ""
  
  for p in moderator.privileges:
    if p.priv.id == newsgate.moderation.moderator.PV_CATEGORY_EDITOR:
      args = p.priv.args
      break

  expressions = []

  for l1 in args.split('\n'):
    for l2 in l1.split('\r'):
      pattern = l2.strip()
      if pattern == "": continue
  
      try: 
        expressions.append(re.compile(pattern))
      except: pass
      
  for p in category.paths:
    for wl in category.word_lists:
      wl_path = p.path + wl.name
      
      for exp in expressions:
        if exp.match(wl_path) != None:
          word_lists.add(wl.name)
          break

#  prn(len(expressions), ':')
#  for wl in word_lists: prn(" ", wl)

  return word_lists

def category_search_menu_param(cat_manager):

  menu = "mod_menu=" + \
         el.string.manip.mime_url_encode("Categories|View|/psp/category/view")

  if cat_manager:
    menu += el.string.manip.mime_url_encode("|New|/psp/category/update")

  menu += el.string.manip.mime_url_encode("|Search|")
  
  return menu

def category_search_link(cat_manager, category):

  search_url = "/psp/search?mod_init=mod_init&mod_script=" +\
    el.string.manip.mime_url_encode("/fixed/js/category/mod_script.js") +\
    "&mod_css=" + \
    el.string.manip.mime_url_encode("/fixed/css/category/mod_css.css") +\
    "&" + category_search_menu_param(cat_manager)

  if category and category != '/':
    search_url += "&v=C" + el.string.manip.mime_url_encode(category)
  
  return search_url

def feed_search_menu_param(feed_creator, feed_moderator):

  menu = "mod_menu=Feeds"

  if feed_moderator:
    menu += el.string.manip.mime_url_encode("|View|/psp/feed/view")

  if feed_creator:
    menu += el.string.manip.mime_url_encode(\
      "|New|/psp/feed/register|Crawling Tasks|/psp/feed/validations")

  if feed_moderator:
    menu += "|XPath|/psp/feed/xpath"

  menu += el.string.manip.mime_url_encode("|Search|")
  
  return menu

def feed_search_link(feed_creator, feed_moderator, feed):

  search_url = "/psp/search?mod_init=mod_init&mod_script=" +\
    el.string.manip.mime_url_encode("/fixed/js/feed/mod_script.js") +\
    "&" + feed_search_menu_param(feed_creator, feed_moderator)

  if feed:
    search_url += "&v=S" + el.string.manip.mime_url_encode(feed)
  
  return search_url

def message_search_link():

  search_url = "/psp/search?mod_init=mod_init&mod_script=" +\
         el.string.manip.mime_url_encode("/fixed/js/message/mod_script.js") +\
         "&mod_css=" + \
         el.string.manip.mime_url_encode("/fixed/css/message/mod_css.css") + \
         "&mod_menu=" + \
         el.string.manip.mime_url_encode("Messages|Fetch Filter|/psp/message/fetch_filter|Search|")   
  
  return search_url

def table_filter_edit(id, width, colspan=0):
  return '<td style="width:' + width + \
         (colspan and ('" colspan="' + str(colspan)) or '') + \
         '"><input class="text_option" type="text" value="" id="' + id +\
         '" autocorrect="off" autocomplete="off" style="width:' + width + \
         '"></td>'
