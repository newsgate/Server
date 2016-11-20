/////////////////////////////////////////////////////////////////////////////
// AddItemDialog
/////////////////////////////////////////////////////////////////////////////

function AddItemDialog(box_id, items, object_type)
{
  this.selected_item = "";
  this.box_id = box_id;
  this.items = items;
  this.object_type = object_type;

  FullScreenDialog.call(this);
}

el_typedef(AddItemDialog, FullScreenDialog,
{
  init: function() 
  { 
    var inner = this.el_call(FullScreenDialog, "init");

    var text = "<div id='items'>Loading ...</div>\
<button onclick='el_close_dlg_by_id(\"" + 
             this.id + "\", false)'>Cancel</button>";

    inner.text = text;
    return inner;
  },

  on_create: function()
  {
    if(this.items)
    {
      this.fill_items();
    }
    else if(this.object_type)
    {
      var request;

      try
      {
        request = new ActiveXObject("Msxml2.XMLHTTP");
      }
      catch(e)
      {
        request = new XMLHttpRequest();
      }

      var data = 
      { req: request,
        dlg:this
      };

      request.onreadystatechange = this.get_items_req_handler(data);
      request.open("GET", "/psp/ad/json?o=" + this.object_type, true);
      request.send("");
    }
  },

  get_items_req_handler: function(data)
  {
    var onready = function()
    {
      if(data.req.readyState == 4)
      {        
        switch(data.req.status)
        {
          case 200:
          {
            data.dlg.read_items(data.req);
            break;
          }
          default:
          {
            break;
          }
        }    
      }
    }

    return onready;
  },

  read_items: function(req)
  {
    if(this.closed)
    {
      return;
    }

    this.items = eval("(" + req.responseText + ")");
    this.fill_items();
  },

  fill_items: function()
  {
    var items = el_by_id(this.box_id).value.split(/[\r\n]/g);
    var cur_items = [];

    for(var i = 0; i < items.length; ++i)
    {
      var val = el_trim(items[i]);

      if(val)
      {
        cur_items.push(val);
      }
    }

    var text = '<table id="itm">';

    for(var i = 0; i < this.items.length; ++i)
    {
      var obj = this.items[i];

      var j = 0;
      for(; j < cur_items.length && cur_items[j] != obj.n; ++j);
      
      if(j < cur_items.length)
      {
        continue;
      }

      text += 
        '<tr class="option_row"><td><a href="javascript:el_dlg_by_id(\''+ 
        this.id + '\').add(\'' + obj.n + '\')">' + obj.n +
        '</a></td><td>' + el_xml_encode(obj.d) + '</td></tr>';
    }

    text += "</table>";

    el_by_id("items").innerHTML = text;
  },

  add: function(name)
  {
    this.selected_item = name;
    el_close_dlg_by_id(this.id, true);
  },

  on_ok: function()
  {
    add_to_text(this.selected_item, this.box_id);
  }

});

var engines = 
[
  { n:"[none]", d:"Not a search engine" },
  { n:"[any]", d:"Any search engine" },
  { n:"google", d:"Google" },
  { n:"yandex", d:"Yandex" },
  { n:"yahoo", d:"Yahoo" },
  { n:"rambler", d:"Rambler" },
  { n:"bing", d:"Bing" },
  { n:"mail.ru", d:"Mail.Ru" },
  { n:"conduit", d:"Conduit" },
  { n:"nigma", d:"Nigma" },
  { n:"ask", d:"Ask" },
  { n:"icq", d:"icq Search" },
  { n:"qip", d:"QIP" },
  { n:"webalta", d:"Webalta" },
  { n:"youtube", d:"YouTube" },
  { n:"avg", d:"AVG" },
  { n:"tut", d:"TUT.BY" }
];

function add_search_engine(id)
{
  var dlg = new AddItemDialog(id, engines, ""); 
}

var crawlers =
[
  { n:"[none]", d:"Not a crawler" },
  { n:"[any]", d:"Any crawler" },
  { n:"googlebot", d:"Google Bot" },
  { n:"adsensebot", d:"Google Media Partners" },
  { n:"yandex", d:"Yandex Crawler" },
  { n:"yadirect", d:"Yandex.Direct Bot" },
  { n:"begun", d:"Begun" },
  { n:"googlewidget", d:"GoogleWidget" },
  { n:"mail.ru", d:"Mail.Ru" },
  { n:"msnbot", d:"Msn Bot" },
  { n:"googlewebpreview", d:"Google Web Preview" },
  { n:"slurp", d:"Yahoo Crawler" },
  { n:"360spider", d:"so.com" },
  { n:"alexa", d:"Alexa" },
  { n:"feedfetcher", d:"Google Feed Fetcher" },
  { n:"googlewltranscoder", d:"Google Wireless Transcoder" },
  { n:"adsbot-google", d:"Google Ads Bot" },
  { n:"jeeves", d:"jeeves.com" },
  { n:"teoma", d:"Ask Jeeves" },
  { n:"altavista", d:"AltaVista" },
  { n:"lycos", d:"Lycos" },
  { n:"rambler", d:"Rambler" },
  { n:"aport", d:"Aport" },
  { n:"webalta", d:"WebAlta" },
  { n:"bazquxbot", d:"BazQux Bot" },
  { n:"docomo", d:"DoCoMo" },
  { n:"socialradar", d:"Social Radar" },
  { n:"integromedb", d:"IntegromeDB" },
  { n:"codegator", d:"CodeGator" },
  { n:"icsbot", d:"ICS Bot" },
  { n:"affectv", d:"Affectv Robot" },
  { n:"theoldreader", d:"TheOldReader" },
  { n:"unwindfetchor", d:"UnwindFetchor" },
  { n:"thumbshots", d:"Thumbshots.RU" },
  { n:"diffbot", d:"Diffbot" },
  { n:"vkshare", d:"VKontakte" },
  { n:"rssreader", d:"RssReader" },
  { n:"pr-cy", d:"PR-CY" },
  { n:"jakarta", d:"Jakarta" },
  { n:"odklbot", d:"OdklBot" },
  { n:"feeddigest", d:"FeedDigest" },
  { n:"baidu", d:"Baidu Spider" },
  { n:"mlbot", d:"MLBot" },
  { n:"voilabot", d:"VoilaBot" },
  { n:"wordpress", d:"WordRress" },
  { n:"sogou", d:"Sogou" },
  { n:"panscient", d:"Panscient" },
  { n:"mj12bot", d:"MJ12bot" },
  { n:"comodo", d:"Comodo" },
  { n:"builtwith", d:"BuiltWith" },
  { n:"ahrefsbot", d:"AhrefsBot" },
  { n:"yodaobot", d:"YodaoBot" },
  { n:"ezooms", d:"Ezooms" },
  { n:"huaweisymspider", d:"Huawei Symantec Spider" },
  { n:"edisterbot", d:"EdisterBot" },
  { n:"dle_spider", d:"DLE Spider" },
  { n:"netcraftsurvagent", d:"NetCraftSurvAgent" },
  { n:"discobot", d:"DiscoBot" },
  { n:"archive", d:"Archive.org Bot" },
  { n:"aboundex", d:"Aboundex" },
  { n:"wbsearch", d:"WbSearch" },
  { n:"turnitinbot", d:"TurnitinBot" },
  { n:"httrack", d:"HT Track" },
  { n:"genieo", d:"Genieo" },
  { n:"indy-app", d:"Indy-based Application" },
  { n:"superfeedr", d:"SuperFeedr" },
  { n:"rssgraffiti", d:"RssGraffiti" },
  { n:"ruby-app", d:"Ruby-written Application" },
  { n:"solomonobot", d:"SolomonoBot" },
  { n:"addthis", d:"addThis" },
  { n:"liveinternet", d:"LiveInternet" },
  { n:"rogerbot", d:"RogerBot" },
  { n:"facebookext", d:"FacebookExternalHit" },
  { n:"nigma", d:"Nigma" },
  { n:"proximic", d:"Proximic" },
  { n:"newsgator", d:"NewsgatorOnline" },
  { n:"python-app", d:"Python-written Application" },
  { n:"acoonbot", d:"AcoonBot" },
  { n:"sistrix", d:"Sistrix" },
  { n:"zend-app", d:"Zend HTTP Client" },
  { n:"panopta", d:"Panopta" },
  { n:"iteco", d:"Iteco" },
  { n:"searchbot", d:"SearchBot" },
  { n:"simplepie", d:"SimplePie" },
  { n:"flipboardproxy", d:"FlipboardProxy" },
  { n:"curl-app", d:"Curl Application" },
  { n:"sputnikbot", d:"SputnikBot" },
  { n:"bazqux", d:"Bazqux" },
  { n:"bitrixsmrss", d:"BitrixsMRSS" },
  { n:"perl-app", d:"Perl-written Application" },
  { n:"admantx", d:"Admantx" },
  { n:"g2reader-bot", d:"G2Reader Bot" },
  { n:"squider", d:"Squider" },
  { n:"taptu", d:"Taptu Downloader" },
  { n:"siteshot", d:"Site Shot" },
  { n:"firephp", d:"FirePhp" },
  { n:"unknown-app", d:"Unknown Application" }
]

var query_types =
[
  { n:"[none]", d:"Not a query" },
  { n:"[any]", d:"Any query" },
  { n:"search", d:"Search query" },
  { n:"event", d:"Event query" },
  { n:"category", d:"Category query" },
  { n:"source", d:"Source query" },
  { n:"message", d:"Message query" }
]

function add_crawler(id)
{
  var dlg = new AddItemDialog(id, crawlers, ""); 
}

function add_query_type(id)
{
  var dlg = new AddItemDialog(id, query_types, ""); 
}

function add_language(id)
{
  var dlg = new AddItemDialog(id, null, "languages"); 
}

function add_country(id)
{
  var dlg = new AddItemDialog(id, null, "countries"); 
}

function add_page_category(categories_selected, update_dialog_holder)
{
  add_to_text(categories_selected[0].path, "pc");
}

function add_page_category_exclusion(categories_selected, update_dialog_holder)
{
  add_to_text(categories_selected[0].path, "pce");
}

function add_category(categories_selected, update_dialog_holder)
{
  add_to_text(categories_selected[0].path, "mc");
}

function add_category_exclusion(categories_selected, update_dialog_holder)
{
  add_to_text(categories_selected[0].path, "mce");
}

function add_to_text(line, id)
{
  var textarea = document.getElementById(id);
  var text = el_trim(textarea.value);

  textarea.value = line + (text == "" ? "" : "\n") + text;
}

function expand_options(name)
{
  var elems = el_by_name(name);

  for(var i = 0; i < elems.length; ++i)
  {
    elems[i].style.display = "table-row";
  }

  el_by_id('e' + name).style.display = "none";
}