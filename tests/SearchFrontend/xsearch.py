#!/usr/bin/python

import subprocess
import sys

def send_request(service, query):
  page = "http://ngdev.ocslab.com:7180" + service + query
  print "Requesting " + page
  
  args = R'''curl -L --compressed --connect-timeout 30 --speed-limit 1 --speed-time 30 "''' + \
         page + '" 2>/dev/null'
  
  proc = subprocess.Popen(args, shell = True, stdout = subprocess.PIPE)
  output, error = proc.communicate()

  if proc.returncode != 0:
    print "Failed"
    sys.exit(1)

  return output

usage = "Usage: xsearch.py\n"

queries = \
[\
  "z=ta&v=Shttp%3A%2F%2Fnewsliga.ru%2Fphp%2Frss__.xml",
  "z=ta&q=ipad",
  "z=abcdefghijk&q=ipad",
  "z=abcdefghijklmnopq&q=ipad",
  "z=abcdefghijklmnopqrstuv&q=ipad",
  "z=abcdefghijklmnopqrstuv&q=ipad&v=Shttp%3A%2F%2Ffeeds2.feedburner.com%2Frosbalt",
  "z=abcdefghijklmnopqrstuv&q=ipad&r=10&p=s&c=2&i=1&e=0&b=3&a=0&n=&y=&se=&v=Shttp%3A%2F%2Ffeeds2.feedburner.com%2Frosbalt",
  "z=abcdefghijklmnopqrstuv&q=ipad&r=10&p=s&c=2&i=1&e=0&b=3&a=0&n=&y=&se=&f=http%3A%2F%2Ffeeds2.feedburner.com%2Frosbalt",
  "z=abcdefghijklmnopqrstuv&v=C%2FIT%2FHardware",
  "z=abcdefghijklmnopqrstuv&g=%2FIT%2FHardware&q=ipad",
  "z=abcdefghijklmnopqrstuv&v=EQvGU6lvq7u4%3D+lSjSb12UCOo%3D",
  "z=abcdefghijklmnopqrstuv&v=EqvGU6lvq7u4%3D+lSjSb12UCOo%3D",
  "z=abcdefghijklmnopqrstuv&v=EQvGU6lvq7u4%3D+LSjSb12UCOo%3D",
  "z=abcdefghijklmnoprstuv&v=EqvGU6lvq7u4%3D+LSjSb12UCOo%3D",
  "z=abcdefghijklmnopqrstuv&q=ipad&h=QvGU6lvq7u4%3D+lSjSb12UCOo%3D",
  "z=abcdefghijklmnopqrstuv&q=ipad&h=qvGU6lvq7u4%3D+lSjSb12UCOo%3D",
  "z=abcdefghijklmnopqrstuv&q=ipad&h=QvGU6lvq7u4%3D+LSjSb12UCOo%3D",
  "z=abcdefghijklmnopqrstuv&q=ipad&a=6&b=3-5&c=1&i=3&j=0&k=2&l=2&m=1&n=rus&o=1.4&p=l&r=20&s=11&se=news&y=RUS"
]

old_service = "/psp/search?TEST=1&t=x&"
new_service = "/p/s/x?TEST=1&"

for q in queries:
  
  new_result = send_request(new_service, q)
  old_result = send_request(old_service, q)

  if new_result != old_result:
    print "Results are different"

    file = open("new_result", "w")
    file.write(new_result)

    file = open("old_result", "w")
    file.write(old_result)
    
    sys.exit(1)

print "Test completed successfully"
