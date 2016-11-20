import weakref

class Interceptor:

  def __init__(this, context, logger, render, interceptor_path):
  
    this.context = context
    this.logger = logger
    this.render = weakref.ref(render)
    
    pos = interceptor_path.rfind('.')
    this.interceptor_path = interceptor_path[0:pos] + ".py"
    
    this.loc = None

  def page_marker(this):
    return None

  def top_bar_menu(this, pos, id):
    return None

  def localization(this, id):

    if this.loc == None:
      this.loc = this.context.get_localization(this.interceptor_path)

    return this.loc.get(id)

  def counters(this, category_path):
    return None

  def footer_anchors(this):
    return None

  def share_page(this):
    return None

  def head(this):
    return None

  def body(this):
    return None

  def adult_content(this):
    return False

  def bastion(this, service):
    return ""
