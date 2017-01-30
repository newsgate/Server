# NewsGate

NewsGate is the news aggregator capable of pulling the data from thousands of 
sources and categorizing it in a number of ways. A comprehensive Web interface
allows end-user to browse the aggregated news items or search for the 
information directly. The XML-based API provides an ability to integrate 
NewsGate server with third-party systems.
 
The live instance of the server is [NewsFiber](http://www.newsfiber.com).

## News sources

NewsGate supports RSS, Atom and RDF formats for news sources. The management
Web interface allows to perform crawling of the specific Web site to find news 
channels URLs of the supported formats. 

It is also possible to collect and categorize (see below) arbitrary information 
from any WEB site page. This can be achieved by adding such a page URL to the 
system and associating it with a custom Python script that parses the page 
represented as a DOM and extracts news messages from it. The development of the 
script can be accomplished in the built-in editor that provides the preview of 
the resulted news items. A Python script can be associated with XML-based news 
feed as well. It can access incoming news items and modify their properties to 
remove some regular junk or irrelevant parts.

If it is not possible to obtain the message country/language properties from 
the feed metadata, this information is calculated heuristically with a high 
degree of accuracy.

## Data processing

NewsGate automatically unites similar news into events, so only the latest 
message of event is displayed to the user by default (while he can browse event
to view all the associated messages chronologically). The dedicated service 
iteratively improves news-by-event distribution in the background.

It is also possible to create the category definition tree using the powerful 
search  language (see below) for such definitions. Incoming news items are 
checked against the categories search expressions and get labeled with matched
ones. The management interface provides all required means for developing such
a categorization. Bootstrapping the category definition with a couple of 
obvious search words you can iteratively widen the coverage (getting hints from 
the system) while keeping the categorization accuracy high enough. On the other 
hand, you can steadily expand the category tree to the hundreds of nodes. Also,
you can edit the specific message category labelling to fix categorization 
mistake (while it always better to do it on the category definition level). 
Note that message categorization is not performed once and for all times. All 
messages get re-categorized (in the background) on every category definition 
change.

Messages get deleted when exceed the configured life limit, or just right after 
being pulled from the source not passing the configured filter. Not surprisingly
such filters are expressed as search expressions. The language is flexible
enough to specify that a message should be deleted before its expiration time
if say it is not viewed or clicked for too long.

## Web interface

NewsGate provides the Web interface to navigate the aggregated news items by
categories, sources, countries, languages, events, matching a specific search 
request or any combination of the above. Besides being able to read the selected
information on the site the user can subscribe to the corresponding RSS channel
or create the email subscription. There is also an online editor for creating 
widgets (of the required style) that display the selected news being embedded 
into a third-party site.
 
The system collects impression and click statistics that is used in the 
sort-by-relevance algorithm to show most popular messages first.

## Search

NewsGate implements the customary 
[Search Expression Language](http://www.newsfiber.com/p/h/s?lang=eng). It allows
to search for news items containing the exact phrase as well as ignoring forms
of words it contains. Such a capability is achieved by means of an extendable 
set of morphological dictionaries. At the moment, the following languages are
supported: English, German, Italian, Portugal, Romanian, Russian, Spanish, 
Turkey. Dictionaries for Chinese, Japanese and Korean languages are supported as 
well, but their usage supposes embedding some third part plugin for news items 
segmentation.

The language allows to search not just by words but also by message metadata, 
such as feed URL, site address, domain, country, language, event, category, 
publication or fetch date.

## Advertising

NewsGate provides advertising support out of the box. You can maintain ad 
creatives, launch advertising campaigns assigning creatives to specific slots on
NewGate WEB pages and specifying impression conditions and caps.

## Software architecture

NewsGate has modular services design and implements features for heavy 
production use:
* Horizontal scaling allows to increase the service performance and capacity 
  by adding nodes.
* High availability is achieved by eliminating single points of failure, so 
  one node failure does not impact the service availability and performance.
* Internal backup/recovery support can be configured according to the required 
  data backup policy.

All services are written in C/C++ and Python, thoroughly tested and optimized.

## Supported OSes

CentOS 6.x, 7.x

## Attributions

This product includes GeoLite data created by MaxMind, available from 

[http://www.maxmind.com](http://www.maxmind.com)
