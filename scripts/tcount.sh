#!/bin/sh

# product   : NewsGate - news search WEB server
# copyright : Copyright (c) 2005-2016 Karen Arutyunov
# licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
#             Commercial; contact karen.arutyunov@gmail.com

#PORT="7188"
#INTERFACE="lo"

PORT="80"
INTERFACE="eth1"
TMP_FILE="/tmp/tcount.tmp"

if test $PORT -eq 80; then
  FILTER_PORT="http"
else
  FILTER_PORT=$PORT
fi

sudo /usr/sbin/tcpdump -l -i $INTERFACE port $PORT 2>&1 > $TMP_FILE
records=`cat $TMP_FILE | sed -n -e "s%.*$FILTER_PORT > .* \(.*\) .*:.*(\(.*\)) .*%\1 \2%p"`

pflags=1
session=0
session_finished=1
total_inbound=0

for rec in $records; do
  
  if test $pflags -eq 1; then
    pflags=0
    last_flags=$rec
    continue
  fi

  pflags=1

  if test X`echo $last_flags | sed -n -e "s%.*\(S\).*%\1%p"` = "XS"; then

    if test $session_finished -eq 0; then
      echo "Session $session reset, read $session_inbound bytes
"
    fi

    session=`expr $session + 1`
    echo "Session $session started"
    session_inbound=0
    session_finished=0
    echo "  $last_flags $rec"
    continue
  fi
    
  echo "  $last_flags $rec"
  session_inbound=`expr $session_inbound + $rec`
  total_inbound=`expr $total_inbound + $rec`

  if test X`echo $last_flags | sed -n -e "s%.*\(F\).*%\1%p"` = "XF"; then
      echo "Session $session finished, read $session_inbound bytes
"
    session_finished=1
  fi   
  
done

echo "total inbound: $total_inbound
"
