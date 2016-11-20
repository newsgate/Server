import el
import newsgate
import search_util
import sys

class InformerRender(search_util.SearchPageContext):

  informer_render_base = search_util.SearchPageContext

  def __init__(this,
               context,
               search_engine,
               protocol,
               read_prefs):
    
    this.informer_render_base.__init__(this,
                                       context,
                                       search_engine,
                                       protocol,
                                       read_prefs)

    conf = context.config.get

    this.version = int(this.raw_param("iv", 0))
    this.informer_create_mode = this.raw_param("ir", "") == "1"

    this.def_informer_type = "j"
    this.informer_type = this.raw_param("it", this.def_informer_type)

    this.def_var_name = "inf_object"
    
    this.var_name = \
      el.string.manip.suppress(this.raw_param("vn", this.def_var_name),
                               " \t\n")
    
    this.def_xml_encoding = 1
    this.xml_encoding = int(this.raw_param("xe", this.def_xml_encoding))

    this.def_image_pos = 1
    this.image_pos = int(this.raw_param("ip", this.def_image_pos))

    this.def_title_length = 50
    this.title_length = int(this.raw_param("tl", this.def_title_length))

    this.def_desc_length = 200
    val = str(this.raw_param("dl", this.def_desc_length))
    if val[0:2] == "s-": val = val[2:]
    this.desc_length = int(val)

    this.def_message_date = 0
    this.message_date = int(this.raw_param("md", this.def_message_date))

    this.def_source_country = 0
    this.source_country = int(this.raw_param("sc", this.def_source_country))

    this.def_message_categories = 1
    
    this.message_categories = \
      int(this.raw_param("mc", this.def_message_categories))

    this.def_msg_more = 1
    this.msg_more = int(this.raw_param("mm", this.def_msg_more))
  
    this.def_similar_messages = 1
    
    this.similar_messages = \
      int(this.raw_param("sm", this.def_similar_messages))
  
    this.img_size = this.raw_param("is",
                                   conf("search.informer.default_thumbnail"))

    if this.img_size[-1].upper() == 'C':
      this.image_cropped = True
      image_dimensions = this.img_size[0:-1].split('x')
    else:
      this.image_cropped = False
      image_dimensions = this.img_size.split('x')

    this.image_width = int(image_dimensions[0])
    this.image_height = int(image_dimensions[1])
    this.image_square = this.image_width * this.image_height    

    this.def_width = "100%"
    this.width = this.raw_param("iw", this.def_width)

    this.def_link_color = ""
    this.link_color = this.raw_param("lc", this.def_link_color)

    this.def_border_color = "#000050"
    this.border_color = this.raw_param("rc", this.def_border_color)

    this.def_background_color = ""
    this.background_color = this.raw_param("gc", this.def_background_color)

    this.def_divider_color = ""
    this.divider_color = this.raw_param("dc", this.def_divider_color)

    this.def_title_color = "black"
    this.title_color = this.raw_param("tc", this.def_title_color)

    this.def_title_underline_color = "#969696"
    this.title_underline_color = this.raw_param("uc", this.def_title_underline_color)

    this.def_title_size = "110%"
    this.title_size = this.raw_param("ts", this.def_title_size)

    this.def_title_font = "Georgia,Palatino,Arial,Times,Times New Roman,serif"
    this.title_font = this.raw_param("tf", this.def_title_font)

    this.def_annotation_color = "#3C3C3C"
    this.annotation_color = this.raw_param("ac", this.def_annotation_color)

    this.def_annotation_size = "95%"
    this.annotation_size = this.raw_param("as", this.def_annotation_size)

    this.def_annotation_font = "Times,Times New Roman,serif";
    this.annotation_font = this.raw_param("af", this.def_annotation_font)

    this.def_annotation_alignment = 0;
    this.annotation_alignment = int(this.raw_param("aa", this.def_annotation_alignment))

    this.def_source_length = 25
    this.source_length = int(this.raw_param("sl", this.def_source_length))

    this.def_metainfo_columns = 2
    this.metainfo_columns = int(this.raw_param("ml", this.def_metainfo_columns))

    this.def_metainfo_color = "black"
    this.metainfo_color = this.raw_param("mr", this.def_metainfo_color)

    this.def_metainfo_size = "65%"
    this.metainfo_size = this.raw_param("ms", this.def_metainfo_size)

    this.def_metainfo_font = "Verdana,Tahoma,Geneva,Helvetica,Arial,Sans-serif"
    this.metainfo_font = this.raw_param("mf", this.def_metainfo_font)

    this.def_catchword_pos = 2
    this.catchword_pos = int(this.raw_param("wp", this.def_catchword_pos))

    this.def_catchword_background_color = "#E9E9F0"

    this.catchword_background_color = \
      this.raw_param("oc", this.def_catchword_background_color)

    this.def_catchword_link_color = "#0000EE"

    this.catchword_link_color = this.raw_param("cl", 
                                      this.def_catchword_link_color)

    this.def_catchword_color = ""
    this.catchword_color = this.raw_param("cc", this.def_catchword_color)

    this.def_catchword_size = "95%"
    this.catchword_size = this.raw_param("cs", this.def_catchword_size)

    this.def_catchword_font = \
      "Georgia,Palatino,Arial,Times,Times New Roman,serif"
    
    this.catchword_font = this.raw_param("cf", this.def_catchword_font)

    this.def_style_placement = 0
    this.style_placement = int(this.raw_param("sp", this.def_style_placement))
    this.external_style = this.style_placement == 1

    this.def_class_prefix = "inf_"
    this.class_prefix = this.raw_param("cp", this.def_class_prefix)

    this.def_ad_tag = "ir"
    this.ad_tag = this.raw_param("at", this.def_ad_tag)    

  def set_catchword_link_text(this):

    loc = this.context.localization.get

    if this.modifier.similar.message_id:
      text = el.string.manip.xml_decode(this.modifier.similar.title)

    elif this.modifier.source:

      if this.search_result != None and len(this.search_result.messages) > 0:

        text = \
         search_util.truncate_text(this.search_result.messages[0].source_title,
                                    25)
      else:
        text = this.modifier.source

    elif this.modifier.category:

      if this.search_result != None:
        text = this.search_result.category_locale.short_title

      if not text:
        text = \
          this.translate_category_name(\
            this.modifier.category).replace("/", " / ")
      
    elif this.modifier.all:
      text = loc("IR_NEWS")

    else:
      text = search_util.truncate_text(this.query, 25)

    this.def_catchword_link_text = text.strip()

    this.catchword_link_text = \
      this.raw_param("ct", this.def_catchword_link_text).strip()

  def search_context(this):    
    ctx = this.informer_render_base.search_context(this)
    ctx.in_2_columns = False
    return ctx

class InformerThumb:

  def __init__(this, thumbnail, adjust_ratio, image):
    this.thumbnail = thumbnail
    this.adjust_ratio = adjust_ratio
    this.image = image

class JSInformerRender(InformerRender):

  js_informer_render_base = InformerRender

  def __init__(this,
               context,
               search_engine,
               protocol,
               read_prefs):

    this.js_informer_render_base.__init__(this,
                                          context,
                                          search_engine,
                                          protocol,
                                          read_prefs)

    this.start_from = 0
    this.informer_create_mode = this.raw_param("ir") == "1"
    this.debug_mode = False

  def search_context(this):
    
    ctx = this.js_informer_render_base.search_context(this)

    ctx.optimize_query = True
    ctx.title_format = 0
    ctx.keywords_format = 0

    ctx.sr_flags = newsgate.search.SearchContext.RF_CATEGORY_STAT

    ctx.gm_flags = newsgate.search.SearchContext.GM_LINK | \
                   newsgate.search.SearchContext.GM_ID

    if this.img_size == '0x0':
      ctx.max_image_count = 0
    elif ctx.max_image_count:
      ctx.max_image_count = 1
      ctx.gm_flags |= newsgate.search.SearchContext.GM_IMG

    if this.title_length:
      ctx.max_title_from_desc_len = 25
      ctx.gm_flags |= newsgate.search.SearchContext.GM_TITLE

    if this.title_length > 0:
      ctx.max_title_len = this.title_length

    if this.desc_length:
      ctx.gm_flags |= newsgate.search.SearchContext.GM_DESC

    if this.desc_length > 0:
      ctx.max_desc_len = this.desc_length

    if this.source_length or this.source_country:
      ctx.gm_flags |= newsgate.search.SearchContext.GM_SOURCE

    if this.img_size != '0x0':
      ctx.gm_flags |= newsgate.search.SearchContext.GM_IMG

    if this.message_date != 0:
      ctx.gm_flags |= newsgate.search.SearchContext.GM_PUB_DATE | \
                      newsgate.search.SearchContext.GM_FETCH_DATE

    if this.message_categories != 0:
      ctx.gm_flags |= newsgate.search.SearchContext.GM_CATEGORIES

    if this.similar_messages != 0:
      ctx.gm_flags |= newsgate.search.SearchContext.GM_EVENT

    if this.context.request.cache_entry != None or this.debug_mode:
      ctx.etag = 0

    ctx.category_locale = this.modifier.category

    return ctx

  def send_headers(this):

    search_result = this.search_result
    request = this.context.request

    etag = str(search_result.etag)
    
    if request.cache_entry != None:

      if search_result.message_load_status == \
        newsgate.search.SearchResult.MLS_LOADING:
        request.cache_entry.timeout(60)

      request.cache_entry.etag(etag)
    
    request.output.send_header("ETag", etag)

    if search_result.nochanges and this.debug_mode == False:
      this.exit(304) # not modified

    if this.debug_mode and request.cache_entry != None:
      request.cache_entry.timeout(0)

    request.output.content_type("application/x-javascript; charset=UTF-8")

  def is_cropped(this, thumb, image_proportion):

    thumb_proportion = float(thumb.width) / float(thumb.height)
    return thumb.cropped or abs(image_proportion - thumb_proportion) > 0.05

  def find_image_thumb(this, msg, crop_flag = True):

    if this.image_square == 0: return None

    thumbnail = None
    image = None
    adjust_ratio = None
    min_square_dif = sys.maxint

    if crop_flag: image_cropped = this.image_cropped
    else: image_cropped = None

    if thumbnail == None:
      for img in msg.images:

        if len(img.thumbs) == 0: continue
        image_proportion = float(img.width) / img.height
    
        # Searching for best match with at least one dimension equal
        for thumb in img.thumbs:
          if (image_cropped == None or \
              this.is_cropped(thumb, image_proportion) == image_cropped) and \
             (thumb.width == this.image_width and \
              thumb.height <= this.image_height or \
              thumb.height == this.image_height and \
              thumb.width <= this.image_width):

            square_dif = this.image_square - thumb.width * thumb.height

            if square_dif < min_square_dif:
              thumbnail = thumb
              image = img
              min_square_dif = square_dif
              if min_square_dif == 0: break

        if min_square_dif == 0: break

    # Searching for best match with one dimensions bigger; 
    # image size to be adjusted
    if thumbnail == None:

      min_ratio = float(sys.maxint)

      for img in msg.images:
         
        image_proportion = float(img.width) / float(img.height)

        for thumb in img.thumbs:
          if (image_cropped == None or \
              this.is_cropped(thumb, image_proportion) == image_cropped) and \
             (thumb.width >= this.image_width or \
              thumb.height >= this.image_height):

            wratio = float(thumb.width) / this.image_width
            hratio = float(thumb.height) / this.image_height
            ratio = max(wratio, hratio)

            if ratio < min_ratio:
              thumbnail = thumb
              image = img
              min_ratio = ratio
              if min_ratio == 1: break
          
        if min_ratio == 1: break

      if thumbnail != None: adjust_ratio = min_ratio

    # Searching for best match with dimensions smaller
    if thumbnail == None:
      for img in msg.images:
         
        image_proportion = float(img.width) / img.height

        for thumb in img.thumbs:
          if (image_cropped == None or \
              this.is_cropped(thumb, image_proportion) == image_cropped) and \
             thumb.width <= this.image_width and \
             thumb.height <= this.image_height:
            square_dif = this.image_square - thumb.width * thumb.height

            if square_dif < min_square_dif:
              thumbnail = thumb
              image = img
              min_square_dif = square_dif
              if min_square_dif == 0: break
          
        if min_square_dif == 0: break

    if thumbnail == None: return None
    return InformerThumb(thumbnail, adjust_ratio, image)
