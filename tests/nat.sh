#!/bin/bash

# product   : NewsGate - news search WEB server
# copyright : Copyright (c) 2005-2016 Karen Arutyunov
# licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
#             Commercial; contact karen.arutyunov@gmail.com

# accept 4 arguments:
# required destination server
# required destination port
# original destination
# original destination port
#
# as an instance:
#
#sudo nat.sh ngdev.ocslab.com 7180 www.newsfiber.com 80
#
# requests to www.newsfiber.com are transparently redirected to ngdev.ocslab.com 

flush() {
  ipfw -f flush
}

#flush firewall rules on interrupt

trap flush SIGINT

dnat_ip=$1
dnat_port=$2
target_ip=$3
target_port=$4
ipfw add 10 check-state
ipfw add 20 allow ip from any to $dnat_ip $dnat_port setup keep-state
ipfw add 30 divert natd ip from any to $target_ip $target_port
ipfw add 40 divert natd ip from  $dnat_ip $dnat_port to any

natd -v -n en1 -reverse -redirect_port tcp $dnat_ip:$dnat_port $target_ip:$target_port
