#!/usr/bin/python

import sys
import subprocess
import re
import urlparse
import socket

usage = "Usage: informers_page.py <host>\n"

ip_blacklist = \
[ '93.95.103.63',
  '89.149.223.80',
  '62.141.94.45',
  '78.108.93.75',
  '213.19.128.72',
  '213.19.128.77',
  '176.103.49.84',
  '78.46.67.226',
  '193.169.86.70'
]

if len(sys.argv) < 2:
  sys.stderr.write("Error: too few arguments.\n" + usage)
  sys.exit(-1)

host = sys.argv[1]

args = 'echo "set NAMES \'utf8\'; select ref_url from StatPageImpression where protocol=\'J\' and ref_site !=\'' + \
       host + '\' group by ref_site order by ref_site;" | ng-mysql --user=root NewsGate'

proc = subprocess.Popen(args, shell = True, stdout = subprocess.PIPE)
output, error = proc.communicate()

if proc.returncode != 0:
  sys.stderr.write(error)
  sys.exit(proc.returncode)

urls = output.split()
p1 = re.compile('"http://.*?' + host.replace(".", "\\.") + '.*(iv=\d+).*?"')
p2 = re.compile('"http://.*?' + host.replace(".", "\\.") + '.*?"')

#urls = [ "ref_page",
#         "http://aleksleon.name/2011/?a=b#aaa",
#         "http://www.aleksleon.name/2011/?a=b#aaa",
#         "http://ess.okis.ru/"
#       ]

host_resolution_failures = 0
ip_blacklistings = 0
request_failures = 0
informer_not_contained = 0
informers_count = 0
url_dups = 0

informers_pages = {}
norm_urls = set()

for url in urls:

  url_elems = urlparse.urlsplit(url)
  endpoint = url_elems[1]

  if endpoint == "":
    print "Not an url: " + url
    host_resolution_failures += 1
    continue
    
  norm_endpoint = endpoint[0:4] == "www." and endpoint[4:] or endpoint
  
  norm_url = url_elems[0] + "://" + norm_endpoint + url_elems[2] + "?" + \
             url_elems[3] + "#" + url_elems[4]

  if norm_url in norm_urls:
    print "Duplicate : " + url
    url_dups += 1
    continue
  
  hostname = unicode(endpoint.split(":")[0],
                     encoding="utf-8").encode('idna')

  try:
    ip = socket.gethostbyname(hostname)
  except:
    print "Failed to resolve '" + hostname + "' for " + url
    host_resolution_failures += 1
    continue

  if ip in ip_blacklist:
    print "Ip " + ip + " blacklisted for " + url
    ip_blacklistings += 1
    continue
  
  print "Requesting: " + url

  args = R'''curl -L --compressed --connect-timeout 30 --speed-limit 1 --speed-time 30 -H "User-Agent:Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10.5; ru; rv:1.9.2.16) Gecko/20110319 Firefox/3.6.16" -H "Accept:text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8" -H "Accept-Language:ru-ru,ru;q=0.8,en-us;q=0.5,en;q=0.3" -H "Accept-Charset:windows-1251,utf-8;q=0.7,*;q=0.7" "''' + \
         url + '" 2>/dev/null'
  
  proc = subprocess.Popen(args, shell = True, stdout = subprocess.PIPE)
  output, error = proc.communicate()

  if proc.returncode != 0:
    print "Failed"
    request_failures += 1
    continue

  norm_urls.add(norm_url)

  print "Parsing"

  m1 = p1.search(output) 
  m2 = p2.search(output)
 
  if m1 == None and m2 == None:
    print "Informer Not Contained"
    informer_not_contained += 1
    continue

  inf_version = 0

  if m1 != None:
    try:
      inf_version = int(m1.group(1).split('=')[1])
    except: pass
  
  print "Informer version: " + str(inf_version)

  if inf_version not in informers_pages:
    informers_pages[inf_version] = []
    
  informers_pages[inf_version].append(url)
  informers_count += 1

file = open("informers.html", "w")

file.write(R'''<html>
<head>
<meta http-equiv="content-type" content="text/html; charset=UTF-8">
</head>
<body>''')

for item in informers_pages.items():
  file.write('\n<p>Version ' + str(item[0]) + ':')

  for url in item[1]:

    u = url.replace("&", "&amp;").replace('"', "&quot;").replace("'", "&apos;").\
            replace("<", "&lt;")
    
    file.write('\n<br><a href="' + u + '">' + u + '</a>')

file.write("\n<p>Statistics:")

file.write("\n<br>&nbsp;&nbsp;Host resolution failures: " + \
           str(host_resolution_failures))

file.write("\n<br>&nbsp;&nbsp;IP Blacklistings: " + str(ip_blacklistings))
file.write("\n<br>&nbsp;&nbsp;URL duplicates: " + str(url_dups))
file.write("\n<br>&nbsp;&nbsp;Request failures: " + str(request_failures))

file.write("\n<br>&nbsp;&nbsp;Informer not contained: " + \
           str(informer_not_contained))

file.write("\n<br>&nbsp;&nbsp;Informers found: " + str(informers_count))
file.write("\n<br>Informers by versions:")

for k in informers_pages:
  file.write("\n<br>&nbsp;&nbsp;&nbsp;&nbsp;V" + str(k) + ": " + \
             str(len(informers_pages[k])))

    
file.write('\n</body>\n</html>')

print "\nStatistics:"
print "  Host resolution failures: " + str(host_resolution_failures)
print "  IP Blacklistings: " + str(ip_blacklistings)
print "  URL duplicates: " + str(url_dups)
print "  Request failures: " + str(request_failures)
print "  Informer not contained: " + str(informer_not_contained)
print "  Informers found: " + str(informers_count)

print "Informers by versions:"

for k in informers_pages:
  print "    V" + str(k) + ": " + str(len(informers_pages[k]))
